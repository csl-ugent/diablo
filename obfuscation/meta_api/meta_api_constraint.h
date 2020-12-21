#ifndef META_API_CONSTRAINTS_H
#define META_API_CONSTRAINTS_H

struct _MetaAPI_Constraint;
typedef struct _MetaAPI_Constraint MetaAPI_Constraint;

struct _MetaAPI_Constraint {
  enum class OperandType {
    GlobalVariable,
    InstanceVariable,
    LocalVariable,
    Integer,
    String
  };

  std::string expression;

  OperandType v1_type;
  MetaAPI_Variable *v1;
  std::string v1_identifier;

  OperandType v2_type;
  MetaAPI_Variable *v2;
  std::string v2_identifier;

  MetaAPI_Relation::Type relation;

  std::string Print();

  _MetaAPI_Constraint(std::string expr);
};

typedef std::map<MetaAPI_Relation::Type, std::vector<MetaAPI_Variable *>> RelationToOtherVariables;
typedef std::map<MetaAPI_Variable *, RelationToOtherVariables> VariableConstraints;
typedef std::vector<MetaAPI_Constraint *> ConstraintList;

void MetaAPI_ParseConstraintWithGrammar(MetaAPI_Constraint *constraint);

#endif /* META_API_CONSTRAINTS_H */
