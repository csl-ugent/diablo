/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/***********************************************************
 * LOCO files
 *
 * Matias Madou
 *
 * mmadou@elis.ugent.be
 **********************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include "diablodiversity_branch_function.h"
#ifdef __cplusplus
}
#endif

static t_function* branch_fun = NULL;

t_bbl * AddDataInCode(t_bbl* bbl, t_bool fixed_rub_data, t_uint8 * rub_data, t_bool fixed_rub_length, t_uint32 rub_length)/* {{{ */
{
  /* Arguments:
   * (1) t_bbl* bbl: 		After this basic block add rubbish
   * (2) t_bool fixed_rub_data: 	Is the rubbish data fixed?
   * (3) t_uint8 * rub_data:		if (2==true) Insert this data
   * (4) t_bool fixed_rub_length:	Is the length of the rubbish data fixed?
   * (5) t_uint32 rub_length:		if (4==true) This is the length
   * */

  t_bbl * rubbish=BblNew(BBL_CFG(bbl));

  BblInsertInFunction(rubbish,BBL_FUNCTION(bbl));
  
  if(fixed_rub_data)
  {
    if(fixed_rub_length)
    {
      t_uint32 i=0;

      for(i=0;i<rub_length;i++)
      {
	t_i386_ins * ins;
	I386MakeInsForBbl(Data,Append,ins,rubbish);
	I386_INS_SET_DATA(ins,rub_data[i]);
      }
    }
    else
      FATAL(("Not yet implemented...\n"));
  }
  else
    FATAL(("Not yet implemented...\n"));

  BBL_SET_ATTRIB(rubbish,BBL_IS_DATA);

  CfgEdgeCreate(BBL_CFG(bbl),bbl,rubbish,ET_FALLTHROUGH);

  return rubbish;
}
/* }}} */

t_bool AddBranchFunctionCallToBbl(t_bbl * bbl, t_bbl *branch_bbl)/* {{{ */
{
  t_cfg * cfg=BBL_CFG(bbl);
  t_bbl * target_bbl, * rubbish;
  t_cfg_edge * edge, * s_edge;
  t_i386_ins * ins, *last_ins;
  t_uint8  rub_data[2]={0x0a, 0x05};
  t_uint32 nr_edges=0;

  if( BBL_INS_LAST(bbl)==NULL ||
      I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))!=I386_JMP || 
      I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(BBL_INS_LAST(bbl)))) != i386_optype_imm)
    return FALSE;

//      VERBOSE(0,("Nieks @iB",bbl));
  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, s_edge)
  {
    if(CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
    {
//      VERBOSE(0,("Nieks @E",edge));
      if(CFG_EDGE_REFCOUNT(edge)>1)
	FATAL(("ERR"));

      nr_edges++;
      target_bbl=(t_bbl*)CFG_EDGE_TAIL(edge);
//      VERBOSE(0,("Nieks @iB",target_bbl));
      CfgEdgeKill(edge);
    }
    /* integrity check {{{ */
    else
    {
      VERBOSE(0,("Nieks @E",edge));
      FATAL(("ERR\n"));
    }
    /* }}} */
  }

  if(nr_edges>1)
    FATAL(("ERR"));

  rubbish = AddDataInCode(bbl, TRUE, rub_data, TRUE, 2);
  last_ins=T_I386_INS(BBL_INS_LAST(bbl));

    /* integrity check {{{ */
  if( I386_INS_OPCODE(last_ins)!=I386_JMP)
  {
    VERBOSE(0,("Nieks @I",last_ins));
    FATAL(("ERRRR"));
  }
  /* }}} */

  /* Transform jmp to:
   * 
   *  push offset
   *  pushf
   *  call branch_function
   * */
   
  I386InstructionMakeCall(last_ins);
  CfgEdgeCreateCall(BBL_CFG(bbl), bbl, branch_bbl, rubbish, NULL);

  I386MakeInsForIns(Push,Before,ins,last_ins,I386_REG_NONE,0x0);
  /* 10: call=5 bytes; push=5 bytes; pushf=1byte; (10=5+5+1-1)*/
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),AddressNew32(0),T_RELOCATABLE(ins),0,T_RELOCATABLE(target_bbl),0,FALSE,NULL,NULL,T_RELOCATABLE(rubbish),"R00R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) =I386_OPFLAG_ISRELOCATED;

  I386MakeInsForIns(PushF,Before,ins,last_ins);

  return TRUE;
}
/* }}} */

t_bool TransformJumpIntoCallToBranchFunction(t_bbl * bbl)/* {{{ */
{
  t_bbl * branch_bbl;
  t_cfg * cfg=FUNCTION_CFG(BBL_FUNCTION(bbl));

  if(branch_fun==NULL)
  {
    printf("ERR: Branch function should be added before this function can be used\n");
    return FALSE;
  }
  
  branch_bbl=FUNCTION_BBL_FIRST(branch_fun);
  /* }}} */ 

  AddBranchFunctionCallToBbl(bbl, branch_bbl);

  return TRUE;
}
/* }}} */ 

t_bool TransformAllJumpsIntoCallsToBranchFunction(t_function * fun)/* {{{ */
{
  t_function * branch_fun=NULL;
  t_bbl * b_bbl;
  t_cfg * cfg=FUNCTION_CFG(fun);

  /* Find Branch Function {{{ */
  CFG_FOREACH_FUN(cfg,branch_fun)
  {
    if(!strcmp("BranchFunction",FUNCTION_NAME(branch_fun)))
      break;
  }
  
  if(branch_fun==NULL)
  {
    printf("ERR: Branch function should be added before this function can be used\n");
    return FALSE;
  }
  
  b_bbl=FUNCTION_BBL_FIRST(branch_fun);
  /* }}} */ 

  AddBranchFunctionCallToFunction( fun, b_bbl);

  return TRUE;
}
/* }}} */

t_uint32 AddBranchFunctionCallToFunction(t_function * fun, t_bbl * b_bbl)/* {{{ */
{
  
  t_bbl * bbl;
  t_uint32 tel=0;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    if(BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_JMP && I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(BBL_INS_LAST(bbl))))== i386_optype_imm)
// if(tel<1)
    {
      AddBranchFunctionCallToBbl( bbl, b_bbl);
      tel++;
    }
  }
  return tel;
}
/* }}} */

  
t_bbl * AddBranchFunction(t_object * obj)/* {{{ */
{
  return AddBranchFunctionToCfg(OBJECT_CFG(obj));
} /* }}} */
  
t_bbl * AddBranchFunctionToCfg(t_cfg * cfg)/* {{{*/
{
  t_i386_ins * ins;
  t_function * fun;
  t_bbl * bbl=BblNew(cfg);
  t_bbl * retBlock = BblNew(cfg);

  fun=FunctionMake(bbl,"BranchFunction",FT_NORMAL);


  /* Insert Bbl in Branch Function:
   * xchg %eax, 0(%esp)
   * add 8(%esp), %eax
   * pop %eax
   * popf
   * ret 
   * */
  
  /* Basic Block Instructions {{{ */
  I386MakeInsForBbl(ArithmeticToMem,Append,ins,bbl, I386_XCHG, 0x0, I386_REG_ESP, I386_REG_NONE, 0, I386_REG_EAX, 0x0);
  I386MakeInsForBbl(ArithmeticToMem,Append,ins,bbl, I386_ADD, 0x8, I386_REG_ESP, I386_REG_NONE, 0, I386_REG_EAX, 0x0);
  I386MakeInsForBbl(Pop,Append,ins,bbl,I386_REG_EAX);
  I386MakeInsForBbl(PopF,Append,ins,bbl);
  I386MakeInsForBbl(Return,Append,ins,bbl);
  /* }}} */

  BblInsertInFunction(retBlock,fun);

  CfgEdgeNew(cfg,bbl,retBlock,ET_JUMP);
  CfgEdgeNew(cfg,retBlock,CFG_HELL_NODE(cfg),ET_RETURN);

  return bbl;
}
/* }}} */ 

static t_bblList * bblList = NULL;

t_diversity_options DiversityThwartDisassembly(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase) /*{{{*/
{

  t_diversity_options ret;
  
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(cfg));
  }


  if(phase == 0)/* {{{ */
  {
    t_bbl * bbl;
    t_uint32 counter=0;
    bblList = BblListNew();
    AddBranchFunctionToCfg(cfg);

    CFG_FOREACH_BBL(cfg,bbl)
    {
      if(bbl && BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_JMP && !BblIsHot(bbl) && !BblIsFrozen(bbl)) {
        t_cfg_edge* edge;
        t_bool ok = TRUE;
        BBL_FOREACH_SUCC_EDGE(bbl, edge) {
          if (CFG_EDGE_CAT(edge) == ET_SWITCH) {
            VERBOSE(1, ("NOT OK @eiB", bbl));
            ok = FALSE;
          }
          if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))) // TODO: port to generic branch function framework
            ok = FALSE;
        }
        if (ok)
          BblListAdd(bblList,bbl);
      }
    }
    ret.flags = TRUE;
    ret.done = FALSE;
    ret.range = bblList->count;
    ret.phase = 1;
    ret.element1 = bblList;
    phase = 1;
    return ret;
  }/* }}} */
  else if(phase==1)/* {{{ */
  {
    t_bbl_item * item = bblList->first;
    t_uint32 counter = 0;

    while(item != NULL)
    {
      t_bbl_item * tmp = NULL;
      if(!choice->flagList->flags[counter])
	tmp = item;
      
      item = item->next;
      counter++;
      if(tmp)
	BblListUnlink(tmp,bblList);
    }

    item = bblList->first;
    
    while(item != NULL)
    {
      VERBOSE(1, ("Transforming: @eiB", item->bbl));
      TransformJumpIntoCallToBranchFunction(item->bbl);
      VERBOSE(1, ("DONE: @eiB", item->bbl));
      item=item->next;
    }
  }/* }}} */
  ret.done = TRUE;
  return ret;
}

t_arraylist* DiversityThwartDisassemblyCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (bbl &&
      BBL_INS_LAST(bbl) &&
      (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_JMP) &&
      (I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(BBL_INS_LAST(bbl)))) == i386_optype_imm)) {
    return SimpleCanTransform(bbl, NULL, 1);
  }
  return NULL;
}

t_diversity_options DiversityThwartDisassemblyDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(cfg));
  }

  TransformJumpIntoCallToBranchFunction(bbl);

  return diversity_options_null;
}


t_bbl* SplitBasicBlockWithJump(t_bbl* bbl, t_ins* ins, t_bool before) {
  t_bbl* split_off = BblSplitBlock(bbl, ins, TRUE);
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  t_i386_ins* ins386 = (t_i386_ins*) ins;

  I386MakeInsForBbl(Jump, Append, ins386, bbl);

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, split_off, ET_JUMP);

  return split_off;
}

/*
 DiversityBranchFunctionBeforeAndAfterCalls. This transform changes
 BB1
 call X
 BB2

 into

 BB1
 Branch Function to BB1'
 BB1' := call X
 Branch Function to BB2
 BB2

 (Other jumps with as target BB2 should remain pointing to BB2)

 That is: the call to X is now in a seperate chain from BB1 and BB2 and can now be randomly relayouted.
 
 If we would just create a branch function at the entry of BBL2, ALL edges coming there would be disadvantaged...
 Thus, we make a new basic block whose sole responsability is to jump to basic block 2.
 
 Note that we split basic blocks to end in an unconditional jump, AddBranchFunctionCallToBbl expects this, and it's easiest that way...
*/
void TransformCallIntoCallToBranchFunction(t_bbl * bbl_1)/* {{{ */
{
  t_bbl* branch_bbl_1;
  t_bbl* bbl_2 = NULL;
  t_bbl* branch_bbl_2;
  t_bbl* branch_function_bbl;
  t_cfg* cfg = FUNCTION_CFG(BBL_FUNCTION(bbl_1));
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  t_i386_ins* ins = NULL;
  t_bbl* exit_bbl;
  t_cfg_edge* call_edge = NULL;
  t_bbl* callee;

  if(branch_fun==NULL)
  {
    printf("ERR: Branch function should be added before this function can be used\n");
    return;
  }
  
  branch_function_bbl=FUNCTION_BBL_FIRST(branch_fun);

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl_1, edge, edge_s) {
    if (CFG_EDGE_CAT(edge) == ET_CALL) {
      call_edge = edge;
    }
  }

  if (!call_edge) {
    /* This can happen in, for example, exit(), it seems? */
    VERBOSE(0, ("No call edge for bbl_1!!!"));
    return;
  }
  
  if (!CFG_EDGE_CORR(call_edge)) {
	VERBOSE(0, ("Only transforming call edges that actually have a fall-through path..."));
	return;
  }
  
  bbl_2 = CFG_EDGE_TAIL(CFG_EDGE_CORR(call_edge));

  callee = CFG_EDGE_TAIL(call_edge);
  VERBOSE(0, ("ObfuscatingCall to: '%s'", BBL_FUNCTION(callee) ? FUNCTION_NAME(BBL_FUNCTION(callee)) : "Name unknown"));

//#define SPLIT_AFTER
#ifdef SPLIT_AFTER
  /* Make the basic block containing the jump to BBL2. This means that the call will have to RETURN to this newly made BBL */
  branch_bbl_2 = BblNew(cfg);
  if (BBL_FUNCTION(bbl_2))
	  BblInsertInFunction(branch_bbl_2, BBL_FUNCTION(bbl_2));

  I386MakeInsForBbl(Jump, Append, ins, branch_bbl_2);
  CfgEdgeCreate(cfg, branch_bbl_2, bbl_2, ET_JUMP);
  
  /* Update return edge of the function to this new basic block */
  exit_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(call_edge));

  CfgEdgeKill(CFG_EDGE_CORR(call_edge));
  CfgEdgeKill(call_edge);
  CfgEdgeCreateCall(cfg, bbl_1, callee, branch_bbl_2, exit_bbl);

  /* Now split the first basic block just before the call */
  branch_bbl_1 = SplitBasicBlockWithJump(bbl_1, BBL_INS_LAST(bbl_1), TRUE);

  /* Add the two branch function calls, and we're done! */
  AddBranchFunctionCallToBbl(bbl_1, branch_function_bbl);
  AddBranchFunctionCallToBbl(branch_bbl_2, branch_function_bbl);
#else
  /* Now split the first basic block just before the call */
  branch_bbl_1 = SplitBasicBlockWithJump(bbl_1, BBL_INS_LAST(bbl_1), TRUE);
  /* Add a single branch function */
  AddBranchFunctionCallToBbl(bbl_1, branch_function_bbl);
#endif
  //FunctionDrawGraphWithHotness(BBL_FUNCTION(bbl_1), "newblockadded.dot");
}

static t_function* branch_fun_for_with_ret = NULL;
static t_section* global_jump_var = NULL;

static void InitGlobalJumpVar(t_cfg* cfg) {
  global_jump_var = ObjectNewSubsection(CFG_OBJECT(cfg),sizeof(t_uint32),BSS_SECTION);
}

static void InitRetFun(t_cfg* cfg) {
  t_i386_ins * ins;
  t_function * fun;
  t_bbl * bbl=BblNew(cfg);
  t_bbl * retBlock = BblNew(cfg);
  t_reloc* global_reloc;

  InitGlobalJumpVar(cfg);
  
  fun=FunctionMake(bbl, "BranchFunctionWithReturn", FT_NORMAL);


  /* Insert Bbl in Branch Function:
   * pushf
   * subl $4, GLOBALVAR
   * popf
   * jmpl *GLOBALVAR
   * */
  
  /* Basic Block Instructions {{{ */
  I386MakeInsForBbl(PushF,Append,ins,bbl);
  I386MakeInsForBbl(ArithmeticToMem,Append,ins,bbl, I386_SUB, 0x0, I386_REG_NONE, I386_REG_NONE, 0, I386_REG_NONE, 0x4);
  global_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(2),  /* from-offset: begin instructie + 2 bytes */
      T_RELOCATABLE(global_jump_var), /* to */
      AddressNew32(0), /* to-offset: begin globale variable  */
      FALSE, /* hell */
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_DEST(ins)) = I386_OPFLAG_ISRELOCATED;
 

  I386MakeInsForBbl(PopF,Append,ins,bbl);
  
  I386MakeInsForBbl(JumpMem,Append,ins,bbl, I386_REG_NONE, I386_REG_NONE);
  global_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(2),  /* from-offset: begin instructie + 2 bytes */
      T_RELOCATABLE( global_jump_var ), /* to */
      AddressNew32(0), /* to-offset: begin globale variable  */
      FALSE, /* hell */
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  BblInsertInFunction(retBlock,fun);

  CfgEdgeNew(cfg,bbl,retBlock,ET_JUMP);
  CfgEdgeNew(cfg,retBlock,CFG_HELL_NODE(cfg),ET_RETURN);

  branch_fun_for_with_ret = fun;
}

static t_cfg_edge* GetCalleeEdge(t_bbl* bbl) {
  t_cfg_edge* edge;
  t_cfg_edge* found = NULL;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
	if (CFG_EDGE_CAT(edge) == ET_CALL) {
	  if (found) {
		FATAL(("Double call found in @eiB",bbl));
	  }
	  found = edge;
	} else {
	  // FATAL(("Non-call edge @E",edge));
	  // This can happen with branch functions (disassembly thwarting): the call gets a fallthrough to the garbage. Just don't transform those (yet? TODO)
	  return NULL;
	}
  }
  ASSERT(found,("No outgoing call edge in @eiB",bbl));
  ASSERT(BBL_FUNCTION(CFG_EDGE_TAIL(found)),("bbl not in function @eiB",CFG_EDGE_TAIL(found)));
  return found;
}

static void
RelocAddOrUpdateHellEdgeForBbl(t_cfg *cfg, t_reloc *rel, t_bbl *block)
 {
   t_cfg_edge *i_edge, *found;
 
   found = NULL;
   BBL_FOREACH_PRED_EDGE(block,i_edge)
   {
     if ((CFG_EDGE_HEAD(i_edge)==CFG_HELL_NODE(cfg)) && (CFG_EDGE_CAT(i_edge) == ET_CALL))
     {
       found=i_edge;
       break;
     }
   }
 
   if (found)
   {
     RELOC_SET_EDGE(rel, found);
     CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found)+1);
     /* also increment the refcount of the corresponding return edge!!! */
     ASSERT(CFG_EDGE_CORR(found),("Call edge @E does not have a corresponding edge!",found));
     CFG_EDGE_SET_REFCOUNT( CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT( CFG_EDGE_CORR(found)) + 1);
   }
   else
   {
     RELOC_SET_EDGE(rel, CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), block,NULL,NULL));
   }
 }
 
 


static void TransformCallIntoRetFunction(t_bbl * bbl_1)/* {{{ */
{
  t_cfg_edge* callee_edge;
  t_function* callee;
 
  t_i386_ins* ins;
  t_reloc* callee_reloc;
  t_cfg* cfg = BBL_CFG(bbl_1);
  
  t_bbl* exit_bbl;
  t_bbl* orig_fallthrough;
  
  t_reloc *immrel;
  
  if (!branch_fun_for_with_ret) {
	InitRetFun(cfg);
  }
  
  callee_edge = GetCalleeEdge(bbl_1);
  if (!callee_edge)
	return;
  
  callee = BBL_FUNCTION(CFG_EDGE_TAIL(callee_edge));
  
  /* call to exit, etc */
  if (!CFG_EDGE_CORR(callee_edge))
	return;
  
  /* Transform call 'callee' to:
   * 
   *  movl $callee + 4, GLOBALVAR
   *  call branch_fun_for_with_ret
   * */
  ins = I386InsNewForBbl(bbl_1);
  I386InstructionMakeMovToMem(ins, 0, I386_REG_NONE, I386_REG_NONE, 0, I386_REG_NONE, 0);
  InsInsertBefore(T_INS(ins), BBL_INS_LAST(bbl_1));

    /* voeg relocatie toe voor doeladres mov */
  callee_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(2),  /* from-offset: begin instructie + 2 bytes */
      T_RELOCATABLE(  global_jump_var  ), /* to */
      AddressNew32(0), /* to-offset: begin globale variable  */
      TRUE, /* hell */
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);

	   /* voeg relocatie toe voor constante die moet worden geschreven door mov */
   immrel = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(4), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(6),  /* from-offset: begin instructie + 6 bytes */
      T_RELOCATABLE(CFG_EDGE_TAIL(callee_edge)), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      TRUE, /* hell: functie is nu via hell reachable */
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);
	 
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;
  I386_OP_FLAGS(I386_INS_DEST(ins)) = I386_OPFLAG_ISRELOCATED;

  RelocAddOrUpdateHellEdgeForBbl(cfg, callee_reloc, CFG_EDGE_TAIL(callee_edge));
  
    /* Update return edge of the function to this new basic block */
  exit_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(callee_edge));
  orig_fallthrough = CFG_EDGE_TAIL(CFG_EDGE_CORR(callee_edge));

  CfgEdgeKill(CFG_EDGE_CORR(callee_edge));
  CfgEdgeKill(callee_edge);
  CfgEdgeCreateCall(cfg, bbl_1, FUNCTION_BBL_FIRST(branch_fun_for_with_ret), orig_fallthrough, exit_bbl);
}


t_arraylist* DiversityBranchFunctionBeforeAndAfterCallsCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (bbl &&
      BBL_INS_LAST(bbl) &&
      (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_CALL)) {
    /* Don't transform calls to silly functions */
    t_cfg_edge* edge;
    t_cfg_edge* call_edge = NULL;
    t_function* callee = NULL;

    BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      if (CFG_EDGE_CAT(edge) == ET_CALL) {
        call_edge = edge;
      }
    }

    if (!call_edge) {
      return NULL;
    }

    if (!BBL_FUNCTION(CFG_EDGE_TAIL(call_edge))) {
      return NULL;
    }

    callee = BBL_FUNCTION(CFG_EDGE_TAIL(call_edge));

    if (!callee)
      return NULL;

    if (FUNCTION_IS_HELL(callee))
      return NULL;

    //if (callee == branch_fun)
    //  return NULL;

	if (callee == branch_fun_for_with_ret)
      return NULL;

    return SimpleCanTransform(bbl, NULL, 1);
  }
  return NULL;
}

t_diversity_options DiversityBranchFunctionBeforeAndAfterCallsDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(cfg));
  }

  TransformCallIntoCallToBranchFunction(bbl);
  
  //FunctionDrawGraphWithHotness(fun, "voor.dot");
  
  //TransformCallIntoRetFunction(bbl);
  
  //FunctionDrawGraphWithHotness(fun, "na.dot");

  return diversity_options_null;
}

t_arraylist* DiversityBranchFunctionBeforeAndAfterCallsGlobalVarCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  return DiversityBranchFunctionBeforeAndAfterCallsCanTransform(cfg, fun, bbl, additional_info);
}

t_diversity_options DiversityBranchFunctionBeforeAndAfterCallsGlobalVarDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(cfg));
  }

  //TransformCallIntoCallToBranchFunction(bbl);
  
  //FunctionDrawGraphWithHotness(fun, "voor.dot");
  
  TransformCallIntoRetFunction(bbl);
  
  //FunctionDrawGraphWithHotness(fun, "na.dot");

  return diversity_options_null;
}



t_arraylist* DiversityBranchFunctionInFirstBlockCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (FUNCTION_IS_HELL(fun))
      return NULL;

  if (BBL_NINS(bbl) < 2)
	return NULL;
  
  if (FUNCTION_BBL_FIRST(fun) && FUNCTION_BBL_FIRST(fun) == bbl)
    return SimpleCanTransform(bbl, NULL, 1);

  return NULL;
}

t_diversity_options DiversityBranchFunctionInFirstBlockDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  t_bbl* split_off;
  t_bbl* branch_function_bbl;
  
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(cfg));
  }

  branch_function_bbl = FUNCTION_BBL_FIRST(branch_fun);

  /* Split basic block after the second-to-last ins */
  split_off = SplitBasicBlockWithJump(bbl, BBL_INS_LAST(bbl), TRUE);
  /* And let it go to the branch function */
  AddBranchFunctionCallToBbl(bbl, branch_function_bbl);

  return diversity_options_null;
}




/* vim: set shiftwidth=2 foldmethod=marker:*/
