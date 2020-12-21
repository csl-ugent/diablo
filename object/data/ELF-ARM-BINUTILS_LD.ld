linker="ARM GNU LD"
fileformats="ELF"
architectures="ARM"


/* compatibility symbols */
GlibcCompatibility {
  trigger { LINKED_SYMBOL_EXISTS("GLIBC_*") }
  action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_OBJECT) }
  symbol  { ABS(0) }
}

/* section symbols */

/* ADD_SYMBOL_NEW() because the symbol must override any section symbols from
 * objects
 */
SectionSymbolsExisting {
  trigger { LINKED_SYMBOL_EXISTS("SECTIONSYM:*") && SECTION_EXISTS(SUBSTRING(MATCHED_NAME(),11,0)) }
  action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_SECTION) }
  symbol  { START_OF_SECTION(SUBSTRING(MATCHED_NAME(),11,0) ) }
}

/* ADD_LOCAL_SYMBOL() because we threw away the section, so if we add it to the
 * linked binary we'll get an error that the symbol is still undefined at the
 * end of the linking process
 */
SectionSymbolsNonExisting {
  trigger { LINKED_SYMBOL_EXISTS("SECTIONSYM:*") && !SECTION_EXISTS(SUBSTRING(MATCHED_NAME(),11,0)) }
  action  { ADD_LOCAL_SYMBOL(MATCHED_NAME(), SYMBOL_TYPE_SECTION) }
  symbol  { ABS(0) }
}

/* versions with lots of underscores */
BssStartAfterSdata {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".sdata") }
   symbol  { END_OF_SECTION(".sdata") }
}

BssStartAfterData1 {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".data1") }
   symbol  { END_OF_SECTION(".data1") }
}

BssStartAfterData {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

BssStartAfterGot {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".got") }
   symbol  { END_OF_SECTION(".got") }
}

BssStartWithSbss {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".sbss") }
   symbol  { START_OF_SECTION(".sbss") } 
}


BssStartWithBss {
   action  { ADD_SYMBOL("__bss_start__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start__")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}

BssEndWithSbss {
   action  { ADD_SYMBOL("__bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end__")   && SECTION_EXISTS(".sbss") }
   symbol  { END_OF_SECTION(".sbss") } 
}


BssEndWithBss {
   action  { ADD_SYMBOL("__bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end__")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") } 
}

BssEndWithData {
   action  { ADD_SYMBOL("__bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end__")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

BssEndWithSbssLessLeading {
   action  { ADD_SYMBOL("_bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_bss_end__")   && SECTION_EXISTS(".sbss") }
   symbol  { END_OF_SECTION(".sbss") }
}


BssEndWithBssLessLeading {
   action  { ADD_SYMBOL("_bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_bss_end__")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") }
}

BssEndWithDataLessLeading {
   action  { ADD_SYMBOL("_bss_end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_bss_end__")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

AllEnd {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_end")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") }
}

AllEndData {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_end")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}



AllEnd2 {
   action  { ADD_SYMBOL("__end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__end__")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") }
}

AllEnd2Data {
   action  { ADD_SYMBOL("__end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__end__")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}


/* version with less underscores (gcc 3.2.2 & binutils 2.13.2.1) */
BssStartAfterSdataNoTrailing {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".sdata") }
   symbol  { END_OF_SECTION(".sdata") }
}

BssStartAfterData1NoTrailing {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".data1") }
   symbol  { END_OF_SECTION(".data1") }
}

BssStartAfterDataNoTrailing {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

BssStartAfterGotNoTrailing {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".got") }
   symbol  { END_OF_SECTION(".got") }
}

BssStartWithSbssNoTrailing {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".sbss") }
   symbol  { START_OF_SECTION(".sbss") } 
}


BssStartWithBssNoTrailing  {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_start")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}

BssEndWithSbssNoTrailing  {
   action  { ADD_SYMBOL("__bss_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end")   && SECTION_EXISTS(".sbss") }
   symbol  { END_OF_SECTION(".sbss") } 
}


BssEndWithBssNoTrailing  {
   action  { ADD_SYMBOL("__bss_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") } 
}

BssEndWithDataNoTrailing  {
   action  { ADD_SYMBOL("__bss_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__bss_end")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

/***********************************************************************/

ExidxBegin {
   action  { ADD_SYMBOL("__exidx_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__exidx_start")   && SECTION_EXISTS(".ARM.exidx") }
   symbol  { START_OF_SECTION(".ARM.exidx") } 
}

ExidxEnd {
   action  { ADD_SYMBOL("__exidx_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__exidx_end")   && SECTION_EXISTS(".ARM.exidx") }
   symbol  { END_OF_SECTION(".ARM.exidx") } 
}

EhFrameBegin {
   action  { ADD_SYMBOL("__EH_FRAME_BEGIN__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__EH_FRAME_BEGIN__")   && SECTION_EXISTS(".eh_frame") }
   symbol  { START_OF_SECTION(".eh_frame") } 
}

End {
   action  { ADD_SYMBOL("end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("end") && SECTION_EXISTS(".bss")   }
   symbol  { END_OF_SECTION(".bss") } 
}

EndData {
   action  { ADD_SYMBOL("end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("end")   }
   symbol  { END_OF_SECTION(".data") }
}


/* on some platforms, the ctors are also put in the .init_array section */
CtorsVector {
   action {  VECTORIZE(".ctors") }
   trigger { SUBSECTION_EXISTS("*",".ctors") && SECTION_EXISTS(".ctors") }
}

End {
   action  { ADD_SYMBOL("_CTOR_END__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__CTOR_END__") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
   symbol  { END_OF_SUBSECTION("Linker","VECTOR___.ctors") }
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

/* ========= Start of preinit ========= */

/* Have preinit */

PreInitStart {
 action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEF_OR_NONEXIST_SYMBOL("__preinit_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.preinit_array") }
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
 trigger { UNDEF_OR_NONEXIST_SYMBOL("__preinit_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.preinit_array") }
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
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__init_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
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
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__init_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
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
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__fini_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.fini_array") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.fini_array") }
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
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__fini_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.fini_array") }
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
  trigger { UNDEF_OR_NONEXIST_SYMBOL("__JCR_LIST__") && SUBSECTION_EXISTS("Linker","VECTOR___.jcr") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___.jcr") }
}

CtorVector {
  action { VECTORIZE(".ctors") }
  trigger { SUBSECTION_EXISTS("*",".ctors") }
}

DtorVector {
  action { VECTORIZE(".dtors") }
  trigger { SUBSECTION_EXISTS("*",".dtors") }
}

GotTable {
   action  { ADD_SYMBOL("_GLOBAL_OFFSET_TABLE_",SYMBOL_TYPE_OBJECT) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_GLOBAL_OFFSET_TABLE_")  && SECTION_EXISTS(".got") }
   symbol  { START_OF_SECTION(".got") }
}


AtExitVecor {
  action { VECTORIZE("__libc_atexit") }
  trigger { SUBSECTION_EXISTS("*","__libc_atexit") }
  
}

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




/* Devkitpro linker */

bcThreadSubFreeResVector {
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

/* */

TlsStartTbss {
   action  { ADD_SYMBOL("$tls_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("$tls_start") && SECTION_EXISTS(".tdata") }
   symbol  { START_OF_SECTION(".tdata") }
}

TlsStartTbss {
   action  { ADD_SYMBOL("$tls_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("$tls_start") && SECTION_EXISTS(".tbss") && !SECTION_EXISTS(".tdata") }
   symbol  { START_OF_SECTION(".tbss") }
}

TlsEndWithTbss {
   action  { ADD_SYMBOL("_etls",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_etls")   && SECTION_EXISTS(".tbss") }
   symbol  { END_OF_SECTION(".tbss") }
}

__sp_irq {
   action  { ADD_SYMBOL("__sp_irq",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__sp_irq") }
   symbol  { ABS(0x3007fa0) }
}

__sp_usr {
   action  { ADD_SYMBOL("__sp_usr",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__sp_usr") }
   symbol  { ABS(0x3007f00) }
}

TxtStart {
   action  { ADD_SYMBOL("__text_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__text_start")   && SECTION_EXISTS(".text") }
   symbol  { START_OF_SECTION(".text") } 
}

SbssStart {
   action  { ADD_SYMBOL("__sbss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__sbss_start")   && SECTION_EXISTS(".sbss") }
   symbol  { START_OF_SECTION(".sbss") } 
}
SbssStart2 {
   action  { ADD_SYMBOL("__sbss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__sbss_start")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}
SbssStart2 {
   action  { ADD_SYMBOL("__sbss_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__sbss_end")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}



DataLma {
   action  { ADD_SYMBOL("__data_lma",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__data_lma") && SECTION_EXISTS(".iwram") && SECTION_EXISTS(".gcc_except_table") }
   symbol  { START_OF_SECTION(".text") + SIZE_OF_SECTION(".text") + SIZE_OF_SECTION(".rodata") + SIZE_OF_SECTION(".ctors") + SIZE_OF_SECTION(".dtors") + SIZE_OF_SECTION(".eh_frame") + SIZE_OF_SECTION(".gcc_except_table") + SIZE_OF_SECTION(".iwram") }
   }

DataLma {
   action  { ADD_SYMBOL("__data_lma",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__data_lma") }
   symbol  { START_OF_SECTION(".text") + SIZE_OF_SECTION(".text") + SIZE_OF_SECTION(".rodata") + SIZE_OF_SECTION(".ctors") + SIZE_OF_SECTION(".dtors") + SIZE_OF_SECTION(".eh_frame") }
   }


DataStart {
   action  { ADD_SYMBOL("__data_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__data_start")   && SECTION_EXISTS(".data") }
   symbol  { START_OF_SECTION(".data") } 
}

DataEnd {
   action  { ADD_SYMBOL("__data_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__data_end")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") } 
}

EDataSdata {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_edata")   && SECTION_EXISTS(".sdata") }
   symbol  { END_OF_SECTION(".sdata") }
}

EDataData1 {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_edata")   && SECTION_EXISTS(".data1") }
   symbol  { END_OF_SECTION(".data1") }
}

EDataData {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_edata")   && SECTION_EXISTS(".data") }
   symbol  { END_OF_SECTION(".data") }
}

EDataGot {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_edata")   && SECTION_EXISTS(".got") }
   symbol  { END_OF_SECTION(".got") }
}

EDataBSS {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_edata")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") }
}

iwram_lma {
   action  { ADD_SYMBOL("__iwram_lma",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__iwram_lma") }
   symbol  { ABS(0x0) }
}

iwram_start {
   action  { ADD_SYMBOL("__iwram_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__iwram_start") }
   symbol  { ABS(0x0) }
}

iwram_end {
   action  { ADD_SYMBOL("__iwram_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__iwram_end") }
   symbol  { ABS(0x0) }
}

iwram_load_stop {
   action  { ADD_SYMBOL("__load_stop_iwram0",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__load_stop_iwram0") }
   symbol  { ABS(0x0) }
}
iwram_load_start {
   action  { ADD_SYMBOL("__load_start_iwram0",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__load_start_iwram0") }
   symbol  { ABS(0x0) }
}
iwram_overlay_start {
   action  { ADD_SYMBOL("__iwram_overlay_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__iwram_overlay_start") }
   symbol  { ABS(0x0) }
}

ewram_lma {
   action  { ADD_SYMBOL("__ewram_lma",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__ewram_lma") }
   symbol  { ABS(0x0) }
}
ewram_start {
   action  { ADD_SYMBOL("__ewram_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__ewram_start") }
   symbol  { ABS(0x0) }
}
ewram_end {
   action  { ADD_SYMBOL("__ewram_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__ewram_end") }
   symbol  { ABS(0x0) }
}
ewram_load_stop {
   action  { ADD_SYMBOL("__load_stop_ewram0",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__load_stop_ewram0") }
   symbol  { ABS(0x0) }
}
ewram_load_start {
   action  { ADD_SYMBOL("__load_start_ewram0",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__load_start_ewram0") }
   symbol  { START_OF_SECTION(".pad") }
}
ewram_overlay_start {
   action  { ADD_SYMBOL("__ewram_overlay_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__ewram_overlay_start") }
   symbol  { ABS(0x2000000) }
}
eheap_end {
   action  { ADD_SYMBOL("__eheap_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__eheap_end") }
   symbol  { ABS(0x2040000) }
}

/* Dynamic linking stuff */

/* If the object for which we are emulating the link holds an .interp section
 * we need to recreate this section. The interp section lists the dynamic
 * loader that is to be used to load this dynamically linked program. */

Interp {
  trigger { SECTION_EXISTS(".interp") && ! SUBSECTION_EXISTS("Linker",".interp")}
	action  { CLONE_PARENT_AS_SUBSECTION("Linker", ".interp") }
}

ARMattrib {
  trigger { SECTION_EXISTS(".ARM.attributes") && ! SUBSECTION_EXISTS("Linker",".ARM.attributes")}
	action  { CLONE_PARENT_AS_SUBSECTION("Linker", ".ARM.attributes") }
}

/* Create a helper symbol to mark the start of the .got section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

GotStart {
	action  { ADD_SYMBOL("$got_start",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEF_OR_NONEXIST_SYMBOL("$got_start") && SECTION_EXISTS(".got") }
	symbol  { START_OF_SECTION(".got") } 
}

/* Create a helper symbol to mark the start of the .plt section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

PltSym {
	action  { ADD_SYMBOL("$plt_start",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEF_OR_NONEXIST_SYMBOL("$plt_start") && SECTION_EXISTS(".plt") }
	symbol  { START_OF_SECTION(".plt") } 
}

/* Create a helper symbol to mark the start of the .dynsym section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

DynSymSym {
	action  { ADD_SYMBOL("$dynsym_start",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEF_OR_NONEXIST_SYMBOL("$dynsym_start") && SECTION_EXISTS(".dynsym") }
	symbol  { START_OF_SECTION(".dynsym") } 
}

/* Create a helper symbol to mark the start of the .dynstr section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

DynSymSym {
	action  { ADD_SYMBOL("$dynstr_start",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEF_OR_NONEXIST_SYMBOL("$dynstr_start") && SECTION_EXISTS(".dynstr") }
	symbol  { START_OF_SECTION(".dynstr") } 
}

/* At the start of the .got section, we need some zero bytes. Not really sure
 * why they are all used, but at least the first one will be filled in by the
 * dynamic loader to point to the real dynamic resolver  */

NullGot {
  trigger {  SECTION_EXISTS(".got") && SECTION_EXISTS(".dynamic") && !SUBSECTION_EXISTS("Linker","NULLGOT") }
	action { ADD_SUBSECTION("Linker",".got","NULLGOT", DATA, 4, 12) }
	section {
  START_OF_SECTION(".dynamic"),
	CONST32(0x0),
	CONST32(0x0)
	} 
	address { START_OF_SECTION(".got") }
}

NullGot {
  trigger { SECTION_EXISTS(".got.") && !SECTION_EXISTS(".dynamic") && !SUBSECTION_EXISTS("Linker","NULLGOT") }
  action { ADD_SUBSECTION("Linker",".got","NULLGOT", DATA, 4, 12) }
  section {
  CONST32(0x0),
  CONST32(0x0),
  CONST32(0x0)
  } 
  address { START_OF_SECTION(".got") }
}

/* The plt starts with a stub that calls the dynamic resolver. It reads the
 * previously added NULLGOT section */

Plt {
	trigger { SECTION_EXISTS(".plt") && !SUBSECTION_EXISTS("Linker","INITPLT") }
	action { ADD_SUBSECTION("Linker",".plt","INITPLT", RODATA,4, 20) }
	section {
	CONST32(0xe52de004), /*        str     lr, [sp, #-4]!   */
	CONST32(0xe59fe004), /*        ldr     lr, [pc, #4]     */ 
	CONST32(0xe08fe00e), /*        add     lr, pc, lr       */
	CONST32(0xe5bef008), /*        ldr     pc, [lr, #8]!    */
	RELOCATED32(0x0, "$got_start", 0 ,0 ,0, "S00P-\l*w\s0000$")  /*        DATA:   0x834c           */
	} 
	address { START_OF_SECTION(".plt") }
}

/* A symbol table needs a leading '\0' in the string table as name of the null
 * symbol. We add it here. */

NullDynStr {
	trigger { SECTION_EXISTS(".dynstr") && !SUBSECTION_EXISTS("Linker","NULLDYNSTR") }
	action { ADD_SUBSECTION("Linker",".dynstr","NULLDYNSTR", RODATA,1, 1) }
	section {
	CONST8(0x0)
	} 
	address { START_OF_SECTION(".dynstr") }
}

/* A symbol table needs to start with a null symbol. We add it here. */

NullDynSym {
	trigger { SECTION_EXISTS(".dynsym") && !SUBSECTION_EXISTS("Linker","NULLDYNSYM") }
	action { ADD_SUBSECTION("Linker",".dynsym","NULLDYNSYM", RODATA,4, 16) }
	section {
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0)
	} 
	address { START_OF_SECTION(".dynsym") }
}

/* The dynamic section is terminated by a DT_NULL entry. We add it here */

EndDynamic {
	trigger {  SECTION_EXISTS(".dynamic") && !SUBSECTION_EXISTS("Linker",".dynamic.END") }
	action { ADD_SUBSECTION("Linker",".dynamic",".dynamic.END", DATA,4, 8) }
	section {
		CONST32(0x0),
		CONST32(0x0)
	 }
	 address { END_OF_SECTION(".dynamic") - ABS(0x8) }
}

/* Each dynamic entry found in the object for which we are emulating the link
 * gets marked with a special symbol by the object backend. For each of these
 * symbols we need to recreate the dynamic entry in a form that Diablo
 * understands */

/* DtInit: points to the start of the .init section */

DtInit {
	trigger { LINKED_SYMBOL_EXISTS("$dt_init") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.INIT", DATA,4, 8) }
	section { 
		/* DT_INIT */
		CONST32(0xc),
		RELOCATED32(0x0,"_init",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_init") }
}

/* DtFini: points to the start of the .fini section */

DtFini {
	trigger { LINKED_SYMBOL_EXISTS("$dt_fini") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FINI", DATA,4, 8) }
	section { 
		/* DT_FINI */
		CONST32(0xd),
		RELOCATED32(0x0,"_fini",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_fini") }
}

DtStrtab {
	trigger { LINKED_SYMBOL_EXISTS("$dt_strtab") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRTAB", DATA,4, 8) }
	section { 
		CONST32(0x5),
		RELOCATED32(0x0,"$dynstr_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_strtab") }
}

DtHash {
	trigger { LINKED_SYMBOL_EXISTS("$dt_hash") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.HASH", DATA,4, 8) }
	section { 
		CONST32(0x4),
		START_OF_SECTION(".hash")
	}
	address { SYMBOL("$dt_hash") }
}

DtPltGot {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltgot") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTGOT", DATA,4, 8) }
	section { 
		CONST32(0x3),
		START_OF_SECTION(".got")
	}
	address { SYMBOL("$dt_pltgot") }
}

DtJmpRel {
	trigger { LINKED_SYMBOL_EXISTS("$dt_jmprel") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.JMPREL", DATA,4, 8) }
	section { 
		CONST32(0x17),
		START_OF_SECTION(".rel.plt")
	}
	address { SYMBOL("$dt_jmprel") }
}

DtBindNow {
	trigger { LINKED_SYMBOL_EXISTS("$dt_bind_now") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.BIND_NOW", DATA,4, 8) }
	section {
		CONST32(0x18),
                CONST32(0x0)
	}
	address { SYMBOL("$dt_bind_now") }
}

DtRel {
	trigger { LINKED_SYMBOL_EXISTS("$dt_rel") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.REL", DATA,4, 8) }
	section { 
		CONST32(0x11),
		START_OF_SECTION(".rel.dyn")
	}
	address { SYMBOL("$dt_rel") }
}

DtRelSize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_relsize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELSIZE", DATA,4, 8) }
	section { 
		CONST32(0x12),
		SIZE_OF_SECTION(".rel.dyn")
	}
	address { SYMBOL("$dt_relsize") }
}

DtPltRelSize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltrelsize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTRELSIZE", DATA,4, 8) }
	section { 
		CONST32(0x2),
		SIZE_OF_SECTION(".rel.plt")
	}
	address { SYMBOL("$dt_pltrelsize") }
}

DtSymtab {
	trigger { LINKED_SYMBOL_EXISTS("$dt_symtab") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMTAB", DATA,4, 8) }
	section { 
		CONST32(0x6),
		RELOCATED32(0x0,"$dynsym_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_symtab") }
}

DtStrsz {
	trigger { LINKED_SYMBOL_EXISTS("$dt_strsize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRSZ", DATA,4, 8) }
	section {
		CONST32(0xa),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL(MATCHED_NAME()) }
}

DtRelEnt {
	trigger { LINKED_SYMBOL_EXISTS("$dt_relent") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELENT", DATA,4, 8) }
	section { 
		CONST32(0x13),
		CONST32(0x8)
	}
	address { SYMBOL("$dt_relent") }

}

DtPltRel {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltrel") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTREL", DATA,4, 8) }
	section { 
		CONST32(0x14),
		CONST32(0x11)
	}
	address { SYMBOL("$dt_pltrel") }

}

DtDebug {
	trigger { LINKED_SYMBOL_EXISTS("$dt_debug") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.DEBUG", DATA,4, 8) }
	section { 
		CONST32(0x15),
		CONST32(0x0)
	}
	address { SYMBOL("$dt_debug") }

}

DtSymEnt {
	trigger { LINKED_SYMBOL_EXISTS("$dt_syment") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMENT", DATA,4, 8) }
	section { 
		CONST32(0xb),
		CONST32(0x10)
	}
	address { SYMBOL("$dt_syment") }

}


DtInitArray {
	trigger { LINKED_SYMBOL_EXISTS("$dt_initarray") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.INITARRAY", DATA,4, 8) }
	section {
		/* DT_INIT_ARRAY */
		CONST32(0x19),
		RELOCATED32(0x0,"__init_array_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_initarray") }
}

DtPreInitArray {
	trigger { LINKED_SYMBOL_EXISTS("$dt_preinitarray") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PREINITARRAY", DATA,4, 8) }
	section {
		/* DT_INIT_ARRAY */
		CONST32(0x20),
		RELOCATED32(0x0,"__preinit_array_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_preinitarray") }
}

DtFiniArray {
	trigger { LINKED_SYMBOL_EXISTS("$dt_finiarray") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FINIARRAY", DATA,4, 8) }
	section {
		/* DT_FINI_ARRAY */
		CONST32(0x1a),
		RELOCATED32(0x0,"__fini_array_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_finiarray") }
}

DtInitArraySize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_initarraysize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.INITARRAYSIZE", DATA,4, 8) }
	section {
		CONST32(0x1b),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL("$dt_initarraysize") }

}

DtPreInitArraySize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_preinitarraysize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PREINITARRAYSIZE", DATA,4, 8) }
	section {
		CONST32(0x21),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL("$dt_preinitarraysize") }

}

DtFiniArraySize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_finiarraysize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FINIARRAYSIZE", DATA,4, 8) }
	section {
		CONST32(0x1c),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL("$dt_finiarraysize") }

}

DtFlags {
	trigger { LINKED_SYMBOL_EXISTS("$dt_flags") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FLAGS", DATA,4, 8) }
	section {
		CONST32(0x1e),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL("$dt_flags") }

}

DtFlags_1 {
	trigger { LINKED_SYMBOL_EXISTS("$dt_flags_1") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FLAGS_1", DATA,4, 8) }
	section {
		CONST32(0x6ffffffb),
		CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+4))
	}
	address { SYMBOL("$dt_flags_1") }
}

DtNeededString {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("$dt_needed:*") && !SUBSECTION_EXISTS("Linker",CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),11, 0))) }
	section { STRING(SUBSTRING(MATCHED_NAME(), 11, 0)), CONST8(0x0) }
	address { READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+ABS(4)) + START_OF_SECTION(".dynstr")}
}

DtNeededStringAfterLinkerEmulation {
	trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("$dt_needed:*") && !SUBSECTION_EXISTS("Linker",CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),11, 0))) }
	section { STRING(SUBSTRING(MATCHED_NAME(), 11, 0)), CONST8(0x0) }
	address { ABS(0x0) }
}

DtNeededStringSymbol {
	trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") && !SYMBOL_EXISTS(CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	action { ADD_SYMBOL_NEW(CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
}

DtNeeded {
	trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*")  && !SUBSECTION_EXISTS("Linker",CONCAT(".dynamic.NEEDED:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	action  { ADD_SUBSECTION("Linker", ".dynamic", CONCAT(".dynamic.NEEDED:",SUBSTRING(MATCHED_NAME(),11, 0)), DATA, 4, 8) }
	section { 
		CONST32(0x1),
		RELOCATED32(0x0, CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$")

	}
	address { SYMBOL(MATCHED_NAME()) }
}

DtSonameString {
	trigger { LINKED_SYMBOL_EXISTS("$dt_soname:*") }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("SONAMESTR:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),11, 0))) }
	section { STRING(SUBSTRING(MATCHED_NAME(), 11, 0)), CONST8(0x0) }
	address { READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+ABS(4)) + START_OF_SECTION(".dynstr")}
}

DtSonameStringSymbol {
	trigger { LINKED_SYMBOL_EXISTS("$dt_soname:*") }
	action { ADD_SYMBOL_NEW(CONCAT("SONAMESTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker", CONCAT("SONAMESTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	 }

DtSoname {
	trigger { LINKED_SYMBOL_EXISTS("$dt_soname:*") }
	action  { ADD_SUBSECTION("Linker", ".dynamic", CONCAT(".dynamic.SONAME:",SUBSTRING(MATCHED_NAME(),11, 0)), DATA, 4, 8) }
	section {
		CONST32(0xe),
		RELOCATED32(0x0, CONCAT("SONAMESTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$")
	}
	address { SYMBOL(MATCHED_NAME()) }
}

DtVerSym {
	trigger { LINKED_SYMBOL_EXISTS("$dt_versym") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.VERSYM", DATA,4, 8) }
	section {
		/* DT_VERSYM */
		CONST32(0x6ffffff0),
		START_OF_SECTION(".gnu.version")
	}
	address { SYMBOL("$dt_versym") }
}

DtVerDef {
	trigger { LINKED_SYMBOL_EXISTS("$dt_verdef") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.VERDEF", DATA,4, 8) }
	section {
		/* DT_VERDEF */
		CONST32(0x6ffffffc),
		START_OF_SECTION(".gnu.version_d")
	}
	address { SYMBOL("$dt_verdef") }
}

DtVerDefNum {
	trigger { LINKED_SYMBOL_EXISTS("$dt_verdefnum") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.VERDEFNUM", DATA,4, 8) }
	section {
		/* DT_VERDEFNUM */
		CONST32(0x6ffffffd),
	        /* every entry's size is 4 Elf half words + 3 Elf words, but
	         * there are also "aux" entries. We shouldn't add/remove
	         * data here, so just copy the original value
	         */
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4))
	}
	address { SYMBOL("$dt_verdefnum") }
}


DtVerNeed {
	trigger { LINKED_SYMBOL_EXISTS("$dt_verneed") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.VERNEED", DATA,4, 8) }
	section {
		/* DT_VERNEED */
		CONST32(0x6ffffffe),
		START_OF_SECTION(".gnu.version_r")
	}
	address { SYMBOL("$dt_verneed") }
}

DtVerNeedNum {
	trigger { LINKED_SYMBOL_EXISTS("$dt_verneednum") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.VERNEEDNUM", DATA,4, 8) }
	section {
		/* DT_VERNEEDNUM */
		CONST32(0x6fffffff),
	        /* every entry's size is 2 Elf half words + 3 Elf words, but
	         * there are also "aux" entries. We shouldn't add/remove
	         * data here, so just copy the original value
	         */
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4))
	}
	address { SYMBOL("$dt_verneednum") }
}

DtSymbolic {
  trigger { LINKED_SYMBOL_EXISTS("$dt_symbolic") }
  action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMBOLIC", DATA,4, 8) }
  section {
    CONST32(0x10),
                CONST32(0x0)
  }
  address { SYMBOL("$dt_symbolic") }
}

DtTextrel {
  trigger { LINKED_SYMBOL_EXISTS("$dt_textrel") }
  action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.TEXTREL", DATA,4, 8) }
  section {
    CONST32(0x16),
    CONST32(0x0)
  }
  address { SYMBOL("$dt_textrel") }
}

DtRPathString {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),10, 0))) }
  section {
    STRING(SUBSTRING(MATCHED_NAME(), 10, 0)),
    CONST8(0x0)
  }
  address { READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME()) + ABS(4)) + START_OF_SECTION(".dynstr")}
}

DtRPathStringSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action { ADD_SYMBOL_NEW(CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 12, SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0))) }
}

DtRPath {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action  { ADD_SUBSECTION("Linker", ".dynamic", ".dynamic.RPATH", DATA, 4, 8) }
  section {
    CONST32(0xf),
    RELOCATED32(0x0, CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$")
  }
  address { SYMBOL(MATCHED_NAME()) }
}

/* entries in the .gnu.versym section */
VerSymNullEntry {
  trigger { SECTION_EXISTS(".gnu.version") && !SUBSECTION_EXISTS("Linker",".gnu.version.NULLSYM") }
  action  { ADD_SUBSECTION("Linker",".gnu.version",".gnu.version.NULLSYM", RODATA, 2, 2) }
  section { CONST16(0x0) }
  address { START_OF_SECTION(".gnu.version") }
}

VerSymEntry {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("VERSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
  action  { ADD_SUBSECTION("Linker",".gnu.version",CONCAT(".gnu.version.",SUBSTRING(MATCHED_NAME(),7,0)), RODATA, 2, 2) }
  section { CONST16(READ_LINKED_VALUE16(MATCHED_SYMBOL_VALUE())) }
  address { MATCHED_SYMBOL_VALUE() }
}

VerSymEntryAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("VERSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
  action  { ADD_SUBSECTION("Linker",".gnu.version",CONCAT(".gnu.version.",SUBSTRING(MATCHED_NAME(),7,0)), RODATA, 2, 2) }
  section { CONST16(READ_LINKED_VALUE16(MATCHED_SYMBOL_VALUE())) }/* DIRTY HACK! This entry is supposed to tell the dynamic linker which version of the shared library is to be used. As it is too hard to determine whether the entry we need already exists (and if so which index it has) we just use the value 0 which means it's a local symbol. This does not make a difference to our ends as the symbols we add are unversioned anyway */
  address { MATCHED_SYMBOL_VALUE() }
}

/* entry in the VERDEF section */
VerdefEntry {
  trigger { LINKED_SYMBOL_EXISTS("VERDEFDATA:ENTRY:*") }
  action  { ADD_SUBSECTION("Linker",".gnu.version_d",CONCAT(".gnu.version_d.",MATCHED_NAME()), RODATA, 4, 20) }
  section {
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE())), /* version and flags */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4)), /* version index and cnt of aux entries */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)), /* hash */
            RELOCATED32(0x0, CONCAT("VERDEFDATA:AUX_FOR_ENTRY:",SUBSTRING(MATCHED_NAME(),17,0)), 0, MATCHED_NAME(), 0, "S00S01-\l*w\s0000$"), /* offset from start of this entry to the aux data */
            RELOCATED32(0x0, CONCAT("VERDEFDATA:ENTRYNEXT:",SUBSTRING(MATCHED_NAME(),17,0)), 0, MATCHED_NAME(),0, "S00S01-\l*w\s0000$") /* offset from start of this entry to the next entry */
          }
  address { MATCHED_SYMBOL_VALUE() }
}

/* auxiliary entry in the VERDEF section */
VerdefAux {
  trigger { LINKED_SYMBOL_EXISTS("VERDEFDATA:AUX:*") }
  action  { ADD_SUBSECTION("Linker",".gnu.version_d",CONCAT(".gnu.version_d.",MATCHED_NAME()), RODATA, 4, 8) }
  section {
            RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 0, "$dynstr_start", 0, "S00S01-\l*w\s0000$"), /* offset of version name in dynstr section */
            RELOCATED32(0x0, CONCAT("VERDEFDATA:AUXNEXT:",STRINGTOKEN(MATCHED_NAME(),":",2)), 0, MATCHED_NAME(),0, "S00S01-\l*w\s0000$") /* offset from start of this aux to the next aux */
          }
  address { MATCHED_SYMBOL_VALUE() }
}

/* copy all symbols that we use to (re)generate the data */
VerDefDataSymbols {
  trigger { LINKED_SYMBOL_EXISTS("VERDEFDATA:*") }
  action  { ADD_SYMBOL(MATCHED_NAME(),SYMBOL_TYPE_NOTYPE) }
  symbol  { DUPLICATE_ORIG(MATCHED_NAME()) }
}

/* entry in the VERNEED section */
VerneedEntry {
  trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:ENTRY:*") }
  action  { ADD_SUBSECTION("Linker",".gnu.version_r",CONCAT(".gnu.version_r.",MATCHED_NAME()), RODATA, 4, 16) }
  section {
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE())), /* version and cnt of aux entries */
            RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 0, "$dynstr_start", 0, "S00S01-\l*w\s0000$"), /* file name for this entry */
            RELOCATED32(0x0, CONCAT("VERNEEDDATA:AUX_FOR_ENTRY:",STRINGTOKEN(MATCHED_NAME(),":",2)), 0, MATCHED_NAME(), 0, "S00S01-\l*w\s0000$"), /* offset from start of this entry to the aux data */
            RELOCATED32(0x0, CONCAT("VERNEEDDATA:ENTRYNEXT:",STRINGTOKEN(MATCHED_NAME(),":",2)), 0, MATCHED_NAME(),0, "S00S01-\l*w\s0000$") /* offset from start of this entry to the next entry */
          }
  address { MATCHED_SYMBOL_VALUE() }
}

/* auxiliary entry in the VERNEED section */
VerneedAux {
  trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:AUX:*") }
  action  { ADD_SUBSECTION("Linker",".gnu.version_r",CONCAT(".gnu.version_r.",MATCHED_NAME()), RODATA, 4, 16) }
  section {
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE())), /* hash */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() + 4)), /* flags and other */
            RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 0, "$dynstr_start", 0, "S00S01-\l*w\s0000$"), /* offset of version name in dynstr section */
            RELOCATED32(0x0, CONCAT("VERNEEDDATA:AUXNEXT:",STRINGTOKEN(MATCHED_NAME(),":",2)), 0, MATCHED_NAME(),0, "S00S01-\l*w\s0000$") /* offset from start of this aux to the next aux */
          }
  address { MATCHED_SYMBOL_VALUE() }
}

/* copy all symbols that we use to (re)generate the data */
VerNeedDataSymbols {
  trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:*") }
  action  { ADD_SYMBOL(MATCHED_NAME(),SYMBOL_TYPE_NOTYPE) }
  symbol  { DUPLICATE_ORIG(MATCHED_NAME()) }
}

/* The "!= 3" check is to skip section symbols, their name needs to be empty */
DynSymStringNoSection {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 3) }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 1, STRLEN(SUBSTRING(MATCHED_NAME(),13,0)) + 1) }
	section { STRING(SUBSTRING(MATCHED_NAME(),13,0)) , CONST8(0) }
	address { READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) + START_OF_SECTION(".dynstr") }
}

DynSymStringNoSectionAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") }
  action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 1, STRLEN(SUBSTRING(MATCHED_NAME(),13,0)) + 1) }
  section { STRING(SUBSTRING(MATCHED_NAME(),13,0)) , CONST8(0) }
  address { ABS(0x0) }
}

DynSymStringSymbolNoSection {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 3) }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0))) }
}

DynSymStringSymbolNoSectionAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") }
  action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 12, SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0))) }
}

/* the start of the dynstr section contains an empty symbol name */
DynSymStringSymbolSection {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) == 3) }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SECTION(".dynstr") }
}

/* strings for versioning names */
DynStringVerdefAux {
        trigger { LINKED_SYMBOL_EXISTS("VERDEFDATA:AUX:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
        action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", STRINGTOKEN(MATCHED_NAME(),":",3)), RODATA, 1, STRLEN(STRINGTOKEN(MATCHED_NAME(),":",3)) + 1) }
	section { STRING(STRINGTOKEN(MATCHED_NAME(),":",3)) , CONST8(0) }
        address { START_OF_SECTION(".dynstr") + READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) }
}

DynStringVerneed {
        trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:ENTRY:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
        action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", STRINGTOKEN(MATCHED_NAME(),":",3)), RODATA, 1, STRLEN(STRINGTOKEN(MATCHED_NAME(),":",3)) + 1) }
	section { STRING(STRINGTOKEN(MATCHED_NAME(),":",3)) , CONST8(0) }
        address { START_OF_SECTION(".dynstr") + READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() + 4) }
}

DynStringVerneedAux {
        trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:AUX:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
        action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", STRINGTOKEN(MATCHED_NAME(),":",3)), RODATA, 1, STRLEN(STRINGTOKEN(MATCHED_NAME(),":",3)) + 1) }
	section { STRING(STRINGTOKEN(MATCHED_NAME(),":",3)) , CONST8(0) }
        address { START_OF_SECTION(".dynstr") + READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() + 8) }
}

/* string symbols for versioning names (!SYMBOL_EXISTS() rather than SYMBOL_UNDEFINED(), because
 * these only look in the SUB_SYMBOL_TABLE() and the symbol references have been added here in the
 * linker script -> we would ened a LINKED_SYMBOL_UNDEFINED() or so
 */
DynStringSymVerdefAux {
        trigger { LINKED_SYMBOL_EXISTS("VERDEFDATA:AUX:*") && !SYMBOL_EXISTS(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
}

DynStringSymVerneed {
        trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:ENTRY:*") && !SYMBOL_EXISTS(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
}

DynStringSymVerneedAux {
        trigger { LINKED_SYMBOL_EXISTS("VERNEEDDATA:AUX:*") && !SYMBOL_EXISTS(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",STRINGTOKEN(MATCHED_NAME(),":",3))) }
}


/* address/value encoded in a dynamic symbol:
 *  - undefined/external (UNDEF_OR_NONEXIST_SYMBOL) symbol accessed from a dynamic library (GENERATE_PIC),
 *    nowhere do we take the address of its plt entry (DYNSYMPLTADDR) and the symbol is not defined in the
 *    current library (UNDEF_OR_NONEXIST_SYMBOL): NULL (DynSymUndefinedPIC rule)
 *  - undefined/external (UNDEF_OR_NONEXIST_SYMBOL) weak symbol (WEAKDYNSYMSYM) accessed from a non-PIE
 *    binary (!GENERATE_PIC) with same other conditions as above: NULL (DynSymWeakUndefinedNonPic rule)
 *  - symbols defined in the current binary/library: address of the symbol. In case of exported code
 *    we will change the address of the symbol in the new binary to the location where we generate the
 *    PLT, so that all calls to the original symbol are redirected to the PLT. The dynamic symbol must
 *    however refer to the original location of the symbol (since it's used by the dynamic linker to
 *    determine the address to which the PLT entry must branch), so we make a copy of the original
 *    symbol under a new name (DynSymOrig rule) and then refer to this copy (DynSymDefined rule)
 *  - all other undefined symbols: address of the symbol, which may also be NULL/undefined, the address
 *    of a plt entry, or the address of copied piece of data in case of an ARM_COPY relocation etc
 *    (DynSymOtherExternal)
 */
DynSymUndefinedPIC {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && GENERATE_PIC() && !LINKED_SYMBOL_EXISTS(CONCAT("DYNSYMPLTADDR:", SUBSTRING(MATCHED_NAME(), 13, 0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, MATCHED_NAME(), 0, 0, 0, "S00*s0000\l*w\s0000$"),  /* force 0, but in a way so that we get a fatal if the original binary did not have 0 */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

DynSymUndefinedPICAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && GENERATE_PIC() && !LINKED_SYMBOL_EXISTS(CONCAT("DYNSYMPLTADDR:", SUBSTRING(MATCHED_NAME(), 13, 0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, MATCHED_NAME(), 0, 0, 0, "S00*s0000\l*w\s0000$"),  /* force 0, but in a way so that we get a fatal if the original binary did not have 0 */
            CONST32(0x0) /* The symbol size */,
            CONST32(0x12) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymWeakUndefinedNonPic {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("WEAKDYNSYMSYM:*") && !GENERATE_PIC() && !LINKED_SYMBOL_EXISTS(CONCAT("DYNSYMPLTADDR:", SUBSTRING(MATCHED_NAME(), 14, 0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),14,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),14,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),14,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),14,0)), 0, 0, 0, "S00*s0000\l*w\s0000$"),  /* force 0, but in a way so that we get a fatal if the original binary did not have 0 */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

/* extra subsection exists check because UNDEF_OR_NONEXIST_SYMBOL() return TRUE for "weak undefined" symbols
 * (the reason a "weak undefined" symbol is not considered to be only "undefined" is because the absence of
 *  such a symbol must not trigger an error, since it's weak)
 */
DynSymOrig {
        trigger {
                  LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)))
                }
        action  { ADD_SYMBOL_NEW(CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(),13,0)), 12, MATCHED_SYMBOL_FLAGS()) }
        symbol  { DUPLICATE(SUBSTRING(MATCHED_NAME(),13,0)) }
}

DynSymDefined {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) &&
            ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 3) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 6) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, 0, 0, "S00A00+S00M|\l*w\s0000$"),  /* jump slot address entry */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
      CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymDefinedTLS {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) &&
            ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 3) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) == 6) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$tls_start", 0, "S00S01-S00M|\l*w\s0000$"),  /* jump slot address entry */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
      CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymDefinedSection {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) &&
            ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) == 3) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, 0, 0, "S00A00+\l*w\s0000$"),  /* jump slot address entry */
            CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
      CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymDefinedAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, 0, 0, "S00A00+\l*w\s0000$"),  /* jump slot address entry */
            CONST32(0x0) /* The symbol size */,
            CONST32(0x12) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymOtherExternal {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(), 13, 0), 0, 0, 0, "S00A00+S00M|\l*w\s0000$"),  /* jump slot address entry */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

/* The case where we need to add PLT */

/* The DYNSYMSYM:* symbols refer to the Elf32_Sym struct describing the dynamic symbol.
 * The 32 bit value at +12 contains the st_info (lowest byte on little endian), st_other
 * (second byte on little endian) and st_shndx (upper 16 bits on little endian) fields.
 *
 * The st_info contains the binding (upper nibble) and type (lower nible) info.
 * Meanings of the checks:
 *   & 0xffff000f == 2: the symbol has shndx==0 (undefined section) and type=2 (STT_FUNC)
 *   & 0xf == 2: the symbol has type=2 (STT_FUNC)
 */


/* In case we have a (dynamic) JUMP_SLOT relocation to a dynamic symbol, we
 * need to add a PLT entry. This PLT entry loads the target address from
 * the GOT, so we also need to add a GOT entry.
 *
 * The JUMP_SLOT symbol has been added at the address of the original
 * R_ARM_JUMP_SLOT relocation in the linked binary. This is a dynamic
 * relocation, which means that the first word is the address of the
 * place where the value is that needs to be relocated (= the entry
 * in the GOT that is to contain the address of the routine called by
 * the PLT entry) */
DynSymGot {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action  { ADD_SUBSECTION("Linker", ".got", CONCAT("DYNAMIC_GOTELEM:",SUBSTRING(MATCHED_NAME(), 10, 0)), DATA, 4, 4) }
	section { RELOCATED32(0x0, "$plt_start", 0, 0, 0,"S00A00+\l*w\s0000$") }

	address { READ_LINKED_ADDR32(SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0)))) }
}

DynSymGotSymbol {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
        action { ADD_SYMBOL_NEW(CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_GOTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSectionNormalArm {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          !LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0)))
                }
	action  { ADD_SUBSECTION("Linker", ".plt", CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 12) }
	section { 
		RELOCATED32(0xe28fc600, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000008-i00000014>\liffffff00&|w\s0000$"), 
		RELOCATED32(0xe28cca00, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000004-i0000000c>i000000ff&\liffffff00&|w\s0000$"), 
		RELOCATED32(0xe5bcf000, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000fff&\lifffff000&|w\s0000$"), 
		RELOCATED0(0x0, CONCAT("PLTREL:", SUBSTRING(MATCHED_NAME(),10,0)), 0, 0,0, "S00\*\s0000$")  
       }
       address { SYMBOL(CONCAT("PLTELEMORIG:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSymbolARM {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          !LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
	          UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0))
		}
	action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltTmpSymbolARM {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          !LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0)))
                }
	action { ADD_SYMBOL_NEW(CONCAT("PLTELEMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSectionWithThumbStub {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
	action  { ADD_SUBSECTION("Linker", ".plt", CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 16) }
	section {
                CONST32(0x46c04778), /* (in thumb) bx pc; nop */
		RELOCATED32(0xe28fc600, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000008-i00000014>\liffffff00&|w\s0000$"),
		RELOCATED32(0xe28cca00, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000004-i0000000c>i000000ff&\liffffff00&|w\s0000$"),
		RELOCATED32(0xe5bcf000, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0,0,0, "S00A00+P-i00000fff&\lifffff000&|w\s0000$"),
		RELOCATED0(0x0, CONCAT("PLTREL:", SUBSTRING(MATCHED_NAME(),10,0)), 0, 0,0, "S00\*\s0000$")
       }
       address { SYMBOL(CONCAT("PLTELEMTHUMBORIG:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSymbolThumb {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
	action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_FUNCTION) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) + 4 }
}

PltTmpSymbolThumb {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
	action { ADD_SYMBOL_NEW(CONCAT("PLTELEMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) + 4 }
}

PltSymbolThumbTrampoline {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
	action { ADD_SYMBOL_NEW(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),10,0)),"_from_thumb"), 12, SYMBOL_TYPE_FUNCTION) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) + 1}
}


PltSectionWithThumbStubCodeSym1 {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
        action { ADD_SYMBOL_NEW("$t",0-1,SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSectionWithThumbStubCodeSym2 {
	trigger {
	          LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
	          LINKED_SYMBOL_EXISTS(CONCAT("PLTELEMTHUMBORIG:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 17, 0)))
                }
        action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) + 4 }
}


PltRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.plt", CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 8) }
	section {
		RELOCATED32(0x0, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0,"S00A00+\l*w\s0000$"),
		RELOCATED32(0x16, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
        address { SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0))) }
     }

PltRelSymbol {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SYMBOL_NEW(CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

DynSymSymbolJump {
	trigger { LINKED_SYMBOL_EXISTS("JUMP_SLOT:*") && LINKED_SYMBOL_EXISTS(CONCAT("ANYDYNSYMSYM:", SUBSTRING(MATCHED_NAME(), 10, 0))) }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
	     }

/* The case where we allocate some data */

/* ARM_COPY relocs exist for static data that exists in a dynamic library and that must be
 * copied into the executable. There may not be a reference from inside the executable to
 * this data (all uses may be in the dynamic library), so the symbol may not exist rather
 * than be undefined -> "undefine" it so that we create data for it.
 */

ArmCopyRelNonExisting {
	trigger { LINKED_SYMBOL_EXISTS("ARM_COPY:*") && !SYMBOL_EXISTS(SUBSTRING(MATCHED_NAME(),9,0))}
	action { ADD_SYMBOL(SUBSTRING(MATCHED_NAME(),9,0), SYMBOL_TYPE_NOTYPE) }
	symbol { UNDEFINED() }
}

DynSymDataBss {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && (!LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:",SUBSTRING(MATCHED_NAME(),13,0))) || LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),13,0)))) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) }
	action  { ADD_SUBSECTION("Linker", ".bss", CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(), 13, 0)), BSS, 4, READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) }
	section { RELOCATED32(0x0, CONCAT("ARM_COPY:", SUBSTRING(MATCHED_NAME(),13,0)), 0, 0,0, "S00\*\s0000$")  }
	address { READ_LINKED_ADDR32(MATCHED_SYMBOL_VALUE()+4) }
}

/* This rule hasn't been actually implemented yet as I'm not sure how this would look */
DynSymDataBssAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) && (!LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:",SUBSTRING(MATCHED_NAME(),13,0))) || LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),13,0)))) }
  action  { ADD_SUBSECTION("Linker", ".bss", CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(), 13, 0)), BSS, 4, READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) }
  section { RELOCATED32(0x0, CONCAT("ARM_COPY:", SUBSTRING(MATCHED_NAME(),13,0)), 0, 0,0, "S00\*\s0000$")  }
  address { READ_LINKED_ADDR32(MATCHED_SYMBOL_VALUE()+4) }
}

/* add ANYDYNSYMSYM:* alias used in rules that apply to both weak and regular dynamic symbols (both .bss and .data/.rodata -> no undefined check) */
DynSymNoJumpSlotSymbol {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("DYNSYMSYM:*")  && (!LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:",SUBSTRING(MATCHED_NAME(),10,0))) || LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),10,0))) ) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) }
	action { ADD_SYMBOL_NEW(CONCAT("ANY",MATCHED_NAME()), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

DynSymDataSymbol {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && (!LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:",SUBSTRING(MATCHED_NAME(),10,0))) || LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),10,0))) ) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
        action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* This rule hasn't been actually implemented yet as I'm not sure how this would look */
DynSymDataSymbolAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && (!LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:",SUBSTRING(MATCHED_NAME(),10,0))) || LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),10,0)))) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
  action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* weak symbols that are not explicitly defined should stay undefined so they
 * can dynamically bind against a symbol in a dynamic library if that exists
 * at run time, except in case of a copy relocation, in which case it must
 * be bound to the copied data
 */
WeakDynSymDataSymbol {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("WEAKDYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) && LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),14,0))) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),9,0)) }
        action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),9,0), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),9,0))) }
}

/* This rule hasn't been actually implemented yet as I'm not sure how this would look */
WeakDynSymDataSymbolAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("WEAKDYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) && LINKED_SYMBOL_EXISTS(CONCAT("ARM_COPY:",SUBSTRING(MATCHED_NAME(),14,0))) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),9,0)) }
        action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),9,0), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),9,0))) }
}

/* globdat */
GlobDatRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),9,0)), 0, 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(0x15, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),9,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("GLOB_DAT:", SUBSTRING(MATCHED_NAME(), 9, 0))) }
}

/* tls_tpoff */
TlsTpOffRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("TLS_TPOFF:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(0x13, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("TLS_TPOFF:", SUBSTRING(MATCHED_NAME(), 10, 0))) }
}

/* tls_dtpmod */
TlsDtpModRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("TLS_DTPMOD:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),11,0)), 0, 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(0x11, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),11,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("TLS_DTPMOD:", SUBSTRING(MATCHED_NAME(), 11, 0))) }
}

/* tls_dtpoff */
TlsDtpOffRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("TLS_DTPOFF:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    /* "ELF for the ARM Architecture" 4.6.1.9
     * R_ARM_TLS_GD32 (parsed in diabloelf_arm.c) causes two adjacent entries to be added (...)
     * The first of these is dynamically relocated by R_ARM_TLS_DTPMOD32,
     * the second by R_ARM_TLS_DTPOFF32.
     *
     * Since we're handling DTPOFF here, an addend of 4 to the target symbol is required.
     */
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),11,0)), 4, 0, 0, "S00A00+\l*w\s0000$"),
		RELOCATED32(0x12, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),11,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("TLS_DTPOFF:", SUBSTRING(MATCHED_NAME(), 11, 0))) }
}

/* bind any potentially unresolved references to (WEAK/ANY/)DYNSYMSYM
 * to the dynamic symbols
 */
DynSymSymbolUndefined {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0))) }
	     }

WeakDynSymSymbolUndefined {
	trigger { LINKED_SYMBOL_EXISTS("WEAKDYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),14,0))) }
}

AnyDynSymSymbolUndefined2 {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
}



/* some exceptions: __aeabi_unwind_cpp_pr0 - __aeabi_unwind_cpp_pr1 (added by the linker,
 * so we have to do the same)
 */
SymbolAeabi_unwind {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:__aeabi_unwind_cpp_pr*") && !LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:",SUBSTRING(MATCHED_NAME(),13,0))) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
	action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),13,0), 12, SYMBOL_TYPE_FUNCTION) }
	symbol { ABS(0) }
}


/* ARM COPY REL */

ArmCopyRel {
	trigger { LINKED_SYMBOL_EXISTS("ARM_COPY:*") }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
		RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),9,0), 0, 0, 0,"S00A00+\l*w\s0000$"),
		RELOCATED32(0x14, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),9,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { MATCHED_SYMBOL_VALUE() }
     }

ArmCopySymbol {
	trigger { LINKED_SYMBOL_EXISTS("ARM_COPY:*") }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNREL:",MATCHED_NAME())) }
}

ArmCopyAliases {
	trigger { LINKED_SYMBOL_EXISTS("DIABLOMAPSYM:*") }
	action { ADD_SYMBOL_NEW(STRINGTOKEN(MATCHED_NAME(),":",1), 12, MATCHED_SYMBOL_FLAGS()) }
	symbol { DUPLICATE(STRINGTOKEN(MATCHED_NAME(),":",2)) }
}

ElfResizeHashSection {
  trigger { SECTION_EXISTS(".hash") }
  action { ELF_RESIZE_HASH_SECTION() }
}

/* End of dynamic linking */

/* thumb-arm and arm-thumb stubs */


/* for additionally linked objects it's possible that no veneer exists, but
 * that we need one anyway -> enlarge the text section and add symbols
 * on which the following rules will trigger
 * Ideally we would also do this for local symbols (because then multiple
 * trampolines with the same name can exist), but this is not yet supported.
 */
BranchVeneerTargetNewSection {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") &&
            SYMBOL_EXISTS("DIABLO_POTENTIAL_THUMB_VENEER:*") &&
            UNDEFINED_SYMBOL(MATCHED_NAME()) &&
            !LINKED_SYMBOL_EXISTS(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_from_arm")) &&
            !LINKED_SYMBOL_EXISTS(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_veneer")) }
  action { ARM_MAYBE_ADD_FROM_ARM_TO_THUMB_STUB(MATCHED_NAME()) }
}

/* Add at most one "short format" __*_from_thumb stub.  */
BranchToArmFromThumbPIC {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe) == 0x46c04778) && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe +4) == 0xe59fc000) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.stubPIC",MATCHED_NAME()),CODE,8,16) }
  section { CONST16(0x4778),  /* bx      pc */
            CONST16(0x46c0),  /* nop ; (mov r8, r8) */
            CONST32(0xe59fc000), /* ldr ip, [pc] */
            CONST32(0xe08cf00f), /* add pc, ip, pc */
            RELOCATED32(0x00000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-13),0xfffffffc,0,0,"S00A00+P-\l*w\s0000$" )
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()-1) }
}

BranchToArmFromThumb {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe) == 0x46c04778) && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe +4) & 0xff000000) == 0xea000000) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.stub",MATCHED_NAME()),CODE,8,8) }
  section { CONST16(0x4778),  /* bx      pc */
            CONST16(0x46c0),  /* nop ; (mov r8, r8) */
            RELOCATED32(0xea000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-13),0,0,0,"S00 P - i00000008 - i00000002 > i00ffffff & \ l iff000000 & | w \s0000$" )
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()-1) }
}

/* Add at most one "short format" __*_from_arm stub.  */

BranchToThumbFromArmPIC {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) && READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) == 0xe59fc004 }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.stubPIC",MATCHED_NAME()),CODE,8,16) }
  section { CONST32(0xe59fc004), /* ldr ip, [pc, #4] */
            CONST32(0xe08fc00c), /* add ip, pc, ip */
            CONST32(0xe12fff1c), /* bx ip  */
            RELOCATED32(0x00000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-11),0,0,0,"U00?s0000A00+:S00P-A00+!S00M|\l*w\s0000$" )
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()) }
}


BranchToThumbFromArm {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.stub",MATCHED_NAME()),CODE,8,8) }
  section { CONST32(0xe51ff004), /* ldr pc, [pc, #-4] */
            RELOCATED32(0x00000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-11),0,0,0,"S00A00+S00M|\l*w\s0000$" )
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()) }
}

/* add a new __*_from_thumb symbol that overrides all others */
BranchToArmFromThumbSymbol0 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) + 1 }
}

BranchToArmFromThumbPICSymbol0 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 1 }
}

/* add a new __*_from_arm symbol that overrides all others */
BranchToThumbFromArmSymbol0 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) + 1 }
}

BranchToThumbFromArmPICSymbol0 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 1 }
}

/* mark thumb and ARM code in the stub */
BranchToArmFromThumbSymbol1 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$t",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
}

BranchToArmFromThumbPICSymbol1 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$t",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
}

BranchToArmFromThumbSymbol2 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) + 4 }
}

BranchToArmFromThumbPICSymbol2 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 4 }
}

BranchToArmFromThumbPICSymbol3 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 12 }
}

BranchToThumbFromArmPICSymbol3 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 12 }
}

/* mark ARM code and data in the stub */
BranchToThumbFromArmSymbol1 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
}

BranchToThumbFromArmPICSymbol1 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
}

BranchToThumbFromArmSymbol2 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) + 4 }
}

BranchToThumbFromArmPICSymbol2 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_arm") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stubPIC",MATCHED_NAME())) + 12 }
}

/* if there is no short thumb stub, reuse a long one */
BranchToArmFromThumbFar {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe) == 0x46c04778) && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe +4) == 0xe51ff004) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.farstub",MATCHED_NAME()),CODE,4,12) }
  section { CONST16(0x4778),  /* bx      pc */
            CONST16(0x46c0),  /* nop ; (mov r8, r8) */
            CONST32(0xe51ff004), /* ldr pc, [pc, #-4] */
            RELOCATED32(0x00000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-13),0,0,0,"S00A00+S00M|\l*w\s0000$" ) /* the A00+ is because we pattern match on that during flowgraphing */
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()-1) }
}

/* add a new __*_from_thumb symbol that overrides all others */
BranchToArmFromThumbSymbol0Far {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.farstub",MATCHED_NAME())) + 1 }
}

/* mark thumb and ARM code in the stub */
BranchToArmFromThumbSymbol1Far {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$t",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
}

BranchToArmFromThumbSymbol2Far {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.farstub",MATCHED_NAME())) + 4 }
}


BranchToArmFromThumbSymbol3Far {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && SUBSECTION_EXISTS("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.farstub",MATCHED_NAME())) + 8 }
}

/* replace far thumb jump stubs with dummy code in case we reused a short stub. It's
 * better than just leaving them in, otherwise the relinked binary will contain 0-bytes
 * at those addresses, which will then be disassembled and fall through into whatever
 * comes after it.
 * We make this code end in a jump that branches to itself so that this won't
 * happen. It will then be killed as unreachable by Diablo.
 */
BranchToArmFromThumbFar {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe) == 0x46c04778) && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe +4) == 0xe51ff004) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.farstub",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.farstub",MATCHED_NAME()),CODE,4,12) }
  section {
            CONST32(0x46c046c0),  /* nop ; (mov r8, r8) */
            CONST32(0x46c046c0),  /* nop ; (mov r8, r8) */
            CONST16(0x46c0),  /* nop ; (mov r8, r8) */
            CONST16(0xe7fe)  /* b .-4) */
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()-1) }
}

/* the dummy stub contains thumb code */
BranchToArmFromThumbSymbol3 {
  trigger { LINKED_SYMBOL_EXISTS("__*_from_thumb") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe) == 0x46c04778) && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE() & 0xfffffffe +4) == 0xe51ff004) }
  action { ADD_SYMBOL_NEW("$t",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { ABS(MATCHED_SYMBOL_VALUE()-1) }
}

/* similar code as for __from_thumb/arm stubs above, but for __*_veneer symbols */
BranchToThumbFromArmVeneer {
  trigger { LINKED_SYMBOL_EXISTS("__*_veneer") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) == 0xe51ff004) && !SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SUBSECTION("Linker",".text",CONCAT(".text.stub",MATCHED_NAME()),CODE,4,8) }
  section { CONST32(0xe51ff004), /* ldr pc, [pc, #-4] */
            RELOCATED32(0x00000000, SUBSTRING(MATCHED_NAME(), 2, STRLEN(MATCHED_NAME())-9),0,0,0,"S00A00+S00M|\l*w\s0000$" )
  }
  /* don't use MATCHED_NAME(), as it may match another symbol with the same name */
  address { ABS(MATCHED_SYMBOL_VALUE()) }
}

/* add a new __*_veneer symbol that overrides all others */
BranchToThumbFromArmSymbol0 {
  trigger { LINKED_SYMBOL_EXISTS("__*_veneer") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
}

/* ARM code and data in the stub */
BranchToArmFromThumbSymbol1 {
  trigger { LINKED_SYMBOL_EXISTS("__*_veneer") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$a",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
}

BranchToArmFromThumbSymbol2 {
  trigger { LINKED_SYMBOL_EXISTS("__*_veneer") && SUBSECTION_EXISTS("Linker",CONCAT(".text.stub",MATCHED_NAME())) }
  action { ADD_SYMBOL_NEW("$d",0-1,SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(".text.stub",MATCHED_NAME())) + 4 }
}


/* place the DIABLO_POTENTIAL_THUMB_VENEER: symbols at the corresponding
 * _from_arm/_veneer veneers if they exist, otherwise at the real symbols
 */
BranchVeneerTargetFromArm {
  trigger { SYMBOL_EXISTS("DIABLO_POTENTIAL_THUMB_VENEER:*") && UNDEFINED_SYMBOL(MATCHED_NAME()) && !UNDEF_OR_NONEXIST_SYMBOL(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_from_arm")) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { DUPLICATE_ORIG(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_from_arm")) }
}

BranchVeneerTargetVeneer {
  trigger { SYMBOL_EXISTS("DIABLO_POTENTIAL_THUMB_VENEER:*") && UNDEFINED_SYMBOL(MATCHED_NAME()) && !UNDEF_OR_NONEXIST_SYMBOL(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_veneer")) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { DUPLICATE_ORIG(CONCAT(CONCAT("__",SUBSTRING(MATCHED_NAME(),30,0)),"_veneer")) }
}

/* this one uses DUPLICATE() instead of DUPLICATE_ORIG, because the DIABLO_POTENTIAL_THUMB_VENEER:*
 * symbols don't exist in the linked binary (and DUPLICATE_ORIG() looks in the (re)linked binary's
 * symbol table, while DUPLICATE() looks in the subsymbol table)
 */
BranchRealTargets {
  trigger { SYMBOL_EXISTS("DIABLO_POTENTIAL_THUMB_VENEER:*") && UNDEFINED_SYMBOL(MATCHED_NAME()) }
  action { ADD_SYMBOL_NEW(MATCHED_NAME(),12,SYMBOL_TYPE_FUNCTION) }
  symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),30,0)) }
}


/* slightly adapted gcc/binutils linker script serves as our layout script */
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = LINK_BASE_ADDRESS + SIZEOF_HEADERS;

  SEGMENT_START("Code");
  
  .interp: { *(.interp) }

  .note.ABI-tag : {} =0
  PUT_REMAINING_SECTIONS(NOTE_SECTION);

  /* after passing through diablo, all code sections will be merged */
  .merged_code : {} =0

  .init           :
  {
  } =0
  .plt            : { *(.plt) }
  .text           :
  {
  } =0
  .fini           :
  {
  } =0
  PUT_REMAINING_SECTIONS(CODE_SECTION);

  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  PUT_REMAINING_SECTIONS(RO_DATA_SECTION);

  SEGMENT_END("Code");

  SEGMENT_START("Data");
  .ARM.extab      : { KEEP (*(.ARM.extab)) }
  .ARM.exidx      : { KEEP (*(.ARM.exidx)) }
  
  .sdata2         : { *(.sdata2 .sdata2.* .gnu.linkonce.s2.*) }
  .sbss2          : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up. */
  . = ALIGN(0x8000) + (. & (0x8000 - 1)) ;
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  . = ALIGN(32 / 8);
  .eh_frame_hdr : { *(.eh_frame_hdr) }
  .  = ALIGN_START_OF_RELRO ;
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .gcc_except_table   : { *(.gcc_except_table) }
  /* make these the first writable sections, because you have to start
   * a new segment when encountering the first TLS section
   */
  .tdata          : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
  .tbss           : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
  .preinit_array     : { *(.preinit_array) }
  .init_array     : { *(SORT(.init_array.*)) *(.init_array) }
  .fini_array     : { *(.fini_array) *(SORT(.fini_array.*)) }
  .data.rel.ro    : { *(.data.rel.ro) }
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
  .jcr            : { KEEP (*(.jcr)) }
  .dynamic        : { *(.dynamic) }
  . = ALIGN_GOT_AFTER_RELRO ; 
  .got            : { *(.got) }
  . = ALIGN_DATA_AFTER_RELRO ; 
  .data           :
  {
  }
  .data1          : { *(.data1) }
  .sdata          :
  {
  }
  PUT_REMAINING_SECTIONS(DATA_SECTION);
  _edata = .;
  __bss_start = .;
  __bss_start__ = .;
  .sbss           :
  {
  }
  .bss            :
  {
   . = ALIGN(32 / 8);
  }
  PUT_REMAINING_SECTIONS(BSS_SECTION);

  
  . = ALIGN(32 / 8);
  _end = .;
  _bss_end__ = . ; __bss_end__ = . ; __end__ = . ;
  SEGMENT_END("Data");
  
  .ARM.attributes:{ *(.ARM.attributes) }
  .stack         0x80000 :
  {
    _stack = .;
    *(.stack)
  } 
}


