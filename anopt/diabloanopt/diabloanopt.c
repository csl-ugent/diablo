#include <diabloanopt.h>
	
static int anopt_module_usecount = 0;
	
void
DiabloAnoptCmdlineVersion ()
{
  printf ("DiabloAnopt version %s\n", DIABLOANOPT_VERSION);
}

void
DiabloAnoptInit (int argc, char **argv)
{
  if (!anopt_module_usecount)
  {
    DiabloFlowgraphInit (argc, argv);
    DiabloAnoptCmdlineInit ();
    OptionParseCommandLine (diabloanopt_option_list, argc, argv,FALSE);
    OptionGetEnvironment (diabloanopt_option_list);
    DiabloAnoptCmdlineVerify ();
    OptionDefaults (diabloanopt_option_list);
    IoModifierAdd ('@', 'C', "lt-=", IoModifierProcState);
  }
  anopt_module_usecount++;
}

void 
DiabloAnoptFini()
{
  anopt_module_usecount--;
  if (!anopt_module_usecount)
  {
    DiabloAnoptCmdlineFini ();
    DiabloFlowgraphFini ();
  }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
