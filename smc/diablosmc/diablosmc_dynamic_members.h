/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef SMC_DYNAMIC_MEMBERS
#define SMC_DYNAMIC_MEMBERS
#include <diablosmc.h>

extern t_dynamic_member_info ins_statelist_array;
extern t_dynamic_member_info cfg_codebytelist_array;
extern t_dynamic_member_info codebyte_next_in_chain_array;
extern t_dynamic_member_info codebyte_prev_in_chain_array;

#endif

#ifdef DIABLOSMC_FUNCTIONS
#ifndef SMC_DYNAMIC_MEMBERS_FUNCTIONS
#define SMC_DYNAMIC_MEMBERS_FUNCTIONS 
CALLBACKPREFIX void CodebyteCallbackInstall(t_cfg * cfg, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data);
CALLBACKPREFIX void CodebyteCallbackUninstall(t_cfg * cfg, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data);

void InsStartStatelist(t_cfg * cfg);
void InsStopStatelist(t_cfg * cfg);
void SmcInstructionInit(t_ins * ins, t_statelist ** states);
void SmcInstructionFini(t_ins * ins, t_statelist ** states);
void SmcInstructionDup(t_ins * duplicated_ins, t_statelist ** pointer_to_original_state_list);

void CfgStartCodebytelist(t_object * obj);
void CfgStopCodebytelist(t_object * obj);
void SmcCfgInit(t_cfg * cfg, t_codebytelist ** codebytes);
void SmcCfgFini(t_cfg * ins, t_codebytelist ** codebytes);
void SmcCfgDup(t_cfg * duplicated_cfg, t_codebytelist ** pointer_to_original_codebyte_list);

void CodebyteStartPrevInChain(t_cfg * cfg);
void CodebyteStopPrevInChain(t_cfg * cfg);
static void CodebytePrevInChainInit(t_codebyte * codebyte, t_codebyte ** prev) { *prev = NULL; }
static void CodebytePrevInChainFini(t_codebyte * codebyte, t_codebyte ** prev) { return; }
static void CodebytePrevInChainDup(t_codebyte * codebyte, t_codebyte ** prev) { return; }

void CodebyteStartNextInChain(t_cfg * cfg);
void CodebyteStopNextInChain(t_cfg * cfg);
static void CodebyteNextInChainInit(t_codebyte * codebyte, t_codebyte ** next) { *next = NULL; }
static void CodebyteNextInChainFini(t_codebyte * codebyte, t_codebyte ** next) { return; }
static void CodebyteNextInChainDup(t_codebyte * codebyte, t_codebyte ** next) { return; }

DYNAMIC_MEMBER(ins, t_cfg *, ins_statelist_array, t_statelist *, statelist, STATELIST, Statelist, CFG_FOREACH_INS, SmcInstructionInit, SmcInstructionFini, SmcInstructionDup)
DYNAMIC_MEMBER(cfg, t_object *, cfg_codebytelist_array, t_codebytelist *, codebytelist, CODEBYTELIST, Codebytelist, OBJECT_FOREACH_CFG, SmcCfgInit, SmcCfgFini, SmcCfgDup)
DYNAMIC_MEMBER(codebyte, t_cfg *, codebyte_prev_in_chain_array, t_codebyte *, prev_in_chain, PREV_IN_CHAIN, PrevInChain, t_codebyte_ref * codebyte_ref; CFG_FOREACH_CODEBYTE, CodebyteNextInChainInit, CodebyteNextInChainFini, CodebyteNextInChainDup);
DYNAMIC_MEMBER(codebyte, t_cfg *, codebyte_next_in_chain_array, t_codebyte *, next_in_chain, NEXT_IN_CHAIN, NextInChain, t_codebyte_ref * codebyte_ref; CFG_FOREACH_CODEBYTE, CodebytePrevInChainInit, CodebytePrevInChainFini, CodebytePrevInChainDup);

#endif
#endif
