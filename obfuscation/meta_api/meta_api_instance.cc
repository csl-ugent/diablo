#include "meta_api.h"

using namespace std;

static
map<t_uint32, MetaAPI_Instance *> all_instances;

static t_bbl *meta_api_init_function_current_bbl = NULL;
static bool meta_api_init_function_first = true;

static
map<string, MetaAPI_Variable *> NO_LOCALS = map<string, MetaAPI_Variable *>();

struct OneWayQueryFunction {
  t_bbl *decision_block;
  MetaAPI_Effect::Effect evaluated_value;
};
static map<MetaAPI_ActivePredicate *, vector<OneWayQueryFunction>> one_ways;

static
void call_function_implementation(t_cfg *cfg, MetaAPI_Instance *instance, MetaAPI_Function *function, string fn_name, t_bbl *& target, PreparedCallSite& call_site, t_int32& nr_allocated_locals, BblVector& jump_to_return, ConstraintList local_constraints, t_regset& overwritten_registers, bool is_instance_implementation);

static
void call_implementation(vector<MetaAPI_AbstractStmt *> implementation, bool& local_allocated, t_cfg *cfg, MetaAPI_Instance *instance, MetaAPI_Function *function, t_bbl *& target, PreparedCallSite& call_site, t_int32& nr_allocated_locals, BblVector& jump_to_return, ConstraintList local_constraints, t_regset& overwritten_registers, t_int32& stack_slot, bool is_instance_implementation, t_bbl *loop_exit_block);

static
t_section *MetaAPI_CreateSectionForInstancePointer(t_cfg *cfg) {
  return ObjectNewSubsection(CFG_OBJECT(cfg), sizeof(t_uint32), DATA_SECTION);
}

static
void RedirectToReturnSites(BblVector jump_to_return, t_bbl *target) {
  /* jumps to the return block */
  t_cfg *cfg = BBL_CFG(target);

  for (auto bbl : jump_to_return) {
    CFG_DESCRIPTION(cfg)->BblAddJumpInstruction(bbl);
    CfgEdgeCreate(cfg, bbl, target, ET_JUMP);
  }
}

static
t_function *SeparateFunction(PreparedCallSite call_site, string fn_name) {
  /* create a new function */
  ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(call_site.before)) == NULL, ("expected only one outgoing edge @eiB", call_site.before));
  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.after)) != NULL)
    CfgDrawFunctionGraphs(BBL_CFG(call_site.after), "unexpected");
  ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.after)) == NULL, ("expected only one incoming edge @eiB", call_site.after));
  ASSERT(!BBL_INS_FIRST(call_site.before), ("expected empty block @eiB", call_site.before));
  ASSERT(!BBL_INS_FIRST(call_site.after), ("expected empty block @eiB", call_site.after));

  ASSERT(!fn_name.empty(), ("must have function name"));

  BblUnlinkFromFunction(call_site.before);
  t_function *result = FunctionMake(call_site.before, fn_name.c_str(), FT_NORMAL);

  BblUnlinkFromFunction(call_site.after);
  BblInsertInFunction(call_site.after, result);

  /* this function is a meta-API related function */
  FUNCTION_SET_METAAPI_DATA(result, new FunctionMetaApiData());

  /* make edges interprocedural */
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(call_site.before, e)
    EdgeMakeInterprocedural(e);
  BBL_FOREACH_SUCC_EDGE(call_site.after, e)
    EdgeMakeInterprocedural(e);

  return result;
}

static
t_reg GetReturnRegister(t_cfg *cfg) {
  t_reg reg;
  REGSET_FOREACH_REG(CFG_DESCRIPTION(cfg)->return_regs, reg)
    break;
  return reg;
}

static
t_reg GetLinkRegister(t_cfg *cfg) {
  t_reg reg;
  REGSET_FOREACH_REG(CFG_DESCRIPTION(cfg)->link_register, reg)
    break;
  return reg;
}

static
vector<t_reg> GetArgumentRegisters(t_cfg *cfg) {
  vector<t_reg> result;

  /* TODO fixme if need to use floating-point arguments */
  t_reg reg;
  REGSET_FOREACH_REG(RegsetIntersect(CFG_DESCRIPTION(cfg)->argument_regs, CFG_DESCRIPTION(cfg)->int_registers), reg)
    result.push_back(reg);

  return result;
}

static
void MetaAPI_CallFunction(MetaAPI_Instance *instance, MetaAPI_Function *function, PreparedCallSite call_site, t_int32 first_stack_index, t_regset& overwritten_registers, bool is_class) {
  t_cfg_edge *e;
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "calling function '%s' in pseudo-function @F", function->Print().c_str(), BBL_FUNCTION(call_site.before)));

  /* use argument registers to produce values in */
  t_cfg *cfg = BBL_CFG(call_site.before);

  /* collect argument registers */
  vector<t_reg> argument_registers = GetArgumentRegisters(cfg);
  auto argument_it = argument_registers.begin();

  auto next_register = [&argument_it, &argument_registers] () {
    ASSERT(argument_it != argument_registers.end(), ("no registers left"));

    t_reg reg = *argument_it;
    argument_it++;

    return reg;
  };

  /* reset values: we generate new values for this call */
  vector<MetaAPI_Variable *> arguments_to_assign;
  for (MetaAPI_FunctionParameter *param : function->parameters) {
    if (param->variable) {
      param->variable->has_value = false;
      arguments_to_assign.push_back(param->variable);
    }
  }

  /* assign values to each meta-API annotated argument,
   * but don't write them to the data sections */
  MetaAPI_AssignVariableValues(cfg, arguments_to_assign, false);

  t_uint32 nr_stack_arguments = 0;
  t_uint32 nr_argument_registers = argument_registers.size();
  if (function->parameters.size() > nr_argument_registers)
    nr_stack_arguments = function->parameters.size() - nr_argument_registers;

  t_int32 stack_index = -1;
  t_uint32 nr_stack_slots = 0;
  for (MetaAPI_FunctionParameter *param : function->parameters) {
    t_reg reg = argument_registers[0];
    bool on_stack = false;
    if (argument_it == argument_registers.end()) {
      /* produce value on the stack */
      on_stack = true;
      stack_index++;
      nr_stack_slots++;
    }
    else {
      /* produce the value in a register */
      reg = next_register();
    }

    if (!on_stack)
      RegsetSetAddReg(overwritten_registers, reg);

    VERBOSE(meta_api_verbosity, ("producing parameter '%s' in r%d (stack index %d)", param->Print().c_str(), reg, stack_index));

    if (!param->variable) {
      /* this is not a meta-API annotated argumant */
      VERBOSE(meta_api_verbosity, ("not a meta-API annotated argument"));

      /* produce the value if it is defined */
      if (!param->value.IsNull()) {
        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = reg;
        info.in_bbl = call_site.before;
        if (on_stack)
          info.stack_index = stack_index;
        info.first_local_stack_index = nr_stack_arguments;
        info.helper_register = GetLinkRegister(cfg);

        param->value.ProduceValueInRegister(instance, true, &info);
      }
      else {
        /* need to generate a new value for the datatype */
        RelationToOtherVariables no_relations;
        auto x = param->type->GenerateRandomValue(no_relations, meta_api_value_rng);

        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = reg;
        info.in_bbl = call_site.before;
        if (on_stack)
          info.stack_index = stack_index;
        info.first_local_stack_index = nr_stack_arguments;

        x.ProduceValueInRegister(instance, false, &info);
      }
    }
    else {
      /* this is a meta-API annotated argument */
      VERBOSE(meta_api_verbosity, ("meta-API annotated argument"));
      ASSERT(param->variable->has_value, ("variable doesn't have a value '%s'", param->variable->Print().c_str()));

      AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
      info.destination_register = reg;
      info.in_bbl = call_site.before;
      if (on_stack)
        info.stack_index = stack_index;
      info.first_local_stack_index = nr_stack_arguments;

      param->variable->abstract_value.ProduceValueInRegister(instance, false, &info);
    }
  }

  /* reserve stack space if needed */
  ASSERT(nr_stack_slots == nr_stack_arguments, ("%d %d unexpected", nr_stack_slots, nr_stack_arguments));
  if (nr_stack_slots > 0)
    DiabloBrokerCall("MetaAPI_ReserveStackSpace", &call_site, nr_stack_slots);

  /* insert function call */
  string callee_name = function->identifier;
  bool callee_is_external = false;

  t_function *callee = GetFunctionByName(cfg, callee_name.c_str());
  if (!callee && function->can_be_external) {
    callee_name = string("--DYNCALL-HELL--") + callee_name;
    callee_is_external = true;

    callee = GetFunctionByName(cfg, callee_name.c_str());
  }
  ASSERT(callee, ("could not find function '%s'", callee_name.c_str()));

  ASSERT(RegsetCountRegs(FUNCTION_FLOAT_ARG_REGS(callee)) == 0, ("function @F needs float arguments @X", callee, CPREGSET(cfg, FUNCTION_FLOAT_ARG_REGS(callee))));
  ASSERT(RegsetCountRegs(FUNCTION_FLOAT_RET_REGS(callee)) == 0, ("function @F needs float returns @X", callee, CPREGSET(cfg, FUNCTION_FLOAT_RET_REGS(callee))));

  if (function->inlined) {
    ASSERT(!callee_is_external, ("can't inline external function @F", callee));

    /* ideally, the callee should be inlined here
     * for the sake of prototyping, we insert a call here */
    t_ins *ins = CFG_DESCRIPTION(cfg)->BblAddCallInstruction(call_site.before);
    InsAppendToBbl(ins, call_site.before);

    /* fallthrough edge removal */
    CfgEdgeKill(BBL_SUCC_FIRST(call_site.before));

    CfgEdgeCreateCall(cfg, call_site.before, FUNCTION_BBL_FIRST(callee), call_site.after, FunctionGetExitBlock(callee));

    /* TODO: maybe call 'ArmInlineFunAtCallsite' in the future */
  } else {
    /* call instruction */
    t_ins *ins = CFG_DESCRIPTION(cfg)->BblAddCallInstruction(call_site.before);
    InsAppendToBbl(ins, call_site.before);

    /* fallthrough edge removal */
    CfgEdgeKill(BBL_SUCC_FIRST(call_site.before));

    if (callee_is_external)
      CallDynamicSymbol(cfg, const_cast<t_string>(function->identifier.c_str()), T_INS(ins), call_site.before, call_site.after);
    else
      CfgEdgeCreateCall(cfg, call_site.before, FUNCTION_BBL_FIRST(callee), call_site.after, FunctionGetExitBlock(callee));
  }

  BBL_FOREACH_PRED_EDGE(call_site.before, e)
    EdgeMakeInterprocedural(e);
  BBL_FOREACH_SUCC_EDGE(call_site.after, e)
    EdgeMakeInterprocedural(e);

  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->callee_may_change);
  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->callee_may_return);
  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->link_register);
}

static
void MetaAPI_CallNew(PreparedCallSite &call_site, size_t size, t_regset& overwritten_registers) {
  t_cfg *cfg = BBL_CFG(call_site.before);

  string callee_name = string("--DYNCALL-HELL--") + "_Znwj";
  t_function *callee = GetFunctionByName(cfg, callee_name.c_str());
  ASSERT(callee, ("can't find 'new' in the PLT"));

  /* produce size */
  AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
  info.destination_register = GetArgumentRegisters(cfg)[0];

  MetaAPI_Datatype *size_datatype = MetaAPI_GetDatatypePtr("size_t");
  AbstractValue size_value = size_datatype->FromString(cfg, to_string(size));
  info.in_bbl = call_site.before;
  size_value.ProduceValueInRegister(NULL, true, &info);

  /* call instruction */
  t_ins *call_ins = CFG_DESCRIPTION(cfg)->BblAddCallInstruction(call_site.before);
  InsAppendToBbl(call_ins, call_site.before);

  /* fallthrough edge removal */
  CfgEdgeKill(BBL_SUCC_FIRST(call_site.before));

  /* '_Znwj' is the mangled name for 'new' */
  CallDynamicSymbol(cfg, "_Znwj", call_ins, call_site.before, call_site.after);

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(call_site.before, e)
    EdgeMakeInterprocedural(e);
  BBL_FOREACH_SUCC_EDGE(call_site.after, e)
    EdgeMakeInterprocedural(e);

  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->callee_may_change);
  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->callee_may_return);
  RegsetSetUnion(overwritten_registers, CFG_DESCRIPTION(cfg)->link_register);
}

MetaAPI_Instance *MetaAPI_InstantiateDatastructure(t_cfg *cfg, MetaAPI_Datatype *datatype, MetaAPI_Function *constructor, t_bbl *target, string fn_name, t_bbl *& return_site, t_function *& fun, bool sub_instance, function<MetaAPI_Storage()> storage_fn, PreparedCallSite& call_site, bool create_in_init_function, bool call_implementation) {
  static uint32_t instance_uid = 0;

  t_regset overwritten_registers = RegsetNew();

  MetaAPI_Instance *instance = new MetaAPI_Instance();
  instance->uid = instance_uid++;
  instance->datatype = datatype;

  ASSERT(datatype->constructors.size() > 0, ("no constructor for '%s'", datatype->name.c_str()));

  /* random constructor if needed */
  if (!constructor)
    constructor = PickRandomElement(datatype->constructors, meta_api_rng);

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "calling constructor '%s' in", constructor->Print().c_str()));
  if (target)
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "@eiB", target));
  else
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "initialisation routine"));

  /* function call */
  t_regset live_before;

  bool new_function = false;

  if (sub_instance) {
    ASSERT(target, ("need target block for sub instance"));

    call_site = PrepareForCallInsertion(target, true);
    if (!fn_name.empty())
      SeparateFunction(call_site, fn_name);

    live_before = BblRegsLiveBefore(call_site.after);
  }
  else {
    call_site.after = NULL;

    /* create a new function that will be called upon initialisation */
    if (create_in_init_function) {
      if (!meta_api_init_function_current_bbl)
        meta_api_init_function_current_bbl = FUNCTION_BBL_FIRST(meta_api_init_function);

      call_site.before = meta_api_init_function_current_bbl;
      ASSERT(!BblReferedToByExtab(call_site.before), ("from extab! @eiB", call_site.before));
      BblSplitBlock(call_site.before, BBL_INS_FIRST(call_site.before), true);
      fn_name = "__DIABLO_METAAPI_INIT__";

      new_function = true;
    }
    else {
      if (target == NULL) {
        call_site.before = BblNew(cfg);
        FunctionMake(call_site.before, fn_name.c_str(), FT_NORMAL);

        FATAL(("check me"));
        new_function = true;
      }
      else {
        call_site = PrepareForCallInsertion(target, false);

        ASSERT(!fn_name.empty(), ("expected function name"));
        SeparateFunction(call_site, fn_name);

        BBL_SET_OLD_ADDRESS(call_site.before, 0);
      }
    }

    ASSERT(!BblReferedToByExtab(call_site.before), ("from extab! @eiB", call_site.before));
    if (!call_site.after)
      call_site.after = BblSplitBlock(call_site.before, BBL_INS_LAST(call_site.before), false);

    t_function *fun = BBL_FUNCTION(call_site.before);

    /* create a fallthrough edge to the exit block */
    if (!create_in_init_function
        && !BBL_SUCC_FIRST(call_site.after))
      CfgEdgeCreate(cfg, call_site.after, FunctionGetExitBlock(fun), ET_FALLTHROUGH);

    /* keep this function live */
    if ((create_in_init_function && meta_api_init_function_first)
        || !create_in_init_function) {
      BBL_SET_ATTRIB(call_site.before, BBL_ATTRIB(call_site.before) | BBL_FORCE_REACHABLE);
      CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), call_site.before, NULL, FunctionGetExitBlock(fun));

      /* this function is a meta-API related function */
      FUNCTION_SET_METAAPI_DATA(fun, new FunctionMetaApiData());
    }

    live_before = BBL_REGS_USE(CFG_HELL_NODE(cfg));
  }

  t_reg reg;
  REGSET_FOREACH_REG(CFG_DESCRIPTION(cfg)->return_regs, reg)
    break;

  RegsetSetAddReg(overwritten_registers, reg);
  RegsetSetAddReg(overwritten_registers, reg+1);

  /* save result (=pointer to instance) */
  MetaAPI_Storage storage;
  if (sub_instance) {
    storage = storage_fn();

    instance->pointer_section = storage.pointer_section;
    instance->stack_slot = storage.stack_slot;
  }
  else {
    instance->pointer_section = MetaAPI_CreateSectionForInstancePointer(cfg);
    Free(SECTION_NAME(instance->pointer_section));

    string section_name = "meta_instance_" + to_string(instance->uid) + "_pointer";
    SECTION_SET_NAME(instance->pointer_section, StringDup(section_name.c_str()));

    storage.pointer_section = instance->pointer_section;
    storage.uid = instance->uid;
    storage.stack_slot = -1;
  }

  if (call_implementation) {
    if (datatype->is_class) {
      if (!(constructor->flags & MetaAPI_Function::Flag_Implemented)) {
        /* instantiating a class, so call 'new' first */
        MetaAPI_CallNew(call_site, datatype->size, overwritten_registers);
        DiabloBrokerCall("MetaAPI_StoreInstancePointer", &storage, call_site.after, reg, reg+1);
      }

      t_bbl *first_before = call_site.before;
      call_site.before = call_site.after;

      ASSERT(!BblReferedToByExtab(call_site.before), ("from extab! @eiB", call_site.before));
      call_site.after = BblSplitBlock(call_site.before, BBL_INS_LAST(call_site.before), false);
      MetaAPI_CallFunction(instance, constructor, call_site, -1, overwritten_registers, true);

      if (constructor->flags & MetaAPI_Function::Flag_Implemented)
        DiabloBrokerCall("MetaAPI_StoreInstancePointer", &storage, call_site.after, reg, reg+1);

      if (datatype->vtable_symbol)
        DiabloBrokerCall("MetaAPI_WriteVTablePointer", &call_site, datatype->vtable_symbol, &overwritten_registers);

      call_site.before = first_before;
    }
    else {
      /* instantiating a non-class */
      MetaAPI_CallFunction(NULL, constructor, call_site, -1, overwritten_registers, false);
      DiabloBrokerCall("MetaAPI_StoreInstancePointer", &storage, call_site.after, reg, reg+1);
    }
  }

  /* per-instance code generation */
  if (call_implementation) {
    if (datatype->instance_implementation && !sub_instance) {
      t_bbl *old_before = call_site.before;

      /* prepare the call site */
      call_site.before = call_site.after;
      if (BBL_INS_LAST(call_site.before)) {
        ASSERT(!BblReferedToByExtab(call_site.before), ("from extab! @eiB", call_site.before));
        call_site.before = BblSplitBlock(call_site.before, BBL_INS_LAST(call_site.before), false);
      }

      ASSERT(!BblReferedToByExtab(call_site.before), ("from extab! @eiB", call_site.before));
      call_site.after = BblSplitBlock(call_site.before, BBL_INS_LAST(call_site.before), false);

      /* create local variables */
      for (auto l : datatype->instance_implementation_locals) {
        string name = l.first;
        MetaAPI_Variable *var = l.second;

        MetaAPI_Variable *local = MetaAPI_CreateVariable(cfg, var->datatype->name, fn_name, var->identifier, true, var->array_size);
        ASSERT(instance->variables.find(name) == instance->variables.end(), ("already instance-local variable with name '%s'", name.c_str()));
        instance->variables[name] = local;
      }

      MetaAPI_ResetConstraints(datatype->instance_implementation);

      t_bbl *target;
      t_int32 nr_allocated_locals = 0;
      BblVector jump_to_return;
      call_function_implementation(cfg, instance, datatype->instance_implementation, "", target, call_site, nr_allocated_locals, jump_to_return, datatype->instance_implementation->constraints, overwritten_registers, true);
      ASSERT(jump_to_return.size() == 0, ("need to redirect to return sites"));

      /* reserve stack space for local variables, if needed */
      if (nr_allocated_locals > 0)
        DiabloBrokerCall("MetaAPI_ReserveStackSpace", &call_site, nr_allocated_locals);

      call_site.before = old_before;
    }
  }

  if (!sub_instance) {
    /* save the instance for later use */
    all_instances[instance->uid] = instance;

    /* save and restore registers */
    ASSERT(RegsetCountRegs(live_before) > 0, ("no registers live before!"));
    DiabloBrokerCall("MetaAPI_SaveRestoreRegisters", &call_site, &overwritten_registers, &live_before, !meta_api_init_function_first, new_function);
  }

  ASSERT(BBL_SUCC_FIRST(call_site.after), ("no successor edge in @eiB", call_site.after));
  ASSERT_WITH_DOTS((cfg, "fallthrough"), CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(call_site.after)) == NULL, ("expected only one outgoing edge @eiB", call_site.after));

  return_site = NULL;
  if (sub_instance || target)
    return_site = CFG_EDGE_TAIL(BBL_SUCC_FIRST(call_site.after));

  fun = BBL_FUNCTION(call_site.after);

  if (create_in_init_function) {
    meta_api_init_function_first = false;
    // if (!sub_instance) {
    //   PreparedCallSite printf_call_site = PrepareForCallInsertion(call_site.before, true);
    //   DiabloBrokerCall("MetaAPI_PrintInstancePointer", &printf_call_site, instance);
    //   call_site.after = printf_call_site.after;
    // }
    meta_api_init_function_current_bbl = call_site.after;
  }

  return instance;
}

static
MetaAPI_Variable *declare_variable(t_cfg *cfg, MetaAPI_Function *transformer, MetaAPI_VariableDeclarationStmt *S, ConstraintList local_constraints) {
  /* parse if needed */
  if (! transformer->implementation_parsed) {
    S->assigned = MetaAPI_CreateVariable(cfg, S->datatype, transformer->identifier, S->identifier, false, 1);

    switch (S->value.type) {
    case MetaAPI_ImplementationValue::Type::Annotation:
      ASSERT(S->value.value == "generate", ("expected 'generate' annotation"));
      break;

    case MetaAPI_ImplementationValue::Type::Null:
      S->assigned->no_rvalue = true;
      break;

    default:
      FATAL(("unexpected rvalue %d", S->value.type));
    }
  }

  /* look through constraints to associate it with the variable */
  for (MetaAPI_Constraint *c : local_constraints) {
    if (c->v1_type == MetaAPI_Constraint::OperandType::LocalVariable
        && (c->v1_identifier == S->identifier))
      c->v1 = S->assigned;

    if (c->v2_type == MetaAPI_Constraint::OperandType::LocalVariable
        && (c->v2_identifier == S->identifier))
      c->v2 = S->assigned;
  }

  return S->assigned;
}

static
AbstractValue generate_value(t_cfg *cfg, MetaAPI_ImplementationValue arg, map<string, MetaAPI_Variable *> locals, map<string, MetaAPI_Variable *> instance_variables, function<AbstractValue()> others) {
  AbstractValue result;

  if (arg.type == MetaAPI_ImplementationValue::Type::MetaGlobalVariable) {
    MetaAPI_Variable *v = MetaAPI_GetGlobalVariableByName(arg.value);
    result = AbstractValue::CreateVariable(v, arg.modifiers, arg.value2, arg.array_index);
  }
  else if (arg.type == MetaAPI_ImplementationValue::Type::MetaLocalVariable) {
    ASSERT(locals.find(arg.value) != locals.end(), ("local variable %s not defined", arg.value.c_str()));
    MetaAPI_Variable *v = locals[arg.value];
    result = AbstractValue::CreateVariable(v, arg.modifiers, arg.value2, arg.array_index);
  }
  else if (arg.type == MetaAPI_ImplementationValue::Type::MetaInstanceVariable) {
    ASSERT(instance_variables.find(arg.value) != instance_variables.end(), ("instance variable %s not defined", arg.value.c_str()));
    MetaAPI_Variable *v = instance_variables[arg.value];
    result = AbstractValue::CreateVariable(v, arg.modifiers, arg.value2, arg.array_index);
  }
  else if (arg.type == MetaAPI_ImplementationValue::Type::Number) {
    MetaAPI_Datatype *int_dt = MetaAPI_GetDatatypePtr("size_t");
    result = int_dt->FromString(cfg, arg.value);
  }
  else {
    result = others();
  }

  /* optional array index */
  if (arg.array_index)
    result.array_index = new AbstractValue(generate_value(cfg, *(arg.array_index), locals, instance_variables, others));
  else
    result.array_index = NULL;

  return result;
}

static
MetaAPI_Function *function_call(t_cfg *cfg, MetaAPI_Function *transformer, MetaAPI_FunctionCallStmt *S, t_bbl *target, MetaAPI_Instance *instance) {
  auto generate_instance = [instance] (MetaAPI_FunctionParameter *parameter, MetaAPI_ImplementationValue arg) {
    AbstractValue result;

    ASSERT(arg.type == MetaAPI_ImplementationValue::Type::Annotation, ("%s\n%s", arg.Print().c_str(), parameter->Print().c_str()));
    ASSERT(arg.value == "instance", ("unexpected value %s", arg.value.c_str()));

    result = AbstractValue::CreateInstance();
    if (arg.modifiers & MetaAPI_ImplementationValue::MODIFIER_MEMBER) {
      result.assigned.flags |= AbstractValue::Contents::FLAG_MEMBER;
      result.assigned.offset = instance->datatype->MemberOffset(arg.value2);
      ASSERT(result.assigned.offset != -1, ("can't find member '%s' in %s", arg.value2.c_str(), instance->datatype->Print().c_str()));
    }

    return result;
  };

  /* parse if needed */
  if (! transformer->implementation_parsed) {
    /* look up the function declaration */
    MetaAPI_Function *meta_function = MetaAPI_GetFunctionByName(S->callee);

    S->assigned = new MetaAPI_Function();
    S->assigned->type = meta_function->type;
    S->assigned->flags = meta_function->flags;
    S->assigned->on_datatype = meta_function->on_datatype;
    S->assigned->can_be_external = meta_function->can_be_external;

    /* return type and identifier */
    S->assigned->return_type = meta_function->return_type;
    S->assigned->identifier = meta_function->identifier;

    /* parameters */
    auto arg_it = S->values.begin();
    for (auto parameter : meta_function->parameters) {
      /* sanity check in case we found syntax error (missing argument) in pseudo-code */
      ASSERT(arg_it != S->values.end(), ("Syntax error in pseudo-code '%s': missing argument '%s'", S->Print().c_str(), meta_function->Print().c_str()));
      MetaAPI_ImplementationValue arg = *arg_it;

      MetaAPI_FunctionParameter *new_parameter = new MetaAPI_FunctionParameter();
      new_parameter->type = parameter->type;
      new_parameter->identifier = parameter->identifier;

      ASSERT(instance, ("should have instance"));
      new_parameter->value = generate_value(cfg, arg, transformer->implementation_locals, instance->variables, [parameter, arg, new_parameter, cfg, instance, generate_instance] () {
        AbstractValue result;

        switch(parameter->value.type) {
        case AbstractValue::Type::MetaAPI_Instance:
          result = generate_instance(parameter, arg);
          break;

        case AbstractValue::Type::MetaAPI_Null:
          if (arg.type == MetaAPI_ImplementationValue::Type::Annotation) {
            if (arg.value == "generate")
              result = parameter->value;
            else if (arg.value == "instance")
              result = generate_instance(parameter, arg);
            else
              FATAL(("unexpected annotation %s", arg.value.c_str()));
            
          }
          else {
            result = new_parameter->type->FromString(cfg, arg.value);
          }
          break;

        default:
          FATAL(("unhandled %s\n%s", result.Print().c_str(), arg.Print().c_str()));
        }

        return result;
      });

      S->assigned->parameters.push_back(new_parameter);
      arg_it++;
    }

    /* sanity check */
    ASSERT(arg_it == S->values.end(), ("what the f?\n%s\n vs\n%s", S->Print().c_str(), meta_function->Print().c_str()));
  }

  return S->assigned;
}

static
BblVector process_statement(MetaAPI_AbstractStmt *S, bool& local_allocated, t_cfg *cfg, MetaAPI_Function *function, t_int32& stack_slot, t_bbl *& target, MetaAPI_Instance *instance, t_int32& nr_allocated_locals, PreparedCallSite& call_site, BblVector& jump_to_return, ConstraintList local_constraints, t_regset& overwritten_registers, MetaAPI_Variable *var, MetaAPI_ImplementationValue *array_index, MetaAPI_ImplementationValue *var_implementation_value, bool is_instance_implementation, t_bbl *loop_exit_block) {
  BblVector result;

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "process_statement %s", S->Print().c_str()));

  auto create_compare_blocks = [] (t_bbl *bbl, BblVector& result, bool switch_jp_ft) {
    t_cfg *cfg = BBL_CFG(bbl);

    ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)), ("expected only one outgoing edge @eiB", bbl));
    ASSERT(CfgEdgeIsFallThrough(BBL_SUCC_FIRST(bbl)), ("expected outgoing fallthrough edge @eiB", bbl));

    ASSERT(!BblReferedToByExtab(bbl), ("from extab! @eiB", bbl));
    t_bbl *bbl1 = BblSplitBlock(bbl, BBL_INS_LAST(bbl), false);
    if (switch_jp_ft)
      CFG_EDGE_SET_CAT(BBL_PRED_FIRST(bbl1), ET_JUMP);

    t_bbl *bbl2 = BblNew(cfg);
    BblInsertInFunction(bbl2, BBL_FUNCTION(bbl));
    CfgEdgeCreate(cfg, bbl, bbl2, switch_jp_ft ? ET_FALLTHROUGH : ET_JUMP);

    if (switch_jp_ft) {
      result.push_back(bbl2);
      result.push_back(bbl1);
    }
    else {
      result.push_back(bbl1);
      result.push_back(bbl2);
    }
  };

  auto process_rvalue = [function, instance, &overwritten_registers] (MetaAPI_ImplementationValue v, t_bbl *target, t_reg r, std::function<void(AbstractValue&)> modifier, MetaAPI_Variable *var) {
    t_cfg *cfg = BBL_CFG(target);

    switch (v.type) {
    case MetaAPI_ImplementationValue::Type::MetaGlobalVariable:
    case MetaAPI_ImplementationValue::Type::MetaLocalVariable:
    case MetaAPI_ImplementationValue::Type::MetaInstanceVariable:
    case MetaAPI_ImplementationValue::Type::Number: {
      AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
      info.destination_register = r;
      info.in_bbl = target;
      info.helper_register = GetLinkRegister(BBL_CFG(target));

      AbstractValue val = generate_value(cfg, v, function->implementation_locals, instance->variables, [v] () {FATAL(("error %s", v.Print().c_str())); return AbstractValue::CreateNull();});

      if (v.modifiers & MetaAPI_ImplementationValue::MODIFIER_MEMBER) {
        val.assigned.flags |= AbstractValue::Contents::FLAG_MEMBER;
        val.assigned.offset = val.variable->datatype->MemberOffset(v.value2);
        ASSERT(val.assigned.offset != -1, ("can't find member '%s' in %s", v.value2.c_str(), val.variable->datatype->Print().c_str()));
      }

      if (v.modifiers & MetaAPI_ImplementationValue::MODIFIER_REFERENCE)
        val.assigned.flags |= AbstractValue::Contents::FLAG_REFERENCE;

      val.ProduceValueInRegister(instance, true, &info, modifier);
      RegsetSetUnion(overwritten_registers, info.overwritten_registers);
    } break;

    case MetaAPI_ImplementationValue::Type::Annotation: {
      bool handled = false;

      if (v.value == "instance") {
        handled = true;

        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = r;
        info.in_bbl = target;

        AbstractValue val = AbstractValue::CreateInstance();

        if (v.modifiers & MetaAPI_ImplementationValue::MODIFIER_MEMBER) {
          val.assigned.flags |= AbstractValue::Contents::FLAG_MEMBER;
          val.assigned.offset = instance->datatype->MemberOffset(v.value2);
          ASSERT(val.assigned.offset != -1, ("can't find member '%s' in %s", v.value2.c_str(), instance->datatype->Print().c_str()));
        }

        if (v.modifiers & MetaAPI_ImplementationValue::MODIFIER_REFERENCE)
          val.assigned.flags |= AbstractValue::Contents::FLAG_REFERENCE;

        val.ProduceValueInRegister(instance, false, &info);
      }
      
      if (v.value == "generate") {
        handled = true;

        ASSERT(var != NULL, ("need variable to $[generate] value for!"));

        RelationToOtherVariables no_relations;
        AbstractValue val = var->datatype->GenerateRandomValue(no_relations, meta_api_value_rng);

        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = r;
        info.in_bbl = target;

        val.ProduceValueInRegister(instance, false, &info, modifier);
      }

      ASSERT(handled, ("unsupported annotation '%s'", v.value.c_str()));
    } break;

    default:
      FATAL(("unhandled %s", v.Print().c_str()));
    }
  };

  auto get_variable = [instance, function] (MetaAPI_ImplementationValue& value) {
    MetaAPI_Variable *var = NULL;

    string var_name = value.value;

    switch(value.type) {
    case MetaAPI_ImplementationValue::Type::MetaLocalVariable:
      ASSERT(function->implementation_locals.find(var_name) != function->implementation_locals.end(), ("can't find local variable '%s'", var_name.c_str()));
      var = function->implementation_locals[var_name];
      ASSERT(var->section == NULL, ("didn't expect section for local variable %s", var_name.c_str()));
      break;

    case MetaAPI_ImplementationValue::Type::MetaInstanceVariable:
      ASSERT(instance->variables.find(var_name) != instance->variables.end(), ("can't find instance-local variable '%s'", var_name.c_str()));
      var = instance->variables[var_name];
      break;

    case MetaAPI_ImplementationValue::Type::MetaGlobalVariable:
    default:
      FATAL(("unhandled %s", value.Print().c_str()));
    }

    ASSERT(var != NULL, ("can't find variable %s", value.Print().c_str()));
    return var;
  };

  /* assign values to local variables */
  if ((S->type != MetaAPI_AbstractStmt::Type::VariableDeclaration)
      && !local_allocated) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "all local variables have been declared, assigning their values"));

    local_allocated = true;

    /* bring the instance-local variable into scope for calculating constraints */
    for (MetaAPI_Constraint *c : local_constraints) {
      if (c->v1_type == MetaAPI_Constraint::OperandType::InstanceVariable) {
        ASSERT(instance->variables.find(c->v1_identifier) != instance->variables.end(), ("can't find instance variable %s", c->v1_identifier.c_str()));
        c->v1 = instance->variables[c->v1_identifier];
      }

      if (c->v2_type == MetaAPI_Constraint::OperandType::InstanceVariable) {
        ASSERT(instance->variables.find(c->v2_identifier) != instance->variables.end(), ("can't find instance variable %s", c->v2_identifier.c_str()));
        c->v2 = instance->variables[c->v2_identifier];
      }
    }

    MetaAPI_LoadVariableConstraints(cfg, local_constraints);

    vector<MetaAPI_Variable *> locals_to_assign;
    for (auto var_ : function->implementation_locals) {
      MetaAPI_Variable *var = var_.second;

      if (var->no_rvalue)
        /* no value should be assigned */
        continue;

      locals_to_assign.push_back(var);
    }

    MetaAPI_AssignVariableValues(cfg, locals_to_assign, false);

    for (auto var : locals_to_assign) {
      t_reg r;
      REGSET_FOREACH_REG(CFG_DESCRIPTION(cfg)->argument_regs, r)
        break;

      AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
      info.destination_register = r;
      info.in_bbl = target;
      info.stack_index = var->stack_slot;

      var->abstract_value.ProduceValueInRegister(instance, false, &info);
    }
  }

  switch (S->type) {
  case MetaAPI_AbstractStmt::Type::VariableDeclaration: {
    ASSERT(!local_allocated, ("no support for local variables in the middle of the implementation"));

    MetaAPI_Variable *var = declare_variable(cfg, function, dynamic_cast<MetaAPI_VariableDeclarationStmt *>(S), local_constraints);
    ASSERT(var, ("unexpected"));
    ASSERT(function->implementation_locals.find(var->identifier) == function->implementation_locals.end(), ("variable %s already defined", var->Print().c_str()));
    function->implementation_locals[var->identifier] = var;

    /* allocate a stack slot for this variable */
    stack_slot++;
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "allocate local variable %s in slot %d", var->Print().c_str(), stack_slot));
    var->stack_slot = stack_slot;

    nr_allocated_locals++;

    result.push_back(target);
  } break;

  case MetaAPI_AbstractStmt::Type::VariableAssignment: {
    /* for now only support assignment to instance-local variables */
    MetaAPI_VariableAssignmentStmt *S_ = dynamic_cast<MetaAPI_VariableAssignmentStmt *>(S);
    string var_name = S_->target.value;

    MetaAPI_Variable *var;
    switch (S_->target.type) {
    case MetaAPI_ImplementationValue::Type::MetaInstanceVariable:
      ASSERT(instance->variables.find(var_name) != instance->variables.end(), ("can't find instance-local variable '%s'", var_name.c_str()));
      VERBOSE(meta_api_verbosity, ("target is instance variable %s", var_name.c_str()));
      var = instance->variables[var_name];
      break;

    case MetaAPI_ImplementationValue::Type::MetaLocalVariable:
      ASSERT(function->implementation_locals.find(var_name) != function->implementation_locals.end(), ("can't find local variable '%s'", var_name.c_str()));
      VERBOSE(meta_api_verbosity, ("target is local variable %s", var_name.c_str()));
      var = function->implementation_locals[var_name];
      ASSERT(var->section == NULL, ("didn't expect section for local variable %s", var_name.c_str()));
      break;

    default:
      FATAL(("unsupported target %s", S_->target.Print().c_str()));
    }

    if (S_->S) {
      BblVector _result = process_statement(S_->S, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, var, S_->target.array_index, &(S_->target), is_instance_implementation, loop_exit_block);

      result.push_back(_result[0]);
    }
    else
      FATAL(("unsupported %s", S_->Print().c_str()));
  } break;

  case MetaAPI_AbstractStmt::Type::Rvalue: {
    MetaAPI_RvalueStmt *S_ = dynamic_cast<MetaAPI_RvalueStmt *>(S);

    vector<t_reg> regs = GetArgumentRegisters(cfg);

    process_rvalue(S_->value, target, regs[0], nullptr, var);
    RegsetSetAddReg(overwritten_registers, regs[0]);

    if (S_->value.modifiers & MetaAPI_ImplementationValue::MODIFIER_DEREFERENCE)
      DiabloBrokerCall("MetaAPI_Dereference", target, regs[0], regs[0]);

    ASSERT(var, ("expected variable for rvalue statement %s", S_->Print().c_str()));

    if (var_implementation_value->modifiers & MetaAPI_ImplementationValue::MODIFIER_DEREFERENCE) {
      /* dereferenced target */
      ASSERT(!var->section, ("unexpected variable %s", var->Print().c_str()));
      ASSERT(var->stack_slot >= 0, ("invalid stack slot %s", var->Print().c_str()));

      process_rvalue(*var_implementation_value, target, regs[1], nullptr, var);
      DiabloBrokerCall("MetaAPI_Store", target, regs[0], regs[1]);
      RegsetSetAddReg(overwritten_registers, regs[1]);
    }
    else {
      /* regular target */
      if (var->section) {
        DiabloBrokerCall("MetaAPI_ProduceRegValueInSection", target, var->section, array_index, regs[0], regs[1]);
        RegsetSetAddReg(overwritten_registers, regs[1]);
      }
      else
        DiabloBrokerCall("MetaAPI_ProduceValueOnStack", target, regs[0], var->stack_slot, false);
    }

    result.push_back(target);
  } break;

  case MetaAPI_AbstractStmt::Type::FunctionCall: {
    MetaAPI_FunctionCallStmt *S_ = dynamic_cast<MetaAPI_FunctionCallStmt *>(S);
    MetaAPI_Function *fun = function_call(cfg, function, S_, target, instance);
    ASSERT(fun, ("unexpected"));

    if (fun->type == MetaAPI_Function::Type::Constructor) {
      t_bbl *return_site;
      t_function *fun_;
      MetaAPI_InstantiateDatastructure(cfg, fun->on_datatype, fun, target, "", return_site, fun_, true, [var] () {
        MetaAPI_Storage result;
        result.pointer_section = var->section;
        result.uid = var->uid;
        result.stack_slot = var->stack_slot;

        return result;
      }, call_site, false, true);
    }
    else {
      call_site = PrepareForCallInsertion(target, true, false);
      MetaAPI_CallFunction(instance, fun, call_site, stack_slot, overwritten_registers, false);

      if (S_->dereference)
        DiabloBrokerCall("MetaAPI_Dereference", call_site.after, GetReturnRegister(cfg), GetReturnRegister(cfg));

      if (var) {
        if (!var->section) {
          /* variable on stack */
          ASSERT(var->stack_slot >= 0, ("invalid stack slot %s", var->Print().c_str()));
          DiabloBrokerCall("MetaAPI_ProduceValueOnStack", call_site.after, GetReturnRegister(cfg), var->stack_slot, false);
        }
        else {
          /* variable in section */
          DiabloBrokerCall("MetaAPI_ProduceRegValueInSection", call_site.after, var->section, array_index, GetReturnRegister(cfg), GetReturnRegister(cfg)+1);
        }
      }
    }

    result.push_back(call_site.after);
  } break;

  case MetaAPI_AbstractStmt::Type::Expression: {
    MetaAPI_ExpressionStmt *T = dynamic_cast<MetaAPI_ExpressionStmt *>(S);

    vector<t_reg> regs = GetArgumentRegisters(cfg);

    process_rvalue(T->op1, target, regs[0], nullptr, var);
    RegsetSetAddReg(overwritten_registers, regs[0]);

    process_rvalue(T->op2, target, regs[1], nullptr, var);
    RegsetSetAddReg(overwritten_registers, regs[1]);

    DiabloBrokerCall("MetaAPI_DoOperand", target, regs[0], regs[1], T->operand);

    if (var) {
      ASSERT(!var->section, ("unexpected variable %s", var->Print().c_str()));
      ASSERT(var->stack_slot >= 0, ("invalid stack slot %s", var->Print().c_str()));

      DiabloBrokerCall("MetaAPI_ProduceValueOnStack", target, regs[0], var->stack_slot, false);
    }

    result.push_back(target);
  } break;

  case MetaAPI_AbstractStmt::Type::If: {
    MetaAPI_IfStmt *T = dynamic_cast<MetaAPI_IfStmt *>(S);
    BblVector _result = process_statement(T->S, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, var, array_index, var_implementation_value, is_instance_implementation, loop_exit_block);
    ASSERT(_result.size() == 1, ("expected only one result block, got %d", _result.size()));

    if (T->S->type == MetaAPI_AbstractStmt::Type::FunctionCall)
      DiabloBrokerCall("MetaAPI_CompareFunctionResult", _result[0], T->inverted_condition);
    else if (T->S->type == MetaAPI_AbstractStmt::Type::Relation) {
      MetaAPI_RelationStmt *U = dynamic_cast<MetaAPI_RelationStmt *>(T->S);

      vector<t_reg> regs = GetArgumentRegisters(cfg);

      MetaAPI_CompareConfiguration conf;
      conf.reg_lhs = regs[0];
      conf.reg_rhs = regs[1];
      conf.relation = U->rel.value;

      DiabloBrokerCall("MetaAPI_Compare", target, T->inverted_condition, &conf);
    }

    BblVector temp_result;
    create_compare_blocks(_result[0], temp_result, false);

    target = temp_result[1];
    result.push_back(temp_result[0]);

    CfgEdgeCreate(cfg, target, result[0], ET_FALLTHROUGH);

    call_implementation(T->body, local_allocated, cfg, instance, function, target, call_site, nr_allocated_locals, jump_to_return, local_constraints, overwritten_registers, stack_slot, is_instance_implementation, loop_exit_block);

    /* check if the last statement is a RETURN */
    if (T->body.back()->type == MetaAPI_AbstractStmt::Type::Return) {
      jump_to_return.push_back(target);

      ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(target)), ("unexpected @eiB", target));
      CfgEdgeKill(BBL_SUCC_FIRST(target));
    }
    else {
      if (!BBL_INS_LAST(target)
          || !CFG_DESCRIPTION(cfg)->InsIsControlflow(BBL_INS_LAST(target))) {
        /* add a jump instruction to the continuation point after the IF */
        CFG_DESCRIPTION(cfg)->BblAddJumpInstruction(target);
        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(target), ET_JUMP);
      }
    }
  } break;

  case MetaAPI_AbstractStmt::Type::Break: {
    /* jump to the loop exit block */
    ASSERT(loop_exit_block, ("can't BREAK outside loop"));

    /* kill the existing edge */
    ASSERT(BBL_SUCC_FIRST(target), ("expected at least one outgoing edge @eiB", target));
    CfgEdgeKill(BBL_SUCC_FIRST(target));

    /* add a new edge */
    CFG_DESCRIPTION(cfg)->BblAddJumpInstruction(target);
    CfgEdgeCreate(cfg, target, loop_exit_block, ET_JUMP);

    result.push_back(target);
  } break;

  case MetaAPI_AbstractStmt::Type::While: {
    MetaAPI_WhileStmt *T = dynamic_cast<MetaAPI_WhileStmt *>(S);

    /* condition variable */
    MetaAPI_CompareConfiguration conf;
    vector<t_reg> regs = GetArgumentRegisters(cfg);

    ASSERT(!BblReferedToByExtab(target), ("from extab! @eiB", target));
    target = BblSplitBlock(target, BBL_INS_LAST(target), false);

    bool inverted = T->inverted_condition;
    bool infinite_loop = false;
    if (T->S->type == MetaAPI_AbstractStmt::Type::Rvalue) {
      /* TODO: if the rvalue is a constant, special case it when we have an infinite loop */
      MetaAPI_RvalueStmt *U = dynamic_cast<MetaAPI_RvalueStmt *>(T->S);

      if ((U->value.type == MetaAPI_ImplementationValue::Type::Number)
          && (U->value.value == "1"))
        infinite_loop = true;

      process_rvalue(U->value, target, regs[0], nullptr, var);
      RegsetSetAddReg(overwritten_registers, regs[0]);

      conf.reg_lhs = regs[0];
      conf.reg_rhs = REG_NONE;
      conf.imm_rhs = 0;
      conf.relation = MetaAPI_Relation::Type::Ne;
    }
    else if (T->S->type == MetaAPI_AbstractStmt::Type::Relation) {
      MetaAPI_RelationStmt *U = dynamic_cast<MetaAPI_RelationStmt *>(T->S);

      BblVector _result = process_statement(U, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, var, array_index, var_implementation_value, is_instance_implementation, loop_exit_block);

      conf.reg_lhs = regs[0];
      conf.reg_rhs = regs[1];
      conf.relation = U->rel.value;

      inverted = !inverted;
    }

    /* TODO: no compare needed for infinite loops */
    DiabloBrokerCall("MetaAPI_Compare", target, inverted, &conf);

    BblVector _result;
    create_compare_blocks(target, _result, !infinite_loop);

    t_bbl *ft = infinite_loop ? _result[1] : _result[0];
    t_bbl *jp = infinite_loop ? _result[0] : _result[1];

    /* jump path is continuation point */
    result.push_back(jp);

    /* jump back to the beginning */
    ASSERT(!BBL_SUCC_FIRST(ft), ("expected no outgoing edge @eiB", ft));
    CfgEdgeCreate(cfg, ft, target, ET_JUMP);

    CFG_DESCRIPTION(cfg)->BblAddJumpInstruction(ft);

    /* jump path is body of loop */
    ASSERT(T->body.size() > 0, ("expected at least one statement in WHILE body %s", T->Print().c_str()));

    target = jp;

    t_bbl *target = ft;
    ASSERT(!BblReferedToByExtab(target), ("from extab! @eiB", target));
    BblSplitBlock(target, BBL_INS_FIRST(target), true);
    call_implementation(T->body, local_allocated, cfg, instance, function, target, call_site, nr_allocated_locals, jump_to_return, local_constraints, overwritten_registers, stack_slot, is_instance_implementation, result.back());
  } break;

  case MetaAPI_AbstractStmt::Type::Relation: {
    MetaAPI_RelationStmt *T = dynamic_cast<MetaAPI_RelationStmt *>(S);

    vector<t_reg> regs = GetArgumentRegisters(cfg);

    /* first operand */
    process_rvalue(T->op1, target, regs[0], nullptr, var);
    RegsetSetAddReg(overwritten_registers, regs[0]);

    /* second operand */

    /* limited support for MOD operations */
    std::function<void(AbstractValue&)> modifier = nullptr;
    if (T->rel.value == MetaAPI_Relation::Type::Mod) {
      ASSERT(T->op2.type == MetaAPI_ImplementationValue::Type::Number, ("expected rhs of MOD operation to be immediate"));
      t_uint32 value = stoi(T->op2.value);
      ASSERT(value == 2, ("unsupported value %s: %d", T->op2.Print().c_str(), value));

      /* The 'mod' operation is currently implemented as an AND operation in the ARM backend.
       * So, the 'mod-2' operation needs to be an 'and-1' instructions.
       * This modifier function will adapt the generated immediate for that purpose. */
      modifier = [] (AbstractValue& abstract_value) {
        ASSERT(abstract_value.assigned.type == AbstractValue::Type::Integer, ("expected integer, got %d", abstract_value.assigned.type));
        abstract_value.assigned.uint32 -= 1;
      };
    }

    process_rvalue(T->op2, target, regs[1], modifier, var);
    RegsetSetAddReg(overwritten_registers, regs[1]);

    result.push_back(target);
  } break;

  case MetaAPI_AbstractStmt::Type::Return: {
    MetaAPI_ReturnStmt *T = dynamic_cast<MetaAPI_ReturnStmt *>(S);

    if (!T->S) {
      result.push_back(target);
    }
    else {
      switch (T->S->type) {
      case MetaAPI_AbstractStmt::Type::Rvalue: {
        MetaAPI_RvalueStmt *U = dynamic_cast<MetaAPI_RvalueStmt *>(T->S);

        AbstractValue value = function->return_type->FromString(cfg, U->value.value);

        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = GetReturnRegister(cfg);
        info.in_bbl = target;

        value.ProduceValueInRegister(instance, true, &info);

        result.push_back(target);
      } break;

      case MetaAPI_AbstractStmt::Type::FunctionCall: {
        BblVector _result = process_statement(T->S, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, var, array_index, var_implementation_value, is_instance_implementation, loop_exit_block);
        result.push_back(call_site.after);
      } break;

      case MetaAPI_AbstractStmt::Type::Relation: {
        MetaAPI_RelationStmt *U = dynamic_cast<MetaAPI_RelationStmt *>(T->S);

        vector<t_reg> registers = GetArgumentRegisters(cfg);

        ASSERT(instance, ("should have instance"));

        if (U->S1) {
          ASSERT(U->S1->type == MetaAPI_AbstractStmt::Type::FunctionCall, ("expected function call %s", U->S1->Print().c_str()));

          BblVector _result = process_statement(U->S1, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, var, array_index, var_implementation_value, is_instance_implementation, loop_exit_block);
          ASSERT(_result.size() == 1, ("expected only one result block, got %d", _result.size()));

          target = _result[0];
        }
        else
          process_rvalue(U->op1, target, registers[0], nullptr, var);

        process_rvalue(U->op2, target, registers[1], nullptr, var);

        RegsetSetAddReg(overwritten_registers, registers[0]);
        RegsetSetAddReg(overwritten_registers, registers[1]);

        DiabloBrokerCall("MetaAPI_CompareRegisters", target, registers[0], registers[1], U->rel.value);

        BblVector _result;
        create_compare_blocks(target, _result, false);

        AbstractValue::ProduceValueInfo info = AbstractValue::ProduceValueInfo();
        info.destination_register = GetReturnRegister(cfg);

        /* fallthrough block (relation not successful) */
        AbstractValue false_value = function->return_type->False(cfg);
        info.in_bbl = _result[0];
        false_value.ProduceValueInRegister(instance, true, &info);

        /* jump block (relation successful) */
        AbstractValue true_value = function->return_type->True(cfg);
        info.in_bbl = _result[1];
        true_value.ProduceValueInRegister(instance, true, &info);

        jump_to_return.push_back(_result[1]);

        result.push_back(_result[0]);
      } break;

      default:
        FATAL(("unsupported %s", T->Print().c_str()));
      }
    }
  } break;

  default:
    FATAL(("unsupported %s", S->Print().c_str()));
  }

  return result;
}

static
void call_implementation(vector<MetaAPI_AbstractStmt *> implementation, bool& local_allocated, t_cfg *cfg, MetaAPI_Instance *instance, MetaAPI_Function *function, t_bbl *& target, PreparedCallSite& call_site, t_int32& nr_allocated_locals, BblVector& jump_to_return, ConstraintList local_constraints, t_regset& overwritten_registers, t_int32& stack_slot, bool is_instance_implementation, t_bbl *loop_exit_block) {
  for (auto S_it = implementation.begin(); S_it != implementation.end(); S_it++) {
    MetaAPI_AbstractStmt *S = *S_it;
    BblVector result = process_statement(S, local_allocated, cfg, function, stack_slot, target, instance, nr_allocated_locals, call_site, jump_to_return, local_constraints, overwritten_registers, NULL, NULL, NULL, is_instance_implementation, loop_exit_block);

    ASSERT(result.size() == 1, ("expected one result block, got %d", result.size()));
    target = result[0];
  }
}

static
void call_function_implementation(t_cfg *cfg, MetaAPI_Instance *instance, MetaAPI_Function *function, string fn_name, t_bbl *& target, PreparedCallSite& call_site, t_int32& nr_allocated_locals, BblVector& jump_to_return, ConstraintList local_constraints, t_regset& overwritten_registers, bool is_instance_implementation) {
  function->implementation_locals.clear();
  t_int32 stack_slot = -1;
  t_bbl *first_call_site_before = call_site.before;

  target = call_site.before;

  bool local_allocated = false;

  call_implementation(function->implementation, local_allocated, cfg, instance, function, target, call_site, nr_allocated_locals, jump_to_return, local_constraints, overwritten_registers, stack_slot, is_instance_implementation, NULL);

  /* is this the last element in the list? */
  if (BBL_NINS(target) > 0) {
    /* if so, add one last block to contain the (optional) register restore operations */
    ASSERT(!BblReferedToByExtab(target), ("from extab! @eiB", target));
    call_site.after = BblSplitBlock(target, BBL_INS_LAST(target), false);
  }
  else
    call_site.after = target;

  function->implementation_parsed = true;

  call_site.before = first_call_site_before;
}

MetaAPI_TransformResult MetaAPI_TransformDatastructure(MetaAPI_Instance *instance, MetaAPI_Function *transformer, t_bbl *target, size_t setter_index, string fn_name, SetterVerificationInfo& verify_info, bool call_implementation) {
  MetaAPI_TransformResult result;

  t_regset overwritten_registers = RegsetNew();

  t_cfg *cfg = BBL_CFG(target);

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "calling transformer '%s' (configuration %d) in @eiB", transformer->Print().c_str(), setter_index, target));
  ASSERT_WITH_DOTS((cfg, "setter_index"), setter_index < transformer->setter_confs.size(), ("invalid setter index %d (%d setter configurations exist)", setter_index, transformer->setter_confs.size()));

  PreparedCallSite call_site = PrepareForCallInsertion(target, false, false);
  SeparateFunction(call_site, fn_name);
  BBL_SET_OLD_ADDRESS(call_site.before, 0);

  t_regset live_before = BblRegsLiveBefore(call_site.after);

  /* reset constraints */
  MetaAPI_ResetConstraints(transformer);

  t_int32 nr_allocated_locals = 0;
  BblVector jump_to_return;
  if (call_implementation) {
    if (transformer->type == MetaAPI_Function::Type::InlinedTransformer) {
      /* load constraints associated with specified setter configuration */
      ConstraintList local_constraints;
      if (transformer->setter_confs[setter_index].constraint)
        local_constraints.push_back(transformer->setter_confs[setter_index].constraint);

      call_function_implementation(cfg, instance, transformer, fn_name, target, call_site, nr_allocated_locals, jump_to_return, local_constraints, overwritten_registers, false);
    }
    else {
      /* set up the argument constraints */
      if (transformer->setter_confs[setter_index].constraint)
        MetaAPI_LoadArgumentConstraints(transformer, {transformer->setter_confs[setter_index].constraint});

      /* call the function */
      MetaAPI_CallFunction(instance, transformer, call_site, -1, overwritten_registers, false);
    }
  }

  RedirectToReturnSites(jump_to_return, call_site.after);

  /* set metadata */
  FUNCTION_METAAPI_DATA(BBL_FUNCTION(call_site.before))->setter = MetaAPI_Setter{.function = transformer, .configuration = setter_index};

  if (meta_api_debug) {
    PrintfDebugData debug_data;

    /* print debug information */
    static MetaAPI_String *format_string = NULL;
    if (!format_string)
      format_string = MetaAPI_CreateString(CFG_OBJECT(cfg), "set(%s) %s @ 0x%x", true);
    debug_data.format_string = format_string;

    MetaAPI_String *predicate_name_and_value = NULL;
    switch (verify_info.expected) {
    case MetaAPI_Effect::Effect::True:
      if (!verify_info.predicate->embedded_name_true) {
        string pred = verify_info.predicate->Print() + "->" + MetaAPI_Effect::EffectToString(verify_info.expected);;
        verify_info.predicate->embedded_name_true = MetaAPI_CreateString(CFG_OBJECT(cfg), pred);
      }
      predicate_name_and_value = verify_info.predicate->embedded_name_true;
      break;

    case MetaAPI_Effect::Effect::False:
      if (!verify_info.predicate->embedded_name_false) {
        string pred = verify_info.predicate->Print() + "->" + MetaAPI_Effect::EffectToString(verify_info.expected);;
        verify_info.predicate->embedded_name_false = MetaAPI_CreateString(CFG_OBJECT(cfg), pred);
      }
      predicate_name_and_value = verify_info.predicate->embedded_name_false;
      break;

    default:
      FATAL(("unsupported effect"));
    }
    debug_data.args[0] = predicate_name_and_value;

    if (!transformer->embedded_identifier)
      transformer->embedded_identifier = MetaAPI_CreateString(CFG_OBJECT(cfg), transformer->identifier);
    debug_data.args[1] = transformer->embedded_identifier;

    PreparedCallSite printf_call_site = PrepareForCallInsertion(target, true);
    DiabloBrokerCall("MetaAPI_EmitPrintfDebug", &printf_call_site, &debug_data, &overwritten_registers);

    call_site.after = printf_call_site.after;
  }

  /* reserve stack space for local variables, if needed */
  if (nr_allocated_locals > 0)
    DiabloBrokerCall("MetaAPI_ReserveStackSpace", &call_site, nr_allocated_locals);

  /* save and restore registers */
  DiabloBrokerCall("MetaAPI_SaveRestoreRegisters", &call_site, &overwritten_registers, &live_before, false, false);

  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.before)))
    CfgDrawFunctionGraphs(cfg, "error");
  ASSERT(!CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.before)), ("expected one incoming edge @eiB", call_site.before));
  if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(call_site.after)))
    CfgDrawFunctionGraphs(cfg, "error");
  ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(call_site.after)), ("expected one incoming edge @eiB", call_site.after));

  result.function = BBL_FUNCTION(call_site.before);

  ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.before)) == NULL, ("expected only one incoming edge @eiB", call_site.before));
  result.first_edge = BBL_PRED_FIRST(call_site.before);
  result.last_block = call_site.after;

  /* TODO: need to find the cause of this problem.
   * For now, this is the fastest solution, however. */
  t_cfg_edge *succ = BBL_SUCC_FIRST(result.last_block);
  if (!CFG_EDGE_SUCC_NEXT(succ)
      && (CFG_EDGE_CAT(succ) == ET_FALLTHROUGH)) {
    ASSERT(BBL_NINS(CFG_EDGE_TAIL(succ)) == 0, ("error @eiB", CFG_EDGE_TAIL(succ)));
    result.last_block = CFG_EDGE_TAIL(succ);
  }

  MetaAPI_LinkEdges(result.first_edge, BBL_SUCC_FIRST(result.last_block));

  return result;
}

t_bbl *preferred_decision_block = NULL;
MetaAPI_Effect::Effect MetaAPI_PreferToReuseGetter(MetaAPI_ActivePredicate *active_predicate, t_randomnumbergenerator *rng) {
  MetaAPI_Effect::Effect result = MetaAPI_Effect::Effect::Unknown;
  preferred_decision_block = NULL;

  if (one_ways.find(active_predicate) != one_ways.end()) {
    auto& candidates = one_ways[active_predicate];

    auto it = candidates.begin();
    if (candidates.size() > 1)
      advance(it, RNGGenerateWithRange(rng, 0, candidates.size() - 1));

    OneWayQueryFunction x = *it;
    result = MetaAPI_Effect::Invert(x.evaluated_value);
    preferred_decision_block = x.decision_block;

    candidates.erase(it);
    if (candidates.size() == 0)
      one_ways.erase(active_predicate);
  }

  return result;
}

MetaAPI_QueryResult MetaAPI_QueryDatastructure(MetaAPI_ActivePredicate *active_predicate, MetaAPI_Function *getter, t_bbl *target, string fn_name, MetaAPI_Effect::Effect value, bool try_reuse, bool call_implementation) {
  MetaAPI_QueryResult result;

  t_regset overwritten_registers = RegsetNew();

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "calling getter in @eiB: %s", target, getter->Print().c_str()));

  t_cfg *cfg = BBL_CFG(target);
  MetaAPI_Instance *instance = active_predicate->instance;

  /* call the function */
  PreparedCallSite call_site = PrepareForCallInsertion(target);
  t_regset live_before = BblRegsLiveBefore(call_site.after);

  result.two_way = false;
  if (try_reuse) {
    t_bbl *decision_block = preferred_decision_block;
    preferred_decision_block = NULL;

    /* look for existing query functions */
    if ((decision_block == NULL)
        && (one_ways.find(active_predicate) != one_ways.end())) {
      /* look through one way functions */
      auto& candidates = one_ways[active_predicate];

      MetaAPI_Effect::Effect inverse_value = MetaAPI_Effect::Invert(value);

      auto it = candidates.begin();
      for (; it != candidates.end(); it++) {
        OneWayQueryFunction x = *it;
        if (x.evaluated_value == inverse_value) {
          /* found! */
          decision_block = x.decision_block;
          break;
        }
      }

      if (decision_block != NULL) {
        /* remove the candidate */
        candidates.erase(it);

        if (candidates.size() == 0)
          one_ways.erase(active_predicate);
      }
    }

    if (decision_block != NULL) {
      /* we can reuse the query function */
      VERBOSE(meta_api_verbosity, ("reusing %s\n@eiB", active_predicate->Print().c_str(), decision_block));

      /* kill the edge between before and after */
      ASSERT(!CFG_EDGE_CORR(BBL_SUCC_FIRST(call_site.before)), ("didn't expect corresponding edge @E", BBL_SUCC_FIRST(call_site.before)));
      CfgEdgeKill(BBL_SUCC_FIRST(call_site.before));

      /* incoming edge */
      CFG_DESCRIPTION(cfg)->BblAddJumpInstruction(call_site.before);
      t_cfg_edge *in = CfgEdgeCreate(cfg, call_site.before, FUNCTION_BBL_FIRST(BBL_FUNCTION(decision_block)), ET_JUMP);
      EdgeMakeInterprocedural(in);

      /* outgoing edge */
      t_cfg_edge *out;
      BBL_FOREACH_SUCC_EDGE(decision_block, out) {
        if (CfgEdgeIsFake(out))
          break;
      }
      ASSERT(out, ("can't find outgoing fake edge @eiB", decision_block));
      ASSERT(!CfgEdgeIsInterproc(out), ("expected intraproc edge @E", out));

      auto edge_type = CFG_EDGE_CAT(out);
      t_bbl *temp_block = CFG_EDGE_TAIL(out);
      CfgEdgeKill(out);
      t_cfg_edge *e;
      BBL_FOREACH_SUCC_EDGE(temp_block, e)
        CfgEdgeKill(e);
      BblKill(temp_block);

      t_cfg_edge *new_out = CfgEdgeCreate(cfg, decision_block, call_site.after, edge_type);
      EdgeMakeInterprocedural(new_out);

      MetaAPI_LinkEdges(BBL_PRED_FIRST(call_site.before), new_out);

      result.decision_bbl = decision_block;
      result.two_way = true;

      /* update liveness information */
      PreparedCallSite reused_call_site;
      reused_call_site.before = FUNCTION_BBL_FIRST(BBL_FUNCTION(decision_block));
      reused_call_site.after = NULL;
      DiabloBrokerCall("MetaAPI_SaveRestoreRegisters", &reused_call_site, &(FunctionGetMetaApiData(BBL_FUNCTION(decision_block))->overwritten_registers), &live_before, true, false);
    }
  }

  if (!result.two_way) {
    SeparateFunction(call_site, fn_name);
    BBL_SET_OLD_ADDRESS(call_site.before, 0);

    t_int32 nr_allocated_locals = 0;
    BblVector jump_to_return;

    if (call_implementation) {
      if (getter->type == MetaAPI_Function::Type::InlinedGetter) {
        ConstraintList local_constraints;
        call_function_implementation(BBL_CFG(target), instance, getter, fn_name, target, call_site, nr_allocated_locals, jump_to_return, local_constraints, overwritten_registers, false);
      }
      else {
        MetaAPI_CallFunction(instance, getter, call_site, -1, overwritten_registers, false);
      }
    }

    RedirectToReturnSites(jump_to_return, call_site.after);

    FUNCTION_METAAPI_DATA(BBL_FUNCTION(call_site.before))->getter = getter;

    ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(call_site.after)) == NULL, ("expected single outgoing edge @eiB", call_site.after));
    ASSERT(CfgEdgeIsFallThrough(BBL_SUCC_FIRST(call_site.after)), ("expected outgoing fallthrough edge on postcall @eiB", call_site.after));

    /* TODO: need to find the cause of this problem.
     * For now, this is the fastest solution, however. */
    t_cfg_edge *succ = BBL_SUCC_FIRST(call_site.after);
    if (succ
        && !CFG_EDGE_SUCC_NEXT(succ)
        && (CFG_EDGE_CAT(succ) == ET_FALLTHROUGH)) {
      ASSERT(BBL_NINS(CFG_EDGE_TAIL(succ)) == 0, ("error @eiB", CFG_EDGE_TAIL(succ)));
      call_site.after = CFG_EDGE_TAIL(succ);
    }

    /* kill the existing edge */
    result.next_bbl = CFG_EDGE_TAIL(BBL_SUCC_FIRST(call_site.after));

    if (call_implementation) {
      t_cfg_edge *e = BBL_SUCC_FIRST(call_site.after);
      if (CFG_EDGE_CORR(e))
        CfgEdgeKill(CFG_EDGE_CORR(e));
      CfgEdgeKill(e);

      /* add backend-specific compare instructions */
      DiabloBrokerCall("MetaAPI_EvaluateGetterResult", call_site.after, &overwritten_registers);
    }
    result.decision_bbl = call_site.after;

    /* reserve stack space for local variables, if needed */
    if (nr_allocated_locals > 0)
      DiabloBrokerCall("MetaAPI_ReserveStackSpace", &call_site, nr_allocated_locals);

    /* save and restore registers */
    DiabloBrokerCall("MetaAPI_SaveRestoreRegisters", &call_site, &overwritten_registers, &live_before, false, false);

    /* for now, assume that the branch is TAKEN when the result is FALSE */

    ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(call_site.before)) == NULL, ("expected only one incoming edge @eiB", call_site.before));
    result.first_edge = BBL_PRED_FIRST(call_site.before);

    /* record */
    OneWayQueryFunction one_way = OneWayQueryFunction();
    one_way.decision_block = result.decision_bbl;
    one_way.evaluated_value = value;

    if (one_ways.find(active_predicate) == one_ways.end())
      one_ways[active_predicate] = vector<OneWayQueryFunction>();
    one_ways[active_predicate].push_back(one_way);
  }

  return result;
}
