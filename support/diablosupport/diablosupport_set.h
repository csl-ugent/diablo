/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file diablosupport_set.h
 *
 * Set support
 *
 */

#include <diablosupport.h>

/* String Typedefs {{{ */
#ifndef DIABLOSUPPORT_SET_TYPEDEFS
#define DIABLOSUPPORT_SET_TYPEDEFS
typedef struct _t_set t_set;
typedef struct _t_set_iterator t_set_iterator;
#endif /* }}} Typedefs */

#ifndef DIABLOSUPPORT_SET_TYPES
#define DIABLOSUPPORT_SET_TYPES

struct _t_set
{
  int nr_elements;
  int capacity;
  void **data;
  int nr_iterators_active;
  int first_free;
};

struct _t_set_iterator
{
  t_set *set;
  int position;
  int count;
  t_bool may_remove;
};
#endif

#ifdef DIABLOSUPPORT_FUNCTIONS
/* Set Functions {{{ */
#ifndef DIABLOSUPPORT_SET_FUNCTIONS
#define DIABLOSUPPORT_SET_FUNCTIONS
void SetRemoveElemAt (t_set * set, int i);
void SetRemoveElem (t_set * set, const void *elem);
t_bool SetAddElem (t_set * set, void *elem);
void SetAddElemBlindly (t_set * set, void *elem); /* don't look if it was already present */
t_set *SetNew (int size);
void SetFree (const t_set * set);
void SetReplaceElem (t_set * set, const void *old, void *new_);
void *SetGetFirstElement (const t_set * set);
#endif /* }}} Set Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
