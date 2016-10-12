#ifndef DIABLOTILINKER_H
#define DIABLOTILINKER_H
#ifdef DIABLOTILINKER_INTERNAL
#include "diabloobject_config.h"
#endif

#include <diabloobject.h>
#include "diablotilinker_cmdline.h"



#ifndef DIABLOTILINKER_FUNCTIONS
#ifndef DIABLOTILINKER_TYPES
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


#ifndef DIABLOTILINKER_TYPES
#define DIABLOTILINKER_TYPES
#undef DIABLOTILINKER_H
#include <diablotilinker.h>
#endif

#ifndef DIABLOTILINKER_FUNCTIONS
#define DIABLOTILINKER_FUNCTIONS
#undef DIABLOTILINKER_H
#include <diablotilinker.h>

void DiabloTiLinkerInit(int, char **);
t_map_node * TiLinkerNode (t_string name, t_string mapped_name, t_address start, t_address size, t_string file, t_string lib);
void DiabloTiLinkerFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
