#include "meta_api.h"

using namespace std;

AbstractValue::Contents variable_contents(MetaAPI_Variable *var) {
  AbstractValue::Contents x;
  x.type = AbstractValue::Type::MetaAPI_Variable;
  x.generic = AbstractValue::pack(var);
  return x;
}

MetaAPI_Variable *variable_value(AbstractValue::Contents x) {
  return AbstractValue::unpack<MetaAPI_Variable>(x.generic);
}

/* constructor */
AbstractValue::_AbstractValue() {
  have_assigned = false;
  variable = NULL;
}

/* instance */
static AbstractValue::HelperFunctions instance_interface = {
  [] (AbstractValue *, RelationToOtherVariables) {
    FATAL(("not implemented for Instance"));
    return AbstractValue::Contents();
  },
  [] (AbstractValue *, AbstractValue *) {
    FATAL(("not implemented for Instance"));
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *v) {
    return "Instance";
  }
};

AbstractValue AbstractValue::CreateInstance() {
  AbstractValue x = AbstractValue();
  x.type = Type::MetaAPI_Instance;
  x.interface = instance_interface;
  x.assigned.flags = 0;
  x.assigned.offset = -1;
  return x;
}

/* variable */
static AbstractValue::HelperFunctions variable_interface = {
  [] (AbstractValue *value, RelationToOtherVariables) {
    return value->generator_arguments[0];
  },
  [] (AbstractValue *, AbstractValue *) {
    FATAL(("not implemented for Variable"));
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *v) {
    return "Variable";
  }
};

AbstractValue AbstractValue::CreateVariable(MetaAPI_Variable *v, t_uint8 modifiers, string member, MetaAPI_ImplementationValue *array_index) {
  AbstractValue x = AbstractValue();
  x.type = Type::MetaAPI_Variable;
  x.interface = variable_interface;
  x.variable = v;

  Contents contents = variable_contents(v);
  contents.array_index = array_index;

  contents.flags = 0;
  if (modifiers & MetaAPI_ImplementationValue::MODIFIER_REFERENCE)
    contents.flags |= Contents::FLAG_REFERENCE;
  if (modifiers & MetaAPI_ImplementationValue::MODIFIER_MEMBER)
    contents.flags |= Contents::FLAG_MEMBER;

  contents.offset = -1;

  if (contents.flags & Contents::FLAG_MEMBER) {
    /* look for the member offset */
    contents.offset = v->datatype->MemberOffset(member);
    ASSERT(contents.offset != -1, ("can't find member '%s' in %s", member.c_str(), v->datatype->Print().c_str()));
  }

  x.generator_arguments.push_back(contents);
  return x;
}

/* null */
static AbstractValue::HelperFunctions null_interface = {
  [] (AbstractValue *, RelationToOtherVariables) {
    FATAL(("not implemented for Null"));
    return AbstractValue::Contents();
  },
  [] (AbstractValue *, AbstractValue *) {
    FATAL(("not implemented for Null"));
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *v) {
    return "Null";
  }
};

AbstractValue AbstractValue::CreateNull() {
  AbstractValue x = AbstractValue();
  x.type = Type::MetaAPI_Null;
  x.interface = null_interface;
  return x;
}

/* proxy */
static AbstractValue::HelperFunctions proxy_interface = {
  [] (AbstractValue *, RelationToOtherVariables) {
    FATAL(("not implemented for proxy"));
    return AbstractValue::Contents();
  },
  [] (AbstractValue *, AbstractValue *) {
    FATAL(("not implemented for proxy"));
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *v) {
    return "Proxy";
  }
};

AbstractValue AbstractValue::CreateProxy() {
  AbstractValue x = AbstractValue();
  x.type = Type::MetaAPI_Proxy;
  x.interface = proxy_interface;
  return x;
}

AbstractValue::Contents AbstractValue::Generate() {
  RelationToOtherVariables no_relations;
  return interface.generator(this, no_relations);
}

string AbstractValue::Print() {
  return "AbstractValue(" + interface.print(this) + ")";
}

void AbstractValue::ProduceValueInRegister(MetaAPI_Instance *instance, bool generate_new, ProduceValueInfo *info, std::function<void(AbstractValue&)> modifier) {
  if (generate_new) {
    if (type != Type::MetaAPI_Instance) {
      assigned = Generate();
      have_assigned = true;

      if (modifier != nullptr)
        modifier(*this);
    }
  }

  DiabloBrokerCall("MetaAPI_ProduceValueInRegister", instance, this, info);
}

void AbstractValue::ProduceValueInSection(t_cfg *cfg, t_section *section) {
  DiabloBrokerCall("MetaAPI_ProduceValueInSection", cfg, this, section);
}

bool AbstractValue::Equals(AbstractValue *other) {
  return interface.compare_to(this, other) == CompareResult::Equal;
}

bool AbstractValue::LessThan(AbstractValue *other) {
  return interface.compare_to(this, other) == CompareResult::Less;
}

AbstractValue::PossibleValueList AbstractValue::PossibleValues(RelationToOtherVariables relations, Properties properties) {
  return interface.possible_values(this, relations, properties);
}
