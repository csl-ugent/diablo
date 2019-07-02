#include <diabloanopt.h>

#ifndef DIABLOANOPT_FUNCTIONS
#ifndef DIABLOANOPT_LIVENESS_FUNCTIONS
#define DIABLOANOPT_LIVENESS_FUNCTIONS
void   CfgComputeSavedChangedRegisters(t_cfg *);
void   CfgComputeLiveness(t_cfg * cfg, t_analysis_complexity level);
void   CfgComputeUselessRegisters (t_cfg *cfg);
void   CfgDumpWithLiveness(t_cfg * cfg, t_string dotsdir, t_bool livebefore, t_bool liveafter, t_bool compute_fresh);
t_bool CfgKillUselessInstructions (t_cfg *cfg);

/* needed in AF */
t_regset BblComputeLiveInFromLiveOut(t_bbl * bbl, t_bool update, t_bool with_kill_useless);
#endif
#endif
