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
/* PpcAssembleSection {{{ */
/*!
 *  
 * Assemble the entire section. The assembled instructions are written to the
 * temporary buffer of the section (allocated by DiabloAction).
 *
 * \param sec The section to assemble
 *
 * \return void 
*/
void
PpcAssembleSection(t_section * sec)
{
  int nins = 0;
  t_ppc_ins * i_ins;
  char * data = SECTION_TMP_BUF(sec);
  t_cfg * cfg;

  i_ins = (t_ppc_ins *) SECTION_DATA(sec); 
  cfg=PPC_INS_CFG(i_ins);

  while(i_ins != NULL)
  {

    /* we have to reassemble it */
    /* Use ppc assemble one to assemble the instruction */
    i_ins->Assemble(i_ins, (t_uint32*)data);

    if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    {
      (*((t_uint32 *) data)) = Uint32SwapEndian(*((t_uint32 *) data));
    }

    nins++;
    data += 4;

#if 0
    /* TODO: Move me to DiabloAction */
    while ((rr = PPC_INS_REFED_BY(i_ins)))
    {
      t_uint32 i;
      rel = RELOC_REF_RELOC(rr);
      for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
      {
        if (RELOC_TO_RELOCATABLE_REF(rel)[i]==rr)
        {
          RelocSetToRelocatable(rel, i, T_RELOCATABLE(sec));
          RELOC_TO_RELOCATABLE_OFFSET(rel)[i] = AddressAdd(RELOC_TO_RELOCATABLE_OFFSET(rel)[i], AddressSub(PPC_INS_CADDRESS(i_ins),SECTION_CADDRESS(sec)));
        }
      }
    }

    while ((rr = PPC_INS_REFERS_TO(i_ins)))
    {
      rel = RELOC_REF_RELOC(rr);
      RelocSetFrom(rel,T_RELOCATABLE(sec));
      RELOC_SET_FROM_OFFSET(rel, AddressAdd(RELOC_FROM_OFFSET(rel),AddressSub(PPC_INS_CADDRESS(i_ins),SECTION_CADDRESS(sec))));
    }
    /* END TODO */
#endif 
    i_ins = PPC_INS_INEXT(i_ins); 
  }

  /* Verbose information */
  VERBOSE(1,("Assembled %d instructions",nins));

}
/* }}} */
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
