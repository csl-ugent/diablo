/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2012, Bart Coppens
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

#include <diablodiversity.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DIABLO_ENGINE_ITERATIVE_H
#define DIABLO_ENGINE_ITERATIVE_H

t_int32 DiversityEngineIterative(t_cfg * cfg);

#define MAX_TRANSFORMS 50
extern int current_transform_max_idx;
extern t_transformation_info* supported_transforms[MAX_TRANSFORMS];


t_bool TrueWithChance(int chance); // Already defined in diablodiversity_engine.c

/* {{{ Hash table for keeping track of something that has an address */
t_uint32 FunHashAddress(const void* key, const t_hash_table * table);
int FunAddressCmp(const void* key, const void* key2);
void FunAddrKeyFree(const void* he, void * data);
/* }}} */

/* {{{ Different reasons BinDiff can match functions */
#define BD_REASON_EDGES_HASH_MATCHING 2
#define BD_REASON_EDGES_FLOWGRAPH 3
#define BD_REASON_EDGES_CALLGRAPH 4

#define BD_REASON_EDGES_PRIME_SIG 7

#define BD_REASON_CALL_SEQUENCE_EXACT 16
#define BD_REASON_CALL_SEQUENCE_TOPOLOGY 17
#define BD_REASON_CALL_SEQUENCE_SEQUENCE 18
#define BD_REASON_CALL_REFERENCE 19
/* }}} */


/* {{{ For use by the different iterative transformer strategies */

/* Comparator for t_diversity_function_info */
int order_funinfo_retransform(const void *a, const void *b);

/* Add a possible transform to the list in the function */
void AddPossibleTransform(t_function* fun, t_transformation_info* transform);

/* Get a t_transformation_info* based on the character string */
t_transformation_info* GetTransformByName(const char* name);

/* Output some extra info about transformations */
void ExtraOutput(const char* prefix, t_cfg* cfg, t_function* fun);

/* Get the original adddress (address of its first instruction) in the original binary, if it exists */
uintptr_t address_of_function(t_function* fun);

/* Add this function to the list of functions to be transformed again*/
void SetFunctionAsMapped(t_function* fun);

/* Has this function been mapped by bindiff? */
t_bool functionIsMapped(t_function* fun);

/* {{{ Global references to the (singleton) transformations */
extern t_transformation_info* transformationDiversityFlipBranches;
extern t_transformation_info* transformationDiversityOpaquePredicates;
extern t_transformation_info* transformationDiversityFlattenFunction;
extern t_transformation_info* transformationDiversityInlineFunction;
extern t_transformation_info* transformationDiversityThwartDisassembly;
extern t_transformation_info* transformationDiversitySplitByTwoWayPredicate;
extern t_transformation_info* transformationDiversityBranchFunctionBeforeAndAfterCalls;
extern t_transformation_info* transformationDiversityBranchFunctionBeforeAndAfterCallsGlobalVar;
extern t_transformation_info* transformationDiversityBranchFunctionInFirstBlock;
extern t_transformation_info* transformationDiversityUnfold;

/* }}} */

t_arraylist* ListPossibleTransformations(t_cfg* cfg, t_function* fun, t_arraylist* transformations, int max_cost);
void FreeTransformations(t_arraylist* transformations);
/* }}} */

/* {{{ The different iterative transformer strategies */
/* {{{ Single seed */
void InitStrategySingleSeed(t_cfg* cfg);
void FunctionHashTableTransformFunctionSingleSeed(void* e, void* data);
void StrategySingleSeedDoTransforms(FILE* iterative_file, t_arraylist* previous_information, t_arraylist* transformations, t_cfg* cfg, t_hash_table* functions_table);
void FinishStrategySingleSeed(t_cfg* cfg);

int TransformFunction(t_cfg* cfg, t_function* fun, t_arraylist* transformations, int seed, int max_cost);

t_arraylist* DiversityEngineReadIterativeFile(FILE* file);
/* }}} */

/* {{{ Scripted */
void InitStrategyScripted(t_cfg* cfg);
void StrategyScriptedDoTransforms(FILE* iterative_file, t_arraylist* previous_information, t_arraylist* transformations, t_cfg* cfg, t_hash_table* functions_table);
void FiniStrategyScripted();
/* }}} */

#endif /* DIABLO_ENGINE_ITERATIVE_H */

#ifdef __cplusplus
}
#endif

/* }}} */
