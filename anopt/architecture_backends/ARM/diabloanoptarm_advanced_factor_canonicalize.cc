#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

static
t_uint16 InsGetFingerprint(t_ins *ins)  {
  return INS_FINGERPRINT(ins);
}

static
void CanonicalizeBblSchedule(t_bbl *bbl) {
  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins) {
    /* calculate the abstract instruction for form this instruction.
     * We'll use this information later on for rescheduling this BBL. */
    INS_SET_ABSTRACT_FORM(ins, ArmInsGetAbstractInstructionForm(ins));
    INS_SET_FINGERPRINT(ins, InstructionFingerprint::GetFingerprint(ins));
  }

  DiabloBrokerCall("CanonicalizeBbl", bbl, InsGetFingerprint);
}

void Canonicalize(t_cfg *cfg)
{
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (!BBL_CAN_TRANSFORM(bbl))
      continue;
    CanonicalizeBblSchedule(bbl);
  }
}
