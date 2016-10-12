/*
 * Copyright (C) 2006 Lluis Vilanova <xscript@gmx.net> {{{
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
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diablospe.h>

t_address SPE_LSLR;

static const char *names[] =
{
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
    "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19",
    "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29",
    "r30", "r31", "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47", "r48", "r49",
    "r50", "r51", "r52", "r53", "r54", "r55", "r56", "r57", "r58", "r59",
    "r60", "r61", "r62", "r63", "r64", "r65", "r66", "r67", "r68", "r69",
    "r70", "r71", "r72", "r73", "r74", "r75", "r76", "r77", "r78", "r79",
    "r80", "r81", "r82", "r83", "r84", "r85", "r86", "r87", "r88", "r89",
    "r90", "r91", "r92", "r93", "r94", "r95", "r96", "r97", "r98", "r99",
    "r100", "r101", "r102", "r103", "r104", "r105", "r106", "r107", "r108", "r109",
    "r110", "r111", "r112", "r113", "r114", "r115", "r116", "r117", "r118", "r119",
    "r120", "r121", "r122", "r123", "r124", "r125", "r126", "r127", "r128", "r129",
    "r130", "r131", "r132", "r133", "r134", "r135", "r136", "r137", "r138", "r139",
    "r140", "r141", "r142", "r143", "r144", "r145", "r146", "r147", "r148", "r149",
    "r150", "r151", "r152", "r153", "r154", "r155", "r156", "r157", "r158", "r159",
    "r160", "r161", "r162", "r163", "r164", "r165", "r166", "r167", "r168", "r169",
    "r170", "r171", "r172", "r173", "r174", "r175", "r176", "r177", "r178", "r179"
        ,
    "sp0", "sp1", "sp2", "sp3", "sp4", "sp5", "sp6", "sp7", "sp8", "sp9",
    "sp10", "sp11", "sp12", "sp13", "sp14", "sp15", "sp16", "sp17", "sp18", "sp19",
    "sp20", "sp21", "sp22", "sp23", "sp24", "sp25", "sp26", "sp27", "sp28", "sp29",
    "sp30", "sp31", "sp32", "sp33", "sp34", "sp35", "sp36", "sp37", "sp38", "sp39",
    "sp40", "sp41", "sp42", "sp43", "sp44", "sp45", "sp46", "sp47", "sp48", "sp49",
    "sp50", "sp51", "sp52", "sp53", "sp54", "sp55", "sp56", "sp57", "sp58", "sp59",
    "sp60", "sp61", "sp62", "sp63", "sp64", "sp65", "sp66", "sp67", "sp68", "sp69",
    "sp70", "sp71", "sp72", "sp73", "sp74", "sp75", "sp76", "sp77", "sp78", "sp79",
    "sp80", "sp81", "sp82", "sp83", "sp84", "sp85", "sp86", "sp87", "sp88", "sp89",
    "sp90", "sp91", "sp92", "sp93", "sp94", "sp95", "sp96", "sp97", "sp98", "sp99",
    "sp100", "sp101", "sp102", "sp103", "sp104", "sp105", "sp106", "sp107", "sp108", "sp109",
    "sp110", "sp111", "sp112", "sp113", "sp114", "sp115", "sp116", "sp117", "sp118", "sp119",
    "sp120", "sp121", "sp122", "sp123", "sp124", "sp125", "sp126", "sp127", "sp128", "sp129",
    "sp130", "sp131", "sp132", "sp133", "sp134", "sp135", "sp136", "sp137", "sp138", "sp139",
    "sp140", "sp141", "sp142", "sp143", "sp144", "sp145", "sp146", "sp147", "sp148", "sp149",
    "sp150", "sp151", "sp152", "sp153", "sp154", "sp155", "sp156", "sp157", "sp158", "sp159",
    "sp160", "sp161", "sp162", "sp163", "sp164", "sp165", "sp166", "sp167", "sp168", "sp169",
    "sp170", "sp171", "sp172", "sp173", "sp174", "sp175", "sp176", "sp177", "sp178", "sp179"
};

/* Callback plug wrappers {{{ */
static t_bool
SpeInsHasSideEffectPlug (t_ins *ins)
{
    return SpeInsHasSideEffect (T_SPE_INS (ins));
}

static t_bool
SpeInsIsLoadPlug (t_ins *ins)
{
    return SpeInsIsLoad (T_SPE_INS (ins));
}

static t_bool
SpeInsIsStorePlug (t_ins *ins)
{
    return SpeInsIsStore (T_SPE_INS (ins));
}

static t_bool
SpeInsIsProcedureCallPlug (t_ins *ins)
{
    return SpeInsIsProcedureCall (T_SPE_INS (ins));
}

static t_bool
SpeInsIsIndirectCallPlug (t_ins *ins)
{
    return SpeInsIsIndirectCall (T_SPE_INS (ins));
}

static t_bool
SpeInsIsUnconditionalJumpPlug (t_ins *ins)
{
    return SpeInsIsUnconditionalJump (T_SPE_INS (ins));
}

static t_bool
SpeInsIsControlFlowPlug (t_ins *ins)
{
    return SpeInsIsControlFlow (T_SPE_INS (ins));
}

static t_bool
SpeInsIsSystemPlug (t_ins *ins)
{
    return SpeInsIsSystem (T_SPE_INS (ins));
}

static t_tristate
SpeInsIsSyscallExitPlug (t_ins *ins)
{
    return SpeInsIsSyscallExit (T_SPE_INS (ins));
}

static t_bool 
SpeFunIsGlobal(t_function * fun)
{
    if (FUNCTION_FLAGS(fun) & FF_IS_EXPORTED)
    {
        if(FunctionBehaves(fun))
        {
            return TRUE;
        }
        else 
        {
            return FALSE;
        }
    }
    return FALSE;
}

/* }}} */

t_architecture_description spe_description =
{
    /*! Size of an instruction: encoded minimal size */
    SPE_INS_SIZE,
    /*! Size of an instruction: encoded maximum size */
    SPE_INS_SIZE,
    /*! Mod size */
    SPE_INS_SIZE,
    /*! Bundle size */
    1,
    /*! Bundle template size */
    0,
    /*! Size of an instruction: disassembled instruction size */
    sizeof(t_spe_ins),
    /*! Address size */
    ADDRSIZE32,
    /*! The number of general purpose integer registers */
    SPE_REG_NGPR,
    /*! The number of general purpose floating point registers */
    0,
    /*! The number of condition registers */
    0,
    /*! The number of branch registers */
    0,
    /*! The number of special registers */
    SPE_REG_NSPR,
#if MAX_REG_ITERATOR > 64
    /* These are really filled in the initialization function */
    /*! All registers           */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Integer registers       */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Floating point registers*/   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Condition registers     */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Callee saved            */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Callee may use          */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Callee may change       */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Callee may return       */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Always live             */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Registers prop over hell*/   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Const registers         */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Dead over call          */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Link registers          */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Argument registers      */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! Return registers        */   {MAX_REG_ITERATOR, {0x00000000}},
    /*! dyncall use registers   */   {MAX_REG_ITERATOR, {0}},
#else
#error   SPE has more than 64 registers!!!
#endif
    /*! The program counter */
    REG_NONE,
    /*! An array containing the name of each register */
    (char **)names,
    /*! Callback to disassemble a section */
    SpeDisassembleSection,
    /*! Callback to assemble a section */
    SpeAssembleSection,
    /*! Callback to create a flowgraph for an object */
    SpeFlowgraph,
    /*! Callback to deflowgraph */
    SpeDeflowgraph,
    /*! Callback to make address producers for a section */
    SpeMakeAddressProducers,
    /*! Callback to free an instruction */
    NULL,
    /*! Callback to transform an instruction into a jump */
    NULL,
    /*! Callback to duplicate an instruction */
    NULL,
    /*! Returns true when the instruction has side effects */
    SpeInsHasSideEffectPlug,
    /*! Returns true when instruction is a load */
    SpeInsIsLoadPlug,
    /*! Returns true when instruction is a store */
    SpeInsIsStorePlug,
    /*! Returns true when instruction is a call */
    SpeInsIsProcedureCallPlug,
    /*! Returns true when instruction is an indirect call */
    SpeInsIsIndirectCallPlug,
    NULL,
    /*! Returns true when instruction is an unconditional jump */
    SpeInsIsUnconditionalJumpPlug,
    /*! Returns true when instruction modifies control flow */
    SpeInsIsControlFlowPlug,
    /*! Returns true when instruction is system control */
    SpeInsIsSystemPlug,
    /*! Returns true/perhaps when instruction is program exit */
    SpeInsIsSyscallExitPlug,
    /*! Callback to print an instruction */
    SpeInstructionPrint,
    /*! Callback to transform an instruction into a NOP */
    SpeInsMakeNop,
    /*! Return true if instructions are identical */
    NULL,
    /*! Callback to unconditionalize an instruction of which constant
     * propagation has determined that they will always be executed */
    NULL,
    /*! Callback to build an instruction from a string */
    NULL,
    /*! Callback to add a jump instruction inside a BBL */
    NULL,
    /*! Callback to detect if a function is globally exported */
    SpeFunIsGlobal,
    /*! An array with names of unbehaved functions */
    NULL,   
    /*! Callback to compute live registers before a SWI */
    NULL,
    /*! Returns true if ... what?! TODO */
    NULL,
    /*! Callback to... what?! TODO */
    NULL
};

void
SpeDescriptionInit ()
{
    int i;

    /* Set every register */
    for (i = 0; i < SPE_REG_MAX-1; i++)
    {
        RegsetSetAddReg (spe_description.all_registers, i);
    }
    for (i = 0; i < SPE_REG_NGPR-1; i++)
    {
        RegsetSetAddReg (spe_description.int_registers, i);
    }

    /* Set other special register sets */
    #ifndef _MSC_VER
    #warning "Missing register usage definitions"
    #endif

    /* Callee saved:: stack + non-volatile */
    RegsetSetAddReg (spe_description.callee_saved, SPE_REG_SP);
    for (i = 80; i < SPE_REG_NGPR; i++)
        RegsetSetAddReg (spe_description.callee_saved, i);

    /* Callee may use:: registers which are valid on function
     * entry and which may be used by the callee. This includes
     * the stack pointer, (the new) lr, FPSCR and all argument
     * registers
     */
    for (i = 0; i < 75; i++)
        RegsetSetAddReg (spe_description.callee_may_use, i);
    RegsetSetAddReg (spe_description.callee_may_use, SPE_REG_FPSCR);
    /* also include all sprs (mailboxes etc) */
    for (i = SPE_REG_NGPR; i < SPE_REG_NGPR+SPE_REG_NSPR; i++)
        RegsetSetAddReg (spe_description.callee_may_use, i);

    /* Callee may change:: (for normal functions: volatile registers)
     *   Not:
     *     - LR (return to where we came from)
     *     - SP (stack pointer on entry == at exit)
     */
    for (i = 2; i < 79; i++)
        RegsetSetAddReg (spe_description.callee_may_change, i);
    /* also include all sprs (mailboxes etc) */
    for (i = SPE_REG_NGPR; i < SPE_REG_NGPR+SPE_REG_NSPR; i++)
        RegsetSetAddReg (spe_description.callee_may_change, i);
    
    /* Callee may return:: parameter registers (r3-r74) */
    for (i = 3; i < 75; i++)
        RegsetSetAddReg (spe_description.callee_may_return, i);
    
    /* Always live:: SP */
    RegsetSetAddReg (spe_description.always_live, SPE_REG_SP);
    /* also include all sprs (mailboxes etc) */
    for (i = SPE_REG_NGPR; i < SPE_REG_NGPR+SPE_REG_NSPR; i++)
        RegsetSetAddReg (spe_description.callee_may_use, i);
    
    /* Propagated over hell:: none */

    /* Constant:: none */

    /* Dead over call:: LR + scratch registers */
    RegsetSetAddReg (spe_description.dead_over_call, SPE_REG_LR);
    for (i = 75; i < 79; i++)
        RegsetSetAddReg (spe_description.dead_over_call, i);

    /* Link register:: */
    RegsetSetAddReg (spe_description.callee_saved, SPE_REG_LR);

    /* Argument registers:: same as return value registers */
    spe_description.argument_regs=spe_description.callee_may_return;

    /* Return registers:: same as return value registers */
    spe_description.return_regs=spe_description.callee_may_return;

    /* Set current Local Store size as default */
    SPE_LSLR = AddressNew32 (SPE_LSLR_DEFAULT);
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
