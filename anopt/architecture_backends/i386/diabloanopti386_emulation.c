/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h>
#include <diabloanopti386.h>

/* TODO ugly hack! */
extern t_argstate *current_argstate;

#define DUMMY_ARGREG	I386_REG_XMM7

/* bit manipulation macros:
 *   x is the value to be manipulated
 *   p is the bit position of the most significant bit
 *     (e.g. for 8-bit values, p = 7)
 */
#define sign_extend(x,p)        ((((t_uint32) x) ^ (1u << (p))) - (1u << (p)))
#define get_bit(x,p)		(((t_uint32) x) & (1u << (p)))
#define sign_unextend(x,p)	((t_uint32)(((unsigned long long) x) & ((1ULL << (p+1))-1)))
#define down_bit(x,p)		((((t_uint32) x) >> (p)) & 1)
#define parity(x)		((down_bit(x,0) + down_bit(x,1) + \
      				  down_bit(x,2) + down_bit(x,3) + \
      				  down_bit(x,4) + down_bit(x,5) + \
				  down_bit(x,6) + down_bit(x,7) + \
				  1) % 2)

static void SetArg(t_i386_ins *iins, long argno, t_procstate *state)
{
  t_register_content rc;
  t_lattice_level vlevel, tlevel;
  t_reloc *rel;
  t_bool change;
  t_ins *ins = T_INS(iins);

  vlevel = ProcStateGetReg(state, DUMMY_ARGREG, &rc);
  tlevel = ProcStateGetTag(state, DUMMY_ARGREG, &rel);
  change = ArgStateSetArg(CFG_EDGE_ARGS(INS_DEFEDGE(ins)), argno,
      vlevel, rc.i, tlevel, rel);
  if (change)
    CFG_EDGE_SET_ARGS_CHANGED(INS_DEFEDGE(ins), TRUE);
}

/* {{{ GetConstValueForOp */
void GetConstValueForOp(
    t_i386_ins *ins, t_i386_operand *op, t_procstate *state, t_bool sign_extend,
    t_lattice_level *vlevel_r, t_uint32 *val_r,
    t_lattice_level *rlevel_r, t_reloc **rel_r)
{
  t_register_content cont;
  int bits = 32;
  t_lattice_level vlevel, rlevel;
  t_int32 val = 0;
  t_reloc * rel = NULL;

  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      /* {{{ register operand */
      vlevel = ProcStateGetReg(state,I386_OP_BASE(op),&cont);
      rlevel = ProcStateGetTag(state,I386_OP_BASE(op),&rel);

      if (rlevel == CP_VALUE)
	if (I386_OP_REGMODE(op) != i386_regmode_full32)
	  rlevel = CP_BOT;

      if (vlevel == CP_VALUE)
      {
	val = G_T_UINT32(cont.i);
	if (I386_OP_REGMODE(op) == i386_regmode_lo8)
	{
	  val &= 0xff;
	  bits = 8;
	}
	else if (I386_OP_REGMODE(op) == i386_regmode_hi8)
	{
	  val = (val >> 8) & 0xff;
	  bits = 8;
	}
	else if (I386_OP_REGMODE(op) == i386_regmode_lo16)
	{
	  val &= 0xffff;
	  bits = 16;
	}
	/*DiabloPrint(stdout,"CONSTANT REG %s = %d\n",I386RegisterName(I386_OP_BASE(op)),val);*/
      }
      break;
      /* }}} */

    case i386_optype_imm:
      /* {{{ immediate operand */
      /* immediate values have already been extended to 32 bits at disassembly */
      val = I386_OP_IMMEDIATE(op);
      vlevel = CP_VALUE;
      if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
      {
	rlevel = CP_VALUE;
	rel = I386GetRelocForOp(ins,op);
      }
      else
	rlevel = CP_TOP;
      break;
      /* }}} */

    case i386_optype_mem:
      /* {{{ memory operand */
      if (INS_USEARG(T_INS(ins)) == -1L)
      {
	t_register_content base, index;
	t_lattice_level blevel = CP_TOP, ilevel = CP_TOP, immlevel = CP_TOP;
	t_lattice_level brlevel = CP_TOP, irlevel = CP_TOP, immrlevel = CP_TOP;
	t_reloc *brel, *irel, *immrel = NULL, *loadrel;
	t_uint32 immediate, address;
	int relcount = 0;
	t_bool has_base = (I386_OP_BASE(op) != I386_REG_NONE);
	t_bool has_index = (I386_OP_INDEX(op) != I386_REG_NONE);
	t_bool address_is_bot = FALSE, loadrel_is_bot = FALSE;

	/* {{{ extract the different parts of the address */
	if (has_base)
	{
	  blevel = ProcStateGetReg(state,I386_OP_BASE(op),&base);
	  brlevel = ProcStateGetTag(state,I386_OP_BASE(op),&brel);
	  if (brlevel != CP_TOP) ++relcount;
	}
	if (has_index)
	{
	  ilevel = ProcStateGetReg(state,I386_OP_INDEX(op),&index);
	  irlevel = ProcStateGetTag(state,I386_OP_INDEX(op),&irel);
	  if (irlevel != CP_TOP) ++relcount;
	}
	immlevel = CP_VALUE;
	immediate = I386_OP_IMMEDIATE(op);
	if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
	{
	  immrlevel = CP_VALUE;
	  immrel = I386GetRelocForOp(ins,op);
	  ++relcount;
	}
	/* }}} */

	/* {{{ compute address */
	address = immediate;
	if (has_base)
	{
	  if (blevel == CP_VALUE)
	    address += G_T_UINT32(base.i);
	  else
	    address_is_bot = TRUE;
	}
	if (has_index)
	{
	  if (ilevel == CP_VALUE)
	    address += G_T_UINT32(index.i) << I386_OP_SCALE(op);
	  else
	    address_is_bot = TRUE;
	}
	/* }}} */

	/* {{{ compute load relocation */
	loadrel = NULL;
	if (relcount == 1)
	{
	  if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
	    loadrel = immrel;
	  else if (has_base && (brlevel != CP_TOP))
	  {
	    if (brlevel == CP_VALUE)
	      loadrel = brel;
	    else
	      loadrel_is_bot = TRUE;
	  }
	  else if (has_index && (irlevel != CP_TOP))
	  {
	    if (irlevel == CP_VALUE)
	      loadrel = irel;
	    else
	      loadrel_is_bot = TRUE;
	  }
	}
	else if (relcount > 1)
	  loadrel_is_bot = TRUE;
	/* }}} */

	if (I386_INS_OPCODE(ins) == I386_LEA)
	{
	  /* {{{ for lea instructions no actual load is performed */
	  if (address_is_bot)
	    vlevel = CP_BOT;
	  else
	  {
	    vlevel = CP_VALUE;
	    val = address;
	  }
	  if (relcount == 0)
	    rlevel = CP_TOP;
	  else if (loadrel_is_bot)
	    rlevel = CP_BOT;
	  else
	  {
	    rlevel = CP_VALUE;
	    rel = loadrel;
	  }
	  /* }}} */
	}
	else
	{
	  if (address_is_bot || loadrel_is_bot || relcount == 0) 
	  {
	    rlevel = CP_BOT;
	    vlevel = CP_BOT;
	  }
	  else if (RELOC_N_TO_RELOCATABLES(loadrel)==1)
	  {
	    t_bool can_load = TRUE;
	    t_relocatable *r = RELOC_TO_RELOCATABLE(loadrel)[0];
	    t_address aaddress=AddressNew32(address);
	    if (RELOCATABLE_RELOCATABLE_TYPE(r) == RT_BBL)
	    {
	      /* do not load from data blocks in the code. while technically
	       * this is possible (the commented-out code does it right), we
	       * do not expect any gain from it: on the i386 there is no need
	       * for data blocks in the code, so compilers will never generate
	       * data in the code sections.
	       * Furthermore, the 2.4 linux kernel has a data block in the code
	       * section (swapper_pg_dir) that is writable! This means that the
	       * code should be turned off for kdiablo anyway */	      
	      t_bbl *bbl = T_BBL(r);
	      t_i386_ins *iter;
	      int i;
        vlevel = CP_BOT;
	      rlevel = CP_BOT;

	      ASSERT(IS_DATABBL(bbl),("load from non-databbl"));
	      if (
		  AddressIsGe(aaddress,BBL_CADDRESS(bbl)) &&
		  AddressIsLt(aaddress,
		    AddressAdd(BBL_CADDRESS(bbl),BBL_CSIZE(bbl))) &&
		  (!DiabloFlowgraphInKernelMode() || 
		   address < 0xc0100000 || address > 0xc0105000)
		 )
	      {
		for (iter=T_I386_INS(BBL_INS_FIRST(bbl)); iter; iter = I386_INS_INEXT(iter))
		  if (AddressIsEq(I386_INS_CADDRESS(iter), aaddress))
		    break;
		/* {{{ is there a reloc attached? */
		if (I386_INS_REFERS_TO(iter))
		{
		  rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(iter));
		  rlevel = CP_VALUE;
		}
		else
		  rlevel = CP_TOP;
		/* }}} */
		/*  {{{ load the value */
		val = 0;
		for (i=0; i<I386_OP_MEMOPSIZE(op) && i<4; i++)
		{
		  ASSERT(iter,("not enough data in block to perform load"));
		  val |= I386_INS_DATA(iter) << 8*i;
		  iter = I386_INS_INEXT(iter);
		}
		bits = 8*I386_OP_MEMOPSIZE(op);
		vlevel = CP_VALUE;
		/* }}} */
	      }
	      else
	      {
		vlevel = CP_BOT;
		rlevel = CP_BOT;
	      }
	    }
	    else if (RELOCATABLE_RELOCATABLE_TYPE(r) == RT_SUBSECTION)
	    {
	      t_section * sec = T_SECTION(r);
	      t_address offset = AddressSub(aaddress,SECTION_CADDRESS(sec));
	      t_bool ignored = FALSE;
	      if (SECTION_TYPE(sec) != RODATA_SECTION && 
		  !IsKnownToBeConstant(aaddress,loadrel))
		can_load = FALSE;
	      if (AddressIsGe(offset,SECTION_CSIZE(sec)))
		can_load = FALSE;

	      if (can_load)
	      {
		/* {{{ perform the actual load */
		if (SECTION_TYPE(sec) == BSS_SECTION)
		{
		  val = 0;
		  vlevel = CP_VALUE;
		  rlevel = CP_TOP;
		}
		else
		{
		  t_reloc_ref * rr;
		  for (rr = SECTION_REFERS_TO(sec); rr; rr = RELOC_REF_NEXT(rr))
		    if (AddressIsEq(RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)), offset))
		      break;
		  val = SectionGetData32Ignore(sec,offset, &ignored);
		  vlevel = CP_VALUE;
		  if (rr)
		  {
		    rlevel = CP_VALUE;
		    rel = RELOC_REF_RELOC(rr);
		  }
		  else
		    rlevel = CP_TOP;

		  bits = 8*I386_OP_MEMOPSIZE(op);
		  if(bits<32)
		    val &= ((1U << bits) - 1);
		}
		/* }}} */
	      }
	      else
	      {
		vlevel = CP_BOT;
		rlevel = CP_BOT;
	      }
	    }
	    else
	    {
	      /* load from a section or something like that */
	      vlevel = CP_BOT;
	      rlevel = CP_BOT;
	    }
	  }
	  else
	  {
            vlevel = CP_BOT;
            rlevel = CP_BOT;
          }	
	}
      }
      else
      {
	/* instruction accesses a forwarded function argument */
	int argno = INS_USEARG(T_INS(ins));
	if (current_argstate)
	{
	  vlevel = current_argstate->vlevel[argno];
	  rlevel = current_argstate->tlevel[argno];
	  if (vlevel == CP_VALUE)
	    val = G_T_UINT32(current_argstate->val[argno]);
	  rel = current_argstate->tag[argno];

#if 0
	  if (vlevel == CP_TOP)
	    VERBOSE(0,("not defined"));
	  else if (vlevel == CP_VALUE)
	    VERBOSE(0, ("value %x", val));
	  if (rlevel == CP_TOP)
	    VERBOSE(0, ("no tag"));
	  else if (rlevel == CP_VALUE)
	    VERBOSE(0, ("one reloc"));
#endif

	  /* if we were unable to find a definition for this argument,
	   * the value level will still be TOP. In this case, the proper
	   * conservative treatment is to assume BOT for this argument */
	  if (vlevel == CP_TOP)
	  {
	    vlevel = CP_BOT;
	    rlevel = CP_BOT;
	  }
	}
	else
	{
	  vlevel = CP_BOT;
	  rlevel = CP_BOT;
	}
      }
      break;
      /* }}} */

    default:
      /* just return CP_BOT on all counts */
      vlevel = rlevel = CP_BOT;
  }

  if (sign_extend)
    val = sign_extend(val,bits-1);

  *vlevel_r = vlevel;
  *rlevel_r = rlevel;
  *val_r = val;
  *rel_r = rel;
} /* }}} */

/* {{{ GetBitsForOp */
static int GetBitsForOp(t_i386_operand *op)
{
  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      switch (I386_OP_REGMODE(op))
      {
	case i386_regmode_lo8:
	case i386_regmode_hi8:
	  return 8;
	case i386_regmode_lo16:
	  return 16;
	case i386_regmode_full32:
	  return 32;
        case i386_regmode_invalid:
	  FATAL(("OOPS"));
      }
      break;
    case i386_optype_imm:
      return 8*I386_OP_IMMEDSIZE(op);
    case i386_optype_mem:
      return 8*I386_OP_MEMOPSIZE(op);
    default:
      FATAL(("implement"));
  }
  return 0;
} /* }}} */

/* {{{ ComputeFinalRegValue */
static t_uint32 ComputeFinalRegValue(t_procstate *orig, t_reg r, t_i386_regmode mode, t_uint32 value)
{
  t_uint32 origval;
  t_register_content rc;

  if (mode == i386_regmode_full32) return value;

  if (ProcStateGetReg(orig,r,&rc) != CP_VALUE)
    FATAL(("need constant original value"));

  origval = G_T_UINT32(rc.i);
  switch (mode)
  {
    case i386_regmode_lo8:
      return (origval & 0xffffff00) | (value & 0xff);
    case i386_regmode_hi8:
      return (origval & 0xffff00ff) | ((value & 0xff) << 8);
    case i386_regmode_lo16:
      return (origval & 0xffff0000) | (value & 0xffff);
    default:
      FATAL(("should not get here"));
  }
  return value;
} /* }}} */

/* {{{ I386ConditionHolds */
t_tristate I386ConditionHolds(t_i386_condition_code cond, t_procstate * state)
{
  t_bool of,cf,zf,sf,pf;
  t_bool kof,kcf,kzf,ksf,kpf;
  t_register_content ecx;
  t_bool kecx;

  kof = ProcStateGetCond(state,I386_CONDREG_OF,&of) == CP_VALUE;
  kcf = ProcStateGetCond(state,I386_CONDREG_CF,&cf) == CP_VALUE;
  kzf = ProcStateGetCond(state,I386_CONDREG_ZF,&zf) == CP_VALUE;
  ksf = ProcStateGetCond(state,I386_CONDREG_SF,&sf) == CP_VALUE;
  kpf = ProcStateGetCond(state,I386_CONDREG_PF,&pf) == CP_VALUE;
  kecx = ProcStateGetReg(state,I386_REG_ECX,&ecx) == CP_VALUE;

  switch (cond)
  {
    case I386_CONDITION_O :
      if (!kof) return PERHAPS;
      return of ? YES : NO;

    case I386_CONDITION_NO:
      if (!kof) return PERHAPS;
      return !of ? YES : NO;

    case I386_CONDITION_B :
      if (!kcf) return PERHAPS;
      return cf ? YES : NO;

    case I386_CONDITION_AE:
      if (!kcf) return PERHAPS;
      return !cf ? YES : NO;

    case I386_CONDITION_Z :
      if (!kzf) return PERHAPS;
      return zf ? YES : NO;

    case I386_CONDITION_NZ:
      if (!kzf) return PERHAPS;
      return !zf ? YES : NO;

    case I386_CONDITION_BE:
      {
	t_tristate b, z;
	b = I386ConditionHolds(I386_CONDITION_B,state);
	z = I386ConditionHolds(I386_CONDITION_Z,state);

	if (b == YES || z == YES)
	  return YES;
	if (b == PERHAPS || z == PERHAPS)
	  return PERHAPS;
	return NO;
      }

    case I386_CONDITION_A :
      {
	t_tristate ae, nz;
	ae = I386ConditionHolds(I386_CONDITION_AE,state);
	nz = I386ConditionHolds(I386_CONDITION_NZ,state);
	if (ae == NO || nz == NO)
	  return NO;
	if (ae == PERHAPS || nz == PERHAPS)
	  return PERHAPS;
	return YES;
      }

    case I386_CONDITION_S :
      if (!ksf) return PERHAPS;
      return sf ? YES : NO;

    case I386_CONDITION_NS:
      if (!ksf) return PERHAPS;
      return !sf ? YES : NO;

    case I386_CONDITION_P :
      if (!kpf) return PERHAPS;
      return pf ? YES : NO;

    case I386_CONDITION_NP:
      if (!kpf) return PERHAPS;
      return !pf ? YES : NO;

    case I386_CONDITION_L :
      if (!ksf || !kof) return PERHAPS;
      return (sf != of) ? YES : NO;

    case I386_CONDITION_GE:
      if (!ksf || !kof) return PERHAPS;
      return (sf == of) ? YES : NO;

    case I386_CONDITION_LE:
      {
	t_tristate z, l;
	z = I386ConditionHolds(I386_CONDITION_Z,state);
	l = I386ConditionHolds(I386_CONDITION_L,state);
	if (z == YES || l == YES)
	  return YES;
	if (z == PERHAPS || l == PERHAPS)
	  return PERHAPS;
	return NO;
      }

    case I386_CONDITION_G :
      {
	t_tristate ge, nz;
	ge = I386ConditionHolds(I386_CONDITION_GE,state);
	nz = I386ConditionHolds(I386_CONDITION_NZ,state);
	if (ge == NO || nz == NO)
	  return NO;
	if (ge == PERHAPS || nz == PERHAPS)
	  return PERHAPS;
	return YES;
      }

    case I386_CONDITION_LOOP:
      if (!kecx) return PERHAPS;
      return (AddressIsNull(ecx.i)) ? NO : YES;

    case I386_CONDITION_LOOPZ :
      {
	t_tristate loop,z;
	loop = I386ConditionHolds(I386_CONDITION_LOOP,state);
	z = I386ConditionHolds(I386_CONDITION_Z,state);
	if (loop == NO || z == NO)
	  return NO;
	if (loop == PERHAPS || z == PERHAPS)
	  return PERHAPS;
	return YES;
      }

    case I386_CONDITION_LOOPNZ:
      {
	t_tristate loop,nz;
	loop = I386ConditionHolds(I386_CONDITION_LOOP,state);
	nz = I386ConditionHolds(I386_CONDITION_NZ,state);
	if (loop == NO || nz == NO)
	  return NO;
	if (loop == PERHAPS || nz == PERHAPS)
	  return PERHAPS;
	return YES;
      }

    case I386_CONDITION_ECXZ:
      if (!kecx) return PERHAPS;
      return (AddressIsNull(ecx.i)) ? YES : NO;

    case I386_CONDITION_NONE:
      return YES;
  }
  return YES;
} /* }}} */

/* {{{ EmulAdd */
static void EmulAdd(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand * dest = I386_INS_DEST(ins);
  t_i386_operand * src = I386_INS_SOURCE1(ins);
  t_procstate * orig = ProcStateNew(&i386_description);

  t_lattice_level destlevel, rdestlevel, srclevel, rsrclevel;
  t_reloc *srcrel, *destrel;
  t_int32 srcval, destval;
  t_uint32 usrcval, udestval;

  ProcStateDup(orig,state,&i386_description);
  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));

  
  GetConstValueForOp(ins,dest,orig,TRUE,&destlevel,&udestval,&rdestlevel,&destrel);
  destval = (t_int32) udestval;
  GetConstValueForOp(ins,src,orig,TRUE,&srclevel,&usrcval,&rsrclevel,&srcrel);
  srcval = (t_int32) usrcval;
  
  if (I386_OP_TYPE(dest) == i386_optype_reg || INS_DEFARG(T_INS(ins)) != -1)
  {
    t_reg d = I386_OP_BASE(dest);
    t_i386_regmode mode = I386_OP_REGMODE(dest);
    long argno = -1;
    t_reloc * finaltag = NULL;
    int relcount = 0;

    /* hack: for argdefs, use a register that is certainly unused.
     * when emulation is finished, we can read the argstate settings from the 
     * procstate entry for this register */
    if (INS_DEFARG(T_INS(ins)) != -1)
    {
      d = DUMMY_ARGREG;
      mode = i386_regmode_full32;
      argno = INS_DEFARG(T_INS(ins));
    }

    /* {{{ compute final relocation */    
    if (rdestlevel != CP_TOP)
    {
      relcount++;
      if (rdestlevel == CP_VALUE) finaltag = destrel;
    }
    if (rsrclevel != CP_TOP)
    {
      relcount++;
      if (rsrclevel == CP_VALUE) finaltag = srcrel;
    }
    if (relcount == 0)
      ProcStateSetTagTop(state,d);
    else if (relcount == 1 && finaltag)
      ProcStateSetTag(state,d,finaltag);
    else
      ProcStateSetTagBot(state,d);
    /* }}} */

    /* compute final value */
    if (destlevel == CP_VALUE && srclevel == CP_VALUE)
    {
      t_uint32 finalval = ComputeFinalRegValue(orig,d,I386_OP_REGMODE(dest),srcval+destval);
      t_register_content dummy;
      dummy.i=AddressNew32(finalval);
      ProcStateSetReg(state,d,dummy);
    }
    else
      ProcStateSetRegBot(state,d);

    if (argno != -1)
      SetArg(ins, argno, state);
  }

  /* adjust condition bits */
  if (destlevel == CP_VALUE && srclevel == CP_VALUE)
  {
    int nbits = GetBitsForOp(I386_INS_DEST(ins));
    t_int32 sval = srcval+destval;
    t_uint32 uval = (unsigned) sval;

    ProcStateSetCond(state,I386_CONDREG_ZF,sval == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(sval,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(sval));
    /* overflow can only happen if both operands have the same sign */
    ProcStateSetCond(state,I386_CONDREG_OF,
	get_bit(srcval,nbits-1) == get_bit(destval,nbits-1) 
	&& get_bit(srcval,nbits-1) != get_bit(sval,nbits-1));
    /* carry happens if the unsigned result is smaller than any of the unsigned operands */
    ProcStateSetCond(state,I386_CONDREG_CF,
	sign_unextend(uval,nbits-1) < sign_unextend(srcval,nbits-1));
  }
  ProcStateFree(orig);
} /* }}} */

/* {{{ EmulAnd */
static void EmulAnd(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand * dest = I386_INS_DEST(ins);
  t_i386_operand * src = I386_INS_SOURCE1(ins);
  t_procstate * orig = ProcStateNew(&i386_description);

  t_lattice_level destlevel, rdestlevel, srclevel, rsrclevel;
  t_reloc *srcrel, *destrel;
  t_uint32 srcval, destval;

  ProcStateDup(orig,state,&i386_description);
  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
  
  GetConstValueForOp(ins,dest,orig,TRUE,&destlevel,&destval,&rdestlevel,&destrel);
  GetConstValueForOp(ins,src,orig,TRUE,&srclevel,&srcval,&rsrclevel,&srcrel);
  
  if (I386_OP_TYPE(dest) == i386_optype_reg || INS_DEFARG(T_INS(ins)) != -1)
  {
    t_reg d = I386_OP_BASE(dest);
    t_i386_regmode mode = I386_OP_REGMODE(dest);
    long argno = -1;
    t_reloc * finaltag = NULL;
    int relcount = 0;

    /* hack: for argdefs, use a register that is certainly unused.
     * when emulation is finished, we can read the argstate settings from the 
     * procstate entry for this register */
    if (INS_DEFARG(T_INS(ins)) != -1)
    {
      d = DUMMY_ARGREG;
      mode = i386_regmode_full32;
      argno = INS_DEFARG(T_INS(ins));
    }

    /* {{{ compute final relocation */    
    if (rdestlevel != CP_TOP)
    {
      relcount++;
      if (rdestlevel == CP_VALUE) finaltag = destrel;
    }
    if (rsrclevel != CP_TOP)
    {
      relcount++;
      if (rsrclevel == CP_VALUE) finaltag = srcrel;
    }
    if (relcount == 0)
      ProcStateSetTagTop(state,d);
    else if (relcount == 1 && finaltag)
      ProcStateSetTag(state,d,finaltag);
    else
      ProcStateSetTagBot(state,d);
    /* }}} */

    /* compute final value */
    if (destlevel == CP_VALUE && srclevel == CP_VALUE)
    {
      t_uint32 finalval = ComputeFinalRegValue(orig,d,mode,srcval & destval);
      t_register_content dummy;
      dummy.i=AddressNew32(finalval);
      ProcStateSetReg(state,d,dummy);
    }
    else
      ProcStateSetRegBot(state,d);

    if (argno != -1)
      SetArg(ins, argno, state);
  }

  /* adjust condition bits */
  /* OF and CF are cleared, AF undefined, SF, ZF, PF set according to the result */
  if (destlevel == CP_VALUE && srclevel == CP_VALUE)
  {
    int nbits = GetBitsForOp(I386_INS_DEST(ins));
    t_int32 sval = srcval&destval;

    ProcStateSetCond(state,I386_CONDREG_ZF,sval == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(sval,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(sval));
  }
  ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
  ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
  ProcStateSetCondBot(state,I386_CONDREG_AF);
  
  ProcStateFree(orig);
} /* }}} */

/* {{{ EmulMov */
static void EmulMov(t_i386_ins *ins, t_procstate *state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_reg d = I386_OP_BASE(dest);
  t_i386_regmode mode = I386_OP_REGMODE(dest);
  long argno = -1;

  /* hack: for argdefs, use a register that is certainly unused.
   * when emulation is finished, we can read the argstate settings from the 
   * procstate entry for this register */
  if (INS_DEFARG(T_INS(ins)) != -1)
  {
    d = DUMMY_ARGREG;
    mode = i386_regmode_full32;
    argno = INS_DEFARG(T_INS(ins));
  }

  if (I386_OP_TYPE(dest) == i386_optype_reg || INS_DEFARG(T_INS(ins)) != -1)
  {
    t_register_content dummy;
    t_lattice_level vlevel, rlevel;
    t_uint32 val; t_reloc * rel;
    t_i386_operand *src = I386_INS_SOURCE1(ins);

    t_procstate *orig = ProcStateNew(&i386_description);
    ProcStateDup(orig,state,&i386_description);
    ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));

    GetConstValueForOp(ins,src,orig,TRUE,&vlevel,&val,&rlevel,&rel);

    /* set value */
    if (vlevel == CP_TOP)
      ProcStateSetRegTop(state,I386_OP_BASE(dest));
      //FATAL(("mov CP_TOP into register"));
    else if (vlevel == CP_BOT)
      ProcStateSetRegBot(state,d);
    else
    {
      t_uint32 finalval;
      if (mode != i386_regmode_full32
	  && ProcStateGetReg(state,d,&dummy) != CP_VALUE)
      {
	ProcStateSetRegBot(state,d);
      }
      else
      {
	finalval = ComputeFinalRegValue(orig,d,mode,val);
	dummy.i=AddressNew32(finalval);
	ProcStateSetReg(state,d,dummy);
      }
    }

    /* set reloc */
    if (rlevel == CP_TOP)
      ProcStateSetTagTop(state,d);
    /* lack of i386_regmode_full32 can happen in e.g. inlined memcpy of a constant struct */
    else if (rlevel == CP_BOT || mode != i386_regmode_full32)
      ProcStateSetTagBot(state,d);
    else
    {
      ProcStateSetTag(state,d,rel);
    }

    ProcStateFree(orig);

    if (argno != -1)
      SetArg(ins, argno, state);
  }
  else
    ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
} /* }}} */

int CMPCounter = 0;

/* {{{ EmulCmp */
static void EmulCmp(t_i386_ins *ins, t_procstate *state)
{
  t_int32 val1, val2;
  t_uint32 uval1, uval2;
  t_lattice_level level1, rlevel1, level2, rlevel2;
  t_reloc *rel1, *rel2;
  t_i386_operand *src1 = I386_INS_SOURCE1(ins), *src2 = I386_INS_SOURCE2(ins);

  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
  GetConstValueForOp(ins,src1,state,TRUE,&level1,&uval1,&rlevel1,&rel1);
  val1 = (t_int32) uval1;
  GetConstValueForOp(ins,src2,state,TRUE,&level2,&uval2,&rlevel2,&rel2);
  val2 = (t_int32) uval2;

  if (level1 == CP_VALUE && level2 == CP_VALUE)
  {
    if ((rlevel1 == CP_TOP && rlevel2 == CP_TOP)
	|| (rlevel1 == CP_VALUE && rlevel2 == CP_VALUE && RELOC_N_TO_RELOCATABLES(rel1) == 1 && RELOC_N_TO_RELOCATABLES(rel2)
	    && RELOC_TO_RELOCATABLE(rel1)[0] == RELOC_TO_RELOCATABLE(rel2)[0]))
    {
      t_int32 result = val1 - val2;
      int nbits = GetBitsForOp(src1);

#if 0
      if (!(INS_ATTRIB(ins) & IF_DEBUG_MARK) && (CMPCounter++ < diablosupport_options.debugcounter))
      {
	DiabloPrint(stdout,"CAN PERFORM CMP @I: cmp %d,%d\n",ins,val2,val1);
	INS_ATTRIB(ins) |= IF_DEBUG_MARK;
      }
#endif

      /*if (INS_ATTRIB(ins) & IF_DEBUG_MARK)*/
      {
	ProcStateSetCond(state,I386_CONDREG_ZF,result == 0);
	ProcStateSetCond(state,I386_CONDREG_SF,get_bit(result,nbits-1));
	ProcStateSetCond(state,I386_CONDREG_PF,parity(result));
	/* A-B sets the carry flag if (unsigned)B > (unsigned)A */
	ProcStateSetCond(state,I386_CONDREG_CF,
	    ((unsigned long long)val2) > ((unsigned long long)val1));
	/* A-B sets the overflow flag if sign(A) != sign(B) && sign(A-B) != sign(A) */
	ProcStateSetCond(state,I386_CONDREG_OF,
	    get_bit(val1,nbits-1) != get_bit(val2,nbits-1) 
	    && get_bit(val1,nbits-1) != get_bit(result,nbits-1));
      }
    }
  }
} /* }}} */

/* {{{ EmulSub */
static void EmulSub(t_i386_ins * ins, t_procstate *state)
{
  t_i386_operand * dest = I386_INS_DEST(ins);
  t_i386_operand * src = I386_INS_SOURCE1(ins);
  t_procstate * orig = ProcStateNew(&i386_description);
  t_lattice_level destlevel, rdestlevel, srclevel, rsrclevel;
  t_reloc *srcrel, *destrel;
  t_int32 srcval, destval;
  t_uint32 usrcval, udestval;

  ProcStateDup(orig,state,&i386_description);
  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));

  GetConstValueForOp(ins,dest,orig,TRUE,&destlevel,&udestval,&rdestlevel,&destrel);
  destval = (t_int32) udestval;
  GetConstValueForOp(ins,src,orig,TRUE,&srclevel,&usrcval,&rsrclevel,&srcrel);
  srcval = (t_int32) usrcval;

  if (INS_DEFARG(T_INS(ins)) != -1)
    FATAL(("implement argument definition"));

  /* fill in the destination register */
  if (I386_OP_TYPE(dest) == i386_optype_reg)
  {
    t_reg d = I386_OP_BASE(dest);
    t_reloc * finaltag = NULL;
    int relcount = 0;

    /* {{{ compute final relocation */
    if (rdestlevel != CP_TOP)
    {
      relcount++;
      if (rdestlevel == CP_VALUE) finaltag = destrel;
    }
    if (rsrclevel != CP_TOP)
    {
      relcount++;
      if (rsrclevel == CP_VALUE) finaltag = srcrel;
    }
    if (relcount == 0)
      ProcStateSetTagTop(state,d);
    else if (relcount == 1 && finaltag)
      ProcStateSetTag(state,d,finaltag);
    else
      ProcStateSetTagBot(state,d);
    /* }}} */

    /* compute the final value */
    if (destlevel == CP_VALUE && srclevel == CP_VALUE)
    {
      t_uint32 finalval = ComputeFinalRegValue(orig,d,I386_OP_REGMODE(dest),destval-srcval);
      t_register_content dummy;
      dummy.i=AddressNew32(finalval);
      ProcStateSetReg(state,d,dummy);
    }
    else
      ProcStateSetRegBot(state,d);

  }

  /* set condition flags */
  if (destlevel == CP_VALUE && srclevel == CP_VALUE)
  {
    if ((rdestlevel == CP_TOP && rsrclevel == CP_TOP))
    {
      t_int32 result = destval - srcval;
      int nbits = GetBitsForOp(dest);

      ProcStateSetCond(state,I386_CONDREG_ZF,result == 0);
      ProcStateSetCond(state,I386_CONDREG_SF,get_bit(result,nbits-1));
      ProcStateSetCond(state,I386_CONDREG_PF,parity(result));
      /* A-B sets the carry flag if (unsigned)B > (unsigned)A */
      ProcStateSetCond(state,I386_CONDREG_CF,
	  ((unsigned long long)srcval) > ((unsigned long long) destval));
      /* A-B sets the overflow flag if sign(A) != sign(B) && sign(A-B) != sign(A) */
      ProcStateSetCond(state,I386_CONDREG_OF,
	  get_bit(destval,nbits-1) != get_bit(srcval,nbits-1) 
	  && get_bit(destval,nbits-1) != get_bit(result,nbits-1));
    }
  }
  ProcStateFree(orig);
} /* }}} */

/* {{{ EmulXor */
static void EmulXor(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_i386_operand *src = I386_INS_SOURCE1(ins);



  t_procstate * orig;
  t_lattice_level destlevel, rdestlevel, srclevel, rsrclevel;
  t_reloc *srcrel, *destrel;
  t_uint32 srcval, destval;

  if (INS_DEFARG(T_INS(ins)) != -1)
    FATAL(("implement argument definition"));

#if 1
  /* {{{ special case: xor %reg,%reg */
  if (I386_OP_TYPE(dest) == i386_optype_reg &&
      I386_OP_TYPE(src) == i386_optype_reg && 
      I386_OP_BASE(dest) == I386_OP_BASE(src) &&
      I386_OP_REGMODE(dest) == i386_regmode_full32 &&
      I386_OP_REGMODE(src) == i386_regmode_full32)
  {
    t_register_content dummy;
    dummy.i=AddressNew32(0);
    ProcStateSetTagTop(state,I386_OP_BASE(dest));
    ProcStateSetReg(state,I386_OP_BASE(dest),dummy);
    ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
    ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
    ProcStateSetCond(state,I386_CONDREG_SF,FALSE);
    ProcStateSetCond(state,I386_CONDREG_ZF,TRUE);
    ProcStateSetCond(state,I386_CONDREG_PF,TRUE);
    return;
  } /* }}} */
#endif

  orig = ProcStateNew(&i386_description);

  ProcStateDup(orig,state,&i386_description);
  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));

  
  GetConstValueForOp(ins,dest,orig,TRUE,&destlevel,&destval,&rdestlevel,&destrel);
  GetConstValueForOp(ins,src,orig,TRUE,&srclevel,&srcval,&rsrclevel,&srcrel);

  if (I386_OP_TYPE(dest) == i386_optype_reg)
  {
    t_reg d = I386_OP_BASE(dest);

    /* compute the relocation */
    if (rdestlevel == CP_TOP && rsrclevel == CP_TOP)
      ProcStateSetTagTop(state,d);
    else if (rdestlevel == CP_VALUE && rsrclevel == CP_TOP)
      ProcStateSetTag(state,d,destrel);
    else if (rsrclevel == CP_VALUE && rdestlevel == CP_TOP)
      ProcStateSetTag(state,d,srcrel);
    else
      ProcStateSetTagBot(state,d);

    /* compute the value */
    if (destlevel == CP_VALUE && srclevel == CP_VALUE)
    {
      t_uint32 finalval = ComputeFinalRegValue(orig,d,I386_OP_REGMODE(dest),
	                                       ((t_uint32)destval)^((t_uint32)srcval));
      t_register_content dummy;
      dummy.i=AddressNew32(finalval);
      ProcStateSetReg(state,d,dummy);
    }
    else
      ProcStateSetRegBot(state,d);
  }

  /* set condition flags */
  ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
  ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
  if (srclevel == CP_VALUE && destlevel == CP_VALUE)
  {
    t_uint32 result = (t_uint32)((unsigned long long)destval)^((unsigned long long)srcval);
    int nbits = GetBitsForOp(dest);
    ProcStateSetCond(state,I386_CONDREG_ZF,sign_unextend(result,nbits-1) == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(result,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(result));
  }

  ProcStateFree(orig);
} /* }}} */

/* {{{ EmulTest */
static void EmulTest(t_i386_ins * ins, t_procstate * state)
{
  t_uint32 val1, val2;
  t_lattice_level level1, rlevel1, level2, rlevel2;
  t_reloc *rel1, *rel2;
  t_i386_operand *src1 = I386_INS_SOURCE1(ins), *src2 = I386_INS_SOURCE2(ins);

  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
  GetConstValueForOp(ins,src1,state,TRUE,&level1,&val1,&rlevel1,&rel1);
  GetConstValueForOp(ins,src2,state,TRUE,&level2,&val2,&rlevel2,&rel2);

  if (level1 == CP_VALUE && level2 == CP_VALUE)
  {
    t_uint32 result = ((unsigned)val1) & ((unsigned)val2);
    int nbits = GetBitsForOp(src1);
    
    ProcStateSetCond(state,I386_CONDREG_ZF,result == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(result,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(result));
  }
  ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
  ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
} /* }}} */

/* {{{ EmulOr */
static void EmulOr(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_i386_operand *src = I386_INS_SOURCE1(ins);

  t_procstate * orig = ProcStateNew(&i386_description);
  t_lattice_level destlevel, rdestlevel, srclevel, rsrclevel;
  t_reloc *srcrel, *destrel;
  t_uint32 srcval, destval;

  ProcStateDup(orig,state,&i386_description);
  ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));

  if (INS_DEFARG(T_INS(ins)) != -1)
    FATAL(("implement argument definition"));

  
  GetConstValueForOp(ins,dest,orig,TRUE,&destlevel,&destval,&rdestlevel,&destrel);
  GetConstValueForOp(ins,src,orig,TRUE,&srclevel,&srcval,&rsrclevel,&srcrel);

  if (I386_OP_TYPE(dest) == i386_optype_reg)
  {
    t_reg d = I386_OP_BASE(dest);

    /* {{{ special case: or $-1, %reg */
    if (srclevel == CP_VALUE && srcval == 0xffffffffU && I386_OP_REGMODE(dest) == i386_regmode_full32)
    {
      t_register_content rc;
      ProcStateSetTagTop(state,d);
      rc.i=AddressNew32(0xffffffff);
      ProcStateSetReg(state,d,rc);
      ProcStateSetCond(state,I386_CONDREG_ZF,FALSE);
      ProcStateSetCond(state,I386_CONDREG_SF,TRUE);
      ProcStateSetCond(state,I386_CONDREG_PF,TRUE);
      ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
      ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
      ProcStateFree(orig);
      return;
    } /* }}} */

    /* compute the relocation */
    if (rdestlevel == CP_TOP && rsrclevel == CP_TOP)
      ProcStateSetTagTop(state,d);
    else if (rdestlevel == CP_VALUE && rsrclevel == CP_TOP)
      ProcStateSetTag(state,d,destrel);
    else if (rsrclevel == CP_VALUE && rdestlevel == CP_TOP)
      ProcStateSetTag(state,d,srcrel);
    else
      ProcStateSetTagBot(state,d);

    /* compute the value */
    if (destlevel == CP_VALUE && srclevel == CP_VALUE)
    {
      t_uint32 finalval = ComputeFinalRegValue(orig,d,I386_OP_REGMODE(dest),
	                                       ((t_uint32)destval)|((t_uint32)srcval));
      t_register_content dummy;
      dummy.i=AddressNew32(finalval);
      ProcStateSetReg(state,d,dummy);
    }
    else
      ProcStateSetRegBot(state,d);
  }

  /* set condition flags */
  ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
  ProcStateSetCond(state,I386_CONDREG_OF,FALSE);
  if (srclevel == CP_VALUE && destlevel == CP_VALUE)
  {
    t_uint32 result = ((t_uint32)destval)|((t_uint32)srcval);
    int nbits = GetBitsForOp(dest);
    ProcStateSetCond(state,I386_CONDREG_ZF,sign_unextend(result,nbits-1) == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(result,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(result));
  }

  ProcStateFree(orig);

} /* }}} */

/* {{{ EmulLea */
static void EmulLea(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_i386_operand *src = I386_INS_SOURCE1(ins);
  t_lattice_level vlevel, rlevel;
  t_uint32 val;
  t_reloc *rel;

  GetConstValueForOp(ins,src,state,FALSE,&vlevel,&val,&rlevel,&rel);
  if (vlevel == CP_TOP)
    ProcStateSetRegTop(state,I386_OP_BASE(dest));
  else if (vlevel == CP_BOT)
    ProcStateSetRegBot(state,I386_OP_BASE(dest));
  else
  {
    t_register_content rc;
    rc.i=AddressNew32(val);
    ProcStateSetReg(state,I386_OP_BASE(dest),rc);
  }
  if (rlevel == CP_TOP)
    ProcStateSetTagTop(state,I386_OP_BASE(dest));
  else if (rlevel == CP_BOT)
    ProcStateSetTagBot(state,I386_OP_BASE(dest));
  else
    ProcStateSetTag(state,I386_OP_BASE(dest),rel);
} /* }}} */

/* {{{ EmulPush */
static void EmulPush(t_i386_ins *ins, t_procstate *state)
{
  t_lattice_level vlevel, rlevel;
  t_uint32 val; t_reloc * rel;
  t_i386_operand *src = I386_INS_SOURCE1(ins);
  long argno = INS_DEFARG(T_INS(ins));
  t_bool change = FALSE;

  /* a push that is not an argument definition has no effect on 
   * the procstate or the argstate */
  if (argno == -1) return;

  GetConstValueForOp(ins,src,state,FALSE,&vlevel,&val,&rlevel,&rel);
  change = ArgStateSetArg(CFG_EDGE_ARGS(INS_DEFEDGE(T_INS(ins))), argno,
      vlevel, AddressNew32(val), rlevel, rel);
  if (change)
    CFG_EDGE_SET_ARGS_CHANGED(INS_DEFEDGE(T_INS(ins)), TRUE);
} /* }}} */

/* {{{ EmulSet */
static void EmulSet(t_i386_ins * ins, t_procstate * state)
{
  t_tristate exec = I386ConditionHolds(I386_INS_CONDITION(ins), state);
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_register_content rc;
  t_uint32 val;

  if (I386_OP_TYPE(dest) != i386_optype_reg ||
      ProcStateGetReg(state, I386_OP_BASE(dest), &rc) != CP_VALUE ||
      exec == PERHAPS)
  {
    ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
    if (I386_OP_TYPE(dest) == i386_optype_reg)
      ProcStateSetTagTop(state, I386_OP_BASE(dest));
    return;
  }
  
  if (exec == YES)
    val = ComputeFinalRegValue(state, I386_OP_BASE(dest), I386_OP_REGMODE(dest), 1);
  else
    val = ComputeFinalRegValue(state, I386_OP_BASE(dest), I386_OP_REGMODE(dest), 0);

  rc.i = AddressNew32(val);
  ProcStateSetReg(state, I386_OP_BASE(dest), rc);
  ProcStateSetTagTop(state, I386_OP_BASE(dest));
} /* }}} */

/* {{{ EmulCdq */
static void EmulCdq(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand *src = I386_INS_SOURCE1(ins);
  t_register_content rc;
  t_uint32 val;
  
  ProcStateSetTagTop(state, I386_REG_EDX);

  if (I386_OP_REGMODE(src) == i386_regmode_lo16)
  {
    /* not worth the effort */
    ProcStateSetRegBot(state, I386_REG_EDX);
    return;
  }

  if (ProcStateGetReg(state, I386_REG_EAX, &rc) != CP_VALUE)
  {
    ProcStateSetRegBot(state, I386_REG_EDX);
    return;
  }

  val = G_T_UINT32(rc.i);
  if (val & 0x80000000)
    rc.i = AddressNew32(0xffffffff);
  else
    rc.i = AddressNew32(0);
  ProcStateSetReg(state, I386_REG_EDX, rc);
} /* }}} */

/* {{{ EmulMovzx */
static void EmulMovzx(t_i386_ins * ins, t_procstate * state)
{
  t_i386_operand *src = I386_INS_SOURCE1(ins);
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_register_content rc;
  t_uint32 val;
  t_reloc *rel;
  t_lattice_level vlevel, rlevel;
  
  if (I386_OP_REGMODE(dest) == i386_regmode_lo16)
  {
    /* not worth the effort */
    ProcStateSetRegBot(state, I386_OP_BASE(dest));
    ProcStateSetTagBot(state, I386_OP_BASE(dest));
    return;
  }

  GetConstValueForOp(ins, src, state, FALSE, &vlevel, &val, &rlevel, &rel);

  if (vlevel != CP_VALUE)
  {
    ProcStateSetRegBot(state, I386_OP_BASE(dest));
  }
  else
  {
    int nbits;
    if (I386_OP_TYPE(src) == i386_optype_reg)
      nbits = I386_OP_REGMODE(src) == i386_regmode_lo16 ? 16 : 8;
    else
      nbits = I386_OP_MEMOPSIZE(src) == 2 ? 16 : 8;

    val &= ((1U << nbits) - 1);
    rc.i = AddressNew32(val);
    ProcStateSetReg(state, I386_OP_BASE(dest), rc);
  }

  /* sign-extended value inherits tag from original value */
  /* TODO: shouldn't this always be TAG_TOP? */
  if (rlevel == CP_TOP)
    ProcStateSetTagTop(state, I386_OP_BASE(dest));
  else if (rlevel == CP_BOT)
    ProcStateSetTagBot(state, I386_OP_BASE(dest));
  else
    ProcStateSetTag(state, I386_OP_BASE(dest), rel);

} /* }}} */

/* {{{ EmulShiftRight */
static void EmulShiftRight(t_i386_ins *ins, t_procstate *state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_i386_operand *src = I386_INS_SOURCE1(ins);

  t_uint32 val, val2;
  t_reloc *rel, *rel2;
  t_lattice_level vlevel, rlevel, vlevel2, rlevel2;

  t_uint32 shiftmask;
  t_uint32 newval;
  t_bool arith = I386_INS_OPCODE(ins) == I386_SAR;
  t_register_content rc;

  GetConstValueForOp(ins, dest, state, FALSE, &vlevel, &val, &rlevel, &rel);
  GetConstValueForOp(ins, src, state, FALSE, &vlevel2, &val2, &rlevel2, &rel2);

  /* emulate only the simplest case */
  if (vlevel != CP_VALUE || vlevel2 != CP_VALUE ||
      I386_OP_TYPE(dest) != i386_optype_reg ||
      I386_OP_REGMODE(dest) != i386_regmode_full32)
  {
    ProcStateSetAllBot(state, I386_INS_REGS_DEF(ins));
    return;
  }

  if (arith && (vlevel & 0x80000000))
    shiftmask = 0xffffffff;
  else
    shiftmask = 0;

  val2 &= 31;
  newval = (val >> val2) | (shiftmask & ~((1U << (32-val2)) - 1));
  rc.i = AddressNew32(newval);
  ProcStateSetReg(state, I386_OP_BASE(dest), rc);

  /* condition bits */
  /* SF, ZF, PF according to result
   * OF: something complicated (we always set to BOT)
   * AF: undefined
   * CF: last shifted-out bit
   * flags are unaffected if the count is 0 */
  if (val2 > 0)
  {
    int nbits = GetBitsForOp(I386_INS_DEST(ins));
    t_int32 sval = (t_int32) newval;

    ProcStateSetCond(state,I386_CONDREG_ZF,sval == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(sval,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(sval));
    if (val & (1U << (val2 - 1)))
      ProcStateSetCond(state,I386_CONDREG_CF,TRUE);
    else
      ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
    ProcStateSetCondBot(state,I386_CONDREG_AF);
    ProcStateSetCondBot(state,I386_CONDREG_OF);
  }
} /* }}} */

/* {{{ EmulShiftLeft */
static void EmulShiftLeft(t_i386_ins *ins, t_procstate *state)
{
  t_i386_operand *dest = I386_INS_DEST(ins);
  t_i386_operand *src = I386_INS_SOURCE1(ins);

  t_uint32 val, val2;
  t_reloc *rel, *rel2;
  t_lattice_level vlevel, rlevel, vlevel2, rlevel2;

  t_uint32 newval;
  t_register_content rc;

  GetConstValueForOp(ins, dest, state, FALSE, &vlevel, &val, &rlevel, &rel);
  GetConstValueForOp(ins, src, state, FALSE, &vlevel2, &val2, &rlevel2, &rel2);

  /* emulate only the simplest case */
  if (vlevel != CP_VALUE || vlevel2 != CP_VALUE ||
      I386_OP_TYPE(dest) != i386_optype_reg ||
      I386_OP_REGMODE(dest) != i386_regmode_full32)
  {
    ProcStateSetAllBot(state, I386_INS_REGS_DEF(ins));
    return;
  }

  val2 &= 31;
  newval = val << val2;
  rc.i = AddressNew32(newval);
  ProcStateSetReg(state, I386_OP_BASE(dest), rc);

  /* condition bits */
  /* SF, ZF, PF according to result
   * OF: something complicated (we always set to BOT)
   * AF: undefined
   * CF: last shifted-out bit
   * flags are unaffected if the count is 0 */
  if (val2 > 0)
  {
    int nbits = GetBitsForOp(I386_INS_DEST(ins));
    t_int32 sval = (t_int32) newval;

    ProcStateSetCond(state,I386_CONDREG_ZF,sval == 0);
    ProcStateSetCond(state,I386_CONDREG_SF,get_bit(sval,nbits-1));
    ProcStateSetCond(state,I386_CONDREG_PF,parity(sval));
    if (val & (1U << (32 - val2)))
      ProcStateSetCond(state,I386_CONDREG_CF,TRUE);
    else
      ProcStateSetCond(state,I386_CONDREG_CF,FALSE);
    ProcStateSetCondBot(state,I386_CONDREG_AF);
    ProcStateSetCondBot(state,I386_CONDREG_OF);
  }
} /* }}} */

void I386InstructionEmulator(t_i386_ins * ins, t_procstate * state, t_bool update_known_values)
{
  t_tristate execed = I386ConditionHolds(I386_INS_CONDITION(ins),state);
  /*static int count = 0;*/

  if (I386_INS_OPCODE(ins) == I386_SETcc)
  {
    /* SETcc is always executed, the condition determines only the result */
    I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_EXECED);
  }
  else
  {
    if (execed != YES)
      I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) & (~IF_ALWAYS_EXECED));
    if (execed != NO)
      I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_EXECED);
    else
      return;
  }

#if 0
  if (!(I386_INS_ATTRIB(ins) & IF_DEBUG_MARK))
  {
    if (count++ < diablosupport_options.debugcounter)
    {
      I386_INS_SET_ATTRIB(ins, I386_INS_ATTRIB(ins) | IF_DEBUG_MARK);
      VERBOSE(0,("Will emulate @I", ins));
    }
    else
    {
      ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
      return;
    }
  }
#endif

  switch (I386_INS_OPCODE(ins))
  {
      
    case I386_AND:
      EmulAnd(ins,state);
      break;

    case I386_ADD:
      EmulAdd(ins,state);
      break;

    case I386_MOV:
      EmulMov(ins,state);
      break;

    case I386_SUB:
      EmulSub(ins,state);
      break;

    case I386_CMP:
      EmulCmp(ins,state);
      break;

    case I386_XOR:
      EmulXor(ins,state);
      break;

    case I386_TEST:
      EmulTest(ins,state);
      break;

    case I386_OR:
      EmulOr(ins,state);
      break;

    case I386_LEA:
      EmulLea(ins,state);
      break;

    case I386_PUSH:	/* emulation only makes sense for argument forwarding */
      EmulPush(ins,state);
      break;

    case I386_SETcc:
      EmulSet(ins,state);
      break;

    case I386_CDQ:
      EmulCdq(ins, state);
      break;

    case I386_MOVZX:
      EmulMovzx(ins, state);
      break;

    case I386_SAR:
    case I386_SHR:
      EmulShiftRight(ins, state);
      break;

    case I386_SHL:
      EmulShiftLeft(ins, state);
      break;
      
    default:
#if 0
      {
	t_regset intregs = CFG_DESCRIPTION(BBL_CFG(I386_INS_BBL(ins)))->int_registers;
	t_reg r;
	t_bool all_value = TRUE;
	RegsetSetIntersect(intregs,I386_INS_REGS_USE(ins));
	REGSET_FOREACH_REG(intregs,r)
	{
	  t_register_content dummy;
	  if (ProcStateGetReg(state,r,&dummy) != CP_VALUE)
	    all_value = FALSE;
	}
	if (all_value && !RegsetIsEmpty(intregs))
	  VERBOSE(0,("IMPLEMENT @I\n",ins));
	if (INS_USEARG(ins) != -1)
	{
	  long argno = INS_USEARG(ins);
	  if (current_argstate &&
	      current_argstate->vlevel[argno] == CP_VALUE)
	    VERBOSE(0,("IMPLEMENT ARG @I", ins));
	}
      }
#endif
      
      if (INS_DEFARG(T_INS(ins)) != -1)
      {
	t_ins *gins = T_INS(ins);
	long argno = INS_DEFARG(gins);
	t_cfg_edge *edge = INS_DEFEDGE(gins);
	t_bool change = ArgStateSetArg(CFG_EDGE_ARGS(edge), argno,
	    CP_BOT, AddressNullForIns(gins), CP_BOT, NULL);
	if (change)
	  CFG_EDGE_SET_ARGS_CHANGED(edge, TRUE);
      }
      ProcStateSetAllBot(state,I386_INS_REGS_DEF(ins));
  }
}







/******************************************************
 * ARGUMENT FORWARDING CODE                           *
 ******************************************************/

FUNCTION_DYNAMIC_MEMBER(delta,DELTA,Delta,long,0);
BBL_DYNAMIC_MEMBER(copies,COPIES,Copies,t_uint32,0);
BBL_DYNAMIC_MEMBER(escape_point,ESCAPE_POINT,EscapePoint,t_ins *,NULL);
BBL_DYNAMIC_MEMBER(has_escaped,HAS_ESCAPED,HasEscaped,t_bool,FALSE);

#define SYMBOLIC_STACK_POINTER (t_reloc*)0x1
#define FUNCTION_DELTA_BOT	((long) 0xffffff00)

static t_bool FunctionHasIncomingCalls(t_function *fun)
{
  t_cfg_edge *edge;
  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun), edge)
    if (CFG_EDGE_CAT(edge) == ET_CALL)
      return TRUE;
  return FALSE;
}

/* {{{ CfgGetFunctionStackDeltas
 * On the i386, even function preserving the stack height may actually
 * change it by using a "ret $imm" instruction. This form is used to 
 * pop implicit function arguments off the stack. The stack delta in this
 * case equals $imm. NOTE: this function does not compute the stack delta
 * for functions that do not "preserve" stack height! */
static void CfgGetFunctionStackDeltas(t_cfg *cfg)
{
  t_function *fun;

  CFG_FOREACH_FUN(cfg, fun)
  {
    t_bbl *bbl;
    t_cfg_edge *edge;
    t_bool found_ret = 0;

    bbl = FunctionGetExitBlock(fun);
    if (!bbl) continue;

    BBL_FOREACH_PRED_EDGE(bbl, edge)
    {
      t_bbl *head;
      t_i386_ins *ins;

      if (CfgEdgeIsInterproc(edge)) continue;
      head = CFG_EDGE_HEAD(edge);
      ins = T_I386_INS(BBL_INS_LAST(head));
      if (!ins) continue;

      if (I386_INS_OPCODE(ins) == I386_RET)
      {
	int delta = 0;
	if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	{
	  delta = I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins));
	}
	if (found_ret && delta != FUNCTION_DELTA(fun))
	{
	  FUNCTION_SET_DELTA(fun, FUNCTION_DELTA_BOT);
	  break;
	}
	FUNCTION_SET_DELTA(fun, delta);
	found_ret = TRUE;
      }
    }
  }
} /* }}} */

/* {{{ FunctionRestoresStackTrivial */
static t_bool FunctionRestoresStackTrivial(t_function *fun)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_i386_ins *ins;
  t_bool has_enter = FALSE;
  t_bool has_push = FALSE;
  t_bool has_leave = FALSE;
  t_bool has_pop = FALSE;

  bbl = FUNCTION_BBL_FIRST(fun);
  if (!bbl) return FALSE;

  /* look for either
   * 	enter
   * or
   * 	push %ebp
   * 	mov %esp, %ebp
   * in the entry block
   * {{{ */
  BBL_FOREACH_I386_INS(bbl, ins)
  {
    if (I386_INS_OPCODE(ins) == I386_ENTER)
    {
      has_enter = TRUE;
      break;
    }

    if (I386_INS_OPCODE(ins) == I386_PUSH &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP)
    {
      has_push = TRUE;
      continue;
    }

    if (has_push &&
	I386_INS_OPCODE(ins) == I386_MOV &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP &&
	I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP)
    {
      has_enter = TRUE;
      break;
    }

    if (RegsetIn(I386_INS_REGS_DEF(ins), I386_REG_ESP))
      break;
  } /* }}} */
  if (!has_enter) return FALSE;

  /* look for either
   * 	leave
   * or
   * 	mov %ebp, %esp
   * 	pop %ebp
   * in the return blocks
   * {{{ */
  bbl = FunctionGetExitBlock(fun);
  if (!bbl) return FALSE;
  BBL_FOREACH_PRED_EDGE(bbl, edge)
  {
    t_bbl *head;
    has_leave = FALSE;
    if (CfgEdgeIsInterproc(edge)) continue;
    head = CFG_EDGE_HEAD(edge);
    BBL_FOREACH_I386_INS_R(head, ins)
    {
      if (I386_INS_OPCODE(ins) == I386_RET ||
	  I386_INS_OPCODE(ins) == I386_RETF)
	continue;
      if (I386_INS_OPCODE(ins) == I386_LEAVE)
      {
	has_leave = TRUE;
	break;
      }

      if (I386_INS_OPCODE(ins) == I386_POP &&
	  I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	  I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP)
      {
	has_pop = TRUE;
	continue;
      }

      if (has_pop &&
	  I386_INS_OPCODE(ins) == I386_MOV &&
	  I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	  I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP &&
	  I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	  I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP)
      {
	has_leave = TRUE;
	break;
      }

      if (RegsetIn(I386_INS_REGS_DEF(ins), I386_REG_ESP))
      {
	has_leave = FALSE;
	break;
      }
    }
    if (!has_leave)
      return FALSE;
  } /* }}} */

  if (!has_leave) /* if the exit block had no intraproc incoming edges */
    return FALSE;
  return TRUE;
} /* }}} */

/* {{{ FunctionRestoresStackNontrivial */
static t_bool FunctionRestoresStackNontrivial(t_function *fun, t_bool keep_eqs)
{
  t_cfg * cfg=FUNCTION_CFG(fun);
  t_bbl * entry_bbl;
  t_bbl * ibbl;
  t_cfg_edge * iedge;
  t_i386_ins * i_ins;
  t_equations eqs;
  t_equations tmp;
  t_int32 dummy=0;
  t_bool restores_stack = TRUE;
  t_int32 diff;
  t_regset invalid_regs = RegsetIntersect(
      CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_may_change,
      CFG_DESCRIPTION(FUNCTION_CFG(fun))->int_registers);
  /* hack needed to make this analysis useful: suppose no functions change
   * the stack pointer of the called function */
  RegsetSetSubReg(invalid_regs, I386_REG_ESP);  

  /*{{{ Initialize*/
  FunctionUnmarkAllBbls(fun);

  eqs = EquationsNew(cfg);
  tmp = EquationsNew(cfg);
  FUNCTION_FOREACH_BBL(fun, ibbl)
  {
    BBL_SET_EQS_IN(ibbl,  EquationsNew(cfg));
	 if (BBL_PRED_FIRST(ibbl)==NULL && ibbl == FUNCTION_BBL_LAST(fun))
		EquationsSetAllBot(cfg, BBL_EQS_IN(ibbl));
    else
		EquationsSetAllTop(cfg, BBL_EQS_IN(ibbl));
  }

  entry_bbl = FUNCTION_BBL_FIRST(fun);
  EquationsSetAllBot(cfg, BBL_EQS_IN(entry_bbl));

  /* We add an equation which initializes the stackpointer to some identifiable
   * value */
  EquationsAdd(cfg, BBL_EQS_IN(entry_bbl),
      I386_REG_ESP, CFG_DESCRIPTION(cfg)->num_int_regs, 0,
      SYMBOLIC_STACK_POINTER, NULL);
  FunctionMarkBbl(fun, entry_bbl);
  /*}}}*/

  /* Now propagate the affine equations through the function, fixpoint stuff
   * {{{*/
  while (FunctionUnmarkBbl(fun, &ibbl))
  {
    EquationsCopy(cfg, BBL_EQS_IN(ibbl), eqs);

    BBL_FOREACH_I386_INS(ibbl, i_ins)
    {
      /* do not emulate calls because they substract 4 from the stack pointer,
       * and in strictly intraprocedural propagation there is no ret to restore
       * the stack pointer properly.
       * The same goes for returns actually */
      if (I386_INS_OPCODE(i_ins) != I386_CALL &&
	  I386_INS_OPCODE(i_ins) != I386_RET)
	I386CopyInstructionPropagator(i_ins, eqs, FALSE);

      if (EquationsRegsDifferWithTagAllowed(cfg, eqs, I386_REG_ESP,
	    CFG_DESCRIPTION(cfg)->num_int_regs, &dummy, SYMBOLIC_STACK_POINTER)
	  != YES)
      {
	restores_stack = FALSE;
	VERBOSE(20, ("losing stack pointer at @I", i_ins));
	goto cleanup;
      }
    }

    /*{{{ Now propagate over the edges */
    if (BBL_INS_LAST(ibbl) &&
	I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(ibbl))) == I386_UD2)
    {
      /* do nothing: this is for bugs in the kernel, this path
       * should never be taken anyway */
      continue;
    }

    BBL_FOREACH_SUCC_EDGE(ibbl, iedge)
    {
      if (!CfgEdgeIsInterproc(iedge))
      {
	if (EquationsJoin(cfg, BBL_EQS_IN(CFG_EDGE_TAIL(iedge)), eqs))
	  FunctionMarkBbl(fun, CFG_EDGE_TAIL(iedge));
      }
      else if (CfgEdgeIsForwardInterproc(iedge))
      {
	if (CFG_EDGE_CORR(iedge))
	{
	  if (FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(iedge)))
	      & FF_PRESERVE_STACKHEIGHT)
	  {
	    t_reg tmp_reg;
	    t_bbl *tail = CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge));

	    if (BBL_FUNCTION(tail) != fun)
	      FATAL(("Interprocedural!"));

	    EquationsCopy(cfg, eqs, tmp);
	    REGSET_FOREACH_REG(invalid_regs, tmp_reg)
	      EquationsInvalidate(cfg, tmp, tmp_reg);

	    EquationsAdd(cfg, tmp,
		I386_REG_ESP, I386_REG_ESP,
		-FUNCTION_DELTA(BBL_FUNCTION(CFG_EDGE_TAIL(iedge))),
		NULL, NULL);

	    if (EquationsJoin(cfg, BBL_EQS_IN(tail), tmp))
	      FunctionMarkBbl(fun, tail);
	  }
	  else
	  {
	    /* stack behaviour becomes untraceable. stop propagation */
	    VERBOSE(20, ("callee not well behaved @E", iedge));
	    restores_stack = FALSE;
	    goto cleanup;
	  }
	}
	else
	{
	  /* escaping edge without compensating: apparently the call is not
	   * returning, so there's no need to propagate further over this
	   * path */
	}
      }
    }
    /*}}}*/
    
  }/*}}}*/

  ibbl = FunctionGetExitBlock(fun);
  if (!ibbl)
  {
    restores_stack = FALSE;
    goto cleanup;
  }
  
  if (YES == EquationsRegsDifferWithTagAllowed(cfg, BBL_EQS_IN(ibbl),
	I386_REG_ESP, CFG_DESCRIPTION(cfg)->num_int_regs, &diff,
	SYMBOLIC_STACK_POINTER))
  {
    if (diff)
    {
      restores_stack = FALSE;
      VERBOSE(20, ("difference at exit block: %d", diff));
      goto cleanup;
    }
  }
  else	
  {
    restores_stack = FALSE;
    VERBOSE(20, ("lost it at exit block"));
    goto cleanup;
  }

cleanup:
  if (!keep_eqs)
  {
    FUNCTION_FOREACH_BBL(fun, ibbl)
    {
      EquationsFree(BBL_EQS_IN(ibbl));
      BBL_SET_EQS_IN(ibbl,  NULL);
    }
  }
  else
  {
    /* look for blocks with top in eqs_in. Make all bot for these blocks */
    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl)
    {
      int i;
      t_equations eqs = BBL_EQS_IN(bbl);
      for(i = 0; i < (CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2; i++)
	if (EquationIsTop(eqs[i]))
	{
	  EquationsSetAllBot(cfg, eqs);
	  break;
	}
    }
  }

  EquationsFree(eqs);
  EquationsFree(tmp);
    
  return restores_stack;
} /* }}} */

/* {{{ find functions that do not change the caller's stack height upon call */
static void CfgMarkWellbehavedFunctions(t_cfg *cfg)
{
  t_function *fun;
  t_bool change;

  //TODO move this to a more suitable place and add a fini call as well
  CfgGetFunctionStackDeltas(cfg);

  /* initialize */
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (((FUNCTION_FLAGS(fun) & FF_IS_EXPORTED) ||
	FunctionRestoresStackTrivial(fun) ||
	FUNCTION_CALL_HELL_TYPE(fun))
	&& FUNCTION_DELTA(fun) != FUNCTION_DELTA_BOT)
    {
      VERBOSE(20,("trivially well behaved %s", FUNCTION_NAME(fun)));
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_PRESERVE_STACKHEIGHT);
    }
    else
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & ~FF_PRESERVE_STACKHEIGHT);
  }
 
  /* iteratively find additional well-behaved functions */
  change = TRUE;
  while (change)
  {
    change = FALSE;
    CFG_FOREACH_FUN(cfg, fun)
    {
      if (FUNCTION_FLAGS(fun) & FF_PRESERVE_STACKHEIGHT)
	continue;
      /* these functions can never be well-behaved */
      if (FUNCTION_DELTA(fun) == FUNCTION_DELTA_BOT)
	continue;
      if (FunctionRestoresStackNontrivial(fun, FALSE))
      {
	change = TRUE;
	FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_PRESERVE_STACKHEIGHT);
      }
    }
  }

#if 0
  /* report on the non-safe functions */
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_FLAGS(fun) & FF_PRESERVE_STACKHEIGHT)
      continue;
    VERBOSE(0,("REMAINING %s (@G)",
	  FUNCTION_NAME(fun), BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun))));
    FunctionRestoresStackNontrivial(fun, FALSE);
  }
#endif
} /* }}} */

/* Analysis that determines where possible pointers to the argument stack
 * slots escape {{{ */

/* {{{ InsGetRegsUsedForComputation -- helper function */
static t_regset InsGetRegsUsedForComputation(t_i386_ins *ins)
{
  /* find out which registers are actually used to compute the produced
   * value(s) */
  t_regset rs = RegsetNew();
  t_i386_operand *op;

  if (I386_INS_HAS_FLAG(ins, I386_IF_DEST_IS_SOURCE))
  {
    op = I386_INS_DEST(ins);
    if (I386_OP_TYPE(op) == i386_optype_reg)
      RegsetSetAddReg(rs, I386_OP_BASE(op));
  }
  op = I386_INS_SOURCE1(ins);
  if (I386_OP_TYPE(op) == i386_optype_reg)
    RegsetSetAddReg(rs, I386_OP_BASE(op));
  op = I386_INS_SOURCE2(ins);
  if (I386_OP_TYPE(op) == i386_optype_reg)
    RegsetSetAddReg(rs, I386_OP_BASE(op));
  /* special case: enter and leave */
  if (I386_INS_OPCODE(ins) == I386_ENTER)
    RegsetSetAddReg(rs, I386_REG_ESP);
  else if (I386_INS_OPCODE(ins) == I386_LEAVE)
    RegsetSetAddReg(rs, I386_REG_EBP);
  /* special case: lea uses a mem operand but does no loading */
  else if (I386_INS_OPCODE(ins) == I386_LEA)
  {
    op = I386_INS_SOURCE1(ins);
    if (I386_OP_BASE(op) != I386_REG_NONE)
      RegsetSetAddReg(rs, I386_OP_BASE(op));
    if (I386_OP_INDEX(op) != I386_REG_NONE)
      RegsetSetAddReg(rs, I386_OP_INDEX(op));
  }

  return rs;
} /* }}} */

/* {{{ InsPropagateCopies */
static t_uint32 InsPropagateCopies(t_ins *gins, t_uint32 in, t_equations eqs)
{
  t_i386_ins *ins = T_I386_INS(gins);
  t_regset rsin, rsout;
  rsin = RegsetNewFromUint32(in);

  if (RegsetIsEmpty(RegsetIntersect(I386_INS_REGS_USE(ins), rsin)))
  {
    rsout = RegsetDiff(rsin, I386_INS_REGS_DEF(ins));
  }
  else if (I386InsIsStore(ins))
  {
    /* either a copy is stored in memory (i.e. it escapes) or something is
     * written to the stack through one of the copies, which means the 
     * argument values are possibly overwritten.
     * Exceptions:
     * 	- call
     * 	- stack pushes that don't push a register from the 'copies' set
     */
    if (I386_INS_OPCODE(ins) == I386_CALL ||
	I386_INS_OPCODE(ins) == I386_PUSHF ||
	(I386_INS_OPCODE(ins) == I386_PUSH && 
	 (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg ||
	  !RegsetIn(rsin, I386_OP_BASE(I386_INS_SOURCE1(ins))))))
    {
      rsout = RegsetDup(rsin);
    }
    else
    {
      /* special case: if we know the stack height, we can determine whether
       * a store to the stack actually writes in the argument slots */
      t_int32 diff;
      t_i386_operand *op = I386InsGetMemStoreOp(ins);
      t_cfg *cfg = BBL_CFG(I386_INS_BBL(ins));

      if (YES == EquationsRegsDifferWithTagAllowed(cfg, eqs,
	    I386_REG_ESP, CFG_DESCRIPTION(cfg)->num_int_regs, &diff,
	    SYMBOLIC_STACK_POINTER) &&
	  op &&
	  I386_OP_INDEX(op) == I386_REG_NONE &&
	  I386_OP_BASE(op) == I386_REG_ESP &&
	  diff - I386_OP_IMMEDIATE(op) > 0)
      {
	/* not writing in the argument slots. we still have to check whether
	 * we are storing a register from the copies set */
	t_regset realuse = InsGetRegsUsedForComputation(ins);
	if (RegsetIsEmpty(RegsetIntersect(rsin, realuse)))
	{
	  rsout = RegsetDup(rsin);
	}
	else
	{
	  BBL_SET_ESCAPE_POINT(I386_INS_BBL(ins), T_INS(ins));
	  return 0;
	}
      }
      else
      {
	BBL_SET_ESCAPE_POINT(I386_INS_BBL(ins), T_INS(ins));
	return 0;
      }
    }
  }
  else
  {
    /* find out which registers are actually used to compute the produced
     * value(s) */
    t_regset rs = InsGetRegsUsedForComputation(ins);
    
    if (RegsetIsEmpty(RegsetIntersect(rs, rsin)))
    {
      /* remove the defined registers from copies */
      rsout = RegsetDiff(rsin, I386_INS_REGS_DEF(ins));
      /* %esp might disappear from the set because of an instruction like
       * 	pop %edx
       * We have to make sure it stays in the set. */
      RegsetSetAddReg(rsout, I386_REG_ESP);
    }
    else
    {
      /* add the defined registers to copies */
      rsout = RegsetUnion(rsin,
	  RegsetIntersect(I386_INS_REGS_DEF(ins),
	    CFG_DESCRIPTION(BBL_CFG(I386_INS_BBL(ins)))->int_registers));
    }
  }
  if (!RegsetIn(rsout, I386_REG_ESP))
  {
    /* apparently the stack pointer was redefined independent of its previous
     * value. This is a weird situation, so we just assume the worst and say
     * the stack pointer has escaped */
    BBL_SET_ESCAPE_POINT(I386_INS_BBL(ins), T_INS(ins));
    return 0;
  }
  return RegsetToUint32(rsout);
} /* }}} */

static t_uint32 InsGetStackCopiesBefore(t_i386_ins *before)
{
  t_bbl *bbl = I386_INS_BBL(before);
  t_cfg *cfg = BBL_CFG(bbl);
  t_i386_ins *ins;
  t_uint32 copies = BBL_COPIES(bbl);
  t_equations eqs;

  if (BBL_HAS_ESCAPED(bbl))
    return (t_uint32) -1;

  eqs = EquationsNew(cfg);
  EquationsCopy(cfg, BBL_EQS_IN(bbl), eqs);

  BBL_FOREACH_I386_INS(bbl, ins)
  {
    if (ins == before)
      break;
    if (ins == T_I386_INS(BBL_ESCAPE_POINT(bbl)))
    {
      copies = (t_uint32) -1;
      break;
    }

    copies = InsPropagateCopies(T_INS(ins), copies, eqs);
    I386CopyInstructionPropagator(ins, eqs, FALSE);
  }

  EquationsFree(eqs);
  return copies;
}

/* {{{ BblPropagateCopies */
static t_uint32 BblPropagateCopies(t_bbl *bbl)
{
  t_uint32 copies = BBL_COPIES(bbl);
  t_equations eqs;
  t_i386_ins *ins;

  if (BBL_HAS_ESCAPED(bbl))
    return 0;

  eqs = EquationsNew(BBL_CFG(bbl));
  EquationsCopy(BBL_CFG(bbl), BBL_EQS_IN(bbl), eqs);
  BBL_FOREACH_I386_INS(bbl, ins)
  {
    copies = InsPropagateCopies(T_INS(ins), copies, eqs);
    I386CopyInstructionPropagator(ins, eqs, FALSE);
    if (BBL_ESCAPE_POINT(bbl) == T_INS(ins))
      break;
    if (copies == 0)
      FATAL(("@I made copies 0", ins));
  }
  EquationsFree(eqs);
  return copies;
} /* }}} */

/* {{{ FunctionStackPointerEscapes */
static void FunctionStackPointerEscapes(t_function *fun)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_bbl *entry = FUNCTION_BBL_FIRST(fun);
  t_bool unsafe_function = FALSE;

  /* only do this analysis for functions with incoming calls.
   * all other functions are considered to be split-off chunks of other
   * functions (thanks to single-entry), and there we cannot expect
   * to know whether there are any copies of the stack pointer on entry */
  BBL_FOREACH_PRED_EDGE(entry, edge)
    if (CFG_EDGE_CAT(edge) == ET_CALL)
      break;
  if (!edge && !(FUNCTION_FLAGS(fun) & FF_IS_EXPORTED))
    unsafe_function = TRUE;
  /* functions created by basic block factoring are also unsafe.
   * They are called like regular functions but they can overwrite
   * the caller's stack frame (as indeed they were once part of it) */
  if (!(FUNCTION_FLAGS(fun) & FF_IS_EXPORTED) &&
      FUNCTION_NAME(fun) &&
      StringPatternMatch("factor-*", FUNCTION_NAME(fun)))
    unsafe_function = TRUE;

  if (unsafe_function)
  {
    FUNCTION_FOREACH_BBL(fun, bbl)
      BBL_SET_HAS_ESCAPED(bbl, TRUE);
    return;
  }

  FunctionUnmarkAllBbls(fun);
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    BBL_SET_ESCAPE_POINT(bbl, NULL);
    BBL_SET_HAS_ESCAPED(bbl, FALSE);
    BBL_SET_COPIES(bbl, 0);
  }
  BBL_SET_COPIES(entry, 1U << I386_REG_ESP);
  FunctionMarkBbl(fun, entry);


  while (FunctionUnmarkBbl(fun, &bbl))
  {
    t_uint32 out = 0;

    if (!BBL_HAS_ESCAPED(bbl))
      out = BblPropagateCopies(bbl);

    BBL_FOREACH_SUCC_EDGE(bbl, edge)
    {
      t_bbl *tail = CFG_EDGE_TAIL(edge);
      if (CfgEdgeIsInterproc(edge))
      {
	t_function *tailfun;

	if (!CfgEdgeIsForwardInterproc(edge) || !CFG_EDGE_CORR(edge))
	  continue;

	tail = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));

	tailfun = BBL_FUNCTION(CFG_EDGE_TAIL(edge));
	if (!(FUNCTION_FLAGS(tailfun) & FF_IS_EXPORTED) &&
	    FUNCTION_NAME(tailfun) &&
	    StringPatternMatch("factor-*", FUNCTION_NAME(tailfun)))
	{
	  /* special case: functions created by bbl factoring.
	   * These can freely write in their caller's stack frame as they were
	   * once part of it. A call to one of these functions is equal to
	   * having your stack pointer escape */
	  BBL_SET_HAS_ESCAPED(tail, TRUE);
	  FunctionMarkBbl(fun, tail);
	}
	
      }

      if (BBL_HAS_ESCAPED(bbl) || BBL_ESCAPE_POINT(bbl) != NULL)
      {
	/* propagate HAS_ESCAPED over all outgoing edges */
	if (!BBL_HAS_ESCAPED(tail))
	{
	  BBL_SET_HAS_ESCAPED(tail, TRUE);
	  FunctionMarkBbl(fun, tail);
	}
      }
      else
      {
	/* propagate copies over all outgoing edges */
	if (!BBL_HAS_ESCAPED(tail) && 
	    (BBL_COPIES(tail) | out) != BBL_COPIES(tail))
	{
	  BBL_SET_COPIES(tail, BBL_COPIES(tail) | out);
	  FunctionMarkBbl(fun, tail);
	}
      }
    }
  }
} /* }}} */


/* }}} */

/* {{{ FunctionFindArgUses */
static void FunctionFindArgUses(t_function *fun)
{
  t_cfg *cfg = FUNCTION_CFG(fun);
  t_bbl *bbl;
  t_equations eqs;

  FUNCTION_SET_NARGS(fun, 0);

  if (!FunctionHasIncomingCalls(fun))
    return;

  eqs = EquationsNew(cfg);

  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    t_ins *ins;
    t_int32 diff;

    if (BBL_HAS_ESCAPED(bbl)) continue;
    if (EquationsRegsDifferWithTagAllowed(cfg, BBL_EQS_IN(bbl), I386_REG_ESP,
	  CFG_DESCRIPTION(cfg)->num_int_regs, &diff, SYMBOLIC_STACK_POINTER)
	!= YES)
      continue;

    EquationsCopy(cfg, BBL_EQS_IN(bbl), eqs);
    BBL_FOREACH_INS(bbl, ins)
    {
      t_i386_ins *iins = T_I386_INS(ins);
      if (ins == BBL_ESCAPE_POINT(bbl))
	break;

      if (I386InsIsLoad(iins))
      {
	t_i386_operand *op = I386InsGetMemLoadOp(iins);
	t_reg base;
	t_int32 offset;

	if (!op)
	  goto no_arg_load;
	base = I386_OP_BASE(op);
	if (base == I386_REG_NONE)
	  goto no_arg_load;
	if (I386_OP_INDEX(op) != I386_REG_NONE)
	  goto no_arg_load;
	if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
	  goto no_arg_load;
	if (I386_OP_MEMOPSIZE(op) != 4)
	  goto no_arg_load;

	if (EquationsRegsDifferWithTagAllowed(cfg, eqs, base,
	      CFG_DESCRIPTION(cfg)->num_int_regs, &diff, SYMBOLIC_STACK_POINTER)
	    != YES)
	  goto no_arg_load;

	offset = ((t_int32) I386_OP_IMMEDIATE(op)) + diff;
	if (offset % 4 != 0)
	  goto no_arg_load;
	if (offset < 0)
	  goto no_arg_load;
	if (offset == 0)
	{
	  VERBOSE(20,("this is odd: @I loads return address", ins));
	  goto no_arg_load;
	}

	INS_SET_USEARG(ins, offset/4 - 1);
	VERBOSE(20, ("USE arg %d: @I", INS_USEARG(ins), ins));
	if (offset/4 > FUNCTION_NARGS(fun))
	  FUNCTION_SET_NARGS(fun, offset/4);
      }
no_arg_load:

      I386CopyInstructionPropagator(iins, eqs, FALSE);
      if (EquationsRegsDifferWithTagAllowed(cfg, eqs, I386_REG_ESP,
	    CFG_DESCRIPTION(cfg)->num_int_regs, &diff, SYMBOLIC_STACK_POINTER)
	  != YES)
	break;
    }
  }

  EquationsFree(eqs);
} /* }}} */

/* {{{ FunctionFindArgDefines */
static void FunctionFindArgDefines(t_function *fun)
{
  t_bbl *bbl;
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    t_cfg_edge *edge;
    t_function *callee;
    t_i386_ins *ins;
    int found = 0;
    int nargs;

    BBL_FOREACH_SUCC_EDGE(bbl, edge)
      if (CFG_EDGE_CAT(edge) == ET_CALL)
	break;
    if (!edge) continue;
    callee = BBL_FUNCTION(CFG_EDGE_TAIL(edge));
    nargs = FUNCTION_NARGS(callee);

    if (nargs == 0)
      continue;

    VERBOSE(20,("call %s, %d arguments. call block: @iB",
	  FUNCTION_NAME(callee), nargs, bbl));
    CFG_EDGE_SET_ARGS(edge, ArgStateNew(nargs));

    found = 0;
    BBL_FOREACH_I386_INS_R(bbl, ins)
    {
      if (I386_INS_OPCODE(ins) == I386_CALL)
	continue;

      if (!I386InsIsStore(ins))
	continue;

      if (I386_INS_OPCODE(ins) != I386_PUSH)
      {
	t_i386_operand *op = I386InsGetMemStoreOp(ins);

	if (op && (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED))
	  continue;
	else if (op &&
	    I386_OP_BASE(op) == I386_REG_ESP &&
	    I386_OP_INDEX(op) == I386_REG_NONE &&
	    I386_OP_IMMEDIATE(op) == 0 &&
	    I386_OP_MEMOPSIZE(op) == 4)
	{
	  /* this defines the current argument on the stack.
	   * While further refinement is possible, for now we just stop 
	   * looking for argument defines after this instruction */
	  INS_SET_DEFARG(T_INS(ins), found++);
	  INS_SET_DEFEDGE(T_INS(ins), edge);
	  break;
	}
	else if (op &&
	    I386_OP_BASE(op) == I386_REG_ESP &&
	    I386_OP_INDEX(op) == I386_REG_NONE &&
	    I386_OP_IMMEDIATE(op) >= 4*(nargs-found))
	{
	  /* writing above the argument slots */
	  continue;
	}
	else if (op &&
	    I386_OP_INDEX(op) == I386_REG_NONE &&
	    !(I386_OP_BASE(op) & InsGetStackCopiesBefore(ins)))
	{
	  /* cannot write in this function's stack frame */
	  continue;
	}

	//TODO: this can be refined by checking for writes relative to %esp.
	//This is not too hard, it just takes some programming effort.
	VERBOSE(20,("Losing it after %d found: @I", found, ins));
	break;
      }

      INS_SET_DEFARG(T_INS(ins), found++);
      INS_SET_DEFEDGE(T_INS(ins), edge);

      if (found == nargs)
	break;
    }
    VERBOSE(20,("found %d/%d arg defs. diff = %d", found, nargs, nargs-found));
  }
} /* }}} */

void I386InitArgumentForwarding(t_cfg *cfg)
{
  t_function *fun;
  t_bbl *bbl;

  FunctionInitDelta(cfg);
  BblInitEscapePoint(cfg);
  BblInitHasEscaped(cfg);
  BblInitCopies(cfg);
  BblInitEqsIn(cfg);

  /* mark functions that preserve the stack height */
  CfgMarkWellbehavedFunctions(cfg);

  /* try to determine stack height at the beginning of each basic block */ 
  CFG_FOREACH_FUN(cfg, fun)
    FunctionRestoresStackNontrivial(fun, TRUE);

  /* Find out where pointers to a function's stack frame possibly escape */
  CFG_FOREACH_FUN(cfg, fun)
    FunctionStackPointerEscapes(fun);

  /* find argument uses and definitions */
  CFG_FOREACH_FUN(cfg, fun)
    FunctionFindArgUses(fun);
  CFG_FOREACH_FUN(cfg,fun)
    FunctionFindArgDefines(fun);

  /* clean up */
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_EQS_IN(bbl))
    {
      EquationsFree(BBL_EQS_IN(bbl));
      BBL_SET_EQS_IN(bbl, NULL);
    }
  }

  FunctionFiniDelta(cfg);
  BblFiniEscapePoint(cfg);
  BblFiniHasEscaped(cfg);
  BblFiniCopies(cfg);
  BblFiniEqsIn(cfg);
}


/* vim: set shiftwidth=2 foldmethod=marker : */
