#include "diabloflowgraph_dwarf.hpp"

#include <map>
#include <string>
#include <vector>

#include <cxxabi.h>

using namespace std;

#define DEBUG_DWARF_CFG(x)
// #define DEBUG_DWARF_CFG(x) DEBUG(x)
#define DEBUG_DWARF_CFG2(x)
// #define DEBUG_DWARF_CFG2(x) DEBUG(x)

FUNCTION_DYNAMIC_MEMBER(dwarf_signature, DWARF_SIGNATURE, DwarfSignature, DwarfFunctionDefinition *, NULL);

DwarfFunctionDefinition *GetDwarfFunctionSignature(t_function *f) {
  return FUNCTION_DWARF_SIGNATURE(f);
}

static map<t_address, Datatype *> resolved_types;
static Datatype *void_datatype = NULL;
static string *anonymous_name = NULL;
static map<t_address, DwarfFunctionDefinition *> dwarf_functions;
static map<t_address, DwarfFunctionDefinition::Parameter *> dwarf_function_parameters;
static vector<t_address> to_check;
static map<t_address, DwarfFunctionDefinition *> cfg_address_to_dwarf_function;
static vector<DwarfFunctionDefinition *> dwarf_function_without_cfg_address;
static vector<DwarfAbbrevTableEntry *> member_functions;
static map<t_address, DwarfAbbrevTableEntry *> address_to_entry;

static Datatype *GetDatatype(DwarfAbbrevTableEntry *entry, int depth=1);

static set<t_function *> functions_without_dwarf;
bool FunctionHasNoDwarf(t_function *fun) {
  return functions_without_dwarf.find(fun) != functions_without_dwarf.end();
}

static
DwarfFunctionDefinition *GetSubprogram(DwarfAbbrevTableEntry *entry, bool is_method = false) {
  if (dwarf_functions.find(entry->offset) != dwarf_functions.end())
    return dwarf_functions[entry->offset];

  DwarfFunctionDefinition *function = NULL;

  /* function name */
  auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
  auto linkage_name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_linkage_name);
  auto specification_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_specification);
  auto artificial_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_artificial);

  if (specification_attr) {
    ASSERT(!name_attr, ("unexpected"));

    /* this is a member function */
    member_functions.push_back(entry);

    DEBUG_DWARF_CFG(("found function but is member"));
  }
  else if (!name_attr && !linkage_name_attr && !artificial_attr) {
    auto abstract_origin_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_abstract_origin);
    ASSERT(abstract_origin_attr, ("no DW_AT_abstract_origin for function at offset @G", entry->offset));
    to_check.push_back(abstract_origin_attr->value);

    /* this definition is a reference to another function definition,
      * so we don't need to process it anymore, for now. */
    DEBUG_DWARF_CFG(("found function but no name, it has an abstract origin that we'll check later"));

    if (dwarf_functions.find(abstract_origin_attr->value) == dwarf_functions.end()) {
      function = new DwarfFunctionDefinition();
      function->unparsed = true;
      function->offset = entry->offset;
      dwarf_functions[abstract_origin_attr->value] = function;
    }
    else
      function = dwarf_functions[abstract_origin_attr->value];
  }
  else {
    string function_name = name_attr ? *(name_attr->value) : "";
    string linkage_name = linkage_name_attr ? *(linkage_name_attr->value) : "";
    if (artificial_attr
        && !name_attr) {
      function_name = "<artificial>";
    }
    DEBUG_DWARF_CFG(("found function '%s' (linkage '%s') at @G", function_name.c_str(), linkage_name.c_str(), entry->offset));

    /* create instance and keep reference */
    if (dwarf_functions.find(entry->offset) == dwarf_functions.end()) {
      function = new DwarfFunctionDefinition();
      dwarf_functions[entry->offset] = function;
    }
    else
      function = dwarf_functions[entry->offset];

    function->offset = entry->offset;
    function->unparsed = false;

    function->name = function_name;
    function->linkage_name = linkage_name;

    /* calling conventions */
    auto cc_attr = GetDwarfAttribute<DwarfConstantAttribute *>(entry, DwarfAttributeCode::DW_AT_calling_convention);
    ASSERT(!cc_attr, ("implement me: DW_AT_calling_convention"));
    function->adheres_calling_conventions = true;

    /* external */
    auto ext_attr = GetDwarfAttribute<DwarfConstantAttribute *>(entry, DwarfAttributeCode::DW_AT_external);
    function->is_static = (ext_attr == NULL);

    /* inline */
    auto inline_attr = GetDwarfAttribute<DwarfConstantAttribute *>(entry, DwarfAttributeCode::DW_AT_inline);
    function->inlined = inline_attr ? static_cast<DwarfInlineCode>(inline_attr->value) : DwarfInlineCode::DW_INL_not_inlined;

    /* return type */
    DEBUG_DWARF_CFG2(("return2 at @G", entry->offset));
    auto returntype_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_type);
    function->return_datatype = returntype_attr ? GetDatatype(entry) : NULL;

    /* declaration or not */
    auto declaration_attr = GetDwarfAttribute<DwarfFlagAttribute *>(entry, DwarfAttributeCode::DW_AT_declaration);
    function->is_declaration = declaration_attr ? static_cast<bool>(declaration_attr->value) : false;

    /* children, look for parameters */
    for (auto _entry_it : entry->children) {
      DwarfAbbrevTableEntry *entry_it = static_cast<DwarfAbbrevTableEntry *>(_entry_it);

      if (entry_it->declaration->tag == DwarfTagCode::DW_TAG_formal_parameter) {
        auto abstract_origin_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_abstract_origin);
        if (abstract_origin_attr) {
          DEBUG_DWARF_CFG(("abstract origin"));
          continue;
        }

        auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry_it, DwarfAttributeCode::DW_AT_name);
        auto artificial_attr = GetDwarfAttribute<DwarfFlagAttribute *>(entry_it, DwarfAttributeCode::DW_AT_artificial);

        DEBUG_DWARF_CFG(("found parameter at @G", entry_it->offset));

        DwarfFunctionDefinition::Parameter *p = new DwarfFunctionDefinition::Parameter();
        p->datatype = Datatype::RESOLVING;
        p->datatype = GetDatatype(entry_it);
        p->name = name_attr ? *(name_attr->value) : "<anonymous>";
        p->artificial = artificial_attr ? static_cast<bool>(artificial_attr->value) : false;
        ASSERT(dwarf_function_parameters.find(entry_it->offset) == dwarf_function_parameters.end(), ("parameter at @G already resolved", entry_it->offset));
        dwarf_function_parameters[entry_it->offset] = p;

        function->parameters.push_back(p);
      }
    }

    /* location in binary */
    auto low_pc_attr = GetDwarfAttribute<DwarfAddressAttribute *>(entry, DwarfAttributeCode::DW_AT_low_pc);
    t_address low_pc = low_pc_attr ? static_cast<t_address>(low_pc_attr->value) : AddressNew32(0);
    if (! AddressIsEq(low_pc, AddressNew32(0))) {
      if (cfg_address_to_dwarf_function.find(low_pc) == cfg_address_to_dwarf_function.end()) {
        cfg_address_to_dwarf_function[low_pc] = function;
      }
      else {
        DwarfFunctionDefinition *existing = cfg_address_to_dwarf_function[low_pc];
        if (! existing->linkage_name.compare(function->linkage_name)
            && !function->linkage_name.empty()) {
          /* no need to do anything as the linkage names are identical and hence the functions have the same signature */
        }
        else
          FATAL(("already have function at @G\nNew (offset @G): %s", low_pc, function->offset, function->to_string().c_str()));
      }
    }
    else if (declaration_attr) {
      /* declarations */
      dwarf_function_without_cfg_address.push_back(function);
    }
    else {
      dwarf_function_without_cfg_address.push_back(function);
    }

    DEBUG_DWARF_CFG(("function: %s", function->to_string().c_str()));
  }

  return function;
}

static
Datatype *GetDatatype(DwarfAbbrevTableEntry *entry, int depth) {
  string _prefix = "";
  for (int i = 0; i < depth; i++)
    _prefix += " ";
  t_const_string prefix = _prefix.c_str();

  auto get_void_type = [] () {
    if (!void_datatype) {
      void_datatype = new Datatype();
      void_datatype->type = Datatype::Type::Void;
    }

    return void_datatype;
  };

  auto get_anonymous_name = [] () {
    if (!anonymous_name) {
      anonymous_name = new string("<anonymous>");
    }

    return anonymous_name;
  };

  /* get pointer to datatype definition in DWARF */
  auto type_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_type);
  if (type_attr) {
    ASSERT(type_attr->data != nullptr, ("no data for type attribute, value @G", type_attr->value));
    entry = type_attr->data;
  }

  DEBUG_DWARF_CFG(("get datatype (depth %d) at offset @G", depth, entry->offset));

  if (resolved_types.find(entry->offset) != resolved_types.end()) {
    /* type has already been resolved */
    return resolved_types[entry->offset];
  }

  /* need to resolve the type */
  Datatype *t = new Datatype();
  resolved_types[entry->offset] = t;

  auto type_or_void = [entry, depth, get_void_type] () {
    auto type_type_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_type);
    return type_type_attr ? GetDatatype(entry, depth+1) : get_void_type();
  };

  switch(entry->declaration->tag) {
  case DwarfTagCode::DW_TAG_pointer_type: {
    t->type = Datatype::Type::Pointer;
    t->pointer_data.datatype = Datatype::RESOLVING;
    t->pointer_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_volatile_type: {
    t->type = Datatype::Type::Volatile;
    t->volatile_data.datatype = Datatype::RESOLVING;
    t->volatile_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_restrict_type: {
    t->type = Datatype::Type::Restrict;
    t->restrict_data.datatype = Datatype::RESOLVING;
    t->restrict_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_ptr_to_member_type: {
    t->type = Datatype::Type::PointerToMember;
    t->pointer_to_member_data.datatype = Datatype::RESOLVING;
    t->pointer_to_member_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_GNU_template_template_param: {
    t->type = Datatype::Type::TemplateTemplateParam;
    auto attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_GNU_template_name);
    t->template_template_param_data.name = attr->value;
  } break;

  case DwarfTagCode::DW_TAG_typedef: {
    t->type = Datatype::Type::Typedef;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    ASSERT(name_attr, ("DW_AT_name not found for DW_TAG_typedef"));
    t->typedef_data.name = name_attr->value;

    t->typedef_data.datatype = Datatype::RESOLVING;
    t->typedef_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_base_type: {
    t->type = Datatype::Type::BaseType;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    ASSERT(name_attr, ("DW_AT_name not found for DW_TAG_base_type"));
    t->base_type_data.name = name_attr->value;
  } break;

  case DwarfTagCode::DW_TAG_structure_type:
  case DwarfTagCode::DW_TAG_union_type:
  case DwarfTagCode::DW_TAG_class_type: {
    if (entry->declaration->tag == DwarfTagCode::DW_TAG_structure_type) {
      DEBUG_DWARF_CFG2(("structure at @G", entry->offset));
      t->type = Datatype::Type::Structure;
    }
    else if (entry->declaration->tag == DwarfTagCode::DW_TAG_union_type)
      t->type = Datatype::Type::Union;
    else
      t->type = Datatype::Type::Class;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    t->structure_type_data.name = name_attr ? name_attr->value : get_anonymous_name();

    auto size_attr = GetDwarfAttribute<DwarfConstantAttribute *>(entry, DwarfAttributeCode::DW_AT_byte_size);
    t->structure_type_data.size = size_attr ? size_attr->value : -1;

    /* members */
    t->structure_type_data.members = new vector<Datatype::TypeNameTuple>();
    t->structure_type_data.methods = new vector<DwarfFunctionDefinition *>();
    t->structure_type_data.inheritance = new vector<Datatype *>();
    for (auto _x : entry->children) {
      DwarfAbbrevTableEntry *x = static_cast<DwarfAbbrevTableEntry *>(_x);
      DEBUG_DWARF_CFG2(("child at @G", x->offset));
      if ((x->declaration->tag == DwarfTagCode::DW_TAG_member)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_union_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_structure_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_const_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_class_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_template_type_parameter)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_enumeration_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_typedef)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_template_value_parameter)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_pointer_type)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_imported_declaration)
          || (x->declaration->tag == DwarfTagCode::DW_TAG_GNU_template_template_param)) {
        auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(x, DwarfAttributeCode::DW_AT_name);

        Datatype::TypeNameTuple new_data;
        new_data.datatype = Datatype::RESOLVING;
        new_data.datatype = GetDatatype(x, depth+1);
        new_data.name = name_attr ? name_attr->value : get_anonymous_name();
        t->structure_type_data.members->push_back(new_data);
      }
      else if (x->declaration->tag == DwarfTagCode::DW_TAG_subprogram) {
        DwarfFunctionDefinition *method = GetSubprogram(x, true);
        ASSERT(method, ("no function definition for DW_TAG_subprogram at offset @G (entry @G)", x->offset, entry->offset));
        t->structure_type_data.methods->push_back(method);
      }
      else if (x->declaration->tag == DwarfTagCode::DW_TAG_inheritance) {
        t->structure_type_data.inheritance->push_back(GetDatatype(x, depth+1));
      }
      else if (x->declaration->tag == DwarfTagCode::DW_TAG_GNU_template_parameter_pack) {
        /* ignored */
      }
      else if (x->declaration->tag == DwarfTagCode::DW_TAG_reference_type) {
        /* ignored */
      }
      else
        FATAL(("unexpected tag %s", TagToString(x->declaration->tag).c_str()));
    }
  } break;

  case DwarfTagCode::DW_TAG_subroutine_type: {
    t->type = Datatype::Type::Subroutine;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    t->subroutine_data.name = name_attr ? name_attr->value : NULL;

    /* return type */
    DEBUG_DWARF_CFG2(("return1 @G", entry->offset));
    t->subroutine_data.return_datatype = type_or_void();

    /* parameters */
    t->subroutine_data.parameters = new vector<Datatype *>();
    for (auto _x : entry->children) {
      DwarfAbbrevTableEntry *x = static_cast<DwarfAbbrevTableEntry *>(_x);
      if (x->declaration->tag == DwarfTagCode::DW_TAG_formal_parameter) {
        auto paramtype_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(x, DwarfAttributeCode::DW_AT_type);
        ASSERT(paramtype_attr, ("no parameter type found"));

        DEBUG_DWARF_CFG2(("param"));
        t->subroutine_data.parameters->push_back(GetDatatype(paramtype_attr->data, depth+1));
      }
      else if (x->declaration->tag == DwarfTagCode::DW_TAG_unspecified_parameters)
        t->subroutine_data.parameters->push_back(GetDatatype(x, depth+1));
      else
        FATAL(("unexpected tag %s", TagToString(x->declaration->tag).c_str()));
    }
  } break;

  case DwarfTagCode::DW_TAG_const_type: {
    t->type = Datatype::Type::Const;

    t->const_data.datatype = Datatype::RESOLVING;
    t->const_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_imported_declaration: {
    t->type = Datatype::Type::ImportedDeclaration;

    t->imported_declaration_data.datatype = Datatype::RESOLVING;
    t->imported_declaration_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_rvalue_reference_type:
  case DwarfTagCode::DW_TAG_reference_type: {
    t->type = Datatype::Type::Reference;

    t->reference_data.datatype = Datatype::RESOLVING;
    t->reference_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_array_type: {
    t->type = Datatype::Type::Array;

    t->array_data.datatype = Datatype::RESOLVING;
    t->array_data.datatype = type_or_void();
  } break;

  case DwarfTagCode::DW_TAG_enumeration_type: {
    t->type = Datatype::Type::Enum;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    t->enum_data.name = name_attr ? name_attr->value : get_anonymous_name();

    t->enum_data.datatype = Datatype::RESOLVING;
    t->enum_data.datatype = type_or_void();

    /* enumerators */
    t->enum_data.values = new vector<Datatype::NameConstTuple>();
    for (auto _x : entry->children) {
      DwarfAbbrevTableEntry *x = static_cast<DwarfAbbrevTableEntry *>(_x);
      ASSERT(x->declaration->tag == DwarfTagCode::DW_TAG_enumerator, ("expected enumerator"));

      auto x_name_attr = GetDwarfAttribute<DwarfStringAttribute *>(x, DwarfAttributeCode::DW_AT_name);
      ASSERT(x_name_attr, ("expected DW_AT_name"));

      auto x_value_attr = GetDwarfAttribute<DwarfConstantAttribute *>(x, DwarfAttributeCode::DW_AT_const_value);
      ASSERT(x_value_attr, ("expected DW_AT_const_value"));

      auto val = Datatype::NameConstTuple();
      val.name = x_name_attr->value;
      val.value = x_value_attr->value;
      t->enum_data.values->push_back(val);
    }
  } break;

  case DwarfTagCode::DW_TAG_unspecified_parameters: {
    t->type = Datatype::Type::UnspecifiedParameters;
  } break;

  case DwarfTagCode::DW_TAG_unspecified_type: {
    t->type = Datatype::Type::UnspecifiedType;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    ASSERT(name_attr, ("expected DW_AT_name"));
    t->unspecified_type_data.name = name_attr->value;
  } break;

  case DwarfTagCode::DW_TAG_template_type_parameter: {
    t->type = Datatype::Type::TemplateTypeParameter;

    auto name_attr = GetDwarfAttribute<DwarfStringAttribute *>(entry, DwarfAttributeCode::DW_AT_name);
    ASSERT(name_attr, ("expected DW_AT_name"));
    t->template_type_parameter_data.name = name_attr->value;

    t->template_type_parameter_data.datatype = Datatype::RESOLVING;
    t->template_type_parameter_data.datatype = type_or_void();
  } break;

  default:
    FATAL(("unexpected tag %s for entry at @G", TagToString(entry->declaration->tag).c_str(), entry->offset));
  }

  return t;
}

string Datatype::to_string(bool verbose) {
  string result = "";

  if (this == Datatype::RESOLVING) {
    result = "<resolving>";
    return result;
  }

  switch(type) {
  case Type::Pointer:
    result = pointer_data.datatype->to_string() + " *";
    break;

  case Type::Volatile:
    result = "volatile " + volatile_data.datatype->to_string();
    break;

  case Type::Restrict:
    result = restrict_data.datatype->to_string() + " restrict";
    break;

  case Type::PointerToMember:
    result = pointer_to_member_data.datatype->to_string() + " T::*";
    break;

  case Type::Reference:
    result = reference_data.datatype->to_string() + " &";
    break;

  case Type::Array:
    result = array_data.datatype->to_string() + " []";
    break;

  case Type::Typedef:
    result = *(typedef_data.name) + "(=\"" + typedef_data.datatype->to_string() + "\")";
    break;

  case Type::BaseType:
    result = *(base_type_data.name);
    break;

  case Type::Structure:
  case Type::Union:
  case Type::Class:
    if (type == Type::Structure)
      result += "struct ";
    else if (type == Type::Union)
      result += "union ";
    else
      result += "class ";
    result += *(structure_type_data.name);

    if (verbose) {
      result += "{ ";
      for (auto m : *structure_type_data.members) {
        result += m.datatype->to_string();
        result += " " + *(m.name) + "; ";
      }
      result += "}";
    }
    break;

  case Type::Subroutine:
    result += (subroutine_data.return_datatype) ? subroutine_data.return_datatype->to_string() : "void";
    result += (subroutine_data.name) ? *(subroutine_data.name) : "<anonymous>";

    result += "(";
    for (auto p : *subroutine_data.parameters)
      result += p->to_string() + ", ";
    if (subroutine_data.parameters->size() > 0)
      result = result.substr(0, result.size() - 2);
    result += ")";
    break;

  case Type::Const:
    result += "const ";
    result += const_data.datatype->to_string();
    break;

  case Type::Void:
    result += "void";
    break;

  case Type::Enum:
    result += "enum ";
    result += *(enum_data.name);

    if (verbose) {
      result += "{";
      for (auto v : *enum_data.values) {
        result += *v.name;
        result += "=" + to_string(v.value) + ", ";
      }
      result += "}";
    }
    break;

  case Type::UnspecifiedParameters:
    result += "...";
    break;

  case Type::UnspecifiedType:
    result += *(unspecified_type_data.name);
    break;

  case Type::TemplateTypeParameter:
    result += template_type_parameter_data.datatype->to_string() + " " + *(template_type_parameter_data.name);
    break;

  case Type::ImportedDeclaration:
    result = "[imported] " + imported_declaration_data.datatype->to_string();
    break;

  case Type::TemplateTemplateParam:
    result = *(template_template_param_data.name);
    break;

  default:
    FATAL(("unsupported type %d", static_cast<t_uint32>(type)));
  }

  result += "{" + std::to_string(static_cast<t_uint32>(type)) + "}";

  return result;
}

string DwarfFunctionDefinition::to_string() {
  string result;

  if (unparsed) {
    t_string str_offset = StringIo("@G", offset);
    result = "<unparsed>@" + string(str_offset);
    Free(str_offset);
    return result;
  }

  if (is_static)
    result += "STATIC ";

  if (adheres_calling_conventions)
    result += "CC ";

  result += "INLINE(" + DwarfInlineCodeToString(inlined) + ") ";

  result += "[" + (return_datatype ? return_datatype->to_string() : "void") + "]";

  result += " " + name + "/" + linkage_name + "(";
  for (auto p : parameters) {
    result += "[" + p->datatype->to_string() + "]" + (p->artificial ? "/art" : "") + p->name + ", ";
  }
  if (parameters.size() > 0)
    result = result.substr(0, result.size() - 2);
  result += ")";

  t_string str_offset = StringIo("@G", offset);
  result += "@" + string(str_offset);
  Free(str_offset);

  return result;
}

void
FindFunctionDeclarations(DwarfAbbrevTableEntry *entry) {
  DwarfAbbrevDeclaration *declaration = entry->declaration;
  DwarfTagCode tag = static_cast<DwarfTagCode>(declaration->tag);

  ASSERT(address_to_entry.find(entry->offset) == address_to_entry.end(), ("unexpected"));
  address_to_entry[entry->offset] = entry;

  switch (tag) {
  case DwarfTagCode::DW_TAG_subprogram:
    GetSubprogram(entry);
    break;

  case DwarfTagCode::DW_TAG_inlined_subroutine:
    DEBUG_DWARF_CFG(("implement me: DW_TAG_inlined_subroutine"));
    break;

  case DwarfTagCode::DW_TAG_entry_point:
    FATAL(("implement me: DW_TAG_entry_point"));
    break;

  default: break;
  }

  /* process children */
  for (auto entry_it : entry->children)
    FindFunctionDeclarations(static_cast<DwarfAbbrevTableEntry *>(entry_it));
}

map<t_address, DwarfFunctionDefinition *>
DwarfParseInformation(t_cfg *cfg)
{
  /* member functions */
  for (DwarfAbbrevTableEntry *entry : member_functions) {
    /* look up the referenced function */
    auto specification_attr = GetDwarfAttribute<DwarfReferenceAttribute *>(entry, DwarfAttributeCode::DW_AT_specification);
    ASSERT(specification_attr, ("unexpected"));

    ASSERT(dwarf_functions.find(specification_attr->value) != dwarf_functions.end(), ("no function definition at @G for specification @G", specification_attr->value, entry->offset));
    dwarf_functions[entry->offset] = dwarf_functions[specification_attr->value];
  }

  /* sanity checks */
  for (t_address addr : to_check)
    ASSERT(dwarf_functions.find(addr) != dwarf_functions.end(), ("no function definition at @G", addr));

  for (auto p : dwarf_functions)
    VERBOSE(1, ("dwarf function at @G: %s", p.first, p.second->to_string().c_str()));

  if (!cfg)
    return dwarf_functions;

  /* look up function by name */
  auto lookup_by_name = [] (string name, bool test_unique = false, string mangled_name = "") {
    vector<DwarfFunctionDefinition *> functions;

    auto push = [&functions, mangled_name] (DwarfFunctionDefinition *dwarf_fun) {
      if (mangled_name == ""
          || dwarf_fun->linkage_name == "") {
        functions.push_back(dwarf_fun);
      }
      else {
        /* more extensive testing */
        if (! mangled_name.compare(dwarf_fun->linkage_name))
          functions.push_back(dwarf_fun);
      }
    };

    /* maybe Dwarf didn't specify an address (e.g., for inlined functions) */
    for (auto dwarf_fun : dwarf_function_without_cfg_address) {
      if (! dwarf_fun->name.compare(name)) {
        if (test_unique)
          push(dwarf_fun);
        else
          return dwarf_fun;
      }

      if (! dwarf_fun->linkage_name.compare(name)) {
        if (test_unique)
          push(dwarf_fun);
        else
          return dwarf_fun;
      }
    }

    /* fallback */
    for (auto pair : cfg_address_to_dwarf_function) {
      if (! pair.second->name.compare(name)) {
        if (test_unique)
          push(pair.second);
        else
          return pair.second;
      }
    }

    if (functions.size() == 0)
      return static_cast<DwarfFunctionDefinition *>(NULL);
    else {
      if (functions.size() > 1) {
        WARNING(("no unique Dwarf candidate for '%s' (mangled '%s')", name.c_str(), mangled_name.c_str()));
        for (auto f : functions)
          WARNING(("- %s", f->to_string().c_str()));
      }
    }

    return functions[0];
  };

  auto name_without_suffix = [] (string str_name, string suffix, string& short_name) {
    size_t pos = str_name.rfind(suffix);
    string number = str_name.substr(pos + suffix.size());

    /* try the conversion
     * if it works, OK */
    try {
      stoi(number);
    } catch(...) {
      return false;
    }

    short_name = str_name.substr(0, pos);
    return true;
  };

  FunctionInitDwarfSignature(cfg);

  /* associate functions in CFG with Dwarf information */
  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    t_address entry = BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun));
    string short_name = "";

    DwarfFunctionDefinition *dwarf_fun = NULL;
    if (cfg_address_to_dwarf_function.find(entry) == cfg_address_to_dwarf_function.end()) {
      if (FUNCTION_NAME(fun))
        dwarf_fun = lookup_by_name(FUNCTION_NAME(fun));
    }
    else
      dwarf_fun = cfg_address_to_dwarf_function[entry];

    if (!dwarf_fun) {
      string name = FUNCTION_NAME(fun) ? string(FUNCTION_NAME(fun)) : "";
      if (StringPatternMatch("ORIG:*", name.c_str())) {
        DEBUG(("original function name %s", name.c_str()));
        name = name.substr(strlen("ORIG:"));
        DEBUG(("   name %s", name.c_str()));
      }

      if (!name.empty()
          && (!strcmp(name.c_str(), "_start")
              || !strcmp(name.c_str(), "_init")
              || !strcmp(name.c_str(), "_fini")
              || !strcmp(name.c_str(), "call_weak_fn"))) {
        /* some functions don't have any DWARF information associated with them */
        functions_without_dwarf.insert(fun);
        continue;
      }

      if (!name.empty()
          && (!strcmp(name.c_str(), SP_IDENTIFIER_PREFIX "Init")
              || !strcmp(name.c_str(), "print"))) {
        /* Diablo-specific self-profiling helper functions */
        functions_without_dwarf.insert(fun);
        continue;
      }

      if (!name.empty()
          && StringPatternMatch("*.part.*", name.c_str())
          && name_without_suffix(name.c_str(), ".part.", short_name)) {
        /* skip compiler-optimised functions */
        DEBUG_DWARF_CFG(("skip compiler-generated part @F", fun));
        functions_without_dwarf.insert(fun);
        continue;
      }

      t_bool has_info = false;
      DiabloBrokerCall("DwarfArchitectureSpecificFunctionHasInfo", fun, &has_info);
      if (!has_info) {
        /* some architecture-specific functions don't have DWARF information */
        functions_without_dwarf.insert(fun);
        continue;
      }

      if (!name.empty()
          && StringPatternMatch("*.isra.*", name.c_str())
          && name_without_suffix(name, ".isra.", short_name)) {
        /* perhaps this function is a compiler-optimised version of some other function */
        dwarf_fun = lookup_by_name(short_name);
      }
      else if (!name.empty()
          && StringPatternMatch("*.constprop.*", name.c_str())
          && name_without_suffix(name, ".constprop.", short_name)) {
        /* compiler-constantpropagated version of some other function */
        dwarf_fun = lookup_by_name(short_name);
      }

      if (!dwarf_fun && !strncmp(name.c_str(), "_Z", strlen("_Z"))) {
        int status = 0;
        char *_demangled = abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
        ASSERT(status == 0, ("@F looks to be mangled function name, but could not be demangled (%d)", fun, status));

        string demangled = string(_demangled);
        delete[] _demangled;

        auto paren_pos = demangled.find_first_of('(');
        ASSERT(paren_pos != string::npos, ("can't find '(' in @F", fun));

        auto colon_pos = demangled.rfind(':', paren_pos);
        if (colon_pos == string::npos)
          colon_pos = 0;

        string fn_name = demangled.substr(colon_pos+1, paren_pos-colon_pos-1);
        dwarf_fun = lookup_by_name(fn_name, true, name);
      }

      /* default case */
      if (!dwarf_fun) {
        WARNING(("no dwarf function for @F at @G", fun, FUNCTION_BBL_FIRST(fun) ? BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun)) : AddressNew32(0)));
        functions_without_dwarf.insert(fun);
      }
    }

    if (dwarf_fun) {
      DEBUG_DWARF_CFG(("dwarf function for (@G) @F is %s", FUNCTION_BBL_FIRST(fun) ? BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun)) : AddressNew32(0), fun, dwarf_fun->to_string().c_str()));
      FUNCTION_SET_DWARF_SIGNATURE(fun, dwarf_fun);
    }
  }

  DiabloBrokerCall("DwarfArchitectureSpecificStuff", cfg);

  return dwarf_functions;
}
