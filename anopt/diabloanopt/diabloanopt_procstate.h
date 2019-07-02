/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/*!\addtogroup DIABLO_DATATYPES */
/*@{ */
#ifndef PROCSTATE_TYPEDEFS
#define PROCSTATE_TYPEDEFS
/** \internal */
typedef struct _t_procstate t_procstate;
/** \internal */
typedef struct _t_argstate t_argstate;
/** \internal */
typedef union  _t_register_content  t_register_content;
typedef struct _t_register_helper t_register_helper;
/** \internal */
typedef struct _t_regstate t_regstate;
/*! t_lattice_level is used to return information about the values,
   relocation tags and condition bits propagated in state */
typedef enum {CP_TOP=-1,CP_VALUE=0,CP_BOT=1} t_lattice_level; 
#endif
/** \internal */
/*! \defgroup PROCSTATES Processor States 
 *
 * Processor states are used during constant propagation: there they
 * represent the state of the processor at a given moment during
 * execution/propagation */
/*@{ */


#ifndef PROCSTATE_HEADER
#define PROCSTATE_HEADER

#include <diabloflowgraph.h>

typedef void (*TestAndSetConditionFunction)(t_cfg_edge *,t_procstate*, t_procstate **, t_procstate **, t_procstate **);

/*! This union represents register contents on a generic target
architecture. It can be integer (addresses) or floating-point
values. The latter are not yet used. */
union _t_register_content
{
  t_address i;
  /*  t_float f; */
};

struct _t_register_helper
{
  int nr_helpers;
  t_ins **helpers;
};

/*! This struct holds processor states, i.e. the register contents
    (including possible TOP and BOT values) and condition flags that
    are propagated over a program during constant propagation.

    We also propagate so called relocation tags: if a register content
    being propagated is a code or data address, the relocation in the
    program describing this address is propagated along. This will
    become useful for dead data detection.
 */

struct _t_procstate
{
  /*! The registers for which the propagated value is TOP are set to 1
    in this register set. */
  t_regset top;
  /*! The registers for which the propagated value is BOT are set to 1
    in this register set. */
  t_regset bot;
  /*! The registers for which the propagated relocation tag is TOP are
    set to 1 in this register set. Relocation tags are propagated
    together with values. Although not exploited today, the
    information in the relocation tags will be useful later on. TOP in
    this case means: the propagated value cannot be an address*/
  t_regset tag_bot;
  /*! The registers for which the propagated relocation is BOT are set
    to 1 in this register set. Bot for relocation tags means that the
    propagated value (a constant or bot) can be an unknown address,
    used to access an unknown block*/
  t_regset tag_top;
  /*! The condition flags (modeled as separate registers) being
      propagated are either set, clear, bot or top. These values are
      encoded in two register sets: with the following encoding:
      - bot: cond_true = 1, cond_false = 1
      - top: cond_true = 0, cond_false = 0
      - set: cond_true = 1, cond_false = 0
      - clear: cond_true = 0, cond_false = 1
  */
  t_regset cond_true;
  /*! \ref _t_procstate::cond_true */
  t_regset cond_false;

  /*! In case there are known constant values propagated, they are stored in
   * this arrays. */

  t_register_content* register_values;
#if CONSTPROP_HELPERS
  t_register_helper* register_helpers;
#endif

  /*! A similar array is used to store the actual relocation tags in case they
   * are constant. */

  t_reloc** register_tags;
};

t_procstate* ProcStateNew(t_architecture_description * desc);
void ProcStateFree(t_procstate* state);
void ProcStateRealFree(void);
void ProcStateSetAllTop(t_procstate* state, t_regset regs);
void ProcStateSetAllBot(t_procstate* state, t_regset regs);
void ProcStateSetRegsetTop(t_procstate* state, t_regset regs);
void ProcStateSetRegsetBot(t_procstate* state, t_regset regs);
void ProcStateSetTagsetTop(t_procstate* state, t_regset regs);
void ProcStateSetTagsetBot(t_procstate* state, t_regset regs);
void ProcStateSetCondsetTop(t_procstate* state, t_regset regs);
void ProcStateSetCondsetBot(t_procstate* state, t_regset regs);
void ProcStateSetRegTop(t_procstate* state, t_reg reg);
void ProcStateSetRegBot(t_procstate* state, t_reg reg);
void ProcStateSetBothBot(t_procstate* state, t_reg reg);
void ProcStateSetTagTop(t_procstate* state, t_reg reg);
void ProcStateSetTagBot(t_procstate* state, t_reg reg);
void ProcStateSetCondTop(t_procstate* state, t_reg reg);
void ProcStateSetCondBot(t_procstate* state, t_reg reg);
void ProcStateSetReg(t_procstate* state, t_reg reg, t_register_content value);
void ProcStateSwapRegisterInfo(t_procstate *state, t_reg rx, t_reg ry);
t_string IoModifierProcState (t_const_string modifiers, va_list *ap);
void ProcStateSetTag(t_procstate* state, t_reg reg, t_reloc* value);
//void ProcStateSetCond(t_procstate* state, t_reg reg, t_bool value);
/** Set the propagated condition bit in reg in a state to value */
static inline void ProcStateSetCond(t_procstate* state, t_reg reg, t_bool value)
{
  t_regset * set1 = &(state->cond_true);
  t_regset * set2 = &(state->cond_false);
  t_regset * tmp;

  if (!value)
    {
      tmp = set1;
      set1 = set2;
      set2 = tmp;
    }
  
  real_RegsetSetAddReg(set1,reg);
  real_RegsetSetSubReg(set2,reg);
}



t_lattice_level ProcStateGetReg(t_procstate* state, t_reg reg, t_register_content * value);
t_lattice_level ProcStateGetTag(t_procstate* state, t_reg reg, t_reloc ** value);
t_lattice_level ProcStateGetCond(t_procstate* state, t_reg reg, t_bool * value);
t_bool ProcStateJoinSimple(t_procstate* dest, t_procstate* src, t_regset regs, t_architecture_description * desc) ;
void ProcStateDup(t_procstate *dest, t_procstate* src, t_architecture_description* desc);
bool ProcStateEquals(t_procstate *a, t_procstate *b, t_architecture_description *desc);
t_procstate * ProcStateNewDup(t_procstate* src);
void ProcStateJoinTags(t_procstate * state, t_reg dest, t_reg src);

void ProcStateAdvance (t_procstate *state, t_ins *from_before, t_ins *to_after);
t_procstate *BblProcStateBefore (t_bbl *bbl);
t_procstate *InsProcStateAfter (t_ins *ins);
t_procstate *InsProcStateBefore (t_ins *ins);
t_procstate *BblProcStateAfter (t_bbl *bbl);
/* @} */

#define ISVALUE(x) ((x)==CP_VALUE)
#define ISBOT(x) ((x)==CP_BOT)
#define ISTOP(x) ((x)==CP_TOP)



struct _t_argstate {
  int nargs;
  t_address *val;
  t_reloc **tag;
  t_lattice_level *vlevel;
  t_lattice_level *tlevel;
};

t_argstate *ArgStateNew(int nargs);

/* Dominique: typedefs for storing constant values in the instruction structure */

typedef struct _valuereg {
  /** level of the plain register value (TOP, VALUE or BOT) */
  t_lattice_level level;
  /** the actual plain register value */
  t_register_content val;
  /** level of the relocation associated with the register value (TOP, VALUE or BOT) */
  t_lattice_level taglevel;
  /** the actual relocation associated with the register value */
  t_reloc * tag;
} valuereg;

typedef union _allstate {
  /** if it's a normal register (not just a condition bit) use a valuereg structure */
  valuereg v;
  /** if it's a condition register instead of a normal one, value and level are stored in
      this variable: bot = 11b, top = 00b, set = 10b, clear = 01b */
  t_uint8 c;
} allstate;

/*! Holds the state of a register */
struct _t_regstate {
  /** the register we are describing */
  t_reg reg;
  allstate state; 
};


/* some helper definitions */
#define __rsVLevel(s)		(s)->state.v.level
#define __rsTLevel(s)		(s)->state.v.taglevel
#define __rsValue(s)		(s)->state.v.val
#define __rsTag(s)		(s)->state.v.tag
#define __rsCond(s)		(s)->state.c
#define __rsReg(s)		(s)->reg

#define RegStateSetReg(s,r)	__rsReg(s) = (r)
#define RegStateGetReg(s)	__rsReg(s)

#define RegStateSetTop(s)	(__rsVLevel(s) = TOP)
#define RegStateSetBot(s)	(__rsVLevel(s) = BOT)
#define RegStateSetTagTop(s)	(__rsTLevel(s) = TOP, __rsTag(s) = NULL)
#define RegStateSetTagBot(s)	(__rsTLevel(s) = BOT, __rsTag(s) = NULL)
#define RegStateSetCondTop(s)	__rsCond(s) = 0x0
#define RegStateSetCondBot(s)	__rsCond(s) = 0x3

#define RegStateSetValue(s,value)	(__rsVLevel(s) = VALUE, __rsValue(s) = (value))
#define RegStateSetTag(s,reloc)		(__rsTLevel(s) = VALUE, __rsTag(s) = (reloc))
#define RegStateSetCond(s,value)	__rsCond(s) = (value) ? 0x2 : 0x1

#define RegStateGetValue(s,valptr)	(*(valptr) = __rsValue(s), __rsVLevel(s))
#define RegStateGetTag(s,tagptr)	(*(tagptr) = __rsTag(s),   __rsTLevel(s))
#define RegStateGetCond(s,boolptr)	(*(boolptr) = (__rsCond(s) == 0x2), (__rsCond(s) == 0) ? TOP : (__rsCond(s) == 0x3) ? BOT : VALUE)


//t_lattice_level RegStateGetRegValue(t_ins * ins, t_regset * regset, t_reg reg, t_register_content * content);
//t_lattice_level RegStateGetRegCond(t_ins * ins, t_regset * regset, t_reg reg, t_bool * cond);
//t_lattice_level RegStateGetRegTag(t_ins * ins, t_regset * regset, t_reg reg, t_reloc ** tag);

void RegStateInit(t_regstate * state);
t_regstate * RegStateAlloc(t_uint32 n);
t_regstate * RegStateArrayCreateFromProcState(t_procstate * ps, t_regset whichregs, const t_architecture_description * desc);
t_bool TwoRelocsInSameBlock(t_reloc * reloc1, t_reloc* reloc2);
t_string BblConstantsAfterTextual (t_bbl * bbl);
#define T_PROCSTATE(x)   ((t_procstate*)(x))
void ArgStateFree(t_argstate *as);
t_bool ArgStateJoin(t_argstate *dest, t_argstate *src);
void ArgStateSetUndefinedBot(t_argstate *as);
void ArgStateDup(t_argstate *dest, t_argstate *src);
t_bool ArgStateSetArg(t_argstate *as, int argno, t_lattice_level vlevel, t_address val, t_lattice_level tlevel, t_reloc *rel);
#endif
/* @} */
/* vim: set shiftwidth=2 foldmethod=marker: */
