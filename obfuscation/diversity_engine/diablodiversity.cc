/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef __cplusplus
extern "C" {
#endif

#include <diablodiversity.h>

void DiversityOptionsVersion();

#ifdef __cplusplus
}
#endif



#ifdef _MSC_VER
// automake doesn't generate the version define
#define DIABLODIVERSITY_VERSION "0.1"
#endif

void 
DiversityOptionsVersion()
{
  printf("DiabloStego version %s\n",DIABLODIVERSITY_VERSION);
}
