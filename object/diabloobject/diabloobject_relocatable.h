/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/** \file
 *
 * Relocatables are the base abstraction for all object file structures that
 * can have an address, i.e. sections, segments... This file holds the
 * typedefs, types, function, and defines needed to handle them.
 */

/* Relocatable Typedefs {{{ */
#ifndef DIABLO_RELOCATABLE_TYPEDEFS
#define DIABLO_RELOCATABLE_TYPEDEFS
typedef struct _t_symbol_ref t_symbol_ref, renamed_t_symbol_ref;

/*! different types of t_relocatables */
typedef enum
{
  RT_NONE = 0,
  RT_SEGMENT,
  RT_SECTION,
  RT_SUBSECTION,
  RT_BBL,
  RT_INS,
  RT_CODEBYTE
} t_relocatable_type, renamed_t_relocatable_type;

typedef enum
{
  MIN_ADDRESS,
  OLD_ADDRESS,
  NEW_ADDRESS
} t_relocatable_address_type;

#endif /* }}} Relocatable Typedefs */

/* Relocatable Defines {{{ */
#ifndef DIABLO_RELOCATABLE_DEFINES
#define DIABLO_RELOCATABLE_DEFINES
#define T_RELOCATABLE(relocatable) ((t_relocatable *) relocatable)
#endif /* }}} Relocatable Defines */
#include <diablosupport.h>
#ifdef DIABLOOBJECT_TYPES
/* Relocatable Types {{{ */
#ifndef DIABLO_RELOCATABLE_TYPES
#define DIABLO_RELOCATABLE_TYPES
struct _t_symbol_ref
{
  struct _t_symbol_ref *next;
  struct _t_symbol_ref *prev;
  t_symbol *sym;
};

typedef void (*t_relocatablereviver)(t_object *, t_graph *, t_relocatable *, t_bool);

#endif /* }}} Relocatable Types */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
