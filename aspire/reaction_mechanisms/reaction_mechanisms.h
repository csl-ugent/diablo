/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef REACTION_MECHANISMS_H
#define REACTION_MECHANISMS_H

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
}
#include <diabloannotations.h>

#include <string>
#include <vector>

void AddReactions(t_object* obj);
void AddReactionForceReachables(const t_object* obj, std::vector<std::string>& reachable_vector);

#endif
