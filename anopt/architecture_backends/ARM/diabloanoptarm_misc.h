/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOANOPTARM_MISC_H_
#define DIABLOANOPTARM_MISC_H_
void ArmOptimizeStackLoadAndStores(t_cfg * cfg);
t_bool ArmKillUselessInstructions (t_cfg *cfg);
void OptimizeSingleThreaded(t_cfg * cfg);
void ArmInsCanMoveDown(t_arm_ins * ins, t_bool * ret);
void ArmInsCanMoveUp(t_arm_ins * ins, t_bool * ret);
void ArmBblCanMerge(t_bbl * a, t_bbl * b, t_bool * ret);
void ArmBranchEliminationDo(t_bbl * a, t_bbl * b, t_bool * ret);
void ArmBranchForwardingDo(t_cfg_edge * edge, t_bool * ret);
void ArmPossiblyFlipConditionalBranches(t_cfg *cfg);

/* needed by special branch flipping support in AF_branch.cc */
void DoFlipBranch(t_bbl *bbl, t_cfg_edge *ft_edge, t_cfg_edge *jump_edge);

/* Checks the preconditions for branch flipping:
    - non-empty, non-hell
    - ends in conditional branch
    - no outgoing loop jump edge
    - no outgoing call edge
    - not a jump to an exit block
    Returns TRUE if a branch flip can be applied, and the pass-by-reference FALLTHROUGH and JUMP edges. */
t_bool CanFlipBranch(t_bbl *bbl, t_cfg_edge ** ft_edge, t_cfg_edge ** jump_edge);
t_bool CanModifyBranch(t_bbl *bbl);
t_bool CanModifyBranchConditional(t_bbl *bbl);
t_bool CanModifyJumpEdge(t_cfg_edge *e);
void DoUncondBranchToFallthrough(t_bbl *bbl);

#endif
