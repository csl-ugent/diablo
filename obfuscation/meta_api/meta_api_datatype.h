#ifndef META_API_DATATYPE_H
#define META_API_DATATYPE_H

struct MetaAPI_Datatype_cmp {
    bool operator() (MetaAPI_Datatype *a, MetaAPI_Datatype *b) const;
};
typedef std::map<MetaAPI_Datatype *, std::set<AbstractValue::Contents>, MetaAPI_Datatype_cmp> PossibleValueSet;

struct _MetaAPI_Datatype {
  struct Member {
    std::string name;
    t_uint32 offset;
  };
  std::vector<Member> members;

  std::string name;
  std::string aliases_to;
  int uid;
  bool enabled;

  bool is_class;
  size_t size;
  t_symbol *vtable_symbol;

  /* instances created for this datatype */
  std::vector<MetaAPI_Instance *> instances;

  /* functions defined on this datatype */
  std::vector<MetaAPI_Function *> constructors;
  std::vector<MetaAPI_Function *> transformers;
  std::vector<MetaAPI_Function *> interface;

  /* predicates defined on this datatype */
  std::vector<MetaAPI_Predicate *> predicates;

  /* possible values */
  std::vector<MetaAPI_Value> values;
  std::vector<AbstractValue::Contents> possible_values;

  /* instance-specific stuff */
  std::map<std::string, MetaAPI_Variable *> instance_implementation_locals;

  MetaAPI_Function *instance_implementation;

  /* don't make these pure-virtual as they don't _need_ to be overridden
   * (e.g., for constructed datatypes) */
  virtual AbstractValue FromString(t_cfg *cfg, std::string str);
  virtual AbstractValue FromExpression(t_cfg *cfg, MetaAPI_Expression expr) {
    FATAL(("override FromExpression for '%s'", name.c_str()));
  }
  virtual std::string Print(AbstractValue::Contents contents) {
    FATAL(("override Print for '%s'", name.c_str()));
  }
  virtual AbstractValue AbstractValueInstance() {
    FATAL(("override AbstractValueInstance for '%s'", name.c_str()));
  }
  virtual AbstractValue True(t_cfg *cfg) {
    FATAL(("override True for '%s'", name.c_str()));
  }
  virtual AbstractValue False(t_cfg *cfg) {
    FATAL(("override False for '%s'", name.c_str()));
  }

  t_int32 MemberOffset(std::string name);

  AbstractValue FromContents(AbstractValue::Contents contents) {
    AbstractValue x = AbstractValueInstance();
    x.assigned = contents;
    x.have_assigned = true;
    return x;
  }

  _MetaAPI_Datatype(std::string _name) {
    static int _uid = 0;
    name = _name;
    uid = _uid++;

    instance_implementation = NULL;
  }

  std::string Print();
  AbstractValue GenerateRandomValue(RelationToOtherVariables relations, t_randomnumbergenerator *rng);

private:
  PossibleValueSet CollectPossibleValues(RelationToOtherVariables relations, Properties properties);
};

std::vector<MetaAPI_Datatype *> DatatypesWithPredicates();

#endif /* META_API_DATATYPE_H */
