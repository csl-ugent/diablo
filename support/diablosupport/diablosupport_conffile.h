/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>

#ifdef DIABLOSUPPORT_FUNCTIONS
#ifndef DIABLOSUPPORT_CONFFILE_FUNCTIONS
#define DIABLOSUPPORT_CONFFILE_FUNCTIONS
void ConfValueFree(const void * tf, void *unused_data);
t_string ConfValueGet(t_hash_table * in,t_string x);
t_hash_table * ConfFileRead(t_const_string conffile);
void ConfValuePrint(const void * string1, void * string2);
void ConfValueSet(t_string x, t_string y);
#endif
#endif
