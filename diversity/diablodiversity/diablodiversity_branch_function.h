/***********************************************************
 * LOCO files
 *
 * Matias Madou
 *
 * mmadou@elis.ugent.be
 **********************************************************/

#ifndef OBF_BRANCH_FUNCTION
#define OBF_BRANCH_FUNCTION

#include <diabloflowgraph.h>
#include <diabloi386.h>
#include <diablodiversity.h>

t_diversity_options DiversityThwartDisassembly(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);
t_uint32 AddBranchFunctionCallToFunction(t_function * fun, t_bbl * b_bbl);
t_bbl * AddBranchFunction(t_object * obj);
t_bbl * AddBranchFunctionToCfg(t_cfg * cfg);
t_bool TransformJumpIntoCallToBranchFunction(t_bbl * bbl);
t_bool TransformAllJumpsIntoCallsToBranchFunction(t_function * fun);
  
t_arraylist* DiversityThwartDisassemblyCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
t_diversity_options DiversityThwartDisassemblyDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info);


t_arraylist* DiversityBranchFunctionBeforeAndAfterCallsCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
t_diversity_options DiversityBranchFunctionBeforeAndAfterCallsDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info);

t_arraylist* DiversityBranchFunctionBeforeAndAfterCallsGlobalVarCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
t_diversity_options DiversityBranchFunctionBeforeAndAfterCallsGlobalVarDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info);

t_arraylist* DiversityBranchFunctionInFirstBlockCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
t_diversity_options DiversityBranchFunctionInFirstBlockDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info);


#endif

