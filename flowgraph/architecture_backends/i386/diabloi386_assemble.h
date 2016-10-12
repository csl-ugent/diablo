
#include <diabloi386.h>

#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_ASSEMBLE_FUNCTIONS
#define I386_ASSEMBLE_FUNCTIONS

void I386AssembleSection(t_section * sec);
t_uint32 I386AssembleIns(t_i386_ins * ins, t_uint8 * buf);
t_uint32 I386AssembleToSpecificForm(t_i386_ins * ins, t_i386_opcode_entry * form, t_uint8 * buf);
t_uint32 I386GetPossibleEncodings(t_i386_ins * ins, t_i386_opcode_entry * forms[]);
t_bool I386ParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_i386_ins * at_ins, t_bool before);
#ifdef OBFUSCATION_SUPPORT
void I386_ReOrganizeSections(t_object * obj);
void I386FreeEncoded();
#endif
#endif
#endif
/* vim: set shiftwidth=2: */
