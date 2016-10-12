#include <diabloalpha.h>

/* Disassemble one instruction in the code section. */

t_uint32 AlphaAssembleOne(t_alpha_ins * i_ins)
{

	t_uint32 instr;

	alpha_opcode_table[ALPHA_INS_OPCODE(i_ins)].Assemble(i_ins, &instr);
	return instr;

}

/* assemble an entire section */

void AlphaAssembleSection(t_section * sec)
{
  t_uint64 address=0;
  int nins = 0;
  t_alpha_ins * i_ins;
  char * data = SECTION_TMP_BUF(sec);

  i_ins = SECTION_DATA(sec);
  address=G_T_UINT64(SECTION_CADDRESS(sec));

	
  while(i_ins != NULL)
  {
    *((t_uint32 *)data) = AlphaAssembleOne(i_ins);
    nins++;
    data += 4;
    address+=4L;

    i_ins = ALPHA_INS_INEXT(i_ins); 
  }
}
/* vim: set shiftwidth=2: */

