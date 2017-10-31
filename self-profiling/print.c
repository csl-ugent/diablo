/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* The functions that will be used by Diablo */
#ifdef LINKIN_AFTER
void DIABLO_Profiling_Init();
#else
/* Make this function an initialization routine, executed as late as possible (because of the fork */
void DIABLO_Profiling_Init() __attribute__((constructor(65500)));
#endif

/* Global variables, already initialize them so they won't end up in BSS section */
uint64_t DIABLO_Profiling_data[] __attribute__((section (".data.profiling_data"))) = { 0 };
size_t DIABLO_Profiling_nr_of_bbls = 42;
char DIABLO_Profiling_output_name[] __attribute__((section (".data.profiling_output_name"))) = "noname";

static void print()
{
  /* Remember how often we have dumped the section already */
  static uint64_t nr_of_times_written = 0;
  const uint64_t nr_of_bbls = DIABLO_Profiling_nr_of_bbls;/* Cast to uint64_t */

  /* Open the file */
  FILE* fd = fopen(DIABLO_Profiling_output_name, "a");
  if (!fd)
  {
    /* If we can't open a file in the current directory (as might happen in Android), create it in /data/ */
    char filename[256] = "/data/";
    strncpy(filename + strlen(filename), DIABLO_Profiling_output_name, sizeof(filename) - (strlen(filename)));
    fd = fopen(filename, "a");
  }

  /* Write some metadata */
  fwrite(&nr_of_bbls, sizeof(uint64_t), 1, fd);
  fwrite(&nr_of_times_written, sizeof(uint64_t), 1, fd);
  nr_of_times_written++;

  /* Dump profiling section */
  fwrite((void*) DIABLO_Profiling_data, 2 * sizeof(uint64_t), DIABLO_Profiling_nr_of_bbls, fd);

  /* Close file */
  fclose(fd);
}

/* In some cases we want to be able to print out the profiling information during the run, but we can only do this for dynamically linked applications.
 * For dynamically linked applications we can handle this in a cleaner fashion by installing an initialization routine (that also installs a signal handler).
 * The problem is that we can't link in any amount of functionality in a statically linked application in Diablo.
 */
void DIABLO_Profiling_Init()
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
