/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef ATTESTATION_JSON_H
#define ATTESTATION_JSON_H

#include <attestation.h>
struct Attestator;

static const std::string attestator_token = "guard_attestator";
static const std::string codeguard_token = "guarded_region";
static const std::string remoteattestation_token = "remote_attestation";

struct AttestationAnnotationInfo
: public AbstractAnnotationInfo
{
  t_uint32 index;/* The index of the area associated to the info */
  static t_const_string AID_string;
  static t_const_string output_name;
  std::set<Attestator*> attestators;/* The attestators for this region */

  AttestationAnnotationInfo() {}
  AttestationAnnotationInfo(t_uint32 index) : index(index) {}

  virtual AbstractAnnotationInfo* clone() const = 0;
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content) = 0;
};

struct AttestatorAnnotationInfo
: public AbstractAnnotationInfo
{
  Attestator* attestator;
  t_uint32 area_id;

  AttestatorAnnotationInfo() {}
  AttestatorAnnotationInfo(Attestator* attestator) : attestator(attestator) {}

  virtual AbstractAnnotationInfo* clone() const { return new AttestatorAnnotationInfo(attestator); }
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
};

struct AttestatorAnnotationInfoFactory
: public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new AttestatorAnnotationInfo(); }
};

struct CodeGuardAnnotationInfo
: public AbstractAnnotationInfo
{
  std::string label;/* The label of the guarded region */

  CodeGuardAnnotationInfo() {}

  virtual AbstractAnnotationInfo* clone() const { return new CodeGuardAnnotationInfo(); }
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
};

struct CodeGuardAnnotationInfoFactory
: public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new CodeGuardAnnotationInfo(); }
};

struct RemoteAttestationAnnotationInfo
: public AttestationAnnotationInfo
{
  RemoteAttestationAnnotationInfo() {}
  RemoteAttestationAnnotationInfo(t_uint32 index): AttestationAnnotationInfo(index) {}

  bool at_startup = false;

  virtual AbstractAnnotationInfo* clone() const { return new RemoteAttestationAnnotationInfo(this->index); }
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content);
};

struct RemoteAttestationAnnotationInfoFactory
: public AbstractAnnotationInfoFactory
{
  virtual AbstractAnnotationInfo *create() { return new RemoteAttestationAnnotationInfo(); }
};

/* This is specially created AnnotationInfo that is not directly described by an annotation, but rather created
 * at a later moment when the individual code guard regions are merged into larger regions that are attested by
 * the same call(s). Therefore it doesn't have a factory, nor does parseAnnotationContent have an actual implementation.
 */
struct CodeGuardAttestationAnnotationInfo
: public AttestationAnnotationInfo
{
  /* We keep a map of all complete code guard annotations, with their labels as key. This label usually consists of
   * the labels of multiple regions.
   */
  typedef std::map<std::string, CodeGuardAttestationAnnotationInfo*> OrderedMap;
  static OrderedMap annotations;

  CodeGuardAttestationAnnotationInfo() {}
  CodeGuardAttestationAnnotationInfo(t_uint32 index): AttestationAnnotationInfo(index) {}

  virtual AbstractAnnotationInfo* clone() const { return new CodeGuardAttestationAnnotationInfo(this->index); }
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content) {}
};

/* Usage:
  Region *region;
  AttestationAnnotationInfo *info;
  BBL_FOREACH_ATTESTATION_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_ATTESTATION_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
  for (auto request_it__ = CreateRegionIterator<AttestationAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<AttestationAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  AttestationAnnotationInfo *info;
  CFG_FOREACH_ATTESTATION_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_ATTESTATION_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<AttestationAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<AttestationAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  CodeGuardAnnotationInfo *info;
  BBL_FOREACH_CODEGUARD_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_CODEGUARD_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
  for (auto request_it__ = CreateRegionIterator<CodeGuardAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CodeGuardAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  CodeGuardAnnotationInfo *info;
  CFG_FOREACH_CODEGUARD_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_CODEGUARD_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<CodeGuardAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<CodeGuardAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  AttestatorAnnotationInfo *info;
  BBL_FOREACH_ATTESTATOR_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_ATTESTATOR_REGION(bbl, region, info)\
  BBL_FOREACH_REGION(bbl, region)\
  for (auto request_it__ = CreateRegionIterator<AttestatorAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<AttestatorAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

/* Usage:
  Region *region;
  AttestatorAnnotationInfo *info;
  CFG_FOREACH_ATTESTATOR_REGION(cfg, region, info)
    ...
*/
#define CFG_FOREACH_ATTESTATOR_REGION(cfg, region, info)\
  CFG_FOREACH_REGION(cfg, region)\
    for (auto request_it__ = CreateRegionIterator<AttestatorAnnotationInfo>(region->requests); (request_it__ != region->requests.end()) ? (info = static_cast<AttestatorAnnotationInfo *>(*request_it__), TRUE) : FALSE; ++request_it__)

#endif /* REMOTE_ATTESTATION_JSON_H */
