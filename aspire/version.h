/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef ASPIRE_VERSION_H
#define ASPIRE_VERSION_H

extern "C" {
#include <diablosupport.h>
#include <support/version.h>
}

#ifdef DIABLO_MAJOR_VERSION
#undef DIABLO_MAJOR_VERSION
#endif

#ifdef DIABLO_MINOR_VERSION
#undef DIABLO_MINOR_VERSION
#endif

#define DIABLO_MAJOR_VERSION "2"
#define DIABLO_MINOR_VERSION "10"
#define DIABLO_PATCH_VERSION "1"

#define ASPIRE_VERSION_STRING "Diablo (Aspire) " DIABLO_MAJOR_VERSION "." DIABLO_MINOR_VERSION "." DIABLO_PATCH_VERSION " (rev. " DIABLO_SVN_REVISION ")"

void PrintAspireVersionInformationIfRequested();

#endif
