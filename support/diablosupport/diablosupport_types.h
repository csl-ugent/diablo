/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Type support
 *
 * Interface:
 *
 */

#include <diablosupport.h>

/* Types Typedefs {{{ */
#ifndef DIABLOSUPPORT_TYPES_TYPEDEFS
#define DIABLOSUPPORT_TYPES_TYPEDEFS

/* For the UINT_MAX, SIZE_MAX, etc. macro's */
#include <limits.h>

#ifdef __cplusplus
#ifdef __STDC_CONSTANT_MACROS
#undef __STDC_CONSTANT_MACROS
#endif
#define __STDC_CONSTANT_MACROS 1

#ifdef __STDC_LIMIT_MACROS
#undef __STDC_LIMIT_MACROS
#endif
#define __STDC_LIMIT_MACROS 1
#endif
#include <stdint.h>

/* For PRId64 */
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

/* Typedefs from these general C integer types to Diablo types */
typedef uint8_t t_uint8, renamed_t_uint8;
typedef int8_t t_int8, renamed_t_int8;

typedef uint16_t t_uint16, renamed_t_uint16;
typedef int16_t t_int16, renamed_t_int16;

typedef uint32_t t_uint32, renamed_t_uint32;
typedef int32_t t_int32, renamed_t_int32;

typedef uint64_t t_uint64, renamed_t_uint64;
typedef int64_t t_int64, renamed_t_int64;

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef PERHAPS
#undef PERHAPS
#endif
#ifdef YES
#undef YES
#endif
#ifdef NO
#undef NO
#endif

#include <stdbool.h>

#define FALSE false
#define TRUE true
typedef bool t_bool, renamed_t_bool;
typedef enum
{ NO = FALSE, YES = TRUE, PERHAPS = 2 } t_tristate;

typedef uintptr_t t_pointer_int;

static inline void ignore_result_helper(int __attribute__((unused)) dummy, ...)
{
}

#define IGNORE_RESULT(X) ignore_result_helper(0,(X))

#endif /* }}} Types Typedefs */

/* Types Define {{{ */
#ifndef DIABLOSUPPORT_TYPES_DEFINES
#define DIABLOSUPPORT_TYPES_DEFINES

#define MAXIMUM(x,y) (((x)>(y))?(x):(y))
#define MINIMUM(x,y) (((x)<(y))?(x):(y))

#define LL(x) (x##LL)
#define ULL(x) (x##ULL)

#define T_POINTER_INT(x) ((t_pointer_int) x)
#define T_TRISTATE(x) ((x) ? YES : NO)

#define Uint16SwapEndian(val) ((t_uint16) ((((t_uint16) (val)&(t_uint16) 0xffU)<<8) | (((t_uint16) (val)&(t_uint16) 0xff00U)>>8)))
#define Uint32SwapEndian(val) ((t_uint32) ((((t_uint32) (val)&(t_uint32) 0xffU)<<24)|(((t_uint32) (val)&(t_uint32) 0xff00U)<<8)|(((t_uint32) (val)&(t_uint32) 0xff0000U)>>8)|(((t_uint32) (val)&(t_uint32) 0xff000000U)>>24)))
#define Uint64SwapEndian(val) ((t_uint64) ((((t_uint64) (val)&(t_uint64) ULL(0xff))<<56)|(((t_uint64) (val)&(t_uint64) ULL(0xff00))<<40)|(((t_uint64) (val)&(t_uint64) ULL(0xff0000))<<24)|(((t_uint64) (val)&(t_uint64) ULL(0xff000000))<<8)|(((t_uint64) (val)&(t_uint64) ULL(0xff00000000))>>8)|(((t_uint64) (val)&(t_uint64) ULL(0xff0000000000))>>24)|(((t_uint64) (val)&(t_uint64) ULL(0xff000000000000))>>40)|(((t_uint64) (val)&(t_uint64) ULL(0xff00000000000000))>>56)))

/* For explanation of the rotates, see: http://blog.regehr.org/archives/1063 */
#define Uint32RotateRight(x,r) ( (( ((t_uint32)(x)) << ((-r) & 31)) | ( ((t_uint32)(x)) >> (r) )) & 0xffffffff )
#define Uint32RotateLeft(x, r)( (( ((t_uint32)(x)) >> ((t_uint32)((-r) & 31))) | ( (t_uint32)(x) << (r) )) & 0xffffffff )
#define Uint32GetBit(x,b) ( ((x) >> (b)) & 0x1)
#define Uint32SignExtend(x,p) (((x) ^ ((t_uint32)1 << (p))) - ((t_uint32)1 << (p)))
#define Uint32SelectBits(i,b,e) ((((t_uint32) (i))&(((t_uint32)1<<((b)+1))-1))>>(e))

/* TODO
 * \param x Number to test for sign-extension
 * \param p Position of the sign bit
 * \return TRUE if x is a valid sign-extended number (sign-bit p)
 */
#define Uint32CheckSignExtend(x,p) ((((t_uint32)(x) & ~((1<<(p)) - 1)) == ~((1<<(p)) - 1)) || (((t_uint32)(x) & ~((1<<(p)) - 1)) == 0))

#define Uint64SignExtend(x,p) ((((t_uint64)(x)) ^ (1ULL << (p))) - (1ULL << (p)))

#define Uint32CountSetBits(x) __builtin_popcount(x)
#define Uint64CountSetBits(x) __builtin_popcountll(x)
#endif /* }}} Types Defines */

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
