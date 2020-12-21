/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>
#include <string.h>

static void I386PatchMpnSwitches(t_object *obj);
static t_uint32 I386FindBBLLeaders(t_object * obj);
static t_uint32 I386AddBasicBlockEdges(t_object * obj);

void I386Flowgraph(t_object *obj)
{  
  t_uint32 ret;
  t_cfg *cfg = OBJECT_CFG(obj);

  /* Find the leaders in the instruction list */
  STATUS(START,("Leader detection"));
  ret=I386FindBBLLeaders(obj);
  STATUS(STOP,("Leader detection"));

  /* Create the basic blocks (platform independent) */
  STATUS(START,("Creating Basic Blocks"));
  ret=CfgCreateBasicBlocks(obj);
  STATUS(STOP,("Creating Basic Blocks"));

  /* Create the edges between basic blocks */
  STATUS(START,("Creating Basic Block graph"));
  ret=I386AddBasicBlockEdges(obj);
  STATUS(STOP,("Creating Basic Block graph"));

  STATUS(START,("Removing useless relocations"));
  /* {{{ kill all relocations from the immediate operands of jump and call instructions: 
   * they have no use any more because their information is represented by flow graph edges */
  {
    t_bbl * bbl;
    t_i386_ins * ins;
    CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_FOREACH_I386_INS(bbl,ins)
      {
	if (I386_INS_OPCODE(ins) == I386_CALL || I386_INS_OPCODE(ins) == I386_JMP || I386_INS_OPCODE(ins) == I386_Jcc)
	{
	  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	  {
	    t_reloc_ref * ref;
	    t_bool dynamic = FALSE;

	    for (ref=I386_INS_REFERS_TO(ins); ref!= NULL; ref=RELOC_REF_NEXT(ref))
	    {
	      t_reloc * rel = RELOC_REF_RELOC(ref);
	      t_uint32 i;

	      for (i = 0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
	      {
		t_relocatable * rb=RELOC_TO_RELOCATABLE(rel)[i];
		if (RELOCATABLE_RELOCATABLE_TYPE(rb) == RT_SUBSECTION)
		{
		  t_section * sec = T_SECTION(rb);

		  if (StringPatternMatch("PLTELEM:*", SECTION_NAME(sec)))
		  {
		    dynamic = TRUE;
		    break;
		  }
		}

	      }
	    }

	    if (dynamic) continue;
	    
	    while (I386_INS_REFERS_TO(ins))
	    {
	      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)));
	    }

	    I386_OP_FLAGS(I386_INS_SOURCE1(ins)) &= ~I386_OPFLAG_ISRELOCATED;
	  }
	}
      }
    }
  } /* }}} */
  STATUS(STOP,("Removing useless relocations"));

  /* patch the cfg representation of the __mpn_add_n and 
   * __mpn_sub_n functions of glibc: they use code address computations,
   * which cannot be reliably modelled in a conventional flow graph */
  I386PatchMpnSwitches(obj);
}

/* iteratively disconnect bbls without outgoing edges by getting rid of their incoming edges */
void I386StucknessAnalysis(t_object * obj)
{
  /*{{{*/
  t_section * code = OBJECT_CODE(obj)[0];
  t_cfg * cfg = OBJECT_CFG(SECTION_OBJECT(code));
  t_bbl * bbl;
  t_cfg_edge * edge1, * edge2;
  t_bool change = TRUE;

  while(change)
  {
    change = FALSE;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      
      if (bbl == CFG_UNIQUE_EXIT_NODE(BBL_CFG(bbl)))
	continue;
      if(bbl == CFG_SWI_HELL_NODE(BBL_CFG(bbl)))
	continue;
      if(BBL_SUCC_FIRST(bbl)==NULL)
      {
	if(BBL_PRED_FIRST(bbl)==NULL)
	  continue;

        VERBOSE(2,("Stuckness: getting rid of incoming edges from no-exit bbl @eiB",bbl));
	BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge1,edge2)
	{
	  if(CFG_EDGE_CAT(edge1)==ET_FALLTHROUGH && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == I386_Jcc)
	  {
	    I386InstructionMakeJump(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    I386_INS_SET_CONDITION(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))), I386_CONDITION_NONE);
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  
	  else if(CFG_EDGE_CAT(edge1)==ET_JUMP && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == I386_Jcc)
	  {
	    I386InstructionMakeNoop(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_FALLTHROUGH && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && !I386InsIsControlTransfer(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))))
	  {
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_JUMP && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == I386_JMP)
	  {
	    I386InstructionMakeNoop(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  
	  else if(CFG_EDGE_CAT(edge1)==ET_CALL && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == I386_CALL)
	  {
	    I386InstructionMakeNoop(T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    if(CFG_EDGE_CORR(edge1))
	      CfgEdgeKill(CFG_EDGE_CORR(edge1));
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_SWITCH)
	  {
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_RETURN)
	  {
	    if(CFG_EDGE_CORR(edge1))
	      CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge1), NULL);
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	}
      }
    }
  }
#if 0
  {
    t_bbl * tmp;
    CFG_FOREACH_BBL_SAFE(cfg,bbl,tmp)
      {
	if (bbl == CFG_UNIQUE_EXIT_NODE(BBL_CFG(bbl)))
	  continue;
	if(bbl == CFG_SWI_HELL_NODE(BBL_CFG(bbl)))
	  continue;
	if(NODE_SUCC_FIRST(bbl)==NULL && NODE_PRED_FIRST(bbl)==NULL)
	  {
	    BblKill(bbl);
	  }
      }
  }
#endif
  /*}}}*/
}

/* {{{ I386AddBasicBlockEdges */
static t_uint32 I386AddBasicBlockEdges(t_object * obj)
{
  /* Basic Block Edges
   * -----------------
   *
   * Add edges between the newly created basic blocks. Both the lineair (array)
   * representation of the instructions and the basic block representation are
   * available: you can access the (copy) basic block by using INS_BBL of a
   * leader in the lineair representation.
   *
   * Although connecting the blocks isn't that hard, it could be difficult to
   * find out if a given jump instructions is a call or just a jump (without
   * return). Getting this completly safe is probably not what you want: if you
   * implement this completly safe, you'll probably end up with one big
   * function, which will degrade the performance of the rest of diablo or hell
   * nodes after each jump which will degrade the performance even further.
   *
   * On the other hand, if you get this wrong (e.g add a jump edge where there
   * should have been a call edge), then chances are that your cfg (and
   * thus the produced binary) is wrong (the successor block of the call block
   * will have no incoming (return) edge, and thus this block is dead). 
   *
   * The solution is to look at code that is produced by your compiler(s).
   * It'll probably have only a few possibilties to produce calls. If you
   * handle them as safe as possible, than you're off the hook.  */

  t_i386_ins *ins, *iter,  *block_end, *block_start, *prev, *prev_controltransfer;
  t_bbl * block, * ft_block;
  t_cfg * cfg = OBJECT_CFG(obj);
  t_reloc_ref * ref;
  t_uint32 nedges=0; 

  t_section *section;
  t_uint32 section_counter;

  OBJECT_FOREACH_CODE_SECTION(obj, section, section_counter)
  {
    /* {{{ first detect switch tables and add the necessary switch edges.
     * we don't have to do this, but otherwise a lot of hell edges will
     * be added that substantially degrade the accuracy and performance of a 
     * number of analyses */

    /* Switch table; first possibility {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = I386_INS_INEXT(ins))
    {
      /* switch constructs have the following pattern:
       *   cmp $ubound, %reg
       *   ...
       *   ja <default_case>
       *   ...
       *   jmp *<jumptable_base>(,%reg,4)
       */

      struct _jtentry {
        t_reloc *rel;
        t_address from_offset;
      };
      typedef struct _jtentry t_jtentry;

      t_i386_operand * op = I386_INS_SOURCE1(ins);
      t_reg jumpreg;
      t_reloc * jumptablerel;
      t_uint32 bound;
      t_regset changed;
      t_i386_ins * cmp;
      t_reloc_ref * rr;
      t_ptr_array jtrelocs;

      if (I386_INS_IPREV(ins))
        prev = I386_INS_IPREV(ins);
      if (prev && I386InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      if (!prev_controltransfer || (I386_INS_OPCODE(prev_controltransfer) != I386_Jcc)) continue;
      if (I386_INS_CONDITION(prev_controltransfer) != I386_CONDITION_A) continue;
      if (I386_INS_OPCODE(ins) != I386_JMP) continue;
      if (I386_OP_TYPE(op) != i386_optype_mem) continue;
      if (I386_OP_BASE(op) != I386_REG_NONE) continue;
      if (I386_OP_SCALE(op) != I386_SCALE_4) FATAL(("unexpected scale in switch jump @I",ins));
      jumpreg = I386_OP_INDEX(op);

      /* check to see if the jumpreg isn't changed between the ja and the jmp */
      changed = NullRegs;
      for (iter = I386_INS_IPREV(ins); iter != prev_controltransfer; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (RegsetIn(changed,jumpreg))
        continue;

      if (!INS_REFERS_TO(BBL_INS_LAST(I386_INS_BBL(ins)))) FATAL(("could not find jumptable relocation for @I",ins));
      jumptablerel = RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(I386_INS_BBL(ins))));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) &&
          (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_BBL))
        continue;
      /* if the jump table is in a subsection, it must be read-only */
      if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) == RT_SUBSECTION) &&
          (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION))
        continue;

      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = I386_INS_IPREV(prev_controltransfer); cmp; cmp = I386_INS_IPREV(cmp))
      {
        if (I386_INS_OPCODE(cmp) == I386_CMP && 
            I386_OP_TYPE(I386_INS_SOURCE1(cmp)) == i386_optype_reg &&
            I386_OP_BASE(I386_INS_SOURCE1(cmp)) == jumpreg &&
            I386_OP_TYPE(I386_INS_SOURCE2(cmp)) == i386_optype_imm)
          break;

        RegsetSetUnion(changed, I386_INS_REGS_DEF(cmp));

        if (I386InsIsControlTransfer(cmp) || RegsetIn(changed,jumpreg) ||
            RegsetIn(changed,I386_CONDREG_CF) || RegsetIn(changed,I386_CONDREG_ZF))
        {
          cmp = NULL;
          break;
        }
      }
      if (!cmp) continue;

      bound = I386_OP_IMMEDIATE(I386_INS_SOURCE2(cmp));
      if (G_T_UINT32(RELOCATABLE_CSIZE(RELOC_TO_RELOCATABLE(jumptablerel)[0])) < 4*(bound+1))
        DEBUG(("boo"));
      ASSERT(G_T_UINT32(RELOCATABLE_CSIZE(RELOC_TO_RELOCATABLE(jumptablerel)[0])) >= 4*(bound+1), ("jump table too small for switch bound"));

      PtrArrayInit(&jtrelocs,FALSE);
      switch (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]))
      {
        case RT_SUBSECTION:
        {
          for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
          {
            t_jtentry *jtentry = Malloc(sizeof(*jtentry));
            jtentry->rel = RELOC_REF_RELOC(rr);
            jtentry->from_offset = RELOC_FROM_OFFSET(jtentry->rel);
            PtrArrayAdd(&jtrelocs,jtentry);
          }
          break;
        }
        case RT_BBL:
        {
          t_ins *jtins;

          BBL_FOREACH_INS(T_BBL(RELOC_TO_RELOCATABLE(jumptablerel)[0]),jtins)
          {
            if (INS_REFERS_TO(jtins))
            {
              t_jtentry *jtentry = Malloc(sizeof(*jtentry));
              /* this is a set of bytes whereby every fourth has a relocation
               */
              ASSERT(AddressIsEq(AddressInverseMaskUint32(INS_CADDRESS(jtins),3),INS_CADDRESS(jtins)),("Relocation for non-aligned jt entry: @I - @R",jtins,RELOC_REF_RELOC(INS_REFERS_TO(jtins))));
              /* ensure there's only one relocation */
              ASSERT(!RELOC_REF_NEXT(INS_REFERS_TO(jtins)),("At least relocations for @I -- @R\n *AND* \n@R",jtins,RELOC_REF_RELOC(INS_REFERS_TO(jtins)),RELOC_REF_RELOC(RELOC_REF_NEXT(INS_REFERS_TO(jtins)))));
              jtentry->rel = RELOC_REF_RELOC(INS_REFERS_TO(jtins));
              jtentry->from_offset = AddressSub(INS_OLD_ADDRESS(jtins),BBL_OLD_ADDRESS(T_BBL(RELOC_TO_RELOCATABLE(jumptablerel)[0])));
              PtrArrayAdd(&jtrelocs,jtentry);
            }
          }
          break;
        }
        default:
          FATAL(("Unsupported jt relocatable type: @T",RELOC_TO_RELOCATABLE(jumptablerel)[0]));
      }

      {
        int i;

        for (i = 0; i < PtrArrayCount(&jtrelocs); i++)
        {
          t_jtentry *jtentry = (t_jtentry *)PtrArrayGet(&jtrelocs,i);
          if (AddressIsGe(jtentry->from_offset,RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
              AddressIsLe(jtentry->from_offset,AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0], 4*bound)))
          {
            t_cfg_edge * edge;
            t_reloc *rel = jtentry->rel;
            RELOC_SET_HELL(rel, FALSE);
            ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
            edge = CfgEdgeCreate(cfg,I386_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
            CFG_EDGE_SET_REL(edge,  rel);
            RELOC_SET_SWITCH_EDGE(rel, edge);
            CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT32(AddressSub(jtentry->from_offset,RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
          }
        }
      }
      PtrArrayFini(&jtrelocs, TRUE);
    }
    /* }}} */

    /* Switch table; first possibility (in uclibc) {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = I386_INS_INEXT(ins))
    {
      /* switch constructs have the following pattern:
       *   cmp $ubound, off(%esp)	(cmp)
       *   ...
       *   ja <default_case>	(prev_controltransfer)
       *   ...
       *   mov off(%esp),%reg	(mov)
       *   ...
       *   jmp *<jumptable_base>(,%reg,4)	(ins)
       */

      t_reloc * jumptablerel;
      t_uint32 bound;
      t_regset changed;
      t_i386_ins * cmp, *mov;
      t_reloc_ref * rr;

      if (I386_INS_IPREV(ins))
        prev = I386_INS_IPREV(ins);
      if (prev && I386InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      /* chack: ja <default_case> {{{*/
      if (!prev_controltransfer || (I386_INS_OPCODE(prev_controltransfer) != I386_Jcc)) continue;
      if (I386_INS_CONDITION(prev_controltransfer) != I386_CONDITION_A) continue;
      /* }}} */
      /* check: jmp *<jumptable_base>(,%reg,4) {{{ */
      if (I386_INS_OPCODE(ins) != I386_JMP) continue;
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_mem) continue;
      if (I386_OP_BASE(I386_INS_SOURCE1(ins)) != I386_REG_NONE) continue;
      if (I386_OP_SCALE(I386_INS_SOURCE1(ins)) != I386_SCALE_4) FATAL(("unexpected scale in switch jump @I",ins));
      /* }}} */
      /* Check  mov  off(%esp),%reg {{{ */
      for(mov=ins;mov!=NULL;mov=I386_INS_IPREV(mov))
      {
        if(I386_INS_OPCODE(mov) == I386_MOV)
        {
          if(I386_OP_BASE(I386_INS_DEST(mov))==I386_OP_INDEX(I386_INS_SOURCE1(ins)))
            break;
        }
        else if (mov==prev_controltransfer)
          break;
      }
      if (mov==prev_controltransfer)
        continue;
      /* }}} */

      /* check to see if %reg isn't changed between the mov and the jmp {{{ */
      changed = NullRegs;
      for (iter = I386_INS_IPREV(ins); iter != mov; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (RegsetIn(changed,I386_OP_INDEX(I386_INS_SOURCE1(ins))))
        continue;
      /* }}} */

      if (!I386_INS_REFERS_TO(T_I386_INS(BBL_INS_LAST(I386_INS_BBL(ins))))) FATAL(("could not find jumptable relocation for @I",ins));
      jumptablerel = RELOC_REF_RELOC(I386_INS_REFERS_TO(T_I386_INS(BBL_INS_LAST(I386_INS_BBL(ins)))));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;

      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = I386_INS_IPREV(prev_controltransfer); cmp; cmp = I386_INS_IPREV(cmp))
      {
        if (I386_INS_OPCODE(cmp) == I386_CMP && 
            I386_OP_TYPE(I386_INS_SOURCE1(cmp)) == i386_optype_mem &&
            I386_OP_TYPE(I386_INS_SOURCE2(cmp)) == i386_optype_imm)
          break;

        RegsetSetUnion(changed, I386_INS_REGS_DEF(cmp));

        if (I386InsIsControlTransfer(cmp) || RegsetIn(changed,I386_CONDREG_CF) || RegsetIn(changed,I386_CONDREG_ZF))
        {
          cmp = NULL;
          break;
        }
      }
      if (!cmp) continue;

      bound = I386_OP_IMMEDIATE(I386_INS_SOURCE2(cmp));
      ASSERT(G_T_UINT32(SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0]))) >= 4*(bound+1), ("jump table too small for switch bound"));

      for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
      {
        t_reloc * rel = RELOC_REF_RELOC(rr);
        if (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
            AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],4*bound)))
        {
          t_cfg_edge * edge;
          RELOC_SET_HELL(rel, FALSE);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
          edge = CfgEdgeCreate(cfg,I386_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
          CFG_EDGE_SET_REL(edge,  rel);
          RELOC_SET_SWITCH_EDGE(rel, edge);
          CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT32(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
        }
      }
    }
    /* }}} */

    /* Switch table; first possibility (in uclibc) {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = I386_INS_INEXT(ins))
    {
      /* another switch construct has the following pattern:
       *
       * example Uclibc sysconf.o:
       *
       * cmp    $0x83,%edx
       * ja     28 <$a>
       * lea    0x0(%ebx),%eax
       * mov    (%eax,%edx,4),%eax
       * add    %ebx,%eax
       * jmp    *%eax
       * 
       * general:
       * 
       *   cmp 	$ubound, %reg1 		(cmp)
       *   ...
       *   ja 	<default_case> 		(prev_controller)
       *   ...
       *   lea 	off(%reg2),%reg3	(lea)
       *   ...
       *   mov 	(%reg3,%reg1,4),%reg4	(mov)
       *   ...
       *   add 	%reg2,%reg4		(add)
       *   ...
       *   jmp 	*%reg4 			(ins)
       */

      t_reloc * jumptablerel;
      t_uint32 bound;
      t_regset changed;
      t_i386_ins * cmp, *lea, *mov, *add, *h_ins;
      t_reloc_ref * rr;

      if (I386_INS_IPREV(ins))
        prev = I386_INS_IPREV(ins);
      if (prev && I386InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      /* Check: ja     <default_case> {{{*/
      if (!prev_controltransfer || (I386_INS_OPCODE(prev_controltransfer) != I386_Jcc)) continue;
      if (I386_INS_CONDITION(prev_controltransfer) != I386_CONDITION_A) continue;
      /* }}} */
      /* Check: jmp    *%reg4 {{{ */
      if (I386_INS_OPCODE(ins) != I386_JMP) continue;
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg) continue;
      if (I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_NONE) continue;
      /* }}} */ 
      /* Check add    %reg2,%reg4  {{{ */
      for(add=ins;add!=NULL;add=I386_INS_IPREV(add))
      {
        if(I386_INS_OPCODE(add) == I386_ADD)
        {
          if(I386_OP_BASE(I386_INS_DEST(add))==I386_OP_BASE(I386_INS_SOURCE1(ins)))
            break;
        }
        else if (add==prev_controltransfer)
          break;
      }
      if (add==prev_controltransfer)
        continue;
      /* }}} */
      /* Check  mov    (%reg3,%reg1,4),%reg4 {{{ */
      for(mov=add;mov!=NULL;mov=I386_INS_IPREV(mov))
      {
        if(I386_INS_OPCODE(mov) == I386_MOV)
        {
          if(I386_OP_BASE(I386_INS_DEST(mov))==I386_OP_BASE(I386_INS_DEST(add)))
            break;
        }
        else if (mov==prev_controltransfer)
          break;
      }
      if (mov==prev_controltransfer)
        continue;
      /* }}} */

      /* Check lea    off(%reg2),%reg3 {{{ */
      for(lea=mov;lea!=NULL;lea=I386_INS_IPREV(lea))
      {
        if(I386_INS_OPCODE(lea) == I386_LEA)
        {
          if(I386_OP_BASE(I386_INS_DEST(lea))==I386_OP_BASE(I386_INS_SOURCE1(mov)) &&
              I386_OP_BASE(I386_INS_SOURCE1(lea))==I386_OP_BASE(I386_INS_SOURCE1(add)))
            break;
        }
        else if (lea==prev_controltransfer)
          break;
      }
      if (lea==prev_controltransfer)
        continue;
      /* }}} */

      /* check to see if the registers aren't changed between the initialisation of the register and the use of it {{{*/
      /* reg4 */
      changed = NullRegs;
      for (iter = I386_INS_IPREV(ins); iter != add; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (RegsetIn(changed,I386_OP_BASE(I386_INS_SOURCE1(ins))))
        continue;
      changed = NullRegs;
      for (iter = I386_INS_IPREV(add); iter != mov; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (RegsetIn(changed,I386_OP_BASE(I386_INS_DEST(add))))
        continue;

      /* reg3 */
      changed = NullRegs;
      for (iter = I386_INS_IPREV(mov); iter != lea; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (RegsetIn(changed,I386_OP_BASE(I386_INS_DEST(lea))))
        continue;

      for(h_ins=T_I386_INS(BBL_INS_LAST(I386_INS_BBL(ins)));h_ins!=NULL;h_ins=I386_INS_IPREV(h_ins))
      {
        if(I386_INS_REFERS_TO(h_ins))
          if(I386_INS_OPCODE(h_ins)==I386_LEA)
            break;
        if(h_ins ==prev_controltransfer)
          break;
      }
      if(h_ins ==prev_controltransfer)
        continue;
      /* }}} */

      if (!I386_INS_REFERS_TO(h_ins)) FATAL(("could not find jumptable relocation for @I",h_ins));
      jumptablerel = RELOC_REF_RELOC(I386_INS_REFERS_TO(h_ins));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;


      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = I386_INS_IPREV(prev_controltransfer); cmp; cmp = I386_INS_IPREV(cmp))
      {
        if (I386_INS_OPCODE(cmp) == I386_CMP && 
            I386_OP_TYPE(I386_INS_SOURCE1(cmp)) == i386_optype_reg &&
            I386_OP_BASE(I386_INS_SOURCE1(cmp)) == I386_OP_INDEX(I386_INS_SOURCE1(mov)) &&
            I386_OP_TYPE(I386_INS_SOURCE2(cmp)) == i386_optype_imm)
          break;
      }

      /*  check to see if the registers aren't changed between the initialisation of the register and the use of it : reg1 {{{*/
      changed = NullRegs;
      for (iter = I386_INS_IPREV(mov); iter != cmp; iter = I386_INS_IPREV(iter))
        RegsetSetUnion(changed, I386_INS_REGS_DEF(iter));
      if (I386InsIsControlTransfer(cmp) || RegsetIn(changed,I386_OP_BASE(I386_INS_SOURCE1(cmp))) ||
          RegsetIn(changed,I386_CONDREG_CF) || RegsetIn(changed,I386_CONDREG_ZF))
        continue;
      /* }}} */

      if (!cmp) continue;

      bound = I386_OP_IMMEDIATE(I386_INS_SOURCE2(cmp));
      ASSERT(G_T_UINT32(SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0]))) >= 4*(bound+1), ("jump table too small for switch bound"));

      for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
      {
        t_reloc * rel = RELOC_REF_RELOC(rr);
        if (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
            AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],4*bound)))
        {
          t_cfg_edge * edge;
          RELOC_SET_HELL(rel, FALSE);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
          edge = CfgEdgeCreate(cfg,I386_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
          CFG_EDGE_SET_REL(edge,  rel);
          RELOC_SET_SWITCH_EDGE(rel, edge);
          CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT32(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
        }
      }
    } 
    /* }}} */

    /* }}} */

    /* walk through the original(!) array (we need offsets) and add
     * all possible successors to every node */

    for(ins=SECTION_DATA(section); ins!=NULL; ins=I386_INS_INEXT(ins))
    {
      /* we're only interested in the first and last instructions of a basic block */
      if (!(I386_INS_ATTRIB(ins) & IF_BBL_LEADER)) 
        continue;

      block_start = ins;
      block=I386_INS_BBL(block_start);

      block_end = block_start;
      iter = I386_INS_INEXT(block_end);
      while (iter)
      {
        if (I386_INS_ATTRIB(iter) & IF_BBL_LEADER)
          break;
        else
          block_end = iter;
        iter = I386_INS_INEXT(iter);
      }
      /* block_end now points to the last "real" instruction of the block in the linear list,
       * iter points to the first instruction of the next block (or NULL if there is no next block) */
      ft_block = iter ? I386_INS_BBL(iter) : NULL;

      /* {{{ 1. ADD EDGES FOR RELOCATIONS:
       *
       * each relocation gets it's own edge. These edges will be automatically
       * deleted when the relocation is removed. There's no need to add an edge
       * for the obvious relocations (such as the relocations that are present
       * for the calculation of the offsets in intermodular jumps), since they
       * are completly described by the edge itself (the reason for the edge is
       * that there is a jump, not that there is a relocation)
       */

      for (ref=BBL_REFED_BY(block); ref!=NULL; ref=RELOC_REF_NEXT(ref))
      {
        /* Skip the obvious relocations and reloctions to data */
        if (RELOC_HELL(RELOC_REF_RELOC(ref)))
        {
          t_cfg_edge * i_edge;

          /* do not add hell edges to data blocks */
          if (IS_DATABBL(block))
          {
            /* the hell flag shouldn't be on for the relocation either: turn it off */
            RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);
            continue;
          }

          /* skip jump relocations: they are already represented by jump and call edges in the cfg */
          if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(RELOC_REF_RELOC(ref)))==RT_INS)
          {
            if (I386InsIsControlTransfer((t_i386_ins *)RELOC_FROM(RELOC_REF_RELOC(ref))))
            {
              RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);
              continue;
            }
          }

          /* check if a hell edge already exists */
          BBL_FOREACH_PRED_EDGE(block,i_edge)
            if ((CFG_EDGE_HEAD(i_edge)==CFG_HELL_NODE(cfg))&&(CFG_EDGE_CAT(i_edge)==ET_CALL))
              break;

          if (i_edge)
          {
            /* found a hell edge: just increment the refcount */
            RELOC_SET_EDGE(RELOC_REF_RELOC(ref), i_edge);
            CFG_EDGE_SET_REFCOUNT(i_edge, CFG_EDGE_REFCOUNT(i_edge)+1);
            /* also increment the refcount of the corresponding return edge!!! */
            ASSERT(CFG_EDGE_CORR(i_edge),("Call edge @E does not have a corresponding edge!",i_edge));
            CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(i_edge),  CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(i_edge))+1);
          }
          else
          {
            /* not found: create a new edge */
            RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), block,NULL,NULL));
            nedges++;
          }
        }
      } /* }}} */
      /* 2. ADD EDGES FOR DATA
       *
       * No edges enter the data block, no edges leave the data block.
       */
      if (IS_DATABBL(block)) continue;

      /* 3. ADD NORMAL EDGES
       *
       * now determine the successors of the block and their types
       */
      if (I386InsIsControlTransfer((t_i386_ins *)block_end))
      {
        if (I386InsIsConditional(block_end))
        {
          if (!ft_block || IS_DATABBL(ft_block))
          {
            VERBOSE(0,("conditional instruction but non-suitable fallthrough block\nblock @iBft_block @iB", block, ft_block));
          }
          else
          {
            CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
            nedges++;
          }
        }

        switch (I386_INS_OPCODE(block_end))
        {
          case I386_JMP:
          case I386_Jcc:
          case I386_JECXZ:
            {
              if (I386_OP_TYPE(I386_INS_SOURCE1(block_end)) == i386_optype_imm)
              {
                /* we can determine the jump destination */
                t_bbl * target;
                t_i386_ins * target_ins;
                t_address dest = AddressAdd(I386_INS_CSIZE(block_end),I386_INS_CADDRESS(block_end));
                dest = AddressAddUint32(dest,I386_OP_IMMEDIATE(I386_INS_SOURCE1(block_end)));
                target_ins = T_I386_INS(ObjectGetInsByAddress(obj, dest));
                if (target_ins)
                {
                  target = I386_INS_BBL(target_ins);
                  CfgEdgeCreate(cfg,block,target,ET_JUMP);
                  nedges++;
                }
                else
                {
                  t_object * lo = ObjectGetLinkerSubObject(obj);	
                  t_section * sec = SectionGetFromObjectByAddress(lo, dest);

                  if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
                  {
                    t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
                    t_section * rel  =  SectionGetFromObjectByName(lo, relname);
                    Free(relname);
                    ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));
                    {
	              t_bbl *dyncallhell = CfgGetDynamicCallHell(cfg, SECTION_NAME(sec) + 8);
	              CfgEdgeCreate(cfg, block, dyncallhell, ET_JUMP);
		      nedges++;
                    }
                  }
                  else
                  {

                    ASSERT(AddressIsNull(dest),("could not find destination of @I",block_end));

                    /* jmp 0 was encountered.
                     *         movl %eax <- $0
                     *         testl %eax,%eax
                     *         je label
                     *         ...
                     *         jmp 0
                     *         ...
                     *   label: ...
                     *
                     * The BBL with the call 0 as block_end is either a dead block or data
                     * (otherwise the program would crash because of the call 0)
                     * We will add a jump edge that goes back to the block itself. this is an incorrect
                     * modelling, but we don't care as the block will never be executed anyway
                     */
                    I386InstructionMakeNoop(block_end);
                    I386InstructionMakeNoop(T_I386_INS(BBL_INS_LAST(I386_INS_BBL(block_end))));
                  }
                }
              }
              else
              {
                /* destination unknown: add a hell edge, except if we're dealing with a 
                 * switch here: in that case the switch edges that are already in place 
                 * suffice. */
                t_bool is_switch = FALSE;
                t_cfg_edge * edge;
                BBL_FOREACH_SUCC_EDGE(block,edge)
                  if (CFG_EDGE_CAT(edge) == ET_SWITCH)
                  {
                    is_switch = TRUE;
                    break;
                  }
                if (!is_switch)
                {
                  CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
                  nedges++;
                }
              }
            }
            break;
          case I386_CALL:
            {
              t_bool nolink = !ft_block || IS_DATABBL(ft_block);

              if (I386_OP_TYPE(I386_INS_SOURCE1(block_end)) == i386_optype_imm)
              {
                /* we can determine the call destination */
                t_bbl * target;
                t_i386_ins * target_ins;
                t_address dest = AddressAdd(I386_INS_CSIZE(block_end),I386_INS_CADDRESS(block_end));
                dest = AddressAddUint32(dest,I386_OP_IMMEDIATE(I386_INS_SOURCE1(block_end)));
                target_ins = T_I386_INS(ObjectGetInsByAddress(obj, dest));
                if (target_ins)
                {
                  target = I386_INS_BBL(target_ins);
                  if (!nolink)
                  {
                    CfgEdgeCreateCall(cfg, block, target, ft_block, NULL);
                    nedges++;
                  }
                  else
                  {
                    /* if the function return would end up in data or out of the code section,
                     * we can't add a link edge. Therefore we assume the call is to a non-returning
                     * function and so we can just as well model it with a jump edge */
                    CfgEdgeCreate(cfg,block,target,ET_JUMP);
                    nedges++;
                  }
                }
                else
                {
                  t_object * lo = ObjectGetLinkerSubObject(obj);	
                  t_section * sec = SectionGetFromObjectByAddress(lo, dest);

                  if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
                  {
                    t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
                    t_section * rel  =  SectionGetFromObjectByName(lo, relname);
                    Free(relname);
                    ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));
                    {
                      t_bbl *dyncallhell = CfgGetDynamicCallHell(cfg, SECTION_NAME(sec) + 8);
                      CfgEdgeCreateCall(cfg, block, dyncallhell, ft_block, NULL);
                      nedges++;
                    }
                  }
                  else
                  {
                    ASSERT(AddressIsNull(dest),("could not find destination of @I",block_end));

                    /* call 0 was encountered.
                     *         movl %eax <- $0
                     *         testl %eax,%eax
                     *         je label
                     *         ...
                     *         call 0
                     *         ...
                     *   label: ...
                     *
                     * The BBL with the call 0 as block_end is either a dead block or data
                     * (otherwise the program would crash because of the call 0)
                     * We will add a jump edge that goes back to the block itself. this is an incorrect
                     * modelling, but we don't care as the block will never be executed anyway
                     */
                    I386InstructionMakeNoop(block_end);
                    I386InstructionMakeNoop(T_I386_INS(BBL_INS_LAST(I386_INS_BBL(block_end))));
                  }
                }
              }
              else
              {
                /* destination unknown: add a hell edge */
                if (!nolink)
                {
                  CfgEdgeCreateCall(cfg,block,CFG_HELL_NODE(cfg),ft_block,NULL);
                  nedges++;
                }
                else
                {
                  CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
                  nedges++;
                }
              }
            }
            break;
          case I386_LOOP:
          case I386_LOOPZ:
          case I386_LOOPNZ:
            {
              t_i386_ins * target_ins;
              t_bbl * target;
              t_address dest = AddressAdd(I386_INS_CADDRESS(block_end),I386_INS_CSIZE(block_end));
              dest = AddressAddUint32(dest,I386_OP_IMMEDIATE(I386_INS_SOURCE1(block_end)));
              target_ins = T_I386_INS(ObjectGetInsByAddress(obj, dest));
              ASSERT(target_ins,("Could not find destination for @I",block_end));
              target = I386_INS_BBL(target_ins);
              CfgEdgeCreate(cfg,block,target,ET_JUMP);
              nedges++;
            }
            break;
          case I386_RET:
            /* add a return edge that at the moment points to hell */
            CfgEdgeCreate(cfg,block,CFG_EXIT_HELL_NODE(cfg),ET_RETURN);
            nedges++;
            break;
          case I386_INT:
          case I386_INTO:
          case I386_INT3:
          case I386_SYSENTER:
            {
              t_cfg_edge * swi_edge;
              swi_edge = CfgEdgeCreateSwi(cfg,block,ft_block);
              nedges++;
              nedges++;
              if (!ft_block || IS_DATABBL(ft_block))
              {
                CfgEdgeKill(CFG_EDGE_CORR(swi_edge));
                CFG_EDGE_SET_CORR(swi_edge, NULL);
                nedges--;
              }
            }
            break;
          case I386_UD2:
            /* ud2 causes an illegal opcode trap, so this actually jumps to hell.
             * furthermore, check if there is a data block following this
             * instruction. if so, it should be marked, as this data is
             * associated with the ud2 instruction (for debugging purposes in the
             * linux kernel) */
            CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
            if (ft_block && IS_DATABBL(ft_block))
            {
              /* add relocation to this block, so it won't be removed by
               * CfgRemoveDeadCodeAndData() */
              RelocTableAddRelocToRelocatable(
                  OBJECT_RELOC_TABLE(obj),
                  AddressNew32(0),
                  T_RELOCATABLE(BBL_INS_LAST(block)),
                  AddressNew32(0),
                  T_RELOCATABLE(ft_block),
                  AddressNew32(0),
                  FALSE, NULL, NULL, NULL,
                  "R00A00+" "\\\\s0000$"
                  );
            }
            break;
          case I386_BOUND:
            /* for all these strange instructions, we just add a jump edge to hell
             * (as the control flow may be altered in inpredictible ways), and a 
             * fallthrough edge to be on the safe side */
            CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
            nedges++;
            if (ft_block && !IS_DATABBL(ft_block))
            {
              CfgEdgeCreate(cfg,block,ft_block,ET_FALLTHROUGH);
              nedges++;
            }
            break;
          case I386_HLT:
            /* for a hlt instruction, we assume the program will never be
             * resumed.  therefore, it suffices to add a jump edge back to the
             * beginning of the block (the hlt instruction will almost certainly
             * be alone in a block */
            /* TODO for kernels, we will have to forego this assumption, as the
             * hlt instruction is used within the idle task */
            if (DiabloFlowgraphInKernelMode())
              CfgEdgeCreate(cfg,block,ft_block,ET_FALLTHROUGH);
            else 
              CfgEdgeCreate(cfg,block,block,ET_JUMP);
            nedges++;
            break;
          case I386_IRET:
          case I386_SYSEXIT:
            /* add a hell edge... a correct modeling will be decided upon later */
            CfgEdgeCreate(cfg,block,CFG_EXIT_HELL_NODE(cfg),ET_RETURN);
            break;

          case I386_JMPF:
            if (I386_OP_TYPE(I386_INS_SOURCE1(block_end)) == i386_optype_mem)
            {
              CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
              nedges++;
            }
            else
            {
              /* destination given as an immediate operand. assume the destination
               * code segment is the same as the current code segment, because I
               * wouldn't know what to do otherwise :-( */
              t_address dest;
              t_i386_ins * target;
              t_bbl * target_block;
              dest=AddressNew32(I386_OP_IMMEDIATE(I386_INS_SOURCE1(block_end)));
              target = T_I386_INS(ObjectGetInsByAddress(obj, dest));
              if (target)
              {
                target_block = I386_INS_BBL(target);
                CfgEdgeCreate(cfg,block,target_block,ET_JUMP);
                nedges++;
              }
              else
              {
                /* if we can't find the destination, it's supposedly a jump to 
                 * a different segment, so we just add a hell edge */
                CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
                nedges++;
              }
            }
            break;
          case I386_CALLF:
            {
              t_bool nolink = !ft_block || IS_DATABBL(ft_block);

              if (I386_OP_TYPE(I386_INS_SOURCE1(block_end)) == i386_optype_mem)
              {
                /* add hell edge */
                if (!nolink)
                {
                  CfgEdgeCreateCall(cfg,block,CFG_HELL_NODE(cfg),ft_block,NULL);
                  nedges++;
                }
                else
                {
                  CfgEdgeCreate(cfg,block,CFG_HELL_NODE(cfg),ET_JUMP);
                  nedges++;
                }
              }
              else
              {
                /* destination given as an immediate operand. assume the destination
                 * code segment is the same as the current code segment, because I
                 * wouldn't know what to do otherwise :-( */
                t_address dest;
                t_i386_ins * target;
                t_bbl * target_block;
                dest=AddressNew32(I386_OP_IMMEDIATE(I386_INS_SOURCE1(block_end)));
                target = T_I386_INS(ObjectGetInsByAddress(obj, dest));
                if (target)
                {
                  target_block = I386_INS_BBL(target);

                  if (!nolink)
                  {
                    CfgEdgeCreateCall(cfg,block,target_block,ft_block,NULL);
                    nedges++;
                  }
                  else
                  {
                    CfgEdgeCreate(cfg,block,target_block,ET_JUMP);
                    nedges++;
                  }
                }
                else
                {
                  /* apparently it's a call to a different segment, which is pretty odd */
                  FATAL(("implement call to another code segment"));
                }
              }
            }
            break;
          case I386_RETF:
            /* add a return edge that at the moment points to hell */
            CfgEdgeCreate(cfg,block,CFG_EXIT_HELL_NODE(cfg),ET_RETURN);
            nedges++;
            break;
          default:
            FATAL(("unknown control transfer instruction @I",block_end));
        }
      }
      else
      {
        /* just add a fallthrough edge to the next block */
        if (ft_block && !IS_DATABBL(ft_block))
          CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
      }
    }
  }
  return nedges;
}
/* }}} */

/* {{{ I386FindBBLLeaders */
static t_uint32 I386FindBBLLeaders(t_object * obj)
{
  /* Basic leader-detection algorithm.
   * ---------------------------------
   *
   * See e.g. the Red Dragon Book ("Compilers: Principles, Techniques and
   * Tools", by Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman) for more
   * details.
   *
   * The aim of this function is to identify all start-addresses of basic
   * blocks (and mark them). There are four reasons to mark an instructions as
   * a basic block leader:
   *
   * 1. The instruction is the target of a direct or indirect jump. This is not
   * as easy as it seems, especially when the program counter is explicit:
   * every instruction that changes the pc can be considered as a jump
   * instruction.
   *
   * 2. It's the successor of one of the above (if a jump is conditional, than
   * the next instruction will be the first instruction of the fall-through
   * block, else it is either target of an other jump or a dead block. In the
   * latter case, it will be removed by the unreachable block removal routines,
   * so we just mark it as a basic block leader).
   *
   * 3. There is an address produced to this basic block, this is either
   *
   *    a. done directly, using the program counter (we assume no real
   *    code-address calculations are present (so not address of function + x) 
   *
   *    b. using a relocation (and not necessary detected when scanning the
   *    instructions)
   *
   * For both cases we need to assume there will be an indirect jump to this
   * address.
   *
   * 4. The start of data blocks in code or instructions following datablocks
   * are considered basic block leaders.
   *
   */

  /* We can still assume that all instructions are stored sequentially in the
   * array Ains the last instruction stored can be recognised with INS_INEXT(ins) ==
   * NULL
   */

  t_uint32 nleaders=0;
  t_section *code;
  t_uint32 i;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_i386_ins * ins, * next;
    t_uint32 pc;

    /* Initialisation: The first instruction of the program is always a leader.
     * The reasoning is equal to that of reason 2: Either there will be a jump to
     * this address or it's the entry point or it is dead. */

    /* Instructions are still in a sequential list, stored in t_section->data */
    ins = T_I386_INS(SECTION_DATA(code)); 
    I386_INS_SET_ATTRIB(ins, I386_INS_ATTRIB(ins) | IF_BBL_LEADER);

    for (pc = 0; ins != NULL; pc += G_T_UINT32(I386_INS_CSIZE(ins)), ins = I386_INS_INEXT(ins))
    {
      if (!ins) break;

      /* the entry point is always a leader */
      if (AddressIsEq(I386_INS_CADDRESS(ins),OBJECT_ENTRY(obj)))
        I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_BBL_LEADER);

      /* reason 4: ins is the start of a data block */
      if (I386_INS_TYPE(ins) == IT_DATA)
      {
        if (I386_INS_IPREV(ins) && (I386_INS_TYPE(I386_INS_IPREV(ins)) != IT_DATA))
          I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_BBL_LEADER);
        continue;
      }
      /* reason 4b: ins is the first after a data block */
      if (I386_INS_IPREV(ins) && (I386_INS_TYPE(I386_INS_IPREV(ins)) == IT_DATA))
      {
        I386_INS_SET_ATTRIB(ins,   I386_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }

      /* reason 3b: ins is pointed to by a relocation */
      if (I386_INS_REFED_BY(ins))
      {
        I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }

      /* reason 2: ins following a control transfer */
      if (I386InsIsControlTransfer((t_i386_ins *)ins))
      {
        next = I386_INS_INEXT(ins);
        if (next) 
        {
          I386_INS_SET_ATTRIB(next, I386_INS_ATTRIB(next)| IF_BBL_LEADER);
        }
      }

      /* reason 1: target of a control transfer */
      switch (I386_INS_OPCODE(ins))
      {
        case I386_Jcc:
        case I386_JMP:
        case I386_JECXZ:
        case I386_CALL:
        case I386_LOOP:
        case I386_LOOPZ:
        case I386_LOOPNZ:
          if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
          {
            t_address dest = AddressAdd(I386_INS_CADDRESS(ins),I386_INS_CSIZE(ins));
            t_i386_ins *  target;
            dest = AddressAddUint32(dest,I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)));
            target = T_I386_INS(ObjectGetInsByAddress(obj, dest));

            if (target) 
            {
              I386_INS_SET_ATTRIB(target,  I386_INS_ATTRIB(target) | IF_BBL_LEADER);
            }
            else
            {
              t_object * lo = ObjectGetLinkerSubObject(obj);	
              t_section * sec = SectionGetFromObjectByAddress(lo, dest);

              if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
              {
                t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
                t_section * rel  =  SectionGetFromObjectByName(lo, relname);
                Free(relname);
                ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));

                RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(rel), AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
              }
              else
              {
                ASSERT(AddressIsNull(dest),("no destination for @I!", ins));
              }
            }
          }
          break;
        case I386_JMPF:
        case I386_CALLF:
          if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_farptr)
          {
            /* assume the destination segment is equal to the current code segment (what else could it be?) */
            t_address dest;
            t_i386_ins * target;
            dest=AddressNew32(I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)));
            target = T_I386_INS(ObjectGetInsByAddress(obj, dest));
            if (target) 
            {
              I386_INS_SET_ATTRIB(target, I386_INS_ATTRIB(target) | IF_BBL_LEADER);
            }
            else
            {
              /* apparently it's a jump to another segment, we'll just add a hell edge */
            }
          }
          break;
        default:
          /* do nothing but it saves us a ton of compiler warnings to include this default case here */
          break;
      }
    }
  }

  /* count the number of basic block leaders */
  nleaders = 0;
  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_i386_ins *ins;
    for (ins = (t_i386_ins *) SECTION_DATA(code); ins; ins = I386_INS_INEXT(ins))
      if (I386_INS_ATTRIB(ins) & IF_BBL_LEADER)
        nleaders++;
  }

  return nleaders;
} /* }}} */

static void
I386MakePicRelocationSafe(t_reloc *rel, t_i386_ins *call)
{
  t_bbl *bbl;
  /* to avoid problems with instructions getting added between the call
   * and the add, use the address of the call + its size instead of
   * the address of the add
   */
  ASSERT(RELOC_N_TO_RELOCATABLES(rel)==0,("GOTPC relocatable points to one or more relocatables: @R",rel));
  ASSERT(AddressIsEq(I386_INS_CSIZE(call),AddressNew32(5)),("got self address call != 5 -- @I",call));
  Free(RELOC_CODE(rel));
  RELOC_SET_CODE(rel,StringDup("gA00+R00s0005+-\\" WRITE_32));
  /* adjust the addend: it initially has to take into account the offset
   * inside the instruction (since it uses "P"), we no longer need
   * that. Additionally, it also takes into account the offset between
   * the return address of the call and the address of the add (which can
   * be different from 0 in case of "call .+5; pop ebx; add ebx, gotoff")
   */
  /* the offset inside the add */
  RELOC_ADDENDS(rel)[0] = AddressSub(RELOC_ADDENDS(rel)[0],RELOC_FROM_OFFSET(rel));
  /* the difference between the return address of the call
   * and the start of the add instruction
   */
  RELOC_ADDENDS(rel)[0] = AddressSub(RELOC_ADDENDS(rel)[0],AddressSubUint32(AddressSub(RELOCATABLE_CADDRESS(RELOC_FROM(rel)),I386_INS_CADDRESS(call)),5));
  /* relocations always have to point to bbls, can't point to instructions
   * -> split off the call in its own bbl if necessary
   */
  bbl = I386_INS_BBL(call);
  if (BBL_INS_FIRST(bbl) != T_INS(call))
  {
    bbl = BblSplitBlock(bbl,T_INS(call),TRUE);
    BblMark(bbl);
  }
  RelocAddRelocatable(rel,T_RELOCATABLE(bbl),0);
}

/* {{{ I386MakeAddressProducers */
static t_bool
HandleCallToNextInsPic(t_cfg *cfg, t_i386_ins *call, t_i386_ins *add)
{
  t_i386_operand *op;
  t_i386_ins *pop;
  t_i386_ins *mov;
  t_int32 offset;
  t_reloc *rel;
  t_i386_ins *nop1, *nop2;
  t_bbl *bbl, *next;
  t_cfg_edge *edge;
  t_reg destreg;

  op = I386_INS_SOURCE1(call);
  if (I386_OP_TYPE(op) != i386_optype_imm)
    return FALSE;
  if (I386_OP_IMMEDIATE(op) != 0)
    return FALSE;

  /* we've found a call to the next instruction. check for the rest of the pattern */
  bbl = I386_INS_BBL(call);
  pop = NULL;
  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if (CFG_EDGE_CAT(edge) & ET_FORWARD_INTERPROC)
    {
      next = CFG_EDGE_TAIL(edge);
      pop = T_I386_INS(BBL_INS_FIRST(next));
      break;
    }
  }
  if (!pop || (I386_INS_OPCODE(pop) != I386_POP))
  {
    WARNING (("found call to next instruction, but don't recognize the "
          "pattern. Assuming this is a genuine call.\n@I",call));
    return FALSE;
  }

  /* pattern is ok */
  rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(add));

  if (I386_OP_TYPE(I386_INS_DEST(pop)) != i386_optype_reg)
    FATAL(("found call to next ins but unknown pattern (@I)", call));

  /* keep the pic construct if we have to generate pic, or if the intermediate
   * value may be used between the pop and the add
   */
  if ((OBJECT_TYPE(CFG_OBJECT(cfg)) == OBJTYP_SHARED_LIBRARY_PIC) ||
      (OBJECT_TYPE(CFG_OBJECT(cfg)) == OBJTYP_EXECUTABLE_PIC) ||
      (I386_INS_INEXT(pop) != add))
  {
    I386MakePicRelocationSafe(rel,call);
    return TRUE;
  }

  /* make mov instruction */
  offset = (t_int32) I386_OP_IMMEDIATE(I386_INS_SOURCE1(add));
  destreg = I386_OP_BASE(I386_INS_DEST(add));
  I386MakeInsForBbl(MovToReg,Append,mov,bbl,destreg,I386_REG_NONE,G_T_UINT32(I386_INS_CADDRESS(pop)) + offset);
  I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
  I386_INS_SET_SECTION(mov,I386_INS_SECTION(call));
  /* have to be the same because the relocation is inserted in
   * OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)), and
   * OBJECT_RELOC_TABLE(INS_OBJECT(add)) is used to remove the
   * relocation again if the instruction is killed afterwards
   */
  ASSERT(INS_OBJECT(T_INS(mov))==CFG_OBJECT(cfg),("address producer object different from cfg object"));

  ASSERT(RELOC_N_TO_RELOCATABLES(rel)<=1, ("Complex relocation found: @R",rel));
  ASSERT(strncmp(RELOC_CODE(rel),"gA00+P-\\",8)==0, ("Unexpected relocation for PIC code %s", RELOC_CODE(rel)));


  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressSub(RELOC_ADDENDS(rel)[0], AddressAdd(RELOC_FROM_OFFSET(rel), I386_INS_CSIZE(pop))),
      (t_relocatable *) mov,
      AddressNew32(1),
      NULL,
      AddressNew32(0),
      RELOC_HELL(rel),
      RELOC_EDGE(rel),
      NULL,
      NULL,
      "gA00+\\" WRITE_32
      );

  I386_INS_SET_OLD_SIZE(mov,  I386_INS_OLD_SIZE(call));
  I386_INS_SET_OLD_ADDRESS(mov,  I386_INS_OLD_ADDRESS(call));

  /* {{{ save information about the original instructions:
   * this is only needed for instrumentation, but as the
   * placeholder instructions are nops, there is no problem:
   * they will just be removed in all non-instrumentation cases. */
  I386_INS_SET_AP_ORIGINAL(mov, Malloc(sizeof(t_i386_ins)));
  memcpy(I386_INS_AP_ORIGINAL(mov),call,sizeof(t_i386_ins));
  I386InsDupDynamic(I386_INS_AP_ORIGINAL(mov),call);
  I386_INS_SET_REFERS_TO(I386_INS_AP_ORIGINAL(mov),  NULL);
  I386_INS_SET_REFED_BY(I386_INS_AP_ORIGINAL(mov),  NULL);

  /* make two noops to replace the pop and add instructions */
  I386MakeInsForBbl(Noop,Prepend,nop2,next);
  I386_INS_SET_OLD_SIZE(nop2,  I386_INS_OLD_SIZE(add));
  I386_INS_SET_OLD_ADDRESS(nop2,  I386_INS_OLD_ADDRESS(add));
  /* save information about the original instruction */
  I386_INS_SET_AP_ORIGINAL(nop2, Malloc(sizeof(t_i386_ins)));
  memcpy(I386_INS_AP_ORIGINAL(nop2),add,sizeof(t_i386_ins));
  I386InsDupDynamic(I386_INS_AP_ORIGINAL(nop2),add);
  I386_INS_SET_REFERS_TO(I386_INS_AP_ORIGINAL(nop2),  NULL);
  I386_INS_SET_REFED_BY(I386_INS_AP_ORIGINAL(nop2),  NULL);

  I386MakeInsForBbl(Noop,Prepend,nop1,next);
  I386_INS_SET_OLD_SIZE(nop1,  I386_INS_OLD_SIZE(pop));
  I386_INS_SET_OLD_ADDRESS(nop1,  I386_INS_OLD_ADDRESS(pop));
  /* save information about the original instruction */
  I386_INS_SET_AP_ORIGINAL(nop1, Malloc(sizeof(t_i386_ins)));
  memcpy(I386_INS_AP_ORIGINAL(nop1),pop,sizeof(t_i386_ins));
  I386InsDupDynamic(I386_INS_AP_ORIGINAL(nop1),pop);
  I386_INS_SET_REFERS_TO(I386_INS_AP_ORIGINAL(nop1),  NULL);
  I386_INS_SET_REFED_BY(I386_INS_AP_ORIGINAL(nop1),  NULL);
  /* }}} */

  /* kill the pattern */
  I386InstructionMakeNoop(call);
  I386InstructionMakeNoop(pop);
  I386InstructionMakeNoop(add);
  while (BBL_SUCC_FIRST(bbl))
  {
    if (CFG_EDGE_CORR(BBL_SUCC_FIRST(bbl)))
      CfgEdgeKill(CFG_EDGE_CORR(BBL_SUCC_FIRST(bbl)));
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(bbl)));
  }

  /* add a fallthrough edge to connect the blocks involved in the pattern */
  CfgEdgeCreate(cfg,bbl,next,ET_FALLTHROUGH);
  return TRUE;
}


static t_bool
IsThunkCallPic(t_i386_ins *call, t_cfg_edge **call_edge)
{
  t_bbl* bbl = I386_INS_BBL(call);
  t_bbl* target = NULL;
  BBL_FOREACH_SUCC_EDGE(bbl,*call_edge)
  {
    if (CFG_EDGE_CAT(*call_edge) == ET_CALL)
    {
      target = CFG_EDGE_TAIL(*call_edge);
      break;
    }
  }
  ASSERT(target,("Can't find call edge in @eiB",bbl));

  return I386IsPcThunk(target);
}

t_bool
I386IsPcThunk(t_bbl* target)
{
  t_i386_ins *ins;
  t_i386_operand *src, *dest;

  /* look for __i686.get_pc_thunk.bx pattern */
  if (BBL_NINS(target) != 2)
    return FALSE;
  ins = T_I386_INS(BBL_INS_FIRST(target));
  src = I386_INS_SOURCE1(ins);
  dest = I386_INS_DEST(ins);
  if (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(target))) != I386_RET)
    return FALSE;
  if (I386_INS_OPCODE(ins) != I386_MOV)
    return FALSE;
  if (I386_OP_TYPE(src) != i386_optype_mem ||
      I386_OP_BASE(src) != I386_REG_ESP ||
      I386_OP_INDEX(src) != I386_REG_NONE ||
      I386_OP_IMMEDIATE(src) != 0)
    return FALSE;
  if (I386_OP_TYPE(dest) != i386_optype_reg)
    return FALSE;
  return TRUE;
}


static t_bool
HandleThunkCallPic(t_cfg *cfg, t_i386_ins *call, t_i386_ins *add)
{
  t_cfg_edge *edge;
  t_i386_ins *mov;
  t_i386_operand *src, *dest;
  t_reg destreg;
  t_reloc *rel;
  t_bbl *head, *ret;

  if (!IsThunkCallPic(call,&edge))
    return FALSE;

  rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(add));

  ASSERT(RELOC_N_TO_RELOCATABLES(rel)<=1, ("Complex relocation found @R",rel));
  ASSERT(strncmp(RELOC_CODE(rel),"gA00+P-\\",8)==0, ("Unexpected relocation for PIC code %s", RELOC_CODE(rel)));
  head = I386_INS_BBL(call);
  ret = NULL;
  BBL_FOREACH_SUCC_EDGE(head,edge)
  {
    if (CFG_EDGE_CAT(edge) & ET_FORWARD_INTERPROC)
    {
      ret = CFG_EDGE_TAIL(edge);
      break;
    }
  }
  /* keep the pic construct if we have to generate pic, or if the intermediate
   * value may be used between the call's return and the add
   */
  if ((OBJECT_TYPE(CFG_OBJECT(cfg)) == OBJTYP_SHARED_LIBRARY_PIC) ||
      (OBJECT_TYPE(CFG_OBJECT(cfg)) == OBJTYP_EXECUTABLE_PIC) ||
      (BBL_INS_FIRST(ret)!=T_INS(add)))
  {
    I386MakePicRelocationSafe(rel,call);
    return TRUE;
  }
  /* constant operand from the add */
  src = I386_INS_SOURCE1(add);
  /* destination register containing GOT address */
  dest = I386_INS_DEST(add);
  destreg = I386_OP_BASE(dest);
  /* create the address producer */
  I386MakeInsForBbl(MovToReg,Append,mov,head,destreg,I386_REG_NONE,G_T_UINT32(BBL_CADDRESS(ret)) + I386_OP_IMMEDIATE(src));
  I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
  I386_INS_SET_SECTION(mov,I386_INS_SECTION(call));
  /* have to be the same because the relocation is inserted in
   * OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)), and
   * OBJECT_RELOC_TABLE(INS_OBJECT(add)) is used to remove the
   * relocation again if the instruction is killed afterwards
   */
  ASSERT(INS_OBJECT(T_INS(mov))==CFG_OBJECT(cfg),("address producer object different from cfg object"));

  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressSub(RELOC_ADDENDS(rel)[0], RELOC_FROM_OFFSET(rel)),
      (t_relocatable *) mov,
      AddressNew32(1),
      NULL,
      AddressNew32(0),
      RELOC_HELL(rel),
      RELOC_EDGE(rel),
      NULL,
      NULL,
      "gA00+\\" WRITE_32
      );

  I386_INS_SET_OLD_SIZE(mov,  I386_INS_OLD_SIZE(call));
  I386_INS_SET_OLD_ADDRESS(mov,  I386_INS_OLD_ADDRESS(call));

  /* kill the call instruction */
  I386InstructionMakeNoop(call);
  /* turn the add instruction into a noop */
  I386InstructionMakeNoop(add);
  /* kill the call and return edges */
  if (CFG_EDGE_CORR(edge))
    CfgEdgeKill(CFG_EDGE_CORR(edge));
  CfgEdgeKill(edge);
  /* add a fallthrough edge between head and ret */
  CfgEdgeCreate(cfg,head,ret,ET_FALLTHROUGH);

  return TRUE;
}


static t_uint32
I386InsAddressHash(const void *voidins, const t_hash_table *ht)
{
  t_i386_ins *ins = T_I386_INS(voidins);
  return
    G_T_UINT32(I386_INS_OLD_ADDRESS(ins)) % HASH_TABLE_TSIZE(ht);
}


static t_int32
I386InsAddressCmp(const void *voidins1, const void *voidins2)
{
  t_ins *ins1, *ins2;

  ins1 = T_INS(voidins1);
  ins2 = T_INS(voidins2);
  /* return 0 if equal */
  return !AddressIsEq(INS_OLD_ADDRESS(ins1),INS_OLD_ADDRESS(ins2));
}


static void
I386InsAddressNodeFree(const void * he, void * data)
{
  Free(he);
}


void I386MakeAddressProducers(t_cfg *cfg)
{
  t_hash_table *calls_ht;
  t_ins *gins;

  BblMarkInit();

  /* collect all calls so we can look them up by address */
  calls_ht = HashTableNew(21011,0,I386InsAddressHash,I386InsAddressCmp,I386InsAddressNodeFree);
  CFG_FOREACH_INS(cfg,gins)
  {
    t_i386_ins *ins;

    ins = T_I386_INS(gins);
    if (I386_INS_OPCODE(ins) == I386_CALL)
    {
      t_hash_table_node *n;
      n = Calloc(1,sizeof(*n));
      HASH_TABLE_NODE_SET_KEY(n,ins);
      HashTableInsert(calls_ht,n);
    }
  }

  /* walk over all gotpc adds */
  CFG_FOREACH_INS(cfg,gins)
  {
    t_i386_ins *add;

    add = T_I386_INS(gins);
    /* look for all adds used to calculate the address of the GOT */
    if (I386_INS_OPCODE(add) == I386_ADD &&
        I386_OP_TYPE(I386_INS_DEST(add)) == i386_optype_reg &&
        I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_imm &&
        I386_INS_REFERS_TO(add))
    {
      t_reloc *rel;
      rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(add));
      if (strncmp(RELOC_CODE(rel),"gA00+P-\\",8)==0)
      {
        /* determine the address of the call that was used to get
         * the PC
         */
        t_address call_address;
        t_i386_ins hash_dummy;
        t_i386_ins *call;
        t_hash_table_node *res;

        /* get the return address of the call: since this "add" calculates
         * the address of the GOT by adding  "<address of the GOT> + A00 - P>"
         * to the return address of the call (which is in a register at this
         * point), this return address must be P - A00 (so that the result is
         * just the address of the GOT). P = the address of the "add" +
         * RELOC_FROM_OFFSET(rel).
         */
        call_address = AddressSub(AddressAdd(I386_INS_OLD_ADDRESS(add),RELOC_FROM_OFFSET(rel)),RELOC_ADDENDS(rel)[0]);
        /* the above was the return address of the call, and a call is
         * 5 bytes long
         */
        call_address = AddressSubUint32(call_address,5);
        /* look up the call */
        I386_INS_SET_OLD_ADDRESS(&hash_dummy,call_address);
        res = HashTableLookup(calls_ht,&hash_dummy);
        ASSERT(res,("Cannot find call at address @G based on add @I with rel @R",call_address,add,rel));
        call = T_I386_INS(HASH_TABLE_NODE_KEY(res));

        /* GLIBC:
         * we're looking for the following pattern:
         *    call <next ins>
         *    pop %reg
         *    add $offset, %reg
         */
        if (HandleCallToNextInsPic(cfg,call,add))
        {
          /* ok */
        }
        /* UCLIBC
         * a slightly different pattern:
         * call __i686.get_pc_thunk.bx
         * add $offset, %ebx
         *
         * where __i686.get_pc_thunk.bx looks like
         * mov (%esp), %ebx
         * ret
         */
        else if (HandleThunkCallPic(cfg,call,add))
        {
          /* ok */
        }
        else
          FATAL(("Unrecognised call @I corresponding to GOTPC add @I",call,add));
      }
    }
  }
  HashTableFree(calls_ht);
} /* }}}*/

/* {{{ patch __mpn_{add,sub}_n
 * these glibc functions use computed jumps in the code, which cannot
 * be represented reliably in a control flow graph. We fix this by
 * rewriting the code into a more traditional switch statement */
static void I386PatchMpnSwitches(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);
  t_bbl *bbl;
  t_i386_ins *ins;
  t_object *linker = ObjectGetLinkerSubObject(obj);

  /* we are looking for the following pattern:
   * lea <some code address>(<some index expression>),%reg
   * jmp *%reg
   */
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_reg jmpreg,indexreg;
    t_i386_operand *op;
    t_bbl *splits[8];
    t_bbl *base;
    t_section *subsec;
    int i;

    /* last ins has to be indirect jump */
    ins = T_I386_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;
    if (I386_INS_OPCODE(ins) != I386_JMP) continue;
    if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg) continue;
    jmpreg = I386_OP_BASE(I386_INS_SOURCE1(ins));

    /* next-to-last instruction has to be a lea */
    ins = I386_INS_IPREV(ins);
    if (!ins) continue;
    if (I386_INS_OPCODE(ins) != I386_LEA) continue;
    if (I386_OP_BASE(I386_INS_DEST(ins)) != jmpreg) continue;
    if (!I386_INS_REFERS_TO(ins)) continue;
    if (RELOC_N_TO_RELOCATABLES(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)))!=1) continue;
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)))[0]) != RT_BBL) continue;
    /* ignore tail calls to GOT entries */
    if (strchr(RELOC_CODE(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins))),'g')) continue;

    /* see if it's one of the constructs we recognize */
    op = I386_INS_SOURCE1(ins);
    if (!(I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)) FATAL(("Unrecognized"));
    indexreg = I386_OP_INDEX(op);
    if (indexreg != I386_OP_BASE(op)) FATAL(("Unrecognized"));
    if (indexreg == I386_REG_NONE) FATAL(("Unrecognized"));
    if (I386_OP_SCALE(op) != I386_SCALE_8) FATAL(("Unrecognized"));

    base = T_BBL(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)))[0]);
    /* the base block should contain at least 24 instructions */
    if (BBL_NINS(base) < 24) FATAL(("Unrecognized"));

    /* remove hell edge from bbl */
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(bbl)));

    /* create new data block for switch jump table */
    if (!OBJECT_NRODATAS(obj)) FATAL(("implement no rodatas in parent object"));
    subsec = SectionCreateForObject(linker,RODATA_SECTION,OBJECT_RODATA(obj)[0],
	AddressNew32(32),"__mpn_switch");

    /* split the block in smaller pieces */
    for (i=0; i<8; i++)
    {
      t_cfg_edge * edge = NULL;
      t_reloc * rel;
      splits[i] = base;
      base = BblSplitBlock(base,INS_INEXT(INS_INEXT(BBL_INS_FIRST(base))),FALSE);
      /* add a switch edge from bbl to splits[i] if i > 0 */
      if (i>0)
	{
	  edge = CfgEdgeCreate(cfg,bbl,splits[i],ET_SWITCH);
	  CFG_EDGE_SET_SWITCHVALUE(edge,  i);
	}
      /* add relocation in switch jump table */
      rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
	  AddressNew32(0),T_RELOCATABLE(subsec),
	  AddressNew32(4*i),
	  T_RELOCATABLE(splits[i]),AddressNew32(0),FALSE,NULL,
	  NULL,NULL,"R00A00+" "\\" WRITE_32);
 
      if (i>0)
	{
	  RELOC_SET_SWITCH_EDGE(rel, edge);
	}
   }

    /* TODO lea -> mov */
    I386InstructionMakeMovFromMem(ins,jmpreg,0,I386_REG_NONE,indexreg,I386_SCALE_4);
    op = I386_INS_SOURCE1(ins);
    I386_OP_FLAGS(op) |= I386_OPFLAG_ISRELOCATED;
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
	AddressNew32(0),T_RELOCATABLE(ins),
	AddressNew32(3),
	T_RELOCATABLE(subsec),AddressNew32(0),FALSE,NULL,
	NULL,NULL, "R00A00+" "\\" WRITE_32);
  }
}

/* Add a call between these BBL's by splitting the from BBL. Is used by CfgAddSelfProfiling */
void I386AddCallFromBblToBbl (t_object* obj, t_bbl* from, t_bbl* to)
{
  t_i386_ins* ins;
  t_bbl* split = BblSplitBlock (from, T_INS(BBL_INS_FIRST(from)), TRUE);
  CfgEdgeKill (BBL_SUCC_FIRST(from));/* A fallthrough edge to split has been created by BblSplitBlock, remove it */
  CfgEdgeCreateCall (OBJECT_CFG(obj), from, to, split, NULL);/* Create call edge with corresponding return edge to split */
  I386MakeInsForBbl(Call, Append, ins, from);
}

/* Add instrumentation to the beginning of the BBL. This is one instruction that
 * increments a certain memory location every time the BBL is executed. Used by CfgAddSelfProfiling
 */
void I386AddInstrumentationToBbl (t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_section* sequencing_counter_sec, t_address offset)
{
  t_i386_ins* ins, *tmp;
  t_i386_ins* orig_first = T_I386_INS(BBL_INS_FIRST(bbl));
  t_cfg_edge* edge;
  static t_regset regset_flags, possible;
  static t_bool initialized = FALSE;
  t_bool pushPopF = FALSE;
  offset = AddressAddUint32(offset, sizeof(t_uint64));

  /* Initialize some regsets we use */
  if (!initialized)
  {
    /* This one contains all flags that may be set by the inc instruction.. */
    regset_flags = RegsetNew();
    RegsetSetAddReg(regset_flags, I386_CONDREG_OF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_SF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_AF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_PF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_ZF);

    /* ..or by the add instruction (used in case of PIC code). */
    RegsetSetAddReg(regset_flags, I386_CONDREG_CF);

    /* This one contains all general purpose registers we can use. */
    possible = RegsetNew();
    RegsetSetAddReg(possible, I386_REG_EAX);
    RegsetSetAddReg(possible, I386_REG_EBX);
    RegsetSetAddReg(possible, I386_REG_ECX);
    RegsetSetAddReg(possible, I386_REG_EDX);
    RegsetSetAddReg(possible, I386_REG_ESI);
    RegsetSetAddReg(possible, I386_REG_EDI);

    initialized = TRUE;
  }

  /* If BBL is a PcThunk, don't instrument */
  if (I386IsPcThunk(bbl))
    return;

  /* If BBL calls a PcThunk, don't instrument */
  BBL_FOREACH_SUCC_EDGE(bbl, edge)
    if (CfgEdgeTestCategoryOr(edge, ET_CALL) && I386IsPcThunk(CFG_EDGE_TAIL(edge)))
      return;

  /* If the flags possibly set by the inc instruction are live-in, the EFLAGS register must be
   * pushed and popped
   */
  pushPopF = !RegsetIsEmpty(RegsetIntersect(BblRegsLiveBefore(bbl), regset_flags));
  if (pushPopF)
    I386MakeInsForBbl(PopF, Prepend, ins, bbl);

  /* The way we increment the right entry depends on whether the code is PIC or not */
  if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
  {
    t_reg inc_reg = I386_REG_NONE;
    t_regset available = RegsetDiff(possible, BblRegsLiveBefore(bbl));

    /* If we find a dead register we can use it, else we'll need to push and pop one */
    t_bool push_needed = TRUE;
    REGSET_FOREACH_REG(available, inc_reg)
    {
      push_needed = FALSE;
      break;
    }

    if(push_needed)
    {
      inc_reg = I386_REG_EAX;
      I386MakeInsForBbl(Pop, Prepend, ins, bbl, inc_reg);
    }

    /* Make the instructions we actually need:
     *    call <next_ins>
     *    pop  <inc_reg>
     *    add  <inc_reg>, <offset_to_profiling_section_entry>
     *    inc [<inc_reg>]
     */
    I386MakeInsForBbl(IncDec, Prepend, ins, bbl, I386_INC, inc_reg);
    I386OpSetMem(I386_INS_DEST(ins), 0, inc_reg, I386_REG_NONE, 0, sizeof(t_uint32));
    I386MakeInsForBbl(Arithmetic, Prepend, ins, bbl, I386_ADD, inc_reg, I386_REG_NONE, 0);
    I386_OP_FLAGS(I386_INS_SOURCE1(ins)) |= I386_OPFLAG_ISRELOCATED;
    I386MakeInsForBbl(Pop, Prepend, tmp, bbl, inc_reg);

    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
      AddressNullForObject(obj),
      T_RELOCATABLE(ins),
      AddressNew32(1), /* Offset of immediate within add instruction */
      T_RELOCATABLE(profiling_sec),
      offset,
      FALSE,
      NULL,
      NULL,
      T_RELOCATABLE(tmp),/* We subtract the address of the pop instruction from the address of the profiling section entry */
      "R00A00+R01-\\l*w\\s0000$");

    I386MakeInsForBbl(Call, Prepend, ins, bbl);/* Immediate is set to 0, so this is a call to the next instruction */

    if(push_needed)
      I386MakeInsForBbl(Push, Prepend, ins, bbl, inc_reg, 0);
  }
  else
  {
    I386MakeInsForBbl(IncDec, Prepend, ins, bbl, I386_INC, I386_REG_EAX );
    I386OpSetMem(I386_INS_DEST(ins), 0, I386_REG_NONE, I386_REG_NONE, 0, sizeof(t_uint32));
    I386_OP_FLAGS(I386_INS_DEST(ins)) |= I386_OPFLAG_ISRELOCATED;
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
      AddressNullForObject(obj),
      T_RELOCATABLE(ins),
      2,/* Offset of memory operand within inc instruction */
      T_RELOCATABLE(profiling_sec),
      offset,
      FALSE,
      NULL,
      NULL,
      NULL,
      "R00A00+\\l*w\\s0000$");
  }

  if (pushPopF)
    I386MakeInsForBbl(PushF, Prepend, ins, bbl);

  /* We need to check if the BBL is on the return edge from x86.get_pc_thunk. If so there is a
   * relocation on the first original (ADD) instruction of the BBL. We will replace this relocation with
   * another one that takes into account instructions have been added before the add instruction that makes
   * use of the pc to calculate something in position-independent way.
   */
  BBL_FOREACH_PRED_EDGE(bbl, edge)
    if (CfgEdgeTestCategoryOr(edge, ET_RETURN))
      break;
  if(edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(edge));
    if(I386IsPcThunk(FUNCTION_BBL_FIRST(BBL_FUNCTION(head)))
      && (I386_INS_OPCODE(orig_first) == I386_ADD))
    {
      t_reloc* rel = RELOC_REF_RELOC(INS_REFERS_TO(T_INS(orig_first)));

      /* Add a piece to the relocation code so the offset of the add instruction within its BBL is calculated
       * and added to the immediate that is the result of this relocation.
       */
      t_string new_code = StringConcat2(RELOC_CODE(rel), "PR01-+");

      RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
        RELOC_ADDENDS(rel)[0],
        RELOC_FROM(rel),
        RELOC_FROM_OFFSET(rel),
        RELOC_TO_RELOCATABLE(rel)[0],
        RELOC_TO_RELOCATABLE_OFFSET(rel)[0],
        RELOC_HELL(rel),
        RELOC_EDGE(rel),
        NULL,
        T_RELOCATABLE(bbl),/* We add another relocatable, the BBL */
        new_code);

      Free(new_code);
      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj), rel);
    }
  }
}/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
