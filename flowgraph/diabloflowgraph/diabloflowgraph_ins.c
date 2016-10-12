/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h> /* For memcopy */
#include <diabloflowgraph.h>

/* RealGetInsByNumber {{{ */
/**
 * \param filename The calling-file-name
 * \param lnno The calling-line-number
 * \param obj The object of the instruction
 * \param section The section of the instruction
 * \param ins_num the number of the instruction
 * \return a void * (= a void-casted struct ins)
 *
 * This function returns instruction ins_num in
 * a section in an object. The behaviour is different
 * for different types of sections: When we are
 * disassembling we can are indexing the
 * to_disassemble_buffer, else we are indexing a
 * buffer with all the instructions in it.*/

void *
RealGetInsByNumber (const char *filename, int lnno, t_section * section, t_uint32 ins_num)
{
  t_object *obj = SECTION_OBJECT(section);
  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);


  if ((SECTION_TYPE(section) != DISASSEMBLING_CODE_SECTION) /* Instruction is in the tmp buffer */
      && (SECTION_TYPE(section) != DISASSEMBLED_CODE_SECTION) /* instruction is in the data buffer */
      && (SECTION_TYPE(section) != FLOWGRAPHING_CODE_SECTION) /* instruction is in the data buffer */
      && (SECTION_TYPE(section) != ASSEMBLING_CODE_SECTION)) /* instruction is in the data buffer */
    FATAL(("Instruction number not supported in fase %c", SECTION_TYPE(section)));

  if (ins_num >= SECTION_NINS(section))
  {
    printf ("Num %d nins %d\n", ins_num, SECTION_NINS(section));
    FATAL(("Instruction not yet allocated at line %d in file %s!", lnno, filename));
    return NULL;
  }
  return ((SECTION_TYPE(section) == DISASSEMBLING_CODE_SECTION) ? (((char *) SECTION_TMP_BUF(section)) + (ins_num * desc->decoded_instruction_size)) : (((char *) SECTION_DATA(section)) + (ins_num * desc->decoded_instruction_size)));
}

/*}}}*/
/* InsNewForSec {{{ */
t_ins *
InsNewForSec (t_section * sec)
{

  if ((SECTION_TYPE(sec) != DISASSEMBLING_CODE_SECTION) && (SECTION_TYPE(sec) != FLOWGRAPHING_CODE_SECTION) && (SECTION_TYPE(sec) != DEFLOWGRAPHING_CODE_SECTION) && (SECTION_TYPE(sec) != FLOWGRAPHED_CODE_SECTION))
  {
    FATAL(("Cannot allocate instructions in fase %c", SECTION_TYPE(sec)));
  }

  if ((SECTION_TYPE(sec) == DISASSEMBLING_CODE_SECTION) || (SECTION_TYPE(sec) == DISASSEMBLED_CODE_SECTION))
  {
    t_uint32 nins;

    SECTION_SET_NINS(sec, SECTION_NINS(sec) + 1);
    
    nins = SECTION_NINS(sec);

    if (nins > SECTION_TMP(sec)) 
      FATAL(("We did not provide enough room to store all instructions! This probably means the architecture description (instruction_min_size) is wrong!"));


    InsInit (AINS(obj, sec, nins - 1), OBJECT_CFG(SECTION_OBJECT(sec)));

    INS_SET_PHASE(AINS(obj, sec, nins - 1),GetDiabloPhase());

    if (nins > 1)
    {
      INS_SET_IPREV(AINS(obj, sec, nins - 1), AINS(obj, sec, nins - 2));
      INS_SET_INEXT(AINS(obj, sec, nins - 2), AINS(obj, sec, nins - 1));
      INS_SET_INEXT(AINS(obj, sec, nins - 1), NULL);
    }
    else
    {
      INS_SET_IPREV(AINS(obj, sec, nins - 1), NULL);
      INS_SET_INEXT(AINS(obj, sec, nins - 1), NULL);
    }

    INS_SET_SECTION(T_INS(AINS(obj, sec, nins - 1)), sec);

    INS_SET_REFERS_TO(AINS(obj, sec, nins - 1), NULL);
    INS_SET_REFED_BY(AINS(obj, sec, nins - 1), NULL);
    INS_SET_REFED_BY_SYM(AINS(obj, sec, nins - 1), NULL);
    INS_SET_COPY(AINS(obj, sec, nins - 1), NULL);
    return AINS(obj, sec, nins - 1);
  }
  else if (SECTION_TYPE(sec) == FLOWGRAPHING_CODE_SECTION || SECTION_TYPE(sec) == FLOWGRAPHED_CODE_SECTION)
  {
    t_ins *ret = Calloc (1, CFG_DESCRIPTION(OBJECT_CFG(SECTION_OBJECT(sec)))->decoded_instruction_size);

    InsInit (ret, OBJECT_CFG(SECTION_OBJECT(sec)));
    INS_SET_PHASE(ret,GetDiabloPhase());
    INS_SET_CSIZE(ret, AddressNewForSection(sec,
      CFG_DESCRIPTION(OBJECT_CFG(SECTION_OBJECT(sec)))->minimal_encoded_instruction_size / 8));
    INS_SET_RELOCATABLE_TYPE(ret, RT_INS);
    INS_SET_SECTION(T_INS(ret), sec);
    return ret;
  }
  else if (SECTION_TYPE(sec) == DEFLOWGRAPHING_CODE_SECTION)
  {
    t_ins *ret = Calloc (1, CFG_DESCRIPTION(OBJECT_CFG(SECTION_OBJECT(sec)))->decoded_instruction_size);

    InsInit (ret, OBJECT_CFG(SECTION_OBJECT(sec)));
    INS_SET_PHASE(ret,GetDiabloPhase());
    INS_SET_CSIZE(ret, AddressNewForSection(sec,
      CFG_DESCRIPTION(OBJECT_CFG(SECTION_OBJECT(sec)))->minimal_encoded_instruction_size / 8));
    INS_SET_RELOCATABLE_TYPE(ret, RT_INS);
    INS_SET_SECTION(T_INS(ret), sec);
    return ret;
  }
  return NULL; /* keep the compiler happy */
}

/*}}}*/
/* InsNewForCfg {{{ */
t_ins *
InsNewForCfg (t_cfg * cfg)
{
  t_ins *ret = Calloc (1, CFG_DESCRIPTION(cfg)->decoded_instruction_size);

  InsInit (ret, cfg);
  INS_SET_PHASE(ret,GetDiabloPhase());
  INS_SET_SECTION(ret, OBJECT_CODE(CFG_OBJECT(cfg))[0]);
  INS_SET_CSIZE (ret, AddressNewForCfg(cfg,
                   CFG_DESCRIPTION(cfg)->minimal_encoded_instruction_size / 8));
  INS_SET_OLD_ADDRESS(ret,AddressNullForCfg(cfg));
  INS_SET_OLD_SIZE(ret,AddressNullForCfg(cfg));
  INS_SET_CADDRESS(ret,AddressNullForCfg(cfg));
  INS_SET_RELOCATABLE_TYPE(ret, RT_INS);
  return ret;
}

/*}}}*/
/* InsNewForBbl {{{ */
t_ins *
InsNewForBbl (t_bbl * bbl)
{
  t_section *sec = OBJECT_CODE(CFG_OBJECT(BBL_CFG(bbl)))[0];
  t_ins *ret = Calloc (1, CFG_DESCRIPTION(BBL_CFG(bbl))->decoded_instruction_size);

  InsInit (ret, BBL_CFG(bbl));
  INS_SET_PHASE(ret,GetDiabloPhase());
  INS_SET_SECTION(ret, sec);
  INS_SET_BBL(ret, bbl);
  INS_SET_CSIZE(ret, AddressNewForBbl(bbl,
          CFG_DESCRIPTION(BBL_CFG(bbl))->minimal_encoded_instruction_size / 8));
                                       
  INS_SET_OLD_ADDRESS(ret,AddressNullForBbl(bbl));
  INS_SET_OLD_SIZE(ret,AddressNullForBbl(bbl));
  INS_SET_CADDRESS(ret,AddressNullForBbl(bbl));
  INS_SET_RELOCATABLE_TYPE(ret, RT_INS);
  return ret;
}

/*}}}*/

void
InsFreeAllReferedRelocsBase (t_ins * ins)
{
  while (INS_REFERS_TO(ins))
  {
    t_reloc_ref *tmp = RELOC_REF_NEXT(INS_REFERS_TO(ins));

    Free (INS_REFERS_TO(ins));
    INS_SET_REFERS_TO(ins, tmp);
  }
  while (INS_REFED_BY(ins))
  {
    t_reloc_ref *tmp = RELOC_REF_NEXT(INS_REFED_BY(ins));

    Free (INS_REFED_BY(ins));
    INS_SET_REFED_BY(ins, tmp);
  }
}

void
InsFreeReferedRelocs (t_ins * ins)
{
  t_reloc_ref *iter = INS_REFERS_TO(ins);
  t_reloc_ref *itern;

  while (iter)
  {
    itern = RELOC_REF_NEXT(iter);
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(RELOC_REF_RELOC(INS_REFERS_TO(ins)))) == RT_INS)
      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), RELOC_REF_RELOC(INS_REFERS_TO(ins)));
    else
      FATAL(("Illegal relocation for instruction!"));
    iter = itern;
  }

  while (INS_REFED_BY_SYM(ins))
  {
	  SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(INS_REFED_BY_SYM(ins)->sym), INS_REFED_BY_SYM(ins)->sym);
  }
}

/* InsKill {{{ */
void
InsKill (t_ins * ins)
{
  if (INS_INEXT(ins))
  {
    INS_SET_IPREV(INS_INEXT(ins), INS_IPREV(ins));
  }
  else
  {
    BBL_SET_INS_LAST(INS_BBL(ins), INS_IPREV(ins));
  }
  if (INS_IPREV(ins))
  {
    INS_SET_INEXT(INS_IPREV(ins), INS_INEXT(ins));
  }
  else
  {
    BBL_SET_INS_FIRST(INS_BBL(ins), INS_INEXT(ins));
  }

  BBL_SET_NINS(INS_BBL(ins), BBL_NINS(INS_BBL(ins)) - 1);

  InsFree (ins);
}

/*}}}*/

/*!
 * \brief Check if the instruction has a side effect
 *
 * Architecture-independent wrapper around the architecture specific callbacks
 * for InsHasSideEffect
 *
 * \param ins The instruction to check
 *
 * \return t_bool TRUE if the instruction has a side effect, FALSE otherwise
 */

t_bool
InsHasSideEffect (t_ins * ins)
{
  const t_architecture_description *desc;

  if (INS_BBL(ins))
    desc = CFG_DESCRIPTION(BBL_CFG(INS_BBL(ins)));
  else
    FATAL(("InsHasSideEffect called without an architecture description"));

  return desc->InsHasSideEffect (ins);
}

void
InsMakeNoop (t_ins * ins)
{
  const t_architecture_description *desc;

  if (INS_BBL(ins))
    desc = CFG_DESCRIPTION(BBL_CFG(INS_BBL(ins)));
  else
    FATAL(("InsMakeNoop called without an architecture description"));

  desc->InsMakeNoop (ins);
}

t_ins *ObjectGetInsByAddress(t_object *obj, t_address addr)
{
  t_uint32 i;
  t_section *sec;

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    if (AddressIsGe(addr, SECTION_CADDRESS(sec)) &&
        AddressIsLt(addr, AddressAdd(SECTION_CADDRESS(sec), SECTION_CSIZE(sec))))
      return SecGetInsByAddress(sec, addr);
  }
  return NULL;
}

/* XXX this function no longer cares about bundle size, so if you want
 * to use it for the itanium beware! */
t_ins *
SecGetInsByAddress (t_section * sec, t_address addr)
{
  t_uint32 offset;
  t_ins *ins = NULL;

  if (AddressIsLt (addr, SECTION_CADDRESS (sec)) ||
      AddressIsGe (addr, SECTION_END_ADDRESS (sec)))
    return NULL;

  offset = AddressExtractUint32 (AddressSub (addr, SECTION_CADDRESS (sec)));
  ins = ((t_ins **) SECTION_ADDRESS_TO_INS_MAP(sec))[offset];
  while (ins == NULL && offset >= 1)
    ins = ((t_ins **) SECTION_ADDRESS_TO_INS_MAP(sec))[--offset];
  return ins;
}

t_ins *
InsFindMovableDownEqualInsInBbl (t_ins * search, t_bbl * bbl)
{
  t_bool ins_can_move;
  t_ins *j_ins;
  t_bool load_seen = FALSE;
  t_bool store_seen = FALSE;
  t_regset use2 = RegsetNew (), def2 = RegsetNew (), tmp = RegsetNew ();
  const t_architecture_description *desc;

  desc = CFG_DESCRIPTION(BBL_CFG(bbl));

  if (!desc->InsCmp)
    return NULL;
  BBL_FOREACH_INS_R(bbl, j_ins)
  {
    ins_can_move = TRUE;
    if (desc->InsIsSystem(j_ins)) /* e.g. cli on i386 */
      ins_can_move = FALSE;
    if (desc->InsIsLoad(j_ins))
      if (store_seen)
        ins_can_move=FALSE;
    if (desc->InsIsStore(j_ins))
      if (store_seen||load_seen)
        ins_can_move=FALSE;
    if (desc->InsIsLoad(j_ins))
      load_seen=TRUE;
    if (desc->InsIsStore(j_ins))
      store_seen=TRUE;
    RegsetSetDup (tmp, INS_REGS_DEF(j_ins));
    RegsetSetIntersect (tmp, use2);
    if (!RegsetIsEmpty (tmp))
      ins_can_move = FALSE;
    RegsetSetDup (tmp, INS_REGS_USE(j_ins));
    RegsetSetIntersect (tmp, def2);
    if (!RegsetIsEmpty (tmp))
      ins_can_move = FALSE;
    RegsetSetUnion (use2, INS_REGS_USE(j_ins));
    RegsetSetUnion (def2, INS_REGS_DEF(j_ins));
    RegsetSetUnion (use2, INS_REGS_DEF(j_ins));

    DiabloBrokerCall("InsCanMoveDown", j_ins, &ins_can_move);

    if (!ins_can_move)
      continue;
    if (desc->InsCmp (search, j_ins))
    {
      return j_ins;
    }
  }
  return NULL;
}

t_ins *
InsFindMovableUpEqualInsInBbl (t_ins * search, t_bbl * bbl)
{
  t_bool ins_can_move;
  t_ins *j_ins;
  t_bool load_seen = FALSE;
  t_bool store_seen = FALSE;
  t_regset use2 = RegsetNew (), def2 = RegsetNew (), tmp = RegsetNew ();
  const t_architecture_description *desc;

  desc = CFG_DESCRIPTION(BBL_CFG(bbl));

  if (!desc->InsCmp)
    return NULL;
  BBL_FOREACH_INS(bbl, j_ins)
  {
    ins_can_move = TRUE;
    if (desc->InsIsSystem(j_ins)) /* e.g. cli on i386 */
      ins_can_move = FALSE;
    if (desc->InsIsLoad(j_ins))
      if (store_seen)
        ins_can_move=FALSE;
    if (desc->InsIsStore(j_ins))
      if (store_seen||load_seen)
        ins_can_move=FALSE;
    if (desc->InsIsLoad(j_ins))
      load_seen=TRUE;
    if (desc->InsIsStore(j_ins))
      store_seen=TRUE;
    RegsetSetDup (tmp, INS_REGS_DEF(j_ins));
    RegsetSetIntersect (tmp, use2);
    if (!RegsetIsEmpty (tmp))
      ins_can_move = FALSE;
    RegsetSetDup (tmp, INS_REGS_USE(j_ins));
    RegsetSetIntersect (tmp, def2);
    if (!RegsetIsEmpty (tmp))
      ins_can_move = FALSE;
    RegsetSetUnion (use2, INS_REGS_USE(j_ins));
    RegsetSetUnion (def2, INS_REGS_DEF(j_ins));
    RegsetSetUnion (use2, INS_REGS_DEF(j_ins));

    DiabloBrokerCall("InsCanMoveUp", j_ins, &ins_can_move);

    if (!ins_can_move)
      continue;
    if (desc->InsCmp (search, j_ins))
    {
      return j_ins;
    }
  }
  return NULL;
}

t_regset
InsRegsLiveAfter (t_ins * ins)
{
  if (!INS_INEXT(ins)) 
    return BBL_REGS_LIVE_OUT (INS_BBL (ins));
  return InsRegsLiveBefore(INS_INEXT(ins));
}

t_regset
InsRegsLiveBefore (t_ins * ins)
{
  t_ins *i_ins;

  t_bbl *bbl = INS_BBL(ins);
  t_regset live = RegsetNew ();

  RegsetSetDup (live, BBL_REGS_LIVE_OUT(bbl));

  BBL_FOREACH_INS_R(bbl, i_ins)
  {
    if (!INS_IS_CONDITIONAL(i_ins))
      RegsetSetDiff (live, INS_REGS_DEF (i_ins));
    RegsetSetUnion (live, INS_REGS_USE (i_ins));
    
    if (i_ins == ins) break;
  }
  if(!INS_IPREV(ins))
    RegsetSetDiff(live, BBL_REGS_NEVER_LIVE(bbl));

#ifdef DEBUG_LIVENESS
  if (!i_ins)
    FATAL(("ins is not in his own basic block!"));
#endif

  return live;
}

/** Check if register r is not modified in the bbl after instruction
  ins */
t_bool
InsRegUnmodifiedAfter (t_reg reg, t_ins * ins)
{

  t_ins *i_ins = ins;

  for (i_ins = INS_INEXT(i_ins); i_ins != NULL; i_ins = INS_INEXT(i_ins))
    if (RegsetIn (INS_REGS_DEF(i_ins), reg))
      return FALSE;

  return TRUE;
}

t_regset
InsRegsDefBefore (t_ins * ins)
{
  t_regset def;
  t_function *fun = BBL_FUNCTION(INS_BBL(ins));

  if (FUNCTION_IS_HELL(fun))
  {
    FATAL(("implement"));
  }

  def = RegsetNew ();

  for (ins = INS_IPREV(ins); ins != NULL; ins = INS_IPREV(ins))
  {
    RegsetSetUnion (def, INS_REGS_DEF(ins));
  }

  return def;
}

static t_inslist *delayed_ins_kill_list;
static t_inslist *delayed_ins_free_list;

void
InsKillDelayed (t_ins * ins)
{
  t_inslist *new_entry = Malloc (sizeof (t_inslist));

  new_entry->ins = ins;
  new_entry->next = delayed_ins_kill_list;
  delayed_ins_kill_list = new_entry;
}

void
InitDelayedInsKilling ()
{
  delayed_ins_kill_list = NULL;
}

void
ApplyDelayedInsKilling ()
{
  t_inslist *iterator = delayed_ins_free_list;

  while (iterator)
  {
    InsFreeReferedRelocs (iterator->ins);
    iterator = iterator->next;
    Free (delayed_ins_free_list);
    delayed_ins_free_list = iterator;
  }

  iterator = delayed_ins_kill_list;

  while (iterator)
  {
    InsKill (iterator->ins);
    iterator = iterator->next;
    Free (delayed_ins_kill_list);
    delayed_ins_kill_list = iterator;
  }
}

void
InsFreeReferedRelocsDelayed (t_ins * ins)
{
  t_inslist *new_entry = Malloc (sizeof (t_inslist));

  new_entry->ins = ins;
  new_entry->next = delayed_ins_free_list;
  delayed_ins_free_list = new_entry;
}


t_bool
GenericInsIsLoad (t_ins * ins)
{
  return (INS_TYPE(ins) == IT_LOAD || INS_TYPE(ins) == IT_LOAD_MULTIPLE || INS_TYPE(ins) == IT_FLT_LOAD );
}

t_bool
GenericInsIsStore (t_ins * ins)
{
  return (INS_TYPE(ins) == IT_STORE || INS_TYPE(ins) == IT_STORE_MULTIPLE || INS_TYPE(ins) == IT_FLT_STORE );
}

void
InsSwap (t_ins * first_ins, t_ins * sec_ins)
{
  t_ins *prev_ins;
  t_ins *next_ins;

  if (INS_BBL(first_ins) != INS_BBL(sec_ins))
    FATAL(("Not allowed to change place of 2 instructions from a differt BBL"));

  if (INS_IPREV(first_ins))
    prev_ins = INS_IPREV(first_ins);
  else
    prev_ins = NULL;

  if (INS_INEXT(sec_ins))
    next_ins = INS_INEXT(sec_ins);
  else
    next_ins = NULL;

  if (prev_ins)
    INS_SET_INEXT(prev_ins, sec_ins);
  if (next_ins)
    INS_SET_IPREV(next_ins, first_ins);

  INS_SET_IPREV(sec_ins, prev_ins);
  INS_SET_INEXT(sec_ins, first_ins);
  INS_SET_IPREV(first_ins, sec_ins);
  INS_SET_INEXT(first_ins, next_ins);

  if (BBL_INS_FIRST(INS_BBL(first_ins)) == first_ins)
    BBL_SET_INS_FIRST(INS_BBL(first_ins), sec_ins);

  if (BBL_INS_LAST(INS_BBL(sec_ins)) == sec_ins)
    BBL_SET_INS_LAST(INS_BBL(sec_ins), first_ins);

}
/* Functions to insert instructions in bbls {{{ */
/* {{{ Append an instruction at the end of a bbl */
void
InsAppendToBbl (t_ins * ins, t_bbl * bbl)
{
  if (INS_IPREV(ins) || INS_INEXT(ins))
    FATAL(("Already in bbl!\n"));
  INS_SET_BBL(ins, bbl);

  INS_SET_ATTRIB(ins, (INS_ATTRIB(ins) | IF_IN_BBL));

  if (!BBL_INS_FIRST(bbl))
  {
    if (BBL_NINS(bbl) != 0)
    {
      VERBOSE(0, ("@B\n", bbl));
    }
    ASSERT(!BBL_NINS(bbl), ("No instructions, yet ins count != 0"));
    BBL_SET_INS_FIRST(bbl, ins);
    BBL_SET_INS_LAST(bbl, ins);
    BBL_SET_NINS(bbl, 1);
    INS_SET_INEXT(ins, NULL);
    INS_SET_IPREV(ins, NULL);
    BBL_SET_CSIZE(bbl, INS_CSIZE(ins));
  }
  else
  {
    INS_SET_INEXT(BBL_INS_LAST(bbl), ins);
    INS_SET_IPREV(ins, BBL_INS_LAST(bbl));
    INS_SET_INEXT(ins, NULL);
    BBL_SET_INS_LAST(bbl, ins);
    BBL_SET_CSIZE(bbl, AddressAdd (BBL_CSIZE(bbl), INS_CSIZE(ins)));
    BBL_SET_NINS(bbl, BBL_NINS(bbl) + 1);
  }
}
/* }}}*/
/* {{{ Prepend an instruction to the front of a bbl */
void
InsPrependToBbl (t_ins * ins, t_bbl * bbl)
{
  if (INS_IPREV(ins) || INS_INEXT(ins))
    FATAL(("Already in bbl!\n"));
  INS_SET_REFED_BY(ins, NULL);

  INS_SET_BBL(ins, bbl);
  INS_SET_INEXT(ins, BBL_INS_FIRST(bbl));
  if (BBL_INS_FIRST(bbl))
    INS_SET_IPREV(BBL_INS_FIRST(bbl), ins);
  INS_SET_IPREV(ins, NULL);
  BBL_SET_INS_FIRST(bbl, ins);
  if (!BBL_INS_LAST(bbl))
    BBL_SET_INS_LAST(bbl, ins);
  BBL_SET_NINS(bbl, BBL_NINS(bbl) + 1);
  BBL_SET_CSIZE(bbl, AddressAdd (BBL_CSIZE(bbl), INS_CSIZE(ins)));
}
/* }}}*/
/* {{{ Insert instruction after */
void
InsInsertAfter (t_ins * ins, t_ins * after)
{
  INS_SET_BBL(ins, INS_BBL(after));
  INS_SET_ATTRIB(ins, (INS_ATTRIB(ins) | IF_IN_BBL));

  if (INS_INEXT(after))
    INS_SET_IPREV(INS_INEXT(after), ins);
  INS_SET_INEXT(ins, INS_INEXT(after));
  INS_SET_IPREV(ins, after);
  INS_SET_INEXT(after, ins);

  if (BBL_INS_LAST(INS_BBL(after)) == after)
    BBL_SET_INS_LAST(INS_BBL(after), ins);

  BBL_SET_NINS(INS_BBL(ins), BBL_NINS(INS_BBL(ins)) + 1);
  BBL_SET_CSIZE(INS_BBL(ins), AddressAdd (BBL_CSIZE(INS_BBL(ins)), INS_CSIZE(ins)));
}
/*}}}*/
/* {{{ Insert instruction before */
void
InsInsertBefore (t_ins * ins, t_ins * before)
{
  INS_SET_BBL(ins, INS_BBL(before));
  INS_SET_ATTRIB(ins, (INS_ATTRIB(ins) | IF_IN_BBL));

  if (INS_IPREV(before))
    INS_SET_INEXT(INS_IPREV(before), ins);
  INS_SET_IPREV(ins, INS_IPREV(before));
  INS_SET_INEXT(ins, before);
  INS_SET_IPREV(before, ins);

  if (BBL_INS_FIRST(INS_BBL(before)) == before)
    BBL_SET_INS_FIRST(INS_BBL(before), ins);

  BBL_SET_NINS(INS_BBL(ins), BBL_NINS(INS_BBL(ins)) + 1);
  BBL_SET_CSIZE(INS_BBL(ins), AddressAdd (BBL_CSIZE(INS_BBL(ins)), INS_CSIZE(ins)));
}
/*}}}*/
/*}}}*/

/* {{{ dump listing of disassembled program */
/* this is only here because it should be in object/ but it cannot be there
 * because it depends on the existence of the t_ins type */
void ObjectDumpDisassembledCode (t_object *obj, t_string dumpname)
{
  t_ins *ins;
  t_section *sec;
  t_uint32 i;
  FILE *f = fopen(dumpname, "w");
  if (f)
  {
    OBJECT_FOREACH_CODE_SECTION (obj, sec, i)
    {
      fprintf(f, "===========[%s]==========\n", SECTION_NAME (sec));
      if (SECTION_TYPE (sec) != DISASSEMBLED_CODE_SECTION)
      {
        fprintf (f, "Section is not in DISASSEMBLED state\n");
        continue;
      }
      SECTION_FOREACH_INS (sec, ins)
        FileIo (f, "@I\n", ins);
    }
    fclose (f);
  }
  else
    WARNING (("Could not open %s for writing", dumpname));
}
/* }}} */

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
