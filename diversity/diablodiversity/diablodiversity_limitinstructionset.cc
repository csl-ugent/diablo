/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

#ifdef __cplusplus
extern "C" {
#endif
#include <diabloanopti386.h>
#include <diablodiversity.h>
#ifdef __cplusplus
}
#endif

#define dlis_CopyAddresses(ins1, ins2) \
  AddressListAddList(INS_ADDRESSLIST(T_INS(ins1)), INS_ADDRESSLIST(T_INS(ins2)));


/*
   HORRIBLE HACK because the original __VA_ARGS__ doesn't seem to work in MSVC!?!
Originally, it was a SINGLE define:
#define dlis_I386MakeInsForIns(type,where,ins,existing, ...)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, ## __VA_ARGS__);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)
*/

#define dlis_I386MakeInsForIns_1(type,where,ins,existing, arg1)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)
#define dlis_I386MakeInsForIns_2(type,where,ins,existing, arg1, arg2)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)
#define dlis_I386MakeInsForIns_3(type,where,ins,existing, arg1, arg2, arg3)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)
#define dlis_I386MakeInsForIns_4(type,where,ins,existing, arg1, arg2, arg3, arg4)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)

#define dlis_I386MakeInsForIns_5(type,where,ins,existing, arg1, arg2, arg3, arg4, arg5)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4, arg5);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)


#define dlis_I386MakeInsForIns_6(type,where,ins,existing, arg1, arg2, arg3, arg4, arg5, arg6)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4, arg5, arg6);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)

#define dlis_I386MakeInsForIns_7(type,where,ins,existing, arg1, arg2, arg3, arg4, arg5, arg6, arg7)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4, arg5, arg6, arg7);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)

#define dlis_I386MakeInsForIns_8(type,where,ins,existing, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)

#define dlis_I386MakeInsForIns_9(type,where,ins,existing, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)     \
  do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    dlis_I386InstructionMake ## type(ins, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    dlis_CopyAddresses((ins), (existing));                  \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
  } while (0)


dlis_reg_list full32reglist, lo16reglist, hi8reglist, lo8reglist;
dlis_graph dep_graph = NULL;
int opcode_count_array[MAX_I386_OPCODE];

/*
 * Utility functions
 */

/* Register related */
t_reg FindFreeRegister(t_i386_ins *ins)/*{{{*/
{
  t_regset regset;
  t_reg reg = I386_REG_NONE;

  regset = InsRegsLiveBefore((t_ins *) ins);

  if(!RegsetIn(regset, I386_REG_EAX))
  {
    reg = I386_REG_EAX;
  }
  else if(!RegsetIn(regset, I386_REG_EBX))
  {
    reg = I386_REG_EBX;
  }
  else if(!RegsetIn(regset, I386_REG_ECX))
  {
    reg = I386_REG_ECX;
  }
  else if(!RegsetIn(regset, I386_REG_EDX))
  {
    reg = I386_REG_EDX;
  }
  else if(!RegsetIn(regset, I386_REG_EDI))
  {
    reg = I386_REG_EDI;
  }
  else if(!RegsetIn(regset, I386_REG_ESI))
  {
    reg = I386_REG_ESI;
  }
  else if(!RegsetIn(regset, I386_REG_EBP))
  {
    reg = I386_REG_EBP;
  }
  else
  {
    reg = I386_REG_NONE;
  }

  return reg;
}/*}}}*/

t_reg FindFreeRegisterForRegmode(t_i386_ins *ins, t_i386_regmode regmode) /*{{{*/
{
  switch (regmode)
  {
    case i386_regmode_full32:
      return DlisRegListFindFreeRegister(full32reglist, ins);
    case i386_regmode_lo16:
      return DlisRegListFindFreeRegister(lo16reglist, ins);
    case i386_regmode_hi8:
      return DlisRegListFindFreeRegister(hi8reglist, ins);
    case i386_regmode_lo8:
      return DlisRegListFindFreeRegister(lo8reglist, ins);
    default:
      FATAL(("Unsupported regmode."));
  }
} /*}}}*/

t_reg ChooseRandomWithout(dlis_reg_list l, t_i386_regmode regmode) /*{{{*/
{
  switch (regmode)
  {
    case i386_regmode_full32:
      return DlisRegListRandomWithout(full32reglist, l);
    case i386_regmode_lo16:
      return DlisRegListRandomWithout(lo16reglist, l);
    case i386_regmode_lo8:
      return DlisRegListRandomWithout(lo8reglist, l);
    case i386_regmode_hi8:
      return DlisRegListRandomWithout(hi8reglist, l);
    default:
      FATAL(("Unknown regmode."));
  }
} /*}}}*/

/* Register related: dlis_reg_list */
dlis_reg_list DlisRegList(int length)/*{{{*/
{
  int i;
  dlis_reg_list drl = (dlis_reg_list) Malloc(sizeof(*drl));
  drl->length = length;
  drl->index = 0;
  drl->list = (t_reg*) Calloc(drl->length, sizeof(t_reg));
  for (i = 0; i < drl->length; i++)
  {
    drl->list[i] = I386_REG_NONE;
  }
  return drl;
}/*}}}*/

void DlisRegListAdd(dlis_reg_list self, t_reg reg) /*{{{*/
{
  /* Yes, I am a Python-addict */
  if (self->index < self->length)
  {
    self->list[self->index] = reg;
    self->index += 1;
  }
  else
  {
    FATAL(("Trying to add a register to a full dlis_reg_list."));
  }
}/*}}}*/

void DlisRegListSafeAdd(dlis_reg_list self, t_reg reg) /*{{{*/
{
  int i;
  if (reg == I386_REG_NONE)
  {
    return;
  }
  for (i=0; i < self->index; i++)
  {
    if (self->list[i] == reg)
    {
      return;
    }
  }
  DlisRegListAdd(self, reg);
} /*}}}*/

void DlisRegListDestruct(dlis_reg_list self)/*{{{*/
{
  if (self)
  {
    Free(self->list);
    Free(self);
  }
}/*}}}*/

t_reg ChooseRandomRegisterFrom(t_reg *f, int length) /*{{{*/
{
  return f[diablo_rand_next()%length];
}/*}}}*/

t_reg ChooseRandomRegister() /*{{{*/
{
  t_reg choices[7] = {I386_REG_EAX, I386_REG_EBX, I386_REG_ECX, I386_REG_EDX, I386_REG_EDI, I386_REG_ESI, I386_REG_EBP};

  return ChooseRandomRegisterFrom(choices, 7);
}/*}}}*/

t_reg ChooseRandomRegisterWithout(t_reg *wo, int length) /*{{{*/
{
  int choice_length = 7;
  int choice_end = choice_length - 1;
  t_reg choices[7] = {I386_REG_EAX, I386_REG_EBX, I386_REG_ECX, I386_REG_EDX, I386_REG_EDI, I386_REG_ESI, I386_REG_EBP};
  int i, j;

  for (i = 0; i < length; i++)
  {
    for(j=0; j < choice_length; j++)
    {
      if (wo[i] == choices[j])
      {
	choices[j] = choices[choice_end];
	choices[choice_end] = I386_REG_NONE;
	choice_end -= 1;
	break;
      }
    }
  }
  ASSERT((choice_end + 1) == (choice_length - length), ("Double registers in wo?"));
  return ChooseRandomRegisterFrom(choices, choice_end + 1);
}/*}}}*/

t_bool DlisRegListContainsRegister(dlis_reg_list self, t_reg reg) /*{{{*/
{
  int i;
  for (i = 0; i < self->length; i++)
  {
    if (self->list[i] == reg)
    {
      return TRUE;
    }
  }
  return FALSE;
} /*}}}*/

void DlisRegListAddFromOp(dlis_reg_list self, t_i386_operand *op) /*{{{*/
{
  /* Filter out empty existing ops */
  if (!op)
  {
    return;
  }

  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      DlisRegListSafeAdd(self, I386_OP_BASE(op));
      break;
    case i386_optype_mem:
      DlisRegListSafeAdd(self, I386_OP_BASE(op));
      DlisRegListSafeAdd(self, I386_OP_INDEX(op));
      break;
    default:
      break;
  }
} /*}}}*/

void DlisRegListAddFromIns(dlis_reg_list self, t_i386_ins *ins) /*{{{*/
{
  /* Fill from the destination operand, then source1, then source2 */
  DlisRegListAddFromOp(self, I386_INS_DEST(ins));
  DlisRegListAddFromOp(self, I386_INS_SOURCE1(ins));
  DlisRegListAddFromOp(self, I386_INS_SOURCE2(ins));
}/*}}}*/

t_reg DlisRegListRandomWithout(dlis_reg_list self, dlis_reg_list wo) /*{{{*/
{
  int i, j;
  t_bool flag;
  t_reg reg;
  dlis_reg_list filtered = DlisRegList(self->index);
  for (i=0; i < self->index; i++)
  {
    flag = FALSE;
    /* Check for each register in self if the register is in wo
     * if it isn't, add it to filtered. */
    for (j=0; j < wo->index; j++)
    {
      if (self->list[i] == wo->list[j])
      {
	flag = TRUE;
	break;
      }
    }
    if (!flag)
    {
      DlisRegListAdd(filtered, self->list[i]);
    }
  }

  reg = ChooseRandomRegisterFrom(filtered->list, filtered->index);
  DlisRegListDestruct(filtered);
  return reg;
}/*}}}*/

void DlisRegListPrint(dlis_reg_list self) /*{{{*/
{
  int i;
  VERBOSE(0, ("DlisRegList %d", self));
  VERBOSE(0, ("  Length: %d", self->length));
  VERBOSE(0, ("  Index: %d", self->index));
  for (i=0; i < self->index; i++)
  {
    VERBOSE(0, ("  Item %d: %d", i, self->list[i]));
  }
  VERBOSE(0, ("  ------"));
  for (i = self->index; i < self->length; i++)
  {
    VERBOSE(0, ("  Item %d: %d", i, self->list[i]));
  }
} /*}}}*/

t_reg DlisRegListRandomRegister(dlis_reg_list self) /*{{{*/
{
  return ChooseRandomRegisterFrom(self->list, self->index);
} /*}}}*/

/*t_reg DlisRegListRandomRegisterForRegmode(t_i386_ins *ins, t_i386_regmode regmode) [>{{{<]
  {
  switch (regmode)
  {
  case i386_regmode_full32:
  return DlisRegListRandomRegister(full32reglist);
  case i386_regmode_lo16:
  return DlisRegListRandomRegister(lo16reglist);
  case i386_regmode_hi8:
  return DlisRegListRandomRegister(hi8reglist);
  case i386_regmode_lo8:
  return DlisRegListRandomRegister(lo8reglist);
  default:
  FATAL(("Unsupported regmode."));
  }
  } [>}}}<]*/

t_reg DlisRegListFindFreeRegister(dlis_reg_list self, t_i386_ins *ins) /*{{{*/
{
  int i;
  t_regset regset;

  regset = InsRegsLiveBefore((t_ins *) ins);

  for (i = 0 ; i < self->index ; i++)
  {
    if (!RegsetIn(regset, self->list[i]))
    {
      return self->list[i];
    }
  }
  return I386_REG_NONE;
} /*}}}*/

/* Histogram related */
dlis_histogram DlisHistogram(int length) /*{{{*/
{
  dlis_histogram self = (dlis_histogram) Malloc(sizeof(*self));
  self->length = length;
  self->index = 0;
  self->opcode_list = (dlis_histogram_item*) Calloc(MAX_I386_OPCODE, sizeof(dlis_histogram_item));
  memset(self->opcode_list, 0, MAX_I386_OPCODE*sizeof(dlis_histogram_item));
  self->list = (dlis_histogram_item*) Calloc(length, sizeof(dlis_histogram_item));
  return self;
} /*}}}*/

void DlisHistogramDestruct(dlis_histogram self) /*{{{*/
{
  int i;
  for (i = 0; i < self->index; i++)
  {
    DlisHistogramItemDestruct(self->list[i]);
  }
  Free(self->opcode_list);
  Free(self->list);
  Free(self);
} /*}}}*/

int __helper_order_histogram(const void *a, const void *b) /*{{{*/
{
  dlis_histogram_item item_a = *((dlis_histogram_item*) a);
  dlis_histogram_item item_b = *((dlis_histogram_item*) b);
  return (item_a->count == item_b->count) ? 0 : 
    ((item_a->count > item_b->count) ? -1 : 1);
} /*}}}*/

void DlisHistogramSort(dlis_histogram self) /*{{{*/
{
  diablo_stable_sort (self->list, self->index, sizeof(dlis_histogram_item), __helper_order_histogram);
} /*}}}*/

void DlisHistogramPrint(dlis_histogram self) /*{{{*/
{
  int i;
  VERBOSE(0, ("Histogram (%d/%d opcodes):", self->index, self->length));
  for (i=0; i < self->index; i++)
  {
    DlisHistogramItemPrint(DlisHistogramGet(self, i));
  }
} /*}}}*/

dlis_histogram_item DlisHistogramGet(dlis_histogram self, int i) /*{{{*/
{
  if (i >= self->index && (-1*i) > self->index)
  {
    FATAL(("Histogram index out of range"));
  }
  else if (i >= 0)
  {
    return self->list[i];
  }
  else
  {
    /* Yay, negative indexing! */
    return self->list[self->index + i];
  }
} /*}}}*/

t_i386_opcode DlisHistogramGetOpcode(dlis_histogram self, int i) /*{{{*/
{
  return DlisHistogramGet(self, i)->opcode;
} /*}}}*/

int DlisHistogramGetSize(dlis_histogram self) /*{{{*/
{
  return self->index;
} /*}}}*/

dlis_opcode_list DlisHistogramGetOpcodeSlice(dlis_histogram self, int begin, int end) /*{{{*/
{
  int i;
  dlis_opcode_list ret = DlisOpcodeList(self->length);
  if (0 <= begin && begin < self->index && 0 <= end && end < self->index &&
      begin <= end)
  {
    /* Both are positive and within range */
    for (i=begin; i <= end; i++)
    {
      DlisOpcodeListAdd(ret, DlisHistogramGetOpcode(self, i));
    }
    return ret;
  }
  else if ((-1*self->index) <= begin && begin < 0 && (-1*self->index) <= end &&
      end < 0 && end <= begin)
  {
    /* Both are negative and within range */
    for (i=begin; i >= end; i--)
    {
      DlisOpcodeListAdd(ret, DlisHistogramGetOpcode(self, i));
    }
    return ret;
  }
  else
  {
    FATAL(("Indexes out of bounds. Mixtures of positive and negative indexes are not supported!"));
  }
} /*}}}*/

dlis_histogram_item DlisHistogramGetForOpcode(dlis_histogram self, t_i386_opcode opcode) /*{{{*/
{
  if (self->opcode_list[opcode] != 0)
  {
    return (dlis_histogram_item) self->opcode_list[opcode];
  }
  else
  {
    /* Need to add a dlis_histogram_item for this opcode */
    DlisHistogramAdd(self, opcode);
    ASSERT(self->opcode_list[opcode], ("Histogram Item should be in opcode_list."));
    return self->opcode_list[opcode];
  }
} /*}}}*/

void DlisHistogramAdd(dlis_histogram self, t_i386_opcode opcode) /*{{{*/
{
  dlis_histogram_item item;
  /* Add to list and opcode_list */
  if (self->index == self->length)
  {
    FATAL(("Histogram full!"));
  }
  item = DlisHistogramItem(opcode, 0, NULL);
  self->list[self->index] = item;
  self->index += 1;
  self->opcode_list[opcode] = item;
} /*}}}*/

dlis_histogram_item DlisHistogramItem(t_i386_opcode opcode, int count, dlis_ins_list_item list) /*{{{*/
{
  /* Note: list could be NULL */
  dlis_histogram_item self = (dlis_histogram_item) Malloc(sizeof(*self));
  self->opcode = opcode;
  self->count = count;
  self->list = list;
  return self;
} /*}}}*/

void DlisHistogramItemDestruct(dlis_histogram_item self) /*{{{*/
{
  DlisInsListItemDestructRecursive(self->list);
  Free(self);
} /*}}}*/

void DlisHistogramItemAddIns(dlis_histogram_item self, t_i386_ins *ins) /*{{{*/
{
  dlis_ins_list_item l_item = DlisInsListItem(ins, NULL, self->list);
  if (self->list)
  {
    self->list->previous = l_item;
  }
  self->list = l_item;
  self->count += 1;
} /*}}}*/

void DlisHistogramItemPrint(dlis_histogram_item self) /*{{{*/
{
  VERBOSE(0, ("Opcode: %s:\t\t%d instructions", i386_opcode_table[self->opcode].textual, self->count));
} /*}}}*/

t_i386_opcode DlisHistogramItemGetOpcode(dlis_histogram_item self) /*{{{*/
{
  return self->opcode;
} /*}}}*/

dlis_ins_list_item DlisInsListItem(t_i386_ins *ins, dlis_ins_list_item previous, dlis_ins_list_item next) /*{{{*/
{
  dlis_ins_list_item self = (dlis_ins_list_item) Malloc(sizeof(*self));
  self->ins = ins;
  self->previous = previous;
  self->next = next;
  return self;
} /*}}}*/

void DlisInsListItemDestruct(dlis_ins_list_item self) /*{{{*/
{
  Free(self);
} /*}}}*/

void DlisInsListItemDestructRecursive(dlis_ins_list_item self) /*{{{*/
{
  /* Only in ->next direction... */
  dlis_ins_list_item a, b;
  for (b = self; b; b = a)
  {
    a = DlisInsListItemNext(b);
    DlisInsListItemDestruct(b);
  }
} /*}}}*/

dlis_ins_list_item DlisInsListItemNext(dlis_ins_list_item self) /*{{{*/
{
  return self->next;
} /*}}}*/

/* Histogram related: DlisOpcodeList */
dlis_opcode_list DlisOpcodeList(int length) /*{{{*/
{
  dlis_opcode_list self = (dlis_opcode_list) Malloc(sizeof(*self));
  self->length = length;
  self->index = 0;
  self->list = (t_i386_opcode*) Calloc(length, sizeof(t_i386_opcode));
  return self;
} /*}}}*/

void DlisOpcodeListAdd(dlis_opcode_list self, t_i386_opcode opcode) /*{{{*/
{
  if (self->index == self->length)
  {
    FATAL(("DlisOpcodeList index out of bounds"));
  }
  self->list[self->index] = opcode;
  self->index += 1;
} /*}}}*/

int DlisOpcodeListGetSize(dlis_opcode_list self) /*{{{*/
{
  return self->index;
} /*}}}*/

t_i386_opcode DlisOpcodeListGet(dlis_opcode_list self, int i) /*{{{*/
{
  ASSERT(0 <= i && i <= self->index, 
      ("Opcode list index out of bounds"));
  return self->list[i];
} /*}}}*/

t_bool DlisOpcodeListIn(dlis_opcode_list self, t_i386_opcode opcode) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    if (self->list[i] == opcode)
    {
      return TRUE;
    }
  }
  return FALSE;
} /*}}}*/

/* Graph related: DlisGraph */
dlis_graph DlisGraph(dlis_opcode_node *opcode_node_list, int length) /*{{{*/
{
  dlis_graph self = (dlis_graph) Malloc(sizeof(*self));
  self->length = length;
  self->index = 0;
  self->opcode_node_list = opcode_node_list;
  self->node_list = (dlis_graph_node*) Calloc(length, sizeof(dlis_graph_node*));
  return self;
} /*}}}*/

dlis_graph_node DlisGraphAddNode(dlis_graph self, t_i386_opcode opcode) /*{{{*/
{
  dlis_graph_node ret;
  /* Boundscheck */
  if (self->index == self->length)
  {
    FATAL(("Graph node list full."));
  }
  /* Keep opcode_node_list up to date */
  if (!self->opcode_node_list[opcode])
  {
    self->opcode_node_list[opcode] = DlisOpcodeNode(opcode);
  }
  /* Add a graph node, pointing to the opcode_node */
  ret = DlisGraphNode(self->opcode_node_list[opcode]);
  self->node_list[self->index] = ret;
  self->index += 1;
  return ret;
} /*}}}*/

void DlisGraphAddEdge(dlis_graph self, t_i386_opcode from, t_i386_opcode to) /*{{{*/
{
  dlis_graph_node from_node, to_node;
  ASSERT(self->opcode_node_list[from], ("Trying to make a connection from a non-existing node."));
  ASSERT(self->opcode_node_list[to], ("Trying to make a connection to a non-existing node."));
  from_node = DlisGraphGet(self, from);
  to_node = DlisGraphGet(self, to);
  DlisGraphNodeAddOutgoing(from_node, to_node);
} /*}}}*/

dlis_graph_node DlisGraphGet(dlis_graph self, t_i386_opcode opcode) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    if (self->node_list[i]->opcode_node->opcode == opcode)
    {
      return self->node_list[i];
    }
  }
  FATAL(("No graph node found for opcode %s.", i386_opcode_table[opcode].textual));
} /*}}}*/

void DlisGraphClearColors(dlis_graph self) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    DlisGraphNodeClearColor(self->node_list[i]);
  }
} /*}}}*/

void DlisGraphClearUseCounts(dlis_graph self) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    DlisGraphNodeClearUseCount(self->node_list[i]);
  }
} /*}}}*/

void DlisGraphClearRecursionFlags(dlis_graph self) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    DlisGraphNodeClearRecursionFlag(self->node_list[i]);
  }
} /*}}}*/

t_bool DlisGraphHasNode(dlis_graph self, t_i386_opcode opcode) /*{{{*/
{
  int i;
  for (i=0; i<self->index; i++)
  {
    if (self->node_list[i]->opcode_node->opcode == opcode)
    {
      return TRUE;
    }
  }
  return FALSE;
} /*}}}*/

dlis_graph DlisGraphCreateTree(dlis_graph self, dlis_graph_node root) /*{{{*/
{
  dlis_graph_node new_root;
  dlis_graph tree = DlisGraph(self->opcode_node_list, self->length);
  DlisGraphAddNode(tree, root->opcode_node->opcode);
  new_root = DlisGraphGet(tree, root->opcode_node->opcode);
  DlisGraphNodeSetRecursionFlag(new_root);
  DlisGraphExtendTree(tree, new_root, root);
  return tree;
} /*}}}*/

void DlisGraphExtendTree(dlis_graph self, dlis_graph_node new_node, dlis_graph_node old_node) /*{{{*/
{
  /* Check if the successors of old_node aren't already in the tree */
  dlis_graph_node tmp;
  dlis_graph_node_list_item a, b;
  /* Increase use count */
  new_node->opcode_node->use_count += 1;
  for (b=old_node->outgoing; b; b=a)
  {
    a = b->next;
    if (DlisGraphHasNode(self, b->node->opcode_node->opcode))
    {
      /* A successor of old_node was already added.
       * Do not continue on this path to avoid recursion */
      continue;
    }
    tmp = DlisGraphAddNode(self, b->node->opcode_node->opcode);
    DlisGraphAddEdge(self, new_node->opcode_node->opcode, tmp->opcode_node->opcode);
    /* Extend recursively... */
    DlisGraphExtendTree(self, tmp, b->node);
  }
} /*}}}*/

void DlisGraphPrint(dlis_graph self) /*{{{*/
{
  int i;
  VERBOSE(0, ("Graph (%d/%d nodes):", self->index, self->length));
  for (i=0; i < self->index; i++)
  {
    DlisGraphNodePrint(self->node_list[i]);
  }
} /*}}}*/

dlis_graph_node DlisGraphGetRoot(dlis_graph self) /*{{{*/
{
  ASSERT(self->index != 0, ("Can not return root; Graph is empty!"));
  return self->node_list[0];
} /*}}}*/

t_bool DlisGraphIsIn(dlis_graph self, t_i386_opcode opcode) /*{{{*/
{
  int i;
  for (i=0; i < self->index; i++)
  {
    if (self->node_list[i]->opcode_node->opcode == opcode)
    {
      return TRUE;
    }
  }
  return FALSE;
} /*}}}*/

void DlisGraphDestruct(dlis_graph self) /*{{{*/
{
  /* TODO */
  return;
} /*}}}*/

/* Graph related: DlisGraphNode */
dlis_graph_node DlisGraphNode(dlis_opcode_node opcode_node) /*{{{*/
{
  dlis_graph_node self = (dlis_graph_node) Malloc(sizeof(*self));
  self->opcode_node = opcode_node;
  self->incoming = NULL;
  self->outgoing = NULL;
  DlisGraphNodeClearRecursionFlag(self);
  return self;
} /*}}}*/

void DlisGraphNodeAddOutgoing(dlis_graph_node self, dlis_graph_node to) /*{{{*/
{
  dlis_graph_node_list_item b, a;
  for (b=self->outgoing; b; b=a)
  {
    a = b->next;
    ASSERT(b->node != to, ("Trying to re-add an existing outgoing edge."));
  }
  b = DlisGraphNodeListItem(to, self->outgoing);
  self->outgoing = b;
} /*}}}*/

void DlisGraphNodeClearColor(dlis_graph_node self) /*{{{*/
{
  self->opcode_node->color = COLOR_NONE;
} /*}}}*/

void DlisGraphNodeClearUseCount(dlis_graph_node self) /*{{{*/
{
  self->opcode_node->use_count = 0;
} /*}}}*/

dlis_node_color DlisGraphGetColor(dlis_graph_node self) /*{{{*/
{
  return self->opcode_node->color;
} /*}}}*/

void DlisGraphNodeSetColor(dlis_graph_node self, dlis_node_color color) /*{{{*/
{
  self->opcode_node->color = color;
} /*}}}*/

void DlisGraphNodeClearRecursionFlag(dlis_graph_node self) /*{{{*/
{
  self->recursion_flag = 0;
} /*}}}*/

void DlisGraphNodeSetRecursionFlag(dlis_graph_node self) /*{{{*/
{
  self->recursion_flag = 1;
} /*}}}*/

t_bool DlisGraphNodeGetRecursionFlag(dlis_graph_node self) /*{{{*/
{
  return  self->recursion_flag;
} /*}}}*/

int DlisGraphNodeGetUseCount(dlis_graph_node self) /*{{{*/
{
  return self->opcode_node->use_count;
} /*}}}*/

void DlisGraphNodePrint(dlis_graph_node self) /*{{{*/
{
  dlis_graph_node_list_item a, b;
  /* TODO: incoming */
  VERBOSE(0, ("Graph Node for opcode %s (color %d):", i386_opcode_table[self->opcode_node->opcode].textual, self->opcode_node->color));
  for (b=self->outgoing; b; b=a)
  {
    a = b->next;
    VERBOSE(0, ("  ----> Graph Node for opcode %s", i386_opcode_table[b->node->opcode_node->opcode]));
  } 
} /*}}}*/

dlis_node_color DlisGraphNodeGetColor(dlis_graph_node self) /*{{{*/
{
  return self->opcode_node->color;
} /*}}}*/

t_i386_opcode DlisGraphNodeGetOpcode(dlis_graph_node self) /*{{{*/
{
  return self->opcode_node->opcode;
} /*}}}*/

/* Graph related: DlisOpcodeNode */
dlis_opcode_node DlisOpcodeNode(t_i386_opcode opcode) /*{{{*/
{
  dlis_opcode_node self = (dlis_opcode_node) Malloc(sizeof(*self));
  self->opcode = opcode;
  self->color = COLOR_NONE;
  self->use_count = 0;
  return self;
} /*}}}*/

/* Graph related: DlisGraphNodeList(Item) */
dlis_opcode_node * OpcodeNodeList() /*{{{*/
{
  dlis_opcode_node *ret = (dlis_opcode_node *) Calloc(MAX_I386_OPCODE, sizeof(dlis_opcode_node));
  memset(ret, 0, MAX_I386_OPCODE*sizeof(dlis_opcode_node));
  return ret;
} /*}}}*/

dlis_graph_node_list_item DlisGraphNodeListItem(dlis_graph_node node, dlis_graph_node_list_item next) /*{{{*/
{
  dlis_graph_node_list_item self = (dlis_graph_node_list_item) Malloc(sizeof(*self));
  self->node = node;
  self->next = next;
  return self;
} /*}}}*/

dlis_graph_list DlisGraphList(int length) /*{{{*/
{
  dlis_graph_list self = (dlis_graph_list) Malloc(sizeof(*self));
  self->length = length;
  self->index = 0;
  self->list = (dlis_graph*) Calloc(length, sizeof(dlis_graph));
  return self;
} /*}}}*/

dlis_graph DlisGraphListGet(dlis_graph_list self, int i) /*{{{*/
{
  ASSERT(0 <= i && i < self->index, ("GraphList index out of bounds!"));
  return self->list[i];
} /*}}}*/

void DlisGraphListAdd(dlis_graph_list self, dlis_graph g) /*{{{*/
{
  ASSERT(self->index < self->length, ("GraphList full (%d/%d elements)!", self->index, self->length));
  self->list[self->index] = g;
  self->index += 1;
} /*}}}*/

int DlisGraphListGetSize(dlis_graph_list self) /*{{{*/
{
  return self->index;
} /*}}}*/

dlis_graph DlisGraphListPop(dlis_graph_list self, int i) /*{{{*/
{
  dlis_graph popped, last;
  popped = DlisGraphListGet(self, i);
  if ((i+1) == self->index ||
      self->index == 1)
  {
    /* Last element or only 1 element */
    self->index -= 1;
    return popped;
  }
  else
  {
    last = DlisGraphListGet(self, self->index - 1);
    self->list[i] = last;
    self->index -= 1;
    return popped;
  }
} /*}}}*/

void DlisGraphListDestruct(dlis_graph_list self) /*{{{*/
{
  /* TODO */
  return;
} /*}}}*/

/*
 * Instruction creation
 */
void dlis_I386InstructionMakeAdd(t_i386_ins *ins, t_uint32 val, t_reg reg, t_uint32 size, t_i386_regmode regmode)/*{{{*/
{
  if (size == 0)
  {
    size = dlis_getSizeFromRegmode(regmode);
  }
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_ADD);
  I386OpSetReg(I386_INS_DEST(ins), reg, regmode);
  I386OpSetImm(I386_INS_SOURCE1(ins), val, size);

  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSub(t_i386_ins *ins, t_reg dst, t_reg src, int imm, t_i386_regmode regmode)/*{{{*/
{
  t_uint32 size = dlis_getSizeFromRegmode(regmode);
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_SUB);
  I386OpSetReg(I386_INS_DEST(ins), dst, regmode);
  if (src == I386_REG_NONE)
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, size);
  }
  else
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }

  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSubMem(t_i386_ins *ins, t_reg reg, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_i386_regmode regmode)/*{{{*/
{
  t_uint32 size = dlis_getSizeFromRegmode(regmode);
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_SUB);
  I386OpSetReg(I386_INS_DEST(ins), reg, regmode);
  I386OpSetMem(I386_INS_SOURCE1(ins), offset, base, index, scale, size);

  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSubToMem(t_i386_ins *ins, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_reg src, int imm, t_i386_regmode regmode)/*{{{*/
{
  t_uint32 size = dlis_getSizeFromRegmode(regmode);
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_SUB);
  I386OpSetMem(I386_INS_DEST(ins), offset, base, index, scale, size);
  if (src == I386_REG_NONE)
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, size);
  }
  else
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }

  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeShr(t_i386_ins *ins, t_reg dest, t_uint32 amount, t_uint32 size, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_SHR);
  I386OpSetImm(I386_INS_SOURCE1(ins), amount, size);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);

  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSimpleToReg_DestIsSource(t_i386_ins *ins, t_i386_opcode opcode, t_reg dest, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, memopsize);
  }
  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSimpleToMem_DestIsSource(t_i386_ins *ins, t_i386_opcode opcode, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetMem(I386_INS_DEST(ins), offset, base, index, scale, memopsize);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, memopsize);
  }
  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeSimpleFromMem_DestIsSource(t_i386_ins *ins, t_i386_opcode opcode, t_reg dst, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetReg(I386_INS_DEST(ins), dst, regmode);
  I386OpSetMem(I386_INS_SOURCE1(ins), offset, base, index, scale, memopsize);
  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeAndToReg(t_i386_ins *ins, t_reg dest, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode) /*{{{*/
{
  dlis_I386InstructionMakeSimpleToReg_DestIsSource(ins, I386_AND, dest, src, imm, memopsize, regmode);
} /* }}} */

void dlis_I386InstructionMakeAndFromMem(t_i386_ins *ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_uint32 memopsize, t_i386_regmode regmode) /*{{{*/
{
  dlis_I386InstructionMakeSimpleFromMem_DestIsSource(ins, I386_AND, dest, offset, base, index, scale, memopsize, regmode);
} /*}}}*/

void dlis_I386InstructionMakeOrToReg(t_i386_ins *ins, t_reg dest, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode) /*{{{*/
{
  dlis_I386InstructionMakeSimpleToReg_DestIsSource(ins, I386_OR, dest, src, imm, memopsize, regmode);
} /* }}} */

void dlis_I386InstructionMakeOrToMem(t_i386_ins *ins, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_reg src, int imm, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  dlis_I386InstructionMakeSimpleToMem_DestIsSource(ins, I386_OR, offset, base, index, scale, src, imm, memopsize, regmode);
}/*}}}*/

void dlis_I386InstructionMakeLogicToMem(t_i386_ins *ins, t_i386_opcode opcode, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetMem(I386_INS_DEST(ins), offset, base, index, scale, memopsize);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, memopsize);
  }
  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

void dlis_I386InstructionMakeNotReg(t_i386_ins *ins, t_reg dest, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_NOT);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);

  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

void dlis_I386InstructionMakeArithmeticToMem(t_i386_ins * ins, t_i386_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opc);
  I386OpSetMem(I386_INS_DEST(ins), offset, base, index, scale, memopsize);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, memopsize);
  }

  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

t_i386_ins * dlis_I386InsNewForBbl(t_bbl *bbl)/*{{{*/
{
  return (t_i386_ins*) InsNewForBbl(bbl);
}/*}}}*/

void dlis_I386InsInsertBefore(t_i386_ins *subject, t_i386_ins *reference)/*{{{*/
{
  I386InsInsertBefore(subject, reference);
  dlis_CopyAddresses(subject, reference);
}/*}}}*/

void dlis_I386InsInsertAfter(t_i386_ins *subject, t_i386_ins *reference)/*{{{*/
{
  I386InsInsertAfter(subject, reference);
  dlis_CopyAddresses(subject, reference);
}/*}}}*/

void dlis_I386InstructionMakeMovToRegFromMem(t_i386_ins *ins, t_reg dest, t_uint32 offset, t_reg src, t_reg index, int scale, t_uint32 memopsize, t_i386_regmode regmode)/*{{{*/
{
  /* memopsize in (1, 2, 4) */
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetMem(I386_INS_SOURCE1(ins), offset, src, index, scale, memopsize);

  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);

  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeMovToReg(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, dlis_getSizeFromRegmode(regmode));
  }

  I386SetGenericInsInfo(ins);
} /* }}} */

void dlis_I386InstructionMakeMovToMem(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale, dlis_getSizeFromRegmode(regmode)/*memopsize*/);
  if (src != I386_REG_NONE)
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), src, regmode);
  }
  else
  {
    I386OpSetImm(I386_INS_SOURCE1(ins),imm, dlis_getSizeFromRegmode(regmode));
  }

  I386SetGenericInsInfo(ins);
}
/* }}} */

void dlis_I386InstructionMakeLea(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_LEA);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);
  I386OpSetMem(I386_INS_SOURCE1(ins), offset, base, index, scale, memopsize);

  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeShift(t_i386_ins *ins, t_i386_opcode opcode, t_reg dest, int imm, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetReg(I386_INS_DEST(ins), dest, regmode);
  if (imm >= 0)
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, 1/*immedsize*/);
  }
  else
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), I386_REG_ECX, i386_regmode_lo8);
  }
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeShiftMem(t_i386_ins *ins, t_i386_opcode opcode, t_uint32 offset, t_reg base, t_reg index, int scale, int imm, t_i386_regmode regmode) /*{{{*/
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetMem(I386_INS_DEST(ins), offset, base, index, scale, dlis_getSizeFromRegmode(regmode));
  if (imm >= 0)
  {
    I386OpSetImm(I386_INS_SOURCE1(ins), imm, 1/*immedsize*/);
  }
  else
  {
    I386OpSetReg(I386_INS_SOURCE1(ins), I386_REG_ECX, i386_regmode_lo8);
  }
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}/*}}}*/

void dlis_I386InstructionMakeXor(t_i386_ins *ins, t_reg dst, t_reg src, int imm, t_i386_regmode regmode) /*{{{*/
{
  dlis_I386InstructionMakeSimpleToReg_DestIsSource(ins, I386_XOR, dst, src, imm, dlis_getSizeFromRegmode(regmode), regmode);
} /*}}}*/

void dlis_I386InstructionMakeXorToMem(t_i386_ins *ins, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_reg src, int imm, t_i386_regmode regmode) /*{{{*/
{
  dlis_I386InstructionMakeSimpleToMem_DestIsSource(ins, I386_XOR, offset, base, index, scale, src, imm, dlis_getSizeFromRegmode(regmode), regmode);
}/*}}}*/

void dlis_I386InstructionMakeXorFromMem(t_i386_ins *ins, t_reg dst, t_uint32 offset, t_reg base, t_reg index, t_uint32 scale, t_i386_regmode regmode)/*{{{*/
{
  dlis_I386InstructionMakeSimpleFromMem_DestIsSource(ins, I386_XOR, dst, offset, base, index, scale, dlis_getSizeFromRegmode(regmode), regmode);
}/*}}}*/

/*
 * Returns the correct size in bytes for a given regmode
 */
t_uint32 dlis_getSizeFromRegmode(t_i386_regmode regmode) /*{{{*/
{
  switch (regmode)
  {
    case i386_regmode_full32:
      return (t_uint32) 4;
    case i386_regmode_lo16:
      return (t_uint32) 2;
    case i386_regmode_lo8:
      return (t_uint32) 1;
    case i386_regmode_hi8:
      return (t_uint32) 1;
    case i386_regmode_invalid:
      FATAL(("Can not handle regmode %d", regmode));
    default:
      FATAL(("Unknown regmode %d", regmode));
  }
}/*}}}*/

t_i386_regmode dlis_getRegmodeFromSize(t_uint32 size) /*{{{*/
{
  switch (size)
  {
    case 4:
      return i386_regmode_full32;
    case 2:
      return i386_regmode_lo16;
    case 1:
      return i386_regmode_lo8;
    default:
      FATAL(("Invalid memopsize %d", size));
  }
} /*}}}*/

t_i386_regmode dlis_getRegmode(t_i386_ins *ins) /*{{{*/
{
  if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
  {
    return I386_OP_REGMODE(I386_INS_DEST(ins));
  }
  else
  {
    if (I386_INS_SOURCE1(ins) && (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg))
    {
      return I386_OP_REGMODE(I386_INS_SOURCE1(ins));
    }
    else if (I386_INS_SOURCE2(ins) && (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_reg))
    {
      return I386_OP_REGMODE(I386_INS_SOURCE2(ins));
    }
    else
    {
      if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
      {
	return dlis_getRegmodeFromSize(I386_OP_MEMOPSIZE(I386_INS_DEST(ins)));
      }
      else if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
      {
	return dlis_getRegmodeFromSize(I386_OP_MEMOPSIZE(I386_INS_SOURCE1(ins)));
      }
      else if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
      {
	return dlis_getRegmodeFromSize(I386_OP_MEMOPSIZE(I386_INS_SOURCE2(ins)));
      }
      else
      {
	FATAL(("Could not find a reg or mem operand for @I", ins));
      }
    }
  }
} /*}}}*/

t_uint32 dlis_getDeltaEsp(t_i386_regmode regmode)/*{{{*/
{
  switch (regmode)
  {
    case i386_regmode_full32:
      return (t_uint32) 4;
    case i386_regmode_lo16:
      return (t_uint32) 2;
    default:
      FATAL(("Can not handle regmode %d", regmode));
  }
}/*}}}*/

t_bool dlis_CanInvertJcc(t_i386_ins *jcc) /*{{{*/
{
  switch(I386_INS_CONDITION((jcc)))
  {
    case I386_CONDITION_B:
    case I386_CONDITION_BE:
    case I386_CONDITION_L:
    case I386_CONDITION_LE:
    case I386_CONDITION_NO:
    case I386_CONDITION_NP:
    case I386_CONDITION_NS:
    case I386_CONDITION_NZ:
      /* FALLTHROUGHS */
      return TRUE;
    default:
      return FALSE;
  }
} /*}}}*/

t_i386_condition_code dlis_getInvertedCondition(t_i386_ins *jcc) /*{{{*/
{
  switch(I386_INS_CONDITION(jcc))
  {
    case I386_CONDITION_B:
      return I386_CONDITION_AE;
    case I386_CONDITION_BE:
      return I386_CONDITION_A;
    case I386_CONDITION_L:
      return I386_CONDITION_GE;
    case I386_CONDITION_LE:
      return I386_CONDITION_G;
    case I386_CONDITION_NO:
      return I386_CONDITION_O;
    case I386_CONDITION_NP:
      return I386_CONDITION_P;
    case I386_CONDITION_NS:
      return I386_CONDITION_S;
    case I386_CONDITION_NZ:
      return I386_CONDITION_Z;
    default:
      FATAL(("Can not invert condition of @I", jcc));
  }
}/*}}}*/

t_bool dlis_HasFallthroughPred(t_bbl *bbl) /*{{{*/
{
  t_cfg_edge *e, *e_corr;
  BBL_FOREACH_PRED_EDGE(bbl, e)
  {
    if (CfgEdgeTestCategoryOr(e, (ET_FALLTHROUGH | ET_IPFALLTHRU)))
    {
      return TRUE;
    }
    else if (CFG_EDGE_CORR(e))
    {
      e_corr = CFG_EDGE_CORR(e);
      if (CfgEdgeTestCategoryOr(e_corr, (ET_CALL | ET_SWI)))
      {
	return TRUE;
      }
    }
  }
  return FALSE;
}/*}}}*/

t_bool dlis_ImmediateSignedMinimum(t_i386_ins *ins) /*{{{*/
{
  int sw, val;
  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_imm)
  {
    return FALSE;
  }

  val = I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins));

  if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
  {
    sw = I386_OP_MEMOPSIZE(I386_INS_DEST(ins));
  }
  else if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
  {
    sw = dlis_getSizeFromRegmode(I386_OP_REGMODE(I386_INS_DEST(ins)));
  }
  else
  {
    FATAL(("Unknown destination optype for @I", ins));
  }

  switch (sw)
  {
    case 1:
      return (val == -128);
      break;
    case 2:
      return (val == -32768);
      break;
    case 4:
      return ((val+1) == -2147483647);
      break;
    default:
      FATAL(("Unknown memopsize/regmode %d!", sw));
  }
  return FALSE;
}/*}}}*/


t_bool computeCost = FALSE;
t_int32 * frequencies = NULL;
void InitiateScoreComputation(t_cfg * cfg)
{
  t_ins * ins;
  frequencies = (t_int32*) Calloc(MAX_I386_OPCODE,sizeof(t_int32));
  CFG_FOREACH_INS(cfg,ins)
  {
    frequencies[I386_INS_OPCODE(T_I386_INS(ins))]++;
  }
}
void ScoreRemove(t_int32 * scorePtr, t_i386_ins * ins)
{
  if(computeCost)
    return;
  else
  {
    if(frequencies == NULL) {
      FATAL(("call InitiateScoreComputation first"));
	} else {
	  t_int32 frequency = frequencies[I386_INS_OPCODE(ins)];
      t_int32 loss = -2*frequency+1;
      *scorePtr += loss;
	}
  }
  return;
}
void ScoreAdd(t_int32 * scorePtr, t_i386_ins * ins, t_i386_opcode opcode)
{
  if(computeCost)
    *scorePtr+=1;
  else
  {
    if(frequencies == NULL) {
      FATAL(("call InitiateScoreComputation first"));
	} else {
      t_int32 frequency = frequencies[opcode];
      t_int32 gain = 2*frequency+1;
      *scorePtr += gain;
	}
  }
  return;
}

/*
 * Limit* functions
 */
t_int32 LimitImmCallEx(t_cfg *cfg, t_i386_ins *call,t_bool execute)/*{{{*/
{
  t_i386_ins * push, * jmp;
  t_bbl *bbl = I386_INS_BBL(call);
  t_int32 score=0;
  t_edge * corr;
  t_edge * call_edge;

  ScoreRemove(&score,call);
  ScoreAdd(&score,call,I386_PUSH);
  ScoreAdd(&score,call,I386_JMP);

  if(!execute)
    return score;

  //toevoegen van push en jmp instructies voor de call instructie
  push = dlis_I386InsNewForBbl(bbl);
  jmp = dlis_I386InsNewForBbl(bbl);
  //push dient het adres van de instructie volgend op de call op de stapel te plaatsen. Dat adres is echter nog niet bekend, voorlopig zetten we daar gewoon 0, straks zullen we met behulp van een relocatie de juiste manier van berekenen vastleggen, zodat het tijdens het deflowgraphen ingevuld kan worden.
  I386InstructionMakePush(push,I386_REG_NONE,0);
  I386InstructionMakeJump(jmp);
  dlis_I386InsInsertBefore(push,call);
  dlis_I386InsInsertBefore(jmp,call);

  //de constante die push op de stapel plaatst is het adres van de instructie volgend op de call-instructie
  //probleem: we kennen dat adres voorlopig niet => oplossing: relocaties
  corr = NULL;
  if(BBL_SUCC_FIRST(bbl))
      {
  if(EDGE_CAT(T_EDGE(BBL_SUCC_FIRST(bbl)))==ET_CALL)
   corr = EDGE_CORR(T_EDGE(BBL_SUCC_FIRST(bbl)));
  else if (EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl))) && EDGE_CAT(EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl))))==ET_CALL)
    corr = EDGE_CORR(EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl))));
      }
    
  if (corr)
  {
    // BBL_SUCC_FIRST= the first outgoing edge
    // EDGE_CORR stores the correspondig edge (for the first outgoing edge)
    // EDGE_TAIL The node to which this CORR edge points
    // T_BBL Contructor for a bbl
    // Result: the bbl that will be returned to from the bbl that is called
    t_bbl * link_path = T_BBL(EDGE_TAIL(corr));
    RelocTableAddRelocToRelocatable(
	OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),//cfg.section.object.reloc_table
	0x0,
	T_RELOCATABLE(push),
	0x0,
	T_RELOCATABLE(link_path),
	0,
	FALSE,
	NULL,
	NULL,
	NULL,
	"R00" "\\" WRITE_32
	);
    I386_OP_FLAGS(I386_INS_SOURCE1(push)) = I386_OPFLAG_ISRELOCATED;
  }

  //aanpassen pijlen
  call_edge=NULL;
  if(BBL_SUCC_FIRST(bbl))
  {
    if(EDGE_CAT(T_EDGE(BBL_SUCC_FIRST(bbl)))==ET_CALL)
      call_edge = T_EDGE(BBL_SUCC_FIRST(bbl));
    else if (EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl))) && EDGE_CAT(EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl))))==ET_CALL)
    {
      call_edge = EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(bbl)));
    }
    else 
      call_edge = T_EDGE(BBL_SUCC_FIRST(bbl));
  }
  else
    FATAL(("@ieB",bbl));
  if(call_edge)
  {
    t_bbl * target = T_BBL(CFG_EDGE_TAIL(T_CFG_EDGE(call_edge)));
    CfgEdgeKill(T_CFG_EDGE(call_edge));
    CfgEdgeCreate(cfg,bbl,target,ET_JUMP);
  }

  //verwijderen call instructie
  I386InsKill(call);

  return score;
}/*}}}*/

t_bool LimitImmCall(t_cfg *cfg, t_i386_ins *call)
{
  return (LimitImmCallEx(cfg,call,TRUE)==0)?FALSE:TRUE;
}


/*
 * Limits instructions:
 * C3
 * CB
 * TODO: try to make this work even without a free register (push/pop a register?)
 */
t_int32 LimitRetEx(t_cfg *cfg, t_i386_ins *ret,t_bool execute)/*{{{*/
{
  t_reg reg = FindFreeRegister(ret);
  t_int32 score = 0;

  if (reg != I386_REG_NONE)
  {	
    t_i386_ins *pop, *jmp;
    t_bbl *bbl = INS_BBL((t_ins*) ret);

    ScoreRemove(&score,ret);
    ScoreAdd(&score,ret,I386_POP);
    ScoreAdd(&score,ret,I386_JMP);

    if(!execute)
      return score;

    pop = dlis_I386InsNewForBbl(bbl);
    jmp = dlis_I386InsNewForBbl(bbl);

    I386InstructionMakePop(pop, reg);
    I386InstructionMakeJumpReg(jmp, reg);
    dlis_I386InsInsertBefore(pop, ret);
    dlis_I386InsInsertBefore(jmp, ret);


    I386InsKill(ret);

    //aanpassen pijlen
    if(BBL_SUCC_FIRST(bbl))
    {
      t_bbl * target = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
      CfgEdgeKill(BBL_SUCC_FIRST(bbl));
      CfgEdgeNew(cfg,bbl,target,ET_JUMP);
    }
    else
    {
      FATAL(("shouldn't happen"));
    }
    return score;
  }
  else
  {
    return score;
  }
}/*}}}*/
t_bool LimitRet(t_cfg *cfg, t_i386_ins *ret)
{
  return (LimitRetEx(cfg,ret,TRUE)==0)?FALSE:TRUE;
}


/*
 * Untested...
 * Limit instructions:
 * C2 iw
 * CA iw
 * TODO: try to make this work even without a free register (push/pop a register?)
 */
t_int32 LimitImmRetEx(t_cfg *cfg, t_i386_ins *ret,t_bool execute)/*{{{*/
{
  //t_reg reg = FindFreeRegister(ret);
  t_int32 score = 0;

  return score;
  /*
     if (reg != I386_REG_NONE)
     {
     t_i386_ins *pop, *jmp;
     t_bbl *bbl = INS_BBL((t_ins*) ret);

     ScoreRemove(&score,ret);
     ScoreAdd(&score,ret,I386_POP);
     ScoreAdd(&score,ret,I386_JMP);

     if(!execute)
     return score;


     pop = dlis_I386InsNewForBbl(bbl);
     jmp = dlis_I386InsNewForBbl(bbl);

     I386InstructionMakePop(pop, reg);
     I386InstructionMakeJumpReg(jmp, reg);

     dlis_I386InsInsertBefore(pop, ret);
     dlis_I386InsInsertBefore(jmp, ret);

     I386InsKill(ret);

  //aanpassen pijlen
  {
  t_bbl * target = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
  CfgEdgeKill(BBL_SUCC_FIRST(bbl));
  CfgEdgeNew(cfg,bbl,target,ET_JUMP);
  }
  return score;
  }
  else
  {
  return score;
  }*/
}
t_bool LimitImmRet(t_cfg *cfg, t_i386_ins *ret)
{
  return (LimitImmRetEx(cfg,ret,TRUE)==0)?FALSE:TRUE;
}/*}}}*/

/*
 *
 * Limits instructions:
 * 50+ rw (r16)
 * 50+ rd (r32)
 */
t_int32 LimitRegPushEx(t_cfg *cfg, t_i386_ins *push,t_bool execute)/*{{{*/
{
  t_i386_ins *mov, *sub;

  t_bbl *bbl = INS_BBL((t_ins*) push);

  t_reg src_reg = I386_OP_BASE(I386_INS_SOURCE1(push));

  t_uint32 delta_esp = dlis_getDeltaEsp(I386_OP_REGMODE(I386_INS_SOURCE1(push)));

  t_regset regset = I386InsRegsLiveBefore(push);

  t_int32 score = 0;
  if (RegsetIn(regset, I386_CONDREG_OF) ||
      RegsetIn(regset, I386_CONDREG_SF) ||
      RegsetIn(regset, I386_CONDREG_ZF) ||
      RegsetIn(regset, I386_CONDREG_AF) ||
      RegsetIn(regset, I386_CONDREG_CF) ||
      RegsetIn(regset, I386_CONDREG_PF))
  {
    return score;
  }

  ScoreRemove(&score,push);
  ScoreAdd(&score,push,I386_SUB);
  ScoreAdd(&score,push,I386_MOV);

  if(!execute)
    return score;

  sub = dlis_I386InsNewForBbl(bbl);
  mov = dlis_I386InsNewForBbl(bbl);

  dlis_I386InstructionMakeSub(sub, I386_REG_ESP, I386_REG_NONE, delta_esp, i386_regmode_full32);
  I386InstructionMakeMovToMem(mov, 0, I386_REG_ESP, I386_REG_NONE, /*scale*/1, src_reg, /*imm*/0);

  dlis_I386InsInsertBefore(sub, push);
  dlis_I386InsInsertBefore(mov, push);

  I386InsKill(push);
  return score;
}/*}}}*/
t_bool LimitRegPush(t_cfg *cfg, t_i386_ins *push)
{
  return (LimitRegPushEx(cfg,push,TRUE)==0)?FALSE:TRUE;
}


/*
 * Limits instructions:
 * 6A (imm8)
 * 68 (imm16)
 * 68 (imm32)
 */
t_int32 LimitImmPushEx(t_cfg *cfg, t_i386_ins *push,t_bool execute)/*{{{*/
{
  t_i386_ins *mov, *sub;

  t_bbl *bbl = INS_BBL((t_ins*) push);


  t_uint32 imm = I386_OP_IMMEDIATE(I386_INS_SOURCE1(push));
  t_uint32 delta_esp = dlis_getDeltaEsp(I386_OP_REGMODE(I386_INS_SOURCE1(push)));

  t_regset regset = I386InsRegsLiveBefore(push);
  t_int32 score = 0;

  if (RegsetIn(regset, I386_CONDREG_OF) ||
      RegsetIn(regset, I386_CONDREG_SF) ||
      RegsetIn(regset, I386_CONDREG_ZF) ||
      RegsetIn(regset, I386_CONDREG_AF) ||
      RegsetIn(regset, I386_CONDREG_CF) ||
      RegsetIn(regset, I386_CONDREG_PF))
  {
    return score;
  }

  ScoreRemove(&score,push);
  ScoreAdd(&score,push,I386_SUB);
  ScoreAdd(&score,push,I386_MOV);

  if(!execute)
    return score;

  sub = dlis_I386InsNewForBbl(bbl);
  mov = dlis_I386InsNewForBbl(bbl);


  dlis_I386InstructionMakeSub(sub, I386_REG_ESP, I386_REG_NONE, delta_esp, i386_regmode_full32);
  I386InstructionMakeMovToMem(mov, /*offset*/0, I386_REG_ESP, I386_REG_NONE, /*scale*/1, I386_REG_NONE, imm);

  if (I386_OP_FLAGS(I386_INS_SOURCE1(push)) & I386_OPFLAG_ISRELOCATED)
  {
    RelocSetFrom(RELOC_REF_RELOC(INS_REFERS_TO((t_ins*)push)), T_RELOCATABLE(mov));
    I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
  }

  dlis_I386InsInsertBefore(sub, push);
  dlis_I386InsInsertBefore(mov, push);

  I386InsKill(push);
  return score;
}/*}}}*/
t_bool LimitImmPush(t_cfg *cfg, t_i386_ins *push)
{
  return (LimitImmPushEx(cfg,push,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * POP r16
 * POP r32
 */
t_int32 LimitRegPopEx(t_cfg *cfg, t_i386_ins *pop, t_bool execute)/*{{{*/
{
  t_i386_ins *mov, *add;
  t_regset regset = I386InsRegsLiveBefore(pop);
  t_bbl *bbl = I386_INS_BBL(pop);
  t_int32 score = 0;
  t_reg dest_reg;
  t_i386_regmode regmode;
  t_uint32 delta_esp, size;

  if (RegsetIn(regset, I386_CONDREG_OF) ||
      RegsetIn(regset, I386_CONDREG_SF) ||
      RegsetIn(regset, I386_CONDREG_ZF) ||
      RegsetIn(regset, I386_CONDREG_AF) ||
      RegsetIn(regset, I386_CONDREG_CF) ||
      RegsetIn(regset, I386_CONDREG_PF))
  {
    return score;
  }

  dest_reg = I386_OP_BASE(I386_INS_DEST(pop));
  regmode = I386_OP_REGMODE(I386_INS_DEST(pop));
  delta_esp = dlis_getDeltaEsp(I386_OP_REGMODE(I386_INS_DEST(pop)));

  /* Detect size */
  size = dlis_getSizeFromRegmode(regmode);

  ScoreRemove(&score,pop);
  ScoreAdd(&score,pop,I386_MOV);
  ScoreAdd(&score,pop,I386_ADD);
  if(!execute)
    return score;
  mov = dlis_I386InsNewForBbl(bbl);
  add = dlis_I386InsNewForBbl(bbl);

  dlis_I386InstructionMakeMovToRegFromMem(mov, dest_reg, /*offset*/0, I386_REG_ESP, I386_REG_NONE, /*scale*/0, /*memopsize*/size, /*regmode*/regmode);
  dlis_I386InstructionMakeAdd(add, /*value*/delta_esp, I386_REG_ESP, /*size*/(t_uint32)0, i386_regmode_full32);

  dlis_I386InsInsertBefore(mov, pop);
  dlis_I386InsInsertBefore(add, pop);

  I386InsKill(pop);
  return score;
}/*}}}*/
t_bool LimitRegPop(t_cfg *cfg, t_i386_ins *pop)
{
  return (LimitRegPopEx(cfg,pop,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * NEG r/m8, r/m16, r/m32
 * TODO: No support for regmode/memopsize, but works for the tests
 */
t_int32 LimitNegEx(t_cfg *cfg, t_i386_ins *neg, t_bool execute) /*{{{*/
{
  t_i386_ins *sub, *mov1, *mov2, *push, *pop;

  t_bbl *bbl = INS_BBL((t_ins*) neg);

  t_i386_operand *op = I386_INS_DEST(neg);
  t_uint32 offset = I386_OP_IMMEDIATE(op);
  t_reg base = I386_OP_BASE(op);
  t_reg index = I386_OP_INDEX(op);
  int scale = I386_OP_SCALE(op);

  t_reg reg = FindFreeRegister(neg);
  t_bool pushpop = FALSE;
  t_int32 score=0;

  ScoreRemove(&score,neg);
  ScoreAdd(&score,neg,I386_SUB);
  ScoreAdd(&score,neg,I386_MOV);
  ScoreAdd(&score,neg,I386_MOV);

  if (reg == I386_REG_NONE)
  {
    ScoreAdd(&score,neg,I386_PUSH);
    ScoreAdd(&score,neg,I386_POP);
    if(!execute)
      return score;
    pushpop = TRUE;
    reg = ChooseRandomRegister();
    while (reg == base)
    {
      reg = ChooseRandomRegister();
    }
    push = dlis_I386InsNewForBbl(bbl);
    pop = dlis_I386InsNewForBbl(bbl);

    I386InstructionMakePush(push, reg, 0);
    I386InstructionMakePop(pop, reg);
      if(RegsetIn(I386InsUsedRegisters(neg),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(neg),I386_REG_ESP))
	FATAL(("ESP corrupt"));
  }
  if(!execute)
    return score;
  sub = dlis_I386InsNewForBbl(bbl);
  mov1 = dlis_I386InsNewForBbl(bbl);
  mov2 = dlis_I386InsNewForBbl(bbl);

  I386InstructionMakeMovToReg(mov1, reg, I386_REG_NONE, 0);
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    I386InstructionMakeArithmetic(sub, I386_SUB, reg, base, 0);
    I386InstructionMakeMovToReg(mov2, base, reg, 0);
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    I386InstructionMakeArithmeticFromMem(sub, I386_SUB, reg, offset, base, index, scale);
    I386InstructionMakeMovToMem(mov2, offset, base, index, scale, reg, 0);
    if (I386_OP_FLAGS(I386_INS_DEST(neg)) & I386_OPFLAG_ISRELOCATED)
    {
      t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(neg));
      RelocSetFrom(rel, T_RELOCATABLE(sub));
      rel = RelocTableDupReloc(RELOC_TABLE(rel), rel);
      RelocSetFrom(rel, T_RELOCATABLE(mov2));
      I386_OP_FLAGS(I386_INS_SOURCE1(sub)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(mov2)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(neg)) &= ~I386_OPFLAG_ISRELOCATED;
    }
  }
  else
  {
    FATAL(("Unsupported optype for neg"));
  }
  if (pushpop)
  {
    /* dlis_I386InsInsertBefore also sets the addresses
     * so make sure it has neg as it second arg. */
    dlis_I386InsInsertBefore(push, neg);
  }
  dlis_I386InsInsertBefore(mov1, neg);
  dlis_I386InsInsertBefore(sub, neg);
  dlis_I386InsInsertBefore(mov2, neg);
  if (pushpop)
  {
    dlis_I386InsInsertBefore(pop, neg);
  }

  I386InsKill(neg);
  return score;
}/*}}}*/
t_bool LimitNeg(t_cfg *cfg, t_i386_ins *neg) 
{
  return (LimitNegEx(cfg,neg,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * INC r16, r32
 * TODO: merge the Limit*Reg functions...
 */
t_int32 LimitRegIncEx(t_cfg *cfg, t_i386_ins *inc, t_bool execute) /*{{{*/
{
  t_i386_ins *add;

  t_bbl *bbl = INS_BBL((t_ins*) inc);

  t_regset r = InsRegsLiveAfter((t_ins*)inc);

  t_int32 score = 0;
  /*
   * inc does not set CF, add does, so we must filter
   * CF-sensitive cases out
   */
  if (!RegsetIn(r, I386_CONDREG_CF))
  {
    t_reg dest_reg = I386_OP_BASE(I386_INS_DEST(inc));
	t_i386_regmode regmode;

    ScoreRemove(&score,inc);
    ScoreAdd(&score,inc,I386_ADD);
    if(!execute)
      return score;
    add = dlis_I386InsNewForBbl(bbl);
    regmode = I386_OP_REGMODE(I386_INS_DEST(inc));

    dlis_I386InstructionMakeAdd(add, (t_uint32)1, dest_reg, /*size*/0, regmode);

    dlis_I386InsInsertBefore(add, inc);
    I386InsKill(inc);
    return score;
  }
  else
  {
    return score;
  }
}/*}}}*/
t_bool LimitRegInc(t_cfg *cfg, t_i386_ins *inc) 
{
  return (LimitRegIncEx(cfg,inc,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * INC r/m8, r/m16, r/m32
 */
t_int32 LimitIncEx(t_cfg *cfg, t_i386_ins *inc, t_bool execute) /*{{{*/
{
  t_i386_ins *add;

  t_bbl *bbl = INS_BBL((t_ins*) inc);

  t_regset r = InsRegsLiveAfter((t_ins*)inc);

  t_int32 score = 0;
  /*
   * inc does not set CF, add does, so we must filter
   * CF-sensitive cases out
   */
  if (!RegsetIn(r, I386_CONDREG_CF))
  {
    t_i386_operand *op = I386_INS_DEST(inc);
    t_uint32 offset = I386_OP_IMMEDIATE(op);
    t_reg base = I386_OP_BASE(op);
    t_reg index = I386_OP_INDEX(op);
    int scale = I386_OP_SCALE(op);
    t_uint32 memopsize = I386_OP_MEMOPSIZE(op);

    ScoreRemove(&score,inc);
    ScoreAdd(&score,inc,I386_ADD);

    if(!execute)
      return score;
    add = dlis_I386InsNewForBbl(bbl);
    dlis_I386InstructionMakeArithmeticToMem(
	add, 
	I386_ADD, 
	offset, 
	base, 
	index, 
	scale, 
	I386_REG_NONE, 
	(t_uint32)1 /*imm*/, 
	memopsize, 
	i386_regmode_full32/*doesn't matter here*/
	);

    if (I386_OP_FLAGS(I386_INS_DEST(inc)) & I386_OPFLAG_ISRELOCATED)
    {
      RelocSetFrom(RELOC_REF_RELOC(INS_REFERS_TO((t_ins*)inc)), T_RELOCATABLE(add));
      I386_OP_FLAGS(I386_INS_DEST(add)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(inc)) &= ~I386_OPFLAG_ISRELOCATED;
    }

    dlis_I386InsInsertBefore(add, inc);
    I386InsKill(inc);
    return score;
  }
  else
  {
    return score;
  }
}/*}}}*/
t_bool LimitInc(t_cfg *cfg, t_i386_ins *inc) 
{
  return (LimitIncEx(cfg,inc,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * SETcc r/m8
 */
t_int32 LimitSetccEx(t_cfg *cfg, t_i386_ins *setcc,t_bool execute)/*{{{*/
{
  t_i386_ins *mov1, *jcc, *mov2;

  t_bbl *bbl = I386_INS_BBL(setcc);

  t_i386_operand *op = I386_INS_DEST(setcc);
  t_reg base = I386_OP_BASE(op);
  t_reg index = I386_OP_INDEX(op);
  t_uint32 offset = I386_OP_IMMEDIATE(op);
  int scale = I386_OP_SCALE(op);
  t_int32 score = 0;
  t_bbl *end_bbl, *fallthrough_bbl;
  t_i386_condition_code condition;

  ScoreRemove(&score,setcc);
  ScoreAdd(&score,setcc,I386_MOV);
  ScoreAdd(&score,setcc,I386_Jcc);
  ScoreAdd(&score,setcc,I386_MOV);

  if(!execute)
    return score;

  mov1 = dlis_I386InsNewForBbl(bbl);
  jcc = dlis_I386InsNewForBbl(bbl);
  mov2 = dlis_I386InsNewForBbl(bbl);

  condition = I386_INS_CONDITION(setcc);

  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    dlis_I386InstructionMakeMovToReg(mov1, base, I386_REG_NONE, 1/*imm*/, i386_regmode_lo8);
    dlis_I386InstructionMakeMovToReg(mov2, base, I386_REG_NONE, 0/*imm*/, i386_regmode_lo8);
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    dlis_I386InstructionMakeMovToMem(mov1, offset, base, index, scale, I386_REG_NONE, 1/*imm*/, i386_regmode_lo8);
    dlis_I386InstructionMakeMovToMem(mov2, offset, base, index, scale, I386_REG_NONE, 0/*imm*/, i386_regmode_lo8);
    if (I386_OP_FLAGS(I386_INS_DEST(setcc)) & I386_OPFLAG_ISRELOCATED) {
      /* move relocations as well */
      t_reloc *rel;
	  t_reloc *rel2;
	  rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(setcc));
      RelocSetFrom(rel, T_RELOCATABLE(mov1));
      rel2 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
      RelocSetFrom(rel2, T_RELOCATABLE(mov2));
      I386_OP_FLAGS(I386_INS_DEST(mov1)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(mov2)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(setcc)) &= ~I386_OPFLAG_ISRELOCATED;
    }
  }
  else
  {
    FATAL(("Failed to limit setcc instruction."));
  }

  /* NOTE: jump destination to be added later 
   * Should jump to instruction following mov2 if condition is true */
  I386InstructionMakeCondJump(jcc, condition);

  /* Insert mov1, jcc and mov2 before setcc in the original bbl */
  dlis_I386InsInsertBefore(mov1, setcc);
  dlis_I386InsInsertBefore(jcc, setcc);
  dlis_I386InsInsertBefore(mov2, setcc);
  /* Kill setcc, mov2 is now the target for splitting */
  I386InsKill(setcc);

  /* Split the bbl before mov2, creating the fallthrough_bbl */
  fallthrough_bbl = BblSplitBlock(bbl, (t_ins*) mov2, TRUE/*before*/);
  /* Split again after mov2, creating the end_bbl */
  end_bbl = BblSplitBlock(fallthrough_bbl, (t_ins*) mov2, FALSE/*before*/);

  CfgEdgeCreate(cfg, bbl, end_bbl, ET_JUMP);

  return score;
}/*}}}*/
t_bool LimitSetcc(t_cfg *cfg, t_i386_ins *setcc) 
{
  return (LimitSetccEx(cfg,setcc,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitCmovccEx(t_cfg *cfg, t_i386_ins *cmovcc,t_bool execute)/*{{{*/
{
  t_i386_ins *mov, *jcc;

  t_bbl *bbl = I386_INS_BBL(cmovcc);

  t_bbl *fallthrough_bbl, *end_bbl;

  t_int32 score = 0;

  t_i386_operand *op;
  t_reg dest;
  t_i386_regmode regmode;
  t_i386_operand *src_op;
  t_reg base;
  t_reg index;
  t_uint32 offset;
  int scale;

  t_i386_condition_code condition;


  ScoreRemove(&score,cmovcc);
  ScoreAdd(&score,cmovcc,I386_MOV);
  ScoreAdd(&score,cmovcc,I386_Jcc);
  if(!execute)
    return score;
  mov = dlis_I386InsNewForBbl(bbl);
  jcc = dlis_I386InsNewForBbl(bbl);

  op = I386_INS_DEST(cmovcc);
  dest = I386_OP_BASE(op);
  regmode = I386_OP_REGMODE(op);

  src_op = I386_INS_SOURCE1(cmovcc);
  base = I386_OP_BASE(src_op);
  index = I386_OP_INDEX(src_op);
  offset = I386_OP_IMMEDIATE(src_op);
  scale = I386_OP_SCALE(src_op);

  condition = I386InvertCondition(I386_INS_CONDITION(cmovcc));

  if (I386_OP_TYPE(src_op) == i386_optype_reg)
  {
    dlis_I386InstructionMakeMovToReg(mov, dest, base, 0, regmode);
  }
  else if (I386_OP_TYPE(src_op) == i386_optype_mem)
  {
    dlis_I386InstructionMakeMovToRegFromMem(mov, dest, offset, base, index, scale, dlis_getSizeFromRegmode(regmode), regmode);
    if (I386_OP_FLAGS(I386_INS_SOURCE1(cmovcc)) & I386_OPFLAG_ISRELOCATED)
    {
      t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(cmovcc));
      RelocSetFrom(rel, T_RELOCATABLE(mov));
      I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_SOURCE1(cmovcc)) &= ~I386_OPFLAG_ISRELOCATED;
    }
  }

  I386InstructionMakeCondJump(jcc, condition);

  dlis_I386InsInsertBefore(jcc, cmovcc);
  dlis_I386InsInsertBefore(mov, cmovcc);
  I386InsKill(cmovcc);

  fallthrough_bbl = BblSplitBlock(bbl, (t_ins*) mov, TRUE/*before*/);
  end_bbl = BblSplitBlock(fallthrough_bbl, (t_ins*) mov, FALSE/*before*/);

  CfgEdgeCreate(cfg, bbl, end_bbl, ET_JUMP);

  return score;
}/*}}}*/
t_bool LimitCmovcc(t_cfg *cfg, t_i386_ins *cmovcc) 
{
  return (LimitCmovccEx(cfg,cmovcc,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits instructions:
 * std
 */
t_int32 LimitStdEx(t_cfg *cfg, t_i386_ins *std,t_bool execute) /*{{{*/
{
  t_i386_ins *pushf, *orinst, *popf;

  t_bbl *bbl = INS_BBL((t_ins*) std);

  t_int32 score = 0;
  ScoreRemove(&score,std);
  ScoreAdd(&score,std,I386_PUSHF);
  ScoreAdd(&score,std,I386_OR);
  ScoreAdd(&score,std,I386_POPF);
  if(!execute)
    return score;
  pushf = dlis_I386InsNewForBbl(bbl);
  orinst = dlis_I386InsNewForBbl(bbl);
  popf = dlis_I386InsNewForBbl(bbl);

  I386InstructionMakePushF(pushf);
  dlis_I386InstructionMakeLogicToMem(orinst, I386_OR, (t_uint32)0, I386_REG_ESP, I386_REG_NONE, 1/*scale*/, I386_REG_NONE, 0x400, (t_uint32)4, i386_regmode_full32);
  I386InstructionMakePopF(popf);

  dlis_I386InsInsertBefore(pushf, std);
  dlis_I386InsInsertBefore(orinst, std);
  dlis_I386InsInsertBefore(popf, std);
  I386InsKill(std);
  return score;
}/*}}}*/
t_bool LimitStd(t_cfg *cfg, t_i386_ins *std) 
{
  return (LimitStdEx(cfg,std,TRUE)==0)?FALSE:TRUE;
}

/*
 * Works
 */
t_int32 LimitLeaveEx(t_cfg *cfg, t_i386_ins *leave,t_bool execute) /*{{{*/
{
  t_i386_ins *mov, *pop;

  t_bbl *bbl = INS_BBL((t_ins*) leave);

  t_int32 score = 0;
  ScoreRemove(&score,leave);
  ScoreAdd(&score,leave,I386_MOV);
  ScoreAdd(&score,leave,I386_POP);
  if(!execute)
    return score;
  mov = dlis_I386InsNewForBbl(bbl);
  pop = dlis_I386InsNewForBbl(bbl);

  I386InstructionMakeMovToReg(mov, I386_REG_ESP, I386_REG_EBP, 0/*imm*/);
  I386InstructionMakePop(pop, I386_REG_EBP);

  dlis_I386InsInsertBefore(mov, leave);
  dlis_I386InsInsertBefore(pop, leave);
  I386InsKill(leave);
  return score;
}/*}}}*/
t_bool LimitLeave(t_cfg *cfg, t_i386_ins *leave) 
{
  return (LimitLeaveEx(cfg,leave,TRUE)==0)?FALSE:TRUE;
}

/*
 * Limits the jcc instructions as much as possible
 * by inverting some of the conditions
 */
t_int32 LimitJccEx(t_cfg *cfg, t_i386_ins *jcc,t_bool execute) /*{{{*/

{
  t_i386_condition_code c;
  t_bbl *bbl = I386_INS_BBL(jcc);
  t_bbl *ft_bbl, *jmp_bbl, *new_bbl;
  t_cfg_edge *jmp_e, *ft_e;
  t_uint32 ft_type, jmp_type;
  t_i386_ins *jmp;
  t_int32 score = 0;

  if (dlis_CanInvertJcc(jcc))
  {

    /* Set up the edges */
    ft_e = BBL_SUCC_FIRST(bbl);
    if (CfgEdgeIsFallThrough(ft_e))
    {
      jmp_e = BBL_SUCC_LAST(bbl);
    }
    else
    {
      jmp_e = ft_e;
      ft_e = BBL_SUCC_LAST(bbl);
    }


    /* Change the outgoing edges
     * Fallthrough -> Jump
     * Jump -> Fallthrough
     */
    if (CfgEdgeTestCategoryOr(ft_e, ET_FALLTHROUGH|ET_IPFALLTHRU) &&
	CfgEdgeTestCategoryOr(jmp_e, ET_JUMP|ET_IPJUMP))
    {
      jmp_bbl = CFG_EDGE_TAIL(jmp_e);
      ft_bbl = CFG_EDGE_TAIL(ft_e);
      if(execute)
      {
	/* Change the condition code */
	c = dlis_getInvertedCondition(jcc);
	I386_INS_SET_CONDITION(jcc, c);
	CfgEdgeKill(jmp_e);
	CfgEdgeKill(ft_e);
	jmp_type = ET_JUMP;
	ft_type = ET_FALLTHROUGH;
	ASSERT(!BBL_SUCC_FIRST(bbl), ("Not all succ edges processed!"));
      }
    }
    else
    {
      FATAL(("Unknown edges voor jcc!"));
    }

    ScoreRemove(&score,jcc);
    ScoreAdd(&score,jcc,I386_Jcc);
    if (dlis_HasFallthroughPred(jmp_bbl) || jmp_bbl == bbl || FtPath(jmp_bbl, bbl, NULL))
    {
      ScoreAdd(&score,jcc,I386_JMP);
      if(!execute)
	return score;
      jmp = dlis_I386InsNewForBbl(bbl);
      I386InstructionMakeJump(jmp);
      /* ft_bbl should now be connected using a jmp edge
       * jmp_bbl should now be connected using a fallthrough edge
       */
      dlis_I386InsInsertAfter(jmp, jcc);
      new_bbl = BblSplitBlock(bbl, (t_ins*) jmp, TRUE/*before*/);
      ASSERT(!BBL_SUCC_FIRST(new_bbl), ("NEW BBL should not have any successors!"));

      CfgEdgeCreate(cfg, bbl, ft_bbl, jmp_type);
      CfgEdgeCreate(cfg, new_bbl, jmp_bbl, ET_JUMP);
      return score;
    }
    else
    {
      if(!execute)
	return score;
      /* No double fallthrough, nor loops, we can just switch the edge types. */
      CfgEdgeCreate(cfg, bbl, ft_bbl, jmp_type);
      CfgEdgeCreate(cfg, bbl, jmp_bbl, ft_type);
      return score;
    }
  }
  else
  {
    return score;
  }
}/*}}}*/
t_bool LimitJcc(t_cfg *cfg, t_i386_ins *jcc) {
  return (LimitJccEx(cfg,jcc,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitBswapEx(t_cfg *cfg, t_i386_ins *bswap,t_bool execute) /*{{{*/
{
  t_i386_ins *push, *pop, *pushf, *popf, *mov1, *shl1, *shr1;
  t_i386_ins *mov2, *shl2, *shr2, *mov3, *shl3, *shr3;
  t_i386_ins *orinst;
  t_reg extra_reg, orig_reg;
  t_regset r = InsRegsLiveAfter((t_ins*)bswap);
  t_int32 score = 0;

  /* Push and pop only used when lacking a free register */
  orig_reg = I386_OP_BASE(I386_INS_DEST(bswap));
  extra_reg = FindFreeRegister(bswap);
  if (extra_reg == I386_REG_NONE)
  {
    ScoreRemove(&score,bswap);
    ScoreAdd(&score,bswap,I386_PUSH);
    ScoreAdd(&score,bswap,I386_POP);
    if(execute)
    {
      extra_reg = ChooseRandomRegister();
      while (extra_reg == orig_reg)
      {
	extra_reg = ChooseRandomRegister();
      }

      I386MakeInsForIns(Push, Before, push, bswap, extra_reg, 0/*imm*/);
      I386MakeInsForIns(Pop, After, pop, bswap, extra_reg);
      if(RegsetIn(I386InsUsedRegisters(bswap),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(bswap),I386_REG_ESP))
	FATAL(("ESP corrupt"));
      dlis_CopyAddresses(push, bswap);
      dlis_CopyAddresses(pop, bswap);
    }
  }

  if (RegsetIn(r, I386_CONDREG_CF) ||
      RegsetIn(r, I386_CONDREG_SF) ||
      RegsetIn(r, I386_CONDREG_ZF) ||
      RegsetIn(r, I386_CONDREG_PF) ||
      RegsetIn(r, I386_CONDREG_AF))
  {
    ScoreAdd(&score,bswap,I386_PUSHF);
    ScoreAdd(&score,bswap,I386_POPF);
    if(execute)
    {
      I386MakeInsForIns(PushF, Before, pushf, bswap);
      I386MakeInsForIns(PopF, After, popf, bswap);
      dlis_CopyAddresses(pushf, bswap);
      dlis_CopyAddresses(popf, bswap);
    }
  }

  ScoreAdd(&score,bswap,I386_MOV);
  ScoreAdd(&score,bswap,I386_SHL);
  ScoreAdd(&score,bswap,I386_SHR);
  ScoreAdd(&score,bswap,I386_MOV);
  ScoreAdd(&score,bswap,I386_SHL);
  ScoreAdd(&score,bswap,I386_SHR);
  ScoreAdd(&score,bswap,I386_MOV);
  ScoreAdd(&score,bswap,I386_SHL);
  ScoreAdd(&score,bswap,I386_SHR);
  ScoreAdd(&score,bswap,I386_MOV);
  if(!execute)
    return score;

  dlis_I386MakeInsForIns_4(MovToReg, Before, mov1, bswap, extra_reg, orig_reg, 0/*imm*/, i386_regmode_lo8);
  dlis_I386MakeInsForIns_4(Shift, Before, shl1, bswap, I386_SHL, extra_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_4(Shift, Before, shr1, bswap, I386_SHR, orig_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_4(MovToReg, Before, mov2, bswap, extra_reg, orig_reg, 0/*imm*/, i386_regmode_lo8);
  dlis_I386MakeInsForIns_4(Shift, Before, shl2, bswap, I386_SHL, extra_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_4(Shift, Before, shr2, bswap, I386_SHR, orig_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_4(MovToReg, Before, mov3, bswap, extra_reg, orig_reg, 0/*imm*/, i386_regmode_lo8);
  dlis_I386MakeInsForIns_4(Shift, Before, shl3, bswap, I386_SHL, extra_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_4(Shift, Before, shr3, bswap, I386_SHR, orig_reg, 8, i386_regmode_full32);
  dlis_I386MakeInsForIns_5(OrToReg, Before, orinst, bswap, orig_reg, extra_reg, 0/*imm*/, 4, i386_regmode_full32);

  I386InsKill(bswap);
  return score;
}/*}}}*/
t_bool LimitBswap(t_cfg *cfg, t_i386_ins *bswap) 
{
  return (LimitBswapEx(cfg,bswap,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitRolrEx(t_cfg *cfg, t_i386_ins *rol,t_bool execute) {/*{{{*/
  t_i386_ins *mov, *shl, *shr, *orinst, *push, *pop;
  t_reg reg = FindFreeRegister(rol);
  t_reg dst = I386_OP_BASE(I386_INS_DEST(rol));
  t_reg base = dst;
  t_reg index = I386_OP_INDEX(I386_INS_DEST(rol));
  t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(rol));
  int imm_l, imm_r, tmp;
  t_uint32 offset = I386_OP_IMMEDIATE(I386_INS_DEST(rol));
  t_uint32 scale = I386_OP_SCALE(I386_INS_DEST(rol));

  t_bool is_rol =  (I386_INS_OPCODE(rol) == I386_ROL);
  t_int32 score = 0;

  ScoreRemove(&score,rol);

  if (reg == I386_REG_NONE)
  {
    ScoreAdd(&score,rol,I386_PUSH);
    ScoreAdd(&score,rol,I386_POP);

    if(execute)
    {
      reg = ChooseRandomRegister();
      while (reg == dst)
      {
	reg = ChooseRandomRegister();
      }

      I386MakeInsForIns(Push, Before, push, rol, reg, 0/*imm*/);
      I386MakeInsForIns(Pop, After, pop, rol, reg);
      if(RegsetIn(I386InsUsedRegisters(rol),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(rol),I386_REG_ESP))
	FATAL(("ESP corrupt"));
      dlis_CopyAddresses(push, rol);
      dlis_CopyAddresses(pop, rol);
    }
  }
  ScoreAdd(&score,rol,I386_MOV);
  ScoreAdd(&score,rol,I386_SHL);
  ScoreAdd(&score,rol,I386_SHR);
  ScoreAdd(&score,rol,I386_OR);
  if(!execute)
    return score;

  if (I386_OP_TYPE(I386_INS_SOURCE1(rol)) == i386_optype_imm)
  {
    imm_l = I386_OP_IMMEDIATE(I386_INS_SOURCE1(rol));
    imm_r = 8*(dlis_getSizeFromRegmode(regmode)) - imm_l;
    if (!is_rol)
    {
      tmp = imm_l;
      imm_l = imm_r;
      imm_r = tmp;
    }
  }
  else /* reg */
  {
    FATAL(("ROL/ROR reg unsupported"));
    imm_l = imm_r = -1; /* Will use CX */
  }

  /* The left shift must come last. This way the CF is set correctly */
  if (I386_OP_TYPE(I386_INS_DEST(rol)) == i386_optype_reg)
  {
    I386MakeInsForIns(MovToReg, Before, mov, rol, reg/*dest*/, dst/*src*/, 0/*imm*/);
    dlis_CopyAddresses(mov, rol);
    if (is_rol)
    {
      dlis_I386MakeInsForIns_4(Shift, Before, shr, rol, I386_SHR, reg, imm_r, regmode);
      dlis_I386MakeInsForIns_4(Shift, Before, shl, rol, I386_SHL, dst, imm_l, regmode);
    }
    else
    {
      dlis_I386MakeInsForIns_4(Shift, Before, shl, rol, I386_SHL, dst, imm_l, regmode);
      dlis_I386MakeInsForIns_4(Shift, Before, shr, rol, I386_SHR, reg, imm_r, regmode);
    }
    dlis_I386MakeInsForIns_5(OrToReg, Before, orinst, rol, dst, reg, 0/*imm*/, 1/*memopsize, unused*/, regmode);
  }
  else if (I386_OP_TYPE(I386_INS_DEST(rol)) == i386_optype_mem)
  {
    dlis_I386MakeInsForIns_7(MovToRegFromMem, Before, mov, rol, reg/*dest*/, offset, base, index, scale, dlis_getSizeFromRegmode(regmode), regmode);
    if (is_rol)
    {
      dlis_I386MakeInsForIns_4(Shift, Before, shr, rol, I386_SHR, reg, imm_r, regmode);
      dlis_I386MakeInsForIns_7(ShiftMem, Before, shl, rol, I386_SHL, offset, base, index, scale, imm_l, regmode);
    }
    else
    {
      dlis_I386MakeInsForIns_7(ShiftMem, Before, shl, rol, I386_SHL, offset, base, index, scale, imm_l, regmode);
      dlis_I386MakeInsForIns_4(Shift, Before, shr, rol, I386_SHR, reg, imm_r, regmode);
    }
    dlis_I386MakeInsForIns_8(OrToMem, Before, orinst, rol, offset, base, index, scale, reg, 0/*imm*/, 1/*memopsize, unused*/, regmode);
    /* Relocations */
    if (I386_OP_FLAGS(I386_INS_DEST(rol)) & I386_OPFLAG_ISRELOCATED)
    {
      t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(rol));
      RelocSetFrom(rel, T_RELOCATABLE(mov));
      rel = RelocTableDupReloc(RELOC_TABLE(rel), rel);
      RelocSetFrom(rel, T_RELOCATABLE(shl));
      rel = RelocTableDupReloc(RELOC_TABLE(rel), rel);
      RelocSetFrom(rel, T_RELOCATABLE(orinst));
      I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(shl)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(orinst)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(rol)) &= ~I386_OPFLAG_ISRELOCATED;
    }
  }
  I386InsKill(rol);
  return score;
}/*}}}*/
t_bool LimitRolr(t_cfg *cfg, t_i386_ins *rolr) 
{
  return (LimitRolrEx(cfg,rolr,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitAddEx(t_cfg *cfg, t_i386_ins *add,t_bool execute) /*{{{*/
{
  t_i386_ins *push, *pop, *mov, *subz, *sub;
  t_reg dst = I386_OP_BASE(I386_INS_DEST(add));
  t_reg src, base, index;
  dlis_reg_list wo = DlisRegList(3);
  int offset, imm;
  t_uint32 scale;
  t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(add));
  t_reg reg = FindFreeRegisterForRegmode(add, regmode);
  t_regset regset = InsRegsLiveAfter((t_ins *) add);

  t_int32 score=0;

  /* sub and add have different behaviour relating OF and CF:
   * sub indicates a borrow with OF and CF, add indicates
   * a carry with OF and CF */
  if (RegsetIn(regset, I386_CONDREG_OF) ||
      RegsetIn(regset, I386_CONDREG_CF))
  {
    return score;
  } 
  ScoreRemove(&score,add);

  if (RegsetIn(regset, I386_CONDREG_AF))
  {
    /* Do not use eax if the AF flag is live */
    DlisRegListAdd(wo, I386_REG_EAX);
  }

  if (I386_OP_TYPE(I386_INS_SOURCE1(add)) != i386_optype_imm)
  {
    switch (I386_OP_TYPE(I386_INS_SOURCE1(add)))
    {
      case i386_optype_reg:
	DlisRegListAdd(wo, I386_OP_BASE(I386_INS_SOURCE1(add)));
	break;
      case i386_optype_mem:
	DlisRegListAdd(wo, I386_OP_BASE(I386_INS_SOURCE1(add)));
	DlisRegListAdd(wo, I386_OP_INDEX(I386_INS_SOURCE1(add)));
	break;
      case i386_optype_imm:
	break;
      default:
	FATAL(("Unknown optype for add source1"));
    }

    switch (I386_OP_TYPE(I386_INS_DEST(add)))
    {
      case i386_optype_reg:
	DlisRegListAdd(wo, I386_OP_BASE(I386_INS_DEST(add)));
	break;
      case i386_optype_mem:
	DlisRegListAdd(wo, I386_OP_BASE(I386_INS_DEST(add)));
	DlisRegListAdd(wo, I386_OP_INDEX(I386_INS_DEST(add)));
	break;
      default:
	FATAL(("Unknown optype for add dest of @I", add));
    }

    if (reg == I386_REG_NONE)
    {
      ScoreAdd(&score,add,I386_PUSH);
      ScoreAdd(&score,add,I386_POP);
      /* Find a register that is not used by the add instruction
       * and push/pop it. */
      if(execute)
      {
	reg = ChooseRandomWithout(wo, regmode);
	I386MakeInsForIns(Push, Before, push, add, reg, 0);
	I386MakeInsForIns(Pop, After, pop, add, reg);
	if(RegsetIn(I386InsUsedRegisters(add),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(add),I386_REG_ESP))
	{
	  if(I386_OP_BASE(I386_INS_DEST(add)) == I386_REG_ESP)
	  {
	    if(I386_OP_TYPE(I386_INS_DEST(add))!=i386_optype_mem)
	      FATAL(("esp corrupted"));
	    if(regmode != i386_regmode_full32)
	      FATAL(("esp corrupted"));
	    I386_OP_IMMEDIATE(I386_INS_DEST(add))+=4;
	  }
	  if(I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_reg || I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_mem)
	  {
	    if(I386_OP_BASE(I386_INS_SOURCE1(add)) == I386_REG_ESP)
	    {
	      if(I386_OP_TYPE(I386_INS_SOURCE1(add))!=i386_optype_mem)
		FATAL(("esp corrupted"));
	      if(regmode != i386_regmode_full32)
		FATAL(("esp corrupted"));
	      I386_OP_IMMEDIATE(I386_INS_SOURCE1(add))+=4;
	    }
	  }
	}
	dlis_CopyAddresses(push, add);
	dlis_CopyAddresses(pop, add);
      }
    }

    ScoreAdd(&score,add,I386_MOV);
    if(execute)
    {
      /* Mov only needed when not adding an immediate */
      I386MakeInsForIns(MovToReg, Before, mov, add, reg, I386_REG_NONE, 0/*imm*/);
      dlis_CopyAddresses(mov, add);
    }
  }

  switch (I386_OP_TYPE(I386_INS_SOURCE1(add)))
  {
    case i386_optype_reg:
      src = I386_OP_BASE(I386_INS_SOURCE1(add));
      ScoreAdd(&score,add,I386_SUB);
      if(execute)
	dlis_I386MakeInsForIns_4(Sub, Before, subz, add, reg, src, 0, regmode);
      break;
    case i386_optype_imm:
      /* It would be silly to calculate this during execution... */
      break;
    case i386_optype_mem:
      offset = I386_OP_IMMEDIATE(I386_INS_SOURCE1(add));
      base = I386_OP_BASE(I386_INS_SOURCE1(add));
      index = I386_OP_INDEX(I386_INS_SOURCE1(add));
      scale = I386_OP_SCALE(I386_INS_SOURCE1(add));
      ScoreAdd(&score,add,I386_SUB);
      if(execute)
	dlis_I386MakeInsForIns_6(SubMem, Before, subz, add, reg, offset, base, index, scale, regmode);
      break;
    default:
      FATAL(("Unknown add source optype"));
  }

  imm = I386_OP_IMMEDIATE(I386_INS_SOURCE1(add));
  switch (I386_OP_TYPE(I386_INS_DEST(add)))
  {
    case i386_optype_reg:
      if (I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_imm)
      {
	if (dlis_ImmediateSignedMinimum(add))
	{
	  int imm2;

	  imm = -1*imm;
	  imm2 = (diablo_rand_next()%(imm-1)) + 1; /* [1, imm-1] */
	  ScoreAdd(&score,add,I386_SUB);
	  ScoreAdd(&score,add,I386_SUB);
	  if(execute)
	  {
	    dlis_I386MakeInsForIns_4(Sub, Before, sub, add, dst, I386_REG_NONE, imm2, regmode);
	    dlis_I386MakeInsForIns_4(Sub, Before, sub, add, dst, I386_REG_NONE, imm-imm2, regmode);
	  }
	}
	else
	{
	  ScoreAdd(&score,add,I386_SUB);
	  if(execute)
	    dlis_I386MakeInsForIns_4(Sub, Before, sub, add, dst, I386_REG_NONE, -1*imm, regmode);
	}
      }
      else
      {
	ScoreAdd(&score,add,I386_SUB);
	if(execute)
	  dlis_I386MakeInsForIns_4(Sub, Before, sub, add, dst, reg, 0, regmode);
      }
      break;
    case i386_optype_mem:
      offset = I386_OP_IMMEDIATE(I386_INS_DEST(add));
      base = I386_OP_BASE(I386_INS_DEST(add));
      index = I386_OP_INDEX(I386_INS_DEST(add));
      scale = I386_OP_SCALE(I386_INS_DEST(add));
      if (I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_imm)
      {
	if (dlis_ImmediateSignedMinimum(add))
	{
	  int imm2;
	  imm  = -1*imm;
	  imm2 = (diablo_rand_next()%(imm-1)) + 1; /* [1, imm-1] */
	  ScoreAdd(&score,add,I386_SUB);
	  ScoreAdd(&score,add,I386_SUB);
	  if(execute)
	  {
	    dlis_I386MakeInsForIns_7(SubToMem, Before, sub, add, offset, base, index, scale, I386_REG_NONE, imm2, regmode);
	    dlis_I386MakeInsForIns_7(SubToMem, Before, sub, add, offset, base, index, scale, I386_REG_NONE, imm-imm2, regmode);
	  }
	}
	else
	{
	  ScoreAdd(&score,add,I386_SUB);
	  if(execute)
	    dlis_I386MakeInsForIns_7(SubToMem, Before, sub, add, offset, base, index, scale, I386_REG_NONE, -1*imm, regmode);
	}
      }
      else
      {
	ScoreAdd(&score,add,I386_SUB);
	if(execute)
	  dlis_I386MakeInsForIns_7(SubToMem, Before, sub, add, offset, base, index, scale, reg, 0, regmode);
      }
      break;
    default:
      FATAL(("Unknown add destination optype"));
  }

  if(!execute)
    return score;

  if (I386_OP_FLAGS(I386_INS_DEST(add)) & I386_OPFLAG_ISRELOCATED)
  {
    t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(add));
    RelocSetFrom(rel, T_RELOCATABLE(sub));
    I386_OP_FLAGS(I386_INS_DEST(sub)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(add)) &= ~I386_OPFLAG_ISRELOCATED;
  }

  if (I386_OP_FLAGS(I386_INS_SOURCE1(add)) & I386_OPFLAG_ISRELOCATED)
  {
    t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(add));
    if (I386_OP_TYPE(I386_INS_SOURCE1(add)) == i386_optype_imm)
    {
      t_string first;
	  t_string new_reloc_code, old_reloc_code;
      /* We need to invert the relocation value before writing it. */
      t_string_array *parts = StringDivide(RELOC_CODE(rel), "\\", TRUE, FALSE);

      ASSERT(StringArrayNStrings(parts) >= 2, ("Not enough parts for relocation."));
      first = parts->first->string;
      parts->first->string = StringConcat3("s0000", first, "-");
      Free(first);
      new_reloc_code = StringArrayJoin(parts, "\\");
      old_reloc_code = RELOC_CODE(rel);
      Free(old_reloc_code);
      StringArrayFree(parts);
      RELOC_SET_CODE(rel, new_reloc_code);
      RelocSetFrom(rel, T_RELOCATABLE(sub));
      I386_OP_FLAGS(I386_INS_SOURCE1(sub)) |= I386_OPFLAG_ISRELOCATED;
    }
    else
    {
      RelocSetFrom(rel, T_RELOCATABLE(subz));
      I386_OP_FLAGS(I386_INS_SOURCE1(subz)) |= I386_OPFLAG_ISRELOCATED;
    }
    I386_OP_FLAGS(I386_INS_SOURCE1(add)) &= ~I386_OPFLAG_ISRELOCATED;
  }

  I386InsKill(add);
  DlisRegListDestruct(wo);
  return score;
}/*}}}*/

t_bool LimitAdd(t_cfg *cfg, t_i386_ins *add) 
{
  return (LimitAddEx(cfg,add,TRUE)==0)?FALSE:TRUE;
}

/* ONLY USE IN SINGLE THREADED APPLICATIONS!!! */
t_int32 LimitXchgEx(t_cfg *cfg, t_i386_ins *xchg,t_bool execute) /*{{{*/
{
  t_i386_ins *push, *pop, *mov1, *mov2, *mov3;
  t_reg reg = FindFreeRegister(xchg);
  t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(xchg));
  t_i386_operand *src = I386_INS_SOURCE1(xchg);
  t_i386_operand *dst = I386_INS_DEST(xchg);
  t_int32 score = 0;

  ScoreRemove(&score,xchg);

  if (reg == I386_REG_NONE)
  {
    ScoreAdd(&score,xchg,I386_PUSH);
    ScoreAdd(&score,xchg,I386_POP);

    if(execute)
    {
      reg = ChooseRandomRegister();
      I386MakeInsForIns(Push, Before, push, xchg, reg, 0/*imm*/);
      I386MakeInsForIns(Pop, After, pop, xchg, reg);
      if(RegsetIn(I386InsUsedRegisters(xchg),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(xchg),I386_REG_ESP))
	FATAL(("ESP corrupt"));
      dlis_CopyAddresses(push, xchg);
      dlis_CopyAddresses(pop, xchg);
    }
  }
  //for src
  ScoreAdd(&score,xchg,I386_MOV);
  //for dst
  ScoreAdd(&score,xchg,I386_MOV);
  ScoreAdd(&score,xchg,I386_MOV);
  if(!execute)
    return score;

  switch (I386_OP_TYPE(src))
  {
    case i386_optype_reg:
      dlis_I386MakeInsForIns_4(MovToReg, After, mov3, xchg,
	  I386_OP_BASE(src),
	  reg,
	  0 /*imm*/,
	  regmode);
      break;
    case i386_optype_mem:
      dlis_I386MakeInsForIns_7(MovToMem, After, mov3, xchg,
	  I386_OP_IMMEDIATE(src),
	  I386_OP_BASE(src),
	  I386_OP_INDEX(src),
	  I386_OP_SCALE(src),
	  reg,
	  0 /*imm*/,
	  regmode);
      break;
    default:
      FATAL(("Unknown source optype for xchg: @I", xchg));
  }

  switch (I386_OP_TYPE(dst))
  {
    case i386_optype_reg:
      dlis_I386MakeInsForIns_4(MovToReg, Before, mov1, xchg,
	  reg,
	  I386_OP_BASE(dst),
	  0 /*imm*/,
	  regmode);
      switch (I386_OP_TYPE(src))
      {
	case i386_optype_reg:
	  dlis_I386MakeInsForIns_4(MovToReg, Before, mov2, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_BASE(src),
	      0 /*imm*/,
	      regmode);
	  break;
	case i386_optype_mem:
	  dlis_I386MakeInsForIns_7(MovToRegFromMem, Before, mov2, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_IMMEDIATE(src),
	      I386_OP_BASE(src),
	      I386_OP_INDEX(src),
	      I386_OP_SCALE(src),
	      dlis_getSizeFromRegmode(regmode),
	      regmode);
	  break;
	default:
	  FATAL(("Unknown source optype for xchg: @I", xchg));
      }
      break;
    case i386_optype_mem:
      /* Only possible to mov to reg */
      ASSERT(I386_OP_TYPE(src) == i386_optype_reg,
	  ("Unknown source optype for xchg: @I, %d", xchg, I386_OP_TYPE(src)));
      dlis_I386MakeInsForIns_7(MovToRegFromMem, Before, mov1, xchg,
	  reg,
	  I386_OP_IMMEDIATE(dst),
	  I386_OP_BASE(dst),
	  I386_OP_INDEX(dst),
	  I386_OP_SCALE(dst),
	  dlis_getSizeFromRegmode(regmode),
	  regmode);
      dlis_I386MakeInsForIns_7(MovToMem, Before, mov2, xchg,
	  I386_OP_IMMEDIATE(dst),
	  I386_OP_BASE(dst),
	  I386_OP_INDEX(dst),
	  I386_OP_SCALE(dst),
	  I386_OP_BASE(src),
	  0 /*imm*/,
	  regmode);
      break;
    default:
      FATAL(("Unknown dest optype for xchg: @I || %d", xchg, I386_OP_TYPE(dst)));
  }

  if (I386_OP_FLAGS(I386_INS_SOURCE1(xchg)) & I386_OPFLAG_ISRELOCATED)
  {
    /* source1 is mem, so dest is reg */
	t_reloc *rel2;
    t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(xchg));
    RelocSetFrom(rel, T_RELOCATABLE(mov2));

    rel2 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel2, T_RELOCATABLE(mov3));

    I386_OP_FLAGS(I386_INS_SOURCE1(mov2)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(mov3)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(xchg)) &= ~I386_OPFLAG_ISRELOCATED;
  }

  if (I386_OP_FLAGS(I386_INS_DEST(xchg)) & I386_OPFLAG_ISRELOCATED)
  {
	t_reloc *rel2;
    /* dest is mem, so source1 is reg */
    t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(xchg));
    RelocSetFrom(rel, T_RELOCATABLE(mov1));

    rel2 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel2, T_RELOCATABLE(mov2));

    I386_OP_FLAGS(I386_INS_SOURCE1(mov1)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(mov2)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(xchg)) &= ~I386_OPFLAG_ISRELOCATED;
  }
  I386InsKill(xchg);
  return score;
}/*}}}*/
t_bool LimitXchg(t_cfg *cfg, t_i386_ins *xchg) 
{
  return (LimitXchgEx(cfg,xchg,TRUE)==0)?FALSE:TRUE;
}

/* ONLY USE IN SINGLE THREADED APPLICATIONS!!! */
/* Use 3 xors to limit an xchg without the need of a free register.
 * Idea: http://graphics.stanford.edu/~seander/bithacks.html */
t_int32 LimitXchgXorEx(t_cfg *cfg, t_i386_ins *xchg,t_bool execute) /*{{{*/
{
  t_i386_ins *xor1, *xor2, *xor3;
  t_i386_operand *src = I386_INS_SOURCE1(xchg);
  t_i386_operand *dst = I386_INS_DEST(xchg);
  t_i386_regmode regmode = I386_OP_REGMODE(dst);
  t_int32 score = 0;

  /* special case, does not work with the xors */
  if (I386_OP_TYPE(dst) == i386_optype_reg &&
      I386_OP_TYPE(src) == i386_optype_reg &&
      I386_OP_BASE(dst) == I386_OP_BASE(src))
  {
    /* xchg %reg,%reg
     * Useless instruction, just kill it */
    //overridden, not our job
    return score;
    //    I386InsKill(xchg);
    //  return TRUE;
  }

  ScoreRemove(&score,xchg);
  ScoreAdd(&score,xchg, I386_XOR);
  ScoreAdd(&score,xchg, I386_XOR);
  ScoreAdd(&score,xchg, I386_XOR);
  if(!execute)
    return score;

  switch (I386_OP_TYPE(dst))
  {
    case i386_optype_reg:
      switch (I386_OP_TYPE(src))
      {
	case i386_optype_reg:
	  dlis_I386MakeInsForIns_4(Xor, Before, xor1, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_BASE(src),
	      0 /*imm*/,
	      regmode);
	  dlis_I386MakeInsForIns_4(Xor, Before, xor2, xchg,
	      I386_OP_BASE(src),
	      I386_OP_BASE(dst),
	      0 /*imm*/,
	      regmode);
	  dlis_I386MakeInsForIns_4(Xor, Before, xor3, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_BASE(src),
	      0 /*imm*/,
	      regmode);
	  break;
	case i386_optype_mem:
	  dlis_I386MakeInsForIns_6(XorFromMem, Before, xor1, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_IMMEDIATE(src),
	      I386_OP_BASE(src),
	      I386_OP_INDEX(src),
	      I386_OP_SCALE(src),
	      regmode);
	  dlis_I386MakeInsForIns_7(XorToMem, Before, xor2, xchg,
	      I386_OP_IMMEDIATE(src),
	      I386_OP_BASE(src),
	      I386_OP_INDEX(src),
	      I386_OP_SCALE(src),
	      I386_OP_BASE(dst),
	      0 /*imm*/,
	      regmode);
	  dlis_I386MakeInsForIns_6(XorFromMem, Before, xor3, xchg,
	      I386_OP_BASE(dst),
	      I386_OP_IMMEDIATE(src),
	      I386_OP_BASE(src),
	      I386_OP_INDEX(src),
	      I386_OP_SCALE(src),
	      regmode);
	  break;
	default:
	  FATAL(("Unknown source optype for xchg: @I", xchg));
      }
      break; /* dst reg */
    case i386_optype_mem:
      ASSERT(I386_OP_TYPE(src) == i386_optype_reg,
	  ("Unknown src optype for xchg: @I", xchg));
      dlis_I386MakeInsForIns_7(XorToMem, Before, xor1, xchg,
	  I386_OP_IMMEDIATE(dst),
	  I386_OP_BASE(dst),
	  I386_OP_INDEX(dst),
	  I386_OP_SCALE(dst),
	  I386_OP_BASE(src),
	  0 /*imm*/,
	  regmode);
      dlis_I386MakeInsForIns_6(XorFromMem, Before, xor2, xchg,
	  I386_OP_BASE(src),
	  I386_OP_IMMEDIATE(dst),
	  I386_OP_BASE(dst),
	  I386_OP_INDEX(dst),
	  I386_OP_SCALE(dst),
	  regmode);
      dlis_I386MakeInsForIns_7(XorToMem, Before, xor3, xchg,
	  I386_OP_IMMEDIATE(dst),
	  I386_OP_BASE(dst),
	  I386_OP_INDEX(dst),
	  I386_OP_SCALE(dst),
	  I386_OP_BASE(src),
	  0 /*imm*/,
	  regmode);
      break;
    default:
      FATAL(("Unknown dest optype for xchg: @I", xchg));
  }

  if (I386_OP_FLAGS(I386_INS_SOURCE1(xchg)) & I386_OPFLAG_ISRELOCATED)
  {
	t_reloc *rel2, *rel3;
    /* source1 is mem, so dest is reg */
    t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(xchg));
    RelocSetFrom(rel, T_RELOCATABLE(xor1));

    rel2 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel2, T_RELOCATABLE(xor2));

    rel3 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel3, T_RELOCATABLE(xor3));

    I386_OP_FLAGS(I386_INS_SOURCE1(xor1)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(xor2)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(xor3)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(xchg)) &= ~I386_OPFLAG_ISRELOCATED;
  }

  if (I386_OP_FLAGS(I386_INS_DEST(xchg)) & I386_OPFLAG_ISRELOCATED)
  {
	t_reloc *rel, *rel2, *rel3;
    /* dest is mem, so source1 is reg */
    rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(xchg));
    RelocSetFrom(rel, T_RELOCATABLE(xor1));

    rel2 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel2, T_RELOCATABLE(xor2));

    rel3 = RelocTableDupReloc(RELOC_TABLE(rel), rel);
    RelocSetFrom(rel3, T_RELOCATABLE(xor3));

    I386_OP_FLAGS(I386_INS_DEST(xor1)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(xor2)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(xor3)) |= I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_DEST(xchg)) &= ~I386_OPFLAG_ISRELOCATED;
  }

  I386InsKill(xchg);

  return score;
}/*}}}*/
t_bool LimitXchgXor(t_cfg *cfg, t_i386_ins *xchg) 
{
  return (LimitXchgXorEx(cfg,xchg,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitDecEx(t_cfg *cfg, t_i386_ins *dec,t_bool execute) /*{{{*/
{
  t_i386_ins *sub;
  t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(dec));
  t_regset r = InsRegsLiveAfter((t_ins*)dec);

  t_int32 score= 0;
  /*
   * dec does not set CF, add does, so we must filter
   * CF-sensitive cases out
   */
  if (!RegsetIn(r, I386_CONDREG_CF))
  {
    ScoreRemove(&score,dec);
    ScoreAdd(&score,dec,I386_ADD);
    if(!execute)
      return score;
    switch (I386_OP_TYPE(I386_INS_DEST(dec)))
    {
      case i386_optype_reg:
	dlis_I386MakeInsForIns_4(Sub, Before, sub, dec,
	    I386_OP_BASE(I386_INS_DEST(dec)),
	    I386_REG_NONE,
	    1 /*imm*/,
	    regmode);
	break;
      case i386_optype_mem:
	dlis_I386MakeInsForIns_7(SubToMem, Before, sub, dec,
	    I386_OP_IMMEDIATE(I386_INS_DEST(dec)),
	    I386_OP_BASE(I386_INS_DEST(dec)),
	    I386_OP_INDEX(I386_INS_DEST(dec)),
	    I386_OP_SCALE(I386_INS_DEST(dec)),
	    I386_REG_NONE,
	    1 /*imm*/,
	    regmode);
	break;
      default:
	FATAL(("Unknown optype for dec: @I", dec));
    }

    if (I386_OP_FLAGS(I386_INS_DEST(dec)) & I386_OPFLAG_ISRELOCATED)
    {
      t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(dec));
      RelocSetFrom(rel, T_RELOCATABLE(sub));
      I386_OP_FLAGS(I386_INS_DEST(sub)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(I386_INS_DEST(dec)) &= ~I386_OPFLAG_ISRELOCATED;
    }

    I386InsKill(dec);
    return score;
  }
  else
  {
    return score;
  }
} /*}}}*/
t_bool LimitDec(t_cfg *cfg, t_i386_ins *dec) 
{
  return (LimitDecEx(cfg,dec,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitTestEx(t_cfg *cfg, t_i386_ins *test, t_bool execute) /*{{{*/
{
  t_i386_ins *push, *pop, *mov, *andinst;
  t_i386_operand *src1 = I386_INS_SOURCE1(test); /* reg or mem */
  t_i386_operand *src2 = I386_INS_SOURCE2(test); /* reg or imm */
  t_i386_regmode regmode;
  t_regset regset = InsRegsLiveAfter((t_ins *) test);
  t_reg reg;
  t_int32 score = 0;

  ScoreRemove(&score,test);
  ScoreAdd(&score,test,I386_AND);

  if (I386_OP_TYPE(src1) == i386_optype_reg &&
      !RegsetIn(regset, I386_OP_BASE(src1)))
  {
    if(!execute)
      return score;

    regmode = I386_OP_REGMODE(src1);
    /* We can alter the source1 register, it isn't live */
    dlis_I386MakeInsForIns_5(AndToReg, Before, andinst, test,
	I386_OP_BASE(src1),
	(I386_OP_TYPE(src2) == i386_optype_reg ?
	 I386_OP_BASE(src2) : I386_REG_NONE),
	I386_OP_IMMEDIATE(src2),
	dlis_getSizeFromRegmode(regmode),
	regmode);
  }
  else if (I386_OP_TYPE(src2) == i386_optype_reg &&
      !RegsetIn(regset, I386_OP_BASE(src2)))
  {
    if(!execute)
      return score;
    regmode = I386_OP_REGMODE(src2);
    /* We can alter the source2 register, it isn't live */
    if (I386_OP_TYPE(src1) == i386_optype_reg ||
	I386_OP_TYPE(src1) == i386_optype_imm)
    {
      dlis_I386MakeInsForIns_5(AndToReg, Before, andinst, test,
	  I386_OP_BASE(src2),
	  (I386_OP_TYPE(src1) == i386_optype_reg ?
	   I386_OP_BASE(src1) : I386_REG_NONE),
	  I386_OP_IMMEDIATE(src1),
	  dlis_getSizeFromRegmode(regmode),
	  regmode);
    }
    else if (I386_OP_TYPE(src1) == i386_optype_mem)
    {
      dlis_I386MakeInsForIns_7(AndFromMem, Before, andinst, test,
	  I386_OP_BASE(src2),
	  I386_OP_IMMEDIATE(src1),
	  I386_OP_BASE(src1),
	  I386_OP_INDEX(src1),
	  I386_OP_SCALE(src1),
	  dlis_getSizeFromRegmode(regmode),
	  regmode);
    }
    else
    {
      FATAL(("Unknown source1 optype for @I", test));
    }
  }
  else
  {
    /* First step: find a free register */
    regmode = (I386_OP_TYPE(src1) == i386_optype_reg) ?
      I386_OP_REGMODE(src1) : (
	  (I386_OP_TYPE(src2) == i386_optype_reg) ? 
	  I386_OP_REGMODE(src2) : 
	  dlis_getRegmodeFromSize(I386_OP_MEMOPSIZE(src1))
	  );
    reg = FindFreeRegisterForRegmode(test, regmode);
    if (reg == I386_REG_NONE)
    {
      /* Too bad, no free register available,
       * we need to create one */
      ScoreAdd(&score,test,I386_PUSH);
      ScoreAdd(&score,test,I386_POP);

      if(execute)
      {
	dlis_reg_list reglist = DlisRegList(3);
	DlisRegListAddFromIns(reglist, test);
	reg = ChooseRandomWithout(reglist, regmode);
	DlisRegListDestruct(reglist);

	I386MakeInsForIns(Push, Before, push, test, reg, 0/*imm*/);
	I386MakeInsForIns(Pop, After, pop, test, reg);
	if(RegsetIn(I386InsUsedRegisters(test),I386_REG_ESP) || RegsetIn(I386InsDefinedRegisters(test),I386_REG_ESP))
	{
	  if(I386_OP_BASE(src1) == I386_REG_ESP)
	  {
	    if(I386_OP_TYPE(src1)!=i386_optype_mem)
	      FATAL(("esp corrupted"));
	    //if(regmode != i386_regmode_full32)
	    //  FATAL(("esp corrupted"));
	    I386_OP_IMMEDIATE(src1)+=4;
	  }
	  if(I386_OP_BASE(src2) == I386_REG_ESP)
	  {
	    if(I386_OP_TYPE(src2)!=i386_optype_mem)
	      FATAL(("esp corrupted"));
	    //if(regmode != i386_regmode_full32)
	    //  FATAL(("esp corrupted"));
	    I386_OP_IMMEDIATE(src2)+=4;
	  }
	}
	dlis_CopyAddresses(push, test);
	dlis_CopyAddresses(pop, test);
      }
    }

    ScoreAdd(&score,test,I386_MOV);
    if(!execute)
      return score;

    if (I386_OP_TYPE(src1) == i386_optype_reg)
    {
      /*regmode = I386_OP_REGMODE(src1);*/
      dlis_I386MakeInsForIns_4(MovToReg, Before, mov, test,
	  reg, I386_OP_BASE(src1), 0/*imm*/, regmode);
    }
    /* If src1 is a memory location, move the contents
     * of that memory location to the free register */
    else if (I386_OP_TYPE(src1) == i386_optype_mem)
    {
      int memopsize = I386_OP_MEMOPSIZE(src1);
      /*regmode = (I386_OP_TYPE(src2) == i386_optype_reg) ? 
	I386_OP_REGMODE(src2) : dlis_getRegmodeFromSize(memopsize);*/
      dlis_I386MakeInsForIns_7(MovToRegFromMem, Before, mov, test,
	  reg,
	  I386_OP_IMMEDIATE(src1),
	  I386_OP_BASE(src1),
	  I386_OP_INDEX(src1),
	  I386_OP_SCALE(src1),
	  memopsize,
	  regmode);
      /* We might need a relocation here */
      if (I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED)
      {
	t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(test));
	RelocSetFrom(rel, T_RELOCATABLE(mov));
	I386_OP_FLAGS(I386_INS_SOURCE1(mov)) |= I386_OPFLAG_ISRELOCATED;
	I386_OP_FLAGS(src1) &= ~I386_OPFLAG_ISRELOCATED;
      }
    }
    else
    {
      FATAL(("Unknown source1 optype %d for @I",
	    I386_OP_TYPE(src1), test));
    }
    /* Create "and reg<-src2" */
    dlis_I386MakeInsForIns_5(AndToReg, Before, andinst, test,
	reg,
	(I386_OP_TYPE(src2) == i386_optype_reg ?
	 I386_OP_BASE(src2) : I386_REG_NONE),
	I386_OP_IMMEDIATE(src2),
	I386_OP_IMMEDSIZE(src2),
	regmode);
    if (I386_OP_FLAGS(src2) & I386_OPFLAG_ISRELOCATED)
    {
      t_reloc *rel = RELOC_REF_RELOC(I386_INS_REFERS_TO(test));
      RelocSetFrom(rel, T_RELOCATABLE(andinst));
      I386_OP_FLAGS(I386_INS_SOURCE1(andinst)) |= I386_OPFLAG_ISRELOCATED;
      I386_OP_FLAGS(src2) &= ~I386_OPFLAG_ISRELOCATED;
    }
  }

  I386InsKill(test);
  return score;
} /*}}}*/
t_bool LimitTest(t_cfg *cfg, t_i386_ins *test) 
{
  return (LimitTestEx(cfg,test,TRUE)==0)?FALSE:TRUE;
}

t_int32 LimitNopEx(t_cfg *cfg, t_i386_ins *nop,t_bool execute) /*{{{*/
{
  //banckaer: overruled, not our job
  //I386InsKill(nop);
  //return TRUE;
  return 0;
} /*}}}*/
t_bool LimitNop(t_cfg *cfg, t_i386_ins *nop) /*{{{*/
{
  return (LimitNopEx(cfg,nop,TRUE)==0)?FALSE:TRUE;
} /*}}}*/

t_int32 LimitSbbEx(t_cfg *cfg, t_i386_ins *sbb, t_bool execute) /*{{{*/
{
  t_i386_ins *mov, *jcc, *sub1, *sub2;
  t_i386_operand *src = I386_INS_SOURCE1(sbb);
  t_i386_operand *dst = I386_INS_DEST(sbb);
  t_i386_regmode regmode;
  t_bbl *fallthrough_bbl, *end_bbl, *bbl = I386_INS_BBL(sbb);
  t_i386_condition_code condition = I386_CONDITION_AE;

  t_int32 score = 0;

  ScoreRemove(&score,sbb);
  ScoreAdd(&score,sbb,I386_Jcc);
  if(execute)
  {
    I386MakeInsForIns(CondJump, Before, jcc, sbb, condition);
    dlis_CopyAddresses(jcc, sbb);
  }

  ScoreAdd(&score,sbb,I386_SUB);
  ScoreAdd(&score,sbb,I386_SUB);

  if (I386_OP_TYPE(src) == i386_optype_imm)
  {
    /* sub imm, dst
     * jcc
     * sub 1, dst */
    int imm = I386_OP_IMMEDIATE(src);

    if(!execute)
      return score;

    if (I386_OP_TYPE(dst) == i386_optype_reg)
    {
      t_reg dst_reg = I386_OP_BASE(dst);
      regmode = I386_OP_REGMODE(dst);
      dlis_I386MakeInsForIns_4(Sub, Before, sub1, jcc, dst_reg, 
	  I386_REG_NONE, imm, regmode);
      dlis_I386MakeInsForIns_4(Sub, After, sub2, jcc, dst_reg, 
	  I386_REG_NONE, 1, regmode);
    }
    else if (I386_OP_TYPE(dst) == i386_optype_mem)
    {
      int offset = I386_OP_IMMEDIATE(dst);
      t_reg base = I386_OP_BASE(dst);
      t_reg index = I386_OP_INDEX(dst);
      int scale = I386_OP_SCALE(dst);
      regmode = dlis_getRegmodeFromSize(I386_OP_MEMOPSIZE(dst));
      dlis_I386MakeInsForIns_7(SubToMem, Before, sub1, jcc, 
	  offset, base, index, scale, I386_REG_NONE, imm, regmode);
      dlis_I386MakeInsForIns_7(SubToMem, After, sub2, jcc, 
	  offset, base, index, scale, I386_REG_NONE, 1, regmode);
    }
    else
    {
      FATAL(("Unknown destination optype for sbb: @I", sbb));
    }
  }
  else if (I386_OP_TYPE(src) == i386_optype_reg)
  {
	t_reg src_reg = I386_OP_BASE(src);
    regmode = I386_OP_REGMODE(src);
    if (I386_OP_TYPE(dst) == i386_optype_reg)
    {
      t_reg dst_reg = I386_OP_BASE(dst);
      if (src_reg == dst_reg)
      {
	/* Special case! */
	ScoreAdd(&score,sbb,I386_MOV);
	if(!execute)
	  return score;
	dlis_I386MakeInsForIns_4(MovToReg, Before, mov, jcc,
	    dst_reg, I386_REG_NONE, 0, regmode);
	dlis_I386MakeInsForIns_4(Sub, Before, sub1, sbb,
	    dst_reg, I386_REG_NONE, 1, regmode);
	dlis_I386MakeInsForIns_4(Sub, Before, sub2, sbb,
	    dst_reg, I386_REG_NONE, 0, regmode);
      }
      else
      {
	if(!execute)
	  return score;
	dlis_I386MakeInsForIns_4(Sub, Before, sub1, sbb,
	    dst_reg, I386_REG_NONE, 1, regmode);
	dlis_I386MakeInsForIns_4(Sub, Before, sub2, sbb,
	    dst_reg, src_reg, 0, regmode);
      }
    }
    else if (I386_OP_TYPE(dst) == i386_optype_mem)
    {
      int offset = I386_OP_IMMEDIATE(dst);
      t_reg base = I386_OP_BASE(dst);
      t_reg index = I386_OP_INDEX(dst);
      int scale = I386_OP_SCALE(dst);
      if(!execute)
	return score;
      dlis_I386MakeInsForIns_7(SubToMem, Before, sub1, sbb,
	  offset, base, index, scale, I386_REG_NONE, 1, regmode);
      dlis_I386MakeInsForIns_7(SubToMem, Before, sub2, sbb,
	  offset, base, index, scale, src_reg, 0, regmode);
    }
    else
    {
      FATAL(("Unknown destination optype for sbb: @I", sbb));
    }
  }
  else if (I386_OP_TYPE(src) == i386_optype_mem)
  {
    t_reg dst_reg = I386_OP_BASE(dst);
    int offset = I386_OP_IMMEDIATE(src);
    t_reg base = I386_OP_BASE(src);
    t_reg index = I386_OP_INDEX(src);
    int scale = I386_OP_SCALE(src);

	ASSERT(I386_OP_TYPE(dst) == i386_optype_reg,
	("Unknown destination optype for sbb: @I", sbb));
    regmode = I386_OP_REGMODE(dst);

    if(!execute)
      return score;
    dlis_I386MakeInsForIns_4(Sub, Before, sub1, sbb,
	dst_reg, I386_REG_NONE, 1, regmode);
    dlis_I386MakeInsForIns_6(SubMem, Before, sub2, sbb,
	dst_reg, offset, base, index, scale, regmode);
  }
  else
  {
    FATAL(("Unknow source optype for sbb: @I", sbb));
  }

  /* Split the bbl, create edges */
  /* A fallthrough bbl: cut after jcc/before sub */
  fallthrough_bbl = BblSplitBlock(bbl, (t_ins*) sub1, TRUE/*before*/);
  /* A bbl where everything comes together */
  end_bbl = BblSplitBlock(fallthrough_bbl, (t_ins*) sub1, FALSE/*before*/);
  /* Create a jump edge from the original bbl (with jcc)
   * to end_bbl */
  CfgEdgeCreate(cfg, bbl, end_bbl, ET_JUMP);

  I386InsKill(sbb);
  return score;
} /*}}}*/
t_bool LimitSbb(t_cfg *cfg, t_i386_ins *sbb) 
{
  return (LimitSbbEx(cfg,sbb,TRUE)==0)?FALSE:TRUE;
}

/*
 * Main stuff
 */
dlis_histogram CreateHistogram(t_cfg *cfg) /*{{{*/
{
  t_ins *ins;
  dlis_histogram histogram = DlisHistogram(200/*max amount of opcodes*/);
  dlis_histogram_item histogram_item;

  CFG_FOREACH_INS(cfg, ins)
  {
    histogram_item = DlisHistogramGetForOpcode(histogram, I386_INS_OPCODE(T_I386_INS(ins)));
    DlisHistogramItemAddIns(histogram_item, T_I386_INS(ins));
  }

  DlisHistogramSort(histogram);
  return histogram;
} /*}}}*/

void CreateDepGraph() /*{{{*/
{
  dlis_opcode_node *opcode_node_list = OpcodeNodeList();
  ASSERT(!dep_graph, ("dep_graph already created!"));
  dep_graph = DlisGraph(opcode_node_list, 100);

  DlisGraphAddNode(dep_graph, I386_CALL);
  DlisGraphAddNode(dep_graph, I386_PUSH);
  DlisGraphAddNode(dep_graph, I386_JMP);
  DlisGraphAddNode(dep_graph, I386_RET);
  DlisGraphAddNode(dep_graph, I386_POP);
  DlisGraphAddNode(dep_graph, I386_ADD);
  DlisGraphAddNode(dep_graph, I386_MOV);
  DlisGraphAddNode(dep_graph, I386_SUB);
  DlisGraphAddNode(dep_graph, I386_NEG);
  DlisGraphAddNode(dep_graph, I386_INC);
  DlisGraphAddNode(dep_graph, I386_SETcc);
  DlisGraphAddNode(dep_graph, I386_Jcc);
  DlisGraphAddNode(dep_graph, I386_CMOVcc);
  DlisGraphAddNode(dep_graph, I386_STD);
  DlisGraphAddNode(dep_graph, I386_PUSHF);
  DlisGraphAddNode(dep_graph, I386_POPF);
  DlisGraphAddNode(dep_graph, I386_OR);
  DlisGraphAddNode(dep_graph, I386_LEAVE);
  DlisGraphAddNode(dep_graph, I386_BSWAP);
  DlisGraphAddNode(dep_graph, I386_SHL);
  DlisGraphAddNode(dep_graph, I386_SHR);
  DlisGraphAddNode(dep_graph, I386_XCHG);
  DlisGraphAddNode(dep_graph, I386_DEC);
  DlisGraphAddNode(dep_graph, I386_TEST);
  DlisGraphAddNode(dep_graph, I386_AND);
  DlisGraphAddNode(dep_graph, I386_NOP);
  DlisGraphAddNode(dep_graph, I386_SBB);

  DlisGraphAddEdge(dep_graph, I386_CALL, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_CALL, I386_JMP);

  DlisGraphAddEdge(dep_graph, I386_RET, I386_POP);
  DlisGraphAddEdge(dep_graph, I386_RET, I386_JMP);
  DlisGraphAddEdge(dep_graph, I386_RET, I386_ADD);

  DlisGraphAddEdge(dep_graph, I386_PUSH, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_PUSH, I386_SUB);

  DlisGraphAddEdge(dep_graph, I386_POP, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_POP, I386_ADD);

  DlisGraphAddEdge(dep_graph, I386_NEG, I386_SUB);
  DlisGraphAddEdge(dep_graph, I386_NEG, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_NEG, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_NEG, I386_POP);

  DlisGraphAddEdge(dep_graph, I386_INC, I386_ADD);

  DlisGraphAddEdge(dep_graph, I386_SETcc, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_SETcc, I386_Jcc);

  DlisGraphAddEdge(dep_graph, I386_CMOVcc, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_CMOVcc, I386_Jcc);

  DlisGraphAddEdge(dep_graph, I386_STD, I386_PUSHF);
  DlisGraphAddEdge(dep_graph, I386_STD, I386_OR);
  DlisGraphAddEdge(dep_graph, I386_STD, I386_POPF);

  DlisGraphAddEdge(dep_graph, I386_LEAVE, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_LEAVE, I386_POP);

  DlisGraphAddEdge(dep_graph, I386_Jcc, I386_Jcc);
  DlisGraphAddEdge(dep_graph, I386_Jcc, I386_JMP);

  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_POP);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_PUSHF);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_POPF);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_SHL);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_SHR);
  DlisGraphAddEdge(dep_graph, I386_BSWAP, I386_OR);

  DlisGraphAddEdge(dep_graph, I386_ADD, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_ADD, I386_POP);
  DlisGraphAddEdge(dep_graph, I386_ADD, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_ADD, I386_SUB);

  DlisGraphAddEdge(dep_graph, I386_XCHG, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_XCHG, I386_POP);
  DlisGraphAddEdge(dep_graph, I386_XCHG, I386_MOV);
  /* NOTE: what to do with LimitXchgXor? */

  DlisGraphAddEdge(dep_graph, I386_DEC, I386_SUB);

  DlisGraphAddEdge(dep_graph, I386_TEST, I386_PUSH);
  DlisGraphAddEdge(dep_graph, I386_TEST, I386_POP);
  DlisGraphAddEdge(dep_graph, I386_TEST, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_TEST, I386_AND);

  DlisGraphAddEdge(dep_graph, I386_SBB, I386_MOV);
  DlisGraphAddEdge(dep_graph, I386_SBB, I386_Jcc);
  DlisGraphAddEdge(dep_graph, I386_SBB, I386_SUB);

  /* This is a hack: nop depends on nothing. But MOV's
   * are in (almost?) all programs, so this should work */
  DlisGraphAddEdge(dep_graph, I386_NOP, I386_MOV);
} /*}}}*/

t_int32 DlisLimitInsEx(t_cfg* cfg, t_i386_ins *ins, t_bool execute) /*{{{*/
{
  dlis_graph_node node;
  if (DlisGraphIsIn(dep_graph, I386_INS_OPCODE(ins)))
  {
    node = DlisGraphGet(dep_graph, I386_INS_OPCODE(ins));
  }
  else
  {
    return 0;
  }

  /* Only limit opcodes that are colored LIMIT */
  /*
     if(execute)
     if (!(DlisGraphNodeGetColor(node) == COLOR_LIMIT))
     {
     return 0;
     }*/
  switch (I386_INS_OPCODE(ins))
  {
    case I386_CALL:
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
      {
	return LimitImmCallEx(cfg, ins, execute);
      }
      break;
    case I386_RET:
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_none)
      {
	return LimitRetEx(cfg, ins,execute);
      }
      else if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
      {
	return LimitImmRetEx(cfg, ins,execute);
      }
      else
      {
	FATAL(("Not None and not Immediate return"));
      }
      break;
    case I386_PUSH:
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
      {
	return LimitImmPushEx(cfg, ins,execute);
      }
      else if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg)
      {
	return LimitRegPushEx(cfg, ins, execute);
      }
      else if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
      {
	FATAL(("Mem push!"));
      }
      break;
    case I386_POP:
      return LimitRegPopEx(cfg, ins,execute);
    case I386_INC:
      if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
      {
	return LimitRegIncEx(cfg, ins,execute);
      }
      else if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
      {
	return LimitIncEx(cfg, ins,execute);
      }
      break;
    case I386_NEG:
      return LimitNegEx(cfg, ins,execute);
    case I386_SETcc:
      return LimitSetccEx(cfg, ins,execute);
    case I386_CMOVcc:
      return LimitCmovccEx(cfg, ins,execute);
    case I386_STD:
      return LimitStdEx(cfg, ins,execute);
    case I386_LEAVE:
      return LimitLeaveEx(cfg, ins,execute);
    case I386_Jcc:
      return LimitJccEx(cfg, ins,execute);
    case I386_BSWAP:
      return LimitBswapEx(cfg, ins,execute);
    case I386_ROL:
      return LimitRolrEx(cfg, ins,execute);
    case I386_ROR:
      return LimitRolrEx(cfg, ins,execute);
    case I386_ADD:
      return LimitAddEx(cfg, ins,execute);
    case I386_XCHG:
      return LimitXchgXorEx(cfg, ins,execute);
    case I386_DEC:
      return LimitDecEx(cfg, ins,execute);
    case I386_TEST:
      return LimitTestEx(cfg, ins,execute);
    case I386_NOP:
      return LimitNopEx(cfg, ins,execute);
    case I386_SBB:
      return LimitSbbEx(cfg, ins,execute);
    default:
      return 0;
  }
  return 0;
} /*}}}*/

t_int32 GetNrOfReplacingInstructions(t_cfg* cfg, t_i386_ins *ins)
{
  int ret;
  computeCost = TRUE;
  ret = DlisLimitInsEx(cfg,ins,FALSE);
  computeCost = FALSE; 
  return ret;
}

t_int32 SubstituteInstruction(t_cfg * cfg,  t_i386_ins * ins)
{
  computeCost = TRUE;
  return DlisLimitInsEx(cfg,ins,TRUE);
}

t_int32 GetGain(t_cfg * cfg, t_i386_ins * ins)
{
  computeCost = FALSE;
  return DlisLimitInsEx(cfg,ins,FALSE);
}
t_bool DlisLimitIns(t_cfg* cfg, t_i386_ins *ins) 
{
  return DlisLimitInsEx(cfg,ins,TRUE)==0?FALSE:TRUE;
}

void DlisLimit(t_cfg *cfg) /*{{{*/
{
  t_i386_ins *ins, *tmp;
  t_bbl *bbl;
  int changes = 1;
  int r;
  int o;
  while (changes)
  {
    changes = 0;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      BBL_FOREACH_I386_INS_SAFE(bbl, ins, tmp)
      {
	r = (t_uint32) DlisLimitIns(cfg, ins);
	changes += r;
	o = I386_INS_OPCODE(ins);
	if (o >= MAX_I386_OPCODE)
	{
	  VERBOSE(0, ("%d!!!", o));
	}
	if (o > 0 && o < MAX_I386_OPCODE)
	{
	  opcode_count_array[o] += r;
	}
      }
    }
  }
  /* Print opcode numbers */
  VERBOSE(0, ("Opcodes limited:"));
  for (r=0; r < MAX_I386_OPCODE; r++)
  {
    if (opcode_count_array[r] > 0)
    {
      VERBOSE(0, ("%s,%d", i386_opcode_table[r].textual, opcode_count_array[r]));
    }
  }
} /*}}}*/

void InitReglists() /*{{{*/
{
  full32reglist = DlisRegList(7);
  lo16reglist = full32reglist;
  lo8reglist = DlisRegList(4);
  hi8reglist = lo8reglist;

  DlisRegListAdd(full32reglist, I386_REG_EAX);
  DlisRegListAdd(full32reglist, I386_REG_EBX);
  DlisRegListAdd(full32reglist, I386_REG_ECX);
  DlisRegListAdd(full32reglist, I386_REG_EDX);
  DlisRegListAdd(full32reglist, I386_REG_EDI);
  DlisRegListAdd(full32reglist, I386_REG_ESI);
  DlisRegListAdd(full32reglist, I386_REG_EBP);

  DlisRegListAdd(lo8reglist, I386_REG_EAX);
  DlisRegListAdd(lo8reglist, I386_REG_EBX);
  DlisRegListAdd(lo8reglist, I386_REG_ECX);
  DlisRegListAdd(lo8reglist, I386_REG_EDX);
} /*}}}*/

void DestructReglists() /*{{{*/
{
  DlisRegListDestruct(full32reglist);
  DlisRegListDestruct(lo8reglist);
} /*}}}*/

dlis_opcode_list DlisSelectLowestOpcodes(dlis_histogram histogram, int n, dlis_opcode_list limited) /*{{{*/
{
  int i;
  t_i386_opcode opcode;
  int n_h_size = -1*DlisHistogramGetSize(histogram);
  /* Find n opcodes that are not in limited, starting with the lowest
   * instruction count. */
  /*   Create the list to return */
  dlis_opcode_list ret = DlisOpcodeList(DlisHistogramGetSize(histogram));
  /*   Iterate over the histogram from the least used opcodes
   *   to the more used opcodes. Add opcode if not in limited.
   *   Again, I'm a Python addict, so I use negative indexing. */
  for(i=-1; n && (i >= n_h_size); i--)
  {
    opcode = DlisHistogramGetOpcode(histogram, i);
    if (!DlisOpcodeListIn(limited, opcode))
    {
      n -= 1;
      DlisOpcodeListAdd(ret, opcode);
    }
  }
  if (n > 0)
  {
    /* We reaches the end of the histogram before finding enough
     * limitable opcodes. Return NULL to signal this to the caller. */
    return NULL;
  }
  return ret;
} /*}}}*/

t_bool DlisGraphNodeIsLimitable(dlis_graph_node node, t_bool is_root) /*{{{*/
{
  /* The root of a tree exists. We need to ignore this color. */
  dlis_graph_node_list_item outgoing, next;
  if (!is_root)
  {
    switch (DlisGraphNodeGetColor(node))
    {
      case COLOR_LIMITED:
      case COLOR_EXISTS:
      case COLOR_MULTI:
	return TRUE;
      default:
	break;
    }
  }
  /* If this node isn't colored or is root, and has no successors,
   * return FALSE. (this is why the NOP-hack is needed) */
  if (!node->outgoing)
  {
    return FALSE;
  }
  /* Check all outgoing edges recursively */
  for (outgoing=node->outgoing; outgoing; outgoing=next)
  {
    next = outgoing->next;
    if (!DlisGraphNodeIsLimitable(outgoing->node, FALSE))
    {
      return FALSE;
    }
  }
  /* Not colored, has outgoing, all outgoing ok -> return TRUE */
  return TRUE;
} /*}}}*/

dlis_graph_list DlisFilterTreeList(dlis_graph_list list) /*{{{*/
{
  int i, l;
  dlis_graph tree;
  dlis_graph_list ret = DlisGraphList(100);
  l = DlisGraphListGetSize(list);
  for (i=0; i<l; i++)
  {
    tree = DlisGraphListPop(list, 0);
    if (DlisGraphNodeIsLimitable(DlisGraphGetRoot(tree), TRUE))
    {
      DlisGraphListAdd(ret, tree);
    }
    else
    {
      DlisGraphDestruct(tree);
    }
  }
  DlisGraphListDestruct(list);
  return ret;
} /*}}}*/

void DlisGraphNodeColorLimitRecursive(dlis_graph_node self, t_bool is_root) /*{{{*/
{
  dlis_graph_node_list_item outgoing, next;
  if (!is_root)
  {
    switch (DlisGraphNodeGetColor(self))
    {
      case COLOR_LIMITED:
	/* FALLTHROUGH */
      case COLOR_EXISTS:
	/* FALLTHROUGH */
      case COLOR_MULTI:
	/* Already done */
	return;
      default:
	break;
    }
  }
  for (outgoing = self->outgoing; outgoing; outgoing=next)
  {
    next = outgoing->next;
    DlisGraphNodeColorLimitRecursive(outgoing->node, FALSE);
  }
  DlisGraphNodeSetColor(self, COLOR_LIMIT);
} /*}}}*/

void DlisColorTreeList(dlis_graph_list tree_list) /*{{{*/
{
  int i;
  dlis_graph tree;
  for (i=0; i<DlisGraphListGetSize(tree_list); i++)
  {
    tree = DlisGraphListGet(tree_list, i);
    DlisGraphNodeColorLimitRecursive(DlisGraphGetRoot(tree), TRUE);
  }
} /*}}}*/

t_bool DlisLimitOpcodes(dlis_histogram histogram, int n, dlis_opcode_list limited, t_cfg *cfg) /*{{{*/
{
  int i;
  t_i386_opcode opcode;
  dlis_graph tree;
  dlis_graph_node node;
  dlis_graph_list tree_list = DlisGraphList(100);
  dlis_opcode_list opcode_list = DlisSelectLowestOpcodes(histogram, n, limited);
  if (!opcode_list)
  {
    return FALSE;
  }
  /* Check if this list can be limited */
  /* Clear the colors and use counts*/
  DlisGraphClearColors(dep_graph);
  DlisGraphClearUseCounts(dep_graph);
  /* Color the existing opcodes EXISTS */
  for (i=0; i<DlisHistogramGetSize(histogram); i++)
  {
    opcode = DlisHistogramItemGetOpcode(DlisHistogramGet(histogram, i));
    if (DlisGraphIsIn(dep_graph, opcode))
    {
      DlisGraphNodeSetColor(DlisGraphGet(dep_graph, opcode), COLOR_EXISTS);
    }
  }
  /* Color the limited opcodes LIMITED */
  for (i=0; i<DlisOpcodeListGetSize(limited); i++)
  {
    opcode = DlisOpcodeListGet(limited, i);
    if (DlisGraphIsIn(dep_graph, opcode))
    {
      DlisGraphNodeSetColor(DlisGraphGet(dep_graph, opcode), COLOR_LIMITED);
    }
  }
  /* For each opcode, create the tree, append to the tree list */
  for (i=0; i<DlisOpcodeListGetSize(opcode_list); i++)
  {
    opcode = DlisOpcodeListGet(opcode_list, i);
    if (DlisGraphIsIn(dep_graph, opcode))
    {
      tree = DlisGraphCreateTree(dep_graph, DlisGraphGet(dep_graph, opcode));
      DlisGraphListAdd(tree_list, tree);
    }
  }
  /* Now all trees are created, so the use_counts can be used to color
   * with color MULTI */
  for (i=0; i<DlisHistogramGetSize(histogram); i++)
  {
    opcode = DlisHistogramItemGetOpcode(DlisHistogramGet(histogram, i));
    if (DlisGraphIsIn(dep_graph, opcode))
    {
      node = DlisGraphGet(dep_graph, opcode);
      if (DlisGraphNodeGetColor(node) == COLOR_NONE &&
	  DlisGraphNodeGetUseCount(node) >= 2)
      {
	DlisGraphNodeSetColor(node, COLOR_MULTI);
      }
    }
  }
  /* We're ready to filter the possible limitations out now. */
  tree_list = DlisFilterTreeList(tree_list);
  /* Check if there are trees left... */
  if (DlisGraphListGetSize(tree_list) == 0)
  {
    return FALSE;
  }
  /* TODO: recolor blue, refilter, ... */
  /* There are trees left. Color with the FILTER color. */
  DlisColorTreeList(tree_list);
  /* We're ready to filter now. Transformations that should be executed
   * have been marked with the LIMIT color. */
  DlisLimit(cfg);
  /* TODO: add limited opcodes to limited */
  for (i=0; i<DlisGraphListGetSize(tree_list); i++)
  {
    node = DlisGraphGetRoot(DlisGraphListGet(tree_list, i));
    opcode = DlisGraphNodeGetOpcode(node);
    DlisOpcodeListAdd(limited, opcode);
  }
  return TRUE;
} /*}}}*/

void DoLimitation(dlis_histogram histogram, int todo_n_opcodes, dlis_opcode_list limited, t_cfg* cfg) /*{{{*/
{
  /* n = amount of opcodes that still need to be limited */
  int size = DlisHistogramGetSize(histogram);
  int g = todo_n_opcodes;
  dlis_histogram fatal_histogram;
  t_bool done = FALSE;
  if (todo_n_opcodes == 0)
  {
    return; /* Done! */
  }
  /* Not done: limit opcodes!
   * First try if a group of g opcodes can be limited */
  while (!done && (g <= size))
  {
    /* DlisSelectLimitOpcodes returns FALSE it cannot find n
     * limitable opcodes. It returns TRUE if a limitation was
     * done. */
    done = DlisLimitOpcodes(histogram, g, limited, cfg);
    g += 1;
  }
  if (!done)
  {
    int i;
    VERBOSE(0, ("Done (%d):", DlisOpcodeListGetSize(limited)));
    for (i=0; i<DlisOpcodeListGetSize(limited); i++)
    {
      VERBOSE(0, ("- %s", i386_opcode_table[limited->list[i]].textual));
    }
    fatal_histogram = CreateHistogram(cfg);
    VERBOSE(0, ("Todo:"));
    DlisHistogramPrint(fatal_histogram);
    FATAL(("Could not limit opcode count enough."));
  }
  else
  {
    /* Done! */
    return;
  }
}/*}}}*/

int compareInsByScore(const void * one, const void * two)
{
  return INS_SCORE((*(t_ins **)one))>INS_SCORE((*(t_ins **)two))?-1:INS_SCORE((*(t_ins**)one))<INS_SCORE((*(t_ins**)two))?1:0;
}

int compareInsByCostInverse(const void * one, const void * two)
{
  return INS_COST((*(t_ins **)one))>INS_COST((*(t_ins **)two))?-1:INS_COST((*(t_ins**)one))<INS_COST((*(t_ins**)two))?1:0;
}

t_int64 ComputeBudget(t_cfg * cfg, t_int32 price)
{
  t_ins * ins;
  t_int64 total = 0;
  CFG_FOREACH_INS(cfg,ins)
  {
    total+=BBL_EXEC_COUNT(INS_BBL(ins));
  }
  VERBOSE(0,("total: %lld",total));
  return total*1.0/100*price;
}

/*
 * Main
 */
void
DiversityLimitInstructionSet(t_cfg * cfg, t_diversity_choice * choice, t_uint32 target_n_opcodes)
{
  t_int32 price = 20;
  //t_int32 nrOfTransformationsPerIteration = 500;

  int i;
  t_bbl *bbl;
  t_ins * ins;

  InsInitCost(cfg);

  if (target_n_opcodes > 0)
  {
    CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
    InitReglists();
    CreateDepGraph();

    {
      //budget is a percentage of the dynamic execution count
      t_int64 budget = ComputeBudget(cfg,price);
      
      t_i386_ins *** arrays = (t_i386_ins ***) Calloc(MAX_I386_OPCODE,sizeof(t_i386_ins **));
      t_int32 * counts = (t_int32 *) Calloc(MAX_I386_OPCODE,sizeof(t_int32));
      t_int32 * minima =  (t_int32 *) Calloc(MAX_I386_OPCODE,sizeof(t_int32));

	  InitiateScoreComputation(cfg);
      
      {
	//compute initial sizes of arrays
	CFG_FOREACH_INS(cfg,ins)
	{
	  t_int32 nrOfIns = GetNrOfReplacingInstructions(cfg,T_I386_INS(ins));
	  //cost of INT_MAX means that it cannot be replaced
	  if(nrOfIns == 0)
	    INS_SET_COST(ins,INT_MIN);
	  else
	    //-1 because replacing by a single instruction is free according to our metric
	    INS_SET_COST(ins,(nrOfIns-1)*BBL_EXEC_COUNT(INS_BBL(ins)));
	  if(BBL_EXEC_COUNT(INS_BBL(ins))>0 && INS_COST(ins) >= 0)
	    counts[I386_INS_OPCODE(T_I386_INS(ins))]++;
	}

	//allocate space for the initial arrays, reset counts for later use
	for(i = 0 ; i < MAX_I386_OPCODE;i++)
	{
	  if(counts[i]>0)
	    arrays[i]=(t_i386_ins**) Calloc(counts[i],sizeof(t_i386_ins*));
	  counts[i]=0;
	}

	//populate the initial arrays and counts
	CFG_FOREACH_INS(cfg,ins)
	{
	  if(BBL_EXEC_COUNT(INS_BBL(ins))>0 && INS_COST(ins) >= 0)
	  {
	    arrays[I386_INS_OPCODE(T_I386_INS(ins))][counts[I386_INS_OPCODE(T_I386_INS(ins))]]=T_I386_INS(ins);
	    counts[I386_INS_OPCODE(T_I386_INS(ins))]++;
	  }
	}

	//sort the initial arrays
	for(i = 0; i < MAX_I386_OPCODE; i++)
	{
	  if(arrays[i]!=NULL)
	    diablo_stable_sort(arrays[i],counts[i],sizeof(t_ins *),compareInsByCostInverse);
	}


	//this will be done as long as we can optimize within the budget.
	while(TRUE)
	{
	  //We're going to search for the best index
	  float best = -1;
	  t_int32 bestIndex = -1;
	  //VERBOSE(0,("Outer loop"));
	  for(i = 0 ; i < MAX_I386_OPCODE;i++)
	  {
	    if(arrays[i]!=NULL)
	    {
	      //VERBOSE(0,("Inner loop, not NULL %d %d",i,counts[i]-1));
	      //immediate gain
	      if(GetGain(cfg,arrays[i][counts[i]-1])>0)
	      {
		minima[i] = 1;
		//for free, just do it :-)
		if(INS_COST(T_INS(arrays[i][counts[i]-1]))==0)
		{
		  bestIndex = i;
		  break;
		}
		else
		{
		  float score = GetGain(cfg,arrays[i][counts[i]-1])*1.0/INS_COST(T_INS(arrays[i][counts[i]-1]));
		  if(score > best && INS_COST(T_INS(arrays[i][counts[i]-1])) < budget)
		  {
		    best = score;
		    bestIndex = i;
		  }
		}
	      }
	      //no immediate gain
	      else
	      {
		int j = 0;
		float current_gain = GetGain(cfg,arrays[i][counts[i]-j-1]);
		t_int32 current_cost = INS_COST(T_INS(arrays[i][counts[i]-j-1]));
		j++;
		while (current_gain < 0 && j < counts[i])
		{
		  current_gain += GetGain(cfg,arrays[i][counts[i]-j-1]);
		  current_cost += INS_COST(T_INS(arrays[i][counts[i]-j-1]));
		  j++;
		}
		if(current_gain > 0 && current_cost < budget)
		{
		  float score = current_gain*1.0/current_cost;
		  if(score > best)
		  {
		    best = score;
		    minima[i]=j;
		    bestIndex = i;
		  }
		}
	      }
	    }
	  }
	  //Search for best index done
	  //VERBOSE(0,("found best"));

	  //We didn't find one
	  if(bestIndex == -1)
	    break;

	  //We did find one
	  {
	    for (i=0; i< minima[bestIndex];i++)
	    {
	      t_bool hackToDetectIfJcc = FALSE;
	      t_bool resultsInMultipleBbls = FALSE;
		  t_int32 checker = 0;
	      t_int32 step = 0;

	      t_bbl * bbl = INS_BBL(T_INS(arrays[bestIndex][counts[bestIndex]-1]));

		  t_int32 nrOfNew;

	      //Substitute the instruction
              frequencies[I386_INS_OPCODE(arrays[bestIndex][counts[bestIndex]-1])]--;
	      if(I386_INS_OPCODE(arrays[bestIndex][counts[bestIndex]-1])==I386_Jcc)
		hackToDetectIfJcc=TRUE;
	      if(I386_INS_OPCODE(arrays[bestIndex][counts[bestIndex]-1])==I386_CMOVcc || I386_INS_OPCODE(arrays[bestIndex][counts[bestIndex]-1])==I386_SBB || I386_INS_OPCODE(arrays[bestIndex][counts[bestIndex]-1])==I386_SETcc)
		resultsInMultipleBbls=TRUE;
	      nrOfNew = SubstituteInstruction(cfg,arrays[bestIndex][counts[bestIndex]-1]);
	      
	      budget-=(nrOfNew-1)*BBL_EXEC_COUNT(bbl);
	      //remove instruction from it's array
	      counts[bestIndex]-=1;
	      //array becomes empty
	      if(counts[bestIndex]==0)
	      {
		Free(arrays[bestIndex]);
		arrays[bestIndex]=NULL;
	      }
	      //otherwhise: shrink
	      else
		arrays[bestIndex]=(t_i386_ins**) Realloc(arrays[bestIndex],counts[bestIndex]*sizeof(t_ins*));


	      //add new instructions to their arrays and resort them
	      checker = 0;
	      step = 0;
	      while(bbl)
	      {
		BBL_FOREACH_INS(bbl,ins)
		{
		  //for new instructions, cost is set to -1
		  if(INS_COST(ins)==-1)
		  {
			t_int32 nrOfIns;
		    checker++;

		    nrOfIns = GetNrOfReplacingInstructions(cfg,T_I386_INS(ins));
		    //cost of INT_MAX means that it cannot be replaced
		    if(nrOfIns == 0)
		      INS_SET_COST(ins,INT_MIN);
		    else
		      //-1 because replacing by a single instruction is free according to our metric
		      INS_SET_COST(ins,(nrOfIns-1)*BBL_EXEC_COUNT(INS_BBL(ins)));

		    frequencies[I386_INS_OPCODE(T_I386_INS(ins))]++;

		    if(BBL_EXEC_COUNT(INS_BBL(ins))==0)
		      FATAL(("not pure"));
		    if(BBL_EXEC_COUNT(INS_BBL(ins))>0 && INS_COST(ins) >= 0)
		    {
		      counts[I386_INS_OPCODE(T_I386_INS(ins))]++;
		      if( counts[I386_INS_OPCODE(T_I386_INS(ins))] == 1 )
			arrays[I386_INS_OPCODE(T_I386_INS(ins))] = (t_i386_ins**) Calloc(counts[I386_INS_OPCODE(T_I386_INS(ins))],sizeof(t_i386_ins*));
		      else
			arrays[I386_INS_OPCODE(T_I386_INS(ins))] = (t_i386_ins**) Realloc( arrays[I386_INS_OPCODE(T_I386_INS(ins))], counts[I386_INS_OPCODE(T_I386_INS((ins)))]*sizeof(t_i386_ins*));
		      arrays[I386_INS_OPCODE(T_I386_INS(ins))][counts[I386_INS_OPCODE(T_I386_INS(ins))]-1]=T_I386_INS(ins);
		      diablo_stable_sort(arrays[I386_INS_OPCODE(T_I386_INS(ins))],counts[I386_INS_OPCODE(T_I386_INS(ins))],sizeof(t_ins *),compareInsByCostInverse);
		    }
		  }
		}
		if(!resultsInMultipleBbls)
		  break;

		if (step == 0)
		{
		  bbl = T_BBL(EDGE_TAIL(T_EDGE(BBL_SUCC_FIRST(bbl))));
		  step = 1;
		}
		else if (step == 1)
		{
		  bbl = T_BBL(EDGE_TAIL(EDGE_SUCC_NEXT(T_EDGE(BBL_SUCC_FIRST(T_BBL(EDGE_HEAD(T_EDGE(BBL_PRED_FIRST(bbl)))))))));
		  step = 2;
		}
		else if (step == 2)
		  break;
	      }
	      if(checker != nrOfNew &&! hackToDetectIfJcc)
	      {
		VERBOSE(0,("AFTER @iB",bbl));
		FATAL(("checker doesn't match nrOfNew %d %d",checker,nrOfNew));
	      }
	    }
	  }
	}
      }

      /*
      //as long as the budget isn't depleted, do a number of iterations in which the n most beneficial substitutions are performed
      while(budget > 0)
      {
        t_int32 nrOfCandidates = 0;
        CFG_FOREACH_INS(cfg,ins)
        {
          t_int32 score = GetGain(cfg,ins);
	  float mediated = score*1.0/BBL_EXEC_COUNT(INS_BBL(ins));
          INS_SET_SCORE(ins,mediated);
          if(mediated > 0)
            nrOfCandidates++;
        }
        if(nrOfCandidates == 0)
          break;

        t_ins ** arrayOfInstructions = Calloc(nrOfCandidates,sizeof(t_ins *));

        t_int32 i = 0;
        CFG_FOREACH_INS(cfg,ins)
        {
          if(INS_SCORE(ins) > 0)
	  {
            arrayOfInstructions[i++]=ins;
	  }
        }

        diablo_stable_sort(arrayOfInstructions,nrOfCandidates,sizeof(t_ins *),compareInsByScore);

        i=0;
        while(i<nrOfTransformationsPerIteration && i< nrOfCandidates && budget > 0)
        {
	  ins = arrayOfInstructions[i];
	  t_int32 exec_count = BBL_EXEC_COUNT(INS_BBL(ins));
          t_int32 cost = SubstituteInstruction(cfg,ins);
          budget-=cost*exec_count;
          i++;
        }
        Free(arrayOfInstructions);
      }
      */
    }
  }
}

/* vim: set shiftwidth=2 foldmethod=marker: */
