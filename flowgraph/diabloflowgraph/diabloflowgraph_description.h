/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOFLOWGRAPH_DESCRIPTION_TYPEDEFS
#define DIABLOFLOWGRAPH_DESCRIPTION_TYPEDEFS
typedef struct _t_architecture_description t_architecture_description;
typedef enum
{ CONTEXT_SENSITIVE, CONTEXT_INSENSITIVE, TRIVIAL } t_analysis_complexity;
#endif

#ifndef DIABLOFLOWGRAPH_DESCRIPTION_DEFINES
#define DIABLOFLOWGRAPH_DESCRIPTION_DEFINES
#define TotalRegs(x) ((x)->num_int_regs + (x)->num_float_regs + (x)->num_predicate_regs + (x)->num_branch_regs + (x)->num_special_regs)
#endif
#include <diabloflowgraph.h>

#ifdef DIABLOFLOWGRAPH_TYPES
/* Description Types {{{ */
#ifndef DIABLOFLOWGRAPH_DESCRIPTION_TYPES
#define DIABLOFLOWGRAPH_DESCRIPTION_TYPES

/*! A description of the architecture */
struct _t_architecture_description
{
  /* Describe the architectures general characteristics */
  t_uint32 minimal_encoded_instruction_size;
  t_uint32 maximum_encoded_instruction_size;
  t_uint32 encoded_instruction_mod_size; /* Normal this is equal to minimal */
  t_uint32 bundle_size;
  t_uint32 template_size;
  t_uint32 decoded_instruction_size;


  t_address_type address_size;


  t_uint32 num_int_regs;
  t_uint32 num_float_regs;
  t_uint32 num_predicate_regs;
  t_uint32 num_branch_regs;
  t_uint32 num_special_regs;
  t_regset all_registers;
  t_regset int_registers;
  t_regset flt_registers;
  t_regset cond_registers;
  t_regset callee_saved;
  t_regset callee_may_use;
  t_regset callee_may_change;
  t_regset callee_may_return;
  t_regset always_live;
  t_regset registers_prop_over_hell;
  t_regset const_registers;
  t_regset dead_over_call; /* Registers that must not be propagated over call, swi and return edges */
  t_regset link_register;
  t_regset argument_regs;
  t_regset return_regs;
  t_regset dyncall_may_use; /* registers whose original value may be used by PLT calls */

  t_reg program_counter; /* set to REG_NONE if there is no visible program counter */
  char **register_names;

  /* Section callbacks */

  void (*DisassembleSec) (t_section *);
  void (*AssembleSec) (t_section *);
  void (*Flowgraph) (t_object *);
  void (*Deflowgraph) (t_object *);
  void (*MakeAddressProducers) (t_cfg *);

  /* Instruction callbacks */
  void (*InsCleanup) (t_ins *);
  void (*InsMakeDirectBranch) (t_ins *);
  void (*InsDupDynamic) (t_ins *, t_ins *);
  t_bool (*InsHasSideEffect) (t_ins *);
  t_bool (*InsIsLoad) (t_ins * ins);
  t_bool (*InsIsStore) (t_ins * ins);
  t_bool (*InsIsProcedureCall) (t_ins *);
  t_bool (*InsIsIndirectCall) (t_ins *);
  t_bool (*InsIsConditional) (t_ins *);
  t_bool (*InsIsUnconditionalJump) (t_ins *);
  t_bool (*InsIsControlflow) (t_ins *);
  t_bool (*InsIsSystem) (t_ins *);
  t_tristate (*InsIsSyscallExit) (t_ins *);
  void (*InsPrint) (t_ins *, t_string);
  void (*InsMakeNoop) (t_ins *);
  t_bool (*InsCmp) (t_ins *, t_ins *);

  t_bool (*InstructionUnconditionalizer) (t_ins *);
  t_bool (*InstructionParseFromStringAndInsertAt) (t_string, t_bbl *, t_ins *, t_bool);

  /* Bbl callbacks */
  t_ins *(*BblAddJumpInstruction) (t_bbl *);

  /* Fun callbacks */
  t_bool (*FunIsGlobal) (t_function *);

  char **unbehaved_funs;
  void (*computeLiveRegsBeforeSwi) (t_regset *, t_ins *);
  t_bool (*IsCopy) (t_ins *, t_reg *, t_reg *);
  t_address (*Modus) (t_address x, t_reloc * rel);
};
#endif
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
