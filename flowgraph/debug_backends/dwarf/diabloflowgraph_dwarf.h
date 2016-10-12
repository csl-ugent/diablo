/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOFLOWGRAPH_DWARF_H
#define DIABLOFLOWGRAPH_DWARF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <diabloobject.h>
#include <diabloflowgraph.h>

#ifdef __cplusplus
}
#endif

#include "diabloobject_dwarf.h"

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

#ifdef __cplusplus

#include "diabloflowgraph_dwarf_cfg.h"

#endif

EXTERN void DwarfFlowgraphInit();

#endif /* DIABLOFLOWGRAPH_DWARF_H */
