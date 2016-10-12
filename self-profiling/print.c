/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Some typedefs */
typedef unsigned int t_uint32;
typedef unsigned long long int t_uint64;
typedef char* t_string;

/* We declare these variables extern as they have to be filled in by Diablo, but 
 * set their visibility to hidden so they won't end up in the GOT (Diablo can't succesfully
 * add GOT variables yet). For information about this pragma, see https://gcc.gnu.org/wiki/Visibility
 */
#pragma GCC visibility push(hidden)
extern t_uint32 DIABLO_nr_of_bbls;
extern char DIABLO_output_name[];
extern t_uint64 DIABLO_profilingSection[];
#pragma GCC visibility pop

/* Global variables, already initialize them so they won't end up in BSS section */
t_uint32 DIABLO_nr_of_bbls = 42;

#ifdef SIGNAL
static void print()
#else
void DIABLO_print()
#endif
{
  /* Remember how often we have dumped the section already */
  static t_uint64 nr_of_times_written = 0;
  const t_uint64 nr_of_bbls = DIABLO_nr_of_bbls;/* Cast to t_uint64 */

  /* Open the file */
  FILE* fd = fopen(DIABLO_output_name, "a");

  /* Write some metadata */
  fwrite(&nr_of_bbls, sizeof(t_uint64), 1, fd);
  fwrite(&nr_of_times_written, sizeof(t_uint64), 1, fd);
  nr_of_times_written++;

  /* Dump profiling section */
  fwrite((void*) DIABLO_profilingSection, 2 * sizeof(t_uint64), DIABLO_nr_of_bbls, fd);

  /* Close file */
  fclose(fd);
}

/* In some cases we want to be able to print out the profiling information during the run, but we can only do this for dynamically linked applications.
 * For dynamically linked applications we can handle this in a cleaner fashion by installing an initialization routine (that also installs a signal handler).
 * The problem is that we can't link in any amount of functionality in a statically linked application in Diablo.
 */
#ifdef SIGNAL
void DIABLO_Init()
{
  /* Install the print routine to be executed when the SIGUSR2 signal is sent */
  signal(SIGUSR2, print);

  /* Install the print routine to executed when the parent exits. For some vague reason atexit (while part of the C standard) is not actually exported by
   * glibc, so we can't use it.. On the other hand, bionic DOES have atexit, but doesn't have on_exit. Fuck my life.
   */
#ifdef __ANDROID__
  atexit(print);
#else
  on_exit(print, NULL);
#endif
}
#endif
