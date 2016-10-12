#include <diablosmc.h>

#ifdef DIABLOSMC_FUNCTIONS
void SmcBranchForwarding(t_cfg * cfg);
t_uint32 SmcFactorInit(t_cfg * cfg, t_codebyte *** leaders);
void SmcFactorFini(t_cfg * cfg, t_codebyte *** leaders);

t_bbl *  SmcFactorWithOneByteModifier(t_codebyte * codebyte_to_write, t_bool i_first, t_uint32 i, t_uint32 j, t_codebyte ** leaders, t_cfg * cfg, t_uint32 valuej, t_bbl * head_of_merged_chain);
void SmcFactorWithFourByteModifier(t_bool i_first, t_uint32 i, t_uint32 j, t_codebyte ** leaders, t_cfg * cfg);

t_codebyte * SmcChainDistance1(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2, t_uint32 * valuej);
t_bool SmcChainDistance4(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2);
#endif
