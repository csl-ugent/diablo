#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

set<t_function *> CfgDontTouchTheseFunctions(t_cfg *cfg)
{
  set<t_function *> result;

  /* construct the call graph */
  CgBuild(cfg);

  /* marked = possible seed
   * marked2 = already grouped */
  CfgUnmarkAllFun(cfg);

  t_function *_start_fun = nullptr;
  t_function *fun;
  CG_FOREACH_FUN(CFG_CG(cfg), fun)
  {
    if (!FUNCTION_NAME(fun)) continue;
    if (StringPatternMatch("_start", FUNCTION_NAME(fun)))
    {
      _start_fun = fun;
      break;
    }
  }

  if (_start_fun == nullptr)
  {
    VERBOSE(AF_VERBOSITY_LEVEL, (AF "no _start function found!"));
    return result;
  }

  /* mark all functions from _start */
  vector<t_function *> worklist = {_start_fun};
  while (true)
  {
    /* get the last unmarked function in the work list */
    t_function *subject = nullptr;
    while (subject == nullptr || FunctionIsMarked(subject)) {
      if (worklist.size() == 0) {
        subject = nullptr;
        break;
      }

      subject = worklist.back();
      worklist.pop_back();
    }
    if (subject == nullptr)
      break;

    FunctionMark(subject);
    result.insert(subject);

    t_cg_edge *e;
    t_function *callee;
    FUNCTION_FOREACH_CALLEE(subject, e, callee)
    {
      if (FUNCTION_IS_HELL(callee))
        continue;

      worklist.push_back(callee);
    }
  }

  return result;
}
