/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef CODE_MOBILITY_JSON_H
#define CODE_MOBILITY_JSON_H

#include <diabloannotations.h>

static const std::string codemobility_token = "code_mobility";

struct CodeMobilityAnnotationInfo
        : public AbstractAnnotationInfo
{
  bool transform = true;
  bool transform_data = false;

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
  virtual AbstractAnnotationInfo* clone() const { return new CodeMobilityAnnotationInfo(); }
};

struct CodeMobilityAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new CodeMobilityAnnotationInfo(); }
};

/* Usage:
  Region *region;
  CodeMobilityInfo *info;
  BBL_FOREACH_CODEMOBILITY_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_CODEMOBILITY_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
  for (auto request_it__ = CreateRegionIterator<CodeMobilityAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CodeMobilityAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  CodeMobilityInfo *info;
  CFG_FOREACH_CODEMOBILITY_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_CODEMOBILITY_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<CodeMobilityAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CodeMobilityAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

void BblIsInCodeMobilityRegionBroker(t_bbl *bbl, t_bool *result);

#endif /* CODE_MOBILITY_JSON_H */
