/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/***********************************************************
 * LOCO files
 *
 * Matias Madou
 *
 * mmadou@elis.ugent.be
 **********************************************************/

extern "C" {
#include <diabloanopti386.h>
}

#include <obfuscation/obfuscation_architecture_backend.h>
#include "i386_architecture_backend.h"
#include "i386_branch_function.h"

static t_function* branch_fun = NULL;

static t_bbl * AddDataInCode(t_bbl* bbl, t_bool fixed_rub_data, t_uint8 * rub_data, t_bool fixed_rub_length, t_uint32 rub_length)/* {{{ */
{
  /* Arguments:
   * (1) t_bbl* bbl:        After this basic block add rubbish
   * (2) t_bool fixed_rub_data:     Is the rubbish data fixed?
   * (3) t_uint8 * rub_data:        if (2==true) Insert this data
   * (4) t_bool fixed_rub_length:   Is the length of the rubbish data fixed?
   * (5) t_uint32 rub_length:       if (4==true) This is the length
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

static t_bool AddBranchFunctionCallToBbl(t_bbl * bbl, t_bbl *branch_bbl)/* {{{ */
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
  //CfgEdgeCreateCall(BBL_CFG(bbl), bbl, branch_bbl, rubbish, NULL);
  CfgEdgeCreateCall(BBL_CFG(bbl), bbl, branch_bbl, target_bbl, FunctionGetExitBlock(BBL_FUNCTION(branch_bbl)));

  I386MakeInsForIns(Push,Before,ins,last_ins,I386_REG_NONE,0x0);
  /* 10: call=5 bytes; push=5 bytes; pushf=1byte; (10=5+5+1-1)*/
  RelocTableAddRelocToRelocatable(
    OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
    AddressNew32(0),
    T_RELOCATABLE(ins),
    0,
    T_RELOCATABLE(target_bbl),
    0,
    FALSE,
    NULL,
    NULL,
    T_RELOCATABLE(rubbish),
    "R00R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  I386MakeInsForIns(PushF,Before,ins,last_ins);

  return TRUE;
}
/* }}} */

static t_bool TransformJumpIntoCallToBranchFunction(t_bbl * bbl)/* {{{ */
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

static t_uint32 AddBranchFunctionCallToFunction(t_function * fun, t_bbl * b_bbl);
static t_bool TransformAllJumpsIntoCallsToBranchFunction(t_function * fun)/* {{{ */
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

static t_uint32 AddBranchFunctionCallToFunction(t_function * fun, t_bbl * b_bbl)/* {{{ */
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


static t_bbl * AddBranchFunctionToCfg(t_cfg * cfg)/* {{{*/
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

static t_bbl * AddBranchFunction(t_object * obj)/* {{{ */
{
  return AddBranchFunctionToCfg(OBJECT_CFG(obj));
} /* }}} */


/* }}} */ 

bool I386BranchFunctionTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  if (!branch_fun) {
    VERBOSE(0, ("Initializing branch function"));
    branch_fun = BBL_FUNCTION(AddBranchFunctionToCfg(BBL_CFG(bbl)));
  }

  TransformJumpIntoCallToBranchFunction(bbl);

  return true;
}

void I386BranchFunctionTransformation::transformJumpToCall(t_bbl* bbl, t_randomnumbergenerator* rng) {
  TransformJumpIntoCallToBranchFunction(bbl);
}


bool I386BranchFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!BranchFunctionTransformation::canTransform(bbl))
    return false;

  t_cfg_edge* edge;
  /* This is required for now (otherwise the HELL node is still referenced when the CFG is freed)
   * but I should investigate fixing the root cause in the transformation */
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))) {
      return false;
    }
  }

  if (bbl &&
      BBL_INS_LAST(bbl) &&
      (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_JMP) &&
      (I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(BBL_INS_LAST(bbl)))) == i386_optype_imm)) {
      return true;
  }

  return false;
}


/* Call functions / Return functions */
static t_function* branch_fun_for_with_ret = NULL;
static t_section* global_jump_var = NULL;

static t_section* GetGlobalJumpVar(t_cfg* cfg) {
  if (global_jump_var)
    return global_jump_var;

  global_jump_var = ObjectNewSubsection(CFG_OBJECT(cfg),sizeof(t_uint32),BSS_SECTION);
  return global_jump_var;
}

/* This is awful. If we want to use the same kind of obfuscation as we used to do, with a global variable,
 * we need to insert additional stubs for PIC code:
 *
 * write target function address in GLOBALVAR
 * call retfun
 * retfun: ; this needs to access a register for PIC code, yet we cannot guarantee a free register! So, workaround: target above = stub
 * push ebx
 * clobber ebx:
 * ebx = pc ... => ebx = GLOBALVAR
 * ebx = [globalvar]
 * jmp *ebx
 * stub: ; at this point, the top of stack is the pushed ebx
 * pop ebx
 * jmp actual_target
 */

static t_function* GetRetFun(t_cfg* cfg) {
  t_i386_ins * ins;
  t_function * fun;
  t_bbl * bbl=BblNew(cfg);
  t_bbl * retBlock = BblNew(cfg);
  t_reloc* global_reloc;

  if (branch_fun_for_with_ret)
    return branch_fun_for_with_ret;

  fun=FunctionMake(bbl, "BranchFunctionWithReturn", FT_NORMAL);

  /* Insert Bbl in Branch Function:
   * Because of PIC, a register needs to contain GLOBALVAR. Pick ebx for now, hardcoded, and spill it (TODO)
   * push ebx
   * pushf ; TODO: use lea
   * call get_eip_ebx
   * ft:
   * mov ebx, offset(ebx) ; ebx = [GLOBALVAR]
   * subl $4, ebx
   * popf
   * NO POP EBX! This is done by the stub that we jump to!
   * jmpl *ebx
   **/

  /* Basic Block Instructions {{{ */
  I386MakeInsForBbl(Push, Append, ins, bbl, I386_REG_EBX, 0x0);
  I386MakeInsForBbl(PushF, Append, ins, bbl);

  t_bbl* ft_block = BblNew(cfg);
  BblInsertInFunction(ft_block, fun);

  t_function* eip_stub = GetReturnAddressStubForRegister(cfg, I386_REG_EBX);

  I386MakeInsForBbl(Call, Append, ins, bbl);
  CfgEdgeCreateCall(cfg, bbl, FUNCTION_BBL_FIRST(eip_stub), ft_block, FunctionGetExitBlock(eip_stub));

  I386MakeInsForBbl(MovFromMem, Append, ins, ft_block, I386_REG_EBX /* dest */, 0 /* offset, relocated */, I386_REG_EBX /* base */, I386_REG_NONE /* index */, 0 /* scale */);

  /* GLOBALVAR = ft_block + offset => offset = GLOBALVAR - ft_block */
  RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE(  GetGlobalJumpVar(cfg)  ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      TRUE, /* hell: functie is nu via hell reachable */
      NULL, /* edge*/
      NULL, /* corresp */
      T_RELOCATABLE(ft_block), /* sec */
      "R00A00+R01-" "\\" WRITE_32);

  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  I386MakeInsForBbl(Arithmetic,Append,ins, ft_block, I386_SUB, I386_REG_EBX, I386_REG_NONE, 0x4);

  I386MakeInsForBbl(PopF,Append,ins, ft_block);

  I386MakeInsForBbl(JumpReg,Append,ins, ft_block, I386_REG_EBX);

  BblInsertInFunction(retBlock,fun);

  //CfgEdgeNew(cfg,bbl,retBlock,ET_JUMP); /* This was so in the original code, but then the JMP is killed ... */
  CfgEdgeNew(cfg, ft_block, CFG_HELL_NODE(cfg), ET_IPJUMP);
  CfgEdgeNew(cfg,retBlock,CFG_HELL_NODE(cfg),ET_RETURN);

  branch_fun_for_with_ret = fun;
  return branch_fun_for_with_ret;
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

static t_bbl* JumpAndPopStubFor(t_cfg* cfg, t_bbl* target) {
  t_bbl* bbl = BblNew(cfg);
  t_bbl* retBlock = BblNew(cfg);
  t_i386_ins* ins;
  t_function* fun;

  fun = FunctionMake(bbl, "ReturnAddressStub", FT_NORMAL);

  /* pop ebx
   * ip jmp target  */
  I386MakeInsForBbl(Pop, Append, ins, bbl, I386_REG_EBX);
  I386MakeInsForBbl(Jump, Append, ins, bbl);

  CfgEdgeNew(cfg, bbl, target, ET_IPJUMP);

  return bbl;
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

  callee_edge = GetCalleeEdge(bbl_1);
  if (!callee_edge)
        return;

  t_bbl* callee_bbl = CFG_EDGE_TAIL(callee_edge);
  callee = BBL_FUNCTION(callee_bbl);

  /* call to exit, etc */
  if (!CFG_EDGE_CORR(callee_edge))
        return;

  /* Transform call 'callee' to:
   *  push ebx (TODO: only when needed)
   *  push eax (TODO: ditto)
   *  call get_eip_ebx
   *  lea eax, eip_offset(ebx) to GLOBALVAR, eax ; eax will contain GLOBALVAR
   *  movl ($callee + 4) - reloc(this), *eax -> is offset relative to eip_this_ebx
   *  pushf (TODO: only when needed)
   *  add ebx, *eax => GLOBALVAR = actual callee address (+4)
   *  popf
   *  pop eax
   *  pop ebx
   *  call branch_fun_for_with_ret
   * */
  /* Update return edge of the function to this new basic block */
  exit_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(callee_edge));
  orig_fallthrough = CFG_EDGE_TAIL(CFG_EDGE_CORR(callee_edge));

  CfgEdgeKill(CFG_EDGE_CORR(callee_edge));
  CfgEdgeKill(callee_edge);
  callee_edge = nullptr;

  t_bbl* jumpstub_bbl = JumpAndPopStubFor(cfg, callee_bbl);

  t_bbl* ft_block = BblSplitBlock(bbl_1, BBL_INS_LAST(bbl_1), TRUE /* before */);

  I386MakeInsForBbl(Push, Append, ins, bbl_1, I386_REG_EBX, 0x0);
  I386MakeInsForBbl(Push, Append, ins, bbl_1, I386_REG_EAX, 0x0);

  t_function* eip_stub = GetReturnAddressStubForRegister(cfg, I386_REG_EBX);

  I386MakeInsForBbl(Call, Append, ins, bbl_1);
  CfgEdgeCreateCall(cfg, bbl_1, FUNCTION_BBL_FIRST(eip_stub), ft_block, FunctionGetExitBlock(eip_stub));

  I386MakeInsForBbl(Lea, Prepend, ins, ft_block, I386_REG_EAX /* dest */, 0 /* offset, relocated */, I386_REG_EBX, I386_REG_NONE, 0);
  /* GLOBALVAR = ft_block + offset => offset = GLOBALVAR - ft_block */
  callee_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE(  GetGlobalJumpVar(cfg)  ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      TRUE, /* hell: functie is nu via hell reachable */
      NULL, /* edge*/
      NULL, /* corresp */
      T_RELOCATABLE(ft_block), /* sec */
      "R00A00+R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  ins = I386InsNewForBbl(ft_block);
  I386InstructionMakeMovToMem(ins, 0, I386_REG_EAX, I386_REG_NONE, 0, I386_REG_NONE, 0);
  InsInsertBefore(T_INS(ins), BBL_INS_LAST(ft_block));
  /* voeg relocatie toe voor constante die moet worden geschreven door mov */
  immrel = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(4), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE( jumpstub_bbl ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      TRUE, /* hell: functie is nu via hell reachable */
      NULL, /* edge*/
      NULL, /* corresp */
      T_RELOCATABLE(ft_block), /* sec */
      "R00A00+R01-" "\\" WRITE_32);

  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  I386MakeInsForIns(PushF, Before, ins, T_I386_INS(BBL_INS_LAST(ft_block)));
  I386MakeInsForIns(ArithmeticToMem, Before, ins, T_I386_INS(BBL_INS_LAST(ft_block)),
                    I386_ADD, 0x0, I386_REG_EAX, I386_REG_NONE, 0, I386_REG_EBX, 0x0);

  I386MakeInsForIns(PopF, Before, ins, T_I386_INS(BBL_INS_LAST(ft_block)));
  I386MakeInsForIns(Pop, Before, ins, T_I386_INS(BBL_INS_LAST(ft_block)), I386_REG_EAX);
  I386MakeInsForIns(Pop, Before, ins, T_I386_INS(BBL_INS_LAST(ft_block)), I386_REG_EBX);

  RelocAddOrUpdateHellEdgeForBbl(cfg, callee_reloc, jumpstub_bbl); /* TODO: not immrel? */

  CfgEdgeCreateCall(cfg, ft_block, FUNCTION_BBL_FIRST(GetRetFun(cfg)), orig_fallthrough, exit_bbl);
}

bool I386CallFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!CallFunctionTransformation::canTransform(bbl))
    return false;

  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))) {
      return false;
    }
  }

  if (BBL_INS_LAST(bbl) &&
      (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_CALL))
    return true;
  return false;
}

bool I386CallFunctionTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  TransformCallIntoRetFunction(bbl);
  callsTransformed++;

  return true;
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
