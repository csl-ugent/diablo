/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "self_debugging_json.h"

using namespace std;

void SelfDebuggingAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content)
{
  string in = getParameterContent(annotation_content, "in");
  if (!in.empty())
  {
    if (in == "debugger")
      this->transform = true;
    else if (in == "application")
      this->transform = false;
    else
      FATAL(("Wrong value for 'in' parameter in anti debugging annotation, can only be 'debugger' or 'application': '%s'.", annotation_content.c_str()));
  }

  result.push_back(this);
}
