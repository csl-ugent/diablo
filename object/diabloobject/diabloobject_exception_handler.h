/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <string.h>
#include <diabloobject.h>
#ifdef DIABLOOBJECT_FUNCTIONS
#ifndef DIABLOOBJECT_EXCEPTION_HANDLER_FUNCTIONS
#define DIABLOOBJECT_EXCEPTION_HANDLER_FUNCTIONS
typedef unsigned long long uleb128;
typedef long long sleb128;

unsigned int process_uleb128 (const unsigned char *buf, uleb128 * result);
unsigned int process_sleb128 (const unsigned char *buf, sleb128 * result);
unsigned int encode_uleb128(uleb128 val, unsigned char *result);
unsigned int encode_sleb128(sleb128 val, unsigned char *result);
void DoEhFrame (t_object * obj, const t_section * sec);
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
