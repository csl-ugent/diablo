/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloannotations.h"

#include <cctype>
#include <sstream>
#include <jansson.h>

using namespace std;

static const string splitter = " ,()";
static const string protection_token = "protection";

size_t skip_spaces(const string& str, size_t pos) {
  if (pos == string::npos)
    return pos;

  while(pos < str.length() && str[pos] == ' ')
    pos++;
  return pos;
}

size_t eat_character_and_spaces(const string& str, char c, size_t pos) {
  pos = skip_spaces(str, pos);
  ASSERT(pos < str.length(), ("Parsed over the end of string '%s' while scanning for '%c'", str.c_str(), c));
  ASSERT(str[pos] == c, ("Expected '%c' in '%s' at %li, but got '%c'", c, str.c_str(), pos, str[pos]));
  return skip_spaces(str, pos + 1);
}

size_t next_token(const string& str, const string& splitter, string& token, size_t pos) {
  pos = skip_spaces(str, pos);
  size_t next_pos = str.find_first_of(splitter, pos);

  token = str.substr(pos, next_pos - pos);

  return skip_spaces(str, next_pos);
}

/* push-down automaton basically: keeps track of opened '('/')'-pairs, starting from '(' before pos, and return the position where the count is back to 0.
 * Returns string::npos if unbalanced */
size_t skip_to_after_next_closed_parenthesis(const string& str, size_t pos) {
  int active_parentheses = 1;

  size_t current_pos = pos;

  ASSERT(pos < str.length(), ("Initial position should be before the end of the string, but got: %i >= %i", (int) pos, (int) str.length()));

  do {
    char current_char = str[current_pos];
    if (current_char == '(')
      active_parentheses++;
    else if (current_char == ')')
      active_parentheses--;

    current_pos++;
  } while (current_pos < str.length() && active_parentheses > 0);

  if (active_parentheses == 0)
    return current_pos;

  return string::npos;
}

static
std::string *GetStringFromJSONObject(const json_t *json_obj, t_const_string field)
{
  json_t *data = json_object_get(json_obj, field);

  /* undefined field */
  if (!data)
    return NULL;

  /* convert to std::string */
  ASSERT(json_is_string(data), ("expected string for field '%s'", field));
  return new std::string(json_string_value(data));
}

void ReadAnnotationsFromJSON(t_const_string annotation_file, Annotations& annotations)
{
  json_error_t error;
  json_t *root;

  VERBOSE(0, ("Parsing JSON annotation file '%s'", annotation_file));

  /* try to read and parse the file */
  root = json_load_file(annotation_file, 0, &error);
  ASSERT(root, ("error parsing JSON: error on line %d: '%s'", error.line, error.text));

  ASSERT(json_is_array(root), ("expected array"));

  /* iterate over every annotation specified */
  for (size_t i = 0; i < json_array_size(root); i++)
  {
    json_t *json_data;
    bool file_is_object_file = false;
    std::string *file_name = NULL;
    t_uint32 line_begin = UINT32_MAX, line_end = UINT32_MAX;
    std::string *annotation_type = NULL;
    std::string *function_name = NULL;
    std::string *annotation_content = NULL;

    Annotation *new_annotation = NULL;

    json_t *annotation_entry = json_array_get(root, i);
    ASSERT(json_is_object(annotation_entry), ("expected object"));

    /* retrieve file name */
    file_name = GetStringFromJSONObject(annotation_entry, "file name");

    /* retrieve function name */
    function_name = GetStringFromJSONObject(annotation_entry, "function name");

    /* retrieve annotation content */
    annotation_content = GetStringFromJSONObject(annotation_entry, "annotation content");

    /* retrieve annotation type */
    annotation_type = GetStringFromJSONObject(annotation_entry, "annotation type");
    ASSERT(annotation_type, ("expected field 'annotation type' to be present"));
    if (annotation_type->compare("code") != 0) {
      VERBOSE(0, ("annotation type other than 'code' detected (got '%s'); ignoring this annnotation", annotation_type->c_str()));
      continue;
    }

    /* retrieve line number(s) */
    json_data = json_object_get(annotation_entry, "line number");
    if (json_data)
    {
      if (json_is_integer(json_data))
      {
        /* single line number */
        line_begin = json_integer_value(json_data);
        line_end = line_begin;
      }
      else if (json_is_array(json_data))
      {
        /* line number range */
        ASSERT(json_array_size(json_data) == 2, ("expected array given as line number to be of size 2"));

        ASSERT(json_is_integer(json_array_get(json_data, 0)), ("expected first element of line number array to be an integer"));
        line_begin = json_integer_value(json_array_get(json_data, 0));

        ASSERT(json_is_integer(json_array_get(json_data, 1)), ("expected second element of line number array to be an integer"));
        line_end = json_integer_value(json_array_get(json_data, 1));

        ASSERT(line_end > line_begin, ("invalid line number range: %u->%u (end number should be bigger or equal to begin number)", line_begin, line_end));
      }
      else
        FATAL(("expected integer or array for field 'line number'"));
    }

    /* Optional: "file is objectfile" (integer: non-zero => true) */
    json_data = json_object_get(annotation_entry, "file is objectfile");
    if (json_data && json_is_integer(json_data) && json_integer_value(json_data) != 0)
      file_is_object_file = true;

    /* sanity check: either [file name] or [function name] should be specified.
     * In case a file name is given without line range, the entire file is marked with this annotation. */
    ASSERT(file_name || function_name, ("expected at least file function name for annotation, but found neither"));

    /* create a new Annotation object and add it to the array */
    new_annotation = new Annotation();
    new_annotation->file_is_object_file = file_is_object_file;
    new_annotation->file_name = file_name;
    new_annotation->line_begin = line_begin;
    new_annotation->line_end = line_end;
    new_annotation->function_name = function_name;
    new_annotation->annotation_content = annotation_content;
    new_annotation->index = i;
    annotations.push_back(new_annotation);

    delete annotation_type;
  }

  json_decref(root);

  VERBOSE(0, ("   found %d annotations in '%s'", annotations.size(), annotation_file));
}

void AnnotationsDestroy(Annotations& annotations)
{
  for (auto annotation : annotations)
  {
    if (annotation->file_name)
      delete annotation->file_name;

    if (annotation->function_name)
      delete annotation->function_name;

    delete annotation->annotation_content;

    delete annotation;
  }
}

struct AnnotationLogInfo {
  AnnotationRequests requests;
  string text;
};

/* annotationLog[i]    -> corresponds to the single i'th JSON annotation line
 * annotationLog[i][j] -> corresponds to the j'th recognized protection()-part of the i'th JSON annotation lone. Thus, it can be empty if Diablo does
 *                        not recognize any annotations */
static vector< vector<AnnotationLogInfo> > annotationLog;

AnnotationRequests ParseAnnotationContent(const Annotation *annotation)
{
  AnnotationRequests result;
  const string& annotation_content = *(annotation->annotation_content);

  /* binary obfuscation annotations look like this: protection (obfuscations, enable_obfuscation(opaque_predicates:percent_apply=25,flattening) */
  size_t pos = 0;
  string maybe_protection_token;
  string protection_type_token;

  vector<AnnotationLogInfo> currentLog;

  size_t first_request_to_add = 0;

  do {
    AnnotationLogInfo logInfo;
    bool known = false;
    size_t annotation_start_pos = pos; /* starts (hopefully) before 'protection(' */

    /* Get the possible protection_token */
    pos = next_token(annotation_content, splitter, maybe_protection_token, pos);
    VERBOSE(1, ("Token '%s' found in '%s'", maybe_protection_token.c_str(), annotation_content.c_str()));
    pos = eat_character_and_spaces(annotation_content, '(', pos);

    /* Get the protection_type_token and the positions of its parameters */
    pos = next_token(annotation_content, splitter, protection_type_token, pos);
    size_t protection_start_pos = pos; /* starts after the initial '(' */
    size_t protection_end_pos = skip_to_after_next_closed_parenthesis(annotation_content, protection_start_pos);
    ASSERT(protection_end_pos != string::npos, ("Missing matching parenthesis in annotation %s", annotation_content.c_str()));
    size_t protection_len = protection_end_pos - protection_start_pos;

    if (maybe_protection_token == protection_token)
    {
      AbstractAnnotationInfoFactory *factory = GetAnnotationInfoFactoryForToken(protection_type_token);
      if (factory)
      {
        VERBOSE(1, ("Protection annotation '%s' (from '%s') RECOGNIZED in Diablo, OK", protection_type_token.c_str(), annotation_content.c_str()));
        AbstractAnnotationInfo *new_info = factory->create();
        new_info->parseAnnotationContent(result, annotation_content.substr(protection_start_pos, protection_len));
        known = true;
      } else {
        VERBOSE(0, ("Protection annotation '%s' (from '%s') not recognized in Diablo, skipping", protection_type_token.c_str(), annotation_content.c_str()));
      }
    }
    else
    {
      VERBOSE(0, ("Not a protection annotation '%s' (from '%s'), skipping", maybe_protection_token.c_str(), annotation_content.c_str()));
    }

    /* Move pos along and do logging */
    logInfo.text = annotation_content.substr(annotation_start_pos, protection_len);
    pos = skip_spaces(annotation_content, protection_end_pos);

    if (pos != string::npos && pos < annotation_content.length()) {
      ASSERT(annotation_content[pos] == ',', ("Expected a ',' at position %i, but got '%c'", (int)pos, annotation_content[pos]));
      pos++;
    }

    if (known) {
      for (size_t i = first_request_to_add; i < result.size(); i++) {
        logInfo.requests.push_back(result.at(i));
      }
      first_request_to_add = result.size();
      currentLog.push_back(logInfo);
    }
  } while (pos < annotation_content.length() && pos != string::npos);

  annotationLog.push_back(currentLog);

  return result;
}

void DumpConsumedAnnotations(t_const_string consumed_annotations_file) {
  FILE* f = fopen(consumed_annotations_file, "w");
  fprintf(f, "[\n");

  bool add_comma_outer = false;

  for (auto annotation_json: annotationLog) {
    if (add_comma_outer) {
      fprintf(f, ",{\n");
    } else {
      fprintf(f, "{\n");
      add_comma_outer = true;
    }

    fprintf(f, "\"tool\":\"diablo\",\n");
    fprintf(f, "\"consumed annotation\": \"");

    /* The annotations in this list are by definition the ones Diablo consumed */
    bool add_comma_inner = false;

    for (auto sub_annotation: annotation_json) {
      if (add_comma_inner) { fprintf(f, ","); }
      add_comma_inner = true;

      fprintf(f, "%s", sub_annotation.text.c_str());
    }

    /* Print whether or not they were actually applied at least once: */
    fprintf(f, "\"\n");
    fprintf(f, "\"additional decisions\": \"could_actually_apply_anywhere=");

    add_comma_inner = false;
    for (auto sub_annotation: annotation_json) {
      bool any_applied = false;
      for (auto request: sub_annotation.requests) {
        if (request->successfully_applied)
          any_applied = true;
      }

      if (add_comma_inner) { fprintf(f, ","); }
      add_comma_inner = true;

      fprintf(f, "%i", any_applied ? 1 : 0);
    }

    fprintf(f, "\"\n");

    fprintf(f, "}\n");
  }

  fprintf(f, "]\n");
  fclose(f);
}

void AnnotationRequestsFree(AnnotationRequests *script)
{
#if 0
  for (auto it = script->begin(); it != script->end(); it++)
  {
    AnnotationRequestHighLevel annot = it->second;
    for (auto itt = std::begin(annot.request); itt != std::end(annot.request); itt++)
      delete *itt;
  }
#endif
}

void JanssonFreeObject(json_t *json)
{
  if (json_is_object(json))
  {
    const char *key;
    json_t *value;

    json_object_foreach(json, key, value)
      JanssonFreeObject(value);
  }
  else if (json_is_array(json))
  {
    size_t index;
    json_t *value;

    json_array_foreach(json, index, value)
      JanssonFreeObject(value);
  }

  json_decref(json);
}

t_bool AnnotationContainsToken(const Annotations& annotations, const string token_p)
{
  for(auto* annotation: annotations)
  {
    const string& annotation_content = *(annotation->annotation_content);
    size_t pos = 0;

    /* Parse every protection in the annotation */
    do
    {
      string token;
      pos = next_token(annotation_content, splitter, token, pos);

      if (token == protection_token)
      {
        pos = eat_character_and_spaces(annotation_content, '(', pos);
        pos = next_token(annotation_content, splitter, token, pos);

        /* Check if the protection type (token) is the right one */
        if (token == token_p)
          return TRUE;

        /* Move along to the next protection in this annotation */
        pos = skip_to_after_next_closed_parenthesis(annotation_content, pos);
        ASSERT(pos != string::npos, ("Missing matching parenthesis in annotation %s", annotation_content.c_str()));
        pos = skip_spaces(annotation_content, pos);
        pos++;/* Get the ',' */
      }
      else
        return FALSE;
    }
    while (pos < annotation_content.length() && pos != string::npos);
  }

  return FALSE;
}

AnnotationIntOptions ParseIntOptions(stringstream& ss)
{
  AnnotationIntOptions current_options;
  bool has_next;
  char c;

  do {
    string parameter;
    int value;

    c = ss.get();
    while(isalnum(c) || c == '_') {
      parameter.push_back(c);
      c = ss.get();
    }

    ss >> value;

    VERBOSE(1, ("  Option pair: '%s' = %i", parameter.c_str(), value));

    current_options.insert(make_pair(parameter, value));

    if (ss.eof())
      break;

    has_next = ss.peek() == ':';

    ss.get(); /* skip ':' and ',' */
  } while(has_next);

  return current_options;
}

void ParseOptions(AnnotationRequests& result, const string& options, string token)
{
  AbstractAnnotationInfoFactory *factory = GetAnnotationInfoFactoryForToken(token);
  stringstream ss(options);

  while(!ss.eof()) {
    string current_name;
    AnnotationIntOptions current_options;

    /* gets obfuscation names */
    char c;
    c = ss.get();
    while (c != ',' && c != ':' && !ss.eof()) {
      /* Part of the obfuscation name */
      current_name.push_back(c);
      c = ss.get();
    }

    if (!ss.eof() && c == ':') {
      /* Now comes the parameter list (optional) */
      current_options = ParseIntOptions(ss);
    }

    AbstractAnnotationInfo *info = factory->create();
    info->name = current_name;
    info->options = current_options;
    result.push_back(info);
  }
}

string getParameterContent(const string& annotation_content, const string& param_name)
{
  /* Start searching the parameter */
  size_t pos  = annotation_content.find(param_name);
  while (pos != string::npos)
  {
    /* The parameter name can only be preceded by a comma or a space (or be at the beginning) */
    if ((pos == 0) || (pos - 1 == annotation_content.find_last_of(" ,", pos)))
    {
      size_t start = annotation_content.find_first_not_of(" (", pos + param_name.length());

      /* The parameter name should be followed by a space or a '(' */
      if ((start != string::npos) && (start != pos + param_name.length()))
      {
        size_t end = annotation_content.find(')', start);
        end = annotation_content.find_last_not_of(' ', end - 1);
        ASSERT(start != end, ("The %s parameter is empty in annotation: %s", param_name.c_str(), annotation_content.c_str()));
        ASSERT(end != string::npos, ("No matching ')' found for the %s parameter in annotation %s!", param_name.c_str(), annotation_content.c_str()));

        return annotation_content.substr(start, end - start + 1);
      }
    }

    pos  = annotation_content.find(param_name, pos + param_name.length());
  }

  /* If we don't find the parameter, return the empty string */
  return "";
}

vector<string> splitListOfIDs(const string& list)
{
  vector<string> tokens;
  size_t start = 0;

  while(true)
  {
    size_t end = list.find_first_of(" ,", start);
    if (end == string::npos)
    {
      ASSERT(start != list.length() - 1, ("Uncorrectly terminated list of IDs: %s", list.c_str()));
      tokens.push_back(list.substr(start, list.length() - start));
      break;
    }

    tokens.push_back(list.substr(start, end - start));
    start = list.find_first_not_of(" ,", end);
    ASSERT(list.find_first_of(',', end) < start, ("Missing comma separator between IDs in list: *s", list.c_str()));
  }

  return tokens;
}
