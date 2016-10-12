linker="AMD64 GNU LD"
fileformats="ELF"
architectures="AMD64"

/* version with less underscores (gcc 3.2.2 & binutils 2.13.2.1) */

BssStart {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__bss_start")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}

/***********************************************************************/

EhFrameVector {
   action {  VECTORIZE(".eh_frame") }
   trigger { SUBSECTION_EXISTS("*",".eh_frame") }
}

EhFrameVector {
   action {  VECTORIZE(".gcc_except_table") }
   trigger { SUBSECTION_EXISTS("*",".gcc_except_table") }
}

EhFrameBegin {
   action  { ADD_SYMBOL("__EH_FRAME_BEGIN__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__EH_FRAME_BEGIN__")   && SUBSECTION_EXISTS("Linker","VECTOR___.eh_frame") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.eh_frame") }
}

End {
   action  { ADD_SYMBOL("end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("end")  && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") } 
}

_End {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end") && SECTION_EXISTS(".bss")   }
   symbol  { END_OF_SECTION(".bss") } 
}

GotTable1 {
   action  { ADD_SYMBOL("_GLOBAL_OFFSET_TABLE_",SYMBOL_TYPE_OBJECT) }
   trigger { UNDEFINED_SYMBOL("_GLOBAL_OFFSET_TABLE_")  && SECTION_EXISTS(".got.plt") }
   symbol  { START_OF_SECTION(".got.plt") }
}

GotTable2 {
   action  { ADD_SYMBOL("_GLOBAL_OFFSET_TABLE_",SYMBOL_TYPE_OBJECT) }
   trigger { UNDEFINED_SYMBOL("_GLOBAL_OFFSET_TABLE_")  && SECTION_EXISTS(".got") }
   symbol  { START_OF_SECTION(".got") }
}

GotTableReservedEntries1 {
    action { ADD_SUBSECTION("Linker",".got",".got.plt",DATA,3,24) }
    trigger { SECTION_EXISTS(".got.plt") }
    section { CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0)}
    address { START_OF_SECTION(".got.plt") }
}

GotTableReservedEntries2 {
    action { ADD_SUBSECTION("Linker",".got",".got",DATA,1,8) }
    trigger { SECTION_EXISTS(".got") &&  SECTION_EXISTS(".got.plt") }
    section { CONST32(0), CONST32(0) }
    address { START_OF_SECTION(".got") }
}

GotTableReservedEntries3 {
    action { ADD_SUBSECTION("Linker",".got",".got",DATA,4,24) }
    trigger { SECTION_EXISTS(".got") &&  (!SECTION_EXISTS(".got.plt")) }
    section { CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0)}
    address { START_OF_SECTION(".got") }
}

TextEnd {
   action  { ADD_SYMBOL("etext",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("etext") && SECTION_EXISTS(".text") }
   symbol  { END_OF_SECTION(".text") }
}

TextEnd {
   action  { ADD_SYMBOL("_etext",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_etext") && SECTION_EXISTS(".text") }
   symbol  { END_OF_SECTION(".text") }
}

TextEnd {
   action  { ADD_SYMBOL("__etext",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__etext") && SECTION_EXISTS(".text") }
   symbol  { END_OF_SECTION(".text") }
}


/* Some special sections:

   .preinit_array
   .init_array
   .fini_array

   We first vectorize them and add start and end symbols, with fallbacks if a
   section does not exists
*/

PreInitVector {
   action {  VECTORIZE(".preinit_array") }
   trigger { SUBSECTION_EXISTS("*",".preinit_array") }
}

InitVector {
   action {  VECTORIZE(".init_array") }
   trigger { SUBSECTION_EXISTS("*",".init_array") }
}

FiniVector {
   action {  VECTORIZE(".fini_array") }
   trigger { SUBSECTION_EXISTS("*",".fini_array") }
}

InitVector2 {
   action {  VECTORIZE(".init") }
   trigger { SUBSECTION_EXISTS("*",".init") }
}

FiniVector2 {
   action {  VECTORIZE(".fini") }
   trigger { SUBSECTION_EXISTS("*",".fini") }
}

/* ========= Start of preinit ========= */

/* Have preinit */

PreInitStart {
 action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.preinit_array") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.preinit_array") }
}

/* Start of preinit, no preinit, have init */

PreInitStartFALLBACK {
   action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__preinit_array_start") } 
   symbol  { ABS(0) }
}

/* ========= End of preinit ========= */

/* Have preinit */

PreInitEnd {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.preinit_array") }
 symbol  { END_OF_SUBSECTION("Linker","VECTOR___.preinit_array") }
}

/* Have init, no preinit */

PreInitEndFALLBACK {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") }
 symbol  { ABS(0) }
}

/* ========= Start of init ========= */

/* Have init */

InitStart {
   action  { ADD_SYMBOL("__init_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__init_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

InitStart2 {
   action  { ADD_SYMBOL("_init",SYMBOL_TYPE_FUNCTION) }
   trigger { UNDEFINED_SYMBOL("_init") && SUBSECTION_EXISTS("Linker","VECTOR___.init") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init") }
}

InitStartFALLBACK {
   action  { ADD_SYMBOL("__init_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__init_array_start") }
   symbol  { ABS(0) }
}

/* ========= End of init ========= */

/* Have init */

InitEnd {
   action  { ADD_SYMBOL("__init_array_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__init_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
   symbol  { END_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

InitEndFALLBACK {
 action  { ADD_SYMBOL("__init_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__init_array_end") }
 symbol  { ABS(0) }
}


/* ========= Start of fini ========= */

/* Have fini */

FiniStart {
   action  { ADD_SYMBOL("__fini_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__fini_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.fini_array") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.fini_array") }
}

FiniStart2 {
   action  { ADD_SYMBOL("_fini",SYMBOL_TYPE_FUNCTION) }
   trigger { UNDEFINED_SYMBOL("_fini") && SUBSECTION_EXISTS("Linker","VECTOR___.fini") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.fini") }
}
/* Start of finit, no fini => data */

FiniStartFALLBACK {
 action  { ADD_SYMBOL("__fini_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__fini_array_start") }
 symbol  { ABS(0) }
}

/* ========= End of fini ========= */

/* Have fini */

FiniEnd {
   action  { ADD_SYMBOL("__fini_array_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__fini_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.fini_array") }
   symbol  { END_OF_SUBSECTION("Linker","VECTOR___.fini_array") }
}

/* Start of finit, no fini => data */

FiniEndFALLBACK {
 action  { ADD_SYMBOL("__fini_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__fini_array_end") }
 symbol  { ABS(0) }
}


/* ===================================*/

JcrVector {
  action { VECTORIZE(".jcr") }
  trigger { SUBSECTION_EXISTS("*",".jcr") }
}

JcrLIST {
   action  { ADD_SYMBOL("__JCR_LIST__",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".jcr") }
   symbol  { START_OF_SECTION(".jcr") } 
}

CtorVector {
  action { VECTORIZE(".ctors") }
  trigger { SUBSECTION_EXISTS("*",".ctors") }
}

DtorVector {
  action { VECTORIZE(".dtors") }
  trigger { SUBSECTION_EXISTS("*",".dtors") }
}

AtExitVecor {
  action { VECTORIZE("__libc_atexit") }
  trigger { SUBSECTION_EXISTS("*","__libc_atexit") }
  
}

LibcThreadFreeresFn {
  action { VECTORIZE("__libc_thread_freeres_fn") }
  trigger { SUBSECTION_EXISTS("*","__libc_thread_freeres_fn") }
}
/*
LibcFreeresFn {
  action { VECTORIZE("__libc_freeres_fn") }
  trigger { SUBSECTION_EXISTS("*","__libc_freeres_fn") }
}
*/

StartOfAtExit {
   action  { ADD_SYMBOL("__start___libc_atexit",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS("__libc_atexit") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR_____libc_atexit") }
}

StopOfAtExit {
   action  { ADD_SYMBOL("__stop___libc_atexit",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS("__libc_atexit") }
   symbol  { END_OF_SUBSECTION("Linker","VECTOR_____libc_atexit") }
}

LibcThreadSubFreeResVector {
  action { VECTORIZE("__libc_thread_subfreeres") }
  trigger { SUBSECTION_EXISTS("*","__libc_thread_subfreeres") }
  
}

LibcSubFreeResVector {
  action { VECTORIZE("__libc_subfreeres") }
  trigger { SUBSECTION_EXISTS("*","__libc_subfreeres") }
  
}
StartOfLibcThreadSubFreeRes {
   action  { ADD_SYMBOL("__start___libc_thread_subfreeres",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS("__libc_thread_subfreeres") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR_____libc_thread_subfreeres") }
}

StopOfLibcThreadSubFreeRes {
   action  { ADD_SYMBOL("__stop___libc_thread_subfreeres",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS("__libc_thread_subfreeres") }
   symbol  { END_OF_SUBSECTION("Linker","VECTOR_____libc_thread_subfreeres") }
}

/* slightly adapted gcc linker script serves as our layout script */
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = 0x400000); . = 0x400000 + SIZEOF_HEADERS;

  PUT_REMAINING_SECTIONS(NOTE_SECTION);

  .interp         : { *(.interp) }
  .hash           : { *(.hash) }
  .dynsym         : { *(.dynsym) }
  .dynstr         : { *(.dynstr) }
  .gnu.version    : { *(.gnu.version) }
  .gnu.version_d  : { *(.gnu.version_d) }
  .gnu.version_r  : { *(.gnu.version_r) }
  .rel.dyn        :
    {
      *(.rel.init)
      *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
      *(.rel.fini)
      *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
      *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
      *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
      *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)
      *(.rel.ctors)
      *(.rel.dtors)
      *(.rel.got)
      *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
    }
  .rela.dyn       :
    {
      *(.rela.init)
      *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
      *(.rela.fini)
      *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
      *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
      *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
      *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
      *(.rela.ctors)
      *(.rela.dtors)
      *(.rela.got)
      *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
    }
  .rel.plt        : { *(.rel.plt) }
  .rela.plt       : { *(.rela.plt) }
  .init           :
  {
    KEEP (*(.init))
  } =0x90909090
  .plt            : { *(.plt) }
  .text           :
  {
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
  } =0x90909090
  .fini           :
  {
    KEEP (*(.fini))
  } =0x90909090
  PUT_REMAINING_SECTIONS(CODE_SECTION);
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  PUT_REMAINING_SECTIONS(RO_DATA_SECTION);
  .eh_frame_hdr : { *(.eh_frame_hdr) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN (0x100000) - ((0x100000 - .) & (0x100000 - 1));
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  . = ALIGN(64 / 8);
  PROVIDE (__preinit_array_start = .);
  .preinit_array     : { *(.preinit_array) }
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array     : { *(.init_array) }
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array     : { *(.fini_array) }
  PROVIDE (__fini_array_end = .);
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  }
  .data1          : { *(.data1) }
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .gcc_except_table   : { *(.gcc_except_table) }
  .dynamic        : { *(.dynamic) }
  .ctors          :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin*.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend*.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }
  .dtors          :
  {
    KEEP (*crtbegin*.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }
  PUT_REMAINING_SECTIONS(DATA_SECTION);
  .jcr            : { KEEP (*(.jcr)) }
  .got            : { *(.got.plt) *(.got) }
  _edata = .;
  PROVIDE (edata = .);
  __bss_start = .;
  .bss            :
  {
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN(64 / 8);
  }
  . = ALIGN(64 / 8);
  PUT_REMAINING_SECTIONS(BSS_SECTION);
  _end = .;
  PROVIDE (end = .);
}
