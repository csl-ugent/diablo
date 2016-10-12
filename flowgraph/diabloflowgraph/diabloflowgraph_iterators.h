/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>

#ifndef _ITERATORS_H_
#define _ITERATORS_H_

#define OBJECT_FOREACH_CFG(obj,cfg)	for (cfg=OBJECT_CFG(obj); cfg!=NULL; cfg=NULL)


/* Functions */

#define FUNCTION_FOREACH_BBL(fun,bbl)\
   for (bbl=FUNCTION_BBL_FIRST(fun); bbl!=NULL; bbl=BBL_NEXT_IN_FUN(bbl))
#define FUNCTION_FOREACH_BBL_R(fun,bbl)\
   for (bbl=FUNCTION_BBL_LAST(fun); bbl!=NULL; bbl=BBL_PREV_IN_FUN(bbl))
#define FUNCTION_FOREACH_BBL_SAFE(fun,bbl,tmp)\
   for(bbl=FUNCTION_BBL_FIRST(fun), tmp=BBL_NEXT_IN_FUN(bbl); bbl!=NULL; bbl=tmp, tmp=(bbl==NULL)?NULL:BBL_NEXT_IN_FUN(bbl))
#define FUNCTION_FOREACH_BBL_SAFE_R(fun,bbl,tmp)\
   for(bbl=FUNCTION_BBL_LAST(fun), tmp=BBL_PREV_IN_FUN(bbl); bbl!=NULL; bbl=tmp, tmp=(bbl==NULL)?NULL:BBL_PREV_IN_FUN(bbl))

#define FUNCTION_FOREACH_CALLEE(fun,edge,callee)\
   for (edge=FUNCTION_SUCC_FIRST(fun), callee = (edge==NULL)?NULL:CG_EDGE_TAIL(edge); edge; edge=CG_EDGE_SUCC_NEXT(edge),callee=(edge==NULL)?NULL:CG_EDGE_TAIL(edge))

#define FUNCTION_FOREACH_CALLER(fun,edge,callee)\
   for (edge=T_CG_EDGE(NODE_PRED_FIRST(T_NODE(fun))), callee = (edge==NULL)?NULL:T_FUN(CG_EDGE_HEAD(edge)); edge; edge=CG_EDGE_PRED_NEXT(edge),callee=(edge==NULL)?NULL:T_FUN(CG_EDGE_HEAD(edge)))

/* Bbl -> ??? */

#define BBL_FOREACH_DOMINATES(ibbl, llelem, jbbl)\
   for (llelem = BBL_DOMINATES(ibbl), jbbl = T_BBL(llelem->data); llelem!=NULL; llelem = LLNext(llelem), jbbl = (llelem?llelem->data:NULL))
#define BBL_FOREACH_PDOMINATES(ibbl, llelem, jbbl)\
   for (llelem = BBL_PDOMINATES(ibbl), jbbl = T_BBL(llelem->data); llelem!=NULL; llelem = LLNext(llelem), jbbl = (llelem?llelem->data:NULL))
#define BBL_FOREACH_SUCC_EDGE(bbl,edge)\
   for (edge=BBL_SUCC_FIRST(bbl); edge!=NULL; edge=CFG_EDGE_SUCC_NEXT(edge))
#define BBL_FOREACH_SUCC_EDGE_R(bbl,edge)\
   for (edge=BBL_SUCC_LAST(bbl); edge!=NULL; edge=CFG_EDGE_SUCC_PREV(edge))
#define BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,tmp)\
   for (edge=BBL_SUCC_FIRST(bbl), tmp=edge?CFG_EDGE_SUCC_NEXT(edge):NULL; edge!=NULL; edge= tmp, tmp=edge?CFG_EDGE_SUCC_NEXT(edge):NULL)
#define BBL_FOREACH_PRED_EDGE(bbl,edge)\
   for (edge=BBL_PRED_FIRST(bbl); edge!=NULL; edge=CFG_EDGE_PRED_NEXT(edge))
#define BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge,tmp) for (edge=BBL_PRED_FIRST(bbl), tmp=edge?CFG_EDGE_PRED_NEXT(edge):NULL; edge!=NULL; edge= tmp, tmp=edge?CFG_EDGE_PRED_NEXT(edge):NULL)

#define BBL_FOREACH_INS(bbl,ins) for(ins=BBL_INS_FIRST(bbl); ins!=NULL; ins=INS_INEXT(ins))
#define BBL_FOREACH_INS_R(bbl,ins) for(ins=BBL_INS_LAST(bbl); ins!=NULL; ins=INS_IPREV(ins))
#define BBL_FOREACH_INS_SAFE(bbl,ins,tmp)   for(ins=BBL_INS_FIRST(bbl), tmp=ins?INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?INS_INEXT(ins):0)
#define BBL_FOREACH_INS_R_SAFE(bbl,ins,tmp) for(ins=BBL_INS_LAST(bbl), tmp=ins?INS_IPREV(ins):0; ins!=NULL; ins=tmp, tmp=ins?INS_IPREV(ins):0)

#define CFG_WHILE_MARKED_BBLS_PER_FUN(cfg,fun,bbl,big_change,small_change)\
  CFG_FOREACH_FUN(cfg,fun) FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) &(~FF_IS_MARKED));\
  CFG_FOREACH_BBL(cfg,bbl) if (NodeIsMarked(bbl) && BBL_FUNCTION(bbl)) FUNCTION_SET_FLAGS(BBL_FUNCTION(bbl) ,  FUNCTION_FLAGS(BBL_FUNCTION(bbl)) |FF_IS_MARKED);\
  big_change = TRUE;\
  while (!(big_change=!big_change)) \
  CFG_FOREACH_FUN(cfg,fun)\
  if ( (!(FUNCTION_FLAGS(fun)&FF_IS_MARKED)) || ((FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED)) && 0)) continue;\
  else while (!(small_change=!small_change))\
  FUNCTION_FOREACH_BBL(fun,bbl)\
  if( !NodeIsMarked(bbl) || ((NodeUnmark(bbl)) && 0)) continue;\
  else 

#define FAST_CFG_WHILE_MARKED_BBLS_PER_FUN(cfg,fun,bbl,big_change,small_change)\
  big_change = TRUE;\
  while (!(big_change=!big_change)) \
  CFG_FOREACH_FUN(cfg,fun)\
  if ( (!(FUNCTION_FLAGS(fun)&FF_IS_MARKED)) || ((FUNCTION_FLAGS(fun)&=~FF_IS_MARKED) && 0)) continue;\
  else while (!(small_change=!small_change))\
  FUNCTION_FOREACH_BBL(fun,bbl)\
  if( !NodeIsMarked(bbl) || ((NodeUnmark(bbl)) && 0)) continue;\
  else 

#define FUNCTION_WHILE_HAS_MARKED_BBLS(fun,bbl,change)\
  while (!(change=!change))\
  FUNCTION_FOREACH_BBL(fun,bbl)\
  if( !NodeIsMarked(bbl) || ((NodeUnmark(bbl)) && 0)) continue;\
  else 

/* Cg -> ??? */

#define CG_FOREACH_NODE(graph,node)\
   for (node=CG_NODE_FIRST(graph); node!=NULL; node=FUNCTION_NEXT(node))

#define SCG_FOREACH_NODE(graph,node)\
   for (node=T_SCG_NODE(GRAPH_NODE_FIRST(graph)); node!=NULL; node=T_SCG_NODE(NODE_NEXT(node)))

#define CG_FOREACH_NODE_FUN(graph,node,fun)\
   for (node=T_CG_NODE(GRAPH_NODE_FIRST(graph)), fun = (node==NULL)?NULL:CG_NODE_FUN(node); node; node=T_CG_NODE(NODE_NEXT(node)), fun = (node==NULL)?NULL:CG_NODE_FUN(node))

#define CG_FOREACH_FUN(graph,fun)\
   for (fun=CG_NODE_FIRST(graph); fun!=NULL; fun=FUNCTION_NEXT(fun))

#define CG_FOREACH_EDGE(graph,edge) for (edge=T_CG_EDGE(GRAPH_EDGE_FIRST(graph)); edge!=NULL; edge=T_CG_EDGE(EDGE_NEXT(edge)))

#define SCG_FOREACH_EDGE(graph,edge) for (edge=T_SCG_EDGE(GRAPH_EDGE_FIRST(graph)); edge!=NULL; edge=T_SCG_EDGE(EDGE_NEXT(edge)))

#define CG_NODE_FOREACH_CALLEE(from,edge,callee)\
   for (edge=T_CG_EDGE(NODE_SUCC_FIRST(from)), callee = (edge==NULL)?NULL:T_FUN(CG_EDGE_TAIL(edge)); edge; edge=T_CG_EDGE(EDGE_SUCC_NEXT(edge)),callee=(edge==NULL)?NULL:T_FUN(CG_EDGE_TAIL(edge)))
   
#define CG_NODE_FOREACH_CALLER(from,edge,caller)\
   for (edge=T_CG_EDGE(NODE_PRED_FIRST(from)), caller = (edge==NULL)?NULL:T_FUN(CG_EDGE_HEAD(edge)); edge; edge=T_CG_EDGE(EDGE_PRED_NEXT(edge)),caller=(edge==NULL)?NULL:T_FUN(CG_EDGE_HEAD(edge)))

#define FUNCTION_FOREACH_SUCC_EDGE(node,edge) for (edge=FUNCTION_SUCC_FIRST(node); edge!=NULL; edge=CG_EDGE_SUCC_NEXT(edge))
#define FUNCTION_FOREACH_PRED_EDGE(node,edge) for (edge=FUNCTION_PRED_FIRST(node); edge!=NULL; edge=CG_EDGE_PRED_NEXT(edge))

/* Cfg -> ??? */

#define CFG_FOREACH_INS(cfg,ins) for(ins=T_INS(CFG_NODE_FIRST(cfg)); ins!=NULL; ins=(RELOCATABLE_RELOCATABLE_TYPE(T_RELOCATABLE(ins))==RT_INS)?(INS_INEXT(ins)?INS_INEXT(ins):T_INS(BBL_NEXT(INS_BBL(ins)))):(BBL_INS_FIRST(T_BBL(ins))?BBL_INS_FIRST(T_BBL(ins)):T_INS(BBL_NEXT(T_BBL(ins))))) if (RELOCATABLE_RELOCATABLE_TYPE(T_RELOCATABLE(ins))==RT_INS)

#define CFG_FOREACH_BBL(graph,bbl)\
   for (bbl=T_BBL(CFG_NODE_FIRST(graph)); bbl!=NULL; bbl=BBL_NEXT(bbl))
#define CFG_FOREACH_BBL_R(graph,bbl)\
   for (bbl=T_BBL(CFG_NODE_LAST(graph)); bbl!=NULL; bbl=BBL_PREV(bbl))

#define CFG_FOREACH_FUN(cfg,fun) for (fun=CFG_FUNCTION_FIRST(cfg); fun!=NULL; fun=FUNCTION_FNEXT(fun))
#define CFG_FOREACH_FUNCTION_SAFE(cfg,fun,tmp) for (fun=CFG_FUNCTION_FIRST(cfg), tmp=fun?FUNCTION_FNEXT(fun):NULL; fun!=NULL; fun=tmp, tmp=fun?FUNCTION_FNEXT(fun):NULL)
#define CFG_FOREACH_HELL_FUNCTION(cfg,fun) for (fun = CFG_HELL_FUNCTIONS(cfg); fun; fun = FUNCTION_NEXT_HELL(fun))
#define CFG_FOREACH_LOOP(cfg,loop) for (loop=CFG_LOOP_FIRST(cfg); loop!=NULL; loop=LOOP_NEXT(loop))

#define CFG_FOREACH_BBL_SAFE(graph,bbl,safe)\
   for (bbl=CFG_NODE_FIRST(graph),safe=bbl?BBL_NEXT(bbl):NULL; bbl!=NULL; bbl=safe,safe=bbl?BBL_NEXT(bbl):NULL)

#define CFG_FOREACH_EDGE(graph,edge) for (edge=CFG_EDGE_FIRST(graph); edge!=NULL; edge=CFG_EDGE_NEXT(edge))

#define CFG_FOREACH_EDGE_SAFE(graph,edge,tmp) for (edge=CFG_EDGE_FIRST(graph), tmp=edge?CFG_EDGE_NEXT(edge):NULL; edge!=NULL; edge = tmp, tmp = edge?CFG_EDGE_NEXT(edge):NULL)

/* Chains */
#define CHAIN_FOREACH_BBL(chain_head,bbl)	for (bbl = (chain_head); bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
#define CHAIN_FOREACH_BBL_R(chain_tail,bbl)	for (bbl = (chain_tail); bbl; bbl = BBL_PREV_IN_CHAIN(bbl))

/* Loops */

/* This iterator walks over all basic blocks in a loop. 'loop' is a loop pointer, 'bbl' is a basic block pointer and 'iterator' is a t_loopiterator pointer.
 * You have to free the iterator after using this macro */
#define LOOP_FOREACH_BBL(loop,iterator,bbl)	for(iterator = LoopNewIterator(loop),bbl = LoopGetNextBbl(iterator); bbl!= NULL;bbl = LoopGetNextBbl(iterator))
#define LOOP_FOREACH_BBL_INTRAFUN(loop,iterator,bbl)	for(iterator = LoopNewIterator(loop),bbl = LoopGetNextBbl(iterator); bbl!= NULL;bbl = LoopGetNextBbl(iterator)) if(BBL_FUNCTION(bbl) == BBL_FUNCTION(LOOP_HEADER(loop)))
#define LOOP_FOREACH_ENTRY_EDGE(loop,edge)      BBL_FOREACH_PRED_EDGE(LOOP_HEADER(loop),edge) if (!LoopContainsBbl(loop,T_BBL(CFG_EDGE_HEAD(edge))))

#define LOOP_FOREACH_EXIT_EDGE(loop,iterator,bbl,edge) for(iterator = LoopNewIterator(loop),bbl = LoopGetNextBbl(iterator), edge=BBL_SUCC_FIRST(bbl);\
                                                   bbl!= NULL && edge!=NULL; \
                                                   edge = CFG_EDGE_SUCC_NEXT(edge), bbl=(edge==NULL)?LoopGetNextBbl(iterator):bbl, edge=(edge!=NULL || bbl==NULL)?edge:BBL_SUCC_FIRST(bbl)) \
                                                 if (!LoopContainsBbl(loop,CFG_EDGE_TAIL(edge)))

#if 0
#define LOOP_FOREACH_EXIT_EDGE_SAFE(loop,bbl,edge,safe) for( LoopInitIterator(loop),bbl = LoopGetNextBbl(loop), edge=T_CFG_EDGE(BBL_SUCC_FIRST(bbl)), safe=edge?T_CFG_EDGE(EDGE_NEXT(edge)):NULL;\
                                                   bbl!= NULL && edge!=NULL; \
                                                   edge = T_CFG_EDGE(EDGE_SUCC_NEXT(edge)), bbl=(edge==NULL)?LoopGetNextBbl(loop):bbl, edge=(bbl==NULL && edge==NULL)?edge:T_CFG_EDGE(BBL_SUCC_FIRST(bbl)),safe=edge?T_CFG_EDGE(EDGE_NEXT(edge)):NULL) \
                                                 if (!LoopContainsBbl(loop,T_BBL(CFG_EDGE_TAIL(edge))))
#endif
#define LOOP_FOREACH_EXIT_EDGE_SAFE(loop,iterator,bbl,edge,safe) for(iterator = LoopNewIterator(loop),bbl = LoopGetNextBbl(iterator); bbl!= NULL;bbl = LoopGetNextBbl(iterator)) \
                                                BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,safe) if (!LoopContainsBbl(loop,T_BBL(CFG_EDGE_TAIL(edge))))


                                                
#define SECTION_FOREACH_INS(code,ins) for(ins=T_INS(SECTION_DATA(code)); ins!=NULL; ins=INS_INEXT(ins))

#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
