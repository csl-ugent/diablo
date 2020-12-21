#ifndef META_API_STRINGS_H
#define META_API_STRINGS_H

struct _MetaAPI_String {
  t_section *section;
  t_address offset;
  size_t uid;

  _MetaAPI_String() {
    static size_t _uid = 0;
    uid = _uid++;
  }

  std::string Print();
  std::string Get();
  t_address Address();

  typedef std::map<MetaAPI_Relation::Type, std::vector<MetaAPI_String *>> RelationSet;
};

void MetaAPI_ForEachProgramString(std::function<bool(MetaAPI_String *)> helper, bool random_iterator = true);
void MetaAPI_FindProgramStrings(t_cfg *cfg);
MetaAPI_String *MetaAPI_CreateString(t_object *obj, std::string value, bool append_newline=false);
MetaAPI_String *MetaAPI_ChooseExistingString(MetaAPI_String::RelationSet relations);
MetaAPI_String *MetaAPI_FindExistingString(std::string str, MetaAPI_String::RelationSet relations);
MetaAPI_String::RelationSet MetaAPI_StringRelationSet(RelationToOtherVariables relations, std::function<bool(MetaAPI_Variable *)> consider, std::function<MetaAPI_String *(MetaAPI_Variable *)> valueof);

#endif
