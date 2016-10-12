#ifndef DIABLOFLOWGRAPH_DATAFLOW_H
#define DIABLOFLOWGRAPH_DATAFLOW_H
#endif

#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLOFLOWGRAPH_DATAFLOW_FUNCTIONS
#define DIABLOFLOWGRAPH_DATAFLOW_FUNCTIONS
/* looks for the first instruction that defines "reg" in the same bbl
 * as "startins", starting the search at "startins" and going backwards
 */
t_ins * FindDefForReg(t_reg reg, t_ins * startins);


/* returns whether any register in the regset "regs" can be modified between
 * "begin" and "end" (inclusive)
 */
t_bool RegsModifiedBetween(t_regset const *regs, t_ins *begin, t_ins *end);
#endif
#endif
