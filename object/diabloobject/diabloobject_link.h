/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/** \file
 *
 * Linking is the process of combining different input objects into one output
 * object. Diabloobject should both offer the functionality to link a binary
 * and the functionality to "emulate" a linker, i.e. redo the linking, just
 * like the platform specific linker would have done it. This file holds the
 * typedefs, types, function, and defines needed to link or emulate a link for
 * an object. */

#include <diabloobject.h>
#ifdef DIABLOOBJECT_FUNCTIONS
#ifndef DIABLOOBJECT_LINK_FUNCTIONS
#define DIABLOOBJECT_LINK_FUNCTIONS

#define LINKIN_IDENTIFIER_PREFIX "DIABLO_"

t_object *LinkEmulate (t_const_string name, t_bool read_debug);
void LinkObjectsFromMap (t_object * obj, t_bool read_debug);
void LinkObjectFile (t_object *, t_const_string, t_const_string);
void LinkObjectFileNew (t_object *, t_const_string, t_const_string, t_bool, t_bool, t_const_string const *);
void LinkMakeFinalObject (t_object *);
t_object *LinkGetParent (t_const_string, t_bool read_debug);

/*! Relocate the binary */
void LinkRelocate (t_object *, t_bool);

extern t_bool emulate_link;
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
