/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

#ifdef __cplusplus
extern "C" {
#endif

#include <diabloanopti386.h>
#include <diablodiversity.h>
#include "diablodiversity_cmdline.h"

 
#ifdef __cplusplus
}
#endif

#include <obfuscation/obfuscation_architecture_backend.h>
#include <obfuscation/architecture_backends/i386/i386_architecture_backend.h>

t_bool TrueWithChance(int chance)
{
  int j = diablo_rand_next_range(0, 99); //(int) (99 * 1.0 * (diablo_rand_next() / (DIABLO_RAND_MAX + 1.0)));
  if(j<chance)
    return TRUE;
  return FALSE;
}

/* FORWARD_TO_ITERATIVE is (un)set by diablo_diversity_config.h.in */

static int debug_support = 0;
#if 0
#define DEBUG_CHECK() (debug_support++ < diablosupport_options.debugcounter)
#else
#define DEBUG_CHECK() (TRUE)
#endif

t_int32 DiversityEngine(t_cfg * cfg)
{
  new I386ObfuscationArchitectureInitializer();
  flatten_always_enabled = true;
#ifdef FORWARD_TO_ITERATIVE
  return DiversityEngineIterative(cfg);
#else
  t_diversity_options ret;
  t_diversity_choice choice;

  /*{{{
    {
    t_function * fun;
    int i = 0;
    CFG_FOREACH_FUN(cfg,fun)
    if (FUNCTION_BBL_FIRST(fun)&&!FunIsFrozen(fun))
    i++;
    VERBOSE(0,("WATCHME %d functions",i));
    }
    {
    t_function * fun;
    int i = 0;
    CFG_FOREACH_FUN(cfg,fun)
    if (FUNCTION_BBL_FIRST(fun))
    i++;
    VERBOSE(0,("WATCHME %d functions",i));

    i=0;
    int j =0;
    t_bbl * bbl;
    CFG_FOREACH_BBL(cfg,bbl)
    {
    if(BBL_NINS(bbl)!=0 && !BBL_IS_HELL(bbl))
    {
    i++;
    j+=BBL_NINS(bbl);
    }
    }
    VERBOSE(0,("WATCHME %d basic blocks",i));
    VERBOSE(0,("WATCHME %d instructions",j));
    }
    }}}*/

  DiabloBrokerCallInstall("RegisterBblFactoring", "t_bbl *, t_bbl *", (void*) RegisterBblFactoring, FALSE);
  DiabloBrokerCallInstall("RegisterFunFactoring", "t_function *", (void*) RegisterFunFactoring, FALSE);
  DiabloBrokerCallInstall("EpilogueFactorBefore", "t_bbl *, t_bbl *", (void*) RegisterBblFactoring, FALSE);
  diablo_rand_seed(diablodiversity_options.div_random_seed);

  /*{{{factor functions*/
  if(diablodiversity_options.div_factor_functions)
  {
	int i =0;
    VERBOSE(0,("FUNCTION FACTORING"));
    ret = DiversityFactorFunctions(cfg,NULL,0);
    choice.flagList = BoolListNewAllTrue(ret.range);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i] = TrueWithChance(diablodiversity_options.div_factor_functions_chance);
        VERBOSE(0, ("FUNCTION FACTORING: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i] = FALSE;
    }
    DiversityFactorFunctions(cfg, &choice,1);
  }
  /*}}}*/

  /*{{{factor epilogues*/
  if(diablodiversity_options.div_factor_epilogues)
  {
	int i = 0;
    VERBOSE(0,("EPILOGUE FACTORING"));
    ret = DiversityFactorEpilogues(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    choice.flagList = BoolListNewAllTrue(ret.range);
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_factor_epilogues_chance);
        VERBOSE(0, ("EPILOGUE FACTORING: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i] = FALSE;
    }
    DiversityFactorEpilogues(cfg, &choice,1);
  }
  /*}}}*/

  /*{{{factor bbls*/
  if(diablodiversity_options.div_factor_bbls)
  {
	int i = 0;
    VERBOSE(0,("BBL FACTORING"));
    ret = DiversityFactorBbls(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    choice.flagList = BoolListNewAllTrue(ret.range);
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_factor_bbls_chance);
        VERBOSE(0, ("BBL FACTORING: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i] = FALSE;
    }
    DiversityFactorBbls(cfg, &choice,1);
  }
  /*}}}*/

  /*{{{inline functions*/
  if(diablodiversity_options.div_inline_functions)
  {
    VERBOSE(0,("FUNCTION INLINING"));
    ret= DiversityInlineFunctions(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    {
      int i = 0;
      choice.flagList = BoolListNewAllTrue(ret.range);
      for(i=0;i<choice.flagList->count;i++)
      {
        if (DEBUG_CHECK()) {
          choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_inline_functions_chance);
          VERBOSE(0, ("FUNCTION INLINING: %i %i", debug_support, choice.flagList->flags[i]));
        } else
          choice.flagList->flags[i] = FALSE;
      }
    }
    DiversityInlineFunctions(cfg, &choice,1);
  }
  /*}}}*/

  /*split by two-way opaque predicate{{{*/
  if(diablodiversity_options.div_inline_two_way_predicates)
  {
    VERBOSE(0,("SPLIT BY TWO_WAY OPAQUE PREDICATES"));
    ret = DiversitySplitByTwoWayPredicate(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %llu",ret.range));
    {
	  int i = 0;
      choice.flagList = BoolListNewAllTrue(ret.range);
      for(i=0;i<choice.flagList->count;i++)
      {
        if (DEBUG_CHECK()) {
          choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_inline_two_way_predicates_chance);
          VERBOSE(0, ("TWOWAY: %i %i", debug_support, choice.flagList->flags[i]));
        } else
          choice.flagList->flags[i]= FALSE;
      }
    }
    DiversitySplitByTwoWayPredicate(cfg, &choice, 1);
  }
  /*}}}*/

  /*inline bbl{{{*/
  if(diablodiversity_options.div_inline_bbls)
  {
    VERBOSE(0,("BBL INLINING"));
    ret = DiversityUnfold(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %llu",ret.range));
    {
	  int i = 0;
      choice.flagList = BoolListNewAllTrue(ret.range);
      for(i=0;i<choice.flagList->count;i++)
      {
        if (DEBUG_CHECK()) {
          choice.flagList->flags[i]=TrueWithChance(diablodiversity_options.div_inline_bbls_chance);     
          VERBOSE(0, ("BBL INLINING: %i %i", debug_support, choice.flagList->flags[i]));
        } else
          choice.flagList->flags[i] = FALSE;
      }
    }
    DiversityUnfold(cfg, &choice, 1);
  }
  /*}}}*/

  /*{{{flatten*/
  if(diablodiversity_options.div_obfuscation_flatten)
  {
	int i = 0;
    VERBOSE(0,("CFG FLATTENING"));
    ret = DiversityFlatten(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    choice.flagList = BoolListNewAllTrue(ret.range);
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_obfuscation_flatten_chance);
        VERBOSE(0, ("CFG Flattening: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i] = FALSE;
    }
    ret=DiversityFlatten(cfg,&choice,1);
    while(!ret.done)
    {
      choice.flagList = BoolListNewAllTrue(ret.range);
      ret=DiversityFlatten(cfg,&choice,2);
    }
  }
  /*}}}*/

  /*{{{static disassembly thwarting*/
  if(diablodiversity_options.div_obfuscation_static_disassembly_thwarting)
  {
	int i = 0;
    VERBOSE(0,("DISASSEMBLY THWARTING"));
    ret = DiversityThwartDisassembly(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));

    choice.flagList = BoolListNewAllTrue(ret.range);
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]=TrueWithChance(diablodiversity_options.div_obfuscation_static_disassembly_thwarting_chance);
        VERBOSE(0, ("Thwarting: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i] = FALSE;
    }
    ret=DiversityThwartDisassembly(cfg,&choice,1);
  }
  /*}}}*/

  /*{{{opaque predicates*/
  if(diablodiversity_options.div_obfuscation_opaque_predicates)
  {
	int i = 0;
    VERBOSE(0,("OPAQUE PREDICATES"));
    /*ask which bbls can be preceeded with an opaque predicate*/
    ret = DiversityOpaquePredicates(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));

    /*tell which ones to precede with an opaque predicate (percentage of total)*/
    choice.flagList = BoolListNewAllTrue(ret.range);
    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_obfuscation_opaque_predicates_chance);
        VERBOSE(0, ("Op_pred: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i]=FALSE;
    }
    ret = DiversityOpaquePredicates(cfg, &choice,1);

    /*choose an opaque predicate (loops for every selected basic block if multiple choices are available*/
    while(!ret.done)
    {
      int random_number = diablo_rand_next_range(0, ret.range); //(int)((ret.range+1)*(double)diablo_rand_next()/((double)DIABLO_RAND_MAX+(double)1));
      choice.choice = random_number;
      ret=DiversityOpaquePredicates(cfg,&choice,2);
    }
  }
  /*}}}*/

  /*{{{branch flipping*/
  if(diablodiversity_options.div_backend_branch_flipping)
  {
	int i = 0;
    VERBOSE(0,("BRANCH FLIPPING"));
    ret = DiversityFlipBranches(cfg,NULL,0);
    /*
       choice.flagList = BoolListNewAllFalse(ret.range);
       int i=0;
       for(i=0;i<diablosupport_options.debugcounter && i <choice.flagList->count;i++)
       {
       choice.flagList->flags[i]=TRUE;
       }
       */
    choice.flagList = BoolListNewAllTrue(ret.range);

    for(i=0;i<choice.flagList->count;i++)
    {
      if (DEBUG_CHECK()) {
        choice.flagList->flags[i]= TrueWithChance(diablodiversity_options.div_backend_branch_flipping_chance);
        VERBOSE(0, ("Flipping: %i %i", debug_support, choice.flagList->flags[i]));
      } else
        choice.flagList->flags[i]= FALSE;
    }


    DiversityFlipBranches(cfg, &choice, 1);
  }
  /*}}}*/

  /*{{{instruction selection*/
  if(diablodiversity_options.div_backend_instruction_selection)
  {
    VERBOSE(0,("INSTRUCTION REPLACING"));
    choice.choice = 0;
    do{
	  int random_number;
      if (DEBUG_CHECK()) {
        VERBOSE(0, ("instruction replacing..."));
      } else
        goto over;
      ret = DiversityInstructionSelection(cfg, &choice, 0);
      //VERBOSE(0,("WATCHME: %d",ret.range));
      if(TrueWithChance(diablodiversity_options.div_backend_instruction_selection_chance))
	random_number = diablo_rand_next_range(0, ret.range); //(int)((ret.range+1)*(double)diablo_rand_next()/((double)DIABLO_RAND_MAX+(double)1));
      else 
	random_number = -1;
      choice.choice = random_number;
    }while(!ret.done);
    over: ;
  }
  /*}}}*/

  /*{{{limit instruction set*/
  if(diablodiversity_options.div_limit_n_opcodes > 0)
  {
    VERBOSE(0,("INSTRUCTION SET LIMITATION"));
    DiversityLimitInstructionSet(cfg,NULL,diablodiversity_options.div_limit_n_opcodes);
  }
  /*}}}*/

  /*{{{scheduling*/
  if(diablodiversity_options.div_backend_scheduling)
  {
    VERBOSE(0,("INSTRUCTION SCHEDULING"));
    choice.choice = 0;
    do{
	  int random_number;
      if (DEBUG_CHECK()) {
        VERBOSE(0, ("INST_SCHEDULE: %i", debug_support));
      } else
        goto over2;
      
      ret = DiversityScheduleBbls(cfg, &choice, 0);
      //VERBOSE(0,("WATCHME: %d",ret.range));
      random_number = diablo_rand_next_range(0, ret.range); //(int)((ret.range+1)*(double)diablo_rand_next()/((double)DIABLO_RAND_MAX+(double)1));
      choice.choice = random_number;
    }while(!ret.done);
    over2: ;
  }
  /*}}}*/

  /*{{{layout*/
  if(diablodiversity_options.div_backend_code_layout)
  {
    VERBOSE(0,("CODE LAYOUT"));
    ret = DiversityLayout(cfg,NULL,0);
    //VERBOSE(0,("WATCHME: %d",ret.range));
    while(ret.done != TRUE)
    {
      int random_number = diablo_rand_next_range(0, ret.range); //(int)((ret.range+1)*(double)diablo_rand_next()/((double)DIABLO_RAND_MAX+(double)1));
      choice.choice = random_number;
      ret = DiversityLayout(cfg,&choice,1);
    }
  }
  /*}}}*/

  
  if(diablodiversity_options.div_smc_factor1)
  {
#ifdef _MSC_VER
	  FATAL(("SMC Not Supported yet on Win32"));
#else
    ret = DiversitySmcFactor(cfg,NULL,0);
    while(ret.done != TRUE)
    {
      choice.choice = TrueWithChance(diablodiversity_options.div_smc_factor1_chance);
      ret = DiversitySmcFactor(cfg,&choice,1);
    }
#endif
  }
  
  if(diablodiversity_options.div_smc_factor4)
  {
#ifdef _MSC_VER
	  FATAL(("SMC Not Supported yet on Win32"));
#else

    ret = DiversitySmcFactor(cfg,NULL,2);
    while(ret.done != TRUE)
    {
      choice.choice = TrueWithChance(diablodiversity_options.div_smc_factor4_chance);
      ret = DiversitySmcFactor(cfg,&choice,3);
    }
#endif
  }

  return 0;
#endif
}
