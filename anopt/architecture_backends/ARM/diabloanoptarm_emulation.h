#include <diabloanoptarm.h>
#ifndef ARM_EMULATION_H
#define ARM_EMULATION_H
/*! \todo Should be in arm instruction! */
#define ARM_SHIFT_AMOUNT(reg) (reg & 0xFF)
/*! \todo Should be in arm instruction! */
#define ARM_ROTATE_AMOUNT(reg) (reg & 0x1F) /* rotate uses only last five bits */
void ArmInsEmulator(t_arm_ins *, t_procstate *, t_bool update_known_values);
t_bool ConditionsAreEqualAndKnown(t_procstate * state_a, t_procstate * state_b);
t_bool RegIsEqualAndKnown(t_reg reg, t_procstate * state_a, t_procstate * state_b);
t_lattice_level ArmInsExtractShift(t_arm_ins * ins, t_procstate * state, t_register_content * shifted_value);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
