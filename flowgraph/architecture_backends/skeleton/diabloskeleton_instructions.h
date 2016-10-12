#include <diabloskeleton.h>

#ifndef SKELETON_INSTRUCTION_H
#define SKELETON_INSTRUCTION_H

/* some convenience macros */
#define SKELETON_INS_OBJECT(ins) 		CFG_OBJECT(SKELETON_INS_CFG(ins))
#define SKELETON_INS_IS_CONDITIONAL(x)	(SKELETON_INS_ATTRIB(x) & IF_CONDITIONAL)
#define BBL_FOREACH_SKELETON_INS(bbl,ins) 	for(ins=T_SKELETON_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=SKELETON_INS_INEXT(ins))
#define BBL_FOREACH_SKELETON_INS_R(bbl,ins)	for(ins=T_SKELETON_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=SKELETON_INS_IPREV(ins))
#define BBL_FOREACH_SKELETON_INS_SAFE(bbl,ins,tmp)   for(ins=T_SKELETON_INS(BBL_INS_FIRST(bbl)), tmp=ins?SKELETON_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?SKELETON_INS_INEXT(ins):0)
#define SECTION_FOREACH_SKELETON_INS(code,ins) for(ins=T_SKELETON_INS(SECTION_DATA(code)); ins!=NULL; ins=SKELETON_INS_INEXT(ins))

#define T_SKELETON_INS(skeleton_ins)            ((t_skeleton_ins *) skeleton_ins)

#endif


#ifdef DIABLOSKELETON_FUNCTIONS
#ifndef SKELETON_INSTRUCTION_FUNCTIONS
#define SKELETON_INSTRUCTION_FUNCTIONS
/* function declarations for the corresponding .c file go here */
#endif
#endif

/* vim: set shiftwidth=2 foldmethod=marker: */
