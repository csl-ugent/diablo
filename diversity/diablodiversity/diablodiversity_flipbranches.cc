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
#ifdef __cplusplus
}
#endif

static t_bool CanInvertBranch(t_bbl * bbl)/*{{{*/
{
  if(!BBL_INS_LAST(bbl))
    return FALSE;
  if(I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))!=I386_Jcc)
    return FALSE;
  return TRUE;
}/*}}}*/

static t_bblList * bblList = NULL;

/* {{{ Iterative diversity backend */
t_arraylist* DiversityFlipBranchesCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (CanInvertBranch(bbl)) {
    return SimpleCanTransform(bbl, NULL, 1);
  } else {
    return NULL;
  }
}

t_diversity_options DiversityFlipBranchesDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  I386InvertBranchBbl(bbl);
  return diversity_options_null;
}

/*}}}*/

/* {{{ Regular diversity backend */
t_diversity_options DiversityFlipBranches(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  t_bbl * bbl;
  
  /*PHASE 0: ask user which basic blocks to split{{{*/
  if(phase == 0)
  {
    t_uint32 counter=0;
    bblList = BblListNew();
    
    CFG_FOREACH_BBL(cfg,bbl)
      if(CanInvertBranch(bbl))
	BblListAdd(bblList,bbl);
    
    ret.flags = TRUE;
    ret.done = FALSE;
    ret.range = bblList->count;
    ret.phase = 1;
    ret.element1 = bblList;
    phase = 1;
    return ret;
  }
  /*}}}*/
  /*PHASE 1 split basic blocks selected by the user{{{*/
  else if (phase == 1)
  {
    t_bbl_item * item = bblList->first;
    t_uint32 counter = 0;

    while(item != NULL)
    {
      if(choice->flagList->flags[counter])
      {
	I386InvertBranchBbl(item->bbl);
      }
      counter++;
      item = item->next;
    }
    ret.done = TRUE;
  }
  /*}}}*/
  
  return ret;
}/*}}}*/

/*}}}*/
