/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>

#ifndef ARM_DEFLOWGRAPH_H
#define ARM_DEFLOWGRAPH_H
void ArmDeflowgraph(t_object *obj);
void SimpleAddressProducersForChain (t_bbl *chain, t_bool optimize);
void OptimizeAddressProducersForChain (t_bbl *chain, t_bool optimize);
#endif
