#ifndef ARM_META_API_H
#define ARM_META_API_H

#include "ARM_obfuscations.h"

extern "C" {
#include <obfuscation/generic/opaque_predicate_opt.h>
}

/* used as broker calls */
void MetaAPI_ProduceValueInRegister(MetaAPI_Instance *instance, AbstractValue *abstract_value, AbstractValue::ProduceValueInfo *info);
void MetaAPI_ProduceValueInSection(t_cfg *cfg, AbstractValue *abstract_value, t_section *section);
void MetaAPI_StoreInstancePointer(MetaAPI_Storage *instance, t_bbl *target, t_reg from, t_reg tmp);
void MetaAPI_EvaluateGetterResult(t_bbl *target, t_regset *overwritten_registers);
void MetaAPI_GetRandomRegistersBroker(t_regset *regs);
void MetaAPI_SaveRestoreRegisters(PreparedCallSite *call_site, t_regset *overwritten_registers, t_regset *live, bool reused, bool from_hell);
void MetaAPI_EmitPrintfDebug(PreparedCallSite *call_site, PrintfDebugData *data);
void MetaAPI_GetCompareRegisters(t_regset *regs);
void MetaAPI_CantBeLiveRegisters(t_regset *regs);
void MetaAPI_ReserveStackSpace(PreparedCallSite *call_site, t_int32 slotcount);
void MetaAPI_CompareFunctionResult(t_bbl *target, bool inverted_condition);
void MetaAPI_CompareRegisters(t_bbl *target, t_reg a, t_reg b, MetaAPI_Relation::Type rel);
void MetaAPI_ActuallySaveRestoreRegisters(t_function *f);
void MetaAPI_ProduceValueOnStack(t_bbl *bbl, t_reg from, t_int32 stack_slot, bool prepend);
void MetaAPI_Compare(t_bbl *target, bool inverted_condition, MetaAPI_CompareConfiguration *conf);
void MetaAPI_WriteVTablePointer(PreparedCallSite *call_site, t_symbol *vtable_symbol, t_regset *overwritten_registers);
void MetaAPI_PrintInstancePointer(PreparedCallSite *call_site, MetaAPI_Instance *instance);
void MetaAPI_Dereference(t_bbl* target, t_reg to, t_reg from);
void MetaAPI_DoOperand(t_bbl *target, t_reg op1, t_reg op2, MetaAPI_Operand::Type operand);
void MetaAPI_Store(t_bbl *target, t_reg destination, t_reg base_register);
void MetaAPI_ProduceRegValueInSection(t_bbl *target, t_section *sec, MetaAPI_ImplementationValue *array_index, t_reg reg, t_reg tmp);

#include "ARM_meta_api_transformation.h"

#endif /* ARM_META_API_H */
