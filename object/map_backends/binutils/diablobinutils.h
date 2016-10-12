#ifndef DIABLOBINUTILS_H
#define DIABLOBINUTILS_H
#ifdef DIABLOBINUTILS_INTERNAL
#include "diabloobject_config.h"
#endif

#include <diabloobject.h>
#include "diablobinutils_cmdline.h"



#ifndef DIABLOBINUTILS_FUNCTIONS
#ifndef DIABLOBINUTILS_TYPES
#define TYPEDEFS
#undef TYPEDEFS 
#else
#define TYPES
#undef TYPES
#endif
#else
#define DEFINES
#undef DEFINES
#define DEFINES2
#undef DEFINES2
#define FUNCTIONS
#undef FUNCTIONS
#define CONSTRUCTORS
#undef CONSTRUCTORS
#endif


#ifndef DIABLOBINUTILS_TYPES
#define DIABLOBINUTILS_TYPES
#undef DIABLOBINUTILS_H
#include <diablobinutils.h>
#endif

#ifndef DIABLOBINUTILS_FUNCTIONS
#define DIABLOBINUTILS_FUNCTIONS
#undef DIABLOBINUTILS_H
#include <diablobinutils.h>

void DiabloBinutilsInit(int, char **);
t_map_node * BinutilsNode (t_string name, t_string mapped_name, t_address start, t_address size, t_string file, t_bool builtin);
void DiabloBinutilsFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
