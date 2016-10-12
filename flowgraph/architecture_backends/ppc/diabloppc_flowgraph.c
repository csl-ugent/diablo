/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * }}}
 * 
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diabloppc.h>

/* PpcFlowgraphFindExitBlock {{{*/
/*t_bbl *
  PpcFlowgraphFindExitBlock(t_bbl * start_block, t_section * code)
  {
  t_bbl *exit_block,*current_block;
  exit_block = NULL;

  current_block = start_block; 

  if(PpcBblJumpsToLinkRegister(current_block)
  &&
  !PpcBblTouchLinkRegister(current_block))
  {
  exit_block = current_block;
  }
  else if(!PpcBblTouchLinkRegister(current_block))
  {
  current_block = PpcBblGetNextBlock(current_block,code);
  exit_block = PpcFlowgraphFindExitBlock(current_block,code);
  }
  else
  {
  FATAL(("Link Register modified!"));
  }
  return exit_block;
  }
  */
/*}}}*/

/* PpcBblTouchLinkRegister {{{*/
/*
   t_bool
   PpcBblTouchLinkRegister(t_bbl *block)
   {
   t_bool res = FALSE;

   return res;
   }
   */
/*}}}*/

/* PpcBblJumpsToLinkRegister {{{*/
/* t_bool
   PpcBblJumpsToLinkRegister(t_bbl *block)
   {
   t_bool res = FALSE;
   t_ppc_ins* last_ins = T_PPC_INS(BBL_INS_LAST(block));
   if (PPC_INS_OPCODE(last_ins)==PPC_BCLR)
   {
   res = TRUE;
   }
   return res;
   }
   */
/*}}}*/

/* PpcBblGetNextBlock {{{*/
#if 0
t_bbl *
PpcBblGetNextBlock(t_bbl *block, t_section *code)
{
  t_bbl *next_block = NULL;
  t_ppc_ins* target; 
  t_address target_address;
  t_ppc_ins* ins = T_PPC_INS(BBL_INS_LAST(block));

  switch(PPC_INS_TYPE(ins))
  {
    case IT_BRANCH:
      if (PPC_INS_INEXT(ins) != NULL)
      {
        return T_BBL(PPC_INS_BBL(PPC_INS_INEXT(ins)));
      }

      /* The target of the branch will also be a basic block leader  */
      if ((PPC_INS_OPCODE(ins) == PPC_B ) 
          || (PPC_INS_OPCODE(ins) == PPC_BC ))
      {
        /* Compute the target address */
        if(PpcInsAbsoluteAddress(ins))
        {
          /* Target address is absolute  */
          target_address = AddressNew32(PPC_INS_IMMEDIATE(ins));
        }
        else
        {
          /* Target address is relative */
          target_address = AddressAddUint32(PPC_INS_CADDRESS(ins),
                                            PPC_INS_IMMEDIATE(ins));
        }

        /* Get instruction target */
        target=T_PPC_INS(ObjectGetInsByAddress(obj, target_address));

        if (target)
        {
          return T_BBL(PPC_INS_BBL(target));
        }
        else
        {
          VERBOSE(1,(" %s : Can't find the target of a jump \
                     \n. Instruction: %x target: %x", __func__,
                     PPC_INS_CADDRESS(ins),target_address));
        }
      }
      else if ((PPC_INS_OPCODE(ins) == PPC_BCCTR ) 
               || (PPC_INS_OPCODE(ins) == PPC_BCLR ))
      {
        FATAL(("branch to count register!")); 
      }
      else
      {
        FATAL((" %s : Unknown Branch type.", __func__ ));
      }
      break;

    case IT_SWI:
      if (PPC_INS_INEXT(ins) != NULL) 
      {
        return T_BBL(PPC_INS_BBL(PPC_INS_INEXT(ins)));
      }
      break;
  }

  return next_block;
}
#endif
/*}}}*/

/* TODO: This is a kind of optimization, should
 * be moved to the anopt library ? */

/* PpcPatchCallsToWeakSymbols {{{ */

/*!
 *
 * \todo Document
 *
 * \param obj
 *
 * \return t_uint32 
 */

t_uint32 PpcPatchCallsToWeakSymbols (t_object *obj)
{
  t_uint32 ncalls = 0;

  t_cfg * cfg = OBJECT_CFG(obj);
  t_bbl * bbl;
  t_ppc_ins * ins;
  t_cfg_edge * edge, * edge2;

  CFG_FOREACH_BBL(cfg,bbl)
  {

    ins = T_PPC_INS(BBL_INS_LAST(bbl));

    if(!ins) continue;

    if (PPC_INS_OPCODE (ins) == PPC_B || PPC_INS_OPCODE (ins) == PPC_BC)
    {
      t_reloc_ref *rr;
      t_reloc *rel;
      t_relocatable *reloc;

      rr = PPC_INS_REFERS_TO(ins);

      if (!rr) continue;
      ASSERT(!RELOC_REF_NEXT (rr), ("Instruction points to more than one place"));

      rel = RELOC_REF_RELOC (rr);

      ASSERT(RELOC_N_TO_RELOCATABLES (rel) == 1, ("Relocation points to more than one place"));
      reloc = RELOC_TO_RELOCATABLE (rel)[0];

      if (RELOCATABLE_RELOCATABLE_TYPE (reloc) == RT_SECTION && !strcmp (SECTION_NAME (T_SECTION (reloc)), "*UNDEF*"))
      {
        ncalls++;

        VERBOSE(1, ("Call to weak symbol found."));
        if(diabloppc_options.patch_calls_to_weak_symbols)
        {
          VERBOSE(1, ("Removing incoming control flow!"));
          BBL_FOREACH_PRED_EDGE_SAFE(bbl, edge, edge2)
          {
            PpcEdgeKillAndUpdateBblRecursive(edge);
            ncalls++;
          }
        }
        else
        {
          VERBOSE(1, ("Replacing the call by a NOP"));
          PpcInsMakeNop (T_INS (ins));
        }
      }
    }
  } 
  VERBOSE(1, ("%d BBLs patched because of weak symbols", ncalls));
  return ncalls;
}
/* }}} */

/* PpcFindBBLLeaders {{{ */
/*!
 * Basic leader-detection algorithm. 
 * ---------------------------------
 *
 * See e.g. the Red Dragon Book ("Compilers: Principles, Techniques and
 * Tools", by Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman) for more
 * details. 
 *
 * The aim of this function is to identify all start-addresses of basic
 * blocks (and mark them). There are four reasons to mark an instructions as
 * a basic block leader:
 * 
 * 1. The instruction is the target of a direct or indirect jump. This is not
 * as easy as it seems, especially when the program counter is explicit:
 * every instruction that changes the pc can be considered as a jump
 * instruction.
 *
 * 2. It's the successor of a control-flow altering instruction (if a jump is
 * conditional, than the next instruction will be the first instruction of the
 * fall-through block, else it is either target of an other jump or a dead
 * block. In the latter case, it will be removed by the unreachable block
 * removal routines, so we just mark it as a basic block leader).
 *
 * 3. There is an address produced to this basic block, this is either 
 *
 *    a. done directly, using the program counter (we assume no real
 *    code-address calculations are present (so not address of function + x) 
 *
 *    b. using a relocation (and not necessary detected when scanning the
 *    instructions) 
 *
 * For both cases we need to assume there will be an indirect jump to this
 * address.
 *
 * 4. The start of data blocks in code or instructions following datablocks
 * are considered basic block leaders.
 *
 *
 * This function will find all basic block leaders that are not part of a
 * switch statement. More switch checking will be done in optimize switches.
 * 
 * \todo needs only one parameter
 *
 * \param obj
 * \param code
 *
 * \return t_uint32 
 */
static t_uint32 PpcFindBBLLeaders (t_object *obj)
{
  /* We can still assume that all instructions are stored sequentially in the
   * array Ains the last instruction stored can be recognised with INSNEXT ==
   * NULL */

  t_uint32  nleaders=0;
  t_section *code;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_ppc_ins * target;
    t_address target_address;
    t_ppc_ins * ins;

    /* Initialisation: The first instruction of the program is always a leader.
     * The reasoning is equal to that of reason 2: Either there will be a jump to
     * this address or it's the entry point or it is dead. */

    PPC_INS_SET_ATTRIB(SECTION_DATA(code),  PPC_INS_ATTRIB(SECTION_DATA(code)) | IF_BBL_LEADER);

    /* Look for BBL_LEADERs in code {{{*/
    SECTION_FOREACH_PPC_INS(code,ins)
    {
      /* reason 4: ins is the start of a data block */
      if (PPC_INS_TYPE(ins) == IT_DATA)
      {
        if (PPC_INS_IPREV(ins) && (PPC_INS_TYPE(PPC_INS_IPREV(ins)) != IT_DATA)) {
          PPC_INS_SET_ATTRIB(ins, PPC_INS_ATTRIB(ins) | IF_BBL_LEADER);
        }
        continue;
      }
      /* reason 4b: ins is the first after a data block */
      if (PPC_INS_IPREV(ins) && (PPC_INS_TYPE(PPC_INS_IPREV(ins)) == IT_DATA))
      {
        PPC_INS_SET_ATTRIB(ins, PPC_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }

      switch(PPC_INS_TYPE(ins))
      {
        /* Branches : reason 1 and 2 {{{*/
        case IT_BRANCH:
          if (PPC_INS_INEXT(ins) != NULL)
          {
            PPC_INS_SET_ATTRIB(PPC_INS_INEXT(ins),
                               PPC_INS_ATTRIB(PPC_INS_INEXT(ins))|IF_BBL_LEADER);
          }

          /* The target of the branch will also be a basic block leader */
          if ((PPC_INS_OPCODE(ins) == PPC_B ) 
              || (PPC_INS_OPCODE(ins) == PPC_BC ))
          {
            /* Compute the target address */ 
            if (PpcInsAbsoluteAddress(ins))
            {
              /* Target address is absolute */
              target_address = PPC_INS_IMMEDIATE(ins);
            }
            else
            {
              /* Target address is relative */
              target_address = AddressAdd (PPC_INS_CADDRESS(ins), PPC_INS_IMMEDIATE(ins));
            }

            /* Get instruction target */
            target=T_PPC_INS(ObjectGetInsByAddress(obj, target_address));

            if (target)
            {
              PPC_INS_SET_ATTRIB(target,  PPC_INS_ATTRIB(target) | IF_BBL_LEADER); 
            }
            else
            {
              t_object * obj=SECTION_OBJECT(code);
              t_object * lo = ObjectGetLinkerSubObject(obj);	
              t_section * sec = SectionGetFromObjectByAddress(lo, target_address);
              t_bool ok = FALSE;

              VERBOSE(1,(" %s : Can't find the target of a jump \
                         \n. Instruction: %x target: %x", __func__,
                         PPC_INS_CADDRESS(ins),target_address));

              if ((sec)&&(StringPatternMatch("JMP_SLOT:*", SECTION_NAME(sec))))
              {
                /* a jump to a .plt entry */
                t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 9);
                t_section * rel  =  SectionGetFromObjectByName(lo, relname);
                ASSERT(rel, ("Relocation section (.rela.plt) for %s not found", SECTION_NAME(sec) + 9));
                ok = TRUE;

              }
              else if (AddressIsEq(target_address, AddressNew32(0x10000000)))
              {
                /* OK, this is a call or branch to a weak symbol */
                ok = TRUE;
              }
              /* Is this instruction a jump to the got section? It must have a
               * relocation in that case */
              else if (PPC_INS_REFERS_TO(ins))
              {
                t_uint32 i;
                t_reloc *rel = RELOC_REF_RELOC(PPC_INS_REFERS_TO(ins));

                for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
                {
                  if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_SUBSECTION)
                  {
                    t_section * sec = (t_section *) RELOC_TO_RELOCATABLE(rel)[i];

                    if (!StringCmp(SECTION_NAME(sec),"NULLGOT"))
                    {
                      /* Flag this ins as a got producer and make a jump to the next
                       * instruction */
                      PPC_INS_SET_FLAGS(ins, PPC_INS_FLAGS(ins) | PPC_FL_GOT_PRODUCER);
                      ok = TRUE;
                    }
                  }
                }
              }

              if (!ok)
                FATAL(("Target of branch @I is not in code section", ins));
            }}
          else if ((PPC_INS_OPCODE(ins) == PPC_BCCTR ) 
                   || (PPC_INS_OPCODE(ins) == PPC_BCLR ))
          {
            VERBOSE(1,(" Branch to LR, Branc to CTR : we don't have any clue of the about the target address, at the moment... "));
          }
          else
          {
            FATAL((" %s : Unknown Branch type.", __func__ ));
          }
          break;
          /*}}}*/

          /* System Calls and Traps {{{*/
        case IT_SWI:

          /* Actually, we could just consider system calls as normal
           * instructions, if we were sure that they are not long-jump-like
           * system calls. Since we do not know that for sure here, we must
           * assume that is will alter control flow => next instruction is a
           * leader */

          if (PPC_INS_INEXT(ins) != NULL) 
          {
            PPC_INS_SET_ATTRIB(PPC_INS_INEXT(ins), PPC_INS_ATTRIB(PPC_INS_INEXT(ins)) | IF_BBL_LEADER);
          }
          break;
          /*}}}*/
      }
      /* Relocations : reason 3 {{{*/
      if (PPC_INS_REFED_BY(ins))
      {
        VERBOSE(1,("Leader because is refed by a relocation: @I",ins));
        PPC_INS_SET_ATTRIB(ins,  PPC_INS_ATTRIB(ins) | IF_BBL_LEADER);
      }
      /*}}}*/
    }
    /*}}}*/
  }

  /* Count and return the number of BBL_LEADERS {{{*/
  nleaders = 0;
  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_ppc_ins *ins = T_PPC_INS(SECTION_DATA(code));
    while(ins)
    {
      if (PPC_INS_ATTRIB(ins)&IF_BBL_LEADER) 
      {
        nleaders++;
      }
      ins=PPC_INS_INEXT(ins);
    }
  }
  /*}}}*/

  return nleaders;
}
/* }}} */

/* PpcAddBasicBlockEdges {{{ */
/*!
 * Add edges between the newly created basic blocks. Both the lineair (array)
 * representation of the instructions and the basic block representation are
 * available: you can access the (copy) basic block by using PPC_INS_BBL of a
 * leader in the lineair representation.
 *
 * Although connecting the blocks isn't that hard, it could be difficult to
 * find out if a given jump instructions is a call or just a jump (without
 * return). Getting this completly safe is probably not what you want: if you
 * implement this completly safe, you'll probably end up with one big
 * function, which will degrade the performance of the rest of diablo or hell
 * nodes after each jump which will degrade the performance even further.
 *
 * On the other hand, if you get this wrong (e.g add a jump edge where there
 * should have been a call edge), then chances are that your flowgraph (and
 * thus the produced binary) is wrong (the successor block of the call block
 * will have no incoming (return) edge, and thus this block is dead). 
 *
 * The solution is to look at code that is produced by your compiler(s).
 * It'll probably have only a few possibilties to produce calls. If you
 * handle them as safe as possible, than you're off the hook. 
 *
 * \param section The disassembled section, with basic blocks already created
 *                we want to flowgraph
 *
 * \return t_uint32 
 */

#define VERBOSE_SWITCH 0  //for switch pattern dectection verbose mode
#define VERBOSE_NORMAL 0 //for normal edge verbose mode
#define VERBOSE_PATTERNS 0 //for unknown control flow verbose mode (usefull to detect patterns)
#define VERBOSE_INDIRECT 0 //for indirect control flow verbose mode
#define VERBOSE_INDIRECT2 0 //for indirect control flow verbose mode extra
#define VERBOSE_INDIRECT_COUNT 0 //for pattern counts of indirect control flow

#if VERBOSE_INDIRECT2
#define VERBOSE_INDIRECT 1
#endif

static t_uint32 PpcAddBasicBlockEdges (t_object *obj)
{
  t_ppc_ins *ins, *iter, *block_end, *block_start, *target, *prev, *prev_controltransfer;
  t_bbl *block, *ft_block;
  t_cfg *cfg = OBJECT_CFG(obj);
  t_reloc_ref *ref;
  t_address target_address, tablebase;
  t_uint32 nedges = 0;
  t_bool is_detected = FALSE;
  t_bool found_address;
  t_cfg_edge * edge;

  t_section *section;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, section, i)
  {
    /* {{{ first detect switch tables and add the necessary switch edges.
     * we don't have to do this, but otherwise a lot of hell edges will
     * be added that substantially degrade the accuracy and performance of a 
     * number of analyses */

    STATUS(START,("Detecting switches")); 
    /* Switch table; first possibility {{{ */
    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = PPC_INS_INEXT(ins))
    {
      /* switch constructs have the following pattern:
       *   cmpli $cr, %reg, %bound
       *   ...
       *   bc $cr <default_case>
       *   ...
       *   lis  <jumptable_base calculation>
       *   addi <jumptable_base calculation>
       *   ...
       *   rlwinm ... 2,0,29 <offset_in_table calculation>
       *   lwzx <read table value>
       *   ...
       *   add  <jump address calculation> 
       *   ...
       *   mtcrt %reg
       *   bctr 
       */

#if VERBOSE_SWITCH
      VERBOSE(0,("-------- Actual ins: @I",ins));
#endif 

      t_reg jumpreg, condreg;
      t_reloc * jumptablerel;
      t_address bound;
      t_ppc_ins *cmpli, *mtspr, *def, *tabledef;
      t_regset changed; 
      t_reloc_ref * rr;
      t_reloc * rel; 

      if (PPC_INS_IPREV(ins))
      {
        prev = PPC_INS_IPREV(ins);
      }

      if (prev && PpcInsIsControlTransfer(T_INS(prev)))
      {
        prev_controltransfer = prev;
      }

      /* looking for candidates, go next until:
       * - previous control transfer not conditional
       *   control transfer is detected
       * - actual ins is a branch unconditional
       *   to ctr
       */

      if (!prev_controltransfer || (PPC_INS_OPCODE(prev_controltransfer) != PPC_BC)) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("Control transfer is conditional"));
      VERBOSE(0,("Previous control transfer is: @I",prev_controltransfer));
      VERBOSE(0,("Previous is cblt: %d, %d == %d",PPC_INS_CB(prev_controltransfer),PPC_INS_CB(prev_controltransfer)%4, PPC_CBGT));
#endif 
      if (!PPC_IS_CBGT(prev_controltransfer)) continue; //TODO: check if could be
      //other conditions
#if VERBOSE_SWITCH
      VERBOSE(0,("Previous control transfer is : bgt")); 
#endif 
      //if (PPC_INS_BO(prev_controltransfer)!=BOTM4) continue; //TODO: check if is
      // necessary to check the
      // branch hint
      if ((PPC_INS_OPCODE(ins) != PPC_BCCTR) && (PPC_INS_OPCODE(ins) != PPC_BCLR)) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("Actual instruction is BCCTR or BCLR"));
#endif 
      if (PpcInsIsConditional(ins)) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("Actual instruction jumps always!"));
      /* Now, we delimited the window where to 
       * look the pattern.
       */

      VERBOSE(0,("Candidate found!"));
#endif 

      /* look for the mtspr instruction */
      for (iter = PPC_INS_IPREV(ins); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
      {
        if((PPC_INS_OPCODE(iter)==PPC_MTSPR) &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_REG_CTR) &&
           (PPC_INS_OPCODE(ins)==PPC_BCCTR))
        {
          mtspr = iter;
          break;
        }

        if((PPC_INS_OPCODE(iter)==PPC_MTSPR) &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_REG_LR) &&
           (PPC_INS_OPCODE(ins)==PPC_BCLR))
        {
          mtspr = iter;
          break;
        }

      }

      if(iter == prev_controltransfer) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("mtspr found: @I",mtspr));
#endif 

      /* retrieve register used */
      jumpreg = PPC_INS_REGA(mtspr);

#if VERBOSE_SWITCH
      VERBOSE(0,("register used: %s",PpcRegisterName(jumpreg)));
#endif 

      /* two cases for calculation:
       *  - offset table: lwzx <jumptable_base> + offset_in_table
       add <jumptable_base> + offset loaded from table
       *  - direct table lwzx <jumptable_base> + offset_in_table
       *
       * Look for the <jumpble_base>
       */
      for (iter = PPC_INS_IPREV(ins); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
      {
        if(((PPC_INS_OPCODE(iter)==PPC_ADD)||(PPC_INS_OPCODE(iter)==PPC_LWZX))
           &&
           RegsetIn(PPC_INS_REGS_DEF(iter),jumpreg))
        {
          def = iter;
          break;
        }
      }
      if(iter == prev_controltransfer) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("Definition found: @I",def));
      VERBOSE(0,("table base could be: %s or %s",PpcRegisterName(PPC_INS_REGA(def)),PpcRegisterName(PPC_INS_REGB(def)))); 
#endif 
      /* The both instructions have 2 source registers :
       * REGA or REGB. Both could contain the jumptable
       * base value. Calculate both and choose the one
       * that could be known.
       */
      tabledef = def;

#if VERBOSE_SWITCH
      VERBOSE(0,("Trying to compute: %s",PpcRegisterName(PPC_INS_REGA(def)))); 
#endif 
      tablebase = PpcComputeRelocatedAddressValue(&tabledef,PPC_INS_REGA(def),&found_address);

      if(!found_address)
      {
        tabledef = def;
#if VERBOSE_SWITCH
        VERBOSE(0,("Trying to compute: %s",PpcRegisterName(PPC_INS_REGB(def)))); 
#endif 
        tablebase = PpcComputeRelocatedAddressValue(&tabledef,PPC_INS_REGB(def),&found_address);
      }

      if(!found_address) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("instruction computing base table address: @I", tabledef));
      VERBOSE(0,("switch table base address: @G", tablebase));
#endif 

      /* tabledef contains the lis instruccion, pointed by
       * a relocation (the one that computes the base address
       * of the swith table).
       *
       * Check the relocation, it must be to the .rodata 
       * subsection.
       */

      rr = PPC_INS_REFERS_TO(PPC_INS_COPY(tabledef));
#if VERBOSE_SWITCH
      VERBOSE(0,("RR points : %x", rr));
#endif 

      if (!INS_REFERS_TO(INS_COPY(T_INS(tabledef))))
      {
#if VERBOSE_SWITCH
        VERBOSE(0,("could not find jumptable relocation for @I",tabledef));
#endif 
        continue;
      }
      jumptablerel = RELOC_REF_RELOC(INS_REFERS_TO(INS_COPY(T_INS(tabledef))));

      if (RELOC_N_TO_RELOCATABLES(jumptablerel) > 1) continue;
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(jumptablerel)[0]) != RT_SUBSECTION) continue;
      if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])) != RODATA_SECTION) continue;

      /* Get the condition register checked for the default case */
      condreg = PPC_COND_REG(prev_controltransfer);

#if VERBOSE_SWITCH
      VERBOSE(0,("Conditional register of jump: %s",PpcRegisterName(condreg))); 
#endif 

      /* Look for the compare intruction that defines the bounds  
       * of the table 
       */
      changed = NullRegs;
      for (cmpli = PPC_INS_IPREV(prev_controltransfer); cmpli; cmpli = PPC_INS_IPREV(cmpli))
      {

#if VERBOSE_SWITCH
        VERBOSE(0,("Looking Compare instructions: @I",cmpli)); 
#endif 

        if ((PPC_INS_OPCODE(cmpli) == PPC_CMPLI) && 
            (PPC_INS_REGT(cmpli)==condreg))	
        { 

#if VERBOSE_SWITCH
          VERBOSE(0,("Is cmpli and condred: %s",PpcRegisterName(PPC_INS_REGT(cmpli)))); 
#endif 
          break;
        }

        RegsetSetUnion(changed, PPC_INS_REGS_DEF(cmpli));

        if (PpcInsIsControlTransfer(T_INS(cmpli)) || RegsetIn(changed,condreg))
        {
#if VERBOSE_SWITCH
          VERBOSE(0,("Or is a control transfer or changes de condition register"));
#endif 
          cmpli = NULL;
          break;
        }
      }
      if (!cmpli) continue;

#if VERBOSE_SWITCH
      VERBOSE(0,("Compare instruction: @I",cmpli)); 
#endif 

      /* Get jump table bounds from the compare intruction */ 
      bound = PPC_INS_IMMEDIATE(cmpli);

#if VERBOSE_SWITCH
      VERBOSE(0,("Switch bounds: @G",bound));
#endif 
      /* Check of size of the table */
      ASSERT(AddressIsGe (SECTION_CSIZE(T_SECTION(RELOC_TO_RELOCATABLE(jumptablerel)[0])), AddressMulUint32 (AddressAddUint32 (bound, 1), 4)), ("jump table too small for switch bound"));
      /* Add the edges usign the relocation information */
#if VERBOSE_SWITCH
      VERBOSE(0,("Table bounds : @G - @G",
                 AddressAdd(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],RELOC_ADDENDS(jumptablerel)[0]),
                 AddressAdd(AddressAdd(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0],AddressMulUint32(bound,4)),RELOC_ADDENDS(jumptablerel)[0])));
#endif 
        for (rr = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(jumptablerel)[0]); rr; rr = RELOC_REF_NEXT(rr))
        {
          rel = RELOC_REF_RELOC(rr);
          ASSERT(RELOC_N_TO_RELOCATABLES(rel) == 1, ("Weird relocation found in jump table!"));
#if VERBOSE_SWITCH
          VERBOSE(0,("Checking if we can add an edge... relocation @R",rel)); 
          VERBOSE(0,("Rel from offset: @G",RELOC_FROM_OFFSET(rel))); 
          VERBOSE(0,("Rel to offset: @G",RELOC_TO_RELOCATABLE_OFFSET(rel)[0])); 
#endif 

          if
            (AddressIsGe(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]) &&
             AddressIsLe(RELOC_FROM_OFFSET(rel),AddressAdd(RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0], AddressMulUint32 (bound, 4))))
            {
              t_cfg_edge * edge;
              RELOC_SET_HELL(rel, FALSE);
              ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL,("@R should point to bbl",rel));
              edge =
                CfgEdgeCreate(cfg,PPC_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),ET_SWITCH);
              CFG_EDGE_SET_REL(edge,  rel);
              RELOC_SET_SWITCH_EDGE(rel, edge);
              CFG_EDGE_SET_SWITCHVALUE(edge,  G_T_UINT32(AddressSub(RELOC_FROM_OFFSET(rel),RELOC_TO_RELOCATABLE_OFFSET(jumptablerel)[0]))/4);

#if VERBOSE_SWITCH
              VERBOSE(0,("Adding edge: @E",edge)); 
#endif 
            }
        }
    }
    /* }}} */
    STATUS(STOP,("Detecting switches")); 
    /* }}} */

    /* Add Control Flow Edges (1. for relocs 2. for the branches ){{{*/

    STATUS(START,("Adding control-flow edges")); 
    /* walk through the original(!) array (we need offsets) and add 
     * all possible successors to every node */
    SECTION_FOREACH_PPC_INS(section, ins)
    {
      /* we're only interested in the first and last instructions of a basic block */
      if (!(PPC_INS_ATTRIB(ins) & IF_BBL_LEADER)) 
      {
        continue;
      }

      /* Initialize variables */
      block_start = ins;
      block = PPC_INS_BBL(block_start);
      block_end = block_start;

      /* Look for block end */ 
      for (iter = PPC_INS_INEXT(block_start); 
           iter && !(PPC_INS_ATTRIB(iter) & IF_BBL_LEADER); 
           iter = PPC_INS_INEXT(iter))
      {
        block_end = iter;
      }

      /* block_end now points to the last "real" instruction of the block in the linear list,
       * iter points to the first instruction of the next block (or NULL if there is no 
       * next block) */
      ft_block = iter ? PPC_INS_BBL(iter) : NULL;

      /* 1. ADD EDGES FOR RELOCATIONS {{{
       * 
       * each relocation gets its own edge. These edges will be automatically
       * deleted when the relocation is removed. There's no need to add an edge
       * for the obvious relocations (such as the relocations that are present
       * for the calculation of the offsets in intermodular jumps), since they
       * are completly described by the edge itself (the reason for the edge is
       * that there is a jump, not that there is a relocation) 
       */
      for (ref = BBL_REFED_BY(block); ref != NULL; ref = RELOC_REF_NEXT(ref))
      {
        t_bool add_edge = FALSE;

        if (PPC_INS_TYPE(block_start) != IT_DATA)
        {
          if (RELOC_HELL(RELOC_REF_RELOC(ref)))
          {
            add_edge = TRUE;
          }
        }

        /* Skip the obvious relocations and reloctions to data */
        if (add_edge)
        {
          /* Some extra checks if the relocation is from an instruction */
          if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(RELOC_REF_RELOC(ref))) == RT_INS)
          {
            t_cfg_edge *found = NULL;
            t_cfg_edge *i_edge;
            if (RELOC_REF_NEXT(RELOCATABLE_REFERS_TO(RELOC_FROM(RELOC_REF_RELOC(ref)))))
            {
              VERBOSE(0, ("1. @R\n", RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(RELOC_FROM(RELOC_REF_RELOC(ref))))));
              VERBOSE(0, ("2. @R\n", RELOC_REF_RELOC(RELOC_REF_NEXT(RELOCATABLE_REFERS_TO(RELOC_FROM(RELOC_REF_RELOC(ref)))))));
              FATAL(("Multiple relocs from instruction?"));
            }
            if ((RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(RELOC_FROM(RELOC_REF_RELOC(ref)))))!=RELOC_REF_RELOC(ref))
            {
              FATAL(("Relocation wrong"));
            }

            BBL_FOREACH_PRED_EDGE(block, i_edge)
            {
              if ((CFG_EDGE_HEAD(i_edge) == CFG_HELL_NODE(cfg)) 
                  && (CFG_EDGE_CAT(i_edge) == ET_CALL))
              {
                found = i_edge;
                break;
              }
            }

            if (found)
            {
              RELOC_SET_EDGE(RELOC_REF_RELOC(ref), found);
              CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found) + 1);
              /* also increment the refcount of the corresponding return edge!!! */
              ASSERT(CFG_EDGE_CORR(found), ("Call edge @E does not have a corresponding edge!", found));
              CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(found)) + 1);
            }
            else
            {
              RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), block, NULL, NULL));
              nedges++;
            }
            /* Some comments on this */
#if VERBOSE_NORMAL
            VERBOSE(0, ("Adding INS arrow from hell! (relocation @R, @B)", RELOC_REF_RELOC(ref), block));
#endif
          }
          else
          {
            t_cfg_edge *found = NULL;
            t_cfg_edge *i_edge;
            /* Add the edge */
            BBL_FOREACH_PRED_EDGE(block, i_edge)
            {
              if ((CFG_EDGE_HEAD(i_edge) == CFG_HELL_NODE(cfg)) && (CFG_EDGE_CAT(i_edge) == ET_CALL))
              {
                found = i_edge;
                break;
              }
            }

            if (found)
            {
              RELOC_SET_EDGE(RELOC_REF_RELOC(ref), found);
              CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found) + 1);
              /* also increment the refcount of the corresponding return edge!!! */
              ASSERT(CFG_EDGE_CORR(found), ("Call edge @E does not have a corresponding edge!", found));
              CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(found)) + 1);
            }
            else
            {
              RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), block, NULL, NULL));
              nedges++;
            }
#if VERBOSE_NORMAL 
            VERBOSE(0, ("Adding arrow from hell! (relocation @R, @B)", RELOC_REF_RELOC(ref), block));
#endif
          }
        }
      }
      /* }}} */

      /* 2. ADD NORMAL EDGES {{{
       * now determine the successors of the block and their types
       */

      switch (PPC_INS_TYPE(block_end))
      {
        case IT_BRANCH:
          /* If the branch is conditional, add a fall-through edge */
          if (ft_block != NULL 
              && PpcInsIsConditional (block_end))
          {
#if VERBOSE_NORMAL
            VERBOSE(0,("Adding a fallthrough edge from @B to @B",block,ft_block));
#endif
            CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
            nedges++;
          }

          if ((PPC_INS_OPCODE(block_end) == PPC_B ) ||
              (PPC_INS_OPCODE(block_end) == PPC_BC ))
          {
            /* Compute the target address */ 
            if(PpcInsAbsoluteAddress (block_end))
            {
              /* Target address is absolute */
              target_address = PPC_INS_IMMEDIATE(block_end);
            }
            else
            {
              /* Target address is relative */
              target_address = AddressAdd (PPC_INS_CADDRESS(block_end),
                                           PPC_INS_IMMEDIATE(block_end));
            }
            /* Get instruction target */     
            target = T_PPC_INS(ObjectGetInsByAddress(obj, target_address));

            if (target)      
            {
              /* Add a jump edge to the target */
              if(PpcInsUpdatesLinkRegister (block_end) && ft_block)
              {
                /* if the branch updates the link register, we assume it's
                 * actually a procedure call instead of a simple branch */
#if VERBOSE_NORMAL
                VERBOSE(0,("Adding call edge from @B to @B wirth fall: @B",block,PPC_INS_BBL(target),ft_block));
#endif
                CfgEdgeCreateCall(cfg, block,
                                  PPC_INS_BBL(target),ft_block,
                                  NULL);
              }
              else
              {
                /* regular branch, not procedure call */
#if VERBOSE_NORMAL
                VERBOSE(0,("Adding jump edge from @B to @B",block,PPC_INS_BBL(target)));
#endif
                CfgEdgeCreate (cfg, block, PPC_INS_BBL(target), ET_JUMP);
              }
              nedges++;
            }
            else
            {
              t_object * lo = ObjectGetLinkerSubObject(obj);	
              t_section * sec = SectionGetFromObjectByAddress(lo, target_address);

              if (AddressIsEq(target_address, AddressNew32(0x10000000)))
              {
                /* target address 0x10000000 implies a jump to a weak undefined
                 * symbol. This means that the code path on which this jump lies
                 * will never be followed during execution.  Therefore, we can
                 * safely replace this with an infinite loop. */
                CfgEdgeCreate (cfg, block, block, ET_JUMP);
              }
              else if ((sec)&&(StringPatternMatch("JMP_SLOT:*", SECTION_NAME(sec))))
              {
                t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 9);
                t_section * rel  =  SectionGetFromObjectByName(lo, relname);                
                ASSERT(rel, ("Relocation section (.rela.plt) for %s not found", SECTION_NAME(sec) + 9));
                {
                  t_bbl *dyncallhell = CfgGetDynamicCallHell(cfg, SECTION_NAME(sec) + 9);
                  /* also create a call edge here for plain jumps to plt entries,
                   * otherwise this bbl won't be marked as a CALL_HELL and liveness
                   * analysis goes wrong
                   */
                  CfgEdgeCreateCall(cfg, block, dyncallhell, ft_block, NULL);
                }
              }
              else if (PPC_INS_FLAGS(block_end) & PPC_FL_GOT_PRODUCER)
              {
                /* GOT_PRODUCER instructions perform a procedure call to the start
                 * of the GOT section. The first word there is a blrl instruction,
                 * which immediately returns and leaves the address of the first
                 * GOT entry in the link register.
                 * As the GOT section is considered a data section, the blrl
                 * instruction is not disassembled and hence the flowgrapher will
                 * not find the destination of the procedure call instruction.
                 *
                 * We model this by just adding a hell edge, and making sure the
                 * relocation that points the call target to the GOT section is
                 * not killed. */
                CfgEdgeCreateCall (cfg, block, CFG_HELL_NODE(cfg), ft_block, NULL);
              }
              else
              {
                FATAL(("jump to unknown destination @I", block_end));
              }
            }
          }
          else if (PPC_INS_OPCODE(block_end) == PPC_BCCTR ) 
          {

            /* Check first if we have already detected this branch as a switch
             * bracnh or an indirect call branch */

            is_detected = FALSE;

            BBL_FOREACH_SUCC_EDGE(block,edge)
              if ((CFG_EDGE_CAT(edge) == ET_SWITCH) || 
                  (CFG_EDGE_CAT(edge) == ET_CALL) ||
                  (CFG_EDGE_CAT(edge) == ET_JUMP))
              {
#if VERBOSE_NORMAL
                VERBOSE(0,("Branch to CTR: Is already detected"));
#endif
                is_detected = TRUE;
                break;
              }

            if(PpcInsUpdatesLinkRegister (block_end) && !is_detected && ft_block)
            {
#if VERBOSE_NORMAL
              VERBOSE(0,("Branch to CTR: Adding call edge to hell from @B and fallthrough @B", block,ft_block));
#endif
              CfgEdgeCreateCall(cfg, block,CFG_HELL_NODE(cfg) ,ft_block,NULL);
#if VERBOSE_PATTERNS
              VERBOSE(0,("Pattern candidate: @ieB",block));
#endif
              nedges++;
            }
            else if (!is_detected)
            {
#if VERBOSE_NORMAL
              VERBOSE(0,("Branch to CTR: Adding jump edge to hell from @B",block));
#endif
              CfgEdgeCreate (cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
#if VERBOSE_PATTERNS
              VERBOSE(0,("Pattern candidate: @ieB",block));
#endif
              nedges++;
            }

          }
          else if (PPC_INS_OPCODE(block_end) == PPC_BCLR )
          {
            is_detected = FALSE;

            BBL_FOREACH_SUCC_EDGE(block,edge)
              if ((CFG_EDGE_CAT(edge) == ET_SWITCH) || 
                  (CFG_EDGE_CAT(edge) == ET_CALL) ||
                  (CFG_EDGE_CAT(edge) == ET_JUMP))
              {
#if VERBOSE_NORMAL
                VERBOSE(0,("Branch to LR: Is already detected"));
#endif
                is_detected = TRUE;
                break;
              }
            if(PpcInsUpdatesLinkRegister (block_end) && !is_detected)
            {
              if(ft_block)
              {
#if VERBOSE_NORMAL
                VERBOSE(0,("Branch to LR: Adding call edge to hell from @B and fallthrough @B", block,ft_block));
#endif
                CfgEdgeCreateCall(cfg, block,CFG_HELL_NODE(cfg) ,ft_block,NULL);
#if VERBOSE_PATTERNS
                VERBOSE(0,("Pattern candidate: @ieB",block));
#endif
              }
              else
              {
#if VERBOSE_NORMAL
                VERBOSE(0,("Branch to LR: Adding jump edge to hell!"));
#endif
                CfgEdgeCreate (cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
#if VERBOSE_PATTERNS
                VERBOSE(0,("Pattern candidate: @ieB",block));
#endif
              }
            }
            else if (!is_detected)
            {
#if VERBOSE_NORMAL
              VERBOSE(0,("Branch to LR: Adding return edge to hell from @B",block));
#endif
              CfgEdgeCreate (cfg, block, CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
            }
            nedges++;
          }
          else
          {
            FATAL((" %s : Unknown Branch type.", __func__ ));
          }
          break;
        case IT_SWI:
          /* If the trap is conditional, add a fall-through edge */
          if (ft_block != NULL  
              && PpcInsIsConditional (block_end))
          {
            CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
            nedges++;
#if VERBOSE_NORMAL
            VERBOSE(0,("SWI: Adding fallthrought, ins is conditional from @B to @B",block, ft_block));
#endif
          }

          /* If is an exit system call, it does not have FALLTHROUHG block*/
          if(PpcInsIsSyscallExit(T_INS(ins))==YES) 
          {
            ft_block = NULL; 
#if VERBOSE_NORMAL
            VERBOSE(0,("SWI: Removing fallthrought, is a Syscall Exit block! \n @B",block));
#endif
          }

          /* since we don't know where a SWI jumps to, we'll add hell as a successor */
          CfgEdgeCreateSwi (cfg, block, ft_block);
#if VERBOSE_NORMAL
          VERBOSE(0,("SWI : Adding SWI edge from @B", block));
#endif
          nedges++;
          break;

          /* Ignore data */
        case IT_DATA:
          break;
        default:
          /* If exists next block add a fall-through edge */
          if (ft_block != NULL )
          {
            CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
#if VERBOSE_NORMAL
            VERBOSE(0,("Adding fallthrough from @B to @B",block,ft_block));
#endif
            nedges++;
          }
          break;
      }
      /* }}} */
    } 
    STATUS(STOP,("Adding control-flow edges")); 
    /*}}}*/

    /* {{{ Finally detect indirect control-flow and fix the necessary edges.
     * we don't have to do this, but otherwise a lot of hell edges will
     * be added that substantially degrade the accuracy and performance of a 
     * number of analyses */
    STATUS(START,("Detecting indirect control-flow")); 
    /* indirect control-flow {{{ */

#if VERBOSE_INDIRECT_COUNT
    t_uint32 pattern1_count, pattern2_count;
    pattern1_count = 0;
    pattern2_count = 0;
#endif

    for (prev = prev_controltransfer = NULL, ins = SECTION_DATA(section); ins; ins = PPC_INS_INEXT(ins))
    {
      /* indirect control-flow constructs could have the following patterns:
       *
       *   PATTERN1
       *
       *  lis %reg, imm
       *  ...
       *  addi %reg, imm
       *  ...
       *  mtspr %reg
       *  ...
       *  bcctr/bclr
       *
       *  ------------------------------
       *
       *   PATTERN2
       *
       *   lis %reg1, imm 
       *   ...
       *   lwz %reg2, %reg1, imm
       *   ...
       *   mtspr %reg
       *   ...
       *   bcctr/bclr 
       *
       */

      t_ppc_ins *mtspr, *def, *lis, *target_ins;
      t_address source, target;
      t_reloc_ref * rr;
      t_reloc * sourcerel;
      t_cfg_edge *edge, *edge2;
      t_uint32 count;
      t_bbl * block;
      t_bbl * ft_block;

      if (PPC_INS_IPREV(ins))
      {
        prev = PPC_INS_IPREV(ins);
      }

      if (prev && PpcInsIsControlTransfer(T_INS(prev)) &&
          !PpcInsIsConditional(ins))
      {
        prev_controltransfer = prev;
      }

      /* looking for candidates, go next until:
       * - previous control transfer not conditional
       * - actual ins is a branch to lr or ctr
       */
#if VERBOSE_INDIRECT
      if (prev_controltransfer)
      {
        VERBOSE(0,("Previous control transfer is: @I",prev_controltransfer));
      }
      else
      {
        VERBOSE(0,("No previous control transfer"));
      }
#endif

      if ((PPC_INS_OPCODE(ins) != PPC_BCCTR) && (PPC_INS_OPCODE(ins) != PPC_BCLR)) continue;
#if VERBOSE_INDIRECT
      VERBOSE(0,("Actual instruction is BCCTR or BCLR"));
#endif

      /* Now, we delimited the window where to 
       * look the pattern.
       */

#if VERBOSE_INDIRECT
      VERBOSE(0,("Candidate found!"));
#endif

      /* look for the mtspr instruction */
      for (iter = PPC_INS_IPREV(ins); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
      {
        if((PPC_INS_OPCODE(iter)==PPC_MTSPR) &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_REG_CTR) &&
           (PPC_INS_OPCODE(ins)==PPC_BCCTR))
        {
          mtspr = iter;
          break;
        }

        if((PPC_INS_OPCODE(iter)==PPC_MTSPR) &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_REG_LR) &&
           (PPC_INS_OPCODE(ins)==PPC_BCLR))
        {
          mtspr = iter;
          break;
        }

      }

      if(iter == prev_controltransfer) continue;

#if VERBOSE_INDIRECT
      VERBOSE(0,("mtspr found: @I",mtspr));
      VERBOSE(0,("register used: %s",PpcRegisterName(PPC_INS_REGA(mtspr))));
#endif

      /* cases for calculation:
       * a)
       *  - offset table: lis <high target address>
       *  - addi <low target address>
       * b)
       *  - lis <high source address>
       *  - lwz <high source address> + imm
       */

      for (iter = PPC_INS_IPREV(mtspr); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
      {
        if((PPC_INS_OPCODE(iter)==PPC_LWZ)
           &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_INS_REGA(mtspr)))
        {
          def = iter;
          break;
        }
        else if((PPC_INS_OPCODE(iter)==PPC_ADDI)
                &&
                RegsetIn(PPC_INS_REGS_DEF(iter),PPC_INS_REGA(mtspr)))
        {
          def = iter;
          break;
        }
        else if (RegsetIn(PPC_INS_REGS_DEF(iter),PPC_INS_REGA(mtspr)))
        {
#if VERBOSE_INDIRECT
          VERBOSE(0,("Looking for addi/lwz but the instruccion that defines the reg is: @I",iter));
#endif
          iter = prev_controltransfer;
          break;
        }
      }

      if(iter == prev_controltransfer) continue;

#if VERBOSE_INDIRECT
      VERBOSE(0,("Definition found: @I",def));
#endif
      /* The instructions have 1 source register :
       * REG, it should contain the target_address
       * base value. Afterwards, we add the displa-
       * cement of the instruction
       */

      /* Even the def is an addi or a lwz, we have to
       * find the lis instruction */

      for (iter = PPC_INS_IPREV(def); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
      {
        if(((PPC_INS_OPCODE(iter)==PPC_ADDIS)&&(PPC_INS_REGA(iter)==0))
           &&
           RegsetIn(PPC_INS_REGS_DEF(iter),PPC_INS_REGA(def)))
        {
          lis = iter;
          break;
        }
        else if (RegsetIn(PPC_INS_REGS_DEF(iter),PPC_INS_REGA(def)))
        {
#if VERBOSE_INDIRECT
          VERBOSE(0,("Looking for lis but the instruccion that defines the reg is: @I",iter));
#endif
          iter = prev_controltransfer;
          break;
        }
      }

      if(iter == prev_controltransfer) continue;

#if VERBOSE_INDIRECT
      VERBOSE(0,("Definition2 found: @I",lis));
#endif

      if(PPC_INS_OPCODE(def)==PPC_ADDI)
      {
        target = AddressAdd (PPC_INS_IMMEDIATE(lis), PPC_INS_IMMEDIATE(def));
#if VERBOSE_INDIRECT
        VERBOSE(0,("Target address of the indirect jump: @G",target));
#endif
#if VERBOSE_INDIRECT_COUNT
        pattern1_count++;
#endif
      }
      else if(PPC_INS_OPCODE(def)==PPC_LWZ)
      {
        rr = PPC_INS_REFERS_TO(PPC_INS_COPY(def));
#if VERBOSE_INDIRECT
        VERBOSE(0,("Reloc : @R", RELOC_REF_RELOC(rr)));
#endif
        if (!rr) 
        {
          FATAL(("Could not find target relocation for @I",def));
        }

        sourcerel = RELOC_REF_RELOC(rr);

        if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(sourcerel)[0]) != RT_SUBSECTION) 
        {
#if VERBOSE_INDIRECT
          VERBOSE(0,("Relocation does not point to a (data) subsection!"));
#endif
          continue;
        }
        if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(sourcerel)[0])) != RODATA_SECTION)
        {
#if VERBOSE_INDIRECT
          VERBOSE(0,("Relocation points to a section called: %s",SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(sourcerel)[0]))));
#endif
          continue;
        }

        source = AddressAdd (PPC_INS_IMMEDIATE(lis), PPC_INS_IMMEDIATE(def));

#if VERBOSE_INDIRECT
        VERBOSE(0,("Source address: @G", source));
#endif

        if (AddressIs32 (source))
        {
          target = AddressNew32 (ObjectGetData32 (obj, SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(sourcerel)[0])), source));
        }
        else if (AddressIs64 (source))
        {
          target = AddressNew64 (ObjectGetData64 (obj, SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(sourcerel)[0])), source));
        }
        else
        {
          FATAL(("Unsupported address type"));
        }

#if VERBOSE_INDIRECT_COUNT
        pattern2_count++;
#endif
#if VERBOSE_INDIRECT
        VERBOSE(0,("Target address : @G",target));
#endif
      }
      else
      {
#if VERBOSE_INDIRECT
        VERBOSE(0,("Implement this case of definition: @I", def));
#endif
        continue;
      }

      target_ins=T_PPC_INS(ObjectGetInsByAddress(obj,  target));

      if(!target_ins)
      {
        FATAL(("Found pattern without target instruccion! Target @G",target)); 
      }
      
      count = 0;
      block = PPC_INS_BBL(ins);

#if 0 /* Enable to get extra info */ 
      for (ref = BBL_REFED_BY(T_BBL(PPC_INS_BBL(target_ins))); ref != NULL; ref = RELOC_REF_NEXT(ref))
      {
        VERBOSE(0,("Target refed by: @Bie",T_BBL(PPC_INS_BBL(T_PPC_INS(RELOC_FROM(RELOC_REF_RELOC(ref))))))); 
      }

      BBL_FOREACH_PRED_EDGE(PPC_INS_BBL(target_ins), edge2)
      {
        VERBOSE(0,("Predecessor edge @E",edge2));
      }
#endif

      /* Remove the edge to hell created during the previous step */
      BBL_FOREACH_SUCC_EDGE_SAFE(block,edge,edge2)
      {
#if VERBOSE_INDIRECT
        VERBOSE(0,("Removing this edge: @E ",edge)); 
#endif
        if(CFG_EDGE_CORR(edge))
        {
#if VERBOSE_INDIRECT2
          VERBOSE(0,("References of the corresponding return edge: %d ",CFG_EDGE_REFCOUNT(edge))); 
#endif
          CfgEdgeKill(CFG_EDGE_CORR(edge));
        }
#if VERBOSE_INDIRECT2
        VERBOSE(0,("References of the edge: %d ",CFG_EDGE_REFCOUNT(edge))); 
#endif
        CfgEdgeKill(edge);
        count++;
      }

      if(count>1)
      {
        FATAL(("We should remove only one edge!"));
      }

      ft_block = PPC_INS_INEXT(ins) ? PPC_INS_BBL(PPC_INS_INEXT(ins)) : NULL ;

#if 0 /* Enable to get extra info */
      for (ref = BBL_REFED_BY(T_BBL(PPC_INS_BBL(target_ins))); ref != NULL; ref = RELOC_REF_NEXT(ref))
      {
        VERBOSE(0,("Target refed by: @Bie",T_BBL(PPC_INS_BBL(T_PPC_INS(RELOC_FROM(RELOC_REF_RELOC(ref))))))); 
      }

      BBL_FOREACH_PRED_EDGE(PPC_INS_BBL(target_ins), edge2)
      {
        VERBOSE(0,("Predecessor edge @E",edge2));
      }
#endif

      if(PpcInsUpdatesLinkRegister (ins) && ft_block )
      {
#if VERBOSE_INDIRECT
        VERBOSE(0,("Creating edge from @Bie to @Bie with fallthrough: @Bie",PPC_INS_BBL(ins),T_BBL(PPC_INS_BBL(target_ins)),ft_block));
#endif
        edge = CfgEdgeCreateCall(cfg,PPC_INS_BBL(ins),T_BBL(PPC_INS_BBL(target_ins)),ft_block,NULL);
      }
      else
      {
#if VERBOSE_INDIRECT
        VERBOSE(0,("Creating edge from @Bie to @Bie without fallthrough",PPC_INS_BBL(ins),T_BBL(PPC_INS_BBL(target_ins))));
#endif
        edge = CfgEdgeCreate(cfg,PPC_INS_BBL(ins),T_BBL(PPC_INS_BBL(target_ins)),ET_JUMP);
      }

    }

#if VERBOSE_INDIRECT_COUNT
    VERBOSE(0,("Pattern1 occurrences: %d",pattern1_count));
    VERBOSE(0,("Pattern2 occurrences: %d",pattern2_count));
#endif

    /* }}} */
    STATUS(STOP,("Detecting indirect control-flow")); 
    /* }}} */
  }

  return nedges;
}
/* }}} */


/* PpcRemoveGot2OffsetLoads {{{ */
/*!
 * Removes the instructions which load a got2 offset from code compiled
 * with -mrelocatable (the data identified by .LCL0 in the introductory
 * comment of DiabloFlowgraphPpcCfgCreated), and replaces it and the
 * next add by an immediate addition of that value. This avoids us
 * having to ensure that the memory location of this offset remains
 * within 32KiB of the load instruction.
 *
 * \param obj The object we are flowgraphing
 *
 * \return void
 */
static void PpcRemoveGot2OffsetLoads(t_object *obj)
{
  t_cfg * cfg = OBJECT_CFG(obj);
  t_ins * tins;
  
  /* we're looking for the following pattern:
   *      .section        ".got2","aw"
   *  .LCTOC1 = .+32768
   *       ...
   *      .section        .rodata.cst8,"aM",@progbits,8
   *      .align 3
   *      .LC0:
   *      .long   1127219200
   *      .long   -2147483648
   *      .section        ".got2","aw"
   *      .LC1:
   *      .long .LC0
   *      ...
   *      .section        ".text"
   *      .align 2
   *      .globl floatdidf
   *  .LCL0:
   *      .long .LCTOC1-.LCF0
   *      .type   floatdidf, @function
   *  floatdidf:
   *      ...
   *      bcl 20,31,.LCF0
   *  .LCF0:
   *      mflr 30
   *      lwz 0,.LCL0-.LCF0(30)
   *      add 30,0,30
   *
   * So a "bcl 20,31" to the next instruction, an mflr to load the pc,
   * a pc-relative load of an offset calculating the difference between
   * the pc and the .got2 section, and finally the "add" to finish the
   * calculation of the .got2 address.
   *
   */

  CFG_FOREACH_INS(cfg,tins)
  {
    t_ppc_ins *ins, *bcl, *mflr, *lwz, *add, *offsetdata, *temp;
    t_reloc *rel, *got2datarel;
    t_reloc_ref *rr;
    t_reg pcreg, got2offsetreg;
    t_address calcaddr;
    t_bool found;
    
    ins = T_PPC_INS(tins);
    if (!PpcInsIsPicBcl(ins))
      continue;

    /* We've found a bcl to the next instruction, now search for the mflr */
    bcl = ins;
    ins = PPC_INS_INEXT(bcl);
    while (ins &&
           !RegsetIn(PpcUsedRegisters(ins),PPC_REG_LR) &&
           !RegsetIn(PpcDefinedRegisters(ins),PPC_REG_LR))
      ins=PPC_INS_INEXT(ins);
    if (!ins ||
        (PPC_INS_OPCODE(ins) != PPC_MFSPR) ||
        (PPC_INS_REGA(ins) != PPC_REG_LR))
    {
      WARNING (("found bclr to next instruction, can't find the mflr."
                "Treating as a genuine call.\n@I",bcl));
      continue;
    }
    mflr = ins;
    pcreg = PPC_INS_REGT(mflr);

    /* look for a lwz of the got2 offset relative to the PC */
    do
    {
      ins = PPC_INS_INEXT(ins);
      found =
        (ins &&
         !RegsetIn(PpcDefinedRegisters(ins),pcreg) &&
         (PPC_INS_OPCODE(ins) == PPC_LWZ) &&
         (NULL != ((rr = PPC_INS_REFERS_TO(ins)))) &&
         (NULL != ((rel = RELOC_REF_RELOC(rr)))) &&
         (RELOC_N_TO_RELOCATABLES(rel) == 2) &&
         (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL) &&
         (NULL != ((offsetdata = T_PPC_INS(ObjectGetInsByAddress(obj, AddressAdd(RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[0]),RELOC_TO_RELOCATABLE_OFFSET(rel)[0])))))) &&
         (PPC_INS_TYPE(offsetdata) == IT_DATA) &&
         (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[1]) == RT_BBL) &&
         (NULL != ((temp = T_PPC_INS(ObjectGetInsByAddress(obj, AddressAdd(RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[1]),RELOC_TO_RELOCATABLE_OFFSET(rel)[1])))))) &&
         (PPC_INS_COPY(temp) == bcl));
    } while (ins &&
             !found);

    if (!found)
      continue;
    lwz = ins;
    got2offsetreg = PPC_INS_REGT(lwz);

    /* ensure the data is indeed the offset between the .got2 and the bcl */
    if ((NULL == ((rr = PPC_INS_REFERS_TO(PPC_INS_COPY(offsetdata))))) ||
        (NULL == ((rel = RELOC_REF_RELOC(rr)))) ||
        (RELOC_N_TO_RELOCATABLES(rel) != 2) ||
        (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) != RT_SUBSECTION) ||
        (strcmp(SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(rel)[0])),".got2")) ||
        (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[1]) != RT_BBL) ||
        (NULL == ((temp = T_PPC_INS(ObjectGetInsByAddress(obj, AddressAdd(RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[1]),RELOC_TO_RELOCATABLE_OFFSET(rel)[1])))))) ||
        (PPC_INS_COPY(temp) != bcl))
      continue;
    got2datarel=rel;

    /* look for the adding of the got2 offset to the calculated pc */  
    found=FALSE;
    do
    {
      ins = PPC_INS_INEXT(ins);
      found =
        (ins &&
         (PPC_INS_OPCODE(ins) == PPC_ADD) &&
         (PPC_INS_REGT(ins) == pcreg) &&
         (PPC_INS_REGA(ins) == got2offsetreg) &&
         (PPC_INS_REGB(ins) == pcreg));
      if (!found &&
          ins &&
          (RegsetIn(PpcDefinedRegisters(ins),pcreg) ||
           RegsetIn(PpcUsedRegisters(ins),got2offsetreg) ||
           RegsetIn(PpcDefinedRegisters(ins),got2offsetreg)))
        break;
    } while (ins &&
             !found);
   if (!found)
      continue;
   add = ins;

#if 1
   /* replace the
    *    lwz  got2offsetreg,x(pcreg)
    *    add  pcreg,got2reg,pcreg
    * with
    *    addi  pcreg,pcreg,got2offst@lo16
    *    addis pcreg,pcreg,got2offst@hi16
    * The resulting code is semantically identical to the original code,
    * but no longer requires the annoying offsetdata (it will be killed
    * by dead code & data removal) -- it's annoying because it has to
    * remain within 32kb of the lwz instruction
    */

   ins = lwz;
   ASSERT(pcreg!=PPC_REG_R0,("pcreg cannot be r0, because then the addi/addis will become li/lis"));
   PpcInsMakeAddi(ins,pcreg,pcreg,AddressExtractInt32(PPC_INS_IMMEDIATE(offsetdata)) & 0xffff);

   rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj),got2datarel);
   Free(RELOC_CODE(rel));
   RELOC_SET_CODE(rel, StringDup("R00 R01A00+ - \\ sffff & s0010 < l sffff & | w \\ s0000$"));
   RelocSetFrom(rel,T_RELOCATABLE(ins));
   RELOC_SET_FROM_OFFSET(rel,AddressNewForIns(T_INS(ins),2));

   calcaddr=StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj);
   ASSERT(AddressIsEq(AddressSignExtend(AddressAnd(calcaddr,AddressNewForIns(T_INS(ins),0xffff)),15),
          PPC_INS_IMMEDIATE(ins)),("Calculated relocation wrong for pic li: expected @G, got @G for reloc\n\n@R\n",PPC_INS_IMMEDIATE(ins),
          AddressSignExtend(AddressAnd(calcaddr,AddressNewForIns(T_INS(ins),0xffff)),15),rel));

   ins = add;
   PpcInsMakeAddis(ins,pcreg,pcreg,(AddressExtractInt32(PPC_INS_IMMEDIATE(offsetdata))+0x8000) >> 16);

   rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj),got2datarel);
   Free(RELOC_CODE(rel));
   RELOC_SET_CODE(rel, StringDup("R00 R01A00+ - \\ i00008000+ iffff0000 & l sffff & | w \\ s0000$"));
   RelocSetFrom(rel,T_RELOCATABLE(ins));
   RELOC_SET_FROM_OFFSET(rel,AddressNewForIns(T_INS(ins),2));

   calcaddr=AddressAdd(calcaddr,AddressNewForIns(T_INS(ins),0x8000));
   calcaddr=AddressSignExtend(AddressShr(calcaddr,AddressNewForIns(T_INS(ins),16)),15);
   ASSERT(AddressIsEq(calcaddr,PPC_INS_IMMEDIATE(ins)),("Calculated relocation wrong for pic addis: expected @G, got @G for reloc\n\n@R\n",
          PPC_INS_IMMEDIATE(ins),calcaddr,rel));
#else
   /* replace the
    *    lwz  got2offsetreg,x(pcreg)
    *    add  pcreg,got2reg,pcreg
    * with
    *    li  pcreg,got2offst@lo16
    *    addis pcreg,pcreg,got2offst@hi16
    *
    * The earlier bcl/mflr should be killed by KillUselessInstructions. This
    * transformation makes the code no longer position-independent.
    */

   finalgot2address=AddressAddUint32(AddressAdd(PPC_INS_IMMEDIATE(offsetdata),PPC_INS_OLD_ADDRESS(bcl)),4);

   ins = lwz;
   PpcInsMakeLi(ins,pcreg,AddressExtractInt32(finalgot2address) & 0xffff);
   rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForIns(ins),
         T_RELOCATABLE(ins), AddressNewForIns(T_INS(ins),2),
         RELOC_TO_RELOCATABLE(got2datarel)[0], RELOC_TO_RELOCATABLE_OFFSET(got2datarel)[0],
         TRUE,  NULL, NULL, NULL, "R00A00+ \\ sffff & s0010 < l sffff & | w \\ s0000$" );
   calcaddr=StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj);
   ASSERT(AddressIsEq(AddressSignExtend(AddressAnd(calcaddr,AddressNewForIns(T_INS(ins),0xffff)),15),
          PPC_INS_IMMEDIATE(ins)),("Calculated relocation wrong for pic li: expected @G, got @G for reloc\n\n@R\n",PPC_INS_IMMEDIATE(ins),
          AddressSignExtend(AddressAnd(calcaddr,AddressNewForIns(T_INS(ins),0xffff)),15),rel));

   ins = add;
   PpcInsMakeAddis(ins,pcreg,pcreg,(AddressExtractInt32(finalgot2address)+0x8000) >> 16);
   rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForIns(ins),
         T_RELOCATABLE(ins), AddressNewForIns(T_INS(ins),2),
         RELOC_TO_RELOCATABLE(got2datarel)[0], RELOC_TO_RELOCATABLE_OFFSET(got2datarel)[0],
         TRUE,  NULL, NULL, NULL, "R00A00+ \\ i00008000+ iffff0000 & l sffff & | w \\ s0000$" );
   calcaddr=AddressAdd(calcaddr,AddressNewForIns(T_INS(ins),0x8000));
   calcaddr=AddressSignExtend(AddressShr(calcaddr,AddressNewForIns(T_INS(ins),16)),15);
   ASSERT(AddressIsEq(calcaddr,PPC_INS_IMMEDIATE(ins)),("Calculated relocation wrong for pic addis: expected @G, got @G for reloc\n\n@R\n",
          PPC_INS_IMMEDIATE(ins),calcaddr,rel));
#endif
  }
}
/* }}} */


/* PpcFlowgraph {{{ */
/*!
 * Probably the most important function in the backend: Create a flowgraph from
 * a list of disassembled ppc instructions. Works in 3 big steps:
 * - Does leader detection on the list of instructions to identify basic blocks
 * - Converts all position-dependent instructions to pseudo instructions
 * - Draws edges between the basic blocks
 *
 * \param section The (disassembled) section to flowgraph
 *
 * \return void 
 */

#define VERBOSE_DUMP_USE_DEF 0  // Show registers used/defined for each instruction
#define VERBOSE_DUMP_BBLS  0 // Dump all bbl after flowgraph construction

void PpcFlowgraph(t_object *obj)
{
  t_uint32 ret;
  t_cfg *cfg = OBJECT_CFG(obj);

  /* Find the leaders in the instruction list */
  STATUS(START,("Leader detection"));
  ret = PpcFindBBLLeaders (obj);
  STATUS(STOP,("Leader detection"));

  /* Create the basic blocks (platform independent) */
  STATUS(START,("Creating Basic Blocks"));
  ret = CfgCreateBasicBlocks (obj);
  STATUS(STOP,("Creating Basic Blocks"));

  /* Create the edges between basic blocks */
  STATUS(START,("Creating Basic Block graph"));
  ret = PpcAddBasicBlockEdges (obj);
  STATUS(STOP,("Creating Basic Block graph"));

  /* Remove the calls created by weak symbols 
   * TODO: it's a kind of optimization, should
   * be moved to anopt */
  STATUS(START,("Patch calls to weak symbols")); 
  ret = PpcPatchCallsToWeakSymbols (obj);
  STATUS(STOP,("Patch calls to weak symbols"));

  PpcRemoveGot2OffsetLoads(obj);

  /* {{{ kill all relocations from the immediate operands of jump and call instructions: 
   * they have no use any more because their information is represented by flow graph edges 
   * and also mark the operands that are relocated.*/
  {
    t_bbl * bbl;
    t_ppc_ins * ins;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      BBL_FOREACH_PPC_INS(bbl,ins)
      {
        if (PPC_INS_OPCODE(ins) == PPC_B || PPC_INS_OPCODE(ins) == PPC_BC)
        {
          t_bool dynamic = FALSE;
          t_reloc_ref *ref;
          
          /* don't throw away the relocation for GOT producers */
          if (PPC_INS_FLAGS(ins) & PPC_FL_GOT_PRODUCER)
            continue;

          /* don't throw away relocations for branches to plt entries either */
          for (ref=PPC_INS_REFERS_TO(ins); ref!= NULL; ref=RELOC_REF_NEXT(ref))
          {
            t_reloc * rel = RELOC_REF_RELOC(ref);
            t_uint32 i;
            
            for (i = 0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
            {
              t_relocatable * rb=RELOC_TO_RELOCATABLE(rel)[i];
              if (RELOCATABLE_RELOCATABLE_TYPE(rb) == RT_SUBSECTION)
              {
                t_section * sec = T_SECTION(rb);
                
                if (StringPatternMatch("JMP_SLOT:*", SECTION_NAME(sec)))
                {
                  dynamic = TRUE;
                  break;
                }
              }
              
            }
          }
          
          if (dynamic) continue;

          while (PPC_INS_REFERS_TO(ins))
          {
            RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(PPC_INS_REFERS_TO(ins)));
          }
          PPC_INS_SET_FLAGS(ins,PPC_INS_FLAGS(ins)&~PPC_FL_RELOCATED);
        }
      }
    }
  } /* }}} */

  /* {{{ Show USE/DEFINE information for debugging*/
#if VERBOSE_DUMP_USE_DEF
  {
    t_bbl * bbl;
    t_ppc_ins * ins;
    STATUS(START,("Dump REG USE/DEF for each ins"));
    CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_FOREACH_PPC_INS(bbl,ins)
      {
        VERBOSE(0,("---------"));
        VERBOSE(0,("Instr: @I",ins));
        VERBOSE(0,("  Use: @X", DPREGSET(CFG_DESCRIPTION(cfg),PPC_INS_REGS_USE(ins))));
        VERBOSE(0,("  Define: @X",DPREGSET(CFG_DESCRIPTION(cfg),PPC_INS_REGS_DEF(ins))));
      }
    }
    STATUS(STOP,("Dump REG USE/DEF for each ins"));
  } 
#endif
  /* }}} */

  /* {{{ Show BBL information for debugging*/
#if VERBOSE_DUMP_BBLS
  {
    t_bbl * bbl;
    STATUS(START,("Dumping all BBLs"));
    CFG_FOREACH_BBL(cfg,bbl)
    {
      VERBOSE(0,("---------"));
      VERBOSE(0,("BBL: @ieB\n",bbl));
    }
    STATUS(STOP,("Dumping all BBLs"));
  }
#endif
  /* }}} */
}
/* }}} */

/* PpcMakeAddressProducers {{{ */
/*!
 * Turns position-dependent instructions (i.e. instructions that use the pc)
 * into pseudo-operations that no longer depend on the program counter.
 *
 * \param code
 *
 * \return void 
 */
void
PpcMakeAddressProducers (t_cfg *cfg)
{
#if 0
  t_bbl * bbl, * next;
  t_ppc_ins * ins;
  
  /* we're looking for the following pattern:
    *    bcl 20,31 <next ins>
    *    <possibly some other instructions>
    *    mflr rA
    *
    * We can remove those two instructions and insert
    *    addis rA, 0, $addr >> 16 (= lis rA, $addr >> 16)
    *    ori   rA, rA, $addr & 0xffff
    *    mtlr  rA
    * instead. The last instruction will be removed by
    * KillUselessInstructions if it is unnecessery (which
    * it probably is) */
  
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_reg destreg;
    t_ppc_ins * bcl, * mflr;
    t_ppc_ins * load1, * load2;
    t_int32 offset;
    t_reloc * rel;
    
    
    ins = T_PPC_INS(BBL_INS_LAST(bbl));
    if (!ins || !PpcInsIsPicBcl(ins))
      continue;
    
    /* We've found the bcl to the next instruction, now search for the mtlr */
    bcl = ins;
    next = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
    ins = T_PPC_INS(BBL_INS_FIRST(next));
    while (ins &&
           !RegsetIn(PpcUsedRegisters(ins),PPC_REG_LR) &&
           !RegsetIn(PpcDefinedRegisters(ins),PPC_REG_LR))
      ins=PPC_INS_INEXT(ins);
    if (!ins ||
        (PPC_INS_OPCODE(ins) != PPC_MFSPR) ||
        (PPC_INS_REGA(ins) != PPC_REG_LR))
    {
      WARNING (("found bclr to next instruction, can't find the mflr."
                "Treating as a genuine call.\n@I",call));
      continue;
    }
    mflr = ins;
    
    PPC_INS_SET_TYPE(bcl,IT_CONSTS);
    PPC_INS_SET_TYPE(mflr,IT_CONSTS);
#endif
}
/* }}} */


/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
