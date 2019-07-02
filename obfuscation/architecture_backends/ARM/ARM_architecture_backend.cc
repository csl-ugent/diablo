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
}

ARMObfuscationArchitectureInitializer::~ARMObfuscationArchitectureInitializer() {
  for (auto obf : created_obfuscations)
    delete obf;

  DiabloAnoptArmFini();
}

void ARMObfuscationArchitectureInitializer::Init(t_uint32 argc, char **argv) {
  DiabloAnoptArmInit (argc, argv);
}
