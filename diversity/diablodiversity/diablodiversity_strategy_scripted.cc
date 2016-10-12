/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

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
#include <diabloanopti386.h>
#include <diablodiversity.h>
#include "diablodiversity_cmdline.h"
}

void AddCallerMatchingReasons(t_cfg* cfg, t_bool removeReasonFromCallee);
void ReadTransformationRules(const char* file);

/* {{{ Read 'script' file */
/* Format is as follows:
functionaddress,seed,reason,MAXCOST(TODO),#transformations following
cost1,DiversityTransformationName1,
cost2,DiversityTransformationName2
functionaddress,seed,reason,MAXCOST(TODO),#transformations following
etc
*/

FILE* output_scripts;

typedef struct _t_script_choice t_script_choice;
struct _t_script_choice {
  int max_cost; // TODO! This is currently the max nr of applied transforms if their cost == 1
  int* costs; /* This is an array of [0..current_transform_max_idx] of the costs for the associated transform */
  int seed;
  int reason;
};

void FreeChoice(t_script_choice* choice) {
  if (choice) {
    Free(choice->costs);
    Free(choice);
  }
}



static t_hash_table* function_scripts = NULL;

void AddScriptToFunction(t_address fun_addr, t_script_choice* choice) {
  t_address_hash_entry* entry;
  t_arraylist* script;

  if (!function_scripts) {
      function_scripts = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);
  }
  
  entry = (t_address_hash_entry*) HashTableLookup(function_scripts, &fun_addr);
  
  if (!entry) {
    script = ArrayListNew(sizeof(t_script_choice*), 10);

    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = fun_addr;
    entry->data = script;

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(function_scripts,entry);
  } else {
    script = (t_arraylist*) entry->data;
  }

  ArrayListAdd(script, &choice);
}

t_arraylist* GetListForFunction(t_function* fun) {
  t_address_hash_entry* entry;
  t_address fun_addr = address_of_function(fun);

  if (!function_scripts)
    return NULL;

  entry = (t_address_hash_entry*) HashTableLookup(function_scripts, &fun_addr);
  if (entry) {
    return (t_arraylist*) entry->data;
  }
  return NULL;
}

void WriteScriptChoiceToFile(FILE* file, t_function* fun, t_script_choice* choice) {
  int following = 0;
  int i;

  for (i = 0; i < current_transform_max_idx; i++) {
    if (choice->costs[i] > 0) {
      following++;
    }
  }

  fprintf(file, "%" PRIuPTR ",%i,%i,%i,%i\n", address_of_function(fun), choice->seed, choice->reason, choice->max_cost, following);

  for (i = 0; i < current_transform_max_idx; i++) {
    if (choice->costs[i] > 0) {
      fprintf(file, "%i,%s\n", choice->costs[i], supported_transforms[i]->name);
    }
  }
}

void ReadScriptFile(const char* filename) {
  FILE* script = fopen(filename, "r");
  t_bool read_ok = TRUE;

  while (script && read_ok && !feof(script)) {
    t_address functionaddress;
    int following;
    int i;
    int items_read = 0;

    t_script_choice* choice = (t_script_choice*) Malloc(sizeof(t_script_choice));
    choice->costs = (int*) Malloc(current_transform_max_idx * sizeof(int));
    for (i = 0; i < current_transform_max_idx; i++)
      choice->costs[i] = 0;

    items_read = fscanf(script, "%x,%i,%i,%i,%i\n", &functionaddress, &(choice->seed), &(choice->reason), &(choice->max_cost), &following);

    if (items_read != 5) {
      VERBOSE(0, ("Couldn't parse a line in the script file, stopped reading script file"));
      read_ok = FALSE;
      continue;
    }

    for (i = 0; i < following; i++) {
      char transform_name[100];
      int cost;
      t_transformation_info* transform;
      fscanf(script, "%i,%s\n", &cost, transform_name);

      transform = GetTransformByName(transform_name);
      choice->costs[transform->idx] = cost;
    }

    AddScriptToFunction(functionaddress, choice);
  }

  fclose(script);
}

static t_hash_table* function_mapped_why = NULL;

t_bool isReasonForMappingInList(t_arraylist* list, int reason) {
  int i;

  for (i = 0; i < list->current_length; i++) {
    if (ARRAYLIST_GET(list, i, int) == reason) {
      return TRUE;
    }
  }
  return FALSE;
}

void removeReasonForMappingInList(t_arraylist* list, int reason) {
  int i;

  for (i = 0; i < list->current_length; i++) {
    if (ARRAYLIST_GET(list, i, int) == reason) {
      ARRAYLIST_GET(list, i, int) = -1;
      return;
    }
  }
}


void AddReasonForMapping(t_function* fun, int reason) {
  t_address_hash_entry* entry;
  t_arraylist* mapped_why;

  if (!function_mapped_why) {
      function_mapped_why = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);
  }

  entry = (t_address_hash_entry*) HashTableLookup(function_mapped_why, &fun);

  if (!entry) {
    mapped_why = ArrayListNew(sizeof(int), 10);

    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = (uintptr_t) fun;
    entry->data = (void*) mapped_why;

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(function_mapped_why,entry);
  } else {
    mapped_why = (t_arraylist*) entry->data;
  }

  if (!isReasonForMappingInList(mapped_why, reason)) {
    ArrayListAdd(mapped_why, &reason);
  }
}

t_arraylist* GetReasonsForBeingMatched(t_function* fun) {
  t_address_hash_entry* entry;
  t_arraylist* mapped_why;

  if (!function_mapped_why) {
    return NULL;
  }

  entry = (t_address_hash_entry*) HashTableLookup(function_mapped_why, &fun);

  if (!entry) {
    return NULL;
  }

  return (t_arraylist*) entry->data;
}


void ReadReasonsForMapping(const char* filename, t_hash_table* insmap) {
  t_address_hash_entry* entry;
  FILE* mapping = fopen(filename, "r");

  while(mapping && !feof(mapping)) {
    t_address ins_addr;
    int why;

    fscanf(mapping, "%x,%i\n", &ins_addr, &why);

    entry = (t_address_hash_entry*) HashTableLookup(insmap, &ins_addr);
    if (entry) {
      AddReasonForMapping((t_function*)(entry->data), why);
    }
  }
}

void ReadInput(t_cfg* cfg) {
  t_hash_table* insmap;
  t_ins* ins;
  t_function* fun;
  t_address_hash_entry* entry;

  /* Look at all instructions that have been matched and why. For those instruction's functions,
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


  if (diablodiversity_options.div_matched_instructions_and_why_file && strcmp(diablodiversity_options.div_matched_instructions_and_why_file, "") != 0) {
    ReadReasonsForMapping(diablodiversity_options.div_matched_instructions_and_why_file, insmap);
  }

  if (diablodiversity_options.div_iterative_allowed_transforms_file && strcmp(diablodiversity_options.div_iterative_allowed_transforms_file, "") != 0) {
    ReadScriptFile(diablodiversity_options.div_iterative_allowed_transforms_file);
  }

  HashTableFree(insmap);
}


void InitStrategyScripted(t_cfg* cfg) {
  ReadInput(cfg);

  if (!diablodiversity_options.div_iterative_allowed_transforms_output_file) {
    FATAL((":("));
  }

  ReadTransformationRules("transformation_rules");

  output_scripts = fopen(diablodiversity_options.div_iterative_allowed_transforms_output_file, "w");

  /* We add extra reasons to the callers of matched functions */
  AddCallerMatchingReasons(cfg, FALSE /* removeReasonFromCallee */);
}


/* {{{ Costs */
/* We have an input 'script' that tells us for each reason which actions we can take. These
   actions depend on the hotness of the code, and how many times we iterated already.

   When we match a function due to reason X, we:
   *) increase cost for reason X -> the number of transformations applied for preventing reason X increases
   *) look at which basic blocks can be transformed for this reason. This is done by some rules that are read from a file.
      These rules tell us:
      - after how many iterations for reason X we can start applying this rule (i.e. rules for Hot Code only after a while)
      - which percentage of the total weight this basic block can have at most for this transform to be applied to this BBL
      Format of such a file:
reason,percentage(as double),after number of iterations,transformation name
Thus, for example,
3,1.00,0,DiversityFlipBranches
3,0.0,0,DiversityFlattenFunction
3,0.50,5,DiversityFlattenFunction
Says that for preventing BD_REASON_EDGES_FLOWGRAPH we can, starting from iteration 0, always try flipping branches.
From iteration 0 we can also flatten frozen functions. But functions that have > 0 <= 50% of all instruction executions can
only be flattened after 5 unsuccesful iterations.
*/

typedef struct _t_transformation_rule t_transformation_rule;
struct _t_transformation_rule {
  double max_weight_pct;
  int after_transformation_nr;
  t_transformation_info* transformation;
  int rule_used_count;
};

/* This table maps a reason id to an (Maybe null) arraylist of transformation rules */
t_hash_table* ReasonToRules() {
  static t_hash_table* reason_to_rules = NULL;
  if (!reason_to_rules) {
    reason_to_rules = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);
  }
  return reason_to_rules;
}

void AddRule(int reason, t_transformation_rule* rule) {
  t_address_hash_entry* entry;
  t_arraylist* rules_list;

  entry = (t_address_hash_entry*) HashTableLookup(ReasonToRules(), &reason);

  if (!entry) {
    rules_list = ArrayListNew(sizeof(t_transformation_rule*), 10);

    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = reason;
    entry->data = rules_list;

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(ReasonToRules(), entry);
  } else {
    rules_list = (t_arraylist*) entry->data;
  }

  ArrayListAdd(rules_list, &rule);
}

t_arraylist* GetRulesForReason(int reason) {
  t_address_hash_entry* entry;
  t_arraylist* rules_list;

  entry = (t_address_hash_entry*) HashTableLookup(ReasonToRules(), &reason);

  if (!entry) {
    return NULL;
  }

  return (t_arraylist*) entry->data;
}

void ReadTransformationRules(const char* file) {
  FILE* rulesfile = fopen(file, "r");
  t_bool read_ok = TRUE;

  while (rulesfile && read_ok && !feof(rulesfile)) {
    int reason;
    char transformation_name[100];
    double max_pct;
    int after_nr;
    t_transformation_info* transformation;
    t_transformation_rule* rule;

    int nr_read = fscanf(rulesfile, "%i,%lf,%i,%s\n", &reason, &max_pct, &after_nr, transformation_name);
    if (nr_read != 4) {
      read_ok = FALSE;
      continue;
    }

    transformation = GetTransformByName(transformation_name);

    rule = (t_transformation_rule*) Malloc(sizeof(t_transformation_rule));
    rule->after_transformation_nr = after_nr;
    rule->max_weight_pct = max_pct;
    rule->transformation = transformation;
    AddRule(reason, rule);
  }

  fclose(rulesfile);
}

/* So what happens is the following. Due to the ruleset, we select a number of transformations. All of those 
   are tried with CanTransform. However, some costs might be too high yet. This function checks if, when we selected
   a transformation, if the current ruleset actually allows transforming this something (bbl of fun) yet at this point.
   Returns the *first* matching rule if an allowing rule is found, NULL otherwise. */
t_transformation_rule* CanTransformSomethingWithThisRuleSet(t_cfg* cfg, long long max_weight, long long weight, int iteration, t_transformation_info* transform, t_arraylist* ruleset) {
  int i;
  t_transformation_rule* rule;

  FOREACH_ARRAY_ITEM(ruleset, i, rule, t_transformation_rule*) {
    if (transform != rule->transformation)
      continue;

    /* We found a rule that applies to this transformation, check if this rule allows it (but if not, maybe another does) */
    if (weight > rule->max_weight_pct * max_weight) {
      VERBOSE(0, ("Skipping rule %i due to weight %lld > %lld", i, weight, rule->max_weight_pct * max_weight));
      continue;
    }

    if (iteration < rule->after_transformation_nr) {
      VERBOSE(0, ("Skipping rule %i due to iteration %i >= %i", i, iteration, rule->after_transformation_nr));
      continue;
    }

    if (rule->rule_used_count + rule->after_transformation_nr > iteration) {
      VERBOSE(0, ("Skipping rule %i due to rule_used_count %i + %i <= %i", i, rule->rule_used_count, rule->after_transformation_nr, iteration));
      continue;
    }

    /* Is allowed! */
    VERBOSE(0, ("Rule allowed: %i", i));
    return rule;
  }
  
  return NULL;
}

t_bool IsTransformInList(t_transformation_info* transform, t_arraylist* list) {
  int i;
  t_transformation_info* info;
  FOREACH_ARRAY_ITEM(list, i, info, t_transformation_info*) {
    if (info == transform)
      return TRUE;
  }
  return FALSE;
}

t_arraylist* TransformsInRules(t_arraylist* rules) {
  t_arraylist* list;
  t_transformation_rule* rule;
  int i;

  if (!rules)
    return NULL;

  list = ArrayListNew(sizeof(t_transformation_info*), 10);

  FOREACH_ARRAY_ITEM(rules, i, rule, t_transformation_rule*) {
    if (IsTransformInList(rule->transformation, list))
      continue;
    ArrayListAdd(list, &(rule->transformation));
  }

  return list;
}

long long GetBblWeight(t_bbl* bbl) {
  return BBL_EXEC_COUNT(bbl) * BBL_NINS(bbl);
}


long long GetFunctionWeight(t_function* fun) {
  long long w = 0;
  t_bbl* bbl;

  FUNCTION_FOREACH_BBL(fun, bbl) {
    w += GetBblWeight(bbl);
  }

  return w;
}

static long long max_weight = -1;
void SetMaxWeight(t_cfg* cfg) {
  t_function* fun;

  max_weight = 0;

  CFG_FOREACH_FUN(cfg, fun) {
    max_weight += GetFunctionWeight(fun);
  }
}

void ClearRulesUsedTimes(t_arraylist* rules) {

  int i;
  t_transformation_rule* rule;

  FOREACH_ARRAY_ITEM(rules, i, rule, t_transformation_rule*) {
    rule->rule_used_count = 0;
  }
}

int TransformFunctionWithRules(t_cfg* cfg, t_function* fun, t_arraylist* rules, int seed, int max_cost, int iteration) {
  int notapplied_count = 0;
  int i;
  int current_cost = 0;
  t_bool applied_transform = FALSE;
  t_transformation_cost_info* transform;
  t_arraylist* possible_transforms;
  t_arraylist* transformations = TransformsInRules(rules);

  if (!transformations)
    return 0;

  if (address_of_function(fun) == 0) {
    VERBOSE(0, ("The 'Diablo' address of function %p (%s) is 0, THIS SHOULD NOT HAPPEN!", fun, FUNCTION_NAME(fun)));
    return 0;
  }

  ClearRulesUsedTimes(rules);

	diablo_rand_seed(seed);

  // For inline functions, we want this to be run immediately on all functions?! TODO

  ExtraOutput("BEFORE", cfg, fun);
  
  possible_transforms = ListPossibleTransformations(cfg, fun, transformations, max_cost);
  VERBOSE(1, ("For function %s there are %i possible transformations!", FUNCTION_NAME(fun), possible_transforms->current_length));

  do {
    /*
       As long as we do not apply a transformation, we can just iterate further over the transforms.
       But once we apply a transformation, BBLs might be added/changed/removed => restart!
    */
    applied_transform = FALSE;

    while(possible_transforms->current_length > 0) {
      int chosen_transform = 0;
      t_transformation_cost_info* transform = NULL;
      t_bool doRulesAllow;
      long long weight;
      t_transformation_rule* which_rule = NULL;

      if (possible_transforms->current_length > 1) {
        chosen_transform = diablo_rand_next() % possible_transforms->current_length;
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

      if (transform->transformation->whole_function_transform) {
        weight = GetFunctionWeight(fun);
      } else {
        weight = GetBblWeight(transform->bbl);
      }
      which_rule = CanTransformSomethingWithThisRuleSet(cfg, max_weight, weight, iteration, transform->transformation, rules);

      doRulesAllow = which_rule == NULL ? FALSE : TRUE;


      //if (TrueWithChance(50)) { // TODO: also use user-configurable chances, or is cost enough?
      /* Since we pick a random transformation from the list of transforms, if we only want single diversity we can just always
         pick this choice */
      if (doRulesAllow) {
        const char* isFrozen;
        const char* isLW;    
        const char* isHot;

        if (transform->transformation->whole_function_transform) {
          isFrozen = FunIsFrozen(fun) ? "Frozen" : "";
          isLW = "";
          isHot = FunIsHot(fun) ? "Hot" : "";
        } else {
          isFrozen = BblIsFrozen(transform->bbl) ? "Frozen" : "";
          isLW = BblIsAlmostHot(transform->bbl) ? "Lukewarm" : "";
          isHot = BblIsHot(transform->bbl) ? "Hot" : "";
        }
        VERBOSE(0, ("Applying %s on a BBL of %s %s %s %s", transform->transformation->name, FUNCTION_NAME(fun), isFrozen, isLW, isHot));
        VERBOSE(4, ("Cost is now %i out of a maximum of %i", current_cost, max_cost));
        VERBOSE(0, ("Rules ALLOW applying %s to a bbl of %s (weight is %lld of %lld, execcnt is %lld)", transform->transformation->name, FUNCTION_NAME(fun), weight, max_weight, transform->bbl ? BBL_EXEC_COUNT(transform->bbl) : (t_int64)0));

        /* Update the number of times this rule was applied */
        which_rule->rule_used_count++;

        /* Since we re-create the entire list each time we increase the cost, we know in advance that adding this will not make cost > max_cost */
        applied_transform = TRUE;
        current_cost += transform->cost;
        transform->transformation->do_transform(cfg, fun, transform->bbl, transform->additional_info);

        break;
      } else {
        VERBOSE(0, ("Rules do not allow applying %s to a bbl of %s (weight is %lld of %lld)", transform->transformation->name, FUNCTION_NAME(fun), weight, max_weight));
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

  ExtraOutput("AFTER", cfg, fun);

  ArrayListFree(transformations);

  return notapplied_count;
}


void IncreaseCostForBy(t_script_choice* choice, t_transformation_info* transform, int by) {
  choice->costs[transform->idx] += by;
}

/* returns FALSE if this reason is unsupported */
t_bool IncreaseCosts(t_script_choice* choice) {
  choice->max_cost++; /* TODO */

  switch (choice->reason) {
    case BD_REASON_EDGES_HASH_MATCHING:
      IncreaseCostForBy(choice, transformationDiversityFlipBranches, 1);
      /* TODO: might also want to add instruction substitution for those without conditional jumps? */
      return TRUE;
      break;
    case BD_REASON_EDGES_FLOWGRAPH:
      IncreaseCostForBy(choice, transformationDiversityFlipBranches, 1);
      IncreaseCostForBy(choice, transformationDiversitySplitByTwoWayPredicate, 1);
      IncreaseCostForBy(choice, transformationDiversityFlattenFunction, 1);
      /* TODO: flip branches! */
      return TRUE;
      break;
    case BD_REASON_EDGES_PRIME_SIG:
      IncreaseCostForBy(choice, transformationDiversityFlipBranches, 1);
      return TRUE;
      break;
    case BD_REASON_EDGES_CALLGRAPH:
      /* TODO! Both have to be inserted before callers/callees only! */
      IncreaseCostForBy(choice, transformationDiversityThwartDisassembly, 1);
      IncreaseCostForBy(choice, transformationDiversitySplitByTwoWayPredicate, 1);
      return TRUE;
      break;
    case BD_REASON_CALL_SEQUENCE_EXACT:
    case BD_REASON_CALL_SEQUENCE_TOPOLOGY:
    case BD_REASON_CALL_SEQUENCE_SEQUENCE:
    case BD_REASON_CALL_REFERENCE:
      IncreaseCostForBy(choice, transformationDiversityBranchFunctionBeforeAndAfterCalls, 1);
      return TRUE;
      break;
    default:
      VERBOSE(0, ("Unknown reason to increase: %i", choice->reason));
  }
  return FALSE;
}

/* Some matching strategies, such as call sequence matching will match calls too. So we have to transform the call sites of such
   functions as well. */
t_bool HaveToTransformCallersInstead(int reason) {
  switch (reason) {
    case BD_REASON_CALL_SEQUENCE_EXACT:
    case BD_REASON_CALL_SEQUENCE_TOPOLOGY:
    case BD_REASON_CALL_SEQUENCE_SEQUENCE:
    case BD_REASON_CALL_REFERENCE:
      return TRUE;
  }
  return FALSE;
}

/* Look at all functions, their match reasons, and see if that function is matched due to the caller. If so,
   add this reason to the caller's list of matched reasons too. The extra boolean indicates whether or not
   to remove this reason from the current function */
typedef struct _t_function_reason_pair t_function_reason_pair;
struct _t_function_reason_pair {
  t_function* function;
  int reason;
};

void AddCallerMatchingReasons(t_cfg* cfg, t_bool removeReasonFromCallee) {
  t_function* fun;
  t_arraylist* reasons_to_add = ArrayListNew(sizeof(t_function_reason_pair), 10);
  t_function_reason_pair reason_pair;
  int i;

  VERBOSE(0, ("Adding reasons to callers when their callees were matched due to them"));
  /* We do not want to just add the reasons to the other function, otherwise when we iterate over _that_
     function, the reasons added there get transferred to other functions, etc */
#if 1
  CFG_FOREACH_FUN(cfg, fun) {
    t_arraylist* reasons_mapped = GetReasonsForBeingMatched(fun);
    int i;
    int reason;

    if(!reasons_mapped) {
      continue;
    }

    FOREACH_ARRAY_ITEM(reasons_mapped, i, reason, int) {
      if (HaveToTransformCallersInstead(reason)) {
        t_cfg_edge* callee_edge;
        t_bbl* head = FUNCTION_BBL_FIRST(fun);

        if (removeReasonFromCallee) {
          removeReasonForMappingInList(reasons_mapped, reason);
        }

        if (!head)
          continue;

        BBL_FOREACH_PRED_EDGE(head, callee_edge) {
          if (CFG_EDGE_CAT(callee_edge) == ET_CALL) {
            t_function* callee = BBL_FUNCTION(CFG_EDGE_HEAD(callee_edge));

            if (callee && !FUNCTION_IS_HELL(callee)) {
              reason_pair.function = callee;
              reason_pair.reason = reason;

              ArrayListAdd(reasons_to_add, &reason_pair);
            }
          }
        }
      }
    }
  }

  /* Now actually add all the reasons */
  FOREACH_ARRAY_ITEM(reasons_to_add, i, reason_pair, t_function_reason_pair) {
    AddReasonForMapping(reason_pair.function, reason_pair.reason);
  }
  ArrayListFree(reasons_to_add);

  VERBOSE(0, ("... Done"));
#endif
}

t_script_choice* InitCosts(int reason) {
  int i;
  t_script_choice* choice = (t_script_choice*) Malloc(sizeof(t_script_choice));
  choice->costs = (int*) Malloc(current_transform_max_idx * sizeof(int));
  for (i = 0; i < current_transform_max_idx; i++)
    choice->costs[i] = 0;
  choice->max_cost = 0;
  choice->reason = reason;
  choice->seed = diablo_rand_next();

  // TODO: seperate costs?
  if (! IncreaseCosts(choice) ) {
    /* This reason was not supported */
    FreeChoice(choice);
    return NULL;
  }

  return choice;
}
/* }}} */

void TransformWithChoice(t_function* fun, t_script_choice* choice) {
  t_arraylist* transformations;
  t_arraylist* rules;
  int i;
  int transforms_to_apply = 0;
  int lowest_iteration = 1000; // TODO
  t_transformation_rule* rule;

  /* Output this choice to our log file */
  WriteScriptChoiceToFile(output_scripts, fun, choice);

  /* Make the list of transformations that we can apply with this seed and cost */
  /* TODO: individual costs! */
  transformations = ArrayListNew(sizeof(t_transformation_info*), 10);
  for (i = 0; i < current_transform_max_idx; i++) {
    if (choice->costs[i] > 0) {
      ArrayListAdd(transformations, &supported_transforms[i]);
    }
  }

  /* TransformFunction(FUNCTION_CFG(fun), fun, transformations, choice->seed, choice->max_cost); */
  rules = GetRulesForReason(choice->reason);

#ifdef SIMPLE_TRANSFORMS_TO_APPLY
  /* Ok, choice->max_cost is the number of iterations we've seen this reason. We start adding transforms
     from the iteration that at least one rule applies to this iteration number */
  FOREACH_ARRAY_ITEM(rules, i, rule, t_transformation_rule*) {
    if (rule->after_transformation_nr < lowest_iteration)
      lowest_iteration = rule->after_transformation_nr;
  }
  transforms_to_apply = choice->max_cost - lowest_iteration; /* ie lowest = 1 -> will apply 1 transform after 1 iteration */
#else
  transforms_to_apply = choice->max_cost;
#endif

  TransformFunctionWithRules(FUNCTION_CFG(fun), fun, rules, choice->seed, choice->max_cost, transforms_to_apply /* TODO for now this is the amount of iterations for the current reason */);
}

void ScriptedTransformFunction(t_function* fun, t_arraylist* script) {
  t_arraylist* reasons_mapped;
  int i, reason;
  t_script_choice* choice;

  /* When we have to re-transform because after applying a technique this technique still applied to the function, we have two options:
     - increase costs and chose new random seed, and re-transform the whole function (possibly interfering with later transformations)
     - transform the same way as the previous iteration, but _afterwards_ apply more transformations against this technique
       (possibly we might need to have a higher allowed cost there, instead of the 'initial cost' so that higher costs get a chance... TODO) */
  t_bool if_still_mapped_retransform_from_scratch = diablodiversity_options.div_if_still_mapped_retransform_from_scratch;

  VERBOSE(0, ("Transforming '%s'", FUNCTION_NAME(fun)));

  reasons_mapped = GetReasonsForBeingMatched(fun);

  /* We read the script. Each script part has a reason it's there (to get rid of sth in BinDiff).
     - If this was succesful (this function no longer mapped with that reason) -> repeat the same way.
     - Otherwise -> increase costs
     In both cases, we treat this reason as 'solved' for now after that part. In the end, there might be some reasons we didn't handle yet, initialize response for those.
  */
  FOREACH_ARRAY_ITEM(script, i, choice, t_script_choice*) {
    /* It's possible that we're transforming a function that has no mapping in BinDiff anymore, so be careful when accessing reasons_mapped */
    if (reasons_mapped) {
      if (isReasonForMappingInList(reasons_mapped, choice->reason)) {
        VERBOSE(0, ("In a previous iteration, we tried preventing BinDiff reason %i, but this didn't work. Increasing costs", choice->reason));
        if (if_still_mapped_retransform_from_scratch) {
          IncreaseCosts(choice);
          /* We also pick a new random seed */
          choice->seed = (choice->seed) * (choice->seed + 1);

          /* Since we tried improving the costs, we can safely remove this reason from the list to take action against later on*/
          removeReasonForMappingInList(reasons_mapped, choice->reason);
        } else {
          /* In this case, we transform as in the iteration before, but since we do NOT remove this reason from reasons_mapped,
             we will again apply a transformation against this technique afterwards */
        }
      } else {
        VERBOSE(0, ("In a previous iteration, we succesfully prevented BinDiff against reason %i!", choice->reason));
        // Keep choice as is!
        removeReasonForMappingInList(reasons_mapped, choice->reason);
      }
    } else {
      /* This function isn't matched at all here! Very good! */
      VERBOSE(0, ("In a previous iteration, we succesfully prevented BinDiff against reason %i!", choice->reason));
      // Keep choice as is!
    }

    TransformWithChoice(fun, choice);
  }

  /* Now there are still some reasons that BinDiff matched... */
  if (reasons_mapped) {
    FOREACH_ARRAY_ITEM(reasons_mapped, i, reason, int) {
      int j;
      if (reason < 0) {
        /* This reason was removed, skip it */
        continue;
      }
      VERBOSE(0, ("We didn't yet protect against %i, trying so...", reason));

      choice = InitCosts(reason);

      /* Only do the transformation if this reason was supported */
      if (choice) {
        TransformWithChoice(fun, choice);
      }
    }
  }
}

void StrategyScriptedDoTransforms(FILE* iterative_file, t_arraylist* previous_information, t_arraylist* transformations, t_cfg* cfg, t_hash_table* functions_table) {
  t_function* fun;

  SetMaxWeight(cfg);

  CFG_FOREACH_FUN(cfg, fun) {
    t_arraylist* script_fun = GetListForFunction(fun);

    if (FUNCTION_NAME(fun) && (strcmp("BranchFunction", FUNCTION_NAME(fun)) == 0) && (strcmp("BranchFunctionWithReturn", FUNCTION_NAME(fun)) == 0)) {
      continue;
    }

    if (FUNCTION_IS_HELL(fun)) {
      continue;
    }

    if (!script_fun) {
      VERBOSE(0, ("No existing script found for function %s, making a new one", FUNCTION_NAME(fun)));
      script_fun = ArrayListNew(sizeof(t_address_hash_entry*), 10);
    }

    VERBOSE(0, ("Max weight is %lld", max_weight));

    ScriptedTransformFunction(fun, script_fun);
  }
}

void FiniStrategyScripted() {
  fclose(output_scripts);
}
