#include <diabloflowgraph.h>

/* looks for the first instruction that defines "reg" in the same bbl
 * as "startins", starting the search at "startins" and going backwards
 */
/*ArmFindDefForReg{{{*/
t_ins * FindDefForReg(t_reg reg, t_ins *startins)
{
  t_ins *runner;

  runner = startins;
  do
  {
    if (RegsetIn(INS_REGS_DEF(runner),reg))
      return runner;
    runner = INS_IPREV(runner);
  } while (runner);
  return NULL;
}
/*}}}*/

/* returns whether any register in the regset "regs" can be modified between
 * "begin" and "end" (inclusive)
 */
/*ArmRegsModifiedBetween{{{*/
t_bool RegsModifiedBetween(t_regset const *regs, t_ins *begin, t_ins *end)
{
  t_ins *runner;

  runner = begin;
  do
  {
    if (!RegsetIsMutualExclusive(*regs,INS_REGS_DEF(runner)))
      return TRUE;
    runner = INS_INEXT(runner);
  } while (runner != INS_INEXT(end));
  return FALSE;
}
/*}}}*/

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
