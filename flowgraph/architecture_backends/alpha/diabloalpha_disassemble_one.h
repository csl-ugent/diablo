#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS
#ifndef ALPHA_DISASSEMBLE_ONE_H
#define ALPHA_DISASSEMBLE_ONE_H

t_uint32 AlphaFindDisassemblyFunction(t_uint32 instr);
void AlphaDisassembleInstruction(t_alpha_ins * ins, t_uint32 instr);
void AlphaDisassembleBranch(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassembleOpr(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassembleMem(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassembleMfc(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassemblePal(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassembleFp(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);
void AlphaDisassembleMemBranch(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode);

#endif
#endif
