#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

void __DumpDots(t_cfg *cfg, string prefix, int uid)
{
#ifdef DEBUG_AF_DOT_ONLY_SET
  if (uid == DEBUG_AF_DOT_ONLY_SET)
#endif
  {
    t_string path = StringIo("%s%u", prefix.c_str(), uid);
    CfgDrawFunctionGraphs(cfg, path);
    Free(path);
  }
}
