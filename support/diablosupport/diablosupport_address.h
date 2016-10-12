/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*
 *
 * Diablo can be compiled with 32-bit only, 64-bit only or 32-bit and
 * 64-bit address support. i.e. for 32-bit, 64-bit target
 * architectures or both. The latter results in cross-compaction
 * possibilities for both types of architectures. To include 32-bit
 * support, compile with -D BIT32ADDRSUPPORT, for 64-bit architectures
 * compile with -D BIT64ADDRSUPPORT. Note that this is completely
 * independent of the platform that is used to compile/use Diablo
 * itself.
 *
 * For all three possibilities, we have provided a number of
 * operations (with macro's or functions) that efficiently perform the
 * most common operations, such as addition, comparison, etc. If both
 * 32-bit and 64-bit targets are supported, most operations are much
 * more expensive, as they require type checking and are implemented
 * as procedures. For portability, we urge you to use these generic
 * operations!!!
 *
 * This documentation is written with both 32-bit and 64-bit support in mind. */
#include <diablosupport.h>

#ifdef BIT32ADDRSUPPORT
#ifdef BIT64ADDRSUPPORT
#define GENERICADDRSUPPORT
#endif
#endif

/* Address Typedefs {{{ */
#ifndef DIABLOSUPPORT_ADDRESS_TYPEDEFS
#define DIABLOSUPPORT_ADDRESS_TYPEDEFS
typedef struct _t_address t_address_generic;

#ifdef GENERICADDRSUPPORT
/*! An address if we have 32 bit and 64 bit address support */
typedef struct _t_address t_address, renamed_t_address;
#else
#ifdef BIT32ADDRSUPPORT
/*! An address if we only have 32 bit address support */
typedef t_uint32 t_address, renamed_t_address;
typedef t_int32 t_signed_address;
#else
/*! An address if we only have 64 bit address support */
typedef t_uint64 t_address, renamed_t_address;
typedef t_int64 t_signed_address;
#endif
#endif
typedef enum _t_address_type
{
  ADDRSIZE32,
  ADDRSIZE64
} t_address_type;

t_uint32 AddressSizeInBytes(t_address_type address_type);

#endif /* }}} Address Typedefs */
/* Address Types {{{ */
#ifndef DIABLOSUPPORT_ADDRESS_TYPES
#define DIABLOSUPPORT_ADDRESS_TYPES
struct _t_address
{
  /*! Stores the kind of address we are dealing with */
  t_address_type type;
  /*! A union to access the real address, depending on 'type' */
  union
  {
    t_uint32 addr32;
    t_uint64 addr64;
  } types;
};
#endif /* }}} Address Types */
/* Address Defines {{{ */
#ifndef DIABLOSUPPORT_ADDRESS_DEFINES
#define DIABLOSUPPORT_ADDRESS_DEFINES
#ifdef GENERICADDRSUPPORT
#define DiabloSupportInit DiabloSupportInitGeneric
#define AddressIs32(x) ((x).type==ADDRSIZE32)
#define AddressIs64(x) ((x).type==ADDRSIZE64)
#define AddressType(x) ((x).type)
#define AddressSize(x) (AddressIs32(x) ? 32 : 64 )
#define G_T_UINT32(x) (x.types.addr32)
#define G_T_UINT64(x) (x.types.addr64)
#define AddressNew32(x) RealAddressNew32(__FILE__,__LINE__,(x))
#define AddressNew64(x) RealAddressNew64(__FILE__,__LINE__,(x))
#define AddressAdd(u,v) RealAddressAdd(__FILE__,__LINE__,(u),(v))
#define AddressNot(u) RealAddressNot(__FILE__,__LINE__,(u))
#define AddressAnd(u,v) RealAddressAnd(__FILE__,__LINE__,(u),(v))
#define AddressOr(u,v) RealAddressOr(__FILE__,__LINE__,(u),(v))
#define AddressXor(u,v) RealAddressXor(__FILE__,__LINE__,(u),(v))
#define AddressShl(u,v) RealAddressShl(__FILE__,__LINE__,(u),(v))
#define AddressShr(u,v) RealAddressShr(__FILE__,__LINE__,(u),(v))
#define AddressAddUint32(u,v) RealAddressAddUint32(__FILE__,__LINE__,u,v)
#define AddressInverseMaskUint32(u,v) RealAddressInverseMaskUint32(__FILE__,__LINE__,u,v)
#define AddressAddInt32(u,v) RealAddressAddInt32(__FILE__,__LINE__,u,v)
#define AddressAddUint64(u,v) RealAddressAddUint64(__FILE__,__LINE__,u,v)
#define AddressAddInt64(u,v) RealAddressAddInt64(__FILE__,__LINE__,u,v)
#define AddressSubUint32(u,v) RealAddressSubUint32(__FILE__,__LINE__,u,v)
#define AddressSubInt32(u,v) RealAddressSubInt32(__FILE__,__LINE__,u,v)
#define AddressExtractUint32(x) RealAddressExtractUint32(__FILE__,__LINE__,(x))
#define AddressExtractUint64(x) RealAddressExtractUint64(__FILE__,__LINE__,(x))
#define AddressExtractInt32(x) RealAddressExtractInt32(__FILE__,__LINE__,(x))
#define AddressExtractInt64SignExtend(x) RealAddressExtractInt64SignExtend(__FILE__,__LINE__,(x))
#define AddressSub(u,v) RealAddressSub(__FILE__,__LINE__,(u),(v))
#define AddressIsNull(u) RealAddressIsNull(__FILE__,__LINE__,u)
#define AddressIsMax(u) RealAddressIsMax(__FILE__,__LINE__,u)
#define AddressIsLt(u,v) RealAddressIsLt(__FILE__,__LINE__,u,v)
#define AddressIsLe(u,v) RealAddressIsLe(__FILE__,__LINE__,u,v)
#define AddressIsGt(u,v) RealAddressIsGt(__FILE__,__LINE__,u,v)
#define AddressIsGe(u,v) RealAddressIsGe(__FILE__,__LINE__,u,v)
#define AddressIsEq(u,v) RealAddressIsEq(__FILE__,__LINE__,u,v)
#define AddressSignExtend(x,signbit) RealAddressSignExtend(__FILE__,__LINE__,x,signbit)
#define AddressAddDispl(addr,disp) RealAddressAddDispl(__FILE__,__LINE__,addr,disp)
#define AddressDivUint32(x,y) RealAddressDivUint32(__FILE__,__LINE__,(x),(y))
#define AddressMulUint32(x,y) RealAddressMulUint32(__FILE__,__LINE__,(x),(y))
#else
#ifdef BIT32ADDRSUPPORT
#define DiabloSupportInit DiabloSupportInit32
#define AddressIs32(x) TRUE
#define AddressIs64(x) FALSE
#define AddressType(x) ADDRSIZE32
#define AddressSize(x) 32
#define G_T_UINT32(x) (x)
#define AddressNew32(x) (x)
#define AddressIsNull(x) ((x)==(0))
#define AddressIsMax(x) ((x)==(0xFFFFFFFF))
#define AddressIsEq(x,y) ((x)==(y))
#define AddressIsLe(x,y) ((x)<=(y))
#define AddressIsLt(x,y) ((x)<(y))
#define AddressIsGt(x,y) ((x)>(y))
#define AddressIsGe(x,y) ((x)>=(y))
#define AddressAdd(x,y) ((x)+(y))
#define AddressAnd(x,y) ((x)&(y))
#define AddressOr(x,y) ((x)|(y))
#define AddressXor(x,y) ((x)^(y))
#define AddressSub(x,y) ((x)-(y))
#define AddressNot(x) (~(x))
#define AddressShl(x,y) ((x) << (y))
#define AddressShr(x,y) ((x) >> (y))
#define AddressAddUint32(x,y) ((x)+((t_uint32) y))
#define AddressAddUint64(x,y) ((x)+((t_uint64) y))
#define AddressInverseMaskUint32(x,y) ((x)&(~((t_uint32) y)))
#define AddressSubUint32(x,y) ((x)-((t_uint32) y))
#define AddressAddInt32(x,y) ((x)+((t_int32) y))
#define AddressSubInt32(x,y) ((x)-((t_int32) y))
#define AddressAddDispl(addr,disp) ((void *) (((char *) addr) + disp))
#define G_T_UINT64(x) (fprintf(stderr,"No 64-bit address support (%s:%d)\n",__FILE__,__LINE__), exit(-1), (t_uint64) 0)
#define AddressNew64(x) (fprintf(stderr,"No 64-bit address support (%s:%d)\n",__FILE__,__LINE__), exit(-1), (t_uint32) 0)
#define AddressDivUint32(x,y) ((x)/(y))
#define AddressMulUint32(x,y) ((x)*(y))
#define AddressExtractUint32(x) (x)
#define AddressExtractUint64(x) ((t_uint64)(x))
#define AddressExtractInt32(x) (t_int32)(x)
#define AddressExtractInt64SignExtend(x) ((t_int64)(t_int32)(x))
#define AddressSignExtend(x,signbit) ((x) ^ (1 << (signbit))) - (1 << (signbit))
#else
#ifdef BIT64ADDRSUPPORT
#define DiabloSupportInit DiabloSupportInit64
#define AddressIs32(x) FALSE
#define AddressIs64(x) TRUE
#define AddressType(x) ADDRSIZE64
#define AddressSize(x) 64
#define AddressNew64(x) (x)
#define G_T_UINT64(x) (x)
#define AddressIsNull(x) ((x)==ULL(0))
#define AddressIsMax(x) ((x)==ULL(0xFFFFFFFFFFFFFFFF))
#define AddressIsEq(x,y) ((x)==(y))
#define AddressIsLe(x,y) ((x)<=(y))
#define AddressIsLt(x,y) ((x)<(y))
#define AddressIsGt(x,y) ((x)>(y))
#define AddressIsGe(x,y) ((x)>=(y))
#define AddressAdd(x,y) ((x)+(y))
#define AddressAnd(x,y) ((x)&(y))
#define AddressOr(x,y) ((x)|(y))
#define AddressXor(x,y) ((x)^(y))
#define AddressSub(x,y) ((x)-(y))
#define AddressNot(x) (~(x))
#define AddressShl(x,y) ((x) << (y))
#define AddressShr(x,y) ((x) >> (y))
#define AddressAddUint32(x,y) ((x)+((t_uint32) y))
#define AddressAddUint64(x,y) ((x)+((t_uint64) y))
#define AddressInverseMaskUint32(x,y) ((x)&(~((t_uint64) y)))
#define AddressSubUint32(x,y) ((x)-((t_uint32) y))
#define AddressAddInt32(x,y) ((x)+((t_int32) y))
#define AddressAddInt64(x,y) ((x)+((t_int64) y))
#define AddressSubInt32(x,y) ((x)-((t_int32) y))
#define AddressAddDispl(addr,disp) ((void *) (((char *) addr) + disp))
#define G_T_UINT32(x) (fprintf(stderr,"No 32-bit address support (%s:%d)\n",__FILE__,__LINE__), exit(-1), (t_uint32) 0)
#define AddressNew32(x) (fprintf(stderr,"No 32-bit address support (%s:%d)\n",__FILE__,__LINE__), exit(-1), (t_uint64) 0)
#define AddressDivUint32(x,y) ((x)/((t_uint64) (y)))
#define AddressMulUint32(x,y) ((x)*((t_uint64) (y)))
#define AddressExtractUint32(x) ((t_uint32) x)
#define AddressExtractUint64(x) (x)
#define AddressExtractInt32(x) (t_int32) ((t_uint32) x)
#define AddressExtractInt64SignExtend(x) ((t_int64)(x))
#define AddressSignExtend(x,signbit) ((x) ^ (1 << (signbit))) - (1 << (signbit))
#endif
#endif
#endif
/* Leave the \ at the end of the line: it fixes a bug in vim indentation */
#define AddressAlign(align, start) ((((AddressExtractUint32 (align))) && (((AddressExtractUint32 (start) % (AddressExtractUint32 (align))))))?AddressAddUint32 (start, (AddressExtractUint32 (align)) - ((AddressExtractUint32 (start) % (AddressExtractUint32 (align))))):\
                                    start)
/*! Generate a new address of type 't' with 'd' as contents */
#ifdef _MSC_VER
inline t_address AddressNewTyped(t_address d, t_uint32 type);
inline t_uint32  WordAlignedSize(t_uint32 s);
#else
#define AddressNewTyped(d,t)                    \
  ({                                            \
   t_address res;                               \
   if (t == ADDRSIZE32)                         \
     res = AddressNew32(d);                     \
   else if (t == ADDRSIZE64)                    \
     res = AddressNew64(d);                     \
   else                                         \
     FATAL(("Unknown address type (%d)", t));   \
   res;                                         \
   })
#define WordAlignedSize(s)                      \
  ({                                            \
   t_uint32 res;                                \
                                                \
   if ((s) <= 8)                                \
      res = 8;                                  \
   else if ((s) <= 16)                          \
      res = 16;                                 \
   else if ((s) <= 32)                          \
      res = 32;                                 \
   else if ((s) <= 64)                          \
      res = 64;                                 \
   else                                         \
    FATAL(("Unsupported size: %d", s));         \
   res;                                         \
   })
#endif

/*! Build a bit mask of size 's', as an address type 't' */
#define AddressBitMask(s,t)         (AddressSubUint32 (AddressShl (AddressNewTyped (0x1, t), AddressNewTyped (s, t)), 1))
/*! Build a bit mask of size 's', left offsetted 'f' bits, as an address type 't' ('s' ones and then 'f' zeros) */
#define AddressBitMaskOffset(s,f,t) (AddressAnd (AddressBitMask (s+f, t), AddressNot (AddressBitMask (f, t))))
/*! Reset the last 's' bits (offseted 'f' bits) from 'd' */
#define AddressBitReset(d,s,f)      (AddressAnd (d, AddressNot (AddressBitMaskOffset (s, f, AddressType(d)))))
/*! Create a new address with just 's' LSB from 'd', but shifted 'f' bits left */
#define AddressBitMove(d,s,f)       (AddressAnd (AddressShl (d, AddressNewTyped (f, AddressType(d))), \
                                                 AddressBitMaskOffset (s, f, AddressType(d))))
#endif
/* }}} */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* Address Functions {{{ */
#ifndef DIABLOSUPPORT_ADDRESS_FUNCTIONS
#define DIABLOSUPPORT_ADDRESS_FUNCTIONS
t_string IoModifierAddress32 (t_const_string modifiers, va_list * ap);
t_string IoModifierAddress64 (t_const_string modifiers, va_list * ap);
t_string IoModifierAddressGeneric (t_const_string modifiers, va_list * ap);

t_address_generic RealAddressNew32 (t_const_string, t_uint32, t_uint32);
t_address_generic RealAddressNew64 (t_const_string, t_uint32, t_uint64);
t_uint32 RealAddressExtractUint32 (t_const_string, int, t_address_generic);
t_uint64 RealAddressExtractUint64 (t_const_string, int, t_address_generic);
t_int32 RealAddressExtractInt32 (t_const_string, int, t_address_generic);
t_int64 RealAddressExtractInt64SignExtend(t_const_string, int, t_address_generic);
t_address_generic RealAddressAdd (t_const_string, t_uint32, t_address_generic, t_address_generic);
t_address_generic RealAddressNot (t_const_string x, t_uint32 lnno, t_address_generic a);
t_address_generic RealAddressAnd (t_const_string x, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_address_generic RealAddressOr (t_const_string x, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_address_generic RealAddressXor (t_const_string x, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_address_generic RealAddressShl (t_const_string x, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_address_generic RealAddressShr (t_const_string x, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_address_generic RealAddressAddUint32 (t_const_string file, t_uint32 lnno, t_address_generic a, t_uint32 b);
t_address_generic RealAddressInverseMaskUint32 (t_const_string file, t_uint32 lnno, t_address_generic a, t_uint32 b);
t_address_generic RealAddressAddInt32 (t_const_string file, t_uint32 lnno, t_address_generic a, t_int32 b);
t_address_generic RealAddressAddUint64 (t_const_string file, t_uint32 lnno, t_address_generic a, t_uint64 b);
t_address_generic RealAddressAddInt64 (t_const_string file, t_uint32 lnno, t_address_generic a, t_int64 b);
t_address_generic RealAddressSubUint32 (t_const_string file, t_uint32 lnno, t_address_generic a, t_uint32 b);
t_address_generic RealAddressSubInt32 (t_const_string file, t_uint32 lnno, t_address_generic a, t_int32 b);
t_address_generic RealAddressSub (t_const_string file, t_uint32 line, t_address_generic a, t_address_generic b);
t_bool RealAddressIsLt (t_const_string file, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_bool RealAddressIsLe (t_const_string file, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_bool RealAddressIsGt (t_const_string file, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_bool RealAddressIsGe (t_const_string file, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_bool RealAddressIsEq (t_const_string file, t_uint32 lnno, t_address_generic a, t_address_generic b);
t_bool RealAddressIsNull (t_const_string file, t_uint32 line, t_address_generic a);
t_bool RealAddressIsMax (t_const_string file, t_uint32 line, t_address_generic a);
t_address_generic RealAddressSignExtend (t_const_string file, t_uint32 lnno, t_address_generic x, t_uint32 signbit);
void *RealAddressAddDispl (t_const_string file, t_uint32 lnno, void *in, t_address_generic addr);
t_address_generic RealAddressDivUint32 (t_const_string file, t_uint32 line, t_address_generic address, t_uint32 div);
t_address_generic RealAddressMulUint32 (t_const_string file, t_uint32 line, t_address_generic address, t_uint32 div);
#endif /* Address Functions }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
