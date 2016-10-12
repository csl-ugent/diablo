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

#include <diablodiversity.h>

void DiversityLimitInstructionSet(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);

/*
 * Common limit/unlimit functions
 */
t_reg FindFreeRegister(t_i386_ins *ins);

void dlis_I386InstructionMakeAdd(t_i386_ins *ins, t_uint32 val, t_reg reg, t_uint32 size, t_i386_regmode regmode);

void dlis_I386InstructionMakeSub(t_i386_ins *ins, t_reg dst, t_reg src, int imm, t_i386_regmode regmode);

t_i386_ins * dlis_I386InsNewForBbl(t_bbl *bbl);

void dlis_I386InsInsertBefore(t_i386_ins *subject, t_i386_ins *reference);

void dlis_I386InstructionMakeMovToRegFromMem(t_i386_ins *ins, t_reg dest, t_uint32 offset, t_reg src, t_reg index, int scale, t_uint32 memopsize, t_i386_regmode regmode);

void dlis_I386InstructionMakeLea(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize, t_i386_regmode regmode);

t_uint32 dlis_getSizeFromRegmode(t_i386_regmode regmode);

/*
 * Data structures
 */

#ifndef DLIS_H
#define DLIS_H

struct dlis_reg_list_ {
	t_reg *list;
	int length;
	int index;
}; 

typedef struct dlis_reg_list_ *dlis_reg_list;

dlis_reg_list DlisRegList(int length);
void DlisRegListAdd(dlis_reg_list self, t_reg reg);
void DlisRegListSafeAdd(dlis_reg_list self, t_reg reg);
void DlisRegListDestruct(dlis_reg_list self);
t_bool DlisRegListContainsRegister(dlis_reg_list self, t_reg reg);
void DlisRegListAddFromOp(dlis_reg_list self, t_i386_operand *op);
void DlisRegListAddFromIns(dlis_reg_list self, t_i386_ins *ins);
t_reg DlisRegListRandomWithout(dlis_reg_list self, dlis_reg_list wo);
void DlisRegListPrint(dlis_reg_list self);
t_reg DlisRegListRandomRegister(dlis_reg_list self);
t_reg DlisRegListFindFreeRegister(dlis_reg_list self, t_i386_ins *ins);

/* Limitation selection algorithm */
/* Histogram */
typedef struct dlis_ins_list_item_ *dlis_ins_list_item;
typedef struct dlis_histogram_item_ *dlis_histogram_item;
typedef struct dlis_histogram_ *dlis_histogram;
typedef struct dlis_opcode_list_ *dlis_opcode_list;

struct dlis_ins_list_item_ {
  t_i386_ins *ins;
  dlis_ins_list_item previous;
  dlis_ins_list_item next;
};

struct dlis_histogram_item_ {
  t_i386_opcode opcode;
  int count;
  dlis_ins_list_item list;
};

struct dlis_histogram_ {
  int length;
  int index;
  dlis_histogram_item *opcode_list;
  dlis_histogram_item *list;
};

struct dlis_opcode_list_
{
  int length;
  int index;
  t_i386_opcode *list;
};

dlis_histogram DlisHistogram(int length);
void DlisHistogramDestruct(dlis_histogram self);
void DlisHistogramSort(dlis_histogram self);
void DlisHistogramPrint(dlis_histogram self);
dlis_histogram_item DlisHistogramGet(dlis_histogram self, int i);
dlis_histogram_item DlisHistogramGetForOpcode(dlis_histogram self, t_i386_opcode opcode);
void DlisHistogramAdd(dlis_histogram self, t_i386_opcode opcode);

dlis_histogram_item DlisHistogramItem(t_i386_opcode opcode, int count, dlis_ins_list_item list);
void DlisHistogramItemDestruct(dlis_histogram_item self);
void DlisHistogramItemAddIns(dlis_histogram_item self, t_i386_ins *ins);
void DlisHistogramItemPrint(dlis_histogram_item self);

dlis_ins_list_item DlisInsListItem(t_i386_ins *ins, dlis_ins_list_item previous, dlis_ins_list_item next);
void DlisInsListItemDestruct(dlis_ins_list_item self);
void DlisInsListItemDestructRecursive(dlis_ins_list_item self);
dlis_ins_list_item DlisInsListItemNext(dlis_ins_list_item self);

dlis_opcode_list DlisOpcodeList(int length);
void DlisOpcodeListAdd(dlis_opcode_list self, t_i386_opcode opcode);
int DlisOpcodeListGetSize(dlis_opcode_list self);

/* Dependency graph */
typedef enum
{
  COLOR_NONE,
  COLOR_LIMITED,
  COLOR_EXISTS,
  COLOR_MULTI,
  COLOR_LIMIT
} dlis_node_color;
typedef struct dlis_opcode_node_ *dlis_opcode_node;
typedef struct dlis_graph_node_ *dlis_graph_node;
typedef struct dlis_graph_ *dlis_graph;
typedef struct dlis_graph_node_list_item_ *dlis_graph_node_list_item;

struct dlis_graph_node_list_item_ {
  dlis_graph_node node;
  dlis_graph_node_list_item next;
};

struct dlis_opcode_node_ {
  t_i386_opcode opcode;
  dlis_node_color color;
  int use_count;
};

struct dlis_graph_node_ {
  dlis_opcode_node opcode_node;
  int recursion_flag;
  dlis_graph_node_list_item incoming;
  dlis_graph_node_list_item outgoing;
};

struct dlis_graph_ {
  int length;
  int index;
  dlis_graph_node *node_list;
  /* Easy access to opcode nodes, passed to constructor */
  dlis_opcode_node *opcode_node_list;
};

dlis_graph DlisGraph(dlis_opcode_node *opcode_node_list, int length);
dlis_graph_node DlisGraphAddNode(dlis_graph self, t_i386_opcode opcode);
void DlisGraphAddEdge(dlis_graph self, t_i386_opcode from, t_i386_opcode to);
dlis_graph_node DlisGraphGet(dlis_graph self, t_i386_opcode opcode);
void DlisGraphClearColors(dlis_graph self);
void DlisGraphClearRecursionFlags(dlis_graph self);
dlis_graph DlisGraphCreateTree(dlis_graph self, dlis_graph_node root); 
void DlisGraphExtendTree(dlis_graph self, dlis_graph_node new_node, dlis_graph_node old_node);
t_bool DlisGraphHasNode(dlis_graph self, t_i386_opcode opcode);
dlis_graph_node DlisGraphGetRoot(dlis_graph self);

dlis_graph_node DlisGraphNode(dlis_opcode_node opcode_node);
void DlisGraphNodeAddOutgoing(dlis_graph_node self, dlis_graph_node to);
void DlisGraphNodeClearColor(dlis_graph_node self);
void DlisGraphNodeSetColor(dlis_graph_node self, dlis_node_color color);
void DlisGraphNodeClearRecursionFlag(dlis_graph_node self);
void DlisGraphNodeSetRecursionFlag(dlis_graph_node self);
t_bool DlisGraphNodeGetRecursionFlag(dlis_graph_node self);
void DlisGraphNodePrint(dlis_graph_node self);
void DlisGraphNodeClearUseCount(dlis_graph_node self);
dlis_node_color DlisGraphNodeGetColor(dlis_graph_node self);

dlis_opcode_node DlisOpcodeNode(t_i386_opcode opcode);

dlis_graph_node_list_item DlisGraphNodeListItem(dlis_graph_node node, dlis_graph_node_list_item next);

typedef struct dlis_graph_list_ *dlis_graph_list;

struct dlis_graph_list_ {
  int length;
  int index;
  dlis_graph *list;
};

dlis_graph_list DlisGraphList(int length);
void DlisGraphListDestruct(dlis_graph_list self);
dlis_graph DlisGraphListGet(dlis_graph_list self, int i);
void DlisGraphListAdd(dlis_graph_list self, dlis_graph g);
int DlisGraphListGetSize(dlis_graph_list self);
dlis_graph DlisGraphListPop(dlis_graph_list self, int i);

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
