/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOANNOTATIONS_ANNOTATION_FACTORY_H
#define DIABLOANNOTATIONS_ANNOTATION_FACTORY_H

#include "diabloannotations.h"
#include <string>

struct AbstractAnnotationInfoFactory
{
  virtual ~AbstractAnnotationInfoFactory() {}
  virtual AbstractAnnotationInfo* create() = 0;
};

void RegisterAnnotationInfoFactory(std::string token, AbstractAnnotationInfoFactory *factory);
void UnregisterAllAnnotationInfoFactories();
AbstractAnnotationInfoFactory *GetAnnotationInfoFactoryForToken(std::string token);

#endif /* DIABLOANNOTATIONS_ANNOTATION_FACTORY_H */
