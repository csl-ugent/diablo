/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloannotations.h"

#include <map>

using namespace std;

typedef map<string, AbstractAnnotationInfoFactory *> t_token_factory_map;

static t_token_factory_map token_factory_map;

void RegisterAnnotationInfoFactory(string token, AbstractAnnotationInfoFactory *factory)
{
  token_factory_map[token] = factory;
}

AbstractAnnotationInfoFactory *GetAnnotationInfoFactoryForToken(string token)
{
  if (token_factory_map.find(token) == token_factory_map.end())
    return NULL;

  return token_factory_map[token];
}

void UnregisterAllAnnotationInfoFactories()
{
  for (auto pair : token_factory_map)
    delete pair.second;
}
