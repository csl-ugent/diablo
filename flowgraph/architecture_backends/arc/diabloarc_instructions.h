#include <diabloarc.h>

#ifndef ARC_INSTRUCTION_H
#define ARC_INSTRUCTION_H

/* some convenience macros */
#define ARC_INS_OBJECT(ins) 		CFG_OBJECT(ARC_INS_CFG(ins))
#define ARC_INS_IS_CONDITIONAL(x)	(ARC_INS_ATTRIB(x) & IF_CONDITIONAL)
#define BBL_FOREACH_ARC_INS(bbl,ins) 	for(ins=T_ARC_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=ARC_INS_INEXT(ins))
#define BBL_FOREACH_ARC_INS_R(bbl,ins)	for(ins=T_ARC_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=ARC_INS_IPREV(ins))
#define BBL_FOREACH_ARC_INS_SAFE(bbl,ins,tmp)   for(ins=T_ARC_INS(BBL_INS_FIRST(bbl)), tmp=ins?ARC_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?ARC_INS_INEXT(ins):0)
#define SECTION_FOREACH_ARC_INS(code,ins) for(ins=T_ARC_INS(SECTION_DATA(code)); ins!=NULL; ins=ARC_INS_INEXT(ins))

#define T_ARC_INS(arc_ins)            ((t_arc_ins *) arc_ins)

#endif


#ifdef DIABLOARC_FUNCTIONS
#ifndef ARC_INSTRUCTION_FUNCTIONS
#define ARC_INSTRUCTION_FUNCTIONS
/* function declarations for the corresponding .c file go here */
#endif
#endif

/* vim: set shiftwidth=2 foldmethod=marker: */
