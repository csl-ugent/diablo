#include <diabloamd64.h>
#include <string.h>

static void Amd64PatchMpnSwitches(t_object *obj);
static void Amd64patchcalculatedjumps(t_object *obj);
static t_uint32 Amd64FindBBLLeaders(t_object * obj);
static t_uint32 Amd64AddBasicBlockEdges(t_object * obj);

void Amd64Flowgraph(t_object *obj)
{  
  t_uint32 ret;
  t_cfg *cfg = OBJECT_CFG(obj);

  /* Find the leaders in the instruction list */
  STATUS(START,("Leader detection"));
  ret=Amd64FindBBLLeaders(obj);
  STATUS(STOP,("Leader detection"));

  /* Create the basic blocks (platform independent) */
  STATUS(START,("Creating Basic Blocks"));
  ret=CfgCreateBasicBlocks(obj);
  STATUS(STOP,("Creating Basic Blocks"));

  /* Create the edges between basic blocks */
  STATUS(START,("Creating Basic Block graph"));
  ret=Amd64AddBasicBlockEdges(obj);
  STATUS(STOP,("Creating Basic Block graph"));

  /* {{{ kill all relocations from the immediate operands of jump and call instructions: 
   * they have no use any more because their information is represented by flow graph edges */
  {
    t_bbl * bbl;
    t_amd64_ins * ins;
    CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_FOREACH_AMD64_INS(bbl,ins)
      {
	if (AMD64_INS_OPCODE(ins) == AMD64_CALL || AMD64_INS_OPCODE(ins) == AMD64_JMP || AMD64_INS_OPCODE(ins) == AMD64_Jcc)
	{
	  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
	  {
	    while (AMD64_INS_REFERS_TO(ins))
	      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)));
	    AMD64_OP_FLAGS(AMD64_INS_SOURCE1(ins)) &= ~AMD64_OPFLAG_ISRELOCATED;
	  }
	}
      }
    }
  } /* }}} */

  /* search calculated jumps and fix flowgraph */
  Amd64patchcalculatedjumps(obj);
  
  /* patch the flowgraph representation of the __mpn_add_n and 
   * __mpn_sub_n functions of glibc: they use code address computations,
   * which cannot be reliably modelled in a conventional flow graph */
  Amd64PatchMpnSwitches(obj);
}

void Amd64StucknessAnalysis(t_object * obj)
{
  /*{{{*/
  t_cfg * cfg = OBJECT_CFG(obj);
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
	
	
	BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge1,edge2)
	{
	  if(CFG_EDGE_CAT(edge1)==ET_FALLTHROUGH && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == AMD64_Jcc)
	  {
	    Amd64InstructionMakeJump(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    AMD64_INS_SET_CONDITION(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))), AMD64_CONDITION_NONE);
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  
	  else if(CFG_EDGE_CAT(edge1)==ET_JUMP && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == AMD64_Jcc)
	  {
	    Amd64InstructionMakeNoop(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_FALLTHROUGH && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && !Amd64InsIsControlTransfer(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))))
	  {
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  else if(CFG_EDGE_CAT(edge1)==ET_JUMP && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == AMD64_JMP)
	  {
	    Amd64InstructionMakeNoop(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    CfgEdgeKill(edge1);
	    change = TRUE;
	  }
	  
	  else if(CFG_EDGE_CAT(edge1)==ET_CALL && BBL_INS_LAST(CFG_EDGE_HEAD(edge1)) && AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1)))) == AMD64_CALL)
	  {
	    Amd64InstructionMakeNoop(T_AMD64_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge1))));
	    if(CFG_EDGE_CORR(edge1))
	      CfgEdgeKill(CFG_EDGE_CORR(edge1));
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

/* {{{ Amd64AddBasicBlockEdges */
static t_uint32 Amd64AddBasicBlockEdges(t_object * obj)
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
   * should have been a call edge), then chances are that your flowgraph (and
   * thus the produced binary) is wrong (the successor block of the call block
   * will have no incoming (return) edge, and thus this block is dead). 
   *
   * The solution is to look at code that is produced by your compiler(s).
   * It'll probably have only a few possibilties to produce calls. If you
   * handle them as safe as possible, than you're off the hook.  */
  t_uint32 nedges=0; 
  int i;
  t_section *section;
  OBJECT_FOREACH_CODE_SECTION(obj, section, i)
  {
    t_amd64_ins *ins, *iter,  *block_end, *block_start, *prev, *prev_controltransfer;
    t_bbl * block, * ft_block;
    t_cfg * flowgraph = OBJECT_CFG(obj);
    t_reloc_ref * ref;

    /* {{{ first detect switch tables and add the necessary switch edges.
     * we don't have to do this, but otherwise a lot of hell edges will
     * be added that substantially degrade the accuracy and performance of a 
     * number of analyses */

    /* Switch table; first possibility {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = AMD64_INS_INEXT(ins))
    {
      /* switch constructs have the following pattern:
       *   cmp $ubound, %reg
       *   ...
       *   ja <default_case>
       *   ...
       *   jmp *<jumptable_base>(,%reg,4)
       */

      t_amd64_operand * op = AMD64_INS_SOURCE1(ins);
      t_reg jumpreg;
      t_reloc * jumptablerel;
      t_uint64 bound;
      t_regset changed;
      t_amd64_ins * cmp;
      t_reloc_ref * rr;

      if (AMD64_INS_IPREV(ins))
        prev = AMD64_INS_IPREV(ins);
      if (prev && Amd64InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      if (!prev_controltransfer || (AMD64_INS_OPCODE(prev_controltransfer) != AMD64_Jcc)) continue;
      if (AMD64_INS_CONDITION(prev_controltransfer) != AMD64_CONDITION_A) continue;
      if (AMD64_INS_OPCODE(ins) != AMD64_JMP) continue;
      if (AMD64_OP_TYPE(op) != amd64_optype_mem) continue;
      if (AMD64_OP_BASE(op) != AMD64_REG_NONE) continue;
      if (AMD64_OP_SCALE(op) != AMD64_SCALE_8) FATAL(("unexpected scale in switch jump @I",ins));
      jumpreg = AMD64_OP_INDEX(op);

      /* check to see if the jumpreg isn't changed between the ja and the jmp */
      changed = NullRegs;
      for (iter = AMD64_INS_IPREV(ins); iter != prev_controltransfer; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (RegsetIn(changed,jumpreg))
        continue;

      if (!INS_REFERS_TO(BBL_INS_LAST(AMD64_INS_BBL(ins)))) FATAL(("could not find jumptable relocation for @I",ins));
      jumptablerel = RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(AMD64_INS_BBL(ins))));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;

      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = AMD64_INS_IPREV(prev_controltransfer); cmp; cmp = AMD64_INS_IPREV(cmp))
      {
        if (AMD64_INS_OPCODE(cmp) == AMD64_CMP && 
            AMD64_OP_TYPE(AMD64_INS_SOURCE1(cmp)) == amd64_optype_reg &&
            AMD64_OP_BASE(AMD64_INS_SOURCE1(cmp)) == jumpreg &&
            AMD64_OP_TYPE(AMD64_INS_SOURCE2(cmp)) == amd64_optype_imm)
          break;

        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(cmp));

        if (Amd64InsIsControlTransfer(cmp) || RegsetIn(changed,jumpreg) ||
            RegsetIn(changed,AMD64_CONDREG_CF) || RegsetIn(changed,AMD64_CONDREG_ZF))
        {
          cmp = NULL;
          break;
        }
      }
      if (!cmp) continue;

      bound = AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE2(cmp));
      ASSERT(G_T_UINT64(SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0]))) >= 4*(bound+1), ("jump table too small for switch bound"));

      for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
      {
        t_reloc * rel = RELOC_REF_RELOC(rr);
        if (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
            AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],4*bound)))
        {
          t_cfg_edge * edge;
          RELOC_SET_HELL(rel, FALSE);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
          edge = CfgEdgeCreate(flowgraph,AMD64_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
          CFG_EDGE_SET_REL(edge,  rel);
          RELOC_SET_SWITCH_EDGE(rel, edge);
          CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT64(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
        }
      }
    }
    /* }}} */

    /* Switch table; first possibility (in uclibc) {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = AMD64_INS_INEXT(ins))
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
      t_uint64 bound;
      t_regset changed;
      t_amd64_ins * cmp, *mov;
      t_reloc_ref * rr;

      if (AMD64_INS_IPREV(ins))
        prev = AMD64_INS_IPREV(ins);
      if (prev && Amd64InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      /* chack: ja <default_case> {{{*/
      if (!prev_controltransfer || (AMD64_INS_OPCODE(prev_controltransfer) != AMD64_Jcc)) continue;
      if (AMD64_INS_CONDITION(prev_controltransfer) != AMD64_CONDITION_A) continue;
      /* }}} */
      /* check: jmp *<jumptable_base>(,%reg,4) {{{ */
      if (AMD64_INS_OPCODE(ins) != AMD64_JMP) continue;
      if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_mem) continue;
      if (AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) != AMD64_REG_NONE) continue;
      if (AMD64_OP_SCALE(AMD64_INS_SOURCE1(ins)) != AMD64_SCALE_8) FATAL(("unexpected scale in switch jump @I",ins));
      /* }}} */
      /* Check  mov  off(%esp),%reg {{{ */
      for(mov=ins;mov!=NULL;mov=AMD64_INS_IPREV(mov))
      {
        if(AMD64_INS_OPCODE(mov) == AMD64_MOV)
        {
          if(AMD64_OP_BASE(AMD64_INS_DEST(mov))==AMD64_OP_INDEX(AMD64_INS_SOURCE1(ins)))
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
      for (iter = AMD64_INS_IPREV(ins); iter != mov; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (RegsetIn(changed,AMD64_OP_INDEX(AMD64_INS_SOURCE1(ins))))
        continue;
      /* }}} */

      if (!AMD64_INS_REFERS_TO(T_AMD64_INS(BBL_INS_LAST(AMD64_INS_BBL(ins))))) FATAL(("could not find jumptable relocation for @I",ins));
      jumptablerel = RELOC_REF_RELOC(AMD64_INS_REFERS_TO(T_AMD64_INS(BBL_INS_LAST(AMD64_INS_BBL(ins)))));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;

      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = AMD64_INS_IPREV(prev_controltransfer); cmp; cmp = AMD64_INS_IPREV(cmp))
      {
        if (AMD64_INS_OPCODE(cmp) == AMD64_CMP && 
            AMD64_OP_TYPE(AMD64_INS_SOURCE1(cmp)) == amd64_optype_mem &&
            AMD64_OP_TYPE(AMD64_INS_SOURCE2(cmp)) == amd64_optype_imm)
          break;

        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(cmp));

        if (Amd64InsIsControlTransfer(cmp) || RegsetIn(changed,AMD64_CONDREG_CF) || RegsetIn(changed,AMD64_CONDREG_ZF))
        {
          cmp = NULL;
          break;
        }
      }
      if (!cmp) continue;

      bound = AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE2(cmp));
      ASSERT(G_T_UINT64(SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0]))) >= 4*(bound+1), ("jump table too small for switch bound"));

      for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
      {
        t_reloc * rel = RELOC_REF_RELOC(rr);
        if (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
            AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAddUint64(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],4*bound)))
        {
          t_cfg_edge * edge;
          RELOC_SET_HELL(rel, FALSE);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
          edge = CfgEdgeCreate(flowgraph,AMD64_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
          CFG_EDGE_SET_REL(edge,  rel);
          RELOC_SET_SWITCH_EDGE(rel, edge);
          CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT64(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
        }
      }
    }
    /* }}} */

    /* Switch table; first possibility (in uclibc) {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = AMD64_INS_INEXT(ins))
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
      t_uint64 bound;
      t_regset changed;
      t_amd64_ins * cmp, *lea, *mov, *add, *h_ins;
      t_reloc_ref * rr;

      if (AMD64_INS_IPREV(ins))
        prev = AMD64_INS_IPREV(ins);
      if (prev && Amd64InsIsControlTransfer(prev))
        prev_controltransfer = prev;

      /* Check: ja     <default_case> {{{*/
      if (!prev_controltransfer || (AMD64_INS_OPCODE(prev_controltransfer) != AMD64_Jcc)) continue;
      if (AMD64_INS_CONDITION(prev_controltransfer) != AMD64_CONDITION_A) continue;
      /* }}} */
      /* Check: jmp    *%reg4 {{{ */
      if (AMD64_INS_OPCODE(ins) != AMD64_JMP) continue;
      if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg) continue;
      if (AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_NONE) continue;
      /* }}} */ 
      /* Check add    %reg2,%reg4  {{{ */
      for(add=ins;add!=NULL;add=AMD64_INS_IPREV(add))
      {
        if(AMD64_INS_OPCODE(add) == AMD64_ADD)
        {
          if(AMD64_OP_BASE(AMD64_INS_DEST(add))==AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)))
            break;
        }
        else if (add==prev_controltransfer)
          break;
      }
      if (add==prev_controltransfer)
        continue;
      /* }}} */
      /* Check  mov    (%reg3,%reg1,4),%reg4 {{{ */
      for(mov=add;mov!=NULL;mov=AMD64_INS_IPREV(mov))
      {
        if(AMD64_INS_OPCODE(mov) == AMD64_MOV)
        {
          if(AMD64_OP_BASE(AMD64_INS_DEST(mov))==AMD64_OP_BASE(AMD64_INS_DEST(add)))
            break;
        }
        else if (mov==prev_controltransfer)
          break;
      }
      if (mov==prev_controltransfer)
        continue;
      /* }}} */

      /* Check lea    off(%reg2),%reg3 {{{ */
      for(lea=mov;lea!=NULL;lea=AMD64_INS_IPREV(lea))
      {
        if(AMD64_INS_OPCODE(lea) == AMD64_LEA)
        {
          if(AMD64_OP_BASE(AMD64_INS_DEST(lea))==AMD64_OP_BASE(AMD64_INS_SOURCE1(mov)) &&
              AMD64_OP_BASE(AMD64_INS_SOURCE1(lea))==AMD64_OP_BASE(AMD64_INS_SOURCE1(add)))
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
      for (iter = AMD64_INS_IPREV(ins); iter != add; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (RegsetIn(changed,AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))))
        continue;
      changed = NullRegs;
      for (iter = AMD64_INS_IPREV(add); iter != mov; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (RegsetIn(changed,AMD64_OP_BASE(AMD64_INS_DEST(add))))
        continue;

      /* reg3 */
      changed = NullRegs;
      for (iter = AMD64_INS_IPREV(mov); iter != lea; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (RegsetIn(changed,AMD64_OP_BASE(AMD64_INS_DEST(lea))))
        continue;

      for(h_ins=T_AMD64_INS(BBL_INS_LAST(AMD64_INS_BBL(ins)));h_ins!=NULL;h_ins=AMD64_INS_IPREV(h_ins))
      {
        if(AMD64_INS_REFERS_TO(h_ins) && RELOC_REF_RELOC(AMD64_INS_REFERS_TO(h_ins)))
          if(AMD64_INS_OPCODE(h_ins)==AMD64_LEA)
            break;
        if(h_ins ==prev_controltransfer)
          break;
      }
      if(h_ins ==prev_controltransfer)
        continue;
      /* }}} */

      if (!AMD64_INS_REFERS_TO(h_ins)) FATAL(("could not find jumptable relocation for @I",h_ins));
      jumptablerel = RELOC_REF_RELOC(AMD64_INS_REFERS_TO(h_ins));
      if (RELOC_N_TO_RELOCATABLES(jumptablerel) != 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;


      /* find compare instruction that indicates the switch bound */
      changed = NullRegs;
      for (cmp = AMD64_INS_IPREV(prev_controltransfer); cmp; cmp = AMD64_INS_IPREV(cmp))
      {
        if (AMD64_INS_OPCODE(cmp) == AMD64_CMP && 
            AMD64_OP_TYPE(AMD64_INS_SOURCE1(cmp)) == amd64_optype_reg &&
            AMD64_OP_BASE(AMD64_INS_SOURCE1(cmp)) == AMD64_OP_INDEX(AMD64_INS_SOURCE1(mov)) &&
            AMD64_OP_TYPE(AMD64_INS_SOURCE2(cmp)) == amd64_optype_imm)
          break;
      }

      /*  check to see if the registers aren't changed between the initialisation of the register and the use of it : reg1 {{{*/
      changed = NullRegs;
      for (iter = AMD64_INS_IPREV(mov); iter != cmp; iter = AMD64_INS_IPREV(iter))
        RegsetSetUnion(changed, AMD64_INS_REGS_DEF(iter));
      if (Amd64InsIsControlTransfer(cmp) || RegsetIn(changed,AMD64_OP_BASE(AMD64_INS_SOURCE1(cmp))) ||
          RegsetIn(changed,AMD64_CONDREG_CF) || RegsetIn(changed,AMD64_CONDREG_ZF))
        continue;
      /* }}} */

      if (!cmp) continue;

      bound = AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE2(cmp));
      ASSERT(G_T_UINT64(SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0]))) >= 4*(bound+1), ("jump table too small for switch bound"));

      for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
      {
        t_reloc * rel = RELOC_REF_RELOC(rr);
        if (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
            AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAddUint64(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],4*bound)))
        {
          t_cfg_edge * edge;
          RELOC_SET_HELL(rel, FALSE);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
          edge = CfgEdgeCreate(flowgraph,AMD64_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
          CFG_EDGE_SET_REL(edge,  rel);
          RELOC_SET_SWITCH_EDGE(rel, edge);
          CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT64(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);
        }
      }
    } 
    /* }}} */

    /* }}} */

    /* walk through the original(!) array (we need offsets) and add
     * all possible successors to every node */

    for (ins = SECTION_DATA(section); ins != NULL; ins = AMD64_INS_INEXT(ins))
    {
      /* we're only interested in the first and last instructions of a basic block */
      if (!(AMD64_INS_ATTRIB(ins) & IF_BBL_LEADER)) 
        continue;

      block_start = ins;
      block=AMD64_INS_BBL(block_start);

      block_end = block_start;
      iter = AMD64_INS_INEXT(block_end);
      while (iter)
      {
        if (AMD64_INS_ATTRIB(iter) & IF_BBL_LEADER)
          break;
        else
          block_end = iter;
        iter = AMD64_INS_INEXT(iter);
      }
      /* block_end now points to the last "real" instruction of the block in the linear list,
       * iter points to the first instruction of the next block (or NULL if there is no next block) */
      ft_block = iter ? AMD64_INS_BBL(iter) : NULL;

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
            if (Amd64InsIsControlTransfer((t_amd64_ins *)RELOC_FROM(RELOC_REF_RELOC(ref))))
            {
              RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);
              continue;
            }
          }

          /* check if a hell edge already exists */
          BBL_FOREACH_PRED_EDGE(block,i_edge)
            if ((CFG_EDGE_HEAD(i_edge)==CFG_HELL_NODE(flowgraph))&&(CFG_EDGE_CAT(i_edge)==ET_CALL))
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
            RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall(flowgraph, CFG_HELL_NODE(flowgraph), block,NULL,NULL));
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
      if (Amd64InsIsControlTransfer((t_amd64_ins *)block_end))
      {
        if (Amd64InsIsConditional(block_end))
        {
          /* add fallthrough edge */
          ASSERT(ft_block && !IS_DATABBL(ft_block),("conditional instruction but non-suitable fallthrough block\nblock @iBft_block @iB", block, ft_block));
          CfgEdgeCreate(flowgraph, block, ft_block, ET_FALLTHROUGH);
          nedges++;
        }

        switch (AMD64_INS_OPCODE(block_end))
        {
          case AMD64_JMP:
          case AMD64_Jcc:
          case AMD64_JRCXZ:
            {
              if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(block_end)) == amd64_optype_imm)
              {
                /* we can determine the jump destination */
                t_bbl * target;
                t_amd64_ins * target_ins;
                t_address dest = AddressAdd(AMD64_INS_CSIZE(block_end),AMD64_INS_CADDRESS(block_end));
                dest = AddressAddUint64(dest,AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(block_end)));
                target_ins = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
                if (target_ins)
                {
                  target = AMD64_INS_BBL(target_ins);
                  CfgEdgeCreate(flowgraph,block,target,ET_JUMP);
                  nedges++;
                }
                else
                {
                  /*ASSERT(AddressIsNull(dest),("could not find destination of @I dest @G",block_end, dest));*/
                  if(!AddressIsNull(dest)){
                    VERBOSE(0,("no destination for @I dest @G!",block_end,dest));
                  }

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
                  Amd64InstructionMakeNoop(block_end);
                  Amd64InstructionMakeNoop(T_AMD64_INS(BBL_INS_LAST(AMD64_INS_BBL(block_end))));
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
                  CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
                  nedges++;
                }
              }
            }
            break;
          case AMD64_CALL:
            {
              t_bool nolink = !ft_block || IS_DATABBL(ft_block);

              if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(block_end)) == amd64_optype_imm)
              {
                /* we can determine the call destination */
                t_bbl * target;
                t_amd64_ins * target_ins;
                t_address dest = AddressAdd(AMD64_INS_CSIZE(block_end),AMD64_INS_CADDRESS(block_end));
                dest = AddressAddUint64(dest,AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(block_end)));
                target_ins = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
                if (target_ins)
                {
                  target = AMD64_INS_BBL(target_ins);
                  if (!nolink)
                  {
                    CfgEdgeCreateCall(flowgraph, block, target, ft_block, NULL);
                    nedges++;
                  }
                  else
                  {
                    /* if the function return would end up in data or out of the code section,
                     * we can't add a link edge. Therefore we assume the call is to a non-returning
                     * function and so we can just as well model it with a jump edge */
                    CfgEdgeCreate(flowgraph,block,target,ET_JUMP);
                    nedges++;
                  }
                }
                else
                {
                  /*ASSERT(AddressIsNull(dest),("could not find destination of @I",block_end));    */
                  if(!AddressIsNull(dest)){
                    VERBOSE(0,("no destination for @I!",block_end));
                  }


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
                  Amd64InstructionMakeNoop(block_end);
                  Amd64InstructionMakeNoop(T_AMD64_INS(BBL_INS_LAST(AMD64_INS_BBL(block_end))));
                }
              }
              else
              {
                /* destination unknown: add a hell edge */
                if (!nolink)
                {
                  CfgEdgeCreateCall(flowgraph,block,CFG_HELL_NODE(flowgraph),ft_block,NULL);
                  nedges++;
                }
                else
                {
                  CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
                  nedges++;
                }
              }
            }
            break;
          case AMD64_LOOP:
          case AMD64_LOOPZ:
          case AMD64_LOOPNZ:
            {
              t_amd64_ins * target_ins;
              t_bbl * target;
              t_address dest = AddressAdd(AMD64_INS_CADDRESS(block_end),AMD64_INS_CSIZE(block_end));
              dest = AddressAddUint64(dest,AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(block_end)));
              target_ins = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
              ASSERT(target_ins,("Could not find destination for @I",block_end));
              target = AMD64_INS_BBL(target_ins);
              CfgEdgeCreate(flowgraph,block,target,ET_JUMP);
              nedges++;
            }
            break;
          case AMD64_RET:
            /* add a return edge that at the moment points to hell */
            CfgEdgeCreate(flowgraph,block,CFG_EXIT_HELL_NODE(flowgraph),ET_RETURN);
            nedges++;
            break;
          case AMD64_INT:
          case AMD64_INTO:
          case AMD64_INT3:
            {
              t_cfg_edge * swi_edge;
              swi_edge = CfgEdgeCreateSwi(flowgraph,block,ft_block);
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
          case AMD64_UD2:
            /* ud2 causes an illegal opcode trap, so this actually jumps to hell.
             * furthermore, check if there is a data block following this instruction.
             * if so, it should be marked, as this data is associated with the ud2
             * instruction (for debugging purposes in the linux kernel) */
            CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
            if (ft_block && IS_DATABBL(ft_block))
            {
              /* add relocation to this block, so it won't be removed by
               * CfgRemoveDeadCodeAndData() */
              RelocTableAddRelocToRelocatable(
                  OBJECT_RELOC_TABLE(obj),
                  AddressNew64(0),
                  T_RELOCATABLE(BBL_INS_LAST(block)),
                  AddressNew64(0),
                  T_RELOCATABLE(ft_block),
                  AddressNew64(0),
                  FALSE, NULL, NULL, NULL,
                  CALC_ABS "\\\\s0000$"
                  );
            }
            break;
          case AMD64_BOUND:
            /* for all these strange instructions, we just add a jump edge to hell
             * (as the control flow may be altered in inpredictible ways), and a 
             * fallthrough edge to be on the safe side */
            CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
            nedges++;
            if (ft_block && !IS_DATABBL(ft_block))
            {
              CfgEdgeCreate(flowgraph,block,ft_block,ET_FALLTHROUGH);
              nedges++;
            }
            break;
          case AMD64_HLT:
            /* for a hlt instruction, we assume the program will never be
             * resumed.  therefore, it suffices to add a jump edge back to the
             * beginning of the block (the hlt instruction will almost certainly
             * be alone in a block */
            /* TODO for kernels, we will have to forego this assumption, as the
             * hlt instruction is used within the idle task */
            if(DiabloFlowgraphInKernelMode())
              CfgEdgeCreate(flowgraph,block,ft_block,ET_FALLTHROUGH);
            else 
              CfgEdgeCreate(flowgraph,block,block,ET_JUMP);
            nedges++;
            break;
          case AMD64_IRET:
          case AMD64_SYSEXIT:
            /* add a hell edge... a correct modeling will be decided upon later */
            CfgEdgeCreate(flowgraph,block,CFG_EXIT_HELL_NODE(flowgraph),ET_RETURN);
            break;
          case AMD64_JMPF:
            if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(block_end)) == amd64_optype_mem)
            {
              CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
              nedges++;
            }
            else
            {
              /* destination given as an immediate operand. assume the destination
               * code segment is the same as the current code segment, because I
               * wouldn't know what to do otherwise :-( */
              t_address dest;
              t_amd64_ins * target;
              t_bbl * target_block;
              dest=AddressNew64(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(block_end)));
              target = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
              if (target)
              {
                target_block = AMD64_INS_BBL(target);
                CfgEdgeCreate(flowgraph,block,target_block,ET_JUMP);
                nedges++;
              }
              else
              {
                /* if we can't find the destination, it's supposedly a jump to 
                 * a different segment, so we just add a hell edge */
                CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
                nedges++;
              }
            }
            break;
          case AMD64_CALLF:
            {
              t_bool nolink = !ft_block || IS_DATABBL(ft_block);

              if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(block_end)) == amd64_optype_mem)
              {
                /* add hell edge */
                if (!nolink)
                {
                  CfgEdgeCreateCall(flowgraph,block,CFG_HELL_NODE(flowgraph),ft_block,NULL);
                  nedges++;
                }
                else
                {
                  CfgEdgeCreate(flowgraph,block,CFG_HELL_NODE(flowgraph),ET_JUMP);
                  nedges++;
                }
              }
              else
              {
                /* destination given as an immediate operand. assume the destination
                 * code segment is the same as the current code segment, because I
                 * wouldn't know what to do otherwise :-( */
                t_address dest;
                t_amd64_ins * target;
                t_bbl * target_block;
                dest=AddressNew64(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(block_end)));
                target = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
                if (target)
                {
                  target_block = AMD64_INS_BBL(target);

                  if (!nolink)
                  {
                    CfgEdgeCreateCall(flowgraph,block,target_block,ft_block,NULL);
                    nedges++;
                  }
                  else
                  {
                    CfgEdgeCreate(flowgraph,block,target_block,ET_JUMP);
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
          case AMD64_RETF:
            /* add a return edge that at the moment points to hell */
            CfgEdgeCreate(flowgraph,block,CFG_EXIT_HELL_NODE(flowgraph),ET_RETURN);
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
          CfgEdgeCreate(flowgraph, block, ft_block, ET_FALLTHROUGH);
      }
    }
  }
  return nedges;
}
/* }}} */

/* {{{ Amd64FindBBLLeaders */
static t_uint32 Amd64FindBBLLeaders(t_object * obj)
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
  t_section *code;
  int i;
  t_uint32 nleaders=0;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_amd64_ins * ins, * next;
    t_uint64 pc;

    /* Instructions are still in a sequential list, stored in t_section->data */
    ins = T_AMD64_INS(SECTION_DATA(code)); 
    AMD64_INS_SET_ATTRIB(ins, AMD64_INS_ATTRIB(ins) | IF_BBL_LEADER);

    for (pc = 0; ins != NULL; pc += G_T_UINT64(AMD64_INS_CSIZE(ins)), ins = AMD64_INS_INEXT(ins))
    {
      if (!ins) break;

      /* the entry point is always a leader */
      if (AddressIsEq(AMD64_INS_CADDRESS(ins),OBJECT_ENTRY(obj)))
        AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) | IF_BBL_LEADER);

      /* reason 4: ins is the start of a data block */
      if (AMD64_INS_TYPE(ins) == IT_DATA)
      {
        if (AMD64_INS_IPREV(ins) && (AMD64_INS_TYPE(AMD64_INS_IPREV(ins)) != IT_DATA))
          AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) | IF_BBL_LEADER);
        continue;
      }
      /* reason 4b: ins is the first after a data block */
      if (AMD64_INS_IPREV(ins) && (AMD64_INS_TYPE(AMD64_INS_IPREV(ins)) == IT_DATA))
      {
        AMD64_INS_SET_ATTRIB(ins,   AMD64_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }

      /* reason 3b: ins is pointed to by a relocation */
      if (AMD64_INS_REFED_BY(ins))
      {
        AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }

      /* reason 2: ins following a control transfer */
      if (Amd64InsIsControlTransfer((t_amd64_ins *)ins))
      {
        next = AMD64_INS_INEXT(ins);
        if (next) 
        {
          AMD64_INS_SET_ATTRIB(next, AMD64_INS_ATTRIB(next)| IF_BBL_LEADER);
        }
      }

      /* reason 1: target of a control transfer */
      switch (AMD64_INS_OPCODE(ins))
      {
        case AMD64_Jcc:
        case AMD64_JMP:
        case AMD64_JRCXZ:
        case AMD64_CALL:
        case AMD64_LOOP:
        case AMD64_LOOPZ:
        case AMD64_LOOPNZ:
          if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
          {
            t_address dest = AddressAdd(AMD64_INS_CADDRESS(ins),AMD64_INS_CSIZE(ins));
            t_amd64_ins *  target;
            dest = AddressAddUint64(dest,AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)));
            target = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
            if (target) 
            {
              AMD64_INS_SET_ATTRIB(target,  AMD64_INS_ATTRIB(target) | IF_BBL_LEADER);
            }
            else{
              /*ASSERT(AddressIsNull(dest),("no destination for @I!",ins));*/
              if(!AddressIsNull(dest)){
                VERBOSE(0,("no destination for @I!",ins));
              }
            }
          }
          break;
        case AMD64_JMPF:
        case AMD64_CALLF:
          if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_farptr)
          {
            /* assume the destination segment is equal to the current code segment (what else could it be?) */
            t_address dest;
            t_amd64_ins * target;
            dest=AddressNew64(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)));
            target = T_AMD64_INS(ObjectGetInsByAddress(obj, dest));
            if (target) 
            {
              AMD64_INS_SET_ATTRIB(target,   AMD64_INS_ATTRIB(target)| IF_BBL_LEADER);
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

    /* count the number of basic block leaders */
    for (ins = (t_amd64_ins *) SECTION_DATA(code); ins; ins = AMD64_INS_INEXT(ins))
      if (AMD64_INS_ATTRIB(ins) & IF_BBL_LEADER)
        nleaders++;
  }

  return nleaders;
} /* }}} */

/* {{{ Amd64MakeAddressProducers */
void Amd64MakeAddressProducers(t_cfg *cfg)
{
  t_bbl * bbl, * next;
  t_amd64_ins * ins;

  /* GLIBC:
   * we're looking for the following pattern:
   *    call <next ins>
   *    pop %reg
   *    add $offset, %reg
   *
   * this code only appears in the initialization and finalization code
   * of glibc, but it is dangerous code: if the pop and add instructions 
   * get moved, the address calculation will fail.
   * we can easily remove these three instructions and insert a 
   *    mov $dest, %reg 
   * instruction instead. */

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_amd64_operand * op;
    t_reg destreg;
    t_amd64_ins * call, * pop, * add;
    t_amd64_ins * mov;
    t_int64 offset;
    t_reloc * rel;

 
    ins = T_AMD64_INS(BBL_INS_LAST(bbl));
    if (!ins || (AMD64_INS_OPCODE(ins) != AMD64_CALL)) continue;
    op = AMD64_INS_SOURCE1(ins);
    if (AMD64_OP_TYPE(op) != amd64_optype_imm) continue;
    if (AMD64_OP_IMMEDIATE(op) != 0) continue;

    /* we've found a call to the next instruction. check for the rest of the pattern */
    call = ins;
    next = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
    pop = T_AMD64_INS(BBL_INS_FIRST(next));
    if (!pop || (AMD64_INS_OPCODE(pop) != AMD64_POP))
    {
      WARNING (("found call to next instruction, but don't recognize the "
	    "pattern. Assuming this is a genuine call.\n@I",call));
      continue;
    }
    if (AMD64_OP_TYPE(AMD64_INS_DEST(pop)) != amd64_optype_reg)
      FATAL(("found call to next ins but unknown pattern (@I)", call));
    destreg = AMD64_OP_BASE(AMD64_INS_DEST(pop));

    add = AMD64_INS_INEXT(pop);
    if (!add || (AMD64_INS_OPCODE(add) != AMD64_ADD))
      FATAL(("found call to next ins but unknown pattern (@I)", call));
    if ((AMD64_OP_TYPE(AMD64_INS_DEST(add)) != amd64_optype_reg) || (AMD64_OP_BASE(AMD64_INS_DEST(add)) != destreg))
      FATAL(("found call to next ins but unknown pattern (@I)", call));
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(add)) != amd64_optype_imm)
      FATAL(("found call to next ins but unknown pattern (@I)", call));
    if (!AMD64_INS_REFERS_TO(add))
      FATAL(("found call to next ins but unknown pattern (@I)", call));
    rel = RELOC_REF_RELOC(AMD64_INS_REFERS_TO(add));
    offset = (t_int64) AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(add));

    /* make mov instruction */
    Amd64MakeInsForBbl(MovToReg,Append,mov,bbl,destreg,AMD64_REG_NONE,G_T_UINT64(AMD64_INS_CADDRESS(pop)) + offset);
    AMD64_OP_FLAGS(AMD64_INS_SOURCE1(mov)) |= AMD64_OPFLAG_ISRELOCATED;
    ASSERT(RELOC_N_TO_RELOCATABLES(rel)<=1, ("Complex relocation found: @R",rel));
    ASSERT(strncmp(RELOC_CODE(rel),"gA00+P-\\",8)==0, ("Unexpected relocation for PIC code %s", RELOC_CODE(rel)));
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
	AddressSub(RELOC_ADDENDS(rel)[0], AddressAdd(RELOC_FROM_OFFSET(rel), AMD64_INS_CSIZE(pop))),
	(t_relocatable *) mov,
	AddressNew64(1),
	NULL,
	AddressNew64(0),
	RELOC_HELL(rel),
	RELOC_EDGE(rel),
	NULL,
	NULL,
	"gA00+\\" WRITE_32
	);

    AMD64_INS_SET_OLD_SIZE(mov,  AMD64_INS_OLD_SIZE(call));
    AMD64_INS_SET_OLD_ADDRESS(mov,  AMD64_INS_OLD_ADDRESS(call));

    /* kill the pattern */
    Amd64InsKill(call);
    Amd64InsKill(pop);
    Amd64InsKill(add);
    while (BBL_SUCC_FIRST(bbl))
    {
      if (CFG_EDGE_CORR(BBL_SUCC_FIRST(bbl)))
	CfgEdgeKill(CFG_EDGE_CORR(BBL_SUCC_FIRST(bbl)));
      CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(bbl)));
    }

    /* add a fallthrough edge to connect the blocks involved in the pattern */
    CfgEdgeCreate(cfg,bbl,next,ET_FALLTHROUGH);
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
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_cfg_edge * edge, * tmp;
    t_amd64_ins * ins;
    t_amd64_operand * src, * dest;
    t_reg destreg;

    /* look for __i686.get_pc_thunk.bx first, work back from there */
    ins = T_AMD64_INS(BBL_INS_FIRST(bbl));
    if (BBL_NINS(bbl) != 2) continue;
    src = AMD64_INS_SOURCE1(ins);
    dest = AMD64_INS_DEST(ins);
    if (AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(bbl))) != AMD64_RET) continue;
    if (AMD64_INS_OPCODE(ins) != AMD64_MOV) continue;
    if (AMD64_OP_TYPE(src) != amd64_optype_mem ||
	AMD64_OP_BASE(src) != AMD64_REG_RSP ||
	AMD64_OP_INDEX(src) != AMD64_REG_NONE ||
	AMD64_OP_IMMEDIATE(src) != 0)
      continue;
    if (AMD64_OP_TYPE(dest) != amd64_optype_reg) continue;
    destreg = AMD64_OP_BASE(dest);

    /* now go back over all incoming call edges of this basic block */
    BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge,tmp)
    {
      t_bbl * head = CFG_EDGE_HEAD(edge);
      t_bbl * ret;
      t_reloc * rel;
      t_amd64_ins * mov, * call;

      if (CFG_EDGE_CAT(edge) != ET_CALL)
	FATAL(("@B should only have incoming call edges!",bbl));

      ret = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
      call = T_AMD64_INS(BBL_INS_LAST(head));
      ins = T_AMD64_INS(BBL_INS_FIRST(ret));
      src = AMD64_INS_SOURCE1(ins);
      dest = AMD64_INS_DEST(ins);
      if (AMD64_INS_OPCODE(ins) != AMD64_ADD ||
	  AMD64_OP_TYPE(dest) != amd64_optype_reg ||
	  AMD64_OP_BASE(dest) != destreg ||
	  AMD64_OP_TYPE(src) != amd64_optype_imm)
	FATAL(("Unexpected @I",ins));
      if (!AMD64_INS_REFERS_TO(ins))
	FATAL(("Expected some PC-relative relocation"));
      rel = RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins));

      ASSERT(RELOC_N_TO_RELOCATABLES(rel)<=1, ("Complex relocation found @R",rel));
      ASSERT(strncmp(RELOC_CODE(rel),"gA00+P-\\",8)==0, ("Unexpected relocation for PIC code %s", RELOC_CODE(rel)));
      /* create the address producer */
      Amd64MakeInsForBbl(MovToReg,Append,mov,head,destreg,AMD64_REG_NONE,G_T_UINT64(BBL_CADDRESS(ret)) + AMD64_OP_IMMEDIATE(src));
      AMD64_OP_FLAGS(AMD64_INS_SOURCE1(mov)) |= AMD64_OPFLAG_ISRELOCATED;
      RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
	AddressSub(RELOC_ADDENDS(rel)[0], RELOC_FROM_OFFSET(rel)),
	(t_relocatable *) mov,
	AddressNew64(1),
	NULL,
	AddressNew64(0),
	RELOC_HELL(rel),
	RELOC_EDGE(rel),
	NULL,
	NULL,
	"gA00+\\" WRITE_32
	);

      AMD64_INS_SET_OLD_SIZE(mov,  AMD64_INS_OLD_SIZE(call));
      AMD64_INS_SET_OLD_ADDRESS(mov,  AMD64_INS_OLD_ADDRESS(call));

      /* kill the call instruction */
      Amd64InsKill(call);
      /* turn the add instruction into a noop */



      Amd64InstructionMakeNoop(ins);
      /* kill the call and return edges */
      if (CFG_EDGE_CORR(edge))
	CfgEdgeKill(CFG_EDGE_CORR(edge));
      CfgEdgeKill(edge);
      /* add a fallthrough edge between head and ret */
      CfgEdgeCreate(cfg,head,ret,ET_FALLTHROUGH);
    }

  }


} /* }}}*/

/*{{{ patchcalculatedjumps
 *  */
static void Amd64patchcalculatedjumps(t_object *obj)
{
/*if jmp from reg and following code all same length this is probably a calculated jump*/
  t_cfg *cfg = OBJECT_CFG(obj);
  t_bbl *bbl;
  int teller=0;

  CFG_FOREACH_BBL(cfg,bbl){
    t_bbl *bbldest;
    t_ins * iins, * iins2;
    t_amd64_ins *ins,*ins1,*ins2;
    t_ins *itarget_ins;
    t_amd64_ins *target_ins;
    t_address adr1;
    int same;

    ins = T_AMD64_INS(BBL_INS_LAST(bbl));
    /*last instruction should be jump to contents of reg*/
    if (!ins) continue;
    if (AMD64_INS_OPCODE(ins) != AMD64_JMP) continue;
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg) continue;

    adr1 = AddressAdd(AMD64_INS_CSIZE(ins),AMD64_INS_CADDRESS(ins));
    ins1 = T_AMD64_INS(ObjectGetInsByAddress(obj, adr1));

    if (AMD64_INS_OPCODE(ins1) == AMD64_DATA)
      continue;
    
    target_ins = ins1;
    bbldest=AMD64_INS_BBL(target_ins);

    same=0;
    BBL_FOREACH_INS(bbldest,iins2){
      t_amd64_ins * ins2 = T_AMD64_INS(iins2);
      if(AddressExtractInt32(AMD64_INS_CSIZE(ins1))!=AddressExtractInt32(AMD64_INS_CSIZE(ins2))){
	same=0;
	break;
      }else{
	same++;
      }
    }
    if(same<4)
      continue;


    if(target_ins){
      bbldest=AMD64_INS_BBL(target_ins);
      if(bbldest){
	t_bbl *prev=bbl;
	t_bbl *new;
	teller++;
/*	VERBOSE(0,("oproeper: @ieB",bbl));
	VERBOSE(0,("bbldest: @ieB",bbldest));
	fflush(stdout);*/

	while(BBL_SUCC_FIRST(bbl))
	  CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(bbl)));
	
	ins2=T_AMD64_INS(BBL_INS_LAST(bbldest));
	BBL_FOREACH_INS_SAFE(bbldest,iins,itarget_ins){
          target_ins = T_AMD64_INS(itarget_ins);
          ins = T_AMD64_INS(iins);
	  if(ins!=ins2){
	    new = BblNew (cfg);
            BBL_SET_OLD_ADDRESS(new, AMD64_INS_OLD_ADDRESS(ins));
	    BBL_SET_CADDRESS(new, AMD64_INS_CADDRESS(ins));
	    BBL_SET_CSIZE(new, AMD64_INS_CSIZE(ins));
	    Amd64InsAppendToBbl (Amd64InsDup (ins), new);
	    /* following is necessary because InsDup doesn't copy address field */
	    AMD64_INS_SET_OLD_ADDRESS(T_AMD64_INS(BBL_INS_LAST(new)), AMD64_INS_OLD_ADDRESS(ins));
	    AMD64_INS_SET_CADDRESS(T_AMD64_INS(BBL_INS_LAST(new)), AMD64_INS_CADDRESS(ins));
	    AMD64_INS_SET_OLD_SIZE(T_AMD64_INS(BBL_INS_LAST(new)), AMD64_INS_OLD_SIZE(ins));

	    CfgEdgeCreate(cfg,bbl,new,ET_JUMP);
	    CfgEdgeCreate(cfg,prev,new,ET_FALLTHROUGH);
	    prev=new;
	    Amd64InsKill(ins);
/*  	    VERBOSE(0,("new: @ieB",new));
	    fflush(stdout);*/
	  }
	}
	CfgEdgeCreate(cfg,prev,bbldest,ET_FALLTHROUGH);
	CfgEdgeCreate(cfg,bbl,bbldest,ET_JUMP);
/*	VERBOSE(0,("oproeper: @ieB",bbl));
 	VERBOSE(0,("bbldest: @ieB",bbldest));
 	fflush(stdout);*/
      }
    }
  }
  /*    VERBOSE(0,("patched %d jumps for risk of being calculated",teller));
        fflush(stdout);*/
}
/*}}}*/
    
/* {{{ patch __mpn_{add,sub}_n
 * these glibc functions use computed jumps in the code, which cannot
 * be represented reliably in a control flow graph. We fix this by
 * rewriting the code into a more traditional switch statement */
static void Amd64PatchMpnSwitches(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);
  t_bbl *bbl;
  t_amd64_ins *ins;
  t_object *linker = ObjectGetLinkerSubObject(obj);

  /* we are looking for the following pattern:
   * lea <some code address>(<some index expression>),%reg
   * jmp *%reg
   */
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_reg jmpreg,indexreg;
    t_amd64_operand *op;
    t_bbl *splits[8];
    t_bbl *base;
    t_section *subsec;
    int i;

    /* last ins has to be indirect jump */
    ins = T_AMD64_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;
    if (AMD64_INS_OPCODE(ins) != AMD64_JMP) continue;
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg) continue;
    jmpreg = AMD64_OP_BASE(AMD64_INS_SOURCE1(ins));

    /* next-to-last instruction has to be a lea */
    ins = AMD64_INS_IPREV(ins);
    if (!ins) continue;
    if (AMD64_INS_OPCODE(ins) != AMD64_LEA) continue;
    if (AMD64_OP_BASE(AMD64_INS_DEST(ins)) != jmpreg) continue;
    if (!AMD64_INS_REFERS_TO(ins)) continue;
    if (RELOC_N_TO_RELOCATABLES(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)))!=1) continue;
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)))[0]) != RT_BBL) continue;

    /* see if it's one of the constructs we recognize */
    op = AMD64_INS_SOURCE1(ins);
    if (!(AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)) FATAL(("Unrecognized"));
    indexreg = AMD64_OP_INDEX(op);
    if (indexreg != AMD64_OP_BASE(op)) FATAL(("Unrecognized"));
    if (indexreg == AMD64_REG_NONE) FATAL(("Unrecognized"));
    if (AMD64_OP_SCALE(op) != AMD64_SCALE_8) FATAL(("Unrecognized"));

    base = T_BBL(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)))[0]);
    /* the base block should contain at least 24 instructions */
    if (BBL_NINS(base) < 24) FATAL(("Unrecognized"));

    /* remove hell edge from bbl */
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(bbl)));

    /* create new data block for switch jump table */
    if (!OBJECT_NRODATAS(obj)) FATAL(("implement no rodatas in parent object"));
    subsec = SectionCreateForObject(linker,RODATA_SECTION,OBJECT_RODATA(obj)[0],
	AddressNew64(32),"__mpn_switch");

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
	  AddressNew64(0),T_RELOCATABLE(subsec),
	  AddressNew64(4*i),
	  T_RELOCATABLE(splits[i]),AddressNew64(0),FALSE,NULL,
	  NULL,NULL,CALC_ABS "\\" WRITE_32);
 
      if (i>0)
	{
	  RELOC_SET_SWITCH_EDGE(rel, edge);
	  /*	  edge->rel = rel;*/
	}
   }

    /* TODO lea -> mov */
    Amd64InstructionMakeMovFromMem(ins,jmpreg,0,AMD64_REG_NONE,indexreg,AMD64_SCALE_8);
    op = AMD64_INS_SOURCE1(ins);
    AMD64_OP_FLAGS(op) |= AMD64_OPFLAG_ISRELOCATED;
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
	AddressNew64(0),T_RELOCATABLE(ins),
	AddressNew64(3),
	T_RELOCATABLE(subsec),AddressNew64(0),FALSE,NULL,
	NULL,NULL,CALC_ABS "\\" WRITE_32);
  }
} /* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
