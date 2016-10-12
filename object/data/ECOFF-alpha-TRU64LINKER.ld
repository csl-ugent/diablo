linker="True64Unix Linker"
fileformats="ECOFF"
architectures="ALPHA"

InitSection
{
   action { ADD_SUBSECTION("Linker",".init", ".init_linker",CODE,4,12) }
   trigger { UNDEFINED_SYMBOL("__istart") }
   section { 
      CONST32(0x23defff0), /*  lda     sp, -16(sp) */
      CONST32(0xb77e0008), /*  stq     t12, 8(sp) */
      CONST32(0xb75e0000)  /*  stq     ra, 0(sp) */
   }
   address { SYMBOL("__istart") }
}

InitStub
{
   trigger { SYMBOL_EXISTS("__init*") }
   action { ADD_SUBSECTION("Linker", ".init", ".init_linker_stub",CODE,4,16) }
   section { 
      CONST32(0xa77e0008),  /*  ldq     t12, 8(sp) */
      RELOCATED32(0x277b0000, MATCHED_NAME() ,0,"__istart",0,"S00A00+I0000000000008000+S01-I0000000000000010> I000000000000ffff&\\ lI00000000ffff0000&|w s0000$"), /*  ldah    t12, -2(t12) */
      RELOCATED32(0x237b0000, MATCHED_NAME() ,0,"__istart",0,"S00A00+S01-I000000000000ffff&\\ lI00000000ffff0000&|w s0000$"), /*  lda     t12, -6048(t12) */
      CONST32(0x6b5b4000)   /*  jsr     ra, (t12), 0x12003149c(zero) */
   }
   address { END_OF_LAST_ADDED()  }
}

InitEnd
{
   trigger { UNDEFINED_SYMBOL("__istart") }
   action { ADD_SUBSECTION("Linker", ".init", ".init_linker_end",CODE,4,12) }
   section { 
      CONST32(0xa75e0000), /*  ldq     ra, 0(sp) */
      CONST32(0x23de0010), /*  lda     sp, 16(sp) */
      CONST32(0x6bfa8001)  /*  ret     zero, (ra), 1 */
   }
   address { END_OF_LAST_ADDED() }
}


FiniSection
{
   action { ADD_SUBSECTION("Linker", ".fini",".fini_linker",CODE,4,12) }
   trigger { UNDEFINED_SYMBOL("__fstart") }
   section {
      CONST32(0x23defff0), /*  lda     sp, -16(sp) */
      CONST32(0xb77e0008), /*  stq     t12, 8(sp) */
      CONST32(0xb75e0000) /*  stq     ra, 0(sp) */
   }
   address { SYMBOL("__fstart") }
}

FiniStub
{
   trigger { SYMBOL_EXISTS("__fini*") }
   action { ADD_SUBSECTION("Linker",".fini", ".fini_stub",CODE,4,16) }
   section { 
      CONST32(0xa77e0008), /*  ldq     t12, 8(sp) */
      RELOCATED32(0x277b0000,MATCHED_NAME(),0,"__fstart",0,"S00A00+I0000000000008000+S01-I0000000000000010> I000000000000ffff&\\ lI00000000ffff0000&|w s0000$"), /*  ldah    t12, -2(t12) */
      RELOCATED32(0x237b0000,MATCHED_NAME(),0,"__fstart",0,"S00A00+S01-I000000000000ffff&\\ lI00000000ffff0000&|w s0000$"), /*  lda     t12, -6048(t12) */
      CONST32(0x6b5b4000) /*  jsr     ra, (t12), 0x12003149c(zero) */
   }
   address { END_OF_LAST_ADDED() }
}

FiniEnd
{
   action { ADD_SUBSECTION("Linker", ".fini", ".fini_end",CODE,4,12) }
   trigger { UNDEFINED_SYMBOL("__fstart") }
   section {
      CONST32(0xa75e0000), /*  ldq     ra, 0(sp) */
      CONST32(0x23de0010), /*  lda     sp, 16(sp) */
      CONST32(0x6bfa8001)  /*  ret     zero, (ra), 1 */
   }
   address { END_OF_LAST_ADDED() }
}

InitSymbol
{
  action  { ADD_SYMBOL("__istart") }
 trigger { UNDEFINED_SYMBOL("__istart") }
 symbol{ START_OF_SUBSECTION("Linker",".init_linker") }
    
}
FiniSymbol
{
  action  { ADD_SYMBOL("__fstart") }
 trigger { UNDEFINED_SYMBOL("__fstart") }
 symbol{ START_OF_SUBSECTION("Linker",".fini_linker") }
    
}
/* Should be start of segment */
FDATA {
  action  { ADD_SYMBOL("_fdata") }
  trigger { UNDEFINED_SYMBOL("_fdata") }
  symbol  { START_OF_SECTION(".data") }
}

FTEXT {
  action  { ADD_SYMBOL("_ftext") }
  trigger { UNDEFINED_SYMBOL("_ftext") }
  symbol  { START_OF_SECTION(".text") }
}
FBSS1 {
  action  { ADD_SYMBOL("_fbss") }
  trigger { UNDEFINED_SYMBOL("_fbss") && SECTION_EXISTS(".sbss") }
  symbol  { START_OF_SECTION(".sbss") }
}
FBSS {
  action  { ADD_SYMBOL("_fbss") }
  trigger { UNDEFINED_SYMBOL("_fbss") && SECTION_EXISTS(".bss") }
  symbol  { START_OF_SECTION(".bss") }
}

/* Should be start of segment */
ETEXT {
  action  { ADD_SYMBOL("_BASE_ADDRESS") }
  trigger { UNDEFINED_SYMBOL("_BASE_ADDRESS") }
  symbol  { ABS(0x120000000) }
}

DYNAMIC {
  action  { ADD_SYMBOL("_DYNAMIC") }
  trigger { UNDEFINED_SYMBOL("_DYNAMIC") }
  symbol  { ABS(0x0) }
}

/* Should be end of segment */
ETEXT {
  action  { ADD_SYMBOL("_etext") }
  trigger { UNDEFINED_SYMBOL("_etext") }
  symbol  { END_OF_SECTION(".text") }
}

FPDATA {
  action  { ADD_SYMBOL("_fpdata") }
  trigger { SECTION_EXISTS(".pdata") && UNDEFINED_SYMBOL("_fpdata") }
  symbol  { START_OF_SECTION(".pdata") }
}

FPDATA {
   action  { ADD_SYMBOL("_fpdata") }
   trigger { UNDEFINED_SYMBOL("_fpdata") }
   symbol  { ABS(0) }
}


/* Should be SIZE_OF_SECTION */
FPDATASIZE {
  action  { ADD_SYMBOL("_fpdata_size") }
  trigger { SECTION_EXISTS(".pdata") && UNDEFINED_SYMBOL("_fpdata_size") }
  symbol  { ABS(0) }
}
DYNAMIC_LINK{
      action  { ADD_SYMBOL("_DYNAMIC_LINK") }
   trigger { UNDEFINED_SYMBOL("_DYNAMIC_LINK") }
   symbol  { ABS(0) }

}

gp {
action { ADD_SYMBOL("DIABLO:gp") }
trigger { UNDEFINED_SYMBOL("DIABLO:gp") }
symbol { START_OF_SECTION(".lita") + OFFSET(32752) }
}

gp {
action { ADD_SYMBOL("_gp") }
trigger { UNDEFINED_SYMBOL("_gp") }
symbol { START_OF_SECTION(".lita") + OFFSET(32752) }
}

external_destructor_functor_list
{
   action { ADD_SUBSECTION("Linker","__external_destructor_functor_list",BSS,4) }
   trigger { UNDEFINED_SYMBOL("__external_destructor_functor_list") }
   section { 
      CONST32(0x0),
      CONST32(0x0) 
      }
   address { SYMBOL("__external_destructor_functor_list") }
}


__external_destructor_functor_list {
   action {ADD_SYMBOL("__external_destructor_functor_list") }
   trigger {UNDEFINED_SYMBOL("__external_destructor_functor_list") }
 symbol{ START_OF_SUBSECTION("Linker","__external_destructor_functor_list") }
}

gpinfo {
action { ADD_SYMBOL("_gpinfo") }
trigger { UNDEFINED_SYMBOL("_gpinfo") }
symbol { ABS(0) }
}

/* Should be end of segment */
END {
  action  { ADD_SYMBOL("_end") }
  trigger { UNDEFINED_SYMBOL("_end") }
  symbol  { END_OF_SECTION(".bss") }
}




/* embedded layout script */
SECTIONS
{
  . = 0x120002000;
  .rconst : {}
  .rdata : {}
  .lita : {}
  PUT_REMAINING_SECTIONS(RO_DATA_SECTION);
  .init : {}
  .fini : {}
  .text : {}
  PUT_REMAINING_SECTIONS(CODE_SECTION);
  . = 0x140000000;
  .data : {}
  .sdata : {}
  .xdata : {}
  .pdata : {}
  PUT_REMAINING_SECTIONS(DATA_SECTION);
  .sbss : {}
  PUT_REMAINING_SECTIONS(BSS_SECTION);
  .bss : {}
  .comment : {}
  PUT_REMAINING_SECTIONS(NOTE_SECTION);
}
