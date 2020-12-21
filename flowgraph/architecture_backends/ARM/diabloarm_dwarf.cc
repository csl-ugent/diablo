#include <diabloarm_dwarf.hpp>
#include <diabloobject.hpp>

#include <map>
using namespace std;

enum class RegisterType {
  None,       /*0*/
  Integer,    /*1*/
  FloatSingle,/*2*/
  FloatDouble,/*3*/
  FloatQuad
};

struct RegNTuple {
  RegisterType type;
  int n;
};
map<Datatype *, RegNTuple> resolved_datatypes;

/* function name to liveness information */
struct RegsetTuple {
  t_regset args;
  t_regset rets;
};
typedef map<string, RegsetTuple> FunctionLivenessInfo;
static map<string, FunctionLivenessInfo> all_library_info;

struct EquivalentInfo {
  map<t_uint32, vector<string>> address_to_symbol;
  map<string, t_uint32> symbol_to_address;
};
static map<string, EquivalentInfo> all_library_equivs;

bool is_syscall(string s) {
  if (   !s.compare("chroot")
      || !s.compare("epoll_create1")
      || !s.compare("epoll_ctl")
      || !s.compare("fchmod")
      || !s.compare("fchown")
      || !s.compare("fchownat")
      || !s.compare("fgetxattr")
      || !s.compare("flistxattr")
      || !s.compare("flock")
      || !s.compare("fremovexattr")
      || !s.compare("fsetxattr")
      || !s.compare("getitimer")
      || !s.compare("getpgrp")
      || !s.compare("getppid")
      || !s.compare("getresgid")
      || !s.compare("getresuid")
      || !s.compare("getxattr")
      || !s.compare("lchown")
      || !s.compare("lgetxattr")
      || !s.compare("link")
      || !s.compare("linkat")
      || !s.compare("listxattr")
      || !s.compare("llistxattr")
      || !s.compare("lremovexattr")
      || !s.compare("lsetxattr")
      || !s.compare("mkdirat")
      || !s.compare("personality")
      || !s.compare("prctl")
      || !s.compare("removexattr")
      || !s.compare("sched_get_priority_min")
      || !s.compare("sched_get_priority_max")
      || !s.compare("sched_getparam")
      || !s.compare("sched_getscheduler")
      || !s.compare("sched_rr_get_interval")
      || !s.compare("sendfile64")
      || !s.compare("setxattr")
      || !s.compare("socketpair")
      || !s.compare("shutdown")
      || !s.compare("symlink")
      || !s.compare("symlinkat")
      || !s.compare("sync")
      || !s.compare("syscall")
      || !s.compare("umask")
      || !s.compare("unlinkat")
      )
    return true;

  return false;
}

static
void split(string s, vector<string>& v)
{
  string delimiter = ":";

  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
      token = s.substr(0, pos);
      v.push_back(token);
      s.erase(0, pos + delimiter.length());
  }

  v.push_back(s);
}

/* fixed interface in ARM EABI specification
 * https://static.docs.arm.com/ihi0043/d/IHI0043D_rtabi.pdf */
bool ArgRetEabi(string name, t_regset& args, t_regset& rets) {
  VERBOSE(1, ("getting information from EABI for '%s'", name.c_str()));

  args = RegsetNew();
  rets = RegsetNew();

  if (!name.compare("__aeabi_uidivmod")
      || !name.compare("__aeabi_idivmod")
      || !name.compare("__aeabi_idiv")
      || !name.compare("__aeabi_idiv0")
      || !name.compare("__aeabi_uidiv")
      || !name.compare("__aeabi_uidivmod")
      || !name.compare("__aeabi_ldivmod")
      || !name.compare("__aeabi_ldiv0")
      || !name.compare("__aeabi_uldivmod")) {
    /* no arguments */
    /* no returns */
    return true;
  }

  if (!name.compare("__aeabi_ul2d")
      || !name.compare("__aeabi_l2d")
      || !name.compare("__aeabi_i2d")
      || !name.compare("__aeabi_ui2d")) {
    /* no arguments */
    RegsetSetAddReg(rets, ARM_REG_S0);
    RegsetSetAddReg(rets, ARM_REG_S1);

    return true;
  }

  if (!name.compare("__aeabi_drsub")
      || !name.compare("__aeabi_dsub")
      || !name.compare("__aeabi_dadd")) {
    RegsetSetAddReg(args, ARM_REG_S0);
    RegsetSetAddReg(args, ARM_REG_S1);
    RegsetSetAddReg(args, ARM_REG_S2);
    RegsetSetAddReg(args, ARM_REG_S3);

    RegsetSetAddReg(rets, ARM_REG_S0);
    RegsetSetAddReg(rets, ARM_REG_S1);

    return true;
  }

  if (!name.compare("__aeabi_d2lz")
      || !name.compare("__aeabi_d2ulz")) {
    RegsetSetAddReg(args, ARM_REG_S0);
    RegsetSetAddReg(args, ARM_REG_S1);
    /* no returns */

    return true;
  }

  if (!name.compare("__aeabi_f2d")) {
    RegsetSetAddReg(args, ARM_REG_S0);

    RegsetSetAddReg(rets, ARM_REG_S0);
    RegsetSetAddReg(rets, ARM_REG_S1);

    return true;
  }

  if (!name.compare("__aeabi_ul2f")
      || !name.compare("__aeabi_ui2f")
      || !name.compare("__aeabi_i2f")
      || !name.compare("__aeabi_l2f")) {
    /* no arguments */
    RegsetSetAddReg(rets, ARM_REG_S0);

    return true;
  }

  if (!name.compare("__aeabi_frsub")
      || !name.compare("__aeabi_fsub")
      || !name.compare("__aeabi_fadd")) {
    RegsetSetAddReg(args, ARM_REG_S0);
    RegsetSetAddReg(args, ARM_REG_S1);

    RegsetSetAddReg(rets, ARM_REG_S0);

    return true;
  }

  if (!strncmp(name.c_str(), "__aeabi_", strlen("__aeabi_")))
    FATAL(("unhandled EABI function '%s'", name.c_str()));

  return false;
}

void ArgRetDynLib(DyncallFunctionInfo info, t_regset& args, t_regset& rets) {
  VERBOSE(1, ("getting information for '%s'@'%s' in library '%s'", info.name.c_str(), info.version.c_str(), info.library.c_str()));

  args = RegsetNew();
  rets = RegsetNew();

  if (all_library_info.find(info.library) == all_library_info.end()) {
    /* only read each file once */
    static t_string liveness_directory = NULL;
    static vector<string> liveness_directory_list;

    if (!liveness_directory) {
      DiabloBrokerCall("EnhancedLivenessDirectory", &liveness_directory);
      split(liveness_directory, liveness_directory_list);
    }

    string lfilename;
    string efilename;
    ifstream linput;
    ifstream einput;

    for (auto p : liveness_directory_list) {
      lfilename = p + "/" + info.library + ".liveness";
      efilename = p + "/" + info.library + ".equivalents";

      linput = ifstream(lfilename);
      if (!linput.is_open())
        continue;

      einput = ifstream(efilename);
      ASSERT(einput.is_open(), ("can't open file '%s'", efilename.c_str()));

      break;
    }

    ASSERT(linput.is_open(), ("can't find liveness file for '%s' in directories %s", info.library.c_str(), liveness_directory));

    string line;

    VERBOSE(0, ("reading file '%s'", lfilename.c_str()));
    FunctionLivenessInfo info_for_file;
    while (getline(linput, line)) {
      vector<string> components;
      split(line, components);

      string name = components[0];

      RegsetTuple data;
      data.args = RegsetDeserialize(const_cast<t_string>(components[1].c_str()));
      data.rets = RegsetDeserialize(const_cast<t_string>(components[2].c_str()));

      /* TODO: duplicate entries are possible, check for equal regsets */
      info_for_file[name] = data;
    }

    all_library_info[info.library] = info_for_file;

    /* read equivalent symbol names */
    VERBOSE(0, ("reading file '%s'", efilename.c_str()));
    EquivalentInfo equiv_info_for_library;
    while (getline(einput, line)) {
      vector<string> components;
      split(line, components);

      t_uint32 symbol_address = StringToUint32(components[0].c_str(), 0);
      string symbol_name = components[1];

      if (equiv_info_for_library.address_to_symbol.find(symbol_address) == equiv_info_for_library.address_to_symbol.end())
        equiv_info_for_library.address_to_symbol[symbol_address] = vector<string>();
      equiv_info_for_library.address_to_symbol[symbol_address].push_back(symbol_name);

      equiv_info_for_library.symbol_to_address[symbol_name] = symbol_address;
    }

    all_library_equivs[info.library] = equiv_info_for_library;
  }

  /* look up the information */
  FunctionLivenessInfo library_info = all_library_info[info.library];

  /* canonical name */
  EquivalentInfo library_equivs = all_library_equivs[info.library];

  auto found_sym_noversion = library_equivs.symbol_to_address.find(info.name);

  string versioned_name = info.name + "@@" + info.version;
  auto found_sym_version = library_equivs.symbol_to_address.find(versioned_name);

  ASSERT(!((found_sym_noversion == library_equivs.symbol_to_address.end()) && (found_sym_version == library_equivs.symbol_to_address.end())), ("can't find '%s' (versioned '%s') in library '%s'", info.name.c_str(), versioned_name.c_str(), info.library.c_str()));

  auto found_sym = (found_sym_noversion != library_equivs.symbol_to_address.end()) ? found_sym_noversion : found_sym_version;

  RegsetTuple data;
  bool found = false;
  for (string n : library_equivs.address_to_symbol[(*found_sym).second]) {
    if (library_info.find(n) != library_info.end()) {
      found = true;
      data = library_info[n];
      break;
    }
  }

  if (!found)
    found = ArgRetEabi(info.name, data.args, data.rets);

  if (found) {
    args = data.args;
    rets = data.rets;
  }
  else {
    /* apparently no DWARF information is associated with some library functions */
    if (!info.library.compare("libc.so.6")
        && is_syscall(info.name)) {
      /* no float arguments/return values */
    }
    else {
      // DEBUG(("no data found for '%s' in library '%s'", info.name.c_str(), info.library.c_str()));
      ASSERT(found, ("no data found for '%s' in library '%s'", info.name.c_str(), info.library.c_str()));
    }
  }
}

static
RegisterType regtype_for_datatype(Datatype *dt, int& regcount) {
  /* default case: only one such register is needed */
  regcount = 1;

  if (!dt) {
    /* void */
    return RegisterType::None;
  }

  switch(dt->type) {
  case Datatype::Type::Reference:
  case Datatype::Type::Pointer:
  case Datatype::Type::Enum:
  case Datatype::Type::Class:
    return RegisterType::Integer;

  case Datatype::Type::BaseType: {
    string name = *(dt->base_type_data.name);
    if (/* bool */
           !name.compare("bool")
        || !name.compare("_Bool")

        /* character */
        || !name.compare("wchar_t")
        || !name.compare("char32_t")
        || !name.compare("char16_t")

        /* 8-bit */
        || !name.compare("char")
        || !name.compare("signed char")
        || !name.compare("unsigned char")

        /* 16-bit */
        || !name.compare("short int")
        || !name.compare("short unsigned int")
        /* fixed-point */
        || !name.compare("short _Accum")
        || !name.compare("unsigned short _Accum")
        || !name.compare("short _Fract")
        || !name.compare("unsigned short _Fract")

        /* 32-bit */
        || !name.compare("int")
        || !name.compare("long int")
        || !name.compare("unsigned int")
        || !name.compare("long unsigned int")
        /* fixed-point */
        || !name.compare("_Accum")
        || !name.compare("unsigned _Accum")
        || !name.compare("long _Accum")
        || !name.compare("unsigned long _Accum")
        || !name.compare("_Fract")
        || !name.compare("unsigned _Fract")
        || !name.compare("long _Fract")
        || !name.compare("unsigned long _Fract")

        /* 64-bit */
        || !name.compare("long long int")
        || !name.compare("long long unsigned int")
        /* fixed-point */
        || !name.compare("long long _Accum")
        || !name.compare("unsigned long long _Accum")
        || !name.compare("long long _Fract")
        || !name.compare("unsigned long long _Fract"))
      return RegisterType::Integer;

    if (!name.compare("float"))
      return RegisterType::FloatSingle;

    if (!name.compare("complex float")) {
      regcount = 2;
      return RegisterType::FloatSingle;
    }

    if (!name.compare("double")
        || !name.compare("long double"))
      return RegisterType::FloatDouble;

    if (!name.compare("complex double")
        || !name.compare("complex long double")) {
      regcount = 2;
      return RegisterType::FloatDouble;
    }

    FATAL(("unsupported basetype '%s'", name.c_str()));
  } break;

  case Datatype::Type::Typedef:
    return regtype_for_datatype(dt->typedef_data.datatype, regcount);

  case Datatype::Type::Structure: {
    /* need to have homogeneous aggregate */
    RegisterType type;

    for (auto m : *(dt->structure_type_data.members)) {
      if (m.datatype->type != Datatype::Type::BaseType)
        return RegisterType::Integer;

      type = regtype_for_datatype(m.datatype, regcount);
      if (type == RegisterType::Integer)
        return RegisterType::Integer;
    }

    /* empty structs */
    if (dt->structure_type_data.members->size() == 0)
      return RegisterType::Integer;

    /* A Homogeneous Aggregate with a Base Type of a single- or double-precision floating-point type with one to four Elements */
    auto is_homo_agg = [] (Datatype *dt, RegisterType& reg_type, int& regcount) {
      /* single- or double-precision */
      Datatype *first_dt = dt->structure_type_data.members->at(0).datatype;

      regcount = 1;
      reg_type = regtype_for_datatype(first_dt, regcount);

      if (regcount > 1)
        return false;

      if ((reg_type != RegisterType::FloatSingle)
          && (reg_type != RegisterType::FloatDouble))
        return false;

      if (dt->structure_type_data.members->size() > 4)
        return false;

      for (auto m : *(dt->structure_type_data.members)) {
        if (first_dt != m.datatype)
          return false;
      }

      regcount = dt->structure_type_data.members->size();

      return true;
    };

    if (is_homo_agg(dt, type, regcount)) {
      DEBUG(("homo aggregate '%s' with type %d in %d registers ", dt->to_string(true).c_str(), type, regcount));
      return type;
    }

    FATAL(("unsupported struct '%s'", dt->to_string(true).c_str()));
  } break;

  case Datatype::Type::Const:
    return regtype_for_datatype(dt->const_data.datatype, regcount);

  case Datatype::Type::Restrict:
    return regtype_for_datatype(dt->restrict_data.datatype, regcount);

  case Datatype::Type::PointerToMember:
    return regtype_for_datatype(dt->pointer_to_member_data.datatype, regcount);

  case Datatype::Type::Union:
    return RegisterType::Integer;

  case Datatype::Type::UnspecifiedType: {
    string name = *(dt->unspecified_type_data.name);
    if (!name.compare("decltype(nullptr)"))
      return RegisterType::Integer;

    FATAL(("unsupported unspecified type '%s'", name.c_str()));
  } break;

  case Datatype::Type::Subroutine:
    return RegisterType::Integer;

  default:
    FATAL(("unknown %s", dt->to_string(true).c_str()));
  }
};

void DwarfArmFunctionFloatArgRetRegsets(DwarfFunctionDefinition *def, t_regset *p_args, t_regset *p_rets) {
  t_regset args = RegsetNew();
  t_regset rets = RegsetNew();

  /* all floating-point argument registers */
  t_regset float_argument_registers = arm_description.flt_registers;
  RegsetSetIntersect(float_argument_registers, arm_description.argument_regs);

  t_regset free_float_argument_registers = float_argument_registers;

  /* find a sequence of 'n' Sx registers, aligned to 'alignment' */
  auto allocate_registers = [&free_float_argument_registers] (int n, int alignment) {
    t_reg result = ARM_REG_NONE;

    t_reg r;
    REGSET_FOREACH_REG(free_float_argument_registers, r) {
      if (((r - ARM_REG_S0) % alignment) != 0) {
        continue;
      }

      bool ok = true;
      t_reg it = r+1;
      while ((it - r) < n) {
        if (it > ARM_REG_S15)
          break;

        if (!RegsetIn(free_float_argument_registers, it)) {
          ok = false;
          break;
        }

        it++;
      }

      if (ok) {
        result = r;
        break;
      }
    }

    if (result > ARM_REG_S15)
      result = ARM_REG_NONE;
    else {
      t_reg it = result;
      while ((it <= ARM_REG_S15) && ((it - result) < n)) {
        RegsetSetSubReg(free_float_argument_registers, it);
        it++;
      }
    }

    return result;
  };

  /* floating-point arguments? */
  for (auto param : def->parameters) {
    RegisterType reg_type;
    int n;

    /* resolve the register type for this argument */
    auto found = resolved_datatypes.find(param->datatype);
    if (found != resolved_datatypes.end()) {
      reg_type = found->second.type;
      n = found->second.n;
    }
    else {
      reg_type = regtype_for_datatype(param->datatype, n);
      resolved_datatypes[param->datatype] = RegNTuple{reg_type, n};
    }

    /* allocate register */
    switch (reg_type) {
    case RegisterType::FloatSingle:
      allocate_registers(n, 1);
      break;

    case RegisterType::FloatDouble:
      allocate_registers(2*n, 2);
      break;

    case RegisterType::FloatQuad:
      allocate_registers(4*n, 4);
      break;

    default:
      /* do_nothing */
      break;
    }
  }

  RegsetSetInvers(free_float_argument_registers);
  args = RegsetIntersect(free_float_argument_registers, float_argument_registers);

  /* floating-point return value? */
  int n;
  RegisterType return_reg = regtype_for_datatype(def->return_datatype, n);

  switch (return_reg) {
  case RegisterType::FloatSingle:
    break;

  case RegisterType::FloatDouble:
    n *= 2;
    break;

  case RegisterType::FloatQuad:
    n *= 4;
    break;

  default:
    /* don't add any register */
    n = 0;
    break;
  }
  for (int i = 0; i < n; i++) {
    RegsetSetAddReg(rets, ARM_REG_S0 + i);
  }

  *p_args = args;
  *p_rets = rets;
}

void DwarfArmSpecificStuff(t_cfg *cfg) {
  /* process all the functions in the CFG */
  t_architecture_description *desc = CFG_DESCRIPTION(cfg);

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    t_regset args = RegsetNew();
    t_regset rets = RegsetNew();

    DwarfFunctionDefinition *def = GetDwarfFunctionSignature(fun);
    t_function *also_set = NULL;

    /* some functions don't have any DWARF information */
    if (!def) {
      if (FUNCTION_NAME(fun)
          && ArgRetEabi(string(FUNCTION_NAME(fun)), args, rets)) {
        /* 'args' and 'rets' have been set by the (succeeded) call to ArgRetEabi */
      }
      else if (FUNCTION_IS_HELL(fun)
                && (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)) == BBL_CH_DYNCALL)) {
        /* functions in dynamic libraries */
        string name = string(FUNCTION_NAME(fun)).substr(strlen("--DYNCALL-HELL--"));

        t_bool found_dynamic = false;
        t_bool found_exported = false;

        DyncallFunctionInfo info;
        DiabloBrokerCall("GetDynamicVersionInformation", name.c_str(), &info, &found_dynamic);
        if (found_dynamic) {
          if (info.index == SYMBOL_VERSION_GLOBAL) {
            /* the symbol is called through the PLT,
             * though it is defined in the library itself. */
            t_function *impl = GetFunctionByName(cfg, name.c_str());
            ASSERT(impl, ("can't find global function @F with name '%s'", fun, name.c_str()));
            def = GetDwarfFunctionSignature(impl);
          }
          else if (info.library.size() == 0) {
            /* the only case we observed this is for __gmon_start__ */
            VERBOSE(1, ("no DWARF information for function @F", fun));
          }
          else
            ArgRetDynLib(info, args, rets);
        }
        else {
          ExportedFunctionInfo info;
          DiabloBrokerCall("GetExportedVersionInformation", name.c_str(), &info, &found_exported);

          if (found_exported) {
            VERBOSE(1, ("found exported information for function @F", fun));

            /* look up local dwarf information for this HELL function */
            also_set = fun;
            fun = GetFunctionByName(cfg, name.c_str());
            def = GetDwarfFunctionSignature(fun);
            ASSERT(fun, ("can't find exported function with name '%s' for @F", name.c_str(), also_set));
          }
        }

        ASSERT(found_dynamic || found_exported, ("can't handle @F", fun));
      }
      else {
        if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun))
            || (fun == CFG_LONGJMP_HELL_FUNCTION(cfg))
            || (fun == CFG_HELL_FUNCTION(cfg))
            || (fun == CFG_WRAP_FUNCTION(cfg))) {
          /* all float arguments/return values assumed */
          args = RegsetIntersect(desc->flt_registers, desc->argument_regs);
          rets = RegsetIntersect(desc->flt_registers, desc->return_regs);
        }
        else if (fun == CFG_SWI_HELL_FUNCTION(cfg)
                  || FunctionHasNoDwarf(fun)) {
          /* no float arguments/return values assumed */
        }
        else
          FATAL(("unhandled @F", fun));
      }
    }

    /* 'def' may be set in the previous block */
    if (def) {
      VERBOSE(1, ("look up DWARF information for @F / %s", fun, def->to_string().c_str()));

      DwarfArmFunctionFloatArgRetRegsets(def, &args, &rets);
    }

    FUNCTION_SET_FLOAT_ARG_REGS(fun, args);
    FUNCTION_SET_FLOAT_RET_REGS(fun, rets);

    if (also_set) {
      FUNCTION_SET_FLOAT_ARG_REGS(fun, args);
      FUNCTION_SET_FLOAT_RET_REGS(fun, rets);
    }

    string s = def ? def->to_string() : "<unknown signature>";
    if (!(RegsetIsEmpty(args) && RegsetIsEmpty(rets)))
      VERBOSE(1, ("DWARF_FLOAT(%d/%d) arg[@X] return[@X]: %s: @F @G", RegsetCountRegs(args), RegsetCountRegs(rets), CPREGSET(cfg, args), CPREGSET(cfg, rets), s.c_str(), fun, BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun))));
    else
      VERBOSE(1, ("DWARF_INTEGER(%d/%d) arg[@X] return[@X]: %s: @F @G", RegsetCountRegs(args), RegsetCountRegs(rets), CPREGSET(cfg, args), CPREGSET(cfg, rets), s.c_str(), fun, BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun))));
  }
}

/* we would expect each function in the binary to have DWARF information
 * associated with it. In some cases, however, this seems not the case.
 * The EABI functions, for example, are written in Assembly language and
 * hence no DWARF information is generated for them. */
void DwarfArmFunctionHasInfo(t_function *fun, t_bool *result) {
  *result = true;

  /* ARM EABI functions */
  if (StringPatternMatch("__aeabi_*", FUNCTION_NAME(fun)))
    *result = false;

  /* division helper functions */
  else if (!strcmp(FUNCTION_NAME(fun), ".divsi3_skip_div0_test")
            || !strcmp(FUNCTION_NAME(fun), "__udivsi3"))
    *result = false;
}

struct ArgumentLocation {
  enum class Type {
    Register,
    Memory
  } type;

  /* single register: b == ARM_REG_NONE */
  /* multiple registers: a-b */
  t_reg a, b;
};

enum class ArmFundamentalDatatype {
  /* integral */
  UnsignedByte,
  SignedByte,
  UnsignedHalfWord,
  SignedHalfWord,
  UnsignedWord,
  SignedWord,
  UnsignedDoubleWord,
  SignedDoubleWord,

  /* floating point */
  HalfPrecision,
  SinglePrecision,
  DoublePrecision,

  /* containerized vector */
  Vector64,
  Vector128,

  /* pointer */
  DataPointer,
  CodePointer
};

static
void fundamental_datatype_bytesize_alignment(ArmFundamentalDatatype x, int& size, int& alignment) {
  switch (x) {
  case ArmFundamentalDatatype::UnsignedByte:
  case ArmFundamentalDatatype::SignedByte:
    size = 1;
    alignment = 1;
    break;
  
  case ArmFundamentalDatatype::UnsignedHalfWord:
  case ArmFundamentalDatatype::SignedHalfWord:
  case ArmFundamentalDatatype::HalfPrecision:
    size = 2;
    alignment = 2;
    break;
  
  case ArmFundamentalDatatype::UnsignedWord:
  case ArmFundamentalDatatype::SignedWord:
  case ArmFundamentalDatatype::SinglePrecision:
  case ArmFundamentalDatatype::DataPointer:
  case ArmFundamentalDatatype::CodePointer:
    size = 4;
    alignment = 4;
    break;
  
  case ArmFundamentalDatatype::UnsignedDoubleWord:
  case ArmFundamentalDatatype::SignedDoubleWord:
  case ArmFundamentalDatatype::DoublePrecision:
  case ArmFundamentalDatatype::Vector64:
    size = 8;
    alignment = 8;
    break;
  
  case ArmFundamentalDatatype::Vector128:
    size = 16;
    alignment = 8;
    break;

  default:
    FATAL(("unexpected %d", x));
  }
}

static
bool is_fundamental_datatype(Datatype *dt, ArmFundamentalDatatype& type, int& size, int& alignment) {
  return false;
}

static
bool is_composite_type(Datatype *dt, int& size) {
  return false;
}

static
bool is_homogeneous_aggregate(Datatype *dt, ArmFundamentalDatatype& base_type, int& nr_elements) {
  return false;
}

static
ArgumentLocation abi_return_location(Datatype *dt) {
  ArgumentLocation loc;

  loc.type = ArgumentLocation::Type::Register;
  loc.a = ARM_REG_NONE;
  loc.b = ARM_REG_NONE;

  ArmFundamentalDatatype type;
  int size, alignment;
  if (is_fundamental_datatype(dt, type, size, alignment)) {
    if (type == ArmFundamentalDatatype::HalfPrecision)
      loc.a = ARM_REG_R0;
    else if (size <= 4)
      loc.a = ARM_REG_R0;
    else if (size == 8) {
      loc.a = ARM_REG_R0;
      loc.b = ARM_REG_R1;
    }
    else if (type == ArmFundamentalDatatype::Vector128) {
      loc.a = ARM_REG_R0;
      loc.b = ARM_REG_R3;
    }
    else
      FATAL(("unhandled '%s'", dt->to_string().c_str()));
  }
  else if (is_composite_type(dt, size)) {
    if (size <= 4)
      loc.a = ARM_REG_R0;
    else {
      loc.type = ArgumentLocation::Type::Memory;
      loc.a = ARM_REG_R0;
    }
  }
  else
    FATAL(("don't know how to handle this type '%s'", dt->to_string().c_str()));
  
  return loc;
}

static
bool is_vfp_cprc(Datatype *dt, int& nr_single_registers, int& register_alignment) {
  ArmFundamentalDatatype type;
  int size, alignment;
  if (is_fundamental_datatype(dt, type, size, alignment)) {
    switch (type) {
    case ArmFundamentalDatatype::HalfPrecision:
    case ArmFundamentalDatatype::SinglePrecision:
      nr_single_registers = 1;
      register_alignment = 1;
      return true;

    case ArmFundamentalDatatype::DoublePrecision:
    case ArmFundamentalDatatype::Vector64:
      nr_single_registers = 2;
      register_alignment = 2;
      return true;

    case ArmFundamentalDatatype::Vector128:
      nr_single_registers = 4;
      register_alignment = 4;
      return true;
    
    default:;
    }

    return false;
  }
  else if (is_homogeneous_aggregate(dt, type, size)) {
    switch (type) {
    case ArmFundamentalDatatype::SinglePrecision:
      nr_single_registers = size;
      register_alignment = 1;
      return (1 <= size) && (size <= 4);

    case ArmFundamentalDatatype::DoublePrecision:
    case ArmFundamentalDatatype::Vector64:
      nr_single_registers = size * 2;
      register_alignment = 2;
      return (1 <= size) && (size <= 4);

    case ArmFundamentalDatatype::Vector128:
      nr_single_registers = size * 4;
      register_alignment = 4;
      return (1 <= size) && (size <= 4);

    default:;
    }

    return false;
  }

  return false;
}

void ArmAssignFunctionArguments(t_function *fun) {
  t_cfg *cfg = FUNCTION_CFG(fun);

  /* according to the Procedure Call Standard (aapcs32) */
  bool have_vfp = true;

  /* ================================================================================================================================================================================
   * Stage A -- Initialization */
  /* This stage is performed exactly once, before processing of the arguments commences. */

  /* A.1 The Next Core Register Number (NCRN) is set to r0. */
  t_reg NCRN = ARM_REG_R0;

  /* A.2.cp Co-processor argument register initialization is performed. */
  t_regset free_float_arg_regs = RegsetNew();
  if (have_vfp) {
    /* A.2.vfp The floating point argument registers are marked as unallocated. */
    free_float_arg_regs = RegsetIntersect(CFG_DESCRIPTION(cfg)->argument_regs, CFG_DESCRIPTION(cfg)->flt_registers);
  }

  /* A.3 The next stacked argument address (NSAA) is set to the current stack-pointer value (SP). */
  /* here we encode it as the offset to the stack pointer */
  t_address NSAA = 0;

  /* A.4 If the subroutine is a function that returns a result in memory,
   *     then the address for the result is placed in r0 and the NCRN is set to r1. */
  ArgumentLocation return_loc = abi_return_location(NULL);
  if (return_loc.type == ArgumentLocation::Type::Memory) {
    //TODO: place address for return value in R0
    NCRN = ARM_REG_R1;
  }

  /* ================================================================================================================================================================================
   * Stage B -- Pre-padding and extension of arguments */
  /* For each argument in the list the first matching rule from the following list is applied */
  for (;;) {
    Datatype *arg_dt = NULL;

    /* B.1 If the argument is a Composite Type whose size cannot be statically determined by both the caller and callee,
     *     the argument is copied to memory and the argument is replaced by a pointer to the copy. */

    /* B.2 If the argument is an integral Fundamental Data Type that is smaller than a word,
     *     then it is zero- or sign-extended to a full word and its size is set to 4 bytes. If the argument is a Half-precision
     *     Floating Point Type its size is set to 4 bytes as if it had been copied to the least significant bits of a 32-bit
     *     register and the remaining bits filled with unspecified values. */

    /* B.3.cp If the argument is a CPRC then any preparation rules for that co-processor register class are applied. */
    if (have_vfp) {
      /* B.3.vfp Nothing to do. */
    }

    /* B.4 If the argument is a Composite Type whose size is not a multiple of 4 bytes, then its size is rounded up to the nearest multiple of 4. */

    /* B.5 If the argument is an alignment adjusted type its value is passed as a copy of the actual
     *     value. The copy will have an alignment defined as follows.
     *     - For a Fundamental Data Type, the alignment is the natural alignment of that type, after any promotions.
     *     - For a Composite Type, the alignment of the copy will have 4-byte alignment if its
     *       natural alignment is <= 4 and 8-byte alignment if its natural alignment is >= 8
     *     The alignment of the copy is used for applying marshaling rules. */
  }

  /* ================================================================================================================================================================================
   * Stage C -- Assignment of arguments to registers and stack */
  /* For each argument in the list the following rules are applied in turn until the argument has been allocated. */
  for (;;) {
    Datatype *arg_dt = NULL;

    /* C.1.cp If the argument is a CPRC and there are sufficient unallocated co-processor registers of the appropriate class,
     *        the argument is allocated to co-processor registers. */

    /* C.2.cp If the argument is a CPRC then any co-processor registers in that class that are unallocated are marked as unavailable.
     *        The NSAA is adjusted upwards until it is correctly aligned for the argument and the argument is copied to the memory at the adjusted NSAA.
     *        The NSAA is further incremented by the size of the argument. The argument has now been allocated. */
    if (have_vfp) {
      /* C.1.vfp If the argument is a VFP CPRC and there are sufficient consecutive VFP registers of the appropriate type unallocated then
       *         the argument is allocated to the lowest-numbered sequence of such registers. */

      /* C.2.vfp If the argument is a VFP CPRC then any VFP registers that are unallocated are marked as unavailable.
       *         The NSAA is adjusted upwards until it is correctly aligned for the argument and the argument is copied to the stack at the adjusted NSAA.
       *         The NSAA is further incremented by the size of the argument. The argument has now been allocated. */
      int nr_single_registers, register_alignment;
      if (is_vfp_cprc(arg_dt, nr_single_registers, register_alignment)) {

        /* the argument has been allocated */
        continue;
      }
    }

    /* C.3 If the argument requires double-word alignment (8-byte), the NCRN is rounded up to
     *     the next even register number. */
    
    /* C.4 If the size in words of the argument is not more than r4 minus NCRN, the argument is copied into core registers, starting at the NCRN.
     *     The NCRN is incremented by the number of registers used. Successive registers hold the parts of the argument they would hold if its
     *     value were loaded into those registers from memory using an LDM instruction. The argument has now been allocated. */

    /* C.5 If the NCRN is less than r4 and the NSAA is equal to the SP, the argument is split between core registers and the stack.
     *     The first part of the argument is copied into the core registers starting at the NCRN up to and including r3.
     *     The remainder of the argument is copied onto the stack, starting at the NSAA. The NCRN is set to r4 and the NSAA is incremented by the
     *     size of the argument minus the amount passed in registers. The argument has now been allocated. */

    /* C.6 The NCRN is set to r4. */
    NCRN = ARM_REG_R4;

    /* C.7 If the argument required double-word alignment (8-byte), then the NSAA is rounded up to the next double-word address. */

    /* C.8 The argument is copied to memory at the NSAA. The NSAA is incremented by the size of the argument. */
  }
}
