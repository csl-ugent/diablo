/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "code_mobility.h"

using namespace std;

void CodeMobilityAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content)
{
  string status = getParameterContent(annotation_content, "status");
  if (!status.empty())
  {
    if (status == "mobile")
      this->transform = true;
    else if (status == "static")
      this->transform = false;
    else
      FATAL(("Wrong status for status option (code mobility), can only be 'static' or 'mobile': '%s'.", annotation_content.c_str()));
  }

  string data = getParameterContent(annotation_content, "data");
  if (!data.empty())
  {
    if (data == "mobile")
      this->transform_data = true;
    else if (data == "static")
      this->transform_data = false;
    else
      FATAL(("Wrong entry for data option (code mobility), can only be 'static' or 'mobile': '%s'.", annotation_content.c_str()));
  }

  result.push_back(this);
}

void BblIsInCodeMobilityRegionBroker(t_bbl *bbl, t_bool *result)
{
  *result = FALSE;

  Region *region;
  CodeMobilityAnnotationInfo *info;
  BBL_FOREACH_CODEMOBILITY_REGION(bbl, region, info)
  {
  	*result = TRUE;
  	break;
  }
}