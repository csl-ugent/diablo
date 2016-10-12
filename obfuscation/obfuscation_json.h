/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef OBFUSCATION_JSON_H
#define OBFUSCATION_JSON_H

extern "C" {
#include <diablosupport.h>
}

#include <diabloannotations.h>
#include <string>
#include <map>

static const std::string obfuscations_token = "obfuscations";

struct ObfuscationAnnotationInfo
        : public AbstractAnnotationInfo {
  bool enable;

  virtual AbstractAnnotationInfo* clone() const
  {
    ObfuscationAnnotationInfo *info = new ObfuscationAnnotationInfo(*this);
    info->enable = enable;
    return info;
  }

  void parseOptions(AnnotationRequests& result, const std::string& options, bool enable);

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
};

struct ObfuscationAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new ObfuscationAnnotationInfo(); }
};

/* Usage:
  Region *region;
  const SoftVMInfo *info;
  CFG_FOREACH_SOFTVM_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_OBFUSCATION_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<ObfuscationAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<const ObfuscationAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#endif
