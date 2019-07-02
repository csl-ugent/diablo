/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOJSON_H
#define DIABLOJSON_H

extern "C" {
#include <diablosupport.h>
}

#include <sstream>
#include <string>
#include <vector>
#include <map>

struct json_t;

struct Annotation;
struct Region;

typedef std::vector<Annotation *> Annotations;

struct AbstractAnnotationInfo;
typedef std::vector<AbstractAnnotationInfo *> AnnotationRequests;

typedef std::map<std::string, int> AnnotationIntOptions;

/* one entry in the annotation file */
struct Annotation {
  bool file_is_object_file;
  std::string *file_name;
  t_uint32 line_begin, line_end;

  std::string *function_name;
  std::string *annotation_content;

  /* index in the JSON file, multiple AbstractAnnotationInfos can share the same index if they originate from a single JSON line, this allows us to keep track of this. */
  t_uint32 index;

  std::string Print() const;
};

struct AbstractAnnotationInfo {
  AbstractAnnotationInfo() : successfully_applied(false) {}
  virtual ~AbstractAnnotationInfo() {}
  virtual AbstractAnnotationInfo* clone() const = 0;
  /* TODO this is rather dangerous: functions that override should not forget to call result.push_back if needed! */
  virtual void parseAnnotationContent(AnnotationRequests& result, const std::string& annotation_content) { result.push_back(this); }

  /* Some annotations can (for example) *expand* their Region. This is something you really want to do early on, for example if you
   * want to expand a region to its callees (up to a limited depth), you want this before call/branch functions.
   * This function is called on a Region immediately after it and its request have been initialized
   */
  virtual void preprocessRegion(Region* region) {}

  /* TODO: maybe support multiple values for same option here (e.g., option is specified multiple times, but with different values) */
  bool GetValueForIntOption(std::string name, int& value) {
    for (auto option : options)
      if (option.first == name)
      {
        value = option.second;
        return true;
      }

    return false;
  }

  std::string name;
  AnnotationIntOptions options;
  /*
   * Each ,-part of a (JSON) annotation has a globally unique ID. Annotations can be split into multiple AbstractAnnotationInfos (for example, multiple obfuscations).
   * We set the successfully_applied bool on each AbstractAnnotationInfo if it was applied at least *somewhere*. Then later on, for each original annotation, we check
   * if *any* of the AbstractAnnotationInfos of a JSON annotation have been set to successfully_applied.
   */
  bool successfully_applied;

  std::string Print() const;
};

void ReadAnnotationsFromJSON(t_const_string annotation_file, Annotations& annotations);
void AnnotationsDestroy(Annotations& annotations);
AnnotationRequests ParseAnnotationContent(const Annotation *annotation);
void JanssonFreeObject(json_t *json);
AnnotationIntOptions ParseIntOptions(std::stringstream& ss);
void ParseOptions(AnnotationRequests& result, const std::string& options, std::string token);
size_t eat_character_and_spaces(const std::string& str, char c, size_t pos);
size_t skip_spaces(const std::string& str, size_t pos);
t_bool AnnotationContainsToken(const Annotations& annotations, const std::string token_p);
void DumpConsumedAnnotations(t_const_string consumed_annotations_file);
std::string getParameterContent(const std::string& annotation_content, const std::string& param_name);
std::vector<std::string> splitListOfIDs(const std::string& list);

#endif
