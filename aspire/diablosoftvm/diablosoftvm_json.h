/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_JSON_H
#define DIABLOSOFTVM_JSON_H

#include <diabloannotations.h>
#include "diablosoftvm.h"
#include <string>

static const std::string softvm_token = "softvm";
static const std::string enable_softvm_option = "softvm";
static const std::string disable_softvm_option = "application";
static const std::string mobile_softvm_option = "mobile";

struct SoftVMAnnotationInfo
        : public AbstractAnnotationInfo
{
  virtual AbstractAnnotationInfo* clone() const { return new SoftVMAnnotationInfo(); }

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content) {
    size_t pos = 0;
    if (annotation_content.find('(', pos) == std::string::npos)
    {
      /* if no options are specified, enable the protection by default */
      ParseOptions(result, enable_softvm_option, softvm_token);
      return;
    }

    pos = eat_character_and_spaces(annotation_content, '(', pos);

    size_t end = annotation_content.find(')', pos);
    ASSERT(end != std::string::npos, ("No matching ')' found in %s", annotation_content.c_str()));

    /* The append here appends to any previous obfuscations for this functions. Later, when we go over line numbers, we need to verify order in which they are added
     and applied later. */
    ParseOptions(result, annotation_content.substr(pos, end - pos), softvm_token);
  }
};

struct SoftVMAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new SoftVMAnnotationInfo(); }
};

/* Usage:
  Region *region;
  SoftVMInfo *info;
  CFG_FOREACH_SOFTVM_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_SOFTVM_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<SoftVMAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<SoftVMAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#define BBL_FOREACH_SOFTVM_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
    for (auto request_it__ = CreateRegionIterator<SoftVMAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<SoftVMAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#endif /* DIABLOSOFTVM_JSON_H */
