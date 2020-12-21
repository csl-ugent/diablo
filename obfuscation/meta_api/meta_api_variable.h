#ifndef META_API_VARIABLE_H
#define META_API_VARIABLE_H

struct MetaAPI_VariableRelation {
  MetaAPI_Relation::Type relation;
  MetaAPI_Variable *that;
  MetaAPI_Datatype *datatype;
};

struct _MetaAPI_Variable {
  std::vector<MetaAPI_VariableRelation *> constraints;

  /* metadata */
  t_uint32 uid;
  t_section *section;
  int array_size;

  /* declaration */
  MetaAPI_Datatype *datatype;
  std::string identifier;

  _MetaAPI_Variable(t_section *);
  _MetaAPI_Variable(MetaAPI_Datatype *dt, std::string ident);

  std::string Print();
  void SetValue(t_cfg *cfg, std::vector<MetaAPI_Datatype *> instance_ofs, RelationToOtherVariables relations);

  bool has_value;
  AbstractValue abstract_value;
  t_int32 stack_slot;

  /* only used for variables declared in an inlined transformer (locals),
   * set to TRUE when no rvalue is specified and hence no value should be generated */
  bool no_rvalue;

  MetaAPI_FunctionParameter *argument;

  bool Equals(MetaAPI_Variable *other);
  bool LessThan(MetaAPI_Variable *other);
};

/* detect the variables defined in the meta-API */
std::vector<MetaAPI_Variable *> MetaAPI_FindVariables(t_cfg *cfg);

/* load constraints defined on variables */
void MetaAPI_LoadVariableConstraints(t_cfg *cfg, ConstraintList constraints);

/* load constraints defined on function arguments, indexed by 'constraint_index' */
void MetaAPI_ResetConstraints(MetaAPI_Function *function);
void MetaAPI_LoadArgumentConstraints(MetaAPI_Function *function, ConstraintList constraints);

/* assign a value to each variable in the given list */
void MetaAPI_AssignVariableValues(t_cfg *cfg, std::vector<MetaAPI_Variable *> variables, bool set_section_data);

MetaAPI_Variable *GetVariableByName(std::string name);

#endif /* META_API_VARIABLE_H */
