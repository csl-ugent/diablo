/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "ARM_obfuscations.h"
#include "ARM_architecture_backend.h"

/* for the broker calls */
#include <diabloflowgraph_dwarf.h>
#include "ARM_advanced_factoring.h"

/* for the initializers */
#include "ARM_branch_function.h"
#include "ARM_flatten_function.h"
#include "ARM_flip_branches.h"
#include "ARM_opaque_predicates.h"
#include "ARM_schedule_instructions.h"
#include "ARM_split_twoway_predicate.h"
#include "ARM_meta_api_transformation.h"
#include "ARM_meta_api.h"

/* for the command-line options */
#include <obfuscation/generic/flatten_function_opt.h>

using namespace std;

ARMObfuscationArchitectureInitializer::ARMObfuscationArchitectureInitializer() {
  created_obfuscations.clear();

  created_obfuscations.push_back(new ARMRegisterBasedBranchFunctionTransformation());
  created_obfuscations.push_back(new ARMSplitCallWithBranchFunctionsTransformation());
  if (obfuscation_flatten_function_options.flatten_function_double) {
    created_obfuscations.push_back(new ARMFlattenFunctionTransformation(true /* flatten multiple */));
  }
  created_obfuscations.push_back(new ARMFlattenFunctionTransformation(false /* flatten multiple */));
  created_obfuscations.push_back(new ARMXX2OpaquePredicateTransformation());
  created_obfuscations.push_back(new ARM7Y2OpaquePredicateTransformation());
  created_obfuscations.push_back(new ARM2DivXXOpaquePredicateTransformation());
  created_obfuscations.push_back(new ARMX2GE0OpaquePredicateTransformation());
  created_obfuscations.push_back(new ARMX2DivX22OpaquePredicateTransformation());
  created_obfuscations.push_back(new ARMX2DivXv8DivX21OpaquePredicateTransformation());
  created_obfuscations.push_back(new ARMArboitPredicateSumOpaquePredicateTransformation());
  created_obfuscations.push_back(new ARM3DivX3_3OpaquePredicateTransformation());

  created_obfuscations.push_back(new ArmFlipBranchesTransformation());
  /* TODO: some of the above obfuscations might really only have a subclass to call setArchitectureInfoWrapper, do those like SplitOffFunctionHeadTransformation as well */

  created_obfuscations.push_back(new ARMMetaApiTransformation());

  created_obfuscations.push_back(new SplitOffFunctionHeadTransformation());
  created_obfuscations.push_back(new ARMSplitWithTwoWayPredicateTransformation());
  created_obfuscations.push_back(new ARMCallFunctionTransformation());

  created_obfuscations.push_back(new ARMScheduleInstructionsTransformation());

  DiabloBrokerCallInstall("MarkSlicesInBbl", "t_bbl *, int (*)(t_ins *), void (*)(t_ins *, int)", (void*)MarkSlicesInBbl, TRUE);
  DiabloBrokerCallInstall("MarkSliceForIns", "t_ins *, int (*)(t_ins *), void (*)(t_ins *, int)", (void*)MarkSliceForIns, TRUE);
  DiabloBrokerCallInstall("RescheduleBblForSlice", "t_bbl *, void (*)(t_ins *, int)", (void*)RescheduleBblForSlice, TRUE);
  DiabloBrokerCallInstall("RescheduleBblForSequence", "t_bbl *, int (*)(t_ins *)", (void*)RescheduleBblForSequence, TRUE);
  DiabloBrokerCallInstall("CanonicalizeBbl", "t_bbl *, t_uint16 (*)(t_ins *)", (void*)CanonicalizeBbl, TRUE);
  DiabloBrokerCallInstall("FreeDagForBbl", "t_bbl *", (void*)FreeDagForBbl, TRUE);
  DiabloBrokerCallInstall("ShouldKeepInsCombination", "t_ins *, t_ins *, t_bool *", (void *)ShouldKeepInsCombination, TRUE);

  DiabloBrokerCallInstall("MetaAPI_ProduceValueInRegister", "MetaAPI_Instance *, AbstractValue *, AbstractValue::ProduceValueInfo *", (void *)MetaAPI_ProduceValueInRegister, TRUE);
  DiabloBrokerCallInstall("MetaAPI_ProduceValueInSection", "t_cfg *, AbstractValue *, t_section *", (void *)MetaAPI_ProduceValueInSection, TRUE);
  DiabloBrokerCallInstall("MetaAPI_StoreInstancePointer", "MetaAPI_Storage *, t_bbl *, t_reg, t_reg", (void *)MetaAPI_StoreInstancePointer, TRUE);
  DiabloBrokerCallInstall("MetaAPI_EvaluateGetterResult", "t_bbl *, t_regset *", (void *)MetaAPI_EvaluateGetterResult, TRUE);
  DiabloBrokerCallInstall("MetaAPI_GetRandomRegisters", "t_regset *", (void *)MetaAPI_GetRandomRegistersBroker, TRUE);
  DiabloBrokerCallInstall("MetaAPI_SaveRestoreRegisters", "PreparedCallSite *, t_regset *, t_regset *, t_bool, t_bool", (void *)MetaAPI_SaveRestoreRegisters, TRUE);
  DiabloBrokerCallInstall("MetaAPI_EmitPrintfDebug", "PreparedCallSite *, PrintfDebugData *", (void *)MetaAPI_EmitPrintfDebug, TRUE);
  DiabloBrokerCallInstall("MetaAPI_GetCompareRegisters", "t_regset *regs", (void *)MetaAPI_GetCompareRegisters, TRUE);
  DiabloBrokerCallInstall("MetaAPI_CantBeLiveRegisters", "t_regset *regs", (void *)MetaAPI_CantBeLiveRegisters, TRUE);
  DiabloBrokerCallInstall("MetaAPI_Test", "t_cfg *, t_string", (void *)MetaAPI_PerformTest, TRUE);
  DiabloBrokerCallInstall("MetaAPI_ReserveStackSpace", "PreparedCallSite *, t_int32", (void *)MetaAPI_ReserveStackSpace, TRUE);
  DiabloBrokerCallInstall("MetaAPI_CompareFunctionResult", "t_bbl *, t_bool", (void *)MetaAPI_CompareFunctionResult, TRUE);
  DiabloBrokerCallInstall("MetaAPI_CompareRegisters", "t_bbl *, t_reg, t_reg, MetaAPI_Relation::Type", (void *)MetaAPI_CompareRegisters, TRUE);
  DiabloBrokerCallInstall("MetaAPI_ProduceValueOnStack", "t_bbl *, t_reg, t_int32, t_bool", (void *)MetaAPI_ProduceValueOnStack, TRUE);
  DiabloBrokerCallInstall("MetaAPI_Compare", "t_bbl *, bool, MetaAPI_CompareConfiguration *", (void *)MetaAPI_Compare, TRUE);
  DiabloBrokerCallInstall("MetaAPI_WriteVTablePointer", "PreparedCallSite *, t_symbol *, t_regset *", (void *)MetaAPI_WriteVTablePointer, TRUE);
  DiabloBrokerCallInstall("MetaAPI_KeepLive", "t_object *", (void *)MetaAPI_KeepLive, TRUE);
  DiabloBrokerCallInstall("MetaAPI_PrintInstancePointer", "PreparedCallSite *, MetaAPI_Instance *", (void *)MetaAPI_PrintInstancePointer, TRUE);
  DiabloBrokerCallInstall("MetaAPI_Dereference", "t_bbl*, t_reg, t_reg", (void *)MetaAPI_Dereference, TRUE);
  DiabloBrokerCallInstall("MetaAPI_DoOperand", "t_bbl*, t_reg, t_reg, MetaAPI_Operand::Type", (void *)MetaAPI_DoOperand, TRUE);
  DiabloBrokerCallInstall("MetaAPI_Store", "t_bbl *, t_reg, t_reg", (void *)MetaAPI_Store, TRUE);
  DiabloBrokerCallInstall("MetaAPI_ProduceRegValueInSection", "t_bbl *, t_section *, MetaAPI_ImplementationValue *, t_reg, t_reg", (void *)MetaAPI_ProduceRegValueInSection, TRUE);
}

ARMObfuscationArchitectureInitializer::~ARMObfuscationArchitectureInitializer() {
  for (auto obf : created_obfuscations)
    delete obf;

  DiabloAnoptArmFini();
}

void ARMObfuscationArchitectureInitializer::Init(t_uint32 argc, char **argv) {
  DiabloAnoptArmInit (argc, argv);
}
