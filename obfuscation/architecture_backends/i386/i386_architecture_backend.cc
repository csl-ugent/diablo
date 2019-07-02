/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "i386_obfuscations.h"
#include "i386_architecture_backend.h"

/* for the initializers */
#include "i386_branch_function.h"
#include "i386_flatten_function.h"
#include "i386_flip_branches.h"
#include "i386_opaque_predicates.h"
#include "i386_schedule_instructions.h"
#include "i386_split_twoway_predicate.h"

/* for the command-line options */
#include <obfuscation/generic/flatten_function_opt.h>

using namespace std;

void BrokerI386Stuckness(t_object *obj) {
  VERBOSE(0, ("Broker call: I386 stuckness"));
  I386StucknessAnalysis (obj);
}

I386ObfuscationArchitectureInitializer::I386ObfuscationArchitectureInitializer() {
  if (obfuscation_flatten_function_options.flatten_function_double) {
    new I386FlattenFunctionTransformation(true /* flatten multiple */);
  }
  new I386FlattenFunctionTransformation(false /* flatten multiple */);
  new I386BranchFunctionTransformation();
  
  /* For now: only for diversity */
  if (DiversityTransformsEnabled()) {
    new SplitCallWithBranchFunctionsTransformation();
    new I386FlipBranchesTransformation();
    new SplitOffFunctionHeadTransformation();
    new I386SplitWithTwoWayPredicateTransformation();
  }

  new I386CallFunctionTransformation();

  new I386OpaquePredicateTransformation();
#if 0
  new I386InlineFunctionTransformation();
  // TODO: this should really only be instantiated at the end of a set of program transformations, so that it does not pollute the pool of transformations
  //new I386InstructionSelectionTransformation();
#endif

  new I386ScheduleInstructionsTransformation();

  DiabloBrokerCallInstall (
      "StucknessAnalysis", "t_object *", 
      (void*) BrokerI386Stuckness, FALSE);
}

void I386ObfuscationArchitectureInitializer::Init(t_uint32 argc, char **argv) {
  DiabloAnoptI386Init(argc, argv);
}

t_function* GetReturnAddressStubForRegister(t_cfg* cfg, t_reg reg) {
  static map<t_reg, t_function*> funmap;

  if (funmap.find(reg) != funmap.end())
    return funmap[reg];

  t_bbl* bbl = BblNew(cfg);
  t_bbl* retBlock = BblNew(cfg);
  t_i386_ins* ins;
  t_function* fun;

  fun = FunctionMake(bbl, "ReturnAddressStub", FT_NORMAL);

  /* mov (%esp), %reg
   * ret  */
  I386MakeInsForBbl(MovFromMem, Append, ins, bbl, reg /* dest */, 0 /* offset */, I386_REG_ESP /* base */, I386_REG_NONE /* index */, 0 /* scale */);
  I386MakeInsForBbl(Return,Append,ins,bbl);

  BblInsertInFunction(retBlock,fun);

  CfgEdgeNew(cfg,bbl,retBlock,ET_JUMP);
  CfgEdgeNew(cfg,retBlock,CFG_HELL_NODE(cfg),ET_RETURN);

  funmap[reg] = fun;
  return fun;
}
