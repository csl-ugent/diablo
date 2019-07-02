/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#ifndef ARM_UTILS_H
#define ARM_UTILS_H
/*! \todo Document */ 
#define ArmInsIsConditional(x)	   (!(ARM_INS_CONDITION(x) == ARM_CONDITION_AL))
/*! \todo Document */ 
#define ArmInsUpdatesCond(x)               (ARM_INS_FLAGS(x) & FL_S)
/*! \todo Document */ 
#define ArmInsHasImmediate(x)              ((ARM_INS_REGC(x) == ARM_REG_NONE)?1:0)
/*! \todo Document */ 
#define ArmInsIsNOOP(x)            (((ARM_INS_OPCODE(x) == ARM_MOV) && (ARM_INS_REGA(x) == ARM_INS_REGC(x)) && !(ARM_INS_FLAGS(x) & FL_S) && !ArmInsHasShiftedFlexible(x)) || ARM_INS_OPCODE(x)==ARM_NOP || ARM_INS_OPCODE(x)==ARM_T2NOP)
#define ArmInsIsExclusive(x)		((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(x)) && (ARM_INS_OPCODE(x) <= ARM_LDRSTREX_LAST))
#endif

#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_UTILS_FUNCTIONS
#define ARM_UTILS_FUNCTIONS
t_regset ArmUsedRegistersX(t_arm_ins * ins, t_uint8 *use);
#define ArmUsedRegisters(ins) ArmUsedRegistersX(ins, NULL)
t_regset ArmDefinedRegistersX(t_arm_ins * ins, t_uint8 *def);
#define ArmDefinedRegisters(ins) ArmDefinedRegistersX(ins, NULL)
t_bool ArmInsIsCommutative(t_arm_ins * ins);
int ArmInsWriteBackHappens(t_arm_ins * ins);
int ArmInsHasShiftedFlexible(t_arm_ins * ins);
t_arm_condition_code ArmInvertCondition(t_arm_condition_code condition);
t_bool ArmIsEncodableConstantForOpcode(t_uint32 cons, t_uint16 opc);
t_bool ArmIsEncodableConstantForOpcodeThumb(t_uint32 cons, t_uint16 opc);
t_bool ArmInsIsEncodableConstantForOpcode(t_uint32 cons, t_uint16 opc, t_bool thumb);
t_bool ArmConstantIsProducableInTwoInstructions(t_uint32 val);
void ArmEncodeConstantInTwoInstructions(t_arm_ins * ins, t_uint32 value);
t_arm_ins * ArmGetLastConditionalInsFromBlockWherePropagationSplits(t_bbl * bbl);
t_regset ArmInsRegsLiveAfterConditional(t_arm_ins * ins);
t_bool ArmBblCanInsertAfter(t_bbl *bbl);
t_bool ArmBblInSwitchStatement(t_bbl *bbl, t_bool only_check_default_case);
void ArmForwardToReturnDo(t_bbl * bbl, t_bool * do_forwarding);
t_bool ArmInsIsCopy(t_arm_ins * i_ins, t_reg * copy, t_reg * original);
t_bool ArmFunIsGlobal(t_function * fun);
t_address ArmModus(t_address ina, t_reloc * inr);
t_regset ArmGetThumbRegset(void);
t_bool ArmAdjustRegReadsBetweenByOffset(t_reg reg, t_arm_ins *begin, t_arm_ins *end, int stack_offset, t_bool perform_modifications);
t_int32 ArmInsMemBaseAdjustment(t_arm_ins *ins);
t_int32 ArmInsMemSize(t_arm_ins *ins);
void ArmInsGetMemOffsetRangeTouched(t_arm_ins *ins, t_int32 *ofs_low, t_int32 *ofs_high);
t_tristate ArmBblIsPerhapsThumb(t_bbl *bbl);
t_bool ArmBblIsThumb(t_bbl *bbl);
t_bool ArmConstIsImm12Encodable(int val);
t_bool ArmConstCanBeImmediateOperand(int val, t_bool thumb);
void ArmChainCreateHoleBefore(t_bbl *bbl);
t_bool ArmIsThumb1Encodable(t_arm_ins *ins);
t_bool ArmIsThumb1EncodableCheck(t_arm_ins *ins, t_bool check_condition);
t_bool ArmIsThumb1EncodableCheckItNoIt(t_arm_ins *ins, t_bool check_condition, t_bool it_ins_generated);
t_bool ArmIsThumb2Encodable(t_arm_ins *ins);
t_bool ArmIsThumb2EncodableCheck(t_arm_ins *ins, t_bool check_condition);
t_bool ArmIsThumb2ImmediateEncodable(t_uint32 imm);
t_bool ArmInsMustBeLastInITBlock(t_arm_ins *ins);
t_bool ArmInsIsValidInITBlock(t_arm_ins *ins);
t_bool ArmInsMustBeInITBlock(t_arm_ins *ins);
t_bool ArmInsChangesInstructionSet(t_arm_ins *ins);
t_arm_ins * ArmFindFirstIns(t_bbl *bbl);
t_bool ArmInsIsEncodable(t_arm_ins * ins);
t_bool ArmInsIsThumbEncodable(t_arm_ins * ins);
t_bool ArmInsIsSwitchedBL(t_arm_ins * ins);
t_uint32 ArmInsSwitchedBLTableEntrySize(t_arm_ins * ins);
t_bool ArmInsSwitchedBLIsSignExtend(t_arm_ins * ins);
void ArmInstructionIsDirectControlTransfer(t_ins * ins, t_bool* result);
t_bool ArmInsHasImmediateOp(t_arm_ins * ins);
t_bool ArmInsCanReplaceImmediateWithRegister(t_arm_ins *ins);
t_bool ArmInsIsInvariant(t_arm_ins *ins);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
