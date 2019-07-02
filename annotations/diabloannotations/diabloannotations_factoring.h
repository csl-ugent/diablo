#ifndef FACTORING_JSON_H
#define FACTORING_JSON_H

#include <diabloannotations.h>
#include <string>

static const std::string factoring_token = "factoring";

struct FactoringAnnotationInfo
        : public AbstractAnnotationInfo
{
  virtual AbstractAnnotationInfo* clone() const { return new FactoringAnnotationInfo(); }

  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content) {
    size_t pos = 0;
    pos = eat_character_and_spaces(annotation_content, '(', pos);

    size_t end = annotation_content.find(')', pos);
    ASSERT(end != std::string::npos, ("No matching ')' found in %s", annotation_content.c_str()));

    ParseOptions(result, annotation_content.substr(pos, end - pos), factoring_token);

    delete this;
  }
};

struct FactoringAnnotationInfoFactory
        : public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new FactoringAnnotationInfo(); }
};

#define CFG_FOREACH_FACTORING_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<FactoringAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<FactoringAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#define BBL_FOREACH_FACTORING_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
    for (auto request_it__ = CreateRegionIterator<FactoringAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<FactoringAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

extern "C"
{
t_bool BblIsInFactoringRegion(t_bbl *bbl);
}

#endif
