/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloanopt.h>

#if CONSTPROP_HELPERS
static
void FreeHelpers(t_register_helper *helpers)
{
  for (int i = 0; i < MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE; i++)
    if (helpers[i]->nr_helpers > 0)
      Free(helpers[i]->helpers);

  Free(helpers);
}

static
void ResetHelpers(t_register_helper *helpers)
{
  for (int i = 0; i < MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE; i++)
  {
    if (helpers[i]->nr_helpers > 0)
    {
      Free(helpers[i]->helpers);
      helpers[i]->nr_helpers = 0;
    }
  }
}
#endif

/*! Used to see whether or not two propagated relocs point to the same
  block (basic block or data block) */
t_bool TwoRelocsInSameBlock(t_reloc * reloc1, t_reloc* reloc2)
{
  if (reloc1==reloc2)
    return TRUE;

  if (RELOC_N_TO_RELOCATABLES(reloc1)!=1) return FALSE;
  if (RELOC_N_TO_RELOCATABLES(reloc2)!=1) return FALSE;
  
  if (RELOC_TO_RELOCATABLE(reloc1)[0]==RELOC_TO_RELOCATABLE(reloc2)[0])
    return TRUE;

  return FALSE;
}

/* list of free procstates */
static void ** pslist = NULL;

/*! Allocates an object of type t_procstate */
t_procstate* ProcStateNew(t_architecture_description * desc)
{
  t_procstate * state;
  t_register_content * rc;
  t_register_helper *helpers;
  t_reloc ** tags;

  if (pslist)
  {
    state = (t_procstate *) pslist;
    pslist = *pslist;
    
    /* note: the following bzero's are necessary if floating point support is reintroduced in t_register_content*/

    /* bzero(state->register_values,MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE*sizeof(t_register_content));*/
    /* bzero(state->register_tags,MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE*sizeof(t_reloc *));*/
  }
  else
  {
    state = (t_procstate *) Malloc(sizeof(t_procstate));
    rc = Calloc(MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE,sizeof(t_register_content));
    state->register_values = rc;
    tags = Malloc(MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE*sizeof(t_reloc *));
    state->register_tags = tags;
#if CONSTPROP_HELPERS
    helpers = Calloc(MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE*sizeof(t_register_helper));
    state->register_helpers = helpers;
#endif
  }

  RegsetSetDup(state->top, desc->all_registers);
  RegsetSetDup(state->tag_top, desc->all_registers);
  RegsetSetEmpty(state->bot);
  RegsetSetEmpty(state->tag_bot);
  RegsetSetEmpty(state->cond_true);
  RegsetSetEmpty(state->cond_false);

  return state;
}

/* #define DEBUG_PROCSTATE_FREE */

/*! Frees an object of type t_procstate */
void ProcStateFree(t_procstate* state)
{
#ifdef DEBUG_PROCSTATE_FREE
  void ** i_list = NULL;
#endif
  if (pslist) {
    *((void **)state) = pslist;
    pslist = (void *)state;
  } else {
    Free(state->register_tags);
    Free(state->register_values);
#if CONSTPROP_HELPERS
    FreeHelpers(state->register_helpers);
#endif
    Free(state);
  }
#ifdef DEBUG_PROCSTATE_FREE
  i_list = *pslist;
  while (i_list)
  {
    if (state == (t_procstate *) i_list) 
      FATAL(("Duplicate ProcStateFree"));
    i_list = *i_list;
  }
#endif
}

void ProcStateRealFree(void)
{
  t_procstate * ps;

  while (pslist)
  {
    ps = (t_procstate *)pslist;
    pslist = *pslist;
    Free(ps->register_values);
    Free(ps->register_tags);
#if CONSTPROP_HELPERS
    FreeHelpers(ps->register_helpers);
#endif

    Free(ps);
  }
}

/** Set all propagated items in a state to Top (value, reloc tag, cond bits */
void ProcStateSetAllTop(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->top,regs);
  RegsetSetUnion(state->tag_top,regs);
  RegsetSetDiff(state->bot,regs);
  RegsetSetDiff(state->tag_bot,regs);
  RegsetSetDiff(state->cond_true,regs);
  RegsetSetDiff(state->cond_false,regs);
#if CONSTPROP_HELPERS
  ResetHelpers(state->helpers);
#endif
}

/** Set all propagated items in a state to Bot (value, reloc tag, cond bits */
void ProcStateSetAllBot(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->bot,regs);
  RegsetSetUnion(state->tag_bot,regs);
  RegsetSetDiff(state->top,regs);
  RegsetSetDiff(state->tag_top,regs);
  RegsetSetUnion(state->cond_true,regs);
  RegsetSetUnion(state->cond_false,regs);
#if CONSTPROP_HELPERS
  ResetHelpers(state->helpers);
#endif
}


/** Set all propagated items in a state to Top (value, reloc tag, cond bits */
void ProcStateSetRegsetTop(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->top,regs);
  RegsetSetDiff(state->bot,regs);
}

/** Set all propagated values in regs a state to Bot */
void ProcStateSetRegsetBot(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->bot,regs);
  RegsetSetDiff(state->top,regs);
}

/** Set all propagated relocation tags in regs a state to Top */
void ProcStateSetTagsetTop(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->tag_top,regs);
  RegsetSetDiff(state->tag_bot,regs);
}

/** Set all propagated relocation tags in regs a state to Bot */
void ProcStateSetTagsetBot(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->tag_bot,regs);
  RegsetSetDiff(state->tag_top,regs);
}

/** Set all condition bits in regs a state to Top */
void ProcStateSetCondsetTop(t_procstate* state, t_regset regs)
{
  RegsetSetDiff(state->cond_true,regs);
  RegsetSetDiff(state->cond_false,regs);
}
 
/** Set all condition bits in regs a state to Bot */
void ProcStateSetCondsetBot(t_procstate* state, t_regset regs)
{
  RegsetSetUnion(state->cond_true,regs);
  RegsetSetUnion(state->cond_false,regs);
}

/** Set the propagated value in reg in a state to Top */
void ProcStateSetRegTop(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->top,reg);
  RegsetSetSubReg(state->bot,reg);
}

/** Set the propagated value in reg in a state to Bot */
void ProcStateSetRegBot(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->bot,reg);
  RegsetSetSubReg(state->top,reg);
}

void ProcStateSetBothBot(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->bot,reg);
  RegsetSetSubReg(state->top,reg);
  RegsetSetAddReg(state->tag_bot,reg);
  RegsetSetSubReg(state->tag_top,reg);
}


/** Set the propagated relocation tag in reg in a state to Top */
void ProcStateSetTagTop(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->tag_top,reg);
  RegsetSetSubReg(state->tag_bot,reg);
}

/** Set the propagated relocation tag in reg in a state to Bot */
void ProcStateSetTagBot(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->tag_bot,reg);
  RegsetSetSubReg(state->tag_top,reg);
}

/** Set the propagated cond bit in reg in a state to Top */
void ProcStateSetCondTop(t_procstate* state, t_reg reg)
{
  RegsetSetSubReg(state->cond_true,reg);
  RegsetSetSubReg(state->cond_false,reg);
}

/** Set the propagated cond bit in reg in a state to Bot */
void ProcStateSetCondBot(t_procstate* state, t_reg reg)
{
  RegsetSetAddReg(state->cond_true,reg);
  RegsetSetAddReg(state->cond_false,reg);
}

/** Set the propagated value in reg in a state to value */
void ProcStateSetReg(t_procstate* state, t_reg reg, t_register_content value)
{
  RegsetSetSubReg(state->bot,reg);
  RegsetSetSubReg(state->top,reg);
  state->register_values[reg] = value;
}

/** Set the propagated relocation tag in reg in a state to value */
void ProcStateSetTag(t_procstate* state, t_reg reg, t_reloc* value)
{
  RegsetSetSubReg(state->tag_bot,reg);
  RegsetSetSubReg(state->tag_top,reg);
  state->register_tags[reg] = value;
}

void ProcStateSwapRegisterInfo(t_procstate *state, t_reg rx, t_reg ry) {
  /* reset x */
  t_register_content x_content;
  t_reloc *x_tag = NULL;

  t_lattice_level x_level_reg = ProcStateGetReg(state, rx, &x_content);
  RegsetSetSubReg(state->bot, rx);
  RegsetSetSubReg(state->top, rx);

  t_lattice_level x_level_tag = ProcStateGetTag(state, rx, &x_tag);
  RegsetSetSubReg(state->tag_bot, rx);
  RegsetSetSubReg(state->tag_top, rx);
  
  /* reset y */
  t_register_content y_content;
  t_reloc *y_tag = NULL;

  t_lattice_level y_level_reg = ProcStateGetReg(state, ry, &y_content);
  RegsetSetSubReg(state->bot, ry);
  RegsetSetSubReg(state->top, ry);

  t_lattice_level y_level_tag = ProcStateGetTag(state, ry, &y_tag);
  RegsetSetSubReg(state->tag_bot, ry);
  RegsetSetSubReg(state->tag_top, ry);
  
  /* update necessary info */
  state->register_tags[rx] = y_tag;
  state->register_tags[ry] = x_tag;
  
  state->register_values[rx] = y_content;
  state->register_values[ry] = x_content;
  
  if (x_level_reg == CP_BOT)
    RegsetSetAddReg(state->bot, ry);
  else if (x_level_reg == CP_TOP)
    RegsetSetAddReg(state->top, ry);
  if (x_level_tag == CP_BOT)
    RegsetSetAddReg(state->tag_bot, ry);
  else if (x_level_tag == CP_TOP)
    RegsetSetAddReg(state->tag_top, ry);
    
  if (y_level_reg == CP_BOT)
    RegsetSetAddReg(state->bot, rx);
  else if (y_level_reg == CP_TOP)
    RegsetSetAddReg(state->top, rx);
  if (y_level_tag == CP_BOT)
    RegsetSetAddReg(state->tag_bot, rx);
  else if (y_level_tag == CP_TOP)
    RegsetSetAddReg(state->tag_top, rx);
}

#if 0
/** Set the propagated condition bit in reg in a state to value */
void ProcStateSetCond(t_procstate* state, t_reg reg, t_bool value)
{
  if (value)
    {
      RegsetSetAddReg(state->cond_true,reg);
      RegsetSetSubReg(state->cond_false,reg);
    }
  else
    {
      RegsetSetAddReg(state->cond_false,reg);
      RegsetSetSubReg(state->cond_true,reg);
    }
}
#endif
/** Get the value of a reg in a propagated state. This is returned in
    the third argument. The actual return value is CP_TOP,CP_VALUE or
    CP_BOT. */
t_lattice_level ProcStateGetReg(t_procstate* state, t_reg reg, t_register_content * value)
{
  if (RegsetIn(state->top,reg))
    {
      return CP_TOP;
    }

  if (RegsetIn(state->bot,reg))
    {
      return CP_BOT;
    }

  *value = state->register_values[reg];

  return CP_VALUE;
}

/** Get the relocation tag of a reg in a propagated state. This is returned in
    the third argument. The actual return value is CP_TOP,CP_VALUE or
    CP_BOT. */
t_lattice_level ProcStateGetTag(t_procstate* state, t_reg reg, t_reloc ** value)
{
  if (RegsetIn(state->tag_top,reg))
    {
      return CP_TOP;
    }

  if (RegsetIn(state->tag_bot,reg))
    {
      return CP_BOT;
    }

  *value = state->register_tags[reg];

  return CP_VALUE;
}

/** Get the condition bit of a reg in a propagated state. This is returned in
    the third argument. The actual return value is CP_TOP,CP_VALUE or
    CP_BOT. */
t_lattice_level ProcStateGetCond(t_procstate* state, t_reg reg, t_bool * value)
{
  if (RegsetIn(state->cond_true,reg) && RegsetIn(state->cond_false,reg))
    {
      return CP_BOT;
    }
  else if (RegsetIn(state->cond_true,reg))
    {
      *value = 1;
      return CP_VALUE;
    }
  else if (RegsetIn(state->cond_false,reg))
    {
      *value = 0;
      return CP_VALUE;
    }
  else
    {
      return CP_TOP;
    }
}

/** Join src with dest, but only for the registers in regs. The final
    argument is not yet used, but it will be once dead data detection is
    implemented.*/

t_bool ProcStateJoinSimple(t_procstate* dest, t_procstate* src, t_regset regs, t_architecture_description * desc) 
{
  t_bool change = FALSE;
  t_reg  reg;
  t_regset dest_old_bot = RegsetNew();
  t_regset dest_old_top = RegsetNew();
  t_regset old_tag_bot  = RegsetNew();
  t_regset tmp          = RegsetNew();
  t_regset singleton    = RegsetNew();
  t_regset regs2        = RegsetNew();
  t_regset dest_cond_false = RegsetNew();
  t_regset dest_cond_true  = RegsetNew();

  t_register_content * src_ptr, * dest_ptr;
  t_reloc ** src_tag_ptr, **dest_tag_ptr;
  int set;
  t_register_content *src_set;
  t_register_content *dest_set;
  t_reloc **src_tag_set;
  t_reloc **dest_tag_set;

  if (!dest) { FATAL(("DEST = NULL!\n")); }

  /* first do normal registers */

  /* Speedup the case where we are joining all top */
  if (RegsetEquals(dest->top,desc->all_registers) && RegsetEquals(regs, desc->all_registers))
  {
    ProcStateDup(dest,src,desc);
    /* printf("true 1\n");  */
    return TRUE;
  }

  RegsetSetDup(dest_old_bot,dest->bot);
  RegsetSetDup(dest_old_top,dest->top);

  RegsetSetDup(tmp,src->bot);
  RegsetSetIntersect(tmp,regs);
  RegsetSetUnion(dest->bot,tmp);
  RegsetSetDiff(dest->top,tmp);

  /* More bots than dest? */
  change |= !RegsetEquals(dest->bot,dest_old_bot);

  /* if (change) { printf("true 2 %x %x\n",dest_bot,dest_old_bot); }  */

  RegsetSetDup(regs2,regs);
  RegsetSetDiff(regs2,dest->bot);
  RegsetSetDiff(regs2,src->top);

  src_set = src->register_values;
  dest_set = dest->register_values;

  for (set=MAX_NR_REG_SUBSETS-1;set>=0;set--)
  {
    t_regset subset = register_subset(set);
    RegsetSetDup(tmp,regs2);
    RegsetSetIntersect(tmp,subset);
    RegsetSetDiff(regs2,tmp);

    REGSUBSET_FOREACH_SINGLETON_REGSET3(set,tmp,reg,singleton)
    {
      src_ptr = src_set+reg;
      dest_ptr = dest_set + reg;

      if (RegsetIsSubset(dest->top,singleton))
      {
	*dest_ptr = *src_ptr;
	RegsetSetDiff(dest->top,singleton);
	change = TRUE;
	continue;
      }

      bool tag_in_src = !RegsetIn(src->tag_bot, reg) && !RegsetIn(src->tag_top, reg);
      bool tag_in_dest = !RegsetIn(dest->tag_bot, reg) && !RegsetIn(dest->tag_top, reg);
      if (AddressIsEq(src_ptr->i,dest_ptr->i)
      		&& !(tag_in_src ^ tag_in_dest))
	continue;

      RegsetSetUnion(dest->bot,singleton);

      change = TRUE;

      /*	  printf("true3 %d\n",reg); */


    }
    if (RegsetIsEmpty(regs2))
      break;
  }

  /* then do tag registers */

  RegsetSetDup(old_tag_bot,dest->tag_bot);

  RegsetSetDup(tmp,src->tag_bot);
  RegsetSetIntersect(tmp,regs);
  RegsetSetUnion(dest->tag_bot,tmp);
  RegsetSetDiff(dest->tag_top,tmp);
  change |= !RegsetEquals(dest->tag_bot,old_tag_bot);

  RegsetSetDup(regs2,regs);
  RegsetSetDiff(regs2,dest->tag_bot);
  RegsetSetDiff(regs2,src->tag_top);

  src_tag_set = src->register_tags;
  dest_tag_set = dest->register_tags;

  for (set=MAX_NR_REG_SUBSETS-1;set>=0;set--)
  {
    t_regset subset = register_subset(set);

    RegsetSetDup(tmp,regs2);
    RegsetSetIntersect(tmp,subset);
    RegsetSetDiff(regs2,tmp);

    REGSUBSET_FOREACH_SINGLETON_REGSET3(set,tmp,reg,singleton)
    {	    
      src_tag_ptr = src_tag_set + reg;
      dest_tag_ptr = dest_tag_set + reg;

      if (RegsetIsSubset(dest->tag_top,singleton))
      {
	*dest_tag_ptr = *src_tag_ptr;
	RegsetSetDiff(dest->tag_top,singleton);
	change = TRUE;
	continue;
      }

      if (TwoRelocsInSameBlock(*src_tag_ptr,*dest_tag_ptr))
	continue;

      RegsetSetUnion(dest->tag_bot,singleton);
      RegsetSetUnion(dest->bot,singleton);

      change = TRUE;
      /*	  printf("true4 %d\n",reg); */
    }
    if (RegsetIsEmpty(regs2))
      break;
  }

  /* and then do the condition registers */

  RegsetSetDup(dest_cond_false,dest->cond_false);
  RegsetSetDup(dest_cond_true,dest->cond_true);
  RegsetSetUnion(dest->cond_false,src->cond_false);
  RegsetSetUnion(dest->cond_true,src->cond_true);

  if (!RegsetEquals(dest_cond_false,dest->cond_false) || !RegsetEquals(dest_cond_true,dest->cond_true))
  {
    /* VERBOSE(0,("true 5 @X\n\n @X\n\n @X\n\n @X\n",desc, dest_cond_false,desc, dest_cond_true,desc, dest->cond_false,desc, dest->cond_true));  */
    change |= TRUE;
  }

  RegsetFree(dest_old_bot);
  RegsetFree(tmp);
  RegsetFree(singleton);
  RegsetFree(dest_cond_false);
  RegsetFree(dest_cond_true);

  return change;
}

/** Duplicate state src to dest */
void ProcStateDup(t_procstate *dest, t_procstate* src, t_architecture_description * desc)
{
  RegsetSetDup(dest->top,src->top);
  RegsetSetDup(dest->bot,src->bot);
  RegsetSetDup(dest->tag_top,src->tag_top);
  RegsetSetDup(dest->tag_bot,src->tag_bot);
  RegsetSetDup(dest->cond_true,src->cond_true);
  RegsetSetDup(dest->cond_false,src->cond_false);

  if (!RegsetEquals(RegsetUnion(src->top,src->bot), desc->all_registers))
    memcpy(dest->register_values,src->register_values,sizeof(t_register_content)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
  if (!RegsetEquals(RegsetUnion(src->tag_top,src->tag_bot), desc->all_registers))
    memcpy(dest->register_tags,src->register_tags,sizeof(t_reloc*)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
}

bool ProcStateEquals(t_procstate *a, t_procstate *b, t_architecture_description *desc)
{
  if (!RegsetEquals(a->top, b->top)) return false;
  if (!RegsetEquals(a->bot, b->bot)) return false;
  if (!RegsetEquals(a->tag_top, b->tag_top)) return false;
  if (!RegsetEquals(a->tag_bot, b->tag_bot)) return false;

  t_reg r;
  REGSET_FOREACH_REG (desc->int_registers, r) {
    if (!RegsetIn(a->tag_top, r) && !RegsetIn(a->tag_bot, r)) {
      /* check if the tag refers to the same location */
      if (!TwoRelocsInSameBlock(a->register_tags[r],b->register_tags[r]))
        return false;
    }

    if (!RegsetIn(a->top, r) && !RegsetIn(a->bot, r))
      if (!AddressIsEq(a->register_values[r].i, b->register_values[r].i)) return false;
  }

  REGSET_FOREACH_REG(desc->cond_registers, r) {
    t_bool flag1;
    t_lattice_level lvl1 = ProcStateGetCond(a, r, &flag1);

    t_bool flag2;
    t_lattice_level lvl2 = ProcStateGetCond(b, r, &flag2);

    if (lvl1 != lvl2)
      return false;

    /* lvl1 == lvl2 */
    if (lvl1 == CP_VALUE
        && flag1 != flag2)
      return false;
  }

  return true;
}

void ProcStateDupRegsIntoTop(t_procstate *dest, t_procstate* src, t_regset regs)
{
  RegsetSetDup(dest->top,src->top);
  RegsetSetDup(dest->bot,src->bot);
  RegsetSetDup(dest->tag_top,src->tag_top);
  RegsetSetDup(dest->tag_bot,src->tag_bot);
  RegsetSetDup(dest->cond_true,src->cond_true);
  RegsetSetDup(dest->cond_false,src->cond_false);

  memcpy(dest->register_values,src->register_values,sizeof(t_register_content)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
  memcpy(dest->register_tags,src->register_tags,sizeof(t_reloc*)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
}


t_procstate * ProcStateNewDup(t_procstate* src)
{
  t_procstate * dest;
  t_register_content * rc;
  t_reloc ** tags;

  if (pslist)
  {
    dest = (t_procstate *) pslist;
    pslist = *pslist;
  }
  else
  {
    dest = (t_procstate *) Malloc(sizeof(t_procstate));
    rc = Calloc(MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE,sizeof(t_register_content));
    dest->register_values = rc;
    tags = Malloc(MAX_NR_REG_SUBSETS*REG_SUBSET_SIZE*sizeof(t_reloc *));
    dest->register_tags = tags;
  }

  RegsetSetDup(dest->top,src->top);
  RegsetSetDup(dest->bot,src->bot);
  RegsetSetDup(dest->tag_top,src->tag_top);
  RegsetSetDup(dest->tag_bot,src->tag_bot);
  RegsetSetDup(dest->cond_true,src->cond_true);
  RegsetSetDup(dest->cond_false,src->cond_false);

  memcpy(dest->register_values,src->register_values,sizeof(t_register_content)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
  memcpy(dest->register_tags,src->register_tags,sizeof(t_reloc*)*REG_SUBSET_SIZE*MAX_NR_REG_SUBSETS);
  
  return dest;
}

/** Joins the relocatoin tags in reg src in state with the one in
    register dest */
void ProcStateJoinTags(t_procstate * state, t_reg dest, t_reg src)
{
  t_reloc * reloc_src;
  t_reloc * reloc_dest;
  t_lattice_level value_src;
  t_lattice_level value_dest;


  if (CP_TOP == (value_src = ProcStateGetTag(state,src,&reloc_src)))
    return;
  else if (CP_BOT == value_src)
    ProcStateSetTagBot(state,dest);
  else 
    {
      if (CP_TOP == (value_dest = ProcStateGetTag(state,dest,&reloc_dest)))
	ProcStateSetTag(state,dest,reloc_src);
      else if (CP_BOT==value_dest)
	return;
      else if (!TwoRelocsInSameBlock(reloc_src,reloc_dest))
	ProcStateSetTagBot(state,dest);
    }
}


t_string
BblConstantsAfterTextual (t_bbl * bbl) /* {{{ */
{
  t_architecture_description * desc = BBL_DESCRIPTION(bbl);
  char **register_names = desc->register_names;
  t_reg reg;
  t_string_array *regs = StringArrayNew ();
  char *ret = NULL;
  t_bool join = FALSE;
  t_cfg * cfg=BBL_CFG(bbl);

  if (!register_names)
  {
    WARNING(("No register names found for @B", bbl));
    return NULL;
  }
  if(BBL_IS_HELL(bbl))
    return NULL;


  {
    t_ins * i_ins;
    t_procstate * prev_state, * next_state;
    t_regset allregs;

    //    ConstantPropagationInit(cfg);

    if(CFG_INSTRUCTION_EMULATOR(cfg) == NULL)
      return NULL;

    prev_state = ProcStateNew(desc);
    next_state = ProcStateNew(desc);
    allregs=desc->all_registers;

    ProcStateDup(next_state,BBL_PROCSTATE_IN(bbl),desc);
    ProcStateDup(prev_state,BBL_PROCSTATE_IN(bbl),desc);

    BBL_FOREACH_INS(bbl,i_ins)
    {
      CFG_INSTRUCTION_EMULATOR(cfg)(i_ins,next_state,TRUE);
      ProcStateDup(prev_state,next_state,desc);
    }
    ProcStateFree(prev_state);

    REGSET_FOREACH_REG(allregs, reg)
    {
      t_lattice_level level;
      t_register_content value;

      level= ProcStateGetReg(next_state,reg, &value);
      if(level==CP_VALUE)
      {
	StringArrayAppendString (regs, StringIo ("%s=%d ", register_names[reg],value));
	join = TRUE;
      }
    }
    ProcStateFree(next_state);

  }
  if (join)
    ret = StringArrayJoin (regs, ",");
  else
    ret = StringDup ("empty");

  StringArrayFree (regs);

  return ret;
}
/* }}} */

t_string IoModifierProcState (t_const_string modifiers, va_list *ap)
{
  t_architecture_description *desc = va_arg (*ap, t_architecture_description *);
  t_procstate *state = va_arg (*ap, t_procstate *);
  t_string tmp, ret = StringDup("");
  char condbuf[200];
  char *iter;
  char *nl_iter;

  t_reg r;

  t_bool showtags = FALSE;
  t_bool single_line = FALSE;
  t_bool double_line = FALSE;
  t_string new_line  = "\n";

  int regs_per_line = desc->address_size == ADDRSIZE32 ? 4 : 2;

  while (modifiers && *modifiers)
  {
    switch (*modifiers)
    {
      case 't':
	showtags = TRUE;
	break;
      case '-':
	single_line = TRUE;
	break;
      case '=':
	double_line = TRUE;
	break;
      case 'l':
        new_line = "\\l";
        break;
      default:
	FATAL (("unknown modifier"));
    }
    ++modifiers;
  }

  /* {{{ int registers */
  REGSET_FOREACH_REG (desc->int_registers, r)
  {
    char regbuf[1000];
    char *iter = regbuf;
    t_register_content c;
    t_reloc *rel;
    t_lattice_level lvl;

    //iter += sprintf (iter, "%4s:", desc->register_names[r]);
    iter += sprintf (iter, "r%d:", r);
    
    lvl = ProcStateGetReg (state, r, &c);
    if (desc->address_size == ADDRSIZE32)
    {
      if (lvl == CP_BOT)
	iter += sprintf (iter, "%s ", "BOT");
      else if (lvl == CP_TOP)
	iter += sprintf (iter, "%s ", "TOP");
      else
	iter += sprintf (iter, "%x ", AddressExtractUint32 (c.i));
    }
    else
    {
      if (lvl == CP_BOT)
	iter += sprintf (iter, "%s ", "BOT");
      else if (lvl == CP_TOP)
	iter += sprintf (iter, "%s ", "TOP");
      else
	iter += sprintf (iter, "%x ", AddressExtractUint32 (c.i));
    }

    if (showtags)
    {
      lvl = ProcStateGetTag (state, r, &rel);
      if (lvl == CP_BOT)
	iter += sprintf (iter, "(tB)  ");
      else if (lvl == CP_TOP)
	iter += sprintf (iter, "(tT)  ");
      else
	iter += sprintf (iter, "(tag) ");
    }

    /*if (((r+1) % regs_per_line) == 0)
    {
      *(iter-1) = *new_line;
      for (nl_iter=new_line; *nl_iter; nl_iter++)
        *(++iter)=*nl_iter;
    }*/

    tmp = ret;
    ret = StringConcat2 (tmp, regbuf);
    Free (tmp);
  } /* }}} */

  /* {{{ condition registers */
  iter = condbuf;
  REGSET_FOREACH_REG (desc->cond_registers, r)
  {
    t_bool flag;
    t_lattice_level lvl = ProcStateGetCond (state, r, &flag);
    if (lvl == CP_BOT)
      iter += sprintf (iter, "%s: B ", desc->register_names[r]);
    else if (lvl == CP_TOP)
      iter += sprintf (iter, "%s: T ", desc->register_names[r]);
    else
      iter += sprintf (iter, "%s: %c ", desc->register_names[r], flag ? '1' : '0');
  }
  if (iter != condbuf)
  {
    *(iter-1) = *new_line;
    for (nl_iter=new_line; *nl_iter; nl_iter++)
      *(++iter)=*nl_iter;
  }
  else
    *iter = '\0';
  /* }}} */

  tmp = ret;
  if (single_line)
    ret = StringConcat3 (tmp, condbuf, "\n------------------------------------------\n");
  else if (double_line)
    ret = StringConcat3 (tmp, condbuf, "\n==========================================\n");
  else
    ret = StringConcat2 (tmp, condbuf);
  Free (tmp);
  return ret;
}


/* get procstate after to_after, starting from the state before from_before.
 * Advancing over one instruction is then as simple as calling
 * ProcStateAdvance (state, ins, ins);
 */
void ProcStateAdvance (t_procstate *state, t_ins *from_before, t_ins *to_after)
{
  t_ins *i_ins;
  t_bbl *bbl = INS_BBL (from_before);
  t_ConstantPropagationInsEmul propagator = CFG_INSTRUCTION_EMULATOR(BBL_CFG(bbl));

  ASSERT (INS_BBL (to_after) == bbl, ("Instructions should be in same basic block"));

  for (i_ins = from_before; i_ins; i_ins = INS_INEXT (i_ins))
  {
    if ((INS_ATTRIB(i_ins) & IF_FAST_CP_EVAL) &&
	!RegsetIsEmpty (
	  RegsetIntersect (INS_REGS_USE(i_ins), state->bot)))
      ProcStateSetAllBot(state,INS_REGS_DEF(i_ins));
    else
      propagator(i_ins,state,FALSE);

    if (i_ins == to_after)
      break;
  }
}

t_procstate *BblProcStateBefore (t_bbl *bbl)
{
  return ProcStateNewDup (BBL_PROCSTATE_IN (bbl));
}

t_procstate *InsProcStateAfter (t_ins *ins)
{
  t_procstate *state = BblProcStateBefore (INS_BBL (ins));
  ProcStateAdvance (state, BBL_INS_FIRST (INS_BBL (ins)), ins);
  return state;
}

t_procstate *InsProcStateBefore (t_ins *ins)
{
  if (INS_IPREV (ins))
    return InsProcStateAfter (INS_IPREV (ins));
  return ProcStateNewDup (BBL_PROCSTATE_IN (INS_BBL (ins)));
}

t_procstate *BblProcStateAfter (t_bbl *bbl)
{
  if (BBL_INS_LAST (bbl))
    return InsProcStateAfter (BBL_INS_LAST (bbl));
  return ProcStateNewDup (BBL_PROCSTATE_IN (bbl));
}
  
/* Dominique: supporting functions for t_regstate */
#if 0
/** initializes a regstate */
void RegStateInit(t_regstate * state)
{
  RegStateSetTop(state);
  RegStateSetTagTop(state);
  RegStateSetCondTop(state);
  RegStateSetReg(state,(t_reg) -1);
}

/** allocates a regstate array of length n */
t_regstate * RegStateAlloc(t_uint32 n)
{
  t_regstate * retval = Malloc(n*sizeof(t_regstate));
  t_uint32 i;
  for (i=0; i<n; i++)
    RegStateInit(&(retval[i]));
  return retval;
}

/** creates an array of regstates from a procstate and a give register set.
    returns NULL if the register set is empty or if all registers are CP_BOT (this saves a lot of memory) */
t_regstate * RegStateArrayCreateFromProcState(t_procstate * ps, t_regset whichregs, const t_architecture_description * desc)
{
  t_regstate * ret;
  t_uint32 regcount;
  t_reg r;
  t_uint32 index;
  t_bool allbot = TRUE;
  t_regset condregs = desc->cond_registers;

  regcount = RegsetCountRegs(whichregs);
  if (regcount == 0)
    return NULL;

  ret = RegStateAlloc(regcount);

  index = 0;
  REGSET_FOREACH_REG(whichregs,r)
  {
    t_regstate * current = &(ret[index]);
    RegStateSetReg(current,r);

    if (RegsetIn(condregs,r))
    {
      t_bool set;
      t_lattice_level level;
      level = ProcStateGetCond(ps,r,&set);
      if (level == CP_TOP)
      {
	RegStateSetCondTop(current);
	allbot = FALSE;
      }
      else if (level == CP_BOT)
      {
	RegStateSetCondBot(current);
      }
      else
      {
	RegStateSetCond(current,set);
	allbot = FALSE;
      }
    }
    else
    {
      t_register_content cont;
      t_lattice_level level;
      t_reloc * tag;
      t_lattice_level taglevel;
      
      level = ProcStateGetReg(ps,r,&cont);
      if (level == CP_TOP)
      {
	RegStateSetTop(current);
	allbot = FALSE;
      }
      else if (level == CP_BOT)
      {
	RegStateSetBot(current);
      }
      else
      {
	RegStateSetValue(current,cont);
	allbot = FALSE;
      }

      taglevel = ProcStateGetTag(ps,r,&tag);
      if (taglevel == CP_TOP)
      {
	RegStateSetTagTop(current);
	allbot = FALSE;
      }
      else if (taglevel == CP_BOT)
      {
	RegStateSetTagBot(current);
      }
      else
      {
	RegStateSetTag(current,tag);
	allbot = FALSE;
      }
    }
    index++;
  }
  
  if (allbot) {
    Free(ret);
    return NULL;
  }
  
  return ret;
}

t_lattice_level RegStateGetRegValue(t_ins * ins, t_regset * regset, t_reg reg, t_register_content * content)
{
  t_reg tmp_reg;
  t_uint32 i = 0;
  if (!INS_USED_REGSTATE(ins))
    return CP_BOT;
  else
  {
    REGSET_FOREACH_REG(*regset,tmp_reg)
    {
      if (RegStateGetReg(&INS_USED_REGSTATE(ins)[i]) == reg) break;
      i++;
    }
    return RegStateGetValue(&INS_USED_REGSTATE(ins)[i],content); 
  }
}

t_lattice_level RegStateGetRegTag(t_ins * ins, t_regset * regset, t_reg reg, t_reloc ** tag)
{
  t_reg tmp_reg;
  t_uint32 i = 0;
  if (!INS_USED_REGSTATE(ins))
    return CP_BOT;
  else
  {
    REGSET_FOREACH_REG(*regset,tmp_reg)
    {
      if (RegStateGetReg(&INS_USED_REGSTATE(ins)[i]) == reg) break;
      i++;
    }
    return RegStateGetTag(&INS_USED_REGSTATE(ins)[i],tag); 
  }
}

t_lattice_level RegStateGetRegCond(t_ins * ins, t_regset * regset, t_reg reg, t_bool * cond)
{
  t_reg tmp_reg;
  t_uint32 i = 0;
  if (!INS_USED_REGSTATE(ins))
    return CP_BOT;
  else
  {
    REGSET_FOREACH_REG(*regset,tmp_reg)
    {
      if (RegStateGetReg(&INS_USED_REGSTATE(ins)[i]) == reg) break;
      i++;
    }
    return RegStateGetCond(&INS_USED_REGSTATE(ins)[i],cond); 
  }
}
#endif


/*****************************************************
 * ARGSTATE SUPPORT FUNCTIONS                        *
 *****************************************************/

t_argstate *ArgStateNew(int nargs)
{
  t_argstate *ret = Malloc(sizeof(t_argstate));
  int i;

  ret->nargs = nargs;
  ret->val = Calloc(nargs, sizeof(t_address));
  ret->tag = Calloc(nargs, sizeof(t_reloc *));
  ret->vlevel = Calloc(nargs, sizeof(t_lattice_level));
  ret->tlevel = Calloc(nargs, sizeof(t_lattice_level));
  for (i = 0; i < nargs; ++i)
    ret->vlevel[i] = ret->tlevel[i] = CP_TOP;
  return ret;
}

void ArgStateFree(t_argstate *as)
{
  Free(as->val);
  Free(as->tag);
  Free(as->vlevel);
  Free(as->tlevel);
  Free(as);
}

void ArgStateDup(t_argstate *dest, t_argstate *src)
{
  int nargs = dest->nargs;

  ASSERT(dest->nargs == src->nargs, ("argstates of different size"));
  memcpy(dest->val, src->val, nargs*sizeof(t_address));
  memcpy(dest->tag, src->tag, nargs*sizeof(t_reloc *));
  memcpy(dest->vlevel, src->vlevel, nargs*sizeof(t_lattice_level));
  memcpy(dest->tlevel, src->tlevel, nargs*sizeof(t_lattice_level));
}

t_bool ArgStateJoin(t_argstate *dest, t_argstate *src)
{
  int nargs = dest->nargs;
  t_bool change = FALSE;
  int i;

  for (i = 0; i < nargs; ++i)
  {
    if (dest->vlevel[i] != CP_BOT && src->vlevel[i] != CP_TOP)
    {
      if (src->vlevel[i] == CP_BOT)
      {
	change = TRUE;
	dest->vlevel[i] = CP_BOT;
      }
      else if (dest->vlevel[i] == CP_TOP)
      {
	change = TRUE;
	dest->vlevel[i] = src->vlevel[i];
	dest->val[i] = src->val[i];
      }
      else
      {
	/* both CP_VALUE */
	if (!AddressIsEq (dest->val[i], src->val[i]))
	{
	  change = TRUE;
	  dest->vlevel[i] = CP_BOT;
	}
      }
    }

    if (dest->tlevel[i] != CP_BOT && src->tlevel[i] != CP_TOP)
    {
      if (src->tlevel[i] == CP_BOT)
      {
	change = TRUE;
	dest->tlevel[i] = CP_BOT;
      }
      else if (dest->tlevel[i] == CP_TOP)
      {
	change = TRUE;
	dest->tlevel[i] = src->tlevel[i];
	dest->tag[i] = src->tag[i];
      }
      else
      {
	/* both CP_VALUE */
	if (dest->tag[i] != src->tag[i])
	{
	  change = TRUE;
	  dest->tlevel[i] = CP_BOT;
	}
      }
    }
  }
  return change;
}

void ArgStateSetUndefinedBot(t_argstate *as)
{
  int i;
  for (i = 0; i < as->nargs; ++i)
    if (as->vlevel[i] == CP_TOP)
      as->vlevel[i] = as->tlevel[i] = CP_BOT;
}

t_bool ArgStateSetArg(t_argstate *as, int argno, t_lattice_level vlevel, t_address val, 
    t_lattice_level tlevel, t_reloc *rel)
{
  t_bool change = FALSE;

  ASSERT(as, ("NULL argstate"));
  ASSERT(as->nargs > argno, ("setting a non-existent argument"));
  ASSERT(vlevel != CP_TOP, ("setting an argument to TOP"));  

  if (as->vlevel[argno] == CP_TOP)
  {
    as->vlevel[argno] = vlevel;
    as->val[argno] = val;
    change = TRUE;
  }
  else if (as->vlevel[argno] == CP_BOT)
  {
  }
  else
  {
    if (vlevel == CP_BOT)
    {
      as->vlevel[argno] = vlevel;
      change = TRUE;
    }
    else
    {
      if (!AddressIsEq(as->val[argno], val))
      {
	as->vlevel[argno] = CP_BOT;
	change = TRUE;
      }
    }
  }

  if (as->tlevel[argno] == CP_TOP)
  {
    if (tlevel != CP_TOP)
    {
      change = TRUE;
      as->tlevel[argno] = tlevel;
      as->tag[argno] = rel;
    }
  }
  else if (as->tlevel[argno] == CP_BOT)
  {
  }
  else
  {
    if (tlevel == CP_TOP)
    {
    }
    else if (tlevel == CP_BOT)
    {
      change = TRUE;
      as->tlevel[argno] = CP_BOT;
    }
    else if (rel != as->tag[argno])
    {
      change = TRUE;
      as->tlevel[argno] = CP_BOT;
    }
  }

  return change;
}




/* vim: set shiftwidth=2 foldmethod=marker: */
