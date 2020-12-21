#ifndef META_API_VALUE_H
#define META_API_VALUE_H

struct _AbstractValue;
typedef struct _AbstractValue AbstractValue;

struct _AbstractValue {
  enum class Type {
    /*0*/ None,
    /*1*/ Integer,
    /*2*/ String,
    /*3*/ ProgramString,
    /*4*/ Bool,
    /*5*/ FunctionPointer,
    /*6*/ Register,

    /*7*/ MetaAPI_Instance,
    /*8*/ MetaAPI_Null,
    /*9*/ MetaAPI_Proxy,
    /*10*/ Void,
    /*11*/ MetaAPI_Variable
  };
  Type type;

  enum class CompareResult {
    Less,
    Equal,
    Greater
  };

  struct Contents_;
  typedef struct Contents_ Contents;

  struct Contents_ {
    static constexpr t_uint8 FLAG_REFERENCE = 1<<0;
    static constexpr t_uint8 FLAG_MEMBER = 1<<1;

    Type type;
    union {
      uint32_t uint32;
      bool b;
      void* generic;
      t_function *function;
      MetaAPI_Variable *variable;
    };
    std::function<bool(const Contents *a, const Contents *b)> compare;
    t_uint8 flags;
    t_int32 offset;
    MetaAPI_ImplementationValue *array_index;

    bool operator<(const Contents_ &x) const {
      t_uint32 t1 = static_cast<t_uint32>(type);
      t_uint32 t2 = static_cast<t_uint32>(x.type);

      /* calling 'compare' on instances with different types is undefined */
      return (t1 != t2) ? (t1 < t2) : compare(this, &x);
    }
  };

  typedef std::set<Contents> PossibleValueList;

  std::vector<Contents> generator_arguments;
  void AddGeneratorArgument(Contents contents) { generator_arguments.push_back(contents); }

  Contents assigned;
  bool have_assigned;
  MetaAPI_Variable *variable;
  AbstractValue *array_index;

  typedef struct {
    std::function<Contents(_AbstractValue *, RelationToOtherVariables)> generator;
    std::function<CompareResult(AbstractValue *, AbstractValue *)> compare_to;
    std::function<std::string(AbstractValue *)> print;
    std::function<PossibleValueList(AbstractValue *, RelationToOtherVariables, Properties)> possible_values;
  } HelperFunctions;

  /* default implementation */
  HelperFunctions interface = {
    /* generator */
    [] (AbstractValue *, RelationToOtherVariables) {
      FATAL(("override generator!"));
      return Contents();
    },
    /* compare_to */
    [] (AbstractValue *, AbstractValue *) {
      FATAL(("override compare_to!"));
      return CompareResult::Equal;
    },
    /* print */
    [] (AbstractValue *v) {
      std::string result = "<implement me (type ";
      result += std::to_string(static_cast<t_uint32>(v->type));
      result += ")>";

      return result;
    },
    /* possible_values */
    [] (AbstractValue *v, RelationToOtherVariables, Properties) {
      FATAL(("override possible_values!"));
      return PossibleValueList();
    }
  };

  _AbstractValue();

  /* constructors */
  static AbstractValue CreateInstance();
  static AbstractValue CreateNull();
  static AbstractValue CreateProxy();
  static AbstractValue CreateVariable(MetaAPI_Variable *v, t_uint8 modifiers, std::string member, MetaAPI_ImplementationValue *array_index);

  /* helpers */
  std::string Print();
  bool IsNull() { return type == Type::MetaAPI_Null; }
  bool IsVariable() { return type == Type::MetaAPI_Variable; }

  struct ProduceValueInfo {
    /* inputs */
    t_reg destination_register;
    t_bbl *in_bbl;
    t_ins *after_ins;
    t_int32 stack_index;
    t_int32 first_local_stack_index;
    MetaAPI_ImplementationValue *array_index;
    t_reg helper_register;

    /* outputs */
    t_regset overwritten_registers;

    ProduceValueInfo() {
      in_bbl = NULL;
      after_ins = NULL;
      stack_index = -1;
      first_local_stack_index = 0;
      array_index = NULL;
      helper_register = REG_NONE;

      overwritten_registers = RegsetNew();
    }
  };

  void ProduceValueInRegister(MetaAPI_Instance *instance, bool generate_new, ProduceValueInfo *info, std::function<void(AbstractValue&)> modifier = nullptr);
  void ProduceValueInSection(t_cfg *cfg, t_section *section);

  template<typename T>
  static void* pack(T *x) { return static_cast<void*>(x); }

  template<typename T>
  static T* unpack(void *x) { return static_cast<T*>(x); }

  /* used to generate a new random value when no meta-API values are defined */
  Contents Generate();

  bool Equals(AbstractValue *other);
  bool LessThan(AbstractValue *other);

  PossibleValueList PossibleValues(RelationToOtherVariables relations, Properties properties);
};

static inline
void PossibleValueListAdd(AbstractValue::PossibleValueList& l, AbstractValue::Contents x) {
  l.insert(x);
}

#endif
