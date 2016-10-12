/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*! \file diablosupport_string.h
 *
 * String support
 *
 */

/* String Defines {{{ */
#ifndef DIABLOSUPPORT_STRING_DEFINES
#define DIABLOSUPPORT_STRING_DEFINES
/*! \brief iterate over all string array elements in a string array */
#define STRING_ARRAY_FOREACH_ELEM(x,y) for (y=x->first; y!=NULL; y=y->next)
/*! \brief iterate over all string array elements in a string array, and allow
 * deletion */
#define STRING_ARRAY_FOREACH_ELEM_SAFE(x,y,z) for (y=x->first, z=y?y->next:NULL; y!=NULL; y=z, z=y?y->next:NULL)
/*! \brief iterate over all string array elements reverse */
#define STRING_ARRAY_FOREACH_ELEM_R(x,y) for (y=x->last; y!=NULL; y=y->prev)
#endif /* }}} String Defines */
/* String Typedefs {{{ */
#ifndef DIABLOSUPPORT_STRING_TYPEDEFS
#define DIABLOSUPPORT_STRING_TYPEDEFS
/*! \brief Typedef for a string array */
typedef struct _t_string_array t_string_array;

/*! \brief Typedef for an element in a string array */
typedef struct _t_string_array_elem t_string_array_elem;

/*! \brief Typedef for a string table */
typedef struct _t_strtab t_strtab;

/*! \brief Typedef for a string table hash element*/
typedef struct _t_strtab_hash t_strtab_hash;

/*! \brief Typedef for a string */
typedef char *t_string;
typedef const char *t_const_string;
typedef char *renamed_t_string;
#endif /* }}} Typedefs */

#include <diablosupport.h>

#ifdef DIABLOSUPPORT_TYPES
/* String Types {{{ */
#ifndef DIABLOSUPPORT_STRING_TYPES
#define DIABLOSUPPORT_STRING_TYPES
/*! \brief An element in a string array */
struct _t_string_array_elem
{
  /*! \brief the string for this element */
  t_string string;
  /*! \brief pointer to the next element */
  struct _t_string_array_elem *next;
  /*! \brief pointer to the previous element */
  struct _t_string_array_elem *prev;
};

/*! \brief A string array */
struct _t_string_array
{
  /*! \brief the number of strings in the array */
  t_uint16 nstrings;
  /*! \brief pointer to the first element */
  struct _t_string_array_elem *first;
  /*! \brief pointer to the last element */
  struct _t_string_array_elem *last;
};

/*! \brief A string table */
struct _t_strtab
{
  char * strtab;
  t_uint32 len;
  t_hash_table * ht;
};
#endif /* }}} String Types */
#endif /* Diablosupport Types */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* String Functions {{{ */
#ifndef DIABLOSUPPORT_STRING_FUNCTIONS
#define DIABLOSUPPORT_STRING_FUNCTIONS
/*! \brief A string table hash element*/
struct _t_strtab_hash
{
  t_hash_table_node node;
  t_uint32 offset;
};
/*** Various ***/
/*! \brief return TRUE if the character matches the pattern specified in the string
 * \todo does not match naming conventions */
t_bool CharStringPatternMatch (t_const_string, char);

/*! \brief convert a string to an unsigned int (32 bits) */
t_uint32 StringToUint32 (t_const_string, t_uint32);

/*! \brief convert a string to an unsigned int (64 bits) */
t_uint64 StringToUint64 (t_const_string, t_uint32);

/*** Strings ***/
/*! \brief Duplicates a string */
#define StringDup(x) RealStringDup(FORWARD_MALLOC_DEFINE x)
#define StringnDup(x, n) RealStringnDup(FORWARD_MALLOC_DEFINE x, n)
/*! \brief Alloc wrapper. Do not use. Use StringDup() */
t_string RealStringDup (FORWARD_MALLOC_PROTOTYPE t_const_string);
t_string RealStringnDup(FORWARD_MALLOC_FUNCTION_DEF t_const_string in, int length);

/*! \brief Compares 2 strings */
t_int32 StringCmp (t_const_string, t_const_string);

/*! \brief Remove trailing character from string and return TRUE if one is found */
t_bool StringChop (t_string, char);

/*! \brief Remove a trailing newline from a string */
void StringChomp (t_string);

/*! \brief TODO */
void StringTrim (t_string);

/*! \brief Concatenates 2 strings */
#define StringConcat2(x,y) RealStringConcat2(FORWARD_MALLOC_DEFINE x,y)
/*! \brief Alloc wrapper. Do not use. Use StringConcat2() */
t_string RealStringConcat2 (FORWARD_MALLOC_PROTOTYPE t_const_string,
                            t_const_string);

/*! \brief Concatenates 3 strings */
#define StringConcat3(x,y,z) RealStringConcat3(FORWARD_MALLOC_DEFINE x,y,z)
/*! \brief Alloc wrapper. Do not use. Use StringConcat3() */
t_string RealStringConcat3 (FORWARD_MALLOC_PROTOTYPE t_const_string,
                            t_const_string, t_const_string);

/*! \brief Concatenates 4 strings */
#define StringConcat4(w,x,y,z) RealStringConcat4(FORWARD_MALLOC_DEFINE w,x,y,z)
/*! \brief Alloc wrapper. Do not use. Use StringConcat4() */
t_string RealStringConcat4 (FORWARD_MALLOC_PROTOTYPE t_const_string,
                            t_const_string, t_const_string, t_const_string);

/*! \brief Checks if the second string matches the pattern defined in the first string */
t_uint32 StringPatternMatch (t_const_string, t_const_string);

/*! \brief Chops up the first string using the separators in string 2. */
#define StringDivide(in,separators,strict,keep_sep) RealStringDivide(FORWARD_MALLOC_DEFINE in,separators,strict,keep_sep)

/*! \brief Alloc wrapper. Do not use. Use StringDivide() */
t_string_array *RealStringDivide (FORWARD_MALLOC_PROTOTYPE t_const_string, t_const_string,
                                  t_bool, t_bool);

/*! \brief Return a hash value for a string */
t_uint32 StringHash (t_const_string, const t_hash_table *);

/*! \brief Converts an ascii string to upper case */
void StringToUpper(t_string);

/*! \brief Converts an ascii string to lower case */
void StringToLower(t_string);

/*** StringArrays ***/
/*! \brief Allocates and initializes and empty string array */
#define StringArrayNew() RealStringArrayNew(FORWARD_MALLOC_ONLY_DEFINE)
/*! \brief Alloc wrapper. Do not use. Use StringArrayNew() */
t_string_array *RealStringArrayNew (FORWARD_MALLOC_ONLY_PROTOTYPE);

/*! \brief Appends a string to a string array (the string must be allocated!) */
#define StringArrayAppendString(x,y) RealStringArrayAppendString(FORWARD_MALLOC_DEFINE x,y)
/*! \brief Alloc wrapper. Do not use. Use StringArrayAppendString() */
void RealStringArrayAppendString (FORWARD_MALLOC_PROTOTYPE t_string_array *,
                                  t_string);

/*! \brief Prepends a string to a string array (the string must be allocated!) */
#define StringArrayPrependString(x,y) RealStringArrayPrependString(FORWARD_MALLOC_DEFINE x,y)
/*! \brief Alloc wrapper. Do not use. Use StringArrayPrependString() */
void RealStringArrayPrependString (FORWARD_MALLOC_PROTOTYPE t_string_array *
                                   array, t_string);

/*! \brief Returns the number of strings in a string array */
t_uint16 StringArrayNStrings (const t_string_array *);

/*! \brief Concatenates all strings in the string array (using the second argument as a separator) */
#define StringArrayJoin(in,con) RealStringArrayJoin(FORWARD_MALLOC_DEFINE in,con)
/*! \brief Alloc wrapper. Do not use. Use StringArrayJoin() */
t_string RealStringArrayJoin (FORWARD_MALLOC_PROTOTYPE const t_string_array *,
                              t_const_string);

/*! \brief Deallocates a string array (and all strings in it) */
void StringArrayFree (const t_string_array *);
/*** StringTable ***/
t_strtab_hash * StrtabHashNew (t_const_string, t_uint32);
void StrtabHashFree (const t_strtab_hash *, void *);
t_strtab * StrtabNew ();
char * StrtabFindString (const t_strtab *,  t_const_string);
t_uint32 StrtabAddString (t_strtab *, t_const_string);
void StrtabFree (const t_strtab *);
#endif /* }}} String Functions */
#endif /* Diablosupport Functions */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
