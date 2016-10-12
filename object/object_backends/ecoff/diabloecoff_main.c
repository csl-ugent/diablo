#ifdef BIT64ADDRSUPPORT
#include <diabloecoff.h>

/* Section Names {{{ */

/*! Static array containing all known alpha-ecoff sections. Normally there
 * should be no other sections in an alpha_ecoff binary */ 
static char * ecoff_section_names[29]=
{
  /*  0 */   ".text",
  /*  1 */   ".init",
  /*  2 */   ".fini",
  /*  3 */   ".data",
  /*  4 */   ".rdata",
  /*  5 */   ".rconst",
  /*  6 */   ".sdata",
  /*  7 */   ".sbss",
  /*  8 */   ".bss",
  /*  9 */   ".lita",
  /* 10 */   ".lit4",
  /* 11 */   ".lit8",
  /* 12 */   ".xdata",
  /* 13 */   ".pdata",
  /* 14 */   ".tlsdata",
  /* 15 */   ".tlsbss",
  /* 16 */   ".tlsinit",
  /* 17 */   ".got",
  /* 18 */   ".dynamic",
  /* 19 */   ".dynsym",
  /* 20 */   ".rel.dyn",
  /* 21 */   ".dynstr",
  /* 22 */   ".hash",
  /* 23 */   ".msym",
  /* 24 */   ".conflict",
  /* 25 */   ".liblist",
  /* 26 */   ".comment",
  /* 27 */   ".ucode",
  /* 28 */   "INVALID"
};
/*}}}*/
/*!
 *
 * This function returns the name of the section referenced by a relocation
 * 
 * \param a relocation section type (as an integer) (e.g. R_SN_TEXT)
 *
 * \return t_string with the name of the section (e.g. ".text") or NULL if
 * there is no section name (ABS and NULL)
 */
/* EcoffRelocToSectionName {{{ */
static t_string EcoffRelocToSectionName(t_uint32 scn)
{
  switch(scn)
  {
    case R_SN_NULL:   return NULL;
    case R_SN_ABS:    return NULL; 
    case R_SN_TEXT:   return ecoff_section_names[0];
    case R_SN_INIT:   return ecoff_section_names[1];
    case R_SN_FINI:   return ecoff_section_names[2];
    case R_SN_DATA:   return ecoff_section_names[3];
    case R_SN_RDATA:  return ecoff_section_names[4];
    case R_SN_RCONST: return ecoff_section_names[5];
    case R_SN_SDATA:  return ecoff_section_names[6];
    case R_SN_SBSS:   return ecoff_section_names[7];
    case R_SN_BSS:    return ecoff_section_names[8];
    case R_SN_LITA:   return ecoff_section_names[9]; 
    case R_SN_LIT4:   return ecoff_section_names[10]; 
    case R_SN_LIT8:   return ecoff_section_names[11]; 
    case R_SN_XDATA:  return ecoff_section_names[12];
    case R_SN_PDATA:  return ecoff_section_names[13];
    case R_SN_TLSDATA:  return ecoff_section_names[16];
    default:          FATAL(("unknow relocation section %d",scn));
  }
  return NULL;
}
/* }}} */
/* EcoffSymbolStorageToSectionName {{{ */
static t_string EcoffSymbolStorageToSectionName(t_uint32 sc)
{
  switch(sc)
  {
    case scText: 
      return ecoff_section_names[0];
    case scData: 
      return ecoff_section_names[3];
    case scBss: 
      return ecoff_section_names[8];
    case scSData: 
      return ecoff_section_names[6];
    case scSBss: 
      return ecoff_section_names[7];
    case scRData: 
      return ecoff_section_names[4];
    case scInit: 
      return ecoff_section_names[1];
    case scXData: 
      return ecoff_section_names[12];
    case scPData: 
      return ecoff_section_names[13];
    case scFini: 
      return ecoff_section_names[2];
    case scRConst: 
      return ecoff_section_names[5];
    default: 
      FATAL(("Not a section storage class! %d",sc));
  }
  return NULL;
}
/* }}} */
/* EcoffRelocToSecIndex {{{ */
#if 0
static t_uint8 EcoffRelocToSecIndex(t_uint32 scn)
{
  switch(scn)
  {
    case R_SN_NULL:   return 28;
    case R_SN_ABS:    return 28; 
    case R_SN_TEXT:   return 0;
    case R_SN_INIT:   return 1;
    case R_SN_FINI:   return 2;
    case R_SN_DATA:   return 3;
    case R_SN_RDATA:  return 4;
    case R_SN_RCONST: return 5;
    case R_SN_SDATA:  return 6;
    case R_SN_SBSS:   return 7;
    case R_SN_BSS:    return 8;
    case R_SN_LITA:   return 9; 
    case R_SN_LIT4:   return 10; 
    case R_SN_LIT8:   return 11; 
    case R_SN_XDATA:  return 12;
    case R_SN_PDATA:  return 13;
    default: FATAL(("unknow relocation section %d\n",scn));
  }
  return 0;
}
#endif
/* }}} */
/*!  
 * This function returns true if the file pointed to is a (alpha)-eCOFF
 * objectfile. The filepointer stated is not changed
 *
 * \param fp a file-pointer pointing to the start of an objectfile
 *
 * \return t_bool true or false depending on the type of the objectfile
 */
/* IsEcoff {{{ */
t_bool IsEcoff(FILE * fp)
{
  unsigned char magic[2];
  t_uint64 fpos=ftell(fp);
  int ret=fread(magic,sizeof(char),2,fp);
  fseek(fp,fpos,SEEK_SET);
  if (ret!=2) return FALSE;
  if ((magic[0]==ALPHAMAGICZBYTE1)&&(magic[1]==ALPHAMAGICZBYTE2)) {
    VERBOSE(1,("File is a compressed Alpha-ECOFF objectfile")); 
    return TRUE; 
  }
  if ((magic[0]!=ALPHAMAGICBYTE1)||(magic[1]!=ALPHAMAGICBYTE2)) return FALSE;
  VERBOSE(1,("File is an Alpha-ECOFF objectfile")); 
  return TRUE;
}
/* }}} */
void EcoffProcessRelocsForSection(t_uint64 num_relocs, t_ecoff_reloc * relocs, t_object * obj, t_section * sec, t_symbol ** symtab, t_uint64 gpvalue)
{
  t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
  t_uint32 j;
  t_string code = NULL;
  /* Foreach reloc in this section:  */
  for (j=0; j<num_relocs; j++)
  {
    t_uint64 data;
    t_address addend;
    t_address generic;
    t_address null = AddressNew64(0);

    code=NULL;

    switch(relocs[j].r_type)
    {
      /* Ignore ABS relocs. They are already performed or they represent
       * the real number of relocs in an object. Normally we should not
       * encounter many of them... */
      case R_ABS:        
        break; 

      case R_REFQUAD:    
        {
          generic = AddressNew64(relocs[j].r_vaddr);
          data = SectionGetData64(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
          addend = AddressNew64(data);
          
	  if (relocs[j].r_extern==1)
          {
            code = CALC_ABS "\\" WRITE_64; 
            if (symtab[relocs[j].r_symndx]==NULL) 
              FATAL(("Global symbol %d is used by relocation %d from section %s, but not loaded by the symbol handling code. Fix this!",relocs[j].r_symndx, j, SECTION_NAME(sec)));

            RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),addend,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),symtab[relocs[j].r_symndx],TRUE,NULL,NULL,NULL,code);
          }
          else
          {
            t_reloc * rel;
            code = "R00A00+" "\\" WRITE_64; 
            t_section * tosec = SectionGetFromObjectByName(obj, EcoffRelocToSectionName(relocs[j].r_symndx));
            rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),T_RELOCATABLE(tosec),AddressSub(addend,SECTION_CADDRESS(tosec)),TRUE,NULL,NULL,NULL,code);
            RELOC_SET_LABEL(rel, StringDup("NONSYMBOLREFQUAD"));
          }
          break; 
        }
        /* GPREL32 relocation: a 32-bit displacement from the global pointer to the symbol's virtual address. */
      case R_GPREL32:    
        {
          t_uint32 data;
          t_section * lita=SectionGetFromObjectByName(obj,".lita");
          t_section * plita=SectionGetFromObjectByName(OBJECT_PARENT(obj),".lita");
          generic=AddressNew64(relocs[j].r_vaddr);
          data = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
          if (relocs[j].r_extern==1)
          {
            FATAL(("Implement global GPREL32!"));
          }
          else
          {
            code="R00R01I0000000000007ff0+-" "\\" "l*w s0000$";
            t_int32 sdata=data;  
            t_section * to_sec=SectionGetFromObjectByName(obj,EcoffRelocToSectionName(relocs[j].r_symndx));
            t_uint64 to_address=((t_int64) sdata)+((t_int64) G_T_UINT64(SECTION_CADDRESS(lita)))+0x7ff0LL;

            addend=AddressNew64(to_address);

            RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),T_RELOCATABLE(to_sec),AddressSub(addend,SECTION_CADDRESS(to_sec)),TRUE,NULL,NULL,T_RELOCATABLE(plita),code);
          }
        }

        break;
      case R_LITERAL: 
        {
          t_uint32 data32;
          t_int64 gpdata;
          t_reloc * rel;

          t_section * lita=SectionGetFromObjectByName(obj,".lita");

          ASSERT(lita, ("Could not find the .lita section (literal address pool)!"));

          generic=AddressNew64(relocs[j].r_vaddr);
          data32 = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
          gpdata=(data32 & 0xffffU);
          if (gpdata & 0x8000)
          {
            gpdata|=0xffffffffffff0000LLU;
          }

          if (gpvalue!=G_T_UINT64(SECTION_CADDRESS(lita))+32752) FATAL(("Strange gp value"));
          
          gpdata += gpvalue;
          gpdata -= G_T_UINT64(SECTION_CADDRESS(lita));

          if ((gpdata>=G_T_UINT64(SECTION_CSIZE(lita)))||(gpdata<0)) 
            FATAL(("Bad literal! %lx %x",relocs[j].r_vaddr,gpdata));

          if (!SECTION_IS_MAPPED(lita))
          {
            t_section * plita ;
            ASSERT(OBJECT_PARENT(obj), ("Reading relocs for an object that does not have a parent object!"));
            plita =  SectionGetFromObjectByName (OBJECT_PARENT(obj), ".lita");

            MAP_SECTION(lita, ".lita", plita);
          }

          addend=AddressNew64(gpdata);
          code="R00A00+R01-I0000000000007ff0-I000000000000ffff& \\ lI00000000ffff0000&|w s0000$";
          rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),T_RELOCATABLE(lita),addend,TRUE,NULL,NULL,T_RELOCATABLE(SECTION_PARENT_SECTION(lita)),code);
          break; 
        }
      case R_LITUSE:     
        /*printf("lit_use   : "); */
        break;
      case R_GPDISP:  
        {
          t_address ldah_adr;
          t_address lda_adr;
          t_address setter_offset;
          t_uint32 ldah;
          t_uint32 lda;
          //t_uint32 setter;
          t_int64 ldahval;
          t_int64 ldaval;
          t_section * lita = SectionGetFromObjectByName(obj,".lita");
          t_int64 res;
          t_reloc* rel;
          //t_bool move_extra_relocatable = FALSE;

          t_symbol * sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), "DIABLO:gp", "R00A00+$" , 0, PERHAPS, TRUE, T_RELOCATABLE(undef), AddressNullForObject(obj),  AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

          if (!lita)
          {
            t_address addr;
            addr=AddressNew64(gpvalue-32752);
            lita=SectionCreateForObject(obj, RODATA_SECTION, NULL, addr, ".lita");
            addr=AddressNew64(0);
            SECTION_SET_CSIZE(lita, addr);
            SECTION_SET_OLD_SIZE(lita, addr);
          }

          /* The address where we expect the ldah to be */
          ldah_adr=AddressNew64(relocs[j].r_vaddr);
          /* The address where we expect the lda to be */
          lda_adr=AddressNew64(relocs[j].r_vaddr+relocs[j].r_symndx);


          /* Load the ldah instruction */
          ldah = SectionGetData32(sec,AddressSub(ldah_adr,SECTION_CADDRESS(sec)));
          /* Load the lda instruction */
          lda = SectionGetData32(sec,AddressSub(lda_adr,SECTION_CADDRESS(sec)));


          /* Some checks: do we really have an ldah and an lda? */
          if (((ldah &0xfc000000) != 0x24000000) &&((ldah &0xfc000000) == 0x20000000) && ((lda &0xfc000000) == 0x24000000)) FATAL(("Swapped GPDISP not implemented!"));
          else if ((ldah &0xfc000000) != 0x24000000) FATAL(("GPDISP not with ldah in file %s (vaddr=@G)!",OBJECT_NAME(obj),ldah_adr));
          if ((lda  &0xfc000000) != 0x20000000) FATAL(("GPDISP not with lda!"));

          /* Another check: is the gp value what we expected? */ 
          if (gpvalue!=G_T_UINT64(SECTION_CADDRESS(lita))+32752) FATAL(("Strange gp value"));



          res=((t_int64) (gpvalue))- ((t_int64) relocs[j].r_vaddr);


          if (lda&0x8000) ldaval=(t_uint64)(0xffffffffffff0000ULL | (((t_uint64) lda)&0xffffULL));
          else ldaval=((t_uint64) lda)&0xffffULL;
          if (ldah&0x8000) ldahval=(t_uint64)(0xffffffffffff0000ULL | (((t_uint64) ldah)&0xffffULL));
          else ldahval=((t_uint64) ldah)&0xffffULL;

          addend=AddressNew64(ldahval*65536+ldaval-res);

          /* Phew..... Some explaining for this 
           * 
           * GPDISP calculates the gp as: gp = input address + ldahval * 65536 + ldaval
           * For the associated relocatable we want to know input address.
           *
           * input address = gp - (ldahval * 65536 + ldaval)
           *               = ldah_addr + gp - ldah_addr - (ldahval * 65536 + ldaval)
           *               = ldah_addr + res - (ldahval * 65536 + ldaval)
           *               = ldah_addr - (ldahval * 65536 + ldaval - res)
           *
           * input address offset = (ldah offset) - (ldahval * 65536 + ldaval - res) 
           *
           * It is possible this address is set by a preceding br or
           * bsr. The address of this instruction would be:
           *
           * setter_offset = input address - 4
           *               = (ldah offset) - (ldahval * 65536 + ldaval - res + 4 )
           */



          setter_offset=AddressSubInt32(AddressSub(ldah_adr,SECTION_CADDRESS(sec)),ldahval*65536+ldaval-res);
          addend=null;

          if (ldahval*65536+ldaval-res<0) FATAL(("<0"));

          /* This relocation calculates: 
           *
           * for ldah: ((((final gp)                                      - (input value))      + 65536) >> 16) & 0xffff
           *         = (((address of parent of the lita section + 0x7ff0) - (address of setter))+ 65536) >> 16) & 0xffff
           *         = OI0000000000007ff0+                                - R                   + 0x8000 >> 0x10 & 0xffff     
           * 
           * for lda: ((gp value + 65536 - associated) >> 16 ) & 0xffff */


          code="S00I0000000000008000+A00-R00-s0010> I000000000000ffff&\\ lI00000000ffff0000&|w s0000$";

          rel=RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj), addend, T_RELOCATABLE(sec), AddressSub(ldah_adr,SECTION_CADDRESS(sec)), sym, FALSE, NULL, NULL, NULL,code);
          RelocAddRelocatable(rel, T_RELOCATABLE(sec), setter_offset);


          code="S00S00I0000000000008000+A00-R00-s0010>#10s0010<R00+-I000000000000ffff&\\ lI00000000ffff0000&|w s0000$";
          rel=RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj), addend, T_RELOCATABLE(sec), AddressSub(lda_adr,SECTION_CADDRESS(sec)), sym, FALSE,NULL, NULL, NULL,code);
          RelocAddRelocatable(rel, T_RELOCATABLE(sec), setter_offset);

        }
        break;
      case R_BRADDR:     
        {
          t_int32 data;
          generic=AddressNew64(relocs[j].r_vaddr);
          data = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
          data&=0xffff;
          if (data&0x8000) data|=0xffff0000;
          data<<=2;
          data-=4;
          addend=AddressAddInt32(generic,data);
          if (relocs[j].r_extern==1)
          {
            code="S00A00+P-I0000000000000002>I0000000000000001+I000000000000ffff& \\ lI00000000ffff0000&|w s0000$"; 
            if (symtab[relocs[j].r_symndx]==NULL) FATAL(("Symbol global %d not set",relocs[j].r_symndx));

            RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),symtab[relocs[j].r_symndx],FALSE,NULL,NULL,NULL,code);
          }
          else
          {
            code="R00A00+P-I0000000000000002>I0000000000000001+I000000000000ffff& \\ lI00000000ffff0000&|w s0000$"; 
            t_section * sec=SectionGetFromObjectByName(obj,EcoffRelocToSectionName(relocs[j].r_symndx));
            RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),T_RELOCATABLE(sec),AddressSub(addend,SECTION_CADDRESS(sec)),FALSE,NULL,NULL,NULL,code);
          }
        }		
        break;
      case R_HINT:    
        {			
          code="S00A00+P-I0000000000000002>I0000000000000001-I0000000000003fff& \\ lI00000000ffffc000&|w s0000$"; 
          generic=AddressNew64(relocs[j].r_vaddr);
          data = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
          if (relocs[j].r_extern==1)
          {

            if (data&0x3fff) FATAL(("Hint has addend!"));

            addend=AddressNew64(0);
            if (symtab[relocs[j].r_symndx]==NULL) FATAL(("Symbol global %d not set",relocs[j].r_symndx));

            RelocTableAddRelocToSymbol(
                                       OBJECT_RELOC_TABLE(obj),
                                       addend,
                                       T_RELOCATABLE(sec),
                                       AddressSub(
                                                  generic,
                                                  SECTION_CADDRESS(sec)
                                                 ),
                                       symtab[relocs[j].r_symndx],
                                       FALSE,
                                       NULL,
                                       NULL,
                                       NULL,
                                       code
                                      );
          }
          else
          {
            /*
               t_section * sec=SectionGetFromObjectByName(obj,EcoffRelocToSectionName(relocs[j].r_symndx));
               data&=0x3fff;
               data<<=2;
               addend=AddressAddUint32(generic,data);
               RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),null,T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),T_RELOCATABLE(sec),AddressSub(addend,SECTION_CADDRESS(sec)),TRUE,NULL,NULL,NULL,code,FALSE);
               */
          }
        }		
        break;

      case R_SREL32:     code=CALC_REL "\\" WRITE_32; break;
      case R_OP_PUSH:    /*printf("op_push   : ");*/ break;
      case R_OP_STORE:   /*printf("op_store  : ");*/ break;
      case R_OP_PSUB:    /*printf("op_psub   : ");*/ break;
      case R_OP_PRSHIFT: /*printf("op_prshift: ");*/ break;
      case R_GPVALUE:    FATAL(("gpvalue")); break;
      case R_IMMED:     
                         break;
                         {
                           static t_int32 immed;
                           generic=AddressNew64(relocs[j].r_vaddr);
                           code="BJORN'S SPECIAL EFFECT";
                           if (relocs[j].r_size==R_IMMED_SCN_HI32)
                           {
#define ALPHAOPCODEBITS_MDISP(n) (n & 0xffff)
#define ALPHAOPCODEBITS_MSIGN(n) (n & 0x8000)  
                             immed = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
                             immed = ALPHAOPCODEBITS_MDISP(immed);
                             if( ALPHAOPCODEBITS_MSIGN(immed))
                               immed |= -1 ^ 0xffff;
                             immed <<= 16; 
                           }
                           else if (relocs[j].r_size==R_IMMED_LO32)
                           {
                             t_int32 immed2 = SectionGetData32(sec,AddressSub(generic,SECTION_CADDRESS(sec)));
                             t_address s_address;
                             char string[64];
                             t_address temp;
                             t_int64 immed64;
                             t_symbol* sym;
                             immed2 = ALPHAOPCODEBITS_MDISP(immed2);
                             if( ALPHAOPCODEBITS_MSIGN(immed2))
                               immed2 |= -1 ^ 0xffff;
                             immed += immed2;
                             immed64 = immed;
                             temp=AddressNew64(immed64);
                             s_address =  AddressAdd(temp,SECTION_CADDRESS(sec));
                             sprintf(string,"BJORN'S SPECIAL SYMBOL AT %7llx",relocs[j].r_vaddr);

                             FATAL(("WHAT THE F***"));
                             /*
                             sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), string, "R00A00+$" , 10, FALSE, FALSE, T_RELOCATABLE(secarray[0]),s_address,  AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0); */

                             RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(sec),AddressSub(generic,SECTION_CADDRESS(sec)),sym,TRUE,NULL,NULL,NULL,code);
                           }
                         }

                         printf("IMMED address %llx\n",relocs[j].r_vaddr);
                         printf("      subtype %d\n",relocs[j].r_size);
                         if (relocs[j].r_size==R_IMMED_LO32)
                         {
                           if (!relocs[j].r_extern)
                           {
                             printf("section number %d name %s\n",relocs[j].r_symndx,ecoff_section_names[relocs[j].r_symndx]);
                           }
                           else
                           {
                             printf("extern symbol index %d\n",relocs[j].r_symndx);
                           }
                         }

                         break;
                         /*		 

                                         static int teller = 0;
                                         teller++;
                                         printf("IMMED address %lx\n",relocs[j].r_vaddr);
                                         printf("      subtype %d\n",relocs[j].r_size);
                                         if (relocs[j].r_size==R_IMMED_LO32)
                                         {
                                         if (!relocs[j].r_extern)
                                         {
                                         printf("section number %d name %s\n",relocs[j].r_symndx,ecoff_section_names[relocs[j].r_symndx]);
                                         }
                                         else
                                         {
                                         printf("extern symbol index %d\n",relocs[j].r_symndx);
                                         }
                                         }
                                         if (teller==2)
                                         FATAL(("immed")); 
                                         break;
                                         */
      default: FATAL(("Implement  %d",relocs[j].r_type));  
    }
  }
}


void EcoffGetRelocsForHeader(t_ecoff_scn_hdr shdr, t_object * obj, FILE * fp, t_ecoff_reloc ** relocs, t_uint64 * num_relocs)
{
  t_uint64 stored2=ftell(fp);

  /* Normally the number of relocs in a ecoff object file is listed in
   * s_nrelocs. However, as this field is only 16 bits large, it might
   * overflow. In this case the S_NRELOC_OVFL is added to the secion flags
   * and a special relocation of type R_ABS is added to start of the
   * section reloc table which holds the real number of relocs in its
   * r_vaddr field. */

  if (shdr.s_flags & S_NRELOC_OVFL)
  {
    t_ecoff_reloc ovfl_reloc;
    fseek(fp,OBJECT_STREAMPOS(obj)+shdr.s_relptr,SEEK_SET);
    fread(&ovfl_reloc,sizeof(t_ecoff_reloc),1,fp);
    if (ovfl_reloc.r_type!=R_ABS) 
      FATAL(("Expected an ABS reloc (relocation overflow)"));
    *num_relocs=ovfl_reloc.r_vaddr;
  }
  else
  {
    *num_relocs=shdr.s_nreloc;
    fseek(fp,OBJECT_STREAMPOS(obj)+shdr.s_relptr,SEEK_SET);
  }

  /* After this, fp points to the start of the relocations for the section
   * at hand. We allocate space for the relocs and read them. */

  *relocs=Malloc(*num_relocs*sizeof(t_ecoff_reloc));
  fread(*relocs,sizeof(t_ecoff_reloc)*(*num_relocs),1,fp);

  fseek(fp, stored2, SEEK_SET);

}
/* ReadEcoff {{{ */
void EcoffRead(FILE * fp,t_object * obj, t_bool read_debug)
{
  t_ecoff_file_hdr fhdr;
  t_ecoff_aout_hdr ahdr;
  t_ecoff_scn_hdr  shdr;
  t_symbol ** symlookup=NULL;
  int ret;
  t_uint32 i;
  t_uint64 verify_start, verify_stop, verify_section_start;
  t_uint16 verify_section=-1;
  t_uint64 save, verify_section_fptr;
  t_uint64 stored=0;
  t_section ** secarray;

  ASSERT(obj, ("EcoffRead called without object (object = NULL)"));
  ASSERT(fp, ("EcoffRead called without open file pointer (fp = NULL)"));
  
  OBJECT_SET_ADDRESS_SIZE (obj, ADDRSIZE64);

  if (!OBJECT_PARENT(obj))
  {
    /* create global dummy sections (abs and undef) */
    OBJECT_SET_UNDEF_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*UNDEF*"));
    OBJECT_SET_ABS_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*ABS*"));
  }


  /* Read ecoff file header */
  ret=fread(&fhdr,sizeof(t_ecoff_file_hdr),1,fp);
  ASSERT(ret==1,("Corrupted  ecoff file header!\n"));
  
  /* Check what type of ecoff object we have compressed */

  if (fhdr.f_magic==0x188) 
  {
    /* A compressed object: The header is followed by the uncompressed size and padding (which should be zero).
     * We will decompress the object, write it to disk and let the normal routines load it. {{{ */
#ifdef HAVE_OBJZ_SUPPORT
    FILE * uncompressed_out_file;
    unsigned char * data;
#endif
    t_uint64 size;
    t_uint64 pad;
    fread(&size,sizeof(long),1,fp);
    fread(&pad,sizeof(long),1,fp);
    if (pad!=0) FATAL(("Corrupted compressed objectfile"));
#ifdef HAVE_OBJZ_SUPPORT
    lseek(fileno(fp),OBJECT_STREAMPOS(obj),SEEK_SET);
    fseek(fp,OBJECT_STREAMPOS(obj),SEEK_SET);
    data=(unsigned char *) Calloc(1,size);
    /*Decompress in a buffer (needs libmld)*/
    obj_Zcopytocore(fileno(fp),data,obj->size);
    /* We're lazy: write it to disk and reuse the normal case */
    uncompressed_out_file=fopen("/tmp/tmp_obj.o","wb");
    fwrite(data,size,1,uncompressed_out_file);
    fclose(uncompressed_out_file);
    fclose(fp);
    /* Reopen it and use the normal load routines */
    uncompressed_out_file=fopen("/tmp/tmp_obj.o","rb");
    ReadEcoff(uncompressed_out_file,obj); 
    fclose(uncompressed_out_file);
    /* Remove the temporary file */
    unlink("/tmp/tmp_obj.o");
    return;
#else
    /* If we do not have the decompression library we fatal... */
    FATAL(("Compressed alpha archives not supported on this architecture (or not compiled in), define -DHAVE_OBJZ_SUPPORT when compiling the source on Tru64 Unix systems and link with libmld. This should be autoconfigured. Contact me (bdebus@@elis.ugent.be) if you have information about the ObjZ compression algoritm"));
#endif
    /* }}} */
  }
  else if (!(fhdr.f_magic==0x183))
    FATAL(("Alpha Ecoff: Trying to read an objectfile, with magic %x (%s)",fhdr.f_magic,OBJECT_NAME(obj)));

  /* Read ecoff a.out (optional) header */
  ret=fread(&ahdr,sizeof(t_ecoff_aout_hdr),1,fp);
  ASSERT(ret==1,("Corrupted  ecoff aout header!\n"));
  /* Verify the correctness of the symboltable-size */
  ASSERT ((fhdr.f_nsyms==sizeof(ecoff_symbol_table_hdr)) || (fhdr.f_nsyms==0), ("Corrupted  ecoff file header!\n"));
  /* Verify the correctness of the optional header size */
  ASSERT ((fhdr.f_opthdr==80), ("Corrupted  ecoff file header!\n"));
  ASSERT ((fhdr.f_nsyms==144) || (fhdr.f_symptr==0), ("Corrupted  ecoff file header %x!\n",fhdr.f_symptr));

  /* Check if the ecoff-objecttype is supported.  */
  ASSERT((ahdr.magic==ECOFF_OMAGIC)||(ahdr.magic==ECOFF_NMAGIC)||(ahdr.magic==ECOFF_ZMAGIC),("Unsupported Ecoff type!"));

  /* We want to verify that the part of the bss-segment that is included in the data-segment if zero-initialized. */
  verify_start=verify_stop=ahdr.bss_start;
  verify_section_start=0;

  /* Store the entry point in the object structure  */
  OBJECT_SET_ENTRY(obj, AddressNew64(ahdr.entry));


  OBJECT_SET_RELOC_TABLE(obj, RelocTableNew(obj));


  secarray=Malloc(sizeof(t_section *)*fhdr.f_nscns);

  /* Store the offset of the section tables for later use (we need to process
   * the section headers again when we process the relocations corresponding to
   * each section */
  stored=ftell(fp);

  /* Read normal section-data and allocate diablo sections for each of the
   * sections. {{{ */
  for (i=0; i<fhdr.f_nscns; i++)
  {
    t_address tmp_addr;
    t_address tmp_size;
    t_address tmp_padding;
    t_uint64 align;


    /* Read the section header */      
    ret=fread(&shdr,sizeof(t_ecoff_scn_hdr),1,fp);
    ASSERT(ret == 1, ("Could not read all sections in %s. This ecoff object file is probably corrupted!\n", OBJECT_NAME(obj)));

    /* Decode the section alignment */
    align = shdr.s_nlnno_or_align;
    align = align==0?16:1<<(align+3);
    if (align<8) 
      align=8;

    /* Ignore the comment section. It contains debug info and compact
     * relocations, stuff we do not need for now. */
    if ((strncmp(shdr.s_name,".comment",8)==0))
    {
      /* TODO: Compact relocations */
    }
    /* Code sections */
    else if ((strncmp(shdr.s_name,".text",8)==0) ||
             (strncmp(shdr.s_name,".fini",8)==0) ||
             (strncmp(shdr.s_name,".init",8)==0))
    {
      tmp_addr=AddressNew64(shdr.s_vaddr);
      tmp_size=AddressNew64(shdr.s_size);
      tmp_padding=AddressNew64(align);
      secarray[i]=ObjectAddSectionFromFile(obj, CODE_SECTION, TRUE, fp, shdr.s_scnptr + OBJECT_STREAMPOS(obj), tmp_addr, tmp_size, tmp_padding, shdr.s_name,i);
    }
    /* Read-only data sections */
    else if ((shdr.s_vaddr+shdr.s_size)<=ahdr.data_start ||
             (strncmp(shdr.s_name,".lita",8)==0) ||
             (strncmp(shdr.s_name,".rconst",8)==0) ||
             (strncmp(shdr.s_name,".rdata",8)==0) ||
             (strncmp(shdr.s_name,".pdata",8)==0))
    {
      tmp_addr=AddressNew64(shdr.s_vaddr);
      tmp_size=AddressNew64(shdr.s_size);
      tmp_padding=AddressNew64(align);
      secarray[i]=ObjectAddSectionFromFile(obj,DATA_SECTION,TRUE,fp,shdr.s_scnptr+OBJECT_STREAMPOS(obj),  tmp_addr, tmp_size, tmp_padding, shdr.s_name,i);
    }
    /* Zero-initialized sections */
    else if ((strncmp(shdr.s_name,".sbss",8)==0) ||
             (strncmp(shdr.s_name,".bss",8)==0))
    {
      tmp_addr=AddressNew64(shdr.s_vaddr);
      tmp_size=AddressNew64(shdr.s_size);
      tmp_padding=AddressNew64(align);
      secarray[i]=ObjectAddSectionFromFile(obj,BSS_SECTION,FALSE,fp,shdr.s_scnptr+OBJECT_STREAMPOS(obj),  tmp_addr, tmp_size, tmp_padding, shdr.s_name,i);

    }
    /* Normal (RW) Data sections */
    else if (((shdr.s_vaddr+shdr.s_size)<=ahdr.bss_start) || 
             (!ahdr.bss_start))
              /* Could be that we do not have a bss section! => ahdr.bss_start = 0*/
    {
      if (verify_section_start<shdr.s_vaddr)
      {
        verify_section_start=shdr.s_vaddr;
        verify_section=OBJECT_NDATAS(obj);
        verify_section_fptr=shdr.s_scnptr;
      }

      tmp_addr=AddressNew64(shdr.s_vaddr);
      tmp_size=AddressNew64(shdr.s_size);
      tmp_padding=AddressNew64(align);
      secarray[i]=ObjectAddSectionFromFile(obj,DATA_SECTION,FALSE,fp,shdr.s_scnptr+OBJECT_STREAMPOS(obj),  tmp_addr, tmp_size, tmp_padding, shdr.s_name,i);
    }
    else
    {
      FATAL(("In ecoff object file %s: Section %s not added because we did not know its type. Add support for sections named %s.", OBJECT_NAME(obj), shdr.s_name, shdr.s_name));
    }
  }
  /* }}} */

  /* Read the symbols in the binary and allocate diablo symbols for them. {{{ */
  if ((fhdr.f_nsyms==sizeof(ecoff_symbol_table_hdr)) && (fhdr.f_symptr!=0))
  {
    ecoff_symbol_table_hdr symbol_tablehdr;
    OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew(obj));
    fseek(fp,OBJECT_STREAMPOS(obj)+fhdr.f_symptr,SEEK_SET);
    fread(&symbol_tablehdr,sizeof(ecoff_symbol_table_hdr),1,fp);
    if (symbol_tablehdr.magic!=magicSym)
    {
      FATAL(("Symbol header magic is wrong\n"));
    }
    else
    {
      char * ext_symstrtab=NULL;
      char * loc_symstrtab=NULL;
      ecoff_ext_sym * extrs=NULL;
      ecoff_pdr * pdrs=NULL;

      if (symbol_tablehdr.issExtMax)
      {
        ext_symstrtab=Malloc(symbol_tablehdr.issExtMax);
        fseek(fp,OBJECT_STREAMPOS(obj)+symbol_tablehdr.cbSsExtOffset,SEEK_SET);
        fread(ext_symstrtab,symbol_tablehdr.issExtMax,1,fp);
      }

      if (symbol_tablehdr.issMax)
      {
        loc_symstrtab=Malloc(symbol_tablehdr.issMax);
        fseek(fp,OBJECT_STREAMPOS(obj)+symbol_tablehdr.cbSsOffset,SEEK_SET);
        fread(loc_symstrtab,symbol_tablehdr.issMax,1,fp);
      }

      /* Read pdrs (Procedure descriptors) {{{ */
      if (symbol_tablehdr.ipdMax)
      {
        pdrs=Malloc(symbol_tablehdr.ipdMax*sizeof(ecoff_pdr));
        fseek(fp,OBJECT_STREAMPOS(obj)+symbol_tablehdr.cbPdOffset,SEEK_SET);
        fread(pdrs,sizeof(ecoff_pdr),symbol_tablehdr.ipdMax,fp);
      }
      /* }}} */
      /* Read global symbols {{{ */
      if (symbol_tablehdr.iextMax)
      {
        t_uint32 tel;
        t_uint32 tel2;
        extrs=Malloc(symbol_tablehdr.iextMax*sizeof(ecoff_ext_sym));
        symlookup=(t_symbol **) Calloc(symbol_tablehdr.iextMax , sizeof(t_symbol *));
        fseek(fp,OBJECT_STREAMPOS(obj)+symbol_tablehdr.cbExtOffset,SEEK_SET);
        fread(extrs,sizeof(ecoff_ext_sym),symbol_tablehdr.iextMax,fp);

        for (tel=0; tel<symbol_tablehdr.iextMax; tel++)
        {
          t_int32 order=-1;
          t_tristate dup=TRUE;
          t_tristate search=FALSE;
          t_uint32 prolog=0;
          t_address generic;
          generic=AddressNew64(extrs[tel].asym.value);

          if (extrs[tel].asym.st==stGlobal)
          {
            order = 10;
            dup = FALSE;
            search = FALSE;
          }
          else if (extrs[tel].asym.st==stLabel)
          {
            order = 10;
            dup = FALSE;
            search = FALSE;
          }
          else if (extrs[tel].asym.st==stProc)
          {
            t_bool found=FALSE;
            order = 10;
            dup = FALSE;
            search = FALSE;
            for (tel2=0; tel2<symbol_tablehdr.ipdMax; tel2++)
            {
              if (pdrs[tel2].adr==extrs[tel2].asym.value)
              {
                found=TRUE;
                break;
              }
            }

            if (found) 
            {
              prolog=pdrs[tel2].gp_prologue;
            }
          }
          else if (extrs[tel].asym.st==stStaticProc)
          {
            t_bool found=FALSE;
            order = 10;
            dup = FALSE;
            search = FALSE;
            for (tel2=0; tel2<symbol_tablehdr.ipdMax; tel2++)
            {
              if (pdrs[tel2].adr==extrs[tel2].asym.value)
              {
                found=TRUE;
                break;
              }
            }
            if (found) prolog=pdrs[tel2].gp_prologue;
          }
          else if (extrs[tel].asym.st==stLocal)
          {
            order = -1;
            dup = TRUE;
            search = FALSE;
          }
	  else if (extrs[tel].asym.st==stNil)
	  {
		  /*  Dummy entry */
	  }
          else 
	  { 
            FATAL(("Symbol %s has an unknown symbol type %d. Implement!\n", ext_symstrtab+extrs[tel].asym.iss, extrs[tel].asym.st)); 
          }

          if (extrs[tel].asym.sc==scNil)
          {
            /* Dummy entry */
          }
          else if ((extrs[tel].asym.sc==scText)||(extrs[tel].asym.sc==scData)||(extrs[tel].asym.sc==scBss)||(extrs[tel].asym.sc==scSData)||(extrs[tel].asym.sc==scSBss)||(extrs[tel].asym.sc==scRData)||(extrs[tel].asym.sc==scInit)||(extrs[tel].asym.sc==scFini)||(extrs[tel].asym.sc==scRConst)||(extrs[tel].asym.sc==scPData)||(extrs[tel].asym.sc==scXData))  
          {
            t_section * sec = SectionGetFromObjectByName(obj,EcoffSymbolStorageToSectionName(extrs[tel].asym.sc));
            symlookup[tel] = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), ext_symstrtab+extrs[tel].asym.iss,  "R00A00+$", order, dup, search, T_RELOCATABLE (sec), AddressSub(generic, SECTION_CADDRESS(sec)), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
          }
          else if (extrs[tel].asym.sc==scAbs)
          {
            t_section * abs= OBJECT_PARENT(obj)?OBJECT_ABS_SECTION(OBJECT_PARENT(obj)):OBJECT_ABS_SECTION(obj);
            symlookup[tel] = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), ext_symstrtab+extrs[tel].asym.iss,  "R00A00+$", order, dup, search, T_RELOCATABLE (abs), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
          }
          else if ((extrs[tel].asym.sc==scUndefined) || (extrs[tel].asym.sc==scSUndefined))
          {
            t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
            /* Undefined */

	    ASSERT(undef, ("Could not find the undef section for object %s", OBJECT_NAME(obj)));
            symlookup[tel] = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), ext_symstrtab+extrs[tel].asym.iss,  "R00A00+$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
          }
          /* Common/.bss symbols */
          else if (extrs[tel].asym.sc==scCommon)
          {
            t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
            symlookup[tel]  = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), 
                                                      ext_symstrtab+extrs[tel].asym.iss,  "R00A00+$", 8, PERHAPS, search, 
                                                      T_RELOCATABLE (undef),
                                                      AddressNullForObject(obj),
                                                      AddressNullForObject(obj),
                                                      "CommonSection {\
                                                      action {\
                                                      ADD_SUBSECTION(\"Linker\", \".bss\", CONCAT(\"COMMON:\",MATCHED_NAME()), BSS, 8, MATCHED_SYMBOL_SIZE())\
                                                      }\
                                                      address { SYMBOL(MATCHED_NAME()) }\
                                                      }\
                                                      CommonSymbol {\
                                                      action {\
                                                      ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
                                                      }\
                                                      symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"COMMON:\",MATCHED_NAME())) }\
                                                      }\
                                                      ",
                                                      generic, 0);

          }
          /* Small Common/.sbss sections */
          else if (extrs[tel].asym.sc==scSCommon)
          {
            t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
            symlookup[tel]  = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), 
                                                      ext_symstrtab+extrs[tel].asym.iss,  "R00A00+$", 8, PERHAPS, search, 
                                                      T_RELOCATABLE (undef),
                                                      AddressNullForObject(obj),
                                                      AddressNullForObject(obj),
                                                      "CommonSection {\
                                                      action {\
                                                      ADD_SUBSECTION(\"Linker\", \".sbss\", CONCAT(\"COMMON:\",MATCHED_NAME()), BSS, 4, MATCHED_SYMBOL_SIZE())\
                                                      }\
                                                      address { SYMBOL(MATCHED_NAME()) }\
                                                      }\
                                                      CommonSymbol {\
                                                      action {\
                                                      ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
                                                      }\
                                                      symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"COMMON:\",MATCHED_NAME())) }\
                                                      }\
                                                      ",
                                                      generic, 0);


          }
          else
          {
            FATAL(("Symbol %s has an unknown storage class %d. Implement", ext_symstrtab+extrs[tel].asym.iss, extrs[tel].asym.sc));
          }
        }
      }
      /* }}} */

      if (extrs) 
        Free(extrs);
      if (pdrs) 
        Free(pdrs);
      if (loc_symstrtab) 
        Free(loc_symstrtab);
      if (ext_symstrtab) 
        Free(ext_symstrtab);
    }
  }
  /* }}} */

  /* Restore fp to the start of the section headers */
  fseek(fp,stored,SEEK_SET);


  /* Restore fp to the start of the section headers */
  fseek(fp,stored,SEEK_SET);

  for (i=0; i<fhdr.f_nscns; i++)
  {
    t_section * current_section=NULL;
    /* Read the section header */      
    ret=fread(&shdr, sizeof(t_ecoff_scn_hdr),1,fp);
    ASSERT(ret==1,("Corrupted  ecoff file (sections)!\n"));
    
    /* Get the (already loaded) section */
    current_section=SectionGetFromObjectByName(obj,shdr.s_name);

    printf("%s\n", shdr.s_name);
    if ((current_section) && (shdr.s_relptr!=0))
    {
      t_uint64 num_relocs=0;
      t_ecoff_reloc * relocs=NULL;
      
      EcoffGetRelocsForHeader(shdr, obj, fp, &relocs, &num_relocs);

      EcoffProcessRelocsForSection(num_relocs, relocs, obj, secarray[i], symlookup, ahdr.gp_value);
      
      Free(relocs);
    }
  }

  /* Some checks {{{ */ 
  if (verify_start!=verify_stop)
  {
    int tel=0;  
    char * tmp=(char *) Malloc(verify_stop-verify_start);
    /*VERBOSE(1,("Special for ECOFF_Alpha: Need to verify %d bytes. Should all be zero and should follow section %s with size %d (0x%x)",verify_stop-verify_start, obj->data[verify_section]->name,G_T_UINT64(obj->data[verify_section]->size)));*/
    FATAL(("Implement"));
    save=ftell(fp);
    /*fseek(fp,OBJECT_STREAMPOS(obj)+verify_section_fptr+G_T_UINT64(obj->data[verify_section]->size),SEEK_SET);*/

    fread(tmp,verify_stop-verify_start,1,fp);
    fseek(fp,save,SEEK_SET);

    for (tel=0; tel<verify_stop-verify_start; tel++)
    {
      if (tmp[tel]!=0) FATAL(("Corrupted bss segment, overlap should be zero-initialized"));
    }
    Free(tmp);
  }
  /* }}} */

  Free(symlookup);
  Free(secarray);
}
/* }}} */
/* WriteEcoff {{{ */
typedef t_section * t_section_ptr;

int sec_sort(const void * a, const void * b)
{
  const t_section_ptr * sa=a;
  const t_section_ptr * sb=b;
  if (AddressIsGt(SECTION_CADDRESS(*sa),SECTION_CADDRESS(*sb)))
  {
    return 1;
  }
  else if (AddressIsLt(SECTION_CADDRESS(*sa),SECTION_CADDRESS(*sb)))
  {
    return -1;
  }
  return 0;
}

void EcoffWrite(FILE * fp, t_object * obj)
{
  t_section ** sections;
  t_uint32 * pointers;
  t_ecoff_file_hdr fhdr;
  t_ecoff_aout_hdr ahdr;
  t_ecoff_scn_hdr  shdr;
  t_section * sec;
  t_uint32 nr_sections=0, i=0, dummy=0;
  t_uint32 run=0, pad=0;
  t_uint64 maxtext=0;
  t_uint64 maxdata=0;
  t_uint64 maxbss=0;
  t_uint64 litastart=0;
  t_uint64 current_segment_start,next_segment_start,segment_pad;
  t_uint64 total_header_size;

  OBJECT_FOREACH_SECTION(obj,sec,dummy) 
  {
    if ((G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) > LL(0x0000000120000000))
        && (G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) < LL(0x0000000140000000))
        && (G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) > maxtext))
      maxtext=G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec)));
    else if ((G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) > LL(0x0000000140000000))
             && (G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) > maxdata)
             && (SECTION_TYPE(sec)!=BSS_SECTION))
      maxdata=G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec)));
    else if ((SECTION_TYPE(sec)==BSS_SECTION)
             && (G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec))) > maxbss))
      maxbss=G_T_UINT64(AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec)));

    if (strcmp(SECTION_NAME(sec),".lita")==0) litastart=G_T_UINT64(SECTION_CADDRESS(sec));
    nr_sections++; 
  }

  sections=Malloc(sizeof(t_section *)*nr_sections);
  pointers=Malloc(sizeof(t_uint32)*nr_sections);

  nr_sections=0;
  OBJECT_FOREACH_SECTION(obj,sec,dummy) 
  {
    sections[nr_sections]=sec;
    nr_sections++;
  }

  qsort(sections,nr_sections,sizeof(t_section *),sec_sort);

  for (i=0; i<nr_sections; i++)
  {
    VERBOSE(0,("SORTED: %s @G",SECTION_NAME(sections[i]),SECTION_CADDRESS(sections[i])));
  }

  fhdr.f_magic=0x183;
  fhdr.f_nscns=nr_sections;
  fhdr.f_timdat=0;
  fhdr.f_symptr=0;
  fhdr.f_nsyms=0;
  fhdr.f_opthdr=80;
  fhdr.f_flags=F_NO_REORG | F_NO_REMOVE;
  fwrite(&fhdr,sizeof(t_ecoff_file_hdr),1,fp);

  ahdr.magic=ECOFF_ZMAGIC;
  ahdr.vstamp=0x030d;
  ahdr.bldrev=0x000e;
  ahdr.padcell=0;
  ahdr.tsize=maxtext-LL(0x0000000120000000); 
  if (ahdr.tsize&0x1fff) ahdr.tsize=(ahdr.tsize+0x2000L)&~0x1fffL;
  ahdr.dsize=maxdata-LL(0x0000000140000000); 
  if (ahdr.dsize&0x1fff) ahdr.dsize=(ahdr.dsize+0x2000L)&~0x1fffL;
  if (maxbss>maxdata)
    ahdr.bsize=maxbss-(ahdr.dsize+LL(0x0000000140000000)); 
  else
    ahdr.bsize=0;
  ahdr.entry=G_T_UINT64(OBJECT_ENTRY(obj));
  ahdr.text_start=LL(0x0000000120000000); /* hardcoded for now */ 
  ahdr.data_start=LL(0x0000000140000000); /* hardcoded for now */
  ahdr.bss_start=ahdr.dsize+LL(0x0000000140000000); 
  ahdr.gprmask=0xffffffff;
  ahdr.fprmask=0xffffffff;
  ahdr.gp_value=litastart+32752; 

  fwrite(&ahdr,sizeof(t_ecoff_aout_hdr),1,fp);
  run=0;
  pad=0;
  current_segment_start = 0x0000000120000000ULL;
  next_segment_start = 0x0000000140000000ULL;
  segment_pad = 0ULL;

  total_header_size = sizeof(t_ecoff_aout_hdr)+sizeof(t_ecoff_file_hdr)+sizeof(t_ecoff_scn_hdr)*nr_sections;
  printf("total headers size: %llx\n",total_header_size);

  for (i=0; i<nr_sections; i++)
  {
    sec = sections[i];

    shdr.s_name[0] =
    shdr.s_name[1] =
    shdr.s_name[2] =
    shdr.s_name[3] =
    shdr.s_name[4] =
    shdr.s_name[5] =
    shdr.s_name[6] =
    shdr.s_name[7] = '\0';
    printf("Section name: %s\n",SECTION_NAME(sec));
    strncpy(shdr.s_name,SECTION_NAME(sec),7);
    printf("sname = %s\n",shdr.s_name);
    shdr.s_vaddr=shdr.s_paddr=G_T_UINT64(SECTION_CADDRESS(sec));
    shdr.s_size=AddressExtractUint32(SECTION_CSIZE(sec));
    shdr.s_nreloc=shdr.s_relptr=0;
    shdr.s_nlnno_or_align=shdr.s_lnnoptr=0;

    if (SECTION_TYPE(sec)!=BSS_SECTION)
    {
      if (((total_header_size+run+pad)&0x1fff)!=(G_T_UINT64(SECTION_CADDRESS(sec))&0x1fff))
      {
        t_int32 padadd;
        if (G_T_UINT64(SECTION_CADDRESS(sec))>=next_segment_start)
        {
          current_segment_start=next_segment_start;
          next_segment_start=0xffffffffffffffffULL;
          padadd=(G_T_UINT64(SECTION_CADDRESS(sec))&0x1fff)-((total_header_size+run+pad)&0x1fff);

          if (padadd<1) padadd+=0x2000;
          segment_pad = total_header_size+run+pad+padadd-(G_T_UINT64(SECTION_CADDRESS(sec))-current_segment_start);
        }
        else
        {
          padadd=G_T_UINT64(SECTION_CADDRESS(sec))-current_segment_start+segment_pad-(total_header_size+run+pad);
          if (padadd<0) padadd+=0x2000;		  
        }

        pad+=padadd;  
        printf("pad: %x padadd %x\n",pad,padadd);
      }
      shdr.s_scnptr=total_header_size+run+pad;
      printf("Writing this section at %llx\n",shdr.s_scnptr);
      pointers[i]=shdr.s_scnptr;
      run+=AddressExtractUint32(SECTION_CSIZE(sec));
    }
    else
      shdr.s_scnptr=0;

    if (SECTION_TYPE(sec)==CODE_SECTION)
    {
      shdr.s_flags=STYP_REG;
      if (strcmp(SECTION_NAME(sec),".text")==0) shdr.s_flags|=STYP_TEXT;
      else if (strcmp(SECTION_NAME(sec),".init")==0) shdr.s_flags|=STYP_INIT;
      else if (strcmp(SECTION_NAME(sec),".fini")==0) shdr.s_flags|=STYP_FINI;
      else if (strcmp(SECTION_NAME(sec),".merged_code")==0) shdr.s_flags|=STYP_TEXT;
      else FATAL(("Implement code section %s",SECTION_NAME(sec)));
    }
    else if (SECTION_TYPE(sec)==RODATA_SECTION)
    {
      shdr.s_flags=STYP_REG;
      if (strcmp(SECTION_NAME(sec),".rconst")==0) shdr.s_flags|=STYP_RCONST;
      else if (strcmp(SECTION_NAME(sec),".rdata")==0) shdr.s_flags|=STYP_RDATA;
      else if (strcmp(SECTION_NAME(sec),".rodata")==0) shdr.s_flags|=STYP_RDATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.pdata")==0) shdr.s_flags|=STYP_PDATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.rconst")==0) shdr.s_flags|=STYP_RCONST;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.rdata")==0) shdr.s_flags|=STYP_RDATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.lita")==0) shdr.s_flags|=STYP_LITA;
      else if (strcmp(SECTION_NAME(sec),".lita")==0) shdr.s_flags|=STYP_LITA;
      else if (strcmp(SECTION_NAME(sec),".pdata")==0) shdr.s_flags|=STYP_PDATA;
      else FATAL(("Implement rodata section %s",SECTION_NAME(sec)));
    }
    else if (SECTION_TYPE(sec)==DATA_SECTION)
    {
      shdr.s_flags=STYP_REG;
      if (strcmp(SECTION_NAME(sec),".data")==0) shdr.s_flags|=STYP_DATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.data")==0) shdr.s_flags|=STYP_DATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.sbss")==0) shdr.s_flags|=STYP_DATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.bss")==0) shdr.s_flags|=STYP_DATA;
      else if (strcmp(SECTION_NAME(sec),".wridata")==0) shdr.s_flags|=STYP_DATA;
      else if (strcmp(SECTION_NAME(sec),".sdata")==0) shdr.s_flags|=STYP_SDATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.sdata")==0) shdr.s_flags|=STYP_SDATA;
      else if (strcmp(SECTION_NAME(sec),".xdata")==0) shdr.s_flags|=STYP_XDATA;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.xdata")==0) shdr.s_flags|=STYP_XDATA;
      else FATAL(("Implement data section %s",SECTION_NAME(sec)));
    }
    else if (SECTION_TYPE(sec)==BSS_SECTION)
    {
      shdr.s_flags=STYP_REG;
      if (strcmp(SECTION_NAME(sec),".sbss")==0) shdr.s_flags|=STYP_SBSS;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.sbss")==0) shdr.s_flags|=STYP_SBSS;
      else if (strcmp(SECTION_NAME(sec),".bss")==0) shdr.s_flags|=STYP_BSS;
      else if (strcmp(SECTION_NAME(sec),"__diablo_inst_.bss")==0) shdr.s_flags|=STYP_BSS;
      else FATAL(("Implement bss section %s",SECTION_NAME(sec)));
    }
    else
    {
      FATAL(("Unkown section type!"));
    }

    fwrite(&shdr,sizeof(t_ecoff_scn_hdr),1,fp);
  }

  for (i=0; i<nr_sections; i++)
  {
    sec=sections[i];
    printf("Writing %s at %lx\n",SECTION_NAME(sec),ftell(fp));
    if (SECTION_TYPE(sec)!=BSS_SECTION)
    {
      char c=0;
      while (ftell(fp)!=pointers[i]) 
      {
        fwrite(&c,sizeof(char),1,fp);
      }


      if ((ftell(fp)&0x1fff)!=(0x1fff&G_T_UINT64(SECTION_CADDRESS(sec))))
      {
        FATAL(("Misaligned section %s address @G, fp=%lx pointer said it should be %x\n",SECTION_NAME(sec),SECTION_CADDRESS(sec),ftell(fp), pointers[i]));
      }
      fwrite(SECTION_DATA(sec),sizeof(char),AddressExtractUint32(SECTION_CSIZE(sec)),fp);
    }

  }

  {
    t_uint32 bsspad=ahdr.dsize+LL(0x0000000140000000)-maxdata;
    char c=0;
    while (bsspad--) fwrite(&c,sizeof(char),1,fp);
  }
  fclose(fp);
  Free(pointers);
  Free(sections);
}
/* }}} */


#if 0
void set_scncompactrels(t_object * obj)
{
  int tel1, tel2;
  compact_rels_ret * crels;
  if (obj->filename==NULL)
  {
    return;
  }
  crels=get_compact_rels(obj->filename);
  if (crels!=NULL)
  {
    printf("Got some compact relocs....\n");
    /* Now merge them ....  */

    for (tel1=0; tel1<crels->numsec; tel1++)
    {
      int matched=0;
      for (tel2=0; tel2<OBJ_NSECS(obj); tel2++)
      {
        if (strncmp(crels->rels[tel1].secname,obj->scnhdrs[tel2]->s_name,8)==0)
        {
          matched=1;
          printf("Section %0.8s has %ld compact relocs and %ld normal relocs\n",crels->rels[tel1].secname,crels->rels[tel1].nrels,obj->secinfos[tel2].real_nreloc);
          /* TODO: implement */
          exit(0);
          break;
        }
      }
      if (!matched)
      {
        fprintf(stderr, "Compact relocations have a section %s that is\n",crels->rels[tel1].secname);
        fprintf(stderr, "not present in the binary.. Exit!\n");
        exit(0);
      }
    }
    free_compact_rels(crels);
  }
}

void set_symhdr(t_object * obj)
{
  if ((obj->fhdr->f_nsyms==sizeof(HDRR))&& (obj->fhdr->f_symptr!=0))
  {
    obj->symhdr=(HDRR *) (obj->data+obj->fhdr->f_symptr);
    if (obj->symhdr->magic!=magicSym)
    {
      fprintf(stderr,"Symbol header magic is wrong\n");
      exit(0);
    }
  }

}

void process_syms(t_object * obj)
{
  if (obj->symhdr==NULL) return;
  if (obj->symhdr->iextMax!=0)
  {
    obj->extrs=(EXTR *) (obj->data+obj->symhdr->cbExtOffset);
  }
  if (obj->symhdr->ifdMax!=0)
  {
    obj->fdrs=(FDR *) (obj->data+obj->symhdr->cbFdOffset);
  }
  if (obj->symhdr->ipdMax!=0)
  {
    obj->pdrs=(PDR *) (obj->data+obj->symhdr->cbPdOffset);
  }
  if (obj->symhdr->issExtMax!=0)
  {
    obj->extss=(char *) (obj->data+obj->symhdr->cbSsExtOffset);
  }
  if (obj->symhdr->ilineMax!=0)
  {
    obj->lines=(LINER *) (obj->data+obj->symhdr->cbLineOffset);
  }
  if (obj->symhdr->isymMax!=0)
  {
    obj->lsyms=(SYMR *) (obj->data+obj->symhdr->cbSymOffset);
  }
  if (obj->symhdr->issMax!=0)
  {
    obj->locss=(char *) (obj->data+obj->symhdr->cbSsOffset);
  }
  init_symbol_table_hashes(obj);
}

SCNHDR * find_scnhdr(t_object * obj, char * name)
{
  SCNHDR * ret=NULL;
  hashelement * tmp;
  tmp=hash_lookup_not_delimited(obj->scnhdr_hash,name,8);
  if (tmp!=NULL)
  {
    ret=obj->scnhdrs[(long) tmp->value];
  }
  return ret;
}

int find_scnno(t_object * obj, char * name)
{
  long ret=-1;
  hashelement * tmp;
  tmp=hash_lookup_not_delimited(obj->scnhdr_hash,name,8);
  if (tmp!=NULL)
  {
    ret=(long) tmp->value;
  }

  return ret;
}

int find_scnno_by_addr(t_object * obj, long address)
{
  int tel;
  for (tel=0; tel<OBJ_NSECS(obj); tel++)
  {
    if ((OBJ_SCNSTART_BY_NO(obj,tel)<=address)&&(OBJ_SCNSTART_BY_NO(obj,tel)+OBJ_SCNSIZE_BY_NO(obj,tel)>address))
    {
      return tel;
    }
  }
}

void build_scnhdr_hash(t_object *obj)
{
  long tel;
  obj->scnhdr_hash=hash_create(7,0,StringHash_secname);
  for (tel=0; tel<obj->fhdr->f_nscns; tel++)
  {
    hash_insert_new(obj->scnhdr_hash,obj->scnhdrs[tel]->s_name,(void *) tel);
  }
}

void process_object(t_object * obj,int mode)
{
  set_filehdr(obj);
  set_aouthdr(obj);
  set_scnhdrs(obj);
  build_scnhdr_hash(obj);
  set_scndatas(obj);
  set_scnrels(obj);
  set_scncompactrels(obj);
  set_symhdr(obj);
  process_syms(obj);
}
#endif

t_uint64 EcoffGetSizeofHeaders(t_object * obj, t_layout_script * layoutscript)
{
  t_uint64 ret = 0;
  ret += 24; /* file header */
  ret += 80; /* a.out header */
  ret += 64 * (OBJECT_NCODES(obj) + OBJECT_NDATAS(obj) + OBJECT_NRODATAS(obj) + OBJECT_NBSSS(obj) + OBJECT_NNOTES(obj)); /* section headers */
  return ret;
}
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
