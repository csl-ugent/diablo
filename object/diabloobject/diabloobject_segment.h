/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * A segment groups different object sections that have the same properties.
 * This information is used by the runtime system to quickly transfer the
 * objectfile from storage to main memory. This file holds the typedefs,
 * types, function, and defines needed to handle and manipulate them.
 */

/* Segment Defines {{{ */
#ifndef DIABLOOBJECT_SEGMENT_DEFINES
#define DIABLOOBJECT_SEGMENT_DEFINES
#endif /* Segment Defines }}} */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_FUNCTIONS
/* Segment Functions {{{ */
#ifndef DIABLOOBJECT_SEGMENT_FUNCTIONS
#define DIABLOOBJECT_SEGMENT_FUNCTIONS
t_segment *SegmentNew (t_const_string name);
#endif /* }}} Segment Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
