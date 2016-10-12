/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <algorithm>
#include <vector>

extern "C" {
#include <diablosupport_options.h>
}

using namespace std;

static vector<t_option*> global_optionslists;
void AddOptionsListInitializer(t_option* optionlist) {
  /* avoid adding multiple initializers */
  if (find(global_optionslists.begin(), global_optionslists.end(), optionlist) != global_optionslists.end())
    return;

  global_optionslists.push_back(optionlist);
}

void ParseRegisteredOptionLists(t_uint32 argc, char ** argv)
{
  for (auto options_list : global_optionslists)
  {
    OptionParseCommandLine (options_list, argc, argv, FALSE);
    OptionGetEnvironment (options_list);
    OptionDefaults (options_list);
  }
}
