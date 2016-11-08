/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOSUPPORT_LOGGING_H
#define DIABLOSUPPORT_LOGGING_H

#include <diablosupport.h>
void DiabloSupportLoggingFini();

extern t_uint32 diablo_phase;
extern t_const_string diablo_phases [];

static inline t_uint32 GetDiabloPhase()                   {return diablo_phase;}
static inline t_const_string GetDiabloPhaseName(t_uint32 phase) {return diablo_phases[phase];}
static inline t_uint32 NewDiabloPhase(t_const_string name)      {diablo_phases[++diablo_phase]=StringDup(name); return diablo_phase;}
static inline void ResetDiabloPhase()                     {diablo_phase = 0;}

extern t_uint64 current_transformation_number;

struct LogFile
{
  FILE* fp;
  t_uint32 reference_count;
};


#define LOG(lf, ...) do { if (lf) { FileIo(lf->fp, ##__VA_ARGS__); } } while(0)
#define INIT_LOGGING(lf,filename) do { if (diablosupport_options.enable_transformation_log && !lf)\
  {\
    lf = (LogFile*) Malloc(sizeof(LogFile));\
    lf->fp = fopen(filename,"w");\
    lf->reference_count = 1;\
  }\
} while (0)
#define ATTACH_LOGGING(lf1,lf2) do {lf1 = lf2; if (lf1) lf1->reference_count++;} while (0)
#define FINI_LOGGING(lf) do { if (lf)\
  {\
    lf->reference_count--;\
    if (lf->reference_count == 0)\
    {\
      fclose(lf->fp);\
      Free(lf);\
    }\
    lf = NULL;\
  }\
} while (0)

void StartTransformation(); /* Create/Initialize the logging directory for logging the next transformation */
t_const_string GetCurrentTransformationDirectory();
t_string FilenameForCurrentTransformation(t_const_string filename); /* Quick helper function that prepends the current transformation directory to a file name */
void EndTransformation(); /* Increases the counter for which transformation that is being logged */

#define START_LOGGING_TRANSFORMATION(fp, message_string, ...) \
  { \
    if (fp) { \
      StartTransformation(); \
      LOG(fp, "%" PRIu64 "," message_string "\n", current_transformation_number, ##__VA_ARGS__); \
    } \
  }

#define LOG_MORE(fp) \
  if (fp)

#define START_LOGGING_TRANSFORMATION_AND_LOG_MORE(fp, message_string, ...) \
  START_LOGGING_TRANSFORMATION(fp, message_string, ## __VA_ARGS__) \
  LOG_MORE(fp)

#define STOP_LOGGING_TRANSFORMATION(fp) do { if (fp) EndTransformation(); } while (0)

#endif
