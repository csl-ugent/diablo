/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOSUPPORT_LOGGING_H
#define DIABLOSUPPORT_LOGGING_H

#include <diablosupport.h>
void DiabloSupportLoggingFini();

typedef t_int32 TransformationID;
static const TransformationID INVALID_TRANSFORMATION_ID = -1;

extern t_uint32 diablo_phase;
extern t_const_string diablo_phases [];

static inline t_uint32 GetDiabloPhase()                   {return diablo_phase;}
static inline t_const_string GetDiabloPhaseName(t_uint32 phase) {return diablo_phases[phase];}
static inline t_uint32 NewDiabloPhase(t_const_string name)      {diablo_phases[++diablo_phase]=StringDup(name); return diablo_phase;}
static inline void ResetDiabloPhase()                     {diablo_phase = 0;}

/*extern t_uint64 current_transformation_number;
static inline t_uint64 GetTransformationId() {return current_transformation_number;}
static inline void SetTransformationIdForDeflowgraph() { current_transformation_number = -2; }*/
void SetTransformationIdForDeflowgraph();
uint64_t GetTransformationId();
t_string GetTransformationIdCString();
TransformationID GetTransformationNumberFromId(uint64_t id);

struct LogFile
{
  FILE* fp;
  t_uint32 reference_count;
};

void RegisterOpenedLog(struct LogFile *lf);
void UnregisterOpenedLog(struct LogFile *lf);

#define LOG(lf, ...) do { if (lf) { FileIo(lf->fp, ##__VA_ARGS__); } } while(0)
#define INIT_LOGGING(lf,filename) do { if (diablosupport_options.enable_transformation_log && !lf)\
  {\
    lf = (LogFile*) Malloc(sizeof(LogFile));\
    lf->fp = fopen(filename,"w");\
    lf->reference_count = 1;\
    RegisterOpenedLog(lf);\
  }\
} while (0)
#define ATTACH_LOGGING(lf1,lf2) do {lf1 = lf2; if (lf1) lf1->reference_count++;} while (0)
#define FINI_LOGGING(lf) do { if (lf)\
  {\
    lf->reference_count--;\
    if (lf->reference_count == 0)\
    {\
      fclose(lf->fp);\
      UnregisterOpenedLog(lf);\
      Free(lf);\
    }\
    lf = NULL;\
  }\
} while (0)

#define FLUSH_LOG(lf) do { if (lf)\
  {\
    fflush(lf->fp);\
  }\
} while (0)

void StartTransformation(); /* Create/Initialize the logging directory for logging the next transformation */
t_const_string GetCurrentTransformationDirectory();
t_string FilenameForCurrentTransformation(t_const_string filename); /* Quick helper function that prepends the current transformation directory to a file name */
void EndTransformation(); /* Increases the counter for which transformation that is being logged */

#define LOG_MESSAGE(fp, message_string, ...) \
  { \
    LOG(fp, message_string, ##__VA_ARGS__); \
  }

#define START_LOGGING_TRANSFORMATION_NONEWLINE(fp, message_string, ...) \
  { \
    if (fp) { \
      StartTransformation(); \
      t_string tf_id_str = GetTransformationIdCString(); \
      LOG_MESSAGE(fp, "%s,0x%016" PRIx64 "," message_string, tf_id_str, GetTransformationId(), ##__VA_ARGS__); \
      Free(tf_id_str); \
    } \
  }

#define START_LOGGING_TRANSFORMATION(fp, message_string, ...) \
  START_LOGGING_TRANSFORMATION_NONEWLINE(fp, message_string "\n", ##__VA_ARGS__);

#define LOG_MORE(fp) \
  if (fp)

#define START_LOGGING_TRANSFORMATION_AND_LOG_MORE_NONEWLINE(fp, message_string, ...) \
  START_LOGGING_TRANSFORMATION_NONEWLINE(fp, message_string, ## __VA_ARGS__) \
  LOG_MORE(fp)

#define START_LOGGING_TRANSFORMATION_AND_LOG_MORE(fp, message_string, ...) \
  START_LOGGING_TRANSFORMATION(fp, message_string "\n", ## __VA_ARGS__)

#define STOP_LOGGING_TRANSFORMATION(fp) do { if (fp) EndTransformation(); } while (0)

void OpenKilledInstructionLog(t_string filename);
void LogKilledInstruction(t_address addr);
void LogKilledInstructionBarrier(t_string str);

t_string OutputFilename();

#endif
