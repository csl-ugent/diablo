/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloobject.h>
#ifdef DIABLOOBJECT_FUNCTIONS
#ifndef DIABLOOBJECT_RELOCATE_FUNCTIONS
#define DIABLOOBJECT_RELOCATE_FUNCTIONS
void RelocsMigrateToRelocatables (t_object * obj);
void FixRelocsToLinkerCreatedSymbols(t_object * obj);
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
