#include <diabloalpha.h>
/*!
 * Disassemble an instruction at a given address. Not yet implemented. 
 *
 * \param obj
 * \param start
 * \param size_ret
 *
 * \return void *
*/
/* AlphaDisassembleOneInstruction {{{ */
void * 
AlphaDisassembleOneInstruction(t_object * obj, t_address start, int * size_ret)
{

  t_alpha_ins * ins;
  t_section ** code_sections = OBJECT_CODE(obj);
  t_section * code;
  t_int32 i, nins;
  t_uint32 instruction;

  code = *code_sections;
  for(i = 0; i < OBJECT_NCODES(obj); code = *(code_sections + ++i))
  {
    nins = SECTION_NINS(code);
    ins = AlphaInsNewForSec(code);
    if(G_T_UINT64(start) <= nins * 4 && G_T_UINT64(start) >= G_T_UINT64(SECTION_CADDRESS(code)))
    {
      instruction = SectionGetData32(code,start);
      AlphaDisassembleInstruction(ins, instruction);
      return ((void *) ins);
    }
  }
  FATAL(("COULD NOT FIND INSTRUCTION IN SECTION!"));
  return NULL; /* Keep the compiler happy */
}
/* }}} */
/*!
 * Disassemble an instruction at a given offset in a section. 
 *
 * \todo parameter size_ret seems unused!
 * 
 * \param sec The section in which we want to disassemble something
 * \param offset The offset in the section
 * \param size_ret seems unused!
 *
 * \return void * a void casted t_alpha_ins pointer
*/
/* AlphaDisassembleOneInstructionForSection {{{ */
void * 
AlphaDisassembleOneInstructionForSection(t_section * sec, t_uint32 offset, int * size_ret)
{

  /* Does this function ever even get called? */

   t_uint32 instruction;
   t_alpha_ins * ins = AlphaInsNewForSec(sec);

   instruction = SectionGetData32(sec,AddressNew64(offset));
   AlphaDisassembleInstruction(ins, instruction);
   return ( (void * ) ins);
}
/* }}} */
/*!
 * Disassemble an entire section. Creates an array of t_alpha_ins's.
 *
 * \param code The section we want to disassemble
 *
 * \return void 
*/
/* AlphaDisassembleSection {{{ */
void 
AlphaDisassembleSection(t_section * code)
{
  int nins = 0;
  t_address offset;
  t_alpha_ins * ins_s = NULL;
  t_alpha_ins * prev;
  t_uint32 data;
  t_address o_offset;
  int ins_map_idx = 0;

  offset=AddressNew64(0);
  o_offset=AddressNew64(0);

  for (prev = NULL; AddressIsLt(offset, SECTION_CSIZE(code)); offset=AddressAddUint64(offset,4) , o_offset=AddressAddUint64(o_offset,4), prev=ins_s, ins_map_idx += 4)
  {

    nins++;
    ins_s = AlphaInsNewForSec(code);
    SECTION_ADDRESS_TO_INS_MAP(code)[ins_map_idx] = ( t_ins * ) ins_s;

    data = SectionGetData32(code,offset);
    if (!ins_s) FATAL(("No instructions!"));

    AlphaDisassembleInstruction(ins_s,data);


    ALPHA_INS_SET_PREV(ins_s, prev);
    ALPHA_INS_SET_REGS_USE(ins_s, AlphaInsUsedRegisters(ins_s));
    ALPHA_INS_SET_REGS_DEF(ins_s, AlphaInsDefinedRegisters(ins_s));

    if (prev) 
      ALPHA_INS_SET_NEXT(prev, ins_s);

    ALPHA_INS_SET_CADDRESS(ins_s, AddressAdd(SECTION_CADDRESS(code),offset));
    ALPHA_INS_SET_CSIZE(ins_s, AddressNew64(4));

    ALPHA_INS_SET_OLD_ADDRESS(ins_s, AddressAdd(SECTION_CADDRESS(code),offset));
    ALPHA_INS_SET_OLD_SIZE(ins_s, AddressNew64(4));

  }
  ALPHA_INS_SET_INEXT(ins_s, NULL);
  VERBOSE(0,("Disassembled %d instructions",nins));
}
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
