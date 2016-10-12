#include <diabloanoptamd64.h>
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
  t_amd64_ins * def;
  t_regset changed;
} t_stack_cont;

static t_stack_cont stack[STACK_SIZE];
static int top = -1;

static void RemoveStackSlot(int slot, t_amd64_ins * alloc, t_amd64_ins * release, t_bool remove_release)
{
  t_amd64_ins * ins;

  /* {{{ adjust all stack references between allocation and deletion of the slot */
  for (ins = AMD64_INS_INEXT(alloc); ins != release; ins = AMD64_INS_INEXT(ins))
  {
    t_amd64_operand *dest = AMD64_INS_DEST(ins), *src1 = AMD64_INS_SOURCE1(ins), *src2 = AMD64_INS_SOURCE2(ins);
    long local_top = (long) AMD64_INS_TMP(ins);
    int thres = (local_top - slot) * 8;

    /* local stack top is one slot lower after the slot is removed */

    AMD64_INS_SET_TMP(ins,  (void*) ((long)AMD64_INS_TMP(ins)-1));

    if (!RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP)) continue;
    
    if (AMD64_OP_TYPE(dest) == amd64_optype_mem && AMD64_OP_BASE(dest) == AMD64_REG_RSP)
    {
      if (AMD64_OP_IMMEDIATE(dest) > thres) 
	AMD64_OP_IMMEDIATE(dest) -= 8;
      else if (AMD64_OP_IMMEDIATE(dest) == thres)
	FATAL(("stack propagation algorithm is corrupt"));
    }
    if (AMD64_OP_TYPE(src1) == amd64_optype_mem && AMD64_OP_BASE(src1) == AMD64_REG_RSP)
    {
      if (AMD64_OP_IMMEDIATE(src1) > thres) 
	AMD64_OP_IMMEDIATE(src1) -= 8;
      else if (AMD64_OP_IMMEDIATE(src1) == thres)
	FATAL(("stack propagation algorithm is corrupt\n ins @I",ins));
    }
    if (AMD64_OP_TYPE(src2) == amd64_optype_mem && AMD64_OP_BASE(src2) == AMD64_REG_RSP)
    {
      if (AMD64_OP_IMMEDIATE(src2) > thres) 
	AMD64_OP_IMMEDIATE(src2) -= 8;
      else if (AMD64_OP_IMMEDIATE(src2) == thres)
	FATAL(("stack propagation algorithm is corrupt"));
    }
  } /* }}} */

  /* {{{ remove allocation of the slot */
  switch (AMD64_INS_OPCODE(alloc))
  {
    case AMD64_PUSH:
      Amd64InsKill(alloc);
      break;
    case AMD64_SUB:
      AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(alloc)) -= 8;
      if (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(alloc)) == 0)
	Amd64InsKill(alloc);
      break;
    default:
      FATAL(("unknown stack slot allocation @I",alloc));
  }
  /* }}} */

  /* {{{ remove release of the slot */
  if (remove_release)
  {
    switch (AMD64_INS_OPCODE(release))
    {
      case AMD64_ADD:
	AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(release)) -= 8;
	if (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(release)) == 0)
	  Amd64InstructionMakeNoop(release);	/* do not kill this instruction immediately, it is still referenced in the main loop of PropBlock */
	break;
      case AMD64_POP:
	Amd64InstructionMakeNoop(release);		/* do not kill this instruction immediately, it is still referenced in the main loop of PropBlock */
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
  t_amd64_ins * ins, * tmp;
  t_regset changed = NullRegs;
  int i;
  /*static int counter = 0;*/

  top = -1;

  BBL_FOREACH_AMD64_INS_SAFE(bbl,ins,tmp)
  {
    t_amd64_operand * src1 = AMD64_INS_SOURCE1(ins);
    /*t_amd64_operand * src2 = AMD64_INS_SOURCE2(ins); */
    t_amd64_operand * dest = AMD64_INS_DEST(ins);

    AMD64_INS_SET_TMP(ins,  (void *) top);

    /* skip instructions that do not read from or write to memory, and that do
     * not use or define the stack pointer */
    if (!RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP) && !RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP) && !Amd64InsIsLoad(ins) && !Amd64InsIsStore(ins))
    {
      for (i=0; i<=top; i++) RegsetSetUnion(stack[i].changed,AMD64_INS_REGS_DEF(ins));
      continue;
    }


    /* 1. loads */
    if (Amd64InsIsLoad(ins))
    {
      t_amd64_operand * op = Amd64InsGetMemLoadOp(ins);
      if (op)
      {
	/* possible aliasing? */
	if (AMD64_OP_BASE(op) != AMD64_REG_RSP && AMD64_OP_BASE(op) != AMD64_REG_NONE)
	{
	  int i;
	  for (i=0; i<=top; i++) stack[i].refcount++;
	}
	/* stack-relative? */
	if (AMD64_OP_BASE(op) == AMD64_REG_RSP)
	{
	  if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	  {
	    /* there is an index register, so we can't know for sure which
	     * stack slot we change: increase their refcounts */
	    int i;
	    for (i=0; i<=top; i++) stack[i].refcount++;
	  }
	  else
	  {
	    int slot = top - (AMD64_OP_IMMEDIATE(op) / 8);
	    int nslots = (AMD64_OP_MEMOPSIZE(op) + 3) / 8; /* round up, not down */
	    int i;
	    for (i = slot; i >= 0 && i > slot - nslots; i--)
	    {
	      stack[i].refcount++;
	      if (nslots == 1 && 
		  !((AMD64_INS_OPCODE(ins) == AMD64_CMP || AMD64_INS_OPCODE(ins) == AMD64_TEST)
		    && op == AMD64_INS_SOURCE1(ins)))
	      {
		/* optimization only possible if slots are read one at a time */
		if (stack[i].kind == imm)
		{
/*                  VERBOSE(0,("@I loads immediate %x\n",ins,stack[i].u.imm));*/
		  Amd64OpSetImm(op,stack[i].u.imm,8);
		  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
		  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
		  change = TRUE;
		}
		else if (stack[i].kind == rel)
		{
		  t_reloc * rel = stack[i].u.rel;
		  t_reloc * rel2 ;
/*                  VERBOSE(0,("@I loads relocated immediate @R\n",ins,stack[i].u.rel));*/
		  ASSERT(!AMD64_INS_REFERS_TO(ins),("ins already refers to something!"));
		  Amd64OpSetImm(op,0,4);
		  AMD64_OP_FLAGS(op) |= AMD64_OPFLAG_ISRELOCATED;

		  rel2 = RelocTableDupReloc (RELOC_TABLE(rel), rel);
		  RelocSetFrom(rel2, T_RELOCATABLE(ins));
		  
		  RELOC_SET_LABEL(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)), StringDup(RELOC_LABEL(rel)));
		  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
		  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
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
    if (Amd64InsIsStore(ins))
    {
      t_amd64_operand * op = Amd64InsGetMemStoreOp(ins);
      if (op)
      {
	/* possible aliasing? */
	if (AMD64_OP_BASE(op) != AMD64_REG_RSP && AMD64_OP_BASE(op) != AMD64_REG_NONE)
	  goto stack_reset;
	/* stack-relative? */
	if (AMD64_OP_BASE(op) == AMD64_REG_RSP)
	{
	  if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
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
	    int slot = top - (AMD64_OP_IMMEDIATE(op) / 8);
	    int nslots = (AMD64_OP_MEMOPSIZE(op) + 3) / 8; /* round up, not down */
	    if (AMD64_INS_OPCODE(ins) == AMD64_MOV 
		&& nslots == 1 && slot == top && slot >= 0 
		&& stack[top].refcount == 0
		&& AMD64_OP_MEMOPSIZE(op) == 4
		&& AMD64_OP_IMMEDIATE(op) == 0
		&& (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg 
		  || AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) != AMD64_REG_RSP)
	       )
	    {
	      /* special case: remove the previous definition and turn this into a push */
	      RemoveStackSlot(top,stack[top].def,ins,FALSE);
	      AMD64_INS_SET_OPCODE(ins, AMD64_PUSH);
	      AMD64_OP_TYPE(AMD64_INS_DEST(ins)) = amd64_optype_none;
	      AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
	      AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
	      change = TRUE;
	      stack[top].refcount = 0;
	      stack[top].kind = unk;
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
    if (AMD64_INS_OPCODE(ins) == AMD64_LEA)
    {
      if (AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP)
      {
	if (AMD64_OP_INDEX(AMD64_INS_SOURCE1(ins)) != AMD64_REG_NONE)
	{
	  /* we don't know which stack slot is referenced, mark them all */
	  int i;
	  for (i=0; i<=top; i++) stack[i].refcount++;
	}
	else
	{
	  int slot = top - AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins))/8;
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
     * 		push 8(%rsp)
     * this is both a stack manipulation and a load instruction. if the
     * stack manipulation is processed first, the analysis' logical stack pointer
     * has already changed by the time the effective load address is calculated. */
    /* 4. stack manipulation */
    switch (AMD64_INS_OPCODE(ins))
    {
      case AMD64_PUSH:
	switch (AMD64_OP_TYPE(src1))
	{
	  case amd64_optype_reg:
	    if (AMD64_OP_REGMODE(src1) == amd64_regmode_full64)
	      PushReg(AMD64_OP_BASE(src1),ins);
	    else
	      PushUnk(ins);
	    break;
	  case amd64_optype_imm:
	    if (!(AMD64_OP_FLAGS(src1) & AMD64_OPFLAG_ISRELOCATED))
	      PushImm(AMD64_OP_IMMEDIATE(src1),ins);
	    else if (!StringPatternMatch("*P*",RELOC_CODE(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)))))
	      PushRel(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)),ins);
	    else		/* pc-relative relocations are too hard to propagate. we just skip them */
	      PushUnk(ins);
	    break;
	  default:
	    PushUnk(ins);
	}
	break;
      case AMD64_PUSHF:
	PushUnk(ins);
	break;
      case AMD64_POP:
	PopCheck();
	if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)))
	{
	  if (stack[top].kind == reg && !RegsetIn(stack[top].changed,stack[top].u.reg) && stack[top].refcount == 0)
	  {
	    if (stack[top].u.reg == AMD64_OP_BASE(dest))
	    {
	      /*            VERBOSE(0,( "@I pops value that should never have been saved\n",ins));*/
	      RemoveStackSlot(top,stack[top].def,ins,TRUE);
	      change = TRUE;
	    }
	    else
	    {
	      t_reg d = AMD64_OP_BASE(dest), s = stack[top].u.reg;
	      RemoveStackSlot(top,stack[top].def,ins,TRUE);
	      Amd64InstructionMakeMovToReg(ins,d,s,0);
	      change = TRUE;
	    }
	  }
	  else if (stack[top].kind == imm && stack[top].refcount == 0)
	  {
	    t_reg d = AMD64_OP_BASE(dest);
	    t_uint32 imm = stack[top].u.imm;

	    RemoveStackSlot(top,stack[top].def,ins,TRUE);
	    Amd64InstructionMakeMovToReg(ins,d,AMD64_REG_NONE,imm);
	    change = TRUE;
	  }
	  else if (stack[top].kind == rel && !StringPatternMatch("*P*",RELOC_CODE(stack[top].u.rel)) && stack[top].refcount == 0)
	  {
	    t_reg d = AMD64_OP_BASE(dest);
	    t_reloc * rel = stack[top].u.rel;
	    t_reloc * rel2;

	    RemoveStackSlot(top,stack[top].def,ins,TRUE);
	    Amd64InstructionMakeMovToReg(ins,d,AMD64_REG_NONE,0);
	    AMD64_OP_FLAGS(AMD64_INS_SOURCE1(ins)) |= AMD64_OPFLAG_ISRELOCATED;
	    rel2 = RelocTableDupReloc (RELOC_TABLE(rel), rel);
	    RelocSetFrom(rel2, T_RELOCATABLE(ins));
	    change = TRUE;
	  }
	}
	--top;
	break;
      case AMD64_POPF:
	PopCheck();
	--top;
	break;
      case AMD64_ADD:
	if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_BASE(dest) == AMD64_REG_RSP)
	{
	  int slots,i;
	  t_bool do_reset = FALSE;

	  if (AMD64_OP_TYPE(src1) != amd64_optype_imm)
	    goto stack_reset; 	/* we can't know the new stack pointer value, so we reset the  propagation */
	  slots = ((t_int32)AMD64_OP_IMMEDIATE(src1)) / 8;
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
	  {
	    t_bool* killable = (t_bool*)Malloc(slots*sizeof(t_bool));
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
		if (killable[i])
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

      Free(killable);
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

      case AMD64_SUB:
	if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_BASE(dest) == AMD64_REG_RSP)
	{
	  int slots,i;

	  if (AMD64_OP_TYPE(src1) != amd64_optype_imm)
	    goto stack_reset; 	/* we can't know the new stack pointer value, so we reset the propagation */
	  slots = ((t_int32)AMD64_OP_IMMEDIATE(src1)) / 4;
	  for (i=0; i<slots; i++)
	    PushUnk(ins);
	}
	else
	{
	  /* this instruction does something with the stack pointer we don't understand. halt propagation. */
	  goto function_exit;
	}
	break;

      case AMD64_CALL:
      case AMD64_RET:
	/* ignore these, they always come at the end of a bbl so propagation
	 * stops anyway */
	break;

      case AMD64_MOV:
	if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg &&
	    AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP)
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

    for (i=0; i<=top; i++) RegsetSetUnion(stack[i].changed,AMD64_INS_REGS_DEF(ins));

    continue;

stack_reset:
    top = -1;
  }

function_exit:
  return change;
}

void Amd64PeepHoleStack(t_cfg * cfg)
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
void Amd64PeepHoleIdempotent(t_cfg * cfg)
{
  t_bbl * bbl;
  t_amd64_ins * ins, * tmp;
  t_uint32 kills = 0;
  int nop,lea,mov,add_sub,xchg;
  nop = lea = mov = add_sub = xchg =  0;
  
  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_AMD64_INS_SAFE(bbl,ins,tmp)
    {
      t_amd64_operand * dest = AMD64_INS_DEST(ins), * src1 = AMD64_INS_SOURCE1(ins);

      switch (AMD64_INS_OPCODE(ins))
      {
	case AMD64_XCHG:
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src1) == amd64_optype_reg)
	    if (AMD64_OP_BASE(dest) == AMD64_OP_BASE(src1) && AMD64_OP_REGMODE(dest) == AMD64_OP_REGMODE(src1))
	    {
	      if (AMD64_OP_REGMODE(dest) == amd64_regmode_full64)
   	      {
	        kills++;
	        xchg++;
	        Amd64InsKill(ins);
	      }
	      if(AMD64_OP_REGMODE(dest) == amd64_regmode_lo32 && AMD64_OP_BASE(dest) == AMD64_REG_RAX){
	 	kills++;
		xchg++;
		Amd64InsKill(ins);
	      }
	      if(AMD64_OP_REGMODE(dest) == amd64_regmode_lo16){
	        kills++;
	        xchg++;
	        Amd64InsKill(ins);
	      }
	    }
	  break;
	case AMD64_NOP:
	  kills++;
	  nop++;
	  Amd64InsKill(ins);
	  break;
	case AMD64_MOV:
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src1) == amd64_optype_reg)
	    if (AMD64_OP_BASE(dest) == AMD64_OP_BASE(src1) && AMD64_OP_REGMODE(dest) == AMD64_OP_REGMODE(src1))
	      if (AMD64_OP_REGMODE(dest) == amd64_regmode_full64)
  	      {
	        kills++;
	        mov++;
	        Amd64InsKill(ins);
	      }
	  break;
	case AMD64_LEA:
	  if (AMD64_OP_BASE(dest) == AMD64_OP_BASE(src1) &&
	      AMD64_OP_IMMEDIATE(src1) == 0 && AMD64_OP_INDEX(src1) == AMD64_REG_NONE)
	  {
	    kills++;
	    lea++;
	    Amd64InsKill(ins);
	  }
	  break;
	case AMD64_ADD:
	case AMD64_SUB:
	  if (AMD64_OP_TYPE(src1) == amd64_optype_imm && AMD64_OP_IMMEDIATE(src1) == 0)
	  {
	    kills++;
	    add_sub++;
	    Amd64InsKill(ins);
	  }
	  break;
	default:
	  break;
      }
    }
  VERBOSE(0,("[idempotent instruction removal] killed %d instructions",kills));
/*  printf("nop %d xchg %d mov %d lea %d add/sub %d\n", nop,xchg,mov,lea,add_sub);*/
} /* }}} */

/* {{{ redundant pop/push removal */
/* look for patterns like
 *  pop reg
 *  ... some stuff that doesn't change %rsp or reg
 *  push reg
 *
 * we can then change the pop into a load and remove the push,
 * if we can adjust all intermediate uses of the stack pointer
 */

/* helper function */
static t_bool StackUseIsWellbehaved(t_amd64_ins * ins)
{
  t_amd64_operand * op1, * op2;
  t_bool found_esp_use = FALSE;
  t_bool well_behaved = TRUE;

  op1 = Amd64InsGetMemLoadOp(ins);
  op2 = Amd64InsGetMemStoreOp(ins);
  
  if (op1)
  {
    if (AMD64_OP_TYPE(op1) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(op1) == AMD64_REG_RSP)
	found_esp_use = TRUE;
    }
    else
      well_behaved = FALSE;
  }
  if (op2)
  {
    if (AMD64_OP_TYPE(op2) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(op2) == AMD64_REG_RSP)
	found_esp_use = TRUE;
    }
    else
      well_behaved = FALSE;
  }
  return found_esp_use && well_behaved;
}

void Amd64PeepHoleRedundantPushes(t_cfg * cfg)
{
  t_bbl * bbl;
  t_amd64_ins * ins;
  /*static int teller = 0; */

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_bool rerun = TRUE;
    while (rerun)
    {
      t_amd64_ins * pop = NULL;
      t_regset use,def;
      t_reg reg = REG_NONE;
      
      rerun = FALSE;
      use = def = NullRegs;
      BBL_FOREACH_AMD64_INS(bbl,ins)
      {
	if (AMD64_INS_OPCODE(ins) == AMD64_POP)
	{
	  pop = ins;
	  reg = AMD64_OP_BASE(AMD64_INS_DEST(ins));
	  use = def = NullRegs;
	  continue;
	}

	/* for speed */
	if (!pop) continue;

	if (AMD64_INS_OPCODE(ins) == AMD64_PUSH)
	{
	  t_amd64_operand * op = AMD64_INS_SOURCE1(ins);
	  if (AMD64_OP_TYPE(op) == amd64_optype_reg && AMD64_OP_BASE(op) == reg)
	  {
	    t_amd64_ins * iter;
	    /* change the pop instruction */
	    Amd64InstructionMakeMovFromMem(pop,reg,0,AMD64_REG_RSP,AMD64_REG_NONE,AMD64_SCALE_1);
	    /* adjust all intermediate %rsp uses */
	    for (iter = AMD64_INS_INEXT(pop); iter != ins; iter = AMD64_INS_INEXT(iter))
	    {
	      t_amd64_operand *op1, *op2;
	      if (!RegsetIn(AMD64_INS_REGS_USE(iter),AMD64_REG_RSP)) continue;
	      
	      op1 = Amd64InsGetMemLoadOp(iter);
	      op2 = Amd64InsGetMemStoreOp(iter);

	      if (op1 && AMD64_OP_BASE(op1) == AMD64_REG_RSP)
		Amd64OpSetMem(op1,AMD64_OP_IMMEDIATE(op1)+8,AMD64_OP_BASE(op1),AMD64_OP_INDEX(op1),AMD64_OP_SCALE(op1),AMD64_OP_MEMOPSIZE(op1));
	      if (op2 && op2 != op1 && AMD64_OP_BASE(op2) == AMD64_REG_RSP)
		Amd64OpSetMem(op2,AMD64_OP_IMMEDIATE(op2)+8,AMD64_OP_BASE(op2),AMD64_OP_INDEX(op2),AMD64_OP_SCALE(op2),AMD64_OP_MEMOPSIZE(op2));
	    }
	    /* kill the push instruction */
	    Amd64InsKill(ins);
	    /* restart examination of the block to find further opportunities */
	    rerun = TRUE;
	    break;
	  }
	}

	if (RegsetIn(AMD64_INS_REGS_DEF(ins),reg) || RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP))
	{
	  pop = NULL;
	  continue;
	}
	
	if (RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP))
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

/*int total_count=0;*/
/* {{{ substitute instructions with smaller, equivalent forms */
void Amd64PeepHoleShorterEquivalent(t_cfg * cfg)
{
  t_bbl * bbl;
  t_amd64_ins * ins;
  t_regset condflags = NullRegs;

  RegsetSetAddReg(condflags,AMD64_CONDREG_OF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_SF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_ZF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_AF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_PF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_CF);

  /* for most substitutions, we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_AMD64_INS(bbl,ins)
    {
      t_amd64_operand * dest = AMD64_INS_DEST(ins);
      t_amd64_operand * src  = AMD64_INS_SOURCE1(ins);
      t_amd64_operand * src2 = AMD64_INS_SOURCE2(ins);

      switch (AMD64_INS_OPCODE(ins))
      {
	case AMD64_TEST:
          if(AMD64_OP_TYPE(src) == amd64_optype_imm && AMD64_OP_TYPE(src2) == amd64_optype_reg && AMD64_OP_REGMODE(src2) == amd64_regmode_full64){
	    if(AMD64_OP_IMMEDIATE(src)<4294967296LL && AMD64_OP_IMMEDIATE(src)>=0){
	      AMD64_OP_REGMODE(src2) = amd64_regmode_lo32;
	    }
	  }
	  if(AMD64_OP_TYPE(src2) == amd64_optype_imm && AMD64_OP_TYPE(src) == amd64_optype_reg && AMD64_OP_REGMODE(src) == amd64_regmode_full64){
	    if(AMD64_OP_IMMEDIATE(src2)<4294967296LL && AMD64_OP_IMMEDIATE(src2)>=0){
	      AMD64_OP_REGMODE(src) = amd64_regmode_lo32;
	    }
	  }
		      
	  break;
	case AMD64_AND:
	  if(AMD64_OP_TYPE(src) == amd64_optype_imm && AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_REGMODE(dest) == amd64_regmode_full64){
	    if(AMD64_OP_IMMEDIATE(src)<4294967296LL && AMD64_OP_IMMEDIATE(src)>=0){
	      AMD64_OP_REGMODE(dest) = amd64_regmode_lo32;
	    }
	  }
	  break;
	case AMD64_XOR:
	  if(AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src) == amd64_optype_reg)
	  {
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_full64
		&& Amd64IsGeneralPurposeReg(AMD64_OP_BASE(src)) && AMD64_OP_REGMODE(src) == amd64_regmode_full64
		&& AMD64_OP_BASE(dest) == AMD64_OP_BASE(src) )
	    {
	      AMD64_OP_REGMODE(dest)=amd64_regmode_lo32;
	      AMD64_OP_REGMODE(src)=amd64_regmode_lo32;
	    }
	  }
	  break;
	case AMD64_MOV:
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src) == amd64_optype_imm)
	  {
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && (AMD64_OP_REGMODE(dest) == amd64_regmode_full64 ))
	    {
	      if (AMD64_OP_IMMEDIATE(src) == 0 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		/* mov $0,%reg -> xor %reg,%reg */
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = AMD64_OP_BASE(dest);
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeArithmetic(ins,AMD64_XOR,d,d,0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	      else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		
		/* mov $-1,%reg -> or $-1,%reg */
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = AMD64_OP_BASE(dest);
		  /* VERBOSE(0,("SR: @I -> ",ins)); */
		  Amd64InstructionMakeArithmetic(ins,AMD64_OR,d,AMD64_REG_NONE,-1);
		  /* VERBOSE(0,("SR: <- @I ",ins)); */
		}
	      }
	      else if (AMD64_OP_IMMEDSIZE(src) == 4){
		if(AMD64_OP_IMMEDIATE(src)<2147483648LL && AMD64_OP_IMMEDIATE(src)>=0){
		  AMD64_OP_REGMODE(dest) = amd64_regmode_lo32;
		}
	      }
	      else if (AMD64_OP_IMMEDSIZE(src) == 8){
		if(AMD64_OP_IMMEDIATE(src)<4294967296LL && AMD64_OP_IMMEDIATE(src)>=0){
		  AMD64_OP_REGMODE(dest) = amd64_regmode_lo32;
		  AMD64_OP_IMMEDSIZE(src) = 4;
		}
	      }
	    }
            if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_lo32 )
	    {
	      if (AMD64_OP_IMMEDIATE(src) == 0 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		/* mov $0,%reg -> xor %reg,%reg */
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = AMD64_OP_BASE(dest);
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeArithmetic32(ins,AMD64_XOR,d,d,0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	      else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		
		/* mov $-1,%reg -> or $-1,%reg */
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  t_reg d = AMD64_OP_BASE(dest);
		  /* VERBOSE(0,("SR: @I -> ",ins)); */
		  Amd64InstructionMakeArithmetic(ins,AMD64_OR,d,AMD64_REG_NONE,-1);
		  /* VERBOSE(0,("SR: <- @I ",ins)); */
		}
	      }
	    }
	  }

	  if (AMD64_OP_TYPE(src)== amd64_optype_imm && (AMD64_OP_IMMEDSIZE(src) == 4 || AMD64_OP_IMMEDSIZE(src) == 8) && AMD64_OP_IMMEDIATE(src) == 0 && AMD64_OP_TYPE(dest)== amd64_optype_mem  && AMD64_OP_BASE(dest)!=AMD64_REG_RIP){
            if(AMD64_INS_INEXT(ins)){
              if(!Amd64InsIsConditional(AMD64_INS_INEXT(ins))){
		t_regset dead = Amd64InsRegsLiveBefore(ins);
		t_regset zero = RegsetNew(); 
		t_reg r = AMD64_REG_NONE;
		t_amd64_ins *insnew;
		t_ins *gcheck;
		t_amd64_ins *check;

		RegsetSetInvers(dead);
	
		/*check if a register had a zero value*/
		BBL_FOREACH_INS(bbl,gcheck){
		  check = T_AMD64_INS(gcheck);
		  if(check == ins)
		    break;
		  if( AMD64_INS_OPCODE(check) == AMD64_XOR && 
		      AMD64_OP_TYPE(AMD64_INS_DEST(check)) == amd64_optype_reg && 
		      AMD64_OP_TYPE(AMD64_INS_SOURCE1(check)) == amd64_optype_reg && 
		      Amd64IsGeneralPurposeReg(AMD64_OP_BASE(AMD64_INS_DEST(check))) && 
		      AMD64_OP_REGMODE(AMD64_INS_DEST(check)) == amd64_regmode_lo32 && 
		      Amd64IsGeneralPurposeReg(AMD64_OP_BASE(AMD64_INS_SOURCE1(check))) && 
		      AMD64_OP_REGMODE(AMD64_INS_SOURCE1(check)) == amd64_regmode_lo32 && 
		      AMD64_OP_BASE(AMD64_INS_DEST(check)) == AMD64_OP_BASE(AMD64_INS_SOURCE1(check)) )
		  {
		    zero=RegsetAddReg(zero,AMD64_OP_BASE(AMD64_INS_DEST(check)));
		  }else{
		    zero=RegsetDiff(zero,AMD64_INS_REGS_DEF(check));
		  }						    
		}
		REGSET_FOREACH_REG(zero,r)
		  break;
		if (!Amd64IsGeneralPurposeReg(r) || (r == AMD64_REG_RSP)){
		  REGSET_FOREACH_REG(dead,r)
		    break;
		  if (Amd64IsGeneralPurposeReg(r) && (r != AMD64_REG_RSP)){
		    insnew = T_AMD64_INS(InsNewForBbl(bbl));
		    Amd64InstructionMakeArithmetic32(insnew,AMD64_XOR,r,r,0);
		    Amd64InsInsertBefore (insnew, ins);
		  }
		}

		if (Amd64IsGeneralPurposeReg(r) && (r != AMD64_REG_RSP)){
/*	        if(total_count < diablosupport_options.debugcounter){
		  total_count++;
		  VERBOSE(0,("changed: @ieB",bbl));*/
		  AMD64_OP_TYPE(src) = amd64_optype_reg;
		  AMD64_OP_BASE(src) = r;
		  if(AMD64_OP_MEMOPSIZE(dest) == 4)
		  {
		    AMD64_OP_REGMODE(src) = amd64_regmode_lo32;
		  }
		  else if(AMD64_OP_MEMOPSIZE(dest) == 8)
		  {
		    AMD64_OP_REGMODE(src) = amd64_regmode_full64;
		  }
		  else FATAL (("unexpected memopsize"));
		  Amd64InsSetGenericInsInfo(ins);
/*		  VERBOSE(0,("into: @ieB",bbl));
		}*/
	        }
	      }
	    }
	  }
	  break;
	case AMD64_ADD:
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src) == amd64_optype_imm)
	  {
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_full64)
	    {
	      if (!RegsetIn(Amd64InsRegsLiveAfter(ins),AMD64_CONDREG_CF))
	      {
		if (AMD64_OP_IMMEDIATE(src) == 1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* add $1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec(ins,AMD64_INC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* add $-1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec(ins,AMD64_DEC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
            if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_lo32)
	    {
	      if (!RegsetIn(Amd64InsRegsLiveAfter(ins),AMD64_CONDREG_CF))
	      {
		if (AMD64_OP_IMMEDIATE(src) == 1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* add $1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec32(ins,AMD64_INC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* add $-1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec32(ins,AMD64_DEC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case AMD64_SUB:
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg && AMD64_OP_TYPE(src) == amd64_optype_imm)
	  {
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_full64)
	    {
	      if (!RegsetIn(Amd64InsRegsLiveAfter(ins),AMD64_CONDREG_CF))
	      {
		if (AMD64_OP_IMMEDIATE(src) == 1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* sub $1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec(ins,AMD64_DEC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* sub $-1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec(ins,AMD64_INC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest)) && AMD64_OP_REGMODE(dest) == amd64_regmode_lo32)
	    {
	      if (!RegsetIn(Amd64InsRegsLiveAfter(ins),AMD64_CONDREG_CF))
	      {
		if (AMD64_OP_IMMEDIATE(src) == 1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* sub $1,%reg -> dec %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec32(ins,AMD64_DEC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
		else if (AMD64_OP_IMMEDIATE(src) == -1 && (!(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)))
		{
		  /* sub $-1,%reg -> inc %reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeIncDec32(ins,AMD64_INC,AMD64_OP_BASE(dest));
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case AMD64_CMP:
	  if (AMD64_OP_TYPE(src) == amd64_optype_reg && AMD64_OP_TYPE(src2) == amd64_optype_imm)
	  {
	    if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(src)) && AMD64_OP_REGMODE(src) == amd64_regmode_full64)
	    {
	      if (AMD64_OP_IMMEDIATE(src2) == 0 && (!(AMD64_OP_FLAGS(src2) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  /* cmp $0,%reg -> test %reg,%reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeTest(ins,AMD64_OP_BASE(src),AMD64_OP_BASE(src),0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
            if (Amd64IsGeneralPurposeReg(AMD64_OP_BASE(src)) && AMD64_OP_REGMODE(src) == amd64_regmode_lo32)
            {
	      if (AMD64_OP_IMMEDIATE(src2) == 0 && (!(AMD64_OP_FLAGS(src2) & AMD64_OPFLAG_ISRELOCATED)))
	      {
		if (RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(ins),condflags))
		{
		  /* cmp $0,%reg -> test %reg,%reg */
		  /*VERBOSE(0,("SR: @I -> ",ins));*/
		  Amd64InstructionMakeTest32(ins,AMD64_OP_BASE(src),AMD64_OP_BASE(src),0);
		  /*VERBOSE(0,("@I\n",ins));*/
		}
	      }
	    }
	  }
	  break;
	case AMD64_LEA:
	  /* lea (%rega),%regb -> mov %rega,%regb */
	  if (AMD64_OP_IMMEDIATE(src) == 0 && AMD64_OP_INDEX(src) == AMD64_REG_NONE)
	  {
	    Amd64InstructionMakeMovToReg(ins,AMD64_OP_BASE(dest),AMD64_OP_BASE(src),0);
	  }
	  /* lea <abs address>,%reg -> mov $<abs address>,%reg */
	  else if (AMD64_OP_BASE(src) == AMD64_REG_NONE && AMD64_OP_INDEX(src) == AMD64_REG_NONE)
	  {
	    t_amd64_ins * extra;
	    Amd64MakeInsForIns(MovToReg,Before,extra,ins,AMD64_OP_BASE(dest),AMD64_REG_NONE,AMD64_OP_IMMEDIATE(src));
	    if (AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED)
	    {
	      RelocSetFrom(RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)),(t_relocatable *)extra);
	      AMD64_OP_FLAGS(AMD64_INS_SOURCE1(extra)) |= AMD64_OPFLAG_ISRELOCATED;
	    }
	    Amd64InsKill(ins);
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
void Amd64PeepHoleAddSub64(t_cfg * cfg)
{
  t_bbl * bbl;
  t_amd64_ins * ins, * tmp;
  t_regset condflags = NullRegs;
  
  RegsetSetAddReg(condflags,AMD64_CONDREG_OF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_SF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_ZF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_AF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_PF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_CF);
  
  /* we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  
  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_AMD64_INS_SAFE(bbl,ins,tmp)
    {
      if (AMD64_INS_OPCODE(ins) == AMD64_ADD || AMD64_INS_OPCODE(ins) == AMD64_SUB)
      {
	t_amd64_operand * dest = AMD64_INS_DEST(ins), * src = AMD64_INS_SOURCE1(ins);
	t_amd64_operand * dest2, * src2;
	
	if (AMD64_OP_TYPE(dest) == amd64_optype_reg
	    && AMD64_OP_REGMODE(dest) == amd64_regmode_full64
	    && Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest))
	    && AMD64_OP_TYPE(src) == amd64_optype_imm
	    && !(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED))
	{
	  t_regset def = AMD64_INS_REGS_DEF(ins);
	  t_reg d = AMD64_OP_BASE(dest);
	  t_bool found = FALSE;
	  t_amd64_ins * iter;
	  for (iter = AMD64_INS_INEXT(ins); iter; iter = AMD64_INS_INEXT(iter))
	  {
	    dest2 = AMD64_INS_DEST(iter);
	    src2 = AMD64_INS_SOURCE1(iter);
            if (RegsetIsMutualExclusive(AMD64_INS_REGS_USE(iter),def))

	    {
	      if (!RegsetIn(AMD64_INS_REGS_DEF(iter),d))
		continue;       /* look further down */
	      else
		break;          /* d overwritten: stop */
	    }
	    /* should be add or sub */
	    if (AMD64_INS_OPCODE(iter) == AMD64_ADD || AMD64_INS_OPCODE(iter) == AMD64_SUB)
	    {
	      if (AMD64_OP_TYPE(dest2) == amd64_optype_reg
		  && AMD64_OP_REGMODE(dest2) == amd64_regmode_full64
		  && AMD64_OP_BASE(dest2) == d
		  && AMD64_OP_TYPE(src2) == amd64_optype_imm
		  && !(AMD64_OP_FLAGS(src2) & AMD64_OPFLAG_ISRELOCATED)
		  && RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(iter),condflags))
		found = TRUE;
	    }
	    break;
	  }
	  
	  /* merge the two instructions */
	  if (found)
	  {
	    t_amd64_opcode opcode = AMD64_INS_OPCODE(iter);
	    t_uint64 imm;
	    /*VERBOSE(0,("AS: will merge @I and @I\n",ins,iter));*/
	    /*VERBOSE(0,("in @iB",INS_BBL(ins)));*/
	    imm = AMD64_OP_IMMEDIATE(src2) +
	      (AMD64_INS_OPCODE(iter) == AMD64_INS_OPCODE(ins) ? 1 : -1)*AMD64_OP_IMMEDIATE(src);
	    if (imm == 128)
	    {
	      /* 128 is encoded in 4 bytes, -128 in 1 byte */
	      opcode = (opcode == AMD64_SUB) ? AMD64_ADD : AMD64_SUB;
	      imm = -imm;
	    }
	    AMD64_INS_SET_OPCODE(iter, opcode);
	    AMD64_OP_IMMEDIATE(src2) = imm;
	    Amd64InsKill(ins);
	    /*VERBOSE(0,("--> @I\n",iter));*/
	  }
	}
      }
    }
  }
}

void Amd64PeepHoleAddSub32(t_cfg * cfg)
{
  t_bbl * bbl;
  t_amd64_ins * ins, * tmp;
  t_regset condflags = NullRegs;

  RegsetSetAddReg(condflags,AMD64_CONDREG_OF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_SF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_ZF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_AF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_PF);
  RegsetSetAddReg(condflags,AMD64_CONDREG_CF);

  /* we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_AMD64_INS_SAFE(bbl,ins,tmp)
    {
      if (AMD64_INS_OPCODE(ins) == AMD64_ADD || AMD64_INS_OPCODE(ins) == AMD64_SUB)
      {
	t_amd64_operand * dest = AMD64_INS_DEST(ins), * src = AMD64_INS_SOURCE1(ins);
	t_amd64_operand * dest2, * src2;

	if (AMD64_OP_TYPE(dest) == amd64_optype_reg 
	    && AMD64_OP_REGMODE(dest) == amd64_regmode_lo32 
	    && Amd64IsGeneralPurposeReg(AMD64_OP_BASE(dest))
	    && AMD64_OP_TYPE(src) == amd64_optype_imm 
	    && !(AMD64_OP_FLAGS(src) & AMD64_OPFLAG_ISRELOCATED))
	{
	  t_regset def = AMD64_INS_REGS_DEF(ins);
	  t_reg d = AMD64_OP_BASE(dest);
          t_bool found = FALSE;
	  t_amd64_ins * iter;
	  for (iter = AMD64_INS_INEXT(ins); iter; iter = AMD64_INS_INEXT(iter))
	  {
	    dest2 = AMD64_INS_DEST(iter);
	    src2 = AMD64_INS_SOURCE1(iter);

	    if (RegsetIsMutualExclusive(AMD64_INS_REGS_USE(iter),def))
	    {
	      if (!RegsetIn(AMD64_INS_REGS_DEF(iter),d))
		continue;	/* look further down */
	      else
		break;		/* d overwritten: stop */
	    }
	    /* should be add or sub */
	    if (AMD64_INS_OPCODE(iter) == AMD64_ADD || AMD64_INS_OPCODE(iter) == AMD64_SUB)
	    {
	      if (AMD64_OP_TYPE(dest2) == amd64_optype_reg 
		  && AMD64_OP_REGMODE(dest2) == amd64_regmode_lo32 
		  && AMD64_OP_BASE(dest2) == d
		  && AMD64_OP_TYPE(src2) == amd64_optype_imm 
		  && !(AMD64_OP_FLAGS(src2) & AMD64_OPFLAG_ISRELOCATED)
		  && RegsetIsMutualExclusive(Amd64InsRegsLiveAfter(iter),condflags))
		found = TRUE;
	    }
	    break;
	  }

	  /* merge the two instructions */
	  if (found)
	  {
	    t_amd64_opcode opcode = AMD64_INS_OPCODE(iter);
	    t_uint64 imm;
	    /*VERBOSE(0,("AS: will merge @I and @I\n",ins,iter));*/
	    /*VERBOSE(0,("in @iB",INS_BBL(ins)));*/
	    imm = AMD64_OP_IMMEDIATE(src2) + 
	         (AMD64_INS_OPCODE(iter) == AMD64_INS_OPCODE(ins) ? 1 : -1)*AMD64_OP_IMMEDIATE(src);
	    if (imm == 128)
	    {
	      /* 128 is encoded in 4 bytes, -128 in 1 byte */
	      opcode = (opcode == AMD64_SUB) ? AMD64_ADD : AMD64_SUB;
	      imm = -imm;
	    }
	    AMD64_INS_SET_OPCODE(iter, opcode);
	    AMD64_OP_IMMEDIATE(src2) = imm;
	    Amd64InsKill(ins);
	    /*VERBOSE(0,("--> @I\n",iter));*/
	  }
	}
      }
    }
  }
}

void Amd64PeepHoleAddSub(t_cfg * cfg){
  /*add/sub's with 32bit operands*/
  Amd64PeepHoleAddSub32(cfg);
  /*add/sub's with 64bit operands*/
  Amd64PeepHoleAddSub64(cfg);
}
    //      
/* }}} */

/* {{{ if possible, remove frame pointer set/reset from one-block functions 
 * this helps for simple function inlining */
void Amd64PeepHoleFramePointerRemoval(t_cfg * cfg)
{
  t_function * fun;
  /*static int count=0;*/

  Amd64FramePointerAnalysis(cfg);
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_bbl * first = FUNCTION_BBL_FIRST(fun);
    t_amd64_ins * ins, * prologue, * epilogue;
    int height = 0;
    t_bool can_remove = TRUE;

    if (!(FUNCTION_FLAGS(fun) & FF_HAS_FRAMEPOINTER)) continue;
    if (!first) continue;
    if (BBL_NEXT_IN_FUN(first) && BBL_NEXT_IN_FUN(first) != FunctionGetExitBlock(fun)) continue;

    /* find frame pointer set/reset prologue and epilogue */
    ins = T_AMD64_INS(BBL_INS_FIRST(first));
    if (AMD64_INS_OPCODE(ins) != AMD64_PUSH || 
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg ||
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) != AMD64_REG_RBP ||
	AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) != amd64_regmode_full64)
      continue;
    ins = AMD64_INS_INEXT(ins);
    if (AMD64_INS_OPCODE(ins) != AMD64_MOV ||
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_reg ||
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) != AMD64_REG_RSP ||
	AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) != amd64_regmode_full64 ||
	AMD64_OP_TYPE(AMD64_INS_DEST(ins)) != amd64_optype_reg ||
	AMD64_OP_BASE(AMD64_INS_DEST(ins)) != AMD64_REG_RBP ||
	AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) != amd64_regmode_full64)
      continue;
    prologue = AMD64_INS_INEXT(ins);

    ins = T_AMD64_INS(BBL_INS_LAST(first));
    if (AMD64_INS_OPCODE(ins) != AMD64_RET)
      continue;
    ins = AMD64_INS_IPREV(ins);
    if (!(
	  (AMD64_INS_OPCODE(ins) == AMD64_LEAVE) || 
	  (AMD64_INS_OPCODE(ins) == AMD64_POP &&
	   AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	   AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP &&
	   AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64)
	  ))
      continue;
    ins = AMD64_INS_IPREV(ins);
    if (AMD64_INS_OPCODE(ins) == AMD64_MOV &&
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg &&
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RBP &&
	(AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) == amd64_regmode_full64 ||
	 AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) == amd64_regmode_lo32
	)&&
	AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP &&
	(AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64 ||
	 AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo32
	))
      ins = AMD64_INS_IPREV(ins);
    epilogue = AMD64_INS_INEXT(ins);

    /* check if we can really remove the frame pointer */
    for (ins = prologue; ins != epilogue; ins = AMD64_INS_INEXT(ins))
    {
      if (RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RBP) ||
	  RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RBP))
      {
	can_remove = FALSE;
	break;
      }
      if (RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP) ||
	  RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP))
      {
	if (AMD64_INS_OPCODE(ins) == AMD64_PUSH)
	  height += 4;
	else if (AMD64_INS_OPCODE(ins) == AMD64_POP)
	  height -= 4;
	else if (AMD64_INS_OPCODE(ins) == AMD64_SUB &&
	    AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	    AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP &&
	    (AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64 ||
	     AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo32)&&
	    AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
	  height += AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins));
	else if (AMD64_INS_OPCODE(ins) == AMD64_ADD &&
	    AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	    AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP &&
	    (AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64 ||
	     AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo32)&&
	    AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
	  height -= AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins));
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
      while (AMD64_INS_IPREV(prologue))
	Amd64InsKill(AMD64_INS_IPREV(prologue));
      ins = AMD64_INS_IPREV(epilogue);
      while (AMD64_INS_IPREV(T_AMD64_INS(BBL_INS_LAST(first))) != ins)
	Amd64InsKill(AMD64_INS_IPREV(T_AMD64_INS(BBL_INS_LAST(first))));
      /* compensate for unbalanced stack */
      if (height != 0)
	Amd64MakeInsForIns(Arithmetic,Before,ins,T_AMD64_INS(BBL_INS_LAST(first)),AMD64_ADD,AMD64_REG_RSP,AMD64_REG_NONE,height);
      VERBOSE (1, ("after @ieB", first));
    }

  }
  
  Amd64FramePointerAnalysis(cfg);
}
/* }}} */

/* replace add/sub $8,%rsp with a (smaller) pop or push */
void Amd64PeepHoleAddSubRSP(t_cfg * cfg)
{
  t_bbl *bbl;
  t_amd64_ins *ins;
  /* we need accurate liveness information */
  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    BBL_FOREACH_AMD64_INS(bbl,ins)
    {
      t_amd64_operand *dest = AMD64_INS_DEST(ins);
      t_amd64_operand *src = AMD64_INS_SOURCE1(ins);
      if (AMD64_INS_OPCODE(ins) == AMD64_ADD
	  && AMD64_OP_TYPE(dest) == amd64_optype_reg
	  && AMD64_OP_BASE(dest) == AMD64_REG_RSP
	  && AMD64_OP_TYPE(src) == amd64_optype_imm
	  && AMD64_OP_IMMEDIATE(src) == 8)
      {
	t_regset dead = Amd64InsRegsLiveAfter(ins);
	t_reg r;
	RegsetSetInvers(dead);
	REGSET_FOREACH_REG(dead,r)
	  break;
	if (Amd64IsGeneralPurposeReg(r) && (r != AMD64_REG_RSP))
	{
	  Amd64InstructionMakePop(ins,r);
/*	  VERBOSE(0,("+_+_+ MADE @I\n",ins));*/
	}
      }
      else if (AMD64_INS_OPCODE(ins) == AMD64_SUB
	  && AMD64_OP_TYPE(dest) == amd64_optype_reg
	  && AMD64_OP_BASE(dest) == AMD64_REG_RSP
	  && AMD64_OP_TYPE(src) == amd64_optype_imm
	  && AMD64_OP_IMMEDIATE(src) == 8)
      {
	/* push some live register instead */
	t_regset live = Amd64InsRegsLiveAfter(ins);
	t_reg r;
	REGSET_FOREACH_REG(live,r)
	  break;
	if (Amd64IsGeneralPurposeReg(r))
	  Amd64InstructionMakePush(ins,r,0);
	else
	{
	  Amd64InstructionMakePush(ins,AMD64_REG_NONE,0);
	  AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins)) = 1;		/* push $0 */
	}
/*	VERBOSE(0,("+_+_+ MADE @I\n",ins));*/
      }
    }
  }
}

/* remove xor r,r test r,r jne of  xor r,r test r,r, je {{{*/
void Amd64PeepHoleXorTestJC(t_cfg * cfg){
  t_bbl *bbl;
  t_amd64_ins *ins;
  t_reg r;
		  
  CFG_FOREACH_BBL(cfg,bbl)
  {
    ins=T_AMD64_INS(BBL_INS_LAST(bbl));
    if(ins){
      if(!(AMD64_INS_OPCODE(ins) == AMD64_Jcc && (AMD64_INS_CONDITION(ins) == AMD64_CONDITION_Z || AMD64_INS_CONDITION(ins) == AMD64_CONDITION_NZ)))
	continue;
    }else
      continue;
    
    ins=AMD64_INS_IPREV(ins);
    
    if(ins){
      t_amd64_operand * dest = AMD64_INS_SOURCE2(ins);
      t_amd64_operand * src  = AMD64_INS_SOURCE1(ins);
      if( AMD64_INS_OPCODE(ins) == AMD64_TEST && 
	  AMD64_OP_TYPE(dest) == amd64_optype_reg &&
	  AMD64_OP_TYPE(src) == amd64_optype_reg &&
	  AMD64_OP_BASE(dest) == AMD64_OP_BASE(src) &&
	  (
	   AMD64_OP_REGMODE(src) == amd64_regmode_full64 ||
	   AMD64_OP_REGMODE(src) == amd64_regmode_lo32
	  )
	  ){
	r = AMD64_OP_BASE(src);
      }else
	continue;
    }else
      continue;

    ins=AMD64_INS_IPREV(ins);
    if(ins){
      t_amd64_operand * dest = AMD64_INS_DEST(ins);
      t_amd64_operand * src  = AMD64_INS_SOURCE1(ins);
      if(!(AMD64_INS_OPCODE(ins) == AMD64_XOR &&
	  AMD64_OP_TYPE(dest) == amd64_optype_reg &&
	  AMD64_OP_TYPE(src) == amd64_optype_reg &&
	  AMD64_OP_BASE(dest) == AMD64_OP_BASE(src) &&
	  AMD64_OP_BASE(dest) == r &&
	  (
	   AMD64_OP_REGMODE(src) == amd64_regmode_full64 ||
	   AMD64_OP_REGMODE(src) == amd64_regmode_lo32
	  )
	))
	continue;
    }else
      continue;

    ins=T_AMD64_INS(BBL_INS_LAST(bbl));
    if(AMD64_INS_CONDITION(ins) == AMD64_CONDITION_Z){
      t_cfg_edge *edge,*tempedge;
      t_amd64_ins *temp=AMD64_INS_IPREV(ins);
      Amd64InsKill(temp);
      AMD64_INS_SET_OPCODE(ins,AMD64_JMP);
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_NONE);
      BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,tempedge){
	if(CFG_EDGE_CAT(edge)==ET_FALLTHROUGH)
	  CfgEdgeKill(edge);
      }
    }else{
      t_cfg_edge *edge,*tempedge;
      t_amd64_ins *temp=ins;
      ins=AMD64_INS_IPREV(ins);
      Amd64InsKill(temp);
      temp=ins;
      Amd64InsKill(temp);
      BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,tempedge){
	if(CFG_EDGE_CAT(edge)==ET_JUMP)
	  CfgEdgeKill(edge);	  
      }
    }
  }
				    
}
/*}}}*/

void Amd64PeepHoles(t_cfg * cfg)
{
  Amd64PeepHoleFramePointerRemoval(cfg);
  Amd64PeepHoleStack(cfg);
  Amd64PeepHoleRedundantPushes(cfg); 
  Amd64PeepHoleShorterEquivalent(cfg); 
  Amd64PeepHoleAddSub(cfg); 
  Amd64PeepHoleIdempotent(cfg);  
  Amd64PeepHoleAddSubRSP(cfg);
  Amd64PeepHoleXorTestJC(cfg);
}

/* vim: set shiftwidth=2 foldmethod=marker : */
