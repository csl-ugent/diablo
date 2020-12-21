#include <string>

using namespace std;

static MetaAPI_Constraint *_current_constraint;

static
void process_identifier(string ident, MetaAPI_Constraint::OperandType& type, string& identifier, MetaAPI_Variable *& variable, bool find_variable=true) {
  if (ident.find("$$$") == 0) {
    type = MetaAPI_Constraint::OperandType::InstanceVariable;
    identifier = ident.substr(3);
  }
  else if (ident.find("$$") == 0) {
    type = MetaAPI_Constraint::OperandType::GlobalVariable;
    variable = MetaAPI_GetGlobalVariableByName(ident.substr(2));
  }
  else if (ident.find("$") == 0) {
    type = MetaAPI_Constraint::OperandType::LocalVariable;
    identifier = ident.substr(1);
  }
  else if (find_variable)
    variable = GetVariableByName(ident);
  else
    identifier = ident;
}

void add_constraint(string ident1, string ident2, MetaAPI_Relation relation, bool find_variable=true) {
  process_identifier(ident1, _current_constraint->v1_type, _current_constraint->v1_identifier, _current_constraint->v1, true);
  process_identifier(ident2, _current_constraint->v2_type, _current_constraint->v2_identifier, _current_constraint->v2, find_variable);

  _current_constraint->relation = relation.value;
}

void add_constraint_int(string ident1, string ident2, MetaAPI_Relation relation) {
  add_constraint(ident1, ident2, relation);
  _current_constraint->v2_type = MetaAPI_Constraint::OperandType::Integer;
}

void add_constraint_string(string ident1, string ident2, MetaAPI_Relation relation) {
  add_constraint(ident1, ident2, relation, false);
  _current_constraint->v2_type = MetaAPI_Constraint::OperandType::String;
}

static
string to_string(pANTLR3_UINT8 x) {
  return string(reinterpret_cast<char*>(x));
}

static
string to_string(pANTLR3_STRING x) {
  return to_string(x->chars);
}
