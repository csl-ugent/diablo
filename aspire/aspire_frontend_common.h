/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Included C++ headers */
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

/* C headers */
extern "C"
{
#include <diablosupport.h>
#include <diabloobject.h>
#include <diabloflowgraph.h>
#include <diabloanopt.h>
#include <diabloanoptarm.h>
#include <diablo_arm_options.h>
#include <diablo_options.h>
}

/* Diablo C++ headers */
#include <diabloannotations.h>
#include <diabloflowgraph_dwarf.h>
#include <frontends/common.h>

/* Aspire-specific headers */
#include "aspire_options.h"
#include "version.h"

void
BrokerFrontendId(int *result)
{
  *result = frontend_id;
}

void OutputFilenameBroker(t_string *result)
{
  *result = StringDup(global_options.output_name);
}
