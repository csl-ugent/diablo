/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "attestation.h"

using namespace std;

CodeGuardAttestationAnnotationInfo::OrderedMap CodeGuardAttestationAnnotationInfo::annotations;

void RemoteAttestationAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content)
{
  /* We only care about static remote attestation, these annotations contain 'static_ra_region' */
  if (annotation_content.find("static_ra_region", 0) != string::npos)
  {
    string attestator = getParameterContent(annotation_content, "attestator");
    ASSERT(!attestator.empty(), ("A static remote attestation annotation must have an attestator!"));

    /* If specified in the annotation, this region should be attestated at startup */
    string startup = getParameterContent(annotation_content, "attest_at_startup");
    if (!startup.empty() && startup == "true")
      this->at_startup = true;

    /* Add the attestator to the request */
    VERBOSE(0, ("Found static remote attestation with attestator: '%s'.", attestator.c_str()));
    this->attestators.insert(Attestator::Create(attestator));

    result.push_back(this);
  }
  /* If we can't use this request, just delete it (we can freely do this as it is THIS function that has the responsibility
   * to add the request to the results list.
   */
  else
    delete this;
}

void AttestatorAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content)
{
  string label = getParameterContent(annotation_content, "label");
  ASSERT(!label.empty(), ("A guard attestator must have a label!"));
  VERBOSE(0, ("Found code guard with label: '%s'.", label.c_str()));

  /* Get the list of regions and split them into tokens */
  string regions = getParameterContent(annotation_content, "regions");
  ASSERT(!regions.empty(), ("A guard attestator must have regions!"));
  vector<string> region_tokens = splitListOfIDs(regions);

  /* Create the Attestator and insert it */
  this->attestator = Attestator::Create(label, region_tokens, &this->area_id);

  result.push_back(this);
}

void CodeGuardAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content)
{
  string label = getParameterContent(annotation_content, "label");
  ASSERT(!label.empty(), ("A code guard annotation must have a label!"));

  /* Add the label to the request */
  VERBOSE(0, ("Found code guard with label: '%s'.", label.c_str()));
  this->label = label;

  result.push_back(this);
}
