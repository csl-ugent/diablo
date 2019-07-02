#include "diablosupport.hpp"
#include <dlfcn.h>

#include <map>

using namespace std;

/* library search path */
static string path = "";
void InitPluginSearchDirectory(string dirname) {
  path = dirname;
}

class SharedLibrary {
public:
  /* load */
  SharedLibrary(string p_path)  {
    m_path = path + "/lib/lib" + p_path + ".so";
    m_handle = dlopen(m_path.c_str(), RTLD_LAZY);
    ASSERT(m_handle, ("could not open library '%s': %s", m_path.c_str(), dlerror()));
    VERBOSE(0, ("loaded library %s", m_path.c_str()));
  }

  ~SharedLibrary() {
    if (m_handle)
      dlclose(m_handle);
  }

  /* so we don't have to include dlfcn.h in this header... */
  void *LoadSymbol(string name) {
    void *result = dlsym(m_handle, name.c_str());
    ASSERT(result, ("could not load symbol '%s' from libary '%s': %s", name.c_str(), m_path.c_str(), dlerror()));
    VERBOSE(0, ("loaded symbol %s(%s)", m_path.c_str(), name.c_str()));
    return result;
  }

private:
  string m_path;
  void *m_handle;
};

map<string, SharedLibrary *> loaded_libraries;

void *LoadSymbolFromLibrary(string library, string name) {
  if (loaded_libraries.find(library) == loaded_libraries.end())
    loaded_libraries[library] = new SharedLibrary(library);

  return loaded_libraries[library]->LoadSymbol(name);
}

void CloseAllLibraries() {
  for (auto pair : loaded_libraries)
    delete pair.second;

  loaded_libraries.clear();
}
