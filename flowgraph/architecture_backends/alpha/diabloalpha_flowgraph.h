#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS
#ifndef ALPHA_FLOWGRAPH_H
#define ALPHA_FLOWGRAPH_H

#define ALPHA_MARK_AS_BBLL(ins) \
	ALPHA_INS_SET_ATTRIB(ins, ALPHA_INS_ATTRIB(ins) | IF_BBL_LEADER);

#define ALPHA_IS_BBLL(ins) \
	(ALPHA_INS_ATTRIB(ins) & IF_BBL_LEADER)

void AlphaFlowgraph(t_object *obj);
void AlphaMakeAddressProducers(t_cfg *cfg);
t_bbl * AlphaFindJSRTarget(t_bbl * bb, t_alpha_ins * last_ins, t_cfg * cfg);
void RemoveBranchReloc(t_alpha_ins *);

t_uint32 AlphaAddEdgeForRefedBbls(t_cfg * cfg,t_bbl * cur_bbl, t_alpha_ins * last);
t_uint32 AlphaAddEdgeForBranches(t_cfg * cfg, t_bbl * cur_bbl, t_bbl * ft_bbl, t_alpha_ins *ins_first);
t_uint32 AlphaAddEdgeForSwi(t_cfg * cfg, t_bbl * cur_bbl, t_bbl * ft_bbl, t_alpha_ins *ins_last);

t_uint32 AlphaAddEdgesForRelocs(t_cfg * cfg);

t_uint32 AlphaRemoveUselessLoads(t_section * code);
t_uint32 AlphaChangeJSRs(t_cfg * cfg);



#endif
#endif

