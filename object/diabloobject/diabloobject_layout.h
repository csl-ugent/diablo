/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * Layouting an objectfile is the process of assigning address to all the
 * different object entities (sections, symbols, ... ). This file holds the
 * typedefs, types, function, and defines needed to layout an object. */

/* Layout Defines {{{ */
#ifndef DIABLOOBJECT_LAYOUT_DEFINES
#define DIABLOOBJECT_LAYOUT_DEFINES
/** Flag used during layout to mark all sections that are mentioned in the
 * layout script. Sections that are not marked will be placed with a call to
 * PUT_REMAINING_SECTIONS */
#define SF_MARKED 1
#endif /* }}} Layout Defines */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_FUNCTIONS
/* Layout Functions {{{ */
#ifndef DIABLOOBJECT_LAYOUT_FUNCTIONS
#define DIABLOOBJECT_LAYOUT_FUNCTIONS
void ObjectPlaceSections (t_object * obj, t_bool min, t_bool reorder_subsecs_for_minimal_alignment_loss, t_bool place_subsecs);
void ObjectOrderCodeSectionsContiguously (t_object *);
#endif /* Layout Functions }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
