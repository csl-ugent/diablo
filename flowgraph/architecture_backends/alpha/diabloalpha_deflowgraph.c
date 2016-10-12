#include <diabloalpha.h>

static t_bbl * AlphaChain(t_cfg * cfg)
{
  t_bbl * ret;
  t_chain_holder chains;

  CreateChains(cfg, &chains);
  MergeAllChains(&chains);

  ret = chains.chains[0];
  return ret;

}

void AlphaDeflowgraph(t_object *obj)
{
  t_cfg * cfg = OBJECT_CFG(obj);
  t_section *code = OBJECT_CODE(obj)[0];
  t_bbl * chain_head;

  chain_head = AlphaChain(cfg);
  SECTION_SET_TMP_BUF(code, chain_head);

  AlphaDeflowFixpoint(obj);

  if(diabloflowgraph_options.listfile)
    AlphaListFinalProgram(chain_head);

  OBJECT_SET_ENTRY(CFG_OBJECT(cfg), 
      BBL_CADDRESS(CFG_ENTRY(cfg)->entry_bbl));
}

/* Nice macro from the PPC/x86 port..*/

#define UPDATE_ADDRESSES for(i = 0; i < OBJECT_NCODES(obj); i++) AssignAddressesInChain(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]),SECTION_CADDRESS(OBJECT_CODE(obj)[i]))

void
AlphaDeflowFixpoint(t_object * obj)
{
	
	int i, nkills;
	
	STATUS(START,("Killing useless Jumps"));
	nkills = AlphaKillUselessJumps(obj);
	VERBOSE(0,("Killed %d useless jumps", nkills));
	STATUS(STOP,("Killing useless Jumps"));

	UPDATE_ADDRESSES;
	ObjectPlaceSections(obj, FALSE, FALSE, TRUE);
	UPDATE_ADDRESSES;
	AlphaUpdateBranchDisplacements(obj);
	AlphaRelocate(obj);

}

#undef UPDATE_ADDRESSES

void
AlphaRelocate(t_object * obj)
{

	t_reloc * reloc;
	t_reloc_ref * ref;
	t_relocatable * from;
	t_address ret;
	t_alpha_ins * from_ins;
	t_section * sec;

	OBJECT_FOREACH_RELOC(obj, reloc)
	{
		from = RELOC_FROM(reloc);
		if(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_INS)
		{
			from_ins = T_ALPHA_INS(from);
			if(ALPHA_INS_TYPE(from_ins) == IT_BRANCH) continue;

			ref = ALPHA_INS_REFERS_TO(from_ins);
			ret = StackExec(RELOC_CODE(reloc),reloc,NULL,NULL,FALSE,0,obj);
			ALPHA_INS_SET_IMMEDIATE(from_ins, G_T_UINT64(ret));
		}
		else
		{
			sec = T_SECTION(from);
			StackExec(RELOC_CODE(reloc),reloc,NULL,SECTION_DATA(sec),TRUE,0,obj);
		}
	}

}

void 
AlphaUpdateBranchDisplacements(t_object * obj)
{
	int i;
	t_bbl * bbl;

	for(i = 0; i < OBJECT_NCODES(obj); i++)
	{
		CHAIN_FOREACH_BBL(T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i])),bbl)
			AlphaUpdateControlFlowDisplacement(T_ALPHA_INS(BBL_INS_LAST(bbl)));
	}

}

void
AlphaUpdateControlFlowDisplacement(t_alpha_ins * ins)
{
	t_bbl * dest, * bbl;
	t_cfg_edge * jump_edge;
	t_address offset;

	if(! ins || ALPHA_INS_TYPE(ins) != IT_BRANCH) return;

	bbl = ALPHA_INS_BBL(ins);

	/* Find the jump edge out of this bbl */
	BBL_FOREACH_SUCC_EDGE(bbl, jump_edge)
		if(CfgEdgeTestCategoryOr(jump_edge, ET_JUMP | ET_IPJUMP | ET_CALL)) break;

	ASSERT(jump_edge,("No jump edge from block ending in a jump...weird"));

	dest = CFG_EDGE_TAIL(jump_edge);
	offset = AddressSub(BBL_CADDRESS(dest),AddressAddUint32(ALPHA_INS_CADDRESS(ins),4));
	ALPHA_INS_SET_IMMEDIATE(ins, G_T_UINT64(offset) >> 2);

}

t_uint32
AlphaKillUselessJumps(t_object * obj)
{

	t_uint32 nkills = 0;
	int i;
	t_bbl * bbl;
	t_section * sec;
	t_alpha_ins * ins;
	t_cfg_edge * edge;

	for(i = 0; i < OBJECT_NCODES(obj); i++)
	{
		sec = OBJECT_CODE(obj)[i];

		CHAIN_FOREACH_BBL(T_BBL(SECTION_TMP_BUF(sec)), bbl)
		{
			ins = T_ALPHA_INS(BBL_INS_LAST(bbl));
			/* Ignore non jump block enders */

			if(! ins || ALPHA_INS_FORMAT(ins) != ALPHA_ITYPE_BR) continue;
			if(ALPHA_INS_OPCODE(ins) == ALPHA_BSR) continue;
			if(ALPHA_INS_OPCODE(ins) == ALPHA_BR && \
				 ALPHA_INS_REGD(ins) != ALPHA_REG_ZERO) continue;
			
			edge = NULL;
			BBL_FOREACH_SUCC_EDGE(bbl, edge)
				if(CfgEdgeTestCategoryOr(edge, ET_JUMP | ET_IPJUMP)) break;

			ASSERT(edge,("No jump edge from block ending in a jump..wtf: @ieB", bbl));

			/* We can kill this instruction since the jump just 
			 * goes to the next bbl (fallthrough) */
			if(CFG_EDGE_TAIL(edge) == BBL_NEXT_IN_CHAIN(bbl))
			{
				AlphaInsKill(ins);
				nkills++;
				CFG_EDGE_SET_CAT(edge, \
					CFG_EDGE_CAT(edge) == ET_JUMP ? ET_FALLTHROUGH : ET_IPFALLTHRU);
			}
		}
	}

	return nkills;

}

void AlphaListFinalProgram(t_bbl * bbl)
{
  char * filename = StringDup(diabloflowgraph_options.listfile);
  FILE * f = fopen(filename,"w");
  if (f)
  {
    t_alpha_ins * ins;
    for(;bbl;bbl=BBL_NEXT_IN_CHAIN(bbl))
      for(ins =T_ALPHA_INS(BBL_INS_FIRST(bbl)); ins; ins=ALPHA_INS_INEXT(ins))
	FileIo(f,"@I\n",ins); 
    fclose(f);
  }
  else
    VERBOSE(0,("Could not open %s for writing!",filename));

  Free(filename);
}



/* vim: set shiftwidth=2: */
