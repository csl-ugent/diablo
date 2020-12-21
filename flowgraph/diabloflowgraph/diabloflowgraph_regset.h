/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*!\addtogroup DIABLO_DATATYPES */
/*@{ */
/*! \defgroup REGSETS Register sets
 *
 * Register sets are used in all kinds of analyses and optimisation: They are used to represent a set of registers */
/*@{ */
#ifndef REGSET_TYPEDEFS
#define REGSET_TYPEDEFS

#include <diablosupport.h>
#ifndef MAX_NR_REG_SUBSETS
#define MAX_NR_REG_SUBSETS 3
#endif
#ifndef REG_SUBSET_SIZE
#define REG_SUBSET_SIZE 32
#define REG_SUBSET_SIZE_SHIFT 5
#endif

#define DPREGSET(desc, regset) desc,regset
#define CPREGSET(cfg, regset) CFG_DESCRIPTION(cfg),regset
#define REG_NONE (-1)

#define MAX_REG_ITERATOR (MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE)

#if REG_SUBSET_SIZE == 32
#define RegSubsetCountBits(x) CountSetBits32(x)
#elif REG_SUBSET_SIZE == 64
#define RegSubsetCountBits(x) CountSetBits64(x)
#else
#error "implement me"
#endif

#if MAX_REG_ITERATOR > 64

typedef struct _t_regset t_regset, renamed_t_regset;
typedef t_int16 t_reg, renamed_t_reg;

#elif MAX_REG_ITERATOR > 32
typedef t_uint64 t_regset, renamed_t_regset;
typedef t_int16 t_reg, renamed_t_reg;
#else
typedef t_uint32 t_regset, renamed_t_regset;
typedef t_int16 t_reg, renamed_t_reg;
#endif

typedef enum
{
  REG_TYPE_INT,/* Integer register */
  REG_TYPE_FP,/* Floating point register */
  REG_TYPE_FLAG/* Flag */
} t_register_type;

t_register_type RegisterGetType(t_architecture_description desc, t_reg reg);

#endif /* REGSET_TYPEDEFS*/

#ifndef REGSET_HEADER
#define REGSET_HEADER

/* Regsets > 64 {{{ */
#if MAX_REG_ITERATOR > 64

struct _t_regset
{
  t_uint16 max_reg;
  t_uint32 regset[MAX_REG_ITERATOR / 32]; /* TODO: define REGS_IN_SUBSET for efficiency? */  
};
#define REGSET_FOREACH_SUBSET(r, s, i) for (i = 0, s = r.regset[0]; (i < (MAX_REG_ITERATOR / 32)) ? (s = r.regset[i], TRUE): FALSE; i++)

static inline t_uint32
register_get_subset(t_regset regs, t_uint16 index) {
  return regs.regset[index];
}

extern t_regset NullRegs;

//t_regset register_subset (t_uint16 index);
static inline t_regset
register_subset (t_uint16 index)
{
  t_regset ret = { MAX_REG_ITERATOR, {0} };

  if (index >= (MAX_NR_REG_SUBSETS))
    FATAL(("Registerindex out of range! (%d >= %d)", index, MAX_NR_REG_SUBSETS));
  ret.regset[index] = ~0;
  return ret;
}

/* constructor and destructor */
#define RegsetNew() NullRegs
t_uint32 RegsetToUint32 (t_regset rs);
t_uint64 RegsetToUint64 (t_regset rs);
t_regset RegsetNewFromUint32 (t_uint32 x);
t_regset RegsetNewFromUint64 (t_uint64 x);
t_regset RegsetNewInvers (t_regset x, t_regset max);
t_regset RegsetUnion (t_regset x, t_regset y);
t_regset RegsetIntersect (t_regset x, t_regset y);
t_regset RegsetDiff (t_regset x, t_regset y);

#define RegsetDup(x) (x)

#define RegsetFree(r) while(0)

/* regset manipulations with assignments */

#define RegsetSetEmpty(r) r=NullRegs
#define RegsetEmpty() NullRegs
#define RegsetSetDup(x,y) (x)=(y)
#define RegsetSetInvers(x) real_RegsetSetInvers(&(x))
void real_RegsetSetInvers (t_regset * x);

#define RegsetSetAddReg(r,i) real_RegsetSetAddReg(&(r),i)
#define RegsetAddReg(r,i) real_RegsetAddReg(r,i)
//void real_RegsetSetAddReg (t_regset * r, t_reg i);
//t_regset real_RegsetAddReg (t_regset r, t_reg i);

static inline void
real_RegsetSetAddReg (t_regset *r, t_reg reg)
{
  t_uint16 i = reg / REG_SUBSET_SIZE;

  r->regset[i] |= 1U << (reg & (REG_SUBSET_SIZE - 1));
}

static inline t_regset
real_RegsetAddReg (t_regset r, t_reg reg)
{
  t_uint16 i = reg / REG_SUBSET_SIZE;

  r.regset[i] |= 1U << (reg & (REG_SUBSET_SIZE - 1));
  return r;
}


/** \internal Should not be used, this is a remainder of an existing anomaly in ARM code*/
#define RegsetSetAddMultipleRegs(r,i) real_RegsetSetAddMultipleRegs(&(r),i)
void real_RegsetSetAddMultipleRegs (t_regset * r, t_reg reg);
#define RegsetSetSingleton(r,i) real_RegsetSetSingleton(&(r),i)
static inline void
real_RegsetSetSingleton (t_regset * r, t_uint16 i)
{
  *r = NullRegs;
  r->regset[i >> REG_SUBSET_SIZE_SHIFT] = 1U << (i & (REG_SUBSET_SIZE - 1));
  return;
}
//void real_RegsetSetSingleton (t_regset * r, t_uint16 i);

#define RegsetSetSubReg(r,i) real_RegsetSetSubReg(&(r),i)
#define RegsetSubReg(r,i) real_RegsetSubReg(r,i)
//void real_RegsetSetSubReg (t_regset * r, t_uint16 i);
static inline void
real_RegsetSetSubReg (t_regset * r, t_uint16 i)
{
  r->regset[i >> REG_SUBSET_SIZE_SHIFT] &= ~(1U << (i & (REG_SUBSET_SIZE - 1)));
  return;
}

void real_RegsetSubReg (t_regset r, t_uint16 i);

#define RegsetSetUnion(x,y) real_RegsetSetUnion(&(x),y)
static inline void
real_RegsetSetUnion (t_regset * x, t_regset y) /*(x)|=(y) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] |= y.regset[i];
  return;
}

//void real_RegsetSetUnion (t_regset * x, t_regset y);

#define RegsetUnion(x,y) real_RegsetUnion(x,y)
t_regset real_RegsetUnion (t_regset x, t_regset y);

#define RegsetSetIntersect(x,y) real_RegsetSetIntersect(&(x),y)
static inline void
real_RegsetSetIntersect (t_regset * x, t_regset y) /*(x)&=(y) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] &= y.regset[i];
  return;
}


//void real_RegsetSetIntersect (t_regset * x, t_regset y);


#define RegsetSetDiff(x,y) real_RegsetSetDiff(&(x),y)
//void real_RegsetSetDiff (t_regset * x, t_regset y);
static inline void
real_RegsetSetDiff (t_regset * x, t_regset y) /*(x)&=(~(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] &= ~(y.regset[i]);
  return;
}



#define RegsetSetMutualExclusive(x,y) real_RegsetSetMutualExclusive(&(x),y)
//void real_RegsetSetMutualExclusive (t_regset * x, t_regset y);
static inline void
real_RegsetSetMutualExclusive (t_regset * x, t_regset y) /*(x)=((x)&~(y))|((~x)&(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    x->regset[i] = ((x->regset[i]) & ~(y.regset[i])) | (~(x->regset[i]) & (y.regset[i]));
  return;
}

t_uint32 RegsetCountRegs (t_regset r);

t_reg RegsetFindFirstBlob (t_regset r, t_uint32 * size);

/* regset tests */

t_bool RegsetIsMutualExclusive (t_regset x, t_regset y); /*(!((x)&(y))) */
//t_bool RegsetIsEmpty (t_regset r); /*((r)==NullRegs) */
static inline t_bool
RegsetIsEmpty (t_regset r) /*((r)==NullRegs) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (r.regset[i])
      return FALSE;
  return TRUE;
}


//t_bool RegsetEquals (t_regset x, t_regset y); /* ((x)==(y)) */
static inline t_bool
RegsetEquals (t_regset x, t_regset y) /* ((x)==(y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (x.regset[i] != y.regset[i])
      return FALSE;
  return TRUE;
}
//t_bool RegsetIn (t_regset x, t_reg y); /*((x)&(1<<y)) */

#define RegsetIn(x, y) ((x).regset[(y) >> REG_SUBSET_SIZE_SHIFT] & (ULL(1) << ((y) & (REG_SUBSET_SIZE-1))))

static inline t_bool
RegsetIsSubset (t_regset x, t_regset y) /* (!((~(x))&y)) */
{
  t_uint16 i;

  for (i = 0; i < MAX_NR_REG_SUBSETS; i++)
    if (~(x.regset[i]) & y.regset[i])
      return FALSE;
  return TRUE;
}

//t_bool RegsetIsSubset (t_regset x, t_regset y); /* (!((~(x))&y)) */

/* regset iterators */

t_regset RegsetNextSingleton (t_regset x);

#define REGSET_FOREACH_REG(r,i) for (i=LL(0);i<MAX_REG_ITERATOR;i++) if (RegsetIn(r,i))
#define REGSET_FOREACH_REG_INVERSE(r,i) for (i=(MAX_REG_ITERATOR-1);i>=0 && i<MAX_REG_ITERATOR;i--) if (RegsetIn(r,i))
#define REGSET_FOREACH_SINGLETON_REGSET(r,i,j) for (i=LL(0),RegsetSetSingleton(j,0);i<MAX_REG_ITERATOR;i++,j=RegsetNextSingleton(j)) if (RegsetIn(r,i))
#define FOREACH_REG(i) for (i=LL(0);i<MAX_REG_ITERATOR;i++)
#define REGSUBSET_FOREACH_SINGLETON_REGSET3(s,r,i,j) for (i=s*REG_SUBSET_SIZE,RegsetSetSingleton(j,i);!RegsetIsEmpty(r);RegsetSetDiff(r,j),i++,RegsetSetSingleton(j,i)) if (RegsetIsSubset(r,j))
#define REGSUBSET_FOREACH_SINGLETON_REGSET4(s,r,i,j) for (i=s*REG_SUBSET_SIZE+(REG_SUBSET_SIZE-1),RegsetSetSingleton(j,i);!RegsetIsEmpty(r);RegsetSetDiff(r,j),i--,RegsetSetSingleton(j,i)) if (RegsetIsSubset(r,j))

/*}}}*/
/*Regsets < 64 {{{ */
#else /* MAX_REG_ITERATOR <= 64 */

extern int RegTypeNameOffset[];
extern char *REGNAMES[];

/*  NullRegs should be defined with the correct data length */
#if MAX_REG_ITERATOR > 32
#define NullRegs ULL(0x0)
#else
#define NullRegs 0x0
#endif

#if MAX_REG_ITERATOR > 32
/* 32 < regs <= 64 */
#define register_subset(s) \
        (((ULL(1) << REG_SUBSET_SIZE) - 1) << ((s)*REG_SUBSET_SIZE))
#else
/* regs <= 32 */
#define register_subset(s) \
        (((1 << REG_SUBSET_SIZE) - 1) << ((s)*REG_SUBSET_SIZE))
#endif

/*#define ARM_ALL_BUT_PC_AND_COND 0x7fff */
/* constructor and destructor */

/*! \defgroup REGSET_ALLOC Constructors and destructors */
/*@{*/
#define RegsetNew() NullRegs
#define RegsetToUint32(x)       ((t_uint32)((t_regset)(x)))
#define RegsetToUint64(x)       ((t_uint64)((t_regset)(x)))
#define RegsetNewFromUint32(x)  ((t_regset)((t_uint32)(x)))
#define RegsetNewFromUint64(x)  ((t_regset)((t_uint64)(x)))
#define RegsetNewInvers(x, y) ((~(x)) & (y))
#define RegsetUnion(x,y) (x)|(y)
#define RegsetIntersect(x,y) (x)&(y)
#define RegsetDiff(x,y) (x)&(~(y))
#define RegsetDup(x) (x)

#define RegsetFree(r) while(0)
/*@}*/

/* regset manipulations with assignments */

#define RegsetSetEmpty(r) r=NullRegs
#define RegsetEmpty() NullRegs
#define RegsetSetDup(x,y) (x)=(y)
#define RegsetSetInvers(x) (x)=(~(x))
#define RegsetSetAddReg(r,i) (r)|=(ULL(1)<<(i))
#define RegsetAddReg(r,i) ((r)|(ULL(1)<<(i)))
#define RegsetSetAddMultipleRegs(r,i) (r)|=(i)
#define RegsetAddMultipleRegs(r,i) ((r)|(i))
#define RegsetSetSingleton(r,i) (r)=ULL(1)<<(i)
#define RegsetSetSubReg(r,i) (r)&=~(ULL(1)<<(i))
#define RegsetSubReg(r,i) ((r)&~(ULL(1)<<(i)))
#define RegsetSetUnion(x,y) (x)|=(y)
#define RegsetSetIntersect(x,y) (x)&=(y)
#define RegsetSetDiff(x,y) (x)&=(~(y))
#define RegsetSetMutualExclusive(x,y) (x)=((x)&~(y))|((~x)&(y))

/* regset tests */

#define RegsetIsMutualExclusive(x,y) (!((x)&(y)))
#define RegsetIsEmpty(r) ((r)==NullRegs)
#define RegsetEquals(x,y) ((x)==(y))
#define RegsetIn(x,y) ((x)& ( ULL(1) << (y) )?TRUE:FALSE)
#define RegsetIsSubset(x,y) (!((~(x))&y))

/* regset iterators */

#define REGSET_FOREACH_REG(r,i) for (i=LL(0);i<MAX_REG_ITERATOR;i++) if (RegsetIn(r,i))
#define REGSET_FOREACH_REG_INVERSE(r,i) for (i=(MAX_REG_ITERATOR-1);i>=0;i--) if (RegsetIn(r,i))

#define REGSET_FOREACH_SINGLETON_REGSET(r,i,j) for (i=LL(0),j=ULL(1);i<MAX_REG_ITERATOR;i++,j<<=1) if (RegsetIn(r,i))

#define REGSUBSET_FOREACH_SINGLETON_REGSET(s,r,i,j) for (i=s*REG_SUBSET_SIZE,j=(ULL(1))<<(s*REG_SUBSET_SIZE);i<(s+1)*REG_SUBSET_SIZE;i++,j<<=1) if (RegsetIn(r,i))
#define REGSUBSET_FOREACH_SINGLETON_REGSET2(s,r,i,j) for (i=s*REG_SUBSET_SIZE,j=(ULL(1))<<(s*REG_SUBSET_SIZE);i<(s+1)*REG_SUBSET_SIZE;i++,j<<=1) if (RegsetIsSubset(r,j))
#define REGSUBSET_FOREACH_SINGLETON_REGSET3(s,r,i,j) for (i=s*REG_SUBSET_SIZE,j=(ULL(1))<<(i);!RegsetIsEmpty(r);RegsetSetDiff(r,j),i++,j<<=1) if (RegsetIsSubset(r,j))
/*#define REGSUBSET_FOREACH_SINGLETON_REGSET4(s,r,i,j) for (i=s*REG_SUBSET_SIZE+(REG_SUBSET_SIZE-1),j=(ULL(1))<<(i);!RegsetIsEmpty(r);RegsetSetDiff(r,j),i--,j>>=1) if (RegsetIsSubset(r,j)) */
#define REGSUBSET_FOREACH_SINGLETON_REGSET4(s,r,i,j) for (i=s*REG_SUBSET_SIZE+(REG_SUBSET_SIZE-1),RegsetSetSingleton(j,i);!RegsetIsEmpty(r);RegsetSetDiff(r,j),i--,RegsetSetSingleton(j,i)) if (RegsetIsSubset(r,j))

#define FOREACH_REG(i) for (i=LL(0);i<MAX_REG_ITERATOR;i++)

t_uint32 RegsetCountRegs (t_regset r);

t_reg RegsetFindFirstBlob (t_regset r, t_uint32 * size);

#endif

t_string RegsetSerialize (t_regset r);
t_regset RegsetDeserialize (t_string s);
#endif
/*}}}*/
/*@}*/
/*@}*/
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
