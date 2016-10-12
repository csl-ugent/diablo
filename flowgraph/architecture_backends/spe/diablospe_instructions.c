/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
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

/* SpeInsHasSideEffect {{{ */
/*! Check if instruction has side effects.
 * 
 * \param ins Instruction to check.
 * 
 * \return TRUE if instruction has side effects
 */
t_bool
SpeInsHasSideEffect (t_spe_ins *ins)
{
    return SpeInsIsStore (ins) ||
        SpeInsIsControlFlow (ins) ||
        SpeInsIsSystem (ins) ||
        SPE_INS_TYPE (ins) == IT_PREF ||
        /* the SPRs are memory mapped I/O and things like that */
        SPE_INS_OPCODE (ins) == SPE_MFSPR ||
        SPE_INS_OPCODE (ins) == SPE_MTSPR;
}
/* }}} */

/* SpeInsIsLoad {{{ */
/*! Check if instruction is a load from memory.
 * 
 * \param ins Instruction to check
 * 
 * \return TRUE if instruction is a load.
 */
t_bool
SpeInsIsLoad (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_LOAD;
}
/* }}} */

/* SpeInsIsStore {{{ */
/*! Check if instruction is a store to memory.
 * 
 * \param ins Instruction to check
 * 
 * \return TRUE id instruction is a store.
 */
t_bool
SpeInsIsStore (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_STORE;
}
/* }}} */

/* SpeInsIsProcedureCall {{{ */
/*! Check if instruction is a procedure call.
 * 
 * \param ins Instruction to check.
 * 
 * \return TRUE if instruction is a procedure call.
 */
t_bool
SpeInsIsProcedureCall (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_BRANCH &&
        SPE_INS_UPDATES_LINK_REG (ins);
}
/* }}} */

/* SpeInsIsIndirectCall {{{ */
/*! Check if instruction is an indirect call.
 * 
 * \param ins Instruction to check
 * 
 * \return TRUE if instruction is an indirect call.
 */
t_bool
SpeInsIsIndirectCall (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_BRANCH &&
        SPE_INS_UPDATES_LINK_REG (ins) &&
        spe_opcode_table[SPE_INS_OPCODE (ins)].immsize == 0;
}
/* }}} */

/* SpeInsIsUnconditionalJump {{{ */
/*! Check if instruction is an unconditional jump.
 * 
 * \param ins Instruction to check.
 * 
 * \return TRUE if instruction is an unconditional jump.
 */
t_bool
SpeInsIsUnconditionalJump (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_BRANCH &&
        !SPE_INS_IS_CONDITIONAL (ins) &&
        !SPE_INS_UPDATES_LINK_REG (ins) &&
        spe_opcode_table[SPE_INS_OPCODE (ins)].immsize != 0;
}
/* }}} */

/* SpeInsIsControlFlow {{{ */
/*! Check if instruction modifies the control flow.
 * 
 * \param ins Instruction to check
 * 
 * \return TRUE if instruction modifies the control flow.
 */
t_bool
SpeInsIsControlFlow (t_spe_ins *ins)
{
    switch (SPE_INS_OPCODE (ins))
    {
        case SPE_STOP:
        case SPE_STOPD:
            return TRUE;
            break;
        default:
            break;
    }

    if (SpeInsIsRegularControlFlow (ins))
    {
        return TRUE;
    }

    return FALSE;
}
/* }}} */

/* SpeInsIsSystem {{{ */
/*! Check if an instruction is a system one.
 * 
 * \param ins Instruction to check
 * 
 * \return TRUE if instruction is a system one.
 */
t_bool
SpeInsIsSystem (t_spe_ins *ins)
{
    switch (SPE_INS_TYPE (ins))
    {
        case IT_SWI:
        case IT_SYNC:
            return TRUE;
            break;
        default:
            break;
    }
    return FALSE;
}
/* }}} */

/* SpeInsIsRegularControlFlow {{{ */
/*! Check if an instruction modifies control flow in a regular way (i.e. not system).
 * 
 * \param ins Instruction to check.
 * 
 * \return TRUE if instruction modifies control flow in a regular way.
 */
t_bool
SpeInsIsRegularControlFlow (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_BRANCH;
}
/* }}} */

/* SpeInsIsControlFlowWithDisplacement {{{ */
/*! Check if instruction is a relative control flow modifier.
 * 
 * \param ins Instruction to check.
 * 
 * \return TRUE if instruction is a relative control flow modifier.
 */
t_bool
SpeInsIsControlFlowWithDisplacement (t_spe_ins *ins)
{
    return SPE_INS_TYPE (ins) == IT_BRANCH &&
        spe_opcode_table[SPE_INS_OPCODE (ins)].immsize > 0;
}
/* }}} */

/* SpeInsIsSyscallExit {{{ */
/*! Check if instruction is a program exit.
 * 
 * \param ins Instruction to check.
 * 
 * \return YES if instruction is a program exit, PERHAPS when not sure, NO otherwise.
 */
t_tristate
SpeInsIsSyscallExit (t_spe_ins *ins)
{
    switch (SPE_INS_OPCODE (ins))
    {
        case SPE_STOP:
        case SPE_STOPD:
            return YES;
            break;
        default:
            break;
    }
    return NO;
}
/* }}} */


/* SpeInsMakeNop {{{ */
/*! Turns an instruction into a nop $127 ($127 because it's the least used register).
 * 
 * \param ins Instruction to turn into a nop.
 */
void
SpeInsMakeNop (t_ins *ins)
{
    t_reloc_ref *rr;
    t_spe_ins *pins = T_SPE_INS (ins);
    t_regset tregset;
    
    SPE_INS_SET_OPCODE (pins, SPE_NOP);
    SPE_INS_SET_IMMEDIATE (pins, AddressNullForIns (ins));
    SPE_INS_SET_IMMEDIATE_ORIG (pins, AddressNullForIns (ins));
    SPE_INS_SET_REGT (pins, 127);
    SPE_INS_SET_REGA (pins, 256);
    SPE_INS_SET_REGB (pins, 256);
    SPE_INS_SET_REGC (pins, 256);
    SPE_INS_SET_TYPE (pins, IT_NOP);
    SPE_INS_SET_FLAGS(pins, IF_SPE_NONE);
    SPE_INS_SET_ADDRESS (pins, AddressNullForIns (ins));
    SPE_INS_SET_ADDRESS_ORIG (pins, AddressNullForIns (ins));
    SPE_INS_SET_ASSEMBLER (pins, NULL);
    SPE_INS_SET_BHINT (pins, SPE_UNKNOWN);
    
    RegsetSetEmpty (tregset);
    INS_SET_REGS_USE (ins, tregset);
    INS_SET_REGS_DEF (ins, tregset);

    while ((rr = INS_REFERS_TO(ins)))
    {
        RelocTableRemoveReloc(OBJECT_RELOC_TABLE (CFG_OBJECT(SPE_INS_CFG(pins))),
                              RELOC_REF_RELOC(rr));
    }
}
/* }}} */

/* SpeInsMakeLnop {{{ */
/*! Turns an instruction into an lnop.
 * 
 * \param ins Instruction to turn into an lnop.
 */
void
SpeInsMakeLnop (t_ins *ins)
{
    SpeInsMakeNop(ins);
    SPE_INS_SET_OPCODE (T_SPE_INS(ins), SPE_LNOP);
}
/* }}} */


/* SpeInsMakeHintForBranch {{{ */
/*! Recreates a branch hint which was folded into a branch during flowgraphing
 * 
 * \param ins Instruction to turn into the branch hint.
 * \param branch The branch instruction to be hinted.
 */
void
SpeInsMakeHintForBranch (t_spe_ins *ins, t_spe_ins *branch)
{
    t_object *obj = CFG_OBJECT(SPE_INS_CFG(ins));
    t_reloc_ref *rr;
    t_regset tregset;
    t_reloc *rel;
    
    SPE_INS_SET_IMMEDIATE (ins, AddressNullForIns (T_INS(ins)));
    SPE_INS_SET_IMMEDIATE_ORIG (ins, AddressNullForIns (T_INS(ins)));
    SPE_INS_SET_REGT (ins, 256);
    SPE_INS_SET_REGA (ins, 256);
    SPE_INS_SET_REGB (ins, 256);
    SPE_INS_SET_REGC (ins, 256);
    SPE_INS_SET_TYPE (ins, IT_PREF);
    SPE_INS_SET_ATTRIB (ins, 0);
    SPE_INS_SET_FLAGS(ins, IF_SPE_NONE);
    SPE_INS_SET_ADDRESS (ins, AddressNullForIns (T_INS(ins)));
    SPE_INS_SET_ADDRESS_ORIG (ins, AddressNullForIns (T_INS(ins)));
    SPE_INS_SET_ASSEMBLER (ins, SpeAssembleHfB);

    RegsetSetEmpty (tregset);
    SPE_INS_SET_REGS_DEF (ins, tregset);
    SPE_INS_SET_OPCODE (ins, SPE_INS_BHINT(branch));
    switch (SPE_INS_OPCODE(ins))
    {
        case SPE_HBR:
            switch (SPE_INS_OPCODE(branch))
            {
                case SPE_BI:
                case SPE_IRET:
                case SPE_BISLED:
                case SPE_BISL:
                case SPE_BIZ:
                case SPE_BINZ:
                case SPE_BIHZ:
                case SPE_BIHNZ:
                    break;
                default:
                    FATAL(("Unexpected instruction to create indirect branch hint for: @I",branch));
                    break;
            }
            ASSERT(SPE_INS_REGA(branch)<SPE_REG_MAX,("Invalid indirect branch register @I",branch));
            SPE_INS_SET_REGA(ins, SPE_INS_REGA(branch));
            RegsetAddReg(tregset,SPE_INS_REGA(branch));
            /* HBR (in Diablo) expects the immediate to also contain the value of REGA */
            SPE_INS_SET_IMMEDIATE (ins, AddressNewForIns (T_INS(ins), SPE_INS_REGA (ins)));
            break;
        case SPE_HBRR:
        case SPE_HBRA:
            break;
        default:
            FATAL(("Unknown branch hint stored: @I",ins));
            break;
    }
    SPE_INS_SET_REGS_USE (ins, tregset);

    /* the ADDRESS field has to be set by the caller
     * (depends on how far the branch hint is placed
     *  from the branch)
     */

    /* remove all previous relocations for this instruction */
    while ((rr = SPE_INS_REFERS_TO(ins)))
    {
        RelocTableRemoveReloc(OBJECT_RELOC_TABLE (obj),
                              RELOC_REF_RELOC(rr));
    }

    /* branch with relocation? */
    rr = SPE_INS_REFERS_TO(branch);
    if (rr)
    {
        t_reloc *rel;
        
        /* add the relocation from the branch target to the
         * branch hint (we checked during deflowgrafing that
         * these relocations matched)
         */
        ASSERT(!RELOC_REF_NEXT (rr), ("Branch points to more than one place @I",branch));

        rel = RelocTableDupReloc(RELOC_TABLE(RELOC_REF_RELOC (rr)),RELOC_REF_RELOC (rr));
        RelocSetFrom(rel,T_RELOCATABLE(ins));
        Free(RELOC_CODE(rel));
        switch (SPE_INS_OPCODE(ins))
        {
            case SPE_HBRR:
                RELOC_SET_CODE(rel,StringDup("R00A00+ P- \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ ifffc0003 & = ? ifffc0000 - :! $"));
                break;
            case SPE_HBRA:
                RELOC_SET_CODE(rel,StringDup("R00A00+ \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ = i0003fffc & - $"));
                break;
            case SPE_HBR:
                FATAL(("Indirect branch hint for branch with relocation? @I",branch));
                break;
            default:
                FATAL(("Unexpected branch hint @I",ins));
                break;
        }
        SPE_INS_SET_IMMEDIATE (ins, StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj));
    }
    else if ((SPE_INS_OPCODE(ins) == SPE_HBRR) ||
             (SPE_INS_OPCODE(ins) == SPE_HBRA))
    {
        t_bbl *branchtarget;
        t_bbl *bbl;
        t_cfg_edge *e_found = NULL;
        t_cfg_edge *e_succ;
        
        /* relocation from the branch has been removed -> construct
         * relocation for the hint based on the branch edge
         */
        bbl = SPE_INS_BBL(branch);
        BBL_FOREACH_SUCC_EDGE (bbl, e_succ)
        {
            if (CfgEdgeTestCategoryOr(e_succ, ET_CALL|ET_JUMP|ET_IPJUMP))
            {
                e_found = e_succ;
                break;
            }
        }
        ASSERT(e_found,("Can't find branch target edge of @I",branch));

        branchtarget=CFG_EDGE_TAIL(e_found);
        ASSERT(e_found,("Can't find branch target instruction of @I",branch));
        VERBOSE(1,("Branch target for branch @I is @T",branch,branchtarget));
        switch (SPE_INS_OPCODE(ins))
        {
            case SPE_HBRR:
                /* rel16 relocation */
                rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE (obj),
                                                      AddressNullForIns(T_INS(ins)),
                                                      T_RELOCATABLE(ins),
                                                      AddressNullForIns(T_INS(ins)),
                                                      T_RELOCATABLE(branchtarget),
                                                      AddressNullForIns(T_INS(ins)),
                                                      FALSE,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      "R00A00+ P- \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ ifffc0003 & = ? ifffc0000 - :! $");
                SPE_INS_SET_IMMEDIATE (ins, StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj));
                break;
            case SPE_HBRA:
                /* abs16 relocation */
                rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE (obj),
                                                      AddressNullForIns(T_INS(ins)),
                                                      T_RELOCATABLE(ins),
                                                      AddressNullForIns(T_INS(ins)),
                                                      T_RELOCATABLE(branchtarget),
                                                      AddressNullForIns(T_INS(ins)),
                                                      FALSE,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      "R00A00+ \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ = i0003fffc & - $");
                SPE_INS_SET_IMMEDIATE (ins, StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj));
                break;
            default:
                break;
        }
    }

    /* add relocation to calculate the offset from the hint to the branch
     * explanation of the relocation code:
     * The format of HBRA and HBRR is:
     *  - 7 bit opcode
     *  - 2 upper bits of offset to of the branch
     *  - 16 bits branch target
     *  - 7 lower bits of offset to of the branch
     * The offset to the branch does not contain the lower two bits of this value,
     * since all instructions are 4-byte aligned.
     * So what we do is:
     *  - we shift right the offset by 2
     *  - we split the calculated value in its upper 2 and lower 7
     *    bits, "or" them, and insert them in the instruction (with a mask)
     *  - in the check part we verify that the offset fits in the 9 bits (signed)
     */
     rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE (obj),
                                           AddressNullForIns(T_INS(ins)),
                                           T_RELOCATABLE(ins),
                                           AddressNullForIns(T_INS(ins)),
                                           T_RELOCATABLE(branch),
                                           AddressNullForIns(T_INS(ins)),
                                           FALSE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           "R00A00+ P- \\ s0002> == s0180 & s0010 < % s007f & | l ife7fff80 & | w \\ s0100+ ifffffc00& $");
    SPE_INS_SET_ADDRESS (ins, StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj));
}
/* }}} */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
