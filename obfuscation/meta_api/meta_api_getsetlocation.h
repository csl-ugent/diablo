#ifndef META_API_GETSETLOCATION_H
#define META_API_GETSETLOCATION_H

struct BblLocation {
  /* the BBL to investigate */
  t_bbl *bbl;
  /* 'TRUE'=split block at the back */
  bool split_after;
  MetaAPI_Function *setter;
};

bool FindSetterLocations(t_bbl *current_bbl, MetaAPI_ActivePredicate *predicate, bool& because_of_predicate, std::vector<BblLocation>& setter_locations);
void BblInsertInCloud(t_bbl *bbl);
bool BblIsInCloud(t_bbl *bbl);
void BblRemoveFromCloud(t_bbl *bbl);
std::set<MetaAPI_ActivePredicate *> LivePredicatesInCloud();

#endif /* META_API_GETSETLOCATION_H */
