/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOFLOWGRAPH_DWARF_CFG_H
#define DIABLOFLOWGRAPH_DWARF_CFG_H

#include <diabloobject_dwarf.h>

typedef struct InsLineInfo {
  t_string file;
  int line;
} t_ins_line_info;

void CfgAssociateLineInfoWithIns(t_cfg *cfg, DwarfSections *dwarf_sections, DwarfInfo dwarf_info);
void CfgFileNameCacheFree();

/* DYNAMIC MEMBERS */
extern bool disable_dwarf_dup;
INS_DYNAMIC_MEMBER_GLOBAL_BODY(lineinfo, LINEINFO, Lineinfo, t_ins_line_info *, {*valp=NULL;}, {if (*valp) delete *valp;}, { if (!disable_dwarf_dup) *valp = new InsLineInfo(*((InsLineInfo *)global_hack_dup_orig)); });

#endif /* DIABLOFLOWGRAPH_DWARF_CFG_H */
