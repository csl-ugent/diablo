#include <diabloflowgraph.h>
#include "reachable_cmdline.h"

t_dynamic_member_info bbl_reachable_array = null_info;


void 
BblReachableInit(t_bbl * bbl, t_bool * reachable)
{
  *reachable = FALSE;
}

void 
BblReachableFini(t_bbl * bbl, t_bool * reachable)
{
  return;
}

void 
BblReachableDup(t_bbl * bbl, t_bool * reachable)
{
  return;
}

DYNAMIC_MEMBER(bbl, t_cfg *, bbl_reachable_array, t_bool, reachable, REACHABLE, Reachable, CFG_FOREACH_BBL, BblReachableInit, BblReachableFini, BblReachableDup);

void 
MarkBblAndSuccessors(t_bbl * bbl)
{
  t_cfg_edge * edge;
  if(BBL_REACHABLE(bbl))
    return;
  BBL_SET_REACHABLE(bbl,TRUE);
  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    MarkBblAndSuccessors(CFG_EDGE_TAIL(edge));
  }
}

t_int32
RemoveUnreachableBasicBlocks(t_cfg * cfg)
  /* Transform cfg, return 0 if success, <0, or FATAL if error  */
{
  t_bbl * entry = CFG_UNIQUE_ENTRY_NODE(cfg), * bbl, * tmp;
  
  BblInitReachable(cfg);
  
  MarkBblAndSuccessors(entry);

  CFG_FOREACH_BBL_SAFE(cfg, bbl, tmp)
  {
    if(!BBL_REACHABLE(bbl))
      BblKill(bbl);
  }
  
  BblFiniReachable(cfg);
}


void ObjectRewrite(t_string name,int (*func)(t_cfg *),t_string oname)
{
  t_object *obj;
  t_cfg *cfg;
  /* Restore a dumped program, it will be loaded by ObjectGet */
  obj = LinkEmulate (name);

  /* 1. Disassemble */

  ObjectDisassemble (obj);
  /* 2.  Create the flowgraph */
  ObjectFlowgraph (obj, NULL, NULL);
  cfg = OBJECT_CFG(obj);

  func(cfg);

  ObjectDeflowgraph (obj);

  /* rebuild the layout of the data sections
   * so that every subsection sits at it's new address */
  ObjectRebuildSectionsFromSubsections (obj);

  ObjectAssemble (obj);

  ObjectWrite (obj, oname);

#ifdef DIABLOSUPPORT_HAVE_STAT
  /* make the file executable */
  chmod (oname,
	 S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
	 S_IXOTH);
#endif

}


int
main(int argc, char ** argv)
{
  t_string objectname=NULL;
  DiabloFlowgraphInit(argc,argv); // Inits Object, Support, all backends,...

  /* Parse your own arguments, to obtain input and output*/
  OptionParseCommandLine(reachable_list,argc,argv,TRUE);
  /* B. Get enviromental variables (Commandline supersedes) */
  OptionGetEnvironment(reachable_list);
  /* C. Set default arguments (Commandline and Environment supersedes) */
  OptionDefaults(reachable_list);

  ObjectRewrite(reachable_options.input_name,RemoveUnreachableBasicBlocks,reachable_options.output_name);

  DiabloFlowgraphFini();
}
