/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * Relocs in Diablo are considered to be very small programs that express how
 * a binary needs to be adapted when the layout is changed. These programs are
 * very architecture specific, and they can express quit complex relations.
 * Diabloobject transforms relocs to an internal (stack machine)
 * representation, that is easy to extend and manipulate. This file holds the
 * typedefs, types, function, and defines that make up the stack machine.
 */

#include <diabloobject.h>

#ifdef DIABLOOBJECT_FUNCTIONS
/* StackMachine Functions {{{ */
#ifndef DIABLOOBJECT_STACK_MACHINE_FUNCTIONS
#define DIABLOOBJECT_STACK_MACHINE_FUNCTIONS
t_address StackExec (t_const_string program, const t_reloc * rel, const t_symbol * sym, char *base, t_bool write, t_uint32 mode, t_object * obj);
t_address StackExecConst (t_const_string program, const t_reloc* rel, const t_symbol* sym, t_uint32 mode, const t_object* obj);
#endif /* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
