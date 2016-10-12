/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/** \file
 *
 * Relocs are the tie that binds different objects and object sections. This
 * file holds the typedefs, types, function, and defines needed to handle
 * them.
 */

/* Reloc Typedefs {{{ */
#ifndef DIABLOOBJECT_RELOC_TYPEDEFS
#define DIABLOOBJECT_RELOC_TYPEDEFS
struct _t_reloc_table;
struct _t_reloc;
typedef void (*t_reloc_add_edge_cb) (struct _t_reloc_table *, struct _t_reloc *, void *);
typedef void (*t_reloc_del_edge_cb) (struct _t_reloc_table *, struct _t_reloc *, void *);
typedef void (*t_reloc_del_switch_edge_cb) (struct _t_reloc_table *, struct _t_reloc *, void *);
typedef struct _t_compressed_reloc t_compressed_reloc;
#endif /* }}} Reloc Typedefs */
/* Reloc Defines {{{ */
#ifndef DIABLOOBJECT_RELOC_DEFINES
#define DIABLOOBJECT_RELOC_DEFINES
/* predefined stack machine programs */
#define CALC_REL "S00P-A00+"
#define CALC_ABS "S00A00+"

#define WRITE_32 "l*w\\s0000$"
#define WRITE_64 "L*W\\s0000$"

/* introduced for amd64 */
#define WRITE_64_32 "iFFFFFFFF&l*w\\s0000$"
#define WRITE_64_16 "sFFFF&k*v\\s0000$"
#define WRITE_64_8  "s00FF&ksFF00&|v\\s0000$"

/* this is only interesting for IA-64*/
#define CALC_REL_ALIGN "S00P Ifffffffffffffff8 & -A00+"

#define WRITE_IMM21 \
  "s0004 > I00000000001fffff &= " \
"I00000000000fffff & s000d < " \
"{ I0000000000100000 & s0010 <" \
"f I000001ee00001fff & || F s0000$"
/*TODO: nog checken !!!*/
#define WRITE_IMM22 \
  "I00000000003fffff &=== " \
"I0000000000200000 & s000f <" \
"{ I000000000000ff80 & s0014 <" \
"{ I00000000001f0000 & s0006 <" \
"{ I000000000000007f & s000d <" \
"f I000001e000301fff & |||| F s0000$"
#define WRITE_IMM64 \
  "===== " \
"I8000000000000000 & s001b >" \
"{ I000000000000ff80 & s0014 <" \
"{ I00000000001f0000 & s0006 <" \
"{ I0000000000200000 & " \
"{ I000000000000007f & s000d <" \
"{ I7fffffffffc00000 & s0016 >" \
"{" \
"f I000001e000101fff & ||||| F" \
"F s0000$"


/* this one is only interesting for ARM:
 * TODO: put it somewhere in arch/arm */
#define WRITE_PC24 "= s0002 & % s0002 > i00ffffff &=l iff000000 &| S00M?i10000000|ifeffffff& } s0017 < |: }* ! w\\l i00ffffff &-$"
/* split the calculated immediate into the instruction fields
 * load the original instruction + immediate
 * mask out the immediate
 * "or" both together
 * write everything
 */
#define WRITE_ARM_MOVWT "=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$"
#define WRITE_THM_MOVWT "===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$"
#define TRANSFORM_THM_PC24 \
  "s0001 > ==== " \
  /* STACK: imm imm imm imm imm */ \
 \
  /* extract imm10, shift it right 11 times */ \
  "i001ff800 & s000b > " \
  /* take a duplicated immediate value, \
     extract imm11 and OR the result \
   */ \
  "% i000007ff & s0010 < | " \
  /* STACK: imm imm imm imm10|imm11 */ \
 \
  /* take a duplicated immediate value, \
     extract both S- and I2-bits \
     and duplicate these bits \
   */ \
  "% i00a00000&=" \
  /* extract the I2-bit, and put it in the position of the J2-bit \
     (in the instruction encoding) \
   */ \
  "i00200000& s0006< " \
  /* take the duplicated value of the S- and I2-bits \
     extract the S-bit, and put it in the position of the J2-bit \
   */ \
  "% i00800000& s0004<" \
  /* NOT-XOR the S- and I2-bits and OR the result \
   */ \
  "^i08000000^ | " \
  /* STACK: imm imm imm10|imm11|J2 */ \
 \
  /* take a duplicated immediate value, \
     extract both S- and I1-bits \
     and duplicate these bits \
   */ \
  "% i00c00000&=" \
  /* extract the I1-bit, and put it in the position of the J1-bit \
     (in the instruction encoding) \
   */ \
  "i00400000& s0007< " \
  /* take the duplicated value of the S- and I1-bits \
     extract the S-bit, and put it in the position of the J1-bit \
   */ \
  "% i00800000& s0006<" \
  /* NOT-XOR the S- and I1-bits and OR the result \
   */ \
  "^i20000000^ | " \
  /* STACK: imm imm10|imm11|J2|J1 */ \
 \
  /* take a duplicated immediate value, \
     extract the S-bit \
     and put it in the right position \
   */ \
  "% i00800000& s000d> | " \
  /* STACK: imm10|imm11|J2|J1|S */

#define WRITE_THM_PC24 \
  TRANSFORM_THM_PC24 \
  /* extract opcode bits \
     and OR them with the calculated encoded immediate \
   */ \
  "l id000f800 & | " \
 \
  /* write the result */ \
  "w\\s0000$"

/* WRITE_THM_PC22 for Thumbv2 cpus -> also 24 bit offsets */
#define WRITE_THM_PC22_EXT \
  TRANSFORM_THM_PC24 \
  /* extract opcode bits*/ \
  "l id000f800 &" \
  /* if the destination is not thumb, force a blx, otherwise \
   * bl (we pushed a thumb==1 on the stack at the start) \
   */ \
  "} ?i10000000|:iefffffff&!" \
  /* or the opcode and the encoded offset */ \
  " | " \
 \
  /* write the result */ \
  "w\\s0000$"


/* SH3 */
#define WRITE_S16 "= sffff& k*v\\k#0f -$"
#define WRITE_S8LO "= s00ff& ksff00& |v\\ks00ff& #07 -$"
#define WRITE_S8HI "= s00ff& s0008< ks00ff& |v\\ksff00& s0008> #07 -$"
#define WRITE_U8LO "= s00ff& ksff00& |v\\ks00ff& -$"
#define WRITE_U8HI "= s00ff& s0008< ks00ff& |v\\ksff00& s0008> -$"
#define T_RELOC(x) ((t_reloc*)(x))

#define AddressNullForRelocTable(rt) AddressNullForObject(rt->obj)
#define AddressNewForRelocTable(rt,a) AddressNewForObject(rt->obj,a)

#endif /* }}} Reloc Defines */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_TYPES
/* Reloc Types {{{ */
#ifndef DIABLOOBJECT_RELOC_TYPES
#define DIABLOOBJECT_RELOC_TYPES
struct _t_compressed_reloc
{
  t_uint32 code;
  t_uint32 from;
  t_uint32 from_offset;
  t_uint32 to;
  t_uint32 to_offset;
  t_uint32 addend;
}; 
#endif /* }}} Reloc Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Reloc Functions {{{ */
#ifndef DIABLOOBJECT_RELOC_FUNCTIONS
#define DIABLOOBJECT_RELOC_FUNCTIONS

t_address RelocGetDataFrom (const t_reloc *, const t_section *, t_address);
#define RelocGetData(r) RelocGetDataFrom ((r), T_SECTION (RELOC_FROM ((r))), RELOC_FROM_OFFSET ((r)))

t_reloc *RelocTableAddRelocToSymbol (t_reloc_table * table, t_address addend, t_relocatable * from, t_address from_offset, t_symbol * to, t_bool hell, void *edge, t_reloc * corresp, t_symbol * e_sym, t_const_string code);
t_reloc *RelocTableAddRelocToRelocatable (t_reloc_table * table, t_address addend, t_relocatable * from, t_address from_offset, t_relocatable * to, t_address to_offset, t_bool hell, void * edge, t_reloc * corresp, t_relocatable * sec, t_const_string code);

void RelocTableRemoveConstantRelocs(t_reloc_table *);
t_reloc_table *RelocTableNew (t_object *);
t_reloc *RelocTableDupReloc (t_reloc_table *, const t_reloc *);
void RelocTableRemoveReloc (t_reloc_table *, const t_reloc *);

t_reloc *RelocGet (const t_reloc_table *, const t_relocatable *, t_address);
void RelocTableFree (const t_reloc_table *);

void RelocSetToRelocatable (t_reloc *, t_uint32, t_relocatable *);
void RelocSetFrom (t_reloc *, t_relocatable *);
void RelocSetToSymbolBase (t_reloc *, t_uint32, t_relocatable *);
void RelocSetToSymbol (t_reloc *, t_uint32, t_symbol *);
void RelocAddAddend (t_reloc *rel, t_address addend);
void RelocAddRelocatable(t_reloc * rel, t_relocatable * relocatable, t_address offset);
void RelocAddSymbol(t_reloc * rel, t_symbol * symbol, t_address offset);
void RelocTableUnlinkReloc (t_reloc_table *, t_reloc *);
void RelocTableLinkReloc (t_reloc_table *, t_reloc *);
void RelocRemoveToRefs (t_reloc *);
void RelocRemoveToSymbolRef(t_reloc * reloc, t_uint32 i);
void RelocTablePrint (const t_reloc_table *);
t_uint32 RelocGetToRelocatableIndex (const t_reloc *rel, const t_relocatable *relocatable);
void RelocMoveToRelocTable(t_reloc* rel, t_reloc_table* table);

int RelocCmp(const t_reloc * rel1, const t_reloc * rel2, t_bool position_dependent);
void RelocTableSetCallbacks (t_reloc_table * table, void (*AddEdgeCallback) (t_reloc_table *, t_reloc *, void *), void (*DelEdgeCallback) (t_reloc_table *, t_reloc *, void *), void (*DelSwitchEdgeCallback) (t_reloc_table *, t_reloc *, void *));

void RelocatableRemoveAllRefersTo(t_relocatable* relocatable);
t_reloc *RelocatableGetRelocForAddress(const t_relocatable *relable, t_address addr);

void SymbolSetSymbolBaseAndUpdateRelocs(t_reloc_table * reltab, t_symbol * sym, t_relocatable * new_to);
#define RelocRefNew(rel) RealRelocRefNew(FORWARD_MALLOC_DEFINE rel)
t_reloc_ref *RealRelocRefNew (FORWARD_MALLOC_PROTOTYPE t_reloc *);
#define SymbolRefNew(sym) RealSymbolRefNew(FORWARD_MALLOC_DEFINE sym)
t_symbol_ref *RealSymbolRefNew (FORWARD_MALLOC_PROTOTYPE t_symbol *);
#endif /* }}} Reloc Functions */

#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
