#include <diablosupport.h>
#include <support/version.h>

void PrintVersionInformationIfRequested()
{
  if (!diablosupport_options.revision)
    return;

  VERBOSE(0, (DIABLO_VERSION_STRING));
  exit(0);
}
