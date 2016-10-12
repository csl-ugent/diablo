#include <diabloamd64.h>
#include <string.h>

/* {{{ write instruction prefixes */
static t_uint32 WritePrefixes(t_amd64_ins * ins, t_uint8 * buf,char rex)
{
  t_uint32 nprefs = 0;

  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_LOCK))
    buf[nprefs++] = '\xf0';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REPNZ))
    buf[nprefs++] = '\xf2';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REP))
    buf[nprefs++] = '\xf3';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_CS_OVERRIDE))
    buf[nprefs++] = '\x2e';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_DS_OVERRIDE))
    buf[nprefs++] = '\x3e';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_ES_OVERRIDE))
    buf[nprefs++] = '\x26';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_FS_OVERRIDE))
    buf[nprefs++] = '\x64';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_GS_OVERRIDE))
    buf[nprefs++] = '\x65';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_SS_OVERRIDE))
    buf[nprefs++] = '\x36';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_OPSIZE_OVERRIDE))
    buf[nprefs++] = '\x66';
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_ADDRSIZE_OVERRIDE))
    buf[nprefs++] = '\x67';
  if(rex!=0x00)
    buf[nprefs++] = rex;
  return nprefs;
} /* }}} */

/* {{{ write instruction opcode */
t_uint32 Amd64WriteOpcode(t_amd64_ins * ins, t_amd64_opcode_entry * form, t_uint8 * buf, t_uint32 * mod_ret, t_uint32 * opcode_ext_ret, t_uint32 * rm_ret)
{
  enum opcodetype {
    onebyte, twobyte, fpu_reg, fpu_mem,
    grp1a, grp1b, grp1c, grp1d,
    grp2a, grp2b,
    grp3a, grp3b, 
    grp4a, grp4b, grp4c, grp4d,
    grp5a, grp5b,
    grp6a, grp6b,
    grp7a, grp7b,
    grp8a,
    grp12,
    grp13,
    grp14,
    grp15a,
    grp15a11,
    grp16,
    pgrp1,
    pgrp2,
    pgrp3,
    pgrp4,
    pgrp5,
    pgrp6,
    pgrp7,
    pgrp8,
    pgrp9,
    pgrpA,
    pgrpB,
    pgrpC,
    pgrpD,
    pgrpE,
    pgrpF,
    pgrp10,
    pgrp11,
    pgrp12,
    pgrp13,
    pgrp14,
    pgrp15,
    pgrp16,
    pgrp17,
    pgrp18,
    pgrp19,
    pgrp1A,
    pgrp1B,
    pgrp1C,
    pgrp1D,
    pgrp1E,
    pgrp1F,
    pgrp20,
    pgrp21,
    pgrp22,
    pgrp23,
    pgrp24,
    pgrp25,
    pgrp26,
    pgrp27,
    pgrp28,
    pgrp29,
    pgrp2A,
    pgrp2B,
    pgrp2C,
    pgrp2D,
    pgrp2E,
    pgrp2F,
    pgrp30,
    pgrp31,
    pgrp32,
    pgrp33,
    pgrp34,
    pgrp35,
    pgrp36,
    pgrp37,
    pgrp38,
    pgrp39,
    pgrp3A,
    pgrp3B,
    pgrp3C,
    pgrp3D,
    pgrp3E,
    pgrp3F,
    pgrp40,
    pgrp41,
    pgrp42,
    pgrp43,
    pgrp44,
    pgrp45,
    pgrp46,
    pgrp47,
    pgrp48,
    pgrp49,
    pgrp4A,
    pgrp4B,
    pgrp4C,
    pgrp4D,
    pgrp4E,
    pgrp4F,
    pgrp50,
    pgrp51,
    pgrp52,
    pgrp53,
    pgrp54,
    pgrp55,
    pgrp56,
    pgrp57,
    pgrp58,
    pgrp59,
    pgrp5A,
    pgrp5B,
    pgrp5C,
    pgrp5D,
    pgrp5E,
    pgrp5F,
    max_opcodetype
  };
  static const t_amd64_opcode_entry * table_limits[][2] = {
    {disamd64, disamd64+256},
    {disamd64_twobyte, disamd64_twobyte+256},
    {disamd64_fpu_reg[0][0], disamd64_fpu_reg[0][0]+512},
    {disamd64_fpu_mem[0], disamd64_fpu_mem[0]+64},
    {disamd64_grps[AMD64_GRP1a-AMD64_GRP1a], disamd64_grps[AMD64_GRP1a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP1b-AMD64_GRP1a], disamd64_grps[AMD64_GRP1b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP1c-AMD64_GRP1a], disamd64_grps[AMD64_GRP1c-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP1d-AMD64_GRP1a], disamd64_grps[AMD64_GRP1d-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP2a-AMD64_GRP1a], disamd64_grps[AMD64_GRP2a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP2b-AMD64_GRP1a], disamd64_grps[AMD64_GRP2b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP3a-AMD64_GRP1a], disamd64_grps[AMD64_GRP3a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP3b-AMD64_GRP1a], disamd64_grps[AMD64_GRP3b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP4a-AMD64_GRP1a], disamd64_grps[AMD64_GRP4a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP4b-AMD64_GRP1a], disamd64_grps[AMD64_GRP4b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP4c-AMD64_GRP1a], disamd64_grps[AMD64_GRP4c-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP4d-AMD64_GRP1a], disamd64_grps[AMD64_GRP4d-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP5a-AMD64_GRP1a], disamd64_grps[AMD64_GRP5a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP5b-AMD64_GRP1a], disamd64_grps[AMD64_GRP5b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP6a-AMD64_GRP1a], disamd64_grps[AMD64_GRP6a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP6b-AMD64_GRP1a], disamd64_grps[AMD64_GRP6b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP7a-AMD64_GRP1a], disamd64_grps[AMD64_GRP7a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP7b-AMD64_GRP1a], disamd64_grps[AMD64_GRP7b-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP8a-AMD64_GRP1a], disamd64_grps[AMD64_GRP8a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP12-AMD64_GRP1a], disamd64_grps[AMD64_GRP12-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP13-AMD64_GRP1a], disamd64_grps[AMD64_GRP13-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP14-AMD64_GRP1a], disamd64_grps[AMD64_GRP14-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP15a-AMD64_GRP1a], disamd64_grps[AMD64_GRP15a-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP15a11-AMD64_GRP1a], disamd64_grps[AMD64_GRP15a11-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_GRP16-AMD64_GRP1a], disamd64_grps[AMD64_GRP16-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_6-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_6-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_7-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_7-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_8-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_8-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_9-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_9-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_F-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_10-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_10-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_11-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_11-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_12-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_12-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_13-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_13-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_14-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_14-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_15-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_15-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_16-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_16-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_17-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_17-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_18-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_18-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_19-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_19-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_1F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_1F-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_20-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_20-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_21-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_21-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_22-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_22-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_23-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_23-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_24-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_24-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_25-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_25-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_26-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_26-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_27-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_27-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_28-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_28-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_29-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_29-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_2F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_2F-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_30-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_30-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_31-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_31-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_32-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_32-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_33-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_33-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_34-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_34-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_35-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_35-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_36-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_36-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_37-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_37-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_38-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_38-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_39-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_39-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_3F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_3F-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_40-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_40-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_41-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_41-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_42-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_42-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_43-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_43-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_44-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_44-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_45-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_45-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_46-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_46-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_47-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_47-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_48-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_48-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_49-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_49-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_4F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_4F-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_50-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_50-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_51-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_51-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_52-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_52-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_53-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_53-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_54-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_54-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_55-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_55-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_56-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_56-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_57-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_57-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_58-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_58-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_59-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_59-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5A-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5A-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5B-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5B-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5C-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5C-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5D-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5D-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5E-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5E-AMD64_GRP1a]+8},
    {disamd64_grps[AMD64_PREFIX_GRP_5F-AMD64_GRP1a], disamd64_grps[AMD64_PREFIX_GRP_5F-AMD64_GRP1a]+8}
  };

  enum opcodetype type;
  for (type = onebyte; type < max_opcodetype; type++)
  {
    if ((form >= table_limits[type][0]) && (form < table_limits[type][1]))
      break;
  }
  ASSERT(type != max_opcodetype, ("Could not find suitable opcode bytes"));

  switch (type)
  {
    case onebyte:
      buf[0] = (t_uint8) (((char *)form - (char *)disamd64)/sizeof(t_amd64_opcode_entry));
      return 1;
    case twobyte:
      buf[0] = '\x0f';
      buf[1] = (t_uint8) (((char *)form - (char *)disamd64_twobyte)/sizeof(t_amd64_opcode_entry));
      return 2;
    case fpu_reg:
      {
	t_uint32 offset, div1, div2, modulo;
	offset = ((char *)form - (char *)disamd64_fpu_reg)/sizeof(t_amd64_opcode_entry);
	div1 = offset >> 6;
	div2 = (offset >> 3) & 7;
	modulo = offset & 7;

	*mod_ret = 3;
	buf[0] = 0xd8 + div1;
	*opcode_ext_ret = div2;
	*rm_ret = modulo;
	return 1;
      }
    case fpu_mem:
      {
	t_uint32 offset, div, modulo;
	offset = ((char *)form - (char *)disamd64_fpu_mem)/sizeof(t_amd64_opcode_entry);
	div = offset >> 3;
	modulo = offset & 7;
	buf[0] = (t_uint8) 0xd8+div;
	*opcode_ext_ret = modulo;
	return 1;
      }
    case grp1a:
      buf[0] = '\x80';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP1a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp1b:
      buf[0] = '\x81';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP1b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp1c:
      buf[0] = '\x82';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP1c-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp1d:
      buf[0] = '\x83';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP1d-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp2a:
      buf[0] = '\xc0';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP2a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp2b:
      buf[0] = '\xc1';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP2b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp3a:
      buf[0] = '\xc6';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP3a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp3b:
      buf[0] = '\xc7';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP3b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp4a:
      buf[0] = '\xd0';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP4a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp4b:
      buf[0] = '\xd1';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP4b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp4c:
      buf[0] = '\xd2';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP4c-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp4d:
      buf[0] = '\xd3';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP4d-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp5a:
      buf[0] = '\xf6';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP5a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp5b:
      buf[0] = '\xf7';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP5b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp6a:
      buf[0] = '\xfe';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP6a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp6b:
      buf[0] = '\xff';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP6b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 1;
    case grp7a:
      buf[0] = '\x0f';
      buf[1] = '\x00';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP7a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 2;
    case grp7b:
      buf[0] = '\x0f';
      buf[1] = '\x01';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP7b-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 2;
    case grp8a:
      buf[0] = '\x0f';
      buf[1] = '\xba';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP8a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 2;
    case grp12:
      FATAL(("please implement assembling of grp12 instructions"));
    case grp13:
      FATAL(("please implement assembling of grp13 instructions"));
    case grp14:
      FATAL(("please implement assembling of grp14 instructions"));
    case grp15a:
      buf[0] = '\x0f';
      buf[1] = '\xae';
      *opcode_ext_ret = ((char *)form - (char *)(disamd64_grps[AMD64_GRP15a-AMD64_GRP1a]))/sizeof(t_amd64_opcode_entry);
      return 2;
    case grp15a11:
      FATAL(("please implement assembling of grp15a11 instructions"));
    case grp16:
      FATAL(("please implement assembling of grp16 instructions"));
    case pgrp1:
      buf[0] = '\x0F';
      buf[1] = '\x10';
      return 2;
    case pgrp2:
      buf[0] = '\x0F';
      buf[1] = '\x2C';
      return 2;
    case pgrp3:
      buf[0] = '\x0F';
      buf[1] = '\x2A';
      return 2; 
    case pgrp4:
      buf[0] = '\x0F';
      buf[1] = '\x11';
      return 2;    
    case pgrp5:
      FATAL(("please implement assembling of pgrp5 instructions"));
    case pgrp6:
      buf[0] = '\x0F';
      buf[1] = '\x57';
      return 2;
    case pgrp7:
      FATAL(("please implement assembling of pgrp7 instructions"));
    case pgrp8:
       buf[0] = '\x0F';
       buf[1] = '\x5A';
       return 2;
    case pgrp9:
      buf[0] = '\x0F';
      buf[1] = '\x28';
      return 2;
    case pgrpA:
      buf[0] = '\x0F';
      buf[1] = '\x5F';
      return 2;
    case pgrpB:
      buf[0] = '\x0F';
      buf[1] = '\xC2';
      return 2;
    case pgrpC:
      buf[0] = '\x0F';
      buf[1] = '\x55';
      return 2;
    case pgrpD:
      buf[0] = '\x0F';
      buf[1] = '\x54';
      return 2;      
    case pgrpE:
      buf[0] = '\x0F';
      buf[1] = '\x56';
      return 2;
    case pgrpF:
      FATAL(("please implement assembling of pgrpF instructions"));
    case pgrp10:
      FATAL(("please implement assembling of pgrp10 instructions"));
    case pgrp11:
      FATAL(("please implement assembling of pgrp11 instructions"));
    case pgrp12:
      FATAL(("please implement assembling of pgrp12 instructions"));
    case pgrp13:
      FATAL(("please implement assembling of pgrp13 instructions"));
    case pgrp14:
      FATAL(("please implement assembling of pgrp14 instructions"));
    case pgrp15:
      FATAL(("please implement assembling of pgrp15 instructions"));
    case pgrp16:
      FATAL(("please implement assembling of pgrp16 instructions"));
    case pgrp17:
      FATAL(("please implement assembling of pgrp17 instructions"));
    case pgrp18:
      FATAL(("please implement assembling of pgrp18 instructions"));
    case pgrp19:
      FATAL(("please implement assembling of pgrp19 instructions"));
    case pgrp1A:
      FATAL(("please implement assembling of pgrp1A instructions"));
    case pgrp1B:
      FATAL(("please implement assembling of pgrp1B instructions"));
    case pgrp1C:
      FATAL(("please implement assembling of pgrp1C instructions"));
    case pgrp1D:
      FATAL(("please implement assembling of pgrp1D instructions"));
    case pgrp1E:
      FATAL(("please implement assembling of pgrp1E instructions"));
    case pgrp1F:
      FATAL(("please implement assembling of pgrp1F instructions"));
    case pgrp20:
      FATAL(("please implement assembling of pgrp20 instructions"));
    case pgrp21:
      FATAL(("please implement assembling of pgrp21 instructions"));
    case pgrp22:
      FATAL(("please implement assembling of pgrp22 instructions"));
    case pgrp23:
      FATAL(("please implement assembling of pgrp23 instructions"));
    case pgrp24:
      FATAL(("please implement assembling of pgrp24 instructions"));
    case pgrp25:
      FATAL(("please implement assembling of pgrp25 instructions"));
    case pgrp26:
      FATAL(("please implement assembling of pgrp26 instructions"));
    case pgrp27:
      FATAL(("please implement assembling of pgrp27 instructions"));
    case pgrp28:
      FATAL(("please implement assembling of pgrp28 instructions"));
    case pgrp29:
      FATAL(("please implement assembling of pgrp29 instructions"));
    case pgrp2A:
      FATAL(("please implement assembling of pgrp2A instructions"));
    case pgrp2B:
      FATAL(("please implement assembling of pgrp2B instructions"));
    case pgrp2C:
      buf[0]='\x0F';
      buf[1]='\x7E';
      return 2;
    case pgrp2D:
      FATAL(("please implement assembling of pgrp2D instructions"));
    case pgrp2E:
      FATAL(("please implement assembling of pgrp2E instructions"));
    case pgrp2F:
      FATAL(("please implement assembling of pgrp2F instructions"));
    case pgrp30:
      FATAL(("please implement assembling of pgrp30 instructions"));
    case pgrp31:
      FATAL(("please implement assembling of pgrp31 instructions"));
    case pgrp32:
      FATAL(("please implement assembling of pgrp32 instructions"));
    case pgrp33:
      buf[0]='\x0F';
      buf[1]='\x12';
      return 2;
    case pgrp34:
      FATAL(("please implement assembling of pgrp34 instructions"));
    case pgrp35:
      buf[0]='\x0F';
      buf[1]='\x58';
      return 2;
    case pgrp36:
      buf[0]='\x0F';
      buf[1]='\x29';
      return 2;
    case pgrp37:
      buf[0]='\x0F';
      buf[1]='\x5C';
      return 2;
    case pgrp38:
      FATAL(("please implement assembling of pgrp38 instructions"));
    case pgrp39:
      buf[0]='\x0F';
      buf[1]='\x59';
      return 2;
    case pgrp3A:
      FATAL(("please implement assembling of pgrp3A instructions"));
    case pgrp3B:
      FATAL(("please implement assembling of pgrp3B instructions"));
    case pgrp3C:
      FATAL(("please implement assembling of pgrp3C instructions"));
    case pgrp3D:
      FATAL(("please implement assembling of pgrp3D instructions"));
    case pgrp3E:
      FATAL(("please implement assembling of pgrp3E instructions"));
    case pgrp3F:
      FATAL(("please implement assembling of pgrp3F instructions"));
    case pgrp40:
      FATAL(("please implement assembling of pgrp40 instructions"));
    case pgrp41:
      FATAL(("please implement assembling of pgrp41 instructions"));
    case pgrp42:
      FATAL(("please implement assembling of pgrp42 instructions"));
    case pgrp43:
      FATAL(("please implement assembling of pgrp43 instructions"));
    case pgrp44:
      FATAL(("please implement assembling of pgrp44 instructions"));
    case pgrp45:
      FATAL(("please implement assembling of pgrp45 instructions"));
    case pgrp46:
      buf[0]='\x0F';
      buf[1]='\x5E';
      return 2;
    case pgrp47:
      buf[0]='\x0F';
      buf[1]='\x2E';
      return 2;
    case pgrp48:
      buf[0]='\x0F';
      buf[1]='\x51';
      return 2;
    case pgrp49:
      buf[0]='\x0F';
      buf[1]='\x14';
      return 2;
    case pgrp4A:
      buf[0]='\x0F';
      buf[1]='\xD6';
      return 2;
    case pgrp4B:
      buf[0]='\x0F';
      buf[1]='\x5D';
      return 2;
    case pgrp4C:
      FATAL(("please implement assembling of pgrp4C instructions"));
    case pgrp4D:
      FATAL(("please implement assembling of pgrp4D instructions"));
    case pgrp4E:
      FATAL(("please implement assembling of pgrp4E instructions"));
    case pgrp4F:
      FATAL(("please implement assembling of pgrp4F instructions"));
    case pgrp50:
      FATAL(("please implement assembling of pgrp50 instructions"));
    case pgrp51:
      FATAL(("please implement assembling of pgrp51 instructions"));
    case pgrp52:
      FATAL(("please implement assembling of pgrp52 instructions"));
    case pgrp53:
      FATAL(("please implement assembling of pgrp53 instructions"));
    case pgrp54:
      FATAL(("please implement assembling of pgrp54 instructions"));
    case pgrp55:
      FATAL(("please implement assembling of pgrp55 instructions"));
    case pgrp56:
      FATAL(("please implement assembling of pgrp56 instructions"));
    case pgrp57:
      FATAL(("please implement assembling of pgrp57 instructions"));
    case pgrp58:
      FATAL(("please implement assembling of pgrp58 instructions"));
    case pgrp59:
      FATAL(("please implement assembling of pgrp59 instructions"));
    case pgrp5A:
      FATAL(("please implement assembling of pgrp5A instructions"));
    case pgrp5B:
      FATAL(("please implement assembling of pgrp5B instructions"));
    case pgrp5C:
      FATAL(("please implement assembling of pgrp5C instructions"));
    case pgrp5D:
      FATAL(("please implement assembling of pgrp5D instructions"));
    case pgrp5E:
      FATAL(("please implement assembling of pgrp5E instructions"));
    case pgrp5F:
      FATAL(("please implement assembling of pgrp5F instructions"));
    default:
      FATAL(("group not added to cases %d ",type));
      /* keep the compiler happy */
      break;
  }
  FATAL(("Should never get here"));
  return 0;
} /* }}} */

int cvtlo8tohi8(t_amd64_ins * ins){
  int ret=0;
  if(AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg
      && AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo8
      && AMD64_OP_BASE(AMD64_INS_DEST(ins))>=AMD64_REG_RSI
      && AMD64_OP_BASE(AMD64_INS_DEST(ins))<=AMD64_REG_RSP){
    AMD64_OP_REGMODE(AMD64_INS_DEST(ins))=amd64_regmode_hi8;
    switch(AMD64_OP_BASE(AMD64_INS_DEST(ins))){
      case AMD64_REG_RSI:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RDX;
	break;
      case AMD64_REG_RDI:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RBX;
	break;
      case AMD64_REG_RBP:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RCX;
	break;
      case AMD64_REG_RSP:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RAX;
	break;
    }
    ret=1;
  }
  if(AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg
      && AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) == amd64_regmode_lo8
      && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))>=AMD64_REG_RSI
      && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))<=AMD64_REG_RSP){
    AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins))=amd64_regmode_hi8;
    switch(AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))){
      case AMD64_REG_RSI:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RDX;
	break;
      case AMD64_REG_RDI:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RBX;
	break;
      case AMD64_REG_RBP:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RCX;
	break;
      case AMD64_REG_RSP:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RAX;
	break;
    }
    ret=1;
  }
  if(AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg
      && AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins)) == amd64_regmode_lo8
      && AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))>=AMD64_REG_RSI
      && AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))<=AMD64_REG_RSP){
    AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins))=amd64_regmode_hi8;
    switch(AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))){
      case AMD64_REG_RSI:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RDX;
	break;
      case AMD64_REG_RDI:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RBX;
	break;
      case AMD64_REG_RBP:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RCX;
	break;
      case AMD64_REG_RSP:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RAX;
	break;
    }
    ret=1;
  }
  return ret;
}

void cvthi8tolo8(t_amd64_ins * ins){
  if(AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg
     && AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_hi8){
    AMD64_OP_REGMODE(AMD64_INS_DEST(ins))=amd64_regmode_lo8;
    switch(AMD64_OP_BASE(AMD64_INS_DEST(ins))){
      case AMD64_REG_RAX:
        AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RSP;
        break;
      case AMD64_REG_RBX:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RDI;
	break;
      case AMD64_REG_RCX:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RBP;
	break;
      case AMD64_REG_RDX:
	AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RSI;
	break;
    }
  }
  if(AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg
     && AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) == amd64_regmode_hi8){
    AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins))=amd64_regmode_lo8;
    switch(AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))){
      case AMD64_REG_RAX:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RSP;
	break;
      case AMD64_REG_RBX:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RDI;
	break;
      case AMD64_REG_RCX:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RBP;
	break;
      case AMD64_REG_RDX:
	AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RSI;
	break;
    }
  }
  if(AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg
     && AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins)) == amd64_regmode_hi8){
    AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins))=amd64_regmode_lo8;
    switch(AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))){
      case AMD64_REG_RAX:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RSP;
	break;
      case AMD64_REG_RBX:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RDI;
	break;
      case AMD64_REG_RCX:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RBP;
        break;
      case AMD64_REG_RDX:
	AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RSI;
	break;
    }
  }
}

/* {{{ Assemble instruction into a specific form */
t_uint32 Amd64AssembleToSpecificForm(t_amd64_ins * ins, t_amd64_opcode_entry * form, t_uint8 * buf)
{
  t_uint32 len;
  t_uint32 mod = 0, reg = 0, rm = 0;
  t_uint32 scale = 0, index = 0, base = 0;
  t_uint64 immediate;
  t_uint64  displacement;
  t_uint32 segsel;
  unsigned int immedsz = 0, dispsz = 0;
  t_bool has_modrm = FALSE, has_sib = FALSE;
  t_bool has_segsel = FALSE;
  t_reloc * disp_reloc = NULL, * imm_reloc = NULL;
  t_amd64_operand * op;
  char rex;

  has_modrm = form->has_modrm;

  len = WritePrefixes(ins,buf,0);
  
  len += Amd64WriteOpcode(ins, form, buf+len, &mod, &reg, &rm);
  

  /* set disp_reloc and imm_reloc if operands are relocated {{{ */
  AMD64_INS_FOREACH_OP(ins,op){
    if (AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)
    {
      if (AMD64_OP_TYPE(op) == amd64_optype_imm
	  || (AMD64_OP_TYPE(op) == amd64_optype_mem && !has_modrm)
	  || AMD64_OP_TYPE(op) == amd64_optype_farptr)
	imm_reloc = Amd64GetRelocForOp(ins,op);
      else if (AMD64_OP_TYPE(op) == amd64_optype_mem && has_modrm)
	disp_reloc = Amd64GetRelocForOp(ins,op);
      else
	FATAL(("optype should not be reloced\n"));
    }
  }
  /* }}} */

  rex=cvtlo8tohi8(ins)?0x40:0x00;
    
  /* now determine mod/rm and sib byte and displacement and immediate field */
  form->op1as(ins,AMD64_INS_DEST(ins), form->op1bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel,&rex);
  form->op2as(ins,AMD64_INS_SOURCE1(ins), form->op2bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel,&rex);
  form->op3as(ins,AMD64_INS_SOURCE2(ins), form->op3bm, &mod, &reg, &rm, &scale, &index, &base, &has_sib, &immediate, &immedsz, &displacement, &dispsz, &segsel, &has_segsel,&rex);

  if(rex!=0)
    cvthi8tolo8(ins);
  
  if(has_modrm){
    if(reg>7){
      reg-=8;
      rex=rex|0x04;
    }
    if(has_sib){
      if(index>7){
	index-=8;
	rex=rex|0x02;
      }
      if(base>7){
	base-=8;
	rex=rex|0x01;
      }
    }else{
      if(rm>7){
	rm-=8;
	rex=rex|0x01;
      }
    }
  }else{
    if(  (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg 
	&&AMD64_OP_BASE(AMD64_INS_DEST(ins)) >= AMD64_REG_R8 
	&&AMD64_OP_BASE(AMD64_INS_DEST(ins)) <= AMD64_REG_R15)
      || (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg
	&&AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) >= AMD64_REG_R8 
	&&AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) <= AMD64_REG_R15)
      || (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg
	&&AMD64_OP_BASE(AMD64_INS_SOURCE2(ins)) >= AMD64_REG_R8 
	&&AMD64_OP_BASE(AMD64_INS_SOURCE2(ins)) <= AMD64_REG_R15)){
      rex=rex|0x01;
    }
  }
 
  if(rex!=0){
    rex=rex|0x40;
  }
  len = WritePrefixes(ins,buf,rex);
  len += Amd64WriteOpcode(ins, form, buf+len, &mod, &reg, &rm);
  
  if (has_modrm)
  {
    t_uint8 modrm = ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7);
    buf[len++] = modrm;
    if (has_sib)
    {
      t_uint8 sib = ((scale & 3) << 6) | ((index & 7) << 3) | (base & 7);
      buf[len++] = sib;
    }
    switch (dispsz)
    {
      case 0:
	break;
      case 1:
	if(disp_reloc != NULL)
	  RELOC_SET_FROM_OFFSET(disp_reloc, AddressNew64(len));
	buf[len++] = displacement & 0xff;
	break;
      case 4:
	if(disp_reloc != NULL)
	  RELOC_SET_FROM_OFFSET(disp_reloc, AddressNew64(len));
	buf[len++] = displacement & 0xff;
	buf[len++] = (displacement & 0xff00) >> 8;
	buf[len++] = (displacement & 0xff0000) >> 16;
	buf[len++] = (displacement & 0xff000000) >> 24;
	break;
      default:
	FATAL(("unexpected displacement size (%d bytes) for @I",dispsz,ins));
    }
  }

  switch (immedsz)
  {
    case 0:
      break;
    case 1:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew64(len));
      buf[len++] = immediate & 0xff;
      break;
    case 2:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew64(len));
      buf[len++] = immediate & 0xff;
      buf[len++] = (immediate & 0xff00) >> 8;
      break;
    case 4:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew64(len));
      buf[len++] = immediate & 0xff;
      buf[len++] = (immediate & 0xff00) >> 8;
      buf[len++] = (immediate & 0xff0000) >> 16;
      buf[len++] = (immediate & 0xff000000) >> 24;
      break;
    case 8:
      if(imm_reloc != NULL)
	RELOC_SET_FROM_OFFSET(imm_reloc, AddressNew64(len));
      buf[len++] = (immediate & 0xff);
      buf[len++] = (immediate & 0xff00) >> 8;
      buf[len++] = (immediate & 0xff0000) >> 16;
      buf[len++] = (immediate & 0xff000000) >> 24;
      buf[len++] = (immediate & 0xff00000000ULL) >> 32;
      buf[len++] = (immediate & 0xff0000000000ULL) >> 40;
      buf[len++] = (immediate & 0xff000000000000ULL) >> 48;
      buf[len++] = (immediate & 0xff00000000000000ULL) >> 56;
      break;
    default:
      FATAL(("Unexpected immediate size (%d bytes) ", immedsz));
  }

  if (has_segsel)
  {
    buf[len++] = segsel & 0xff;
    buf[len++] = (segsel & 0xff00) >> 8;
  }

  return len;
} /* }}} */

/* Returns number of possible encodings, regardless of IMMEDSIZE  {{{*/
t_uint32 Amd64GetNumberOfEncodings(t_amd64_ins * ins, t_amd64_opcode_entry * forms[], t_uint32 nfitting)
{
  t_uint32 nchecked = 0;

  if(AMD64_INS_OPCODE(ins)==AMD64_LEA && AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins))==4){
    AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins))=1;
    nfitting+=Amd64GetNumberOfEncodings(ins, forms, nfitting);
    if(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins))==0){
      AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins))=0;
      nfitting+=Amd64GetNumberOfEncodings(ins, forms, nfitting);
    }
    AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins))=4;
  }

  if (!Amd64InsIsConditional(ins) || 
      (AMD64_INS_OPCODE(ins) == AMD64_JRCXZ) || (AMD64_INS_OPCODE(ins) == AMD64_LOOP) ||
      (AMD64_INS_OPCODE(ins) == AMD64_LOOPZ) || (AMD64_INS_OPCODE(ins) == AMD64_LOOPNZ))
  {
    /* {{{ regular instructions */
    t_amd64_opcode opcode=AMD64_INS_OPCODE(ins);
    void * key = (void *) &(opcode);

    t_amd64_opcode_he * he = HashTableLookup(amd64_opcode_hash, key);
    while (he)
    {
      t_amd64_opcode_entry * entry = (t_amd64_opcode_entry *) he->entry;
      Amd64OpCheckFunc op1check, op2check, op3check;
      
      if(entry->op1check == Amd64OpChecksI)
	op1check=Amd64OpChecksI2;
      else if(entry->op1check == Amd64OpCheckI)
	op1check=Amd64OpCheckI2;
      else{
       	op1check = entry->op1check;
      }
 
      if(entry->op2check == Amd64OpChecksI){
	op2check=Amd64OpChecksI2;
      }
      else if(entry->op2check == Amd64OpCheckI)
	op2check=Amd64OpCheckI2;
      else op2check = entry->op2check;
      
      if(entry->op3check == Amd64OpChecksI)
	op3check=Amd64OpChecksI2;
      else if(entry->op3check == Amd64OpCheckI)
	op3check=Amd64OpCheckI2;
      else op3check = entry->op3check;

      nchecked++;
      if (op1check(AMD64_INS_DEST(ins),entry->op1bm)){
	if(op2check(AMD64_INS_SOURCE1(ins),entry->op2bm)){
	  if(op3check(AMD64_INS_SOURCE2(ins),entry->op3bm)){
	    forms[nfitting++] = entry;
	  }
	}
      }
      he = (t_amd64_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    /* }}} */
  }
  else
  { 
    /*Conditional instructions {{{*/
    t_amd64_opcode_entry * entry = NULL;
    switch (AMD64_INS_OPCODE(ins))
    {
      case AMD64_SETcc:
	entry =&(disamd64_twobyte[0x90 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_CMOVcc:
	entry =&(disamd64_twobyte[0x40 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_Jcc:
	entry =&(disamd64[0x70 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_FCMOVcc:
	FATAL(("Currently not supported"));
	break;
      default:
	FATAL(("unexpected opcode: @I",ins));
    }
    if (!(entry->op1check(AMD64_INS_DEST(ins),entry->op1bm) && 
	  entry->op2check(AMD64_INS_SOURCE1(ins),entry->op2bm) && 
	  entry->op3check(AMD64_INS_SOURCE2(ins),entry->op3bm))){
      nfitting=0;
    }
    else 
      nfitting=1;
    /*}}}*/
  }
  return nfitting;
}
/* }}} */

/* {{{ build a list of all possible instruction encodings */
/* expects forms to be an array of at least size 10, returns number of possible encodings */

t_uint32 Amd64GetPossibleEncodings(t_amd64_ins * ins, t_amd64_opcode_entry * forms[])
{
  t_uint32 nfitting = 0, nchecked = 0;

  if (!Amd64InsIsConditional(ins) || 
      (AMD64_INS_OPCODE(ins) == AMD64_JRCXZ) || (AMD64_INS_OPCODE(ins) == AMD64_LOOP) ||
      (AMD64_INS_OPCODE(ins) == AMD64_LOOPZ) || (AMD64_INS_OPCODE(ins) == AMD64_LOOPNZ))
  {
    /* {{{ regular instructions */
    t_amd64_opcode opcode=AMD64_INS_OPCODE(ins);
    void * key = (void *) &(opcode);

    t_amd64_opcode_he * he = HashTableLookup(amd64_opcode_hash, key);

    while (he)
    {
      t_amd64_opcode_entry * entry = (t_amd64_opcode_entry *) he->entry;
      nchecked++;
      if (entry->op1check(AMD64_INS_DEST(ins),entry->op1bm) && entry->op2check(AMD64_INS_SOURCE1(ins),entry->op2bm) && entry->op3check(AMD64_INS_SOURCE2(ins),entry->op3bm))
      {
	forms[nfitting++] = entry;
      }
      he = (t_amd64_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    if (nfitting > 10)
      FATAL(("Make the forms array larger!"));
    /* }}} */
  }
  else
  { 
    /* {{{ conditional instructions */
    /* special case: there is not enough assembly information available
     * in the disamd64* tables to determine the correct opcode entry for the different
     * condition codes in conditional instructions, so we do the selection here, manually */
    switch (AMD64_INS_OPCODE(ins))
    {
      case AMD64_Jcc:
	nchecked = nfitting = 1;
	if (AMD64_OP_IMMEDSIZE(AMD64_INS_SOURCE1(ins)) == 1)
	  forms[0] = &(disamd64[0x70 + AMD64_INS_CONDITION(ins)]);
	else 
	  forms[0] = &(disamd64_twobyte[0x80 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_SETcc:
	nchecked = nfitting = 1;
	forms[0] = &(disamd64_twobyte[0x90 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_CMOVcc:
	nchecked = nfitting = 1;
	forms[0] = &(disamd64_twobyte[0x40 + AMD64_INS_CONDITION(ins)]);
	break;
      case AMD64_FCMOVcc:
	{
	  t_uint32 regno = AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) - AMD64_REG_ST0;
	  nchecked = 8;
	  nfitting = 1;
	  switch (AMD64_INS_CONDITION(ins))
	  {
	    case AMD64_CONDITION_B:
	      forms[0] = &(disamd64_fpu_reg[2][0][regno]);
	      break;
	    case AMD64_CONDITION_Z:
	      forms[0] = &(disamd64_fpu_reg[2][1][regno]);
	      break;
	    case AMD64_CONDITION_BE:
	      forms[0] = &(disamd64_fpu_reg[2][2][regno]);
	      break;
	    case AMD64_CONDITION_P:
	      forms[0] = &(disamd64_fpu_reg[2][3][regno]);
	      break;
	    case AMD64_CONDITION_AE:
	      forms[0] = &(disamd64_fpu_reg[3][0][regno]);
	      break;
	    case AMD64_CONDITION_NZ:
	      forms[0] = &(disamd64_fpu_reg[3][1][regno]);
	      break;
	    case AMD64_CONDITION_A:
	      forms[0] = &(disamd64_fpu_reg[3][2][regno]);
	      break;
	    case AMD64_CONDITION_NP:
	      forms[0] = &(disamd64_fpu_reg[3][3][regno]);
	      break;
	    default:
	      FATAL(("unexpected condition code for fcmovcc instruction"));
	  }
	}
	break;
      default:
	FATAL(("unexpected opcode: @I",ins));
    }
	      
    if (!(forms[0]->op1check(AMD64_INS_DEST(ins),forms[0]->op1bm) && 
	  forms[0]->op2check(AMD64_INS_SOURCE1(ins),forms[0]->op2bm) && 
	  forms[0]->op3check(AMD64_INS_SOURCE2(ins),forms[0]->op3bm)))
      nfitting = 0;
    /* }}} */
  }
  return nfitting;
}
/* }}} */

/* {{{ assemble an instruction into the shortest form possible */
t_uint32 Amd64AssembleIns(t_amd64_ins * ins, t_uint8 * buf)
{
  if (AMD64_INS_TYPE(ins) == IT_DATA)
  {
    buf[0] = AMD64_INS_DATA(ins);
    return 1;
  }
  else
  {
    int nfitting = 0;
    t_amd64_opcode_entry * forms[10];   /* this is certainly large enough */
    t_uint32 minlen = 10000000, minindex = 0, i;

/*    VERBOSE(0,("assembling @I",ins));*/

#ifdef ALTERNATIVES_SAFE
    nfitting = Amd64GetShortestEncodings(ins,forms);
#else
    nfitting = Amd64GetPossibleEncodings(ins,forms);
#endif

    ASSERT(nfitting != 0, ("found no possible encodings for Instruction: @I",ins));
	
      /* forms[] now contains a list of all possible assembly forms for the
       * instruction */
      if (nfitting == 1)
	return Amd64AssembleToSpecificForm(ins, forms[0], buf);
    
	for (i = 0; i < nfitting; i++)
	{ 
	  t_uint32 len = Amd64AssembleToSpecificForm(ins,forms[i],buf);
	  minindex = minlen < len ? minindex : i;
	  minlen = minlen < len ? minlen : len;
	}
        
	      
  
      return Amd64AssembleToSpecificForm(ins,forms[minindex],buf);


      }

/* keep the compiler happy */
return 0;
} /* }}} */


/* {{{ AssembleSection */
void Amd64AssembleSection(t_section * sec)
{
  t_uint32 total;
  t_uint32 len;
  int nins = 0;
  t_amd64_ins * ins;

  t_uint8 * data = SECTION_TMP_BUF(sec);
  t_uint8 * resized;

  /* if we've only disassembled and reassembled without flowgraphing, we have
   * to give all instructions their old length back (because otherwise we'd
   * need to relocate all jump offsets etc, which is impossible without a flow
   * graph). however, this is not always possible (the diablo instruction
   * representation doesn't allow an exact description of the instruction
   * encoding), so if necessary we pad the instruction with noops until it
   * reaches the desired length.
   */
  total = 0;
  for ( ins = T_AMD64_INS(SECTION_DATA(sec)); ins; ins = AMD64_INS_INEXT(ins))
  {
    len = Amd64AssembleIns(ins,data);
    
    if (len > AddressExtractUint32(AMD64_INS_CSIZE(ins))){
      char prinst[128];
      Amd64InstructionPrint(ins, prinst);
      FATAL(("%s: size corrupt: reports @G should be 0x%x",prinst,AMD64_INS_CSIZE(ins),len));
    }
    else
    {
      if (AMD64_INS_INEXT(ins)){
	for (;len < AddressExtractUint32(AMD64_INS_CSIZE(ins)); len++)
	  data[len] = '\x90';
      }
    }

    nins++;
    data += len;
    total += len;
  }

  {
    resized=Malloc(sizeof(t_uint8)*total);
    data = SECTION_TMP_BUF(sec);
    memcpy(resized,data,(size_t) total);

    Free(data);

    VERBOSE(0,("section prev size @G new size 0x%x\n", SECTION_CSIZE(sec),total));
    SECTION_SET_TMP_BUF(sec, resized);
    SECTION_SET_CSIZE(sec, AddressNew64(total));
  }

  printf("assembled %d instructions, for a total of 0x%x bytes\n",nins,total);

  /* clean up the opcode hash table (no longer needed if all sections are assembled) */
  if (sec == OBJECT_CODE(SECTION_OBJECT(sec))[OBJECT_NCODES(SECTION_OBJECT(sec))-1])
    HashTableFree(amd64_opcode_hash);

} /* }}} */

/* Defined in amd64_nasm/amd64_nasm.c */
int amd64_assembleFromStringNoErr(char * string, char * result);

t_bool Amd64ParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before)
{
  FATAL(("Reimplement"));
#if 0
  char tmp_string[15];
  char * a2i_result = amd64_a2i(ins_text);

  if(a2i_result)
  {
    t_ins * new = InsNewForBbl(bbl);
    if(at_ins)
    {
      if(before == TRUE)
	InsInsertBefore(new, at_ins);
      else
	InsInsertAfter(new, at_ins);
    }
    else
      InsPrependToBbl(new,bbl);
    
    if(amd64_assembleFromStringNoErr(a2i_result,tmp_string) > 0)
    {
      Amd64DisassembleOne((t_amd64_ins *)new,tmp_string,NULL,0,0);
      return TRUE;
    }
    else 
    {
      InsKill(new);
    }
  }

  return FALSE;
#endif
}

/* vim: set shiftwidth=2 foldmethod=marker: */
