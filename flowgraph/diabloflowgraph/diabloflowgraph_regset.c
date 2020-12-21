/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>

t_register_type RegisterGetType(t_architecture_description desc, t_reg reg)
{
  if (RegsetIn(desc.int_registers, reg))
    return REG_TYPE_INT;

  if (RegsetIn(desc.flt_registers, reg))
    return REG_TYPE_FP;

  if (RegsetIn(desc.cond_registers, reg))
    return REG_TYPE_FLAG;

  FATAL(("Type of register could not be determined!"));
}

#if MAX_REG_ITERATOR > 64

t_regset NullRegs = { MAX_REG_ITERATOR, {0} };

t_uint32 RegsetToUint32 (t_regset rs)
{
  t_uint32 ret = 0;
  t_reg r;
  REGSET_FOREACH_REG (rs, r)
  {
    if (r < 32)
      ret |= 1U << r;
    else
      break;
  }
  return ret;
}

t_uint64 RegsetToUint64 (t_regset rs)
{
  t_uint64 ret = 0;
  t_reg r;
  REGSET_FOREACH_REG (rs, r)
  {
    if (r < 64)
      ret |= (t_uint64)1 << r;
    else
      break;
  }
  return ret;
}

t_regset RegsetNewFromUint32 (t_uint32 x)
{
  t_regset ret = RegsetNew();
  t_reg r;
  for (r = 0; r < 32; ++r)
  {
    if (x & ((t_uint32)1 << r))
      RegsetSetAddReg (ret, r);
  }

  return ret;
}

t_regset RegsetNewFromUint64 (t_uint64 x)
{
  t_regset ret = RegsetNew();
  t_reg r;
  for (r = 0; r < 32; ++r)
    if (x & (t_uint64)(1 << r))
      RegsetSetAddReg (ret, r);
  
  return ret;
}

#if 0
t_regset
register_subset (t_uint16 index)
{
  t_regset ret = { MAX_REG_ITERATOR, {0} };

  if (index >= (MAX_NR_REG_SUBSETS))
    FATAL(("Registerindex out of range! (%d >= %d)", index, MAX_NR_REG_SUBSETS));
  ret.regset[index] = ~0;
  return ret;
}
#endif

t_regset
RegsetNewInvers (t_regset x, t_regset max)
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x.regset[i] = (~(x.regset[i])) & (max.regset[i]);
  return x;
}

t_regset
RegsetUnion (t_regset x, t_regset y)
{
  t_uint16 i;
  t_regset ret;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    ret.regset[i] = (x.regset[i]) | (y.regset[i]);
  return ret;
}

t_regset
RegsetIntersect (t_regset x, t_regset y)
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x.regset[i] = (x.regset[i]) & (y.regset[i]);
  return x;
}

t_regset
RegsetDiff (t_regset x, t_regset y)
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x.regset[i] = (x.regset[i]) & (~(y.regset[i]));
  return x;
}

void
real_RegsetSetInvers (t_regset * x)
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] = ~(x->regset[i]);
  return;
}

#if 0
t_regset
real_RegsetAddReg (t_regset r, t_reg reg)
{
  t_uint16 i = reg / REG_SUBSET_SIZE;

  r.regset[i] |= 1U << (reg & (REG_SUBSET_SIZE - 1));
  return r;
}

void
real_RegsetSetAddReg (t_regset *r, t_reg reg)
{
  t_uint16 i = reg / REG_SUBSET_SIZE;

  r->regset[i] |= 1U << (reg & (REG_SUBSET_SIZE - 1));
}
#endif

/** \internal Should not be used, this is a remainder of an existing anomaly */
void
real_RegsetSetAddMultipleRegs (t_regset * r, t_reg reg)
{
  t_uint16 i = sizeof (t_reg) * 8;
  t_reg j;


  for (j = 0; j < i; j++)
  {
    if (reg & 0x1)
      real_RegsetSetAddReg (r, j);
    reg >>= 1;
  }
  return;
}

#if 0
void
real_RegsetSetSingleton (t_regset * r, t_uint16 i)
{
  *r = NullRegs;
  r->regset[i >> REG_SUBSET_SIZE_SHIFT] = 1U << (i & (REG_SUBSET_SIZE - 1));
  return;
}

void
real_RegsetSetSubReg (t_regset * r, t_uint16 i)
{
  r->regset[i >> REG_SUBSET_SIZE_SHIFT] &= ~(1U << (i & (REG_SUBSET_SIZE - 1)));
  return;
}

void
real_RegsetSetUnion (t_regset * x, t_regset y) /*(x)|=(y) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] |= y.regset[i];
  return;
}

void
real_RegsetSetIntersect (t_regset * x, t_regset y) /*(x)&=(y) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] &= y.regset[i];
  return;
}

void
real_RegsetSetDiff (t_regset * x, t_regset y) /*(x)&=(~(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] &= ~(y.regset[i]);
  return;
}

t_regset
real_RegsetDiff (t_regset x, t_regset y) /*(x)&=(~(y)) */
{
  t_regset z = x;
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    z.regset[i] = (x.regset[i] & ~(y.regset[i]));
  return z;
}


void
real_RegsetSetMutualExclusive (t_regset * x, t_regset y) /*(x)=((x)&~(y))|((~x)&(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] = ((x->regset[i]) & ~(y.regset[i])) | (~(x->regset[i]) & (y.regset[i]));
  return;
}
#endif

t_bool
RegsetIsMutualExclusive (t_regset x, t_regset y) /*(!((x)&(y))) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (x.regset[i] & y.regset[i])
      return FALSE;
  return TRUE;
}

#if 0
t_bool
RegsetIsEmpty (t_regset r) /*((r)==NullRegs) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (r.regset[i])
      return FALSE;
  return TRUE;
}

t_bool
RegsetEquals (t_regset x, t_regset y) /* ((x)==(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (x.regset[i] != y.regset[i])
      return FALSE;
  return TRUE;
}
#endif

#if 0
t_bool
RegsetIn (t_regset x, t_reg y) /*((x)&(1<<y)) */
{
  if (x.regset[y >> REG_SUBSET_SIZE_SHIFT] & (1 << (y & (REG_SUBSET_SIZE-1))))
    return TRUE;
  return FALSE;
}

t_bool
RegsetIsSubset (t_regset x, t_regset y) /* (!((~(x))&y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (~(x.regset[i]) & y.regset[i])
      return FALSE;
  return TRUE;
}
#endif

/** \internal Used in iterator */
t_regset
RegsetNextSingleton (t_regset x)
{
  t_regset ret = NullRegs;
  t_uint16 i = 0;

  while (!x.regset[i])
    i++;
  if (x.regset[i] & (1U << (REG_SUBSET_SIZE - 1)))
    ret.regset[i + 1] = 0x1;
  else
    ret.regset[i] = x.regset[i] << 1;
  return ret;
}
#endif

/** count the number of registers in the set */
t_uint32
RegsetCountRegs (t_regset r)
{
  t_uint32 count = 0;

  int i;
  uint32_t x;
  REGSET_FOREACH_SUBSET(r, x, i)
    count += RegSubsetCountBits(x);

  return count;
}

/** Find the first 'blob' of consecutive ones in the regset.
 * return the location, the length of the blob is saved in *size.
 */
t_reg
RegsetFindFirstBlob (t_regset r, t_uint32 * size)
{
  t_uint32 count = 0;
  t_bool is_location = FALSE;
  t_reg location = 0;
  t_reg i;

  REGSET_FOREACH_REG(r, i)
  {
    location = (is_location) ? location : i;
    is_location = TRUE;

    count++;
  }

  *size = count;
  return location;
}

t_string RegsetSerialize (t_regset r) {
  t_string result = StringIo("0x%08x", r.regset[0]);
  for (int i = 1; i < MAX_NR_REG_SUBSETS; i++) {
    t_string tmp = result;

    t_string t = StringIo(",0x%08x", r.regset[i]);
    result = StringConcat2(tmp, t);
    Free(t);
    Free(tmp);
  }

  return result;
}

t_regset RegsetDeserialize (t_string s) {
  t_string_array *arr = StringDivide(s, ",", FALSE, FALSE);

  t_regset result = RegsetNew();

  t_string_array_elem *iter;
  int i = 0;
  STRING_ARRAY_FOREACH_ELEM(arr, iter) {
    result.regset[i] = StringToUint32(iter->string, strlen(iter->string));
    i++;
  }

  StringArrayFree(arr);

  return result;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
