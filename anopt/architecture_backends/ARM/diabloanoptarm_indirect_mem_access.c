/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*#define DEBUG_INDIRECT_MEM_ACCESS*/
/*#define EVEN_MORE_UNSAFE*/
/* Includes {{{*/
#include <diabloanoptarm.h>
#include <string.h>
#include <strings.h>
/*}}}*/

void ArmReplaceMemAccessSideEffects(t_cfg * cfg)
{
  t_bbl * bbl;
  t_arm_ins * ins;

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_ARM_INS(bbl,ins)
    {
      if (ARM_INS_TYPE(ins)==IT_LOAD || ARM_INS_TYPE(ins)==IT_STORE)
	VERBOSE(0,("@I",ins));
    }
}
