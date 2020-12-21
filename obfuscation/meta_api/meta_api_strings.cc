#include "meta_api.h"

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (strings)] "

using namespace std;

struct MetaAPI_String_cmp {
    bool operator() (MetaAPI_String *a, MetaAPI_String *b) const {
      return AddressIsLt(a->uid, b->uid);
    }
};
typedef set<MetaAPI_String *, MetaAPI_String_cmp> MetaAPI_StringSet;

/* keep a list of locations in the input program per string for easy lookup */
typedef map<string, MetaAPI_StringSet> StringList;

static StringList program_strings;

void MetaAPI_ForEachProgramString(function<bool(MetaAPI_String *)> helper, bool random_iterator) {
  /* start at the first element by default... */
  auto it = program_strings.begin();
  if (random_iterator) {
    /* ... or advance the iterator to a random element in the list if requested */
    ASSERT(program_strings.size() > 0, ("need at least one program string"));
    auto n = RNGGenerateWithRange(meta_api_rng, 0, program_strings.size() - 1);
    advance(it, n);
  }

  auto first_it = it;
  do {
    /* early exit when this value is set to TRUE */
    bool x = false;

    auto i = *it;
    for (auto ii : i.second) {
      x = helper(ii);
      if (x) break;
    }

    if (x) break;

    /* increment the iterator,
     * wrapping around if at end */
    it++;
    if (it == program_strings.end())
      it = program_strings.begin();
  } while (it != first_it);
}

void AddStringToList(StringList& strings, MetaAPI_String *str) {
  string x = str->Get();
  if (strings.find(x) == strings.end())
    strings[x] = MetaAPI_StringSet();
  strings[x].insert(str);
}

string MetaAPI_String::Get() {
  return string(static_cast<t_const_string>(SECTION_DATA(section)) + G_T_UINT32(offset));
}

string MetaAPI_String::Print() {
  t_string address = StringIo("ID=%d (@@@G)", uid, Address());
  string result = string(address) + " \"" + Get() + "\"";
  Free(address);
  return result;
}

t_address MetaAPI_String::Address() {
  return AddressAdd(SECTION_OLD_ADDRESS(section), offset);
}

void MetaAPI_FindProgramStrings(t_cfg *cfg) {
  VERBOSE(0, (META_API_PREFIX "Looking for strings in the input program..."));

  t_object *obj = CFG_OBJECT(cfg);

  /* map address in input binary to string */
  map<t_address, MetaAPI_String *> address_to_string;
  t_reloc *rel;
  OBJECT_FOREACH_RELOC(obj, rel) {
    if (RELOC_N_TO_RELOCATABLES(rel) == 0)
      continue;

    /* iterate over all the TO relocatables */
    for (t_uint32 i = 0; i < RELOC_N_TO_RELOCATABLES(rel); i++) {
      /* we're looking for strings here.
       * We assume that GCC puts each string in a separate section '.rodata.str*'. */
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) != RT_SUBSECTION)
        continue;

      t_section *section = T_SECTION(RELOC_TO_RELOCATABLE(rel)[i]);
      if (StringPatternMatch(".rodata.str*", SECTION_NAME(section))) {
        MetaAPI_String *str = new MetaAPI_String();
        str->section = section;
        str->offset = RELOC_TO_RELOCATABLE_OFFSET(rel)[i];

        t_address original_address = AddressAdd(SECTION_OLD_ADDRESS(str->section), str->offset);
        if (str->Get().size() == 0
            || address_to_string.find(original_address) != address_to_string.end()) {
          /* empty string is useless */
          /* duplicates to the same address are useless */
          delete str;
        }
        else if (!isprint(str->Get()[0]))
          delete str;
        else
          address_to_string[original_address] = str;
      }
    }
  }

  for (auto i : address_to_string)
    AddStringToList(program_strings, i.second);

  VERBOSE(0, (META_API_PREFIX "  %d unique strings found (%d strings at different locations)", program_strings.size(), address_to_string.size()));
}

MetaAPI_String *MetaAPI_CreateString(t_object *obj, string value, bool append_newline) {
  //TODO: allow duplicate strings? For now: yes.
  MetaAPI_String *result = new MetaAPI_String();

  static t_uint32 uid = 0;
  string section_name = string("$MetaAPI_string") + to_string(uid);
  uid++;

  size_t length = value.size();
  if (append_newline)
    length++;

  /* create and fill a new section with the value */
  result->section = SectionCreateForObject(ObjectGetLinkerSubObject(obj),
    RODATA_SECTION, SectionGetFromObjectByName(obj, ".rodata"),
    AddressNew32(length + 1), section_name.c_str());
  memcpy(SECTION_DATA(result->section), value.c_str(), length);

  /* append newline (0x0a) on request */
  if (append_newline)
    reinterpret_cast<char*>(SECTION_DATA(result->section))[length-1] = 0x0a;

  //TODO: need to keep the string live with a from-HELL relocation

  result->offset = AddressNew32(0);

  VERBOSE(meta_api_verbosity, ("Custom string [%s]", result->Print().c_str()));
  return result;
}

static
StringList ListPossibilities(MetaAPI_String::RelationSet relations) {
  /* 'equal' means 'same value' _and_ 'same address' */

  /* consider all program strings by default */
  StringList poss = program_strings;

  bool ne_done = false;
  for (auto i : relations) {
    MetaAPI_Relation::Type relation = i.first;
    vector<MetaAPI_String *> strings = i.second;

    switch (relation) {
    case MetaAPI_Relation::Type::Eq:
      ASSERT(!ne_done, ("should first do equal"));
      poss.clear();

      for (auto x : strings)
        AddStringToList(poss, x);
      break;

    case MetaAPI_Relation::Type::Ne:
      for (auto x : strings) {
        string str = x->Get();

        auto it = poss.find(str);
        if (it != poss.end())
          poss.erase(it);
      }

      ne_done = true;
      break;

    default:
      FATAL((META_API_PREFIX "unsupported constraint relation %d", MetaAPI_Relation::Print(relation).c_str()));
    }
  }

  return poss;
}

MetaAPI_String *MetaAPI_ChooseExistingString(MetaAPI_String::RelationSet relations) {
  ASSERT(program_strings.size() > 0, ("no strings in the program"));

  StringList poss = ListPossibilities(relations);
  ASSERT(poss.size() > 0, ("no options"));

  MetaAPI_String *result = PickRandomElement(PickRandomElement(poss, meta_api_string_rng), meta_api_string_rng);

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "Random existing string [%s]", result->Print().c_str()));
  return result;
}

MetaAPI_String *MetaAPI_FindExistingString(string str, MetaAPI_String::RelationSet relations) {
  StringList poss = ListPossibilities(relations);

  auto it = poss.find(str);
  ASSERT(it != poss.end(), ("string '%s' not found in the program", str.c_str()));

  MetaAPI_String *result = PickRandomElement(it->second, meta_api_string_rng);

  VERBOSE(meta_api_verbosity, (META_API_PREFIX "Existing string '%s' [%s]", str.c_str(), result->Print().c_str()));
  return result;
}

MetaAPI_String::RelationSet MetaAPI_StringRelationSet(RelationToOtherVariables relations, function<bool(MetaAPI_Variable *)> consider, function<MetaAPI_String *(MetaAPI_Variable *)> valueof) {
  MetaAPI_String::RelationSet result;

  for (auto i : relations) {
    MetaAPI_Relation::Type relation = i.first;

    for (auto variable : i.second) {
      if (!consider(variable))
        continue;

      if (result.find(relation) == result.end())
        result[relation] = vector<MetaAPI_String *>();
      result[relation].push_back(valueof(variable));
    }
  }

  return result;
}
