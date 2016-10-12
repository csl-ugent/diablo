
#include <diabloanopt.h>
#include <diabloanopti386.h>
#include "simple_frontend_options.h"

int main (int argc, char **argv)
{
  t_object *obj;
  t_cfg *cfg;

  /* initialize the modules you're going to use */
  DiabloFlowgraphInit (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAnoptI386Init (argc, argv);

  /* the command line options stuff is not for mere mortals:
   * only Bruno really understands it all */
  FrontendOptionsInit ();
  OptionParseCommandLine (frontend_option_list, argc, argv, FALSE);
  OptionGetEnvironment (frontend_option_list);
  FrontendOptionsVerify ();
  OptionDefaults (frontend_option_list);

  /* read and link */
  obj = LinkEmulate (frontend_options.objectfilename, FALSE);

  /* disassemble */
  ObjectDisassemble (obj);

  /* build control flow graph */
  ObjectFlowgraph (obj, NULL, NULL);
  cfg = OBJECT_CFG(obj);
  CfgRemoveDeadCodeAndDataBlocks (cfg); /* remove unreachable code and data */
  /* split functions so they are all single-entry: some Diablo analyses and
   * optimizations expect this */
  CfgPatchToSingleEntryFunctions (cfg);


  /*** Insert your own analysis/optimization code here ***/


  /* deflowgraph/assemble/write out */
  ObjectDeflowgraph (obj);
  ObjectRebuildSectionsFromSubsections (obj);
  ObjectAssemble (obj);
  ObjectWrite (obj, frontend_options.output_name);

  return 0;
}

