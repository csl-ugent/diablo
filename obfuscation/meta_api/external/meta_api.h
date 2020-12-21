#define META_API_VARIABLE_SECTION_PREFIX ".data.MetaAPIvar$"
#define META_API_ARGUMENT_SECTION_PREFIX ".data.MetaAPIarg$"

/* this can't be in small caps because the Diablo binutils parser
 * then matches another rule (<MAP>^.*("LOAD linker stubs"...) */
#define META_API_GLOBAL_SCOPE_NAME __GLOBAL__

#ifndef META_API_SEMANTIC
#define BODY(x) x;
#define META_API_ARGUMENT(T, identifier, datatype_name) \
          T identifier
#define META_API_DATASTRUCTURE(identifier) \
          DATASTRUCTURE *identifier
#define META_API_ARGUMENT_CONSTRAINT(x)
#define META_API_SETTER_EFFECT(...)

#define __quote_ident(x) #x
#define __quote_macro(x) __quote_ident(x)

#define __PASTE(a, b) a ## _ ## b
#define __CONCAT_MACRO(a, b) __PASTE(a, b)

#define META_API_DECLARE_GLOBAL_VARIABLE(T, identifier, meta_api_datatype) \
          T __CONCAT_MACRO(META_API_GLOBAL_SCOPE_NAME, identifier) __attribute__((section(META_API_VARIABLE_SECTION_PREFIX meta_api_datatype "$" __quote_macro(META_API_GLOBAL_SCOPE_NAME) "$" #identifier))) __attribute__((used))
#define META_API_USE_GLOBAL_VARIABLE(identifier) \
          __CONCAT_MACRO(META_API_GLOBAL_SCOPE_NAME, identifier)

#define META_API_DECLARE_VARIABLE(T, identifier, meta_api_datatype) \
          T __CONCAT_MACRO(META_API_FUNCTION_NAME, identifier) __attribute__((section(META_API_VARIABLE_SECTION_PREFIX meta_api_datatype "$" __quote_macro(META_API_FUNCTION_NAME) "$" #identifier))) __attribute__((used))
#define META_API_USE_VARIABLE(identifier) \
          __CONCAT_MACRO(META_API_FUNCTION_NAME, identifier)

#define META_API_DECLARE_ARGUMENT(uid, T, identifier, meta_api_datatype) \
          T __CONCAT_MACRO(META_API_FUNCTION_NAME, identifier) __attribute__((section(META_API_ARGUMENT_SECTION_PREFIX meta_api_datatype "$" __quote_macro(META_API_FUNCTION_NAME) "$" #identifier))) __attribute__((used))
#define META_API_USE_ARGUMENT(uid, T, identifier) \
          T __CONCAT_MACRO(META_API_FUNCTION_NAME, identifier)

#define META_API_SETTER(x) \
  void META_API_FUNCTION_NAME x __attribute__((used)); \
  void META_API_FUNCTION_NAME x
#define META_API_GETTER(x) \
  int META_API_FUNCTION_NAME x __attribute__((used)); \
  int META_API_FUNCTION_NAME x

#else
#define BODY(x)
#define META_API_ARGUMENT(T, identifier, datatype_name) \
          T identifier [datatype=datatype_name]
#define META_API_DATASTRUCTURE(identifier) \
          DATASTRUCTURE *identifier [value="[instance]"]
#define META_API_ARGUMENT_CONSTRAINT(x)
#define META_API_SETTER_EFFECT(...) | __VA_ARGS__

#define META_API_DECLARE_VARIABLE(...)
#define META_API_DECLARE_ARGUMENT(...)

#define META_API_SETTER(x) \
  META_API_FUNCTION_NAME
#define META_API_GETTER(x)

#endif
