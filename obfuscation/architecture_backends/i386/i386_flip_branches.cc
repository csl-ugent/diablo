/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "i386_obfuscations.h"
#include "i386_flip_branches.h"
using namespace std;

I386FlipBranchesTransformation::I386FlipBranchesTransformation() {
  bblsTransformed = 0;
}


bool I386FlipBranchesTransformation::canTransform(const t_bbl* bbl) const {
  t_i386_ins* ins = T_I386_INS(BBL_INS_LAST(bbl));
  
  if (!FlipBranchesTransformation::canTransform(bbl))
    return false;

  if(!ins)
    return false;
  
  if(I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))!=I386_Jcc)
    return false;

  return true;
}

bool I386FlipBranchesTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  I386InvertBranchBbl(bbl);
  bblsTransformed++;
  return true;
}

void I386FlipBranchesTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sBranchFlip_Stats,bbls_transformed,%i", prefix.c_str(), bblsTransformed));
}
