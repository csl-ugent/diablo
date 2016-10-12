/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */



#ifndef OBF_OPAQUE
#define OBF_OPAQUE

#include <diabloflowgraph.h>
#include <diabloanopt.h>
#include <diabloi386.h>
#include <diabloanopti386.h>
#include <diablodiversity.h>

typedef struct _opaque_predicate
{
  t_const_string info;
  t_const_string file_name;
  size_t file_length;
  t_uint8* raw_data;
  t_i386_condition_code ccode;
  t_regset * used;
  t_reg based_on[5]; //The registers that are used to compute the true/false opaque predicate
} opaque_predicate;

void OpaqueInit(t_cfg * cfg);
void AddOpaqueBblGui(t_bbl * bbl, opaque_predicate * op, t_ins * after, t_bool true_opaque_predicate, t_bbl * wrong_target);
t_regset CheckOpaqueBblGui(t_bbl * bbl, opaque_predicate * op, t_ins * after, t_bool recompute);
t_i386_condition_code AddOpaquePredCMov(t_bbl * bbl, opaque_predicate * op, t_i386_ins * before, t_bool true_opaque_predicate);
void AddArboitPredicate(t_bbl * bbl, t_bbl *fake_bbl);
void AddArboitPredicateSum(t_bbl * bbl, t_bbl * fake_bbl);
t_diversity_options DiversityOpaquePredicates(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);


t_arraylist* DiversityOpaquePredicatesCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
t_diversity_options DiversityOpaquePredicatesDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info);


#endif

/* vim: set shiftwidth=2: */
