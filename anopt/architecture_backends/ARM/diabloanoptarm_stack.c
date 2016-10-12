/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>

FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(stack_info);

#define AGGRESSIVE_LIVENESS 
//#define DEBUG_STACKANALYSIS
#define VERBOSE_STACKANALYSIS 1
/* Leave off... Triggers bugs in vortex */
/* #define CALC_SAVED_CHANGED */
/* current ARM abis require 8 byte stack alignment */
#define GUARANTEE_DWORD_STACK_ALIGNMENT

void CheckEquations(t_bbl * bbl);

void InsListCleanup(t_inslist ** start)
{
  t_inslist * iter;
  while(*start)
  {
    iter = *start;
    *start = INS_LIST_NEXT(*start);
    Free(iter);
  }
}


t_bool FunctionOnlySpills(t_function * fun);

t_bool ArmInsSpillsToStack(t_arm_ins * ins);
void ArmOptimizeStackOperations(t_cfg * cfg);
void ArmMakeStackTransparent(t_cfg * cfg);
void FunctionReplaceRegister(t_function * fun, t_reg orig, t_reg new);
void ArmFunComputeStackSavedRegistersFromStackInfo(t_function * fun);
void ArmValidatePushPopRegsets(t_cfg * cfg);


t_stack_info * NewStackInfo(t_int32 size);
void FreeStackInformation(t_cfg * cfg);
void FreeStackSlots(t_stack_info * stack);
void StackInfoPrint(t_stack_info * stack, t_bool print_arm_ins);
void ArmSaveStackInformationForMemory(t_stack_info * stack,t_arm_ins * i_ins,t_equations eqs, t_int32 stackoffset, t_int32 curr_stack_value);
t_bool ArmSaveStackInformationForDataproc(t_stack_info * stack,t_arm_ins * i_ins,t_equations eqs, t_int32 stackoffset);
void ArmSaveStackInformationForStackUse(t_stack_info * stack,t_arm_ins * ins,t_equations eqs, t_int32 stackoffset);

/** This function computes the registers that a procedure saved on the stack */
/* XXX: in fact this is unsafe! (hand-written assembly code can easily change
 * the saved registers on the stack) */
/*{{{*/
extern void ArmFunComputeStackSavedRegisters(t_cfg * cfg, t_function * ifun)
{
  /*t_bbl*  i_bbl;
  t_arm_ins* iins;
  t_int32 count;
  t_int32 SavedRegOffset[64];
  t_regset use;
  t_regset def;
  t_regset saved;
  t_reg reg;
  t_uint32 offset = 0;
  t_cfg_edge * i_edge; */

  FUNCTION_SET_REGS_SAVED(ifun, RegsetNew());
  if (!diabloanopt_options.do_compute_savedchanged_regs)
    return;

#if 0 
  /* {{{ apparently obsoleted code */
#if MAX_NR_REG_SUBSETS > 4
  FATAL(("Immediate regs must be real regsets!"));
#else
  REGSET_FOREACH_REG(arm_description.all_registers,reg)
    {
      SavedRegOffset[reg] = -1;
    }
#endif  
  i_bbl = FUNCTION_BBL_FIRST(ifun);
  saved = RegsetNew();
  use  = RegsetNew();
  def  = RegsetNew();
    
  /* do the stores as a first approximation - we look in the very first bbl of the function*/

  BBL_FOREACH_ARM_INS(i_bbl,iins)
    {
      if(ARM_INS_OPCODE(iins) == ARM_STF && ARM_INS_REGB(iins) == ARM_REG_R13 && ArmInsWriteBackHappens(iins))
	{
	  offset -= ARM_INS_IMMEDIATE(iins);
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if((ARM_INS_OPCODE(iins) == ARM_LDR || ARM_INS_OPCODE(iins) == ARM_LDF) && ARM_INS_REGB(iins) == ARM_REG_R13 && ArmInsWriteBackHappens(iins))
	{
	  offset += ARM_INS_IMMEDIATE(iins);
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if(ARM_INS_OPCODE(iins) == ARM_SUB && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_NONE) 
	{
	  offset -= ARM_INS_IMMEDIATE(iins);
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if(ARM_INS_OPCODE(iins) == ARM_ADD && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_NONE) 
	{
	  offset += ARM_INS_IMMEDIATE(iins);
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if(ARM_INS_OPCODE(iins) == ARM_RSB && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_NONE) 
	{
	  offset -= ARM_INS_IMMEDIATE(iins);
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if ((ARM_INS_OPCODE(iins) == ARM_RSB || ARM_INS_OPCODE(iins) == ARM_SUB  || ARM_INS_OPCODE(iins) == ARM_LDR || ARM_INS_OPCODE(iins) == ARM_MOV) && ARM_INS_REGA(iins) == ARM_REG_R13) 
	{
	  /* WE LOSE TRACK OF THE STACK POINTER, THEREFORE THERE IS NO USE IN ADDING OTHER REGISTERS FURTHER DOWN IN THIS BLOCK*/
	  while (iins)
	    {
	      RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	      REGSET_FOREACH_REG(arm_description.all_registers,reg)
		{
		  if (RegsetIn(saved,reg)) continue;
		  SavedRegOffset[reg] = -1;
		}
	      RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	      RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	      iins = ARM_INS_INEXT(iins);
	    }
	  break;
	}
      else if (ARM_INS_OPCODE(iins) == ARM_LDM && ARM_INS_REGB(iins) == ARM_REG_R13 && ArmInsWriteBackHappens(iins))
	{
	  REGSET_FOREACH_REG(ARM_INS_IMMEDIATE(iins),reg)
	    {
	      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		offset +=4;
	      else 
		offset -=4;
	    }
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if (ARM_INS_OPCODE(iins) == ARM_LDM && RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13) && RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R15))
	{
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
	}
      else if (ARM_INS_OPCODE(iins) == ARM_SWI)
	{
	}
      else if (ARM_INS_OPCODE(iins) == ARM_SFM && ArmInsWriteBackHappens(iins))
	{
	  t_uint32 local_offset = offset;

	  if (!ArmInsWriteBackHappens(iins))
	    FATAL(("no writeback @I\n",iins));
	  else
	  {
	    t_regset saved_regs ;
	    if (ARM_INS_FLAGS(iins) & FL_PREINDEX)
	      {
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  offset +=4*ARM_INS_IMMEDIATE(iins);
		else 
		  offset -=4*ARM_INS_IMMEDIATE(iins);;
	      }
	    
	    local_offset = offset;
	    
	    saved_regs = ARM_INS_MULTIPLE(iins);
	    REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
	      {
		if (!RegsetIn(use,reg) && !RegsetIn(def,reg))
		  {
		    if (RegsetIn(saved,reg))
		      {
			RegsetSetSubReg(saved,reg);
			RegsetSetAddReg(use,reg);
			SavedRegOffset[reg] = -1;
		      }
		    else
		      {
			RegsetSetAddReg(saved,reg);
			SavedRegOffset[reg] = local_offset;
		      }
		  }
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  local_offset-=4;
		else
		  local_offset+=4;
	      }
	    
	    if (!(ARM_INS_FLAGS(iins) & FL_PREINDEX))
	      {
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  offset +=4*ARM_INS_IMMEDIATE(iins);
		else 
		  offset -=4*ARM_INS_IMMEDIATE(iins);
	      }
	    
	  }
	}
      else if (ARM_INS_OPCODE(iins) == ARM_LFM && ArmInsWriteBackHappens(iins))
	{
	  t_uint32 local_offset = offset;
	  t_regset saved_regs;
	  if (!ArmInsWriteBackHappens(iins))
	    FATAL(("no writeback @I\n",iins));
	  else
	  {
	    if (ARM_INS_FLAGS(iins) & FL_PREINDEX)
	      {
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  offset +=4*ARM_INS_IMMEDIATE(iins);
		else 
		  offset -=4*ARM_INS_IMMEDIATE(iins);;
	      }
	    
	    local_offset = offset;
	    
	    saved_regs = ARM_INS_MULTIPLE(iins);
	    REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
	      {
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  local_offset-=4;
		else
		  local_offset+=4;
	      }
	    
	    if (!(ARM_INS_FLAGS(iins) & FL_PREINDEX))
	      {
		if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		  offset +=4*ARM_INS_IMMEDIATE(iins);
		else 
		  offset -=4*ARM_INS_IMMEDIATE(iins);
	      }
	    
	  }
	}
      else if (ARM_INS_OPCODE(iins)==ARM_STR && ARM_INS_REGB(iins) == ARM_REG_R13)
	{
	  t_uint32 imm = ARM_INS_IMMEDIATE(iins);

	  if (ArmInsWriteBackHappens(iins))
	  {
	    if (ARM_INS_FLAGS(iins) & FL_DIRUP)
	      FATAL(("dirup store @I\n",iins));
	    if (!(ARM_INS_FLAGS(iins) & FL_PREINDEX))
	      FATAL(("no preindex @I\n",iins));
	    offset-=imm;
	  }

	  reg = ARM_INS_REGA(iins);

	  /* only if reg has not been used and not been defined
	     we have a true save */ 

	  if (!RegsetIn(use,reg) && !RegsetIn(def,reg))
            {
	      if (RegsetIn(saved,reg))
                {
		  RegsetSetSubReg(saved,reg);
		  RegsetSetAddReg(use,reg);
		  SavedRegOffset[reg] = -1;
                }
	      else
                {
		  RegsetSetAddReg(saved,reg);
		  if(ArmInsWriteBackHappens(iins))
		    SavedRegOffset[reg] = offset; 
		  else 
		    SavedRegOffset[reg] = offset+imm;
                }
            }

	  /* do not consider this a use ! */
        }
      else if (ARM_INS_OPCODE(iins) == ARM_STM && ARM_INS_REGB(iins) == ARM_REG_R13 &&
	       !(ARM_INS_FLAGS(iins) & FL_DIRUP) &&
	       (ARM_INS_FLAGS(iins) & FL_PREINDEX)
	       )
	{
	  t_int32 old_offset = offset;  

	  t_regset saved_regs = ARM_INS_IMMEDIATE(iins);
	  REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
	    {
	      offset-=4;
	      if (!RegsetIn(use,reg) && !RegsetIn(def,reg))
		{
		  if (RegsetIn(saved,reg))
		    {
		      RegsetSetSubReg(saved,reg);
		      RegsetSetAddReg(use,reg);
		      SavedRegOffset[reg] = -1;
		    }
		  else
		    {
		      RegsetSetAddReg(saved,reg);
		      SavedRegOffset[reg] = offset; 
		    }
		}
            }
	  if (!ArmInsWriteBackHappens(iins))
	    offset = old_offset;
	}
      else
        {
	  if (RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13))
	    {
	      WARNING(("ins: @I\n bbl:@iB\nstack pointer manipulation\n",iins,i_bbl));
	      break;
	    }
	  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(ARM_INS_REGS_USE(iins)),def));
	  REGSET_FOREACH_REG(arm_description.all_registers,reg)
            {
	      if (RegsetIn(saved,reg)) continue;
	      SavedRegOffset[reg] = -1;
            }
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));
	  RegsetSetUnion(use,ARM_INS_REGS_USE(iins));
        }
    }

  /* now make sure that nothing used subsequently */

  RegsetSetIntersect(saved,RegsetUnion(RegsetNewInvers(BBL_REGS_LIVE_OUT(i_bbl)),def));

  REGSET_FOREACH_REG(arm_description.all_registers,reg)
    {
      if(RegsetIn(saved,reg)) continue;
      SavedRegOffset[reg] = -1;
    }
#if 0
  printf("========================================================================================================\n");

  DEBUG(("@iB @A\n",i_bbl,&arm_description,BBL_REGS_LIVE_OUT(i_bbl));

  REGSET_FOREACH_REG(saved,reg)
    {
      printf("%d: %d\n",reg,SavedRegOffset[reg]);
    }
#endif
  /*now check the loads - we look at all the return bbls*/
  
  count = 0;
    
  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_LAST(ifun),i_edge)
    {
      if (CFG_EDGE_CAT(i_edge)!=ET_JUMP) continue;
      i_bbl = CFG_EDGE_HEAD(i_edge);

      count++;

      def = RegsetNew();
      saved = RegsetNew();
        
      /*      printf("--------------------------------------------------------------------\n");*/

      offset = 0;

      BBL_FOREACH_ARM_INS_R(i_bbl,iins)
        {
	  /*	  VERBOSE(0,("offset %d @I\n",offset,iins));*/
	  t_regset def_regs = ARM_INS_REGS_DEF(iins);

	  if( !RegsetIsEmpty(RegsetIntersect(def,def_regs)))
	  {
	    if(ARM_INS_OPCODE(iins) == ARM_LDR && ARM_INS_REGB(iins) == ARM_REG_R13)
	    {

	    }
	    continue;
	  }

	  RegsetSetUnion(def,ARM_INS_REGS_USE(iins));
	  RegsetSetUnion(def,ARM_INS_REGS_DEF(iins));

	  /*	  printf("%d %d %d\n",ARM_INS_OPCODE(iins) == ARM_STR,ARM_INS_REGB(iins) == ARM_REG_R13,ArmInsWriteBackHappens(iins));*/
	  
	  if(ARM_INS_OPCODE(iins) == ARM_STR && ARM_INS_REGB(iins) == ARM_REG_R13 && ArmInsWriteBackHappens(iins))
	    {
	      /* opposite operation since we are running backwards */
	      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
		offset -= ARM_INS_IMMEDIATE(iins);
	      else
		offset += ARM_INS_IMMEDIATE(iins);
	    }
	  else if(ARM_INS_OPCODE(iins) == ARM_LDR && ARM_INS_REGB(iins) == ARM_REG_R13 && ArmInsWriteBackHappens(iins))
	    {
	      /* opposite operation since we are running backwards */

	      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
	      {
		offset -= ARM_INS_IMMEDIATE(iins);
	      }
	      else
	      {
		offset += ARM_INS_IMMEDIATE(iins);
	      }

	    }
	  else if(ARM_INS_OPCODE(iins) == ARM_SUB && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_NONE) 
	    {
	      /* opposite operation since we are running backwards */
	      offset += ARM_INS_IMMEDIATE(iins);
	    }
	  else if(ARM_INS_OPCODE(iins) == ARM_ADD && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_NONE) 
	    {
	      /* opposite operation since we are running backwards */
	      offset -= ARM_INS_IMMEDIATE(iins);
	    }
	  else if(ARM_INS_OPCODE(iins) == ARM_RSB && ARM_INS_REGA(iins) == ARM_REG_R13 && ARM_INS_REGC(iins) == ARM_REG_R13 && ARM_INS_REGB(iins) == ARM_REG_NONE) 
	    {
	      /* opposite operation since we are running backwards */
	      offset += ARM_INS_IMMEDIATE(iins);
	    }
	  else if (ARM_INS_OPCODE(iins)==ARM_LDR && ARM_INS_REGA(iins)==ARM_REG_R15 && ARM_INS_IMMEDIATE(iins)==4 && ARM_INS_REGB(iins) == ARM_REG_R13)
	    {
	      if (!ArmInsWriteBackHappens(iins))
		FATAL(("no writeback @I\n",iins));
	      if (!(ARM_INS_FLAGS(iins) & FL_DIRUP))
		FATAL(("dirup store @I\n",iins));
	      if (ARM_INS_FLAGS(iins) & FL_PREINDEX)
		FATAL(("no preindex @I\n",iins));
	      if( SavedRegOffset[ARM_REG_R15] != offset)
		{
		  SavedRegOffset[ARM_REG_R15] = -1;
		  continue;
		}
	      else
		{
		  RegsetSetAddReg(saved,ARM_REG_R15);
		}            
	      offset-=4;
	    }
	  else if (
		   ARM_INS_REGB(iins)==ARM_REG_R13 && 
		   ARM_INS_OPCODE(iins)==ARM_LDM &&
		   ARM_INS_FLAGS(iins) & FL_DIRUP &&
		   !(ARM_INS_FLAGS(iins) & FL_PREINDEX)
		   )
	    {
	      t_regset saved_regs = ARM_INS_IMMEDIATE(iins);
	      if (!ArmInsWriteBackHappens(iins))
		{
		  REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
		    SavedRegOffset[reg] = -1;
		  continue;
		  /*		  FATAL(("no writeback @I\n",iins));*/
		}
	     
	      REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
		{
		  offset -=4;
		  if( SavedRegOffset[reg] != offset )
		    {
		      /*		      printf("not saved %d - offset %d \n",reg,offset);*/
		      SavedRegOffset[reg] = -1;
		    }
		  else
		    {
		      /*		      printf("saved += %d\n",reg);*/
		      RegsetSetAddReg(saved,reg);
		    }            
		  /*		  offset -= 4;*/
		}
	    }
	  else if (
		   ARM_INS_REGB(iins)==ARM_REG_R11 && 
		   ARM_INS_OPCODE(iins)==ARM_LDM &&
		   !(ARM_INS_FLAGS(iins) & FL_DIRUP) &&
		   ARM_INS_FLAGS(iins) & FL_PREINDEX &&
		   RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R15) &&
		   RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13)
		   )
	    {
	      t_regset saved_regs = ARM_INS_IMMEDIATE(iins);
	      offset -=8;
	     
	      REGSET_FOREACH_REG_INVERSE(saved_regs,reg)
		{
		  if( SavedRegOffset[reg] != offset )
		    {
		      /*		      printf("not saved %d - offset %d \n",reg,offset);*/
		      SavedRegOffset[reg] = -1;
		    }
		  else
		    {
		      /*		      printf("saved += %d\n",reg);*/
		      RegsetSetAddReg(saved,reg);
		    }            
		  offset -= 4;
		}
	    }
	  else if ((ARM_INS_OPCODE(iins)==ARM_LDF) && RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13))
	    {
	      REGSET_FOREACH_REG(arm_description.all_registers,reg)
		{
		  saved = RegsetNew();
		  SavedRegOffset[reg] = -1;
		}
	    }
	  else if ((ARM_INS_OPCODE(iins)==ARM_STM) && RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13))
	    {
	      REGSET_FOREACH_REG(arm_description.all_registers,reg)
		{
		  saved = RegsetNew();
		  SavedRegOffset[reg] = -1;
		}
	    }
	  else if ((ARM_INS_OPCODE(iins)==ARM_BIC) && RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13))
	    {
	      REGSET_FOREACH_REG(arm_description.all_registers,reg)
		{
		  saved = RegsetNew();
		  SavedRegOffset[reg] = -1;
		}
	    }
	  else if (ARM_INS_OPCODE(iins) == ARM_LFM && ArmInsWriteBackHappens(iins))
	    {
	      t_uint32 local_offset = offset;
	      
	      if (!ArmInsWriteBackHappens(iins))
		FATAL(("no writeback @I\n",iins));
	      else
		{
		  t_regset saved_regs;
		  if (ARM_INS_FLAGS(iins) & FL_PREINDEX)
		    {
		      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
			offset -=4*ARM_INS_IMMEDIATE(iins);
		      else 
			offset +=4*ARM_INS_IMMEDIATE(iins);;
		    }
		  
		  local_offset = offset;
		  
		  saved_regs = ARM_INS_MULTIPLE(iins);
		  REGSET_FOREACH_REG(saved_regs,reg)
		    {
		      if( SavedRegOffset[reg] != offset )
			{
			  /*		      printf("not saved %d - offset %d \n",reg,offset);*/
			  SavedRegOffset[reg] = -1;
			}
		      else
			{
			  /*		      printf("saved += %d\n",reg);*/
			  RegsetSetAddReg(saved,reg);
			}          
		      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
			local_offset+=4;
		      else
			local_offset-=4;
		    }
		  
		  if (!(ARM_INS_FLAGS(iins) & FL_PREINDEX))
		    {
		      if (ARM_INS_FLAGS(iins) & FL_DIRUP)
			offset -=4*ARM_INS_IMMEDIATE(iins);
		      else 
			offset +=4*ARM_INS_IMMEDIATE(iins);
		    }
		}
	    }
	  else if ((ARM_INS_OPCODE(iins)==ARM_LDR)&&(ARM_INS_REGB(iins)==ARM_REG_R13))
	  {
	    VERBOSE(0,("aaaaaaaaaaaaaaaaaa @iB\n",ARM_INS_BBL(iins)));
	  }
	  else if (RegsetIn(ARM_INS_REGS_DEF(iins),ARM_REG_R13))
	    {
	      VERBOSE(0,("ins: @I\n bbl:@iB\n",iins,i_bbl));	
	      FATAL(("DEFINITION OF THE SP"));
	    }
        }

      /*      PrintRegset(stdout,FUNCTION_CFG(ifun)->sec->obj->description,saved);printf("\n");*/
      
      REGSET_FOREACH_REG(arm_description.all_registers,reg)
	{
	  if (RegsetIn(saved,reg)) continue;
	  SavedRegOffset[reg] = -1;
	}
    }
  
  
  if( count )
    {
      FUNCTION_SET_REGS_SAVED(ifun,  RegsetNew());
      /*      PrintRegset(stdout,FUNCTION_CFG(ifun)->sec->obj->description,FUNCTION_REGS_SAVED(ifun));*/
      
      REGSET_FOREACH_REG(arm_description.all_registers,reg)
        {
	  if( SavedRegOffset[reg]!=-1 )
	    FUNCTION_SET_REGS_SAVED(ifun, RegsetAddReg(FUNCTION_REGS_SAVED(ifun),reg));
        }
    }
  else
    {
      FUNCTION_SET_REGS_SAVED(ifun,  RegsetNew());
    }

  /* WIJZIGING*/

  /*  PrintRegset(stdout,FUNCTION_CFG(ifun)->sec->obj->description,FUNCTION_REGS_SAVED(ifun));*/

  /*  printf("saved before ");*/
  /*  PrintRegset(stdout,FUNCTION_CFG(ifun)->sec->obj->description,FUNCTION_REGS_SAVED(ifun));*/
  /* }}} */
#endif
#ifdef AGGRESSIVE_LIVENESS
  if (diabloanopt_options.rely_on_calling_conventions)
  {
    if (FUNCTION_FLAGS(ifun) & FF_IS_EXPORTED)
      {
	t_architecture_description *desc =  CFG_DESCRIPTION(FUNCTION_CFG(ifun));
	t_regset saved = FUNCTION_REGS_SAVED (ifun);
	RegsetSetUnion (saved,
	    RegsetDiff (desc->callee_saved, desc->callee_may_use));
	FUNCTION_SET_REGS_SAVED(ifun, saved);
      }
  /*  printf("saved after ");*/
  /*  PrintRegset(stdout,FUNCTION_CFG(ifun)->sec->obj->description,FUNCTION_REGS_SAVED(ifun));*/
  }
#ifdef CALC_SAVED_CHANGED
  else 
  {
    FunctionOnlySpills(ifun);
    ArmFunComputeStackSavedRegistersFromStackInfo(ifun);
    if(FUNCTION_STACK_INFO(ifun))
    {
    if(FUNCTION_STACK_INFO(ifun)->size)
      FreeStackSlots(FUNCTION_STACK_INFO(ifun));
    Free(FUNCTION_STACK_INFO(ifun));
    FUNCTION_SET_STACK_INFO(ifun,  NULL);
    }
  }
#endif
#endif
  return;
}
/*}}}*/


void ArmLookAtStack(t_cfg * cfg)
{
  t_function * i_fun;
  t_uint32 nr_funs = 0, nr_traceable = 0;


  
#ifdef DEBUG_STACKANALYSIS
  printf("Looking at stack!\n");
#endif
  CgBuild(cfg);

  CfgComputeLiveness(cfg,TRIVIAL); 
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE); 

  CFG_FOREACH_FUN(cfg,i_fun)
  {
    nr_funs++;
    FUNCTION_SET_FLAGS(i_fun,   FUNCTION_FLAGS(i_fun) | FF_STACKPOINTER_TRACEABLE);
  }

  CFG_FOREACH_FUN(cfg,i_fun)
    if(FUNCTION_BBL_FIRST(i_fun) && !BBL_IS_HELL(FUNCTION_BBL_FIRST(i_fun)))
    {
      if(FunctionOnlySpills(i_fun))
      {
	FUNCTION_SET_FLAGS(i_fun,   FUNCTION_FLAGS(i_fun) | FF_ONLY_SPILLS);
#ifdef DEBUG_STACKANALYSIS
	printf("The function uses the stack only to spill\n");
#endif
      }
      else
      {
	FUNCTION_SET_FLAGS(i_fun, FUNCTION_FLAGS(i_fun) & (~FF_ONLY_SPILLS));
      }
      if(FUNCTION_FLAGS(i_fun) & FF_STACKPOINTER_TRACEABLE) nr_traceable++;
    }

  ArmOptimizeStackOperations(cfg);
  ArmValidatePushPopRegsets(cfg);

  FreeStackInformation(cfg);

/*  VERBOSE(0,("Stackpointer was traceable in %f percent of the functions",nr_traceable*100.0/nr_funs));*/
}

#define ARM_REG_DIFFERENT		254
#define BBL_STACK_INFO(bbl)		(bbl->tmp)


t_bool FunctionOnlySpills(t_function * fun)/*{{{*/
{
  t_cfg * cfg=FUNCTION_CFG(fun);
  t_bbl * entry_bbl;
  t_bbl * ibbl;
  t_cfg_edge * iedge, *iedge2;
  t_arm_ins * i_ins, * last_arm_ins;
  t_equations eqs;
  t_equations eqs_after_split;
  t_equations tmp;
  t_bool ignore_condition, check_preds = FALSE;
  t_bool function_just_spills = TRUE;
  t_regset invalid_regs = RegsetIntersect(CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_may_change, CFG_DESCRIPTION(FUNCTION_CFG(fun))->int_registers);
  t_int32 highest_stack_value = 0, dummy=0;
  t_stack_info * stack = NULL;
  t_bool float_mem_operations = FALSE;
  t_bool writes_below_stack_pointer = FALSE;
  t_bool uses_frame_pointer = FALSE;

#ifdef DEBUG_STACKANALYSIS
  VERBOSE(0,("\nDoing the function %s",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"NoNAME"));
#endif

  FUNCTION_FOREACH_BBL(fun,ibbl)
  {
    BBL_FOREACH_PRED_EDGE(ibbl,iedge) if(CfgEdgeIsForwardInterproc(iedge) && CFG_EDGE_CAT(iedge) != ET_CALL) 
    {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(1,("No call: @E",iedge));
#endif
      check_preds = TRUE;
    }
		BBL_FOREACH_SUCC_EDGE(ibbl,iedge) if(CfgEdgeIsForwardInterproc(iedge) && CFG_EDGE_CAT(iedge) != ET_CALL)
		  {
			 VERBOSE(1,("Stack behavior of @F should not be analyzed because of @E",fun,iedge));
			 return FALSE;
		  }
  }

  if (BBL_PRED_FIRST(FUNCTION_BBL_LAST(fun))==NULL)
   {
	  VERBOSE(1,("Stack behavior of @F should not be analyzed because of disconnected return block",fun));
	  return FALSE;
	}

  /*{{{ Initialize*/
  FunctionUnmarkAllBbls(fun);

  eqs = EquationsNew(cfg);
  eqs_after_split = EquationsNew(cfg);
  tmp = EquationsNew(cfg);
  FUNCTION_FOREACH_BBL(fun,ibbl)
  {
    BBL_SET_EQS_IN(ibbl,  EquationsNew(cfg));
/*    BBL_FOREACH_PRED_EDGE(ibbl,iedge)*/
/*      if(!CfgEdgeIsForwardInterproc(iedge))*/
	EquationsSetAllTop(cfg,BBL_EQS_IN(ibbl));
/*    if(!iedge)*/
/*    {*/
/*      EquationsSetAllBot(BBL_EQS_IN(ibbl));*/
/*      EquationsAdd(BBL_EQS_IN(ibbl),ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,0,SYMBOLIC_STACK_POINTER,NULL);*/
/*      FunctionMarkBbl(fun,ibbl);*/
/*    }*/
  }

  entry_bbl = FUNCTION_BBL_FIRST(fun);
  EquationsSetAllBot(cfg,BBL_EQS_IN(entry_bbl));

  /* We add an equation which initializes the stackpointer to some identifiable value */
  EquationsAdd(cfg,BBL_EQS_IN(entry_bbl),ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,0,SYMBOLIC_STACK_POINTER,NULL);
  FunctionMarkBbl(fun,entry_bbl);
  /*}}}*/
#ifdef DEBUG_STACKANALYSIS
  VERBOSE(0,("Entry block = @B",entry_bbl));
#endif

  /* Now propagate the affine equations through the function, fixpoint stuff {{{*/
  while (FunctionUnmarkBbl(fun,&ibbl))
  {
    /*{{{ Quick check to see if stack pointer is zero when entering a function from an ip-edge that's not a call*/
    if(check_preds && ibbl != FUNCTION_BBL_FIRST(fun))
    {
      BBL_FOREACH_PRED_EDGE(ibbl,iedge2)
	if(CfgEdgeIsForwardInterproc(iedge2) && CFG_EDGE_CAT(iedge2) != ET_CALL) break;
      if(iedge2)
      {
	FATAL(("How can this ever happen? Single entry?"));
	if(EquationsRegsEqual(cfg,BBL_EQS_IN(ibbl),ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,0,SYMBOLIC_STACK_POINTER,NULL) != TRUE)
	{
/*          EquationsPrint(BBL_EQS_IN(ibbl));*/
	  VERBOSE(1,("On entry via ip edge is stack pointer not zero! @E",iedge2));
	}
      }
    }
    /*}}}*/
    ignore_condition = FALSE;
#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("Doing @iB",ibbl));
#endif
    EquationsCopy(cfg,BBL_EQS_IN(ibbl),eqs);
    last_arm_ins = ArmGetLastConditionalInsFromBlockWherePropagationSplits(ibbl);
    BBL_FOREACH_ARM_INS(ibbl,i_ins)
    {
#ifdef DEBUG_STACKANALYSIS
/*      VERBOSE(0,("Doing @I",i_ins));*/
#endif
      if(last_arm_ins == i_ins)
      {
#ifdef DEBUG_STACKANALYSIS
/*        VERBOSE(0,("Splitting state!\n"));*/
#endif
	EquationsCopy(cfg,eqs,eqs_after_split);
	ignore_condition = TRUE;
      }
      ArmCopyInstructionPropagator(i_ins,eqs,ignore_condition);
      if(ARM_INS_TYPE(i_ins) == IT_FLT_LOAD || ARM_INS_TYPE(i_ins) == IT_FLT_STORE)
	float_mem_operations = TRUE;
      /* bug in libgcc gcc 4.3.6, fixed by http://gcc.gnu.org/ml/gcc-patches/2009-06/msg00372.html */
      if ((ARM_INS_OPCODE(i_ins)==ARM_STR) &&
          (ARM_INS_REGB(i_ins)==ARM_REG_R13) &&
          ((ARM_INS_FLAGS(i_ins) & (FL_IMMED|FL_DIRUP|FL_WRITEBACK|FL_PREINDEX)) == (FL_IMMED|FL_PREINDEX)) &&
          (ARM_INS_IMMEDIATE(i_ins) > 0))
        writes_below_stack_pointer = TRUE;
      if (ARM_INS_OPCODE(i_ins)==ARM_SUB && ARM_INS_REGA(i_ins)==ARM_REG_R13 && ARM_INS_REGB(i_ins)==ARM_REG_R11)
        uses_frame_pointer = TRUE;
      if (ARM_INS_OPCODE(i_ins)==ARM_MOV && ARM_INS_REGA(i_ins)==ARM_REG_R13 && ARM_INS_REGC(i_ins)==ARM_REG_R11)
        uses_frame_pointer = TRUE;

      if(EquationsRegsDifferWithTagAllowed(cfg,eqs,ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,&dummy,SYMBOLIC_STACK_POINTER) == YES)
      {
	if(dummy < highest_stack_value) 
	{
	  highest_stack_value = dummy;
#ifdef DEBUG_STACKANALYSIS
          VERBOSE(0,("Found higher stack value: %d",-highest_stack_value));
#endif
	}
      }
      else
      {
	function_just_spills = FALSE;
      }
#ifdef DEBUG_STACKANALYSIS
/*      EquationsPrint(eqs);*/
#endif
    }

#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("After the bbl:"));
    //EquationsPrint(eqs);
#endif
    /*
    if(EquationsRegsDifferWithTagAllowed(eqs,ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,&dummy,SYMBOLIC_STACK_POINTER) != YES)
    {
      VERBOSE(0,("We have lost the stackpointer after @B!\n", ibbl));
      function_just_spills = FALSE;
      BBL_SET_HAS_STACK_COPIES(ibbl,  STACK_ARGS);
    }
    else */
    /*
    {
      if(EquationsRegsDiffer(eqs,ARM_REG_R13,ARM_REG_R0,&dummy) == YES
	|| EquationsRegsDiffer(eqs,ARM_REG_R13,ARM_REG_R1,&dummy) == YES
	|| EquationsRegsDiffer(eqs,ARM_REG_R13,ARM_REG_R2,&dummy) == YES
	|| EquationsRegsDiffer(eqs,ARM_REG_R13,ARM_REG_R3,&dummy) == YES)
      BBL_SET_HAS_STACK_COPIES(ibbl,  STACK_ARGS);
    }*/
    

    /*{{{ Now propagate over the edges */
    BBL_FOREACH_SUCC_EDGE(ibbl,iedge)
    {
      if(!CfgEdgeIsInterproc(iedge))
      {
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("Propping over @E",iedge));
#endif
	if(CFG_EDGE_CAT(iedge) == ET_FALLTHROUGH && last_arm_ins)
	{
	  if(EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(iedge)),eqs_after_split))
	    FunctionMarkBbl(fun,CFG_EDGE_TAIL(iedge));
	}
	else
	{
	  if(EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(iedge)),eqs))
	    FunctionMarkBbl(fun,CFG_EDGE_TAIL(iedge));
	}
      }
      else if (CfgEdgeIsForwardInterproc(iedge))
      {
/*        if(BBL_HAS_STACK_COPIES(ibbl)) FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(iedge))) |= FF_MAY_HAVE_STACK_ARGUMENT;*/
	if(CFG_EDGE_CORR(iedge))
	{
	  t_reg tmp_reg;
	  if(BBL_FUNCTION(CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge))) != fun)
          {
#ifdef DEBUG_STACKANALYSIS
            /* */
            VERBOSE(0,("call does not return to function @E - @E",iedge,CFG_EDGE_CORR(iedge)));
#endif
            FUNCTION_SET_FLAGS(fun,   FUNCTION_FLAGS(fun) & (~FF_STACKPOINTER_TRACEABLE));
            function_just_spills = FALSE;
            goto cleanup;
          }

	  EquationsCopy(cfg,eqs,tmp);
	  REGSET_FOREACH_REG(invalid_regs,tmp_reg)
	    EquationsInvalidate(cfg,tmp,tmp_reg);

#ifdef DEBUG_STACKANALYSIS
	  VERBOSE(0,("Propping over @E",iedge));
#endif
	  if(EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge))),tmp))
	    FunctionMarkBbl(fun,CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge)));
	}
	
      }
    }
    /*}}}*/
    
  }/*}}}*/

#ifdef DEBUG_COPY_ANALYSIS
  FUNCTION_FOREACH_BBL(fun,ibbl)
  {
    if(BBL_PRED_FIRST(ibbl) && BBL_EQS_IN(ibbl)[0].rega == REG_TOP)
    {
      FunctionDrawGraph(fun,FUNCTION_NAME(fun));
      FATAL(("Still top for @eB",ibbl));
    }
  }
#endif
    
  /* If a function uses float mem operations, we currently keep our hands of the function.
   * TODO: add code that correctly emulates the float mem operations and use the info here */
  if(float_mem_operations)
  {
    FUNCTION_SET_FLAGS(fun,   FUNCTION_FLAGS(fun) & (~FF_STACKPOINTER_TRACEABLE));
    goto cleanup;
  }
  if (writes_below_stack_pointer)
  {
    FUNCTION_SET_FLAGS(fun,   FUNCTION_FLAGS(fun) & (~FF_STACKPOINTER_TRACEABLE));
    goto cleanup;
  }
  /* if function_just_spills is set to false, then the stackpointer was lost in the above fixpoint algorithm.
   * We don't know the maximal stack depth, so let's clean everything and return. */
  if(!function_just_spills)
  {
    FUNCTION_SET_FLAGS(fun,   FUNCTION_FLAGS(fun) & ~FF_STACKPOINTER_TRACEABLE);
    goto cleanup;
  }

  if((ibbl = FunctionGetExitBlock(fun)))
  {
    t_int32 dif;
    if(YES == EquationsRegsDifferWithTagAllowed(cfg,BBL_EQS_IN(ibbl),ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,&dif,SYMBOLIC_STACK_POINTER))
    {
      if(dif)
      {
	function_just_spills = FALSE;
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("Stackpointer is known but not restored! %d\n",dif));
#endif
	goto cleanup;
      }
    }
    else	
    {
      function_just_spills = FALSE;
      FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) & (~FF_STACKPOINTER_TRACEABLE));
/*      EquationsPrint(BBL_EQS_IN(ibbl));*/
      goto cleanup;
    }
  }
  else
  {
/*    function_just_spills = FALSE;*/
#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("No exit block in fun!\n"));
#endif
/*    goto cleanup;*/
  }


  /* the following check is probably overkill, but otherwise we get fatals during the 
	* copy analysis for cases we apparently don't handle (i.e., in spec2006 gcc in
   * the function synth_mult */
  if (uses_frame_pointer)
  {
    VERBOSE(1,("STACK ANALYSIS of @F skipped because of frame pointer usage ...",fun));
    FUNCTION_SET_FLAGS(fun,   FUNCTION_FLAGS(fun) & (~FF_STACKPOINTER_TRACEABLE));
    goto cleanup;
  }

  /* When we get here, we know that the stackpointer could be traced throughout the function,
   * so we can now investigate the function to set some flags. */
#ifdef DEBUG_STACKANALYSIS
  printf("Stackpointer is known and restored!\n");
  VERBOSE(0,("Maximal stack size was %d\n",-(highest_stack_value)));
#endif
  if(highest_stack_value < 0)
  {
    stack = NewStackInfo(-highest_stack_value);
    FUNCTION_FOREACH_BBL(fun,ibbl)
    {
/*      t_bbl * jbbl; */
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Doing @B",ibbl));fflush(stdout);
#endif
/*      FUNCTION_FOREACH_BBL(fun,jbbl)*/
/*        CheckEquations(jbbl);*/
      if(!BBL_PRED_FIRST(ibbl)) continue; /* This block is unreachable, but not yet removed from graph */

      ignore_condition = FALSE;
      EquationsCopy(cfg,BBL_EQS_IN(ibbl),eqs);
      /*    EquationsPrint(eqs);*/
      last_arm_ins = ArmGetLastConditionalInsFromBlockWherePropagationSplits(ibbl);

      /*    VERBOSE(0,("Analyzing @iB",ibbl));*/
      BBL_FOREACH_ARM_INS(ibbl,i_ins)
      {
	t_reg base_reg;
	t_int32 offset=0,stackoffset=0;

	if(EquationsRegsDifferWithTagAllowed(cfg,eqs,ARM_REG_R13,CFG_DESCRIPTION(cfg)->num_int_regs,&stackoffset,SYMBOLIC_STACK_POINTER) != YES) 
          {
            if (RegsetIn(BblRegsLiveBefore(ibbl),
                         ARM_REG_R13)
                )
              FATAL(("Stack should be known at @I in @iB!",i_ins,ibbl));
            else 
              DEBUG(("used to fatal but not needed anymore ..."));
          }
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("StackOffset: %d",-stackoffset));
#endif
	if(ARM_INS_IS_MEMORY(i_ins))
	{
	  base_reg = ARM_INS_REGB(i_ins);
	  /*        VERBOSE(0,("Memory ins uses %s as base register.\n",ArmRegisterName(base_reg)));*/
	  if(base_reg == ARM_REG_R13 || EquationsRegsDiffer(cfg,eqs,ARM_REG_R13,base_reg,&offset) == YES)
	  {
	    if(!(stackoffset > 0))
	    {
	      offset=stackoffset - offset;
#ifdef DEBUG_STACKANALYSIS
	      VERBOSE(0,("Offset: %d",-offset));
#endif
	      /*            if(offset > 0) FATAL(("Access in upperstackframe! offset : %d @I",offset,i_ins));*/

	      ArmSaveStackInformationForMemory(stack,i_ins,eqs,offset,stackoffset);

	      /* Ok, we are accessing the stack. Let's see what happens and eventually disable the function_only_spills flag */
	      if(ArmInsSpillsToStack(i_ins))
	      {
	      }
	      else
	      {
		function_just_spills = FALSE;
#ifdef DEBUG_STACKANALYSIS
		VERBOSE(0,("This instruction is not a spill instruction: @I\n",i_ins));
#endif


		/*              VERBOSE(0,("Accessing stack at offset %d.%s@I\n",stackoffset,(stackoffset>0)?" !out the stackframe!\n":"\n",i_ins));*/
		/*              VERBOSE(0,("base register : %s\n\n",ArmRegisterName(base_reg)));*/
	      }
	    }
	  }
	}
	else if(RegsetIn(ARM_INS_REGS_DEF(i_ins),ARM_REG_R13))
	{
          if (!ArmSaveStackInformationForDataproc(stack,i_ins,eqs,stackoffset))
            {
              function_just_spills = FALSE;
            }
	}
	else if(RegsetIn(ARM_INS_REGS_USE(i_ins),ARM_REG_R13) && ARM_INS_TYPE(i_ins) != IT_SWI)
	{
	  ArmSaveStackInformationForStackUse(stack,i_ins,eqs,stackoffset);
	}
	

	if(last_arm_ins && last_arm_ins == i_ins)
	  ignore_condition = TRUE;

	ArmCopyInstructionPropagator(i_ins,eqs,ignore_condition);
      }
    }
/*    StackInfoPrint(stack,FALSE);*/
    FUNCTION_SET_STACK_INFO(fun,  stack);
  }
  else 
  {
    function_just_spills = FALSE;
#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("In fun %s the stack is never used explicitly",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"NoName"));
#endif
  }


  
  
cleanup:
  FUNCTION_FOREACH_BBL(fun,ibbl)
  {
    Free(BBL_EQS_IN(ibbl));
    BBL_SET_EQS_IN(ibbl,  NULL);
  }
  Free(eqs);
  Free(tmp);
  Free(eqs_after_split);
    

  return function_just_spills;
}/*}}}*/


t_bool ArmInsSpillsToStack(t_arm_ins * ins)/*{{{*/
{
  if(!ARM_INS_IS_MEMORY(ins)) return FALSE;
  if(!RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R13)) return FALSE;
  if(ARM_INS_OPCODE(ins) == ARM_LDM && ArmInsWriteBackHappens(ins) && (ARM_INS_FLAGS(ins) & FL_DIRUP) && !(ARM_INS_FLAGS(ins) & FL_PREINDEX)) return TRUE;
  if(ARM_INS_OPCODE(ins) == ARM_LDR && ArmInsWriteBackHappens(ins) && (ARM_INS_FLAGS(ins) & FL_DIRUP) && !(ARM_INS_FLAGS(ins) & FL_PREINDEX)) return TRUE;
  if(ARM_INS_OPCODE(ins) == ARM_STM && ArmInsWriteBackHappens(ins) && !(ARM_INS_FLAGS(ins) & FL_DIRUP) && (ARM_INS_FLAGS(ins) & FL_PREINDEX)) return TRUE;
  if(ARM_INS_OPCODE(ins) == ARM_STR && ArmInsWriteBackHappens(ins) && !(ARM_INS_FLAGS(ins) & FL_DIRUP) && (ARM_INS_FLAGS(ins) & FL_PREINDEX)) return TRUE;
  return FALSE;
}/*}}}*/

void ArmOptimizeStackOperations(t_cfg * cfg)/*{{{*/
{
  t_bbl * i_bbl, *exit_bbl;
  t_arm_ins * i_ins;
  t_function * fun;
  t_regset unused_regs;
  t_uint32 i;
  t_stack_info * stack;
  t_inslist * iter;
  t_reg ireg;
  t_bool redo_liveness = TRUE;
  static t_uint32 tellerke = 0;
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);

  STATUS(START, ("ArmOptimizeStack"));
  CFG_FOREACH_FUN(cfg, fun)
  {
    if(!(FUNCTION_FLAGS(fun) & FF_STACKPOINTER_TRACEABLE)) continue;
    if(FUNCTION_FLAGS(fun) & FF_DONT_CHANGE_STACK) continue;
    if(!(FUNCTION_FLAGS(fun) & FF_ONLY_SPILLS)) continue;

#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("\nStack opt function %s",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"No name"));
#endif

    if(!FUNCTION_STACK_INFO(fun)) continue;// FATAL(("No information available!"));
    stack = FUNCTION_STACK_INFO(fun);

#ifdef DEBUG_STACKANALYSIS
    StackInfoPrint(FUNCTION_STACK_INFO(fun),FALSE);
#endif

    {
      t_bool dont_do_it = FALSE;
      for(i = 1; i < stack->size; i++)
      {
	if(stack->stack[i].savers && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))))
	{
	  t_arm_ins * i_ins = T_ARM_INS(INS_LIST_INS(stack->stack[i].savers));
	  t_inslist * ins_next = INS_LIST_NEXT(stack->stack[i].savers);
	  while(ins_next)
	  {
	    if(ARM_INS_OPCODE(i_ins) != ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(ins_next))))
	    {
	      dont_do_it = TRUE;
	      break;
	    }
	    if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(ins_next))) == ARM_STM && ARM_INS_IMMEDIATE(i_ins) != ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(ins_next))))
	    {
	      dont_do_it = TRUE;
	      break;
	    }
	    ins_next = INS_LIST_NEXT(ins_next);  
	  }
	  if(dont_do_it == TRUE) break;
	}
      }
      /* Skip to next function */
      if(dont_do_it)
      {
/*        VERBOSE(0,("1:Skipping %s because of a weird stack layout",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"NONAME"));*/
/*        StackInfoPrint(stack,TRUE);*/
	continue;
      }
    }

    for(i = 1; i < stack->size; i++)
    {
      /* Have we saved a dead register on the stack? */
      if(stack->stack[i].savers && !(stack->stack[i].live) && stack->stack[i].saved != ARM_REG_DIFFERENT && stack->stack[i].saved != ARM_REG_NONE)
      {
	/* Don't do it when there are alloc instruction that alloc this stack slot. It is pretty strange
	 * and therefore we keep our hands of this stack slot */
#ifdef DEBUG_STACKANALYSIS
        if(!(diablosupport_options.debugcounter <= tellerke))
#endif
	if(!(stack->stack[i].allocers))
        if(ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))) && RegsetIn(CFG_DESCRIPTION(cfg)->callee_saved,stack->stack[i].saved))
	{
	  VERBOSE(VERBOSE_STACKANALYSIS,("Gotcha"));
	  tellerke++;

	  /* Change all relevant instructions, so that the dead register does not get stored or loaded anymore */
	  /*{{{*/
	  iter = (t_inslist*)stack->stack[i].savers;
	  while(iter)
	  {
	    VERBOSE(VERBOSE_STACKANALYSIS,("Before @I",INS_LIST_INS(iter)));
	    if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STR) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
	    else if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STM)
	    {
	      ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))& ~(0x1<<stack->stack[i].saved));
	      if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
	    }
	    else FATAL(("No save!"));
	    ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    VERBOSE(VERBOSE_STACKANALYSIS,("After  @I",INS_LIST_INS(iter)));
	    iter = INS_LIST_NEXT(iter);
	  }
	  iter = (t_inslist*)stack->stack[i].loaders;
	  while(iter)
	  {
	    VERBOSE(VERBOSE_STACKANALYSIS,("Before @I",INS_LIST_INS(iter)));
	    if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDR) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
	    else if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDM)
	    {
	      ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))& ~(0x1<<stack->stack[i].loaded));
	      if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
	    }
	    else FATAL(("No load!"));
	    ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    VERBOSE(VERBOSE_STACKANALYSIS,("After  @I",INS_LIST_INS(iter)));
	    iter = INS_LIST_NEXT(iter);
	  }
	  iter = (t_inslist*)stack->stack[i].deallocers;
	  while(iter)
	  {
	    VERBOSE(VERBOSE_STACKANALYSIS,("Before @I",INS_LIST_INS(iter)));
	    if(ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter))) < 4) FATAL(("Strange immediate"));
	    ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))- 4);
	    if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
	    ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
	    VERBOSE(VERBOSE_STACKANALYSIS,("After  @I",INS_LIST_INS(iter)));
	    iter = INS_LIST_NEXT(iter);
	  }
	  iter = (t_inslist*)stack->stack[i].out_frame;
	  while(iter)
	  {
	    t_arm_ins * curr_ins = T_ARM_INS(INS_LIST_INS(iter));
	    t_int32 imm = ARM_INS_IMMEDIATE(curr_ins);
	    VERBOSE(VERBOSE_STACKANALYSIS,("OUTFRAMERS:Before @I",curr_ins));
	    if(ArmInsWriteBackHappens(curr_ins)) FATAL(("Writeback happens @I",curr_ins));
/*            if(ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter))) < 4) FATAL(("Strange immediate @I",T_ARM_INS(INS_LIST_INS(iter))));*/
	    if(ARM_INS_OPCODE(curr_ins) != ARM_LDR && ARM_INS_OPCODE(curr_ins) != ARM_ADD && ARM_INS_OPCODE(curr_ins) != ARM_STR
		&& ARM_INS_OPCODE(curr_ins) != ARM_LDRH && ARM_INS_OPCODE(curr_ins) != ARM_LDRB
		&& ARM_INS_OPCODE(curr_ins) != ARM_STRH && ARM_INS_OPCODE(curr_ins) != ARM_STRB
		&& ARM_INS_OPCODE(curr_ins) != ARM_LDRSB && ARM_INS_OPCODE(curr_ins) != ARM_LDRSH)
	    {
	      FunctionDrawGraph(fun,"fatal.dot");
	      FATAL(("Strange opcode!"));
	    }
	    imm -=4;
	    if(ArmInsIsEncodableConstantForOpcode((imm>0)?imm:-imm,ARM_INS_OPCODE(curr_ins), ARM_INS_FLAGS(curr_ins) & FL_THUMB) == FALSE)
	    {
	      t_arm_ins * insert;

        if(ARM_INS_TYPE(curr_ins) == IT_LOAD || ARM_INS_TYPE(curr_ins) == IT_STORE) FATAL(("Implement undo! %d",imm));

	      insert = ArmInsNewForBbl(ARM_INS_BBL(curr_ins));
	      ArmInsInsertAfter(insert,curr_ins);

              if (ARM_INS_FLAGS(curr_ins) & FL_THUMB)
                ARM_INS_SET_FLAGS(insert, ARM_INS_FLAGS(insert) | FL_THUMB);
	      ArmInsMakeSub(insert,ARM_INS_REGA(curr_ins),ARM_INS_REGA(curr_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(curr_ins));
	    }
	    else
	    {
	      if(imm > 0)
		ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))- 4);
	      else
	      {
		ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)),  -(imm));
		if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDR || ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STR ||
		    ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDRB || ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STRB || 
		    ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDRH || ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STRH ||
		    ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDRSB || ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDRSB)
		  ARM_INS_SET_FLAGS(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_FLAGS(T_ARM_INS(INS_LIST_INS(iter)))& ~FL_DIRUP);
		else if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_ADD)
                {
		  ARM_INS_SET_OPCODE(T_ARM_INS(INS_LIST_INS(iter)),  ARM_SUB);
                  if (!ArmInsIsEncodable(T_ARM_INS(INS_LIST_INS(iter))))
                        ARM_INS_SET_OPCODE(T_ARM_INS(INS_LIST_INS(iter)), ARM_ADD);
                }
		else FATAL(("Implement"));
	      }
              ASSERT(ArmInsIsEncodable(T_ARM_INS(INS_LIST_INS(iter))), ("instruction can't be encoded anymore @I", T_ARM_INS(INS_LIST_INS(iter))));
	    }
	    VERBOSE(VERBOSE_STACKANALYSIS,("OUTFRAMERS:After  @I",INS_LIST_INS(iter)));
	    iter = INS_LIST_NEXT(iter);
	  }
	  if(stack->stack[i].allocers)
	  {
	    StackInfoPrint(stack,TRUE);
	    FunctionDrawGraph(fun,"fatal.dot");
	    FATAL(("Didn't expected allocer here [%d]! @I",i,INS_LIST_INS(stack->stack[i].allocers)));
	  }
	  /*}}}*/

	  /* Clean up, so that we know this register is not spilled anymore {{{*/
	  InsListCleanup((t_inslist**)&(stack->stack[i].savers));
	  InsListCleanup((t_inslist**)&(stack->stack[i].loaders));
	  InsListCleanup((t_inslist**)&(stack->stack[i].allocers));
	  InsListCleanup((t_inslist**)&(stack->stack[i].deallocers));
	  InsListCleanup((t_inslist**)&(stack->stack[i].out_frame));
	  stack->stack[i].saved = ARM_REG_NONE;
	  stack->stack[i].loaded = ARM_REG_NONE;
	  /*}}}*/
	}
      }
    }


    if(0 && !diabloanopt_options.rely_on_calling_conventions && !FunctionIsReentrant(fun))
    {
      if(redo_liveness)
      {
	CfgComputeLiveness(cfg,CONTEXT_SENSITIVE); 
	redo_liveness = FALSE;
      }
      exit_bbl = FunctionGetExitBlock(fun);
      if(exit_bbl)
      {
	unused_regs = RegsetNewInvers(BblRegsLiveBefore(exit_bbl), desc->all_registers);
	FUNCTION_FOREACH_BBL(fun,i_bbl)
	{
	  BBL_FOREACH_ARM_INS(i_bbl,i_ins)
	  {
	    if(!ArmInsSpillsToStack(i_ins))
	    {
	      RegsetSetDiff(unused_regs,ARM_INS_REGS_DEF(i_ins));
	      RegsetSetDiff(unused_regs,ARM_INS_REGS_USE(i_ins));
	    }
	    else
	      RegsetSetDiff(unused_regs,ArmInsRegsLiveAfter(i_ins));
	  }

	}
	RegsetSetIntersect(unused_regs,BBL_DESCRIPTION(exit_bbl)->int_registers);
	RegsetSetDiff(unused_regs,BBL_DESCRIPTION(exit_bbl)->callee_may_change);
	RegsetSetSubReg(unused_regs, ARM_REG_R15);
	RegsetSetSubReg(unused_regs, ARM_REG_R13);

	if(!RegsetIsEmpty(unused_regs))
	{
	  /*      DiabloPrintArch(stdout,BBL_DESCRIPTION(exit_bbl),"Dead regs: @A\nUsed regs: @A\nChan regs: @A\nUnus regs: @A",RegsetNewInvers(BblRegsLiveBefore(exit_bbl)),FUNCTION_REGS_USED(BBL_FUNCTION(exit_bbl)),FUNCTION_REGS_CHANGED(BBL_FUNCTION(exit_bbl)),unused_regs);*/
	  /*      DiabloPrintArch(stdout,BBL_DESCRIPTION(exit_bbl),"Unus regs: @A",unused_regs);*/

	  /* We first try to eliminate the spills of normal registers, i.e. not R14, the linkregister */
	  for(i = 1; (i < stack->size) && (!RegsetIsEmpty(unused_regs)); i++)
	  {
	    if(stack->stack[i].savers && stack->stack[i].live && stack->stack[i].saved != ARM_REG_DIFFERENT
		&& stack->stack[i].saved != ARM_REG_NONE && stack->stack[i].saved != ARM_REG_R14 && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))) && RegsetIn(CFG_DESCRIPTION(cfg)->callee_saved,stack->stack[i].saved))
	    {
	      if(RegsetIn(ArmInsRegsLiveAfter(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))),stack->stack[i].live)) continue;
#ifdef DEBUG_STACKANALYSIS
	      if(diablosupport_options.debugcounter <= tellerke) continue;
#endif
/*              DiabloPrintArch(stdout,BBL_DESCRIPTION(exit_bbl),"Unus regs: @A, @I\n",unused_regs,INS_LIST_INS(stack->stack[i].savers));*/
	      REGSET_FOREACH_REG(unused_regs,ireg) break;
	      RegsetSetSubReg(unused_regs,ireg);

	      redo_liveness = TRUE;
	      tellerke++;

	      iter = (t_inslist*)stack->stack[i].savers;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_STR)
		{
		  ArmInsMakeMov(T_ARM_INS(INS_LIST_INS(iter)),ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		}
		else
		{
		  ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))& ~(0x1<<stack->stack[i].saved));
		  if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeMov(T_ARM_INS(INS_LIST_INS(iter)),ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		  else
		  {
		    t_arm_ins * insert_arm_ins = ArmInsNewForBbl(INS_BBL(INS_LIST_INS(iter)));
		    ArmInsInsertAfter(insert_arm_ins,T_ARM_INS(INS_LIST_INS(iter)));
                    if (ARM_INS_FLAGS(T_ARM_INS(INS_LIST_INS(iter))) & FL_THUMB)
                        ARM_INS_SET_FLAGS(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_FLAGS(T_ARM_INS(INS_LIST_INS(iter))) | FL_THUMB);
		    ArmInsMakeMov(insert_arm_ins,ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		    VERBOSE(0,("Added @I",insert_arm_ins));
		  }
		}
		ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      iter = (t_inslist*)stack->stack[i].loaders;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_OPCODE(T_ARM_INS(INS_LIST_INS(iter))) == ARM_LDR) ArmInsMakeMov(T_ARM_INS(INS_LIST_INS(iter)),stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		else
		{
		  ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))& ~(0x1<<stack->stack[i].loaded));
		  if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeMov(T_ARM_INS(INS_LIST_INS(iter)),stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		  else
		  {
		    t_arm_ins * insert_arm_ins = ArmInsNewForBbl(INS_BBL(INS_LIST_INS(iter)));
		    if(stack->stack[i].loaded != ARM_REG_R15)
		      ArmInsInsertBefore(insert_arm_ins,T_ARM_INS(INS_LIST_INS(iter)));
		    else
		      ArmInsInsertAfter(insert_arm_ins,T_ARM_INS(INS_LIST_INS(iter)));
                    if (ARM_INS_FLAGS(T_ARM_INS(INS_LIST_INS(iter))) & FL_THUMB)
                        ARM_INS_SET_FLAGS(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_FLAGS(T_ARM_INS(INS_LIST_INS(iter))) | FL_THUMB);
		    ArmInsMakeMov(insert_arm_ins,stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(T_ARM_INS(INS_LIST_INS(iter))));
		    VERBOSE(0,("Added @I",insert_arm_ins));
		  }
		}
		ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      iter = (t_inslist*)stack->stack[i].deallocers;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter))) < 4) FATAL(("Strange immediate"));
		ARM_INS_SET_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)), ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))- 4);
		if(!ARM_INS_IMMEDIATE(T_ARM_INS(INS_LIST_INS(iter)))) ArmInsMakeNoop(T_ARM_INS(INS_LIST_INS(iter)));
		ARM_INS_SET_REGS_USE(T_ARM_INS(INS_LIST_INS(iter)),  ArmUsedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		ARM_INS_SET_REGS_DEF(T_ARM_INS(INS_LIST_INS(iter)),  ArmDefinedRegisters(T_ARM_INS(INS_LIST_INS(iter))));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      if(stack->stack[i].allocers) FATAL(("Didn't expected allocer here!"));
	      FunctionReplaceRegister(fun,stack->stack[i].saved,ireg);

	      /* Clean up, so that we know this registers in not spilled anymore {{{*/
	      InsListCleanup((t_inslist**)&(stack->stack[i].savers));
	      InsListCleanup((t_inslist**)&(stack->stack[i].loaders));
	      InsListCleanup((t_inslist**)&(stack->stack[i].allocers));
	      InsListCleanup((t_inslist**)&(stack->stack[i].deallocers));
	      InsListCleanup((t_inslist**)&(stack->stack[i].out_frame));
	      stack->stack[i].saved = ARM_REG_NONE;
	      stack->stack[i].loaded = ARM_REG_NONE;
	      /*}}}*/

	    }

	  }
#if 0
	  /* Now we also try not to spill the link register */
	  for(i = 1; (i < stack->size) && (!RegsetIsEmpty(unused_regs)); i++)
	  {
	    if(stack->stack[i].savers && stack->stack[i].live && stack->stack[i].saved != ARM_REG_DIFFERENT && stack->stack[i].saved != ARM_REG_NONE 
		&& ArmInsSpillsToStack(INS_LIST_INS(stack->stack[i].savers)) && RegsetIn(CFG_DESCRIPTION(cfg)->callee_saved,stack->stack[i].saved))
	    {
#ifdef DEBUG_STACKANALYSIS
	      if(diablosupport_options.debugcounter <= tellerke) break;
#endif
/*              DiabloPrintArch(stdout,BBL_DESCRIPTION(exit_bbl),"Unus regs: @A, @I\n",unused_regs,INS_LIST_INS(stack->stack[i].savers));*/
	      REGSET_FOREACH_REG(unused_regs,ireg) break;
	      RegsetSetSubReg(unused_regs,ireg);

	      redo_liveness = TRUE;
	      tellerke++;

	      iter = (t_inslist*)stack->stack[i].savers;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_OPCODE(INS_LIST_INS(iter)) == ARM_STR) ArmInsMakeMov(INS_LIST_INS(iter),ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		else
		{
		  ARM_INS_SET_IMMEDIATE(INS_LIST_INS(iter), ARM_INS_IMMEDIATE(INS_LIST_INS(iter))& ~(0x1<<stack->stack[i].saved));
		  if(!ARM_INS_IMMEDIATE(INS_LIST_INS(iter))) ArmInsMakeMov(INS_LIST_INS(iter),ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		  else
		  {
		    t_arm_ins * insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(INS_LIST_INS(iter)));
		    ArmInsInsertAfter(insert_arm_ins,INS_LIST_INS(iter));
		    ArmInsMakeMov(insert_arm_ins,ireg,stack->stack[i].saved,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		    VERBOSE(0,("Added @I",insert_arm_ins));
		  }
		}
		ARM_INS_SET_REGS_USE(INS_LIST_INS(iter),  ArmUsedRegisters(INS_LIST_INS(iter)));
		ARM_INS_SET_REGS_DEF(INS_LIST_INS(iter),  ArmDefinedRegisters(INS_LIST_INS(iter)));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      iter = (t_inslist*)stack->stack[i].loaders;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_OPCODE(INS_LIST_INS(iter)) == ARM_LDR) ArmInsMakeMov(INS_LIST_INS(iter),stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		else
		{
		  ARM_INS_SET_IMMEDIATE(INS_LIST_INS(iter), ARM_INS_IMMEDIATE(INS_LIST_INS(iter))& ~(0x1<<stack->stack[i].loaded));
		  if(!ARM_INS_IMMEDIATE(INS_LIST_INS(iter))) ArmInsMakeMov(INS_LIST_INS(iter),stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		  else
		  {
		    t_arm_ins * insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(INS_LIST_INS(iter)));
		    if(stack->stack[i].loaded != ARM_REG_R15)
		      ArmInsInsertBefore(insert_arm_ins,INS_LIST_INS(iter));
		    else
		      ArmInsInsertAfter(insert_arm_ins,INS_LIST_INS(iter));
		    ArmInsMakeMov(insert_arm_ins,stack->stack[i].loaded,ireg,0,ARM_INS_CONDITION(INS_LIST_INS(iter)));
		    VERBOSE(0,("Added @I",insert_arm_ins));
		  }
		}
		ARM_INS_SET_REGS_USE(INS_LIST_INS(iter),  ArmUsedRegisters(INS_LIST_INS(iter)));
		ARM_INS_SET_REGS_DEF(INS_LIST_INS(iter),  ArmDefinedRegisters(INS_LIST_INS(iter)));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      iter = (t_inslist*)stack->stack[i].deallocers;
	      while(iter)
	      {
		VERBOSE(0,("Before @I",INS_LIST_INS(iter)));
		if(ARM_INS_IMMEDIATE(INS_LIST_INS(iter)) < 4) FATAL(("Strange immediate"));
		ARM_INS_SET_IMMEDIATE(INS_LIST_INS(iter), ARM_INS_IMMEDIATE(INS_LIST_INS(iter))- 4);
		if(!ARM_INS_IMMEDIATE(INS_LIST_INS(iter))) ArmInsMakeNoop(INS_LIST_INS(iter));
		ARM_INS_SET_REGS_USE(INS_LIST_INS(iter),  ArmUsedRegisters(INS_LIST_INS(iter)));
		ARM_INS_SET_REGS_DEF(INS_LIST_INS(iter),  ArmDefinedRegisters(INS_LIST_INS(iter)));
		VERBOSE(0,("After  @I",INS_LIST_INS(iter)));
		iter = INS_LIST_NEXT(iter);
	      }

	      if(stack->stack[i].allocers) FATAL(("Didn't expected allocer here!"));

	    }

	  }
#endif
	}
      }
    }
  }
  VERBOSE(0,("%d registers not spilled anymore!",tellerke));
  STATUS(STOP, ("ArmOptimizeStack"));
  return;
}/*}}}*/

void FreeStackInformation(t_cfg * cfg)
{
  t_function * fun;
  t_stack_info * stack;

  CFG_FOREACH_FUN(cfg,fun)
  {
    if(!FUNCTION_STACK_INFO(fun)) continue;// FATAL(("No information available!"));
    stack = FUNCTION_STACK_INFO(fun);
    if(stack->size)
      FreeStackSlots(FUNCTION_STACK_INFO(fun));
    Free(FUNCTION_STACK_INFO(fun));
    FUNCTION_SET_STACK_INFO(fun,  NULL);
  }
}

t_stack_info * NewStackInfo(t_int32 size)/*{{{*/
{
  t_stack_info * ret;
  t_int32 i;
  if(size < 0) FATAL(("Negative stack size!"));
  size += 4;
  size = size/4;
  if(!size) return NULL;

  ret = Calloc(1,sizeof(t_stack_info));
  ret->size = size;
  ret->stack = Calloc(size,sizeof(t_stack_slot));
  for(i=0;i < size;i++)
    ret->stack[i].saved = ret->stack[i].loaded = ARM_REG_NONE;
  return ret;
}/*}}}*/

void FreeStackSlots(t_stack_info * stack)/*{{{*/
{
  t_uint32 i;

  for(i=0;i<stack->size;i++)
  {
    InsListCleanup((t_inslist**)&(stack->stack[i].savers));
    InsListCleanup((t_inslist**)&(stack->stack[i].loaders));
    InsListCleanup((t_inslist**)&(stack->stack[i].allocers));
    InsListCleanup((t_inslist**)&(stack->stack[i].deallocers));
    InsListCleanup((t_inslist**)&(stack->stack[i].out_frame));
  }
  Free(stack->stack);

}/*}}}*/


void ArmSaveStackInformationForMemory(t_stack_info * stack,t_arm_ins * i_ins,t_equations eqs, t_int32 offset, t_int32 curr_stack_value)/*{{{*/
{
  t_int32 stackoffset = -(offset/4);
  curr_stack_value = -(curr_stack_value/4);

  if(stackoffset < 0 && ARM_INS_REGB(i_ins) == ARM_REG_R13) FATAL(("Stackoffset < 0! @I",i_ins));
  if(stackoffset >= stack->size && ARM_INS_REGB(i_ins) == ARM_REG_R13) FATAL(("Stackoffset (%d) > size (%d)! @I",stackoffset,stack->size,i_ins));

  if(ARM_INS_OPCODE(i_ins) == ARM_STM)
  {
    t_regset regset = RegsetNewFromUint32(ARM_INS_IMMEDIATE(i_ins));
    t_reg reg;
    t_inslist * insert;
    
    if(ARM_INS_FLAGS(i_ins) & FL_DIRUP)
    {
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) stackoffset -= 1;
      REGSET_FOREACH_REG(regset,reg)
      {
	if(stackoffset < 0)
	{
#ifdef DEBUG_STACKANALYSIS
	  VERBOSE(0,("STORE TO CALLERS FRAME: @I",i_ins));
#endif
	  return;
	}
	if(stackoffset >= stack->size)
	{
	  StackInfoPrint(stack,TRUE);
	  FATAL(("@I",i_ins));
	}
	insert = (t_inslist*)Calloc(1,sizeof(t_inslist));
	INS_LIST_INS(insert) = T_INS(i_ins);
	if(stack->stack[stackoffset].saved == ARM_REG_NONE)
	{
	  /*          VERBOSE(0,("@I saves %d at %d",i_ins,reg,stackoffset));*/
	  stack->stack[stackoffset].saved = reg;
	}
	else if(stack->stack[stackoffset].saved != reg)
	  stack->stack[stackoffset].saved = ARM_REG_DIFFERENT;
	/*          FATAL(("Different regs saved at same stackoffset! %d %d @I",reg, stack->stack[stackoffset].saved,i_ins));*/
	INS_LIST_NEXT(insert) = stack->stack[stackoffset].savers;
	stack->stack[stackoffset].savers = insert;
/*        LLAppend(NULL,insert,&(stack->stack[stackoffset].savers));*/
	stackoffset -= 1;
      }

    }
    else
    {
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) stackoffset += 1;
      REGSET_FOREACH_REG_INVERSE(regset,reg)
      {
	if(stackoffset < 0)
	{
#ifdef DEBUG_STACKANALYSIS
	  VERBOSE(0,("STORE TO CALLERS FRAME: @I",i_ins));
#endif
	  return;
	}
	if(stackoffset >= stack->size)
	{
	  StackInfoPrint(stack,TRUE);
	  FATAL(("@I",i_ins));
	}
	insert = (t_inslist*)Calloc(1,sizeof(t_inslist));
	INS_LIST_INS(insert) = T_INS(i_ins);
	if(stack->stack[stackoffset].saved == ARM_REG_NONE)
	{
	  /*          VERBOSE(0,("@I saves %d at %d",i_ins,reg,stackoffset));*/
	  stack->stack[stackoffset].saved = reg;
	}
	else if(stack->stack[stackoffset].saved != reg)
	  stack->stack[stackoffset].saved = ARM_REG_DIFFERENT;
	/*          FATAL(("Different regs saved at same stackoffset! %d %d @I",reg, stack->stack[stackoffset].saved,i_ins));*/
	INS_LIST_NEXT(insert) = stack->stack[stackoffset].savers;
	stack->stack[stackoffset].savers = insert;
	stackoffset += 1;
      }
    }
  }
  else if(ARM_INS_OPCODE(i_ins) == ARM_LDM)
  {
    t_regset regset = RegsetNewFromUint32(ARM_INS_IMMEDIATE(i_ins));
    t_reg reg;
    t_inslist * insert;
    t_regset live_regs = ArmInsRegsLiveAfterConditional(i_ins);

    
    if(ARM_INS_FLAGS(i_ins) & FL_DIRUP)
    {
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) stackoffset -= 1;
      REGSET_FOREACH_REG(regset,reg)
      {
	if(stackoffset < 0)
	{
#ifdef DEBUG_STACKANALYSIS
	  VERBOSE(0,("LOAD TO CALLERS FRAME: @I",i_ins));
#endif
	  return;
	}
	if(stackoffset >= stack->size)
	{
	  StackInfoPrint(stack,TRUE);
	  FATAL(("@I %d %d",i_ins,stackoffset,stack->size));
	}
	insert = (t_inslist*)Calloc(1,sizeof(t_inslist));
	INS_LIST_INS(insert) = T_INS(i_ins);
	if(stack->stack[stackoffset].loaded == ARM_REG_NONE)
	{
	  /*          VERBOSE(0,("@I loads %d at %d",i_ins,reg,stackoffset));*/
	  stack->stack[stackoffset].loaded = reg;
	}
	else if(stack->stack[stackoffset].loaded != reg)
	  stack->stack[stackoffset].loaded = ARM_REG_DIFFERENT;
	/*          FATAL(("Different regs loaded at same stackoffset! %d %d @I",reg, stack->stack[stackoffset].loaded,i_ins));*/
	INS_LIST_NEXT(insert) = stack->stack[stackoffset].loaders;
	stack->stack[stackoffset].loaders = insert;
	if(reg == ARM_REG_R15 || RegsetIn(live_regs,reg)) stack->stack[stackoffset].live = TRUE;
	stackoffset -= 1;
      }
    }
    else
    {
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) stackoffset += 1;
      REGSET_FOREACH_REG_INVERSE(regset,reg)
      {
	if(stackoffset < 0)
	{
#ifdef DEBUG_STACKANALYSIS
	  VERBOSE(0,("LOAD TO CALLERS FRAME: @I",i_ins));
#endif
	  return;
	}
	if(stackoffset >= stack->size)
	{
	  StackInfoPrint(stack,TRUE);
	  FATAL(("@I %d %d",i_ins,stackoffset,stack->size));
	}
	insert = (t_inslist*)Calloc(1,sizeof(t_inslist));
	INS_LIST_INS(insert) = T_INS(i_ins);
	if(stack->stack[stackoffset].loaded == ARM_REG_NONE)
	{
	  /*          VERBOSE(0,("@I loads %d at %d",i_ins,reg,stackoffset));*/
	  stack->stack[stackoffset].loaded = reg;
	}
	else if(stack->stack[stackoffset].loaded != reg)
	  stack->stack[stackoffset].loaded = ARM_REG_DIFFERENT;
	/*          FATAL(("Different regs loaded at same stackoffset! %d %d @I",reg, stack->stack[stackoffset].saved,i_ins));*/
	INS_LIST_NEXT(insert) = stack->stack[stackoffset].loaders;
	stack->stack[stackoffset].loaders = insert;
	if(reg == ARM_REG_R15 || RegsetIn(live_regs,reg)) stack->stack[stackoffset].live = TRUE;
	stackoffset += 1;
      }

    }
  }
  else if(ARM_INS_OPCODE(i_ins) == ARM_STR || ARM_INS_OPCODE(i_ins) == ARM_STRH || ARM_INS_OPCODE(i_ins) == ARM_STRB)
  {
    t_int32 curr_stack = stackoffset;
    if((ARM_INS_REGC(i_ins) == ARM_REG_NONE && ARM_INS_REGS(i_ins) == ARM_REG_NONE))
    {
      t_inslist * insert = NULL;
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX)
      {
	if(ARM_INS_FLAGS(i_ins) & FL_DIRUP)
	  stackoffset -= ARM_INS_IMMEDIATE(i_ins)/4;
	else
	  stackoffset += ARM_INS_IMMEDIATE(i_ins)/4;
      }
      if(stackoffset <= 0)
      {
	int j = 1;
	while((j <= curr_stack) && (j < stack->size) && stack->stack[j].savers && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[j].savers))))
	{
	  t_inslist * insert2;
	  insert2 = (t_inslist*) Calloc(1,sizeof(t_inslist));
	  INS_LIST_INS(insert2) = T_INS(i_ins);
	  INS_LIST_NEXT(insert2) = stack->stack[j].out_frame;
	  stack->stack[j].out_frame = insert2;
	  j++;
	}
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("STORE TO CALLERS FRAME: @I",i_ins));
#endif
	return;
      }

      insert = (t_inslist*) Calloc(1,sizeof(t_inslist));
      INS_LIST_INS(insert) = T_INS(i_ins);

      if(stackoffset >= stack->size)
      {
	FunctionDrawGraph(BBL_FUNCTION(ARM_INS_BBL(i_ins)),"troubles");
	FATAL(("@I",i_ins));
      }

      if(stack->stack[stackoffset].saved == ARM_REG_NONE)
      {
/*        VERBOSE(0,("@I saves %d at %d",i_ins,ARM_INS_REGA(i_ins),stackoffset));*/
	stack->stack[stackoffset].saved = ARM_INS_REGA(i_ins);
      }
      else if(stack->stack[stackoffset].saved != ARM_INS_REGA(i_ins))
	stack->stack[stackoffset].saved = ARM_REG_DIFFERENT;
      /*          FATAL(("Different regs saved at same stackoffset! %d %d @I",ARM_INS_REGA(i_ins),stack->stack[stackoffset/4].saved,i_ins));*/
      INS_LIST_NEXT(insert) = stack->stack[stackoffset].savers;
      stack->stack[stackoffset].savers = insert;

    }
    else if(stackoffset > 0)
    {
      if((stack->stack[stackoffset].savers == NULL || (!ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[stackoffset].savers))))))
      {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
      /* There is no spill code instruction at this offset, so it is probably just an access to a local
       * variable on the stack, skip it */
      }
      else if(/*diabloanopt_options.inlining && */ stack->stack[1].savers && INS_LIST_INS(stack->stack[1].savers) != INS_LIST_INS(stack->stack[stackoffset].savers))
      {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
        /* This instruction accesses the stack at some offset that overlaps with the stack of an inlined function */
      }
      else 
      {
#ifdef DEBUG_STACKANALYSIS
	StackInfoPrint(stack,TRUE);
	VERBOSE(0,("Implement @I, %d",i_ins,stackoffset));
#endif
      }
    }
    else if(curr_stack < 0 && stackoffset < 0 && !RegsetIn(ARM_INS_REGS_USE(i_ins),ARM_REG_R13))
    {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
      /* This is an access to the callers stack frame, it should be safe to skip this one if 
       * it does not use the stackpointer */
    }
    else
    {
      /* This situation occurs once in all our benchmarks, when a mov r0,r13 instruction is
       * pushed down in the callee of several calls. We check here if the stackoffset that
       * the base register points to is 0, in which case the stack frame of the caller is
       * accessed and we do not need to modify the instruction. */
      if(stackoffset != 0)
      {
	StackInfoPrint(stack,TRUE);
	FATAL(("Implement @I, %d",i_ins,stackoffset));
      }
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
    }
  }
  else if(ARM_INS_OPCODE(i_ins) == ARM_LDR || ARM_INS_OPCODE(i_ins) == ARM_LDRH || ARM_INS_OPCODE(i_ins) == ARM_LDRB || ARM_INS_OPCODE(i_ins) == ARM_LDRSB || ARM_INS_OPCODE(i_ins) == ARM_LDRSH)
  {
    t_int32 curr_stack = stackoffset;
    /* The load needs to have an immediate */
    if(ARM_INS_REGC(i_ins) == ARM_REG_NONE && ARM_INS_REGS(i_ins) == ARM_REG_NONE)
    {
      t_inslist * insert;
      t_regset live_regs = RegsetNew();
      if(ArmInsSpillsToStack(i_ins)) live_regs = ArmInsRegsLiveAfterConditional(i_ins);
/*      VERBOSE(0,("@iB\n------------------\nLive after @I : %x, Spills? %d",ARM_INS_BBL(i_ins),i_ins,live_regs,ArmInsSpillsToStack(i_ins)));*/
      if(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) 
      {
	if(ARM_INS_FLAGS(i_ins) & FL_DIRUP)
	  stackoffset -= ARM_INS_IMMEDIATE(i_ins)/4;
	else
	  stackoffset += ARM_INS_IMMEDIATE(i_ins)/4;
      }
      if(stackoffset <= 0)
      {
	int j = 1;
	while((j <= curr_stack) && (j < stack->size) && stack->stack[j].savers && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[j].savers))))
	{
	  t_inslist * insert2;
	  insert2 = (t_inslist*) Calloc(1,sizeof(t_inslist));
	  INS_LIST_INS(insert2) = T_INS(i_ins);
	  INS_LIST_NEXT(insert2) = stack->stack[j].out_frame;
	  stack->stack[j].out_frame = insert2;
	  j++;
	}
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("LOAD FROM CALLERS FRAME: @I",i_ins));
#endif
	return;
      }
      else if(! diabloanopt_options.rely_on_calling_conventions && ARM_INS_REGB(i_ins) == ARM_REG_R13 &&
              ! ArmInsWriteBackHappens(i_ins))
      {
	int j = stackoffset;
	while((j < stack->size) && (j <= curr_stack))
	{
	  if(stack->stack[j].savers && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[j].savers))))
	  {
	    t_inslist * insert2;
	    insert2 =  (t_inslist*) Calloc(1,sizeof(t_inslist));
	    INS_LIST_INS(insert2) = T_INS(i_ins);
	    INS_LIST_NEXT(insert2) = stack->stack[j].out_frame;
	    stack->stack[j].out_frame = insert2;
	  }
	  j++;
	}
#ifdef DEBUG_STACKANALYSIS
	VERBOSE(0,("GUESSED LOAD FROM INLINED CALLERS FRAME: @I",i_ins));
#endif
       return;
      }
      if(stackoffset >= stack->size) 
      {
	WARNING(("@I uses a stack slot that is currently not allocated",i_ins));
	return;
      }

      if(stack->stack[stackoffset].loaded == ARM_REG_NONE)
      {
/*        VERBOSE(0,("@I loads %d at %d",i_ins,ARM_INS_REGA(i_ins),stackoffset));*/
	stack->stack[stackoffset].loaded = ARM_INS_REGA(i_ins);
      }
      else if(stack->stack[stackoffset].loaded != ARM_INS_REGA(i_ins))
	stack->stack[stackoffset].loaded = ARM_REG_DIFFERENT;
      /*          FATAL(("Different regs loaded at same stackoffset! %d %d @I",ARM_INS_REGA(i_ins),stack->stack[stackoffset].loaded,i_ins));*/
      insert = (t_inslist*) Calloc(1,sizeof(t_inslist));
      INS_LIST_INS(insert) = T_INS(i_ins);
      INS_LIST_NEXT(insert) = stack->stack[stackoffset].loaders;
      stack->stack[stackoffset].loaders = insert;
      if(ArmInsSpillsToStack(i_ins))
      {
	if(ARM_INS_REGA(i_ins) == ARM_REG_R15 || RegsetIn(live_regs,ARM_INS_REGA(i_ins))) stack->stack[stackoffset].live = TRUE;
      }
      else stack->stack[stackoffset].live = TRUE;

      if(ARM_INS_REGB(i_ins) == ARM_REG_R13 && !(ARM_INS_FLAGS(i_ins) & FL_PREINDEX) && ARM_INS_IMMEDIATE(i_ins) > 4)
      {
	int r = ARM_INS_IMMEDIATE(i_ins)/4  - 1;
	for(;r > 0;r--)
	{
	  t_inslist * insert2;
	  insert2 = (t_inslist*) Calloc(1,sizeof(t_inslist));
	  INS_LIST_INS(insert2) = T_INS(i_ins);
	  INS_LIST_NEXT(insert2) = stack->stack[stackoffset -r].deallocers;
	  stack->stack[stackoffset -r].deallocers = insert2;
	}
      }

    }
    else if(stackoffset > 0 && stackoffset < stack->size && (stack->stack[stackoffset].savers == NULL || (!ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[stackoffset].savers))))))
    {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
      /* There is no spill code instruction at this offset, so it is probably just an access to a local
       * variable on the stack, skip it */
    }
    else if(curr_stack < 0 && stackoffset < 0 && !RegsetIn(ARM_INS_REGS_USE(i_ins),ARM_REG_R13))
    {
#ifdef DEBUG_STACKANALYSIS
      VERBOSE(0,("Ignoring @I", i_ins));
#endif
      /* This is an access to the callers stack frame, it should be safe to skip this one if 
       * it does not use the stackpointer */
    }
    else
    {
/*      StackInfoPrint(stack,TRUE);*/
/*      VERBOSE(0,("Implement @I, %d, %d",i_ins,stackoffset,curr_stack));*/
    }
  }
/*  else FATAL(("Implement @I",i_ins));*/

}/*}}}*/

void ArmAddStackSlotInformation(t_stack_info * stack, t_arm_ins *ins, t_int32 stackoffset, t_int32 slots)/*{{{*/
{
  t_int32 i;
  t_inslist * insert;

#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("Stackoffset = %d\nSlots = %d",stackoffset,slots));
#endif
  if(slots < 0)
  {
#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("Allocing %d to %d on stack with @I",stackoffset + 1, stackoffset - slots,ins));
#endif

    for(i=stackoffset + 1;i<=stackoffset - slots;i++)
    {
      insert = (t_inslist*) Calloc(1,sizeof(t_inslist));
      INS_LIST_INS(insert) = T_INS(ins);
      INS_LIST_NEXT(insert) = stack->stack[i].allocers;
      stack->stack[i].allocers = insert;
    }
  }
  else if (slots > 0)
  {
#ifdef DEBUG_STACKANALYSIS
    VERBOSE(0,("Deallocing %d to %d on stack with @I",stackoffset, stackoffset + 1 - slots,ins));
#endif
      
    for(i=stackoffset+1-slots;i<=stackoffset;i++)
    {
      if(i<0 || i >= stack->size) FATAL(("OUt of stack frame????? @I",ins));
      insert = (t_inslist*) Calloc(1,sizeof(t_inslist));
      INS_LIST_INS(insert) = T_INS(ins);
      INS_LIST_NEXT(insert) = stack->stack[i].deallocers;
      stack->stack[i].deallocers = insert;
    }
  }
}
/*}}}*/


// returns FALSE if failed to track stack height
t_bool ArmSaveStackInformationForDataproc(t_stack_info * stack,t_arm_ins * ins,t_equations eqs, t_int32 offset)/*{{{*/
{
  t_int32 slots = 0;
  t_int32 stackoffset = -(offset/4);
  t_cfg * cfg=ARM_INS_CFG(ins);


  if(ARM_INS_REGA(ins) == ARM_INS_REGB(ins) && (ARM_INS_FLAGS(ins) & FL_IMMED))
  {
      slots = ARM_INS_IMMEDIATE(ins)/4;
    if(ARM_INS_OPCODE(ins) == ARM_SUB)
    {
      ArmAddStackSlotInformation(stack,ins,stackoffset,-slots);
    }
    else if(ARM_INS_OPCODE(ins) == ARM_ADD)
    {
      if(stackoffset < 0 || stackoffset >= stack->size) FATAL(("Yups"));
      if (ARM_INS_IMMEDIATE(ins)==0xff000000) 
        {
          slots = 0x0100000 / 4;
          ArmAddStackSlotInformation(stack,ins,stackoffset,-slots);
        }
      else
        ArmAddStackSlotInformation(stack,ins,stackoffset,slots);
    }
    else
      FATAL(("Implement @I",ins));
  }
  else if (((ARM_INS_OPCODE(ins) == ARM_SUB) || (ARM_INS_OPCODE(ins) == ARM_ADD)) && (ARM_INS_FLAGS(ins) & FL_IMMED) && EquationsRegsDiffer(cfg,eqs,ARM_REG_R13,ARM_INS_REGB(ins),&slots) == YES)
  {
    /* ARM_REG_R13 - ARM_INS_REGB(ins) = diff => ARM_INS_REGB(ins) = ARM_REG_R13 - diff,
     * so we should invert the sign of slots */
    slots= -(slots/4);
    if (ARM_INS_OPCODE(ins) == ARM_ADD)
      slots += ARM_INS_IMMEDIATE(ins)/4;
    else
      slots -= ARM_INS_IMMEDIATE(ins)/4;
    ArmAddStackSlotInformation(stack,ins,stackoffset,slots);
  }
  else if(ARM_INS_OPCODE(ins) == ARM_MOV && ARM_INS_REGC(ins) != ARM_REG_NONE && EquationsRegsDiffer(cfg,eqs,ARM_REG_R13,ARM_INS_REGC(ins),&slots) == YES)
  {
    /* ARM_REG_R13 - ARM_INS_REGC(ins) = diff => ARM_INS_REGC(ins) = ARM_REG_R13 - diff,
     * so we should invert the sign of slots */
    slots= -(slots/4);
    ArmAddStackSlotInformation(stack,ins,stackoffset,slots);
  }
  else
    {
      return FALSE;
      //FATAL(("Implement @I",ins));
    }
  return TRUE;
}/*}}}*/

void ArmSaveStackInformationForStackUse(t_stack_info * stack,t_arm_ins * ins,t_equations eqs, t_int32 stackoffset)/*{{{*/
{
  t_int32 curr_stackoffset = -(stackoffset/4);
  t_inslist * insert;
  int j;

  stackoffset = -(stackoffset/4);
/*  VERBOSE(0,("Stack Use @I",ins));*/
/*  StackInfoPrint(stack,1);*/
/*  printf("Curr  stackoffset = %d\n",stackoffset);*/

  if(ARM_INS_OPCODE(ins) == ARM_ADD && ARM_INS_REGC(ins) == ARM_REG_NONE)
  {
    stackoffset -= (ARM_INS_IMMEDIATE(ins)&0xfffffffc)/4;
    if(stackoffset <= 0)
      j = 1;
    else 
      j = stackoffset+1;

/*    printf("After stackoffset = %d\n",stackoffset);*/
    while(j <= curr_stackoffset)
    {
      if(stack->stack[j].savers && ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[j].savers))))
      {
	insert = (t_inslist*) Calloc(1,sizeof(t_inslist));
	INS_LIST_INS(insert) = T_INS(ins);
	INS_LIST_NEXT(insert) = stack->stack[j].out_frame;
	stack->stack[j].out_frame = insert;
/*        VERBOSE(0,("OF added: @I",ins));*/
      }
      j++;
    }
  }
#ifdef DEBUG_STACKANALYSIS
  else
    VERBOSE(0,("What to do with @I",ins));
#endif
}/*}}}*/

void StackInfoPrint(t_stack_info * stack, t_bool print_arm_ins)/*{{{*/
{
  t_int32 i;
  t_inslist * iter;

  for(i=0;i<stack->size;i++)
  {
    VERBOSE(0,("At slot %2d reg %3d is stored and reg %3d is loaded%s\n",i,stack->stack[i].saved,stack->stack[i].loaded,stack->stack[i].live?".":" but dead."));
    if(print_arm_ins)
    {
      iter = stack->stack[i].savers;
      if(iter)
      {
	VERBOSE(0,("Savers :"));
	while(iter)
	{
	  VERBOSE(0,("@I ",INS_LIST_INS(iter)));
	  iter = INS_LIST_NEXT(iter);
	}
      }
      iter = stack->stack[i].loaders;
      if(iter)
      {
	VERBOSE(0,("Loaders :"));
	while(iter)
	{
	  VERBOSE(0,("@I ",INS_LIST_INS(iter)));
	  iter = INS_LIST_NEXT(iter);
	}
      }
      iter = stack->stack[i].allocers;
      if(iter)
      {
	VERBOSE(0,("Allocers :"));
	while(iter)
	{
	  VERBOSE(0,("@I ",INS_LIST_INS(iter)));
	  iter = INS_LIST_NEXT(iter);
	}
      }
      iter = stack->stack[i].deallocers;
      if(iter)
      {
	VERBOSE(0,("Deallocs :"));
	while(iter)
	{
	  VERBOSE(0,("@I ",INS_LIST_INS(iter)));
	  iter = INS_LIST_NEXT(iter);
	}
      }
      iter = stack->stack[i].out_frame;
      if(iter)
      {
	VERBOSE(0,("Outframers :"));
	while(iter)
	{
	  VERBOSE(0,("@I ",INS_LIST_INS(iter)));
	  iter = INS_LIST_NEXT(iter);
	}
      }

    }

  }
}/*}}}*/
    
/* Flag functions whose stack should keep the same layout */
void ArmMarkNoStackChangeFunctions(t_cfg * cfg)/*{{{*/
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * ins;
  t_bool change;

  CFG_FOREACH_FUN(cfg,fun)
  {

    if (FUNCTION_IS_HELL(fun))
    {
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED);
      continue;
    }

    /* default: assume ok to optimise */
    FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) & ~FF_DONT_CHANGE_STACK);

    FUNCTION_FOREACH_BBL(fun,bbl)
    {
#ifdef GUARANTEE_DWORD_STACK_ALIGNMENT
      BBL_FOREACH_ARM_INS_R(bbl,ins)
      {
        /* as soon as copies are taken from the stack pointer, strange 
           code might depend on 8-byte stack alignment */

        /* TODO: this kills virtually all stack spill optimizations */
        /* to solve: make spill first count nr of registers to spill less */
        /* and only remove pairs of spilled registers */
        if (ARM_INS_TYPE(ins)==IT_DATAPROC)
          if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R13))
            if (!RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13))
              {
                VERBOSE(2,("Marked function %s as no stack change because it copies stack pointer",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"no_name"));
                FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
                FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED);
                break;
              }
                
        /* alloca -> requires 8-byte stack alignment */
	if ((ARM_INS_REGA(ins) == ARM_REG_R13) &&
	    (((ARM_INS_OPCODE(ins) == ARM_RSB) &&
	      (ARM_INS_REGC(ins) == ARM_REG_R13)) ||
	     ((ARM_INS_OPCODE(ins) == ARM_SUB) &&
	      (ARM_INS_REGB(ins) == ARM_REG_R13) &&
	      (ARM_INS_REGC(ins) != ARM_REG_NONE))))
	{
          VERBOSE(2,("Marked function %s as no stack change because uses alloca",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"no_name"));
	  FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
	  /* mark all functions containing alloca, since their callers
	   * cannot be optimised either
	   */
          FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED);
	  break;
	}
#endif
        /* unmark other functions for starters */
        FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & ~FF_IS_MARKED);

        /* doubleword load/store not yet properly handled by ls_fwd
         * and by stack slot removal
         */
        if ((ARM_INS_OPCODE(ins) == ARM_LDRD) ||
            (ARM_INS_OPCODE(ins) == ARM_STRD))
	{
	  FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
	  break;
	}
      }
      if (FUNCTION_FLAGS(fun) & FF_DONT_CHANGE_STACK)
        break;
    }
  }

#ifdef GUARANTEE_DWORD_STACK_ALIGNMENT
  /* all functions from which alloc-using functions are reachable must
   * also guarantee dword alignment of their stack
   */
  do
  {
    t_cfg_edge *edge;

    change = FALSE; 
    CFG_FOREACH_EDGE(cfg,edge)
    {
      if (CFG_EDGE_CAT(edge) & ET_FORWARD_INTERPROC)
      {
        fun = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
        if (!(FUNCTION_FLAGS(fun) & FF_IS_MARKED) &&
            (FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge))) & FF_IS_MARKED))
        {
          VERBOSE(2,("Marked function %s as no stack change because may indirectly call alloca",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"no_name"));
          FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED | FF_DONT_CHANGE_STACK);
          change = TRUE;
        }
      }
    }
  }
  while (change);
#endif
}/*}}}*/

/* This function tries to replace copies of the stackpointer with the stackpointer 
 * in load or store instructions */
void ArmMakeStackTransparent(t_cfg * cfg)/*{{{*/
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * ins;
  t_equations eqs = EquationsNew(cfg);
  t_int32 diff;
  t_bool dirup;
  static t_uint32 tellerke = 0;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      if(!BBL_EQS_IN(bbl)) 
      {
/*        VERBOSE(0,("No equations available for @B\n",bbl));*/
	continue;
      }
      BBL_FOREACH_ARM_INS_R(bbl,ins)
      {
#ifdef DEBUG_STACKANALYSIS
        if(diablosupport_options.debugcounter <= tellerke) break;
#endif
	if((ARM_INS_OPCODE(ins) == ARM_LDR || ARM_INS_OPCODE(ins) == ARM_STR || ARM_INS_OPCODE(ins) == ARM_LDRB || ARM_INS_OPCODE(ins) == ARM_STRB) /*&& !ArmInsWriteBackHappens(ins)*/ && ARM_INS_REGB(ins) != ARM_REG_R13 && ARM_INS_REGS(ins) == ARM_REG_NONE && ARM_INS_REGC(ins) == ARM_REG_NONE)
	{
	  BblCopyAnalysisUntilIns(T_INS(ins),eqs);
	  if(EquationsRegsDiffer(cfg,eqs,ARM_REG_R13, ARM_INS_REGB(ins), &diff) == YES)
	  {
	    /* ARM_REG_R13 - ARM_INS_REGB(ins) = diff => ARM_INS_REGB(ins) = ARM_REG_R13 - diff,
	     * so we should invert the sign of diff */
	    diff = -diff;
/*            VERBOSE(0,("Difference: %d",diff));*/
	    if(!ArmInsWriteBackHappens(ins))
	    {
	      if(ARM_INS_FLAGS(ins) & FL_DIRUP)
		diff += ARM_INS_IMMEDIATE(ins);
	      else
		diff -= ARM_INS_IMMEDIATE(ins);
	      dirup = (diff >= 0);
	      if(diff < 0) diff = -(diff);
	      if(ArmInsIsEncodableConstantForOpcode(diff,ARM_INS_OPCODE(ins), ARM_INS_FLAGS(ins) & FL_THUMB))
	      {
/*                VERBOSE(0,("Stackpointer! Changed @I",ins));*/
		ARM_INS_SET_REGB(ins,  ARM_REG_R13);
		ARM_INS_SET_IMMEDIATE(ins,  diff);
		if(dirup)
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
		else
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP);
/*                VERBOSE(0,("To @I",ins));*/
		ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
		ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
		tellerke++;
		FUNCTION_SET_FLAGS(fun,    FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
	      }
	    }
	    else
	    {
	      /* Write back happens, check if the base register is live afterwards */
	      t_regset live_after = ArmInsRegsLiveAfter(ins);
	      if(!RegsetIn(live_after,ARM_INS_REGB(ins)))
	      {
		if((ARM_INS_FLAGS(ins) & FL_PREINDEX))
		{
		  if(ARM_INS_FLAGS(ins) & FL_DIRUP)
		    diff += ARM_INS_IMMEDIATE(ins);
		  else
		    diff -= ARM_INS_IMMEDIATE(ins);
		}
		dirup = (diff >= 0);
		if(diff < 0) diff = -(diff);
		if(ArmInsIsEncodableConstantForOpcode(diff,ARM_INS_OPCODE(ins), ARM_INS_FLAGS(ins) & FL_THUMB))
		{
		  VERBOSE(0,("WBStackpointer! Changed @I",ins));
		  ARM_INS_SET_IMMEDIATE(ins,  diff);
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_WRITEBACK);
		  ARM_INS_SET_REGB(ins,  ARM_REG_R13);
		  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
		  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
		  VERBOSE(0,("To @I",ins));
		  tellerke++;
		  FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) | FF_DONT_CHANGE_STACK);
		}
	      }
	    }
	  }
	}
	else if(ARM_INS_OPCODE(ins) == ARM_MOV && ARM_INS_REGC(ins) != ARM_REG_NONE)
	{
	}
      }
    }
  Free(eqs);
}/*}}}*/

void FunctionReplaceRegister(t_function * fun, t_reg orig, t_reg new)/*{{{*/
{
  t_bbl * bbl;
  t_arm_ins * ins;
  t_reg bkreg = ARM_REG_NONE;

  FUNCTION_FOREACH_BBL(fun,bbl)
    BBL_FOREACH_ARM_INS(bbl,ins)
    {
      if(RegsetIn(ARM_INS_REGS_USE(ins),orig) || RegsetIn(ARM_INS_REGS_DEF(ins),orig))
      {
	VERBOSE(0,("Switch regs before @I", ins));
	if(ARM_INS_REGA(ins) == orig)
	{
          bkreg = ARM_INS_REGA(ins);
	  ARM_INS_SET_REGA(ins,  new);
          if (!ArmInsIsEncodable(ins))
            ARM_INS_SET_REGA(ins, bkreg);
	}
	if(ARM_INS_REGB(ins) == orig)
	{
          bkreg = ARM_INS_REGB(ins);
          ARM_INS_SET_REGB(ins,  new);
          if (!ArmInsIsEncodable(ins))
            ARM_INS_SET_REGB(ins, bkreg);
	}
	if(ARM_INS_REGC(ins) == orig)
	{
          bkreg = ARM_INS_REGC(ins);
          ARM_INS_SET_REGC(ins,  new);
          if (!ArmInsIsEncodable(ins))
            ARM_INS_SET_REGC(ins, bkreg);
	}
	if(ARM_INS_REGS(ins) == orig)
	{
          bkreg = ARM_INS_REGS(ins);
          ARM_INS_SET_REGS(ins,  new);
          if (!ArmInsIsEncodable(ins))
            ARM_INS_SET_REGS(ins, bkreg);
	}
	if(ARM_INS_OPCODE(ins) == ARM_LDM || ARM_INS_OPCODE(ins) == ARM_STM) 
	{
	  t_regset tmp = RegsetNewFromUint32(ARM_INS_IMMEDIATE(ins));
	  t_reg i;
	  t_bool intermediate = FALSE;
	  if(orig < new)
	  {
	    REGSET_FOREACH_REG(tmp,i)
	      if(i > orig && i < new) intermediate = TRUE;
	  }
	  else
	  {
	    REGSET_FOREACH_REG(tmp,i)
	      if(i > new && i < orig) intermediate = TRUE;
	  }
	  if(intermediate)
	    FATAL(("Changing regs can be dangerous here! @I, %d, %d",ins,orig,new));

	  ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins)& ~(1<<orig));
	  ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins)| (1<<new));
	}

	VERBOSE(0,("Switch regs after  @I", ins));
      }
    }
}/*}}}*/

void ArmFunComputeStackSavedRegistersFromStackInfo(t_function * fun)
{
  t_uint32 i;
  t_stack_info * stack;
  t_arm_ins * saver = NULL;
  t_regset fun_saved = NullRegs;
  t_bool add_reg;
  char filename[1000];
  static int teller  = 0;

  FUNCTION_SET_REGS_SAVED(fun,  NullRegs);
  if(!(FUNCTION_FLAGS(fun) & FF_STACKPOINTER_TRACEABLE)) return;

  if(!FUNCTION_STACK_INFO(fun)) return;
  sprintf(filename,"stack_%s.dot",FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"NONAME");
  stack = FUNCTION_STACK_INFO(fun);

  for(i = 1; i < stack->size; i++)
  {
    add_reg = TRUE;
    if(stack->stack[i].savers)
    {
      if(INS_LIST_NEXT(stack->stack[i].savers))
      {
	break;

/*        FunctionDrawGraph(fun,filename);*/
/*        FATAL(("Multiple savers, is this a problem in %s, @iB?",FUNCTION_NAME(fun),ARM_INS_BBL(stack->stack[i].savers->data)));*/
      }
      else
      {
	if(!ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))))
	{
	  continue;
/*          FATAL(("Probably just skip this no-spill instruction? @I",stack->stack[i].savers->data));*/
	}
	if(saver && saver != T_ARM_INS(INS_LIST_INS(stack->stack[i].savers)))
	{
	  break;
/*          FunctionDrawGraph(fun,filename);*/
/*          FATAL(("Probably just exit the loop here? @I",stack->stack[i].savers->data));*/
	}
        if(!saver) saver = T_ARM_INS(INS_LIST_INS(stack->stack[i].savers));
	if(stack->stack[i].saved != stack->stack[i].loaded)
	{
	  continue;
/*          FATAL(("Not restored, %d is loaded in %d in %s",stack->stack[i].saved,stack->stack[i].loaded, FUNCTION_NAME(fun)));*/
	}
	else
	{
	  t_inslist * iter;
    t_arm_ins * prev_ins;
    t_bbl * tmp_bbl;
	  t_regset tmp;

	  if(stack->stack[i].deallocers)
	  {
	    continue;
/*            FATAL(("Deallocing on a path (@I) in %s",stack->stack[i].deallocers->data,FUNCTION_NAME(fun)));*/
	  }
	  iter = stack->stack[i].loaders;
	  while(iter)
	  {
	    if(!ArmInsSpillsToStack(T_ARM_INS(INS_LIST_INS(iter))))
	    {
/*              FunctionDrawGraph(fun,filename);*/
	      add_reg = FALSE;
/*              FATAL(("Spilled reg is loaded inside fun %s at @I",FUNCTION_NAME(fun),INS_LIST_INS(iter)));*/
	    }
	    iter = INS_LIST_NEXT(iter);
	  }

	  if (INS_BBL(INS_LIST_INS(stack->stack[i].savers)) != FUNCTION_BBL_FIRST(BBL_FUNCTION(INS_BBL(INS_LIST_INS(stack->stack[i].savers)))))
	    continue;

	  prev_ins = T_ARM_INS(INS_IPREV(INS_LIST_INS(stack->stack[i].savers)));

	  while (prev_ins)
	    {
	      if (RegsetIn(ARM_INS_REGS_USE(prev_ins),stack->stack[i].saved))
		add_reg = FALSE;
	      prev_ins = ARM_INS_IPREV(prev_ins);
	    }
	  tmp_bbl = INS_BBL(INS_LIST_INS(stack->stack[i].savers));
	  tmp=BBL_REGS_LIVE_OUT(tmp_bbl);
	  
	  BBL_SET_REGS_LIVE_OUT(tmp_bbl, RegsetEmpty());
	  
	  if (RegsetIn(ArmInsRegsLiveAfter(T_ARM_INS(INS_LIST_INS(stack->stack[i].savers))),stack->stack[i].saved))
	    add_reg = FALSE;

	  BBL_SET_REGS_LIVE_OUT(tmp_bbl, tmp);

	  if(add_reg)
	  {
	    if(RegsetIn(CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_saved,stack->stack[i].saved))
#ifdef DEBUG_STACKANALYSIS
              if(diablosupport_options.debugcounter >= teller)
#endif
		{
		  RegsetSetAddReg(fun_saved,stack->stack[i].saved);
		  teller++;
		}
	    /*
	      else
		{
		  static t_bool done = FALSE;
		  t_function * fun2;
		  
		  if (!done)
		    {
		      CFG_FOREACH_FUN(FUNCTION_CFG(fun),fun2)
			{
			  char * filename = malloc(100);
			  sprintf(filename, "fun-%s.dot",fun2->name);
			  FunctionDrawGraphWithHotness(fun2,filename);	      
			}
		    }
		  done = TRUE;
		}
	    */
	  }
	}
      }
    }
  }
    
  FUNCTION_SET_REGS_SAVED(fun, fun_saved);
  return;
}

void CheckEquations(t_bbl * bbl)
{
  t_cfg * cfg=BBL_CFG(bbl);
  t_uint32 i;
  if(BBL_EQS_IN(bbl))
  for(i = 0; i < (CFG_DESCRIPTION(cfg)->num_int_regs + 1)-2; i++)
  {
    if(BBL_EQS_IN(bbl)[i].rega > 16 && BBL_EQS_IN(bbl)[i].rega != REG_BOT && BBL_EQS_IN(bbl)[i].rega != REG_TOP)
    {
      EquationsPrint(cfg,BBL_EQS_IN(bbl));
      FATAL(("Troubles for @B",bbl));
    }
    if(BBL_EQS_IN(bbl)[i].regb > 16 && BBL_EQS_IN(bbl)[i].regb != REG_BOT && BBL_EQS_IN(bbl)[i].regb != REG_TOP)
    {
      EquationsPrint(cfg,BBL_EQS_IN(bbl));
      FATAL(("Troubles for @B",bbl));
    }
  }

}

void ArmValidatePushPopRegsets(t_cfg * cfg)
{
  t_bbl * i_bbl = NULL;
  t_ins * i_ins = NULL;

  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    BBL_FOREACH_INS(i_bbl, i_ins)
    {
      if ((ARM_INS_OPCODE(T_ARM_INS(i_ins)) == ARM_LDM) || (ARM_INS_OPCODE(T_ARM_INS(i_ins)) == ARM_STM))
      {
        ArmInsLoadStoreMultipleToSingle(T_ARM_INS(i_ins));
      }
    }
  }
}
/* vim: set shiftwidth=2 foldmethod=marker: */
