extern "C" {
#include <diabloanoptarm.h>
#include <diabloarm.h>
}

#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

extern int anoptarm_cfg_usecount;

void
DiabloAnoptArmInitCfgCpp(void * vcfg, void * data)
{
  t_cfg * cfg=static_cast<t_cfg *>(vcfg);

  if(!anoptarm_cfg_usecount)
  {
  }
}
