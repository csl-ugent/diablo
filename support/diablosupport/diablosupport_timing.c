/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <time.h>
#include "diablosupport_timing.h"

static double  theseSecs = 0.0;
static double  startSecs = 0.0;
static clock_t starts;

void start_CPU_time()
{     
  starts = clock();
  return;
}

double end_CPU_time()
{
  return (double)(clock() - starts)/(double)CLOCKS_PER_SEC;
}   

struct timespec tp1;
static void getSecs()
{
  clock_gettime(CLOCK_REALTIME, &tp1);
  theseSecs =  tp1.tv_sec + tp1.tv_nsec / 1e9;
  return;
}

void start_time()
{
  getSecs();
  startSecs = theseSecs;
  return;
}

double end_time()
{
  getSecs();
  return theseSecs - startSecs;
}