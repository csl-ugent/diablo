/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* This files contains the datastructures and functions to express relations that hold
 * between the contents of registers. All information is stored in the form :
 * "reg1 - reg2 + constant - tag1 + tag2 = 0". If max_nr_equations is the maximum number of registers then
 * there are max_nr_equations - 1 equations to store all information. In these equations a register
 * can appear only once as reg1 and once as reg2. Equality of registers can be found by summing
 * up these equations, and to look for 'constant = 0'. This is exactly what is done by
 * EquationRegsEqual.
 * When reg1 == REG_BOT then the equation is not used, which means eg. 'R12 - R12 + 0 = 0'
 * 
 * With these equations we can keep track of all information like:
 * R1 = R2 +/- Const		-> enter EquationAdd(eqs, R1, R2, -/+ Const)
 * R8 = R3			-> enter EquationAdd(eqs, R8, R3, 0)
 * R4 = Const			-> enter EquationAdd(eqs, R4, NULLREG, -Const)
 * and possibly
 * R3 = R3 +/- Const		-> enter EquationAdd(eqs, R3, R3, -/+ Const)
 * provided that there was already an equation present with R3 in it.
 * When a register gets defined in any other way, use EquationInvalidate to remove the information
 * from the equations.
 */

#ifndef DIABLO_EXTENDED_COPY_TYPEDEFS
#define DIABLO_EXTENDED_COPY_TYPEDEFS
typedef struct _t_equation t_equation;
typedef t_equation* t_equations;

#endif

#ifndef DIABLO_EXTENDED_COPY_H
#define DIABLO_EXTENDED_COPY_H
#include <diabloflowgraph.h>

#define REG_TOP 254
#define REG_BOT 255
#define ZERO_REG(cfg)	(CFG_DESCRIPTION(cfg)->num_int_regs)

#define EquationSetBot(eq,i)   do {(eq).rega = i ; (eq).regb = REG_BOT; } while (0)
#define EquationSetTop(eq)   do {(eq).rega = REG_TOP ; (eq).regb = 0; eq.taga = NULL; eq.tagb = NULL;} while (0)
#define EquationIsBot(eq)   ((eq).regb == REG_BOT)
#define EquationIsTop(eq)   ((eq).rega == REG_TOP)

struct _t_equation
{
  unsigned rega;
  unsigned regb;
  int constant;
  t_reloc* taga;
  t_reloc* tagb;
};


void EquationsAdd(t_cfg *, t_equations eqs, t_reg defined, t_reg used, t_int32 constant, t_reloc* taga, t_reloc* tagb);
void EquationsInvalidate(t_cfg * cfg, t_equations equations, t_reg reg1);
void EquationsInvalidateRegset(t_cfg * cfg, t_equations equations, t_regset regs);
t_bool EquationsJoin(t_cfg *, t_equations eqs1, t_equations eqs2);
void EquationsResetAll(t_equations eqs);
t_tristate EquationsRegsEqual(t_cfg *, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 constant, t_reloc* tag1, t_reloc* tag2);
t_tristate EquationsRegsDiffer(t_cfg *, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 * constant);
t_tristate EquationsRegsDifferByTag(t_cfg *, t_equations eqs, t_reg reg1, t_reg reg2, t_reloc **, t_int32 * offset);
t_tristate EquationsRegsDifferWithTagAllowed(t_cfg * cfg, t_equations eqs, t_reg reg1, t_reg reg2, t_int32 * constant, t_reloc * tag);
t_equations EquationsNew(t_cfg *);
void EquationsFree(t_equations eq);
void EquationsRealFree(void);
void EquationsSetAllBot(t_cfg *, t_equations eqs);
void EquationsSetAllTop(t_cfg *,t_equations eqs);

void EquationsCopy(t_cfg *, t_equations src, t_equations dest);
void EquationsPrint(t_cfg *, t_equations eqs);
void EquationPrint(t_cfg *, t_reg index, t_equation* eq);
t_equation EquationSum(t_cfg *, t_reg a1, t_equation eq1, t_reg a2, t_equation eq2);
#define T_EQUATIONS(x)              ((t_equations)(x))
#endif
