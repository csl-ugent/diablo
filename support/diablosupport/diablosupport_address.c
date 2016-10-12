/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>

t_string
IoModifierAddressGeneric (t_const_string modifiers, va_list * ap)
{
  char buffer[20];
  t_string ret;
  t_string_array *array;
  t_address_generic addr = va_arg (*ap, t_address_generic);

  array = StringArrayNew ();
  if (addr.type == ADDRSIZE32)
    sprintf (buffer, "0x%x", addr.types.addr32);
  else if (addr.type == ADDRSIZE64)
    sprintf (buffer, "0x%"PRIx64"", addr.types.addr64);
  else
    FATAL(("Address not correctly initialized!"));

  StringArrayAppendString (array, StringDup (buffer));

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

t_string
IoModifierAddress32 (t_const_string modifiers, va_list * ap)
{
  char buffer[20];
  t_string ret;
  t_string_array *array;
  t_uint32 addr = va_arg (*ap, t_uint32);

  array = StringArrayNew ();
  sprintf (buffer, "0x%x", addr);

  StringArrayAppendString (array, StringDup (buffer));

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

t_string
IoModifierAddress64 (t_const_string modifiers, va_list * ap)
{
  char buffer[20];
  t_string ret;
  t_string_array *array;
  t_uint64 addr = va_arg (*ap, t_uint64);

  array = StringArrayNew ();
  sprintf (buffer, "0x%"PRIx64"", addr);

  StringArrayAppendString (array, StringDup (buffer));

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

t_address_generic
RealAddressNew32 (t_const_string file, t_uint32 line, t_uint32 a)
{
  t_address_generic ret;

  ret.type = ADDRSIZE32;
  ret.types.addr32 = a;
  return ret;
}

t_address_generic
RealAddressNew64 (t_const_string file, t_uint32 line, t_uint64 a)
{
  t_address_generic ret;

  ret.type = ADDRSIZE64;
  ret.types.addr64 = a;
  return ret;
}

t_address_generic
RealAddressAdd (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s %d %d", line,
          file, a.type, b.type));

  ret.type = b.type;

  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 + b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 + b.types.addr64;
  return ret;
}

t_address_generic
RealAddressAnd (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));

  ret.type = b.type;
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 & b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 & b.types.addr64;
  return ret;
}

t_address_generic
RealAddressOr (t_const_string file, t_uint32 line, t_address_generic a,
               t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));

  ret.type = a.type;
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 | b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 | b.types.addr64;
  return ret;
}

t_address_generic
RealAddressXor (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));

  ret.type = a.type;
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 ^ b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 ^ b.types.addr64;
  return ret;
}

t_address_generic
RealAddressNot (t_const_string file, t_uint32 line, t_address_generic a)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = ~a.types.addr32;
  else
    ret.types.addr64 = ~a.types.addr64;
  return ret;
}

t_address_generic
RealAddressShl (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));

  ret.type = b.type;
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 << b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 << b.types.addr64;
  return ret;
}

t_address_generic
RealAddressShr (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));

  ret.type = b.type;
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
    ret.types.addr32 = a.types.addr32 >> b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 >> b.types.addr64;
  return ret;
}

t_address_generic
RealAddressAddInt32 (t_const_string file, t_uint32 line, t_address_generic a,
                     t_int32 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 + b;
  else
    ret.types.addr64 = a.types.addr64 + b;
  return ret;
}

t_address_generic
RealAddressAddUint64 (t_const_string file, t_uint32 line, t_address_generic a,
                      t_uint64 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 + (t_uint32)b;
  else
    ret.types.addr64 = a.types.addr64 + b;
  return ret;
}

t_address_generic
RealAddressAddUint32 (t_const_string file, t_uint32 line, t_address_generic a,
                      t_uint32 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 + b;
  else
    ret.types.addr64 = a.types.addr64 + b;
  return ret;
}

t_address_generic
RealAddressInverseMaskUint32 (t_const_string file, t_uint32 line, t_address_generic a,
                              t_uint32 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 & (~((t_uint32) (b)));
  else
    ret.types.addr64 = a.types.addr64 & (~((t_uint64) (b)));
  return ret;
}

t_address_generic
RealAddressAddInt64 (t_const_string file, t_uint32 line, t_address_generic a,
                     t_int64 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 + (t_int32)b;
  else
    ret.types.addr64 = a.types.addr64 + b;
  return ret;
}

t_address_generic
RealAddressSubUint32 (t_const_string file, t_uint32 line, t_address_generic a,
                      t_uint32 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 - b;
  else
    ret.types.addr64 = a.types.addr64 - b;
  return ret;
}

t_address_generic
RealAddressSubInt32 (t_const_string file, t_uint32 line, t_address_generic a,
                     t_int32 b)
{
  t_address_generic ret;

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 - b;
  else
    ret.types.addr64 = a.types.addr64 - b;
  return ret;
}

t_address_generic
RealAddressSub (t_const_string file, t_uint32 line, t_address_generic a,
                t_address_generic b)
{
  t_address_generic ret;

  ASSERT((a.type == b.type),
         ("Sub'ed different size addresses at line %d in %s", line, file));

  ret.type = a.type;
  if (a.type == ADDRSIZE32)
    ret.types.addr32 = a.types.addr32 - b.types.addr32;
  else
    ret.types.addr64 = a.types.addr64 - b.types.addr64;
  return ret;
}

t_bool
RealAddressIsNull (t_const_string file, t_uint32 line, t_address_generic a)
{
  if (a.type == ADDRSIZE32)
    return a.types.addr32 == 0;
  else
    return a.types.addr64 == ULL(0);
}

t_bool
RealAddressIsMax (t_const_string file, t_uint32 line, t_address_generic a)
{
  if (a.type == ADDRSIZE32)
    return a.types.addr32 == 0xFFFFFFFF;
  else
    return a.types.addr64 == ULL(0xFFFFFFFFFFFFFFFF);
}

t_bool
RealAddressIsGt (t_const_string file, t_uint32 line, t_address_generic a,
                 t_address_generic b)
{
  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));
  if (a.type == ADDRSIZE32)
    return a.types.addr32 > b.types.addr32;
  else
    return a.types.addr64 > b.types.addr64;
}

t_bool
RealAddressIsGe (t_const_string file, t_uint32 line, t_address_generic a,
                 t_address_generic b)
{
  ASSERT((a.type == b.type), ("Added different size addresses"));
  if (a.type == ADDRSIZE32)
    return a.types.addr32 >= b.types.addr32;
  else
    return a.types.addr64 >= b.types.addr64;
}

t_bool
RealAddressIsLt (t_const_string file, t_uint32 line, t_address_generic a,
                 t_address_generic b)
{
  ASSERT((a.type == b.type),
         ("Added different size addresses at line %d in %s", line, file));
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
  {
    return a.types.addr32 < b.types.addr32;
  }
  else
  {
    return a.types.addr64 < b.types.addr64;
  }
}

t_bool
RealAddressIsLe (t_const_string file, t_uint32 line, t_address_generic a,
                 t_address_generic b)
{
  ASSERT((a.type == b.type), ("Added different size addresses"));
  if ((a.type == ADDRSIZE32) || (b.type == ADDRSIZE32))
  {
    return a.types.addr32 <= b.types.addr32;
  }
  else
  {
    return a.types.addr64 <= b.types.addr64;
  }
}

t_bool
RealAddressIsEq (t_const_string file, t_uint32 line, t_address_generic a,
                 t_address_generic b)
{
  ASSERT((a.type == b.type),
         ("AddressEQ at line %d in %s has different type operands", line,
          file));
  if (a.type == ADDRSIZE32)
    return a.types.addr32 == b.types.addr32;
  else
    return a.types.addr64 == b.types.addr64;
}

void *
RealAddressAddDispl (t_const_string file, t_uint32 line, void *in,
                     t_address_generic addr)
{
  if (addr.type == ADDRSIZE32)
  {
    return ((void *) (((char *) in) + addr.types.addr32));
  }
  else
  {
    return ((void *) (((char *) in) + addr.types.addr64));
  }
}

t_address_generic
RealAddressMulUint32 (t_const_string file, t_uint32 line, t_address_generic address,
                      t_uint32 div)
{
  t_address_generic ret;

  ret.type = address.type;
  if (address.type == ADDRSIZE32)
  {
    ret.types.addr32 = address.types.addr32 * div;
  }
  else if (address.type == ADDRSIZE64)
  {
    ret.types.addr64 = address.types.addr64 * ((t_uint64) div);
  }
  else
  {
    FATAL(("Address is corrupt (called at file %s, line %d) ! %d", file,
           line, address.type));
  }
  return ret;
}

t_address_generic
RealAddressDivUint32 (t_const_string file, t_uint32 line, t_address_generic address,
                      t_uint32 div)
{
  t_address_generic ret;

  ret.type = address.type;
  if (address.type == ADDRSIZE32)
  {
    ret.types.addr32 = address.types.addr32 / div;
  }
  else if (address.type == ADDRSIZE64)
  {
    ret.types.addr64 = address.types.addr64 / ((t_uint64) div);
  }
  else
  {
    FATAL(("Address is corrupt (called at file %s, line %d) ! %d", file,
           line, address.type));
  }
  return ret;
}

t_uint32
RealAddressExtractUint32 (t_const_string file, int line, t_address_generic addr)
{
  if (addr.type == ADDRSIZE32)
  {
    return addr.types.addr32;
  }
  if (addr.types.addr64 > 0xffffffffull)
  {
    /* check for overflow: assume there is no overflow if it is a negative
     * number that also fits in 32 bits */
    if ((addr.types.addr64 & 0xffffffff80000000ull) != 0xffffffff80000000ull)
      FATAL(("Overflow at line %d in file %s val = %"PRIx64"\n", line, file, addr.types.addr64));
  }
  return (t_uint32) addr.types.addr64;
}

t_uint64
RealAddressExtractUint64 (t_const_string file, int line, t_address_generic addr)
{
  if (addr.type == ADDRSIZE32)
  {
    return (t_uint64) addr.types.addr32;
  }
  return addr.types.addr64;
}

t_int32
RealAddressExtractInt32 (t_const_string file, int line, t_address_generic addr)
{
  if (addr.type == ADDRSIZE32)
  {
    return (t_int32) addr.types.addr32;
  }
  if (addr.types.addr64 > 0xffffffffull)
  {
    if ((addr.types.addr64 & 0xffffffff80000000ull) !=
        0xffffffff80000000ull)
      FATAL(("Overflow at line %d in file %s\n", line, file));
    else
      /* negative number */
      return (t_int32) ((t_int64) addr.types.addr64);
  }
  return (t_int32) ((t_uint32) addr.types.addr64);
}

t_int64
RealAddressExtractInt64SignExtend (t_const_string file, int line, t_address_generic addr)
{
  if (addr.type == ADDRSIZE32)
  {
    return (t_int32) addr.types.addr32;
  }
  else if (addr.type == ADDRSIZE64)
  {
    return (t_int64) (addr.types.addr64);
  }
  else
  {
    FATAL(("address is corrupt"));
  }
}

t_address_generic
RealAddressSignExtend (t_const_string file, t_uint32 line, t_address_generic x,
                       t_uint32 signbit)
{
  if (x.type == ADDRSIZE32)
  {
    x.types.addr32 = (x.types.addr32 ^ (1 << signbit)) - (1 << signbit);
  }
  else if (x.type == ADDRSIZE64)
  {
    x.types.addr64 = (x.types.addr64 ^ (1 << signbit)) - (1 << signbit);
  }
  else
  {
    FATAL(("address is corrupt"));
  }

  return x;
}

#ifdef _MSC_VER
inline t_address
AddressNewTyped(t_address d, t_uint32 type)
{ 
  t_address res;
  if (type == ADDRSIZE32)
    res = AddressNew32(d);
  else if (type == ADDRSIZE64)
    res = AddressNew64(d);
  else
    FATAL(("Unknown address type (%d)", type));
  return res;   
}

inline t_uint32
WordAlignedSize(t_uint32 s)
{
  t_uint32 res;
  if ((s) <= 8)
    res = 8;
  else if ((s) <= 16)
    res = 16;
  else if ((s) <= 32)
    res = 32;
  else if ((s) <= 64)
    res = 64;
  else
    FATAL(("Unsupported size: %d", s));
  return res;
}
#endif

t_uint32 AddressSizeInBytes(t_address_type address_type)
{
  return (address_type == ADDRSIZE32) ? 4 : 8;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
