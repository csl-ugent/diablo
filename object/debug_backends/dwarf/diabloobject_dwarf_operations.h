/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_OPERATIONS_H
#define DIABLOOBJECT_DWARF_OPERATIONS_H

#include <string>

enum class DwarfOperationRegister {
	fbreg,
	breg0,
	breg1,
	breg2,
	breg3,
	breg4,
	breg5,
	breg6,
	breg7,
	breg8,
	breg9,
	breg10,
	breg11,
	breg12,
	breg13,
	breg14,
	breg15,
	breg16,
	breg17,
	breg18,
	breg19,
	breg20,
	breg21,
	breg22,
	breg23,
	breg24,
	breg25,
	breg26,
	breg27,
	breg28,
	breg29,
	breg30,
	breg31,
	bregx
};

enum class DwarfOperationCode {
	DW_OP_addr = 0x03,
  DW_OP_deref = 0x06,
  DW_OP_const1u = 0x08,
  DW_OP_const1s = 0x09,
  DW_OP_const2u = 0x0a,
  DW_OP_const2s = 0x0b,
  DW_OP_const4u = 0x0c,
  DW_OP_const4s = 0x0d,
  DW_OP_const8u = 0x0e,
  DW_OP_const8s = 0x0f,
  DW_OP_constu = 0x10,
  DW_OP_consts = 0x11,
  DW_OP_dup = 0x12,
  DW_OP_drop = 0x13,
  DW_OP_over = 0x14,
  DW_OP_pick = 0x15,
  DW_OP_swap = 0x16,
  DW_OP_rot = 0x17,
  DW_OP_xderef = 0x18,
  DW_OP_abs = 0x19,
  DW_OP_and = 0x1a,
  DW_OP_div = 0x1b,
  DW_OP_minus = 0x1c,
  DW_OP_mod = 0x1d,
  DW_OP_mul = 0x1e,
  DW_OP_neg = 0x1f,
  DW_OP_not = 0x20,
  DW_OP_or = 0x21,
  DW_OP_plus = 0x22,
  DW_OP_plus_uconst = 0x23,
  DW_OP_shl = 0x24,
  DW_OP_shr = 0x25,
  DW_OP_shra = 0x26,
  DW_OP_xor = 0x27,
  DW_OP_skip = 0x2f,
  DW_OP_bra = 0x28,
  DW_OP_eq = 0x29,
  DW_OP_ge = 0x2a,
  DW_OP_gt = 0x2b,
  DW_OP_le = 0x2c,
  DW_OP_lt = 0x2d,
  DW_OP_ne = 0x2e,
  DW_OP_lit0 = 0x30,
  DW_OP_lit1 = 0x31,
  DW_OP_lit2 = 0x32,
  DW_OP_lit3 = 0x33,
  DW_OP_lit4 = 0x34,
  DW_OP_lit5 = 0x35,
  DW_OP_lit6 = 0x36,
  DW_OP_lit7 = 0x37,
  DW_OP_lit8 = 0x38,
  DW_OP_lit9 = 0x39,
  DW_OP_lit10 = 0x3a,
  DW_OP_lit11 = 0x3b,
  DW_OP_lit12 = 0x3c,
  DW_OP_lit13 = 0x3d,
  DW_OP_lit14 = 0x3e,
  DW_OP_lit15 = 0x3f,
  DW_OP_lit16 = 0x40,
  DW_OP_lit17 = 0x41,
  DW_OP_lit18 = 0x42,
  DW_OP_lit19 = 0x43,
  DW_OP_lit20 = 0x44,
  DW_OP_lit21 = 0x45,
  DW_OP_lit22 = 0x46,
  DW_OP_lit23 = 0x47,
  DW_OP_lit24 = 0x48,
  DW_OP_lit25 = 0x49,
  DW_OP_lit26 = 0x4a,
  DW_OP_lit27 = 0x4b,
  DW_OP_lit28 = 0x4c,
  DW_OP_lit29 = 0x4d,
  DW_OP_lit30 = 0x4e,
  DW_OP_lit31 = 0x4f,
  DW_OP_reg0 = 0x50,
  DW_OP_reg1 = 0x51,
  DW_OP_reg2 = 0x52,
  DW_OP_reg3 = 0x53,
  DW_OP_reg4 = 0x54,
  DW_OP_reg5 = 0x55,
  DW_OP_reg6 = 0x56,
  DW_OP_reg7 = 0x57,
  DW_OP_reg8 = 0x58,
  DW_OP_reg9 = 0x59,
  DW_OP_reg10 = 0x5a,
  DW_OP_reg11 = 0x5b,
  DW_OP_reg12 = 0x5c,
  DW_OP_reg13 = 0x5d,
  DW_OP_reg14 = 0x5e,
  DW_OP_reg15 = 0x5f,
  DW_OP_reg16 = 0x60,
  DW_OP_reg17 = 0x61,
  DW_OP_reg18 = 0x62,
  DW_OP_reg19 = 0x63,
  DW_OP_reg20 = 0x64,
  DW_OP_reg21 = 0x65,
  DW_OP_reg22 = 0x66,
  DW_OP_reg23 = 0x67,
  DW_OP_reg24 = 0x68,
  DW_OP_reg25 = 0x69,
  DW_OP_reg26 = 0x6a,
  DW_OP_reg27 = 0x6b,
  DW_OP_reg28 = 0x6c,
  DW_OP_reg29 = 0x6d,
  DW_OP_reg30 = 0x6e,
  DW_OP_reg31 = 0x6f,
  DW_OP_breg0 = 0x70,
  DW_OP_breg1 = 0x71,
  DW_OP_breg2 = 0x72,
  DW_OP_breg3 = 0x73,
  DW_OP_breg4 = 0x74,
  DW_OP_breg5 = 0x75,
  DW_OP_breg6 = 0x76,
  DW_OP_breg7 = 0x77,
  DW_OP_breg8 = 0x78,
  DW_OP_breg9 = 0x79,
  DW_OP_breg10 = 0x7a,
  DW_OP_breg11 = 0x7b,
  DW_OP_breg12 = 0x7c,
  DW_OP_breg13 = 0x7d,
  DW_OP_breg14 = 0x7e,
  DW_OP_breg15 = 0x7f,
  DW_OP_breg16 = 0x80,
  DW_OP_breg17 = 0x81,
  DW_OP_breg18 = 0x82,
  DW_OP_breg19 = 0x83,
  DW_OP_breg20 = 0x84,
  DW_OP_breg21 = 0x85,
  DW_OP_breg22 = 0x86,
  DW_OP_breg23 = 0x87,
  DW_OP_breg24 = 0x88,
  DW_OP_breg25 = 0x89,
  DW_OP_breg26 = 0x8a,
  DW_OP_breg27 = 0x8b,
  DW_OP_breg28 = 0x8c,
  DW_OP_breg29 = 0x8d,
  DW_OP_breg30 = 0x8e,
  DW_OP_breg31 = 0x8f,
  DW_OP_regx = 0x90,
  DW_OP_fbreg = 0x91,
  DW_OP_bregx = 0x92,
  DW_OP_piece = 0x93,
  DW_OP_deref_size = 0x94,
  DW_OP_xderef_size = 0x95,
  DW_OP_nop = 0x96,
  DW_OP_push_object_address = 0x97,
  DW_OP_call2 = 0x98,
  DW_OP_call4 = 0x99,
  DW_OP_call_ref = 0x9a,
  DW_OP_form_tls_address = 0x9b,
  DW_OP_call_frame_cfa = 0x9c,
  DW_OP_bit_piece = 0x9d,
  DW_OP_implicit_value = 0x9e,
  DW_OP_stack_value = 0x9f,
  DW_OP_lo_user = 0xe0,
  DW_OP_hi_user = 0xff
};

enum class DwarfOperationType {
	Constant,
	Address,
	RegisterOffset,
	Arith,
	Branch,
	Stack
};

struct DwarfAbstractOperation {
	DwarfOperationType type;

	virtual ~DwarfAbstractOperation() { };
};

struct DwarfConstantOperation
				: public DwarfAbstractOperation {
  DwarfConstantOperation() { type = DwarfOperationType::Constant; }

  bool is_signed;
	t_uint32 width;
	t_uint64 value;
};

struct DwarfAddressOperation
				: public DwarfAbstractOperation {
  DwarfAddressOperation() { type = DwarfOperationType::Address; }

  t_address value;
};

struct DwarfArithOperation
				: public DwarfAbstractOperation {
  DwarfArithOperation() { type = DwarfOperationType::Arith; }

  t_uint64 value;
};

struct DwarfBranchOperation
				: public DwarfAbstractOperation {
  DwarfBranchOperation() { type = DwarfOperationType::Branch; }

  t_address value;
};

struct DwarfStackOperation
				: public DwarfAbstractOperation {
  DwarfStackOperation() { type = DwarfOperationType::Stack; }

  t_uint64 value;
};

struct DwarfRegisterOffsetOperation
				: public DwarfAbstractOperation {
  DwarfRegisterOffsetOperation() { type = DwarfOperationType::RegisterOffset; }

  DwarfOperationRegister reg;
  bool is_regnum;
  t_uint64 regnum;
  t_uint64 value;
};

DwarfAbstractOperation *DwarfDecodeOperation(DwarfCompilationUnitHeader *cu, DwarfSections *);

#endif /* DIABLOOBJECT_DWARF_OPERATIONS_H */
