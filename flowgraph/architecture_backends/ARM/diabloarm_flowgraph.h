/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>

#ifndef DIABLO_ARM_FLOWGRAPH_TYPEDEFS
#define DIABLO_ARM_FLOWGRAPH_TYPEDEFS
typedef struct _t_address_to_bbl_he t_address_to_bbl_he;
#endif

#ifndef ARM_FLOWGRAPH_H
#define ARM_FLOWGRAPH_H
void ArmAddCallFromBblToBbl (t_object* obj, t_bbl* from, t_bbl* to);
void ArmAddInstrumentationToBbl(t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_address offset);
void ArmFlowgraph(t_object *obj);
void ArmMakeAddressProducers(t_cfg *cfg);
/* helper callback for FunctionDuplicate */
void ArmDupSwitchTables (t_function *orig, t_function *new_);
/* helper callback to detect copied code regions */
void ChangeCopiedCodeToData(t_object *obj);
void ArmReviveFromThumbStubs(t_cfg *cfg);
void ArmAlignSpecialBasicBlocks(t_cfg *cfg);

struct _t_address_to_bbl_he
{
  t_hash_table_node node;
  t_bbl * bbl;
};
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
