/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef SELF_DEBUGGING_JSON_H
#define SELF_DEBUGGING_JSON_H

#include <diabloannotations.h>

static const std::string selfdebugging_token = "anti_debugging";

struct SelfDebuggingAnnotationInfo
        : public AbstractAnnotationInfo
{
  bool transform = true;

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
  virtual AbstractAnnotationInfo* clone() const { return new SelfDebuggingAnnotationInfo(); }
};

struct SelfDebuggingAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new SelfDebuggingAnnotationInfo(); }
};

/* Usage:
  Region *region;
  SelfDebuggingInfo *info;
  BBL_FOREACH_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_SELFDEBUGGING_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
  for (auto request_it__ = CreateRegionIterator<SelfDebuggingAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<SelfDebuggingAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  SelfDebuggingInfo *info;
  CFG_FOREACH_SELFDEBUGGING_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_SELFDEBUGGING_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<SelfDebuggingAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<SelfDebuggingAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

void BblIsInSelfDebuggingRegionBroker(t_bbl *bbl, t_bool *result);

#endif
