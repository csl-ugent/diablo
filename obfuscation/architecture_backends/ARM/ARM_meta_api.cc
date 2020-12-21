#include "ARM_obfuscations.h"
#include "ARM_meta_api_transformation.h"

#include <vector>

using namespace std;

#define INCLUDE_LIVENESS
#define INCLUDE_LIVENESS_FLOAT
// #define ALL_REGISTERS_LIVE

void MetaAPI_ProduceValueInRegister(MetaAPI_Instance *instance, AbstractValue *abstract_value, AbstractValue::ProduceValueInfo *info) {
  t_arm_ins *ins;
  t_arm_ins *ins2;
  bool is_thumb = ArmBblIsThumb(info->in_bbl);
  bool on_stack = info->stack_index >= 0;

  switch (abstract_value->type) {
    case AbstractValue::Type::Integer: {
      VERBOSE(meta_api_verbosity, ("producing integer"));

      t_uint32 value = abstract_value->assigned.uint32;
      VERBOSE(meta_api_verbosity, ("  = %d (0x%x)", value, value));

      if (on_stack)
        ArmMakeInsForBbl(ConstantProducer, Prepend, ins, info->in_bbl, is_thumb, info->destination_register, value);
      else
        ArmMakeInsForBbl(ConstantProducer, Append, ins, info->in_bbl, is_thumb, info->destination_register, value);
    } break;

    case AbstractValue::Type::FunctionPointer: {
      VERBOSE(meta_api_verbosity, ("producing function pointer"));

      t_function *target = abstract_value->assigned.function;
      VERBOSE(meta_api_verbosity, ("  -> @F", target));

      /* produce the address of the function to point to */
      if (on_stack)
        ArmMakeInsForBbl(Mov, Prepend, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      else
        ArmMakeInsForBbl(Mov, Append, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);

      t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(info->in_bbl))),
        AddressNew32(0),
        T_RELOCATABLE(ins), AddressNew32(0),
        T_RELOCATABLE(FUNCTION_BBL_FIRST(target)), AddressNew32(0),
        FALSE, NULL, NULL, NULL,
        "R00A00+" "\\" WRITE_32);

      ArmInsMakeAddressProducer(ins, 0, reloc);
    } break;

    case AbstractValue::Type::MetaAPI_Instance: {
      VERBOSE(meta_api_verbosity, ("producing instance pointer"));

      /* produce the address of the data section
       * that contains the instance pointer */
      if (on_stack)
        ArmMakeInsForBbl(Mov, Prepend, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      else
        ArmMakeInsForBbl(Mov, Append, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);

      t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(info->in_bbl))),
        AddressNew32(0),
        T_RELOCATABLE(ins2), AddressNew32(0),
        T_RELOCATABLE(instance->pointer_section), AddressNew32(0),
        FALSE, NULL, NULL, NULL,
        "R00A00+" "\\" WRITE_32);

      ArmInsMakeAddressProducer(ins2, instance->uid, reloc);

      /* load the instance pointer */
      ArmMakeInsForIns(Ldr, After, ins, ins2, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, FALSE, FALSE);

      if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_MEMBER) {
        t_arm_ins *ins3;
        ASSERT(abstract_value->assigned.offset != -1, ("invalid member offset -1"));

        if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_REFERENCE) {
          /* produce address of members */
          ArmMakeInsForIns(Add, After, ins3, ins, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, abstract_value->assigned.offset, ARM_CONDITION_AL);
        }
        else {
          /* load member */
          ArmMakeInsForIns(Ldr, After, ins3, ins, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, abstract_value->assigned.offset, ARM_CONDITION_AL, TRUE, TRUE, FALSE);
        }
        ins = ins3;
      }
    } break;

    case AbstractValue::Type::Register: {
      VERBOSE(meta_api_verbosity, ("producing register"));

      t_uint32 value = abstract_value->assigned.uint32;
      VERBOSE(meta_api_verbosity, ("  = r%d", value));

      ArmMakeInsForBbl(Mov, Append, ins, info->in_bbl, false, info->destination_register, value, 0, ARM_CONDITION_AL);
    } break;

    case AbstractValue::Type::String: {
      VERBOSE(meta_api_verbosity, ("producing string"));

      MetaAPI_String *value = AbstractValue::unpack<MetaAPI_String>(abstract_value->assigned.generic);
      VERBOSE(meta_api_verbosity, ("  = %s", value->Get().c_str()));

      /* produce the address of the string */
      if (on_stack)
        ArmMakeInsForBbl(Mov, Prepend, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      else
        ArmMakeInsForBbl(Mov, Append, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);

      t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(info->in_bbl))),
        AddressNew32(0),
        T_RELOCATABLE(ins), AddressNew32(0),
        T_RELOCATABLE(value->section), value->offset,
        FALSE, NULL, NULL, NULL,
        "R00A00+" "\\" WRITE_32);

      ArmInsMakeAddressProducer(ins, AddressAdd(SECTION_OLD_ADDRESS(value->section), value->offset), reloc);
    } break;

    case AbstractValue::Type::MetaAPI_Variable: {
      /* address producer */
      VERBOSE(meta_api_verbosity, ("producing variable"));

      MetaAPI_Variable *var = AbstractValue::unpack<MetaAPI_Variable>(abstract_value->assigned.generic);
      VERBOSE(meta_api_verbosity, ("  = %s", var->Print().c_str()));

      t_uint32 array_index = 0;
      t_reg array_index_register = ARM_REG_NONE;

      AbstractValue *aiv = abstract_value->array_index;
      if (aiv) {
        if (aiv->type == AbstractValue::Type::None) {
          /* do nothing */
        }
        else if (aiv->type == AbstractValue::Type::Integer) {
          AbstractValue::Contents c = aiv->Generate();
          array_index = c.uint32;
        }
        else if (aiv->type == AbstractValue::Type::MetaAPI_Variable) {
          ASSERT(info->helper_register != REG_NONE, ("helper register needed! %s", abstract_value->assigned.array_index->Print().c_str()));
          array_index_register = info->helper_register;
        }
        else
          FATAL(("unsupported %s", aiv->Print().c_str()));
      }

      if (var->stack_slot >= 0) {
        ASSERT(array_index == 0, ("implement me %d", array_index));
        VERBOSE(meta_api_verbosity, ("in stack slot %d", var->stack_slot));

        if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_REFERENCE) {
          if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_MEMBER) {
            /*if (on_stack)
              ArmMakeInsForBbl(Ldr, Prepend, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL, true, true, false);
            else*/
              ArmMakeInsForBbl(Ldr, Append, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL, true, true, false);

            ArmMakeInsForIns(Add, After, ins, ins2, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, abstract_value->assigned.offset, ARM_CONDITION_AL);
          }
          else {
            /* reference */
            if (on_stack)
              ArmMakeInsForBbl(Add, Prepend, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL);
            else
              ArmMakeInsForBbl(Add, Append, ins, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL);
          }
        }
        else {
          if (on_stack)
            ArmMakeInsForBbl(Ldr, Prepend, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL, true, true, false);
          else
            ArmMakeInsForBbl(Ldr, Append, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_SP, ARM_REG_NONE, (info->first_local_stack_index + var->stack_slot)*4, ARM_CONDITION_AL, true, true, false);

          if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_MEMBER)
            ArmMakeInsForIns(Ldr, After, ins, ins2, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, abstract_value->assigned.offset, ARM_CONDITION_AL, true, true, false);
          else
            ins = ins2;
        }
      }
      else {
        VERBOSE(meta_api_verbosity, ("from section"));

        /*if (on_stack) {
          ASSERT(array_index == 0, ("implement me %d", array_index));
          ArmMakeInsForBbl(Mov, Prepend, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);
        }
        else*/
          ArmMakeInsForBbl(Mov, Append, ins2, info->in_bbl, is_thumb, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);

        t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(info->in_bbl))),
          AddressNew32(0),
          T_RELOCATABLE(ins2), AddressNew32(0),
          T_RELOCATABLE(var->section), AddressNew32(array_index*4),
          FALSE, NULL, NULL, NULL,
          "R00A00+" "\\" WRITE_32);

        ArmInsMakeAddressProducer(ins2, SECTION_OLD_ADDRESS(var->section), reloc);

        /* register-based array indexing, if needed */
        if (array_index_register != ARM_REG_NONE) {
          /* produce value in register */
          AbstractValue::ProduceValueInfo sub_info = AbstractValue::ProduceValueInfo();
          sub_info.destination_register = array_index_register;
          sub_info.in_bbl = info->in_bbl;
          aiv->ProduceValueInRegister(instance, true, &sub_info);
          RegsetSetAddReg(info->overwritten_registers, array_index_register);

          /* add produced value to generated address */
          ArmMakeInsForBbl(Add, Append, ins2, info->in_bbl, is_thumb, info->destination_register, info->destination_register, array_index_register, 0, ARM_CONDITION_AL);
          ARM_INS_SET_SHIFTTYPE(ins2, ARM_SHIFT_TYPE_LSL_IMM);
          ARM_INS_SET_SHIFTLENGTH(ins2, 2);
        }

        if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_REFERENCE) {
          /* we only need the reference to this variable */
          ins = ins2;
        }
        else {
          /* load the value */
          ArmMakeInsForIns(Ldr, After, ins, ins2, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);

          if (abstract_value->assigned.flags & AbstractValue::Contents::FLAG_MEMBER) {
            t_arm_ins *ins3;
            ArmMakeInsForIns(Ldr, After, ins3, ins, is_thumb, info->destination_register, info->destination_register, ARM_REG_NONE, abstract_value->assigned.offset, ARM_CONDITION_AL, true, true, false);
            ins = ins3;
          }
        }
      }
    } break;

    case AbstractValue::Type::Void: {
      VERBOSE(meta_api_verbosity, ("producing void"));

      void *x = abstract_value->assigned.generic;
      ASSERT(x == NULL, ("unexpected non-NULL"));

      if (on_stack)
        ArmMakeInsForBbl(ConstantProducer, Prepend, ins, info->in_bbl, is_thumb, info->destination_register, 0);
      else
        ArmMakeInsForBbl(ConstantProducer, Append, ins, info->in_bbl, is_thumb, info->destination_register, 0);
    } break;

    default:
      FATAL(("producer for '%s' not supported", abstract_value->Print().c_str()));
  }

  if (on_stack) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "store in stack slot %d", info->stack_index));
    ArmMakeInsForIns(Str, After, ins2, ins, is_thumb, info->destination_register, ARM_REG_R13, ARM_REG_NONE, info->stack_index*4, ARM_CONDITION_AL, true, true, false);
  }
}

void MetaAPI_ProduceValueOnStack(t_bbl *bbl, t_reg from, t_int32 stack_slot, bool prepend) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "produce r%d in stack slot %d", from, stack_slot));

  t_arm_ins *ins;

  if (prepend)
    ArmMakeInsForBbl(Str, Prepend, ins, bbl, false, from, ARM_REG_R13, ARM_REG_NONE, stack_slot*4, ARM_CONDITION_AL, true, true, false);
  else
    ArmMakeInsForBbl(Str, Append, ins, bbl, false, from, ARM_REG_R13, ARM_REG_NONE, stack_slot*4, ARM_CONDITION_AL, true, true, false);
}

void MetaAPI_ProduceRegValueInSection(t_bbl *target, t_section *sec, MetaAPI_ImplementationValue *array_index, t_reg reg, t_reg tmp) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "produce r%d in section @T", reg, sec));

  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  /* produce the address of the section in which to store the register value */
  ArmMakeInsForBbl(Mov, Append, ins, target, is_thumb, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  t_uint32 offset = 0;
  if (array_index) {
    ASSERT(array_index->GetNumber(offset), ("unexpected %s", array_index->Print().c_str()));

    /* here wa assume that arrays always have 4-byte elements */
    offset *= 4;
  }

  t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(target))),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(sec), AddressNew32(offset),
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);

  ArmInsMakeAddressProducer(ins, 0, reloc);

  ArmMakeInsForBbl(Str, Append, ins, target, is_thumb, reg, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, TRUE, FALSE);
}

void MetaAPI_ProduceValueInSection(t_cfg *cfg, AbstractValue *abstract_value, t_section *section) {
  switch (abstract_value->type) {
  case AbstractValue::Type::String: {
    MetaAPI_String *str = AbstractValue::unpack<MetaAPI_String>(abstract_value->assigned.generic);
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  string '%s'", str->Print().c_str()));

    t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0),
      T_RELOCATABLE(section), AddressNew32(0),
      T_RELOCATABLE(str->section), str->offset,
      FALSE, NULL, NULL, NULL,
      "R00A00+" "\\" WRITE_32);
  } break;

  case AbstractValue::Type::Integer: {
    t_uint32 value = abstract_value->assigned.uint32;
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  integer %u", value));

    ASSERT(AddressIsEq(SECTION_CSIZE(section), AddressNew32(4)), ("expected section of size 4 @T", section));

    t_uint32 *data = reinterpret_cast<t_uint32 *>(SECTION_DATA(section));
    data[0] = value;
  } break;

  default:
    FATAL(("unsupported variable type '%s'", abstract_value->Print().c_str()));
  }
}

void MetaAPI_StoreInstancePointer(MetaAPI_Storage *storage, t_bbl *target, t_reg from, t_reg tmp) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  if (storage->stack_slot == -1) {
    ArmMakeInsForBbl(Str, Prepend, ins, target, is_thumb, from, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, TRUE, FALSE);

    /* produce the address of the data section in which to store the instance pointer */
    ArmMakeInsForBbl(Mov, Prepend, ins, target, is_thumb, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL);

    t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(target))),
      AddressNew32(0),
      T_RELOCATABLE(ins), AddressNew32(0),
      T_RELOCATABLE(storage->pointer_section), AddressNew32(0),
      FALSE, NULL, NULL, NULL,
      "R00A00+" "\\" WRITE_32);

    ArmInsMakeAddressProducer(ins, storage->uid, reloc);
  }
  else {
    MetaAPI_ProduceValueOnStack(target, from, storage->stack_slot, true);
  }
}

/* taken path is FALSE */
void MetaAPI_EvaluateGetterResult(t_bbl *target, t_regset *overwritten_registers) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  ArmMakeInsForBbl(Cmp, Append, ins, target, is_thumb, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, target, is_thumb, ARM_CONDITION_EQ);

  RegsetSetAddReg(*overwritten_registers, ARM_REG_R0);
}

void MetaAPI_GetRandomRegistersBroker(t_regset *regs) {
  *regs = RegsetNew();

  for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
    RegsetSetAddReg(*regs, r);
  }
}

void MetaAPI_ActuallySaveRestoreRegisters(t_function *f) {
  t_cfg *cfg = FUNCTION_CFG(f);
  t_architecture_description *description = CFG_DESCRIPTION(cfg);

  ASSERT(FunctionIsMeta(f), ("not a meta function @F", f));
  FunctionMetaApiData *d = FunctionGetMetaApiData(f);

  t_regset saved_registers = d->saved_registers;
  if (d->from_hell) {
    /* add the link register */
    RegsetSetAddReg(saved_registers, ARM_REG_R14);
  }

  t_arm_ins *push_location = T_ARM_INS(d->push_location);
  ASSERT(push_location, ("no push location in @F", f));

  t_arm_ins *pop_location = T_ARM_INS(d->pop_location);
  ASSERT(pop_location, ("no pop location in @F", f));

  bool is_thumb = false;

  auto reserve_push = [&push_location, &is_thumb] () {
    t_arm_ins *tmp;
    ArmMakeInsForIns(Noop, After, tmp, push_location, is_thumb);
    push_location = tmp;
  };

  auto reserve_pop = [&pop_location, &is_thumb] () {
    t_arm_ins *tmp;
    ArmMakeInsForIns(Noop, Before, tmp, pop_location, is_thumb);
    pop_location = tmp;
  };

  /* integer registers */
  t_regset int_regs = RegsetIntersect(d->saved_registers, description->int_registers);

  t_reg gpreg = ARM_REG_NONE;
  t_reg r;
  REGSET_FOREACH_REG(int_regs, r) {
    gpreg = r;
    break;
  }
  if ((gpreg == ARM_REG_NONE)
      && RegsetIn(d->saved_registers, ARM_REG_FPSCR)) {
    DEBUG(("DBGINT need at least one integer register, saving r0"));
    gpreg = ARM_REG_R0;
    RegsetSetAddReg(int_regs, gpreg);
  }

  if (!RegsetIsEmpty(int_regs)) {
    DEBUG(("DBGINT save/restore @X", CPREGSET(cfg, int_regs)));

    ArmInsMakePush(push_location, RegsetToUint32(int_regs), ARM_CONDITION_AL, is_thumb);
    reserve_push();

    t_regset int_regs_popped = int_regs;
    if (d->from_hell) {
      if (!(meta_api_testing && (meta_api_project_name == "radare2")) && meta_api_impl_inst) {
        /* replace the link register with the program counter */
        RegsetSetSubReg(int_regs_popped, ARM_REG_R14);
        RegsetSetAddReg(int_regs_popped, ARM_REG_R15);
      }
    }

    ArmInsMakePop(pop_location, RegsetToUint32(int_regs_popped), ARM_CONDITION_AL, is_thumb);
    reserve_pop();
  }

  /* floating-point registers */
  t_regset flt_regs = RegsetIntersect(d->saved_registers, description->flt_registers);
  {
    /* As the floating-point registers need to be saved/restored with VPUSH/VPOP instructions,
     * and as these instructions need ranges of registers, we collect the separate register ranges
     * first and process them all later. */
    struct reg_range {
      t_reg a, b;
    };
    vector<reg_range> ranges;

    t_reg reg;
    REGSET_FOREACH_REG(flt_regs, reg) {
      if (ranges.size() == 0)
        ranges.push_back(reg_range{reg, reg});
      else if (reg == (ranges.back().b + 1)
                && (((ARM_REG_S0 <= reg) && (reg <= ARM_REG_S31)) || ((ARM_REG_D16 <= reg) && (reg <= ARM_REG_D31))))
        ranges.back().b++;
      else
        ranges.push_back(reg_range{reg, reg});
    }

    if (ranges.size() == 0)
      DEBUG(("DBGFLT no need to save flt regs"));
    else {
      for (reg_range range : ranges) {
        if (range.a == ARM_REG_FPSCR) {
          ASSERT(range.b == range.a, ("unexpected"));
          DEBUG(("DBGFLT save/restore FPSCR"));

          ArmInsMakeVmrs(push_location, gpreg, ARM_CONDITION_AL);
          reserve_push();
          ArmInsMakePush(push_location, 1<<gpreg, ARM_CONDITION_AL, is_thumb);
          reserve_push();

          ArmInsMakeVmsr(pop_location, gpreg, ARM_CONDITION_AL);
          reserve_pop();
          ArmInsMakePop(pop_location, 1<<gpreg, ARM_CONDITION_AL, is_thumb);
          reserve_pop();
        }
        else {
          t_regset to_store = RegsetNew();
          bool is_single = true;
          for (t_reg r = range.a; r <= range.b; r++) {
            RegsetSetAddReg(to_store, r);
            if (r >= ARM_REG_D16)
              is_single = false;
          }
          DEBUG(("DBGFLT save/restore %d @X", is_single, CPREGSET(cfg, to_store)));

          ArmInsMakeVpush(push_location, to_store, is_single);
          reserve_push();

          ArmInsMakeVpop(pop_location, to_store, is_single);
          reserve_pop();
        }
      }
    }
  }

  /* kill the nops */
  ASSERT(ArmInsIsNOOP(push_location), ("push location is not NOP @eiB", ARM_INS_BBL(push_location)));
  ASSERT(ArmInsIsNOOP(pop_location), ("pop location is not NOP @eiB", ARM_INS_BBL(pop_location)));

  if (f == meta_api_init_function) {
    if (!(meta_api_testing && (meta_api_project_name == "radare2")) && meta_api_impl_inst) {
      /* clear the default compiler-generated instructions contained in one block */
      t_bbl *exit_block = FunctionGetExitBlock(f);
      ASSERT_WITH_DOTS((BBL_CFG(exit_block), "incoming"), !CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(exit_block)), ("expected single incoming edge @eiB", exit_block));

      t_bbl *old_code = CFG_EDGE_HEAD(BBL_PRED_FIRST(exit_block));
      ASSERT(!CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(old_code)), ("expected single incoming edge @eiB", old_code));

      t_bbl *pred = CFG_EDGE_HEAD(BBL_PRED_FIRST(old_code));
      t_bbl *restore_block = ARM_INS_BBL(pop_location);
      if (pred != restore_block) {
        ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(restore_block)), ("expected single outgoing edge @eiB", restore_block));
        ASSERT(BBL_NINS(pred) == 0, ("expected empty block @eiB", pred));
        ASSERT(CFG_EDGE_TAIL(BBL_SUCC_FIRST(restore_block)) == pred, ("unexpected"));

        CfgEdgeKill(BBL_PRED_FIRST(pred));
        BblKill(pred);
      }

      ASSERT(CFG_EDGE_CAT(BBL_PRED_FIRST(exit_block)) == ET_JUMP, ("expected jump edge @E", BBL_PRED_FIRST(exit_block)));
      CfgEdgeChangeHead(BBL_PRED_FIRST(exit_block), restore_block);

      CfgEdgeKill(BBL_PRED_FIRST(old_code));
      BblKill(old_code);
    }
  }

  ArmInsKill(push_location);
  ArmInsKill(pop_location);
}

void MetaAPI_SaveRestoreRegisters(PreparedCallSite *call_site, t_regset *registers, t_regset *live, bool reused, bool from_hell) {
  t_cfg *cfg = BBL_CFG(call_site->before);
  ASSERT(FunctionIsMeta(BBL_FUNCTION(call_site->before)), ("not a meta function @F", BBL_FUNCTION(call_site->before)));
  FunctionMetaApiData *d = FunctionGetMetaApiData(BBL_FUNCTION(call_site->before));
  d->overwritten_registers = *registers;
  d->from_hell = from_hell;

  static bool first_time = true;
  static t_regset old_float_regs = RegsetNew();
  if (first_time) {
    for (t_reg r = ARM_REG_F0; r <= ARM_REG_F7; r++)
      RegsetSetAddReg(old_float_regs, r);
  }

  t_regset live_before = *live;

  /* 1. INTEGER REGISTERS Rx */
  t_regset live_int_regs = CFG_DESCRIPTION(cfg)->int_registers;
#ifndef ALL_REGISTERS_LIVE
  RegsetSetIntersect(live_int_regs, *registers);
#endif

  /* SP and PC can't be saved on the stack */
  RegsetSetSubReg(live_int_regs, ARM_REG_R13);
  RegsetSetSubReg(live_int_regs, ARM_REG_R15);

#ifdef INCLUDE_LIVENESS
  RegsetSetIntersect(live_int_regs, live_before);
#endif
  RegsetSetUnion(d->saved_registers, live_int_regs);

  /* 2. FLOATING POINT REGISTERS Sx, Dx and FPSCR */
  t_regset live_float_regs = RegsetIntersect(CFG_DESCRIPTION(cfg)->flt_registers,
#ifdef INCLUDE_LIVENESS_FLOAT
  RegsetIntersect(CFG_DESCRIPTION(cfg)->callee_may_change, live_before)
#else
  CFG_DESCRIPTION(cfg)->callee_may_change
#endif
  );
  RegsetSetUnion(d->saved_registers, RegsetDiff(live_float_regs, old_float_regs));

  if (!reused) {
    t_bool is_thumb = ArmBblIsThumb(call_site->before);

    /* create NOP instructions (temporary placeholders)... */
    /* ... for the PUSH location */
    t_arm_ins *x;
    ArmMakeInsForBbl(Noop, Prepend, x, call_site->before, is_thumb);
    d->push_location = T_INS(x);

    /* ... for the POP location */
    /* last instruction is control flow -> need to insert the pop before that instruction */
    if (BBL_INS_LAST(call_site->after)
        && ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(call_site->after))))
      ArmMakeInsForIns(Noop, Before, x, T_ARM_INS(BBL_INS_LAST(call_site->after)), is_thumb);
    else
      ArmMakeInsForBbl(Noop, Append, x, call_site->after, is_thumb);
    d->pop_location = T_INS(x);
  }
}

void MetaAPI_PrintInstancePointer(PreparedCallSite *call_site, MetaAPI_Instance *instance) {
  t_cfg *cfg = BBL_CFG(call_site->before);

  MetaAPI_String *format_string = MetaAPI_CreateString(CFG_OBJECT(cfg), "instance pointer at 0x%x\n");
  t_arm_ins *ins;
  t_reloc *reloc;

  /* r0 = format string */
  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(format_string->section), format_string->offset,
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R1, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(instance->pointer_section), AddressNew32(0),
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  /* call */
  ArmMakeInsForBbl(UncondBranchAndLink, Append, ins, call_site->before, false);
  CfgEdgeKill(BBL_SUCC_FIRST(call_site->before));

  CallDynamicSymbol(cfg, "printf", T_INS(ins), call_site->before, call_site->after);
}

void MetaAPI_EmitPrintfDebug(PreparedCallSite *call_site, PrintfDebugData *data) {
  t_arm_ins *ins;
  t_reloc *reloc;

  t_cfg *cfg = BBL_CFG(call_site->before);

  /* r0 = format string */
  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(data->format_string->section), data->format_string->offset,
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  /* r1 = predicate identifier string */
  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R1, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(data->args[0]->section), data->args[0]->offset,
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  /* r2 = setter identifier string */
  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R2, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(data->args[1]->section), data->args[1]->offset,
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  /* r3 = block address */
  ArmMakeInsForBbl(Mov, Append, ins, call_site->before, false, ARM_REG_R3, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins), AddressNew32(0),
    T_RELOCATABLE(call_site->before), 0,
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0, reloc);

  /* call */
  ArmMakeInsForBbl(UncondBranchAndLink, Append, ins, call_site->before, false);
  CfgEdgeKill(BBL_SUCC_FIRST(call_site->before));

  CallDynamicSymbol(cfg, "printf", T_INS(ins), call_site->before, call_site->after);
}

void MetaAPI_Dereference(t_bbl* target, t_reg to, t_reg from) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);
  ArmMakeInsForBbl(Ldr, Append, ins, target, is_thumb, to, from, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);
}

void MetaAPI_GetCompareRegisters(t_regset *regs) {
  RegsetSetAddReg(*regs, ARM_REG_N_CONDITION);
  RegsetSetAddReg(*regs, ARM_REG_Z_CONDITION);
  RegsetSetAddReg(*regs, ARM_REG_C_CONDITION);
  RegsetSetAddReg(*regs, ARM_REG_V_CONDITION);
}

void MetaAPI_CantBeLiveRegisters(t_regset *regs) {
  if (!meta_api_restrict_push_pop)
    return;

  RegsetSetAddReg(*regs, ARM_REG_FPSCR);
  for (t_reg r = ARM_REG_S0; r <= ARM_REG_S15; r++)
    RegsetSetAddReg(*regs, r);
}

void MetaAPI_ReserveStackSpace(PreparedCallSite *call_site, t_int32 slotcount) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(call_site->before);

  ArmMakeInsForBbl(Sub, Prepend, ins, call_site->before, is_thumb, ARM_REG_R13, ARM_REG_R13, ARM_REG_NONE, slotcount*4, ARM_CONDITION_AL);

  if (BBL_INS_LAST(call_site->after)
      && ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(call_site->after)))) {
    ArmMakeInsForIns(Add, Before, ins, T_ARM_INS(BBL_INS_LAST(call_site->after)), is_thumb, ARM_REG_R13, ARM_REG_R13, ARM_REG_NONE, slotcount*4, ARM_CONDITION_AL);
  }
  else {
    ArmMakeInsForBbl(Add, Append, ins, call_site->after, is_thumb, ARM_REG_R13, ARM_REG_R13, ARM_REG_NONE, slotcount*4, ARM_CONDITION_AL);
  }
}

void MetaAPI_CompareFunctionResult(t_bbl *target, bool inverted_condition) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  ArmMakeInsForBbl(Cmp, Append, ins, target, is_thumb, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  t_arm_condition_code cond = ARM_CONDITION_NE;
  if (inverted_condition)
    cond = ArmInvertCondition(cond);

  ArmMakeInsForBbl(CondBranch, Append, ins, target, is_thumb, cond);
}

t_arm_condition_code MetaAPI_RelationToArchitectureCondition(MetaAPI_Relation::Type rel) {
  t_arm_condition_code cond;

  switch (rel) {
  case MetaAPI_Relation::Type::Eq:
    cond = ARM_CONDITION_EQ; break;
  case MetaAPI_Relation::Type::Ne:
  case MetaAPI_Relation::Type::Mod:
    cond = ARM_CONDITION_NE; break;
  case MetaAPI_Relation::Type::Gt:
    cond = ARM_CONDITION_GT; break;
  case MetaAPI_Relation::Type::Ge:
    cond = ARM_CONDITION_GE; break;
  case MetaAPI_Relation::Type::Lt:
    cond = ARM_CONDITION_LT; break;
  case MetaAPI_Relation::Type::Le:
    cond = ARM_CONDITION_LE; break;
  default:
    FATAL(("unsupported relation %d", rel));
  }

  return cond;
}

void MetaAPI_Compare(t_bbl *target, bool inverted_condition, MetaAPI_CompareConfiguration *conf) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  if (conf->relation == MetaAPI_Relation::Type::Mod) {
    ArmMakeInsForBbl(And, Append, ins, target, is_thumb, conf->reg_lhs, conf->reg_lhs, (conf->reg_rhs == REG_NONE) ? ARM_REG_NONE : conf->reg_rhs, conf->imm_rhs, ARM_CONDITION_AL);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);
  } else {
    ArmMakeInsForBbl(Cmp, Append, ins, target, is_thumb, conf->reg_lhs, (conf->reg_rhs == REG_NONE) ? ARM_REG_NONE : conf->reg_rhs, conf->imm_rhs, ARM_CONDITION_AL);
  }

  t_arm_condition_code cond = MetaAPI_RelationToArchitectureCondition(conf->relation);
  if (inverted_condition)
    cond = ArmInvertCondition(cond);

  ArmMakeInsForBbl(CondBranch, Append, ins, target, is_thumb, cond);
}

void MetaAPI_CompareRegisters(t_bbl *target, t_reg a, t_reg b, MetaAPI_Relation::Type rel) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  ArmMakeInsForBbl(Cmp, Append, ins, target, is_thumb, a, b, 0, ARM_CONDITION_AL);

  t_arm_condition_code cond = MetaAPI_RelationToArchitectureCondition(rel);
  ASSERT((cond == ARM_CONDITION_EQ) || (cond == ARM_CONDITION_NE), ("unsupported condition %d", cond));

  ArmMakeInsForBbl(CondBranch, Append, ins, target, is_thumb, cond);
}

void MetaAPI_WriteVTablePointer(PreparedCallSite *call_site, t_symbol *vtable_symbol, t_regset *overwritten_registers) {
  t_arm_ins *call_ins = T_ARM_INS(BBL_INS_LAST(call_site->before));
  ASSERT(call_ins && (ARM_INS_OPCODE(call_ins) == ARM_BL), ("expected before to end in call @eiB", call_site->before));

  t_arm_ins *new_ins;
  ArmMakeInsForIns(Mov, Before, new_ins, call_ins, false, ARM_REG_R10, ARM_REG_R0, 0, ARM_CONDITION_AL);

  ArmMakeInsForBbl(Mov, Prepend, new_ins, call_site->after, false, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(call_site->before))),
    AddressNew32(0),
    T_RELOCATABLE(new_ins), AddressNew32(0),
    T_RELOCATABLE(SYMBOL_BASE(vtable_symbol)), AddressNew32(8),
    FALSE, NULL, NULL, NULL,
    "R00A00+" "\\" WRITE_32);

  ArmInsMakeAddressProducer(new_ins, 0, reloc);

  t_arm_ins *new_ins2;
  ArmMakeInsForIns(Str, After, new_ins2, new_ins, false, ARM_REG_R0, ARM_REG_R10, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);

  RegsetSetAddReg(*overwritten_registers, ARM_REG_R0);
  RegsetSetAddReg(*overwritten_registers, ARM_REG_R10);
}

void MetaAPI_DoOperand(t_bbl *target, t_reg op1, t_reg op2, MetaAPI_Operand::Type operand) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  switch (operand) {
  case MetaAPI_Operand::Type::Add:
    ArmMakeInsForBbl(Add, Append, ins, target, is_thumb, op1, op1, op2, 0, ARM_CONDITION_AL);
    break;
  
  default:
    FATAL(("unsupported operand %d", static_cast<t_uint32>(operand)));
  }
}

void MetaAPI_Store(t_bbl *target, t_reg destination, t_reg base_register) {
  t_arm_ins *ins;
  bool is_thumb = ArmBblIsThumb(target);

  ArmMakeInsForBbl(Str, Append, ins, target, is_thumb, destination, base_register, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);
}
