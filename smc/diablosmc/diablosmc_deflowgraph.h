#include <diablosmc.h>
#ifndef SMCDEFLOWGRAPH_H
#define SMCDEFLOWGRAPH_H

void SmcCalcReloc(t_reloc * rel, t_object * obj);
void SmcDeflow(t_object * obj);
void SmcDeflowgraph(t_section * code);
void SmcCreateChains(t_cfg * cfg);
t_bool InsIsControlTransferInstructionWithImmediateTarget(t_ins * ins);
t_bbl * ControlTransferGetTarget(t_ins * ins);
void ResetLinksForBbl(t_bbl * bbl);
void AddLinksForBbl(t_bbl * bbl);
  

#endif
