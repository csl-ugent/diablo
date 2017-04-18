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
#include <diabloanopti386.h>
#include <diablodiversity.h>
#include "diablodiversity_cmdline.h"
}

void ExtraOutput(const char* prefix, t_cfg* cfg, t_function* fun); // Comes later on below
void AllocFunctionObjects(t_cfg *cfg);
const char* GetFullFunctionName(t_function* fun);

/* {{{ Global references to the (singleton) transformations */
t_transformation_info* transformationDiversityFlipBranches = NULL;
t_transformation_info* transformationDiversityOpaquePredicates = NULL;
t_transformation_info* transformationDiversityFlattenFunction = NULL;
t_transformation_info* transformationDiversityInlineFunction = NULL;
t_transformation_info* transformationDiversityThwartDisassembly = NULL;
t_transformation_info* transformationDiversitySplitByTwoWayPredicate = NULL;
t_transformation_info* transformationDiversityBranchFunctionBeforeAndAfterCalls = NULL;
t_transformation_info* transformationDiversityBranchFunctionBeforeAndAfterCallsGlobalVar = NULL;
t_transformation_info* transformationDiversityBranchFunctionInFirstBlock = NULL;
t_transformation_info* transformationDiversityUnfold = NULL;

int current_transform_max_idx = 0;
t_transformation_info* supported_transforms[MAX_TRANSFORMS];
/* }}} */

/* {{{ Array Lists */

t_arraylist* ArrayListNew(int element_size, int initial_length) {
	t_arraylist* arr = (t_arraylist*) Malloc(sizeof(t_arraylist));

	arr->max_length     = initial_length;
	arr->current_length = 0;
	arr->element_size   = element_size;
	arr->data           = (char*) Malloc(element_size * initial_length);

	return arr;
}

void ArrayListFree(t_arraylist* arr) {
	Free(arr->data);
	Free(arr);
}

void ArrayListDeepFree(t_arraylist* arr) {
  void** item;
  int i;
  FOREACH_ARRAY_ITEM(arr, i, item, void**)
    Free(*item);
  ArrayListFree(arr);
}

void ArrayListAdd(t_arraylist* arr, void* element) {
	if (arr->current_length >= arr->max_length) {
		arr->max_length = 2 * arr->max_length;
		arr->data = (char*) Realloc(arr->data, arr->element_size * arr->max_length);
	}
	memcpy(arr->data + arr->current_length * arr->element_size, element, arr->element_size);
	arr->current_length++;
}

void ArrayListRemove(t_arraylist* arr, int element) {
  if (element + 1 < arr->current_length) {
    memmove(arr->data + element * arr->element_size, arr->data + (element + 1) * arr->element_size, arr->element_size * (arr->current_length - element - 1));
  }
  arr->current_length--;
}

void ArrayListSort(t_arraylist* arr, int(*cmp)(const void *, const void *)) {
	diablo_stable_sort(arr->data, arr->current_length, arr->element_size, cmp);
}

/* }}} */


/* {{{ Generic transformation stuff */

static int order_transformation_cost(const void *a, const void *b)
{
	const t_transformation_cost_info *A = *(const t_transformation_cost_info **)a;
  const t_transformation_cost_info *B = *(const t_transformation_cost_info **)b;

	if (A->cost < B->cost)
		return -1;
	if (A->cost > B->cost)
		return 1;
	return 0;
}
/* }}} */

/* {{{ Listing transformations */

t_arraylist* SimpleCanTransform(t_bbl* bbl, void** additional_info, int cost) {
  t_arraylist* list = ArrayListNew(sizeof(t_transformation_cost_info*), 2);
  t_transformation_cost_info* cost_data = (t_transformation_cost_info*) Malloc(sizeof(t_transformation_cost_info));

  cost_data->bbl = bbl;
  cost_data->transformation = NULL; /* Must be filled out later */
  cost_data->cost = cost;
  cost_data->additional_info = additional_info;

  ArrayListAdd(list, &cost_data);

  return list;
}

/* Add a transformation to the list. This is generic: it can either be a function-based transform, or a bbl-based transform */
void PotentiallyAddTransformationToList(
      t_arraylist* list,
      t_cfg* cfg,
      t_function* fun,
      t_bbl* bbl,
      int max_cost,
      t_transformation_info* info,
      t_bool (*find_transform)(t_bbl* bbl, t_function* fun, t_transformation_info* info))
{
  void* additional_info = NULL; /* if can_transform is TRUE, it's possible that it pre-computes some data, it can store that here */
  t_arraylist* found_transforms;

  if ( (found_transforms = info->can_transform(cfg, fun, bbl, &additional_info)) ) {
    int f_i;
    t_transformation_cost_info* found_transform;
    t_bool can_add = FALSE;

    if (!find_transform(bbl, fun, info))
      can_add = TRUE;
    //VERBOSE(0, ("%i", found_transforms->current_length));
    FOREACH_ARRAY_ITEM(found_transforms, f_i, found_transform, t_transformation_cost_info*) {
      int j;
      t_transformation_info* existing_transform;
      t_bool transform_found = FALSE;

      int cost = found_transform->cost;

      if (cost > max_cost) {
        if (additional_info) {
          Free(additional_info);
        }
        Free(found_transform);
        continue;
      }

      if (can_add) {
        /* We have to add information that the can_add function doesn't have, before we can add the transform to the list */
        found_transform->transformation = info;
        ArrayListAdd(list, &found_transform);
      } else {
        Free(found_transform);
      }
    }
  } else {
    if (additional_info) {
      Free(additional_info);
    }
  }
}

/* (TODO for BOTH (maybe factor them?): 1. make configurable, 2. optimize x2: break earlier; sort list!) */
t_bool find_transform_bbl(t_bbl* bbl, t_function* fun, t_transformation_info* info) {
  int j;
  t_transformation_info* existing_transform;
  /* If this transformation has already been applied, don't do it again! */
  FOREACH_ARRAY_ITEM(BBL_TRANSFORMATIONS(bbl), j, existing_transform, t_transformation_info*) {
    // TODO: pointer comparisons should be ok, but be sure to comment somewhere that we actually should keep this in mind everywhere!

    VERBOSE(1, ("Transformation %p != %p (%s != %s) was already applied to BB %p", info, existing_transform, info->name, existing_transform->name, bbl));

    if (info == existing_transform) {
      VERBOSE(1, ("The transformation %s was already applied to a BB in %s, skipping", info->name, FUNCTION_NAME(fun)));
      return TRUE;
    }
  }
  return FALSE;
}

t_bool find_transform_fun_generic(t_function* fun, t_transformation_info* info, t_arraylist* list) {
  int j;
  t_transformation_info* existing_transform;
  /* If this transformation has already been applied, don't do it again!
  (TODO: 1. make configurable, 2. optimize x2: break earlier; sort list!) */
  FOREACH_ARRAY_ITEM(list, j, existing_transform, t_transformation_info*) {
    // TODO: pointer comparisons should be ok, but be sure to comment somewhere that we actually should keep this in mind everywhere!

    VERBOSE(1, ("Transformation %p != %p (%s != %s) was already applied to function %p", info, existing_transform, info->name, existing_transform->name, fun));

    if (info == existing_transform) {
      VERBOSE(1, ("The transformation %s was already applied to the function in %s, skipping", info->name, FUNCTION_NAME(fun)));
      return TRUE;
    }
  }
  return FALSE;
}

t_bool find_transform_fun(t_bbl* bbl, t_function* fun, t_transformation_info* info) {
  return find_transform_fun_generic(fun, info, FUNCTION_TRANSFORMATIONS(fun));
}

t_arraylist* ListPossibleTransformations(t_cfg* cfg, t_function* fun, t_arraylist* transformations, int max_cost) {
	t_bbl* bbl;
  int i;
  t_transformation_info* info;
  t_arraylist* list = ArrayListNew(sizeof(t_transformation_cost_info*), 16);

  
  FOREACH_ARRAY_ITEM(transformations, i, info, t_transformation_info*) {
    if (info->whole_function_transform) {
      /* Whole-function transformations, we should have only one time this transformation for the function TODO actually we might want to increase the chance here! */
      PotentiallyAddTransformationToList(list, cfg, fun, bbl, max_cost, info, find_transform_fun);
    } else {
      /* Per-basic block transformations, iterate over all BBLs of the function */
	    FUNCTION_FOREACH_BBL(fun, bbl) {
        if (BBL_ATTRIB(bbl) & BBL_IS_DATA) /* Some transforms insert 'rubbish' basic blocks as data, don't transform those */
          continue;
        PotentiallyAddTransformationToList(list, cfg, fun, bbl, max_cost, info, find_transform_bbl);
      }
    }
	}

  ArrayListSort(list, order_transformation_cost);

	return list;
}


void FreeTransformations(t_arraylist* transformations) {
  t_transformation_cost_info* info;
  int i;
  FOREACH_ARRAY_ITEM(transformations, i, info, t_transformation_cost_info*) {
    if (info->additional_info)
      Free(info->additional_info);
    Free(info);
  }
  ArrayListFree(transformations);
}

/* }}} */

/* {{{ Dynamic member stuff to keep track of transformations per BBL */
/* Bbl Transformations */
void 
BblTransformationsInit(t_bbl * bbl, t_arraylist** transformations)
{
  *transformations = ArrayListNew(sizeof(t_transformation_info*), 10);
}

void 
BblTransformationsFini(t_bbl * bbl, t_arraylist** transformations)
{
  ArrayListFree(*transformations);
}

void 
BblTransformationsDup(t_bbl * bbl, t_arraylist** transformations)
{
  t_bbl* orig_bbl = (t_bbl*) global_hack_dup_orig;
  t_transformation_info* transform;
  int i;

  // TODO: make seperate dup function
  *transformations = ArrayListNew(sizeof(t_transformation_info*), BBL_TRANSFORMATIONS(orig_bbl)->max_length);

  FOREACH_ARRAY_ITEM(BBL_TRANSFORMATIONS(orig_bbl), i, transform, t_transformation_info*)
    ArrayListAdd(*transformations, &transform);
}

t_dynamic_member_info bbl_transformations_array = null_info;

/* Function Transformations */
void 
FunctionTransformationsInit(t_function* fun, t_arraylist** transformations)
{
  *transformations = ArrayListNew(sizeof(t_transformation_info*), 10);
}

void 
FunctionTransformationsFini(t_function* fun, t_arraylist** transformations)
{
  ArrayListFree(*transformations);
}

void 
FunctionTransformationsDup(t_function * fun, t_arraylist** transformations)
{
  t_function* orig_fun = (t_function*) global_hack_dup_orig;
  t_transformation_info* transform;
  int i;

  // TODO: make seperate dup function
  *transformations = ArrayListNew(sizeof(t_transformation_info*), FUNCTION_TRANSFORMATIONS(orig_fun)->max_length);

  FOREACH_ARRAY_ITEM(FUNCTION_TRANSFORMATIONS(orig_fun), i, transform, t_transformation_info*)
    ArrayListAdd(*transformations, &transform);
}


t_dynamic_member_info fun_transformations_array = null_info;


/* Function Original Addresses */
void 
InsOriginalAddressInit(t_ins* ins, t_address* orig_address)
{
  if (INS_BBL(ins) && BBL_FUNCTION(INS_BBL(ins)))
	*orig_address = address_of_function(BBL_FUNCTION(INS_BBL(ins)));
  else
	*orig_address = 0;
}

void 
InsOriginalAddressFini(t_ins* fun, t_address* orig_address)
{
  ;
}

void 
InsOriginalAddressDup(t_ins* fun, t_address* orig_address)
{
  t_ins* orig_ins = (t_ins*) global_hack_dup_orig;
  *orig_address = INS_ORIGINAL_ADDRESS(orig_ins);
}


t_dynamic_member_info ins_original_address_array = null_info;


/* Instruction Execution Count */
void 
InsExecCountInit(t_ins* ins, t_int64* cnt)
{
  if (INS_BBL(ins))
	*cnt = BBL_EXEC_COUNT(INS_BBL(ins));
  else
	*cnt = 0;
}

void 
InsExecCountFini(t_ins* fun, t_int64* cnt)
{
  ;
}

void 
InsExecCountDup(t_ins* fun, t_int64* cnt)
{
  t_ins* orig_ins = (t_ins*) global_hack_dup_orig;
  *cnt = INS_EXECCOUNT(orig_ins);
}


t_dynamic_member_info ins_exec_count_array = null_info;


/* }}} */


/* {{{ Possible transformations*/
void 
FunctionPossibleTransformationsInit(t_function* fun, t_arraylist** transformations)
{
  *transformations = ArrayListNew(sizeof(t_transformation_info*), 10);
}

void 
FunctionPossibleTransformationsFini(t_function* fun, t_arraylist** transformations)
{
  ArrayListFree(*transformations);
}

void 
FunctionPossibleTransformationsDup(t_function * fun, t_arraylist** transformations)
{
  t_function* orig_fun = (t_function*) global_hack_dup_orig;
  t_transformation_info* transform;
  int i;

  // TODO: make seperate dup function
  *transformations = ArrayListNew(sizeof(t_transformation_info*), FUNCTION_TRANSFORMATIONS(orig_fun)->max_length);

  FOREACH_ARRAY_ITEM(FUNCTION_TRANSFORMATIONS(orig_fun), i, transform, t_transformation_info*)
    ArrayListAdd(*transformations, &transform);
}


t_dynamic_member_info fun_possible_transformations_array = null_info;

/* }}} */


/* {{{ The main driver code */

/*
  TODO: ideally, this is sorted already: first the ones we have to transform the same as before, then the ones we don't have to!
*/

int order_funinfo_retransform(const void *a, const void *b)
{
  const t_diversity_function_info* A = *(const t_diversity_function_info **)a;
  const t_diversity_function_info* B = *(const t_diversity_function_info **)b;

	if (A->should_redo < B->should_redo)
		return -1;
  if (A->should_redo > B->should_redo)
		return 1;
	return 0;
}


t_uint32 FunHashAddress(const void* key, const t_hash_table * table)
{
  return (*((t_address *) key)) % HASH_TABLE_TSIZE(table);
}

int FunAddressCmp(const void* key, const void* key2)
{
  return *(t_address*)key - *(t_address*)key2;
}

void FunAddrKeyFree(const void* he, void * data)
{
  /* the data in the hash key is actually a pointer to the hash element, so don't free that too :) */
  Free(he);
}

t_transformation_info* GetTransformByName(const char* name) {
  int i;

  for (i = 0; i < current_transform_max_idx; i++) {
    if (strcmp(supported_transforms[i]->name, name) == 0) {
      return supported_transforms[i];
    }
  }

  return NULL;
}

void AddPossibleTransform(t_function* fun, t_transformation_info* transform) {
  t_arraylist* list = FUNCTION_POSSIBLE_TRANSFORMATIONS(fun);
  VERBOSE(0, ("Adding transformation '%s' to function '%s's list of possible transformations", transform->name, FUNCTION_NAME(fun)));
  if (!find_transform_fun_generic(fun, transform, list)) {
    ArrayListAdd(list, &transform);
  }
}

static t_hash_table* mapped_functions = NULL;

void SetFunctionAsMapped(t_function* fun) {
  t_address_hash_entry* entry;
  uintptr_t fun_addr = (uintptr_t) fun;

  if (!mapped_functions) {
      mapped_functions = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);
  }
  
  entry = (t_address_hash_entry*) HashTableLookup(mapped_functions, (void*) &fun_addr);
  
  if (!entry) {
    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = fun_addr; // BBL_FUNCTION(INS_BBL(ins));
    entry->data = NULL; /* Used as a set */

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(mapped_functions,entry);
  }
}

t_bool functionIsMapped(t_function* fun) {
  t_address_hash_entry* entry;
  uintptr_t fun_addr;

  /* If this iteration is the first iteration, the mapped_functions map has not yet been loaded -> it has to be transformed always! */
  if (!mapped_functions) {
    return TRUE;
  }

  fun_addr = (uintptr_t) fun; //address_of_function(fun);

  if (!fun_addr) {
    return TRUE;
  }

  entry = (t_address_hash_entry*) HashTableLookup(mapped_functions, &fun_addr);
  if (entry) {
      return TRUE;
  }

  return FALSE;
}



static t_hash_table* functions_to_caddr_map;

uintptr_t address_of_function(t_function* fun) {
  uintptr_t addr = (uintptr_t) fun;
  t_address_hash_entry* entry;
  if (!functions_to_caddr_map)
    FATAL(("Expected functions_to_caddr_map to be initialized"));

  entry = (t_address_hash_entry*) HashTableLookup(functions_to_caddr_map, (void*) &addr);
  if (!entry) {
    VERBOSE(0, ("Function address lookup returned 0 for %s", FUNCTION_NAME(fun)));
    return 0;
  }
  return (uintptr_t) entry->data;  //(t_address) entry->data; // TODO TODO TODO bad cast?
}

/* }}} */



static FILE* extra_output_file = NULL;
static t_transformation_info* extra_transform = NULL;

void ExtraOutput(const char* prefix, t_cfg* cfg, t_function* fun) {
  int bbls = 0;
  int insts = 0;
  int couldbe_applied = 0;
  t_bbl* bbl;

  if (FUNCTION_IS_HELL(fun))
    return;

  FUNCTION_FOREACH_BBL(fun, bbl) {
    t_ins* ins;
    bbls++;
    BBL_FOREACH_INS(bbl, ins) {
      insts++;
    }

    if (extra_transform && extra_transform->can_transform(cfg, fun, bbl, NULL)) {
      couldbe_applied++;
    }

  }

  fprintf(extra_output_file, "%s,%" PRIuPTR ",%i,%i,%i,%i,%s\n", prefix, address_of_function(fun), bbls, insts, insts/bbls, couldbe_applied, FUNCTION_NAME(fun));
}

/* {{{ Initialize the list of supported transformations */
t_arraylist* InitializeTransformationsList(t_cfg* cfg) {
  t_arraylist* transformations;
  transformations = ArrayListNew(sizeof(t_transformation_info*), 10);

  /* Set the initial list of all possible transformations */
#define ADD_TRANSFORMATION(basename, wholefunction) \
  { \
    t_transformation_info* info = (t_transformation_info*)Malloc(sizeof(t_transformation_info)); \
    info->name = "" #basename; \
    info->can_transform = basename##CanTransform; \
    info->do_transform = basename##DoTransform; \
    info->whole_function_transform = wholefunction; \
    info->idx = current_transform_max_idx; \
    ArrayListAdd(transformations, &info); \
    transformation##basename = info; \
    supported_transforms[current_transform_max_idx] = info; \
    current_transform_max_idx++; \
  }

  if(diablodiversity_options.div_backend_branch_flipping)
    ADD_TRANSFORMATION(DiversityFlipBranches, FALSE);

  if (diablodiversity_options.div_obfuscation_opaque_predicates) {
    ADD_TRANSFORMATION(DiversityOpaquePredicates, FALSE);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);
  }

  if (diablodiversity_options.div_obfuscation_flatten) {
    ADD_TRANSFORMATION(DiversityFlattenFunction, TRUE);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);
  }

  if (diablodiversity_options.div_inline_functions) {
    ADD_TRANSFORMATION(DiversityInlineFunction, FALSE);
    InitInlineTransformations(cfg);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);
  }

  if (diablodiversity_options.div_obfuscation_static_disassembly_thwarting) {
    ADD_TRANSFORMATION(DiversityThwartDisassembly, FALSE);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);

    /* Calls! */
    ADD_TRANSFORMATION(DiversityBranchFunctionBeforeAndAfterCalls, FALSE);
	ADD_TRANSFORMATION(DiversityBranchFunctionBeforeAndAfterCallsGlobalVar, FALSE);
	ADD_TRANSFORMATION(DiversityBranchFunctionInFirstBlock, FALSE);
  }

  if (diablodiversity_options.div_inline_two_way_predicates) {
    ADD_TRANSFORMATION(DiversitySplitByTwoWayPredicate, FALSE);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);
  }

  if(diablodiversity_options.div_inline_bbls) {
    ADD_TRANSFORMATION(DiversityUnfold, FALSE);
    /* TODO: debug/statistics code */
    extra_transform = ARRAYLIST_GET(transformations, transformations->current_length - 1, t_transformation_info*);
  }

  return transformations;
}
/* }}} */

/* {{{ Make a table that converts instructions into their original function addresses */
t_hash_table*  instructionsToOriginalFunctions(t_cfg* cfg) {
  t_function* fun;
  t_hash_table* instructions_to_originalfunctions_table = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  CFG_FOREACH_FUN(cfg, fun) {
    t_bbl* bbl;
    void* data;
    
    if (!(FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun)))
      continue;

    data = (void*)address_of_function(fun);

    FUNCTION_FOREACH_BBL(fun, bbl) {
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        t_address_hash_entry* entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
        t_address addr = INS_CADDRESS(ins);

        entry->address = addr;
        entry->data = data;

	      HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
        HashTableInsert(instructions_to_originalfunctions_table, entry);
      }
    }
  }

  return instructions_to_originalfunctions_table;
}

void OutputInstructionsToOriginalFunctions(t_cfg* cfg) {
  t_hash_table* instructions_to_originalfunctions_table;
  FILE* mapping_file;
  t_function* fun;

  /* Start by applying the transformations from
     a previous iteration that we need to apply again (because those functions are already no longer matching) */
  VERBOSE(0, ("Re-applying some previously applied transformations"));

  /* Before applying any transformations, we make a mapping: original instruction address -> original function address */
  // TODO: is this actually needed, this table?
  instructions_to_originalfunctions_table = instructionsToOriginalFunctions(cfg);

  /* Done transforming, output a mapping of (transformed) instruction to original function address */
  mapping_file = fopen("instructions_to_function.mapping", "w");
  CFG_FOREACH_FUN(cfg, fun) {
    t_bbl* bbl;
    t_address_hash_entry* entry;
    void* data;

    if (!(FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun)))
      continue;

    data = (void*)address_of_function(fun);

    FUNCTION_FOREACH_BBL(fun, bbl) {
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        t_address addr = INS_CADDRESS(ins);

        if (! addr) {
          // This instruction was generated by Diablo, and has no 'original' function. TODO: make this still linked to the function of the CFG it is in?
          continue;
        }
        entry = (t_address_hash_entry*) HashTableLookup(instructions_to_originalfunctions_table, &addr);

        if (!entry) {
          FATAL(("Entry was supposed to be not NULL!"));
        }

        fprintf(mapping_file, "%x %p\n", addr, entry->data);
      }
    }
  }

  fclose(mapping_file);
  HashTableFree(instructions_to_originalfunctions_table);
}
/* }}} */

t_int32 DiversityEngineIterative(t_cfg * cfg)
{
  t_diversity_options ret;
  t_diversity_choice choice;
  t_function* fun;
  t_arraylist* transformations;
  FILE* iterative_file;
  t_arraylist* previous_information;
  t_diversity_function_info* fun_info;
  t_hash_table* functions_table;
  int i;
  t_ins* ins;

  VERBOSE(0, ("Iterative Diversity, with costs: "));
  VERBOSE(0, ("  initial_max_cost: %i", diablodiversity_options.div_initial_max_cost));
  VERBOSE(0, ("  restart_divider: %i", diablodiversity_options.div_restart_divider));
  VERBOSE(0, ("  max_with_cost: %i", diablodiversity_options.div_max_with_cost));
  VERBOSE(0, ("  cost_increase: %i", diablodiversity_options.div_cost_increase));
  VERBOSE(0, ("  random_seed: %i", diablodiversity_options.div_random_seed));

  DiabloBrokerCallInstall("RegisterBblFactoring", "t_bbl *, t_bbl *", (void*) RegisterBblFactoring, FALSE);
  DiabloBrokerCallInstall("RegisterFunFactoring", "t_function *", (void*) RegisterFunFactoring, FALSE);
  DiabloBrokerCallInstall("EpilogueFactorBefore", "t_bbl *, t_bbl *", (void*) RegisterBblFactoring, FALSE);

  AllocFunctionObjects(cfg);

  diablo_rand_seed(diablodiversity_options.div_random_seed);


  // TODO!
  if (ConstantPropagationInit(cfg))
    ConstantPropagation(cfg, CONTEXT_SENSITIVE);
  else
    FATAL(("Error in constant propagation\n"));

  /* For inlining functions, we need all functions to have had constant propagation? (TODO!!!) */
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(fun))) {
      FunctionUnmarkAllBbls(fun);
      FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);
    }
  }

  transformations = InitializeTransformationsList(cfg);

  if (!diablodiversity_options.div_iterative_output) {
    FATAL(("Iterative diversity needs an output file for the iterations"));
  }

  if(!diablodiversity_options.div_iterative_input || strcmp(diablodiversity_options.div_iterative_input, "") == 0) {
    previous_information = ArrayListNew(sizeof(t_diversity_function_info*), 10);
  } else {
    FILE* input = fopen(diablodiversity_options.div_iterative_input, "r");
    if (!input) {
      FATAL(("Could not open iterative input file! (%s)", diablodiversity_options.div_iterative_input));
    }
    previous_information = DiversityEngineReadIterativeFile(input);
    fclose(input);
  }


  extra_output_file = fopen("extra_iterative_info_file", "w");
  fprintf(extra_output_file, "# prefix, fun_address, bbls, insts, insts_per_bbl, transform_couldbe_applied, fun_name\n");

  iterative_file = fopen(diablodiversity_options.div_iterative_output, "w");

  VERBOSE(1, ("Adding function entries to the hash table"));

  /* We initialize 2 hashmaps here: one that simply is a list of functions that we still have to process, the other
     is to map diablo function to original function address. */

  /* We make a hash set of all functions. The functions in this set have not yet been transformed */
  functions_table = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  /* The functions-to-original-address map */
  functions_to_caddr_map = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);
  CFG_FOREACH_FUN(cfg, fun) {
    t_address_hash_entry* entry;
    t_address addr;

    if (!(FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun))) {
      VERBOSE(1, ("In hashmap, not adding %s", FUNCTION_NAME(fun)));
      continue;
    }
    
    addr = INS_CADDRESS(BBL_INS_FIRST(FUNCTION_BBL_FIRST(fun)));

    VERBOSE(1, ("Adding function %s (%p) to hashmap", FUNCTION_NAME(fun), addr));

    /* To the list of functions to process */
    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = (uintptr_t) addr;
    entry->data = fun;

	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address); /* Correct? TODO */
    HashTableInsert(functions_table,entry);

    /* Mapping between fun_t* and orig_address */
    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = (uintptr_t) fun;
    entry->data = (void*) (t_int64) addr;

    HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address); /* Correct? TODO */
    HashTableInsert(functions_to_caddr_map,entry);
  }

  /* {{{ TRANSFORMING THE FUNCTIONS */

  /* Single seed per function */
  /* Initialize the used strategy */
#if 0 /* Does not work anymore */
#if SINGLESEED
  InitStrategySingleSeed(cfg);
#if 0
  OutputInstructionsToOriginalFunctions(cfg);
#endif
  StrategySingleSeedDoTransforms(iterative_file, previous_information, transformations, cfg, functions_table);
  FinishStrategySingleSeed(cfg);
#endif
#endif

#if 1
  InitStrategyScripted(cfg);
  StrategyScriptedDoTransforms(iterative_file, previous_information, transformations, cfg, functions_table);
  FiniStrategyScripted();
#endif

  fclose(iterative_file);
  fclose(extra_output_file);


  if (diablodiversity_options.div_inline_functions) {
    FiniInlineTransformations(cfg);
  }

  //ArrayListDeepFree(transformations);
  HashTableFree(functions_table);

  if (mapped_functions) {
    HashTableFree(mapped_functions);
  }
  
  if (FALSE /* TODO: make instruction selection optional */) {
	t_diversity_choice choice;
	t_diversity_options ret;

	choice.choice = 0;
	do{
	  int random_number;
	  ret = DiversityInstructionSelection(cfg, &choice, 0);
	  //VERBOSE(0,("WATCHME: %d",ret.range));
	  if(TrueWithChance(/*diablodiversity_options.div_backend_instruction_selection_chance*/100))
		random_number = (int)((ret.range+1)*(double)rand()/((double)RAND_MAX+(double)1));
	  else 
		random_number = -1;
	  choice.choice = random_number;
	} while(!ret.done);
  }

  VERBOSE(0, ("Done transforming iteratively"));

  /* Automaticall initializes the values correctly */
  InsInitOriginalAddress(cfg);
  InsInitExecCount(cfg);
  //scanf("%s", NULL);

  {
    FILE* functionsdump = fopen("functionsdump", "w");
    CFG_FOREACH_INS(cfg, ins) {
      if (INS_CADDRESS(ins) && INS_BBL(ins) && BBL_FUNCTION(INS_BBL(ins))) {
        const char* name = GetFullFunctionName(BBL_FUNCTION(INS_BBL(ins)));
        if (name) {
          fprintf(functionsdump, "%x,%s\n", INS_CADDRESS(ins), name);
        }
      }
    }
    fclose(functionsdump);
  }
  return 0;
}


static int
__helper_sort_csects(const void *a, const void *b)
{
  t_section **A = (t_section **)a;
  t_section **B = (t_section **)b;
  return AddressSub(SECTION_OLD_ADDRESS(*A),SECTION_OLD_ADDRESS(*B));
}

static int
__helper_find_csect(const void *a, const void *b)
{
  t_address *addr = (t_address *)a;
  t_section **sec = (t_section **)b;
  if (AddressIsGe(*addr,SECTION_OLD_ADDRESS(*sec)) &&
      AddressIsLt(*addr,AddressAdd(SECTION_OLD_ADDRESS(*sec),SECTION_OLD_SIZE(*sec))))
   return 0; 
  else if (AddressIsLt(*addr,SECTION_OLD_ADDRESS(*sec)))
    return -1;
  else
    return 1;
}

t_hash_table* function_to_names = NULL;

void AllocFunctionObjects(t_cfg *cfg)
{
  t_object *tmp, *subobj;
  t_ptr_array csects;
  t_function *fun;
  t_address_hash_entry* entry;

  PtrArrayInit(&csects,FALSE);

  function_to_names = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  /* a. Get all code sections and sort them by start address */
  OBJECT_FOREACH_SUBOBJECT(CFG_OBJECT(cfg), subobj, tmp) 
  {
    for (t_uint32 tel = 0; tel < OBJECT_NCODES(subobj); tel++)
    {
      t_section *sec = OBJECT_CODE(subobj)[tel];
      VERBOSE(2,("Found section with size @G: @T",SECTION_OLD_SIZE(sec),sec));
      PtrArrayAdd(&csects,sec);
    }
  }
  /* bubble sort would be better since they're mostly ordered, but... */
  qsort(csects.arr,csects.count,sizeof(t_section*),__helper_sort_csects);

  /* b. For each function, figure out to which object it belongs */
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_section **psec;
    t_bbl *bbl;
    t_address addr = AddressNullForCfg(cfg);

    if (FUNCTION_IS_HELL(fun))
      continue;

    /* find an address that belongs to the function */
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      if (AddressIsNull(BBL_OLD_ADDRESS(bbl)))
        continue;
      addr = BBL_OLD_ADDRESS(bbl);
      break;
    }
    if (AddressIsNull(addr))
    {
      VERBOSE(2,("(objlist)   No address found for function"));
      continue;
    }
    /* map the address to a code section */
    psec = (t_section**) bsearch(&addr,csects.arr,csects.count,sizeof(t_section*),__helper_find_csect);
    if (psec)
    {
      char* result = (char*) Malloc(2000); // TODO TODO TODO
      t_string oname = OBJECT_NAME(SECTION_OBJECT(*psec));

      //VERBOSE(0,("function @F comes from object %s",fun,oname));
      *result = 0;
      strcat(result, oname);
      strcat(result, FUNCTION_NAME(fun));

      /* To the list of functions to process */
      entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
      entry->address = (uintptr_t) fun;
      entry->data = result;

	    HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address); /* Correct? TODO */
      HashTableInsert(function_to_names,entry);
    }
    else
    {
      VERBOSE(2,("(objlist) Cannot figure out to which object function @F at address @G belongs",fun,addr));
    }
  }

  PtrArrayFini(&csects,FALSE);
}

const char* GetFullFunctionName(t_function* fun) {
  t_address_hash_entry* entry;

  entry = (t_address_hash_entry*) HashTableLookup(function_to_names, &fun);
  if (entry) {
    return (const char*) (entry->data);
  }
  return NULL;
}

/* }}} */
