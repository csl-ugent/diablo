/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#define PHASE_MAX 512

t_uint32 diablo_phase = 0;

t_const_string diablo_phases [PHASE_MAX] = {NULL};

void DiabloSupportLoggingFini()
{
  t_uint32 iii;
  for (iii = 0; iii < PHASE_MAX; iii++)
    if(diablo_phases[iii])
      Free(diablo_phases[iii]);
}

static t_string current_transformation_directory = NULL;
t_uint64 current_transformation_number = 0;

void StartTransformation() {
  if (!diablosupport_options.enable_transformation_dumps)
    return;

  /* If the parent directory does not exist we cannot create subdirectories */
  DirMake(diablosupport_options.transformation_log_path, FALSE /* tmp */);

  /* TODO: clean subdirectory */
  current_transformation_directory = StringIo("%s/transformation-%"PRId64, diablosupport_options.transformation_log_path, current_transformation_number);

  DirMake(current_transformation_directory, FALSE);
}

t_const_string GetCurrentTransformationDirectory() {
  return current_transformation_directory;
}

void EndTransformation() {
  current_transformation_number++;

    if (!diablosupport_options.enable_transformation_dumps)
    return;

  Free(current_transformation_directory);
  current_transformation_directory = NULL;
}
