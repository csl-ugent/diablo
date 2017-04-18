/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* {{{ Copyright
 * Copyright 2012 Bart Coppens, Ghent University
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

extern "C" {
#include <diablodiversity.h>
#include "diablodiversity_cmdline.h"
}

t_arraylist* DiversityEngineReadIterativeFile(FILE* file) {
  t_arraylist* fun_info = ArrayListNew(sizeof(t_diversity_function_info*), 10);


  while(!feof(file)) {
    t_diversity_function_info* info = (t_diversity_function_info*) Malloc(sizeof(t_diversity_function_info));
    IGNORE_RESULT(fscanf(file, "%x,%i,%i,%i,%i,%i\n",
        &(info->function_address),
        &(info->seed),
        &(info->transformations_left),
        &(info->max_cost),
        &(info->times_iterated_with_current_cost),
                         &(info->should_redo)));
    ArrayListAdd(fun_info, &info);
  }

  ArrayListSort(fun_info, order_funinfo_retransform);

  return fun_info;
}

void AddReasonedTransformationToFunction(t_function* function, int reason) {
  /* Add this function to the list of functions to be transformed again*/
  SetFunctionAsMapped(function);

  /* Add the transform(s) belonging to the BinDiff reason code */
  switch(reason) {
    case BD_REASON_EDGES_HASH_MATCHING:
      VERBOSE(0, ("Function '%s' WAS matched by BinDiff, reason: Hash Matching!", FUNCTION_NAME(function)));
      AddPossibleTransform(function, transformationDiversityFlipBranches);
      /* TODO: might also want to add instruction substitution for those without conditional jumps? */
      break;
    case BD_REASON_EDGES_FLOWGRAPH:
      VERBOSE(0, ("Function '%s' WAS matched by BinDiff, reason: Edges Flowgraph!", FUNCTION_NAME(function)));
      AddPossibleTransform(function, transformationDiversityFlipBranches);
      AddPossibleTransform(function, transformationDiversitySplitByTwoWayPredicate);
      AddPossibleTransform(function, transformationDiversityFlattenFunction); /* TODO! Partial flattening! */
      /* TODO: flip branches! */
      break;
    case BD_REASON_EDGES_CALLGRAPH:
      VERBOSE(0, ("Function '%s' WAS matched by BinDiff, reason: Edges Callgraph!", FUNCTION_NAME(function)));
      /* TODO! Both have to be inserted before callers/callees only! */
      AddPossibleTransform(function, transformationDiversityThwartDisassembly);
      AddPossibleTransform(function, transformationDiversitySplitByTwoWayPredicate);
      break;
    default: VERBOSE(0, ("Not adding transformation for unsupported reason: %i", reason));
  }
}

void DiversityEngineAddToIterativeFile(FILE* file, t_function* fun, int seed, int transformations_left, int max_cost, int times_iterated_with_current_cost, int redo) {
  t_address first_address;

  if (FUNCTION_IS_HELL(fun))
    return;

  first_address = address_of_function(fun);

  fprintf(file, "%x,%i,%i,%i,%i,%i\n", first_address, seed, transformations_left, max_cost, times_iterated_with_current_cost, redo);
}

void DiversityReadAndProcessIterativeFilePair(FILE* existingTransformFile, FILE* badlymappedInstructions, t_cfg* cfg) {
  t_hash_table* insmap;
  t_hash_table* funmap;
  t_ins* ins;
  t_function* fun;
  t_address_hash_entry* entry;

  /* First add all transformations that were applied in previous iterations */
  funmap = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  CFG_FOREACH_FUN(cfg, fun) {
    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = address_of_function(fun);
    entry->data    = fun;

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(funmap,entry);
  }

  while(existingTransformFile && !feof(existingTransformFile)) {
    // t_diversity_function_info* info = Malloc(sizeof(t_diversity_function_info));
    t_address fun_addr;
    char transform_name[100];

    IGNORE_RESULT(fscanf(existingTransformFile, "%x,%s\n", &fun_addr, transform_name));

    /* Sometimes we 'read in' a line from an empty file, weirdly enough? This works around that */
    if (fun_addr == 0) {
      VERBOSE(0, ("Cannot have a function with address 0, skipping! (Applied transform '%s')", transform_name));
      continue;
    }

    entry = (t_address_hash_entry*) HashTableLookup(funmap, &fun_addr);
    if (entry) {
      t_transformation_info* transform;
      VERBOSE(0, ("Function '%s' had applied the following transformation: %s", FUNCTION_NAME((t_function*)(entry->data)), transform_name));
      
      transform = GetTransformByName(transform_name);

      if (!transform) {
        FATAL(("Reading back of transform '%s' not yet supported!", transform_name));
      }

      AddPossibleTransform((t_function*)(entry->data), transform);
    }
  }


  /* Now look at all instructions that have been matched and why. For those instruction's functions,
     we add to their list of possible transformations the required transformations.
     Furthermore, all these instructions belong to functions that we have to match with new parameters/increased cost! */
  insmap = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  CFG_FOREACH_INS(cfg, ins) {

    if (! ( INS_CADDRESS(ins) && INS_BBL(ins) && BBL_FUNCTION(INS_BBL(ins)) ) )
      continue;

    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = INS_CADDRESS(ins);
    entry->data = BBL_FUNCTION(INS_BBL(ins));

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(insmap,entry);
  }

  while(!feof(badlymappedInstructions)) {
    t_address ins_addr;
    int why;

    IGNORE_RESULT(fscanf(badlymappedInstructions, "%x,%i\n", &ins_addr, &why));

    entry = (t_address_hash_entry*) HashTableLookup(insmap, &ins_addr);
    if (entry) {
      /* Also sets the function as needing to be re-transformed */
      AddReasonedTransformationToFunction((t_function*)(entry->data), why);
    }
  }

  HashTableFree(insmap);
  HashTableFree(funmap);
}

/* {{{ Transforming functions */
int TransformFunction(t_cfg* cfg, t_function* fun, t_arraylist* transformations, int seed, int max_cost) {
  int notapplied_count = 0;
  int i;
  int current_cost = 0;
  t_bool applied_transform = FALSE;
  t_transformation_cost_info* transform;
  t_arraylist* possible_transforms;

  if (address_of_function(fun) == 0) {
    VERBOSE(0, ("The 'Diablo' address of function %p (%s) is 0, THIS SHOULD NOT HAPPEN!", fun, FUNCTION_NAME(fun)));
    return 0;
  }

  t_randomnumbergenerator* rng = RNGCreateBySeed(seed, "transformfunction");

  // For inline functions, we want this to be run immediately on all functions?! TODO
  /*if (FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(fun))) {
    FunctionUnmarkAllBbls(fun);
    FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);
  }*/

  ExtraOutput("BEFORE", cfg, fun);
  
  possible_transforms = ListPossibleTransformations(cfg, fun, transformations, max_cost);
  VERBOSE(1, ("For function %s there are %i possible transformations!", FUNCTION_NAME(fun), possible_transforms->current_length));

  do {
    /*
       As long as we do not apply a transformation, we can just iterate further over the transforms.
       But once we apply a transformation, BBLs might be added/changed/removed => restart!
    */
    applied_transform = FALSE;
    RNGSetRange(rng, 0, possible_transforms->current_length -1);

    while(possible_transforms->current_length > 0) {
      int chosen_transform = 0;
      t_transformation_cost_info* transform = NULL;

      if (possible_transforms->current_length > 1) {
        chosen_transform = RNGGenerate(rng);
      } else {
        chosen_transform = 0;
      }

      transform = ARRAYLIST_GET(possible_transforms, chosen_transform, t_transformation_cost_info*);
      ArrayListRemove(possible_transforms, chosen_transform);

      VERBOSE(0, ("Maybe applying %s", transform->transformation->name));

      /* Keep track that we did (tried to) apply this transformation to this BBL, so that we don't apply the same
         transformation needlessly twice to undo itself, and that we do not end up with a binary where only the first
         basic blocks have been transformed */
      if (transform->transformation->whole_function_transform) {
        ArrayListAdd(FUNCTION_TRANSFORMATIONS(fun), &transform->transformation);
      } else {
        ArrayListAdd(BBL_TRANSFORMATIONS(transform->bbl), &transform->transformation);
      }

      //if (TrueWithChance(50)) { // TODO: also use user-configurable chances, or is cost enough?
      /* Since we pick a random transformation from the list of transforms, if we only want single diversity we can just always
         pick this choice */
      if (TRUE) {
        VERBOSE(0, ("Applying %s on a BBL of %s ", transform->transformation->name, FUNCTION_NAME(fun)));
        VERBOSE(4, ("Cost is now %i out of a maximum of %i", current_cost, max_cost));

        /* Since we re-create the entire list each time we increase the cost, we know in advance that adding this will not make cost > max_cost */
        applied_transform = TRUE;
        current_cost += transform->cost;
        transform->transformation->do_transform(cfg, fun, transform->bbl, transform->additional_info, rng);

        break;
      }

      /* TODO: provide a destructor for this data? */
      if (transform->additional_info)
        Free(transform->additional_info);
      Free(transform);
    }

    if (applied_transform) {
      FreeTransformations(possible_transforms);
      /* If we added transforms, the current maximum cost a transform can have will be less than max_cost, of course */
      possible_transforms = ListPossibleTransformations(cfg, fun, transformations, max_cost - current_cost);
    }

  } while(applied_transform && current_cost <= max_cost && possible_transforms->current_length > 0 /* defensive programming */ );

  notapplied_count = possible_transforms->current_length;

  FreeTransformations(possible_transforms);
  RNGDestroy(rng);

  ExtraOutput("AFTER", cfg, fun);

  return notapplied_count;
}



void FunctionHashTableTransformFunctionSingleSeed(void* e, void* data) {
  t_transform_function_helper* helper = (t_transform_function_helper*) data;
  t_address_hash_entry* entry = (t_address_hash_entry*) e;
  int i;
  t_diversity_function_info* fun_info;
  t_function* fun = (t_function*)entry->data;

  /* We are walking over a hash set of untransformed functions. Transform this function!
     If we already visited this function in a previous iteration, increase that info, otherwise start from scratch. */
  /* Default config */
  int rand_seed = RNGGenerate(RNGGetRootGenerator());
  int max_cost = diablodiversity_options.div_initial_max_cost;
  int times_iterated_with_current_cost = 1;
  int transformations_left;

  //VERBOSE(0, ("Welcome to function '%s'", FUNCTION_NAME(fun)));

  FOREACH_ARRAY_ITEM(helper->previous_information, i, fun_info, t_diversity_function_info*) {
    if (fun_info->function_address != entry->address)
      continue;

#ifndef ITERATIVE_PER_FUNCTION_EXTERNALLY_DECIDED
    /* If the function is not mapped but was transformed previously, we don't change the costs/settings as compared to the previous iteration */
    if (! functionIsMapped(fun)) {
      max_cost = fun_info->max_cost;
      times_iterated_with_current_cost = fun_info->times_iterated_with_current_cost;
      rand_seed = fun_info->seed;
      VERBOSE(0, ("Function '%s' was not matched by BinDiff, transforming same as before (%i, %i) ", FUNCTION_NAME(fun), rand_seed, max_cost));
      break;
    }
    /* It was mapped previously, but there were still instructions in this function matched. Retry... */
#endif
    VERBOSE(0, ("Function '%s' WAS matched by BinDiff, transforming differently!", FUNCTION_NAME(fun)));

    /* We need to (re)transform this function. If the curent amount of iterations is low enough compared to transformations left,
       just use the new seed, otherwise increase the max_cost and reset the number of iterations */
    // TODO, this should be analyzed to have a tighter bound?
    if ( fun_info->times_iterated_with_current_cost >= fun_info->transformations_left / diablodiversity_options.div_restart_divider
      || fun_info->times_iterated_with_current_cost >= diablodiversity_options.div_max_with_cost) {
      // We've had enough iterations with the current cost, increase cost!
      times_iterated_with_current_cost = 0;
      max_cost = diablodiversity_options.div_cost_increase + fun_info->max_cost;
    } else {
      // Another iteration with the current cost
      max_cost = fun_info->max_cost;
      times_iterated_with_current_cost = fun_info->times_iterated_with_current_cost + 1;
    }

    /* We want to be _really_ sure that we use a different random number seed than the previous iteration.
     * If we just do RNGGenerate we will generate a seed that has already been used for another function,
     * therefore create a new, temporary RNG with a new seed. For this seed we use non-linearity so that
     * we do not just reproduce the same randomness with a single offset.
     */
    t_randomnumbergenerator* rng = RNGCreateBySeed(42 * fun_info->seed * fun_info->seed + 1337, "singleseed_temporary_reseed");
    rand_seed = RNGGenerate(rng);
    RNGDestroy(rng);

    break;
  }

  /* Transform this function and log this to file */
#ifdef ITERATIVE_PER_FUNCTION_EXTERNALLY_DECIDED
  VERBOSE(1, ("Transforming %p with new seed", entry->address));
  transformations_left = TransformFunction(helper->cfg, fun, helper->transformations, rand_seed, max_cost);
#else

  //VERBOSE(0, ("Transforming 
  transformations_left = TransformFunction(helper->cfg, fun, FUNCTION_POSSIBLE_TRANSFORMATIONS(fun), rand_seed, max_cost);
#endif

  DiversityEngineAddToIterativeFile(helper->outfile,
                                    fun,
                                    rand_seed,
                                    transformations_left,
                                    max_cost,
                                    times_iterated_with_current_cost,
                                    FALSE /* redo differently? */);


  /* TODO: remove entry? */
}


/* }}} */


void InitStrategySingleSeed(t_cfg* cfg) {
  if ( diablodiversity_options.div_iterative_allowed_transforms_file && strcmp(diablodiversity_options.div_iterative_allowed_transforms_file, "") != 0
    && diablodiversity_options.div_matched_instructions_and_why_file && strcmp(diablodiversity_options.div_matched_instructions_and_why_file, "") != 0) {
    FILE* allowed_transforms_file = fopen(diablodiversity_options.div_iterative_allowed_transforms_file, "r"); /* Might be empty! */
    FILE* matched_instructions_file = fopen(diablodiversity_options.div_matched_instructions_and_why_file, "r");

    VERBOSE(0, ("allowed: %s %p", diablodiversity_options.div_iterative_allowed_transforms_file, allowed_transforms_file));
    VERBOSE(0, ("matched: %s %p", diablodiversity_options.div_matched_instructions_and_why_file, matched_instructions_file));

    DiversityReadAndProcessIterativeFilePair(allowed_transforms_file, matched_instructions_file, cfg);

    if (allowed_transforms_file)
      fclose(allowed_transforms_file);
    fclose(matched_instructions_file);
  }
}


void StrategySingleSeedDoTransforms(FILE* iterative_file, t_arraylist* previous_information, t_arraylist* transformations, t_cfg* cfg, t_hash_table* functions_table) {
  t_transform_function_helper helper;

  helper.previous_information = previous_information;
  helper.transformations = transformations;
  helper.cfg = cfg;
  helper.outfile = iterative_file;

  /* Actually transform all the functopns */
  HashTableWalk(functions_table, FunctionHashTableTransformFunctionSingleSeed, &helper);

}

void FinishStrategySingleSeed(t_cfg* cfg) {
  if ( diablodiversity_options.div_iterative_allowed_transforms_output_file
    && strcmp(diablodiversity_options.div_iterative_allowed_transforms_output_file, "") != 0 ) {
    FILE* allowed_transforms_file = fopen(diablodiversity_options.div_iterative_allowed_transforms_output_file, "w");
    t_function* fun;

    CFG_FOREACH_FUN(cfg, fun) {
      if (address_of_function(fun)) {
        t_transformation_info* info;
        int i;
        FOREACH_ARRAY_ITEM(FUNCTION_POSSIBLE_TRANSFORMATIONS(fun), i, info, t_transformation_info*) {
          fprintf(allowed_transforms_file, "%p,%s\n", (void*)address_of_function(fun), info->name);
        }
      }
    }

    fclose(allowed_transforms_file);
  }
}
