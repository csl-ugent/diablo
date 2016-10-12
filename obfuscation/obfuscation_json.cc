/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "obfuscation_json.h"

#include <sstream>
#include <string>

using namespace std;

void ObfuscationAnnotationInfo::parseOptions(AnnotationRequests& result, const string& options, bool enable)
{
  stringstream ss(options);

  VERBOSE(0, ("Parsing obfuscation options '%s'", options.c_str()));

  AnnotationRequests tmp_requests;
  ParseOptions(tmp_requests, options, obfuscations_token);

  /* obfuscation specific */
  for (auto i : tmp_requests)
  {
    ObfuscationAnnotationInfo *ii = dynamic_cast<ObfuscationAnnotationInfo *>(i);
    ii->enable = enable;
    result.push_back(ii);
  }
}

void ObfuscationAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content)
{
  /* Now follows a list of enable_obfuscation(...)/disable_obfuscation(...). The '...' is parsed by ParseObfuscationOptions */
  size_t pos = 0;

  do {
    size_t enable_obfuscation  = annotation_content.find("enable_obfuscation", pos);
    size_t disable_obfuscation = annotation_content.find("disable_obfuscation", pos);
    size_t next = 0;
    size_t len = 0;
    bool   enabled = false;

    if (enable_obfuscation < disable_obfuscation) {
      next = enable_obfuscation;
      enabled = true;
      len = strlen("enable_obfuscation");
    } else {
      next = disable_obfuscation;
      enabled = false;
      len = strlen("disable_obfuscation");
    }

    if (next == string::npos) {
      VERBOSE(0, ("No more enable_obfuscation/disable_obfuscation found!"));
      break;
    }

    pos = eat_character_and_spaces(annotation_content, '(', next + len);

    size_t end = annotation_content.find(')', pos);
    ASSERT(end != string::npos, ("No matching ')' found in %s", annotation_content.c_str()));

    /* The append here appends to any previous obfuscations for this functions. Later, when we go over line numbers, we need to verify order in which they are added
     and applied later. */
    parseOptions(result, annotation_content.substr(pos, end - pos), enabled);

    pos = skip_spaces(annotation_content, end);
  } while (true); /* Breaks automatically when no more enable_obfuscation/disable_obfuscation is found */

  /* the abstract annotation processing code assumes this function adds the owning class instance to the results list,
   * so it can be freed later on. However, in this case the current class instance is not added to the results list.
   * Other instances are added, though (see parseOptions), thus the current class instance should be freed now. */
  delete this;
}
