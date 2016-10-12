#include <diablosupport.h>
#ifdef DIABLOSUPPORT_FUNCTIONS
#ifndef DIABLOSUPPORT_BROKER_FUNCTIONS
#define DIABLOSUPPORT_BROKER_FUNCTIONS
t_bool DiabloBrokerCallExists (const char *name);
void DiabloBrokerCallInstall (const char *name, const char *prototype, void *implementation, t_bool final, ...);
t_bool DiabloBrokerCall (const char *name, ...);
void DiabloBrokerFini ();
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
