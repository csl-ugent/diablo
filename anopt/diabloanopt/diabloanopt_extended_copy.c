#include <string.h>
#include <diabloanopt.h>

/*#define DEBUG_COPY_ANALYSIS  |+ Uncomment for debugging purposes +|*/
/*#define VERBOSE_COPY_ANALYSIS |+ Uncomment for a whole bunch of write-out's +|*/
#define EQ_SET(eq, new) (eq).regb = (new).regb; (eq).constant = (new).constant; (eq).taga = (new).taga; (eq).tagb = (new).tagb; 
t_equation EquationSum(t_cfg *, t_reg a1, t_equation eq1, t_reg a2, t_equation eq2);
t_equation EquationSub(t_cfg *, t_reg a1, t_equation eq1, t_reg a2, t_equation eq2);
t_bool EquationsDiffer(t_cfg *, t_equations eqs1, t_equations eqs2);
static void EquationInsert(t_cfg *,t_equations eqs, int reg1, int reg2, int constant, t_reloc* taga, t_reloc* tagb);

static void ** eqlist = NULL;
/* Allocate equations and set all but the last to TOP. The last equation is unused,
 * but must always be set to BOT!
 */
t_equations EquationsNew(t_cfg * cfg)
{
  int i;
  t_equations ret;

  if(eqlist)
  {
    ret = (t_equations) eqlist;
    eqlist = *eqlist;
  }
  else
    ret = Malloc(((CFG_DESCRIPTION(cfg)->num_int_regs + 1))*sizeof(t_equation));

  for(i=0; i<=ZERO_REG(cfg); i++)
/*    ret[i] = TopEquation;*/
    EquationSetTop(ret[i]);
  EquationSetBot(ret[ZERO_REG(cfg)], i);
  return ret;
}

void EquationsFree(t_equations eq)
{
  Free(eq); return;
  *((void**)eq) = eqlist;
  eqlist = (void *) eq;
}

void EquationsRealFree(void)
{
  t_equations  eqs;
  
  while(eqlist)
  {
    eqs = (t_equations)eqlist;
    eqlist = *eqlist;
    Free(eqs);
  }
}

/* Set all equations to BOT */
void EquationsSetAllBot(t_cfg * cfg, t_equations eqs)
{
  int i;
  for(i=0;i<=ZERO_REG(cfg);i++)
  {
    EquationSetBot(eqs[i],i);
  }
}

/* Set all equations to TOP */
void EquationsSetAllTop(t_cfg * cfg, t_equations eqs)
{
  int i;
  for(i=0;i<ZERO_REG(cfg);i++)
/*    eqs[i] = TopEquation;*/
    EquationSetTop(eqs[i]);
}

/* Join two sets of equations, eqs1 and eqs2, into eqs1. Returns TRUE if eqs1 differs
 * before and after the join. If eqs1 is still TOP, then we copy eqs2 into eqs1. */
t_bool EquationsJoin(t_cfg * cfg, t_equations eqs1, t_equations eqs2)
{
  t_equation tmp;
  t_reg i; 
  t_bool join_result = FALSE;
  
  /* {{{ */
#ifdef DEBUG_COPY_ANALYSIS  
  if(!EquationIsBot(eqs1[ZERO_REG(cfg)])) FATAL(("Not bot anymore!"));
  if(!EquationIsBot(eqs2[ZERO_REG(cfg)])) FATAL(("Not bot anymore!"));
  for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2;i++)
  {
    if(!EquationIsTop(eqs1[i]) && EquationIsTop(eqs1[i+1]))
    {
      printf("eqs1:\n"); EquationsPrint(cfg, eqs1);
      printf("eqs2:\n"); EquationsPrint(cfg, eqs2);
      FATAL(("Troubles here!"));
    }
    if(!EquationIsTop(eqs2[i]) && EquationIsTop(eqs2[i+1]))
    {
      printf("eqs1:\n"); EquationsPrint(cfg, eqs1);
      printf("eqs2:\n"); EquationsPrint(cfg, eqs2);
      FATAL(("Troubles here!"));
    }
  }
#endif
  
#ifdef VERBOSE_COPY_ANALYSIS  
  printf("Join A:\n");
  EquationsPrint(cfg, eqs1);
  printf("Join B:\n");
  EquationsPrint(cfg, eqs2);
#endif
  /* }}} */

  if (EquationIsTop(eqs2[0]))
    return FALSE; /* join(eqs1,eqs2) == eqs1 */
  
  if (EquationIsTop(eqs1[0]))
  {
    /* join (eqs1, eqs2) == eqs2 */
    EquationsCopy(cfg,eqs2,eqs1);
#ifdef VERBOSE_COPY_ANALYSIS  
    printf("Result A:\n");
    EquationsPrint(cfg,eqs1);
#endif
    return TRUE;
  }

  /* we have to perform an actual join operation */
  for (i = 0 ;i <= ZERO_REG(cfg); i++)
  {
    if (EquationIsBot(eqs1[i]))
    {
      if (eqs1[i].rega < i && EquationIsBot(eqs1[eqs1[i].rega]))
	eqs1[i].rega = i;
      continue;
    }
    if (EquationIsTop(eqs1[i]))
    {
      WARNING(("Still to top!"));
      return FALSE;
    }
      
    tmp = eqs1[i];
    if (tmp.rega < i && eqs1[tmp.rega].regb == i)//! EquationIsBot(eqs1[tmp.rega]))
    {
      EquationSetBot(eqs1[i],tmp.rega);
    }
    else
    {
      EquationSetBot(eqs1[i],i);
    }
    while (1)
    {
      if (EquationsRegsEqual (cfg, eqs2, i, tmp.regb, tmp.constant,
	    tmp.taga, tmp.tagb) == YES)
      {
	/* if this equation also holds in the original eqs2, it is common and
	 * will appear in the join */
	EQ_SET(eqs1[i],tmp);
	eqs1[tmp.regb].rega = i;
	break;
      }
      else
	join_result = TRUE;
      
      if (!EquationIsBot(eqs1[tmp.regb]))
#ifdef DEBUG_COPY_ANALYSIS
        if(EquationIsTop(eqs1[tmp.regb]))
	  FATAL(("Still to top!"));
	else
#else
        if (!EquationIsTop(eqs1[tmp.regb]))
#endif
      /* we can produce another equation by summing two equations */
	{
	  tmp = EquationSum(cfg,i,tmp,tmp.regb,eqs1[tmp.regb]);
	}
	else
	  break; 
#ifndef DEBUG_COPY_ANALYSIS
      else
	break; 
#endif
    }
  }

#ifdef VERBOSE_COPY_ANALYSIS  
  printf("Result B:\n");
  EquationsPrint(cfg, eqs1);
#endif
#ifdef DEBUG_COPY_ANALYSIS
  for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2;i++)
  {
    if(!EquationIsTop(eqs1[i]) && EquationIsTop(eqs1[i+1]))
      FATAL(("Troubles here!"));
    if(!EquationIsTop(eqs2[i]) && EquationIsTop(eqs2[i+1]))
      FATAL(("Troubles here!"));
  }
#endif
  return join_result;
}

/* Answers the question 'reg1 + taga ?= reg2 + tagb - constant'. We return PERHAPS when eqs == NULL,
 * because in this case there is just no information available which is conservative
 */
t_tristate EquationsRegsEqual(t_cfg * cfg, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 constant, t_reloc * taga, t_reloc * tagb)
{
  int i,j,cte = 0;
  t_reloc * tagi, * tagj;
  t_equation tmp;

  if(!eqs) return PERHAPS;

#ifdef DEBUG_COPY_ANALYSIS
  if(!EquationIsBot(eqs[ZERO_REG(cfg)]))
    FATAL(("Not bot anymore!"));
#endif

  if(reg1<reg2)
  { i = reg1; j = reg2; cte += constant; tagi = taga; tagj = tagb;}
  else
  { i = reg2; j = reg1; cte -= constant; tagi = tagb; tagj = taga;}
  tmp = eqs[i];
  if(EquationIsBot(tmp)) return PERHAPS;
  if(EquationIsTop(tmp)) return PERHAPS;
  
  while(1)
  {
    if(tmp.regb == j)
    {
      if(tmp.constant == cte && tmp.taga == tagi && tmp.tagb == tagj) return YES;
      else return NO;
    }
    else if(!EquationIsBot(eqs[tmp.regb])
#ifdef DEBUG_COPY_ANALYSIS
	&& !EquationIsTop(eqs[tmp.regb])
#endif
	)
    {
      tmp = EquationSum(cfg, REG_BOT, tmp, tmp.regb, eqs[tmp.regb]);
    }
    else return PERHAPS;
  }
}

/* Returns YES when there is an integer offset between the two registers and this offset
 * (reg1-reg2 = offset) is filled in in the t_int32* parameter, returns NO
 * when there is a known difference between the registers but it is a difference in tags. Returns
 * PERHAPS when nothing is known about the registers and we have to be conservative */
t_tristate EquationsRegsDiffer(t_cfg * cfg, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 * constant)
{
  t_int32 i,j;
  t_equation tmp;
  
  if(reg1<reg2)
  { i = reg1; j = reg2; }
  else
  { i = reg2; j = reg1; }
  tmp = eqs[i];
  if(EquationIsBot(tmp)) return PERHAPS;
  if(EquationIsTop(tmp)) return PERHAPS;
  
  while(1)
  {
    if(tmp.regb == j)
    {
      if(!tmp.taga && !tmp.tagb)
      {
	if(reg1<reg2)
	*constant = -(tmp.constant);
	else *constant = tmp.constant;
/*        *constant = tmp.constant;*/

	return YES;
      }
      return NO;
    }
    else if(!EquationIsBot(eqs[tmp.regb])
#ifdef DEBUG_COPY_ANALYSIS
	&& !EquationIsTop(eqs[tmp.regb])
#endif
	)
    {
      tmp = EquationSum(cfg,REG_BOT,tmp,tmp.regb,eqs[tmp.regb]);
    }
    else return PERHAPS;
  }
  return PERHAPS;
}

/* Returns YES when there is a tag offset between the two registers, the tag is returned in reloc.
 * Returns NO when there is a known difference between the registers but it is not a difference in tags. Returns
 * PERHAPS when nothing is known about the registers and we have to be conservative */
t_tristate EquationsRegsDifferByTag(t_cfg * cfg, t_equations eqs, t_reg reg1, t_reg reg2, t_reloc** reloc, t_int32 * offset)
{
  t_int32 i,j;
  t_equation tmp;
  
  if(reg1<reg2)
  { i = reg1; j = reg2; }
  else
  { i = reg2; j = reg1; }
  tmp = eqs[i];
  if(EquationIsBot(tmp)) return PERHAPS;
  if(EquationIsTop(tmp)) return PERHAPS;
  
  while(1)
  {
    if(tmp.regb == j)
    {
      if(tmp.taga)
      {
	*reloc = tmp.taga;
	*offset = -(tmp.constant);
	return YES;
      }
      if(tmp.tagb)
      {
	*reloc = tmp.tagb;
	*offset = tmp.constant;
	return YES;
      }
      return NO;
    }
    else if(!EquationIsBot(eqs[tmp.regb])
#ifdef DEBUG_COPY_ANALYSIS
	&& !EquationIsTop(eqs[tmp.regb])
#endif
	)
    {
      tmp = EquationSum(cfg,REG_BOT,tmp,tmp.regb,eqs[tmp.regb]);
    }
    else return PERHAPS;
  }
  return PERHAPS;
}

t_tristate EquationsRegsDifferWithTagAllowed(t_cfg * cfg, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 * constant, t_reloc * tag)
{
  t_int32 i,j;
  t_equation tmp;
  
  if(reg1<reg2)
  { i = reg1; j = reg2; }
  else
  { i = reg2; j = reg1; }
  tmp = eqs[i];
  if(EquationIsBot(tmp)) return PERHAPS;
  if(EquationIsTop(tmp)) return PERHAPS;
  
  while(1)
  {
    if(tmp.regb == j)
    {
      if(tmp.taga == tag || tmp.tagb == tag)
      {
	if(reg1>reg2) *constant = tmp.constant;
	else *constant = -(tmp.constant);
	
	return YES;
      }
      return NO;
    }
    else if(!EquationIsBot(eqs[tmp.regb])
#ifdef DEBUG_COPY_ANALYSIS
	&& !EquationIsTop(eqs[tmp.regb])
#endif
	)
    {
      tmp = EquationSum(cfg,REG_BOT,tmp,tmp.regb,eqs[tmp.regb]);
    }
    else return PERHAPS;
  }
  return PERHAPS;

}


/* Add the equation "defined - used + constant - taga + tagb = 0" to the set of equations, hereby
 * invalidating some of the existing equations. */
void EquationsAdd(t_cfg * cfg, t_equations eqs, t_reg defined, t_reg used, t_int32 constant, t_reloc* taga, t_reloc* tagb)
{
  int i;
  if(defined != used)
  {
    EquationsInvalidate(cfg,eqs,defined);
    EquationInsert(cfg,eqs,defined,used,constant,taga,tagb);
  }
  else
  {
#ifdef DEBUG_COPY_ANALYSIS
    if(taga != NULL || tagb != NULL) FATAL(("Tags should be NULL!"));
#endif
    for(i=0; i<defined; i++)
    {
      if (EquationIsBot(eqs[i])) continue;
      if(eqs[i].regb == defined)
      {
	eqs[i].constant -= constant;
	break;
      }
    }
    if(!EquationIsBot(eqs[defined])) eqs[defined].constant += constant;
  }
  return;
}

static void EquationInsert(t_cfg * cfg, t_equations eqs, int reg1, int reg2, int constant, t_reloc* taga, t_reloc* tagb)
{
  t_equation tmp, old;
  t_reloc * reloc;
  t_int32 offset;
  int diff_tags = 0;

  /* Sort the registerpair */
  if (reg1 == reg2 && constant == 0 && taga == NULL && tagb == NULL)
    FATAL(("I could have found this myself!"));
  if(reg1 < reg2)
  {tmp.rega = reg1; tmp.regb = reg2;tmp.constant = constant; tmp.taga = taga; tmp.tagb = tagb;}
  else
  {tmp.rega = reg2; tmp.regb = reg1;tmp.constant = 0-constant; tmp.taga = tagb; tmp.tagb = taga;}

#ifdef VERBOSE_COPY_ANALYSIS
  printf("Inserting ");  EquationPrint(cfg, &tmp);  printf("\nin\n"); EquationsPrint(cfg,eqs);
#endif

  if (taga)
  {
    if(EquationsRegsDifferByTag(cfg,eqs,reg2,CFG_DESCRIPTION(cfg)->num_int_regs,&reloc,&offset)==YES)
    {
      if (taga != reloc)
      {
	diff_tags++;
	if(!tagb) diff_tags++;
      }
    }
/*    if(EquationsRegsDifferByTag(cfg,eqs,reg1,CFG_DESCRIPTION(cfg)->num_int_regs,&reloc,&offset)==YES)*/
/*    {*/
/*      if(tagb != reloc)*/
/*        diff_tags+=2;*/
/*    }*/
  }
  if (tagb)
  {
    if(EquationsRegsDifferByTag(cfg,eqs,reg1,CFG_DESCRIPTION(cfg)->num_int_regs,&reloc,&offset)==YES)
    {
      if (tagb != reloc)
      {
	diff_tags++;
	if(!taga) diff_tags++;
      }
     
    }
/*    if(EquationsRegsDifferByTag(cfg,eqs,reg2,CFG_DESCRIPTION(cfg)->num_int_regs,&reloc,&offset)==YES)*/
/*    {*/
/*      if(taga != reloc)*/
/*        diff_tags+=2;*/
/*    }*/
  }
  if (diff_tags > 1)
  {
/*    printf("Can't handle this...\n");*/
    return;
  }

  /* See if the highest numbered register pair isn't in an equation already */
  if (eqs[tmp.regb].rega < tmp.regb) /* eqs[e](r) < e */
  {
    t_reg alternate_eq_nr = eqs[tmp.regb].rega;
    if(EquationIsBot(eqs[alternate_eq_nr])) 
    {
      EquationsPrint(cfg,eqs);
      FATAL(("1:Should not be bot! %d", alternate_eq_nr));
    }
    if(eqs[alternate_eq_nr].regb != tmp.regb) FATAL(("2:Should be equal!"));
      /* Substract the equations, we don't lose any information, but we will insert them
       * in an ordered way */
    old = EquationSub(cfg,REG_BOT,tmp,alternate_eq_nr,eqs[alternate_eq_nr]); /* T = (d,Q) - (eqs[e](r), eqs[eqs[e](r)]) */

    /* We get an order result back, so in the case below we know that the result should be stored at offset 'alternate_eq_nr' */
      if (old.rega == alternate_eq_nr) /* T(r) = eqs[e](r) */
      {
	EQ_SET(eqs[alternate_eq_nr],old);
	eqs[old.regb].rega = alternate_eq_nr;
	eqs[tmp.regb].rega = tmp.regb; /* eqs[e](r) = e */
	EquationInsert(cfg,eqs,tmp.rega,tmp.regb,tmp.constant,tmp.taga,tmp.tagb);
#ifdef VERBOSE_COPY_ANALYSIS
	printf("1:\n");EquationsPrint(cfg,eqs);
#endif
	return;
      }

#ifdef DEBUG_COPY_ANALYSIS
      if(old.rega != old.regb)
#endif
	/* Info is not redundant, so insert */
        EquationInsert(cfg,eqs,old.rega,old.regb,old.constant,old.taga,old.tagb);
#ifdef DEBUG_COPY_ANALYSIS
      else /*if (tmp.constant != 0)*/ FATAL(("Conflict when inserting an equation"));
#endif
#ifdef VERBOSE_COPY_ANALYSIS
printf("2:\n");	EquationsPrint(cfg,eqs);
#endif
      return;
  }

  /* If there is already info available over the smallest numbered reg, see if the highest numbered
   * reg is lower than regb in the existing info */
  if(!EquationIsBot(eqs[tmp.rega]))// && !EquationIsTop(eqs[tmp.rega]))
  {
    /* Generate new info and decide which equation goes at offset i */
    old = EquationSub(cfg,tmp.rega,eqs[tmp.rega],REG_BOT,tmp);
    if(eqs[tmp.rega].regb > tmp.regb)
    {
      eqs[eqs[tmp.rega].regb].rega = eqs[tmp.rega].regb;
      eqs[tmp.regb].rega = tmp.rega;
    /* Insert the new info, and insert the substraction of the new and old info */
      EQ_SET(eqs[tmp.rega],tmp);
#ifdef VERBOSE_COPY_ANALYSIS
	printf("3:\n");EquationsPrint(cfg,eqs);
#endif
    }
    else /*if (eqs[tmp.rega].regb < tmp.regb)*/
    {
    /* Leave the old info and insert the substraction of the new and old info */
#ifdef VERBOSE_COPY_ANALYSIS
	printf("4:\n");EquationsPrint(cfg,eqs);
#endif
    }
    EquationInsert(cfg,eqs,old.rega,old.regb,old.constant,old.taga,old.tagb);
    return;
  }
  /* There was no info available yet, just insert it */
  EQ_SET(eqs[tmp.rega],tmp);
  /*if(eqs[tmp.regb].rega == REG_BOT)*/ eqs[tmp.regb].rega = tmp.rega;

#ifdef VERBOSE_COPY_ANALYSIS
	printf("5:\n");EquationsPrint(cfg,eqs);
#endif	
  return;
}



void EquationsInvalidate(t_cfg * cfg, t_equations eqs, t_reg r)
{
  /* skip non-int registers */
  if (r >= ZERO_REG(cfg)) {FATAL(("Should never get here!")); return;}

#ifdef VERBOSE_COPY_ANALYSIS
  printf("Verify before\n");
  EquationsVerify(cfg,eqs);
#endif

  if(!EquationIsBot(eqs[r]) && eqs[r].rega < r && !EquationIsBot(eqs[eqs[r].rega]))
  {
    t_equation new = EquationSum(cfg, eqs[r].rega, eqs[eqs[r].rega], r, eqs[r]);
/*    eqs[r].rega = r;*/
    eqs[new.regb].rega = new.rega;
    EQ_SET(eqs[new.rega],new);
    EquationSetBot(eqs[r],r);
#ifdef VERBOSE_COPY_ANALYSIS
    printf("Invalidated %d\n",r);
    EquationsPrint(cfg,eqs);
#endif
    return;
  }

  if (!EquationIsBot(eqs[r]))
  {
    eqs[eqs[r].regb].rega = eqs[r].regb;
    EquationSetBot(eqs[r],r);
    return;
  }

  if(eqs[r].rega < r && !EquationIsBot(eqs[eqs[r].rega]))
  {
    EquationSetBot(eqs[eqs[r].rega], eqs[eqs[r].rega].rega);
    eqs[r].rega = r;
  }
}

void EquationsInvalidateRegset(t_cfg * cfg, t_equations equations, t_regset regs)
{
#if 0
  t_uint32 i;
#endif
  t_reg r;

  REGSET_FOREACH_REG(regs, r)
    EquationsInvalidate(cfg, equations, r);
#if 0
  for (i = 0; i < (CFG_DESCRIPTION(cfg)->num_int_regs + 1) - 1; ++i)
  {
    if (EquationIsBot(equations[i])) continue;
    /* We can delete an equation without checking for other equation, when the register we
     * look for is equal to rega. All other subsequent equation won't have rega in it. */
    if (RegsetIn (regs, equations[i].rega))
    {
      EquationSetBot(equations[i]);
      continue;
    }
    while (!EquationIsBot(equations[i]) && RegsetIn (regs, equations[i].regb))
    {
      t_reg r = equations[i].regb;
      if (EquationIsBot(equations[r]))
      {
	EquationSetBot(equations[i]);
      }
      else
	equations[i] = EquationSum (cfg, equations[r], equations[i]);
    }
  }
#endif
}

t_equation EquationSum(t_cfg * cfg, t_reg a1, t_equation eq1, t_reg a2, t_equation eq2)
{
  t_equation ret, ret2;

  if(a1 != REG_BOT) eq1.rega = a1;
  if(a2 != REG_BOT) eq2.rega = a2;

  if(eq1.rega == eq2.regb)
  {
    ret.rega = eq2.rega;
    ret.regb = eq1.regb;
    ret.constant = eq1.constant + eq2.constant;
  }
  else if (eq1.regb == eq2.rega)
  {
    ret.rega = eq1.rega;
    ret.regb = eq2.regb;
    ret.constant = eq1.constant + eq2.constant;
  }
  else
  {
    EquationPrint(cfg,a1,&eq1);printf("\n");
    EquationPrint(cfg,a2,&eq2);printf("\n");
    FATAL(("Must have equal register pair for addition\n"));
  }

  if(eq1.taga == eq2.tagb)
  {
    if(eq1.tagb == eq2.taga)
    {
      ret.taga = NULL;
      ret.tagb = NULL;
    }
    else
    {
      ret.taga = eq2.taga;
      ret.tagb = eq1.tagb;
    }
  }
  else if(eq1.tagb == eq2.taga)
  {
    ret.taga = eq1.taga;
    ret.tagb = eq2.tagb;
  }
  else if(!eq2.taga && !eq2.tagb)
  {
    ret.taga = eq1.taga;
    ret.tagb = eq1.tagb;
  }
  else if(!eq1.taga && !eq1.tagb)
  {
    ret.taga = eq2.taga;
    ret.tagb = eq2.tagb;
  }
  else
  {
    FATAL(("Tag troubles in EquationSum"));
  }

  if(ret.taga && ret.taga == ret.tagb)
  {
    ret.taga = NULL;
    ret.tagb = NULL;
  }
  if (ret.rega < ret.regb)
    return ret;
  ret2.rega = ret.regb;
  ret2.regb = ret.rega;
  ret2.constant = -ret.constant;
  ret2.taga = ret.tagb;
  ret2.tagb = ret.taga;
  return ret2;
}

t_equation EquationSub(t_cfg * cfg, t_reg a1, t_equation eq1, t_reg a2, t_equation eq2)
{
  t_equation ret, ret2;

  if(a1 != REG_BOT) eq1.rega = a1;
  if(a2 != REG_BOT) eq2.rega = a2;

  if(eq1.rega == eq2.rega)
  {
    ret.rega = eq2.regb;
    ret.regb = eq1.regb;
    ret.constant = eq1.constant - eq2.constant;
  }
  else if (eq1.regb == eq2.regb)
  {
    ret.rega = eq1.rega;
    ret.regb = eq2.rega;
    ret.constant = eq1.constant - eq2.constant;
  }
  else
  {
    EquationPrint(cfg,a1,&eq1);EquationPrint(cfg,a2,&eq2);
    FATAL(("Can only sub when regA or regB is the same in both equations\n"));
  }

  if(eq1.taga == eq2.taga)
  {
    ret.taga = eq2.tagb;
    ret.tagb = eq1.tagb;
  }
  else if(eq1.tagb == eq2.tagb)
  {
    ret.taga = eq1.taga;
    ret.tagb = eq2.taga;
  }
  else if(!eq2.taga && !eq2.tagb)
  {
    ret.taga = eq1.taga;
    ret.tagb = eq1.tagb;
  }
  else if(!eq1.taga && !eq1.tagb)
  {
    ret.taga = eq2.tagb;
    ret.tagb = eq2.taga;
  }
  else
  {
    EquationPrint(cfg,a1,&eq1);EquationPrint(cfg,a2,&eq2);
    FATAL(("Tag troubles in EquationSub"));
  }
  
  if(ret.taga && ret.taga == ret.tagb)
  {
    ret.taga = NULL;
    ret.tagb = NULL;
  }
  if (ret.rega < ret.regb)
    return ret;
  ret2.rega = ret.regb;
  ret2.regb = ret.rega;
  ret2.constant = -ret.constant;
  ret2.taga = ret.tagb;
  ret2.tagb = ret.taga;
  return ret2;
}


void EquationsPrint(t_cfg * cfg, t_equations eqs)
{
  t_uint32 i;
#ifdef DEBUG_COPY_ANALYSIS
  t_equation tmp;
#endif
  /*if(EquationIsTop(eqs[0]))
  {
    DiabloPrint(stdout,"reg_top\n");
    return;
  }*/
  for(i = 0; i < (CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2; i++)
  {
    if (!EquationIsBot(eqs[i]) && !EquationIsTop(eqs[i]))
    {
      EquationPrint(cfg,i,eqs+i);
#ifdef DEBUG_COPY_ANALYSIS
      tmp = eqs[i];
      while(!EquationIsBot(eqs[tmp.regb]) && !EquationIsTop(eqs[tmp.regb]))
      {
        fprintf(stdout," ; ");
        fflush(stdout);
        tmp = EquationSum(cfg, REG_BOT, tmp, tmp.regb, eqs[tmp.regb]);
	EquationPrint(cfg, REG_BOT, &tmp);
        fflush(stdout);
      }
#endif
      fprintf(stdout,"\n");
    }
    else if (EquationIsTop(eqs[i]))
      VERBOSE(0,("%d : reg_top",i));
#ifdef VERBOSE_COPY_ANALYSIS
    else
      VERBOSE(0,("%d: BOT -> rega = %d",i,eqs[i].rega));
#endif
  }
  if(!EquationIsBot(eqs[(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2]) && !EquationIsTop(eqs[(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2]))
  {
    EquationPrint(cfg,(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2,&eqs[(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2]);
    VERBOSE(0,("\n"));
  }
  else if (EquationIsTop(eqs[(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2]))
    VERBOSE(0,("%d : reg_top",(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2));
#ifdef VERBOSE_COPY_ANALYSIS
  else
    VERBOSE(0,("%d: BOT -> rega = %d",ZERO_REG(cfg)-1,eqs[ZERO_REG(cfg)-1].rega));
  VERBOSE(0,("%d: BOT -> rega = %d",ZERO_REG(cfg),eqs[ZERO_REG(cfg)].rega));
#endif
}

void EquationPrint(t_cfg * cfg, t_reg index, t_equation* eq)
{
  t_reg rega = eq->rega;
  t_architecture_description *desc  = CFG_DESCRIPTION(cfg);
  if(index != REG_BOT) rega = index; 
  if (EquationIsTop(*eq))
  {
    fprintf(stdout,"top\n");
    return;
  }
  if( EquationIsBot(*eq))
  {
    fprintf(stdout,"bot\n");
    return;
  }
  if(eq->constant <= 0)
  {
#ifndef VERBOSE_COPY_ANALYSIS
    fprintf(stdout,"%s = %s + %d + (%p - %p)\n",rega==desc->num_int_regs?"Nullreg":desc->register_names[(rega)],eq->regb==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(eq->regb)],0-eq->constant, eq->taga, eq->tagb);
#else
    fprintf(stdout,"%s = %s + %d + (%p - %p) (%d:%s)",rega==desc->num_int_regs?"Nullreg":desc->register_names[(rega)],eq->regb==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(eq->regb)],0-eq->constant, eq->taga, eq->tagb,eq->rega,rega!=eq->rega?"YEP":"NEP");
#endif
  }
  else
  {
#ifndef VERBOSE_COPY_ANALYSIS
    fprintf(stdout,"%s = %s + %d + (%p - %p)\n",eq->regb==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(eq->regb)],rega==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(rega)],eq->constant, eq->tagb, eq->taga);
#else
    fprintf(stdout,"%s = %s + %d + (%p - %p) (%d:%s)",eq->regb==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(eq->regb)],rega==CFG_DESCRIPTION(cfg)->num_int_regs?"Nullreg":desc->register_names[(rega)],eq->constant, eq->tagb, eq->taga,eq->rega,rega!=eq->rega?"YEP":"NEP");
#endif
  }
}

t_equation EquationNormalize(t_equation* eq, t_reg index)
{
  t_equation result;

  if (eq->constant <= 0)
  {
    result.rega = index;
    result.regb = eq->regb;
    result.constant = -eq->constant;
    result.taga = eq->taga;
    result.tagb = eq->tagb;
  }
  else
  {
    result.rega = eq->regb;
    result.regb = index;
    result.constant = eq->constant;
    result.taga = eq->tagb;
    result.tagb = eq->taga;
  }

  return result;
}

void EquationsCopy(t_cfg * cfg, t_equations src, t_equations dest)
{
  if(!src || !dest) FATAL(("Null equations!"));
  memcpy(dest,src,((CFG_DESCRIPTION(cfg)->num_int_regs + 1))*sizeof(t_equation));
}

t_bool EquationsDiffer(t_cfg *cfg,t_equations eqs1, t_equations eqs2)
{
  if(!eqs1 || !eqs2)
  {
    FATAL(("Null equations!"));
    return TRUE;
  }
#ifdef DEBUG_COPY_ANALYSIS  
  if(!EquationIsBot(eqs1[ZERO_REG(cfg)])) FATAL(("Not bot anymore!"));
  if(!EquationIsBot(eqs2[ZERO_REG(cfg)])) FATAL(("Not bot anymore!"));
#endif
  if(memcmp(eqs1,eqs2,((CFG_DESCRIPTION(cfg)->num_int_regs + 1))*sizeof(t_equation))) return TRUE;
  return FALSE;
}

#if 0 /*for testing*/
int main(void)
{
  int i;
  t_equation eqs[ZERO_REG(cfg)];
  t_equation eq1[ZERO_REG(cfg)];
  t_equation eq2[ZERO_REG(cfg)];
  EquationsResetAll(eqs);
  EquationsResetAll(eq1);
  EquationsResetAll(eq2);

  EquationsAdd(eq1, 1, 2, 0);
  EquationsAdd(eq1, 2, 3, 0);
  EquationsAdd(eq1, 3, 4, 0);
  EquationsAdd(eq2, 1, 3, 0);
  EquationsAdd(eq2, 3, 4, 0);
  EquationsAdd(eqs, 4, 5, -3);
  /*EquationsAdd(eqs, 1, 6, 0);
  EquationsAdd(eqs, 7, 6, 0);
  EquationsAdd(eqs, 4, 2, 0);
  EquationsAdd(eqs, 1, 2, 0);*/
  EquationsPrint(eq1);
  /*EquationsAdd(eqs, 1, 4, 0);*/
  EquationsPrint(eq2);
  /*EquationsAdd(eqs, 7, 2, 0);
  EquationsAdd(eqs, 1, 2, 2);
  EquationsPrint(eqs);*/
  EquationsJoin(eq1,eq2);
  EquationsPrint(eq1);
  if (EquationsRegsEqual(eq1, 2, 1, 0)) printf("2 and 1 equal\n");
  if (EquationsRegsEqual(eq1, 4, 1, 0)) printf("1 and 4 equal\n");
}
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
