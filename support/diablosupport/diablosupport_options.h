/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef STAGE
#include <diablosupport.h>
#endif
/* Option Typedefs {{{ */
#ifndef DIABLOSUPPORT_OPTIONS_TYPEDEFS
#define DIABLOSUPPORT_OPTIONS_TYPEDEFS
#ifdef STAGE
typedef char *t_string;

#include <stdint.h>
typedef uint32_t t_uint32;
typedef int32_t t_int32;

#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif

#include <stdbool.h>

#define FALSE false
#define TRUE true
typedef bool t_bool;
#endif

typedef enum
{
  OPTION_BOOL,
  OPTION_COUNT,
  OPTION_INT,
  OPTION_STRING,
  OPTION_STRING_ARRAY,
  OPTION_PATH,
  OPTION_USAGE,
  OPTION_FILE,
  OPTION_VERSION,
  OPTION_NONE
} t_option_type;

typedef struct _t_option t_option;
#endif /* Option Typedefs }}} */
/* Options Types {{{ */
#ifndef DIABLOSUPPORT_OPTIONS_TYPES
#define DIABLOSUPPORT_OPTIONS_TYPES
/*! \brief A diablo option */
struct _t_option
{
  t_bool required;
  t_bool hidden;
  char *group;
  char *short_option;
  char *long_option;
  char *enviromental_option;
  t_option_type type;
  t_bool *set;
  union
  {
    void ** v;
    char ** string;
#ifndef STAGE
    t_string_array **sarray;
#endif
    t_bool * boolean;
    t_uint32 * count;
    t_int32 * sint;
#ifndef STAGE
    t_path ** path;
#endif
    void (*function)();
  } variable;
  /*! The defaults for the option. Type may vary dependent on the type of the
   * option */
  union
  {
    char *default_string;
    t_bool default_bool;
    t_uint32 default_count;
  } defaults;
  char *description;
};
#endif /* }}} Option Types */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* Option Functions {{{ */
#ifndef DIABLOSUPPORT_OPTION_FUNCTIONS
#define DIABLOSUPPORT_OPTION_FUNCTIONS
void OptionDefaults (t_option *);
void OptionUsage (t_option *);
void OptionParseCommandLine (t_option *, t_uint32, char **, t_bool);
void OptionGetEnvironment (t_option *);
#ifndef STAGE
/* More advanced functions */
void AddOptionsListInitializer(t_option* optionlist);
void ParseRegisteredOptionLists(t_uint32 argc, char ** argv);
#endif
#endif /* }}} Option Functions */
#endif
/* Option Defines {{{ */
#ifndef DIABLOSUPPORT_OPTIONS_DEFINES
#define DIABLOSUPPORT_OPTIONS_DEFINES
#ifdef STAGE
#define Free free
#define FATAL(x) { printf x ; exit(0); }
#define StringDup strdup
#endif
#endif /* }}} Option Defines */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
