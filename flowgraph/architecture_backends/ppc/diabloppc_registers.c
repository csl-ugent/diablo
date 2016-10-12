/*
 * Copyright (C) 2005 {{{
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

t_string PpcRegisterName(t_reg reg) 
{
  if(reg==PPC_REG_NONE)
  {
    return "";
  }
  return ppc_description.register_names[reg];
}

t_uint32 PpcRegisterSpecial(t_uint32 reg)
{
  switch(reg)
  {
    case 0x020:
      return PPC_REG_XER;
    case 0x100:
      return PPC_REG_LR;
    case 0x120:
      return PPC_REG_CTR;
#ifdef PPC_ALTIVEC_SUPPORT
    case 0x008:
      return PPC_REG_ALTIVEC_VRSAVE; 
#endif
    case 0x240:
      return PPC_REG_DSISR;
    case 0x260:
      return PPC_REG_DAR;
    //case 0x2C0:
    //  return PPC_REG_DEC;
    case 0x320:
      return PPC_REG_SDR1;
    case 0x340:
      return PPC_REG_SRR0;
    case 0x360:
      return PPC_REG_SRR1;
    //case 0x3A0:
    //  return PPC_REG_ACCR;
    //case 0x104:
    //  return PPC_REG_CTRL;
    case 0x208:
      return PPC_REG_SPRG0;
    case 0x228:
      return PPC_REG_SPRG1;
    case 0x248:
      return PPC_REG_SPRG2;
    case 0x268:
      return PPC_REG_SPRG3;
    case 0x3E8:
      return PPC_REG_PVR;
    default:
      FATAL(("Unknown special register value: %x",reg));
  }
}

t_uint32 PpcRegisterTimeBase(t_uint32 reg)
{
  if(reg==0x188)
  {
    return PPC_REG_TBL;
  }
  else if(reg==0x1A8)
  {
    return PPC_REG_TBU;
  }
  else
  {
    FATAL(("Unknown time base register value: %x",reg));
  }
}

t_uint32 PpcSetRegisterSpecial(t_uint32 reg)
{
  switch(reg)
  {
#ifdef PPC_ALTIVEC_SUPPORT
    case PPC_REG_ALTIVEC_VRSAVE:
      return 0x008;
#endif
    case PPC_REG_XER:
        return 0x020;
    case PPC_REG_LR:
        return 0x100;
    case PPC_REG_CTR:
        return 0x120;
    case PPC_REG_DSISR:
        return 0x240;
    case PPC_REG_DAR:
        return 0x260;
    //case PPC_REG_DEC:
    //    return 0x2C0;
    case PPC_REG_SDR1:
        return 0x320;
    case PPC_REG_SRR0:
        return 0x340;
    case PPC_REG_SRR1:
        return 0x360;
    //case PPC_REG_ACCR:
    //    return 0x3A0;
    //case PPC_REG_CTRL:
    //    return 0x104;
    case PPC_REG_SPRG0:
        return 0x208;
    case PPC_REG_SPRG1:
        return 0x228;
    case PPC_REG_SPRG2:
        return 0x248;
    case PPC_REG_SPRG3:
        return 0x268;
    case PPC_REG_PVR:
          return 0x3E8;
    default:
          FATAL(("Unknown special register value: %x",reg));
  }
}


t_uint32 PpcSetRegisterTimeBase(t_uint32 reg)
{
  if(reg==PPC_REG_TBL)
  {
    return 0x188;
  }
  else if(reg==PPC_REG_TBU)
  {
    return 0x1A8;
  }
  else
  {
    FATAL(("Unknown time base register value: %x",reg));
  }
}

int PpcRegisterToNumber(int x)
{
  int aux;
  
  if (x >= PPC_REG_ALTIVEC_VR0)
    aux = x - PPC_REG_ALTIVEC_VR0;
  else if (x >= PPC_REG_CR0)
    aux = x - PPC_REG_CR0;
  else if (x >= PPC_REG_F0)
    aux = x - PPC_REG_F0;
  else
    aux = x - PPC_REG_R0;
  
  return aux;
}

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
