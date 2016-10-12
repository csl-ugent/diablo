#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS segment
#define segment_field_select_prefix SEGMENT
#define segment_function_prefix Segment
#endif

/*! \brief Class used to represent the segments stored in a relocatable object
 *
 * A segment is a block of continuous data that will loaded by the loader or
 * the operating system when the program is executed. Each (executable) object
 * file contains a description of the segments in an object file. This class
 * provides the abstraction of that information for Diablo. See also t_object
 * and t_section. */
DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
MEMBER(t_string, name, NAME)
MEMBER(struct _t_segment *, segment_next, SEGMENT_NEXT)
MEMBER(struct _t_segment *, segment_prev, SEGMENT_PREV)
MEMBER(t_bool, hoist_headers, HOIST_HEADERS)
DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS relocatable
#include <diabloobject_relocatable.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
