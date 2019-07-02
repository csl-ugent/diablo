/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
void ArmOptimizeStackLoadAndStores(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_arm_ins *i_ins, *tmp;

  CFG_FOREACH_BBL(cfg,i_bbl)
    BBL_FOREACH_ARM_INS_SAFE(i_bbl,i_ins,tmp)
    {
      if (ARM_INS_OPCODE(i_ins)==ARM_STM &&
	  ARM_INS_REGB(i_ins)==ARM_REG_R13 &&
	  ARM_INS_INEXT(i_ins) &&
	  ARM_INS_OPCODE(ARM_INS_INEXT(i_ins))==ARM_STM &&
	  ARM_INS_REGB(ARM_INS_INEXT(i_ins))==ARM_REG_R13 &&
	  (ARM_INS_FLAGS(i_ins) & (FL_DIRUP|FL_WRITEBACK)) == (ARM_INS_FLAGS(ARM_INS_INEXT(i_ins)) & (FL_DIRUP|FL_WRITEBACK)) &&
	  !(ARM_INS_IMMEDIATE(i_ins)&ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins))))
      {
	int i;
	int num_regs = 0;
        int lowest_reg_first_ins = -1;
        int highest_reg_second_ins = -1;

	for (i=15; i>=0; i--)
	  if (ARM_INS_IMMEDIATE(i_ins) & (1 << i))
            {
              num_regs++;
              lowest_reg_first_ins = i;
            }

	for (i=15; i>=0; i--)
	  if (ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)) & (1 << i))
            {
              highest_reg_second_ins = i;
              break;
            }

        if (!(highest_reg_second_ins<lowest_reg_first_ins))
          continue;

	if (num_regs>=5)
	{
	  ARM_INS_SET_REGS_USE(i_ins,
	      RegsetUnion(ARM_INS_REGS_USE(i_ins),
		ARM_INS_REGS_USE(ARM_INS_INEXT(i_ins))));
	  ARM_INS_SET_IMMEDIATE(i_ins,
	      ARM_INS_IMMEDIATE(i_ins)|ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)));
	  tmp = ARM_INS_INEXT(tmp);
	  ArmInsKill(ARM_INS_INEXT(i_ins));
	}
      }

      if (ARM_INS_OPCODE(i_ins)==ARM_LDM &&
	  ARM_INS_REGB(i_ins)==ARM_REG_R13 &&
	  ARM_INS_INEXT(i_ins) &&
	  ARM_INS_OPCODE(ARM_INS_INEXT(i_ins))==ARM_LDM &&
	  ARM_INS_REGB(ARM_INS_INEXT(i_ins))==ARM_REG_R13 &&
	  (ARM_INS_FLAGS(i_ins) & (FL_DIRUP|FL_WRITEBACK)) == (ARM_INS_FLAGS(ARM_INS_INEXT(i_ins)) & (FL_DIRUP|FL_WRITEBACK)) &&
	  !(ARM_INS_IMMEDIATE(i_ins) & ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins))))
      {
	int i;
	int num_regs = 0;
        int highest_reg_first_ins = -1;
        int lowest_reg_second_ins = -1;
	for (i=0; i<16; i++)
	  if (ARM_INS_IMMEDIATE(i_ins) & (1 << i))
	    {
              highest_reg_first_ins = i;
              num_regs++;
            }

	for (i=0; i<16; i++)
	  if (ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)) & (1 << i))
            {
              lowest_reg_second_ins = i;
              break;
            }

        if (!(lowest_reg_second_ins>highest_reg_first_ins))
          continue;

	if (num_regs>=5)
	{
	  ARM_INS_SET_REGS_DEF(i_ins,
	      RegsetUnion (ARM_INS_REGS_DEF(i_ins),
		ARM_INS_REGS_DEF(ARM_INS_INEXT(i_ins))));
	  ARM_INS_SET_IMMEDIATE(i_ins,
	      ARM_INS_IMMEDIATE(i_ins)|ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)));
	  tmp = ARM_INS_INEXT(tmp);
	  ArmInsKill(ARM_INS_INEXT(i_ins));
	}
      }
    }
}

/*! Specialized version of OptKillUseless for the Arm architecture */

//#define DEBUG_KILL_USELESS

static t_bool ArmOptKillUseless(t_cfg * cfg)
{
  t_regset condition_bits =RegsetNew();

  /* iterators */
  t_bbl * i_bbl;
  t_ins * i_ins, * i_tmp;

  /* local variables */

  t_regset live = RegsetNew();
  t_regset use  = RegsetNew();
  t_regset def  = RegsetNew();
  t_regset tmp  = RegsetNew();
  t_uint32 kcount=0;
  t_uint32 kcountuncond = 0;
#ifdef DEBUG_KILL_USELESS
  static int total_count = 0;
#endif

  RegsetSetAddReg(condition_bits,ARM_REG_Q_CONDITION);
  RegsetSetAddReg(condition_bits,ARM_REG_C_CONDITION);
  RegsetSetAddReg(condition_bits,ARM_REG_V_CONDITION);
  RegsetSetAddReg(condition_bits,ARM_REG_Z_CONDITION);
  RegsetSetAddReg(condition_bits,ARM_REG_N_CONDITION);

  STATUS(START,("Kill Useless Instructions"));

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (!BBL_FUNCTION(i_bbl)) continue;
    if (IS_DATABBL(i_bbl)) continue;
    if( FUNCTION_IS_HELL(BBL_FUNCTION(i_bbl)) ) continue;

    RegsetSetDup(live,BBL_REGS_LIVE_OUT(i_bbl));

    BBL_FOREACH_INS_R_SAFE(i_bbl,i_ins,i_tmp)
    {
      RegsetSetDup(use,INS_REGS_USE(i_ins));
      RegsetSetDup(def,INS_REGS_DEF(i_ins));
      RegsetSetDup(tmp,def);
      RegsetSetIntersect(tmp,live);

      {
	t_arm_ins * ai_ins=(t_arm_ins *) i_ins;
	if (ARM_INS_OPCODE(ai_ins)==ARM_LDM)
	{
	  t_regset tmp2;
	  t_regset loadregs = RegsetNewFromUint32 (ARM_INS_IMMEDIATE (ai_ins));
	  RegsetSetDup(tmp2,def);
	  RegsetSetDiff(tmp2,live);
	  RegsetSetSubReg(tmp2,ARM_REG_R15);

	  if (!RegsetIsEmpty(tmp))
	    if (RegsetIsEmpty(RegsetIntersect(live, loadregs)))
	    {
	      if (!RegsetIn(loadregs, ARM_REG_R15))
	      {
		if (ArmInsWriteBackHappens(ai_ins))
		{
		  int bits=RegsetCountRegs(loadregs);

		  if (ARM_INS_FLAGS(ai_ins) & FL_DIRUP)
		  {
		    ArmInsMakeAdd(ai_ins,ARM_INS_REGB(ai_ins),ARM_INS_REGB(ai_ins),ARM_REG_NONE,4*bits,ARM_INS_CONDITION(ai_ins));
		  }
		  else
		  {
		    /*
		       ArmInsMakeSub(j_ins,ARM_INS_REGB(ai_ins),ARM_INS_REGB(ai_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(ai_ins)); */
		  }
		}
	      }
	    }

	}
	if ((ARM_INS_FLAGS(ai_ins) & FL_S) && !(ARM_INS_FLAGS(ai_ins) & FL_THUMB)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CMFE)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CMF)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CNF)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CNFE)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CMP)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_TST)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_TEQ)
	    && (ARM_INS_OPCODE(ai_ins)!=ARM_CMN)
	    /* protect instructions like MOVS pc,r14 (-> return from supervisor mode) */
	    && !RegsetIn(INS_REGS_DEF(i_ins),ARM_REG_R15)
	   )
	{
	  t_regset tmp2;
	  RegsetSetDup(tmp2,tmp);
	  RegsetSetIntersect(tmp2,condition_bits);
	  if(RegsetIsEmpty(tmp2))
	  {
#ifdef DEBUG_KILL_USELESS
	    if (total_count < diablosupport_options.debugcounter)
#endif
	    {
#ifdef DEBUG_KILL_USELESS
	      total_count++;
	      VERBOSE(0,("Un-S'ing @I\n",i_ins));
#endif
	      ARM_INS_SET_FLAGS(ai_ins,   ARM_INS_FLAGS(ai_ins) & ~FL_S);
	      INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(ai_ins));
	      INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(ai_ins));
              kcountuncond++;
	    }
	  }
	}
      }

      if (RegsetIsEmpty(tmp)
          && !InsHasSideEffect(i_ins)
#ifdef DEBUG_KILL_USELESS
          && total_count < diablosupport_options.debugcounter
#endif
          )
      {
#ifdef DEBUG_KILL_USELESS
        VERBOSE(0,("-- Killing @I in @ieB\n",i_ins,INS_BBL(i_ins)));
        total_count++;
#endif
        LogKilledInstruction(INS_OLD_ADDRESS(i_ins));
        InsKill(i_ins);
        kcount++;
      }
      else
      {
	if (!INS_IS_CONDITIONAL(i_ins))
	  RegsetSetDiff(live,def);
	RegsetSetUnion(live,use);
      }
    }
  }

  RegsetFree(live);
  RegsetFree(use);
  RegsetFree(def);
  RegsetFree(tmp);

  STATUS(STOP,("Kill Useless Instructions (%d killed, %d no longer set flags)",kcount,kcountuncond));
  return kcount > 0;
}

t_bool ArmKillUselessInstructions (t_cfg *cfg)
{
  t_bool change = FALSE;

  STATUS (START, ("Killing useless instructions"));
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeUselessRegisters (cfg);

  while (ArmOptKillUseless (cfg))
  {
    change = TRUE;
    CfgComputeSavedChangedRegisters (cfg);
    CfgComputeUselessRegisters (cfg);
  }
  STATUS (STOP, ("Killing useless instructions"));
  return change;
}

void OptimizeStackLoadAndStores(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_arm_ins *i_ins, *tmp;

  CFG_FOREACH_BBL(cfg,i_bbl)
    BBL_FOREACH_ARM_INS_SAFE(i_bbl,i_ins,tmp)
    {
      if (ARM_INS_OPCODE(i_ins)==ARM_STM &&
	  ARM_INS_REGB(i_ins)==ARM_REG_R13 &&
	  ARM_INS_INEXT(i_ins) &&
	  ARM_INS_OPCODE(ARM_INS_INEXT(i_ins))==ARM_STM &&
	  ARM_INS_REGB(ARM_INS_INEXT(i_ins))==ARM_REG_R13 &&
	  !(ARM_INS_IMMEDIATE(i_ins)&ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins))))
      {
	int i;
	int num_regs = 0;

	for (i=0; i<16; i++)
	  if (ARM_INS_IMMEDIATE(i_ins) & (1 << i))
	    num_regs++;
	if (num_regs>=5)
	{
	  ARM_INS_SET_REGS_USE(i_ins,
	      RegsetUnion (ARM_INS_REGS_USE(i_ins),
		ARM_INS_REGS_USE(ARM_INS_INEXT(i_ins))));
	  ARM_INS_SET_IMMEDIATE(i_ins,
	      ARM_INS_IMMEDIATE(i_ins)|ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)));
	  tmp = ARM_INS_INEXT(tmp);
	  ArmInsKill(ARM_INS_INEXT(i_ins));
	}
      }

      if (ARM_INS_OPCODE(i_ins)==ARM_LDM &&
	  ARM_INS_REGB(i_ins)==ARM_REG_R13 &&
	  ARM_INS_INEXT(i_ins) &&
	  ARM_INS_OPCODE(ARM_INS_INEXT(i_ins))==ARM_LDM &&
	  ARM_INS_REGB(ARM_INS_INEXT(i_ins))==ARM_REG_R13 &&
	  !(ARM_INS_IMMEDIATE(i_ins)&ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins))))
      {
	int i;
	int num_regs = 0;

	for (i=0; i<16; i++)
	  if (ARM_INS_IMMEDIATE(i_ins) & (1 << i))
	    num_regs++;
	if (num_regs>=5)
	{
	  ARM_INS_SET_REGS_DEF(i_ins,
	      RegsetUnion(ARM_INS_REGS_DEF(i_ins),
		ARM_INS_REGS_DEF(ARM_INS_INEXT(i_ins))));
	  ARM_INS_SET_IMMEDIATE(i_ins,
	      ARM_INS_IMMEDIATE(i_ins)|ARM_INS_IMMEDIATE(ARM_INS_INEXT(i_ins)));
	  tmp = ARM_INS_INEXT(tmp);
	  ArmInsKill(ARM_INS_INEXT(i_ins));
	}
      }
    }
}

void OptimizeSingleThreaded(t_cfg * cfg)
{
	t_bbl *i_bbl;
	t_arm_ins *i_ins, *tmp, *new_ins;

	t_uint32 ldrex_count = 0;
	t_uint32 strex_count = 0;
	t_uint32 hint_count = 0;

	CFG_FOREACH_BBL(cfg, i_bbl)
	{
		BBL_FOREACH_ARM_INS_SAFE(i_bbl, i_ins, tmp)
		{
			switch(ARM_INS_OPCODE(i_ins))
			{
			case ARM_LDREX:
			case ARM_LDREXB:
			case ARM_LDREXH:
			case ARM_LDREXD:
				VERBOSE(2, (" replacing multi-thread instruction @I", i_ins));

				switch(ARM_INS_OPCODE(i_ins))
				{
				case ARM_LDREX:
					ARM_INS_SET_OPCODE(i_ins, ARM_LDR);
					break;

				case ARM_LDREXB:
					ARM_INS_SET_OPCODE(i_ins, ARM_LDRB);
					break;

				case ARM_LDREXH:
					ARM_INS_SET_OPCODE(i_ins, ARM_LDRH);
					break;

				case ARM_LDREXD:
					ARM_INS_SET_OPCODE(i_ins, ARM_LDRD);
					break;

				default:
					FATAL(("implement opcode translation for @I", i_ins));
				}

        if (!(ARM_INS_FLAGS(i_ins) & FL_THUMB)
            && (ARM_INS_OPCODE(i_ins)==ARM_LDREX))
				  ARM_INS_SET_IMMEDIATE(i_ins, 0);

				ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_IMMED);

				VERBOSE(2, ("   with @I", i_ins));

				ldrex_count++;
				break;

			case ARM_STREX:
			case ARM_STREXB:
			case ARM_STREXD:
			case ARM_STREXH:
				VERBOSE(2, (" replacing multi-thread instruction @I", i_ins));

				switch(ARM_INS_OPCODE(i_ins))
				{
				case ARM_STREX:
					ARM_INS_SET_OPCODE(i_ins, ARM_STR);
					break;

				case ARM_STREXB:
					ARM_INS_SET_OPCODE(i_ins, ARM_STRB);
					break;

				case ARM_STREXH:
					ARM_INS_SET_OPCODE(i_ins, ARM_STRH);
					break;

				case ARM_STREXD:
					ARM_INS_SET_OPCODE(i_ins, ARM_STRD);
					break;

				default:
					FATAL(("implement opcode translation for @I", i_ins));
				}

        if (!(ARM_INS_FLAGS(i_ins) & FL_THUMB)
            && (ARM_INS_OPCODE(i_ins)==ARM_LDREX))
          ARM_INS_SET_IMMEDIATE(i_ins, 0);

				ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_IMMED);

				/* STREX* instructions have an important side-effect: if the store succeeds, a 1 is written to the destination register
				 * In single-threaded applications, the store will always succeed, so we have to add an extra "MOV Rd, 1" instruction.
				 */
				new_ins = ArmInsNewForBbl(ARM_INS_BBL(i_ins));
				ARM_INS_SET_CSIZE(new_ins, AddressNew32(4));
				ARM_INS_SET_OLD_SIZE(new_ins, AddressNew32(0));

				ArmInsMakeMov(new_ins, ARM_INS_REGC(i_ins), ARM_REG_NONE, 1, ARM_INS_CONDITION(i_ins));
				ArmInsInsertAfter(new_ins,i_ins);

				ARM_INS_SET_REGC(i_ins, ARM_REG_NONE);

				VERBOSE(2, ("   with @I (added an extra instruction @I)", i_ins, new_ins));

				strex_count++;
				break;

			case ARM_WFE:
			case ARM_SEV:
			case ARM_YIELD:
			case ARM_CLREX:
				VERBOSE(2, (" replacing multi-thread instruction @I", i_ins));

				ArmInsMakeNoop(i_ins);

				VERBOSE(2, ("   with @I", i_ins));

				hint_count++;
				break;

			default:
				/* skip other instructions by default */
				break;
			}

			ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
			ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
		}
	}

	VERBOSE(1, ("replaced %d LDREX* with LDR* instructions", ldrex_count));
	VERBOSE(1, ("replaced %d STREX* with STR* instructions", strex_count));
	VERBOSE(1, ("replaced %d hint instructions with NOP's", hint_count));
}

void ArmInsCanMoveDown(t_arm_ins * ins, t_bool * ret)
{
  /* assume the owning BBL of this instruction only has one outgoing
   * edge, which is normally ensured by CfgMoveInsDown in diabloanopt.c. */

  /* for the time being, don't try to move instructions from Thumb to ARM
   * and vice versa. When you do want to do this, care has to be taken as
   * to convert the Thumb instruction to its ARM equivalent. */

  t_bbl * frombbl = ARM_INS_BBL(ins);
  ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(frombbl)), ("expected BBL with only one successor, got @eiB", frombbl));

  if (ArmInsChangesInstructionSet(T_ARM_INS(BBL_INS_LAST(frombbl))))
    *ret = FALSE;
}

void ArmInsCanMoveUp(t_arm_ins * ins, t_bool * ret)
{
  /* analoguous to ArmInsCanMoveDown */

  t_bbl * frombbl = ARM_INS_BBL(ins);
  ASSERT(!CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(frombbl)), ("expected BBL with only one predecessor, got @eiB", frombbl));

  if (ArmInsChangesInstructionSet(T_ARM_INS(BBL_INS_LAST(frombbl))))
    *ret = FALSE;
}

void ArmBblCanMerge(t_bbl * a, t_bbl * b, t_bool * ret)
{
  ASSERT(a && b, ("kaboom!"));

  t_bool a_thumb = (BBL_INS_FIRST(a)) ? (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(a))) & FL_THUMB) : FALSE;
  t_bool b_thumb = (BBL_INS_FIRST(b)) ? (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(b))) & FL_THUMB) : FALSE;

  if (a_thumb ^ b_thumb)
    *ret = FALSE;
}

void ArmBranchEliminationDo(t_bbl * a, t_bbl * b, t_bool * ret)
{
  t_arm_ins * first_ins = ArmFindFirstIns(b);
  if (ArmInsChangesInstructionSet(first_ins))
  {
    /* Prevent the propagation of branch instructions to __from_thumb stubs.
     * We do this by looking at the first instruction to be executed after the
     * branch instruction.
     * Note that it is not simply the first instruction of the BBL the branch
     * points to, because empty BBL's can still exist at this point in the Diablo-flow. */
    VERBOSE(3, ("not eliminating branch @eiB to @eiB because the first following instruction changes the instruction set (@I)", a, b, first_ins));
    *ret = FALSE;
    return;
  }
  else if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(a))) & FL_THUMB)
  {
    /* When we want to forward a Thumb branch, care should be taken that branch instructions
     * created to compensate the small range for CB(N)Z-instructions don't get propageted.
     * In order to prevent this, we look at every possible incoming path to the BBL in which
     * the branch we want to propagate is located. If there is one path that ends in a CB(N)Z
     * instruction, the branch was probably created to compensate the small range.
     *
     * NOTE: Care should be taken when looking for the last instruction of an incoming path.
     *       Empty BBL's can still exist at this point in the Diablo-flow. */

    t_bool cont = TRUE;
    t_bbl * i_bbl = a;
    t_cfg_edge * i_edge = NULL;
    t_bbl * prev_bbl = NULL;
    t_bool pred_cbz = FALSE;
    t_cfg_edge * ft_edge = NULL;
    t_arm_ins * tbb_ins = NULL;

    /* Firstly, we walk as far back as we can, following empty BBL's
     * until we find a BBL with no empty predecessor. */
    while (cont)
    {
      cont = FALSE;

      BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
      {
        /* In theory, every bbl can at most have only one empty predecessor,
         * and since an empty BBL can't contain a branch instruction, the
         * one and only outgoing edge should be a fallthrough edge. Thus,
         * when we look at a BBL, the only case where it can have an empty
         * predecessor is when there is an incoming edge of type ET_FALLTHROUGH
         * or ET_IPFALLTHRU. */

        /* skip hell edges */
        if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) continue;

        /* skip return edges: CB(N)Z instructions will never be used to
         * return from a called function */
        if (CFG_EDGE_CAT(i_edge) == ET_RETURN) continue;

        if ((CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH
            || CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU))
        {
          /* this is a fallthrough edge, check if it is an empty BBL */
          if (!BBL_INS_FIRST(CFG_EDGE_HEAD(i_edge)))
          {
            /* if it is, continue searching up the graph */
            prev_bbl = i_bbl;
            i_bbl = CFG_EDGE_HEAD(i_edge);

            cont = TRUE;
            ASSERT(!CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(i_bbl)), ("empty BBL is expected to have only one outgoing edge @eiB", i_bbl));
          }

          /* this is not an empty BBL */
          break;
        }
        else
        {
          /* not a fallthrough edge, assume the predecessor is not empty */
          ASSERT(BBL_INS_FIRST(CFG_EDGE_HEAD(i_edge)), ("non-empty BBL expected for non-fallthrough edge @E: @eiB", i_edge, CFG_EDGE_HEAD(i_edge)));
        }
      }
    }

    while (prev_bbl)
    {
      /* prev_bbl is the last possible empty BBL;
       * walk over its incoming edges */
      BBL_FOREACH_PRED_EDGE(prev_bbl, i_edge)
      {
        if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) continue;
        if (CFG_EDGE_CAT(i_edge) == ET_RETURN) continue;

        /* check whether the last instruction of this predecessor is a CB(N)Z instruction or not */
        if (BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))
            && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))))==ARM_T2CBZ
            || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))))==ARM_T2CBNZ))
        {
          pred_cbz = TRUE;
          break;
        }
      }

      if (!BBL_INS_FIRST(CFG_EDGE_TAIL(BBL_SUCC_FIRST(prev_bbl))))
      {
        /* the successor is empty */
        prev_bbl = CFG_EDGE_TAIL(BBL_SUCC_FIRST(prev_bbl));
      }
      else
      {
        /* the successor is non-empty;
         * this should be the start BBL (of which we want to forward the branch) */
        ASSERT(CFG_EDGE_TAIL(BBL_SUCC_FIRST(prev_bbl)), ("KABLAMO @eiB", CFG_EDGE_TAIL(BBL_SUCC_FIRST(prev_bbl))));
        prev_bbl = NULL;
      }
    }

    /* finally iterate over every predecessor of the to-be-forwarded branch */
    BBL_FOREACH_PRED_EDGE(a, i_edge)
    {
      if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) continue;
      if (CFG_EDGE_CAT(i_edge) == ET_RETURN) continue;

      /* check whether the last instruction of every predecessor is a CB(N)Z instruction or not */
      if (BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))
          && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))))==ARM_T2CBZ
          || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge))))==ARM_T2CBNZ))
      {
        pred_cbz = TRUE;
        break;
      }
    }

    if (pred_cbz)
    {
      VERBOSE(3, ("not eliminating branch @eiB to @eiB because an incoming path ends in a CB(N)Z instruction via edge @E", a, b, i_edge));
      *ret = FALSE;
      return;
    }

    /* Prevent the elimination of branches in TBB chains */
    i_bbl = a;
    tbb_ins = NULL;

    do
    {
      ft_edge = NULL;

      BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
      {
        if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH
            || CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU
            || CFG_EDGE_CAT(i_edge) == ET_RETURN)
        {
          /* the BBL has an incoming fallthrough edge */
          ft_edge = i_edge;
        }
        else if ((CFG_EDGE_CAT(i_edge)==ET_SWITCH || CFG_EDGE_CAT(i_edge)==ET_IPSWITCH)
                && (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)))) == ARM_T2TBB || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)))) == ARM_T2TBH))
        {
          /* one of the incoming edges originates from a TBB instruction */
          tbb_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)));
        }
      }

      if (ft_edge && !tbb_ins)
      {
        /* The branch we want to eliminate has an incoming FALLTHROUGH/RETURN edge. */

        /* get the corresponding CALL edge for a RETURN edge */
        if (CFG_EDGE_CAT(ft_edge) == ET_RETURN)
          ft_edge = CFG_EDGE_CORR(ft_edge);
        ASSERT(ft_edge, ("BOOOOOM"));

        /* Follow this edge upstream */
        i_bbl = CFG_EDGE_HEAD(ft_edge);
      }
    } while (ft_edge && !tbb_ins);

    if (tbb_ins)
    {
      VERBOSE(3, ("not eliminating branch @eiB because it is on a fallthrough path originating from a TBB/TBH instruction @I", a, tbb_ins));
      *ret = FALSE;
      return;
    }
  }
  else if (ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(a))) & IF_SWITCHJUMP) {
    *ret = FALSE;
  }

  VERBOSE(3, ("eliminating @eiB to @eiB", a, b));
}

void ArmBranchForwardingDo(t_cfg_edge * edge, t_bool * ret)
{
  if ((CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
      && (!BBL_INS_LAST(CFG_EDGE_HEAD(edge)) || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge))))==ARM_T2CBZ || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge))))==ARM_T2CBNZ))
  {
    VERBOSE(3, ("not forwarding @E", edge));
    *ret = FALSE;
  }
}

static bool FlipBranchesCheckFallthroughLoop(t_bbl *from)
{
	/* if the FROM is marked, this is a fallthrough loop! */
	if (BblIsMarked(from))
		return true;

	/* mark this BBL as visited */
	BblMark(from);

	/* lookup the fallthrough edge, if any */
	t_cfg_edge *ft_edge = ArmGetFallThroughEdge(from);

	/* if no fallthrough destination is found, no loop is possible */
	if (!ft_edge)
		return false;

	return FlipBranchesCheckFallthroughLoop(CFG_EDGE_TAIL(ft_edge));
}

t_bool CanModifyBranch(t_bbl *bbl) {
	if (BBL_IS_HELL(bbl))
		return FALSE;

	if (!BBL_INS_LAST(bbl))
		return FALSE;

	/* at least one instruction exists */
	t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(bbl));

	/* do we care about this instruction? */
	if (ARM_INS_TYPE(last) != IT_BRANCH)
		/* don't care about this */
		return FALSE;

	if (ARM_INS_OPCODE(last) != ARM_B)
		return FALSE;

	return TRUE;
}

t_bool CanModifyBranchConditional(t_bbl *bbl) {
	if (!CanModifyBranch(bbl))
		return FALSE;

	t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(bbl));
	if (!(ARM_INS_IS_CONDITIONAL(last)
				&& ARM_INS_CONDITION(last)))
		/* don't care about this */
		return FALSE;

	return TRUE;
}

t_bool CanModifyJumpEdge(t_cfg_edge *e) {
	t_bbl *bbl = CFG_EDGE_HEAD(e);

	/* BBL jumping to itself */
	if (CFG_EDGE_TAIL(e) == bbl)
		return FALSE;

	/* take into account possible conditional branch-and-links */
	if (CFG_EDGE_CAT(e) == ET_CALL)
		return FALSE;

	ASSERT(CFG_EDGE_CAT(e) == ET_JUMP
					|| CFG_EDGE_CAT(e) == ET_IPJUMP, ("did not expect this type of edge @E: @eiB", e, bbl));

	/* jump destination is the exit block of a function, no transformation can be done! */
	if (CFG_EDGE_TAIL(e) == FunctionGetExitBlock(BBL_FUNCTION(bbl)))
		return FALSE;

	if (!CanModifyJumpEdgeAF(e))
		return FALSE;

	return TRUE;
}

t_bool CanFlipBranch(t_bbl *bbl, t_cfg_edge ** ft_edge, t_cfg_edge ** jump_edge) {
	/* look up the jump and fallthrough edges */
	*jump_edge = NULL;
	*ft_edge = NULL;
	t_cfg_edge *e;
	BBL_FOREACH_SUCC_EDGE(bbl, e)
	{
		if (CfgEdgeIsFallThrough(e))
			*ft_edge = e;
		else
			*jump_edge = e;
	}

	ASSERT(*ft_edge, ("no fallthrough edge found for @eiB", bbl));
	ASSERT(*jump_edge, ("no jump edge found for @eiB", bbl));

	if (!CanModifyJumpEdge(*jump_edge))
		return FALSE;

	return TRUE;
}

void DoUncondBranchToFallthrough(t_bbl *bbl) {
	t_cfg_edge *e = BBL_SUCC_FIRST(bbl);
	t_bool e_was_fake = CfgEdgeIsFake(e);
	t_uint32 exec = CFG_EDGE_EXEC_COUNT(e);

	t_bbl *dest = CFG_EDGE_TAIL(e);

	/* kill existing edges */
	if (CFG_EDGE_CORR(e))
		CfgEdgeKill(CFG_EDGE_CORR(e));
	CfgEdgeKill(e);

	/* recreate fallthrough edge */
	t_cfg_edge *new_ft = CfgEdgeCreate(BBL_CFG(bbl), bbl, dest, ET_FALLTHROUGH);
	if (e_was_fake)
		CfgEdgeMarkFake(new_ft);
	EdgeMakeInterprocedural(new_ft);
	CFG_EDGE_SET_EXEC_COUNT(new_ft, exec);

	/* kill last instruction (the branch instruction) */
	t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(bbl));
	ASSERT(ArmInsIsUnconditionalBranch(last), ("expected unconditional branch @eiB", bbl));

	ArmInsKill(last);
}

void DoFlipBranch(t_bbl *bbl, t_cfg_edge *ft_edge, t_cfg_edge *jump_edge) {
	t_bbl *ft_dest = CFG_EDGE_TAIL(ft_edge);
	t_uint32 ft_exec = CFG_EDGE_EXEC_COUNT(ft_edge);
	if (CFG_EDGE_CORR(ft_edge))
		CfgEdgeKill(CFG_EDGE_CORR(ft_edge));
	t_bool ft_was_fake = CfgEdgeIsFake(ft_edge);
	CfgEdgeKill(ft_edge);

	t_bbl *jump_dest = CFG_EDGE_TAIL(jump_edge);
	t_uint32 jump_exec = CFG_EDGE_EXEC_COUNT(jump_edge);
	if (CFG_EDGE_CORR(jump_edge))
		CfgEdgeKill(CFG_EDGE_CORR(jump_edge));
	t_bool jump_was_fake = CfgEdgeIsFake(jump_edge);
	CfgEdgeKill(jump_edge);

	t_cfg_edge *new_jump = CfgEdgeCreate(BBL_CFG(bbl), bbl, ft_dest, ET_JUMP);
	if (ft_was_fake)
		CfgEdgeMarkFake(new_jump);
	EdgeMakeInterprocedural(new_jump);
	CFG_EDGE_SET_EXEC_COUNT(new_jump, ft_exec);

	t_cfg_edge *new_ft = CfgEdgeCreate(BBL_CFG(bbl), bbl, jump_dest, ET_FALLTHROUGH);
	if (jump_was_fake)
		CfgEdgeMarkFake(new_ft);
	EdgeMakeInterprocedural(new_ft);
	CFG_EDGE_SET_EXEC_COUNT(new_ft, jump_exec);

	/* invert the condition by conveniently inverting the LSbit */
	t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(bbl));
	t_arm_condition_code cond = ARM_INS_CONDITION(last) ^ 0x1;
	ARM_INS_SET_CONDITION(last, cond);
}

//#define DEBUG_FLIP
void ArmPossiblyFlipConditionalBranches(t_cfg *cfg)
{
	if (diabloanoptarm_options.af_fake_fallthrough_condbranch_chance > 0)
		return;

	t_uint32 nr_transormations = 0;
	STATUS(START, ("Branch Flipping"));

	t_bbl *bbl;
	CFG_FOREACH_BBL(cfg, bbl)
	{
		if (!CanModifyBranchConditional(bbl))
			continue;

		t_cfg_edge *jump_edge = NULL;
		t_cfg_edge *ft_edge = NULL;
		if (!CanFlipBranch(bbl, &ft_edge, &jump_edge))
			continue;

		/* make sure we don't introduce any fallthrough loops */
		BblMarkInit();
		BblMark(bbl);
		if (FlipBranchesCheckFallthroughLoop(CFG_EDGE_TAIL(jump_edge))) {
			DEBUG(("FT loop!"));
			continue;
		}

		t_bbl *ft_dest = CFG_EDGE_TAIL(ft_edge);
		if (BBL_NINS(ft_dest) == 1
				&& ARM_INS_TYPE(T_ARM_INS(BBL_INS_LAST(ft_dest))) == IT_BRANCH)
		{
			t_bbl *jump_dest = CFG_EDGE_TAIL(jump_edge);
			t_cfg_edge *e;

			/* The jump destination will become the fallthrough destination.
			 * Make sure that this new fallthrough destination does not have an incoming (implicit) fallthrough edge already! */
			bool has_incoming_ft = false;
			BBL_FOREACH_PRED_EDGE(jump_dest, e)
				if (CfgEdgeIsFallThrough(e)
						|| CFG_EDGE_CAT(e) == ET_RETURN)
				{
					has_incoming_ft = true;
					break;
				}
			if (has_incoming_ft)
				continue;

			//DEBUG(("flip %u: @eiB", nr_transormations, bbl));
			DoFlipBranch(bbl, ft_edge, jump_edge);
			//DEBUG(("   -----> @eiB", bbl));
			nr_transormations++;
		}

#ifdef DEBUG_FLIP
		if (diablosupport_options.debugcounter <= nr_transormations) break;
#endif
	}

	STATUS(STOP, ("Branch Flipping (%u flips)", nr_transormations));
}