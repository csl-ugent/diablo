#ifndef _DIABLO_OPT_MISC_H_
#define _DIABLO_OPT_MISC_H_
void MergeBbls(t_cfg * cfg);
void BranchElimination(t_cfg * cfg);
void BranchForwarding(t_cfg * cfg);
void OptimizeStackLoadAndStores(t_cfg * cfg);
void OptKillNoops(t_cfg * cfg);
void TryFindingNonLoopEdges(t_cfg * cfg);
void DetectStackAliases(t_cfg * cfg);
#endif
