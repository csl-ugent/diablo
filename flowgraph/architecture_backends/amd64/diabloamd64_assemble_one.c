#include <diabloamd64.h>

static t_uint32 normalregs[] = {0,3,1,2,6,7,5,4,8,9,10,11,12,13,14,15};
//static t_uint32 normalregs[] = {0,3,1,2,6,7,5,4,8,11,9,10,14,15,13,12};
static t_uint32 hibyteregs[] = {4,7,5,6};

void Amd64OpAsNone(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* do nothing */
}

void Amd64OpAsReg(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* this is called for implicit register operands. do nothing */
  /*set rexw*/
  if(AMD64_OP_REGMODE(op) == amd64_regmode_full64){
    if(  bm== amd64_bm_rAX||bm== amd64_bm_rBX||bm== amd64_bm_rCX||bm== amd64_bm_rDX||bm== amd64_bm_rSI||bm== amd64_bm_rDI||bm== amd64_bm_rSP||bm== amd64_bm_rBP
       ||bm== amd64_bm_r8 ||bm== amd64_bm_r9 ||bm== amd64_bm_r10||bm== amd64_bm_r11||bm== amd64_bm_r12||bm== amd64_bm_r13||bm== amd64_bm_r14||bm== amd64_bm_r15
       ||bm== amd64_bm_rAXrex||bm== amd64_bm_rBXrex||bm== amd64_bm_rCXrex||bm== amd64_bm_rDXrex
       ||bm== amd64_bm_rSIrex||bm== amd64_bm_rDIrex||bm== amd64_bm_rSPrex||bm== amd64_bm_rBPrex
       ){
      *rex=*rex|0x08;    
    }
  }
}

void Amd64OpAsST(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* fpu registers are implicit (or filled in during WriteOpcode) */
}

void Amd64OpAsConst1(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* this is called for an implicit constant operand. do nothing */
}

void Amd64OpAsA(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  ASSERT(bm == amd64_bm_p, ("Unknown bytemode"));
  
  *has_segsel_ret = TRUE;
  *segsel_ret = AMD64_OP_SEGSELECTOR(op);
  *immediate_ret = AMD64_OP_IMMEDIATE(op);
  *immedsz_ret = AMD64_OP_IMMEDSIZE(op);
}

void Amd64OpAsC(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* control register: always in the reg field */
  *reg_ret = AMD64_OP_BASE(op) - AMD64_REG_CR0;
}

void Amd64OpAsD(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* debug register: always in the reg field */
  *reg_ret = AMD64_OP_BASE(op) - AMD64_REG_DR0;
}

void Amd64OpAsE(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  int cor = 0;
  if (AMD64_OP_TYPE(op) == amd64_optype_reg)
  {
    *mod_ret = 3;
    if (AMD64_OP_REGMODE(op) != amd64_regmode_hi8)
      *rm_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
    else
      *rm_ret = hibyteregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
    if ((bm == amd64_bm_v || bm == amd64_bm_v48) &&  (AMD64_OP_REGMODE(op) == amd64_regmode_full64)){
      *rex=*rex|0x08;
    }
  }
  else if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    t_bool has_sib = FALSE;

   if ((bm == amd64_bm_v || bm == amd64_bm_v48) &&  (AMD64_OP_MEMOPSIZE(op) == 8)){
     *rex=*rex|0x08;
    }
    
    if ((AMD64_OP_INDEX(op) != AMD64_REG_NONE) || (AMD64_OP_BASE(op) == AMD64_REG_RSP) || (AMD64_OP_BASE(op) == AMD64_REG_R12))
    {
      has_sib = TRUE;
    }
    
    if (has_sib)
    {
      *rm_ret = 4;

      ASSERT(AMD64_REG_RSP != AMD64_OP_INDEX(op), ("RSP is not suitable as an index register"));
      if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	*index_ret = normalregs[AMD64_OP_INDEX(op)-AMD64_REG_RAX];
      else
	*index_ret = 4;
      
      *scale_ret = AMD64_OP_SCALE(op);

      if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
      {
	*base_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
	switch (AMD64_OP_IMMEDSIZE(op))
	{
	  case 0:
	    *mod_ret = 0;
	    break;
	  case 1:
	    *mod_ret = 1;
	    break;
	  case 4:
	    *mod_ret = 2;
	    break;
	  default:
	    FATAL(("Wrong displacement size (%d bytes)",AMD64_OP_IMMEDSIZE(op)));
	}
      }
      else
      {
	/* special case: [scaled index] + disp32 */
	*mod_ret = 0;
	*base_ret = 5;
      }
    }
    else
    {
      /* no index register and %esp is not the base register => no sib byte needed */
      
      /* first catch a special case */
      if ((AMD64_OP_BASE(op) == AMD64_REG_NONE) || (AMD64_OP_BASE(op) == AMD64_REG_RIP))
      {
	ASSERT(AMD64_OP_IMMEDSIZE(op) == 4, ("Wrong immediate size for displacement-only memory operand (%d)",AMD64_OP_IMMEDSIZE(op)));
	if(AMD64_OP_BASE(op) == AMD64_REG_NONE){
	  cor = AddressExtractInt32(AMD64_INS_CADDRESS(ins)) + AddressExtractInt32(AMD64_INS_CSIZE(ins));
/*	  printf("oldaddress: %d  0x%x\n",INS_OLD_ADDRESS(ins),INS_OLD_ADDRESS(ins));
	  printf("address: %d  0x%x\n",INS_CADDRESS(ins),INS_CADDRESS(ins));
	  printf("inslen: %d  0x%x\n",INS_CSIZE(ins),INS_CSIZE(ins));
	  printf("immediatebefore: %d 0x%x\n",AMD64_OP_IMMEDIATE(op),AMD64_OP_IMMEDIATE(op));
	  printf("immediateafter: %d 0x%x\n",AMD64_OP_IMMEDIATE(op)-cor,AMD64_OP_IMMEDIATE(op)-cor);*/
	}
	*mod_ret = 0;
	*rm_ret = 5;
      }
      else
      {
	/* the regular case */
	*rm_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
	switch (AMD64_OP_IMMEDSIZE(op))
	{
	  case 0:
	    *mod_ret = 0;
	    break;
	  case 1:
	    *mod_ret = 1;
	    break;
	  case 4:
	    *mod_ret = 2;
	    break;
	  default:
	    FATAL(("Wrong displacement size (%d bytes)",AMD64_OP_IMMEDSIZE(op)));
	}
      }
    }

    /* do the displacement */
    *displacement_ret = AMD64_OP_IMMEDIATE(op) - cor;
    *dispsz_ret = AMD64_OP_IMMEDSIZE(op);

    *has_sib_ret = has_sib;
  }
  else
    FATAL(("Need operand of type reg or mem"));
}


void Amd64OpAsF(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* this selects the eflags register, which is implicit in the opcode. do
   * nothing */
}

void Amd64OpAsG(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_reg, ("G mode but operand not a register"));
  ASSERT(Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op)),("Expected a general purpose register"));
  if (AMD64_OP_REGMODE(op) == amd64_regmode_hi8)
    *reg_ret = hibyteregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
  else
    *reg_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
    
  if ((bm == amd64_bm_v || bm == amd64_bm_v48) &&  (AMD64_OP_REGMODE(op) == amd64_regmode_full64)){
    *rex=*rex|0x08;
  }
}

void Amd64OpAssI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  int immval;
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_imm, ("wrong operand type"));
  ASSERT(bm == amd64_bm_b, ("Wrong bytemode for this operand type"));
  immval = (int) AMD64_OP_IMMEDIATE(op);
  ASSERT(-128 <= immval && immval <= 127, ("Value too big for one byte (%d)",immval));
  *immediate_ret = AMD64_OP_IMMEDIATE(op);
  *immedsz_ret = 1;
}

void Amd64OpAsI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  t_uint64 immval = AMD64_OP_IMMEDIATE(op);
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_imm, ("wrong operand type"));
  *immediate_ret = immval;
  switch (bm)
  {
    case amd64_bm_b:
      ASSERT(immval < 256, ("Value too big for one byte (%d)",immval));
      *immedsz_ret = 1;
      return;
    case amd64_bm_w:
      ASSERT(immval < 65536, ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 2;
      return;
    case amd64_bm_d:
      *immedsz_ret = 4;
      return;
    case amd64_bm_q:
      *immedsz_ret = 8;
      return;
    case amd64_bm_z:
      if (AMD64_OP_IMMEDSIZE(op) == 2)
      {
	ASSERT(immval < 65536, ("Value too big for two bytes (%d)",immval));
	*immedsz_ret = 2;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 4)
      {
	ASSERT(immval < 4294967296ULL, ("Value too big for four bytes (%d)",immval));
	*immedsz_ret = 4;
      }
      else
	FATAL(("Wrong immediate size"));
      return;
    case amd64_bm_v:
      if (AMD64_OP_IMMEDSIZE(op) == 2)
      {
	ASSERT(immval < 65536, ("Value too big for two bytes (%d)",immval));
	*immedsz_ret = 2;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 4)
      {
	ASSERT(immval < 4294967296ULL, ("Value too big for four bytes (%d)",immval));
	*immedsz_ret = 4;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 8)
      {
	*rex=*rex|0x08;
    	*immedsz_ret = 8;
      }     
      else{
	FATAL(("Wrong immediate size"));
      }
      if(rexw(*rex)){
	*immedsz_ret = 8;
      }
      return;
    default:
      FATAL(("unexpected bytemode"));
  }
}

void Amd64OpAsJ(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  t_int64 immval = AMD64_OP_IMMEDIATE(op);
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_imm, ("wrong operand type"));
  *immediate_ret = immval;
  switch (bm)
  {
    case amd64_bm_b:
      ASSERT((immval <= 127 && immval >= -128), ("Value too big for one byte (%d)",immval));
      *immedsz_ret = 1;
      return;
    case amd64_bm_w:
      ASSERT((immval <= 32767 && immval >= -32768), ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 2;
      return;
    case amd64_bm_d:
      ASSERT((immval <= 2147483647LL && immval >= -2147483648LL), ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 4;
      return;
    case amd64_bm_q:
      *immedsz_ret = 8;
      return;
    case amd64_bm_z:
      if (AMD64_OP_IMMEDSIZE(op) == 2)
      {
	ASSERT((immval <= 32767 && immval >= -32768), ("Value too big for two bytes (%d)",immval));
	*immedsz_ret = 2;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 4)
      {
	ASSERT((immval <= 2147483647LL && immval >= -2147483648LL), ("Value too big for four bytes (%d)",immval));
	*immedsz_ret = 4;
      }
      else
	FATAL(("Wrong immediate size"));
      return;
    case amd64_bm_v:
      if (AMD64_OP_IMMEDSIZE(op) == 2)
      {
	ASSERT((immval <= 32767 && immval >= -32768), ("Value too big for two bytes (%d)",immval));
	*immedsz_ret = 2;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 4)
      {
	ASSERT((immval <= 2147483647LL && immval >= -2147483648LL), ("Value too big for four bytes (%d)",immval));
	*immedsz_ret = 4;
      }
      else if (AMD64_OP_IMMEDSIZE(op) == 8)
      {
	*rex=*rex|0x08;
	*immedsz_ret = 8;
      }
      else{
	FATAL(("Wrong immediate size"));
      }
      if(rexw(*rex)){
	*immedsz_ret = 8;
      }
      return;
    default:
      FATAL(("unexpected bytemode"));
  }
}

void Amd64OpAsM(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* this is identical to the E case but the operand mustn't be a register */
  /* the byte mode here does nothing for the assembled form of the
   * instruction, it only determines how many bytes will be read from memory.
   * Therefore, we just call OpAsE with a amd64_bm_d bytemode */
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_mem, ("We need a memory operand here"));
  Amd64OpAsE(ins, op,amd64_bm_d,mod_ret,reg_ret,rm_ret,scale_ret,index_ret,base_ret,has_sib_ret,immediate_ret,immedsz_ret,displacement_ret,dispsz_ret,segsel_ret,has_segsel_ret,rex);
}

void Amd64OpAsO(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* memory operand. the offset is coded as an immediate, so no mod/rm byte
   * necessary */
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_mem, ("wrong operand type"));
  ASSERT(AMD64_OP_BASE(op) == AMD64_REG_NONE, ("should not have a base register here"));
  ASSERT(AMD64_REG_NONE == AMD64_OP_INDEX(op), ("should not have an index register here"));

  *immediate_ret = AMD64_OP_IMMEDIATE(op);
  *immedsz_ret = AMD64_OP_IMMEDSIZE(op);
  if(bm == amd64_bm_v && AMD64_OP_MEMOPSIZE(op) == 8){
    *rex=*rex|0x08;
  }
}

void Amd64OpAsO8(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* memory operand. the offset is coded as an immediate, so no mod/rm byte
   *    * necessary */
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_mem, ("wrong operand type"));
  ASSERT(AMD64_OP_BASE(op) == AMD64_REG_NONE, ("should not have a base register here"));
  ASSERT(AMD64_REG_NONE == AMD64_OP_INDEX(op), ("should not have an index register here"));
  
  *immediate_ret = AMD64_OP_IMMEDIATE(op);
  *immedsz_ret = AMD64_OP_IMMEDSIZE(op);
  if(bm == amd64_bm_v && AMD64_OP_MEMOPSIZE(op) == 8){
    *rex=*rex|0x08;
  }
}


void Amd64OpAsR(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* this is identical to the E case but the operand must be a register */
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_reg, ("We need a register operand here"));
  Amd64OpAsE(ins, op,bm,mod_ret,reg_ret,rm_ret,scale_ret,index_ret,base_ret,has_sib_ret,immediate_ret,immedsz_ret,displacement_ret,dispsz_ret,segsel_ret,has_segsel_ret,rex);
}

void Amd64OpAsS(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* segment register */
  int segregs[] = {1,3,0,4,5,2,-1,-1};
  ASSERT(AMD64_OP_TYPE(op) == amd64_optype_reg, ("need register operand here"));
  ASSERT(AMD64_REG_CS <= AMD64_OP_BASE(op) && AMD64_OP_BASE(op) <= AMD64_REG_SS, ("need a segment register"));
  *reg_ret = segregs[AMD64_OP_BASE(op) - AMD64_REG_CS];
}

void Amd64OpAsX(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* implicit register. do nothing */
  if(bm == amd64_bm_v && AMD64_OP_MEMOPSIZE(op) == 8){
    *rex=*rex|0x08;
  } 
}

void Amd64OpAsY(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  /* implicit register. do nothing */
  if(bm == amd64_bm_v && AMD64_OP_MEMOPSIZE(op) == 8){
    *rex=*rex|0x08;
  }
}

void Amd64OpAsV(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  *reg_ret = AMD64_OP_BASE(op) - AMD64_REG_XMM0;	  
}

void Amd64OpAsW(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_reg)
  {
    *mod_ret = 3;
    *rm_ret = AMD64_OP_BASE(op) - AMD64_REG_XMM0;
  }
  else if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    t_bool has_sib = FALSE;

    if ((AMD64_OP_INDEX(op) != AMD64_REG_NONE) || (AMD64_OP_BASE(op) == AMD64_REG_RSP) || (AMD64_OP_BASE(op) == AMD64_REG_R12))
    {
has_sib = TRUE;
    }

    if (has_sib)
    {
      *rm_ret = 4;

      ASSERT(AMD64_REG_RSP != AMD64_OP_INDEX(op), ("RSP is not suitable as an index register"));
      if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	*index_ret = normalregs[AMD64_OP_INDEX(op)-AMD64_REG_RAX];
      else
	*index_ret = 4;
      
      *scale_ret = AMD64_OP_SCALE(op);

      if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
      {
	*base_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
	switch (AMD64_OP_IMMEDSIZE(op))
	{
	  case 0:
	    *mod_ret = 0;
	    break;
	  case 1:
	    *mod_ret = 1;
	    break;
	  case 4:
	    *mod_ret = 2;
	    break;
	  default:
	    FATAL(("Wrong displacement size (%d bytes)",AMD64_OP_IMMEDSIZE(op)));
	}
      }
      else
      {
	/* special case: [scaled index] + disp32 */
	*mod_ret = 0;
	*base_ret = 5;
      }
    }
    else
    {
      /* no index register and %esp is not the base register => no sib byte needed */
      
      /* first catch a special case */
      if (AMD64_OP_BASE(op) == AMD64_REG_NONE)
      {
	ASSERT(AMD64_OP_IMMEDSIZE(op) == 4, ("Wrong immediate size for displacement-only memory operand (%d)",AMD64_OP_IMMEDSIZE(op)));
	*mod_ret = 0;
	*rm_ret = 5;
      }
      else
      {
	/* the regular case */
	*rm_ret = normalregs[AMD64_OP_BASE(op) - AMD64_REG_RAX];
	switch (AMD64_OP_IMMEDSIZE(op))
	{
	  case 0:
	    *mod_ret = 0;
	    break;
	  case 1:
	    *mod_ret = 1;
	    break;
	  case 4:
	    *mod_ret = 2;
	    break;
	  default:
	    FATAL(("Wrong displacement size (%d bytes)",AMD64_OP_IMMEDSIZE(op)));
	}
      }
    }

    /* do the displacement */
    *displacement_ret = AMD64_OP_IMMEDIATE(op);
    *dispsz_ret = AMD64_OP_IMMEDSIZE(op);

    *has_sib_ret = has_sib;
  }
  else
    FATAL(("Need operand of type reg or mem"));
}

/* vim: set shiftwidth=2 foldmethod=marker: */
