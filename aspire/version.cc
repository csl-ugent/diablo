#include <aspire/version.h>

void PrintAspireVersionInformationIfRequested()
{
  if (!diablosupport_options.revision)
    return;

  VERBOSE(0, (ASPIRE_VERSION_STRING));
  exit(0);
}
