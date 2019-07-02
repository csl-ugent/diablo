/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#include <math.h>
#include <string.h>

/*#define SCHEDULE */

/*#define DEBUG_LAYOUT*/


t_graph * ArmCreateLayoutGraph(t_chain_holder *ch, t_bool *consider_cond_thumb2_branches_during_layout);
t_bool InsAddToPool(t_arm_ins * ins, t_bbl * bbl, t_int32 slack);
t_arm_ins* InsFindIndexInPool(t_arm_ins * ins);
void InsRemoveFromPool(char * file, int line, t_arm_ins * ins);
void FreePoolData();
void ScheduleCfg(t_cfg * cfg);

/*! Document */
typedef struct 
{
  t_bbl * bbl;
  int links_to_next_chain;
  int links_to_prev_chain; 
  int links_to_all_prev_chains;
  int links_to_all_next_chains;
} layouted_chain;

#define BBL_CHAIN_INDEX_INT(bbl)	((int)(long)BBL_TMP2(BBL_FIRST_IN_CHAIN(bbl)))
#define BBL_CHAIN_INDEX(bbl)		BBL_TMP2((BBL_FIRST_IN_CHAIN(bbl)))
#define BBL_SET_CHAIN_INDEX(bbl,x)		BBL_SET_TMP2((BBL_FIRST_IN_CHAIN(bbl)),x)
#define BblInChainHolder(bbl,ch)	  \
  ((BBL_CHAIN_INDEX_INT(bbl) < (ch)->nchains) \
  && ((ch)->chains[BBL_CHAIN_INDEX_INT(bbl)] == BBL_FIRST_IN_CHAIN(bbl)))

#define BBL_CHAIN_NODE(bbl)	(BBL_TMP(bbl))
#define BBL_SET_CHAIN_NODE(bbl,x)	(BBL_SET_TMP(bbl,x))
#define BBL_CHAIN_SIZE(bbl)	AddressAdd(BBL_CADDRESS(BBL_LAST_IN_CHAIN(bbl)),BBL_CSIZE(BBL_LAST_IN_CHAIN(bbl)))
#define BBL_DIST_FROM_END(bbl)	AddressSub(BBL_CHAIN_SIZE(bbl),BBL_CADDRESS(bbl))
#define ARM_INS_DIST_FROM_END(ins)	AddressSub(BBL_CHAIN_SIZE(ARM_INS_BBL(ins)),ARM_INS_CADDRESS(ins))

#define MAX_OFFSET_FORWARD	1028
#define MAX_OFFSET_BACKWARD	1020
#define MAX_LOAD_OFFSET_FORWARD		(4092 + 8)
#define MAX_LOAD_OFFSET_BACKWARD	(4092 - 8 - 4)

#define HOTNESS_BONUS		100
#define HOTNESS_THRESHOLD	0.95

t_int32 worst_case_alignment = -1;

t_section **exidx_subsection_order = NULL;
t_uint32 exidx_subsection_count = 0;

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(cluster);
BBL_DYNAMIC_MEMBER(exidx_section, EXIDX_SECTION, ExidxSection, t_section *, NULL);

/*!
 * \todo Document
 *
 * \param ins
 *
 * \return t_uint32 
*/
/* ConstProdGuessNumberOfInstructionsNeeded {{{ */
#if 0
static t_uint32 ConstProdGuessNumberOfInstructionsNeeded(t_arm_ins * ins, t_uint32 min_addr, t_uint32 max_addr)
{
  t_int32 val = ARM_INS_IMMEDIATE(ins);
  t_int32 val_neg = -val;
  t_int32 val_invert = ~val;
  t_int32 val_diff, val_abs;
  t_uint32 uval, max_offset = 0;

  t_arm_ins * j_ins;
  t_bbl * bbl = ARM_INS_BBL(ins);

  t_uint32 a;
  t_uint32 i;

  if(min_addr)
  {
    uval = ARM_INS_IMMEDIATE(ins);
    if (uval <= min_addr) 
      max_offset = max_addr - uval;
    else if(uval >= max_addr) 
      max_offset = uval - min_addr;
    else
      max_offset = (uval - min_addr) > (max_addr - uval) ? (uval - min_addr) : (max_addr - uval);

  }
  /* first we try to generate the constant from scratch in one instruction */
  {
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val, i)) <= 0xff)
      {
	return 1;
      }

    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_neg-1, i)) <= 0xff)
      {
	return 1; 
      }

    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_invert, i)) <= 0xff)
      {
	return 1;
      }
  }

  /* Maybe there is another constantproducer preceding this one, that we can use?*/
  BBL_FOREACH_ARM_INS(bbl,j_ins)
    if(ARM_INS_OPCODE(j_ins) == ARM_CONSTANT_PRODUCER && ARM_INS_CONDITION(j_ins) == ARM_CONDITION_AL && j_ins != ins)
    {
      t_arm_ins * r_ins = ARM_INS_INEXT(j_ins);
      while(r_ins != ins)
      {
	if(RegsetIn(ARM_INS_REGS_DEF(r_ins),ARM_INS_REGA(j_ins))) break;
	r_ins = ARM_INS_INEXT(r_ins);
      }
      
      if(r_ins == ins)
      {
	val_abs = val_diff = ARM_INS_IMMEDIATE(j_ins) - val;
	if(val_diff < 0) val_abs = -val_diff; 

	for (i = 0; i < 32; i += 2)
	  if ((a = Uint32RotateLeft (val_abs, i)) <= 0xff)
	  {
#ifdef DEBUG_LAYOUT
	    VERBOSE(0,("Using a previous constant producer, @I, to produce @I",j_ins,ins));
#endif
	    /* effectively change the instruction, to remove the dependency from the constprod in
	     * j_ins, which may end up in another block when data pools are created */
	    if(val_diff < 0)
	    {
	      ArmInsMakeAdd(ins,ARM_INS_REGA(ins),ARM_INS_REGA(j_ins),ARM_REG_NONE,val_abs,ARM_INS_CONDITION(ins));
	    }
	    else
	    {
	      ArmInsMakeSub(ins,ARM_INS_REGA(ins),ARM_INS_REGA(j_ins),ARM_REG_NONE,val_abs,ARM_INS_CONDITION(ins));
	    }
	    return 1;
	  }
      }
    }
    else if (j_ins == ins)
      break;
  
  return 0;
}
#endif
/*}}}*/
/*!
 * \todo Document 
 *
 * \param ins
 *
 * \return void 
*/
/*!
 * \todo Document
 *
 * \param ins
 *
 * \return void 
*/
/*GenerateInstructionsForConstProd {{{ */

void GenerateInstructionsForConstProdIfPossible(t_arm_ins *ins, t_int32 val)
{
  t_uint32 a;
  t_uint32 i;

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
  {

    if ((val >= 0) &&  (val <= 255))
    {
      /* TODO: why are the condition_bits computed? they are not used ... */
        t_regset condition_bits =RegsetNew();
        RegsetSetAddReg(condition_bits,ARM_REG_Z_CONDITION);
        RegsetSetAddReg(condition_bits,ARM_REG_N_CONDITION);
        RegsetSetIntersect(condition_bits,InsRegsLiveAfter(T_INS(ins)));

        if (!ArmInsIsInITBlock(ins) && ((ARM_INS_FLAGS(ins) & FL_S) || RegsetIsEmpty(condition_bits)))
          {
            //            DEBUG(("converted @I",ins));
            ArmInsMakeMov(ins, ARM_INS_REGA(ins), ARM_REG_NONE, val, ARM_INS_CONDITION(ins));
            ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
            //            DEBUG(("1    into @I",ins));
            return;
          }

        if (ArmInsIsInITBlock(ins) && (!(ARM_INS_FLAGS(ins) & FL_S)))
          {
            //            DEBUG(("converted @I",ins));
            ArmInsMakeMov(ins, ARM_INS_REGA(ins), ARM_REG_NONE, val, ARM_INS_CONDITION(ins));
            ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
            //            DEBUG(("2    into @I",ins));
            return;
          }
      }

    if ((val > 0 && ((val & 0x0000ffff)==val)) && diabloarm_options.fullthumb2)
      {
        //        DEBUG(("converted @I",ins));
        ArmInsMakeMovwImmed(ins,ARM_INS_REGA(ins),val,ARM_INS_CONDITION(ins));
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
        //        DEBUG(("3    into @I",ins));
        return;
      }
    return;
  }

  /* Try it in one instruction from scratch */
  {
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val, i)) <= 0xff)
      {
        ArmInsMakeMov(ins, ARM_INS_REGA(ins), ARM_REG_NONE, val, ARM_INS_CONDITION(ins));
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
        return;
      }

    t_int32 val_invert = ~val;
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_invert, i)) <= 0xff)
      {
        ArmInsMakeMvn(ins, ARM_INS_REGA(ins), ARM_REG_NONE, val_invert, ARM_INS_CONDITION(ins));
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
        return;
      }

    t_int32 val_neg = -val;
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_neg-1, i)) <= 0xff)
      {
        ArmInsMakeMvn(ins, ARM_INS_REGA(ins), ARM_REG_NONE, val_neg -1, ARM_INS_CONDITION(ins));
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
        return;
      }
  }

  /* TODO: this assumes we are always on ARMv6-ARMv7 if not in Thumb */
  if (val > 0 && ((val & 0x0000ffff)==val))
    {
      ArmInsMakeMovwImmed(ins,ARM_INS_REGA(ins),val,ARM_INS_CONDITION(ins));
      ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
      return;
    }
}

/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 *
 * \return void 
*/
/*GenerateInstructionsForFloatProd {{{ */
static void GenerateInstructionsForFloatProd(t_arm_ins *ins)
{
  char * data=ARM_INS_DATA(ins);
  t_arm_ins * extra_ins = ARM_INS_INEXT(ins);
  t_arm_ins * data_ins = ARM_INS_INEXT(extra_ins);

  if (!extra_ins) 
    FATAL(("No extra ins!"));
  if (!ArmInsIsNOOP(extra_ins))
    FATAL(("no NOOP 8\n"));

  if (!data_ins) 
    FATAL(("No data ins!"));
  if (!ArmInsIsNOOP(data_ins))
    FATAL(("no NOOP 9\n"));

  if (ARM_INS_OPCODE(ins) == ARM_FLOAT_PRODUCER)      
    ARM_INS_SET_OPCODE(ins,  ARM_LDF);
  else
  {
    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      //ARM_INS_SET_OPCODE(ins,  ARM_FLDD);
      ARM_INS_SET_OPCODE(ins,  ARM_VLDR);
    else
      //ARM_INS_SET_OPCODE(ins,  ARM_FLDS);
      ARM_INS_SET_OPCODE(ins,  ARM_VLDR);
  }
  ARM_INS_SET_REGB(ins,  15);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  0);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED | FL_PREINDEX | FL_DIRUP);
  ARM_INS_SET_TYPE(ins,  IT_FLT_LOAD);
  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));

  ARM_INS_SET_OPCODE(extra_ins,  ARM_B);
  ARM_INS_SET_REGA(extra_ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(extra_ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(extra_ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(extra_ins,  0);
  ARM_INS_SET_FLAGS(extra_ins,  FL_IMMED);
  ARM_INS_SET_TYPE(extra_ins,  IT_BRANCH);
  ARM_INS_SET_CONDITION(extra_ins,  ARM_CONDITION_AL);
  ARM_INS_SET_REGS_USE(extra_ins,  ArmUsedRegisters(extra_ins));
  ARM_INS_SET_REGS_DEF(extra_ins,  ArmDefinedRegisters(extra_ins));

  ARM_INS_SET_OPCODE(data_ins,  ARM_DATA);
  ARM_INS_SET_TYPE(data_ins,  IT_DATA);
  ARM_INS_SET_IMMEDIATE(data_ins,  *((t_uint32 *) data));

  if ((ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE) || (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE) || (ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(ins)&FL_FLT_PACKED))
  {
    data+=4;
    ARM_INS_SET_IMMEDIATE(extra_ins, ARM_INS_IMMEDIATE(extra_ins)+ 4);
    data_ins=ARM_INS_INEXT(data_ins);
    ARM_INS_SET_OPCODE(data_ins,  ARM_DATA);
    ARM_INS_SET_TYPE(data_ins,  IT_DATA);
    ARM_INS_SET_IMMEDIATE(data_ins,  *((t_uint32 *) data));
  }

  if ((ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(ins)&FL_FLT_PACKED))
  {
    data+=4;
    ARM_INS_SET_IMMEDIATE(extra_ins, ARM_INS_IMMEDIATE(extra_ins)+ 4);
    data_ins=ARM_INS_INEXT(data_ins);
    ARM_INS_SET_OPCODE(data_ins,  ARM_DATA);
    ARM_INS_SET_TYPE(data_ins,  IT_DATA);
    ARM_INS_SET_IMMEDIATE(data_ins,  *((t_uint32 *) data));
  }

}
/*}}}*/

/*!
 * Return the offset between a branch and its destination
 *
 * \param edge Edge between source and destination
 *
 * \return offset (in bytes) from source to destination
 */
 /*ArmCalculateBranchDisplacement{{{*/
static t_int32 ArmCalculateBranchDisplacement(t_cfg_edge *edge)
{
  t_bbl * head = CFG_EDGE_HEAD(edge);
  t_bbl * tail = CFG_EDGE_TAIL(edge);
  t_arm_ins * ins = T_ARM_INS(BBL_INS_LAST(head));
  t_uint32 implicit_disp_base, disp;

  ASSERT(!AddressIsEq(BBL_CADDRESS(head), AddressNew32(0)), ("calculation of branch displacement for edge @E failed: head has CADDRESS 0: @eB", edge, head));
  ASSERT(!AddressIsEq(BBL_CADDRESS(tail), AddressNew32(0)), ("calculation of branch displacement for edge @E failed: tail has CADDRESS 0: @eB", edge, tail));

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    implicit_disp_base=2;
  else
    implicit_disp_base=4;
  /* minus INS_CSIZE(last) because then we have the address where the jump starts
   * minus 2*implicit_disp_base because pc points 2*implicit_disp_base too far
   */
  disp = G_T_UINT32(BBL_CADDRESS(tail)) - (G_T_UINT32(BBL_CADDRESS(head)) + G_T_UINT32(BBL_CSIZE(head)) - G_T_UINT32(INS_CSIZE(BBL_INS_LAST(head)))) - 2*implicit_disp_base;

  if ((ARM_INS_OPCODE(ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(ins) == ARM_T2CBNZ))
    ASSERT((disp & 0x1) == 0, ("CBZ/CBNZ should jump to an even address @I (edge @E)", ins, edge));

  else
  {
    if (ARM_INS_OPCODE(ins) == ARM_BL
        && ((ARM_INS_FLAGS(ins) & FL_THUMB) != 0) != ArmBblIsThumb(tail))
      if (!BBL_IS_HELL(tail))
        FATAL(("BL instruction found in instruction set switch (@I)\nHead: @eiB\nTail: @eiB",ins,head,tail));

    if ((ARM_INS_OPCODE(ins) == ARM_BLX) && (ARM_INS_FLAGS(ins) & FL_THUMB))
      /* for Thumb code, alignment of the PC to 4 byte-boundaries should be considered */
      if (ARM_INS_CADDRESS(ins) % 4 == 2)
        disp += 2;
  }

  return (t_int32)disp;
}
 /*}}}*/



static void TestTbbTbhDistances(t_cfg * cfg)
{
        t_bbl * i_bbl = NULL;

        CFG_FOREACH_BBL(cfg, i_bbl)
        {
                /* skip empty BBL's */
                if (!BBL_INS_LAST(i_bbl)) continue;

                if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_T2TBB || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_T2TBH)
                {
                        t_cfg_edge * i_edge = NULL;

                        BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
                        {
                                /* calculate the distance for every edge */
                                t_int32 disp = ArmCalculateBranchDisplacement(i_edge);
                                ASSERT(disp >= 0, ("displacement should be positive, got %d 0x%x\n   @E", disp, disp, i_edge));

                                if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_T2TBB)
                                        ASSERT(disp <= 0x1fe, ("displacement too large for TBB (max 510) %d 0x%x\n   @E", disp, disp, i_edge));
                                else
                                        ASSERT(disp <= 0x1fffe, ("displacement too large for TBH (max 131070) %d 0x%x\n   @E", disp, disp, i_edge));
                        }
                }
        }
}

/*!
 * \todo Document
 *
 * \param edge
 *
 * \return void 
*/
/*ArmUpdateBranchDisplacement{{{*/
static void ArmUpdateBranchDisplacement(t_cfg_edge * edge)
{
        t_int32 disp = ArmCalculateBranchDisplacement(edge);
        t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));

        /* sanity check: verify that the new PC value is equal to the address of the target BBL */
        t_uint32 pc = G_T_UINT32(ARM_INS_CADDRESS(branch_ins));

        /* modify the PC */
        pc += ((ARM_INS_FLAGS(branch_ins) & FL_THUMB) ? 4 : 8);

        /* PC is aligned to a multiple of 4 bytes only in BLX Thumb instructions */
        if ((ARM_INS_FLAGS(branch_ins) & FL_THUMB) && (ARM_INS_OPCODE(branch_ins) == ARM_BLX))
                pc &= ~0x3;

        pc += disp;

        /* check if the calculated PC equals the expected PC */
        ASSERT(pc == BBL_CADDRESS(CFG_EDGE_TAIL(edge)), ("Calculated target of jump is %x, while it should be %x (instruction @I)", pc, BBL_CADDRESS(CFG_EDGE_TAIL(edge)), branch_ins));

        /* update the immediate value */
        ARM_INS_SET_IMMEDIATE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge))), (t_int64)disp);
}
/*}}}*/
/*
*/
/*ArmInsertNoopBblInChain{{{*/
void ArmInsertNoopBblInChain(t_cfg *cfg, t_bbl* after_bbl, t_bbl* before_bbl, t_bool thumb, t_bool add_to_function)
{
  t_arm_ins * noop;
  t_bbl *n_bbl;

  n_bbl=BblNew(cfg);
  if (add_to_function)
    BblInsertInFunction(n_bbl,BBL_FUNCTION(after_bbl));
  noop=ArmInsNewForBbl(n_bbl);
  if (thumb)
    ARM_INS_SET_FLAGS(noop, ARM_INS_FLAGS(noop) | FL_THUMB);
  ArmInsMakeNoop(noop);
  ArmInsAppendToBbl(noop,n_bbl); 
  BBL_SET_NEXT_IN_CHAIN(after_bbl, n_bbl); 
  BBL_SET_PREV_IN_CHAIN(n_bbl, after_bbl);
  BBL_SET_NEXT_IN_CHAIN(n_bbl, before_bbl); 
  BBL_SET_PREV_IN_CHAIN(before_bbl, n_bbl);
}
/*}}}*/
/*!
 * \todo Document
 *
 * \param cfg
 *
 * \return void 
*/
/* ArmRelocate {{{ */
void ArmRelocate(t_cfg * cfg, t_uint32 mode)
{
  t_reloc * reloc;
  t_bbl * i_bbl;
  t_arm_ins * i_ins;
  t_cfg_edge * i_edge, * safe;

  /* adapt the displacements encoded in direct control flow transfer
   * instructions {{{ */
  STATUS(START,("Relocating branch displacements"));
  CFG_FOREACH_EDGE_SAFE(cfg, i_edge, safe)
    if (CFG_EDGE_CAT(i_edge) == ET_JUMP || CFG_EDGE_CAT(i_edge) == ET_IPJUMP || CFG_EDGE_CAT(i_edge) == ET_CALL)
    {
      /* If the last instruction is a kind of jump and has a displacement (an immediate), update it */
      t_arm_ins *last = T_ARM_INS (BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)));
      if (last && (ARM_INS_FLAGS(last) & FL_IMMED) &&
          (ARM_INS_OPCODE(last) == ARM_B || ARM_INS_OPCODE(last) == ARM_BL || ARM_INS_OPCODE(last) == ARM_BLX
           || ARM_INS_OPCODE(last) == ARM_T2CBZ || ARM_INS_OPCODE(last) == ARM_T2CBNZ))
      {
        ArmUpdateBranchDisplacement(i_edge);
      }
    }
  STATUS(STOP,("Relocating branch displacements"));
  /* }}} */
  /* relocate the addresses stored in non-code sections {{{ */
  STATUS(START,("Relocating in non-code sections"));
  OBJECT_FOREACH_RELOC(CFG_OBJECT(cfg),reloc)
  {
    t_relocatable *from = RELOC_FROM (reloc);

    if (RELOCATABLE_RELOCATABLE_TYPE(from)==RT_SUBSECTION)
    {
	/* TODO this is probably here for FIT. If so, it should be solved
	 * cleaner, or documented better */
	if (G_T_UINT32(SECTION_OLD_ADDRESS(T_SECTION(from)))==0x0)
	  StackExec(RELOC_CODE(reloc),reloc,NULL,SECTION_DATA(T_SECTION(from)),
	      TRUE,0,CFG_OBJECT(cfg));
	else
	{
          /* VERBOSE(0,("relocing @R with label %s old 0x%x new ",reloc,RELOC_LABEL(reloc)?RELOC_LABEL(reloc):"<none>",RelocGetData(reloc))); */
	  StackExec(RELOC_CODE(reloc),reloc,NULL,SECTION_DATA(T_SECTION(from)),
	      TRUE,mode,CFG_OBJECT(cfg));
          /* VERBOSE(0,("0x%x\n",RelocGetData(reloc))); */
	}
    }
    else if (RELOCATABLE_RELOCATABLE_TYPE(from)==RT_BBL) continue;
    else if (RELOCATABLE_RELOCATABLE_TYPE(from)==RT_INS) continue;
    else if (RELOCATABLE_RELOCATABLE_TYPE(from)==RT_SECTION)
      FATAL(("reloc from section @R\n",reloc));
    else FATAL(("Unknown reloc @R\n",reloc));
  }
  STATUS(STOP,("Relocating in non-code sections"));
  /* }}} */
  /* relocate the addresses stored in data blocks in the text section {{{ */
  STATUS(START,("Relocating in code sections"));
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_FOREACH_ARM_INS(i_bbl,i_ins)
    {      
      if (ARM_INS_REFERS_TO(i_ins))
      {
	if (ARM_INS_OPCODE (i_ins) != ARM_ADDRESS_PRODUCER)
	{
	  t_arm_ins rins;
	  char tmp[4];
     void * tmp_ptr = &tmp;
	  t_address ret;
	  t_reloc_ref *relocref;

	  for (relocref=ARM_INS_REFERS_TO(i_ins); relocref; relocref = RELOC_REF_NEXT(relocref))
	  {
	    t_reloc * reloc = RELOC_REF_RELOC(relocref);
            if (StringPatternMatch("ideadc0de*", RELOC_CODE(reloc)))
                continue;

            if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
            {
              ThumbAssembleOne(i_ins,tmp);

              if (ARM_INS_CSIZE(i_ins) == 2)
                *((t_uint32 *) tmp_ptr) &= 0x0000ffff;
              else
              {
                /* As the relocation code assumes the instructions to be encoded as if they would
                 * be encoded in the binary in-memory or on-disk, the first and second 16-bit parts
                 * of the 32-bit Thumb instructions should be exchanged. This is because the upper
                 * 16-bit part is written in the binary BEFORE the lower 16-bit part. */
                t_uint32 d = *(t_uint32 *)tmp_ptr;
                d = ((d & 0xffff) << 16) | ((d & 0xffff0000) >> 16);
                *(t_uint32 *)tmp_ptr = d;
              }
            }
            else
              ArmAssembleOne(i_ins,tmp);

	    if (OBJECT_SWITCHED_ENDIAN(CFG_OBJECT (cfg)))
	      *((t_uint32 *) tmp_ptr) = Uint32SwapEndian(*((t_uint32 *) tmp_ptr));

	    /* TODO: this is here for FIT, but it should be solved cleaner! */
	    if (G_T_UINT32(ARM_INS_OLD_ADDRESS(i_ins))==0xffffffff)
	      ret=StackExec(RELOC_CODE(reloc),reloc,NULL,tmp,TRUE,0,CFG_OBJECT(cfg));
	    else
	      ret=StackExec(RELOC_CODE(reloc),reloc,NULL,tmp,TRUE,mode,CFG_OBJECT(cfg));
	    if (!AddressIsNull(ret))
	      FATAL(("Non-zero return (@G) for instruction @I relocation @R",ret,i_ins,reloc));

      t_bool should_relocate_branch = FALSE;
      if (ARM_INS_TYPE(i_ins) == IT_BRANCH)
      {
        t_cfg_edge * edge = NULL;
        t_cfg_edge * jumping_edge = NULL;
        BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(i_ins), edge)
        {
          if (CFG_EDGE_CAT(edge) == ET_JUMP
              || CFG_EDGE_CAT(edge) == ET_IPJUMP
              || CFG_EDGE_CAT(edge) == ET_CALL)
          {
            ASSERT(!jumping_edge, ("whaaaat? multiple outgoing jump/call edges for @I: @eiB", i_ins, ARM_INS_BBL(i_ins)));
            jumping_edge = edge;
          }
        }

        if (jumping_edge)
          if (FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_TAIL(jumping_edge))))
            should_relocate_branch = TRUE;
      }

	    if (OBJECT_SWITCHED_ENDIAN(CFG_OBJECT (cfg)))
	      *((t_uint32 *) tmp_ptr) = Uint32SwapEndian(*((t_uint32 *) tmp_ptr));

	    if (ARM_INS_TYPE(i_ins)==IT_DATA)
	    {
	      ArmDisassembleData(&rins,*(t_uint32 *) tmp_ptr,ARM_DATA);
	      ARM_INS_SET_IMMEDIATE(i_ins, ARM_INS_IMMEDIATE(&rins));
	    }
            else if (((ARM_INS_TYPE(i_ins) != IT_BRANCH) || should_relocate_branch) &&
              (ARM_INS_FLAGS(i_ins) & (FL_IMMED|FL_IMMEDW)))
            {
              t_reloc * reloc = NULL;
              t_uint32 calculated_immediate = 0, new_immediate = 0;

              if (ARM_INS_FLAGS(i_ins) & FL_THUMB
                  && ARM_INS_CSIZE(i_ins) == 4)
              {
                t_uint32 d = *(t_uint32 *)tmp_ptr;
                d = ((d & 0xffff) << 16) | ((d & 0xffff0000) >> 16);
                *(t_uint32 *)tmp_ptr = d;
              }

              reloc = RELOC_REF_RELOC(ARM_INS_REFERS_TO (i_ins));
              calculated_immediate = G_T_UINT32 (StackExec (RELOC_CODE(reloc), reloc, NULL, NULL, FALSE, 0, CFG_OBJECT (cfg)));
              ArmDisassembleEncoded(T_INS(&rins), *(t_uint32 *)tmp_ptr, (ARM_INS_FLAGS(i_ins) & FL_THUMB) ? TRUE : FALSE);

              new_immediate = (ARM_INS_IMMEDIATE(&rins));
              ASSERT(calculated_immediate == new_immediate, ("calculated immediate (%x) does not equal disassembled immediate (%x) for @I, reloc code %s", calculated_immediate, new_immediate, i_ins, RELOC_CODE(reloc)));
              ARM_INS_SET_IMMEDIATE(i_ins, ARM_INS_IMMEDIATE(&rins));
	    }
	    VERBOSE(5,("Relocated ins @I, reloc @R",i_ins,reloc));
	  }
	}
	else
	{
	  t_reloc *reloc = RELOC_REF_RELOC(ARM_INS_REFERS_TO (i_ins));
	  ARM_INS_SET_IMMEDIATE (i_ins, G_T_UINT32 (StackExec (RELOC_CODE(reloc), reloc, NULL, NULL, FALSE, 0, CFG_OBJECT (cfg))));
	}
      }
    }
  }
  STATUS(STOP,("Relocating in code sections"));
  /* }}} */

  /* set new entry point for the object */
  if (!(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(CFG_ENTRY(cfg)->entry_bbl))) & FL_THUMB))
  {
    OBJECT_SET_ENTRY(CFG_OBJECT(cfg),
        BBL_CADDRESS(CFG_ENTRY(cfg)->entry_bbl));
  }
  else
  {
    OBJECT_SET_ENTRY(CFG_OBJECT(cfg),
        AddressAddUint32(BBL_CADDRESS(CFG_ENTRY(cfg)->entry_bbl),1));
  }
}
/*}}}*/


t_bool
BblsInSameChainsSlow(t_bbl *bbl1, t_bbl *bbl2)
{
  while (BBL_NEXT_IN_CHAIN(bbl1)!=NULL)
    bbl1=BBL_NEXT_IN_CHAIN(bbl1);
  while (BBL_NEXT_IN_CHAIN(bbl2)!=NULL)
    bbl2=BBL_NEXT_IN_CHAIN(bbl2);
  return bbl1 == bbl2;
}


t_cfg_edge *ArmGetFallThroughEdge(t_bbl *i_bbl)
{
  t_cfg_edge * ft=NULL;
  t_cfg_edge * ft_like=NULL;
  t_cfg_edge * i_edge;
  t_bool switch_bbl=FALSE;

  BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
  {
    if (CFG_EDGE_CAT(i_edge)==ET_SWITCH || CFG_EDGE_CAT(i_edge)==ET_IPSWITCH)
    {
      switch_bbl=TRUE;
    }
    else if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH || CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU)
    {
      if (ft) {
        CfgDrawFunctionGraphs(BBL_CFG(i_bbl), "ft");
	FATAL(("Two true fallthrough edges: @eiB @E and @E",i_bbl,ft,i_edge));
      }
      ft = i_edge;
    }
    else if ((CFG_EDGE_CAT(i_edge) == ET_CALL ||
	  CFG_EDGE_CAT(i_edge) == ET_SWI
	  ) && CFG_EDGE_CORR(i_edge) &&
          (CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)) != CFG_EXIT_HELL_NODE(BBL_CFG(i_bbl)))
	)
    {
      if (ft_like)
	if (CFG_EDGE_TAIL(ft_like) != CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)))
	  FATAL(("Two fallthrough-like edges: @E and @E",ft_like,i_edge));
      ft_like = CFG_EDGE_CORR(i_edge);
    }
  }

  /* check for inconsistencies between fallthrough edges and
   * fallthrough-like edges */
  if (ft && ft_like)
  {
    ASSERT(!switch_bbl,("Cannot have switch bbl with 2 kinds of fallthroughs"));
    if (CFG_EDGE_TAIL(ft) != CFG_EDGE_TAIL(ft_like))
    {
      /* this is an inconsistency. the only way in which
       * this is possible, is if the function call at the
       * end of the block is indirect, and there is an address
       * producer that places an address in r14 that does not
       * immediately follow the block */
      t_arm_ins *ins;
      BBL_FOREACH_ARM_INS_R(i_bbl,ins)
	if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R14))
	  break;
      if (!ins ||
	          ARM_INS_OPCODE(ins) != ARM_ADDRESS_PRODUCER) {
        CfgDrawFunctionGraphsWithHotness(BBL_CFG(i_bbl), "inconsistent-ft");
	      FATAL(("Inconsistent fallthrough situation @I @eiB", ins, i_bbl));
      }
    }
  }

  if (!ft)
    ft = ft_like;

  return ft;
}

static
int EdgeTailChainSizeComparerSlow(const void *a, const void *b)
{
  t_bbl *chain1 = CFG_EDGE_TAIL(*(t_cfg_edge**)a);
  t_bbl *chain2 = CFG_EDGE_TAIL(*(t_cfg_edge**)b);
  /* not efficient, but at this point BBL_CHAIN_LAST is not yet size,
   * so BBL_CHAIN_SIZE() can't work yet; additionally, this routine is
   * only used to sort compact thumb switch bbls, of which there can be
   * maximally 8
   */
  t_address chain1size = AssignAddressesInChain(chain1,AddressNullForCfg(cfg));
  t_address chain2size = AssignAddressesInChain(chain2,AddressNullForCfg(cfg));

  return
    AddressIsLt(chain1size,chain2size)?-1:
      AddressIsGt(chain1size,chain2size)?1:0;
}

static void
EdgeArrayAddEdgeIfNewTail(t_cfg_edge **arr, int *arrcount, void *item)
{
  t_bool add = TRUE;
  int tel;

  for (tel=0; tel < *arrcount; tel++)
  {
    if (CFG_EDGE_TAIL(arr[tel])==CFG_EDGE_TAIL(item))
    {
      add=FALSE;
      break;
    }
  }
  if (add)
  {
    arr[*arrcount]=item;
    (*arrcount)++;
  }
}

static void ArmChainPrint(t_bbl * head)
{
        DEBUG(("Chain @B", head));

        do
        {
                DEBUG(("   @B", head));
        } while ((head = BBL_NEXT_IN_CHAIN(head)));
}

/* Chain bbls part of Thumb Switch tables */
void
ArmChainThumbSwitchTables(t_cfg *cfg, t_arm_opcode opcode, t_chain_holder * ch)
{
  t_bbl *i_bbl;

  ASSERT(opcode == ARM_T2TBB || opcode == ARM_T2TBH || opcode == ARM_ADD || opcode == ARM_BL,("Only a few types of ARM THUMB switch tables are currently supported, not the ones with %s instructions",arm_opcode_table[opcode].desc));

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bool switch_bbl=FALSE;
    t_cfg_edge * i_edge;
    int switch_max = 0;

    if (BBL_IS_HELL(i_bbl)) continue;

    /* determine number of switch edges */
    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (CFG_EDGE_CAT(i_edge)==ET_SWITCH || CFG_EDGE_CAT(i_edge)==ET_IPSWITCH)
      {
	switch_bbl=TRUE;
	switch_max++;
      }
    }

    if (switch_bbl
        && opcode == ARM_BL
        && (ArmInsSwitchedBLTableEntrySize(T_ARM_INS(BBL_INS_LAST(i_bbl))) == 1
            || ArmInsSwitchedBLTableEntrySize(T_ARM_INS(BBL_INS_LAST(i_bbl))) == 4))
        continue;

    if (switch_bbl && ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==opcode && ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(i_bbl))) & FL_THUMB)))
    {
      t_cfg_edge ** switch_chain_head_edges = Calloc(sizeof(t_cfg_edge *),switch_max);
      t_cfg_edge ** switch_chain_middle_edges = Calloc(sizeof(t_cfg_edge *),switch_max);
      int switch_head_edge_max, switch_middle_edge_max, tel;

      switch_head_edge_max = 0;
      switch_middle_edge_max = 0;
      /* collect all switch edges, seperating them in ones that go to the head of
       * a chain (-> can chain to this switch statement), and ones that go to the
       * middle of a chain (-> if inside one of the chains of the previous category,
       * no problem; otherwise will have to be replaced with a jump to their final
       * destination)
       */
      BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
      {
	if ((CFG_EDGE_CAT(i_edge)==ET_SWITCH) || (CFG_EDGE_CAT(i_edge)==ET_IPSWITCH))
	{
	  /* in case of fallthrough-paths between case-statements, the
	   * switch edge may not point to the start of a chain ->
	   * ignore. Also ignore duplicates.
	   */
	  if (!BBL_PREV_IN_CHAIN(CFG_EDGE_TAIL(i_edge)))
	  {
	    EdgeArrayAddEdgeIfNewTail(switch_chain_head_edges,&switch_head_edge_max,i_edge);
	  }
	  else
	  {
	    switch_chain_middle_edges[switch_middle_edge_max]=i_edge;
	    switch_middle_edge_max++;
	  }
	}
	else
	{
          if (opcode != ARM_BL)
            FATAL(("Weird edge in switch: @E\n",i_edge));
	}
      }
      /* add jump bbls for non-chain-head bbls that are not part of the chains we found */
      for (tel=0; tel<switch_middle_edge_max;tel++)
      {
        int tel2;

        for (tel2=0; tel2<switch_head_edge_max; tel2++)
        {
          /* if the branch goes to the middle of a bbl that will be chained to
           * this switch statement, nothing special has to happen
           */
          if (BblsInSameChainsSlow(CFG_EDGE_TAIL(switch_chain_middle_edges[tel]),CFG_EDGE_TAIL(switch_chain_head_edges[tel2])))
          {
            switch_chain_middle_edges[tel]=NULL;
            break;
          }
        }
          /* add a branch bbl to the final destination
           * TODO: reuse branch bbls for multiple edges to the same destination
           */
        if (switch_chain_middle_edges[tel])
        {
          t_bbl *target_bbl = CFG_EDGE_TAIL(switch_chain_middle_edges[tel]);
          t_bbl *jmp_bbl;
          t_arm_ins *jmp;

          jmp_bbl = BblNew(cfg);
          BblInsertInFunction(jmp_bbl,BBL_FUNCTION(target_bbl));
          BBL_SET_FIRST_IN_CHAIN(jmp_bbl, jmp_bbl);
          BBL_SET_LAST_IN_CHAIN(jmp_bbl, jmp_bbl);
          jmp=ArmInsNewForBbl(jmp_bbl);
          ARM_INS_SET_FLAGS(jmp,ARM_INS_FLAGS(jmp) | FL_THUMB);
          ArmInsMakeUncondBranch(jmp);
          ArmInsAppendToBbl(jmp,jmp_bbl);

          // old code commented out: why would we need to make an interprocedural edge? jmp_bbl and target_bbl are both in the same function?
          //          edge=CfgEdgeCreate(cfg,jmp_bbl,target_bbl,CFG_EDGE_CAT(switch_chain_middle_edges[tel])==ET_SWITCH?ET_JUMP:ET_IPJUMP);
          //          if (CFG_EDGE_CAT(edge)==ET_IPJUMP)
          //            CfgEdgeCreateCompensating(cfg,edge);
          // new code seems ok:
          CfgEdgeCreate(cfg,jmp_bbl,target_bbl,ET_JUMP);

          /* replace the destination of the switch edge */
          CfgEdgeChangeTail(switch_chain_middle_edges[tel],jmp_bbl);
          if (CFG_EDGE_REL(switch_chain_middle_edges[tel]))
            RelocSetToRelocatable(CFG_EDGE_REL(switch_chain_middle_edges[tel]),0,T_RELOCATABLE(jmp_bbl));
          /* add edge to edges to chain to this switch statement */
          EdgeArrayAddEdgeIfNewTail(switch_chain_head_edges,&switch_head_edge_max,switch_chain_middle_edges[tel]);
        }
      }


      /* order them according size to minimize the chance that
       * one of the switch edges will be placed out of range
       */
      qsort(switch_chain_head_edges,switch_head_edge_max,sizeof(t_cfg_edge *),EdgeTailChainSizeComparerSlow);

      /* OLD COMMENT: the databbl with the thumb switch table should already be chained to the
       * dispatch bbl, and should not be followed by anything
       * UPDATE: this is no longer true: because of CBZ/CBNZ instructions (which have smaller displacement limits)
       * other bbls might already be chained to the data bbl
       */
      /*prev = BBL_NEXT_IN_CHAIN(i_bbl);
      while (BBL_NEXT_IN_CHAIN(prev))
        prev = BBL_NEXT_IN_CHAIN(prev);*/
      //      ASSERT(BBL_NEXT_IN_CHAIN(prev)==NULL,("Thumb switch data bbl not last in chain? @eiB\n @eiB\n @eiB",i_bbl,prev,BBL_NEXT_IN_CHAIN(prev)));

        /* This is the last BBL of the switch-statement itself.
         * Now we want to chain every target chain to this BBL. */
        for (tel = 0; tel < switch_head_edge_max; tel++)
        {
                t_uint32 tel2 = 0;

                ASSERT(BBL_FIRST_IN_CHAIN(i_bbl), ("first element of primary chain should be set @eiB!", i_bbl));
                ASSERT(BBL_FIRST_IN_CHAIN(CFG_EDGE_TAIL(switch_chain_head_edges[tel])), ("first element of secondary chain should be set @eiB!", CFG_EDGE_TAIL(switch_chain_head_edges[tel])));

                /* This can happen when the switch BBL is not the head of a chain, but instead has
                 * an incoming fallthrough path originating in one of its cases. */
                if (BBL_FIRST_IN_CHAIN(i_bbl) == CFG_EDGE_TAIL(switch_chain_head_edges[tel]))
                        continue;

                MergeChains(BBL_FIRST_IN_CHAIN(i_bbl), CFG_EDGE_TAIL(switch_chain_head_edges[tel]));

                /* remove the merged chain from the list */
                for (tel2 = 0; tel2 < ch->nchains; tel2++)
                {
                        if (ch->chains[tel2] == CFG_EDGE_TAIL(switch_chain_head_edges[tel]))
                        {
                                ch->chains[tel2] = NULL;
                                break;
                        }
                }
        }

      /* now chain them */
      /*for (tel=0;tel<switch_head_edge_max;tel++)
      {
	t_bbl *last;
	last=CFG_EDGE_TAIL(switch_chain_head_edges[tel]);
	ASSERT(BBL_PREV_IN_CHAIN(last)==NULL,("has to be head of chain: @eiB"));
	BBL_SET_NEXT_IN_CHAIN(prev,last);
	BBL_SET_PREV_IN_CHAIN(last,prev);
	prev = last;
	while (BBL_NEXT_IN_CHAIN(prev))
	  prev=BBL_NEXT_IN_CHAIN(prev);
      }*/
      Free(switch_chain_head_edges);
      Free(switch_chain_middle_edges);
    }
  }
}

static void
ArmRandomChainOrder(t_chain_holder * ch)
{
  t_bbl *tmpchain;
  int i, index;

  VERBOSE(0, ("Using random chain order, seed %i", diabloarm_options.orderseed));

  srand(diabloarm_options.orderseed); /* TODO: use diablo_random functions */
  /* standard shuffle */
#if 1
  for (i = ch->nchains - 1; i >= 0; --i)
  {
    index = rand() % (i + 1);
    tmpchain = ch->chains[index];
    ch->chains[index] = ch->chains[i];
    ch->chains[i] = tmpchain;
  }
#else
  VERBOSE(0, ("randomizing %d chains", ch->nchains));
  for (i = 0; i < ch->nchains-1; i+=2)
  //for (i = 0; i < diablosupport_options.debugcounter; i+=2)
  //i = 2462;
  {
    index = i+1;
    DEBUG(("switching chain %d <-> %d", i, index));
    ArmChainPrint(ch->chains[index]);
    tmpchain = ch->chains[index];
    ch->chains[index] = ch->chains[i];
    ch->chains[i] = tmpchain;
  }
#endif
}

void ArmCustomSectionPlacement(t_section *sec, t_section **subs, t_uint32 nsubs, t_bool *overridden)
{
  if (StringPatternMatch(SECTION_NAME(sec), ".ARM.exidx"))
  {
    /* we want to override ordering the exidx subsections */
    *overridden = TRUE;

    /* sanity check */
    ASSERT(exidx_subsection_order, ("subsection array order was not determined yet!"));
    ASSERT(nsubs == exidx_subsection_count, ("final exidx section does not contain the same amount of subsections %d/%d", nsubs, exidx_subsection_count));

    for (t_uint32 i = 0; i < nsubs; i++)
      subs[i] = exidx_subsection_order[i];
  }
}

static
void ExidxSplitIntoSubsections(t_cfg *cfg)
{
  t_section *exidx_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".ARM.exidx");
  t_uint32 nr_subsections;
  t_section **subsections;

  /* first make sure every exidx entry resides in its own subsection */
  subsections = SectionGetSubsections(exidx_section, &nr_subsections);

  /* sanity check */
  ASSERT(nr_subsections > 0, ("no exidx subsections found in @T", exidx_section));

  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    t_address current_offset;

    /* sanity check */
    ASSERT(SECTION_CSIZE(subsections[i]) % 8 == 0, ("exidx subsection size not a multiple of 8 @T", subsections[i]));

    /* skip sections already containing only one exidx entry */
    if (SECTION_CSIZE(subsections[i]) == 8) continue;

    /* if more than one entry is present in this subsection,
     * split it into multiple subsections */
    current_offset = AddressNew32(0);

    while (current_offset < SECTION_CSIZE(subsections[i]))
    {
      t_address current_address = AddressAdd(SECTION_CADDRESS(subsections[i]), current_offset);
      t_reloc *rel, *tblrel;
      t_bbl *target_bbl;
      char new_subsection_name[25];
      t_section *new_subsection;

      /* relocations of this exidx entry */
      rel = RelocatableGetRelocForAddress(T_RELOCATABLE(exidx_section), current_address);
      ASSERT(rel, ("expected relocation"));

      tblrel = RelocatableGetRelocForAddress(T_RELOCATABLE(exidx_section), AddressAddUint32(current_address, 4));
      ASSERT(tblrel, ("expected relocation in @T at offset @G (@G): @G", subsections[i], current_offset, current_address, AddressAddUint32(current_address, 4)));

      /* BBL pointed to by this entry */
      ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL, ("expected to relocatable of type RT_BBL, got @R", rel));
      target_bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);

      /* construct a name for the new subsection */
      sprintf(new_subsection_name, ".exidx.diablo.0x%08x", BBL_OLD_ADDRESS(target_bbl));

      /* create a new subsection for this entry */
      new_subsection = SectionCreateForObject(ObjectGetLinkerSubObject(SECTION_OBJECT(exidx_section)),
                                                         SECTION_TYPE(subsections[i]),
                                                         exidx_section, AddressNew32(8),
                                                         new_subsection_name);

      SECTION_SET_CADDRESS(new_subsection, current_address);
      SECTION_SET_OLD_ADDRESS(new_subsection, AddressAdd(SECTION_OLD_ADDRESS(subsections[i]), current_offset));
      SECTION_SET_ALIGNMENT(new_subsection, SECTION_ALIGNMENT(exidx_section));

      /* adjust relocations associated with this entry */
      RelocSetFrom(rel, T_RELOCATABLE(new_subsection));
      RELOC_SET_FROM_OFFSET(rel, AddressNew32(0));

      RelocSetFrom(tblrel, T_RELOCATABLE(new_subsection));
      RELOC_SET_FROM_OFFSET(tblrel, AddressNew32(4));

      /* go to the next exidx entry */
      current_offset = AddressAddUint32(current_offset, 8);
    }

    /* sanity check */
    if (SECTION_REFED_BY(subsections[i]))
      FATAL(("@T still refed by!", subsections[i]));
    if (SECTION_REFERS_TO(subsections[i]))
      FATAL(("@T still refers to!",  subsections[i]));

    SectionKill(subsections[i]);
  }
}

static
void RandomizeExidx(t_cfg *cfg, t_chain_holder *ch, t_bool randomize)
{
  t_section *exidx_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".ARM.exidx");
  t_uint32 nr_subsections;
  t_section **subsections;
  int exidx_index;

  /* randomize the chain order */
  if (randomize)
    ArmRandomChainOrder(ch);

  /* as the final order of the subsections is determined in the final layouting phase,
   * we can not fix this order here. Instead, use a broker call to make the generice
   * layout code do a callback to our function. */
  DiabloBrokerCallInstall("CustomSubsectionPlacement", "t_section *, t_section **, t_uint32, t_bool *", (void*)ArmCustomSectionPlacement, TRUE);

  /* refresh the list of subsections */
  nr_subsections = 0;
  subsections = SectionGetSubsections(exidx_section, &nr_subsections);

  /* these data arrays are used by the broker called function above */
  exidx_subsection_order = Malloc(sizeof(t_section *) * nr_subsections);
  exidx_subsection_count = nr_subsections;

  /* reconstruct exidx section by iterating over all the chains */
  exidx_index = 0;
  for (t_uint32 i = 0; i < ch->nchains; i++)
  {
    t_bbl *tmp_bbl = ch->chains[i];

    while (tmp_bbl)
    {
      t_reloc_ref *rref, *rref2;

      /* iterate over all refed by relocs */
      for (rref = BBL_REFED_BY(tmp_bbl), rref2 = rref ? RELOC_REF_NEXT(rref) : NULL;
           rref;
           rref = rref2, rref2 = rref2 ? RELOC_REF_NEXT(rref2) : NULL)
      {
        t_reloc *rel = RELOC_REF_RELOC(rref);

        /* try to look for an owning exidx subsection */
        for (t_uint32 j = 0; j < nr_subsections; j++)
        {
          if (RELOC_FROM(rel) == T_RELOCATABLE(subsections[j]))
          {
            exidx_subsection_order[exidx_index] = subsections[j];
            exidx_index++;
          }
        }
      }

      tmp_bbl = BBL_NEXT_IN_CHAIN(tmp_bbl);
    }
  }
  ASSERT(exidx_index == nr_subsections, ("waaaaat? %d/%d", exidx_index, nr_subsections));
}

void MergeExidx(t_cfg *cfg)
{
  t_section *exidx_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".ARM.exidx");
  t_uint32 nr_subsections;
  t_section **subsections;

  t_bool seen_cantunwind = FALSE;
  t_uint32 nr_subsections_unique = 0, unique_index;
  t_section **old_subsections;

  /* refresh the list of subsections */
  nr_subsections = 0;
  subsections = SectionGetSubsections(exidx_section, &nr_subsections);

  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    fflush(stdout);
    t_section *sec = exidx_subsection_order[i];
    t_reloc *rel = RelocatableGetRelocForAddress(T_RELOCATABLE(sec), AddressAddUint32(SECTION_CADDRESS(sec), 4));
    nr_subsections_unique++;

    if (RELOC_ADDENDS(rel)[0] == 0x1)
    {
      /* can't unwind */
      if (seen_cantunwind)
      {
        /* kill this subsection */
        exidx_subsection_order[i] = NULL;
        SectionKill(sec);

        /* subtract one again from the nr_subsections_unique,
         * as we counted one too many */
        nr_subsections_unique--;
      }
      else
        seen_cantunwind = TRUE;
    }
    else
      seen_cantunwind = FALSE;
  }

  old_subsections = exidx_subsection_order;
  exidx_subsection_order = Malloc(sizeof(t_section *) * nr_subsections_unique);
  exidx_subsection_count = nr_subsections_unique;
  unique_index = 0;

  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    if (old_subsections[i])
    {
      exidx_subsection_order[unique_index] = old_subsections[i];
      unique_index++;
    }
  }

  Free(old_subsections);

  VERBOSE(0, ("removed %d unnecessary exidx sections (%d left out of %d)",
              nr_subsections - nr_subsections_unique,
              nr_subsections_unique,
              nr_subsections));
}

int SectionAddressComparer(const void * a, const void * b)
{
        t_section *sec1 = *(t_section**)a;
        t_section *sec2 = *(t_section**)b;

        return AddressIsLt(SECTION_CADDRESS(sec1), SECTION_CADDRESS(sec2)) ? -1 : (AddressIsGt(SECTION_CADDRESS(sec1), SECTION_CADDRESS(sec2)) ? 1 : 0);
}

static
void ChainExceptionHandled(t_cfg *cfg, t_chain_holder *ch)
{
  t_section *exidx_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".ARM.exidx");
  t_address addr = SECTION_CADDRESS(exidx_section);
  t_address section_end_addr = AddressAdd(SECTION_CADDRESS(exidx_section), SECTION_CSIZE(exidx_section));

  t_uint32 nr_subsections;
  t_section **subsections = SectionGetSubsections(exidx_section, &nr_subsections);
  t_uint32 chain_idx = 0;

  qsort(subsections, nr_subsections, sizeof(t_section *), SectionAddressComparer);

  VERBOSE(0, ("creating chains for %d existing exidx entries", nr_subsections));

  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    t_reloc *rel;
    t_relocatable *to_relocatable;
    t_bbl *start_bbl, *end_bbl;
    t_bbl *chain_end, *chain_start;
    t_bbl *bbl_it;

    /* region start */
    rel = RelocatableGetRelocForAddress(T_RELOCATABLE(subsections[i]), SECTION_CADDRESS(subsections[i]));

    /* get the BBL this relocatable points to */
    to_relocatable = RELOC_TO_RELOCATABLE(rel)[0];
    ASSERT(RELOCATABLE_RELOCATABLE_TYPE(to_relocatable) == RT_BBL, ("expected to relocatable of type RT_BBL, got @R", rel));

    start_bbl = T_BBL(to_relocatable);

    /* region end, but only if possible (i.e., if a next entry in the .ARM.exidx section is defined) */
    if (i < nr_subsections-1)
    {
      rel = RelocatableGetRelocForAddress(T_RELOCATABLE(subsections[i+1]), SECTION_CADDRESS(subsections[i+1]));

      /* get the BBL this relocatable points to */
      to_relocatable = RELOC_TO_RELOCATABLE(rel)[0];
      ASSERT(RELOCATABLE_RELOCATABLE_TYPE(to_relocatable) == RT_BBL, ("expected to relocatable of type RT_BBL, got @R", rel));

      end_bbl = T_BBL(to_relocatable);
    }
    else
      end_bbl = NULL;

    chain_start = GetHeadOfChain(start_bbl);

    /* iterate over the chains */
    while (chain_idx < ch->nchains)
    {
      /* Initial setup: skip BBLs not present in the .ARM.exidx section,
       * up to the start BBL (inclusive), because this will be the first BBL in our chain. */
      if (AddressIsLe(BBL_OLD_ADDRESS(ch->chains[chain_idx]), BBL_OLD_ADDRESS(start_bbl))
          || BBL_IS_HELL(ch->chains[chain_idx]))
      {
        chain_idx++;
        continue;
      }

      if (BBL_OLD_ADDRESS(start_bbl) <= BBL_OLD_ADDRESS(ch->chains[chain_idx])
          /* the regular case: end_bbl is defined, and a valid range can be determined */
          && ((end_bbl && BBL_OLD_ADDRESS(ch->chains[chain_idx]) < BBL_OLD_ADDRESS(end_bbl))
              /* if end_bbl is not defined, the end of the .ARM.exidx-section has been reached.
               * Add the remaining BBLs to this last chain, but only if these BBLs originate from
               * the same section as start_bbl originates from. */
              || (!end_bbl
                  && SectionGetFromObjectByAddress(CFG_OBJECT(cfg), BBL_OLD_ADDRESS(start_bbl))
                      == SectionGetFromObjectByAddress(CFG_OBJECT(cfg), BBL_OLD_ADDRESS(ch->chains[chain_idx])))))
      {
        MergeChains(chain_start, ch->chains[chain_idx]);

        /* remove the chained BBL from the chain list */
        ch->chains[chain_idx] = NULL;
      }
      else if (BBL_OLD_ADDRESS(ch->chains[chain_idx]) == 0)
      {
        /* Diablo-created BBL: do nothing */
      }
      else
        break;

      chain_idx++;
    }
  }
}

static
void ExidxCreateSubsection(t_section *exidx_section, t_bbl *target_bbl, t_reloc *orig_tblrel)
{
  t_section *new_subsection;
  char new_subsection_name[28];
  t_uint32 nr;
  t_object *obj = SECTION_OBJECT(exidx_section);
  t_reloc *rel;
  t_address new_address = AddressAdd(SECTION_CADDRESS(exidx_section), SECTION_CSIZE(exidx_section));

  /* name of this new subsection */
  sprintf(new_subsection_name, ".exidx.diablofun.0x%08x", new_address);

  /* new subsection */
  new_subsection = SectionCreateForObject(ObjectGetLinkerSubObject(SECTION_OBJECT(exidx_section)),
                                          SECTION_TYPE(SectionGetSubsections(exidx_section, &nr)[0]),
                                          exidx_section, AddressNew32(8),
                                          new_subsection_name);
  SECTION_SET_CADDRESS(new_subsection, new_address);
  SECTION_SET_OLD_ADDRESS(new_subsection, AddressNew32(0));
  SECTION_SET_ALIGNMENT(new_subsection, SECTION_ALIGNMENT(exidx_section));

  /* create relocations */
  rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE (obj),
                                        AddressNew32(0),                /* addend */
                                        T_RELOCATABLE(new_subsection),  /* from */
                                        AddressNew32(0),                /* from offset */
                                        T_RELOCATABLE(target_bbl),      /* to */
                                        AddressNew32(0),                /* to offset */
                                        diabloobject_options.keep_exidx,/* hell */
                                        NULL, NULL, NULL,               /* edge, corresp, sec */
                                        "R00A01+P-A00+ i7fffffff&" "\\" WRITE_32);
  RelocAddAddend(rel, AddressNew32(0));

  /* copy over data if necessary */
  t_address addend = AddressNew32(0);

  if (orig_tblrel)
  {
    t_reloc *duped = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), orig_tblrel);
    RelocSetFrom(duped, T_RELOCATABLE(new_subsection));
  }
  else
  {
    RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE (obj),
                                AddressNew32(0x1),                        /* addend = EXIDX_CANTUNWIND */
                                T_RELOCATABLE(new_subsection),            /* from */
                                AddressNew32(4),                          /* from offset */
                                NULL,                                     /* to */
                                diabloobject_options.keep_exidx,          /* hell */
                                NULL, NULL, NULL,                         /* edge, corresp, sec */
                                "A00" "\\" WRITE_32);
  }
}

static
void ExidxCreateFunctionSubsections(t_cfg *cfg)
{
  t_uint32 nr_subsections = 0;
  t_section *exidx_section = SectionGetFromObjectByName(CFG_OBJECT(cfg), ".ARM.exidx");
  t_section **subsections = SectionGetSubsections(exidx_section, &nr_subsections);
  t_bbl *bbl_it, *target_bbl;
  t_function *func_it;
  t_reloc *rel;
  t_uint32 nr_added_none = 0;
  t_uint32 nr_added_split = 0;

  VERBOSE(0, ("creating additional exidx entries for functions (%d exidx exist already)", nr_subsections));

  BblInitExidxSection(cfg);

  /* iterate over every exidx subsection, setting the appropriate field in the BBL instance */
  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    t_address lower_bound, upper_bound;

    rel = RelocatableGetRelocForAddress(T_RELOCATABLE(subsections[i]), SECTION_CADDRESS(subsections[i]));
    ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL, ("expected to relocatable of type RT_BBL, got @R", rel));
    target_bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
    lower_bound = BBL_OLD_ADDRESS(target_bbl);

    if (i < nr_subsections-1)
    {
      rel = RelocatableGetRelocForAddress(T_RELOCATABLE(subsections[i+1]), SECTION_CADDRESS(subsections[i+1]));
      ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL, ("expected to relocatable of type RT_BBL, got @R", rel));
      target_bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
      upper_bound = BBL_OLD_ADDRESS(target_bbl);
    }
    else
      upper_bound = AddressNew32(0);

    CFG_FOREACH_BBL(cfg, bbl_it)
    {
      if (BBL_IS_HELL(bbl_it)) continue;

      if (AddressIsLe(lower_bound, BBL_OLD_ADDRESS(bbl_it))
          && ((upper_bound != 0 && AddressIsLt(BBL_OLD_ADDRESS(bbl_it), upper_bound))
              || (upper_bound == 0)))
      {
        /* within range */
        BBL_SET_EXIDX_SECTION(bbl_it, subsections[i]);
      }
    }
  }

  nr_added_none = 0;
  nr_added_split = 0;

  CFG_FOREACH_FUN(cfg, func_it)
  {
    if (FUNCTION_IS_HELL(func_it)) continue;

    t_bbl *entry_bbl = FUNCTION_BBL_FIRST(func_it);
    t_section *bbl_exidx = BBL_EXIDX_SECTION(entry_bbl);

    if (!bbl_exidx)
    {
      ExidxCreateSubsection(exidx_section, entry_bbl, NULL);
      nr_added_none++;
    }
    else
    {
      t_reloc *tblrel;

      /* the entry point of this function already has an
       * associated exidx section */
      rel = RelocatableGetRelocForAddress(T_RELOCATABLE(bbl_exidx), SECTION_CADDRESS(bbl_exidx));
      ASSERT(rel, ("could not find relocation in @T", bbl_exidx));
      target_bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);

      tblrel = RelocatableGetRelocForAddress(T_RELOCATABLE(bbl_exidx), AddressAddUint32(SECTION_CADDRESS(bbl_exidx), 4));

      /* only add entries for "can't unwind" sections */
      if (RELOC_N_TO_RELOCATABLES(tblrel) > 0
          || RELOC_ADDENDS(tblrel)[0] != 0x1)
        continue;

      /* no further processing needed for this function entry point,
       * as the associated exidx section already points to the entry
       * of this function */
      if (target_bbl == entry_bbl)
        continue;

      /* we need to add a new subsection for this function */
      ExidxCreateSubsection(exidx_section, entry_bbl, tblrel);
      nr_added_split++;
    }
  }

  BblFiniExidxSection(cfg);

  VERBOSE(0, ("added %d additional exidx sections (%d had no associated exidx section, %d did have one)",
              nr_added_none+nr_added_split, nr_added_none, nr_added_split));
}

int BblOldAddressComparer(const void * a, const void * b)
{
        t_bbl *bbl1 = *(t_bbl**)a;
        t_bbl *bbl2 = *(t_bbl**)b;

        return AddressIsLt(BBL_OLD_ADDRESS(bbl1), BBL_OLD_ADDRESS(bbl2)) ? -1 : (AddressIsGt(BBL_OLD_ADDRESS(bbl1), BBL_OLD_ADDRESS(bbl2)) ? 1 : 0);
}

/* {{{ Create chains of fallthrough-linked blocks */
void ArmCreateChains(t_cfg * cfg, t_chain_holder *ch)
{
  t_bbl *i_bbl, **chains;
  int nchains,tel;
  BblInitCluster(cfg);

  /* Create chains of one block long */
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_SET_NEXT_IN_CHAIN(i_bbl,  NULL);
    BBL_SET_PREV_IN_CHAIN(i_bbl,  NULL);
  }

  /* first chain all the fallthrough edges */
  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    t_cfg_edge * ft = ArmGetFallThroughEdge(i_bbl);

    if (ft)
    {
      /* Normal fall through chaining */
      t_bbl * j_bbl=(t_bbl *)CFG_EDGE_TAIL(ft);

      if(BBL_NEXT_IN_CHAIN(i_bbl) && (BBL_NEXT_IN_CHAIN(i_bbl)!=j_bbl))
      {
        VERBOSE(0,("@ieB -> @ieB",i_bbl,j_bbl));
        VERBOSE(0,("Next = @ieB",BBL_NEXT_IN_CHAIN(i_bbl)));
        FATAL(("Already next for @B, i.e. @B != @B",i_bbl,BBL_NEXT_IN_CHAIN(i_bbl),j_bbl));
      }
      if(BBL_PREV_IN_CHAIN(j_bbl) && (BBL_PREV_IN_CHAIN(j_bbl)!=i_bbl))
      {
        VERBOSE(0,("@ieB -> @ieB",CFG_EDGE_HEAD(ft),j_bbl));
        VERBOSE(0,("Previous = @ieB",BBL_PREV_IN_CHAIN(j_bbl)));
        CfgDrawFunctionGraphsWithHotness(cfg, "prev");
        FATAL(("Already prev for @B, i.e. @B != @B",j_bbl,BBL_PREV_IN_CHAIN(j_bbl),i_bbl));
      }
      //VERBOSE(0,("CHAINING FT @E for @eiB",ft, i_bbl));
      BBL_SET_NEXT_IN_CHAIN(i_bbl, j_bbl);
      BBL_SET_PREV_IN_CHAIN(j_bbl, i_bbl);
    }
  }

  DetectLoopsInChains(cfg);

  /* then chain the switch cases */
  /* {{{ chain fallthrough paths and switches */
  t_bool changed = FALSE;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bbl * j_bbl = NULL;
    t_bool switch_bbl=FALSE;
    t_cfg_edge * ft=NULL;
    t_cfg_edge * i_edge;
    t_int32 switch_max=-1;

    if (changed)
      DetectLoopsInChains(cfg);
    changed = FALSE;

    /* Dominique: turned this off because
     * 	1) It introduces a bug in kcompress/ARM
     * 	2) I think it's completely unnecessary */
    /*if (!BBL_PRED_FIRST(i_bbl)) continue;*/

    if (BBL_IS_HELL(i_bbl)) continue;

    ft = ArmGetFallThroughEdge(i_bbl);

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (CFG_EDGE_CAT(i_edge)==ET_SWITCH || CFG_EDGE_CAT(i_edge)==ET_IPSWITCH)
      {
	switch_bbl=TRUE;
	if ((t_int32)CFG_EDGE_SWITCHVALUE(i_edge) > switch_max) {
	  switch_max=CFG_EDGE_SWITCHVALUE(i_edge);
        }
      }
    }

    if (switch_bbl && (switch_max == -1))
        switch_bbl = FALSE;

    /* in case of Thumb, bbl0 has to be the data bbl holding the offsets */
    if (switch_bbl &&
	(ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_ADD) &&
	ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(i_bbl))) & FL_THUMB)
    {
      t_bbl *databbl;
      t_arm_ins *ldrb;
      t_reloc *rel;
      ldrb = T_ARM_INS(BBL_INS_LAST(i_bbl));
      while (ldrb &&
	  ((ARM_INS_OPCODE(ldrb) != ARM_LDRB) &&
           (ARM_INS_OPCODE(ldrb) != ARM_LDRH)))
	ldrb = ARM_INS_IPREV(ldrb);
      ASSERT(ldrb != NULL,("Could not find offset load in thumb compact switch table block"));
      ASSERT(ARM_INS_REFERS_TO(ldrb) != NULL,("Offset load in thumb compact switch table should refer to data bbl: @I",ldrb));
      rel = RELOC_REF_RELOC (ARM_INS_REFERS_TO (ldrb));
      databbl=T_BBL(RELOC_TO_RELOCATABLE (rel)[0]);
      ASSERT(IS_DATABBL(databbl),("Offset load in thumb compact switch table should refer to data bbl: @I",ldrb));
      ASSERT(!ft,("thumb table-based switch with fallthrough: @eiB",i_bbl));

      if(BBL_NEXT_IN_CHAIN(i_bbl) && (BBL_NEXT_IN_CHAIN(i_bbl)!=databbl)) FATAL(("Already next for @B, i.e. @B",i_bbl,BBL_NEXT_IN_CHAIN(i_bbl)));
      if(BBL_PREV_IN_CHAIN(databbl) && (BBL_PREV_IN_CHAIN(databbl)!=i_bbl)) FATAL(("Already prev for @B, i.e. @B != @B",j_bbl,BBL_PREV_IN_CHAIN(databbl),i_bbl));
      BBL_SET_NEXT_IN_CHAIN(i_bbl, databbl);
      BBL_SET_PREV_IN_CHAIN(databbl, i_bbl);

      changed = TRUE;
    }

    if (switch_bbl &&
        ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl))) == ARM_T2TBB) || (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl))) == ARM_T2TBH) || ArmInsIsSwitchedBL(T_ARM_INS(BBL_INS_LAST(i_bbl)))))
    {
      t_bbl *databbl;
      t_arm_ins *tbb;
      t_reloc *rel;

      tbb = T_ARM_INS(BBL_INS_LAST(i_bbl));
      ASSERT(ARM_INS_REFERS_TO(tbb) != NULL, ("TBB/TBH should refer to data BBL @I", tbb));
      rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(tbb));

      databbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
      ASSERT(IS_DATABBL(databbl), ("TBB/TBH should refer to data BBL @I, but instead refers to @B", tbb, databbl));
      ASSERT(!ft,("TBB/TBH switch with fallthrough: @eiB",i_bbl));

      ASSERT(!BBL_NEXT_IN_CHAIN(i_bbl) || (BBL_NEXT_IN_CHAIN(i_bbl) == databbl), ("Already next for @B: @B", i_bbl, BBL_NEXT_IN_CHAIN(i_bbl)));
      ASSERT(!BBL_NEXT_IN_CHAIN(databbl) || (BBL_NEXT_IN_CHAIN(databbl) == i_bbl), ("Already prev for @B: @B != @B", j_bbl, BBL_PREV_IN_CHAIN(i_bbl), i_bbl));
      BBL_SET_NEXT_IN_CHAIN(i_bbl, databbl);
      BBL_SET_PREV_IN_CHAIN(databbl, i_bbl);

      changed = TRUE;
    }

    if ((switch_bbl || (BBL_INS_LAST(i_bbl) && (ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(i_bbl))) & IF_SWITCHJUMP)))
        && !strcmp(GetDiabloPhaseName(INS_PHASE(BBL_INS_LAST(i_bbl))), "AdvancedFactoring"))
    {
      ASSERT(!changed, ("this switch statement has already been changed! @eiB", i_bbl));

      t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(i_bbl));
      t_arm_ins *prev1 = ARM_INS_IPREV(last);

      if (!prev1) {
        t_uint32 nr_incoming_real = 0;

        t_cfg_edge *e;
        BBL_FOREACH_PRED_EDGE(i_bbl, e) {
          if (!CfgEdgeIsFake(e))
            nr_incoming_real++;

          if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
              || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
            prev1 = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(e)));
        }

        /* invalid results */
        if (nr_incoming_real != 1)
          prev1 = NULL;
      }

      t_bbl *databbl = NULL;
      t_bool have_databbl = TRUE;

      if (!switch_bbl)
      {
        VERBOSE(1, ("found bogus switch! @eiB", i_bbl));

        t_arm_ins *ins;
        BBL_FOREACH_ARM_INS(i_bbl, ins) {
          if (ARM_INS_OPCODE(ins) == ARM_STR
              && ARM_INS_REGB(ins) == ARM_REG_R15) {
            t_arm_ins *ins2 = ARM_INS_INEXT(ins);
            while (ins2) {
              if (ARM_INS_OPCODE(ins2) == ARM_LDR
                  && ARM_INS_REGB(ins2) == ARM_REG_R15
                  && ARM_INS_REGA(ins2) == ARM_INS_REGA(ins))
                break;

              ins2 = ARM_INS_INEXT(ins2);
            }

            if (ins2)
              break;
          }
        }

        ASSERT(ins, ("found instruction! @I", ins));

        t_reloc *rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins));
        databbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
      }
      else if (ARM_INS_OPCODE(last) == ARM_ADD
          && prev1
          && ARM_INS_OPCODE(prev1) == ARM_LDR
          && ARM_INS_REGB(prev1) == ARM_REG_R15)
      {
        /* LDR rX, [pc, rX LSL 2]
         * ADD pc, pc, rX */
        t_reloc *rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(last));
        databbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);

        t_bbl *ft_bbl = NULL;
        t_cfg_edge *e;
        BBL_FOREACH_SUCC_EDGE(i_bbl, e)
        {
          if (CFG_EDGE_CAT(e) == ET_SWITCH || CFG_EDGE_CAT(e) == ET_IPSWITCH)
            continue;
          else if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
            ft_bbl = CFG_EDGE_TAIL(e);
          else
            FATAL(("expected switch or fallthrough edge @E @eiB", e, i_bbl));
        }

        t_bbl *first_in_chain = ft_bbl ? ft_bbl : i_bbl;
        ASSERT(!BBL_NEXT_IN_CHAIN(first_in_chain) || (BBL_NEXT_IN_CHAIN(first_in_chain) == databbl), ("Already next for @eiB: @eiB", first_in_chain, BBL_NEXT_IN_CHAIN(i_bbl)));
        ASSERT(!BBL_PREV_IN_CHAIN(databbl) || (BBL_PREV_IN_CHAIN(databbl) == first_in_chain), ("Already prev for @eiB: @eiB != @eiB", databbl, BBL_PREV_IN_CHAIN(first_in_chain), first_in_chain));
        BBL_SET_NEXT_IN_CHAIN(first_in_chain, databbl);
        BBL_SET_PREV_IN_CHAIN(databbl, first_in_chain);
      }
      else if (ARM_INS_OPCODE(last) == ARM_ADD
                && ARM_INS_REGA(last) == ARM_REG_R15
                && ARM_INS_REGB(last) == ARM_REG_R15
                && ARM_INS_SHIFTTYPE(last) == ARM_SHIFT_TYPE_LSL_IMM
                && ARM_INS_SHIFTLENGTH(last) == 2)
      {
        /* ADD pc, pc, rX LSL 2 */
        t_cfg_edge *e;

        /* count the number of switch edges */
        t_uint32 nr_switch_edges = 0;
        t_bbl *ft_bbl = NULL;
        BBL_FOREACH_SUCC_EDGE(i_bbl, e)
        {
          if (CFG_EDGE_CAT(e) == ET_SWITCH || CFG_EDGE_CAT(e) == ET_IPSWITCH)
            nr_switch_edges++;
          else if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH && ArmInsIsConditional(last))
            ft_bbl = CFG_EDGE_TAIL(e);
          else
            FATAL(("expected switch or fallthrough edge @E @eiB", e, i_bbl));
        }

        /* put all the switch edges in an array */
        t_cfg_edge **switch_edges = Malloc(sizeof(t_cfg_edge *) * nr_switch_edges);
        BBL_FOREACH_SUCC_EDGE(i_bbl, e) {
          if (CFG_EDGE_CAT(e) == ET_SWITCH || CFG_EDGE_CAT(e) == ET_IPSWITCH)
            switch_edges[CFG_EDGE_SWITCHVALUE(e)] = e;
        }

        if (ft_bbl)
          databbl = ft_bbl;
        else {
          databbl = BblNew(cfg);
          if (BBL_FUNCTION(i_bbl))
            BblInsertInFunction(databbl, BBL_FUNCTION(i_bbl));
          t_arm_ins *noop_ins;
          ArmMakeInsForBbl(Noop, Append, noop_ins, databbl, FALSE);
        }

        /* chain the subsequent branch BBLs */
        t_bbl *prev_in_chain = databbl;
        for (t_uint32 i = 0; i < nr_switch_edges; i++)
        {
          ASSERT(switch_edges[i], ("what?? case %d, @eiB", i, i_bbl));
          t_bbl *next_in_chain = CFG_EDGE_TAIL(switch_edges[i]);

          if (!(!BBL_NEXT_IN_CHAIN(prev_in_chain) || (BBL_NEXT_IN_CHAIN(prev_in_chain) == next_in_chain)))
            CfgDrawFunctionGraphs(cfg, "already-next");

          ASSERT(!BBL_NEXT_IN_CHAIN(prev_in_chain) || (BBL_NEXT_IN_CHAIN(prev_in_chain) == next_in_chain),
                  ("Already next for @eiB: @eiB", prev_in_chain, BBL_NEXT_IN_CHAIN(prev_in_chain)));
          BBL_SET_NEXT_IN_CHAIN(prev_in_chain, next_in_chain);

          if (!(!BBL_PREV_IN_CHAIN(next_in_chain) || (BBL_PREV_IN_CHAIN(next_in_chain) == prev_in_chain)))
            CfgDrawFunctionGraphs(cfg, "already-prev");

          ASSERT(!BBL_PREV_IN_CHAIN(next_in_chain) || (BBL_PREV_IN_CHAIN(next_in_chain) == prev_in_chain),
                  ("Already prev for @eiB: @eiB != @eiB", next_in_chain, BBL_PREV_IN_CHAIN(prev_in_chain), prev_in_chain));
          BBL_SET_PREV_IN_CHAIN(next_in_chain, prev_in_chain);

          prev_in_chain = next_in_chain;
        }

        ASSERT(!BBL_NEXT_IN_CHAIN(i_bbl) || (BBL_NEXT_IN_CHAIN(i_bbl) == databbl), ("Already next for @eiB: @eiB", i_bbl, BBL_NEXT_IN_CHAIN(i_bbl)));
        ASSERT(!BBL_PREV_IN_CHAIN(databbl) || (BBL_PREV_IN_CHAIN(databbl) == i_bbl), ("Already prev for @eiB: @eiB != @eiB", databbl, BBL_PREV_IN_CHAIN(i_bbl), i_bbl));
        BBL_SET_NEXT_IN_CHAIN(i_bbl, databbl);
        BBL_SET_PREV_IN_CHAIN(databbl, i_bbl);
      }
      else if (ARM_INS_OPCODE(last) == ARM_ADD
                && ARM_INS_REGA(last) == ARM_REG_R15
                && ARM_INS_REGB(last) == ARM_REG_R15)
      {
        /* add pc, pc, <offset> */
        /* add pc, pc, <register> (for distributed tables) */
        have_databbl = FALSE;
      }
      else
        FATAL(("unhandled switch statement created by Advanced Factoring! @eiB", i_bbl));

      changed = TRUE;
    }

    /* Thumb compact switch tables are handled after regular chaining
     * (in the function ArmChainThumbSwitchTables in this file),
     * because on the one hand their bbls should follow the "add"
     * (limited offset range), but on the other hand fallthrough
     * paths between the different target blocks should also be
     * respected.
     */
    else if (switch_bbl && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_ADD) &&
	!(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(i_bbl))) & FL_THUMB))
    /* {{{ Handle switch statements */
    {
      t_bbl ** array= Calloc(sizeof(t_bbl *),switch_max+2); /* 0 holds fallthrough */
      t_bbl * prev=i_bbl;
      int use_index;
      VERBOSE(1,("Found switch block @B, max switch value = %d",i_bbl,switch_max));

      /* collect all target bbls */
      BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
      {
	if ((CFG_EDGE_CAT(i_edge)==ET_SWITCH) || (CFG_EDGE_CAT(i_edge)==ET_IPSWITCH))
	{
	  use_index = CFG_EDGE_SWITCHVALUE(i_edge)+1;
          /* duplicate edge? */
	  if (array[use_index]) FATAL(("Switch values corrupt!"));
	  array[use_index]=CFG_EDGE_TAIL(i_edge);
	}
	else if ((CFG_EDGE_CAT(i_edge)==ET_FALLTHROUGH)||(CFG_EDGE_CAT(i_edge)==ET_IPFALLTHRU))
	{
	  if (array[0]) FATAL(("Multiple fallthroughs in switch"));
	  array[0]=CFG_EDGE_TAIL(i_edge);
	}
	else
	{
	  FATAL(("Weird edge in switch: @E\n",i_edge));
	}
      }

      if (array[0] == array[1])
        array[0] = NULL;
      for (tel=0; tel<switch_max+2; tel++)
      {
        if ((tel < switch_max+1) &&
            (array[tel] == array[tel+1]))
          array[tel] = NULL;
        if (!array[tel])
        {
          t_arm_ins * noop;
          VERBOSE(1,("Added bbl for case %d\n",tel));
          array[tel]=BblNew(cfg);
          if (BBL_FUNCTION(i_bbl))
            BblInsertInFunction(array[tel],BBL_FUNCTION(i_bbl));
          BblInheritSetInformation(array[tel], i_bbl);
          noop=ArmInsNewForBbl(array[tel]);
          ArmInsMakeMov(noop,ARM_REG_R0,ARM_REG_R0,0,ARM_CONDITION_AL);
          ArmInsAppendToBbl(noop,array[tel]);
          CFG_EDGE_SET_SWITCHVALUE(CfgEdgeCreate(cfg,i_bbl,array[tel],ET_SWITCH),tel);
        }
        else if (BBL_INS_FIRST(array[tel])==NULL)
        {
          t_arm_ins * noop;
          VERBOSE(1,("Added NOP in bbl for case %d\n",tel));
          noop=ArmInsNewForBbl(array[tel]);
          ArmInsMakeMov(noop,ARM_REG_R0,ARM_REG_R0,0,ARM_CONDITION_AL);
          ArmInsAppendToBbl(noop,array[tel]);
        }
        /*	else
                printf("%d ok\n",tel);*/
      }

      /* if the fallthrough or another switch block has been eliminated
       * for whatever reason, the fallthrough/switch edge will have
       * been moved to the next BBL. For memory-based switches this
       * is no problem, but for add-based switches this obviously
       * cannot work. So here we add NOP bbl's to solve that problem.
       *
       * TODO: the individual switch blocks may be > 1 instruction!
       *       (in case of "add pc, rX, lsl #4/#8")
       */

       /* prev = head of last chain */
      prev = GetHeadOfChain(prev);

      for (tel=0; tel<switch_max+2; tel++)
      {
        if (!AlreadyInChain(prev, array[tel]))
          AppendBblToChain(prev, array[tel]);
      }

      Free(array);

      changed = TRUE;
    }
    else if (switch_bbl && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_LDR))
    {
      t_bbl *switchtable = NULL;
      t_reloc *rel;

      ASSERT (BBL_INS_LAST (i_bbl), ("switch without instructions"));
      ASSERT (INS_REFERS_TO (BBL_INS_LAST (i_bbl)), ("gcc-style switch should have relocation attached"));
      rel = RELOC_REF_RELOC (INS_REFERS_TO (BBL_INS_LAST (i_bbl)));
      switchtable = T_BBL(RELOC_TO_RELOCATABLE (rel)[0]);
      ASSERT (IS_DATABBL (switchtable), ("switch table not found? @eiB", i_bbl));
      if (!(IS_SWITCH_TABLE(switchtable)))
	FATAL(("No switch table for switch @ieB\n",i_bbl));

      if (!ft)
      {
        ArmInsertNoopBblInChain(cfg,i_bbl,switchtable,(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(i_bbl))) & FL_THUMB) != 0,TRUE);
        changed = TRUE;
      }
      else
      {
        j_bbl = CFG_EDGE_TAIL(ft);

        BBL_SET_NEXT_IN_CHAIN(j_bbl, switchtable);
        BBL_SET_PREV_IN_CHAIN(switchtable, j_bbl);
      }

    }
    else if (switch_bbl
                && (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(i_bbl))) & FL_THUMB)
                && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(i_bbl)))==ARM_MOV)
    {
        /* switch table of branches */
        t_bbl ** array= Calloc(sizeof(t_bbl *),switch_max+1);
        t_int32 csize = -1;

        BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
        {
                /* fill the ORDERED array with the switch edges */
                ASSERT(CFG_EDGE_CAT(i_edge) == ET_SWITCH, ("something is completely fucked up here @E", i_edge));
                ASSERT(CFG_EDGE_SWITCHVALUE(i_edge) <= switch_max, ("this can't be! @E", i_edge));
                ASSERT(BBL_NINS(CFG_EDGE_TAIL(i_edge))==1
                        && (ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge))))
                                || (RegsetIn(ARM_INS_REGS_DEF(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge)))),ARM_REG_R15) && ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge))))==ARM_CONDITION_AL)),
                                ("expected target switch BBL with only one branch instruction, got @eiB", CFG_EDGE_TAIL(i_edge)));

                if (csize != -1)
                  ASSERT(ARM_INS_CSIZE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge)))) == csize,
                            ("all instructions in the switch table should have the same size; have %d vs %d (@I)", csize, ARM_INS_CSIZE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge)))), BBL_INS_LAST(CFG_EDGE_TAIL(i_edge))));
                else
                  csize = ARM_INS_CSIZE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_TAIL(i_edge))));

                array[CFG_EDGE_SWITCHVALUE(i_edge)] = CFG_EDGE_TAIL(i_edge);
        }

        /* the first target is a little bit different */
        if (!AlreadyInChain(GetHeadOfChain(i_bbl), array[0]))
          AppendBblToChain(i_bbl, array[0]);

        for (tel = 1; tel <= switch_max; tel++)
        {
          /* chain the BBL's in order */
          if (!AlreadyInChain(GetHeadOfChain(array[tel-1]), array[tel]))
            AppendBblToChain(array[tel-1], array[tel]);
        }

        Free(array);
        changed = TRUE;
    }
    /* }}} */

  } /* }}} */

  /* The chains have been made. Calculate the amount of chains, and allocate memory for them.
   * Calculate the number of chains by looking at every BBL in the CFG:
   * if the BBL has no previous BBL in its chain, it must be the head
   * of a new chain */
  nchains=0;
  CFG_FOREACH_BBL(cfg,i_bbl)
    if (BBL_PREV_IN_CHAIN(i_bbl)==NULL)
      {
	nchains++;
      }
  chains=Malloc(sizeof(t_bbl *) * nchains);

  /* Fill the chains[] array with the head-BBL of each chain */
  nchains=0;
  CFG_FOREACH_BBL(cfg,i_bbl)
    if (BBL_PREV_IN_CHAIN(i_bbl)==NULL)
      chains[nchains++]=i_bbl;

  /* set BBL_FIRST_IN_CHAIN and BBL_LAST_IN_CHAIN for each BBL in every chain */
  for (tel=0; tel<nchains; tel++)
  {
    t_bbl *last = NULL;
    for (i_bbl=chains[tel]; i_bbl; last = i_bbl, i_bbl = BBL_NEXT_IN_CHAIN(i_bbl))
      BBL_SET_FIRST_IN_CHAIN(i_bbl,  chains[tel]);
    for (i_bbl=chains[tel]; i_bbl; i_bbl=BBL_NEXT_IN_CHAIN(i_bbl))
      BBL_SET_LAST_IN_CHAIN(i_bbl,  last);
  }

  for (tel=0; tel<nchains; tel++)
  {
    t_address chainsize=AddressNullForCfg(cfg);
    for (i_bbl=chains[tel]; i_bbl != NULL ; i_bbl = BBL_NEXT_IN_CHAIN(i_bbl))
    {
      /* Consider "BX PC" Thumb instructions: insert a NOP after the instruction */
      if (BBL_INS_FIRST(i_bbl) && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(i_bbl))) == TH_BX_R15))
      {
	t_arm_ins * ins = ArmInsNewForBbl(i_bbl);
	ArmInsMakeNoop(ins);
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_THUMB);
	ARM_INS_SET_CSIZE(ins, AddressNew32(2));
	ARM_INS_SET_OLD_SIZE(ins, AddressNew32(2));

	ArmInsAppendToBbl(ins, i_bbl);
      }

      /* Set the address of every instruction in the BBL i_bbl,
       * also set the CSIZE-field of the BBL */
      BblSetAddressSuper(i_bbl, BBL_CADDRESS(i_bbl));
      chainsize=AddressAdd(chainsize, BBL_CSIZE(i_bbl));
    }
  }

  /* fill up the ch-structure:
   *  - set the array pointer to the chains
   *  - set the number of chains */
  ch->chains = chains;
  ch->nchains = nchains;

  t_clustercollection * clustercoll = NULL;

  if (diabloarm_options.keepcodeorder)
  {
    ExidxSplitIntoSubsections(cfg);

    if (diabloarm_options.exidxsplitfunctions)
      ExidxCreateFunctionSubsections(cfg);

    /* sort the chains (which actually only consist of one BBL each) by their old address */
    qsort(ch->chains, ch->nchains, sizeof(t_bbl *), BblOldAddressComparer);

    if (diabloarm_options.shuffleexidx)
    {
      /* step 1: create individual chains for each exidx entry */
      ChainExceptionHandled(cfg, ch);
    }

    RandomizeExidx(cfg, ch, diabloarm_options.shuffleexidx);

    MergeAllChains(ch);

    if (!diabloarm_options.nomergeexidx)
      MergeExidx(cfg);
  }
  else
  {
    /* group all chains together that have to be near one another due
     * to the limited jump range of the CB(N)Z Thumb instructions. */
    clustercoll = ArmClusterChains(cfg, ch);
    ArmClusterChainBblsByAddress(clustercoll, ch);
  }

  // Chaining in three steps: from inner switches to outer switches
  //ArmChainThumbSwitchTables(cfg, ARM_T2TBB, ch);
  ArmChainThumbSwitchTables(cfg, ARM_BL,ch);
  ArmChainThumbSwitchTables(cfg, ARM_T2TBH,ch);
  ArmChainThumbSwitchTables(cfg, ARM_ADD,ch);

  ArmValidateChainsForThumbBranches(ch);

  /* sanity check */
  for (tel = 0; tel < ch->nchains; tel++)
  {
    if (!ch->chains[tel]) continue;
    ASSERT(BBL_FIRST_IN_CHAIN(ch->chains[tel]) == ch->chains[tel], ("chain entry in chain holder's list @eiB is not the head of a chain", ch->chains[tel]));
  }

  BblFiniCluster(cfg);

  if (clustercoll)
    ArmClusterCollectionDestroy(clustercoll, TRUE);
} /* }}} */

#define BBL_LIST_INIT_SIZE 100
void ArmBblListAddBbl(t_bbllist * list, t_bbl * b)
{
        if (list->array_size == 0)
        {
                list->bbls = Malloc(sizeof(t_bbl *) * BBL_LIST_INIT_SIZE);
                list->array_size = BBL_LIST_INIT_SIZE;
        }
        else if (list->nbbls >= list->array_size)
        {
                list->bbls = Realloc(list->bbls, sizeof(t_bbl *) * (list->array_size * 2));
                list->array_size *= 2;
        }

        list->bbls[list->nbbls] = b;
        list->nbbls++;
}

int ArmChaininfoComparer(const void * a, const void * b)
{
        t_chaininfo *info1 = *(t_chaininfo**)a;
        t_chaininfo *info2 = *(t_chaininfo**)b;

        return AddressIsLt(info1->lowest, info2->lowest) ? -1 : (AddressIsGt(info1->lowest, info2->lowest) ? 1 : 0);
}

void ArmClusterChainBblsByAddress(t_clustercollection * cc, t_chain_holder * ch)
{
        t_uint32 i = 0, j = 0, k = 0;
        //        t_uint32 newchaincount = 0;
        t_bbl * i_bbl;

        for (i = 0; i < cc->nclusters; i++)
        {
                //DEBUG(("doing cluster %d", i));
                if (!cc->clusters[i]) continue;

                t_chaininfo ** chaininfos = Malloc(sizeof(t_chaininfo *) * cc->clusters[i]->nchains);
                int superchainidx = -1;

                /* iterate over every chain in this cluster */
                for (j = 0; j < cc->clusters[i]->nchains; j++)
                {
                        ASSERT(cc->clusters[i]->chains[j], ("NULL entry in chain list of cluster"));
                        //DEBUG(("  doing chain %d", j));
                        /* start with the maximal value for address */
                        chaininfos[j] = Malloc(sizeof(t_chaininfo));
                        chaininfos[j]->chain = cc->clusters[i]->chains[j]->head;
                        chaininfos[j]->lowest = ~0;

                        /* remove the chain from the chain holder list,
                         * and mark an entry where we can put our new superchain in */
                        for (k = 0; k < ch->nchains; k++)
                                if (ch->chains[k] == cc->clusters[i]->chains[j]->head)
                                {
                                        if (superchainidx == -1)
                                                superchainidx = k;
                                        ch->chains[k] = NULL;
                                }

                        ASSERT(superchainidx >= 0, ("could not find head of chain in the chainholder's list"));

                        /* find the lowest non-zero address */
                        i_bbl = chaininfos[j]->chain;
                        do
                        {
                                if ((BBL_OLD_ADDRESS(i_bbl) < chaininfos[j]->lowest) && (BBL_OLD_ADDRESS(i_bbl) != 0))
                                        chaininfos[j]->lowest = BBL_OLD_ADDRESS(i_bbl);
                        } while ((i_bbl = BBL_NEXT_IN_CHAIN(i_bbl)));

                        //DEBUG(("Setting lowest for chain %d @B: @G", j, chaininfos[j]->chain, chaininfos[j]->lowest));
                }

                /* sort the chains based on their lowest address */
                //DEBUG(("Sorting %d chains based on lowest address", cc->clusters[i]->nchains));
                for (j = 0; j < cc->clusters[i]->nchains; j++)
                        //DEBUG((" Lowest address of chain @B: @G", chaininfos[j]->chain, chaininfos[j]->lowest));
                qsort(chaininfos, cc->clusters[i]->nchains, sizeof(t_chaininfo *), ArmChaininfoComparer);

                for (j = 0; j < cc->clusters[i]->nchains; j++)
                {
                        if (j == 0) continue;

                        //DEBUG(("Merging chain @B with @B", chaininfos[0]->chain, chaininfos[j]->chain));
                        MergeChains(chaininfos[0]->chain, chaininfos[j]->chain);

                        Free(chaininfos[j]);
                }

                ch->chains[superchainidx] = chaininfos[0]->chain;
                //DEBUG(("Printing superchain"));
                //ArmChainPrint(chaininfos[0]->chain);

                Free(chaininfos[0]);
                Free(chaininfos);
        }

        /* clean up the chain holder's chain list by removing all NULL pointers */
        /* this is only temporary, we can just put NULL in each removed entry of this list */
        // newchaincount = 0;
        // for (i = 0; i < ch->nchains; i++)
        //         if (ch->chains[i])
        //                 newchaincount++;

        // t_bbl ** newchainlist = Malloc(sizeof(t_bbl *) * newchaincount);
        // newchaincount = 0;
        // for (i = 0; i < ch->nchains; i++)
        //         if (ch->chains[i])
        //         {
        //                 newchainlist[newchaincount] = ch->chains[i];
        //                 newchaincount++;
        //         }

        // Free(ch->chains);
        // ch->chains = newchainlist;
        // ch->nchains = newchaincount;
}

t_clustercollection * ArmSingleCluster(t_cfg * cfg, t_chain_holder * ch)
{

  t_bbl * i_bbl;
  t_uint32 tel = 0;

  t_clustercollection * clustercoll = ArmClusterCollectionCreate();

  t_cluster * cluster = ArmClusterCreate();

  for (tel = 0; tel < ch->nchains; tel++)
    {
      /* create a new instance of t_cluster and add it to the array
       * of chains in the cluster */
      ArmClusterAddChain(cluster, ArmClusterChainCreate(ch->chains[tel]));
      
      /* then add every BBL of the current chain to the newly created cluster */
      i_bbl = ch->chains[tel];
      ASSERT(i_bbl, ("OMG, this chain has been decapitated!"));
      do
        {
          BBL_SET_CLUSTER(i_bbl, cluster);
        } while ((i_bbl = BBL_NEXT_IN_CHAIN(i_bbl)));
    }
     
  /* finally, add the new cluster to the global cluster collection */
  ArmClusterCollectionAddCluster(clustercoll, cluster);

  return clustercoll;
}


/* This function implements an algorithm to group chains together, which we call "clusters" of chains.
 *
 * \param cfg The control flow graph
 * \param ch A datastructure containing the chains that have been constructed
 */
t_clustercollection * ArmClusterChains(t_cfg * cfg, t_chain_holder * ch)
{
        t_clustercollection * ret;
        t_bbl * i_bbl;
        t_cfg_edge * i_edge;
        t_bool changed;
        t_uint32 tel = 0, tel2 = 0;

        VERBOSE(2,("clustering %d chains", ch->nchains));

        /* create a new instance of t_clustercollection,
         * which holds information of and references to all clusters. */
        t_clustercollection * clustercoll = ArmClusterCollectionCreateSized(1000);

        /* PHASE 1 ################################################################################ */
        /* Initially, put each chain in its own cluster */
        //DEBUG(("  putting every chain in its own cluster ..."));
        for (tel = 0; tel < ch->nchains; tel++)
        {
                /* create a new instance of t_cluster and add it to the array
                 * of chains in the cluster */
                t_cluster * cluster = ArmClusterCreate();
                ArmClusterAddChain(cluster, ArmClusterChainCreate(ch->chains[tel]));

                /* then add every BBL of the current chain to the newly created cluster */
                i_bbl = ch->chains[tel];
                ASSERT(i_bbl, ("OMG, this chain has been decapitated!"));
                do
                {
                        BBL_SET_CLUSTER(i_bbl, cluster);
                } while ((i_bbl = BBL_NEXT_IN_CHAIN(i_bbl)));

                /* finally, add the new cluster to the global cluster collection */
                ArmClusterCollectionAddCluster(clustercoll, cluster);
        }
        //DEBUG(("     %d clusters created", clustercoll->nclusters));

        /* PHASE 2 ################################################################################ */
        /* make the algorithm faster: we don't modify the chains here,
         * so we can collect information of the BBLs inside the chains
         * once and for all. This way, we don't have to re-iterate over
         * the BBL's in the chains every time we have to calculate the
         * clusters referred to by this chain. */

        /* walk over every newly created cluster */
        //DEBUG(("  looking up every cluster referenced by each chain"));
        for (tel = 0; tel < clustercoll->nclusters; tel++)
        {
                /* retrieve the one and only chain in this cluster */
                t_cluster * currentcluster = clustercoll->clusters[tel];
                t_clusterchain * chain = currentcluster->chains[0];

                /* walk over every BBL in this chain, and collect the
                 *  - clusters from which CB(N)Z jump to this chain
                 *  - clusters to which the CB(N)Z instructions inside this chain jump
                 * A new entry should only be added if the source/target cluster is
                 * different from the current one. */
                i_bbl = chain->head;
                do
                {
                        BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
                        {
                                t_bbl * head = CFG_EDGE_HEAD(i_edge);

                                if (head && BBL_INS_LAST(head))
                                {
                                        t_arm_ins * ins = T_ARM_INS(BBL_INS_LAST(head));

                                        if ((ARM_INS_OPCODE(ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(ins) == ARM_T2CBNZ)
                                            || (ARM_INS_OPCODE(ins)==ARM_T2TBB)
                                            || ArmInsSwitchedBLTableEntrySize(ins)==1)
                                        {
                                                if (BBL_CLUSTER(head) != currentcluster)
                                                        ArmClusterCollectionAddCluster(chain->linked_clusters, BBL_CLUSTER(head));
                                                /*else
                                                        DEBUG(("CB(N)Z found in pred @B, but refers to own chain", i_bbl));*/
                                        }
                                }
                        }

                        BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
                        {
                                t_bbl * tail = CFG_EDGE_TAIL(i_edge);

                                if (tail && BBL_INS_LAST(i_bbl))
                                {
                                        t_arm_ins * ins = T_ARM_INS(BBL_INS_LAST(i_bbl));

                                        if ((ARM_INS_OPCODE(ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(ins) == ARM_T2CBNZ)
                                            || (ARM_INS_OPCODE(ins)==ARM_T2TBB)
                                            || ArmInsSwitchedBLTableEntrySize(ins)==1)
                                        {
                                                if (BBL_CLUSTER(tail) != currentcluster)
                                                        ArmClusterCollectionAddCluster(chain->linked_clusters, BBL_CLUSTER(tail));
                                                /*else
                                                        DEBUG(("CB(N)Z found in succ @B, but refers to own chain", i_bbl));*/
                                        }
                                }
                        }
                } while ((i_bbl = BBL_NEXT_IN_CHAIN(i_bbl)));

                if (chain->linked_clusters->nclusters == 0)
                {
                        /* this chain does not refer to any cluster, remove it from the work list */
                        ArmClusterChainDestroy(chain);
                        ArmClusterDestroy(currentcluster);

                        clustercoll->clusters[tel] = NULL;
                }
                else
                {
                        /* remove duplicate entries to speed up the algorithm */
                        ArmClusterCollectionRemoveDuplicates(chain->linked_clusters);
                        //DEBUG(("  %d clusters linked to chain @B", chain->linked_clusters->nclusters, chain->head));
                }
        }

        /* PHASE 3 ################################################################################ */
        changed = TRUE;
        //DEBUG(("start merging clusters together"));
        while (changed)
        {
                t_cluster * currentcluster = NULL;
                t_uint32 currentclusteridx;
                t_cluster * linkedcluster = NULL;
                t_uint32 linkedclusteridx;
                t_clusterchain * currentchain = NULL;
                t_uint32 i = 0, j = 0, k = 0;
                t_bool found = FALSE;

                changed = FALSE;

                /* find a cluster
                 *   which contains a chain that is linked to another cluster. */
                for (i = 0; i < clustercoll->nclusters; i++)
                {
                        currentcluster = clustercoll->clusters[i];
                        currentclusteridx = i;

                        if (!currentcluster) continue;

                        for (j = 0; j < currentcluster->nchains; j++)
                        {
                                currentchain = currentcluster->chains[j];

                                /* check if there actually are clusters linked to this chain */
                                for (k = 0; k < currentchain->linked_clusters->nclusters; k++)
                                {
                                        if (currentchain->linked_clusters->clusters[k])
                                        {
                                                /* if there is at least one, we have found a candidate for merging */
                                                linkedcluster = currentchain->linked_clusters->clusters[k];
                                                found = TRUE;
                                                break;
                                        }
                                }
                        }

                        if (found) break;
                }

                if (!found) break;
                changed = TRUE;

                /* merge both clusters into a new one */
                ASSERT(currentcluster != linkedcluster, ("merging cluster with itself"));
                t_cluster * newcluster = ArmClusterMerge(currentcluster, linkedcluster);

                /* remove references from currentcluster to linkedcluster from their linked_clusters lists.
                 * Also do it vice-versa */
                ArmClusterRemoveLinkToCluster(currentcluster, linkedcluster);
                ArmClusterRemoveLinkToCluster(linkedcluster, currentcluster);

                /* remove the currentcluster from the cluster list by replacing it
                 * with the newly created cluster */
                clustercoll->clusters[currentclusteridx] = newcluster;

                /* remove the linkedcluster from the cluster list by replacing it
                 * with the last cluster in the list.
                 * CAUTION: If the linkedcluster IS the last one in the list, just
                 *          remove the last element of the list. */
                linkedclusteridx = 0;
                while ((clustercoll->clusters[linkedclusteridx] != linkedcluster) && (linkedclusteridx < clustercoll->nclusters))
                        linkedclusteridx++;
                clustercoll->clusters[linkedclusteridx] = NULL;

                /* iterate over every linked cluster, and replace any references
                 * to either the currentcluster or the linkedcluster by a reference
                 * to the newly created cluster. */
                for (i = 0; i < clustercoll->nclusters; i++)
                {
                        if (!clustercoll->clusters[i]) continue;

                        for (j = 0; j < clustercoll->clusters[i]->nchains; j++)
                                for (k = 0; k < clustercoll->clusters[i]->chains[j]->linked_clusters->nclusters; k++)
                                        if ((clustercoll->clusters[i]->chains[j]->linked_clusters->clusters[k] == currentcluster) ||
                                                (clustercoll->clusters[i]->chains[j]->linked_clusters->clusters[k] == linkedcluster))
                                        {
                                                clustercoll->clusters[i]->chains[j]->linked_clusters->clusters[k] = newcluster;
                                                ASSERT(clustercoll->clusters[i] != newcluster, ("cluster has chain referrring to itself"));
                                        }
                }

                /* remove the old clusters that have been merged into one */
                ArmClusterDestroy(currentcluster);
                ArmClusterDestroy(linkedcluster);
        }
        VERBOSE(2,("Clustering done"));

        //DEBUG(("Cleaning up the remaining clusters"));

        /* count the remaining clusters */
        tel = 0;
        for (tel2 = 0; tel2 < clustercoll->nclusters; tel2++)
        {
                if (clustercoll->clusters[tel2])
                        tel++;
        }
        VERBOSE(2,("found %d clusters", tel));

        /* create a new ClusterCollection instance of exactly the right size */
        ret = ArmClusterCollectionCreateSized(tel);
        ret->nclusters = tel;

        /* copy all non-NULL cluster pointers to the newly created structure */
        tel2 = 0;
        for (tel = 0; tel < clustercoll->nclusters; tel++)
                if (clustercoll->clusters[tel])
                {
                        ret->clusters[tel2] = clustercoll->clusters[tel];
                        tel2++;
                }
        ArmClusterCollectionDestroy(clustercoll, FALSE);

        //DEBUG(("%d clusters remain", ret->nclusters));
        /*for (tel = 0; tel < ret->nclusters; tel++)
        {
                DEBUG(("Cluster %d", tel));
                ArmClusterPrint(ret->clusters[tel]);
        }*/

        return ret;
}

void ArmClusterPrint(t_cluster * c)
{
        t_uint32 i;
        t_bbl * i_bbl;

        DEBUG(("  Cluster %p contains %d chains", c, c->nchains));
        for (i = 0; i < c->nchains; i++)
        {
                i_bbl = c->chains[i]->head;
                DEBUG(("    Chain @B", i_bbl));

                do
                {
                        DEBUG(("       @B", i_bbl));
                } while ((i_bbl = BBL_NEXT_IN_CHAIN(i_bbl)));
        }
}

#define INIT_CLUSTER_ARRAY_SIZE 1
t_clustercollection * ArmClusterCollectionCreate()
{
        return ArmClusterCollectionCreateSized(INIT_CLUSTER_ARRAY_SIZE);
}
t_clustercollection * ArmClusterCollectionCreateSized(t_uint32 n)
{
        t_clustercollection * clustercoll = Malloc(sizeof(t_clustercollection));
        clustercoll->clusters = (n == 0) ? NULL : Malloc(sizeof(t_cluster *) * n);
        clustercoll->array_size = n;
        clustercoll->nclusters = 0;

        return clustercoll;
}
void ArmClusterCollectionDestroy(t_clustercollection * cc, t_bool everything)
{
  if (everything)
  {
    for (t_uint32 i = 0; i < cc->nclusters; i++)
    {
      for (t_uint32 j = 0; j < cc->clusters[i]->nchains; j++)
        ArmClusterChainDestroy(cc->clusters[i]->chains[j]);

      ArmClusterDestroy(cc->clusters[i]);
    }
  }
  if (cc->clusters)
    Free(cc->clusters);
  Free(cc);
}

void ArmClusterCollectionAddCluster(t_clustercollection * cc, t_cluster * c)
{
        if (cc->nclusters >= cc->array_size)
        {
                cc->clusters = Realloc(cc->clusters, sizeof(t_cluster *) * (cc->array_size * 2));
                cc->array_size *= 2;
        }

        cc->clusters[cc->nclusters] = c;
        cc->nclusters++;
}

int ArmClusterAddressComparer(const void * c1, const void * c2)
{
        t_cluster *a = *(t_cluster**)c1;
        t_cluster *b = *(t_cluster**)c2;

        return (a > b) ? 1 : ((a < b) ? -1 : 0);
}


void ArmClusterCollectionRemoveDuplicates(t_clustercollection * cc)
{
        t_cluster ** newclusters;
        t_uint32 i = 0;
        t_uint32 newclustercount = 0;

        if (cc->nclusters == 0)
                return;

        /* At worst, no clusters are equal, so none can be removed */
        newclusters = Malloc(sizeof(t_cluster *) * cc->nclusters);

        /* firstly, sort the clusters according to their memory address */
        qsort(cc->clusters, cc->nclusters, sizeof(t_cluster *), ArmClusterAddressComparer);

        for (i = 0; i < cc->nclusters; i++)
        {
                if (!cc->clusters[i]) continue;
                if ((i >= 1) && (cc->clusters[i] == cc->clusters[i-1])) continue;

                /* only add a new cluster to the list if it is unique */
                newclusters[newclustercount] = cc->clusters[i];
                newclustercount++;
        }

        Free(cc->clusters);

        cc->clusters = newclusters;
        cc->nclusters = newclustercount;
}

t_cluster * ArmClusterMerge(t_cluster * a, t_cluster * b)
{
        t_cluster * newc = Malloc(sizeof(t_cluster));
        t_uint32 i = 0, j = 0;

        /* allocate as much chains as needed */
        newc->chains = Malloc(sizeof(t_clusterchain *) * (a->nchains + b->nchains));
        newc->array_size = a->nchains + b->nchains;
        newc->nchains = a->nchains + b->nchains;

        /* copy the chains from a/b to the new cluster */
        j = 0;
        for (i = 0; i < a->nchains; i++)
        {
                newc->chains[j] = a->chains[i];
                j++;
        }
        for (i = 0; i < b->nchains; i++)
        {
                newc->chains[j] = b->chains[i];
                j++;
        }

        return newc;
}

#define INIT_CHAIN_ARRAY_SIZE 1
t_cluster * ArmClusterCreate()
{
        t_cluster * cluster = Malloc(sizeof(t_cluster));
        cluster->chains = Malloc(sizeof(t_clusterchain *) * INIT_CHAIN_ARRAY_SIZE);
        cluster->array_size = INIT_CHAIN_ARRAY_SIZE;
        cluster->nchains = 0;

        return cluster;
}
void ArmClusterDestroy(t_cluster * c)
{
        Free(c->chains);
        Free(c);
}

void ArmClusterAddChain(t_cluster * c, t_clusterchain * chain)
{
        if (c->nchains >= c->array_size)
        {
                c->chains = Realloc(c->chains, sizeof(t_clusterchain *) * (c->array_size * 2));
                c->array_size *= 2;
        }

        c->chains[c->nchains] = chain;
        c->nchains++;
}

t_clusterchain * ArmClusterChainCreate(t_bbl * chain_head)
{
        t_clusterchain * clusterchain = Malloc(sizeof(t_clusterchain));
        clusterchain->head = chain_head;
        clusterchain->linked_clusters = ArmClusterCollectionCreate();

        return clusterchain;
}
void ArmClusterChainDestroy(t_clusterchain * cc)
{
        ArmClusterCollectionDestroy(cc->linked_clusters, FALSE);
        Free(cc);
}

void ArmClusterRemoveLinkToCluster(t_cluster * c, t_cluster * link)
{
        t_uint32 i = 0, j = 0;

        for (i = 0; i < c->nchains; i++)
        {
                t_clustercollection * clusterlist = c->chains[i]->linked_clusters;

                for (j = 0; j < clusterlist->nclusters; j++)
                {
                        /* every entry is unique, therefore we can break after one match is found */
                        if (clusterlist->clusters[j] == link)
                        {
                                /* remove this reference to 'link' */
                                clusterlist->clusters[j] = NULL;
                        }
                }
        }
}

void ArmValidateChainsForThumbBranches(t_chain_holder * ch)
{
  t_uint32 tel = 0;
  /* 1. Iterate over the existing chains, FATAL when a chain is encountered
   *    in which multiple CB(N)Z instructions jump to DIFFERENT other chains. */
  for (tel = 0; tel < ch->nchains; tel++)
  {
        t_bbl * current_chain = ch->chains[tel];
        t_bbl * tmp = current_chain;
        t_bbl * target_chain = NULL;

        if (!current_chain) continue;

        /* walk over every BBL in the current chain */
        while (tmp)
        {
          if (BBL_INS_LAST(tmp)) {
            /* only look at BBL's that end with a CB(N)Z instruction */
            if ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(tmp))) == ARM_T2CBZ) || (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(tmp))) == ARM_T2CBNZ))
              {
                t_bbl * cbtarget_chain = NULL;
                t_cfg_edge * i_edge = NULL;
                t_cfg_edge * cbedge = NULL;

                /* get the outgoing edge of this BBL which corresponds with the CB(N)Z instruction.
                 * The corresponding edge is stored in 'cbedge'. */
                BBL_FOREACH_SUCC_EDGE(tmp, i_edge)
                  {
                    if (CFG_EDGE_CAT(i_edge) == ET_JUMP || CFG_EDGE_CAT(i_edge) == ET_IPJUMP)
                      {
                        ASSERT(!cbedge, ("multiple ET_JUMP edges from BBL ending with CB(N)Z @eiB", tmp));
                        cbedge = i_edge;
                        break;
                      }
                  }

                ASSERT(cbedge, ("no valid outgoing edge found for a BBL ending with CB(N)Z @eiB", tmp));

                /* retrieve the chain to which this CB(N)Z instruction will jump */
                cbtarget_chain = BBL_FIRST_IN_CHAIN(CFG_EDGE_TAIL(cbedge));

                /* only look at targets in a different chain than the current one */
                if (cbtarget_chain != current_chain)
                {
                        if (cbtarget_chain != target_chain)
                        {
                                if (target_chain)
                                {
                                        DEBUG(("current chain"));
                                        ArmChainPrint(current_chain);
                                        DEBUG(("target chain"));
                                        ArmChainPrint(target_chain);
                                        DEBUG(("cbtarget chain"));
                                        ArmChainPrint(cbtarget_chain);
                                        FATAL(("multiple CB(N)Z instructions in chain jump to different chains - source chain head @B, targets: [@B, @B]", current_chain, target_chain, cbtarget_chain));
                                }
                                target_chain = cbtarget_chain;
                        }
                }
              }
          }

          tmp = BBL_NEXT_IN_CHAIN(tmp);
        }
    }

  /* 2. Iterate over the existing chains, FATAL when a chain is encountered
   *    with multiple incoming edges, coming from CB(N)Z instructions. */
  for (tel = 0; tel < ch->nchains; tel++)
  {
        t_bbl * current_chain = ch->chains[tel];
        t_bbl * tmp = current_chain;
        t_bbl * source_chain = NULL;

        if (!current_chain) continue;

        while(tmp)
        {
                t_cfg_edge * i_edge = NULL;

                /* look at the incoming JUMP edges of this BBL */
                BBL_FOREACH_PRED_EDGE(tmp, i_edge)
                {
                        if (CFG_EDGE_CAT(i_edge) == ET_JUMP)
                        {
                                /* only look at edges originating from CB(N)Z instructions */
                          if ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)))) == ARM_T2CBZ) ||
                              (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)))) == ARM_T2CBNZ) ||
                              ArmInsSwitchedBLTableEntrySize(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)))) == 1)
                                {
                                        t_bbl * cbsource_chain = BBL_FIRST_IN_CHAIN(CFG_EDGE_HEAD(i_edge));

                                        /* only look at sources in different chains than the current one */
                                        if (cbsource_chain != current_chain)
                                        {
                                                if (cbsource_chain != source_chain)
                                                {
                                                        ASSERT(!source_chain, ("multiple CB(N)Z instructions in differen chains jump to one chain - target chain head @B, sources: [@B, @B]", current_chain, source_chain, cbsource_chain));
                                                        source_chain = cbsource_chain;
                                                }
                                        }
                                }
                        }
                }

                tmp = BBL_NEXT_IN_CHAIN(tmp);
        }
  }
}

/*!
 *
 *
 * \param cfg
 *
 * \return t_bbl *
*/
/*  ArmChain {{{ */
static t_bbl * ArmChain(t_cfg * cfg)
{
  t_bbl * ret;
  t_chain_holder ch;

  ArmCreateChains(cfg,&ch);

  if (diabloarm_options.orderseed != -1)
  {
    if (diabloarm_options.orderseed == 0 || diabloarm_options.keepcodeorder)
      ArmSortChains(&ch);
    else
      ArmRandomChainOrder(&ch);
  }

  DiabloBrokerCall("AfterChainsOrdered", cfg, &ch);

  MergeAllChains(&ch);

  ret = ch.chains[0];
  Free(ch.chains);
  return ret;
}
/* }}} */

/* {{{ generates instructions for float producers.
 * this function always generates worst-case code: for better behaviour, it
 * should be merged with the address/constant producer generation code */
void ArmGenerateFloatProducers(t_object *obj)
{
  t_uint32 i;
  t_section *sec;
  t_bbl *chain, *bbl;
  t_arm_ins *ins;

  OBJECT_FOREACH_CODE_SECTION (obj, sec, i)
  {
    chain = T_BBL (SECTION_TMP_BUF (sec));
    CHAIN_FOREACH_BBL (chain, bbl)
    {
      BBL_FOREACH_ARM_INS (bbl, ins)
      {
	if (ARM_INS_OPCODE (ins) == ARM_FLOAT_PRODUCER)
	{
	  t_arm_ins *added;

          WARNING(("float producers of the old type are supposed not to exist with modern tool flows"));

	  added = (t_arm_ins *)InsNewForBbl (bbl);
	  ARM_INS_SET_CSIZE(added, AddressNew32(4));
	  ARM_INS_SET_MIN_SIZE(added, AddressNew32(4));
	  ArmInsMakeNoop(added);
	  ArmInsInsertAfter(added,ins);

	  added = (t_arm_ins *)InsNewForBbl (bbl);
	  ARM_INS_SET_CSIZE(added, AddressNew32(4));
	  ARM_INS_SET_MIN_SIZE(added, AddressNew32(4));
	  ArmInsMakeNoop(added);
	  ArmInsInsertAfter(added,ins);

	  if ((ARM_INS_FLAGS(ins) & FL_FLT_DOUBLE) ||
	      (ARM_INS_FLAGS(ins) & FL_FLT_DOUBLE_EXTENDED) ||
	      (ARM_INS_FLAGS(ins) & FL_FLT_PACKED) ||
              (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE))
	  {
	    added = (t_arm_ins *)InsNewForBbl (bbl);
	    ARM_INS_SET_CSIZE(added, AddressNew32(4));
	    ARM_INS_SET_MIN_SIZE(added, AddressNew32(4));
	    ArmInsMakeNoop(added);
	    ArmInsInsertAfter(added,ins);
	  }

	  if ((ARM_INS_FLAGS(ins) & FL_FLT_DOUBLE_EXTENDED) ||
	      (ARM_INS_FLAGS(ins) & FL_FLT_PACKED))
	  {
	    added = (t_arm_ins *)InsNewForBbl (bbl);
	    ARM_INS_SET_CSIZE(added, AddressNew32(4));
	    ARM_INS_SET_MIN_SIZE(added, AddressNew32(4));
	    ArmInsMakeNoop(added);
	    ArmInsInsertAfter(added,ins);
	  }
	  GenerateInstructionsForFloatProd(ins);
	}
      }
    }
  }
} /* }}} */


static int ArmMaxThumbBranchDisp(t_cfg_edge *edge)
{
  t_arm_ins *ins;
  int ret = 0;

  /* we expect only Thumb instructions here */
  ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  ASSERT(ARM_INS_FLAGS(ins) & FL_THUMB, ("calculating maximal Thumb branch displacement for non-thumb instruction @I (edge @E)", ins, edge));

  switch (ARM_INS_OPCODE(ins))
  {
  case ARM_B:
    if (!diabloarm_options.fullthumb2)
      ASSERT(ARM_INS_CSIZE(ins) == 2, ("Can only have 16-bit B-instructions in Thumb-1! @I", ins));

    if (ArmInsIsConditional(ins))
      ret = (ARM_INS_CSIZE(ins) == 2) ? 1<<8 : 1<<20;
    else
      ret = (ARM_INS_CSIZE(ins) == 2) ? 1<<11 : 1<<24;
    break;

  case ARM_BL:
    if (ArmInsIsSwitchedBL(ins))
    {
      t_uint32 sz = ArmInsSwitchedBLTableEntrySize(ins);
      ASSERT(sz, ("the table entry size for @I could not be determined", ins));
      ret = 1<<sz*8;

      /* if there is sign extension, one bit is used as a sign bit */
      if (ARM_INS_FLAGS(ins) & FL_SWITCHEDBL_SIGNEXT)
        ret >>= 1;

      break;
    }
  case ARM_BLX:
    ret = 1<<24;
    break;

  case ARM_T2CBZ:
  case ARM_T2CBNZ:
    ret = 1<<7;
    ASSERT(diabloarm_options.fullthumb2, ("The 32-bit Thumb-2 instructions CB(N)Z do no exist in Thumb-1! @I", ins));
    break;

  case ARM_T2TBB:
    ret = 1<<9;
    ASSERT(diabloarm_options.fullthumb2, ("The 32-bit Thumb-2 instruction TBB does not exist in Thumb-1! @I", ins));
    break;

  case ARM_T2TBH:
    ret = 1<<17;
    ASSERT(diabloarm_options.fullthumb2, ("The 32-bit Thumb-2 instruction TBH does not exist in Thumb-1! @I", ins));
    break;

  default:
    FATAL(("unknown Thumb branch instruction @I", ins));
  }

  /* 2-complement -> backward jumps can go a little further */
  if (AddressIsGt(BBL_CADDRESS(CFG_EDGE_TAIL(edge)),BBL_CADDRESS(CFG_EDGE_HEAD(edge))))
    ret -= 2;
  else
    ret = -ret;

  return ret;
}

static int ArmMaxBranchDisp(t_cfg_edge *edge)
{
  t_arm_ins *ins;
  int ret = 0;

  /* we expect only ARM instructions here */
  ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  ASSERT(!(ARM_INS_FLAGS(ins) & FL_THUMB), ("calculating maximal ARM branch displacement for thumb instruction @I (edge @E)", ins, edge));

  switch (ARM_INS_OPCODE(ins))
  {
  case ARM_B:
  case ARM_BL:
  case ARM_BLX:
    ret = 1<<25;
    break;

  default:
    FATAL(("unknown ARM branch instruction @I", ins));
  }

  /* 2-complement -> backward jumps can go a little further */
  if (AddressIsGt(BBL_CADDRESS(CFG_EDGE_TAIL(edge)),BBL_CADDRESS(CFG_EDGE_HEAD(edge))))
    ret -= 4;
  else
    ret = -ret;

  return ret;
}

/* This function ... TODO
 *  \param bbl The BBL of which the last instruction is the branch instruction which needs an island
 *  \param curr_disp The current displacement, caused by extra branch islands inserted between the branch instructions and its destination
 *  \param curr_max_disp The maximal allowed displacement for the branch instruction
 *  \param backwards TRUE if the branch jumps backwards
 *  \param skip_succ_edges TRUE if the successors of the BBL should not be checked
 */
static void AddPotentialBranchIslandOverhead(t_bbl *bbl, t_address *curr_disp, t_uint32 curr_max_disp, t_bool backwards, t_bool skip_succ_edges)
{
  t_cfg_edge *edge;
  int branch_disp;
  int max_disp;
  t_arm_ins * branch_ins;

  /* When creating a branch island, at worst 2 instructions have to be added to the CFG:
   *  - a small (16-bit) Thumb branch instruction which reconstructs the original flow;
   *  - a large (32-bit) Thumb branch island
   * The smaller instruction is only added when the island is inserted in an existing
   * BBL, which means the original program flow is broken. The smaller instruction
   * compensates this by inserting a branch instruction which jumps over the island. */
  /* This number should be changed to 4 for Thumb-1: an island consists of 2 16-bit instructions */
  int island_overhead = 0;

  if (BblIsExitBlock(bbl)) return;
  if (BBL_IS_HELL(bbl)) return;

  BBL_FOREACH_PRED_EDGE(bbl,edge)
  {
    /* only look at JUMPs or interprocedural (IP) JUMPs */
    if (!(CFG_EDGE_CAT(edge) & (ET_JUMP | ET_IPJUMP))) continue;

    /* if the edge is comes from HELL, do not add overhead */
    if (BBL_IS_HELL(CFG_EDGE_HEAD(edge))) continue;

    /* only look at branch-immediate instructions */
    branch_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
    if (!ArmIsBranchImmediate(branch_ins)) continue;

    branch_disp = ArmCalculateBranchDisplacement(edge);

    if (ARM_INS_FLAGS(branch_ins) & FL_THUMB)
    {
      max_disp = ArmMaxThumbBranchDisp(edge);
      island_overhead = 6;
    }
    else
    {
      max_disp = ArmMaxBranchDisp(edge);
      island_overhead = 8;
    }

    /* if a branch to this bbl may need an extra branch island, assume it
     * will need it and that it will appear between our begin and end.
     *
     * BRANCH ... (island) ... DESTINATION
     */
    if (abs(branch_disp) >= abs(max_disp))
      *curr_disp = AddressAddUint32(*curr_disp, island_overhead);

    /* and if the current branch may end up before the target of this branch,
     * this branch in turn may have to be extended (thereby pushing back
     * the current branch etc -> break cycle)
     */
    else if (backwards &&
             ((curr_max_disp-G_T_UINT32(*curr_disp)) <= abs(branch_disp)))
      *curr_disp = AddressAddUint32(*curr_disp, island_overhead);
  }

  if (!skip_succ_edges)
  {
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if (!(CFG_EDGE_CAT(edge) & (ET_JUMP | ET_IPJUMP))) continue;
      if (BblIsExitBlock(CFG_EDGE_TAIL(edge))) continue;
      if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))) continue;

      /* only look at branch-immediate instructions */
      branch_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
      if (!ArmIsBranchImmediate(branch_ins)) continue;

      branch_disp = ArmCalculateBranchDisplacement(edge);

      if (ARM_INS_FLAGS(branch_ins) & FL_THUMB)
      {
        max_disp = ArmMaxThumbBranchDisp(edge);
        island_overhead = 6;
      }
      else
      {
        max_disp = ArmMaxBranchDisp(edge);
        island_overhead = 8;
      }

      if (abs(branch_disp) >= abs(max_disp))
        *curr_disp = AddressAddUint32(*curr_disp, island_overhead);
      else if (!backwards &&
               ((curr_max_disp-G_T_UINT32(*curr_disp)) <= abs(branch_disp)))
        *curr_disp = AddressAddUint32(*curr_disp, island_overhead);
    }
  }
}

#if 0
static void CreateBranchLinkTrampoline(t_cfg *cfg, t_cfg_edge *edge)
{
  t_arm_ins *i_ins, *branch;
  t_bbl *target, *source, *new_target;

  /* we start with
   *
   *   b  .L1
   *   <more than 2KB of code/data>
   * .L1:
   *
   * we change it into
   *
   *   push {r0,r14}
   *   bl .L1
   *   <more than 2KB of code/data>
   *   b .L2
   * L1:
   *   ldr r0,[sp,#4]  ;  "pop {r14}" does not exist in thumb,
   *                   ;  and neither does "ldr r14,[sp,#4]"
   *   mov r14,r0
   *   ldr r0,[sp]
   *   add sp, #8
   * L2:
   */

  VERBOSE(2,("  -> Creating branch & link for @E",edge));

  /* push {r0,r14} */
  source = CFG_EDGE_HEAD(edge);
  i_ins = ArmInsNewForBbl(source);
  ArmInsMakePush(i_ins,(1 << ARM_REG_R0) | (1 << ARM_REG_R14), ARM_CONDITION_AL, TRUE);
  InsInsertBefore(T_INS(i_ins),BBL_INS_LAST(source));

  /* bl .L1 ... .L2: */
  ArmInsMakeCondBranchAndLink(T_ARM_INS(BBL_INS_LAST(source)),ARM_CONDITION_AL);
  target = CFG_EDGE_TAIL(edge);
  ArmChainCreateHoleBefore(target);
  new_target = BblNew(cfg);
  BblInsertInChainAfter(new_target,target);

  /* ldr r0, [sp,#4] */
  i_ins = ArmInsNewForBbl(new_target);
  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_THUMB);
  ArmInsMakeLdr(i_ins,ARM_REG_R0,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,TRUE,TRUE,FALSE);
  ArmInsAppendToBbl(i_ins,new_target);
  /* mov r14, r0 */
  i_ins = ArmInsNewForBbl(new_target);
  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_THUMB);
  ArmInsMakeMov(i_ins,ARM_REG_R14,ARM_REG_R0,0,ARM_CONDITION_AL);
  ArmInsAppendToBbl(i_ins,new_target);
  /* ldr r0, [sp] */
  i_ins = ArmInsNewForBbl(new_target);
  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_THUMB);
  ArmInsMakeLdr(i_ins,ARM_REG_R0,ARM_REG_R13,ARM_REG_NONE,0,ARM_CONDITION_AL,TRUE,TRUE,FALSE);
  ArmInsAppendToBbl(i_ins,new_target);
  /* add sp, #8 */
  i_ins = ArmInsNewForBbl(new_target);
  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_THUMB);
  ArmInsMakeAdd(i_ins,ARM_REG_R13,ARM_REG_R13,ARM_REG_NONE,8,ARM_CONDITION_AL);
  ArmInsAppendToBbl(i_ins,new_target);

  /* change branch opcode into BL */
  branch = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  ARM_INS_SET_OPCODE(branch,ARM_BL);

  /* fallthrough path from bbl with pop to next bbl
   * (original branch destination)
   */
  CfgEdgeCreate(cfg,new_target,target,ET_FALLTHROUGH);
  /* and let bl point to pop-bbl */
  CfgEdgeChangeTail(edge,new_target);
}
#endif
 
static t_bool InvertThumb16Branch(t_cfg * cfg, t_arm_ins * branch_ins)
{
        t_bbl * new_bbl = NULL;
        t_arm_ins * new_ins = NULL;
        t_cfg_edge * ft_edge = NULL;
        t_cfg_edge * jmp_edge = NULL;
        t_cfg_edge * i_edge = NULL;

        ASSERT(ARM_INS_OPCODE(branch_ins)==ARM_T2CBZ || ARM_INS_OPCODE(branch_ins)==ARM_T2CBNZ, ("expected CB(N)Z instruction, got @I", branch_ins));

        /* find the necessary edges */
        BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(branch_ins), i_edge)
        {
                if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH ||CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU)
                        ft_edge = i_edge;
                else if (CFG_EDGE_CAT(i_edge) == ET_JUMP || CFG_EDGE_CAT(i_edge) == ET_IPJUMP)
                        jmp_edge = i_edge;
                else
                        FATAL(("Unexpected outgoing edge in @eiB: @E", ARM_INS_BBL(branch_ins), i_edge));
        }
        ASSERT(jmp_edge, ("no outgoing JUMP edge found for @eiB", ARM_INS_BBL(branch_ins)));
        ASSERT(ft_edge, ("no outgoing FALLTHROUGH edge found for @eiB", ARM_INS_BBL(branch_ins)));

        /*if (BBL_INS_FIRST(CFG_EDGE_TAIL(ft_edge)) && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(ft_edge)))) == ARM_B))
                return FALSE;*/

        /* invert the opcode */
        VERBOSE(3, ("looking to invert @I", branch_ins));
        switch (ARM_INS_OPCODE(branch_ins))
        {
        case ARM_T2CBZ:
                ARM_INS_SET_OPCODE(branch_ins, ARM_T2CBNZ);
                break;

        case ARM_T2CBNZ:
                ARM_INS_SET_OPCODE(branch_ins, ARM_T2CBZ);
                break;

        default:
                return FALSE;
        }
        VERBOSE(3, ("     to @I", branch_ins));
        VERBOSE(2, (" switched condition of @I", branch_ins));

        /* create the BBL for the new branch instruction */
        new_bbl = BblNew(cfg);
        BblInsertInChainAfter(new_bbl, ARM_INS_BBL(branch_ins));
        BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(jmp_edge)));

        /* create the new branch instruction */
        new_ins = ArmInsNewForBbl(new_bbl);
        ARM_INS_SET_FLAGS(new_ins, ARM_INS_FLAGS(new_ins) | FL_THUMB);
        ArmInsMakeUncondBranch(new_ins);
        ArmInsAppendToBbl(new_ins, new_bbl);
        CfgEdgeCreate(cfg, new_bbl, CFG_EDGE_TAIL(jmp_edge), (CFG_EDGE_CAT(jmp_edge) == ET_IPJUMP) ? ET_IPJUMP : ET_JUMP);

        /* finally we make sure the edges point to the right locations */
        CfgEdgeChangeTail(jmp_edge, CFG_EDGE_TAIL(ft_edge));
        CfgEdgeChangeTail(ft_edge, new_bbl);

        /* make sure the newly created jump has an approximately right address */
        BblSetAddressSuper(new_bbl, AddressAdd(BBL_CADDRESS(ARM_INS_BBL(branch_ins)), BBL_CSIZE(ARM_INS_BBL(branch_ins))));

        VERBOSE(2, (" inserted trampoline @ieB", new_bbl));
        

        return TRUE;
}

typedef enum {
  INSERTTRAMPOLINE_DIRECTION_NEUTRAL = 0,
  INSERTTRAMPOLINE_DIRECTION_FORCE_FORE = 1,
  INSERTTRAMPOLINE_DIRECTION_FORCE_BACK = 2
} t_insertTrampolineDirection;

/*
 */
/* InsertTrampolineForJmpEdge {{{*/
/* Inserts branch chains for Thumb1 so that jumps can reach their destination
 * in case it's farther away than the maximum displacement. This is done after
 * data/constant pools have been inserted, so these extra branches must not
 * increase the distance between loads and their pools.
 *
 * \param cfg The control flow graph
 * \param ins The original branch instruction for which the branch displacement is too large
 * \param edge The jump edge from the original branch instruction to the branch destination
 * \param disp The original branch displacement
 * \param max_disp The maximal branch displacement for a single branch instruction
 * \param data_pools_generated TODO
 * \param forward_only The branch instruction can only jump forward
 */
static t_bool InsertTrampolineForJmpEdge(t_cfg *cfg, t_arm_ins *ins, t_cfg_edge *edge,t_int32 disp,t_uint32 max_disp,t_bool data_pools_generated, t_bool forward_only, t_bool as_nearby_as_possible, t_bool allow_not_found, t_insertTrampolineDirection direction)
{
  t_arm_ins *i_ins = NULL, *split_at = NULL;
  t_bbl *insert_bbl, *curr_bbl = NULL, *new_bbl = NULL;
  t_address curr_disp = AddressNew32(0), insert_disp = AddressNew32(0);
  t_arm_ins *trampoline = NULL;
  t_bool skip_succ_edge_overhead = FALSE, split_block = FALSE;
  t_address max_disp_address = AddressNew32(0);

  insert_bbl = NULL;
  /* ignore successor edges the first time we look for extra branches
   * needing branch islangs that may influence the distance to our
   * target, because that first time the only such successor edge
   * will be the branch we are currently handling */
  skip_succ_edge_overhead = TRUE;

  i_ins = ins;
  curr_bbl = ARM_INS_BBL(i_ins);

  if ((disp<0 && !forward_only) || direction==INSERTTRAMPOLINE_DIRECTION_FORCE_BACK)
  {
    /* +2 because jump is based on current instruction + 4 relative
     * to the start of the jump instruction (which itself takes up
     * 2 bytes)
     */
    curr_disp = BBL_CSIZE(curr_bbl);

    if (ARM_INS_CSIZE(ins) == 2)
      curr_disp = AddressAddUint32(curr_disp, 2);
  }
  else
  {
    if (forward_only && disp<0)
      VERBOSE(2,("inserting trampoline for forward-only branch currently jumping backward @I", ins));
    if (direction==INSERTTRAMPOLINE_DIRECTION_FORCE_FORE && disp<0)
      VERBOSE(2,("inserting trampoline for forced forward branch currently jumping backward @I", ins));

    /* the branch is already at the end of the current bbl */
    curr_disp = AddressNullForCfg(cfg);

    /* Walk over every EMPTY BBL following the one from which we branch.
     * These BBLs can possibly be generated by the hardening-middle-end. */
    do
    {
      AddPotentialBranchIslandOverhead(curr_bbl,&curr_disp,max_disp,FALSE,skip_succ_edge_overhead);
      skip_succ_edge_overhead = FALSE;
      curr_bbl = BBL_NEXT_IN_CHAIN(curr_bbl);
    } while (!BBL_INS_LAST(curr_bbl));
    /* see disp<0 case, but since we jump ahead it's sub */
    curr_disp = AddressAdd(curr_disp,BBL_CSIZE(curr_bbl));
    if (ARM_INS_CSIZE(ins) == 2)
      curr_disp = AddressSubUint32(curr_disp, 2);

    /* jumps ahead can only go up to 1 less (2-complement) */
    max_disp-=2;
  }
  VERBOSE(2,("Looking for more nearby destination for @I currently going to @B at calculated distance %d",ins,CFG_EDGE_TAIL(edge),disp));
  insert_bbl = NULL;
  max_disp_address = AddressNewForCfg(cfg,max_disp);
  while (AddressIsLt(curr_disp,max_disp_address))
  {
    /* in case of a backward branch, the destination is after the current bbl
     * -> go to the previous bbl but do not yet update curr_disp since the
     * size of the bbl doesn't matter for the branch distance. Do take into
     * account potential increases due to other branches that may insert
     * branch islands already.
     */
    if ((disp<0 && !forward_only && direction==INSERTTRAMPOLINE_DIRECTION_NEUTRAL) || direction==INSERTTRAMPOLINE_DIRECTION_FORCE_BACK)
    {
      t_address prev_disp;
      t_bbl *prev_bbl;

      /* in case we overrun the limit here, restore the previous bbl because
       * the code after the loop expects that it can subtract the size of
       * curr_bbl from curr_disp in order to get the displacement except for
       * the part caused by curr_bbl
       */
      prev_disp = curr_disp;
      prev_bbl = curr_bbl;
      do
      {
        AddPotentialBranchIslandOverhead(curr_bbl,&curr_disp,max_disp,disp<0,skip_succ_edge_overhead);
        skip_succ_edge_overhead = FALSE;
        curr_bbl = BBL_PREV_IN_CHAIN(curr_bbl);
      } while (!BBL_INS_LAST(curr_bbl));
      if (AddressIsGe(curr_disp,max_disp_address))
      {
        curr_disp = prev_disp;
        curr_bbl = prev_bbl;
        break;
      }
      VERBOSE(2,("  Looping, curr_disp now @G after ins @I",curr_disp,BBL_INS_LAST(curr_bbl)));
    }

    /* guaranteed safe case: right after a FORWARD-only data pool (Thumb literal loads
     * can't have negative offsets -> adding a branch here will never
     * increase the distance between a load and its pool)
     */
    if ((BBL_ATTRIB(curr_bbl) & BBL_DATA_POOL) && (BBL_ATTRIB(curr_bbl) & BBL_FORWARD_DATAPOOL))
    {
      insert_bbl = curr_bbl;
      insert_disp = curr_disp;
      VERBOSE(2,("  Picked data pool at distance @G (at address @G)",curr_disp,AddressAdd(BBL_CADDRESS(curr_bbl),BBL_CSIZE(curr_bbl))));
    }
    /* if we haven't encountered a data pool yet, or if the distance between
     * the data pool and the current position is larger than the maximum
     * possible Thumb literal offset (and hence no loads can appear anymore
     * here), instead look for regular basic blocks after which we can insert
     * a branch (there may not be any pool between the branch and its target)
     */
    else if ((!data_pools_generated ||
              !insert_bbl ||
              !((BBL_ATTRIB(insert_bbl) & BBL_DATA_POOL) && (BBL_ATTRIB(curr_bbl) & BBL_FORWARD_DATAPOOL)) ||
              ((disp < 0) && ((G_T_UINT32(curr_disp) - insert_disp - G_T_UINT32(BBL_CSIZE(insert_bbl))) > 256*4))) &&
              ArmBblCanInsertAfter(curr_bbl))
    {
      insert_bbl = curr_bbl;
      insert_disp = curr_disp;
      VERBOSE(2,("  Picked data pool at distance @G (at address @G)",curr_disp,AddressAdd(BBL_CADDRESS(curr_bbl),BBL_CSIZE(curr_bbl))));

      if (as_nearby_as_possible)
        break;
    }

    if (disp>0 || forward_only || direction==INSERTTRAMPOLINE_DIRECTION_FORCE_FORE)
    {
      do
      {
        AddPotentialBranchIslandOverhead(curr_bbl,&curr_disp,max_disp,FALSE,skip_succ_edge_overhead);
        skip_succ_edge_overhead = FALSE;
        curr_bbl = BBL_NEXT_IN_CHAIN(curr_bbl);
      } while (!BBL_INS_LAST(curr_bbl));
    }
    curr_disp = AddressAdd(curr_disp,BBL_CSIZE(curr_bbl));
    if (disp>0)
      VERBOSE(2,("  Looping, curr_disp now @G after ins @I",curr_disp,BBL_INS_LAST(curr_bbl)));
  }
  /* if we found an insertion location right after a data bbl, we're ok
   * (thumb literal loads always have positive offsets -> can be in
   *  between a load and its pool)
   * if we found another insertion location that is more than 255
   * bytes away, we're also ok because
   *  a) if there was a pool in between, we would have selected that
   *     one as insertion location
   *  b) any literal load has to be within 255 bytes of its pool, so
   *     this insertion location is definitely not between a load and
   *     its pool
   *
   *  In other cases, go back as far as possible (which in the worst case
   *  is 256 bytes), split the bbl at that location and insert a jump
   *  island there
   */
  split_block =
    !insert_bbl ||
    (data_pools_generated &&
     !((BBL_ATTRIB(insert_bbl) & BBL_DATA_POOL) && (BBL_ATTRIB(curr_bbl) & BBL_FORWARD_DATAPOOL)) &&
     (AddressIsGt(insert_disp,max_disp_address)));
  /* can't split certain parts of switch statements -> find other place */
  if (split_block)
  {
    while (ArmBblInSwitchStatement(curr_bbl,FALSE))
    {
      curr_disp = AddressSub(curr_disp,BBL_CSIZE(curr_bbl));
      if (disp>0 || forward_only)
        curr_bbl = BBL_PREV_IN_CHAIN(curr_bbl);
      else
        curr_bbl = BBL_NEXT_IN_CHAIN(curr_bbl);
    }
    ASSERT(curr_disp>0,("Went back past begin looking for non-switch bbl for @I",ins));
  }

  /* did we stop at a data block we can never jump over? */
  // if (BBL_INS_LAST(curr_bbl) &&
  //     (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(curr_bbl))) == ARM_DATA) &&
  //     (ARM_INS_CONDITION(ins) == ARM_CONDITION_AL) &&
  //     AddressIsGe(BBL_CSIZE(curr_bbl),AddressNewForCfg(cfg,max_disp)))
  //   {
  //     /* problem, we can never jump over this with a regular
  //      * branch -> use a branch & link instead, but then we
  //      * have to push/pop LR
  //      */
  //     CreateBranchLinkTrampoline(cfg,edge);
  //     return;
  //   }
  if (split_block)
  {
    /* find an insertion place in the last bbl that we processed */
    ASSERT(!ArmBblInSwitchStatement(curr_bbl,FALSE),("have to split switch bbl @eiB",curr_bbl));
    ASSERT(BBL_INS_LAST(curr_bbl),("empty bbl pushed us over size limit?"));
    VERBOSE(2,("  * insert_disp = @G, max_disp = %x",insert_disp,max_disp-2));
    curr_disp = AddressSub(curr_disp,BBL_CSIZE(curr_bbl));
    if (disp>0 || forward_only || direction==INSERTTRAMPOLINE_DIRECTION_FORCE_FORE)
    {
      BBL_FOREACH_ARM_INS(curr_bbl,i_ins)
      {
        t_uint32 compensating_size = (ARM_INS_FLAGS(i_ins) & FL_THUMB) ? 2 : 4;

        curr_disp = AddressAdd(curr_disp, ARM_INS_CSIZE(i_ins));
        /* we will insert two branches: one on the way to the final
         * destination, and one to jump over this inserted branch.
         * The one that jumps over the inserted branch will take up
         * 2 bytes and we have to jump over it -> only look up to
         * max_disp-2
         */
        if (AddressIsEq(curr_disp,AddressNewForCfg(cfg,max_disp-compensating_size)) && !ArmInsIsInITBlock(i_ins))
          break;
        /* can happen in case of 4-byte instruction such as call */
        if (AddressIsGt(curr_disp,AddressNewForCfg(cfg,max_disp-compensating_size)))
        {
          i_ins = ARM_INS_IPREV(i_ins);
          while (!i_ins)
          {
            curr_bbl = BBL_PREV_IN_CHAIN(curr_bbl);
            i_ins = (t_arm_ins*)BBL_INS_LAST(curr_bbl);
          }
          if (!ArmInsIsInITBlock(i_ins)) break;
        }
      }
    }
    else
    {
      t_address max_disp_address_minus_4 = AddressNewForCfg(cfg,max_disp-4);
      BBL_FOREACH_ARM_INS_R(curr_bbl,i_ins)
      {
        curr_disp = AddressAdd(curr_disp, ARM_INS_CSIZE(i_ins));
        /* like above, we will insert two branches. In case of a
         * backward branch, we have to jump over the one on the
         * way to the final destination though, and that one will
         * initially be set to 4 bytes because we don't know yet
         * whether it can reach the end -> max_disp-4
         */
        if (AddressIsEq(curr_disp,max_disp_address_minus_4) && !ArmInsIsInITBlock(i_ins))
          break;
        if (AddressIsGt(curr_disp,max_disp_address_minus_4))
        {
          i_ins = ARM_INS_INEXT(i_ins);
          while (!i_ins)
          {
            curr_bbl = BBL_NEXT_IN_CHAIN(curr_bbl);
            i_ins = (t_arm_ins*)BBL_INS_FIRST(curr_bbl);
          }

          if (!ArmInsIsInITBlock(i_ins)) break;
        }
      }
    }

    if (allow_not_found && !i_ins)
    {
      VERBOSE(2,("Unable to find instruction at limit, but not finding a location is allowed"));
      return FALSE;
    }
    else
    {
      ASSERT(i_ins,("Unable to find instruction at limit"));
    }

    VERBOSE(2,("  Splitting before @I at distance @G in @eiB",i_ins,curr_disp,curr_bbl));

    split_at = i_ins;
    if (disp>0 || forward_only || direction==INSERTTRAMPOLINE_DIRECTION_FORCE_FORE)
    {
      /* Reserve space for the trampoline. This can happen in 2 cases:
       *  - either we did not find an instruction to split a BBL at;
       *  - or the BBL has to be split, which means inserting a new (2-byte)
       *    branch instruction at the end of the first part. This branch
       *    then jumps to the second part of the split BBL. */
      if (!ARM_INS_INEXT(split_at))
      {
        /* find start of next bbl */
        do
        {
          curr_bbl = BBL_NEXT_IN_CHAIN(curr_bbl);
        } while (!BBL_INS_FIRST(curr_bbl));
        ArmChainCreateHoleBefore(curr_bbl);
      }
      else
      {
        /* Split the BBL "curr_bbl" after instruction "split_at" */
        BblSplitBlock(curr_bbl,T_INS(split_at),FALSE);

        /* Create a new instruction for curr_bbl, being the
         * "branch-over-trampoline" instruction. */
        i_ins = ArmInsNewForBbl(curr_bbl);
        ArmInsMakeUncondBranch(i_ins);

        /* check if we are splitting a Thumb or ARM BBL */
        if (ARM_INS_FLAGS(split_at) & FL_THUMB)
        {
          ARM_INS_SET_FLAGS(i_ins,ARM_INS_FLAGS(i_ins) | FL_THUMB);
          ARM_INS_SET_CSIZE(i_ins, AddressNewForCfg(cfg,2));
        }

        ArmInsInsertAfter(i_ins,split_at);
        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(curr_bbl), ET_JUMP);
      }
      insert_disp = AddressSub(AddressAdd(ARM_INS_CADDRESS(split_at),ARM_INS_CSIZE(split_at)),ARM_INS_CADDRESS(ins));

      /* forward jump at ins and offset is from INS_CSIZE(ins) (== 2) + 2 */
      /* compensate the PC reading as address+4 */
      insert_disp = AddressSubUint32(insert_disp,4);
    }
    else
    {
      if (!ARM_INS_IPREV(split_at))
      {
        ArmChainCreateHoleBefore(curr_bbl);
      }
      else
      {
        /* Split the BBL "curr_bbl" before instruction "split_at" */
        BblSplitBlock(curr_bbl,T_INS(split_at),TRUE);

        i_ins = ArmInsNewForBbl(curr_bbl);
        ArmInsMakeUncondBranch(i_ins);

        /* check if we are splitting a Thumb or ARM BBL */
        if (ARM_INS_FLAGS(split_at) & FL_THUMB)
        {
          ARM_INS_SET_FLAGS(i_ins,ARM_INS_FLAGS(i_ins) | FL_THUMB);
          ARM_INS_SET_CSIZE(i_ins, AddressNewForCfg(cfg,2));
        }

        ArmInsInsertBefore(i_ins,split_at);
        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(curr_bbl), ET_JUMP);
      }
      insert_disp = AddressSub(ARM_INS_CADDRESS(ins),ARM_INS_CADDRESS(split_at));

      /* backwards jump at ins and offset is from INS_CSIZE(ins) (== 2) + 2 */
      /* compensate the PC reading as address+4 */
      insert_disp = AddressAddUint32(insert_disp,4);
    }
    insert_bbl = curr_bbl;
  }
  ASSERT(insert_bbl,("Can't find place to insert branch trampoline for @I with offset %d",ins,disp));
  ASSERT(AddressIsLe(insert_disp,AddressNewForCfg(cfg,max_disp)),("insert disp @G > max disp %u",insert_disp,max_disp));

  VERBOSE(2,("------------------------------------------------------------------------------------------"));
  VERBOSE(2,("Changing branch @I", ins));
  VERBOSE(2,("old target: @ieB",CFG_EDGE_TAIL(edge)));
  VERBOSE(2,("trampoline will be added after @ieB",insert_bbl));
  VERBOSE(2,("new displacement becomes @G",insert_disp));

  new_bbl = BblNew(cfg);
  BblInsertInChainAfter(new_bbl,insert_bbl);
  BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
  /* create the trampoline */
  trampoline = ArmInsNewForBbl(new_bbl);
  ARM_INS_SET_FLAGS(trampoline,ARM_INS_FLAGS(trampoline) | FL_THUMB);
  ArmInsMakeUncondBranch(trampoline);
  ArmInsAppendToBbl(trampoline,new_bbl);
  CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
  CfgEdgeChangeTail(edge,new_bbl);


  /* Trampoline insertion is done on a per-chain basis:
   *  - Assign addresses in chain
   *  - Iterate over every BBL, looking for branch instructions that can't reach their destination
   * The "assign addresses in chain"-function is called once because this speeds up the algorithm significantly.
   * This implies, however, that it can happen that one trampoline is inserted in a chain, of which the address
   * is used for trampoline insertion later on in the same chain (thus before a new "assign addresses in chain"-
   * call). This occurs especially in the AddPotentialBranchIslandOverhead-function.
   * In order for the algorithm to work correctly, the newly created trampoline should have an approximate address.
   *
   * As the algorithm is simply a heuristic, and does some approximate estimates, the address of this newly
   * inserted trampoline can also be approximated. */
  BblSetAddressSuper(new_bbl, AddressAdd(BBL_CADDRESS(insert_bbl), BBL_CSIZE(insert_bbl)));
  //AssignAddressesInChain (insert_bbl, BBL_CADDRESS(insert_bbl));

  VERBOSE(2,("added trampoline: @I",trampoline));

  return TRUE;
}
/*}}}*/


static void
ArmConvertBranchesTo32BitIns(t_cfg * cfg)
 {
        t_arm_ins * i_ins;
  t_bbl *bbl;

    CFG_FOREACH_BBL(cfg,bbl)
    {
      i_ins = T_ARM_INS(BBL_INS_LAST(bbl));
      if (!i_ins) continue;

      /* don't look at branch-and-link instructions here */
      if ((ARM_INS_FLAGS(i_ins) & FL_THUMB) && (ARM_INS_OPCODE(i_ins) == ARM_B))
      {
          if (!AddressIsNull(AddressSubUint32(ARM_INS_CSIZE(i_ins), 4)))
          {
            /* simply change the size of thumb branches to 32 bit */
            ARM_INS_SET_CSIZE(i_ins, AddressNew32(4));
            BBL_SET_CSIZE(bbl, AddressAddUint32(BBL_CSIZE(bbl), 2));
          }
      }
    }
}

static void
ArmConvertBranchesToThumb1(t_cfg * cfg)
{
        t_bbl * i_bbl;
        t_arm_ins * i_ins;

        CFG_FOREACH_BBL(cfg, i_bbl)
        {
                i_ins = T_ARM_INS(BBL_INS_LAST(i_bbl));

                /* skip empty BBL's */
                if (!i_ins) continue;

                if (ARM_INS_FLAGS(i_ins) & FL_THUMB
                        && ARM_INS_OPCODE(i_ins) == ARM_B
                        && AddressIsNull(AddressSubUint32(ARM_INS_CSIZE(i_ins), 4)))
                {
                        ARM_INS_SET_CSIZE(i_ins, AddressNew32(2));
                        BBL_SET_CSIZE(i_bbl, AddressSubUint32(BBL_CSIZE(i_bbl), 2));
                }
        }
}

static void
ArmConvertThumbBranchesTo16BitIns(t_cfg * cfg)
 {
        t_arm_ins * i_ins;
  t_bbl *bbl;

    CFG_FOREACH_BBL(cfg,bbl)
    {
      i_ins = T_ARM_INS(BBL_INS_LAST(bbl));
      if (!i_ins) continue;

      if (ArmBblInSwitchStatement(bbl,FALSE))
        continue;

      /* if this instruction may be relocated, the relocation will assume a
       *  16 or 32 bit encoding already
       *  -> don't touch
       */
      if (ARM_INS_REFERS_TO(i_ins))
        continue;

      /* don't look at branch-and-link instructions here */
      if ((ARM_INS_FLAGS(i_ins) & FL_THUMB) && (ARM_INS_OPCODE(i_ins) == ARM_B) && AddressIsNull(AddressSubUint32(ARM_INS_CSIZE(i_ins), 4)))
      {
          t_cfg_edge *edge;
          t_int32 disp;

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
          {
            if (CFG_EDGE_CAT(edge) & (ET_JUMP|ET_IPJUMP))
              break;
          }
          ASSERT(edge,("Bbl ending in branch without ET_JUMP/ET_IPJUMP edge: @B",bbl));
          disp = ArmCalculateBranchDisplacement(edge);

          if (ARM_INS_CONDITION(i_ins) == ARM_CONDITION_AL)
          {
            /* unconditional branch: 16 bit instruction supports signed
             * 12 bit displacement
             */

            if (disp >= (-(1<<11)+16) && (disp < (1<<11)-16))
            {
              if (!AddressIsEq(ARM_INS_CSIZE(i_ins),AddressNew32(2)))
              {
                BBL_SET_CSIZE(bbl, AddressSubUint32(BBL_CSIZE(bbl), 2));
                ARM_INS_SET_CSIZE(i_ins, AddressNew32(2));
              }
            }
          }
          else
          {
            if (disp >= (-(1<<8))+16 && (disp < (1<<8)-16))
            {
              if (!AddressIsEq(ARM_INS_CSIZE(i_ins),AddressNew32(2)))
              {
                BBL_SET_CSIZE(bbl, AddressSubUint32(BBL_CSIZE(bbl), 2));
                ARM_INS_SET_CSIZE(i_ins, AddressNew32(2));
              }
            }
          }
      }
    }
}
/*!
 * Process all ARM Thumb branches and either change them al into
 * 32 bit instructions, or change the ones with near targets back
 * into 16 bit versions
 *
 * \param cfg
 * \param chain_head first chain of instructions
 * \param force32bit if TRUE, change all branches into 32 bit instructions,
 *          if FALSE, change them into 16 bit where possible
 *
 * \return void
 */
 /* ArmProcessThumb2BranchesInChains {{{*/
 static t_bool
 ArmProcessThumb2BranchesInChains(t_cfg *cfg, t_bbl *chain_head, t_bool force32bit, t_bool data_pools_generated, t_uint32 * nr_tramp)
 {
  t_bbl *bbl;
  t_bool iterate, changed_anything;

  changed_anything = FALSE;
  do
  {
    t_arm_ins *i_ins;

    iterate = FALSE;
    AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));

    CHAIN_FOREACH_BBL(chain_head,bbl)
    {
      i_ins = T_ARM_INS(BBL_INS_LAST(bbl));
      if (!i_ins) continue;

      /* skip trampolines inserted during this iteration */
      if (AddressIsNull(ARM_INS_CADDRESS(i_ins))) continue;

      /* don't look at branch-and-link instructions here */
      if ((ARM_INS_FLAGS(i_ins) & FL_THUMB) &&
	         ((ARM_INS_OPCODE(i_ins) == ARM_B) || (ARM_INS_OPCODE(i_ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(i_ins) == ARM_T2CBNZ)))
      {
     	  t_cfg_edge *edge;
     	  t_int32 disp;
        BBL_FOREACH_SUCC_EDGE(bbl,edge)
        {
          if (CFG_EDGE_CAT(edge) & (ET_JUMP|ET_IPJUMP))
            break;
        }
        ASSERT(edge,("Bbl ending in branch without ET_JUMP/ET_IPJUMP edge: @B",bbl));

        disp = ArmCalculateBranchDisplacement(edge);

     	  /* check whether the branch offset fits in a 16 bit instruction */
     	  if (ARM_INS_CONDITION(i_ins) == ARM_CONDITION_AL)
     	  {
          /* unconditional branch */
          if ((ARM_INS_OPCODE(i_ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(i_ins) == ARM_T2CBNZ))
          {
            ASSERT(ARM_INS_CSIZE(i_ins) == 2, ("instruction size should be 2, but is %d (@I)", ARM_INS_CSIZE(i_ins), i_ins));

            /* HACK: apparently sometimes a CB(N)Z branch gets an offset of 126 which eventually results in an immediate
             * bigger than 126 (e.g. the maximum distance a CB(N)Z-instruction can jump) for an unknown reason...
             * The quick and dirty solution is to exclude 126 from the range of allowed offsets. */
            if (disp > 126-(worst_case_alignment - 2) || disp < 0)
            {
              VERBOSE(2,("0 insert trampoline for @I", i_ins));
              InvertThumb16Branch(cfg, i_ins);
              //              AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
              iterate = TRUE;
              changed_anything = TRUE;
            }
          }
          else
          {
            if (diabloarm_options.fullthumb2 && AddressIsEq(ARM_INS_CSIZE(i_ins),AddressNew32(4)))
            {
              if (!Uint32CheckSignExtend(disp, 24))
              {
                VERBOSE(2,("1 insert trampoline for @I",i_ins));
                InsertTrampolineForJmpEdge(cfg,i_ins,edge,disp,(1<<24)-100000,data_pools_generated, FALSE, data_pools_generated, FALSE, INSERTTRAMPOLINE_DIRECTION_NEUTRAL);
                //                AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
                (*nr_tramp)++;
                iterate = TRUE;
                changed_anything = TRUE;
              }
              else
              {
                ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | (FL_THUMB | FL_PREFER32));
              }
            }
            else if (!Uint32CheckSignExtend(disp, 11))
            {
              VERBOSE(2,("2 insert trampoline for @I",i_ins));

              /* When using the Thumb-2 instruction set, we assume that almost every location
               * in the program can be reached by using an unconditional branch instruction
               * (as these instructions have a maximum reach of 24-bit (= 1 MB)).
               * Thus, by inserting one trampoline in this case, chances are really big that
               * the branch can reach its destination.
               * N.B.: For this reason, trampolines are not inserted for ARM-instructions,
               *       because in ARM-mode, we assume the ranges are large enough by default.
               *
               * However, in Thumb-1, the largest distance a branch instruction can jump is
               * 11-bit (2048 B). Hence, by placing the trampoline as close to the original
               * branch as possible just isn't feasible. */
              InsertTrampolineForJmpEdge(cfg,i_ins,edge,disp,(1<<11)-50,data_pools_generated, FALSE,  FALSE/* as_nearby_as_possible */, FALSE, INSERTTRAMPOLINE_DIRECTION_NEUTRAL);
                (*nr_tramp)++;
              //              AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
              iterate = TRUE;
              changed_anything = TRUE;
            }
          }
        }
        else
        {
          /* conditional branch */
          /* +16 and -16 were added by BJORN: because of alignment requirements, it can happen
             that some instructions end up at smaller addresses while converting 32-bit instructions
             to 16-bit instructions, while other remain in place (i.e., do not move along) */
          /* TODO: +16 and -16 are a hack really: in fact we should check how many aligned blocks
             there are between the head and tail of an edge */

          if (diabloarm_options.fullthumb2 && AddressIsEq(ARM_INS_CSIZE(i_ins),AddressNew32(4)))
          {
            if (!Uint32CheckSignExtend(disp, 20))
            {
              VERBOSE(2,("3 insert trampoline for @I",i_ins));
              InsertTrampolineForJmpEdge(cfg,i_ins,edge,disp,(1<<20)-10000,data_pools_generated, FALSE,  data_pools_generated, FALSE, INSERTTRAMPOLINE_DIRECTION_NEUTRAL);
              //              AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
                (*nr_tramp)++;
              iterate = TRUE;
              changed_anything = TRUE;
            }
            else
            {
              ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | (FL_THUMB | FL_PREFER32));
            }
          }
          else if (!Uint32CheckSignExtend(disp, 8))
          {
            VERBOSE(2,("4 insert trampoline for @I",i_ins));

            /* See comments for the unconditional Thumb-1 branches */
            t_bool res = InsertTrampolineForJmpEdge(cfg,i_ins,edge,disp,(1<<8)-16,data_pools_generated, FALSE,  FALSE/* as_nearby_as_possible */, TRUE, INSERTTRAMPOLINE_DIRECTION_NEUTRAL);
            if (!res)
              InsertTrampolineForJmpEdge(cfg,i_ins,edge,disp,(1<<8)-16,data_pools_generated, FALSE,  FALSE/* as_nearby_as_possible */, FALSE, (disp > 0) ? INSERTTRAMPOLINE_DIRECTION_FORCE_BACK : INSERTTRAMPOLINE_DIRECTION_FORCE_FORE);
                (*nr_tramp)++;
            //            AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
            iterate = TRUE;
            changed_anything = TRUE;
          }
        }
     }
     else if (ArmInsIsSwitchedBL(i_ins)
              && ArmInsSwitchedBLTableEntrySize(i_ins) != 4)
     {
        /* iterate over every outgoing SWITCH edge */
        t_cfg_edge * edge = NULL;
        t_uint32 entry_size = ArmInsSwitchedBLTableEntrySize(i_ins);
        t_uint32 max_disp = ((1 << (entry_size * 8)) - 1) << 1;
        t_uint32 sign_bit = (entry_size * 8);
        t_bbl * insert_trampoline_after = BBL_NEXT(ARM_INS_BBL(i_ins));
        t_uint32 n_edges = 0;
        t_cfg_edge ** switch_edges = NULL;
        t_uint32 i = 0;

        /* count the number of switch edges */
        BBL_FOREACH_SUCC_EDGE(bbl,edge)
        {
              if (CFG_EDGE_CAT(edge) & (ET_SWITCH | ET_IPSWITCH))
                      n_edges++;
        }
        ASSERT(n_edges > 0, ("no switch edges found for Thumb-1 switched BL @I", i_ins));

        /* construct a list of the switch edges, in order */
        switch_edges = Calloc(n_edges, sizeof(t_cfg_edge *));
        BBL_FOREACH_SUCC_EDGE(bbl,edge)
          if (CFG_EDGE_CAT(edge) & (ET_SWITCH | ET_IPSWITCH))
            switch_edges[CFG_EDGE_SWITCHVALUE(edge)] = edge;

        for (i=0; i<n_edges; i++)
        {
          edge = switch_edges[i];
          t_uint32 disp = ArmCalculateBranchDisplacement(edge);
          t_bbl * old_target_bbl = CFG_EDGE_TAIL(edge);

          if (disp > max_disp
              || (ARM_INS_FLAGS(i_ins) & FL_SWITCHEDBL_SIGNEXT
                  && !Uint32CheckSignExtend(disp, sign_bit)))
          {
            /* we need to insert a trampoline here, must go forwards */
            if (ARM_INS_FLAGS(i_ins) & FL_SWITCHEDBL_SIGNEXT
                && !Uint32CheckSignExtend(disp, sign_bit))
              VERBOSE(2,("6a insert trampoline for sign-extended @I (%u)", i_ins, i));
            else
              VERBOSE(2,("6b insert trampoline for @I (%u)", i_ins, i));

            t_bbl * new_bbl = BblNew(cfg);
            t_arm_ins * trampoline = NULL;
            BblInsertInChainAfter(new_bbl,insert_trampoline_after);
            BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
            /* create the trampoline */
            trampoline = ArmInsNewForBbl(new_bbl);
            ARM_INS_SET_FLAGS(trampoline,ARM_INS_FLAGS(trampoline) | FL_THUMB);
            ArmInsMakeUncondBranch(trampoline);
            ArmInsAppendToBbl(trampoline,new_bbl);
            CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
            CfgEdgeChangeTail(edge,new_bbl);

            RelocSetToRelocatable(CFG_EDGE_REL(edge), 0, T_RELOCATABLE(CFG_EDGE_TAIL(edge)));

            BblSetAddressSuper(new_bbl, AddressAdd(BBL_CADDRESS(insert_trampoline_after), BBL_CSIZE(insert_trampoline_after)));

            insert_trampoline_after = new_bbl;

            t_uint32 j;
            for (j=i+1; j<n_edges; j++)
            {
              edge = switch_edges[j];
              if (CFG_EDGE_TAIL(switch_edges[j])==old_target_bbl)
              {
                VERBOSE(2,("    also adapting @E",edge));
                CfgEdgeChangeTail(edge,new_bbl);
                RelocSetToRelocatable(CFG_EDGE_REL(edge), 0, T_RELOCATABLE(CFG_EDGE_TAIL(edge)));
              }
            }

            (*nr_tramp)++;
            //            AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
            iterate = TRUE;
            changed_anything = TRUE;
          }
        }

        Free(switch_edges);

        if (iterate)
          break;
     }
        else if ((ARM_INS_OPCODE(i_ins) == ARM_T2TBB) || (ARM_INS_OPCODE(i_ins) == ARM_T2TBH))
        {
          t_cfg_edge *edge = NULL;
          t_int32 disp = 0;
          t_uint32 n_edges = 0;
          t_cfg_edge ** switch_edges = NULL;
          t_bbl * insert_trampoline_after = BBL_NEXT(ARM_INS_BBL(i_ins));
          t_uint32 i = 0;

          t_uint32 max_disp = (ARM_INS_OPCODE(i_ins) == ARM_T2TBB) ? 0xff << 1 : 0xffff << 1;
          max_disp--;

          /* count the number of switch edges */
          BBL_FOREACH_SUCC_EDGE(bbl,edge)
          {
            ASSERT((CFG_EDGE_CAT(edge) & (ET_SWITCH|ET_IPSWITCH)), ("WTF? @E", edge));
            n_edges++;
          }
          ASSERT(n_edges > 0, ("no switch edges found for TBB/TBH @I", i_ins));

          /* construct a list of the switch edges, in order */
          switch_edges = Calloc(n_edges, sizeof(t_cfg_edge *));
          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            switch_edges[CFG_EDGE_SWITCHVALUE(edge)] = edge;

          for (i=0; i<n_edges; i++)
          {
            edge = switch_edges[i];
            disp = ArmCalculateBranchDisplacement(edge);
            t_bbl * old_target_bbl = CFG_EDGE_TAIL(edge);
            /* do we need to add a trampoline for this edge?
             * Remember that TBB/TBH can only jump forward! */
            if (disp > max_disp || disp < 0)
            {
              VERBOSE(2,("5 insert trampoline for @I\n(@E)", i_ins, edge));
              VERBOSE(2,("    old target: @ieB",CFG_EDGE_TAIL(edge)));
              VERBOSE(2,("    trampoline will be added after @ieB",insert_trampoline_after));

              t_bbl * new_bbl = BblNew(cfg);
              t_arm_ins * trampoline = NULL;
              BblInsertInChainAfter(new_bbl,insert_trampoline_after);
              BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
              /* create the trampoline */
              trampoline = ArmInsNewForBbl(new_bbl);
              ARM_INS_SET_FLAGS(trampoline,ARM_INS_FLAGS(trampoline) | FL_THUMB);
              ArmInsMakeUncondBranch(trampoline);
              ArmInsAppendToBbl(trampoline,new_bbl);
              CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
              CfgEdgeChangeTail(edge,new_bbl);

              RelocSetToRelocatable(CFG_EDGE_REL(edge), 0, T_RELOCATABLE(CFG_EDGE_TAIL(edge)));

              BblSetAddressSuper(new_bbl, AddressAdd(BBL_CADDRESS(insert_trampoline_after), BBL_CSIZE(insert_trampoline_after)));

              insert_trampoline_after = new_bbl;

              iterate = TRUE;
              changed_anything = TRUE;
              
              t_uint32 j;
              for (j=i+1; j<n_edges; j++)
                {
                  edge = switch_edges[j];
                  if (CFG_EDGE_TAIL(switch_edges[j])==old_target_bbl)
                    {
                      VERBOSE(2,("    also adapting @E",edge));
                      CfgEdgeChangeTail(edge,new_bbl);
                      RelocSetToRelocatable(CFG_EDGE_REL(edge), 0, T_RELOCATABLE(CFG_EDGE_TAIL(edge)));
                    }
                }

              (*nr_tramp)++;
            }
          }

          Free(switch_edges);

          /* only iterate over every BBL again if we actually changed anything for this TBB/TBH */
          if (iterate)
            break;
        }
    }
  } while (iterate);
  return changed_anything;
 }
 /*}}}*/

/*!
 * Align all address/constant/float/.. pools to a multiple of 4
 * bytes
 *
 * \para cfg
 * \param chain_head
 *
 * \return void
 */
/*ArmAlignPools{{{*/
static void ArmAlignPools(t_cfg *cfg, t_bbl** chain_head)
{
  t_bbl *bbl, *prev_bbl;
  t_address offset = AddressNullForCfg(cfg);
  t_bbl *new_chain_head = *chain_head;

return;
  prev_bbl = NULL;
  CHAIN_FOREACH_BBL(*chain_head,bbl)
  {
    if (!AddressIsNull(BBL_CSIZE(bbl)) &&
	((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) == ARM_DATA) ||
	 !(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(bbl))) & FL_THUMB)))
    {
      VERBOSE(2,("Checking alignment of data bbl @iB (@G-@G)",bbl,AddressAdd(BBL_CADDRESS(bbl),offset),AddressAdd(AddressAdd(BBL_CADDRESS(bbl),offset),BBL_CSIZE(bbl))));
      /* if the data bbl starts with a 2 byte data element, align to a multiple of mod 2
       * so that the next (4 byte) element is 4 byte aligned
       */
      if (!AddressIsNull(AddressAnd(AddressAdd(AddressAdd(BBL_CADDRESS(bbl),INS_CSIZE(BBL_INS_FIRST(bbl))),offset),AddressNew32(3))))
      {
        if (!prev_bbl)
	{
	  prev_bbl = BblNew(cfg);
	  if (BBL_FUNCTION(bbl))
	    BblInsertInFunction(prev_bbl,BBL_FUNCTION(bbl));
	  BblInsertInChainBefore(prev_bbl,bbl);
	  new_chain_head=prev_bbl;
	}
	ASSERT(prev_bbl,("First bbl an unaligned data bbl? @B",bbl));

        if ((BBL_INS_LAST(prev_bbl)) &&
	    ArmInsIsNOOP(T_ARM_INS(BBL_INS_LAST(prev_bbl))) &&
	    AddressIsEq(INS_CSIZE(BBL_INS_LAST(prev_bbl)),AddressNew32(2)))
	{
	  BBL_SET_CSIZE(prev_bbl,AddressSubUint32(BBL_CSIZE(prev_bbl),2));
	  InsKill(BBL_INS_LAST(prev_bbl));
	  BBL_SET_NINS(prev_bbl,BBL_NINS(prev_bbl)-1);
	  offset = AddressSub(offset, AddressNew32(2));
	}
	else
	{
	  ArmInsertNoopBblInChain(cfg,prev_bbl,bbl,TRUE,FALSE);
	  offset = AddressAdd(offset, AddressNew32(2));
	}
	VERBOSE(2,("  -- Starts now at @G",AddressAdd(BBL_CADDRESS(bbl),offset)));
      }
    }
    prev_bbl = bbl;
  }
  *chain_head = new_chain_head;
}
/*}}}*/

t_bool ArmValidateBbl(t_bbl * bbl)
{
        t_arm_ins * i_ins = NULL;
        t_bool is_thumb = FALSE;
        t_bool is_arm = FALSE;

        /* only check non-empty/non-data BBL's */
        if (BBL_INS_FIRST(bbl)
            && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl)))!=ARM_DATA)
        {
                BBL_FOREACH_ARM_INS(bbl, i_ins)
                {
                        if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
                                is_thumb = TRUE;
                        else
                                is_arm = TRUE;
                }
        }

        return !(is_thumb && is_arm);
}

/* ArmConvertAddressProducersToPIC{{{*/
/* Get relocation code to calculate a relative address for a particular
 * absolute address. Returns the code in newcode, and the index of the
 * relocation's relocatable to use to make the result relative in
 * relocatableindex.
 *
 * We don't use "P-" because then things will get messed up if instructions
 * get added in between later on.
 */
static void
RelocAbs32GetEquivRel32(t_cfg *cfg, t_reloc *rel, t_string *newcode, int *relocatableindex)
{
  /* R00 = target bbl/section, A01 = offset within bbl/section, A00 = addend of relocation */
  if (strcmp(RELOC_CODE(rel),"R00A01+A00+R00A01+M|\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A01+R01-A00+R00A01+M|\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00A00+\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A00+R01-\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00Z00+\\l*w\\s0000$") == 0)
  {
    *newcode = "R00Z00+R01-\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00A00+R00M|\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A00+R01-R00M|\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00A01+A00+\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A01+A00+R01-\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00A01+Z00+A00+\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A01+Z00+A00+R01-\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"R00A01+Z00+A00+R00A01+Z00+M|\\l*w\\s0000$") == 0)
  {
    *newcode = "R00A01+Z00+R01-A00+R00A01+Z00+M|\\l*w\\s0000$";
    *relocatableindex = 1;
  }
  else if (strcmp(RELOC_CODE(rel),"gA00+\\l*w\\s0000$") == 0)
  {
    *newcode = "gR00-A00+\\l*w\\s0000$";
    *relocatableindex = 0;
  }
  else
  {
    *newcode = "";
    *relocatableindex = -1;
  }
}


static void
ArmConvertAddressProducersToPIC(t_cfg *cfg)
{
  t_bbl *i_bbl, *tmp_bbl;

  /* don't directly iterate over all instructions,
   * because we're adding some instructions (and since
   * we also split bbls, use the "safe" method
   */
  CFG_FOREACH_BBL_SAFE(cfg,i_bbl,tmp_bbl)
  {
    t_ins *i_ins;
    BBL_FOREACH_INS(i_bbl,i_ins)
    {
      t_arm_ins *ins = T_ARM_INS(i_ins);
      if (ARM_INS_OPCODE(ins) == ARM_ADDRESS_PRODUCER)
      {
        t_reloc *rel;
        t_string newcode;
        t_arm_ins *picadd;
        int relocidx;

        ASSERT(RELOC_REF_NEXT(ARM_INS_REFERS_TO(ins))==NULL,("Address producer with multiple relocs? @I",i_ins));
        ASSERT(RELOC_REF_PREV(ARM_INS_REFERS_TO(ins))==NULL,("Address producer with multiple relocs? @I",i_ins));
        rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins));
        /* already relative -> skip */
        if (RelocIsRelative(rel))
          continue;
        /* is it a supported absolute relocation? */
        RelocAbs32GetEquivRel32(cfg,rel,&newcode,&relocidx);
        /* if this fails, check whether it's a relative formula already and skipt it,
         * or add support for converting it into one
         */
        ASSERT(*newcode,("Unrecognised absolute/relative reloc @R",rel));
        Free(RELOC_CODE(rel));
        RELOC_SET_CODE(rel,StringDup(newcode));
        /* take into account the pc+4/+8 behaviour */
        if(ARM_INS_FLAGS(ins) & FL_THUMB)
          RELOC_ADDENDS(rel)[0] = AddressSubUint32(RELOC_ADDENDS(rel)[0],4);
        else
          RELOC_ADDENDS(rel)[0] = AddressSubUint32(RELOC_ADDENDS(rel)[0],8);
        /* add the add rx,pc,rx instruction */
        picadd = ArmInsNewForBbl(i_bbl);
        if (ARM_INS_FLAGS(ins) & FL_THUMB)
          ARM_INS_SET_FLAGS(picadd,ARM_INS_FLAGS(picadd) | FL_THUMB);

        if (ARM_INS_REGA(ins) == ARM_REG_R15)
        {
          /* special care should be taken when producing an address in the PC */
          ASSERT(!CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(i_bbl))
                  && BBL_INS_FIRST(CFG_EDGE_HEAD(BBL_PRED_FIRST(i_bbl)))
                  && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_HEAD(BBL_PRED_FIRST(i_bbl))))) == TH_BX_R15, ("PIC address producing code in PC only supported for from-thumb stubs!"));

          /* by convention: ip is the from-thumb PIC register */
          ARM_INS_SET_REGA(ins, ARM_REG_R12);
          ArmInsMakeAdd(picadd, ARM_REG_R15, ARM_REG_R12, ARM_REG_R15, 0, ARM_INS_CONDITION(ins));
        }
        else
        {
          ArmInsMakeAdd(picadd,ARM_INS_REGA(ins),ARM_INS_REGA(ins),ARM_REG_R15,0,ARM_INS_CONDITION(ins));
        }
        ArmInsInsertAfter(picadd,ins);
        /* set that picadd as the address to be subtracted in the relocation
         * (by now, relocations have been moved from pointing to bbls back
         *  to pointing individual instructions)
         */
        if (relocidx<=RELOC_N_TO_RELOCATABLES(rel))
          RelocAddRelocatable(rel,T_RELOCATABLE(picadd),AddressNullForCfg(cfg));
        else
          RelocSetToRelocatable(rel,relocidx,T_RELOCATABLE(picadd));
        RELOC_TO_RELOCATABLE_OFFSET(rel)[relocidx] = AddressNullForCfg(cfg);
#ifdef DEBUG_PIC
        VERBOSE(3,(" *** -----> addrprod @I PIC'ed: @R",ins,rel));
#endif
        /* i_ins will now be the last instruction of the current bbl ->
         * we will stop processing it, but process the rest once we arrive
         * at the new bbl we just added
         */
      }
    }
  }
}
/*}}}*/

/* Changes 32 bit branch instructions whereby the instruction data crosses a page boundary
 * and the destination lies in the first page, into branches that go via an intermediate
 * stub in order to avoid triggering a Cortex-A8 erratum
 */
/*ArmFixupArmCortexA8Errata{{{*/
static void
ArmFixupArmCortexA8Errata(t_cfg *cfg, t_bbl *chain_head)
{
/*  static int teller = 0; */
  t_bbl * bbl;

  CFG_FOREACH_BBL(cfg,bbl)
    {
      t_arm_ins * last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
      t_bbl * last_bbl_in_chain;
      t_bbl *  bbl_for_veneer;
      t_address last_ins_address;
      t_address target_address;
      t_address veneer_address;
      t_uint32 disp_to_veneer;

      if (!last_ins) continue;
      if (!(ARM_INS_FLAGS(last_ins) & FL_THUMB)) continue;
      if (INS_CSIZE(T_INS(last_ins))!=4) continue;
      if (ARM_INS_OPCODE(last_ins)!=ARM_B && ARM_INS_OPCODE(last_ins)!=ARM_BL && ARM_INS_OPCODE(last_ins)!=ARM_BLX) continue;
      if (!(ARM_INS_FLAGS(last_ins)| FL_IMMED)) continue;
      if ((G_T_UINT32(ARM_INS_CADDRESS(last_ins)) & 0xfff) != 0xffe) continue;
      if (ARM_INS_IMMEDIATE(last_ins) >= 0x0LL || ARM_INS_IMMEDIATE(last_ins) < -0x1000LL) continue;

      //      if (teller++>=diablosupport_options.debugcounter)
      //        break;

      //      ASSERT(!ArmInsIsInITBlock(last_ins),("Implement VENEER SUPPORT for @ieB",bbl));

      last_bbl_in_chain =  BBL_LAST_IN_CHAIN(chain_head);

      bbl_for_veneer = BblNew(cfg);

      BBL_SET_LAST_IN_CHAIN(chain_head,bbl_for_veneer);
      BBL_SET_NEXT_IN_CHAIN(last_bbl_in_chain,bbl_for_veneer);
      BBL_SET_PREV_IN_CHAIN(bbl_for_veneer,last_bbl_in_chain);
      BBL_SET_NEXT_IN_CHAIN(bbl_for_veneer,NULL);

      last_ins_address = INS_CADDRESS(T_INS(last_ins));

      if (ARM_INS_OPCODE(last_ins)==ARM_BLX)
        last_ins_address = AddressAnd(last_ins_address,AddressNewForCfg(cfg,~0x3));

      target_address = AddressAddUint32(last_ins_address,4+ARM_INS_IMMEDIATE(last_ins));

      veneer_address = AddressAdd(BBL_CADDRESS(last_bbl_in_chain),BBL_CSIZE(last_bbl_in_chain));
      if (ARM_INS_OPCODE(last_ins)==ARM_BLX && G_T_UINT32(veneer_address) & 0x2)
        veneer_address = AddressAddUint32(veneer_address,0x2);

      disp_to_veneer = G_T_UINT32(veneer_address) - G_T_UINT32(INS_CADDRESS(T_INS(last_ins))) - 4;

      BBL_SET_CADDRESS(bbl_for_veneer,veneer_address);

      //      WARNING(("WARNING: INSERTING VENEER TO DEAL WITH ARM ERRATUM 657417 for @I in @ieB",last_ins,bbl));
      //      WARNING(("    TARGET ADDRESS 0x%x",G_T_UINT32(target_address)));
      //      WARNING(("    VENEER ADDRESS 0x%x",veneer_address));
      //      WARNING(("    DISP TO VENEER 0x%x",disp_to_veneer));

      if (ARM_INS_OPCODE(last_ins)==ARM_BLX && disp_to_veneer < 0x1000000)
        {
          t_arm_ins * veneer;
          t_cfg_edge * edge;
          t_int32 disp_from_veneer;

          BBL_SET_ALIGNMENT(bbl_for_veneer,4);
          BBL_SET_ALIGNMENT_OFFSET(bbl_for_veneer,0);
          veneer = ArmInsNewForBbl(bbl_for_veneer);
          ArmInsMakeUncondBranch(veneer);
          ARM_INS_SET_CSIZE(veneer,AddressNewForCfg(cfg,4));
          ARM_INS_SET_CADDRESS(veneer,veneer_address);
          InsAppendToBbl(T_INS(veneer),bbl_for_veneer);
          disp_from_veneer = (t_int32) G_T_UINT32(target_address) - (t_int32) (G_T_UINT32(veneer_address)+8);
          ARM_INS_SET_IMMEDIATE(veneer,disp_from_veneer);
          ARM_INS_SET_IMMEDIATE(last_ins,disp_to_veneer);

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            if (CFG_EDGE_CAT(edge)==ET_CALL)
              {
                BblInsertInFunction(bbl_for_veneer,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
                CfgEdgeCreate(cfg,bbl_for_veneer,CFG_EDGE_TAIL(edge),ET_JUMP);
                CfgEdgeChangeTail(edge,bbl_for_veneer);
              }

          //          WARNING(("BBL POST INSERTION: @ieB",bbl));
          //          WARNING(("VENEER POST INSERTION: @ieB",bbl_for_veneer));
        }
      else if (((ARM_INS_CONDITION(last_ins)==ARM_CONDITION_AL || ArmInsIsInITBlock(last_ins)) && disp_to_veneer < 0x1000000) || (ARM_INS_OPCODE(last_ins)==ARM_B && ARM_INS_CONDITION(last_ins)!=ARM_CONDITION_AL && disp_to_veneer < 0x100000))
        {
          t_arm_ins * veneer;
          t_cfg_edge * edge;
          t_int32 disp_from_veneer;

          veneer = ArmInsNewForBbl(bbl_for_veneer);
          ArmInsMakeUncondThumbBranch(veneer);
          ARM_INS_SET_CSIZE(veneer,AddressNewForCfg(cfg,4));
          ARM_INS_SET_CADDRESS(veneer,veneer_address);
          InsAppendToBbl(T_INS(veneer),bbl_for_veneer);
          disp_from_veneer = (t_int32) G_T_UINT32(target_address) - (t_int32) (G_T_UINT32(veneer_address)+4);
          ARM_INS_SET_IMMEDIATE(veneer,disp_from_veneer);
          ARM_INS_SET_IMMEDIATE(last_ins,disp_to_veneer);

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            {
              if (CFG_EDGE_CAT(edge)==ET_CALL)
                {
                  BblInsertInFunction(bbl_for_veneer,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
                  CfgEdgeCreate(cfg,bbl_for_veneer,CFG_EDGE_TAIL(edge),ET_JUMP);
                  CfgEdgeChangeTail(edge,bbl_for_veneer);
                }
              else if (CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
                {
                  BblInsertInFunction(bbl_for_veneer,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
                  CfgEdgeCreate(cfg,bbl_for_veneer,CFG_EDGE_TAIL(edge),ET_JUMP);
                  CfgEdgeChangeTail(edge,bbl_for_veneer);
                }
            }

          //          WARNING(("BBL POST INSERTION: @ieB",bbl));
          //          WARNING(("VENEER POST INSERTION: @ieB",bbl_for_veneer));
        }
      else if (ARM_INS_OPCODE(last_ins)==ARM_B && ARM_INS_CONDITION(last_ins)!=ARM_CONDITION_AL && disp_to_veneer < 0x1000000)
        {
          t_arm_ins * veneer1, * veneer2, * veneer3;
          t_cfg_edge * edge;
          t_int32 disp_from_veneer2, disp_from_veneer3;
          t_bbl * bbl_for_veneer2, * bbl_for_veneer3;

          veneer1 = ArmInsNewForBbl(bbl_for_veneer);
          ArmInsMakeCondBranch(veneer1,ARM_INS_CONDITION(last_ins));
          ARM_INS_SET_FLAGS(veneer1,ARM_INS_FLAGS(veneer1) | FL_THUMB);
          ARM_INS_SET_CSIZE(veneer1,AddressNewForCfg(cfg,4));
          ARM_INS_SET_CADDRESS(veneer1,veneer_address);
          ARM_INS_SET_IMMEDIATE(veneer1,0x4LL);
          InsAppendToBbl(T_INS(veneer1),bbl_for_veneer);

          bbl_for_veneer2 = BblNew(cfg);

          last_bbl_in_chain =  BBL_LAST_IN_CHAIN(chain_head);
          BBL_SET_LAST_IN_CHAIN(chain_head,bbl_for_veneer2);
          BBL_SET_NEXT_IN_CHAIN(last_bbl_in_chain,bbl_for_veneer2);
          BBL_SET_PREV_IN_CHAIN(bbl_for_veneer2,last_bbl_in_chain);
          BBL_SET_NEXT_IN_CHAIN(bbl_for_veneer2,NULL);
          BBL_SET_CADDRESS(bbl_for_veneer2,AddressAdd(BBL_CADDRESS(last_bbl_in_chain),BBL_CSIZE(last_bbl_in_chain)));

          veneer2 = ArmInsNewForBbl(bbl_for_veneer2);
          ArmInsMakeUncondThumbBranch(veneer2);
          ARM_INS_SET_CSIZE(veneer2,AddressNewForCfg(cfg,4));
          ARM_INS_SET_CADDRESS(veneer2,AddressAddUint32(veneer_address,4));
          InsAppendToBbl(T_INS(veneer2),bbl_for_veneer2);
          disp_from_veneer2 = (t_int32) G_T_UINT32(INS_CADDRESS(T_INS(last_ins))) + 4 - (t_int32) (G_T_UINT32(veneer_address)+4+4);
          ARM_INS_SET_IMMEDIATE(veneer2,disp_from_veneer2);

          bbl_for_veneer3 = BblNew(cfg);

          last_bbl_in_chain =  BBL_LAST_IN_CHAIN(chain_head);
          BBL_SET_LAST_IN_CHAIN(chain_head,bbl_for_veneer3);
          BBL_SET_NEXT_IN_CHAIN(last_bbl_in_chain,bbl_for_veneer3);
          BBL_SET_PREV_IN_CHAIN(bbl_for_veneer3,last_bbl_in_chain);
          BBL_SET_NEXT_IN_CHAIN(bbl_for_veneer3,NULL);
          BBL_SET_CADDRESS(bbl_for_veneer3,AddressAdd(BBL_CADDRESS(last_bbl_in_chain),BBL_CSIZE(last_bbl_in_chain)));

          veneer3 = ArmInsNewForBbl(bbl_for_veneer3);
          ArmInsMakeUncondThumbBranch(veneer3);
          ARM_INS_SET_CSIZE(veneer3,AddressNewForCfg(cfg,4));
          ARM_INS_SET_CADDRESS(veneer3,AddressAddUint32(veneer_address,8));
          InsAppendToBbl(T_INS(veneer3),bbl_for_veneer3);
          disp_from_veneer3 = (t_int32) G_T_UINT32(target_address) - (t_int32) (G_T_UINT32(veneer_address)+4+4+4);
          ARM_INS_SET_IMMEDIATE(veneer3,disp_from_veneer3);

          ArmInsMakeUncondThumbBranch(last_ins);
          ARM_INS_SET_IMMEDIATE(last_ins,disp_to_veneer);

          BblInsertInFunction(bbl_for_veneer,BBL_FUNCTION(bbl));
          BblInsertInFunction(bbl_for_veneer2,BBL_FUNCTION(bbl));
          BblInsertInFunction(bbl_for_veneer3,BBL_FUNCTION(bbl));

          CfgEdgeCreate(cfg,bbl_for_veneer,bbl_for_veneer2,ET_FALLTHROUGH);

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            {
              if (CFG_EDGE_CAT(edge)==ET_JUMP)
                {
                  CfgEdgeCreate(cfg,bbl_for_veneer3,CFG_EDGE_TAIL(edge),ET_JUMP);
                  CfgEdgeChangeTail(edge,bbl_for_veneer);
                }
              else if (CFG_EDGE_CAT(edge)==ET_IPJUMP)
                {
                  CfgEdgeCreate(cfg,bbl_for_veneer3,CFG_EDGE_TAIL(edge),ET_IPJUMP);
                  CfgEdgeChangeTail(edge,bbl_for_veneer);
                }
              else if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH)
                {
                  CfgEdgeCreate(cfg,bbl_for_veneer2,CFG_EDGE_TAIL(edge),ET_JUMP);
                }
              else if (CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
                {
                  if (BBL_FUNCTION(bbl_for_veneer2)==BBL_FUNCTION(CFG_EDGE_TAIL(edge)))
                    CfgEdgeCreate(cfg,bbl_for_veneer2,CFG_EDGE_TAIL(edge),ET_JUMP);
                  else
                    CfgEdgeCreate(cfg,bbl_for_veneer2,CFG_EDGE_TAIL(edge),ET_IPJUMP);
                }
            }

          CfgEdgeCreate(cfg,bbl_for_veneer,bbl_for_veneer3,ET_JUMP);

          //          WARNING(("BBL POST INSERTION: @ieB",bbl));
          //          WARNING(("VENEER block 1 POST INSERTION: @ieB",bbl_for_veneer));
          //          WARNING(("VENEER block 2 POST INSERTION: @ieB",bbl_for_veneer2));
          //          WARNING(("VENEER block 3 POST INSERTION: @ieB",bbl_for_veneer3));
        }
      else
        FATAL(("Implement support for veneers for instructions like @I",last_ins));
    }
}
/*}}}*/

/*!
 * \todo Document
 *
 * \param head
 * \param cfg
 *
 * \return void
 */
/* ArmCfgLayout {{{ */
void ArmCfgLayout(t_cfg * cfg, t_uint32 mode)
{
  t_bbl * chain_head;
  t_uint32 size;
  t_object *obj = CFG_OBJECT(cfg);
  t_section *code = OBJECT_CODE(obj)[0];
  t_bbl * bbl;

  STATUS(START,("Cfg Layout"));

  /* change address producers (back) into PIC code, if necessary */
  if ((OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC) ||
      (OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC))
    ArmConvertAddressProducersToPIC(cfg);
 
  ArmInsertThumbITInstructions(cfg);

  CFG_FOREACH_BBL(cfg,bbl)
  {
     if (!BBL_INS_FIRST(bbl)) continue;
     if (ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(bbl)))==IT_DATA) continue;
     if ((ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(bbl))) & FL_THUMB) &&
         (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) != TH_BX_R15))
     {
        BBL_SET_ALIGNMENT(bbl,2);
     }
     else
        BBL_SET_ALIGNMENT(bbl,4);
     BBL_SET_ALIGNMENT_OFFSET(bbl,0);

     ASSERT(ArmValidateBbl(bbl),("oh god, please don't put ARM and Thumb instructions in one BBL! @eiB", bbl));
  }

  CFG_FOREACH_BBL(cfg, bbl)
        worst_case_alignment = ((t_int32)BBL_ALIGNMENT(bbl) > worst_case_alignment) ? BBL_ALIGNMENT(bbl) : worst_case_alignment;
  if (worst_case_alignment == 0)
        worst_case_alignment = 2;
  VERBOSE(0,("worst-case alignment: %d", worst_case_alignment));

  chain_head=ArmChain(cfg);

  /* In case of Thumb-2, start by making all direct jumps 32 bit;
     afterwards, where possible we'll make them 16 bits again
   */
  
  ArmConvertBranchesToThumb1(cfg);

  if (diabloarm_options.fullthumb2)
     ArmConvertBranchesTo32BitIns(cfg);
  else
     ArmConvertBranchesToThumb1(cfg);

  //ArmProcessThumb2BranchesInChains(cfg, chain_head, diabloarm_options.fullthumb2, FALSE);

  SECTION_SET_TMP_BUF(OBJECT_CODE(CFG_OBJECT(cfg))[0],chain_head);
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
  //  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));

  STATUS(START,("Trampoline insertion round 1"));
  t_uint32 nr_tramp = 0;

  
  // first, take conservative approach that all address/constant producers will require a data pool entry in their neighbourhood
  // for deciding where and how to place trampolines
  // the simplest implementation is to increase the size of those producers and compute trampolines as usual
  CFG_FOREACH_BBL(cfg,bbl)
    {
      t_arm_ins * ins;
      BBL_FOREACH_ARM_INS(bbl,ins)
        {
          if (ARM_INS_OPCODE(ins)==ARM_ADDRESS_PRODUCER || ARM_INS_OPCODE(ins)==ARM_CONSTANT_PRODUCER || ARM_INS_OPCODE(ins)==ARM_VFPFLOAT_PRODUCER)
            ARM_INS_SET_CSIZE(ins,ARM_INS_CSIZE(ins)+4);
        }
    }

  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
  
#if 1
  while (ArmProcessThumb2BranchesInChains(cfg, chain_head, FALSE, FALSE, &nr_tramp))
  {
    /* recalculate all addresses after these changes */
    AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
    ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
    /* constant/address pools have to start at a multiple of 4 bytes,
     * may become less because of 16 bit Thumb instructions
     */
    /* update chain head for cfg (may have changed) */
    SECTION_SET_TMP_BUF(OBJECT_CODE(CFG_OBJECT(cfg))[0],chain_head);
  }
#else
  ArmProcessThumb2BranchesInChains(cfg, chain_head, FALSE, FALSE);
#endif
  STATUS(STOP,("Trampoline insertion round 1: nr_tramp %d", nr_tramp));
  nr_tramp = 0;

  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));

  // undo the above size increase of address and constant producers before actually transforming them

  CFG_FOREACH_BBL(cfg,bbl)
    {
      t_arm_ins * ins;
      BBL_FOREACH_ARM_INS(bbl,ins)
        {
          if (ARM_INS_OPCODE(ins)==ARM_ADDRESS_PRODUCER || ARM_INS_OPCODE(ins)==ARM_CONSTANT_PRODUCER || ARM_INS_OPCODE(ins)==ARM_VFPFLOAT_PRODUCER)
            ARM_INS_SET_CSIZE(ins,ARM_INS_CSIZE(ins)-4);
        }
    }

  SECTION_SET_TMP_BUF(code,  chain_head);
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
  SimpleAddressProducersForChain (chain_head, TRUE);
  ArmAlignPools(cfg,&chain_head);
  /* update chain head for cfg (may have changed) */
  SECTION_SET_TMP_BUF(OBJECT_CODE(CFG_OBJECT(cfg))[0],chain_head);
  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));

  STATUS(START,("Trampoline insertion round 2"));

  while (ArmProcessThumb2BranchesInChains(cfg, chain_head, FALSE, TRUE, &nr_tramp))
  {
    /* recalculate all addresses after these changes */
    AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
    ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
    /* constant/address pools have to start at a multiple of 4 bytes,
     * may become less because of 16 bit Thumb instructions
     */
    /* update chain head for cfg (may have changed) */
    ArmAlignPools(cfg,&chain_head);
	SECTION_SET_TMP_BUF(OBJECT_CODE(CFG_OBJECT(cfg))[0],chain_head);
  }
  /* and once more */
  STATUS(STOP,("Trampoline insertion round 2: nr_tramp %d", nr_tramp));

  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
  TestTbbTbhDistances(cfg); /* Sanity check: verify that the TBB/TBH max jump distances are respected */
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  t_bool optimize = TRUE;
  DiabloBrokerCall("DetermineAddressProducersOptimization", &optimize);

  OptimizeAddressProducersForChain (chain_head, optimize);
  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  ArmConvertThumbBranchesTo16BitIns(cfg);
  AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  /* handle possible errors in the layouting code */

  CFG_FOREACH_BBL(cfg,bbl)
     ASSERT(ArmValidateBbl(bbl),("oh god, please don't put ARM and Thumb instructions in one BBL! @eiB", bbl));

#ifdef SCHEDULE
  ScheduleCfg(cfg);
  /*  SchedulerCfgListSchedule(cfg);*/
#endif

  if (!diabloarm_options.nofixcortexa8)
  {
    ArmFixupArmCortexA8Errata(cfg, chain_head);

    /* make sure that every jump instruction has a valid address assigned to it */
    AssignAddressesInChain (chain_head, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]));
  }

  DiabloBrokerCall("AfterCodeLayoutFixed", cfg);

  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
  DiabloBrokerCall("AfterDataLayoutFixed", cfg);
  ArmRelocate(cfg,mode);
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  STATUS(STOP,("Cfg Layout"));

  if (exidx_subsection_order)
    Free(exidx_subsection_order);
}
/* }}} */


/***************************************************************************/
/***                    FUNCTIONS FOR ORDERING CHAINS                    ***/
/***************************************************************************/

/* Where does 0x8000000 come from?
 * Long story short, the largest hardcoded value that was here before, was 1,000,000
 * (for the CB(N)Z-instructions). This is equal to 0xf4240 in hex.
 * As we want to make the weight depend on the number of available bits, where more
 * bits mean a lower weight, a simple right shift by the amount of available bits
 * seems appropriate. So, a 1-bit-set value that approximately equals 1,000,000 is 0x100000.
 * Shift this left by 7 (the number of available bits in CB(N)Z) gives us 0x8000000. */
#define BASE_WEIGHT 0x8000000

static t_uint32 CalculateWeightForBranchEdge(t_arm_ins * branch_ins)
{
        /* only look at Thumb branches here */
        if (ARM_INS_FLAGS(branch_ins) & FL_THUMB)
        {
                /* differentiate between the availability of full Thumb-2 support or not */
                if (diabloarm_options.fullthumb2)
                {
                        if (ARM_INS_OPCODE(branch_ins) == ARM_T2CBZ
                            || ARM_INS_OPCODE(branch_ins) == ARM_T2CBNZ)
                        {
                                /* 7 bits available */
                                return BASE_WEIGHT >> 7;
                        }

                        if (ARM_INS_OPCODE(branch_ins) == ARM_B)
                        {
                                /* unconditional: 24 bits available
                                 * conditional  : 20 bits available */
                                return (ARM_INS_CONDITION(branch_ins) == ARM_CONDITION_AL) ? BASE_WEIGHT >> 24 : BASE_WEIGHT >> 20;
                        }
                }
                else
                {
                        if (ARM_INS_OPCODE(branch_ins) == ARM_B)
                        {
                                /* unconditional: 11 bits available
                                 * conditional  :  8 bits available */
                                return (ARM_INS_CONDITION(branch_ins) == ARM_CONDITION_AL) ? BASE_WEIGHT >> 11 : BASE_WEIGHT >> 8;
                        }
                }
        }

        return 0;
}

/* {{{ Add edges to the call graph for a particular chain */
static void AddEdgesForChainAndNode(t_graph *graph, t_bbl *chain, t_chain_node *node, t_bool consider_cond_thumb2_branches_during_layout)
{
  t_bbl *bbl;
  t_arm_ins *ins;
  t_edge * existing;
  int increment;

  t_address max_fw, max_bw;
  max_fw=AddressNew32(MAX_OFFSET_FORWARD);
  max_bw=AddressNew32(MAX_OFFSET_BACKWARD);

  CHAIN_FOREACH_BBL(chain,bbl)
  {
    if (IS_DATABBL(bbl)) continue;

    BBL_FOREACH_ARM_INS(bbl,ins)
    {
      t_reloc *rel;
      t_bbl *to;
      t_address fw_offset, bw_offset;
      t_uint32 i;

	  if (ARM_INS_OPCODE(ins)==ARM_T2CBZ || ARM_INS_OPCODE(ins)==ARM_T2CBNZ)
	  {
	     t_cfg_edge *  edge;
	     BBL_FOREACH_SUCC_EDGE(bbl,edge)
          if (CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
		      break;
	     ASSERT(edge, ("NOOO"));

	     to = CFG_EDGE_TAIL(edge);
	     if (to!=bbl && BBL_CHAIN_NODE(bbl)!=BBL_CHAIN_NODE(to))
		  {
		    if ((NodeIsSucc(BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),0xffffffff)))
		    {
		      NODE_FOREACH_SUCC_EDGE(BBL_CHAIN_NODE(bbl),existing)
	           if (EDGE_TAIL(existing)==BBL_CHAIN_NODE(to)) break;
		      ASSERT(existing,("Oops"));
		      EDGE_SET_CAT(existing,  EDGE_CAT(existing) + CalculateWeightForBranchEdge(ins));
		    }
	       else
		    {
		      GraphNewEdge(graph,BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),CalculateWeightForBranchEdge(ins));
		      T_CHAIN_NODE(BBL_CHAIN_NODE(bbl))->nr_edges_out++;
		      T_CHAIN_NODE(BBL_CHAIN_NODE(to))->nr_edges_in++;
		    }
		  }
	  }

	  if (/*consider_cond_thumb2_branches_during_layout &&*/ ARM_INS_OPCODE(ins)==ARM_B /*&& ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL*/ && (ARM_INS_FLAGS(ins) & FL_THUMB))
	    {
	      t_cfg_edge *  edge;
	      BBL_FOREACH_SUCC_EDGE(bbl,edge)
		if (CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
		  break;
	      ASSERT(edge, ("NOOO"));

	      to = CFG_EDGE_TAIL(edge);

	      if (to!=bbl && BBL_CHAIN_NODE(bbl)!=BBL_CHAIN_NODE(to))
		{
		  if ((NodeIsSucc(BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),0xffffffff)))
		    {
		      NODE_FOREACH_SUCC_EDGE(BBL_CHAIN_NODE(bbl),existing)
			if (EDGE_TAIL(existing)==BBL_CHAIN_NODE(to)) break;
		      ASSERT(existing,("Oops"));

		      if (diabloarm_options.fullthumb2)
			EDGE_SET_CAT(existing,  EDGE_CAT(existing) + CalculateWeightForBranchEdge(ins));
		      else
			{
			  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
			    EDGE_SET_CAT(existing,  EDGE_CAT(existing) + CalculateWeightForBranchEdge(ins));
			  else
			    EDGE_SET_CAT(existing,  EDGE_CAT(existing) + CalculateWeightForBranchEdge(ins));
			}
		    }
		  else
		    {
		      if (diabloarm_options.fullthumb2)
			GraphNewEdge(graph,BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),CalculateWeightForBranchEdge(ins));
		      else
			{
			  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
			    GraphNewEdge(graph,BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),CalculateWeightForBranchEdge(ins));
			  else
			    GraphNewEdge(graph,BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),CalculateWeightForBranchEdge(ins));
			}
		      
		      T_CHAIN_NODE(BBL_CHAIN_NODE(bbl))->nr_edges_out++;
		      T_CHAIN_NODE(BBL_CHAIN_NODE(to))->nr_edges_in++;
		    }
		}
	    }

      if (ARM_INS_OPCODE(ins) != ARM_ADDRESS_PRODUCER)
        continue;
      rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins));
      for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
      {
        if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) != RT_BBL)
          continue;
        to = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
        if (node && BBL_CHAIN_NODE(to) != node)
          continue;	/* if a specific node is specified, only do address producers to that node */
        if (BBL_CHAIN_NODE(to) == BBL_CHAIN_NODE(bbl))
          continue;
        if (!BBL_CHAIN_NODE(to))
          continue;	/* skip if the addresses bbl lies in a different code section */

        fw_offset = AddressAdd(ARM_INS_DIST_FROM_END(ins),BBL_CADDRESS(to));
        bw_offset = AddressAdd(ARM_INS_CADDRESS(ins),BBL_DIST_FROM_END(to));

        increment = (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl)) ? HOTNESS_BONUS : 1;

        if (AddressIsLe(fw_offset,max_fw))
        {
          /* edge (bbl -> to) */
          if ((NodeIsSucc(BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),0xffffffff)))
          {
            NODE_FOREACH_SUCC_EDGE(BBL_CHAIN_NODE(bbl),existing)
              if (EDGE_TAIL(existing)==BBL_CHAIN_NODE(to)) break;
            ASSERT(existing,("Oops"));
            EDGE_SET_CAT(existing,  EDGE_CAT(existing) + increment*1000);
          }
          else
          {
            GraphNewEdge(graph,BBL_CHAIN_NODE(bbl),BBL_CHAIN_NODE(to),increment*1000);
            T_CHAIN_NODE(BBL_CHAIN_NODE(bbl))->nr_edges_out++;
            T_CHAIN_NODE(BBL_CHAIN_NODE(to))->nr_edges_in++;
          }
        }
        if (AddressIsLe(bw_offset,max_bw))
        {
          /* edge (to -> bbl) */
          if ((NodeIsSucc(BBL_CHAIN_NODE(to),BBL_CHAIN_NODE(bbl),0xffffffff)))
          {
            NODE_FOREACH_SUCC_EDGE(BBL_CHAIN_NODE(to),existing)
              if (EDGE_TAIL(existing)==BBL_CHAIN_NODE(bbl)) break;
            ASSERT(existing,("Oops"));
            EDGE_SET_CAT(existing,   EDGE_CAT(existing) + increment*1000);
          }
          else
          {
            GraphNewEdge(graph,BBL_CHAIN_NODE(to),BBL_CHAIN_NODE(bbl),increment*1000);
            T_CHAIN_NODE(BBL_CHAIN_NODE(to))->nr_edges_out++;
            T_CHAIN_NODE(BBL_CHAIN_NODE(bbl))->nr_edges_in++;
          }
        }
      }
    }
  }
}

static void AddEdgesForChain(t_graph *graph, t_bbl *chain, t_bool consider_cond_thumb2_branches_during_layout)
{
  AddEdgesForChainAndNode(graph,chain,NULL, consider_cond_thumb2_branches_during_layout);
} /* }}} */

/* {{{ create a graph with the basic block chains as nodes.
 * the weight of each edge (CFG_EDGE_CAT) represents the
 * number of address producers that can be generated in
 * one instruction if the tail of the edge is layouted
 * immediately after the head of the edge */
t_graph * ArmCreateLayoutGraph(t_chain_holder *ch, t_bool * consider_cond_thumb2_branches_during_layout)
{
  t_uint32 i;
  t_bbl * bbl;
  t_graph * graph = GraphNew(sizeof(t_chain_node),sizeof(t_edge));
  t_cfg *cfg;
  t_uint32 total_size = 0;
  t_address address_null = AddressNullForCfg(cfg);

  /* first clear all BBL_CHAIN_NODE() fields (not only of the current chain) */
  if (!ch->chains[0]) FATAL(("first chain is NULL"));
  cfg = BBL_CFG(ch->chains[0]);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_SET_CHAIN_NODE(bbl,  NULL);
      total_size += G_T_UINT32(BBL_CSIZE(bbl));
    }

  if (total_size >= 0xf8000)
    {
      VERBOSE(2,("Taking into account cond. thumb branches during layout because total code size is already %x",total_size));
      *consider_cond_thumb2_branches_during_layout = TRUE;
    }
  else
    {
      VERBOSE(2,("Not taking into account cond. thumb branches during layout because total code size is only %x",total_size));
      *consider_cond_thumb2_branches_during_layout = FALSE;
    }

  for (i=0; i<ch->nchains; i++)
  {
        /* skip empty entries */
        if (!ch->chains[i]) continue;
    t_chain_node * node = (t_chain_node *) GraphNewNode(graph,0);
    node->chain = ch->chains[i];
    node->nr_edges_in = 0;
    node->nr_edges_out = 0;
    if(!(node->chain)) FATAL(("No node in chain!"));
    for (bbl = node->chain; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
      BBL_SET_CHAIN_NODE(bbl,  node);

    /* abuse bbl->caddress: let it contain the distance from the start of the chain */
    AssignAddressesInChain(ch->chains[i],address_null);
  }

  /* add edges between chains if there is an address producer from one chain to another
   * that might possibly be generated in one instruction */
  for (i=0; i<ch->nchains; i++)
    AddEdgesForChain(graph,ch->chains[i],*consider_cond_thumb2_branches_during_layout);

  return graph;
}
/* }}} */

/* {{{ find the best edge in the layout graph for merging */
/* give some sort of estimate of potentially lost possibilities for
 * generating address producers in one instruction if the head and
 * tail of this edge are merged */
static float PotentialLoss(t_edge *edge)
{
  int out_weight_head = 0, in_weight_tail = 0;

  t_chain_node *head = (t_chain_node *)EDGE_HEAD(edge);
  t_chain_node *tail = (t_chain_node *)EDGE_TAIL(edge);
  float head_size = (float) G_T_UINT32(BBL_CHAIN_SIZE(head->chain));
  float min;

  float loss = 0.0;
  float max_offset = ((float)MAX_OFFSET_FORWARD + (float)MAX_OFFSET_BACKWARD)/2;

  t_edge *iter;

  NODE_FOREACH_SUCC_EDGE(T_NODE(head),iter)
    if (iter != edge)
      out_weight_head += EDGE_CAT(iter);
  NODE_FOREACH_SUCC_EDGE(T_NODE(tail),iter)
    if (iter != edge)
      in_weight_tail += EDGE_CAT(iter);

  min=(1.0<(head_size/max_offset))?1.0:(head_size/max_offset);
  loss += min * in_weight_tail;
  loss += min * out_weight_head;

  return loss;
}

static t_edge * GetMaxEdge(t_graph * layout_graph)
{
  t_edge *edge, *edge_max;
  t_uint32 max = 0;
  float min_loss;

  min_loss = 1000000.0;
  edge_max=NULL;

  GRAPH_FOREACH_EDGE(layout_graph,edge)
    if (EDGE_CAT(edge) > max)
    {
      max = EDGE_CAT(edge);
      edge_max = edge;
      min_loss = PotentialLoss(edge);
    }
    else if (EDGE_CAT(edge) == max)
    {
      float new_loss = PotentialLoss(edge);
      if (new_loss < min_loss)
      {
	min_loss = new_loss;
	edge_max = edge;
      }
    }
  return edge_max;
} /* }}} */

/* order the chains and merge a number of them. the objective is to let the address
 * producers be generated as efficiently as possible
 * {{{ */
#define MARK(list,node)	do { \
  		t_chain_node *n = (t_chain_node *)node; \
  	  	n->next_marked = list; \
    		list = n; \
	} while (0)

void ArmSortChains(t_chain_holder *ch)
{
  int i,j;
  t_edge *edge;
  t_arm_ins *ins;
  t_bbl *bbl;
  t_chain_node * node;
  t_chain_node *head, *tail;
  t_chain_node *mark = NULL;
  t_graph *layout_graph;
  t_address chain_size;
  t_address max_fw_load, max_bw_load;
  t_cfg *cfg = BBL_CFG(ch->chains[0]);
  t_bool consider_cond_thumb2_branches_during_layout;
  if (diabloflowgraph_options.blockprofilefile && diabloflowgraph_options.profiled_layout)
  {
    CfgComputeHotBblThreshold(cfg, HOTNESS_THRESHOLD);
    CfgEstimateEdgeCounts (cfg);
  }

#if 1
  /* {{{ try to merge chains to allow for address producers to be generated in one instruction */

  VERBOSE(0,("%d chains left",ch->nchains));
  layout_graph = ArmCreateLayoutGraph(ch,&consider_cond_thumb2_branches_during_layout);
  VERBOSE(0,("Before sorting: %d nodes, %d edges", GRAPH_NNODES(layout_graph), GRAPH_NEDGES(layout_graph)));

  while (GRAPH_NEDGES(layout_graph) > 0)
  {
    ASSERT(mark == NULL,("Unmarking nodes went wrong"));

    edge = GetMaxEdge(layout_graph);
    head = (t_chain_node *)EDGE_HEAD(edge);
    tail = (t_chain_node *)EDGE_TAIL(edge);

    /* merge the chains */
    AssignAddressesInChain(tail->chain,BBL_CHAIN_SIZE(head->chain));	/* no need to reassign addresses in the head chain */
    MergeChains(head->chain,tail->chain);

    CHAIN_FOREACH_BBL(tail->chain,bbl)
      BBL_SET_CHAIN_NODE(bbl,  head);


    /* {{{ update the graph */
    /* kill all invalid edges */
    while ((edge = NODE_SUCC_FIRST(T_NODE(head))))
    {
      node = T_CHAIN_NODE(EDGE_TAIL(edge));
      if (node != tail) MARK(mark,node);
      head->nr_edges_out--;
      node->nr_edges_in--;
      GraphRemoveEdge(layout_graph,edge);
    }
    while ((edge = NODE_PRED_FIRST(T_NODE(head))))
    {
      node = T_CHAIN_NODE(EDGE_HEAD(edge));
      if (node != tail) MARK(mark,node);
      head->nr_edges_in--;
      node->nr_edges_out--;
      GraphRemoveEdge(layout_graph,edge);
    }
    while ((edge = NODE_SUCC_FIRST(T_NODE(tail))))
    {
      node = T_CHAIN_NODE(EDGE_TAIL(edge));
      if (node != tail) MARK(mark,node);
      tail->nr_edges_out--;
      node->nr_edges_in--;
      GraphRemoveEdge(layout_graph,edge);
    }
    while ((edge = NODE_PRED_FIRST(T_NODE(tail))))
    {
      node = T_CHAIN_NODE(EDGE_HEAD(edge));
      if (node != tail) MARK(mark,node);
      tail->nr_edges_in--;
      node->nr_edges_out--;
      GraphRemoveEdge(layout_graph,edge);
    }

    GraphRemoveNode(layout_graph,T_NODE(tail));

    /* create new edges instead */
    AddEdgesForChain(layout_graph,head->chain,consider_cond_thumb2_branches_during_layout);

    while ((node = mark))
    {
      mark = node->next_marked;
      node->next_marked = NULL;
      AddEdgesForChainAndNode(layout_graph,T_CHAIN_NODE(node)->chain,head,consider_cond_thumb2_branches_during_layout);
    }
    /* }}} */
  }

  VERBOSE(0,("After sorting: %d nodes, %d edges", GRAPH_NNODES(layout_graph), GRAPH_NEDGES(layout_graph)));

  /* now put the remaining chains back in the chain holder and deconstruct the layout graph */
  i = 0;
  while (GRAPH_NNODES(layout_graph) > 0)
  {
    ch->chains[i] = ((t_chain_node *)GRAPH_NODE_FIRST(layout_graph))->chain;
    CHAIN_FOREACH_BBL(ch->chains[i],bbl)
      BBL_SET_CHAIN_NODE(bbl,  NULL);
    GraphRemoveNode(layout_graph,GRAPH_NODE_FIRST(layout_graph));
    ++i;
  }
  ch->nchains = i;
  Free(layout_graph);
  /* }}} */
#endif

#if 1
  /* try to merge chains so that different address producers that produce the same address
   * can load that address from a shared data pool 
   * {{{ */
  max_fw_load=AddressNew32(MAX_LOAD_OFFSET_FORWARD);
  max_bw_load=AddressNew32(MAX_LOAD_OFFSET_BACKWARD);

  for (i=0; i<ch->nchains; i++)
    if (ch->chains[i])
      BBL_SET_CHAIN_INDEX(ch->chains[i],  (void *) (long)i);

  for (i=0; i<ch->nchains; i++)
  {
    if (!ch->chains[i]) continue;

restart_search:
    chain_size = BBL_CHAIN_SIZE(ch->chains[i]);
    CHAIN_FOREACH_BBL_R(BBL_LAST_IN_CHAIN(ch->chains[i]),bbl)
    {
      BBL_FOREACH_ARM_INS_R(bbl,ins)
      {
	t_relocatable *to;
	t_reloc_ref *rr;
        t_uint32 j;

	if (AddressIsGt(AddressSub(chain_size,ARM_INS_CADDRESS(ins)),max_fw_load))
	  goto next_chain;
	if (ARM_INS_OPCODE(ins) != ARM_ADDRESS_PRODUCER)
	  continue;

        for (j=0 ;j<RELOC_N_TO_RELOCATABLES(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins))); j++)
        {
	  int compare_count = 0;
          to = RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[j];
          for (rr=RELOCATABLE_REFED_BY(to); rr; rr=RELOC_REF_NEXT(rr))
          {
            t_reloc *rel = RELOC_REF_RELOC(rr);
            t_arm_ins *ins2;
            int index2;

            if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) != RT_INS) continue;
            if ((ins2 = T_ARM_INS(RELOC_FROM(rel))) == ins) continue;
            if (ARM_INS_OPCODE(ins2) != ARM_ADDRESS_PRODUCER) continue;
            if (cfg != BBL_CFG(ARM_INS_BBL(ins2))) continue;
	    /* limit the amount of comparisons for structures with many
	     * references: a structure that is referenced n times has n^2
	     * comparisons, which is way too slow for big n */
	    if (compare_count++ > 50) break;
            if (0!=RelocCmp(rel,RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)),FALSE))
              continue;
            if (!BblInChainHolder(ARM_INS_BBL(ins2),ch)) continue;	/* ins2 is in a different code section */
            if (AddressIsGt(ARM_INS_CADDRESS(ins2),max_bw_load)) continue;
            if ((index2 = BBL_CHAIN_INDEX_INT(ARM_INS_BBL(ins2))) == i) continue;

            /* if we reach this point, we've found a candidate for appending */
            AssignAddressesInChain(ch->chains[index2],chain_size);
            MergeChains(ch->chains[i],ch->chains[index2]);
            BBL_SET_CHAIN_INDEX(bbl,  (void*)(long)i);
            ch->chains[index2] = NULL;
            /* see if we can find another candidate for appending to this chain */
            goto restart_search;
          }
        }
      }
    }
next_chain:
    ;	/* just to keep the compiler happy */
  }

  for (i=0, j=0; i<ch->nchains; i++)
    if (ch->chains[i])
      ch->chains[j++] = ch->chains[i];
  ch->nchains = j;
  VERBOSE(0,("After second merging round: %d chains left",j));
  /* }}} */
#endif

} /* }}} */







/* SCHEDULER */


static t_int32 ComputeNrDep(t_arm_ins * ins1, t_arm_ins * ins2, t_arm_ins * ins3, t_arm_ins * ins4)
{
  t_int32 nr_deps = 0;

  if (ins1 && (ARM_INS_TYPE(ins1)==IT_LOAD || ARM_INS_TYPE(ins1)==IT_LOAD_MULTIPLE || ARM_INS_TYPE(ins1)==IT_FLT_LOAD))
    if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins1),ARM_INS_REGS_USE(ins2))))
      nr_deps += 1;

  if (ins4 && (ARM_INS_TYPE(ins3)==IT_LOAD || ARM_INS_TYPE(ins3)==IT_LOAD_MULTIPLE || ARM_INS_TYPE(ins3)==IT_FLT_LOAD ))
    if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins3),ARM_INS_REGS_USE(ins4))))
      nr_deps += 1;

  return nr_deps;
}

static t_bool InsIsLoad(t_arm_ins * ins)
{
  return ARM_INS_TYPE(ins)==IT_LOAD || ARM_INS_TYPE(ins)==IT_FLT_LOAD || ARM_INS_TYPE(ins)==IT_LOAD_MULTIPLE;
}

static t_bool InsIsSimpleLoad(t_arm_ins * ins)
{
  return ARM_INS_TYPE(ins)==IT_LOAD || ARM_INS_TYPE(ins)==IT_FLT_LOAD;
}

static t_bool InsIsStore(t_arm_ins * ins)
{
  return ARM_INS_TYPE(ins)==IT_STORE || ARM_INS_TYPE(ins)==IT_FLT_STORE || ARM_INS_TYPE(ins)==IT_STORE_MULTIPLE;
}

static t_bool InsIsSimpleStore(t_arm_ins * ins)
{
  return ARM_INS_TYPE(ins)==IT_STORE || ARM_INS_TYPE(ins)==IT_FLT_STORE;
}

static t_bool NoAliasing(t_arm_ins *ins1, t_arm_ins *ins2)
{
  if (ARM_INS_REGB(ins1)!=ARM_INS_REGB(ins2)) return FALSE;
  if (ARM_INS_IMMEDIATE(ins1)==ARM_INS_IMMEDIATE(ins2)) return FALSE;
  return TRUE;
}

static t_arm_ins * PreviousInsFT(t_arm_ins* ins)
{
  t_arm_ins * ret = ARM_INS_IPREV(ins);

  t_cfg_edge * edge;

  if (ret)
    return ret;

  BBL_FOREACH_PRED_EDGE(ARM_INS_BBL(ins),edge)
    {
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	return T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
    }

  return NULL;
}

static t_arm_ins * NextInsFT(t_arm_ins* ins)
{
  t_arm_ins * ret = ARM_INS_INEXT(ins);

  t_cfg_edge * edge;

  if (ret)
    return ret;

  BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(ins),edge)
    {
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	return T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge)));
    }

  return NULL;
}

void ScheduleCfg(t_cfg * cfg)
{
  t_bbl * bbl;
  t_arm_ins * ins;
  t_arm_ins * prev_ins = NULL;
  t_arm_ins * prev_prev_ins = NULL;
  t_arm_ins * next_ins = NULL;

  if (diabloflowgraph_options.blockprofilefile)
    CfgComputeHotBblThreshold(cfg,0.30);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (!BBL_FUNCTION(bbl)) continue;
    restart:
      BBL_FOREACH_ARM_INS_R(bbl,ins)
	{
	  if (ArmIsControlflow(ins)) continue;
	  if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R15)) continue;
	  if (ARM_INS_TYPE(ins)==IT_DATA) continue;
	  if (!(prev_ins=ARM_INS_IPREV(ins))) continue;
	  if (RegsetIn(ARM_INS_REGS_USE(prev_ins),ARM_REG_R15)) continue;
	  if (ARM_INS_TYPE(prev_ins)==IT_DATA) continue;
	  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(prev_ins),ARM_INS_REGS_USE(ins)))) continue;
	  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_USE(prev_ins),ARM_INS_REGS_DEF(ins)))) continue;
	  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(prev_ins),ARM_INS_REGS_DEF(ins)))) continue;
	  if ((InsIsLoad(ins) && InsIsStore(prev_ins)) ||
	      (InsIsStore(ins) && InsIsStore(prev_ins)) ||
	      (InsIsStore(ins) && InsIsLoad(prev_ins))
	      )
	    {
	      if ((InsIsSimpleLoad(ins) || InsIsSimpleStore(ins)) &&
		  (InsIsSimpleLoad(prev_ins) || InsIsSimpleStore(prev_ins)) &&
		  NoAliasing(ins,prev_ins)
		  )
		{
		  prev_prev_ins = PreviousInsFT(prev_ins);
		  next_ins = NextInsFT(ins);
		  if (ComputeNrDep(prev_prev_ins,prev_ins,ins,next_ins)>ComputeNrDep(prev_prev_ins,ins,prev_ins,next_ins))
		    {
#ifdef PRINT_LAYOUT
		      if (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl))
			VERBOSE(0,("BEFORE @iB",bbl));
#endif
		      BblMoveInstructionBefore(T_INS(ins),T_INS(prev_ins));
#ifdef PRINT_LAYOUT
		      if (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl))
			VERBOSE(0,("AFTER @iB",bbl));
#endif
		      goto restart;
		    }
		}
	      else
		continue;
	    }

	  prev_prev_ins = PreviousInsFT(prev_ins);
	  next_ins = NextInsFT(ins);
	  if (ComputeNrDep(prev_prev_ins,prev_ins,ins,next_ins)>ComputeNrDep(prev_prev_ins,ins,prev_ins,next_ins))
	    {
#ifdef PRINT_LAYOUT
	      if (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl))
		VERBOSE(0,("BEFORE @iB",bbl));
#endif
	      BblMoveInstructionBefore(T_INS(ins),T_INS(prev_ins));
#ifdef PRINT_LAYOUT
	      if (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl))
		VERBOSE(0,("AFTER @iB",bbl));
#endif
        goto restart;
      }
    }
  }
}
/* vim: set shiftwidth=2 foldmethod=marker : */
