/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLO_CALLSTACKS_H
#define DIABLO_CALLSTACKS_H

/* Include used C++ headers */
#include <string>
#include <vector>

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloanopt.h>
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
  #include <diabloobject.h>
  #include <diablosupport.h>
}

#include <diabloannotations.h>
#include <string>

static const std::string callcheck_token = "call_stack_check";

struct CallStackCheckAnnotationInfo
        : public AbstractAnnotationInfo
{
  CallStackCheckAnnotationInfo() : call_depth(0) {}
  int call_depth;

  virtual AbstractAnnotationInfo* clone() const { return new CallStackCheckAnnotationInfo(); }

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);

  virtual void preprocessRegion(Region* region) {
    if (call_depth > 0) {
      ExpandRegionToCalleesOfDepth(region, call_depth);
    }
  }
};

struct CallStackCheckAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new CallStackCheckAnnotationInfo(); }
};

#define CFG_FOREACH_CALLCHECK_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<CallStackCheckAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CallStackCheckAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#define BBL_FOREACH_CALLCHECK_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
    for (auto request_it__ = CreateRegionIterator<CallStackCheckAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CallStackCheckAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

void ApplyCallStackChecks(t_cfg* cfg);

extern LogFile* L_CALLCHECKS;

#endif /* DIABLO_CALLSTACKS_H */
