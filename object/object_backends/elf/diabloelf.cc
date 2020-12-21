#include "diabloelf.hpp"

#include <string>
#include <map>

using namespace std;

struct VersioningData {
  t_section *entry;
  t_section *aux;
};
static vector<VersioningData> version_data;

static map<string, DyncallFunctionInfo> name_to_data;
static map<string, ExportedFunctionInfo> exported_name_to_data;

void ElfGetSymbolDataBroker(t_string _name, void *_data, t_bool *found) {
  string name = string(_name);

  *found = false;
  if (name_to_data.find(name) == name_to_data.end())
    return;

  *found = true;
  DyncallFunctionInfo *data = static_cast<DyncallFunctionInfo *>(_data);
  *data = name_to_data[name];
}

void ElfGetExportedSymbolDataBroker(t_string _name, void *_data, t_bool *found) {
  string name = string(_name);

  *found = false;
  if (exported_name_to_data.find(name) == exported_name_to_data.end())
    return;
  
  *found = true;
  ExportedFunctionInfo *data = static_cast<ExportedFunctionInfo *>(_data);
  *data = exported_name_to_data[name];
}

void ElfParseVersioningInformation(t_object *obj) {
  int i;
  t_section *sec;

  /* look for .gnu.version_r section */
  t_section *gnu_version_d = NULL;
  t_section *gnu_version_r = NULL;
  t_section *gnu_version = NULL;
  OBJECT_FOREACH_SECTION(obj, sec, i) {
    if (!SECTION_NAME(sec))
      continue;

    if (!strcmp(SECTION_NAME(sec), ".gnu.version_r"))
      gnu_version_r = sec;
    else if (!strcmp(SECTION_NAME(sec), ".gnu.version_d"))
      gnu_version_d = sec;
    else if (!strcmp(SECTION_NAME(sec), ".gnu.version"))
      gnu_version = sec;
  }
  ASSERT(gnu_version, ("can't find .gnu_version section"));
  ASSERT(gnu_version_r, ("can't find .gnu_version_r section"));

  vector<t_section *> entries;
  vector<t_section *> auxs;

  /* we assume that the .gnu.version_d section always comes before the .gnu.version_r section */
  if (gnu_version_d) {
    SECTION_FOREACH_SUBSECTION(gnu_version_d, sec) {
      if (StringPatternMatch("*VERDEFDATA:ENTRY*", SECTION_NAME(sec)))
        entries.push_back(sec);
      else if (StringPatternMatch("*VERDEFDATA:AUX*", SECTION_NAME(sec)))
        auxs.push_back(sec);
    }
  }

  SECTION_FOREACH_SUBSECTION(gnu_version_r, sec) {
    if (StringPatternMatch("*VERNEEDDATA:ENTRY*", SECTION_NAME(sec)))
      entries.push_back(sec);
    else if (StringPatternMatch("*VERNEEDDATA:AUX*", SECTION_NAME(sec)))
      auxs.push_back(sec);
  }

  /* sanity check */
  for (size_t i = 1; i < entries.size(); i++)
    ASSERT(AddressIsLt(SECTION_OLD_ADDRESS(entries[i-1]), SECTION_OLD_ADDRESS(entries[i])), ("expected entry sections to be sorted"));
  for (size_t i = 1; i < auxs.size(); i++)
    ASSERT(AddressIsLt(SECTION_OLD_ADDRESS(auxs[i-1]), SECTION_OLD_ADDRESS(auxs[i])), ("expected aux sections to be sorted"));

  /* mark each start aux */
  set<t_section *> start_auxs;
  for (auto e : entries) {
    t_address aux_address = SECTION_OLD_ADDRESS(e);

    if (StringPatternMatch("*VERDEFDATA:ENTRY*", SECTION_NAME(e))) {
      Elf32_Verdef *data = static_cast<Elf32_Verdef *>(SECTION_DATA(e));
      aux_address = AddressAdd(aux_address, data->vd_aux);
    }
    else {
      Elf32_Verneed *data = static_cast<Elf32_Verneed *>(SECTION_DATA(e));
      aux_address = AddressAdd(aux_address, data->vn_aux);
    }

    size_t pos;
    for (pos = 0; pos < auxs.size(); pos++) {
      if (AddressIsEq(aux_address, SECTION_OLD_ADDRESS(auxs[pos])))
        break;
    }
    ASSERT(pos < auxs.size(), ("can't find aux at @G for entry @T", aux_address, e));

    start_auxs.insert(auxs[pos]);
  }

  /* construct full versioning data */
  map<t_uint32, VersioningData> version_id_to_data_index;

  auto aux_it = auxs.begin();
  for (auto e : entries) {
    VersioningData data = VersioningData();
    data.entry = e;

    do {
      data.aux = *aux_it;
      aux_it++;

      version_data.push_back(data);

      if (StringPatternMatch("*VERDEFDATA:ENTRY*", SECTION_NAME(e))) {
        /* verdef */
        Elf32_Verdaux *aux_data = static_cast<Elf32_Verdaux *>(SECTION_DATA(data.aux));
        string _section_name = string(SECTION_NAME(data.aux));
        t_uint32 id = stoi(_section_name.substr(0, _section_name.rfind(':')).substr(_section_name.rfind(':') - 1));
        version_id_to_data_index[id] = data;
        // DEBUG(("%d: @T\n @T", id, data.entry, data.aux));
      }
      else {
        /* verneed */
        Elf32_Vernaux *aux_data = static_cast<Elf32_Vernaux *>(SECTION_DATA(data.aux));
        ASSERT(version_id_to_data_index.find(aux_data->vna_other) == version_id_to_data_index.end(), ("unexpected"));
        version_id_to_data_index[aux_data->vna_other] = data;
        // DEBUG(("%d: @T\n  @T", aux_data->vna_other, data.entry, data.aux));
      }
    } while ((aux_it != auxs.end())
              && (start_auxs.find(*aux_it) == start_auxs.end()));
  }

  /* iterate over the subsections of the .gnu.version section */
  SECTION_FOREACH_SUBSECTION(gnu_version, sec) {
    ASSERT(SECTION_CSIZE(sec) == AddressNew32(2), ("unexpected section size for @T: @G", sec, SECTION_CSIZE(sec)));

    t_uint16 *data = static_cast<t_uint16 *>(SECTION_DATA(sec));
    t_uint16 version_id = data[0];

    string symbol_name = string(SECTION_NAME(sec)).substr(strlen(".gnu.version."));

    if (version_id == 0) {
      DyncallFunctionInfo symbol_data = DyncallFunctionInfo();
      symbol_data.index = SYMBOL_VERSION_NODATA;
      name_to_data[symbol_name] = symbol_data;
      continue;
    }

    if (version_id == 1) {
      DyncallFunctionInfo symbol_data = DyncallFunctionInfo();
      symbol_data.index = SYMBOL_VERSION_GLOBAL;
      name_to_data[symbol_name] = symbol_data;
      continue;
    }

    ASSERT(version_id_to_data_index.find(version_id) != version_id_to_data_index.end(), ("unexpected"));
    VersioningData ver_data = version_id_to_data_index[version_id];

    if (StringPatternMatch("*VERDEFDATA:ENTRY*", SECTION_NAME(ver_data.entry))) {
      VERBOSE(1, ("EXPORTED_SYMBOL version information\n  @T\n  entry: @T\n  aux: @T", sec, ver_data.entry, ver_data.aux));

      ExportedFunctionInfo symbol_data = ExportedFunctionInfo();
      symbol_data.index = 0;

      symbol_data.name = symbol_name;
      string _version_name = string(SECTION_NAME(ver_data.aux));
      symbol_data.version = _version_name.substr(_version_name.rfind(':') + 1);

      ASSERT(exported_name_to_data.find(symbol_name) == exported_name_to_data.end(), ("unexpected '%s'", symbol_name.c_str()));

      exported_name_to_data[symbol_name] = symbol_data;
    }
    else {
      VERBOSE(1, ("DYNAMIC_SYMBOL version information\n  @T\n  entry: @T\n  aux: @T", sec, ver_data.entry, ver_data.aux));

      DyncallFunctionInfo symbol_data = DyncallFunctionInfo();
      symbol_data.index = 0;

      symbol_data.name = symbol_name;
      string _library_name = string(SECTION_NAME(ver_data.entry));
      symbol_data.library = _library_name.substr(_library_name.rfind(':') + 1);
      string _version_name = string(SECTION_NAME(ver_data.aux));
      symbol_data.version = _version_name.substr(_version_name.rfind(':') + 1);

      ASSERT(name_to_data.find(symbol_name) == name_to_data.end(), ("unexpected '%s'", symbol_name.c_str()));

      name_to_data[symbol_name] = symbol_data;
    }
  }
}
