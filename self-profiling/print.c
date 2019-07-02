/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef ENABLE_LOGGING
#ifdef __ANDROID__
#include <android/log.h>
#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "Diablo", fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...) printf("Diablo - " fmt "\n", ##__VA_ARGS__)
#endif
#else
#define LOG(fmt, ...)
#endif

/* The functions that will be used by Diablo */
#ifdef LINKIN_AFTER
void DIABLO_Profiling_Init();
#else
/* Make this function an initialization routine, executed as late as possible (because of the fork */
void DIABLO_Profiling_Init() __attribute__((constructor(65500)));
#endif

/* Global variables, already initialize them so they won't end up in BSS section */
uint64_t DIABLO_Profiling_data[] __attribute__((section (".data.profiling_data"))) = { 0 };
size_t DIABLO_Profiling_nr_of_bbls = 1;
char DIABLO_Profiling_output_name[] __attribute__((section (".data.profiling_output_name"))) = "noname";

static void print()
{
  /* Remember how often we have dumped the section already */
  static uint64_t nr_of_times_written = 0;
  const uint64_t nr_of_bbls = DIABLO_Profiling_nr_of_bbls;/* Cast to uint64_t */

  /* construct correct file name */
#ifdef __ANDROID__
  char filename[256] = "/data/";
  strncat(filename, DIABLO_Profiling_output_name, sizeof(filename) - strlen(filename));
#else
  char *filename = DIABLO_Profiling_output_name;
#endif

  FILE *fd = fopen(filename, "a");
  if (!fd) {
    int err = errno;

    LOG("could not open profile file %s (%d: %s)", filename, err, strerror(err));
#ifdef __ANDROID__
    LOG("On Android, this might be solved by touching the file (touch %s) prior to running the self-profiling program.", filename);
#endif
    abort();
  }

  /* acquire lock (blocking call) */
  LOG("acquiring lock on %s", filename);
  if (flock(fileno(fd), LOCK_EX) == -1) {
    int err = errno;

    LOG("could not lock profile file %s (%d: %s)", filename, err, strerror(err));
    abort();
  }

  LOG("dumping profile data in PID %d to %s (%llu) at offset %lld", getpid(), filename, nr_of_times_written, (int64_t)ftell(fd));

  /* Write some metadata */
  fwrite(&nr_of_bbls, sizeof(uint64_t), 1, fd);
  fwrite(&nr_of_times_written, sizeof(uint64_t), 1, fd);

  nr_of_times_written++;

  /* Dump profiling section */
  fwrite((void*) DIABLO_Profiling_data, 2 * sizeof(uint64_t), DIABLO_Profiling_nr_of_bbls, fd);

  /* release lock */
  LOG("flushing and syncing file %s", filename);
  fflush(fd);
  fsync(fileno(fd));

  LOG("unlocking file %s", filename);
  if (flock(fileno(fd), LOCK_UN) == -1) {
    int err = errno;

    LOG("could not unlock profile file %s (%d: %s)", filename, err, strerror(err));
    abort();
  }

  LOG("  ... done for PID %d", getpid());

  /* Close file */
  fclose(fd);
}

/* In some cases we want to be able to print out the profiling information during the run, but we can only do this for dynamically linked applications.
 * For dynamically linked applications we can handle this in a cleaner fashion by installing an initialization routine (that also installs a signal handler).
 * The problem is that we can't link in any amount of functionality in a statically linked application in Diablo.
 */
void DIABLO_Profiling_Init()
{
  /* don't write the final profile if SP has not been initialised by Diablo */
  if (DIABLO_Profiling_nr_of_bbls == 1)
    return;

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
