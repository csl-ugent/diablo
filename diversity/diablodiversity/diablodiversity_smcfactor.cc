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

static t_uint32 i=0,j=0;
static t_codebyte ** leaders=NULL;
static t_uint32 leader_count=0;
static t_bool i_first=FALSE;
static t_bbl * head_of_merged_chain = NULL;
static t_codebyte * codebyte_to_write = NULL;
static t_uint32 valuej = 0;

static t_bool MoveToNextCandidate1Byte()/*{{{*/
{
  j++;
  while(i<leader_count)
  {
    for(;j<leader_count;j++)
    {
      if(leaders[j]==NULL)
	continue;

      codebyte_to_write = SmcChainDistance1(leaders[i],leaders[j],&valuej);
      if(codebyte_to_write)
      {
	return TRUE;
      }
    }
    do
    {
     i++;
     if(i==leader_count)
       return FALSE;
    }
    while(leaders[i]==NULL);
    i_first = TRUE;
    j=i+1;
  }
  return FALSE;
}/*}}}*/

static t_bool MoveToNextCandidate4Byte()/*{{{*/
{
  j++;
  while(i<leader_count)
  {
    for(;j<leader_count;j++)
    {
      if(leaders[j]==NULL)
	continue;
      if(CODEBYTE_SCREWED(leaders[j]))
	continue;

      if(SmcChainDistance4(leaders[i],leaders[j]))
      {
	return TRUE;
      }
    }
    do
    {
     i++;
     if(i==leader_count)
       return FALSE;
    }
    while(leaders[i]==NULL || CODEBYTE_SCREWED(leaders[i]));
    i_first = TRUE;
    j=i+1;
  }
  
  return FALSE;
}/*}}}*/

t_diversity_options DiversitySmcFactor1ByteModifier(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;

  if(phase == 0)
  {
    DiabloSmcInitAfterwards(cfg);
    leader_count = SmcFactorInit(cfg, &leaders);
  }
  else
  {
    if(choice->choice == 1)
    {
      head_of_merged_chain = SmcFactorWithOneByteModifier(codebyte_to_write, i_first, i, j, leaders, cfg, valuej, head_of_merged_chain);
      i_first = FALSE;
    }
  }
  
  if(!MoveToNextCandidate1Byte())
  {
    SmcBranchForwarding(cfg);
    SmcFactorFini(cfg, &leaders);
    ret.done = TRUE;
  }
  else
  {
    ret.done = FALSE;
    ret.flags = FALSE;
    ret.range = 1;
  }
  
  return ret;
}/*}}}*/

t_diversity_options DiversitySmcFactor4ByteModifier(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;

  if(phase == 0)
  {
    DiabloSmcInitAfterwards(cfg);
    leader_count = SmcFactorInit(cfg, &leaders);
  }
  else
  {
    if(choice->choice == 1)
    {
      VERBOSE(0,("%d %d",i,j));
      SmcFactorWithFourByteModifier(i_first, i, j, leaders, cfg);
      i_first = FALSE;
    }
  }
  if(!MoveToNextCandidate4Byte())
  {
    SmcBranchForwarding(cfg);
    SmcFactorFini(cfg, &leaders);
    ret.done = TRUE;
  }
  else
  {
    ret.done = FALSE;
    ret.flags = FALSE;
    ret.range = 1;
  }
  
  return ret;
}/*}}}*/

t_diversity_options DiversitySmcFactor(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase){/*{{{*/
  t_diversity_options ret;
  
  if(phase == 0 || (phase ==2 && leaders==NULL))
  {
    DiabloSmcInitAfterwards(cfg);
    leader_count = SmcFactorInit(cfg, &leaders);
  }
  else if(phase == 1)
  {
    if(choice->choice == 1)
    {
      VERBOSE(0,("1: %d %d",i,j));
      head_of_merged_chain = SmcFactorWithOneByteModifier(codebyte_to_write, i_first, i, j, leaders, cfg, valuej, head_of_merged_chain);
      i_first = FALSE;
    }
  }
  else if (phase == 2)
  {
    i=0;
    j=0;
  }
  else if (phase == 3)
  {
    if(choice->choice == 1)
    {
      VERBOSE(0,("4: %d %d",i,j));
      SmcFactorWithFourByteModifier(i_first, i, j, leaders, cfg);
      i_first = FALSE;
    }
  }
  
  if(phase == 0 || phase == 1)
  {
    if(!MoveToNextCandidate1Byte())
    {
      SmcBranchForwarding(cfg);
      ret.done = TRUE;
      return ret;
    }
  }
  else if (phase == 2 || phase == 3)
  {
    if(!MoveToNextCandidate4Byte())
    {
      SmcBranchForwarding(cfg);
      SmcFactorFini(cfg, &leaders);
      ret.done = TRUE;
      return ret;
    }
  }

  ret.done = FALSE;
  ret.flags = FALSE;
  ret.range = 1;
  return ret;
}/*}}}*/
