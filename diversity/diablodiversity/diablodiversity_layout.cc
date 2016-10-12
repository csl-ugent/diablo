/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

#ifdef __cplusplus
extern "C" {
#endif
#include <diabloanopti386.h>
#include <diablodiversity.h>
#include <diablosmc.h>
#ifdef __cplusplus
}
#endif

t_codebytelist * codebyteList = NULL;
static t_bblList * bblList = NULL;
static t_bbl * bblLast = NULL;

t_diversity_options DiversitySmcLayout(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase){/*{{{*/
  t_diversity_options ret;
#ifdef _MSC_VER
	  FATAL(("SMC Not Supported yet on Win32"));
#else
  if(phase == 0)
  {
  CodebyteStartNextInChain(cfg);
  CodebyteStartPrevInChain(cfg);

    {
      t_codebyte * codebyte;
      t_codebyte_ref * codebyte_ref;
      codebyteList = (t_codebytelist *) Calloc(1,sizeof(t_codebytelist));
      
      SmcCreateChains(cfg);
      CFG_FOREACH_CODEBYTE(cfg,codebyte)
      {
	if(!CODEBYTE_PREV_IN_CHAIN(codebyte))
	  AddCodebyteToList(codebyte,codebyteList);
      }
    }
  }
  else
  {
    t_codebyte_ref * item = CodebyteListGetNthElement(codebyteList,choice->choice);
	t_codebyte * codebyte;
    CodebyteListUnlink(codebyteList,item);
    if(bblLast)
    {
      t_bbl * to = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(item->codebyte)));
      if(!to)
	FATAL((""));
      CfgEdgeCreate(cfg,bblLast,to,ET_FALLTHROUGH);
    }
    for(codebyte = item->codebyte; CODEBYTE_NEXT_IN_CHAIN(codebyte); codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
      ;
    bblLast = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte)));
    if(!bblLast)
      FATAL((""));
  }
  
  if(codebyteList->count == 0)
  {
    CodebyteStopNextInChain(cfg);
    CodebyteStopPrevInChain(cfg);
    ret.done = TRUE;
  }
  else
  {
    ret.flags = FALSE;
    ret.range = codebyteList->count-1;
    ret.done = FALSE;
  }

  return ret;
#endif
}/*}}}*/

t_diversity_options DiversityCcLayout(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase){/*{{{*/
  t_diversity_options ret;

  if(phase == 0)
  {
    t_chain_holder chains;
    bblList = BblListNew();

    I386CreateChains(cfg,&chains);

    for (t_uint32 i = 0; i < chains.nchains; i++)
      BblListAdd(bblList,chains.chains[i]);
    Free(chains.chains);
  }
  else
  {
    t_bbl_item * item = BblListGetNthElement(bblList,choice->choice);
	t_bbl * bbl;
    BblListUnlink(item,bblList);

    if(bblLast)
    {
      CfgEdgeCreate(cfg,bblLast,item->bbl,ET_FALLTHROUGH);
    }
    for(bbl = item->bbl; BBL_NEXT_IN_CHAIN(bbl); bbl = BBL_NEXT_IN_CHAIN(bbl))
      ;
    bblLast = bbl;
  }
  
  if(bblList->count == 0)
  {
    ret.done = TRUE;
  }
  else
  {
    ret.flags = FALSE;
    ret.range = bblList->count-1;
    ret.done = FALSE;
  }

  return ret;
}/*}}}*/

t_diversity_options DiversityLayout(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase){/*{{{*/
  t_architecture_description * arch_desc_pointer = ArchitectureGetDescription("i386");
#ifdef _MSC_VER
//#WARNING TODO: no SMC yet in MSC
	return DiversityCcLayout(cfg,choice,phase);
#else
  if((void*)arch_desc_pointer->Deflowgraph == (void*)SmcDeflowgraph)
    return DiversitySmcLayout(cfg,choice,phase);
  else
	return DiversityCcLayout(cfg,choice,phase);
#endif
    
}/*}}}*/

