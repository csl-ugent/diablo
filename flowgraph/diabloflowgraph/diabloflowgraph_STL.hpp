#ifndef DIABLOFLOWGRAPH_STL_HPP
#define DIABLOFLOWGRAPH_STL_HPP
#include "diabloflowgraph.hpp"

struct bblcmp {
  bool operator() (const t_bbl* lhs, const t_bbl* rhs) const
  {return BBL_ID(lhs) < BBL_ID(rhs);}
};
typedef std::set<t_bbl *, bblcmp> BblSet;
typedef std::vector<t_bbl *> BblVector;

#endif
