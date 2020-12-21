#ifndef META_API_IMPLEMENTATION_H
#define META_API_IMPLEMENTATION_H

struct _MetaAPI_ImplementationValue {
  static constexpr t_uint8 MODIFIER_REFERENCE = 1<<0;
  static constexpr t_uint8 MODIFIER_MEMBER = 1<<1;
  static constexpr t_uint8 MODIFIER_DEREFERENCE = 1<<2;

  enum class Type {
    /*0*/Annotation,
    /*1*/Identifier,
    /*2*/Number,
    /*3*/String,
    /*4*/Null,
    /*5*/MetaGlobalVariable,
    /*6*/MetaLocalVariable,
    /*7*/MetaInstanceVariable,
    /*8*/Range,

    /*9*/None
  };
  Type type;
  std::string value;
  std::string value2;
  t_uint8 modifiers;
  _MetaAPI_ImplementationValue *array_index;

  _MetaAPI_ImplementationValue() {
    value = "";
    value2 = "";
    modifiers = 0;
    type = Type::None;
    array_index = NULL;
  }

  std::string Print() const;
  bool GetNumber(t_uint32 &value) const;
};

typedef struct _MetaAPI_ImplementationValue MetaAPI_ImplementationValue;

struct _MetaAPI_AbstractStmt {
  enum class Type {
    VariableDeclaration,
    FunctionCall,
    Return,
    If,
    Relation,
    Rvalue,
    VariableAssignment,
    While,
    Expression,
    Break
  };
  Type type;

  virtual std::string Print() = 0;
  std::vector<MetaAPI_AbstractStmt *> body;
};

struct MetaAPI_RvalueStmt : MetaAPI_AbstractStmt {
  MetaAPI_RvalueStmt(MetaAPI_ImplementationValue v) {
    type = Type::Rvalue;
    value = v;
  }

  std::string Print();

  MetaAPI_ImplementationValue value;
};

struct MetaAPI_BreakStmt : MetaAPI_AbstractStmt {
  MetaAPI_BreakStmt() {
    type = Type::Break;
  }

  std::string Print();
};

struct MetaAPI_RelationStmt : MetaAPI_AbstractStmt {
  MetaAPI_RelationStmt() {
    type = Type::Relation;
    S1 = NULL;
  }

  std::string Print();

  MetaAPI_ImplementationValue op1;
  MetaAPI_AbstractStmt *S1;
  MetaAPI_ImplementationValue op2;
  MetaAPI_Relation rel;
};

struct MetaAPI_ExpressionStmt : MetaAPI_AbstractStmt {
  MetaAPI_ExpressionStmt() {
    type = Type::Expression;
  }

  std::string Print();

  MetaAPI_ImplementationValue op1;
  MetaAPI_ImplementationValue op2;
  MetaAPI_Operand operand;
};

struct MetaAPI_ReturnStmt : MetaAPI_AbstractStmt {
  MetaAPI_ReturnStmt() {
    type = Type::Return;
    inverted_condition = false;
    S = NULL;
  }

  std::string Print();

  MetaAPI_AbstractStmt *S;
  bool inverted_condition;
};

struct MetaAPI_IfStmt : MetaAPI_AbstractStmt {
  MetaAPI_IfStmt() {
    type = Type::If;
    inverted_condition = false;
  }

  std::string Print();

  MetaAPI_AbstractStmt *S;
  bool inverted_condition;
};

struct MetaAPI_WhileStmt : MetaAPI_AbstractStmt {
  MetaAPI_WhileStmt() {
    type = Type::While;
    inverted_condition = false;
  }

  std::string Print();

  MetaAPI_AbstractStmt *S;
  bool inverted_condition;
};

struct MetaAPI_VariableDeclarationStmt : MetaAPI_AbstractStmt {
  MetaAPI_VariableDeclarationStmt() {
    type = Type::VariableDeclaration;
    value.type = MetaAPI_ImplementationValue::Type::Null;
    assigned = NULL;
    pointer = false;
    array_size = MetaAPI_ImplementationValue();
  }

  std::string Print();

  std::string datatype;
  std::string identifier;
  MetaAPI_ImplementationValue value;
  MetaAPI_Variable *assigned;
  bool pointer;
  MetaAPI_ImplementationValue array_size;
};

struct MetaAPI_VariableAssignmentStmt : MetaAPI_AbstractStmt {
  MetaAPI_VariableAssignmentStmt() {
    type = Type::VariableAssignment;
    S = NULL;
  }

  std::string Print();

  MetaAPI_ImplementationValue target;
  MetaAPI_AbstractStmt *S;
};

struct MetaAPI_FunctionCallStmt : MetaAPI_AbstractStmt {
  MetaAPI_FunctionCallStmt() {
    type = Type::FunctionCall;
    assigned = NULL;
    call_new = false;
    dereference = false;
  }

  std::string Print();

  std::string callee;
  std::vector<MetaAPI_ImplementationValue> values;
  MetaAPI_Function *assigned;
  bool call_new;
  bool dereference;
};

void MetaAPI_ParseImplementationWithGrammar(std::string impl, std::vector<MetaAPI_AbstractStmt *>* stmt_list);

#endif /* META_API_IMPLEMENTATION_H */
