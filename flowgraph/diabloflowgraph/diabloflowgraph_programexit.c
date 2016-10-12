#include <diabloflowgraph.h>

void
CfgCreateExitBlockList (t_cfg * cfg)
{
  t_function *fun;
  t_bbl *bbl;
  t_ins *ins;
  t_cfg_exits *exit_block;
  t_tristate is_exit;

  STATUS(START, ("Detecting program exits"));
  CFG_FOREACH_FUN(cfg, fun)
    FUNCTION_FOREACH_BBL(fun, bbl)
    BBL_FOREACH_INS(bbl, ins)
    {
      if (INS_TYPE(ins) == IT_SWI)
      {
        is_exit = CFG_DESCRIPTION(cfg)->InsIsSyscallExit (ins);
        if (is_exit != NO)
        {
          VERBOSE(1, ("Adding @B as a exit block [%s]! @I", bbl, is_exit == YES ? "certain" : "perhaps", ins));
          exit_block = (t_cfg_exits *) Malloc (sizeof (t_cfg_exits));
          exit_block->exit_bbl = bbl;
          exit_block->exit_ins = ins;
          exit_block->certain = (is_exit == YES);
          exit_block->next = CFG_EXITBLOCKS(cfg);
          exit_block->tmp = NULL;
          CFG_SET_EXITBLOCKS(cfg, exit_block);
          break;
        }
      }
    }
  STATUS(STOP, ("Detecting program exits"));
}

void
CfgFreeExitBlockList (t_cfg * cfg)
{
  t_cfg_exits *to_free;

  while (CFG_EXITBLOCKS(cfg))
  {
    to_free = CFG_EXITBLOCKS(cfg);
    CFG_SET_EXITBLOCKS(cfg, CFG_EXITBLOCKS(cfg)->next);
    Free (to_free);
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
