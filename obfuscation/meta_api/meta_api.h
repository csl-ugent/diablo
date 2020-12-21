#ifndef META_API_H
#define META_API_H

#define META_API_PREFIX "[meta-api] "

#include <diabloflowgraph.hpp>

extern "C" {
#include <diabloobject.h>
#include <diablosupport.h>
}

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <bitset>

/* forward declarations */
struct _MetaAPI_FunctionParameter;
typedef struct _MetaAPI_FunctionParameter MetaAPI_FunctionParameter;

struct _MetaAPI_Instance;
typedef struct _MetaAPI_Instance MetaAPI_Instance;

struct _MetaAPI_Datatype;
typedef struct _MetaAPI_Datatype MetaAPI_Datatype;

struct _MetaAPI_Variable;
typedef struct _MetaAPI_Variable MetaAPI_Variable;

struct _MetaAPI_Predicate;
typedef struct _MetaAPI_Predicate MetaAPI_Predicate;

struct _MetaAPI_Function;
typedef struct _MetaAPI_Function MetaAPI_Function;

struct _MetaAPI_ActivePredicate;
typedef struct _MetaAPI_ActivePredicate MetaAPI_ActivePredicate;

struct _MetaAPI_String;
typedef struct _MetaAPI_String MetaAPI_String;

struct _MetaAPI_AbstractStmt;
typedef struct _MetaAPI_AbstractStmt MetaAPI_AbstractStmt;

struct _MetaAPI_Relation;
typedef struct _MetaAPI_Relation MetaAPI_Relation;

struct _MetaAPI_Operand;
typedef struct _MetaAPI_Operand MetaAPI_Operand;

struct _MetaAPI_ImplementationValue;
typedef struct _MetaAPI_ImplementationValue MetaAPI_ImplementationValue;

typedef std::map<std::string, std::string> Properties;

#include "meta_api_effect.h"
#include "meta_api_helpers.h"
#include "meta_api_constraint.h"
#include "meta_api_abstractvalue.h"
#include "meta_api_variable.h"
#include "meta_api_expression.h"
#include "meta_api_syntax.h"
#include "meta_api_implementation.h"
#include "meta_api_datatype.h"
#include "meta_api_instance.h"
#include "meta_api_strings.h"
#include "meta_api_liveness.h"
#include "meta_api_getsetlocation.h"
#include "meta_api_debug.h"
#include "meta_api_dynsym.h"

/* public interface */
#include "external/meta_api.h"
#include "external/linkin.h"
#include "libraries/interface.h"

extern t_randomnumbergenerator *meta_api_rng;
extern t_randomnumbergenerator *meta_api_getter_rng;
extern t_randomnumbergenerator *meta_api_setter_rng;
extern t_randomnumbergenerator *meta_api_effect_rng;
extern t_randomnumbergenerator *meta_api_string_rng;
extern t_randomnumbergenerator *meta_api_value_rng;
extern t_randomnumbergenerator *meta_api_instruction_rng;
extern t_randomnumbergenerator *meta_api_liveness_rng;

extern t_uint32 meta_api_verbosity;
extern bool meta_api_debug;
extern t_int32 meta_api_max_setter_count;
extern bool meta_api_max_setter_set;
extern bool meta_api_verify_setters;
extern bool meta_api_enable_two_way;
extern bool meta_api_restrict_push_pop;
extern int meta_api_instance_count;
extern int meta_api_setters_per_getter;
extern bool meta_api_impl_inst;
extern bool meta_api_impl_set;
extern bool meta_api_impl_get;
extern bool meta_api_impl_reset;
extern std::string meta_api_project_name;

extern bool meta_api_measuring;
extern std::string meta_api_measure_init_address;
extern std::string meta_api_measure_loc1_address;
extern std::string meta_api_measure_loc2_address;
extern std::string meta_api_measure_loc3_address;

extern t_function *meta_api_init_function;

extern FunctionSet implementing_functions;

void MetaAPI_Init(t_cfg *cfg, t_randomnumbergenerator *rng, t_const_string xml_file);
void MetaAPI_test(t_cfg *cfg);
void MetaAPI_KeepLive(t_cfg *cfg);

// #define ONLY_ONE_POSSIBLE_VALUE

#endif
