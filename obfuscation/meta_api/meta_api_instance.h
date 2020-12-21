#ifndef META_API_INSTANCE_H
#define META_API_INSTANCE_H

struct _MetaAPI_Instance {
  MetaAPI_Datatype *datatype;
  t_uint32 uid;
  t_section *pointer_section;
  std::map<std::string, MetaAPI_Variable *> variables;
  t_int32 stack_slot;
};

typedef struct {
  t_bbl *decision_bbl;
  t_bbl *next_bbl;
  t_cfg_edge *first_edge;
  bool two_way;
} MetaAPI_QueryResult;

typedef struct {
  t_function *function;
  t_cfg_edge *first_edge;
  t_bbl *last_block;
} MetaAPI_TransformResult;

typedef struct {
  t_section *pointer_section;
  t_uint32 uid;
  t_int32 stack_slot;
} MetaAPI_Storage;

typedef struct {
  MetaAPI_Function *getter;
  MetaAPI_Effect::Effect expected;
  t_bbl *decision_bbl;
  MetaAPI_ActivePredicate *predicate;
} SetterVerificationInfo;

typedef struct {
  MetaAPI_String *format_string;
  MetaAPI_String *args[5];
} PrintfDebugData;

MetaAPI_Instance *MetaAPI_InstantiateDatastructure(t_cfg *cfg, MetaAPI_Datatype *datatype, MetaAPI_Function *constructor, t_bbl *target, std::string fn_name, t_bbl *& return_site, t_function *& fun, bool sub_instance, std::function<MetaAPI_Storage()> storage_fn, PreparedCallSite& call_site, bool create_in_init_function, bool call_implementation);
MetaAPI_TransformResult MetaAPI_TransformDatastructure(MetaAPI_Instance *instance, MetaAPI_Function *transformer, t_bbl *target, size_t arg_constraints_set, std::string fn_name, SetterVerificationInfo& verify_info, bool call_implementation);
MetaAPI_QueryResult MetaAPI_QueryDatastructure(MetaAPI_ActivePredicate *active_predicate, MetaAPI_Function *getter, t_bbl *target, std::string fn_name, MetaAPI_Effect::Effect value, bool try_reuse, bool call_implementation);
MetaAPI_Effect::Effect MetaAPI_PreferToReuseGetter(MetaAPI_ActivePredicate *active_predicate, t_randomnumbergenerator *rng);

#endif /* META_API_INSTANCE_H */
