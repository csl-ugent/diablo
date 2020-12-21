#include "meta_api.h"

using namespace std;

bool force_dots = false;

void DumpDots(t_cfg *cfg, string prefix, int uid)
{
  t_string path = StringIo("%s%u", prefix.c_str(), uid);
  CfgDrawFunctionGraphs(cfg, path);
  Free(path);
}

void DumpDotsF(FunctionSet functions_to_draw, string prefix, int uid)
{
  t_string path = StringIo("%s%u", prefix.c_str(), uid);
  for (auto function : functions_to_draw)
    DrawFunctionGraphAnnotated(function, path);
  Free(path);
}
