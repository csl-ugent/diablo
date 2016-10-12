#include <diabloi386.h>

static t_uint32 normalregs[] = {0,3,1,2,6,7,5,4};
static t_uint32 hibyteregs[] = {4,7,5,6};

void I386OpAsNone(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* do nothing */
}

void I386OpAsReg(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* this is called for implicit register operands. do nothing */
}

void I386OpAsST(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* fpu registers are implicit (or filled in during WriteOpcode) */
}

void I386OpAsConst1(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* this is called for an implicit constant operand. do nothing */
}

void I386OpAsA(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  ASSERT(bm == bm_p, ("Unknown bytemode"));
  
  *has_segsel_ret = TRUE;
  *segsel_ret = I386_OP_SEGSELECTOR(op);
  *immediate_ret = I386_OP_IMMEDIATE(op);
  *immedsz_ret = I386_OP_IMMEDSIZE(op);
}

void I386OpAsC(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* control register: always in the reg field */
  *reg_ret = I386_OP_BASE(op) - I386_REG_CR0;
}

void I386OpAsD(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* debug register: always in the reg field */
  *reg_ret = I386_OP_BASE(op) - I386_REG_DR0;
}

void I386OpAsE(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    *mod_ret = 3;
    if (I386_OP_REGMODE(op) != i386_regmode_hi8)
      *rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
    else
      *rm_ret = hibyteregs[I386_OP_BASE(op) - I386_REG_EAX];
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_bool has_sib = FALSE;

    if ((I386_OP_INDEX(op) != I386_REG_NONE) || (I386_OP_BASE(op) == I386_REG_ESP))
    {
      has_sib = TRUE;
    }

    if (has_sib)
    {
      *rm_ret = 4;

      ASSERT(I386_REG_ESP != I386_OP_INDEX(op), ("ESP is not suitable as an index register"));
      if (I386_OP_INDEX(op) != I386_REG_NONE)
	      *index_ret = normalregs[I386_OP_INDEX(op)-I386_REG_EAX];
      else
	      *index_ret = 4;
      
      *scale_ret = I386_OP_SCALE(op);

      if (I386_OP_BASE(op) != I386_REG_NONE)
      {
	      *base_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	      switch (I386_OP_IMMEDSIZE(op))
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
	          FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
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
      if (I386_OP_BASE(op) == I386_REG_NONE)
      {
	ASSERT(I386_OP_IMMEDSIZE(op) == 4, ("Wrong immediate size for displacement-only memory operand (%d)",I386_OP_IMMEDSIZE(op)));
	*mod_ret = 0;
	*rm_ret = 5;
      }
      else
      {
	/* the regular case */
	*rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	switch (I386_OP_IMMEDSIZE(op))
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
	    FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
	}
      }
    }

    /* do the displacement */
    *displacement_ret = I386_OP_IMMEDIATE(op);
    *dispsz_ret = I386_OP_IMMEDSIZE(op);

    *has_sib_ret = has_sib;
  }
  else
    FATAL(("Need operand of type reg or mem"));
}


void I386OpAsF(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* this selects the eflags register, which is implicit in the opcode. do
   * nothing */
}

void I386OpAsG(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  ASSERT(I386_OP_TYPE(op) == i386_optype_reg, ("G mode but operand not a register"));
  ASSERT(I386IsGeneralPurposeReg(I386_OP_BASE(op)),("Expected a general purpose register"));
  if (I386_OP_REGMODE(op) == i386_regmode_hi8)
    *reg_ret = hibyteregs[I386_OP_BASE(op) - I386_REG_EAX];
  else
    *reg_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
}

void I386OpAssI(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  int immval;
  ASSERT(I386_OP_TYPE(op) == i386_optype_imm, ("wrong operand type"));
  immval = (int) I386_OP_IMMEDIATE(op);
  *immediate_ret = I386_OP_IMMEDIATE(op);
  switch (bm)
  {
    case bm_b:
      ASSERT(-128 <= immval && immval <= 127, ("Value too big for one byte (%d)",immval));
      *immedsz_ret = 1;
      break;
    case bm_w:
      ASSERT(-32768 <= immval && immval <= 32767, ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 2;
      break;
    case bm_v:
      switch (I386_OP_IMMEDSIZE(op))
      {
	case 2:
	  ASSERT(-32768 <= immval && immval <= 32767, ("Value too big for two bytes (%d)",immval));
	  *immedsz_ret = 2;
	  break;
	case 4:
	  /* No use to check bounds here, int is 32 bit... */
	  /*ASSERT((-2147483647 <= (immval+1)) && immval <= 2147483647, ("Value too big for four bytes (%d)",immval));*/
	  *immedsz_ret = 4;
	  break;
	default:
	  FATAL(("bm_v only includes bm_v and bm_w"));
      }
      break;
    default:
      FATAL(("Wrong bytemode for this operand type"));
  }
}

void I386OpAsI(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  t_uint32 immval = I386_OP_IMMEDIATE(op);
  ASSERT(I386_OP_TYPE(op) == i386_optype_imm, ("wrong operand type"));
  *immediate_ret = immval;
  switch (bm)
  {
    case bm_b:
      ASSERT(immval < 256, ("Value too big for one byte (%d)",immval));
      *immedsz_ret = 1;
      return;
    case bm_w:
      ASSERT(immval < 65536, ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 2;
      return;
    case bm_d:
      *immedsz_ret = 4;
      return;
    case bm_v:
      if (I386_OP_IMMEDSIZE(op) == 2)
      {
	ASSERT(immval < 65536, ("Value too big for two bytes (%d)",immval));
	*immedsz_ret = 2;
      }
      else if (I386_OP_IMMEDSIZE(op) == 4)
      {
	*immedsz_ret = 4;
      }
      else
	FATAL(("Wrong immediate size"));
      return;
    default:
      FATAL(("unexpected bytemode"));
  }
}

void I386OpAsJ(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  t_int32 immval = (t_int32) I386_OP_IMMEDIATE(op);

  *immediate_ret = I386_OP_IMMEDIATE(op);

  if (bm == bm_b)
  {
    ASSERT(-128 <= immval && immval <= 127, ("Value too big for one byte (%d)",immval));
    *immedsz_ret = 1;
  }
  else if (bm == bm_v)
  {
    if (I386_OP_IMMEDSIZE(op) == 2)
    {
      ASSERT(-32768 <= immval && immval <= 32767, ("Value too big for two bytes (%d)",immval));
      *immedsz_ret = 2;
    }
    else if (I386_OP_IMMEDSIZE(op) == 4)
    {
      *immedsz_ret = 4;
    }
  }
  else
    FATAL(("Unexpected bytemode"));
}

void I386OpAsM(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* this is identical to the E case but the operand mustn't be a register */
  /* the byte mode here does nothing for the assembled form of the
   * instruction, it only determines how many bytes will be read from memory.
   * Therefore, we just call OpAsE with a bm_d bytemode */
  ASSERT(I386_OP_TYPE(op) == i386_optype_mem, ("We need a memory operand here"));
  I386OpAsE(op,bm_d,mod_ret,reg_ret,rm_ret,scale_ret,index_ret,base_ret,has_sib_ret,immediate_ret,immedsz_ret,displacement_ret,dispsz_ret,segsel_ret,has_segsel_ret);
}

void I386OpAsO(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* memory operand. the offset is coded as an immediate, so no mod/rm byte
   * necessary */
  ASSERT(I386_OP_TYPE(op) == i386_optype_mem, ("wrong operand type"));
  ASSERT(I386_OP_BASE(op) == I386_REG_NONE, ("should not have a base register here"));
  ASSERT(I386_REG_NONE == I386_OP_INDEX(op), ("should not have an index register here"));

  *immediate_ret = I386_OP_IMMEDIATE(op);
  *immedsz_ret = I386_OP_IMMEDSIZE(op);
}

void I386OpAsR(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* this is identical to the E case but the operand must be a register */
  ASSERT(I386_OP_TYPE(op) == i386_optype_reg, ("We need a register operand here"));
  I386OpAsE(op,bm,mod_ret,reg_ret,rm_ret,scale_ret,index_ret,base_ret,has_sib_ret,immediate_ret,immedsz_ret,displacement_ret,dispsz_ret,segsel_ret,has_segsel_ret);
}

void I386OpAsS(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* segment register */
  int segregs[] = {1,3,0,4,5,2,-1,-1};
  ASSERT(I386_OP_TYPE(op) == i386_optype_reg, ("need register operand here"));
  ASSERT(I386_REG_CS <= I386_OP_BASE(op) && I386_OP_BASE(op) <= I386_REG_SS, ("need a segment register"));
  *reg_ret = segregs[I386_OP_BASE(op) - I386_REG_CS];
}

void I386OpAsX(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* implicit register. do nothing */
}

void I386OpAsY(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  /* implicit register. do nothing */
}

void I386OpAsV(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    *mod_ret = 3;
    if (I386_OP_REGMODE(op) != i386_regmode_hi8)
      *rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
    else
      *rm_ret = hibyteregs[I386_OP_BASE(op) - I386_REG_EAX];
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_bool has_sib = FALSE;

    if ((I386_OP_INDEX(op) != I386_REG_NONE) || (I386_OP_BASE(op) == I386_REG_ESP))
    {
      has_sib = TRUE;
    }

    if (has_sib)
    {
      *rm_ret = 4;

      ASSERT(I386_REG_ESP != I386_OP_INDEX(op), ("ESP is not suitable as an index register"));
      if (I386_OP_INDEX(op) != I386_REG_NONE)
	*index_ret = normalregs[I386_OP_INDEX(op)-I386_REG_EAX];
      else
	*index_ret = 4;
      
      *scale_ret = I386_OP_SCALE(op);

      if (I386_OP_BASE(op) != I386_REG_NONE)
      {
	*base_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	switch (I386_OP_IMMEDSIZE(op))
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
	    FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
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
      if (I386_OP_BASE(op) == I386_REG_NONE)
      {
	ASSERT(I386_OP_IMMEDSIZE(op) == 4, ("Wrong immediate size for displacement-only memory operand (%d)",I386_OP_IMMEDSIZE(op)));
	*mod_ret = 0;
	*rm_ret = 5;
      }
      else
      {
	/* the regular case */
	*rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	switch (I386_OP_IMMEDSIZE(op))
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
	    FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
	}
      }
    }

    /* do the displacement */
    *displacement_ret = I386_OP_IMMEDIATE(op);
    *dispsz_ret = I386_OP_IMMEDSIZE(op);

    *has_sib_ret = has_sib;
  }
  else
    FATAL(("Need operand of type reg or mem"));
}

void I386OpAsW(t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    *mod_ret = 3;
    if (I386_OP_REGMODE(op) != i386_regmode_hi8)
      *rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
    else
      *rm_ret = hibyteregs[I386_OP_BASE(op) - I386_REG_EAX];
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_bool has_sib = FALSE;

    if ((I386_OP_INDEX(op) != I386_REG_NONE) || (I386_OP_BASE(op) == I386_REG_ESP))
    {
      has_sib = TRUE;
    }

    if (has_sib)
    {
      *rm_ret = 4;

      ASSERT(I386_REG_ESP != I386_OP_INDEX(op), ("ESP is not suitable as an index register"));
      if (I386_OP_INDEX(op) != I386_REG_NONE)
	*index_ret = normalregs[I386_OP_INDEX(op)-I386_REG_EAX];
      else
	*index_ret = 4;
      
      *scale_ret = I386_OP_SCALE(op);

      if (I386_OP_BASE(op) != I386_REG_NONE)
      {
	*base_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	switch (I386_OP_IMMEDSIZE(op))
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
	    FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
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
      if (I386_OP_BASE(op) == I386_REG_NONE)
      {
	ASSERT(I386_OP_IMMEDSIZE(op) == 4, ("Wrong immediate size for displacement-only memory operand (%d)",I386_OP_IMMEDSIZE(op)));
	*mod_ret = 0;
	*rm_ret = 5;
      }
      else
      {
	/* the regular case */
	*rm_ret = normalregs[I386_OP_BASE(op) - I386_REG_EAX];
	switch (I386_OP_IMMEDSIZE(op))
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
	    FATAL(("Wrong displacement size (%d bytes)",I386_OP_IMMEDSIZE(op)));
	}
      }
    }

    /* do the displacement */
    *displacement_ret = I386_OP_IMMEDIATE(op);
    *dispsz_ret = I386_OP_IMMEDSIZE(op);

    *has_sib_ret = has_sib;
  }
  else
    FATAL(("Need operand of type reg or mem"));
}

/* vim: set shiftwidth=2 foldmethod=marker: */
