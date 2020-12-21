#include "meta_api.h"

using namespace std;

static map<string, MetaAPI_Variable *> name_to_variable;

MetaAPI_Variable::_MetaAPI_Variable(t_section *_section) {
  static t_uint32 _uid = 0;

  uid = _uid++;
  section = _section;
  argument = NULL;

  if (section) {
    /* extract information from section name */
    vector<string> components;

    t_string_array *_components = StringDivide(SECTION_NAME(section), "$", FALSE, FALSE);
    t_string_array_elem *element;
    STRING_ARRAY_FOREACH_ELEM(_components, element)
      components.push_back(element->string);
    StringArrayFree(_components);

    // string str_section = components[0];
    string str_datatype = components[1];
    string str_function = components[2];
    identifier = components[3];

    /* fill in the remaining data */
    datatype = MetaAPI_GetDatatypePtr(str_datatype);

    /* if it is an argument */
    if (StringPatternMatch(META_API_ARGUMENT_SECTION_PREFIX "*", SECTION_NAME(section))) {
      MetaAPI_Function *function = MetaAPI_GetFunctionByName(str_function);

      for (auto param : function->parameters) {
        if (param->identifier == identifier) {
          /* associate! */
          VERBOSE(meta_api_verbosity, ("associate '%s' with '%s'", SECTION_NAME(section), function->Print().c_str()));
          param->variable = this;
          argument = param;

          break;
        }
      }
    }
  }

  has_value = false;
  stack_slot = -1;
  no_rvalue = false;

  array_size = -1;
}

MetaAPI_Variable::_MetaAPI_Variable(MetaAPI_Datatype *dt, std::string ident) : _MetaAPI_Variable(NULL) {
  identifier = ident;
  datatype = dt;
}

string MetaAPI_Variable::Print() {
  string result = "META-variable";
  result += " UID(" + to_string(uid) + ")";

  if (section)
    result += " section(" + string(SECTION_NAME(section)) + ")";
  else
    result += " not in section";
  return result;
}

void MetaAPI_Variable::SetValue(t_cfg *cfg, vector<MetaAPI_Datatype *> instance_ofs, RelationToOtherVariables relations) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "generating value for %s", Print().c_str()));

  MetaAPI_Datatype *gen_datatype = datatype;
  if (instance_ofs.size() > 0) {
    gen_datatype = instance_ofs[0];
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  instance of %s", gen_datatype->Print().c_str()));
  }

  /* TODO: need to generate a value if no values are defined */
  ASSERT(gen_datatype->values.size() > 0, ("can't find a value for %s", Print().c_str()));

  /* easy for 'equal' values */
  if (relations.find(MetaAPI_Relation::Type::Eq) != relations.end()) {
    MetaAPI_Variable *other = relations[MetaAPI_Relation::Type::Eq][0];
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  copy from %s", other->Print().c_str()));

    abstract_value = other->abstract_value;
  }
  else
    abstract_value = gen_datatype->GenerateRandomValue(relations, meta_api_value_rng);

  has_value = true;

  /* sanity check for values of other variables */
  for (auto i : relations) {
    MetaAPI_Relation::Type relation = i.first;
    vector<MetaAPI_Variable *> variables = i.second;

    switch (relation) {
    case MetaAPI_Relation::Type::Eq:
      for (auto var : variables) {
        if (var->has_value)
          ASSERT(Equals(var), ("'%s' (value %s) should == '%s' (value %s)", Print().c_str(), abstract_value.Print().c_str(), var->Print().c_str(), var->abstract_value.Print().c_str()));
      }
      break;
    case MetaAPI_Relation::Type::Ne:
      for (auto var : variables) {
        if (var->has_value)
          ASSERT(!Equals(var), ("'%s' (value %s) should != '%s' (value %s)", Print().c_str(), abstract_value.Print().c_str(), var->Print().c_str(), var->abstract_value.Print().c_str()));
      }
      break;
    case MetaAPI_Relation::Type::Lt:
      for (auto var : variables) {
        if (var->has_value)
          ASSERT(LessThan(var), ("'%s' (value %s) should < '%s' (value %s)", Print().c_str(), abstract_value.Print().c_str(), var->Print().c_str(), var->abstract_value.Print().c_str()));
      }
      break;
    default:
      FATAL(("unsupported constraint relation '%s'", MetaAPI_Relation::Print(relation).c_str()));
    }
  }
}

bool MetaAPI_Variable::Equals(MetaAPI_Variable *other) {
  return abstract_value.type == other->abstract_value.type
    && abstract_value.Equals(&(other->abstract_value));
}

bool MetaAPI_Variable::LessThan(MetaAPI_Variable *other) {
  return abstract_value.type == other->abstract_value.type
    && abstract_value.LessThan(&(other->abstract_value));
}

vector<MetaAPI_Variable *> MetaAPI_FindVariables(t_cfg *cfg) {
  vector<MetaAPI_Variable *> result;
  int nr_variables = 0;
  int nr_arguments = 0;

  VERBOSE(0, (META_API_PREFIX "Looking for references to variables and function arguments..."));

  t_section *data_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".data");
  t_section *section;
  SECTION_FOREACH_SUBSECTION(data_section, section) {
    /* variables */
    if (StringPatternMatch(META_API_VARIABLE_SECTION_PREFIX "*", SECTION_NAME(section))) {
      if (name_to_variable.find(string(SECTION_NAME(section))) != name_to_variable.end())
        /* we already created this variable, don't add it twice */
        continue;

      /* create a variable */
      MetaAPI_Variable *var = new MetaAPI_Variable(section);
      VERBOSE(0, (META_API_PREFIX "  variable: %s", var->Print().c_str()));
      result.push_back(var);

      /* take note of its existence */
      name_to_variable[string(SECTION_NAME(section))] = var;
      nr_variables++;
    }
    /* arguments */
    else if (StringPatternMatch(META_API_ARGUMENT_SECTION_PREFIX "*", SECTION_NAME(section))) {
      if (name_to_variable.find(string(SECTION_NAME(section))) != name_to_variable.end())
        continue;

      MetaAPI_Variable *var = new MetaAPI_Variable(section);
      VERBOSE(0, (META_API_PREFIX "  argument: %s", var->Print().c_str()));

      name_to_variable[string(SECTION_NAME(section))] = var;
      nr_arguments++;
    }
  }

  VERBOSE(0, (META_API_PREFIX "  %d variables and %d function arguments found", nr_variables, nr_arguments));

  return result;
}

static vector<MetaAPI_VariableRelation *> variable_relations;

static
void relate(MetaAPI_Variable *lhs, MetaAPI_Variable *rhs, MetaAPI_Relation::Type relation) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "Relation %s %s %s\n%p %p", lhs->Print().c_str(), MetaAPI_Relation::Print(relation).c_str(), rhs->Print().c_str(), lhs, rhs));

  auto relate_variable = [] (MetaAPI_Variable *lhs, MetaAPI_Variable *rhs, MetaAPI_Relation::Type relation) {
    /* sanity check */
    bool already = false;
    for (auto c : lhs->constraints) {
      if (c->that == rhs) {
        ASSERT(!already, ("this is really weird"));
        ASSERT(c->relation == relation, ("different relation already present %s", MetaAPI_Relation::Print(relation).c_str()));
        already = true;
      }
    }

    if (!already) {
      MetaAPI_VariableRelation *vrel = new MetaAPI_VariableRelation();
      vrel->relation = relation;
      vrel->that = rhs;

      lhs->constraints.push_back(vrel);
      variable_relations.push_back(vrel);
    }
  };

  relate_variable(lhs, rhs, relation);
  relate_variable(rhs, lhs, MetaAPI_Relation::Revert(relation));
}

void MetaAPI_LoadVariableConstraints(t_cfg *cfg, ConstraintList constraints) {
  for (auto i : constraints) {
    VERBOSE(meta_api_verbosity, ("variable constraint '%s'", i->Print().c_str()));

    if (i->relation == MetaAPI_Relation::Type::InstanceOf) {
      MetaAPI_Datatype *datatype = MetaAPI_GetDatatypePtr(i->v2_identifier);

      bool already = false;
      for (auto c : i->v1->constraints) {
        if (c->relation == MetaAPI_Relation::Type::InstanceOf) {
          ASSERT(c->datatype == datatype, ("different instanceof already present"));
          already = true;
        }
      }

      if (!already) {
        MetaAPI_VariableRelation *vrel = new MetaAPI_VariableRelation();
        vrel->relation = MetaAPI_Relation::Type::InstanceOf;
        vrel->datatype = datatype;

        i->v1->constraints.push_back(vrel);
        variable_relations.push_back(vrel);
      }
    }
    else {
      ASSERT(i->v1->argument == NULL, ("variable '%s' should not be function argument", i->v1->Print().c_str()));
      ASSERT(i->v2->argument == NULL, ("variable '%s' should not be function argument", i->v2->Print().c_str()));

      relate(i->v1, i->v2, i->relation);
    }
  }
}

void MetaAPI_ResetConstraints(MetaAPI_Function *function) {
  for (auto param : function->parameters) {
    if (!param->variable)
      continue;

    param->variable->constraints.clear();
  }

  for (auto local_ : function->implementation_locals) {
    local_.second->constraints.clear();
  }

  for (auto rel : variable_relations)
    delete rel;
  variable_relations.clear();
}

void MetaAPI_LoadArgumentConstraints(MetaAPI_Function *function, ConstraintList constraints) {
  for (auto i : constraints) {
    ASSERT(i, ("this should not happen"));

    /* sanity checks on both variables */
    VERBOSE(meta_api_verbosity, ("argument constraint '%s'", i->Print().c_str()));

    ASSERT(i->v1 != NULL, ("v1 not set '%s'", i->Print()));
    ASSERT(i->v2 != NULL, ("v2 not set '%s'", i->Print()));

    /* 2. both need to be marked as function arguments */
    ASSERT(i->v1->argument != NULL, ("variable '%s' should be function argument", i->v1->Print().c_str()));
    ASSERT(i->v2->argument != NULL, ("variable '%s' should be function argument", i->v2->Print().c_str()));

    /* 3. both need to be associated with the same function */
    ASSERT(i->v1->argument->function == i->v2->argument->function,
            ("variable '%s' (function '%s') and '%s' (function '%s') should be in same function!",
              i->v1->Print().c_str(), i->v1->argument->function->Print().c_str(),
              i->v2->Print().c_str(), i->v2->argument->function->Print().c_str()));

    relate(i->v1, i->v2, i->relation);
  }
}

void MetaAPI_AssignVariableValues(t_cfg *cfg, vector<MetaAPI_Variable *> variables, bool set_section_data) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "assigning values to %d variables", variables.size()));

  /* TODO: first set variables having less possible values */
  for (auto variable : variables) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  '%s'", variable->Print().c_str()));

    RelationToOtherVariables relations;

    vector<MetaAPI_Datatype *> instance_ofs;

    for (auto ii : variable->constraints) {
      if (ii->relation == MetaAPI_Relation::Type::InstanceOf) {
        instance_ofs.push_back(ii->datatype);
        VERBOSE(meta_api_verbosity, (META_API_PREFIX "    add instance-of '%s'", ii->datatype->Print().c_str()));
      }
      else {
        MetaAPI_Variable *related_variable = ii->that;

        /* no need to take into account variables that don't have a value yet */
        if (!related_variable->has_value)
          continue;

        MetaAPI_Relation::Type relation = ii->relation;
        if (relations.find(relation) == relations.end())
          relations[relation] = vector<MetaAPI_Variable *>();
        relations[relation].push_back(related_variable);
        VERBOSE(meta_api_verbosity, (META_API_PREFIX "    add relation '%s %s'", MetaAPI_Relation::Print(relation).c_str(), related_variable->Print().c_str()));
      }
    }

    ASSERT(instance_ofs.size() <= 1, ("only support single instance-of"));

    variable->SetValue(cfg, instance_ofs, relations);
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "  -> %s", variable->abstract_value.Print().c_str()));

    if (set_section_data)
      variable->abstract_value.ProduceValueInSection(cfg, variable->section);
  }
}

MetaAPI_Variable *GetVariableByName(string name) {
  ASSERT(name_to_variable.find(name) != name_to_variable.end(), ("can't find variable with name '%s'", name.c_str()));
  return name_to_variable[name];
}
