/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define GENERATE_CLASS_CODE
#include <diablosupport.h>

//#define LINUX_TIMING
#ifdef LINUX_TIMING
#include <time.h>
static clock_t start_times[100];
#endif
static t_uint32 indent = 0;
void *global_hack_dup_orig;

static void
FatalIo(t_uint32 id, t_const_string out)
{
  printf ("FATAL ERROR <%s:%d>: %s\n", io_wrapper_file, io_wrapper_lnno, out);
}

static void
DebugIo(t_uint32 id, t_const_string out)
{
  printf ("<DEBUG> %s\n", out);
}

static void
MessageIo(t_uint32 id, t_const_string out)
{
  t_uint32 tel;

  for (tel = 0; tel < indent; tel++)
    printf (" ");
  printf ("%s\n", out);
}

static void
StatusIo(t_uint32 id, t_const_string out)
{
  t_uint32 tel;

  if (io_wrapper_status == START)
  {
    for (tel = 0; tel < indent; tel++)
      printf (" ");
    printf ("Start: %s\n", out);
#ifdef LINUX_TIMING
    start_times[indent]=clock();
#endif
    indent++;
    
  }
  else
  {
    indent--;
#ifdef LINUX_TIMING
    double CPUsecs = (double)(clock() - start_times[indent])/(double)CLOCKS_PER_SEC;
#endif
    for (tel = 0; tel < indent; tel++)
      printf (" ");
#ifdef LINUX_TIMING
    printf ("End:   %s (cpu time %7.2fs)\n", out, CPUsecs);
#else
    printf ("End:   %s\n", out);
#endif
  }
}

void
DiabloSupportInit32 (int argc, char **argv)
{
  IoModifierAdd ('@', 'G', "", IoModifierAddress32);
  DiabloSupportInitCommon (argc, argv);
}

void
DiabloSupportInit64 (int argc, char **argv)
{
  IoModifierAdd ('@', 'G', "", IoModifierAddress64);
  DiabloSupportInitCommon (argc, argv);
}

void
DiabloSupportInitGeneric (int argc, char **argv)
{
  IoModifierAdd ('@', 'G', "", IoModifierAddressGeneric);
  DiabloSupportInitCommon (argc, argv);
}

void
DiabloSupportCmdlineVersion ()
{
  printf ("DiabloSupport version %s\n", DIABLOSUPPORT_VERSION);
}

void
DiabloSupportInitCommon (int argc, char **argv)
{
  IoHandlerAdd (E_DEBUG, DebugIo);
  IoHandlerAdd (E_FATAL, FatalIo);
  IoHandlerAdd (E_MESSAGE, MessageIo);
  IoHandlerAdd (E_WARNING, MessageIo);
  IoHandlerAdd (E_STATUS, StatusIo);
  IoModifierAdd ('%', 0, NULL, IoModifierPrintf);
  IoModifierAdd ('@', 'P', "", IoModifierPath);
  PathInit ();
  DiabloSupportCmdlineInit ();
  OptionParseCommandLine (diablosupport_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablosupport_option_list);
  DiabloSupportCmdlineVerify ();
  OptionDefaults (diablosupport_option_list);

  /* Always flush file streams, even if they are redirected (helps with debugging crashes) */
  setlinebuf(stdout);
  setlinebuf(stderr);
}

void
DiabloSupportFini ()
{
  DiabloSupportCmdlineFini ();
  PathFini ();
  IoHandlersFree ();
  IoModifiersFree ();
  DiabloBrokerFini ();
  DiabloSupportLoggingFini();
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
