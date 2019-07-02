#ifndef DIABLOFLOWGRAPH_STL_HPP
#define DIABLOFLOWGRAPH_STL_HPP

struct bblcmp {
  bool operator() (const t_bbl* lhs, const t_bbl* rhs) const
  {return BBL_ID(lhs) < BBL_ID(rhs);}
};
typedef std::set<t_bbl *, bblcmp> BblSet;
typedef std::vector<t_bbl *> BblVector;

struct functioncmp {
  bool operator() (const t_function *lhs, const t_function *rhs) const
  {return FUNCTION_ID(lhs) < FUNCTION_ID(rhs);}
};
typedef std::set<t_function *, functioncmp> FunctionSet;
typedef std::vector<t_function *> FunctionVector;

struct edgecmp {
  bool operator() (const t_cfg_edge* lhs, const t_cfg_edge* rhs) const
  {return CFG_EDGE_ID(lhs) < CFG_EDGE_ID(rhs);}
};
typedef std::set<t_cfg_edge*, edgecmp> CfgEdgeSet;
typedef std::vector<t_cfg_edge*> CfgEdgeVector;

#endif
