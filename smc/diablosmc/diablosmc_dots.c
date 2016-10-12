/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosmc.h>
#include <diabloi386.h>

/* static t_string GetHellNodeLabel(t_bbl *bbl) {{{ */
static t_string GetHellNodeLabel(t_bbl *bbl)
{
  t_cfg *cfg = BBL_CFG(bbl);

  if (bbl == CFG_HELL_NODE(cfg))
    return StringDup("HELL");
  if (bbl == CFG_CALL_HELL_NODE(cfg))
    return StringDup("CALL HELL");
  if (BBL_CALL_HELL_TYPE(bbl))
    return StringConcat2("DYNAMIC CALL: ", FUNCTION_NAME(BBL_FUNCTION(bbl))+16);
  if (bbl == CFG_SWI_HELL_NODE(cfg))
    return StringDup("SWI HELL");
  if (bbl == CFG_LONGJMP_HELL_NODE(cfg))
    return StringDup("LONGJMP HELL");
  if (bbl == CFG_EXIT_HELL_NODE(cfg))
    return StringDup("EXIT HELL");
  if (bbl == CFG_EXIT_CALL_HELL_NODE(cfg))
    return StringDup("EXIT CALL HELL");
  if (FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(bbl)) &&
      bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(bbl)))
    return StringConcat2("EXIT DYNAMIC CALL: ", FUNCTION_NAME(BBL_FUNCTION(bbl))+16);
  if (bbl == CFG_EXIT_SWI_HELL_NODE(cfg))
    return StringDup("EXIT SWI HELL");
  if (bbl == CFG_EXIT_LONGJMP_HELL_NODE(cfg))
    return StringDup("EXIT LONGJMP HELL");
  if (bbl == CFG_UNIQUE_ENTRY_NODE(cfg))
    return StringDup("UNIQUE ENTRY");
  if (bbl == CFG_UNIQUE_EXIT_NODE(cfg))
    return StringDup("UNIQUE EXIT");

  FATAL(("Unknown hell node @B",bbl));
}
/* }}} */

#define ClearBblData(x)		do { \
  if ((x)->fillcolor) Free((x)->fillcolor); \
  if ((x)->label) Free((x)->label); \
  if ((x)->style) Free((x)->style); \
  if ((x)->extra) Free((x)->extra); \
  memset((x), 0, sizeof(t_bbl_draw_data)); \
} while (0)

#define ClearEdgeData(x)	do { \
  if ((x)->color) Free((x)->color); \
  if ((x)->label) Free((x)->label); \
  if ((x)->style) Free((x)->style); \
  if ((x)->extra) Free((x)->extra); \
  memset((x), 0, sizeof(t_edge_draw_data)); \
} while (0)

/* void WriteOutCodeBytes(t_ins * ins,int i,FILE * out) {{{ */
static void WriteOutCodeBytes(t_ins * ins,int i,FILE * out)
{
  t_codebyte * codebyte;
  t_state_ref * state_ref, * state_ref2;
  t_state * state;
  t_uint32 counter;
  
  
  ASSERT(INS_STATELIST(ins)!=NULL,("INS_STATELIST for %p @I is nil",ins,ins));

  INS_FOREACH_CODEBYTE(ins,codebyte,state_ref)
  {
    if(STATE_REF_STATE(state_ref)==STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte))))
    {
      fprintf(out,"\t\"bytes%p\" [color=white,fillcolor=darkseagreen4,style=filled,fontcolor=white,shape=record,label=\"{ ",codebyte);

      fprintf(out, "%x",CODEBYTE_CADDRESS(codebyte));

      counter=0;

      CODEBYTE_FOREACH_STATE(codebyte,state,state_ref2)
	fprintf(out, "| <s%d> %x",counter++, STATE_VALUE(state));

      fprintf(out,"}");

      fprintf(out,"\"]\n");

      fprintf(out,"\t\"%p\" -> \"bytes%p\":s0 [color=red]\n",ins,codebyte);
    }
    else
    {
      int counter = 0;
      CODEBYTE_FOREACH_STATE(codebyte,state,state_ref2)
      {
	if(state==STATE_REF_STATE(state_ref))
	  break;
	counter++;
      }
      
      fprintf(out,"\t\"%p\" -> \"bytes%p\":s%d [color=red]\n",ins,codebyte,counter);
    }
  }
  
}
/* }}} */

/* void SmcCreateDotsForFunction(t_function * fun, t_string filename) {{{ */
static void SmcCreateDotsForFunction(t_function * fun, t_string filename)
    /*void (*bbl_annotator)(t_bbl *, t_bbl_draw_data *), 
    void (*edge_annotator)(t_cfg_edge *, t_edge_draw_data *))*/
{
  t_ins * ins;
  t_bbl * bbl;
  t_cfg_edge * edge;
  t_bool show_callers = TRUE;
  t_bool draw_link;
  int callers = 0;

  t_bbl_draw_data bbl_data;
  t_edge_draw_data edge_data;

  FILE * out = fopen(filename,"w");
  ASSERT(out,("Could not open %s for writing!",filename));

  memset(&bbl_data, 0, sizeof(t_bbl_draw_data));
  memset(&edge_data, 0, sizeof(t_edge_draw_data));

  /* only show callers if there are less than MAX_SHOW_CALLERS */
#define MAX_SHOW_CALLERS	10
  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun),edge)
    if (CFG_EDGE_CAT(edge) == ET_CALL)
      callers++;
  show_callers = (callers < MAX_SHOW_CALLERS);

  /* graph header */
  if (FUNCTION_NAME(fun) && strlen(FUNCTION_NAME(fun)))
    FileIo(out, "digraph \"%s\" {\ncompound=TRUE;\n",FUNCTION_NAME(fun));
  else
    FileIo(out, "digraph \"noname_@G\" {\n",BBL_CADDRESS(FUNCTION_BBL_FIRST(fun)));

  /* graph body */

  /* {{{ function nodes */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    ClearBblData(&bbl_data);

    if (bbl == CFG_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"HELL\"]",bbl);
    else if (bbl == CFG_CALL_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"CALL HELL\"]",bbl);
    else if (bbl == CFG_SWI_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"SWI HELL\"]",bbl);
    else if (bbl == CFG_LONGJMP_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"LONGJMP HELL\"]",bbl);
    else if (bbl == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"Entry\"]",bbl);
    else if (bbl == FunctionGetExitBlock(fun))
      bbl_data.label = StringIo("\"%p\" [label=\"RETURN\"]",bbl);
    else if (bbl == CFG_EXIT_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"EXIT HELL\"]",bbl);
    else if (bbl == CFG_EXIT_CALL_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"EXIT CALL HELL\"]",bbl);
    else if (bbl == CFG_EXIT_SWI_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"EXIT SWI HELL\"]",bbl);
    else if (bbl == CFG_EXIT_LONGJMP_HELL_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringIo("\"%p\" [label=\"EXIT LONGJMP HELL\"]",bbl);
    else if (BBL_NINS(bbl)==0)
      bbl_data.label = StringIo("\"%p\" [label=\"Empty\"]",bbl);
    else
    {
      /* regular block */
      t_string final, tmp1, tmp2;
      t_uint8 i = 0;
      final = StringIo("");
      BBL_FOREACH_INS(bbl,ins)
      {
	tmp1 = final;
	tmp2 = StringIo("\"%p\" [label = \"@I\"]\n",ins,ins);
	final = StringConcat2(tmp1,tmp2);
	Free(tmp1);
	Free(tmp2);
	if(INS_IPREV(ins))
	{
	  tmp1 = final;
	  tmp2 = StringIo("\"%p\" -> \"%p\"\n",INS_IPREV(ins),ins);
	  final = StringConcat2(tmp1,tmp2);
	  Free(tmp1);
	  Free(tmp2);
	}
	WriteOutCodeBytes(ins,i,out);
	i++;
      }

      bbl_data.label = final;
      bbl_data.style = StringDup("filled");
      bbl_data.fillcolor = StringDup("white");
    }

//    bbl_annotator(bbl, &bbl_data);
    {
      t_string tmp = StringIo("@B",bbl);
      fprintf(out,"\tsubgraph cluster_%p { \nlabel = \"%s\"\n",bbl,tmp);
      fprintf(out, "%s", bbl_data.label);
      Free(tmp);
    }
/*    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);*/
    fprintf(out,"}\n");
  } /* }}} */

  ClearBblData(&bbl_data);

  /* {{{ successor edges */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      t_bbl * tail = CFG_EDGE_TAIL(edge);

      ClearEdgeData(&edge_data);
      ClearBblData(&bbl_data);
      draw_link = FALSE;

      if (!CfgEdgeTestCategoryOr(edge,ET_INTERPROC))
      {
	/* {{{ intraprocedural edges */
	switch (CFG_EDGE_CAT(edge))
	{
	  case ET_FALLTHROUGH:
	    edge_data.style = StringDup("solid");
	    edge_data.color = StringDup("green");
	    break;
	  case ET_UNKNOWN:
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("purple");
	    break;
	  case ET_JUMP:
	  case ET_SWITCH:
	    edge_data.style = StringDup("solid");
	    edge_data.color = StringDup("black");
	    break;
	  default:
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("blue");
	}

	if (CFG_EDGE_CAT(edge) == ET_SWITCH)
	  edge_data.label = StringIo("%d", CFG_EDGE_SWITCHVALUE(edge));
	/*	else
		edge_data.label = StringIo("O(%1.2f)", ORDER(CFG_EDGE_EXEC_COUNT(edge)));*/

//	edge_annotator(edge, &edge_data);

	fprintf(out, "\t\t\"%p\" -> \"%p\" [", BBL_NINS(bbl)>0?(void *)BBL_INS_LAST(bbl):(void *)bbl, BBL_NINS(tail)>0?(void *)BBL_INS_FIRST(tail):(void *)tail);
	if(bbl!=tail)
	  fprintf(out, "ltail=cluster_%p,lhead=cluster_%p,", bbl, tail);
	fprintf(out, "style=%s, color=%s", edge_data.style, edge_data.color);
	if (edge_data.label) fprintf(out, ", label=\"%s\"", edge_data.label);
	if (edge_data.extra) fprintf(out, ", %s", edge_data.extra);
	fprintf(out, "]\n");
	/* }}} */
      }
      else
      {
	/* {{{ interprocedural edges */
	char nodename[20];
	t_string destfun = BBL_FUNCTION(tail)?(FUNCTION_NAME(BBL_FUNCTION(tail)) ? FUNCTION_NAME(BBL_FUNCTION(tail)) : "noname"):"nofun";

	/* don't print successors of the hell or call hell nodes (there's no way you can get them all to fit nicely in a drawing) */
	if (BBL_IS_HELL(bbl)) continue;

	switch (CFG_EDGE_CAT(edge))
	{
	  case ET_CALL:
	    edge_data.style = StringDup("solid");
	    edge_data.color = StringDup("red");
	    bbl_data.fillcolor = StringDup("yellow");
	    if (BBL_FUNCTION(tail) == fun)
	    {
	      /* treat recursive calls differently */
	      bbl_data.label = NULL;
	      sprintf(nodename, "%p", tail);
	    }
	    else
	    {
	      sprintf(nodename, "f%p", tail);
	      if (AddressIsEq(BBL_OLD_ADDRESS(tail),BBL_CADDRESS(tail)))
		bbl_data.label = StringIo("%s (@G)",destfun,BBL_OLD_ADDRESS(tail));
	      else
		bbl_data.label = StringIo("%s (old @G new @G)",destfun,BBL_OLD_ADDRESS(tail),BBL_CADDRESS(tail));
	      bbl_data.style = StringDup("filled");
	    }
	    if (CFG_EDGE_CORR(edge))
	      draw_link = TRUE; 
	    break;
	  case ET_RETURN:
	    edge_data.style = StringDup("solid");
	    edge_data.color = StringDup("black");
	    bbl_data.fillcolor = StringDup("green");
	    sprintf(nodename, "r%p", tail);
	    bbl_data.label = StringIo("@B in %s",tail,destfun);
	    break;
	  case ET_COMPENSATING:
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("black");
	    bbl_data.fillcolor = StringDup("green");
	    sprintf(nodename, "c%p", tail);
	    bbl_data.label = StringIo("return of %s",destfun);
	    break;
	  case ET_SWI:
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("red");
	    bbl_data.fillcolor = StringDup("red");
	    sprintf(nodename,"i%p",tail); /* tail will be the hell node */
	    bbl_data.label = StringDup("HELL");
	    break;
	  case ET_IPUNKNOWN:
	    edge_data.style = StringDup("bold");
	    edge_data.color = StringDup("purple");
	    bbl_data.fillcolor = StringDup("magenta");
	    bbl_data.style = StringDup("filled");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@B in %s",tail,destfun);
	    break;
	  case ET_IPFALLTHRU:
	  case ET_IPJUMP:
	  case ET_IPSWITCH:
	    edge_data.style = StringDup("solid");
	    edge_data.color = StringDup("red");
	    bbl_data.fillcolor = StringDup("black");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@B in %s",tail,destfun);
	    break;
	  default:
	    /* make it stand out as something we haven't seen yet */
	    edge_data.style = StringDup("bold");
	    edge_data.color = StringDup("magenta");
	    bbl_data.fillcolor = StringDup("blue");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@B in %s",tail,destfun);
	    bbl_data.style = StringDup("filled");
	    break;
	}

	/* catch hell nodes */
	if (BBL_IS_HELL(tail))
	{
	  sprintf(nodename,"h%p",tail);
	  Free(bbl_data.label);
	  bbl_data.label = GetHellNodeLabel(tail);
	  if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	  bbl_data.fillcolor = StringDup("red");
	  if (bbl_data.style) Free(bbl_data.style);
	  bbl_data.style = StringDup("filled");
	}

	if (show_callers || CFG_EDGE_CAT(edge) != ET_RETURN)
	{
	  /* print edge */
//	  edge_annotator(edge, &edge_data);
	  fprintf(out,"\t\"%p\" -> \"%s\" [style=%s,color=%s",BBL_NINS(bbl)>0?(void *)BBL_INS_LAST(bbl):(void *)bbl,nodename,edge_data.style,edge_data.color);
	  fprintf(out, ",ltail=cluster_%p", bbl);
	  if (edge_data.label) fprintf(out,",label=\"%s\"",edge_data.label);
	  if (edge_data.extra) fprintf(out,", %s",edge_data.extra);
	  fprintf(out,"]\n");

	  /* print extraprocedural block */
	  if (bbl_data.label)
	  {
	    fprintf(out,"\t\"%s\" [", nodename);
	    fprintf(out, "label=\"%s\"", bbl_data.label);
	    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
	    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
	    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);
	    fprintf(out,"]\n");
	  }

	  if (draw_link)
	  {
	    /* draw an invisible link edge, to improve the layout of function call/return edge pairs */
	    fprintf(out,"\t\"%p\" -> \"%p\" [style=invis]\n",BBL_NINS(bbl)>0?(void *)BBL_INS_LAST(bbl):(void *)bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
	  }
	}
	/* }}} */
      }
    }
  } /* }}} */

  /* {{{ predecessor edges */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      t_bbl * head = CFG_EDGE_HEAD(edge);
      t_string destfun = BBL_FUNCTION(head)?(FUNCTION_NAME(BBL_FUNCTION(head)) ? FUNCTION_NAME(BBL_FUNCTION(head)) : "noname"):"nofun";
      char nodename[20];

      ClearEdgeData(&edge_data);
      ClearBblData(&bbl_data);

      if (!CfgEdgeTestCategoryOr(edge,ET_INTERPROC)) continue; /* all intraprocedural edges have already been handled as predecessors */
      if (BBL_IS_HELL(bbl)) continue;

      switch (CFG_EDGE_CAT(edge))
      {
	case ET_CALL:
	  edge_data.style = StringDup("solid");
	  edge_data.color = StringDup("red");
	  bbl_data.fillcolor = StringDup("yellow");
	  sprintf(nodename,"F%p",head);
	  bbl_data.label = StringIo("@B in %s",head,destfun);
	  break;
	case ET_IPUNKNOWN:
	  edge_data.style = StringDup("bold");
	  edge_data.color = StringDup("purple");
	  bbl_data.fillcolor = StringDup("blue");
	  sprintf(nodename,"I%p",head);
	  bbl_data.label = StringIo("@B in %s",head,destfun);
	  break;
	case ET_IPFALLTHRU:
	case ET_IPJUMP:
	case ET_IPSWITCH:
	  edge_data.style = StringDup("solid");
	  edge_data.color = StringDup("red");
	  bbl_data.fillcolor = StringDup("cyan");
	  bbl_data.label = StringIo("@B in %s",head,destfun);
	  sprintf(nodename,"I%p",head);
	  break;
	case ET_COMPENSATING:
	  edge_data.style = StringDup("solid");
	  edge_data.color = StringDup("blue");
	  bbl_data.fillcolor = StringDup("black");
	  sprintf(nodename,"C%p",head);
	  bbl_data.label = StringIo("return from %s",destfun);
	  break;
	case ET_RETURN:
	  edge_data.style = StringDup("solid");
	  edge_data.color = StringDup("blue");
	  bbl_data.label = NULL;
	  if (BBL_FUNCTION(head) == fun)
	    sprintf(nodename,"%p", head);
	  else if (BBL_IS_HELL(FUNCTION_BBL_FIRST(BBL_FUNCTION(head))))
	    sprintf(nodename,"h%p", FUNCTION_BBL_FIRST(BBL_FUNCTION(head)));
	  else
	    sprintf(nodename,"f%p", FUNCTION_BBL_FIRST(BBL_FUNCTION(head)));
	  break;
	default:
	  FATAL(("unexpected edge type"));
      }

      /* catch hell node and call hell node */
      if (BBL_IS_HELL(head))
      {
	sprintf(nodename,"h%p",head);
	if (bbl_data.label) Free(bbl_data.label);
	bbl_data.label = GetHellNodeLabel(head);
	if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	bbl_data.fillcolor = StringDup("red");
	if (bbl_data.style) Free(bbl_data.style);
	bbl_data.style = StringDup("filled");
      }
      /* catch the entry node */
      if (head == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)))
      {
	sprintf(nodename,"e%p",head);
	if (bbl_data.label) Free(bbl_data.label);
	bbl_data.label = StringDup("Entry");
	if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	bbl_data.fillcolor = StringDup("blue");
	if (bbl_data.style) Free(bbl_data.style);
	bbl_data.style = StringDup("filled");
      }

      if (show_callers || CFG_EDGE_CAT(edge) != ET_CALL)
      {
	  /* print edge */
//	  edge_annotator(edge, &edge_data);
	  fprintf(out,"\t\"%s\" -> \"%p\" [style=%s,color=%s",nodename,BBL_NINS(bbl)>0?(void *)BBL_INS_FIRST(bbl):(void *)bbl,edge_data.style,edge_data.color);
	  fprintf(out, ",lhead=cluster_%p", bbl);
	  if (edge_data.label) fprintf(out,",label=\"%s\"",edge_data.label);
	  if (edge_data.extra) fprintf(out,", %s",edge_data.extra);
	  fprintf(out,"]\n");

	  /* print extraprocedural block */
	  if (bbl_data.label)
	  {
	    fprintf(out,"\t\"%s\" [", nodename);
	    fprintf(out, "label=\"%s\"", bbl_data.label);
	    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
	    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
	    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);
	    fprintf(out,"]\n");
	  }
      }
    }
  } /* }}} */
  
  /* graph footer */
  FileIo(out, "}\n");
  fclose(out);

  /* cleanup */
  ClearBblData(&bbl_data);
  ClearEdgeData(&edge_data);
}
/* }}} */
    
/* void SmcCreateDots(t_cfg * cfg, t_const_string dirprefix) {{{ */
void SmcCreateDots(t_cfg * cfg, t_const_string dirprefix)
{
  t_function * function;
  t_string dirpref = StringDup(dirprefix);
  int noname_count = 0;
  char noname[20];

  /* dirprefix is by default ./dots */
  if (!dirpref) dirpref = StringDup("./dots");

  /* remove trailing slash */
  if (dirpref[strlen(dirpref)-1] == '/') dirpref[strlen(dirpref)-1] = '\0';

#ifdef DIABLOSUPPORT_HAVE_MKDIR
  DirMake(dirpref,FALSE);
#else
  Free(dirpref);
  dirpref = StringDup(".");
#endif

  CFG_FOREACH_FUN(cfg,function) 
  {
    t_string fname;
    t_string inter = "";

    if (!FUNCTION_NAME(function))
      sprintf(noname,"-noname-%d-",noname_count++);
      
    if (FUNCTION_BBL_FIRST(function))
      fname=StringIo("%s/@G.func-%s%s.dot",dirpref,BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(function)),FUNCTION_NAME(function)?FUNCTION_NAME(function):noname,inter);
    else
      fname=StringIo("%s/0x%x.func-%s%s.dot",dirpref,0,FUNCTION_NAME(function)?FUNCTION_NAME(function):noname,inter);

    SmcCreateDotsForFunction(function,fname);
    Free(fname);
  }
  Free(dirpref);
}
/* }}} */


