#include <diabloanopti386.h>
#ifndef I386_EMULATION_H
#define I386_EMULATION_H

#define I386_LOW_8(value)     (value & 0x000000FF)
#define I386_HIGH_8(value)    (value & 0x0000FF00)
#define I386_FULL_16(value)   (value & 0x0000FFFF)

#define I386_BITS_24(value)   (value & 0xFFFFFF00)
#define I386_BITS_16_8(value) (value & 0xFFFF00FF)
#define I386_BITS_16(value)   (value & 0xFFFF0000) 

//void I386InstructionEmulator(t_i386_ins *, t_procstate *, t_bool);
void I386InstructionEmulator(t_i386_ins * ins, t_procstate * state, t_bool update_known_values);
t_tristate I386ConditionHolds(t_i386_condition_code cond, t_procstate * state);
void GetConstValueForOp(
    t_i386_ins *ins, t_i386_operand *op, t_procstate *state, t_bool sign_extend,
    t_lattice_level *vlevel_r, t_uint32 *val_r,
    t_lattice_level *rlevel_r, t_reloc **rel_r);
void I386InitArgumentForwarding(t_cfg *cfg);
#endif
/* vim: set shiftwidth=2: */
