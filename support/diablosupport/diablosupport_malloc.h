/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Malloc support
 *
 * Interface: TODO
 */
#include <diablosupport.h>
/* Malloc Defines {{{ */
#ifndef DIABLOSUPPORT_MALLOC_DEFINES
#define DIABLOSUPPORT_MALLOC_DEFINES

/* The following macros are used for malloc debugging (leak checking). They
 * make it possible to find the allocation program point for memory that is
 * never freed. Constructors and other functions that allocate memory but that
 * do not free it should be specified as:
 *
 * prototype (in the header file):
 * ret_type * RealFunction(FORWARD_MALLOC_PROTOTYPE args);
 * define (in the header file):
 * Function(FORWARD_MALLOC_DEFINE args)
 *
 * function definition (in the c file):
 * ret_type * RealFunction(FORWARD_MALLOC_DEF args)
 * {
 * ret_type * ret = RealSomeKindOfAllocFunction(FORWARD_MALLOC_USE extra_args);
 * ...
 * return ret;
 * }
 *
 * You can of course just ignore this and call the Malloc/Calloc/Realloc
 * functions in your constructors instead of there Real variant. In this case,
 * the constructor will be listed as the allocation program point (instead of
 * the caller of the constructor)
 */

#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define FORWARD_MALLOC_DEFINE __FILE__,__LINE__,
#define FORWARD_MALLOC_ONLY_DEFINE __FILE__,__LINE__
#define FORWARD_MALLOC_PROTOTYPE t_const_string, t_uint32,
#define FORWARD_MALLOC_ONLY_PROTOTYPE t_const_string, t_uint32
#define FORWARD_MALLOC_FUNCTION_DEF t_const_string filename, t_uint32 lnno,
#define FORWARD_MALLOC_ONLY_FUNCTION_DEF t_const_string filename, t_uint32 lnno
#define FORWARD_MALLOC_FUNCTION_USE filename, lnno,
#define FORWARD_MALLOC_ONLY_FUNCTION_USE filename, lnno
#else /* Debug Malloc */
#define FORWARD_MALLOC_DEFINE
#define FORWARD_MALLOC_ONLY_DEFINE
#define FORWARD_MALLOC_PROTOTYPE
#define FORWARD_MALLOC_ONLY_PROTOTYPE
#define FORWARD_MALLOC_FUNCTION_DEF
#define FORWARD_MALLOC_ONLY_FUNCTION_DEF
#define FORWARD_MALLOC_FUNCTION_USE
#define FORWARD_MALLOC_ONLY_FUNCTION_USE
#endif /* Debug Malloc */
#endif /* }}} Malloc Defines */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* Malloc Functions {{{ */
#ifndef DIABLOSUPPORT_MALLOC_FUNCTIONS
#define DIABLOSUPPORT_MALLOC_FUNCTIONS
/* Malloc */
#define Malloc(x) RealMalloc(FORWARD_MALLOC_DEFINE x)
void *RealMalloc (FORWARD_MALLOC_PROTOTYPE t_uint32);

/* Calloc */
#define Calloc(x,y) RealCalloc(FORWARD_MALLOC_DEFINE x,y)
void *RealCalloc (FORWARD_MALLOC_PROTOTYPE t_uint32, t_uint32);

/* Realloc */
#define Realloc(x,y) RealRealloc(FORWARD_MALLOC_DEFINE x,y)
void *RealRealloc (FORWARD_MALLOC_PROTOTYPE const void *, t_uint32);

/* Free */
#define Free(x) RealFree(FORWARD_MALLOC_DEFINE x)
void RealFree (FORWARD_MALLOC_PROTOTYPE const void *);

/* CheckAddress */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define CheckAddress(x) RealCheckAddress(FORWARD_MALLOC_DEFINE x)
void RealCheckAddress (FORWARD_MALLOC_PROTOTYPE const void *);
#else /* Debug Malloc */
#define CheckAddress(x) while(0)
#endif /* Debug Malloc */
/* PrintRemainingBlocks */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define PrintRemainingBlocks() RealPrintRemainingBlocks()
void RealPrintRemainingBlocks (void);
void RealAllocOverride (FORWARD_MALLOC_PROTOTYPE void *);

#define AllocOverride(x) RealAllocOverride(FORWARD_MALLOC_DEFINE x)
#else /* Debug Malloc */
#define AllocOverride(x) while(0)
#define PrintRemainingBlocks() while(0)
#endif /* Debug Malloc */
                                     /* PrintRemainingBlocks */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define GetMemUse() RealGetMemUse()
                                     t_uint64 RealGetMemUse (void);
#else /* Debug Malloc */
#define RealGetMemUse() 0
#endif /* Debug Malloc */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define PrintMemUse() RealPrintMemUse()
                                     void RealPrintMemUse (void);
#else /* Debug Malloc */
#define RealPrintMemUse() while(0);
#endif /* Debug Malloc */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
#define PrintMallocInfo(x) RealPrintMallocInfo(x)
                                     void RealPrintMallocInfo (void *);
#else /* Debug Malloc */
#define RealPrintMallocInfo(x) while(0);
#endif /* Debug Malloc */
#endif /* }}} Malloc Functions */
#endif /* Diablosupport Functions */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
