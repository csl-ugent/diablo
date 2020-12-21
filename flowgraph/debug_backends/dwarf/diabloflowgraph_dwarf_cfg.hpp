#ifndef DIABLOFLOWGRAPH_DWARF_CFG_HPP
#define DIABLOFLOWGRAPH_DWARF_CFG_HPP

#include "diabloflowgraph_dwarf.h"

#include <string>
#include <vector>

struct Datatype;

struct DwarfFunctionDefinition {
  bool unparsed;
  bool adheres_calling_conventions;
  bool is_static;
  bool is_declaration;
  DwarfInlineCode inlined;
  t_address offset;

  Datatype *return_datatype;
  std::string name;
  std::string linkage_name;
  struct Parameter {
    Datatype *datatype;
    std::string name;
    bool artificial;
  };
  std::vector<Parameter *> parameters;

  std::string to_string();
};

struct Datatype {
  enum class Type {
    Pointer,      /*0*/
    Typedef,      /*1*/
    BaseType,     /*2*/
    Structure,    /*3*/
    Subroutine,   /*4*/
    Const,        /*5*/
    Reference,    /*6*/
    Class,        /*7*/
    Void,         /*8*/
    Array,        /*9*/
    Union,        /*10*/
    Enum,         /*11*/
    UnspecifiedParameters,  /*12*/
    Volatile,     /*13*/
    Restrict,     /*14*/
    UnspecifiedType, /*15*/
    TemplateTypeParameter, /*16*/
    ImportedDeclaration, /*17*/
    TemplateTemplateParam, /* 18 */
    PointerToMember /* 19 */
  } type;

  static constexpr Datatype *RESOLVING = reinterpret_cast<Datatype *>(0x1);

  struct TypeNameTuple {
    Datatype *datatype;
    std::string *name;
  };

  struct NameConstTuple {
    std::string *name;
    t_uint64 value;
  };

  union {
    struct {
      Datatype *datatype;
    } pointer_data, reference_data, const_data, array_data, volatile_data, restrict_data, imported_declaration_data, pointer_to_member_data;

    TypeNameTuple typedef_data, template_type_parameter_data;

    struct {
      std::string *name;
    } base_type_data, unspecified_type_data, template_template_param_data;

    struct {
      std::string *name;
      std::vector<TypeNameTuple> *members;
      std::vector<DwarfFunctionDefinition *> *methods;
      std::vector<Datatype *> *inheritance;
      size_t size;
    } structure_type_data;

    struct {
      Datatype *return_datatype;
      std::string *name;
      std::vector<Datatype *> *parameters;
    } subroutine_data;

    struct {
      Datatype *datatype;
      std::string *name;
      std::vector<NameConstTuple> *values;
    } enum_data;
  };

  Datatype() {}

  std::string to_string(bool verbose = false);
};

DwarfFunctionDefinition *GetDwarfFunctionSignature(t_function *f);

#endif
