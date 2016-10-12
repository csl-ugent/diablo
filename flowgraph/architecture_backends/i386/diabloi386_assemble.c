/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>
#include <string.h>

/* {{{ write instruction prefixes */
static t_bool bm_is_variable(t_i386_bytemode bm)
{
  return bm == bm_v ||
    bm == bm_eAX || bm == bm_eBX || bm == bm_eCX || bm == bm_eDX ||
    bm == bm_eSI || bm == bm_eDI || bm == bm_eBP || bm == bm_eSP;
}
static t_bool op_is_16(t_i386_operand *op)
{
  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      return (I386_OP_REGMODE(op) == i386_regmode_lo16);
    case i386_optype_imm:
      return (I386_OP_IMMEDSIZE(op) == 2);
    case i386_optype_mem:
      return (I386_OP_MEMOPSIZE(op) == 2);
    default:
      return FALSE;
  }
}

static t_uint32 WritePrefixes(t_i386_ins * ins, t_uint8 * buf, t_i386_opcode_entry *form)
{
  t_uint32 nprefs = 0;
  t_bool op16;

  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_LOCK))
    buf[nprefs++] = '\xf0';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_REPNZ))
    buf[nprefs++] = '\xf2';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_REP))
    buf[nprefs++] = '\xf3';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_CS_OVERRIDE))
    buf[nprefs++] = '\x2e';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_DS_OVERRIDE))
    buf[nprefs++] = '\x3e';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_ES_OVERRIDE))
    buf[nprefs++] = '\x26';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_FS_OVERRIDE))
    buf[nprefs++] = '\x64';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_GS_OVERRIDE))
    buf[nprefs++] = '\x65';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_SS_OVERRIDE))
    buf[nprefs++] = '\x36';
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_ADDRSIZE_OVERRIDE))
    buf[nprefs++] = '\x67';

  /* the opsize override is determined automatically */
  /*if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_OPSIZE_OVERRIDE))*/
    /*buf[nprefs++] = '\x66';*/
  op16 = FALSE;
  if (bm_is_variable(form->op1bm))
  {
    if (op_is_16(I386_INS_DEST(ins)))
      op16 = TRUE;
  }
  if (bm_is_variable(form->op2bm))
  {
    if (op_is_16(I386_INS_SOURCE1(ins)))
      op16 = TRUE;
  }
  if (bm_is_variable(form->op3bm))
  {
    if (op_is_16(I386_INS_SOURCE2(ins)))
      op16 = TRUE;
  }

  if (op16)
    buf[nprefs++] = '\x66';

  return nprefs;
} /* }}} */

/* {{{ write instruction opcode */
t_uint32 I386WriteOpcode(t_i386_ins * ins, t_i386_opcode_entry * form, t_uint8 * buf, t_uint32 * mod_ret, t_uint32 * opcode_ext_ret, t_uint32 * rm_ret)
{
  enum opcodetype {
    onebyte, twobyte, fpu_reg, fpu_mem,
    grp1a, grp1b, grp1c, grp1d,
    grp2a, grp2b,
    grp3a, grp3b, 
    grp4a, grp4b, grp4c, grp4d,
    grp5a, grp5b,
    grp6a, grp6b,
    grp7a, grp7b,
    grp8a,
    grp15a,
    grp16,
    max_opcodetype
  };
  static const t_i386_opcode_entry * table_limits[][2] = {
    {dis386, dis386+256},
    {dis386_twobyte, dis386_twobyte+256},
    {dis386_fpu_reg[0][0], dis386_fpu_reg[0][0]+512},
    {dis386_fpu_mem[0], dis386_fpu_mem[0]+64},
    {dis386_grps[GRP1a-GRP1a], dis386_grps[GRP1a-GRP1a]+8},
    {dis386_grps[GRP1b-GRP1a], dis386_grps[GRP1b-GRP1a]+8},
    {dis386_grps[GRP1c-GRP1a], dis386_grps[GRP1c-GRP1a]+8},
    {dis386_grps[GRP1d-GRP1a], dis386_grps[GRP1d-GRP1a]+8},
    {dis386_grps[GRP2a-GRP1a], dis386_grps[GRP2a-GRP1a]+8},
    {dis386_grps[GRP2b-GRP1a], dis386_grps[GRP2b-GRP1a]+8},
    {dis386_grps[GRP3a-GRP1a], dis386_grps[GRP3a-GRP1a]+8},
    {dis386_grps[GRP3b-GRP1a], dis386_grps[GRP3b-GRP1a]+8},
    {dis386_grps[GRP4a-GRP1a], dis386_grps[GRP4a-GRP1a]+8},
    {dis386_grps[GRP4b-GRP1a], dis386_grps[GRP4b-GRP1a]+8},
    {dis386_grps[GRP4c-GRP1a], dis386_grps[GRP4c-GRP1a]+8},
    {dis386_grps[GRP4d-GRP1a], dis386_grps[GRP4d-GRP1a]+8},
    {dis386_grps[GRP5a-GRP1a], dis386_grps[GRP5a-GRP1a]+8},
    {dis386_grps[GRP5b-GRP1a], dis386_grps[GRP5b-GRP1a]+8},
    {dis386_grps[GRP6a-GRP1a], dis386_grps[GRP6a-GRP1a]+8},
    {dis386_grps[GRP6b-GRP1a], dis386_grps[GRP6b-GRP1a]+8},
    {dis386_grps[GRP7a-GRP1a], dis386_grps[GRP7a-GRP1a]+8},
    {dis386_grps[GRP7b-GRP1a], dis386_grps[GRP7b-GRP1a]+8},
    {dis386_grps[GRP8a-GRP1a], dis386_grps[GRP8a-GRP1a]+8},
    {dis386_grps[GRP15a-GRP1a], dis386_grps[GRP15a-GRP1a]+8},
    {dis386_grps[GRP16-GRP1a], dis386_grps[GRP16-GRP1a]+8}
  };

  enum opcodetype type;
  for (type = onebyte; type < max_opcodetype; type++)
  {
    if ((form >= table_limits[type][0]) && (form < table_limits[type][1]))
      break;
  }
  ASSERT(type != max_opcodetype, ("Could not find suitable opcode bytes"));

  switch (type)
  {
    case onebyte:
      buf[0] = (t_uint8) (((char *)form - (char *)dis386)/sizeof(t_i386_opcode_entry));
      return 1;
    case twobyte:
      buf[0] = '\x0f';
      buf[1] = (t_uint8) (((char *)form - (char *)dis386_twobyte)/sizeof(t_i386_opcode_entry));
      return 2;
    case fpu_reg:
      {
	t_uint32 offset, div1, div2, modulo;
	offset = ((char *)form - (char *)dis386_fpu_reg)/sizeof(t_i386_opcode_entry);
	div1 = offset >> 6;
	div2 = (offset >> 3) & 7;
	modulo = offset & 7;

	*mod_ret = 3;
	buf[0] = 0xd8 + div1;
	*opcode_ext_ret = div2;
	*rm_ret = modulo;
	return 1;
      }
    case fpu_mem:
      {
	t_uint32 offset, div, modulo;
	offset = ((char *)form - (char *)dis386_fpu_mem)/sizeof(t_i386_opcode_entry);
	div = offset >> 3;
	modulo = offset & 7;
	buf[0] = (t_uint8) 0xd8+div;
	*opcode_ext_ret = modulo;
	return 1;
      }
    case grp1a:
      buf[0] = '\x80';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP1a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp1b:
      buf[0] = '\x81';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP1b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp1c:
      buf[0] = '\x82';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP1c-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp1d:
      buf[0] = '\x83';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP1d-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp2a:
      buf[0] = '\xc0';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP2a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp2b:
      buf[0] = '\xc1';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP2b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp3a:
      buf[0] = '\xc6';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP3a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp3b:
      buf[0] = '\xc7';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP3b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp4a:
      buf[0] = '\xd0';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP4a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp4b:
      buf[0] = '\xd1';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP4b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp4c:
      buf[0] = '\xd2';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP4c-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp4d:
      buf[0] = '\xd3';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP4d-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp5a:
      buf[0] = '\xf6';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP5a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp5b:
      buf[0] = '\xf7';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP5b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp6a:
      buf[0] = '\xfe';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP6a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp6b:
      buf[0] = '\xff';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP6b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 1;
    case grp7a:
      buf[0] = '\x0f';
      buf[1] = '\x00';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP7a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 2;
    case grp7b:
      buf[0] = '\x0f';
      buf[1] = '\x01';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP7b-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 2;
    case grp8a:
      buf[0] = '\x0f';
      buf[1] = '\xba';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP8a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 2;
    case grp15a:
      buf[0] = '\x0f';
      buf[1] = '\xae';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP15a-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 2;
    case grp16:
      buf[0] = '\x0f';
      buf[1] = '\x18';
      *opcode_ext_ret = ((char *)form - (char *)(dis386_grps[GRP16-GRP1a]))/sizeof(t_i386_opcode_entry);
      return 2;
    default:
      /* keep the compiler happy */
      break;
  }
  FATAL(("Should never get here"));
  return 0;
} /* }}} */

/* {{{ Assemble instruction into a specific form */
t_uint32 I386AssembleToSpecificForm(t_i386_ins * ins, t_i386_opcode_entry * form, t_uint8 * buf)
{
  t_uint32 len;
  t_uint32 mod = 0, reg = 0, rm = 0;
  t_uint32 scale = 0, index = 0, base = 0;
  t_uint32 immediate, displacement;
  t_uint32 segsel;
  unsigned int immedsz = 0, dispsz = 0;
  t_bool has_modrm = FALSE, has_sib = FALSE;
  t_bool has_segsel = FALSE;
  t_reloc * disp_reloc = NULL, * imm_reloc = NULL;
  t_i386_operand * op;


  has_modrm = form->has_modrm;

  len = WritePrefixes(ins,buf,form);
  len += I386WriteOpcode(ins, form, buf+len, &mod, &reg, &rm);

  /* set disp_reloc and imm_reloc if operands are relocated {{{ */
  /*check in case relocations have not been migrated to instructions yet (e.g. immediately after disassembly)*/
  if(I386_INS_REFERS_TO(ins))
  {
    I386_INS_FOREACH_OP(ins,op)
      if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
      {
	if (I386_OP_TYPE(op) == i386_optype_imm
	    || (I386_OP_TYPE(op) == i386_optype_mem && !has_modrm)
	    || I386_OP_TYPE(op) == i386_optype_farptr)
	  imm_reloc = I386GetRelocForOp(ins,op);
	else if (I386_OP_TYPE(op) == i386_optype_mem && has_modrm)
	  disp_reloc = I386GetRelocForOp(ins,op);
	else
	  FATAL(("@I: optype should not be reloced\n",ins));
      }
  }
  /* }}} */
  
  /* now determine mod/rm and sib byte and displacement and immediate field */
  form->op1as(I386_INS_DEST(ins), form->op1bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel);
  form->op2as(I386_INS_SOURCE1(ins), form->op2bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel);
  form->op3as(I386_INS_SOURCE2(ins), form->op3bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel);

  if (has_modrm)
  {
    t_uint8 modrm = ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7);
    buf[len++] = modrm;
    if (has_sib)
    {
      t_uint8 sib = ((scale & 3) << 6) | ((index & 7) << 3) | (base & 7);
      buf[len++] = sib;
    }
    switch (dispsz)
    {
      case 0:
	break;
      case 1:
	if(disp_reloc != NULL)
	  RELOC_SET_FROM_OFFSET(disp_reloc, AddressNew32(len));
	buf[len++] = displacement & 0xff;
	break;
      case 4:
	if(disp_reloc != NULL)
	  RELOC_SET_FROM_OFFSET(disp_reloc, AddressNew32(len));
	buf[len++] = displacement & 0xff;
	buf[len++] = (displacement & 0xff00) >> 8;
	buf[len++] = (displacement & 0xff0000) >> 16;
	buf[len++] = (displacement & 0xff000000) >> 24;
	break;
      default:
	FATAL(("unexpected displacement size (%d bytes) for @I",dispsz,ins));
    }
  }

  switch (immedsz)
  {
    case 0:
      break;
    case 1:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew32(len));
      buf[len++] = immediate & 0xff;
      break;
    case 2:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew32(len));
      buf[len++] = immediate & 0xff;
      buf[len++] = (immediate & 0xff00) >> 8;
      break;
    case 4:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew32(len));
      buf[len++] = immediate & 0xff;
      buf[len++] = (immediate & 0xff00) >> 8;
      buf[len++] = (immediate & 0xff0000) >> 16;
      buf[len++] = (immediate & 0xff000000) >> 24;
      break;
    default:
      FATAL(("Unexpected immediate size (%d bytes) for @I", immedsz, ins));
  }

  if (has_segsel)
  {
    buf[len++] = segsel & 0xff;
    buf[len++] = (segsel & 0xff00) >> 8;
  }

  return len;
} /* }}} */

/* Returns number of possible encodings, regardless of IMMEDSIZE  {{{*/
t_uint32 I386GetNumberOfEncodings(t_i386_ins * ins, t_i386_opcode_entry * forms[], t_uint32 nfitting)
{
  t_uint32 nchecked = 0;

  if(I386_INS_OPCODE(ins)==I386_LEA && I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins))==4){
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins))=1;
    nfitting+=I386GetNumberOfEncodings(ins, forms, nfitting);
    if(I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins))==0){
      I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins))=0;
      nfitting+=I386GetNumberOfEncodings(ins, forms, nfitting);
    }
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins))=4;
  }

  if (!I386InsIsConditional(ins) || 
      (I386_INS_OPCODE(ins) == I386_JECXZ) || (I386_INS_OPCODE(ins) == I386_LOOP) ||
      (I386_INS_OPCODE(ins) == I386_LOOPZ) || (I386_INS_OPCODE(ins) == I386_LOOPNZ))
  {
    /* {{{ regular instructions */
    t_i386_opcode opcode=I386_INS_OPCODE(ins);
    void * key = (void *) (&opcode);
    t_i386_opcode_he * he = HashTableLookup(i386_opcode_hash, key);
    while (he)
    {
      t_i386_opcode_entry * entry = (t_i386_opcode_entry *) he->entry;
      I386OpCheckFunc op1check, op2check, op3check;
      
      if(entry->op1check == I386OpChecksI)
	op1check=I386OpChecksI2;
      else if(entry->op1check == I386OpCheckI)
	op1check=I386OpCheckI2;
      else{
       	op1check = entry->op1check;
      }
 
      if(entry->op2check == I386OpChecksI){
	op2check=I386OpChecksI2;
      }
      else if(entry->op2check == I386OpCheckI)
	op2check=I386OpCheckI2;
      else op2check = entry->op2check;
      
      if(entry->op3check == I386OpChecksI)
	op3check=I386OpChecksI2;
      else if(entry->op3check == I386OpCheckI)
	op3check=I386OpCheckI2;
      else op3check = entry->op3check;

      nchecked++;
      if (op1check(I386_INS_DEST(ins),entry->op1bm)){
	if(op2check(I386_INS_SOURCE1(ins),entry->op2bm)){
	  if(op3check(I386_INS_SOURCE2(ins),entry->op3bm)){
	    forms[nfitting++] = entry;
	  }
	}
      }
      he = (t_i386_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    /* }}} */
  }
  else
  { 
    /*Conditional instructions {{{*/
    t_i386_opcode_entry * entry = NULL;
    switch (I386_INS_OPCODE(ins))
    {
      case I386_SETcc:
	entry =&(dis386_twobyte[0x90 + I386_INS_CONDITION(ins)]);
	break;
      case I386_CMOVcc:
	entry =&(dis386_twobyte[0x40 + I386_INS_CONDITION(ins)]);
	break;
      case I386_Jcc:
	entry =&(dis386[0x70 + I386_INS_CONDITION(ins)]);
	break;
      case I386_FCMOVcc:
	FATAL(("Currently not supported"));
	break;
      default:
	FATAL(("unexpected opcode: @I",ins));
    }
    if (!(entry->op1check(I386_INS_DEST(ins),entry->op1bm) && 
	  entry->op2check(I386_INS_SOURCE1(ins),entry->op2bm) && 
	  entry->op3check(I386_INS_SOURCE2(ins),entry->op3bm))){
      nfitting=0;
    }
    else 
      nfitting=1;
    /*}}}*/
  }
  return nfitting;
}
/* }}} */

/* {{{ build a list of all possible instruction encodings */
/* expects forms to be an array of at least size 10, returns number of possible encodings */

t_uint32 I386GetPossibleEncodings(t_i386_ins * ins, t_i386_opcode_entry * forms[])
{
  t_uint32 nfitting = 0, nchecked = 0;

  if (!I386InsIsConditional(ins) || 
      (I386_INS_OPCODE(ins) == I386_JECXZ) || (I386_INS_OPCODE(ins) == I386_LOOP) ||
      (I386_INS_OPCODE(ins) == I386_LOOPZ) || (I386_INS_OPCODE(ins) == I386_LOOPNZ))
  {
    /* {{{ regular instructions */
    t_i386_opcode opcode=I386_INS_OPCODE(ins);
    void * key = (void *) &(opcode);

    t_i386_opcode_he * he = HashTableLookup(i386_opcode_hash, key);

    while (he)
    {
      t_i386_opcode_entry * entry = (t_i386_opcode_entry *) he->entry;
      nchecked++;
      if (entry->op1check(I386_INS_DEST(ins),entry->op1bm) && entry->op2check(I386_INS_SOURCE1(ins),entry->op2bm) && entry->op3check(I386_INS_SOURCE2(ins),entry->op3bm))
      {
	forms[nfitting++] = entry;
      }
      he = (t_i386_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    if (nfitting > 10)
      FATAL(("Make the forms array larger!"));
    /* }}} */
  }
  else
  { 
    /* {{{ conditional instructions */
    /* special case: there is not enough assembly information available
     * in the dis386* tables to determine the correct opcode entry for the different
     * condition codes in conditional instructions, so we do the selection here, manually */
    switch (I386_INS_OPCODE(ins))
    {
      case I386_Jcc:
	nchecked = nfitting = 1;
	if (I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins)) == 1)
	  forms[0] = &(dis386[0x70 + I386_INS_CONDITION(ins)]);
	else 
	  forms[0] = &(dis386_twobyte[0x80 + I386_INS_CONDITION(ins)]);
	break;
      case I386_SETcc:
	nchecked = nfitting = 1;
	forms[0] = &(dis386_twobyte[0x90 + I386_INS_CONDITION(ins)]);
	break;
      case I386_CMOVcc:
	nchecked = nfitting = 1;
	forms[0] = &(dis386_twobyte[0x40 + I386_INS_CONDITION(ins)]);
	break;
      case I386_FCMOVcc:
	{
	  t_uint32 regno = I386_OP_BASE(I386_INS_SOURCE1(ins)) - I386_REG_ST0;
	  nchecked = 8;
	  nfitting = 1;
	  switch (I386_INS_CONDITION(ins))
	  {
	    case I386_CONDITION_B:
	      forms[0] = &(dis386_fpu_reg[2][0][regno]);
	      break;
	    case I386_CONDITION_Z:
	      forms[0] = &(dis386_fpu_reg[2][1][regno]);
	      break;
	    case I386_CONDITION_BE:
	      forms[0] = &(dis386_fpu_reg[2][2][regno]);
	      break;
	    case I386_CONDITION_P:
	      forms[0] = &(dis386_fpu_reg[2][3][regno]);
	      break;
	    case I386_CONDITION_AE:
	      forms[0] = &(dis386_fpu_reg[3][0][regno]);
	      break;
	    case I386_CONDITION_NZ:
	      forms[0] = &(dis386_fpu_reg[3][1][regno]);
	      break;
	    case I386_CONDITION_A:
	      forms[0] = &(dis386_fpu_reg[3][2][regno]);
	      break;
	    case I386_CONDITION_NP:
	      forms[0] = &(dis386_fpu_reg[3][3][regno]);
	      break;
	    default:
	      FATAL(("unexpected condition code for fcmovcc instruction"));
	  }
	}
	break;
      default:
	FATAL(("unexpected opcode: @I",ins));
    } 
    if (!(forms[0]->op1check(I386_INS_DEST(ins),forms[0]->op1bm) && 
	  forms[0]->op2check(I386_INS_SOURCE1(ins),forms[0]->op2bm) && 
	  forms[0]->op3check(I386_INS_SOURCE2(ins),forms[0]->op3bm)))
      nfitting = 0;
    /* }}} */
  }
  return nfitting;
}
/* }}} */

/* {{{ assemble an instruction into the shortest form possible */
t_uint32 I386AssembleIns(t_i386_ins * ins, t_uint8 * buf)
{
  if (I386_INS_TYPE(ins) == IT_DATA)
  {
    buf[0] = I386_INS_DATA(ins);
    return 1;
  }
  else
  {
    t_uint32 nfitting = 0;
    t_i386_opcode_entry * forms[10];   /* this is certainly large enough */
    t_uint32 minlen = 10000000, minindex = 0, i;

    /*VERBOSE(0,("assembling @I\n",ins)); */
    nfitting = I386GetPossibleEncodings(ins,forms);

    ASSERT(nfitting != 0, ("found no possible encodings for @I",ins));


    /* forms[] now contains a list of all possible assembly forms for the
     * instruction */
    if (nfitting == 1)
      return I386AssembleToSpecificForm(ins, forms[0], buf);

    for (i = 0; i < nfitting; i++)
    { 
      t_uint32 len = I386AssembleToSpecificForm(ins,forms[i],buf);
      minindex = minlen < len ? minindex : i;
      minlen = minlen < len ? minlen : len;
    }

    return I386AssembleToSpecificForm(ins,forms[minindex],buf);
  }

  /* keep the compiler happy */
  return 0;
} /* }}} */

/* {{{ AssembleSection */
void I386AssembleSection(t_section * sec)
{
  t_uint32 total;
  t_uint32 len;
  t_address address;
  int nins;
  t_i386_ins * ins;

  t_uint8 * data = SECTION_TMP_BUF(sec);
  t_uint8 * resized;

  /* if we've only disassembled and reassembled without flowgraphing, we have
   * to give all instructions their old length back (because otherwise we'd
   * need to relocate all jump offsets etc, which is impossible without a flow
   * graph). however, this is not always possible (the diablo instruction
   * representation doesn't allow an exact description of the instruction
   * encoding), so if necessary we pad the instruction with noops until it
   * reaches the desired length.
   */

  total = 0;
  nins = 0;
  address = SECTION_CADDRESS(sec);
  for (ins = T_I386_INS(SECTION_DATA(sec)); ins; ins = I386_INS_INEXT(ins))
  {
    for (; AddressIsLt(address, I386_INS_CADDRESS(ins)); address = AddressAddUint32(address, 1))
    {
      *data = '\x90';
      nins++;
      data++;
      total++;
    }
    len = I386AssembleIns(ins,data);

    if (len > G_T_UINT32(I386_INS_CSIZE(ins)))
      FATAL(("@I: size corrupt: reports @G should be 0x%x",ins,I386_INS_CSIZE(ins),len));

    nins++;
    data += len;
    total += len;
    address = AddressAddUint32(address, len);
  }

  resized=Malloc(sizeof(t_uint8)*total);
  data = SECTION_TMP_BUF(sec);
  memcpy(resized,data,(size_t) total);

  Free(data);

  VERBOSE(0,("section prev size @G new size 0x%x", SECTION_CSIZE(sec),total));
  SECTION_SET_TMP_BUF(sec, resized);
  SECTION_SET_CSIZE(sec, AddressNew32(total));

  VERBOSE(0,("assembled %d instructions, for a total of 0x%x bytes",nins,total));
} /* }}} */

/* Defined in i386_nasm/i386_nasm.c */
int i386_assembleFromStringNoErr(char * string, char * result);

t_bool I386ParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_i386_ins * at_ins, t_bool before)
{
  FATAL(("Reimplement"));
#if 0
  char tmp_string[15];
  char * a2i_result = i386_a2i(ins_text);

  if(a2i_result)
  {
    t_i386_ins * new = InsNewForBbl(bbl);
    if(at_ins)
    {
      if(before == TRUE)
	InsInsertBefore(new, at_ins);
      else
	InsInsertAfter(new, at_ins);
    }
    else
      InsPrependToBbl(new,bbl);
    
    if(i386_assembleFromStringNoErr(a2i_result,tmp_string) > 0)
    {
      I386DisassembleOne((t_i386_ins *)new,tmp_string,NULL,0,0);
      return TRUE;
    }
    else 
    {
      InsKill(new);
    }
  }

  return FALSE;
#endif
}

/* vim: set shiftwidth=2 foldmethod=marker: */
