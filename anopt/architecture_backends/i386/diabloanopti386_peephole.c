/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopti386.h>
#include <string.h>

/* {{{ stack optimization */
#define STACK_SIZE	20000

#define PushReg(_reg,ins)                  \
  do {                                     \
    top++;                                 \
    if (top == STACK_SIZE)                 \
      goto function_exit;                  \
    stack[top].kind = reg;                 \
    stack[top].u.reg = _reg;               \
    stack[top].refcount = 0;               \
    stack[top].def = ins;                  \
    stack[top].changed = NullRegs;         \
  } while (0)

#define PushImm(_imm,ins)                  \
  do {                                     \
    top++;                                 \
    if (top == STACK_SIZE)                 \
      goto function_exit;                  \
    stack[top].kind = imm;                 \
    stack[top].u.imm = _imm;               \
    stack[top].refcount = 0;               \
    stack[top].def = ins;                  \
    stack[top].changed = NullRegs;         \
  } while (0)

#define PushRel(_rel,ins)                  \
  do {                                     \
    top++;                                 \
    if (top == STACK_SIZE)                 \
      goto function_exit;                  \
    stack[top].kind = rel;                 \
    stack[top].u.rel = _rel;               \
    stack[top].refcount = 0;               \
    stack[top].def = ins;                  \
    stack[top].changed = NullRegs;         \
  } while (0)


#define PushUnk(ins)                       \
  do {                                     \
    top++;                                 \
    if (top == STACK_SIZE)                 \
      goto function_exit;                  \
    stack[top].kind = unk;                 \
    stack[top].refcount = 0;               \
    stack[top].def = ins;                  \
    stack[top].changed = NullRegs;         \
  } while (0)

#define PopCheck()                         \
  do {                                     \
    if (top < 0)                           \
      goto stack_reset;                    \
  } while (0)

typedef struct _t_stack_cont {
  enum {reg,imm,rel,unk} kind;
  union {
    t_reg reg;
    t_uint32 imm;
    t_reloc * rel;
  } u;
  int refcount;
  t_i386_ins * def;
  t_regset changed;
} t_stack_cont;

static t_stack_cont stack[STACK_SIZE];
static int top = -1;

static void RemoveStackSlot(int slot, t_i386_ins * alloc, t_i386_ins * release, t_bool remove_release)
{
  t_i386_ins * ins;

  /* {{{ adjust all stack references between allocation and deletion of the slot */
  for (ins = I386_INS_INEXT(alloc); ins != release; ins = I386_INS_INEXT(ins))
  {
    t_i386_operand *dest = I386_INS_DEST(ins), *src1 = I386_INS_SOURCE1(ins), *src2 = I386_INS_SOURCE2(ins);
    long local_top = (long) I386_INS_TMP(ins);
    int thres = (local_top - slot) * 4;

    /* local stack top is one slot lower after the slot is removed */

    I386_INS_SET_TMP(ins,  (void*) ((long)I386_INS_TMP(ins)-1));

    if (!RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP)) continue;
    
    if (I386_OP_TYPE(dest) == i386_optype_mem && I386_OP_BASE(dest) == I386_REG_ESP)
    {
      if (I386_OP_IMMEDIATE(dest) > thres) 
	I386_OP_IMMEDIATE(dest) -= 4;
      else if (I386_OP_IMMEDIATE(dest) == thres)
	FATAL(("stack propagation algorithm is corrupt"));
    }
    if (I386_OP_TYPE(src1) == i386_optype_mem && I386_OP_BASE(src1) == I386_REG_ESP)
    {
      if (I386_OP_IMMEDIATE(src1) > thres) 
	I386_OP_IMMEDIATE(src1) -= 4;
      else if (I386_OP_IMMEDIATE(src1) == thres)
	FATAL(("stack propagation algorithm is corrupt\n ins @I",ins));
    }
    if (I386_OP_TYPE(src2) == i386_optype_mem && I386_OP_BASE(src2) == I386_REG_ESP)
    {
      if (I386_OP_IMMEDIATE(src2) > thres) 
	I386_OP_IMMEDIATE(src2) -= 4;
      else if (I386_OP_IMMEDIATE(src2) == thres)
	FATAL(("stack propagation algorithm is corrupt"));
    }
  } /* }}} */

  /* {{{ remove allocation of the slot */
  switch (I386_INS_OPCODE(alloc))
  {
    case I386_PUSH:
      I386InsKill(alloc);
      break;
    case I386_SUB:
      I386_OP_IMMEDIATE(I386_INS_SOURCE1(alloc)) -= 4;
      if (I386_OP_IMMEDIATE(I386_INS_SOURCE1(alloc)) == 0)
	I386InsKill(alloc);
      break;
    default:
      FATAL(("unknown stack slot allocation @I",alloc));
  }
  /* }}} */

  /* {{{ remove release of the slot */
  if (remove_release)
  {
    switch (I386_INS_OPCODE(release))
    {
      case I386_ADD:
	I386_OP_IMMEDIATE(I386_INS_SOURCE1(release)) -= 4;
	if (I386_OP_IMMEDIATE(I386_INS_SOURCE1(release)) == 0)
	  I386InstructionMakeNoop(release);	/* do not kill this instruction immediately, it is still referenced in the main loop of PropBlock */
	break;
      case I386_POP:
	I386InstructionMakeNoop(release);		/* do not kill this instruction immediately, it is still referenced in the main loop of PropBlock */
	break;
      default:
	FATAL(("unknown stack slot release @I",alloc));
    }
  }
  /* }}} */
}

static t_bool PropBlock(t_bbl * bbl)
{
  t_bool change = FALSE;
  t_i386_ins * ins, * tmp;
  t_regset changed = NullRegs;
  int i;
  /*static int counter = 0;*/

  top = -1;

  BBL_FOREACH_I386_INS_SAFE(bbl,ins,tmp)
  {
    t_i386_operand * src1 = I386_INS_SOURCE1(ins);
    /*t_i386_operand * src2 = I386_INS_SOURCE2(ins); */
    t_i386_operand * dest = I386_INS_DEST(ins);

    I386_INS_SET_TMP(ins,  (void *) (long) top);

    /* skip instructions that do not read from or write to memory, and that do
     * not use or define the stack pointer */
    if (!RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP) && !RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP) && !I386InsIsLoad(ins) && !I386InsIsStore(ins))
    {
      for (i=0; i<=top; i++) RegsetSetUnion(stack[i].changed,I386_INS_REGS_DEF(ins));
      continue;
    }


    /* 1. loads */
    if (I386InsIsLoad(ins))
    {
      t_i386_operand * op = I386InsGetMemLoadOp(ins);
      if (op)
      {
	/* possible aliasing? */
	if (I386_OP_BASE(op) != I386_REG_ESP && I386_OP_BASE(op) != I386_REG_NONE)
	{
	  int i;
	  for (i=0; i<=top; i++) stack[i].refcount++;
	}
	/* stack-relative? */
	if (I386_OP_BASE(op) == I386_REG_ESP)
	{
	  if (I386_OP_INDEX(op) != I386_REG_NONE)
	  {
	    /* there is an index register, so we can't know for sure which
	     * stack slot we change: increase their refcounts */
	    int i;
	    for (i=0; i<=top; i++) stack[i].refcount++;
	  }
	  else
	  {
	    int slot = top - (I386_OP_IMMEDIATE(op) / 4);
	    int nslots = (I386_OP_MEMOPSIZE(op) + 3) / 4; /* round up, not down */
	    int i;
	    for (i = slot; i >= 0 && i > slot - nslots; i--)
	    {
	      stack[i].refcount++;
	      if (nslots == 1 && 
		  !((I386_INS_OPCODE(ins) == I386_CMP || I386_INS_OPCODE(ins) == I386_TEST ||
                     I386_INS_OPCODE(ins) == I386_MOVSX || I386_INS_OPCODE(ins) == I386_MOVZX)
		    && op == I386_INS_SOURCE1(ins)))
	      {
		/* optimization only possible if slots are read one at a time */
		if (stack[i].kind == imm)
		{
/*                  VERBOSE(0,("@I loads immediate %x\n",ins,stack[i].u.imm));*/
		  I386OpSetImm(op,stack[i].u.imm,4);
		  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
		  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
		  change = TRUE;
		}
		else if (stack[i].kind == rel)
		{
		  t_reloc * rel = stack[i].u.rel;
		  t_reloc * rel2 ;
/*                  VERBOSE(0,("@I loads relocated immediate @R\n",ins,stack[i].u.rel));*/
		  ASSERT(!I386_INS_REFERS_TO(ins),("ins already refers to something!"));
		  I386OpSetImm(op,0,4);
		  I386_OP_FLAGS(op) |= I386_OPFLAG_ISRELOCATED;

		  rel2 = RelocTableDupReloc (RELOC_TABLE(rel), rel);
		  RelocSetFrom(rel2, T_RELOCATABLE(ins));
		  
		  RELOC_SET_LABEL(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)), StringDup(RELOC_LABEL(rel)));
		  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
		  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
		  change = TRUE;
		}
		else if (stack[i].kind == reg && !RegsetIn(changed,stack[i].u.reg))
		{
/*                  VERBOSE(0,( "@I loads unchanged reg %d\n",ins,changed,stack[top-slot].u.reg));*/
		}
	      }
	    }
	  }
	}
      }
    }

    /* 2. stores */
    if (I386InsIsStore(ins))
    {
      t_i386_operand * op = I386InsGetMemStoreOp(ins);
      if (op)
      {
	/* possible aliasing? */
	if (I386_OP_BASE(op) != I386_REG_ESP && I386_OP_BASE(op) != I386_REG_NONE)
	  goto stack_reset;
	/* stack-relative? */
	if (I386_OP_BASE(op) == I386_REG_ESP)
	{
	  if (I386_OP_INDEX(op) != I386_REG_NONE)
	  {
	    /* there is an index register, so we can't know for sure which
	     * stack slot we change: make them all unknown */
	    int i;
	    for (i=0; i<=top; i++) 
	    {
	      stack[i].kind = unk;
	      stack[i].refcount++;
	    }
	  }
	  else
	  {
	    /* TODO: stores van registers en immediate waarden */
	    int slot = top - (I386_OP_IMMEDIATE(op) / 4);
	    int nslots = (I386_OP_MEMOPSIZE(op) + 3) / 4; /* round up, not down */
	    if (I386_INS_OPCODE(ins) == I386_MOV 
	        /* && (counter++ < diablosupport_options.debugcounter) */
		&& nslots == 1 && slot == top && slot >= 0
		&& stack[top].refcount == 0
		&& I386_OP_MEMOPSIZE(op) == 4
		&& I386_OP_IMMEDIATE(op) == 0
		&& (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg 
		  || I386_OP_BASE(I386_INS_SOURCE1(ins)) != I386_REG_ESP)
	       )
	    {
	      /* special case: remove the previous definition and turn this into a push */
	      RemoveStackSlot(top,stack[top].def,ins,FALSE);
	      I386_INS_SET_OPCODE(ins, I386_PUSH);
	      I386_OP_TYPE(I386_INS_DEST(ins)) = i386_optype_none;
	      I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
	      I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
	      change = TRUE;
	      stack[top].refcount = 0;
	      stack[top].kind = unk;
              top--; // the just created push and its location will be added in case 4, so here we need 
                     // to remove one from the removed push or sub 4 (or 4 out of X>4)
	    }
	    else
	    {
	      int i;
	      for (i = slot; i >= 0 && i > slot - nslots; i--)
	      {
		stack[i].kind = unk;
		stack[i].refcount++;
	      }
	    }
	  }
	}
      }
    }

    /* 3. lea instructions. if they reference a stack slot, the refcount of this slot should be incremented */
    if (I386_INS_OPCODE(ins) == I386_LEA)
    {
      if (I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP)
      {
	if (I386_OP_INDEX(I386_INS_SOURCE1(ins)) != I386_REG_NONE)
	{
	  /* we don't know which stack slot is referenced, mark them all */
	  int i;
	  for (i=0; i<=top; i++) stack[i].refcount++;
	}
	else
	{
	  int slot = top - I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins))/4;
	  if (slot >= 0 && slot <= top) stack[slot].refcount++;
	}
      }
      else
      {
	/* possible aliasing: increment the refcount of all stack slots */
	int i;
	for (i=0; i<=top; i++) stack[i].refcount++;
      }
    }
 
    /* !!! do stack manipulation instructions last, to avoid problems with 
     * instructions like
     * 		push 8(%esp)
     * this is both a stack manipulation and a load instruction. if the
     * stack manipulation is processed first, the analysis' logical stack pointer
     * has already changed by the time the effective load address is calculated. */
    /* 4. stack manipulation */
    switch (I386_INS_OPCODE(ins))
    {
      case I386_PUSH:
	switch (I386_OP_TYPE(src1))
	{
	  case i386_optype_reg:
	    if (I386_OP_REGMODE(src1) == i386_regmode_full32)
	      PushReg(I386_OP_BASE(src1),ins);
	    else
	      PushUnk(ins);
	    break;
	  case i386_optype_imm:
	    if (!(I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED))
	      PushImm(I386_OP_IMMEDIATE(src1),ins);
	    else if (!StringPatternMatch("*P*",RELOC_CODE(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)))))
	      PushRel(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)),ins);
	    else		/* pc-relative relocations are too hard to propagate. we just skip them */
	      PushUnk(ins);
	    break;
	  default:
	    PushUnk(ins);
	}
	break;
      case I386_PUSHF:
	PushUnk(ins);
	break;
      case I386_PUSHA:
	PushReg(I386_REG_EAX,ins);
	PushReg(I386_REG_ECX,ins);
	PushReg(I386_REG_EDX,ins);
	PushReg(I386_REG_EBX,ins);
	PushReg(I386_REG_ESP,ins);
	PushReg(I386_REG_EBP,ins);
	PushReg(I386_REG_ESI,ins);
	PushReg(I386_REG_EDI,ins);
	break;
      case I386_POP:
	PopCheck();
	if (I386IsGeneralPurposeReg(I386_OP_BASE(dest)))
	{
	  if (stack[top].kind == reg && !RegsetIn(stack[top].changed,stack[top].u.reg) && stack[top].refcount == 0 /*&& (counter++ < diablosupport_options.debugcounter)*/)
	  {
	    if (stack[top].u.reg == I386_OP_BASE(dest))
	    {
	      /*            VERBOSE(0,( "@I pops value that should never have been saved\n",ins));*/
	      RemoveStackSlot(top,stack[top].def,ins,TRUE);
	      change = TRUE;
	    }
	    else
	    {
	      t_reg d = I386_OP_BASE(dest), s = stack[top].u.reg;
	      RemoveStackSlot(top,stack[top].def,ins,TRUE);
	      I386InstructionMakeMovToReg(ins,d,s,0);
	      change = TRUE;
	    }
	  }
	  else if (stack[top].kind == imm && stack[top].refcount == 0  /*&& (counter++ < diablosupport_options.debugcounter)*/)
	  {
	    t_reg d = I386_OP_BASE(dest);
	    t_uint32 imm = stack[top].u.imm;

	    RemoveStackSlot(top,stack[top].def,ins,TRUE);
	    I386InstructionMakeMovToReg(ins,d,I386_REG_NONE,imm);
	    change = TRUE;
	  }
	  else if (stack[top].kind == rel && !StringPatternMatch("*P*",RELOC_CODE(stack[top].u.rel)) && stack[top].refcount == 0 /*&& (counter++ < diablosupport_options.debugcounter)*/)
	  {
	    t_reg d = I386_OP_BASE(dest);
	    t_reloc * rel = stack[top].u.rel;
	    t_reloc * rel2;

	    RemoveStackSlot(top,stack[top].def,ins,TRUE);
	    I386InstructionMakeMovToReg(ins,d,I386_REG_NONE,0);
	    I386_OP_FLAGS(I386_INS_SOURCE1(ins)) |= I386_OPFLAG_ISRELOCATED;
	    rel2 = RelocTableDupReloc (RELOC_TABLE(rel), rel);
	    RelocSetFrom(rel2, T_RELOCATABLE(ins));
	    change = TRUE;
	  }
	}
	--top;
	break;
      case I386_POPF:
	PopCheck();
	--top;
	break;
      case I386_POPA:
	top -= 8;
	PopCheck();
	break;

      case I386_ADD:
	if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_BASE(dest) == I386_REG_ESP)
	{
	  int slots,i;
	  t_bool do_reset = FALSE;

	  if (I386_OP_TYPE(src1) != i386_optype_imm)
	    goto stack_reset; 	/* we can't know the new stack pointer value, so we reset the  propagation */
	  slots = ((t_int32)I386_OP_IMMEDIATE(src1)) / 4;
	  if (slots > top + 1)
	  {
	    /*goto stack_reset;*/
	    slots = top+1;
	    do_reset = TRUE;
	    /*VERBOSE(0,("DELAYING THE RESET FOR @I\n",ins));*/
	  }
	  else if (slots < 0)
	  {
	    /* this should really be handled by the SUB case */
	    goto stack_reset;
	  }
	  /* find out if any of the freed stack slots was never referenced.
	   * if so, it should not have been allocated in the first place */
          if (slots > 0)
	  {
#ifdef _MSC_VER
	    t_bool* killable = (t_bool*)Malloc(slots*sizeof(t_bool));
#else
            t_bool killable[slots];
#endif
	    int nkillable = 0;

	    for (i=0; i<slots; i++)
	    {
	      if (stack[top-i].refcount == 0) 
	      {
		      killable[i] = TRUE;
		      nkillable++;
	      }
	      else 
		      killable[i] = FALSE;
	    }
	    while (nkillable > 0)
	    {
	      nkillable--;

	      for (i=0; i<slots; i++)
	      {
		      if (killable[i] /*&& (counter++ < diablosupport_options.debugcounter)*/)
		      {
		        change = TRUE;
		        RemoveStackSlot(top-i,stack[top-i].def,ins,TRUE);
		        /* update datastructures to reflect the new stack layout */
		        memmove(&(stack[top-i]), &(stack[top-i+1]), i*sizeof(t_stack_cont));
		        memmove(&(killable[i]),&(killable[i+1]),(slots-i-1)*sizeof(t_bool));
		        slots--;
		        top--;
		      }
	      }
	    }

#ifdef _MSC_VER
            Free(killable);
#endif
	  }
	  if (do_reset)		/* stack was popped below the beginning of the propagation, reset propagation */
	    goto stack_reset;
	  top -= slots;
	}
	else
	{
	  /* this instruction does something with the stack pointer we don't understand. halt propagation. */
	  goto function_exit;
	}
	break;

      case I386_SUB:
	if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_BASE(dest) == I386_REG_ESP)
	{
	  int slots,i;

	  if (I386_OP_TYPE(src1) != i386_optype_imm)
	    goto stack_reset; 	/* we can't know the new stack pointer value, so we reset the propagation */
	  slots = ((t_int32)I386_OP_IMMEDIATE(src1)) / 4;
	  for (i=0; i<slots; i++)
	    PushUnk(ins);
	}
	else
	{
	  /* this instruction does something with the stack pointer we don't understand. halt propagation. */
	  goto function_exit;
	}
	break;

      case I386_CALL:
      case I386_RET:
	/* ignore these, they always come at the end of a bbl so propagation
	 * stops anyway */
	break;

      case I386_MOV:
	if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	    I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP)
	{
	  /* this creates a reference to the top stack slot. increase this slot's refcount */
	  if (top >= 0)
	    stack[top].refcount++;
	}
	else
	{
	  /* stop propagation as something happens to the stack pointer and we don't know what */
	  goto function_exit;
	}

      default:
	/* this instruction uses or defines the stack pointer in unexpected ways.
	 * stop propagation because we can't judge the effect of any further optimizations
	 * on this instruction */
	goto function_exit;
    }

    for (i=0; i<=top; i++) RegsetSetUnion(stack[i].changed,I386_INS_REGS_DEF(ins));

    continue;

stack_reset:
    top = -1;
  }

function_exit:
  return change;
}

void I386PeepHoleStack(t_cfg * cfg)
{
  t_bbl * bbl;
  CFG_FOREACH_BBL(cfg,bbl)
  {
    if (!BBL_NINS(bbl)) continue;
    while (PropBlock(bbl)) {}
  }
}
/* }}} */

/* {{{ idempotent instruction removal */
void I386PeepHoleIdempotent(t_cfg * cfg)
{
  t_bbl * bbl;
  t_i386_ins * ins, * tmp;
  t_uint32 kills = 0;
  int nop,lea,mov,add_sub;
  nop = lea = mov = add_sub = 0;
  t_regset condflags = NullRegs;

  RegsetSetAddReg(condflags,I386_CONDREG_OF);
  RegsetSetAddReg(condflags,I386_CONDREG_SF);
  RegsetSetAddReg(condflags,I386_CONDREG_ZF);
  RegsetSetAddReg(condflags,I386_CONDREG_AF);
  RegsetSetAddReg(condflags,I386_CONDREG_PF);
  RegsetSetAddReg(condflags,I386_CONDREG_CF);
  
  /* for some substitutions, we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_I386_INS_SAFE(bbl,ins,tmp)
    {
      t_i386_operand * dest = I386_INS_DEST(ins), * src1 = I386_INS_SOURCE1(ins) /* , *src2 = I386_INS_SOURCE2(ins) */ ;

      switch (I386_INS_OPCODE(ins))
      {
	case I386_NOP:
	  kills++;
	  nop++;
	  I386InsKill(ins);
	  break;
	case I386_MOV:
	  if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_TYPE(src1) == i386_optype_reg)
	    if (I386_OP_BASE(dest) == I386_OP_BASE(src1) && I386_OP_REGMODE(dest) == I386_OP_REGMODE(src1))
	    {
	      kills++;
	      mov++;
	      I386InsKill(ins);
	    }
	  break;
	case I386_LEA:
	  if (I386_OP_BASE(dest) == I386_OP_BASE(src1) &&
	      I386_OP_IMMEDIATE(src1) == 0 && I386_OP_INDEX(src1) == I386_REG_NONE)
	  {
	    kills++;
	    lea++;
	    I386InsKill(ins);
	  }
	  break;
	case I386_ADD:
	case I386_SUB:
	  if ((I386_OP_TYPE(src1) == i386_optype_imm && I386_OP_IMMEDIATE(src1) == 0) &&
              RegsetIsMutualExclusive(I386InsRegsLiveAfter(ins),condflags))
	  {
            kills++;
            add_sub++;
            I386InsKill(ins);
	  }
	  break;
	default:
	  break;
      }
    }
  VERBOSE(0,("[idempotent instruction removal] killed %d instructions",kills));
  /*printf("nop %d mov %d lea %d add/sub %d\n", nop,mov,lea,add_sub);*/
} /* }}} */

/* {{{ redundant pop/push removal */
/* look for patterns like
 *  pop reg
 *  ... some stuff that doesn't change %esp or reg
 *  push reg
 *
 * we can then change the pop into a load and remove the push,
 * if we can adjust all intermediate uses of the stack pointer
 */

/* helper function */
static t_bool StackUseIsWellbehaved(t_i386_ins * ins)
{
  t_i386_operand * op1, * op2;
  t_bool found_esp_use = FALSE;
  t_bool well_behaved = TRUE;
  static int counter = 0;

//  if (counter++ >= diablosupport_options.debugcounter)
//    return FALSE;

  op1 = I386InsGetMemLoadOp(ins);
  op2 = I386InsGetMemStoreOp(ins);
  
  if (op1)
  {
    if (I386_OP_TYPE(op1) == i386_optype_mem)
    {
      if (I386_OP_BASE(op1) == I386_REG_ESP)
	found_esp_use = TRUE;
    }
    else
      well_behaved = FALSE;
  }
  if (op2)
  {
    if (I386_OP_TYPE(op2) == i386_optype_mem)
    {
      if (I386_OP_BASE(op2) == I386_REG_ESP)
	found_esp_use = TRUE;
    }
    else
      well_behaved = FALSE;
  }
  return found_esp_use && well_behaved;
}

void I386PeepHoleRedundantPushes(t_cfg * cfg)
{
  t_bbl * bbl;
  t_i386_ins * ins;
  /*static int teller = 0; */

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_bool rerun = TRUE;
    while (rerun)
    {
      t_i386_ins * pop = NULL;
      t_regset use,def;
      t_reg reg = REG_NONE;
      
      rerun = FALSE;
      use = def = NullRegs;
      BBL_FOREACH_I386_INS(bbl,ins)
      {
	if (I386_INS_OPCODE(ins) == I386_POP)
	{
	  pop = ins;
	  reg = I386_OP_BASE(I386_INS_DEST(ins));
	  use = def = NullRegs;
	  continue;
	}

	/* for speed */
	if (!pop) continue;

	if (I386_INS_OPCODE(ins) == I386_PUSH)
	{
	  t_i386_operand * op = I386_INS_SOURCE1(ins);
	  if (I386_OP_TYPE(op) == i386_optype_reg && I386_OP_BASE(op) == reg)
	  {
	    t_i386_ins * iter;
	    /* change the pop instruction */
	    I386InstructionMakeMovFromMem(pop,reg,0,I386_REG_ESP,I386_REG_NONE,I386_SCALE_1);
	    /* adjust all intermediate %esp uses */
	    for (iter = I386_INS_INEXT(pop); iter != ins; iter = I386_INS_INEXT(iter))
	    {
	      t_i386_operand *op1, *op2;
	      if (!RegsetIn(I386_INS_REGS_USE(iter),I386_REG_ESP)) continue;
	      
	      op1 = I386InsGetMemLoadOp(iter);
	      op2 = I386InsGetMemStoreOp(iter);

	      if (op1 && I386_OP_BASE(op1) == I386_REG_ESP)
		I386OpSetMem(op1,I386_OP_IMMEDIATE(op1)+4,I386_OP_BASE(op1),I386_OP_INDEX(op1),I386_OP_SCALE(op1),I386_OP_MEMOPSIZE(op1));
	      if (op2 && op2 != op1 && I386_OP_BASE(op2) == I386_REG_ESP)
		I386OpSetMem(op2,I386_OP_IMMEDIATE(op2)+4,I386_OP_BASE(op2),I386_OP_INDEX(op2),I386_OP_SCALE(op2),I386_OP_MEMOPSIZE(op2));
	    }
	    /* kill the push instruction */
	    I386InsKill(ins);
	    /* restart examination of the block to find further opportunities */
	    rerun = TRUE;
	    break;
	  }
	}

	if (RegsetIn(I386_INS_REGS_DEF(ins),reg) || RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP))
	{
	  pop = NULL;
	  continue;
	}
	
	if (RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP))
	{
	  /* we can tolerate this, if it is possible to adjust the use to the changed stack pointer */
	  if (!StackUseIsWellbehaved(ins))
	    pop = NULL;
	}
      }
    }
  }
}
/* }}} */

/* {{{ substitute instructions with smaller, equivalent forms */
void I386PeepHoleShorterEquivalent(t_cfg * cfg)
{
  t_bbl * bbl;
  t_i386_ins * ins;
  t_regset condflags = NullRegs;

  RegsetSetAddReg(condflags,I386_CONDREG_OF);
  RegsetSetAddReg(condflags,I386_CONDREG_SF);
  RegsetSetAddReg(condflags,I386_CONDREG_ZF);
  RegsetSetAddReg(condflags,I386_CONDREG_AF);
  RegsetSetAddReg(condflags,I386_CONDREG_PF);
  RegsetSetAddReg(condflags,I386_CONDREG_CF);

  /* for most substitutions, we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_I386_INS(bbl,ins)
    {
      t_i386_operand * dest = I386_INS_DEST(ins);
      t_i386_operand * src  = I386_INS_SOURCE1(ins);
      t_i386_operand * src2 = I386_INS_SOURCE2(ins);

      switch (I386_INS_OPCODE(ins))
      {
	case I386_MOV:
	  if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_TYPE(src) == i386_optype_imm)
	  {
	    if (I386IsGeneralPurposeReg(I386_OP_BASE(dest)) && I386_OP_REGMODE(dest) == i386_regmode_full32)
	    {
	      if (I386_OP_IMMEDIATE(src) == 0 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
	      {
		/* mov $0,%reg -> xor %reg,%reg */
		if (RegsetIsMutualExclusive(I386InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = I386_OP_BASE(dest);
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeArithmetic(ins,I386_XOR,d,d,0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	      else if (I386_OP_IMMEDIATE(src) == -1 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
	      {
		
		/* mov $-1,%reg -> or $-1,%reg */
		if (RegsetIsMutualExclusive(I386InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = I386_OP_BASE(dest);
		  /* VERBOSE(0,("SR: @I -> ",ins)); */
		  I386InstructionMakeArithmetic(ins,I386_OR,d,I386_REG_NONE,-1);
		  /* VERBOSE(0,("SR: <- @I ",ins)); */
		}
	      }
	    }
	  }
	  break;
	case I386_ADD:
	  if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_TYPE(src) == i386_optype_imm)
	  {
	    if (I386IsGeneralPurposeReg(I386_OP_BASE(dest)) && I386_OP_REGMODE(dest) == i386_regmode_full32)
	    {
	      if (!RegsetIn(I386InsRegsLiveAfter(ins),I386_CONDREG_CF))
	      {
		if (I386_OP_IMMEDIATE(src) == 1 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
		{
		  /* add $1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeIncDec(ins,I386_INC,I386_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (I386_OP_IMMEDIATE(src) == -1 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
		{
		  /* add $-1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeIncDec(ins,I386_DEC,I386_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case I386_SUB:
	  if (I386_OP_TYPE(dest) == i386_optype_reg && I386_OP_TYPE(src) == i386_optype_imm)
	  {
	    if (I386IsGeneralPurposeReg(I386_OP_BASE(dest)) && I386_OP_REGMODE(dest) == i386_regmode_full32)
	    {
	      if (!RegsetIn(I386InsRegsLiveAfter(ins),I386_CONDREG_CF))
	      {
		if (I386_OP_IMMEDIATE(src) == 1 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
		{
		  /* sub $1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeIncDec(ins,I386_DEC,I386_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (I386_OP_IMMEDIATE(src) == -1 && (!(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)))
		{
		  /* sub $-1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeIncDec(ins,I386_INC,I386_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case I386_CMP:
	  if (I386_OP_TYPE(src) == i386_optype_reg && I386_OP_TYPE(src2) == i386_optype_imm)
	  {
	    if (I386IsGeneralPurposeReg(I386_OP_BASE(src)) && I386_OP_REGMODE(src) == i386_regmode_full32)
	    {
	      if (I386_OP_IMMEDIATE(src2) == 0 && (!(I386_OP_FLAGS(src2) & I386_OPFLAG_ISRELOCATED)))
	      {
		if (RegsetIsMutualExclusive(I386InsRegsLiveAfter(ins),condflags))
		{
		  /* cmp $0,%reg -> test %reg,%reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  I386InstructionMakeTest(ins,I386_OP_BASE(src),I386_OP_BASE(src),0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case I386_LEA:
	  /* lea (%rega),%regb -> mov %rega,%regb */
	  if (I386_OP_IMMEDIATE(src) == 0 && I386_OP_INDEX(src) == I386_REG_NONE)
	  {
	    I386InstructionMakeMovToReg(ins,I386_OP_BASE(dest),I386_OP_BASE(src),0);
	  }
	  /* lea <abs address>,%reg -> mov $<abs address>,%reg */
	  else if (I386_OP_BASE(src) == I386_REG_NONE && I386_OP_INDEX(src) == I386_REG_NONE)
	  {
	    t_i386_ins * extra;
	    I386MakeInsForIns(MovToReg,Before,extra,ins,I386_OP_BASE(dest),I386_REG_NONE,I386_OP_IMMEDIATE(src));
	    if (I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED)
	    {
	      RelocSetFrom(RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)),(t_relocatable *)extra);
	      I386_OP_FLAGS(I386_INS_SOURCE1(extra)) |= I386_OPFLAG_ISRELOCATED;
	    }
	    I386InsKill(ins);
	    ins = extra;
	  }
	  break;
	default:
	  /* keep the compiler happy */
	  break;
      }
    }
  }
}
/* }}} */

/* {{{ subsume adjacent add/sub combinations in one instruction */
void I386PeepHoleAddSub(t_cfg * cfg)
{
  t_bbl * bbl;
  t_i386_ins * ins, * tmp;
  t_regset condflags = NullRegs;
  /*static int counter; */

  RegsetSetAddReg(condflags,I386_CONDREG_OF);
  RegsetSetAddReg(condflags,I386_CONDREG_SF);
  RegsetSetAddReg(condflags,I386_CONDREG_ZF);
  RegsetSetAddReg(condflags,I386_CONDREG_AF);
  RegsetSetAddReg(condflags,I386_CONDREG_PF);
  RegsetSetAddReg(condflags,I386_CONDREG_CF);

  /* we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_I386_INS_SAFE(bbl,ins,tmp)
    {
      if (I386_INS_OPCODE(ins) == I386_ADD || I386_INS_OPCODE(ins) == I386_SUB)
      {
	t_i386_operand * dest = I386_INS_DEST(ins), * src = I386_INS_SOURCE1(ins);
	t_i386_operand * dest2, * src2;

	if (I386_OP_TYPE(dest) == i386_optype_reg 
	    && I386_OP_REGMODE(dest) == i386_regmode_full32 
	    && I386IsGeneralPurposeReg(I386_OP_BASE(dest))
	    && I386_OP_TYPE(src) == i386_optype_imm 
	    && !(I386_OP_FLAGS(src) & I386_OPFLAG_ISRELOCATED))
	{
	  t_regset def = I386_INS_REGS_DEF(ins);
	  t_reg d = I386_OP_BASE(dest);
          t_bool found = FALSE;
	  t_i386_ins * iter;
	  for (iter = I386_INS_INEXT(ins); iter; iter = I386_INS_INEXT(iter))
	  {
	    dest2 = I386_INS_DEST(iter);
	    src2 = I386_INS_SOURCE1(iter);

	    if (RegsetIsMutualExclusive(I386_INS_REGS_USE(iter),def))
	    {
	      if (!RegsetIn(I386_INS_REGS_DEF(iter),d))
		continue;	/* look further down */
	      else
		break;		/* d overwritten: stop */
	    }
	    /* should be add or sub */
	    if (I386_INS_OPCODE(iter) == I386_ADD || I386_INS_OPCODE(iter) == I386_SUB)
	    {
	      if (I386_OP_TYPE(dest2) == i386_optype_reg 
		  && I386_OP_REGMODE(dest2) == i386_regmode_full32 
		  && I386_OP_BASE(dest2) == d
		  && I386_OP_TYPE(src2) == i386_optype_imm 
		  && !(I386_OP_FLAGS(src2) & I386_OPFLAG_ISRELOCATED)
		  && RegsetIsMutualExclusive(I386InsRegsLiveAfter(iter),condflags))
		found = TRUE;
	    }
	    break;
	  }

	  /* merge the two instructions */
	  if (found /*&& (counter++ < diablosupport_options.debugcounter)*/)
	  {
	    t_i386_opcode opcode = I386_INS_OPCODE(iter);
	    t_uint32 imm;
	    /*VERBOSE(0,("AS: will merge @I and @I\n",ins,iter));*/
	    /*VERBOSE(0,("in @iB",INS_BBL(ins)));*/
	    imm = I386_OP_IMMEDIATE(src2) + 
	         (I386_INS_OPCODE(iter) == I386_INS_OPCODE(ins) ? 1 : -1)*I386_OP_IMMEDIATE(src);
	    if (imm == 128)
	    {
	      /* 128 is encoded in 4 bytes, -128 in 1 byte */
	      opcode = (opcode == I386_SUB) ? I386_ADD : I386_SUB;
	      imm = -imm;
	    }
	    I386_INS_SET_OPCODE(iter, opcode);
	    I386_OP_IMMEDIATE(src2) = imm;
	    I386InsKill(ins);
	    /*VERBOSE(0,("--> @I\n",iter));*/
	  }
	}
      }
    }
  }
}
/* }}} */

/* {{{ if possible, remove frame pointer set/reset from one-block functions 
 * this helps for simple function inlining */
void I386PeepHoleFramePointerRemoval(t_cfg * cfg)
{
  t_function * fun;
  /*static int count=0;*/

  I386FramePointerAnalysis(cfg);
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_bbl * first = FUNCTION_BBL_FIRST(fun);
    t_i386_ins * ins, * prologue, * epilogue;
    int height = 0;
    t_bool can_remove = TRUE;

    if (!(FUNCTION_FLAGS(fun) & FF_HAS_FRAMEPOINTER)) continue;
    if (!first) continue;
    if (BBL_NEXT_IN_FUN(first) && BBL_NEXT_IN_FUN(first) != FunctionGetExitBlock(fun)) continue;

    /* find frame pointer set/reset prologue and epilogue */
    ins = T_I386_INS(BBL_INS_FIRST(first));
    if (I386_INS_OPCODE(ins) != I386_PUSH || 
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg ||
	I386_OP_BASE(I386_INS_SOURCE1(ins)) != I386_REG_EBP ||
	I386_OP_REGMODE(I386_INS_SOURCE1(ins)) != i386_regmode_full32)
      continue;
    ins = I386_INS_INEXT(ins);
    if (I386_INS_OPCODE(ins) != I386_MOV ||
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_reg ||
	I386_OP_BASE(I386_INS_SOURCE1(ins)) != I386_REG_ESP ||
	I386_OP_REGMODE(I386_INS_SOURCE1(ins)) != i386_regmode_full32 ||
	I386_OP_TYPE(I386_INS_DEST(ins)) != i386_optype_reg ||
	I386_OP_BASE(I386_INS_DEST(ins)) != I386_REG_EBP ||
	I386_OP_REGMODE(I386_INS_DEST(ins)) != i386_regmode_full32)
      continue;
    prologue = I386_INS_INEXT(ins);

    ins = T_I386_INS(BBL_INS_LAST(first));
    if (I386_INS_OPCODE(ins) != I386_RET)
      continue;
    ins = I386_INS_IPREV(ins);
    if (!(
	  (I386_INS_OPCODE(ins) == I386_LEAVE) || 
	  (I386_INS_OPCODE(ins) == I386_POP &&
	   I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	   I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP &&
	   I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32)
	  ))
      continue;
    ins = I386_INS_IPREV(ins);
    if (I386_INS_OPCODE(ins) == I386_MOV &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP &&
	I386_OP_REGMODE(I386_INS_SOURCE1(ins)) == i386_regmode_full32 &&
	I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP &&
	I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32)
      ins = I386_INS_IPREV(ins);
    epilogue = I386_INS_INEXT(ins);

    /* check if we can really remove the frame pointer */
    for (ins = prologue; ins != epilogue; ins = I386_INS_INEXT(ins))
    {
      if (RegsetIn(I386_INS_REGS_USE(ins),I386_REG_EBP) ||
	  RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_EBP))
      {
	can_remove = FALSE;
	break;
      }
      if (RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP) ||
	  RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP))
      {
	if (I386_INS_OPCODE(ins) == I386_PUSH)
	  height += 4;
	else if (I386_INS_OPCODE(ins) == I386_POP)
	  height -= 4;
	else if (I386_INS_OPCODE(ins) == I386_SUB &&
	    I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	    I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP &&
	    I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32 &&
	    I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	  height += I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins));
	else if (I386_INS_OPCODE(ins) == I386_ADD &&
	    I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	    I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP &&
	    I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32 &&
	    I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	  height -= I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins));
	else
	{
	  can_remove = FALSE;
	  break;
	}

	if (height < 0)
	{
	  can_remove = FALSE;
	  break;
	}
      }
    }

    /* remove the prologue and epilogue */
    if (can_remove /* && (count++ < diablosupport_options.debugcounter)*/)
    {
      VERBOSE (1, ("framepointer thingy @ieB", first));
      while (I386_INS_IPREV(prologue))
	I386InsKill(I386_INS_IPREV(prologue));
      ins = I386_INS_IPREV(epilogue);
      while (I386_INS_IPREV(T_I386_INS(BBL_INS_LAST(first))) != ins)
	I386InsKill(I386_INS_IPREV(T_I386_INS(BBL_INS_LAST(first))));
      /* compensate for unbalanced stack */
      if (height != 0)
	I386MakeInsForIns(Arithmetic,Before,ins,T_I386_INS(BBL_INS_LAST(first)),I386_ADD,I386_REG_ESP,I386_REG_NONE,height);
      VERBOSE (1, ("after @ieB", first));
    }

  }
  
  I386FramePointerAnalysis(cfg);
}
/* }}} */

/* replace add/sub $4,%esp with a (smaller) pop or push */
void I386PeepHoleAddSubESP(t_cfg * cfg)
{
  t_bbl *bbl;
  t_i386_ins *ins;
  /* we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_I386_INS(bbl,ins)
    {
      t_i386_operand *dest = I386_INS_DEST(ins);
      t_i386_operand *src = I386_INS_SOURCE1(ins);
      if (I386_INS_OPCODE(ins) == I386_ADD
	  && I386_OP_TYPE(dest) == i386_optype_reg
	  && I386_OP_BASE(dest) == I386_REG_ESP
	  && I386_OP_TYPE(src) == i386_optype_imm
	  && I386_OP_IMMEDIATE(src) == 4)
      {
	t_regset dead = I386InsRegsLiveAfter(ins);
	t_reg r;
	RegsetSetInvers(dead);
	REGSET_FOREACH_REG(dead,r)
	  break;
	if (I386IsGeneralPurposeReg(r) && (r != I386_REG_ESP))
	{
	  I386InstructionMakePop(ins,r);
	  VERBOSE(0,("+_+_+ MADE @I\n",ins));
	}
      }
      else if (I386_INS_OPCODE(ins) == I386_SUB
	  && I386_OP_TYPE(dest) == i386_optype_reg
	  && I386_OP_BASE(dest) == I386_REG_ESP
	  && I386_OP_TYPE(src) == i386_optype_imm
	  && I386_OP_IMMEDIATE(src) == 4)
      {
	/* push some live register instead */
	t_regset live = I386InsRegsLiveAfter(ins);
	t_reg r;
	REGSET_FOREACH_REG(live,r)
	  break;
	if (I386IsGeneralPurposeReg(r))
	  I386InstructionMakePush(ins,r,0);
	else
	{
	  I386InstructionMakePush(ins,I386_REG_NONE,0);
	  I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins)) = 1;		/* push $0 */
	}
	VERBOSE(0,("+_+_+ MADE @I\n",ins));
      }
    }
  }
}


void I386PeepHoles(t_cfg * cfg)
{
  I386PeepHoleFramePointerRemoval(cfg);
  I386PeepHoleStack(cfg);
  I386PeepHoleRedundantPushes(cfg); 
  I386PeepHoleShorterEquivalent(cfg); 
  I386PeepHoleAddSub(cfg); 
  I386PeepHoleIdempotent(cfg);  
  /*I386PeepHoleAddSubESP(cfg);*/
}

/* vim: set shiftwidth=2 foldmethod=marker : */
