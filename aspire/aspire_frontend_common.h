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

/* Aspire-specific headers */
#include "aspire_options.h"
#include "version.h"

/* Create an array of (actually immutable) t_const_string's from a vector<string> */
t_const_string* stringVectorToConstStringArray(const std::vector<std::string> &v)
{
  if (!v.empty())
  {
    t_const_string* cc = new t_const_string[v.size()+1];
    t_uint32 idx = 0;

    for (const std::string& s : v)
    {
      cc[idx] = s.c_str();
      idx++;
    }

    cc[idx] = NULL;

    return cc;
  }

  return NULL;
}

