#ifndef META_API_LIBRARIES_INTERFACE_H
#define META_API_LIBRARIES_INTERFACE_H

#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
#define EXPORT extern "C"
#else
#define EXPORT
#endif

/* datatype implementations */
#define T_REGISTER_DATATYPE_ARGS (std::string name)
typedef MetaAPI_Datatype * (*t_register_datatype_fn) T_REGISTER_DATATYPE_ARGS;

/* custom libraries */
#define T_CUSTOM_FUNCTION_ARGS ()
typedef void (*t_custom_function) T_CUSTOM_FUNCTION_ARGS;

#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
#define REGISTER_DATATYPE_NAME(x) Register

static const std::string DYNSYM_REGISTER_DATATYPE = "Register";
#else
#define REGISTER_DATATYPE_NAME(x) Register ## x
#endif

#define DEFINE_REGISTER_DATATYPE_FN(name) EXPORT MetaAPI_Datatype * REGISTER_DATATYPE_NAME(name) T_REGISTER_DATATYPE_ARGS
#define DEFINE_CUSTOM_FUNCTION(name) EXPORT void name T_CUSTOM_FUNCTION_ARGS

#endif /* META_API_LIBRARIES_INTERFACE_H */
