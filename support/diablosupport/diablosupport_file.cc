#include <diablosupport.hpp>

#include <cstring>

using namespace std;

#ifdef DIABLOSUPPORT_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

extern "C"
void mkdir_recursive(t_string p_path) {
  string path = string(p_path);
  bool absolute = (path[0] == PATH_SEPARATOR_STR[0]);

  /* split the path into its components */
  vector<string> components;
  size_t current_offset = absolute ? 1 : 0;
  size_t limit = string::npos;
  do {
    limit = path.find(PATH_SEPARATOR_STR, current_offset+1);

    components.push_back(path.substr(current_offset, limit - current_offset));

    current_offset = limit + 1;
  } while (limit != string::npos);

  /* recursively create the full path */
  string create_path = "";
  if (absolute)
    create_path += PATH_SEPARATOR_STR;

  for (auto component : components) {
    create_path += component;

    if (mkdir(create_path.c_str(), S_IRWXU) == -1) {
      int err = errno;
      ASSERT(err == EEXIST, ("could not create directory %s: %s", create_path.c_str(), strerror(err)));
    }

    create_path += PATH_SEPARATOR_STR;
  }
}
