
#include <diabloamd64.h>

#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_ASSEMBLE_FUNCTIONS
#define AMD64_ASSEMBLE_FUNCTIONS

void Amd64AssembleSection(t_section * sec);
t_uint32 Amd64AssembleIns(t_amd64_ins * ins, t_uint8 * buf);
t_uint32 Amd64AssembleToSpecificForm(t_amd64_ins * ins, t_amd64_opcode_entry * form, t_uint8 * buf);
t_uint32 Amd64GetPossibleEncodings(t_amd64_ins * ins, t_amd64_opcode_entry * forms[]);
t_bool Amd64ParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before);
#ifdef OBFUSCATION_SUPPORT
void Amd64_ReOrganizeSections(t_object * obj);
void Amd64FreeEncoded();
#endif
#endif
#endif
/* vim: set shiftwidth=2: */
