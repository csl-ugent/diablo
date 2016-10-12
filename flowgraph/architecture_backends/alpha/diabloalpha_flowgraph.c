#include <diabloalpha.h>
#include "diabloalpha_flowgraph.h"

static void AlphaRemoveBranchRelocs(t_object *obj);
static void AlphaFindBblLeaders(t_object *obj);
static t_uint32 AlphaAddBasicBlockEdges(t_object *obj);
static t_uint32 AlphaRemoveGPRecomputations(t_object *obj);
static t_uint32 AlphaRemoveNops(t_object *obj);

void AlphaFlowgraph(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);

  t_uint32 ngp_kills;
  t_uint32 nnop_kills;
  t_uint32 njmp_mods;

  STATUS(START,("Removing branch and call relocations"));
  AlphaRemoveBranchRelocs(obj);
  STATUS(STOP,("Removing branch and call relocations"));

  STATUS(START,("Finding basic block leaders"));
  AlphaFindBblLeaders(obj);
  STATUS(STOP,("Finding basic block leaders"));

  STATUS(START,("Creating basic blocks"));
  CfgCreateBasicBlocks(obj);
  STATUS(STOP,("Creating basic blocks"));

  STATUS(START,("Adding edges between basic blocks"));
  AlphaAddBasicBlockEdges(obj);
  STATUS(STOP,("Adding edges between basic blocks"));

  if(diabloalpha_options.do_gpreset_removal)
  {
    STATUS(START,("Removing useless GP-Recomputations"));
    ngp_kills = AlphaRemoveGPRecomputations(obj);
    VERBOSE(0,("Removed %d useless $gp computations", ngp_kills));
    STATUS(STOP,("Removing useless GP-Recomputations"));
  }

  if(diabloalpha_options.do_nop_removal)
  {
    STATUS(START,("Removing safe NOPS"));	
    nnop_kills = AlphaRemoveNops(obj);
    VERBOSE(0,("Removed %d NOPS", nnop_kills));
    STATUS(START,("Removing safe NOPS"));	
  }

  if(diabloalpha_options.do_branch_conversion)
  {
    STATUS(START,("Converting indirect jumps to direct jumps"));	
    njmp_mods = AlphaChangeJSRs(cfg);
    VERBOSE(0,("Changed %d indirect jumps to direct ones", njmp_mods));
    STATUS(STOP,("Converting indirect jumps to direct jumps"));	
  }

}

/* We can safely remove normal conditional branches/calls
 * relocation information since we can describe these 
 * relocations perfectly just by using the encoded instruction
 * and the edge */

static void AlphaRemoveBranchRelocs(t_object *obj)
{
  t_alpha_ins * ins;
  t_reloc * reloc, * tmp;
  t_relocatable * from;


  OBJECT_FOREACH_RELOC_SAFE(obj, reloc, tmp)
  {
    from = RELOC_FROM(reloc);
    if(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_INS) 
    {
      ins = T_ALPHA_INS(from);
      if(ALPHA_INS_FORMAT(ins) == ALPHA_ITYPE_BR) 
      {
        RelocTableRemoveReloc(RELOC_TABLE(reloc), reloc);
      }
    }
  }

}

void AlphaMakeAddressProducers(t_cfg *cfg)
{
}

/* Find all the Basic Block entry points.
 * As far as I have figured out, on Alpha there will be the following cases:
 *
 * 1: Branches.
 * 2: Calls to the PAL.
 * 3: Relocations.
 */

static void AlphaFindBblLeaders(t_object *obj)
{
  t_section *code;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_alpha_ins * ins;        /* Current instruction we are checking */
    t_alpha_ins * target_ins; /* Target instruction of a Branch */
    t_alpha_ins * next_ins;    /* The following instruction */
    t_address target_addr;    /* The address of the target instruction */
    t_address tmp;

    /* First instruction is always a BBL Leader */

    /* For each instruction in this code section, check if it is
     * a basic block leader */


    SECTION_FOREACH_ALPHA_INS(code, ins) 
    {

      if( ! ins ) break;

      next_ins = T_ALPHA_INS(ALPHA_INS_INEXT(ins));

      /* Case #1: Instruction is a direct branch/jump */
      if(next_ins && ALPHA_INS_TYPE(ins) == IT_BRANCH) 
      {
        /* Instruction after branch is a BBL Leader */
        ALPHA_MARK_AS_BBLL(next_ins);

        /* Compute the address of the branch target, the instruction at
         * this address is a BBL Leader.  Here only mark standard branch
         * targets, i.e. instructions such as BEQ, BFEQ, BGE, BNE etc.
         * We do not care about procedure calls and or jumps (jmp, jsr etc)
         * at this stage.
         */

        if( alpha_opcode_table[ALPHA_INS_OPCODE( ins )].type != ALPHA_ITYPE_MBR ) {

          /* The address of the instruction that starts a BBL */

          tmp = AddressNew64((ALPHA_INS_IMMEDIATE( ins ) << 2) + 4 );

          target_addr = AddressAdd(tmp, ALPHA_INS_CADDRESS ( ins ));
          target_ins = T_ALPHA_INS(ObjectGetInsByAddress(obj, target_addr));

          if( target_ins )
          {
            ALPHA_MARK_AS_BBLL( target_ins );
          }
          else 
          {
            FATAL(("CFG: Destination instruction not found\n"));
          }
        }

      } /* End if(IT_BRANCH) */

      /* Case 2, entries to PAL */
      if(next_ins && ALPHA_INS_TYPE(ins) == IT_SWI) 
      {
        /* The first instruction after a call to the PAL is a leader */
        ALPHA_MARK_AS_BBLL( next_ins );
      } /* End type PAL */

      /* Relocatables, a leader is ref'd by a relocation */
      if(ALPHA_INS_REFED_BY(ins)) 
      {
        ALPHA_MARK_AS_BBLL(ins);
      }
    }  /* End FOREACH_INS() */

    /* Also mark each data block as a bbl leader */

  }

}  /* End function */


/* Add basic block edges between nodes */
static t_uint32 AlphaAddBasicBlockEdges(t_object *obj)
{
  t_alpha_ins * ins ,* ins_first, * ins_last, * it;
  t_cfg * cfg = OBJECT_CFG(obj);
  t_uint32 nedges = 0;
  t_bbl * cur_bbl, * bbl_ft;
  t_ins * tmp;

  t_section *section;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, section, i)
  {
    SECTION_FOREACH_INS(section, tmp)
    {
      ins = T_ALPHA_INS(tmp);

      /* Skip to the next basic block */
      if(! ALPHA_IS_BBLL(ins) ) continue;

      ins_first = T_ALPHA_INS(ins);
      cur_bbl = ALPHA_INS_BBL(ins_first);
      ins_last = ins_first;
      it = ALPHA_INS_INEXT(ins_first);

      /* Move through the basic block */
      for(; it && (! ALPHA_IS_BBLL(it)); it = ALPHA_INS_INEXT(it)) 
        ins_last = it;

      /* Now ins_last points to the last instruction in the current bbl
       * and 'it' points to the first instruction in the fallthrough bbl */

      bbl_ft = it ? ALPHA_INS_BBL(it) : NULL;

      nedges += AlphaAddEdgeForRefedBbls(cfg, cur_bbl, ins_last);

      switch(ALPHA_INS_TYPE(ins_last)) {
        case IT_BRANCH: 
          nedges += AlphaAddEdgeForBranches(cfg, cur_bbl, bbl_ft, ins_last);
          break;
        case IT_SWI: 
          /* It would be stupid to add an edge after a PAL_halt instruction */
          if(ALPHA_INS_OPCODE(ins_last) != ALPHA_PAL_halt)
            nedges += AlphaAddEdgeForSwi(cfg, cur_bbl, bbl_ft, ins_last);
          break;
        default: 
          if (bbl_ft) CfgEdgeCreate(cfg, cur_bbl, bbl_ft, ET_FALLTHROUGH);
          break;
      }
    }
  }

  return nedges;


} /* End function */

t_uint32 
AlphaAddEdgeForRefedBbls(t_cfg * cfg,t_bbl * cbbl, t_alpha_ins * last)
{
  t_uint32 nedges = 0;
  t_reloc_ref * rref;
  t_bbl * bbl = cbbl;
  t_cfg_edge * edge, * found;
	t_bool add_edge;

  for(rref = BBL_REFED_BY(bbl); rref; rref = RELOC_REF_NEXT(rref)) 
  {
    t_reloc * reloc = RELOC_REF_RELOC(rref);
		add_edge = FALSE;

    if( RELOC_HELL(reloc) ) add_edge = TRUE;
		else {
			if(RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(reloc)) == RT_SUBSECTION || \
				 (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(reloc)) == RT_INS && \
				  ALPHA_INS_TYPE(T_ALPHA_INS(RELOC_FROM(reloc))) != IT_BRANCH))
				add_edge = TRUE;
		}

		if(add_edge) 
		{

     	found = NULL;
			BBL_FOREACH_PRED_EDGE(bbl, edge) {
        if(CFG_EDGE_HEAD(edge) == CFG_HELL_NODE(cfg) && \
					CFG_EDGE_CAT(edge) == ET_CALL) 
					found = edge;
					break;
      }

   	  if(found) {
    	 /* There is an edge so just adjust counts */
   	   RELOC_SET_EDGE(reloc, found);
       CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found) + 1);
			 ASSERT(CFG_EDGE_CORR(found),("Call edge @E does not have a corresponding edge!",found));
       CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(found), \
       CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(found)) + 1);
       }
       else {
         RELOC_SET_EDGE(reloc, 
          CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), bbl, NULL, NULL));
          nedges++;
      }
		}
	}

	return nedges;

}

t_uint32 AlphaAddEdgeForSwi(t_cfg * cfg, t_bbl * bbl, t_bbl * ft_bbl, t_alpha_ins * ins) 
{

  t_uint32 nedges = 0;

  if(ft_bbl) {
    CfgEdgeCreate(cfg, bbl, ft_bbl, ET_FALLTHROUGH);
    nedges++;
  }
  CfgEdgeCreateSwi(cfg, bbl, ft_bbl);
  nedges++;

  return nedges;

}


t_uint32 AlphaAddEdgeForBranches(t_cfg * cfg, t_bbl * bbl, t_bbl * ft_bbl, t_alpha_ins * ins) 
{
  t_uint32 nedges = 0;
  t_address disp, jt_addr;
  t_alpha_ins * jt_ins = NULL; /* Instruction at the target address (jt_addr) */
  t_bbl * jsr_target;

	if(ALPHA_INS_FORMAT(ins) == ALPHA_ITYPE_BR)
	{
  	disp = AddressNew64(((ALPHA_INS_IMMEDIATE(ins) << 2 ) + 4 ) );
  	jt_addr = AddressAdd(ALPHA_INS_CADDRESS(ins), disp);
  	jt_ins = T_ALPHA_INS(ObjectGetInsByAddress(CFG_OBJECT(cfg), jt_addr));
	}
  
  switch( ALPHA_INS_OPCODE(ins) ) {

      /* Alpha BR instruction is an unconditional branch, therefore we
       * add ONLY a jump edge */
	
     case ALPHA_BR:
       if(jt_ins) {
         CfgEdgeCreate(cfg, bbl, ALPHA_INS_BBL(jt_ins), ET_JUMP);
      }
      else {
      	FATAL(("BR could not find target"));
			}
      nedges++;
       break;

      /* BSR needs a call edge since we are branching to another 
       * proceedure?  */
     case ALPHA_BSR:
       if(jt_ins) 
			 {
			 	 CfgEdgeCreateCall(cfg, bbl, ALPHA_INS_BBL(jt_ins), ft_bbl, NULL);
			 }
     	 else {
        FATAL(("BSR could not find target"));
       }
      nedges++;
      break;
      
			/* Alpha JMP, no way back.  So add a jump edge to the instructrion
       * or a edge to the hell node */

      case ALPHA_JMP:
        jsr_target = AlphaFindJSRTarget(bbl, ins, cfg);
        if(jsr_target) {
         	CfgEdgeCreate(cfg, bbl, jsr_target, ET_JUMP);
				}
        else {
         CfgEdgeCreate(cfg, bbl, CFG_HELL_NODE(cfg), ET_JUMP);
				}
        nedges++;
        break;

      case ALPHA_JSR:
        jsr_target = AlphaFindJSRTarget(bbl, ins, cfg);
        if(jsr_target) {
          CfgEdgeCreateCall(cfg, bbl, jsr_target, ft_bbl, NULL);
					/* Could turn this into a BR instruction? */
				}
        else
          CfgEdgeCreateCall(cfg, bbl, CFG_HELL_NODE(cfg), ft_bbl, NULL);
        nedges++;
        break;

      case ALPHA_RET:
        CfgEdgeCreate(cfg,  bbl,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
        nedges++;
        break;

      /* The following instruction types are conditional branches
       * and hence have a jump AND a fall through edge */

     case ALPHA_BEQ:
     case ALPHA_BGE:
     case ALPHA_BGT:
     case ALPHA_BLBC:
     case ALPHA_BLBS:
     case ALPHA_BLE:
     case ALPHA_BLT:
     case ALPHA_BNE:
     case ALPHA_FBEQ:
     case ALPHA_FBGE:
     case ALPHA_FBGT:
     case ALPHA_FBLE:
     case ALPHA_FBLT:
     case ALPHA_FBNE:
      CfgEdgeCreate(cfg, bbl, ft_bbl, ET_FALLTHROUGH);
			CfgEdgeCreate(cfg, bbl, ALPHA_INS_BBL(jt_ins), ET_JUMP);
			nedges+=2;
			break;

    default:
      FATAL(("Attempted to add CFG edge for unknown branch ins %s", alpha_opcode_table[ALPHA_INS_OPCODE(ins)].desc));

  }    

	/* The edges describe the relocation, so we need not have the relocations 
	 * lying around anymore */

  return nedges;
}

/* COnvert JSR and JMP instructions into BSR and BR instructions respectivly 
 * if we know the target of the indirect jump */

t_uint32 AlphaChangeJSRs(t_cfg * cfg)
{
  t_uint32 nchange = 0;
  t_bbl * bbl;
  t_cfg_edge * edge;
  t_alpha_ins * jump;
  t_bbl * target_bbl;

  CFG_FOREACH_BBL(cfg, bbl) {
    jump = T_ALPHA_INS(BBL_INS_LAST(bbl));
    /* convert to BR if possible */

    /* Empty bbl? */
    if(! jump ) continue;

    if(ALPHA_INS_OPCODE(jump) == ALPHA_JMP) 
    {
      edge = NULL;
      BBL_FOREACH_SUCC_EDGE(bbl, edge)
      {
        if(CFG_EDGE_CAT(edge) == ET_JUMP ||
            CFG_EDGE_CAT(edge) == ET_IPJUMP) break;
      }
      ASSERT(edge, ("No jump edge from @ieB", bbl));

      target_bbl = CFG_EDGE_TAIL(edge);
      if(target_bbl && target_bbl != CFG_HELL_NODE(cfg))
      {
        /* Change instruction: GO */
        AlphaInsMakeBr(jump, target_bbl);
        nchange++;
      }
    }
    /* Convert to BSR if possible */
    else if(ALPHA_INS_OPCODE(jump) == ALPHA_JSR) 
    {
      edge = NULL;

      BBL_FOREACH_SUCC_EDGE(bbl, edge)
      {
        if(CFG_EDGE_CAT(edge) == ET_CALL) break;
      }
      ASSERT(edge, ("No call edge from @ieB", bbl));

      target_bbl = CFG_EDGE_TAIL(edge);

      if(target_bbl && target_bbl != CFG_HELL_NODE(cfg))
      {
        /* change jump to a BSR instruction */	
        AlphaInsMakeBsr(jump, target_bbl);
        nchange++;
      }

    }
  }

  return nchange;

}

/* Mainly borrowed from the old alpha port :\ */

static t_uint32 AlphaRemoveNops(t_object *obj)
{
  t_uint32 nkills = 0;
  t_bbl * bbl;
  t_ins * ins;
  t_alpha_ins * ains;

  CFG_FOREACH_BBL(OBJECT_CFG(obj), bbl) {
    BBL_FOREACH_INS(bbl, ins) 
    {
      ains = T_ALPHA_INS(ins);
      if(AlphaInsIsNoop(ains) && ! ALPHA_INS_REFERS_TO(ains) && ! AlphaInsHasSideEffect(ins))
      {
        AlphaInsKill(ains);
        nkills++;
      }
    }
  }
  return nkills;
}


t_bbl * AlphaFindJSRTarget(t_bbl * bb, t_alpha_ins * last_ins, t_cfg * cfg)
{
  t_ins * ins;
  t_alpha_ins * ains;
  t_regset regs = RegsetNew();
  t_bbl * bbl = bb;
	t_reloc * reloc;
	t_section * subsec;
	t_alpha_ins * target;

  /* Add the register which a jsr read from into this set */
  RegsetSetAddReg(regs, ALPHA_INS_REGB(T_ALPHA_INS(BBL_INS_LAST(bb))));

  prev_block:
  BBL_FOREACH_INS_R(bbl, ins)
  {
    if( ins == BBL_INS_LAST(bbl) ) continue; /* The actual jsr */
    if( RegsetIsSubset(INS_REGS_DEF(ins), regs) ) break; /* RegB defined */
  }

  /* If we made it to the first instruction (and another) 
   * we need to track back through the previous basic blocks */
  if(! ins && BBL_PRED_FIRST(bbl)) 
  {
    if( BBL_PRED_FIRST(bbl) == BBL_PRED_LAST(bbl) ) /* One edge in?? */
    {
      bbl = CFG_EDGE_HEAD(BBL_PRED_FIRST(bbl));
      goto prev_block;
    }
  }

  /* Made it through, test if we have a quadword load from the GAT */

  ains = T_ALPHA_INS(ins);

  if(ains && ALPHA_INS_OPCODE(ains) == ALPHA_LDQ && \
		ALPHA_INS_REGB(ains) == ALPHA_REG_GP)
  {

		reloc = RELOC_REF_RELOC(ALPHA_INS_REFERS_TO(ains));
		subsec = T_SECTION(RELOC_TO_RELOCATABLE(reloc)[0]);
    {
		  t_address * offset = RELOC_TO_RELOCATABLE_OFFSET(reloc);
		  t_uint64 addr = SectionGetData64(subsec,*offset);
		  target = T_ALPHA_INS(ObjectGetInsByAddress(CFG_OBJECT(cfg), AddressNew64(addr)));

		
		  ALPHA_INS_SET_ATTRIB(ains, ALPHA_INS_ATTRIB(ains) & IF_ADDRESS_PRODUCER);

		  return ALPHA_INS_BBL(target);
    }	
	}
	/* Instruction does not produce an address for indirect jumps */

  return NULL;

}

static t_uint32 AlphaRemoveGPRecomputations(t_object *obj)
{
  t_uint32 nkill = 0;
  t_cfg * cfg = OBJECT_CFG(obj);
  t_bbl * bbl = NULL, * bbl_ent1 = NULL, * bbl_ent2 = NULL, * bbl_ent3 = NULL, * bbl_ent4 = NULL;
  t_ins * tmp;
  t_cfg_edge * edge;
	t_alpha_ins * ains;

  bbl_ent1 = CFG_UNIQUE_ENTRY_NODE(cfg);

  BBL_FOREACH_SUCC_EDGE(bbl_ent1, edge) {
    bbl_ent2 = CFG_EDGE_TAIL(edge);
    break;
  }

  BBL_FOREACH_SUCC_EDGE(bbl_ent2, edge) {
    bbl_ent3 = CFG_EDGE_TAIL(edge);
    break;
  }

  BBL_FOREACH_SUCC_EDGE(bbl_ent3, edge) {
    bbl_ent4 = CFG_EDGE_TAIL(edge);
    break;
  }

  CFG_FOREACH_BBL(cfg, bbl) {
    if(bbl != bbl_ent1 && bbl != bbl_ent2 && bbl != bbl_ent3 && bbl != bbl_ent4)
    {
      BBL_FOREACH_INS(bbl, tmp) {
				ains = T_ALPHA_INS(tmp);
        if(ALPHA_INS_OPCODE(ains) == ALPHA_LDAH && 
					 ALPHA_INS_REGD(ains) == ALPHA_REG_GP &&
					 ALPHA_INS_REGB(ains) == ALPHA_REG_PV)
        { 
					t_alpha_ins * lda = ALPHA_INS_INEXT(ains);
					if(lda && ALPHA_INS_OPCODE(lda) == ALPHA_LDA &&
						 ALPHA_INS_REGD(lda) == ALPHA_REG_GP &&
						 ALPHA_INS_REGB(lda) == ALPHA_REG_GP) 
					{
						AlphaInsKill(ains);
						AlphaInsKill(lda);
						nkill += 2;
					}
				}
      }
    }
  
  }

  return nkill;

}

  /* vim: set shiftwidth=2: */
