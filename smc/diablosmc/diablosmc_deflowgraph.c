/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopti386.h>
#include <diablosmc.h>
#include "../../diversity/diablodiversity/diablodiversity_structs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ET_MUSTCHAIN  	ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE  	ET_CALL | ET_SWI

static t_reloc * NewRelocForCodebyte(t_reloc * reloc_orig , t_codebyte * codebyte, int i)/*{{{*/
{
  t_reloc * ret = RelocTableDupReloc(RELOC_TABLE(reloc_orig),reloc_orig);
  t_string_array * string_array;

  RelocSetFrom(ret,T_RELOCATABLE(codebyte));
  RELOC_SET_FROM_OFFSET(ret, 0);

  if(strchr(RELOC_CODE(reloc_orig),'P'))
  {
    if(RELOC_N_ADDENDS(ret)!=1)
      WARNING(("danger"));
    RELOC_ADDENDS(ret)[0] = RELOC_ADDENDS(ret)[0] - i;
    WARNING(("jeopardy"));
  }
  
  string_array = StringDivide(RELOC_CODE(reloc_orig),"\\",TRUE,FALSE);
  if(StringArrayNStrings(string_array)!=3)
    FATAL((""));
  
  t_string string = string_array->first->string;
  
  switch(i)
  {
    case 0:
      string_array->first->string = StringConcat2(string_array->first->string,"i000000ff&");
      break;
    case 1:
      string_array->first->string = StringConcat2(string_array->first->string,"i0000ff00&i00000008>");
      break;
    case 2:
      string_array->first->string = StringConcat2(string_array->first->string,"i00ff0000&i00000010>");
      break;
    case 3:
      string_array->first->string = StringConcat2(string_array->first->string,"iff000000&i00000018>");
      break;
    default:
      FATAL(("should not happen"));
  }

  if(RELOC_CODE(ret))
    Free(RELOC_CODE(ret));
  RELOC_SET_CODE(ret, StringArrayJoin(string_array,"\\"));
  StringArrayFree(string_array);
  Free(string);
  
  return ret;
}
/* }}} */

static void MigrateRelocToInsCodebyte(t_reloc * reloc, t_ins * ins)/*{{{ */
{
  int i;
  t_state_ref * state_ref = STATELIST_FIRST(INS_STATELIST(ins));
  if(reloc==NULL)
    return;

  i=RELOC_FROM_OFFSET(reloc);

  while(i>0)
  {
    state_ref = STATE_REF_NEXT(state_ref);
    i--;
  }

  for(i=0;i<4;i++)
  {
    /*Only migrate if it is the initial state*/
    if(STATE_REF_STATE(state_ref)==STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(state_ref))))))
    {
      t_reloc * relly;
      relly = NewRelocForCodebyte(reloc, STATE_CODEBYTE(STATE_REF_STATE(state_ref)), i);
    }
    state_ref = STATE_REF_NEXT(state_ref);
  }
  
}
/* }}} */

static t_address AssignAddressesInCodebyteChain(t_codebyte * codebyte, t_address start)/*{{{*/
{
  for (; codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
    {
      CODEBYTE_SET_CADDRESS(codebyte, start);
      start = AddressAdd(start,1);
    }
  return start;
} /* }}} */

t_bbl * ControlTransferGetTarget(t_ins * ins)/* {{{ */
{
  t_bbl * bbl;
  t_cfg_edge * edge;
  bbl=INS_BBL(ins);
  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if(!(CFG_EDGE_CAT(edge)&ET_FT_BLOCK_TRUE))
      break;
  }
  if(edge==NULL)
  {
    VERBOSE(0,("@ieB",INS_BBL(ins)));
    FATAL(("@I",ins));
  }
  return T_BBL(CFG_EDGE_TAIL(edge));
}
/* }}} */

static void MigrateRelocationsToCodebytes(t_ins * ins)/*{{{ */
{
  t_reloc * reloc;
   
  reloc = I386GetRelocForOp(T_I386_INS(ins), I386_INS_SOURCE1(T_I386_INS(ins)));
  MigrateRelocToInsCodebyte(reloc,ins);
  
  reloc = I386GetRelocForOp(T_I386_INS(ins), I386_INS_SOURCE2(T_I386_INS(ins)));
  MigrateRelocToInsCodebyte(reloc,ins);
  
  reloc = I386GetRelocForOp(T_I386_INS(ins), I386_INS_DEST(T_I386_INS(ins)));
  MigrateRelocToInsCodebyte(reloc,ins);
}
/* }}} */

static void CreateControlTransferRelocs(t_ins * ins)/* {{{ */
{
  t_bbl * bbl = ControlTransferGetTarget(ins);
  t_codebyte * target;
  t_cfg_edge * edge;
  
  while(BBL_NINS(bbl)==0)
  {
    VERBOSE(0,("EMPTY BLOCK"));
    WARNING(("EMPTY BLOCK"));
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if(CfgEdgeIsFallThrough(edge))
	break;
    }
    if(edge==NULL)
      FATAL(("TARGET NOT FOUND",ins));
    bbl = T_BBL(CFG_EDGE_TAIL(edge));
  }
  
  target = STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(BBL_INS_FIRST(bbl)))));

  if(I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(ins))) == 1){
    /*only if initial state*/
    if(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))) == STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))))))))
      RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), 1, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))))), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))))), "R00R01A00+-i000000ff&\\l*w\\s0000$");
  }
  /*assumed 4*/
  else
  {
    int i = 1;
    t_state_ref * state_ref = STATELIST_LAST(INS_STATELIST(ins));
    while(i<5)
    {
      if(state_ref==NULL)
	FATAL(("Give Me a Break"));
      /*only if initial state*/
      if(STATE_REF_STATE(state_ref) == STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(state_ref))))))
	switch(i)
	{
	  case 1: RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), AddressNew32(i), T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), "R00R01A00+-iff000000&i00000018>\\l*w\\s0000$");
		  break;
	  case 2: RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), i, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), "R00R01A00+-i00ff0000&i00000010>\\l*w\\s0000$");
		  break;
	  case 3: RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), i, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), "R00R01A00+-i0000ff00&i00000008>\\l*w\\s0000$");
		  break;
	  case 4: RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(INS_OBJECT(ins)), i, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(state_ref))), "R00R01A00+-i000000ff&\\l*w\\s0000$");
		  break;
	  default: FATAL(("%d",i));
	}
      state_ref = STATE_REF_PREV(state_ref);
      i++;
    }
  }
}
/* }}} */

t_bool InsIsControlTransferInstructionWithImmediateTarget(t_ins * ins)/*{{{ */
{
  switch (I386_INS_OPCODE(T_I386_INS(ins)))
  {
    case I386_JMP:
    case I386_JMPF:
    case I386_Jcc:
    case I386_JECXZ:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
    case I386_CALL:
    case I386_CALLF:
      if(I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(ins)))==i386_optype_imm)
	return TRUE;
    default:
      return FALSE;
  }
}
/* }}} */

void SmcLinkCodebytes(t_codebyte * pred, t_codebyte * succ)/*{{{*/
{
  if(CODEBYTE_NEXT_IN_CHAIN(pred) && CODEBYTE_NEXT_IN_CHAIN(pred)!=succ)
  {
    t_state * state;
    t_state_ref * state_ref;
    CODEBYTE_FOREACH_STATE(pred,state,state_ref)
    {
      VERBOSE(0,("@ieB",INS_BBL(STATE_PARENT_INS(state))));
    }
    CODEBYTE_FOREACH_STATE(succ,state,state_ref)
    {
      VERBOSE(0,("@ieB",INS_BBL(STATE_PARENT_INS(state))));
    }
    FATAL(("Impossible Layout"));
  }
  CODEBYTE_SET_NEXT_IN_CHAIN(pred,succ);
  
  if(CODEBYTE_PREV_IN_CHAIN(succ) && CODEBYTE_PREV_IN_CHAIN(succ)!=pred)
    FATAL(("Impossible Layout"));
  CODEBYTE_SET_PREV_IN_CHAIN(succ,pred);
}
/*}}}*/
  
void ResetLinksForBbl(t_bbl * bbl)
{
  t_ins * ins;
  t_state_ref * state_ref;
  BBL_FOREACH_INS(bbl, ins)
  {
    INS_FOREACH_STATE_REF(ins,state_ref)
    {
      CODEBYTE_SET_NEXT_IN_CHAIN(STATE_CODEBYTE(STATE_REF_STATE(state_ref)),NULL);
      CODEBYTE_SET_PREV_IN_CHAIN(STATE_CODEBYTE(STATE_REF_STATE(state_ref)),NULL);
    }
  }
}

void AddLinksForBbl(t_bbl * bbl)/*{{{*/
{
  t_ins * ins;
  
  BBL_FOREACH_INS(bbl, ins)
  {
    /*codebytes of the same instruction must be placed successively*/
    t_state_ref * state_ref;
    if(INS_STATELIST(ins)==0){
      FATAL(("@B",INS_BBL(ins)));
    }
    INS_FOREACH_STATE_REF(ins,state_ref)
    {
      if(STATE_REF_NEXT(state_ref))
      {
	SmcLinkCodebytes(STATE_CODEBYTE(STATE_REF_STATE(state_ref)), STATE_CODEBYTE(STATE_REF_STATE(STATE_REF_NEXT(state_ref))));
      }
    }
    
    /*instructions within a basic block must be placed successively*/
    if(INS_INEXT(ins))
    {
      SmcLinkCodebytes(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins)))), STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(INS_INEXT(ins))))));
    }
  }
}
/* }}} */

static t_uint64 SectionGetCSize(t_section * sec,t_relocatable_address_type type)/*{{{*/
{
  return SECTION_CSIZE(sec);
}
/*}}}*/

void SmcCalcReloc(t_reloc * rel, t_object * obj)/*{{{*/
{
  t_uint32 data;
  t_uint8 *buf = (t_uint8 *)&data;
  if (!AddressIsNull(RELOC_FROM_OFFSET(rel))) FATAL(("Implement codebyte with non-zero from_offset"));


  StackExec(RELOC_CODE(rel), rel, NULL, (char*)buf, TRUE, 0, obj);
  STATE_SET_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(T_CODEBYTE(RELOC_FROM(rel))))), buf[0]);
}
/* }}} */

void SmcDeflow(t_object * obj)/*{{{*/
#define UPDATE_ADDRESSES	for (i=0; i<OBJECT_NCODES(obj); i++) AssignAddressesInCodebyteChain(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]),SECTION_CADDRESS(OBJECT_CODE(obj)[i]))
{
  int i;

  ObjectPlaceSections(obj, FALSE, FALSE, TRUE);
  
  {
    t_uint32 tel;
    t_section * section;
    OBJECT_FOREACH_SECTION(obj,section,tel)
    {
      VERBOSE(0,("@G @G %s",SECTION_CADDRESS(section),SECTION_CSIZE(section),SECTION_NAME(section))); 
    }
  }

  UPDATE_ADDRESSES;
  I386CalcRelocs(obj);
}
#undef UPDATE_ADDRESSES
/*}}}*/

void SmcCreateChains(t_cfg * cfg) /*create constraints between codebytes{{{*/
{
    t_bbl * bbl;
    t_bbl * iter_bbl;
    
    {
      t_codebyte_ref * codebyte_ref;
      t_codebyte * codebyte;

      CFG_FOREACH_CODEBYTE(cfg,codebyte)
      {
	CODEBYTE_SET_NEXT_IN_CHAIN(codebyte,NULL);
	CODEBYTE_SET_PREV_IN_CHAIN(codebyte,NULL);
      }
    }
    
    CFG_FOREACH_BBL(cfg, bbl)
    {
      t_cfg_edge * edge;

      /*Add constraints within basic block*/
      AddLinksForBbl(bbl);

      /*Add constraints for ft_like edges {{{*/
      if( BBL_IS_HELL(bbl) || (BBL_FUNCTION(bbl) && (FunctionGetExitBlock(BBL_FUNCTION(bbl)) == bbl)) || BBL_NINS(bbl)==0 )
	continue;

      iter_bbl = bbl;
      do
      {
	BBL_FOREACH_SUCC_EDGE(iter_bbl,edge)
	{
	  if (CfgEdgeTestCategoryOr(edge,ET_MUSTCHAIN))
	    break;
	  if (CfgEdgeTestCategoryOr(edge,ET_MUSTCHAINMAYBE) && CFG_EDGE_CORR(edge))
	  {
	    edge = CFG_EDGE_CORR(edge);
	    break;
	  }
	}
	if(edge)
	  iter_bbl = T_BBL(CFG_EDGE_TAIL(edge));
      } while(edge && BBL_NINS(iter_bbl)==0);

      if (edge)
      {
	SmcLinkCodebytes(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(BBL_INS_LAST(bbl))))), STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(BBL_INS_FIRST(iter_bbl))))));
      }
    }
    /*}}}*/
}
  /*}}}*/

t_bbl * getFirstNotEmptySuccessorBbl(t_bbl * bbl)
{
  VERBOSE(0,("@ieB",bbl));
  while(BBL_NINS(bbl)==0)
  {
    t_cfg_edge * edge = BblGetSuccEdgeOfType(bbl, ET_IPFALLTHRU | ET_FALLTHROUGH);
    if(! edge){
      FATAL((""));
      return NULL;
    }
    bbl = CFG_EDGE_TAIL(edge);
  }
  return bbl;
}
void SmcDeflowgraph(t_section * code)/*{{{ */
{
  t_object * obj = SECTION_OBJECT(code);
  t_cfg * cfg = OBJECT_CFG(SECTION_OBJECT(code));
  t_codebytelist * leaders = Calloc(1,sizeof(t_codebytelist));
  t_codebyte * leader;

  CodebyteStartNextInChain(cfg);
  CodebyteStartPrevInChain(cfg);

  {
    t_bbl * iter_bbl;
    t_reloc * rel;
    t_reloc_ref * rr;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      if(BBL_NINS(iter_bbl)==0)
      {
        while ((rr = RELOCATABLE_REFED_BY((t_relocatable *) iter_bbl)))
        {
	  t_bool found = FALSE;
          t_uint32 i;
          rel = RELOC_REF_RELOC(rr);

          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==rr)
            {
              RelocSetToRelocatable (rel, i, T_RELOCATABLE(getFirstNotEmptySuccessorBbl(iter_bbl)));
	      RELOC_SET_EDGE(rel, NULL);
	      found = TRUE;
	      break;
            }
          }
	  ASSERT(found, ("Relocations are corrupt!"));
        }
      }
//	while(RELOCATABLE_REFED_BY(T_RELOCATABLE(iter_bbl)))
//	{
//	  rel = RELOC_REF_RELOC((RELOCATABLE_REFED_BY(T_RELOCATABLE(iter_bbl))));
//	  RelocSetToRelocatable(rel, 0, T_RELOCATABLE(getFirstNotEmptySuccessorBbl(iter_bbl)));
//	  RELOC_SET_EDGE(rel, NULL);
//	}

	//while(RELOCATABLE_E_REFED_BY(T_RELOCATABLE(iter_bbl)))
	//{
	// rel = (RELOCATABLE_E_REFED_BY(T_RELOCATABLE(iter_bbl)))->rel;
	//  RelocESetToRelocatable(rel, T_RELOCATABLE(getFirstNotEmptySuccessorBbl(iter_bbl)));
	//  rel->edge = NULL;
	//}
    }
  }

  /*Phase 1: migrate all relocations to bbls and instructions to their first codebyte {{{*/
  {
    t_bbl * iter_bbl;
    t_reloc * rel;
    t_reloc_ref * rr;
    t_ins * ins;

    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
        while ((rr = RELOCATABLE_REFED_BY((t_relocatable *) iter_bbl)))
        {
	  t_bool found = FALSE;
          t_uint32 i;
          rel = RELOC_REF_RELOC(rr);

          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==rr)
            {
              RelocSetToRelocatable (rel, i, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(BBL_INS_FIRST(iter_bbl)))))));
	      RELOC_SET_EDGE(rel, NULL);
	      found = TRUE;
	      break;
            }
          }
	  ASSERT(found, ("Relocations are corrupt!"));
        }

//      while(RELOCATABLE_E_REFED_BY(T_RELOCATABLE(iter_bbl)))
//      {
//	rel = (RELOCATABLE_E_REFED_BY(T_RELOCATABLE(iter_bbl)))->rel;
//	RelocESetToRelocatable(rel, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(BBL_INS_FIRST(iter_bbl)))))));
//	rel->edge = NULL;
//      }

      BBL_FOREACH_INS(iter_bbl,ins)
      {
        while ((rr = RELOCATABLE_REFED_BY((t_relocatable *) ins)))
        {
	  t_bool found = FALSE;
          t_uint32 i;
          rel = RELOC_REF_RELOC(rr);

          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==rr)
            {
              RelocSetToRelocatable (rel, i, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(ins))))));
	      RELOC_SET_EDGE(rel, NULL);
	      found = TRUE;
	      break;
            }
          }
	  ASSERT(found, ("Relocations are corrupt!"));
        }

//	while(RELOCATABLE_E_REFED_BY(T_RELOCATABLE(ins)))
//	{
//	  rel = (RELOCATABLE_E_REFED_BY(T_RELOCATABLE(ins)))->rel;
//	  RelocESetToRelocatable(rel, T_RELOCATABLE(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(ins))))));
//	  rel->edge = NULL;
//	}
      }
    }
  }
  /*}}}*/
  /*Phase 2: migrate all relocations from instructions to their codebytes if they represent the initial state{{{*/
  {
    t_bbl * iter_bbl;
    t_ins * ins;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	int i=0;
	t_state_ref * state_ref;
	t_codebyte * codebyte;
	INS_FOREACH_CODEBYTE(ins,codebyte,state_ref)
	{
	  CODEBYTE_SET_OLD_ADDRESS(codebyte,INS_OLD_ADDRESS(ins)+i);
	  i++;
	}
	MigrateRelocationsToCodebytes(ins);
      }
    }
  }
  /*}}}*/
  /*Phase 3: replace control flow edges by relocations if initial_state{{{*/
  {
    t_bbl * iter_bbl;
    t_ins * ins;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	if(InsIsControlTransferInstructionWithImmediateTarget(ins))
	{
	  CreateControlTransferRelocs(ins);    
	}
      }
    }
  }
  /*}}}*/
  /*Phase 4: kill all old relocations {{{*/
  {
    t_bbl * iter_bbl;
    t_ins * ins;
    t_reloc * rel;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      while(RELOCATABLE_REFERS_TO(T_RELOCATABLE(iter_bbl)))
      {
	rel = RELOC_REF_RELOC((RELOCATABLE_REFERS_TO(T_RELOCATABLE(iter_bbl))));
	RelocTableRemoveReloc(RELOC_TABLE(rel), rel);
      }
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	while(RELOCATABLE_REFERS_TO(T_RELOCATABLE(ins)))
	{
	  rel = RELOC_REF_RELOC((RELOCATABLE_REFERS_TO(T_RELOCATABLE(ins))));
	  RelocTableRemoveReloc(RELOC_TABLE(rel), rel);
	}
      }
    }
  }

  {
    t_bbl * iter_bbl;
    t_ins * ins;
    t_reloc * rel;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      while(RELOCATABLE_REFED_BY(T_RELOCATABLE(iter_bbl)))
      {

	FATAL(("boe"));
	rel = RELOC_REF_RELOC((RELOCATABLE_REFED_BY(T_RELOCATABLE(iter_bbl))));
	RelocTableRemoveReloc(RELOC_TABLE(rel), rel);
      }
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	while(RELOCATABLE_REFED_BY(T_RELOCATABLE(ins)))
	{
	  FATAL(("boe"));
	  rel = RELOC_REF_RELOC((RELOCATABLE_REFED_BY(T_RELOCATABLE(ins))));
	  RelocTableRemoveReloc(RELOC_TABLE(rel), rel);
	}
      }
    }
  }
  /*}}}*/

  diablosupport_options.verbose++;
  SmcCreateChains(cfg);
  
  /* concatenate remaining chains {{{ */
  {
    t_codebyte * codebyte;
    t_codebyte_ref * codebyte_ref;
    CFG_FOREACH_CODEBYTE(cfg,codebyte)
    {
      if(!CODEBYTE_PREV_IN_CHAIN(codebyte))
	AddCodebyteToList(codebyte,leaders);
    }
  }
  {
    t_codebyte_ref * codebyte_ref = CODEBYTELIST_FIRST(leaders);
    t_codebyte * last;
    t_codebyte * first;

    leader = CODEBYTE_REF_CODEBYTE(codebyte_ref);
    last=leader;

    while(CODEBYTE_REF_NEXT(codebyte_ref))
    {
      while(CODEBYTE_NEXT_IN_CHAIN(last))
      {
	last = CODEBYTE_NEXT_IN_CHAIN(last);
      }
      codebyte_ref = CODEBYTE_REF_NEXT(codebyte_ref);
      first = CODEBYTE_REF_CODEBYTE(codebyte_ref);
      SmcLinkCodebytes(last, first);
    }
  }
  
  /*}}}*/

  SECTION_CALLBACKS(code)->SectionRecalculateSize=SectionGetCSize;
  SECTION_SET_TMP_BUF(code, leader);
 

  {
    int i=0;
    t_codebyte * codebyte;
    {
      for (codebyte=leader; codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
      {
	i++;
      }
    }
    
    SECTION_SET_CSIZE(code, i);
  }
  
  SmcDeflow(obj);
  
  /* {{{ debug code: list the final program */
  {
    char * filename = StringConcat2("b.out",".list");
    FILE * f = fopen(filename,"w");
    if (f)
    {
      t_codebyte * codebyte;
      for (codebyte=leader; codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
      {
	FileIo(f,"--@G @G %x %d\n",CODEBYTE_OLD_ADDRESS(codebyte),CODEBYTE_CADDRESS(codebyte),STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte)))),STATELIST_COUNT(CODEBYTE_STATELIST(codebyte)));
	t_state_ref * state_ref = STATELIST_FIRST(CODEBYTE_STATELIST(codebyte));
	for(;state_ref;state_ref = STATE_REF_NEXT(state_ref))
	{
	      FileIo(f,"@I\n",STATE_PARENT_INS(STATE_REF_STATE(state_ref)));
	}
      }
      fclose(f);
    }
    else
      VERBOSE(0,("Could not open %s for writing!",filename));
    Free(filename);
  } /* }}} */
  
  {
    FILE *f = fopen ("mapping.xml", "w");
    FileIo(f,"<mapping>");
    if (f)
    {
      t_ins *ins;
      t_codebyte * codebyte;
      for (codebyte=leader; codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
      {
	t_state_ref * state_ref = STATELIST_FIRST(CODEBYTE_STATELIST(codebyte));
	for(;state_ref;state_ref = STATE_REF_NEXT(state_ref))
	{
	  if(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(STATE_PARENT_INS(STATE_REF_STATE(state_ref))))) == STATE_REF_STATE(state_ref))
	  {
	      ins = STATE_PARENT_INS(STATE_REF_STATE(state_ref));
	      FileIo (f, "<ins address=\"@G\" encoding=\"", CODEBYTE_CADDRESS(codebyte));
	      t_state_ref * tmp = STATELIST_FIRST(INS_STATELIST(ins));
	      while(tmp)
	      {
		FileIo(f,"%.2x",STATE_VALUE(STATE_REF_STATE(tmp)));
		tmp= STATE_REF_NEXT(tmp);
	      }
	      FileIo (f, "\">");
	      {
		t_address_item * item = INS_ADDRESSLIST(ins)->first;
		while(item)
		{
		  FileIo (f, "@G ", item->address);
		  item = item->next;
		}
	      }
	      FileIo (f, "</ins>\n");
	  }
	}
      }
      FileIo(f,"</mapping>");
      fclose (f);
    }
  }
  {
    int mutants = open("mutantsDiota",O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
    int modifiers = open("modifiersDiota",O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);

    t_ins *ins;
    t_codebyte * codebyte;

    for (codebyte=leader; codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
    {
      t_state_ref * state_ref = STATELIST_FIRST(CODEBYTE_STATELIST(codebyte));
      for(;state_ref;state_ref = STATE_REF_NEXT(state_ref))
      {
	if(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(STATE_PARENT_INS(STATE_REF_STATE(state_ref))))) == STATE_REF_STATE(state_ref))
	{ 
	  ins = STATE_PARENT_INS(STATE_REF_STATE(state_ref));
	  int address = CODEBYTE_CADDRESS(codebyte);
	  
	  /*if(I386_INS_IS_MODIFIER(ins))
	  {
	    write(modifiers,&address,sizeof(int));
	    VERBOSE(0,("modifier ins at @G", address));
	  }*/
	  
	  if(STATELIST_COUNT(CODEBYTE_STATELIST(codebyte))>1)
	  {
	    IGNORE_RESULT(write(mutants,&address,sizeof(int)));
	    VERBOSE(0,("variable ins at @G", address/*CODEBYTE_CADDRESS(codebyte)*/));
	    continue;
	  }

	}
      }
    }
    close(mutants);
    close(modifiers);
  }

  CodebytelistKill(leaders);

  /*{{{*/
  {
    t_bbl * bbl = BblNew(cfg);
    t_codebyte * codebyte = leader;
    while(codebyte)
    {
      t_i386_ins * ins = T_I386_INS(InsNewForBbl(bbl));
      InsAppendToBbl(T_INS(ins),bbl);
      I386InstructionMakeData(ins);
      I386_INS_SET_DATA(ins,STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte)))));
      codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte);
    }
    SECTION_SET_TMP_BUF(code, bbl);
  }
  /*}}}*/

  VERBOSE(0,("old object entry was @G\n",OBJECT_ENTRY(obj)));
  OBJECT_SET_ENTRY(obj, CODEBYTE_CADDRESS(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST((BBL_INS_FIRST(CFG_ENTRY(cfg)->entry_bbl))))))));
  VERBOSE(0,("new object entry is @G\n", OBJECT_ENTRY(obj)));
}
/* }}} */
