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

t_spe_opcode_info *spe_opcode_table;

static inline t_uint32
String2Bin (t_string s)
{
    char *c;
    t_uint32 ret = 0;
    for (c = s; *c; ++c)
    {
        ret <<= 1;
        ret += (*c - '0');
    }
    return ret;
}

static inline void
SetOpcodeInfo (int _idx, t_string _name, t_string _opc, int _masksize,
               t_uint16 _type, t_uint16 _immsize, t_spe_imm_trans _immtrans,
               t_spe_slot_usage _slots, SpeDisassembleFunction _disasm,
               SpeAssembleFunction _asmfunc, t_string _print,
               t_instruction_flags _attribs, t_spe_ins_flags _flags)
{
    spe_opcode_table[(_idx)].name = _name;
    spe_opcode_table[(_idx)].opcode = BitfieldMove(String2Bin (_opc), _masksize, 32 - _masksize);
    spe_opcode_table[(_idx)].mask = BitmaskOffset(_masksize, 32 - _masksize);
    spe_opcode_table[(_idx)].type = _type;
    spe_opcode_table[(_idx)].immstart = _masksize;
    spe_opcode_table[(_idx)].immsize = _immsize;
    spe_opcode_table[(_idx)].immtrans = _immtrans;
    spe_opcode_table[(_idx)].slots = _slots;
    spe_opcode_table[(_idx)].Disassemble = _disasm;
    spe_opcode_table[(_idx)].Assemble = _asmfunc;
    spe_opcode_table[(_idx)].print = _print;
    spe_opcode_table[(_idx)].attribs = _attribs;
    spe_opcode_table[(_idx)].flags = _flags;
}

#define SetOpcodeInfo_RR(idx, name, opc, type, slots, print, attribs, flags)        SetOpcodeInfo (idx, name, opc, 10+1, type,  0,  "", slots, NULL, NULL, print, attribs, flags)
#define SetOpcodeInfo_RRR(idx, name, opc, type, slots, print, attribs, flags)       SetOpcodeInfo (idx, name, opc,  3+1, type,  0,  "", slots, NULL, NULL, print, attribs, flags)
#define SetOpcodeInfo_RI7(idx, name, opc, type, imm, slots, print, attribs, flags)  SetOpcodeInfo (idx, name, opc, 10+1, type,  7, imm, slots, NULL, NULL, print, attribs, flags)
#define SetOpcodeInfo_RI10(idx, name, opc, type, imm, slots, print, attribs, flags) SetOpcodeInfo (idx, name, opc,  7+1, type, 10, imm, slots, NULL, NULL, print, attribs, flags)
#define SetOpcodeInfo_RI16(idx, name, opc, type, imm, slots, print, attribs, flags) SetOpcodeInfo (idx, name, opc,  8+1, type, 16, imm, slots, NULL, NULL, print, attribs, flags)
#define SetOpcodeInfo_RI18(idx, name, opc, type, imm, slots, print, attribs, flags) SetOpcodeInfo (idx, name, opc,  6+1, type, 18, imm, slots, NULL, NULL, print, attribs, flags)

void
SpeOpcodesInit ()
{
    spe_opcode_table = Malloc (sizeof(t_spe_opcode_info) * (SPE_LAST_OPCODE + 1));

    /* Opcode-specific disassembly functions are minimized to special cases,
     * almost everything is declared here */

    /* Memory load/store */
    SetOpcodeInfo_RI10 (SPE_LQD      , "lqd"      ,    "00110100", IT_LOAD    , "s<4" , "_-_-w-Q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_LQX      , "lqx"      , "00111000100", IT_LOAD    ,         "_-w-w-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_LQA      , "lqa"      ,   "001100001", IT_LOAD    , "s<2" , "_-_-_-Q" , "$OPC $RT,$IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_LQR      , "lqr"      ,   "001100111", IT_LOAD    , "s<2" , "_-_-_-Q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_STQD     , "stqd"     ,    "00100100", IT_STORE   , "s<4" , "_-_-w-q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_STQX     , "stqx"     , "00101000100", IT_STORE   ,         "_-w-w-q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_STQA     , "stqa"     ,   "001000001", IT_STORE   , "s<2" , "_-_-_-q" , "$OPC $RT,$IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_STQR     , "stqr"     ,   "001000111", IT_STORE   , "s<2" , "_-_-_-q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_CBD      , "cbd"      , "00111110100", IT_DATAPROC, "s"   , "_-_-w-Q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CBX      , "cbx"      , "00111010100", IT_DATAPROC,         "_-w-w-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_CHD      , "chd"      , "00111110101", IT_DATAPROC, "s"   , "_-_-w-Q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CHX      , "chx"      , "00111010101", IT_DATAPROC,         "_-w-w-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_CWD      , "cwd"      , "00111110110", IT_DATAPROC, "s"   , "_-_-w-Q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CWX      , "cwx"      , "00111010110", IT_DATAPROC,         "_-w-w-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_CDD      , "cdd"      , "00111110111", IT_DATAPROC, "s"   , "_-_-w-Q" , "$OPC $RT,$IMM($RA)", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CDX      , "cdx"      , "00111010111", IT_DATAPROC,         "_-w-w-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);

    /* Constant-formation */
    SetOpcodeInfo_RI16 (SPE_ILH      , "ilh"      ,   "010000011", IT_DATAPROC, ""    , "_-_-_-Q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_ILHU     , "ilhu"     ,   "010000010", IT_DATAPROC, "<4"  , "_-_-_-Q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_IL       , "il"       ,   "010000001", IT_DATAPROC, "s"   , "_-_-_-Q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI18 (SPE_ILA      , "ila"      ,     "0100001", IT_DATAPROC, ""    , "_-_-_-Q" , "$OPC $RT,$IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_IOHL     , "iohl"     ,   "011000001", IT_DATAPROC, ""    ,"_-_-_-qQ" , "$OPC $RT,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_FSMBI    , "fsmbi"    ,   "001100101", IT_DATAPROC, ""    , "_-_-_-Q" , "$OPC $RT,$IMM", 0, IF_SPE_NONE); /* \TODO: quadword? */

    /* Integer and logical */
    SetOpcodeInfo_RR   (SPE_AH       , "ah"       , "00011001000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_AHI      , "ahi"      ,    "00011101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_A        , "a"        , "00011000000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_AI       , "ai"       ,    "00011100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SFH      , "sfh"      , "00001001000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_SFHI     , "sfhi"     ,    "00001101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SF       , "sf"       , "00001000000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_SFI      , "sfi"      ,    "00001100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ADDX     , "addx"     , "01101000000", IT_DATAPROC,         "_-q-q-qQ", "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CG       , "cg"       , "00011000010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: rt=quadword? */
    SetOpcodeInfo_RR   (SPE_CGX      , "cgx"      , "01101000010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: rt=quadword? */
    SetOpcodeInfo_RR   (SPE_SFX      , "sfx"      , "01101000001", IT_DATAPROC,         "_-q-q-qQ", "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BG       , "bg"       , "00001000010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BGX      , "bgx"      , "01101000011", IT_DATAPROC,         "_-q-q-qQ", "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPY      , "mpy"      , "01111000100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYU     , "mpyu"     , "01111001100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_MPYI     , "mpyi"     ,    "01110100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_MPYUI    , "mpyui"    ,    "01110101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_MPYA     , "mpya"     ,        "1100", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYH     , "mpyh"     , "01111000101", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYS     , "mpys"     , "01111000111", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYHH    , "mpyhh"    , "01111000110", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYHHA   , "mpyhha"   , "01101000110", IT_DATAPROC,         "_-q-q-qQ", "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYHHU   , "mpyhhu"   , "01111001110", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MPYHHAU  , "mpyhhau"  , "01101001110", IT_DATAPROC,         "_-q-q-qQ", "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CLZ      , "clz"      , "01010100101", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CNTB     , "cntb"     , "01010110100", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FSMB     , "fsmb"     , "00110110110", IT_DATAPROC,         "_-_-h-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FSMH     , "fsmh"     , "00110110101", IT_DATAPROC,         "_-_-b-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FSM      , "fsm"      , "00110110100", IT_DATAPROC,         "_-_-b-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE); /* \TODO: bits 28:31 */
    SetOpcodeInfo_RR   (SPE_GBB      , "gbb"      , "00110110010", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_GBH      , "gbh"      , "00110110001", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_GB       , "gb"       , "00110110000", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_AVGB     , "avgb"     , "00011010011", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ABSDB    , "absdb"    , "00001010011", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SUMB     , "sumb"     , "01001010011", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_XSBH     , "xsbh"     , "01010110110", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_XSHW     , "xshw"     , "01010101110", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_XSWD     , "xswd"     , "01010100110", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE); /* \TODO: read bytes 4:7, 12:15 */
    SetOpcodeInfo_RR   (SPE_AND      , "and"      , "00011000001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ANDC     , "andc"     , "01011000001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ANDBI    , "andbi"    ,    "00010110", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ANDHI    , "andhi"    ,    "00010101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ANDI     , "andi"     ,    "00010100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_OR       , "or"       , "00001000001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ORC      , "orc"      , "01011001001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ORBI     , "orbi"     ,    "00000110", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ORHI     , "orhi"     ,    "00000101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_ORI      , "ori"      ,    "00000100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ORX      , "orx"      , "00111110000", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_XOR      , "xor"      , "01001000001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_XORBI    , "xorbi"    ,    "01000110", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_XORHI    , "xorhi"    ,    "01000101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_XORI     , "xori"     ,    "01000100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_NAND     , "nand"     , "00011001001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_NOR      , "nor"      , "00001001001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_EQV      , "eqv"      , "01001001001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_SELB     , "selb"     ,        "1000", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_SHUFB    , "shufb"    ,        "1011", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);

    /* Shift and rotate */
    SetOpcodeInfo_RR   (SPE_SHLH     , "shlh"     , "00001011111", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_SHLHI    , "shli"     , "00001111111", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SHL      , "shl"      , "00001011011", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_SHLI     , "shli"     , "00001111011", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SHLQBI   , "shlqbi"   , "00111011011", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 29:31 */
    SetOpcodeInfo_RI7  (SPE_SHLQBII  , "shlqbii"  , "00111111011", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SHLQBY   , "shlqby"   , "00111011111", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 27:31 */
    SetOpcodeInfo_RI7  (SPE_SHLQBYI  , "shlqbyi"  , "00111111111", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SHLQBYBI , "shlqbybi" , "00111001111", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 24:28 */
    SetOpcodeInfo_RR   (SPE_ROTH     , "roth"     , "00001011100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTHI    , "rothi"    , "00001111100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROT      , "rot"      , "00001011000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTI     , "roti"     , "00001111000", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTQBY   , "rotqby"   , "00111011100", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 28:31 */
    SetOpcodeInfo_RI7  (SPE_ROTQBYI  , "rotqbyi"  , "00111111100", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE); /* \TODO: quadword? */
    SetOpcodeInfo_RR   (SPE_ROTQBYBI , "rotqbybi" , "00111001100", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 24:28 */
    SetOpcodeInfo_RR   (SPE_ROTQBI   , "rotqbi"   , "00111011000", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE); /* \TODO: RA bits 29:31 */
    SetOpcodeInfo_RI7  (SPE_ROTQBII  , "rotqbii"  , "00111111000", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTHM    , "rothm"    , "00001011101", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTHMI   , "rothmi"   , "00001111101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTM     , "rotm"     , "00001011001", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTMI    , "rotmi"    , "00001111001", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTQMBY  , "rotqmby"  , "00111011101", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTQMBYI , "rotqmbyi" , "00111111101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTQMBYBI, "rotqmbybi", "00111001101", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTQMBI  , "rotqmbi"  , "00111011001", IT_DATAPROC,         "_-b-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTQMBII , "rotqmbii" , "00111111001", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTMAH   , "rotmah"   , "00001011110", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTMAHI  , "rotmahi"  , "00001111110", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_ROTMA    , "rotma"    , "00001011010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI7  (SPE_ROTMAI   , "rotmai"   , "00001111010", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);

    /* Compare, branch and halt */
    SetOpcodeInfo_RR   (SPE_HEQ      , "heq"      , "01111011000", IT_SWI     ,         "_-w-w-f" , "$OPC $RA,$RB", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_HEQI     , "heqi"     ,    "01111111", IT_SWI     , "s"   , "_-_-w-f" , "$OPC $RA,$IMM", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_HGT      , "hgt"      , "01001011000", IT_SWI     ,         "_-w-w-f" , "$OPC $RA,$RB", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_HGTI     , "hgti"     ,    "01001111", IT_SWI     , "s"   , "_-_-w-f" , "$OPC $RA,$IMM", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_HLGT     , "hlgt"     , "01011011000", IT_SWI     ,         "_-w-w-f" , "$OPC $RA,$RB", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_HLGTI    , "hlgti"    ,    "01011111", IT_SWI     , "s"   , "_-_-w-f" , "$OPC $RA,$IMM", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CEQB     , "ceqb"     , "01111010000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CEQBI    , "ceqbi"    ,    "01111110", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CEQH     , "ceqh"     , "01111001000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CEQHI    , "ceqhi"    ,    "01111101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CEQ      , "ceq"      , "01111000000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CEQI     , "ceqi"     ,    "01111100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE); /* \TODO: quadword? */
    SetOpcodeInfo_RR   (SPE_CGTB     , "cgtb"     , "01001010000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CGTBI    , "cgtbi"    ,    "01001110", IT_DATAPROC, ""    , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CGTH     , "cgth"     , "01001001000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CGTHI    , "cgthi"    ,    "01001101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CGT      , "cgt"      , "01001000000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CGTI     , "cgti"     ,    "01001100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CLGTB    , "clgtb"    , "01011010000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CLGTBI   , "clgtbi"   ,    "01011110", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CLGTH    , "clgth"    , "01011001000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CLGTHI   , "clgthi"   ,    "01011101", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_CLGT     , "clgt"     , "01011000000", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI10 (SPE_CLGTI    , "clgti"    ,    "01011100", IT_DATAPROC, "s"   , "_-_-q-Q" , "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BR       , "br"       ,   "001100100", IT_BRANCH  , "s<2p", "_-_-_-_" , "$OPC $IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BRA      , "bra"      ,   "001100000", IT_BRANCH  , "s<2" , "_-_-_-_" , "$OPC $IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BRSL     , "brsl"     ,   "001100110", IT_BRANCH  , "s<2p", "_-_-_-Q" , "$OPC $RT,$IMMx", 0, IF_SPE_UPDATES_LINK_REG);
    SetOpcodeInfo_RI16 (SPE_BRASL    , "brasl"    ,   "001100110", IT_BRANCH  , "s<2p", "_-_-_-Q" , "$OPC $RT,$IMMx", 0, IF_SPE_UPDATES_LINK_REG);
    SetOpcodeInfo_RR   (SPE_BI       , "bi"       , "00110101000", IT_BRANCH  ,         "_-f-w-_" , "$OPC $RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_IRET     , "iret"     , "00110101010", IT_BRANCH  ,         "_-f-f-_" , "$OPC $RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BISLED   , "bisled"   , "00110101011", IT_BRANCH  ,         "_-f-b-Q" , "$OPC $RT,$RA", IF_CONDITIONAL, IF_SPE_UPDATES_LINK_REG);
    SetOpcodeInfo_RR   (SPE_BISL     , "bisl"     , "00110101001", IT_BRANCH  ,         "_-f-b-Q" , "$OPC $RT,$RA", 0, IF_SPE_UPDATES_LINK_REG);
    SetOpcodeInfo_RI16 (SPE_BRNZ     , "brnz"     ,   "001000010", IT_BRANCH  , "s<2p", "_-_-_-b" , "$OPC $RT,$IMMx", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BRZ      , "brz"      ,   "001000000", IT_BRANCH  , "s<2p", "_-_-_-b" , "$OPC $RT,$IMMx", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BRHNZ    , "brhnz"    ,   "001000110", IT_BRANCH  , "s<2p", "_-_-_-h" , "$OPC $RT,$IMMx", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RI16 (SPE_BRHZ     , "brhz"     ,   "001000100", IT_BRANCH  , "s<2p", "_-_-_-h" , "$OPC $RT,$IMMx", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BIZ      , "biz"      , "00100101000", IT_BRANCH  ,         "_-f-w-w" , "$OPC $RT,$RA", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BINZ     , "binz"     , "00100101001", IT_BRANCH  ,         "_-f-w-w" , "$OPC $RT,$RA", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BIHZ     , "bihz"     , "00100101010", IT_BRANCH  ,         "_-f-w-h" , "$OPC $RT,$RA", IF_CONDITIONAL, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_BIHNZ    , "bihnz"    , "00100101011", IT_BRANCH  ,         "_-f-w-h" , "$OPC $RT,$RA", IF_CONDITIONAL, IF_SPE_NONE);

    /* Hint-for-branch */
    /* Special case:
     *      branch target address     : $IMM
     *      branch instruction address: $ADDR
     */
    SetOpcodeInfo      (SPE_HBR      , "hbr"      , "00110101100", 11,
                                                                 IT_PREF , 0, ""    , "_-_-w-_" , SpeDisassembleHfB, SpeAssembleHfB, "$OPC $ADDRx,$IMMx", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_HBRA     , "hbra"     ,     "0001000",  7,
                                                                 IT_PREF , 16, "s<2", "_-_-_-_" , SpeDisassembleHfB, SpeAssembleHfB, "$OPC $ADDRx,$IMMx", 0, IF_SPE_NONE);
    spe_opcode_table[SPE_HBRA].immstart = 9;
    SetOpcodeInfo      (SPE_HBRR     , "hbrr"     ,     "0001001",  7,
                                                                 IT_PREF , 16,"s<2p", "_-_-_-_" , SpeDisassembleHfB, SpeAssembleHfB, "$OPC $ADDRx,$IMMx", 0, IF_SPE_NONE);
    spe_opcode_table[SPE_HBRR].immstart = 9;

    /* Floating-point */
    SetOpcodeInfo_RR   (SPE_FA       , "fa"       , "01011000100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFA      , "dfa"      , "01011001100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FS       , "fs"       , "01011000101", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFS      , "dfs"      , "01011001101", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FM       , "fm"       , "01011000110", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFM      , "dfm"      , "01011001110", IT_DATAPROC,        "_-q-q-qQ" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_FMA      , "fma"      ,        "1110", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFMA     , "dfma"     , "01101011100", IT_DATAPROC,        "_-q-q-qQ" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_FNMS     , "fnms"     ,        "1101", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFNMS    , "dfnms"    , "01101011110", IT_DATAPROC,        "_-q-q-qQ" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RRR  (SPE_FMS      , "fms"      ,        "1111", IT_DATAPROC,         "Q-q-q-q" , "$OPC $RT,$RA,$RB,$RC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFMS     , "dfms"     , "01101011101", IT_DATAPROC,        "_-q-q-qQ" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DFNMA    , "dfnma"    , "01101011111", IT_DATAPROC,        "_-q-q-qQ" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FREST    , "frest"    , "00110111000", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FRSQEST  , "frsqest"  , "00110111001", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FI       , "fi"       , "01111010100", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_CSFLT    , "csflt"    ,  "0111011010", 10,
                                                                 IT_DATAPROC, 8, ""  ,"_-_-q-Q" , NULL, NULL, "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_CFLTS    , "cflts"    ,  "0111011000", 10,
                                                                 IT_DATAPROC, 8, ""  ,"_-_-q-Q" , NULL, NULL, "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_CUFLT    , "cuflt"    ,  "0111011011", 10,
                                                                 IT_DATAPROC, 8, ""  ,"_-_-q-Q" , NULL, NULL, "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_CFLTU    , "cfltu"    ,  "0111011001", 10,
                                                                 IT_DATAPROC, 8, ""  ,"_-_-q-Q" , NULL, NULL, "$OPC $RT,$RA,$IMM", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FRDS     , "frds"     , "01110111001", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FESD     , "fesd"     , "01110111000", IT_DATAPROC,         "_-_-q-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FCEQ     , "fceq"     , "01111000010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FCMEQ    , "fcmeq"    , "01111001010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FCGT     , "fcgt"     , "01011000010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_FCMGT    , "fcmgt"    , "01011001010", IT_DATAPROC,         "_-q-q-Q" , "$OPC $RT,$RA,$RB", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_FSCRWR   , "fscrwr"   , "01110111010", 11,
                                                                 IT_DATAPROC, "0", ""  ,"_-_-q-f" , SpeDisassembleFPSCR, NULL, "$OPC $RA", 0, IF_SPE_NONE);
    SetOpcodeInfo      (SPE_FSCRRD   , "fscrrd"   , "01110011000", 11,
                                                                 IT_DATAPROC, "0", ""  ,"_-_-_-Q" , SpeDisassembleFPSCR, NULL, "$OPC $RT", 0, IF_SPE_NONE);

    /* Control */
    SetOpcodeInfo_RR   (SPE_STOP     , "stop"     , "00000000000", IT_SWI     ,         "_-_-f-f" , "$OPC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_STOPD    , "stopd"    , "00101000000", IT_SWI     ,         "_-f-f-f" , "$OPC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_LNOP     , "lnop"     , "00000000001", IT_NOP     ,         "_-_-_-_" , "$OPC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_NOP      , "nop"      , "01000000001", IT_NOP     ,         "_-_-_-f" , "$OPC $RT", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_SYNC     , "sync"     , "00000000010", IT_SYNC    ,         "_-f-_-_" , "$OPC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_DSYNC    , "dsync"    , "00000000011", IT_SYNC    ,         "_-_-_-_" , "$OPC", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MFSPR    , "mfspr"    , "00000001100", IT_DATAPROC,         "_-_-q-sQ", "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_MTSPR    , "mtspr"    , "00100001100", IT_DATAPROC,         "_-_-sQ-q", "$OPC $RA,$RT", 0, IF_SPE_NONE);

    /* Channel */
    SetOpcodeInfo_RR   (SPE_RDCH     , "rdch"     , "00000001101", IT_LOAD    ,         "_-_-f-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_RCHCNT   , "rchcnt"   , "00000001111", IT_LOAD    ,         "_-_-f-Q" , "$OPC $RT,$RA", 0, IF_SPE_NONE);
    SetOpcodeInfo_RR   (SPE_WRCH     , "wrch"     , "00100001101", IT_STORE   ,         "_-_-f-q" , "$OPC $RA,$RT", 0, IF_SPE_NONE);

    /* DATA pseudo-instruction */
    SetOpcodeInfo(SPE_DATA, ".data", "", 0, IT_DATA, 32, "", "", NULL, SpeAssembleData, "$OPC $IMM", 0, 0);
}

void
SpeOpcodesFini ()
{
    Free (spe_opcode_table);
}
  
/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
