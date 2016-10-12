/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_STACK_H_
#define ARM_STACK_H_

typedef struct _t_stack_info t_stack_info;
typedef struct _t_stack_slot t_stack_slot;

struct _t_stack_slot
{
  struct _t_stack_slot * next;
  t_reg saved;
  t_reg loaded;
  t_bool live;
  t_inslist * savers; /* All instructions that save a reg in this stack_slot */
  t_inslist * loaders;/* All instructions that load a reg from this stack slot */
  t_inslist * allocers;/* All instructions that decrease the stackpointer and thus allocate this stack slot */
  t_inslist * deallocers;/* All instructions that increase the stackpointer and thus deallocate this stack slot */
  t_inslist * out_frame;/* All instructions that refer to the stackframe of the caller, or produce a pointer into the current stackframe */
};

struct _t_stack_info
{
  t_uint32 size;
  t_stack_slot * stack;
};

FUNCTION_DYNAMIC_MEMBER_GLOBAL_BODY(stack_info, STACK_INFO, StackInfo, t_stack_info *, {*valp=NULL;}, {}, {});

/* An invalid pointer used in copy analysis to track the stackpointer inside a function */
#define SYMBOLIC_STACK_POINTER (t_reloc*)0x1

#define INS_LIST_NEXT(x) (force_cast(t_inslist *, (x)->next))
#define INS_LIST_INS(x)  ((t_inslist*) x)->ins
void ArmFunComputeStackSavedRegisters(t_cfg * cfg,t_function * fun);
t_bool ArmFunIsGlobal(t_function * fun);
void ArmLookAtStack(t_cfg * cfg);
void ArmMarkNoStackChangeFunctions(t_cfg * cfg);
void ArmValidatePushPopRegsets(t_cfg * cfg);
#endif

/* vim: set shiftwidth=2 foldmethod=marker: */
