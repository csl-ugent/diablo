/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* IO support
 *
 * Interface: TODO
 *
 */

#include <diablosupport.h>
#include <assert.h>
#include <stdarg.h>

/* Io Typedefs {{{ */
#ifndef DIABLOSUPPORT_STDIO_TYPEDEFS
#define DIABLOSUPPORT_STDIO_TYPEDEFS
typedef enum
{ E_WARNING, E_FATAL, E_DEBUG, E_STATUS, E_MESSAGE } t_io;
typedef enum
{ START, STOP } t_io_status_type;
typedef struct _t_io_modifier t_io_modifier;
typedef struct _t_io_handler t_io_handler;
#endif /* }}} Io Typedefs */

#ifndef DIABLOSUPPORT_STDIO_TYPES
#define DIABLOSUPPORT_STDIO_TYPES

/*! Describes an Io Modifier. */
struct _t_io_modifier
{
  char escape;
  char command;
  t_string valid_modifiers;
  t_string (*function) (t_const_string modifiers, va_list * ap);
  struct _t_io_modifier *next;
};

/*! Describes an Io Handler. Io in Diablo is done by writing to an io-id. This
 * io handler is used to describe how output to a certain io-id should be
 * done. */
struct _t_io_handler
{
  t_uint32 id;
  void (*function) (t_uint32 id, t_const_string out);
  struct _t_io_handler *next;
};
#endif

/* Io Defines {{{ */
#ifndef DIABLOSUPPORT_STDIO_DEFINES
#define DIABLOSUPPORT_STDIO_DEFINES

#define FATAL(x) do { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_FATAL; IoWrapper x;\
  assert(false);\
  exit(-1); } while(0)
#define WARNING(x) do { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_WARNING; IoWrapper x; } while(0)
#define DEBUG(x) do { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_DEBUG; IoWrapper x; } while(0)
#define STATUS(m,x) do { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_STATUS; io_wrapper_status=m; IoWrapper x; } while(0)
#define VERBOSE(m,x) do { if (m<=diablosupport_options.verbose) { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_MESSAGE; IoWrapper x;} } while(0)
#define ASSERT(x,y) do { if (!(x)) FATAL(y); } while(0)

#endif /* }}} Io Defines */
/* Io Globals {{{ */
#ifndef DIABLOSUPPORT_STDIO_GLOBALS
#define DIABLOSUPPORT_STDIO_GLOBALS
extern t_uint32 io_wrapper_type;
extern t_uint32 io_wrapper_status;
extern t_const_string io_wrapper_file;
extern t_uint32 io_wrapper_lnno;
#endif /* }}} Io Globals */

#ifdef DIABLOSUPPORT_FUNCTIONS
/* Io Functions {{{ */
#ifndef DIABLOSUPPORT_STDIO_FUNCTIONS
#define DIABLOSUPPORT_STDIO_FUNCTIONS
void Io (t_uint32, t_string, ...);
void IoWrapper (t_const_string message, ...);
t_string StringIo (t_const_string message, ...);
t_string vStringIo (t_const_string message, va_list * ap);
void FileIo (FILE * fp, t_const_string message, ...);
t_io_modifier *IoModifierAdd (char, char, t_string, t_string (*)(t_const_string, va_list *));
t_io_handler *IoHandlerAdd (t_uint32, void (*)(t_uint32, t_const_string));
t_string IoModifierPrintf (t_const_string modifiers, va_list * ap);
void InternalIo (t_uint32 type, t_const_string message, va_list * ap);
void IoHandlersFree ();
void IoModifiersFree ();
#endif /* }}} Io Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
