/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

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

#ifndef DIVERSITY_STRUCTS
#define DIVERSITY_STRUCTS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _t_diversity_options t_diversity_options;
typedef struct _t_diversity_choice t_diversity_choice;

typedef struct {
  t_uint64 count;
  t_bool * flags;
}t_bool_list;

#define BoolListAdd(isTrue,list)	\
	do { \
	  if (list->flags) list->flags = (t_bool*) Realloc(list->flags,(list->count+1)*sizeof(t_bool)); \
	  else list->flags = (t_bool*) Malloc(sizeof(t_bool)); \
	  list->flags[list->count++] = isTrue; \
	} while (0)

//void BoolListAdd(t_bool isTrue, t_bool_list * list);
t_bool_list * BoolListNew();
void BoolListFree(t_bool_list * list);
t_bool_list * BoolListNewAll(t_uint64 count, t_bool isTrue);
t_bool_list * BoolListNewAllTrue(t_uint64 count);
t_bool_list * BoolListNewAllFalse(t_uint64 count);
t_bool_list * BoolListCopy(t_bool_list * list);

/*! This structure represents the available choices */
struct _t_diversity_options
{
  /*! a choice can be made from the interval [0,range]*/
  t_uint64 range;
  /*! is this a flag like transformation*/
  t_bool flags;
  /*! transformation finished*/
  t_bool done;
  
  void * element1;
  void * element2;
  t_uint32 phase;
};

static t_diversity_options diversity_options_null = { 0, FALSE, FALSE, NULL, NULL, 0 };

struct _t_diversity_choice
{
  t_bool_list * flagList;
  t_uint64 choice;
};

typedef struct _t_bblint_item t_bblint_item;
typedef struct _t_bblintList t_bblintList;

struct _t_bblint_item
{
  t_bbl * bbl;
  t_bblint_item * next;
  t_bblint_item * prev;
  t_int32 op[6];
  t_bool check;
};

struct _t_bblintList
{
  t_bblint_item * first;
  t_bblint_item * last;
  t_uint64 count;
};

t_bblint_item * BblIntListAdd(t_bblintList * list, t_bbl * bbl);
t_bblintList * BblIntListNew();
void BblIntListFree(t_bblintList * list);
void BblIntListUnlink(t_bblint_item  * item, t_bblintList * bblList);
t_bblint_item * BblIntListGetNthElement(t_bblintList * list, t_uint32 n);

typedef struct _t_bbl_item t_bbl_item;
typedef struct _t_bblList t_bblList;

struct _t_bbl_item
{
  t_bbl * bbl;
  t_bbl_item * next;
  t_bbl_item * prev;
};

struct _t_bblList
{
  t_bbl_item * first;
  t_bbl_item * last;
  t_uint64 count;
};

void BblListAdd(t_bblList * list, t_bbl * bbl);
t_bblList * BblListNew();
void BblListFree(t_bblList * list);
void BblListUnlink(t_bbl_item  * item, t_bblList * bblList);
t_bbl_item * BblListGetNthElement(t_bblList * list, t_uint32 n);

typedef struct _t_ins_item t_ins_item;
typedef struct _t_insList t_insList;

struct _t_ins_item
{
  t_ins * ins;
  t_ins_item * next;
  t_ins_item * prev;
};

struct _t_insList
{
  t_ins_item * first;
  t_ins_item * last;
  t_uint64 count;
};

void InsListAdd(t_insList * list, t_ins * ins);
t_insList * InsListNew();
void InsListFree(t_insList * list);
void InsListUnlink(t_ins_item  * item, t_insList * insList);
t_ins_item * InsListGetNthElement(t_insList * list, t_uint32 n);

typedef struct _t_address_item t_address_item;
typedef struct _t_addressList t_addressList;

struct _t_address_item
{
  t_address address;
  t_address_item * next;
  t_address_item * prev;
};

struct _t_addressList
{
  t_address_item * first;
  t_address_item * last;
  t_uint64 count;
};

void AddressListAdd(t_addressList * list, t_address address);
void AddressListAddList(t_addressList * add_to, t_addressList * to_be_added);
t_addressList * AddressListNew();
void AddressListFree(t_addressList * list);
void AddressListUnlink(t_address_item  * item, t_addressList * addressList);


/* {{{ Array Lists */
typedef struct _t_arraylist t_arraylist;
struct _t_arraylist
{
	int max_length;
	int current_length;
	int element_size;
	char* data; // char* is the new void*
};

t_arraylist* ArrayListNew(int element_size, int initial_length);
void ArrayListFree(t_arraylist* arr);
void ArrayListDeepFree(t_arraylist* arr);
void ArrayListAdd(t_arraylist* arr, void* element);
void ArrayListRemove(t_arraylist* arr, int element);
void ArrayListSort(t_arraylist* arr, int(*cmp)(const void *, const void *));

#define ARRAYLIST_GET(arr, i, type) *(type*)(arr->data + (i) * arr->element_size)

#define FOREACH_ARRAY_ITEM(arr, iterator, item, type) \
  for(iterator = 0; item = *(type*)(arr->data + iterator * arr->element_size), iterator < arr->current_length; iterator++)


/* }}} */


/* {{{ For transformations in the iterative engine */
typedef struct _t_transformation_info t_transformation_info;
typedef struct _t_transformation_cost_info t_transformation_cost_info;

struct _t_transformation_info
{
  /* Returns NULL if cannot be transformed, and an ArrayList of the different possible t_transformation_cost_infos */
  t_arraylist*        (*can_transform)(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info);
	t_diversity_options (*do_transform)(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info, t_randomnumbergenerator* rng);
  const char*         name;
  int                 idx;
  t_bool              whole_function_transform;
};

t_arraylist* SimpleCanTransform(t_bbl* bbl, void** additional_info, int cost);

struct _t_transformation_cost_info
{
	int cost;
  t_transformation_info* transformation;
	t_bbl* bbl;
  void* additional_info;
};


typedef struct _t_diversity_function_info t_diversity_function_info;
struct _t_diversity_function_info {
  t_address function_address;
  int seed;
  int transformations_left;
  int max_cost;
  int times_iterated_with_current_cost;
  int should_redo;
};

typedef struct _t_transform_function_helper t_transform_function_helper;
struct _t_transform_function_helper {
  t_arraylist* transformations;
  t_arraylist* previous_information;
  t_cfg* cfg;
  FILE* outfile;
};

/* {{{ Hash table for keeping track of something that has an address */
typedef struct _t_address_hash_entry t_address_hash_entry;
struct _t_address_hash_entry {
  t_hash_table_node node;
  //t_address address;
  uintptr_t address; // This was originally a t_address, but C++ rightfully complains when I cram t_function* in them....
  void* data; /* typically a t_function* or a t_bb* */
};
/* }}} */
/* }}} */

#endif

#ifndef DIV_DYNAMIC_MEMBERS
#define DIV_DYNAMIC_MEMBERS

/*step 1*/
extern t_dynamic_member_info ins_old_addresses_array;

/*step 2*/
void 
InsOldAddressesInit(t_ins * ins, t_addressList ** list);

void 
InsOldAddressesFini(t_ins * ins, t_addressList ** list);

void 
InsOldAddressesDup(t_ins * ins, t_addressList ** list);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_old_addresses_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_addressList *,               /*the type of the member*/
    addressList,            /*the name of the member in lower case*/
    ADDRESSLIST,            /*the name of the member in upper case*/
    AddressList,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsOldAddressesInit,     /*The function to be called when an instance of the
			    class is created*/
    InsOldAddressesFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsOldAddressesDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

/*step 1*/
extern t_dynamic_member_info ins_score_array;

/*step 2*/
void 
InsScoreInit(t_ins * ins, float * score);

void 
InsScoreFini(t_ins * ins, float * score);

void 
InsScoreDup(t_ins * ins, float  * score);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_score_array,  /*an instance of t_dynamic_member_info to store the members*/
    float,               /*the type of the member*/
    score,            /*the name of the member in lower case*/
    SCORE,            /*the name of the member in upper case*/
    Score,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsScoreInit,     /*The function to be called when an instance of the
			    class is created*/
    InsScoreFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsScoreDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );
/*step 1*/
extern t_dynamic_member_info ins_cost_array;

/*step 2*/
void 
InsCostInit(t_ins * ins, t_int32 * cost);

void 
InsCostFini(t_ins * ins, t_int32 * cost);

void 
InsCostDup(t_ins * ins, t_int32  * cost);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_cost_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_int32,               /*the type of the member*/
    cost,            /*the name of the member in lower case*/
    COST,            /*the name of the member in upper case*/
    Cost,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsCostInit,     /*The function to be called when an instance of the
			    class is created*/
    InsCostFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsCostDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );
/*step 1*/
extern t_dynamic_member_info ins_gain_array;

/*step 2*/
void 
InsGainInit(t_ins * ins, t_int32 * gain);

void 
InsGainFini(t_ins * ins, t_int32 * gain);

void 
InsGainDup(t_ins * ins, t_int32  * gain);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_gain_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_int32,               /*the type of the member*/
    gain,            /*the name of the member in lower case*/
    GAIN,            /*the name of the member in upper case*/
    Gain,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsGainInit,     /*The function to be called when an instance of the
			    class is created*/
    InsGainFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsGainDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );
/*step 1*/
extern t_dynamic_member_info bbl_factored_array;

/*step 2*/
void BblFactoredInit(t_bbl * bbl, t_bool * factored);
void BblFactoredFini(t_bbl * ins, t_bool * factored);
void BblFactoredDup(t_bbl * bbl, t_bool * factored);

/*step 3*/
DYNAMIC_MEMBER(
    bbl,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    bbl_factored_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_bool,               /*the type of the member*/
    factored,            /*the name of the member in lower case*/
    FACTORED,            /*the name of the member in upper case*/
    Factored,            /*the name of the member in regular case*/
    CFG_FOREACH_BBL,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    BblFactoredInit,     /*The function to be called when an instance of the
			    class is created*/
    BblFactoredFini,     /*The function to be called when an instance of the
			    class is killed*/
    BblFactoredDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

/*step 1*/
extern t_dynamic_member_info function_factored_array;

/*step 2*/
void FunctionFactoredInit(t_function * function, t_bool * factored);
void FunctionFactoredFini(t_function * ins, t_bool * factored);
void FunctionFactoredDup(t_function * function, t_bool * factored);

/*step 3*/
DYNAMIC_MEMBER(
    function,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    function_factored_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_bool,               /*the type of the member*/
    factored,            /*the name of the member in lower case*/
    FACTORED,            /*the name of the member in upper case*/
    Factored,            /*the name of the member in regular case*/
    CFG_FOREACH_FUN,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    FunctionFactoredInit,     /*The function to be called when an instance of the
			    class is created*/
    FunctionFactoredFini,     /*The function to be called when an instance of the
			    class is killed*/
    FunctionFactoredDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

/* Keep track of which Basic Blocks have been transformed by which transformations: */
/*step 1*/
extern t_dynamic_member_info bbl_transformations_array;

/*step 2*/
void BblTransformationsInit(t_bbl * bbl, t_arraylist** transformations);
void BblTransformationsFini(t_bbl * ins, t_arraylist** transformations);
void BblTransformationsDup(t_bbl * bbl, t_arraylist** transformations);

/*step 3*/
DYNAMIC_MEMBER(
    bbl,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    bbl_transformations_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_arraylist*,               /*the type of the member*/
    transformations,            /*the name of the member in lower case*/
    TRANSFORMATIONS,            /*the name of the member in upper case*/
    Transformations,            /*the name of the member in regular case*/
    CFG_FOREACH_BBL,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    BblTransformationsInit,     /*The function to be called when an instance of the
			    class is created*/
    BblTransformationsFini,     /*The function to be called when an instance of the
			    class is killed*/
    BblTransformationsDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

/* Keep track of which functions have been transformed by which transformations: */
/*step 1*/
extern t_dynamic_member_info fun_transformations_array;

/*step 2*/
void FunctionTransformationsInit(t_function* fun, t_arraylist** transformations);
void FunctionTransformationsFini(t_function * fun, t_arraylist** transformations);
void FunctionTransformationsDup(t_function* fun, t_arraylist** transformations);

/*step 3*/
DYNAMIC_MEMBER(
    function,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    fun_transformations_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_arraylist*,               /*the type of the member*/
    transformations,            /*the name of the member in lower case*/
    TRANSFORMATIONS,            /*the name of the member in upper case*/
    Transformations,            /*the name of the member in regular case*/
    CFG_FOREACH_FUN,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    FunctionTransformationsInit,     /*The function to be called when an instance of the
			    class is created*/
    FunctionTransformationsFini,     /*The function to be called when an instance of the
			    class is killed*/
    FunctionTransformationsDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );


/* Keep track of which functions have been transformed by which transformations: */
/*step 1*/
extern t_dynamic_member_info fun_possible_transformations_array;

/*step 2*/
void FunctionPossibleTransformationsInit(t_function* fun, t_arraylist** transformations);
void FunctionPossibleTransformationsFini(t_function * fun, t_arraylist** transformations);
void FunctionPossibleTransformationsDup(t_function* fun, t_arraylist** transformations);

/*step 3*/
DYNAMIC_MEMBER(
    function,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    fun_possible_transformations_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_arraylist*,               /*the type of the member*/
    possible_transformations,            /*the name of the member in lower case*/
    POSSIBLE_TRANSFORMATIONS,            /*the name of the member in upper case*/
    PossibleTransformations,            /*the name of the member in regular case*/
    CFG_FOREACH_FUN,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    FunctionPossibleTransformationsInit,     /*The function to be called when an instance of the
			    class is created*/
    FunctionPossibleTransformationsFini,     /*The function to be called when an instance of the
			    class is killed*/
    FunctionPossibleTransformationsDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );



/* Keep track of which functions have which 'original address': */
/*step 1*/
extern t_dynamic_member_info ins_original_address_array;

/*step 2*/
void InsOriginalAddressInit(t_ins* fun, t_address* orig_address);
void InsOriginalAddressFini(t_ins * fun, t_address* orig_address);
void InsOriginalAddressDup(t_ins* fun, t_address* orig_address);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_original_address_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_address,               /*the type of the member*/
    original_address,            /*the name of the member in lower case*/
    ORIGINAL_ADDRESS,            /*the name of the member in upper case*/
    OriginalAddress,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsOriginalAddressInit,     /*The function to be called when an instance of the
			    class is created*/
    InsOriginalAddressFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsOriginalAddressDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

/* Keep track of execution count of instructions, for when BBLs are gone : */
/*step 1*/
extern t_dynamic_member_info ins_exec_count_array;

/*step 2*/
void InsExecCountInit(t_ins* fun, t_int64* cnt);
void InsExecCountFini(t_ins * fun, t_int64* cnt);
void InsExecCountDup(t_ins* fun, t_int64* cnt);

/*step 3*/
DYNAMIC_MEMBER(
    ins,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    ins_exec_count_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_int64,               /*the type of the member*/
    execcount,            /*the name of the member in lower case*/
    EXECCOUNT,            /*the name of the member in upper case*/
    ExecCount,            /*the name of the member in regular case*/
    CFG_FOREACH_INS,      /*an iterator which iterates over all instances of 
			    the managed class contained in the manager class*/
    InsExecCountInit,     /*The function to be called when an instance of the
			    class is created*/
    InsExecCountFini,     /*The function to be called when an instance of the
			    class is killed*/
    InsExecCountDup       /*The function to be called when an instance of the
			    class is duplicated*/
    );

#ifdef __cplusplus
}
#endif

#endif
