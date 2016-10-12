linker="I386 GNU LD"
fileformats="ELF"
architectures="I386"

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
  symbol  { UNDEFINED() }
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

MainThread {
   action  { ADD_SYMBOL("main_thread",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("main_thread") && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
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

AllEnd {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("_end")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") }
}


AllEnd2 {
   action  { ADD_SYMBOL("__end__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__end__")   && SECTION_EXISTS(".bss") }
   symbol  { END_OF_SECTION(".bss") }
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
   trigger { UNDEFINED_SYMBOL("end")    }
   symbol  { ABS(0) } 
}

_EndWithBss {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end") && SECTION_EXISTS(".bss")   }
   symbol  { END_OF_SECTION(".bss") } 
}

_EndWithSbss {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end") && SECTION_EXISTS(".sbss")  }
   symbol  { END_OF_SECTION(".sbss") } 
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
    action { ADD_SUBSECTION("Linker",".got",".got.plt",DATA,3,12) }
    trigger { SECTION_EXISTS(".got.plt") && (!(SECTION_EXISTS(".dynamic"))) }
    section { CONST32(0),CONST32(0),CONST32(0) }
    address { START_OF_SECTION(".got.plt") }
}

/* GotTableReservedEntries2 {
    action { ADD_SUBSECTION("Linker",".got",".got",DATA,1,4) }
    trigger { SECTION_EXISTS(".got") &&  SECTION_EXISTS(".got.plt") }
    section { CONST32(0) }
    address { START_OF_SECTION(".got") }
}  */

GotTableReservedEntries3 {
    action { ADD_SUBSECTION("Linker",".got",".got",DATA,4,12) }
    trigger { SECTION_EXISTS(".got") &&  (!SECTION_EXISTS(".got.plt")) }
    section { CONST32(0),CONST32(0),CONST32(0) }
    address { START_OF_SECTION(".got") }
}

TextEnd {
   action  { ADD_SYMBOL("_etext",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_etext") && SECTION_EXISTS(".text") }
   symbol  { END_OF_SECTION(".text") }
}




/* Some special sections:

   .preinit_array
   .init_array
   .fini_array
   .ctors
   .dtors

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

CtorVector {
  action { VECTORIZE(".ctors") }
  trigger { SUBSECTION_EXISTS("*",".ctors") }
}

DtorVector {
  action { VECTORIZE(".dtors") }
  trigger { SUBSECTION_EXISTS("*",".dtors") }
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
   trigger { UNDEFINED_SYMBOL("__preinit_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
    symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

PreInitStartFALLBACKNoInitArray {
 action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors")  }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

PreInitStartFALLBACKNoInitArrayNoCtorsVector {
 action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_start") }
 symbol  { START_OF_SECTION(".ctors") }
}

/* It always has to be defined nowadays

PreInitStartFALLBACKNoInitArrayNoTbss {
   action  { ADD_SYMBOL("__preinit_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__preinit_array_start") } 
   symbol  { ABS(0) }
}
*/
/* ========= End of preinit ========= */

/* Have preinit */

PreInitEnd {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.preinit_array") }
 symbol  { END_OF_SUBSECTION("Linker","VECTOR___.preinit_array") }
}

/* Have init_array, no preinit */

PreInitFALLBACK {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

PreInitEndFALLBACKNoInitArray {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

PreInitEndFALLBACKNoInitArrayNoCtorsVector {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") }
 symbol  { START_OF_SECTION(".ctors") }
}

/* it always has to be defined nowaydays

PreInitEndFALLBACKNoInitArrayNoTbss {
 action  { ADD_SYMBOL("__preinit_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__preinit_array_end") }
 symbol  { ABS(0) }
}
*/

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
   trigger { UNDEFINED_SYMBOL("__init_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

InitStartFALLBACKNoCtorsVector {
   action  { ADD_SYMBOL("__init_array_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__init_array_start") }
   symbol  { START_OF_SECTION(".ctors") }
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
 trigger { UNDEFINED_SYMBOL("__init_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

InitEndFALLBACKNoCtorsVector {
 action  { ADD_SYMBOL("__init_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__init_array_end") }
 symbol  { START_OF_SECTION(".ctors") }
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
 trigger { UNDEFINED_SYMBOL("__fini_array_start") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

FiniStartFALLBACKNoCtorsArray {
 action  { ADD_SYMBOL("__fini_array_start",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__fini_array_start") }
 symbol  { START_OF_SECTION(".ctors") }
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
 trigger { UNDEFINED_SYMBOL("__fini_array_end") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___.ctors") }
}

FiniEndFALLBACKNoCtorsArray {
 action  { ADD_SYMBOL("__fini_array_end",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("__fini_array_end") }
 symbol  { START_OF_SECTION(".ctors") }
}

/* ===================================*/

/* If we've vectorized .ctors and .dtors, redefine the symbols indicate their ends to be at an offset from the
 * end of the subsection as opposed to an offset from the start. We need this when adding in finalization routines
 * using .dtors for example.
 */
CtorsEnd {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && SUBSECTION_EXISTS("Linker","VECTOR___.ctors") }
  action  { ADD_SYMBOL_NEW("__CTOR_END__", 12, SYMBOL_TYPE_NOTYPE) }
  symbol  { END_OF_SUBSECTION("Linker", "VECTOR___.ctors") - OFFSET(0x4) }
}

DtorsEnd {
  trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && SUBSECTION_EXISTS("Linker","VECTOR___.dtors") }
  action  { ADD_SYMBOL_NEW("__DTOR_END__", 12, SYMBOL_TYPE_NOTYPE) }
  symbol  { END_OF_SUBSECTION("Linker", "VECTOR___.dtors") - OFFSET(0x4) }
}

/* ===================================*/

JcrVector {
  action { VECTORIZE(".jcr") }
  trigger { SUBSECTION_EXISTS("*",".jcr") }
}

JcrLIST {
   action  { ADD_SYMBOL("__JCR_LIST__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__JCR_LIST__") && SECTION_EXISTS(".jcr") }
   symbol  { START_OF_SECTION(".jcr") } 
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
   trigger { UNDEFINED_SYMBOL("_etls")   && SECTION_EXISTS(".tbss") }
   symbol  { END_OF_SECTION(".tbss") } 
}



/* Pistachio stuff */



MemoryDescriptorsSize{
   action  { ADD_SYMBOL("_memory_descriptors_size",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_memory_descriptors_size") }
   symbol  { ABS(((SYMBOL("kip") - SYMBOL("memory_descriptors")) & OFFSET(0x7ff)) / OFFSET(0x8)) }
}

MemoryDescriptorsOffset{
   action  { ADD_SYMBOL("_memory_descriptors_offset",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_memory_descriptors_offset") }
   symbol  { ABS(SYMBOL("memory_descriptors") - SYMBOL("kip")) }
}

EndBootMem{
   action  { ADD_SYMBOL("_end_bootmem",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end_bootmem") }
   symbol  { (END_OF_SECTION(".sets") + OFFSET(0x20000)) }
}

StartBootMem{
   action  { ADD_SYMBOL("_start_bootmem",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_start_bootmem") }
   symbol  { END_OF_SECTION(".sets") }
}

StartTextPhys{
   action  { ADD_SYMBOL("_start_text_phys",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_start_text_phys") }
   symbol  { ABS(0x100200) }
}

EndBootMemPhys{
   action  { ADD_SYMBOL("_end_bootmem_phys",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end_bootmem_phys") }
   symbol  { (END_OF_SECTION(".sets") + OFFSET(0x20000)) - OFFSET(0xf0000000) }
}

BssStart{
   action  { ADD_SYMBOL("_bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_bss_start") }
   symbol  { START_OF_SUBSECTION("src/generic/kmemory.o",".bss") }
}

BssEnd{
   action  { ADD_SYMBOL("_bss_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_bss_end") }
   symbol  { END_OF_SECTION(".data") }
}

InitEnd {
   action  { ADD_SYMBOL("_end_init",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_end_init") }
   symbol  { END_OF_SECTION(".init") }
}

/* Ctors */

Ctors2 {
    action { ADD_SUBSECTION("Linker",".ctors",".ctors.2.padding",DATA,2,8) }
    trigger { UNDEFINED_SYMBOL("__ctors_GLOBAL__") }
    section { CONST32(0),CONST32(0) }
    address { SYMBOL("__ctors_GLOBAL__") - ABS(0x8) }
}

Ctor2Vector {
  action { VECTORIZE_BY_NAME(".ctors.2*") }
  trigger { SUBSECTION_EXISTS("*",".ctors.2*") }
}

__ctors_GLOBAL__ {
	action  { ADD_SYMBOL("__ctors_GLOBAL__",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEFINED_SYMBOL("__ctors_GLOBAL__") }
	symbol  { END_OF_SUBSECTION("Linker","VECTOR___.ctors.2*") }
}

Ctors3 {
    action { ADD_SUBSECTION("Linker",".ctors",".ctors.3.padding",DATA,2,8) }
    trigger { UNDEFINED_SYMBOL("__ctors_NODE__") }
    section { CONST32(0),CONST32(0) }
    address { SYMBOL("__ctors_NODE__") - ABS(0x8) }
}

Ctor3Vector {
  action { VECTORIZE_BY_NAME(".ctors.3*") }
  trigger { SUBSECTION_EXISTS("*",".ctors.3*") }
}

__ctors_NODE__ {
	action  { ADD_SYMBOL("__ctors_NODE__",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEFINED_SYMBOL("__ctors_NODE__") }
	symbol  { END_OF_SUBSECTION("Linker","VECTOR___.ctors.3*") }
}

Ctors {
    action { ADD_SUBSECTION("Linker",".ctors",".mdb_funcs.padding",DATA,0,8) }
    trigger { UNDEFINED_SYMBOL("__ctors_CPU__") }
	section { CONST32(0),CONST32(0) }
    address { SYMBOL("__ctors_CPU__") - ABS(0x8) }
}

Ctor3Vector {
  action { VECTORIZE_BY_NAME(".mdb_funcs*") }
  trigger { SUBSECTION_EXISTS("*",".mdb_funcs*") }
}

__ctors_CPU__ {
	action  { ADD_SYMBOL("__ctors_CPU__",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEFINED_SYMBOL("__ctors_CPU__") }
	symbol  { END_OF_SUBSECTION("Linker","VECTOR___.mdb_funcs*") }
}
/*
Ctors2 {
    action { ADD_SUBSECTION("Linker",".ctors",".ctors",DATA,2,8) }
    trigger { SECTION_EXISTS(".init") }
    section { CONST32(0),CONST32(0) }
    address { END_OF_SECTION(".init") - ABS(0x8) }
}
*/
CtorVector {
  action { VECTORIZE_BY_NAME(".ctors*") }
  trigger { SUBSECTION_EXISTS("*",".ctors*") }
}

/* Dynamic linking stuff */

/* If the object for which we are emulating the link holds an .interp section
 * we need to recreate this section. The interp section lists the dynamic
 * loader that is to be used to load this dynamically linked program. In all
 * currently supported dynamically linked programs it will hold the string
 * /lib/ld-linux.so.2. Mind that we must explicitly add the trailing zero when
 * using STRING filler */

Interp {
	trigger { SECTION_EXISTS(".interp") && ! SUBSECTION_EXISTS("Linker",".interp")}
	action  { ADD_SUBSECTION("Linker",".interp",".interp", RODATA,4, 19) }
	section {
		  STRING("/lib/ld-linux.so.2"),
		  CONST8(0x00)
		}
	address { START_OF_SECTION(".interp") }
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

/* Create a helper symbol to mark the start of the .init section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

InitStart  {
   action  { ADD_SYMBOL("__init_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__init_start") && SECTION_EXISTS(".init") }
   symbol  { START_OF_SECTION(".init") } 
}

/* Create a helper symbol to mark the start of the .fini section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

RelPltStart  {
   action  { ADD_SYMBOL("__rel_plt_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__rel_plt_start") && SECTION_EXISTS(".rel.plt") }
   symbol  { START_OF_SECTION(".rel.plt") } 
}

RelIPltStart  {
   action  { ADD_SYMBOL("__rel_iplt_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__rel_iplt_start") && SECTION_EXISTS(".rel.iplt") }
   symbol  { START_OF_SECTION(".rel.iplt") }
}

RelIPltStartFallback {
   action  { ADD_SYMBOL("__rel_iplt_start",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__rel_iplt_start") && !SECTION_EXISTS(".rel.iplt") && SUBSECTION_EXISTS("Linker","VECTOR___.init") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init") }
}

RelIPltEnd  {
   action  { ADD_SYMBOL("__rel_iplt_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__rel_iplt_end") && SECTION_EXISTS(".rel.iplt") }
   symbol  { EBD_OF_SECTION(".rel.iplt") }
}

RelIPltEndFallback {
   action  { ADD_SYMBOL("__rel_iplt_end",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("__rel_iplt_end") && !SECTION_EXISTS(".rel.iplt") && SUBSECTION_EXISTS("Linker","VECTOR___.init") }
   symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init") }
}

/* At the start of the .got section, we need: 
 * - The address of the dynamic section
 * - Two placeholders for addresses (zero initialized).
 * 
 * Not really sure why they are all used, but the addresses will be filled in
 * by the dynamic loader to point to the real dynamic resolver  */

NullGot {
	trigger {  SECTION_EXISTS(".got.plt") && SECTION_EXISTS(".dynamic") && !SUBSECTION_EXISTS("Linker","NULLGOT") }
	action { ADD_SUBSECTION("Linker",".got.plt","NULLGOT", DATA, 4, 12) }
	section {
	START_OF_SECTION(".dynamic"),
	CONST32(0x0),                   /* Filled by dynamic linker, used by resolver (INITPLT) */
	CONST32(0x0)                    /* Filled by dynamic linker, used by resolver (INITPLT) */
	} 
	address { START_OF_SECTION(".got.plt") }
}


NullGotSym1Exe  {
   action  { ADD_SYMBOL("NULLGOT:ADDR1",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("NULLGOT:ADDR1") && !GENERATE_PIC() && SECTION_EXISTS(".got.plt") && SECTION_EXISTS(".dynamic") }
   symbol  { START_OF_SUBSECTION("Linker", "NULLGOT") + OFFSET(0x4) }
}

NullGotSym2Exe  {
   action  { ADD_SYMBOL("NULLGOT:ADDR2",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("NULLGOT:ADDR2") && !GENERATE_PIC() && SECTION_EXISTS(".got.plt") && SECTION_EXISTS(".dynamic") }
   symbol  { START_OF_SUBSECTION("Linker", "NULLGOT") + OFFSET(0x8) }
}

NullGotSym1Lib  {
   action  { ADD_SYMBOL("NULLGOT:ADDR1",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("NULLGOT:ADDR1") && GENERATE_PIC() && SECTION_EXISTS(".got.plt") && SECTION_EXISTS(".dynamic") }
   symbol  { ABS(0x4) }
}

NullGotSym2Lib  {
   action  { ADD_SYMBOL("NULLGOT:ADDR2",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("NULLGOT:ADDR2") && GENERATE_PIC() && SECTION_EXISTS(".got.plt") && SECTION_EXISTS(".dynamic") }
   symbol  { ABS(0x8) }
}


/* The plt starts with a stub that calls the dynamic resolver. It reads the
 * previously added NULLGOT section */

PltExe {
	trigger { !GENERATE_PIC() && SECTION_EXISTS(".plt") && !SUBSECTION_EXISTS("Linker","INITPLT") }
	action { ADD_SUBSECTION("Linker",".plt","INITPLT", RODATA,4, 16) }
	section {
	CONST16(0x35ff),
	RELOCATED32(0x0,"NULLGOT:ADDR1",0,0,0,"S00A00+\l*w\s0000$"),
	CONST16(0x25ff),
	RELOCATED32(0x0,"NULLGOT:ADDR2",0,0,0,"S00A00+\l*w\s0000$"),
	CONST32(0x0)

	} 
	address { START_OF_SECTION(".plt") }
}

PltLib {
	trigger { GENERATE_PIC() && SECTION_EXISTS(".plt") && !SUBSECTION_EXISTS("Linker","INITPLT") }
	action { ADD_SUBSECTION("Linker",".plt","INITPLT", RODATA,4, 16) }
	section {
	CONST16(0xb3ff),
	RELOCATED32(0x0,"NULLGOT:ADDR1",0,0,0,"S00A00+\l*w\s0000$"),
	CONST16(0xa3ff),
	RELOCATED32(0x0,"NULLGOT:ADDR2",0,0,0,"S00A00+\l*w\s0000$"),
	CONST32(0x0)
	}
	address { START_OF_SECTION(".plt") }
}

PltSym  {
   action  { ADD_SYMBOL("INITPLT",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEF_OR_NONEXIST_SYMBOL("INITPLT") && SECTION_EXISTS(".plt") }
   symbol  { START_OF_SUBSECTION("Linker", "INITPLT") }
}

/* A symbol table needs a leading '\0' in the string table as name of the null
 * symbol. We add it here. */

NullDynStr {
	trigger { SECTION_EXISTS(".dynstr") && !SUBSECTION_EXISTS("Linker", "NULLDYNSTR") }
	action { ADD_SUBSECTION("Linker",".dynstr","NULLDYNSTR", RODATA,1, 1) }
	section {
	CONST8(0x0)
	} 
	address { START_OF_SECTION(".dynstr") }
}

/* A symbol table needs to start with a null symbol. We add it here. */

NullDynSym {
	trigger {  SECTION_EXISTS(".dynsym") && !SUBSECTION_EXISTS("Linker", "NULLDYNSYM") }
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
	trigger {  SECTION_EXISTS(".dynamic") && !SUBSECTION_EXISTS("Linker", "DYNAMIC_DT_NULL") }
	action { ADD_SUBSECTION("Linker",".dynamic","DYNAMIC_DT_NULL", DATA,4, 8) }
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
		START_OF_SECTION(".got.plt")
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
  trigger { SECTION_EXISTS(".gnu.version") && !SUBSECTION_EXISTS("Linker",".gnu.version.NULLSYM")}
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
  section { CONST16(0x0) }/* DIRTY HACK! This entry is supposed to tell the dynamic linker which version of the shared library is to be used. As it is too hard to determine whether the entry we need already exists (and if so which index it has) we just use the value 0 which means it's a local symbol. This does not make a difference to our ends as the symbols we add are unversioned anyway */
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
i	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
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
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) == 3) }
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



/* The address for an undefined/external dynamic code symbol is NULL,
 * except if its address is taken in a non-dynamic library/pie binary.
 * See the code for R_I386_32 relocation in diabloelf_i386.c
 */
DynSymUndefinedNoAddrTaken {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !LINKED_SYMBOL_EXISTS(CONCAT("DYNSYMPLTADDR:", SUBSTRING(MATCHED_NAME(), 13, 0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, MATCHED_NAME(), 0, 0, 0, "S00*s0000\l*w\s0000$"),  /* force 0, but in a way so that we get a fatal if the original binary did not have 0 */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

/* the address of an undefined external function symbol is the address of the PLT entry if its
 * address is taken
 */
DynSymUndefinedAddrTaken {
	trigger { !LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(), 13, 0), 0, 0, 0, "S00A00+\l*w\s0000$"),  /* jump slot address entry */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

DynSymUndefinedNoAddrTakenAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !LINKED_SYMBOL_EXISTS(CONCAT("DYNSYMPLTADDR:", SUBSTRING(MATCHED_NAME(), 13, 0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, MATCHED_NAME(), 0, 0, 0, "S00*s0000\l*w\s0000$"),  /* force 0, but in a way so that we get a fatal if the original binary did not have 0 */
            CONST32(0x0) /* The symbol size */,
            CONST32(0x12) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

DynSymUndefinedAddrTakenAfterLinkerEmulation {
  trigger { LINKED_SYMBOL_EXISTS("$after_linker_emulation") && LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) && UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
  section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
            RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(), 13, 0), 0, 0, 0, "S00A00+\l*w\s0000$"),  /* jump slot address entry */
            CONST32(0x0) /* The symbol size */,
            CONST32(0x12) /* The symbol type & bind */
          }
  address { MATCHED_SYMBOL_VALUE()  }
}

/* now the defined symbols: in this case, the dynamic symbol must point to the address of
 * symbol in this linked binary/library. If a PLT entry is generated for the symbol (e.g.
 * because it is exported), we will place the symbol in the newly linked binary/library
 * at the start of this PLT entry in order to ensure that internal calls to this symbol
 * go via the PLT entry.
 *
 * To make sure that these dynamic symbols point to the actual code and not to the PLT
 * entry (since the dynamic linker uses the dynsym address to resolve the PLT entry,
 * that would result in an endless loop), we make a duplicate here of the original
 * dynsym (ORIG:symname) at its original address and refer to that ORIG:symname address
 * from within our dynsym section
 */
DynSymOrig {
        trigger {
                  LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)))
                }
        action  { ADD_SYMBOL_NEW(CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(),13,0)), 12, MATCHED_SYMBOL_FLAGS()) }
        symbol  { DUPLICATE(SUBSTRING(MATCHED_NAME(),13,0)) }
}

DynSymDefined {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && !SUBSECTION_EXISTS("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0))) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 16) }
	section { RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */
	          RELOCATED32(0x0, CONCAT("ORIG:",SUBSTRING(MATCHED_NAME(), 13, 0)), 0, 0, 0, "S00A00+\l*w\s0000$"),  /* jump slot address entry */
	          CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
		  CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff) /* The symbol type & bind */
	        }
	address { MATCHED_SYMBOL_VALUE()  }
}

/* In case we have a (dynamic) JUMP_SLOT relocation to a dynamic symbol, we
 * need to add a PLT entry. */

PltSectionExe {
	trigger { !GENERATE_PIC() &&
                  LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0)))
	        }
	action  { ADD_SUBSECTION("Linker", ".plt", CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 16) }
	section { 
		  /* jmp *  ... */
		  CONST16(0x25ff), 
		  /* ... loaded from our dynamic got entry */    
		  RELOCATED32(0x0, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0,"S00A00+\l*w\s0000$"),
		  /* push ... */
		  CONST8(0x68), 
		  /* ... the offset of our relocation in rel plt */
		  RELOCATED32(0x0, CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "__rel_plt_start", 0,"S00S01-\l*w\s0000$"),
		  /* jmp .. .*/ 
		  CONST8(0xe9), 
		  /* ... to the resolver/relocator at the start of the plt */
		  RELOCATED32(0x0, "INITPLT", 0, 0, 0,"S00A00+P-s0004-\l*w\s0000$")
	}
       address { READ_LINKED_ADDR32(READ_LINKED_ADDR32(SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0))))) - 6 }
}

PltSectionLib {
	trigger { GENERATE_PIC() &&
                  LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0)))
	        }
	action  { ADD_SUBSECTION("Linker", ".plt", CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 16) }
	section {
		  /* jmp *  ... */
		  CONST16(0xa3ff),
		  /* ... loaded from our dynamic got entry */
		  RELOCATED32(0x0, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "_GLOBAL_OFFSET_TABLE_", 0,"S00A00+S01-\l*w\s0000$"),
		  /* push ... */
		  CONST8(0x68),
		  /* ... the offset of our relocation in rel plt */
		  RELOCATED32(0x0, CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "__rel_plt_start", 0,"S00S01-\l*w\s0000$"),
		  /* jmp .. .*/
		  CONST8(0xe9),
		  /* ... to the resolver/relocator at the start of the plt */
		  RELOCATED32(0x0, "INITPLT", 0, 0, 0,"S00A00+P-s0004-\l*w\s0000$")
	}
       address { READ_LINKED_ADDR32(READ_LINKED_ADDR32(SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0))))) - 6 }
}

PltSymbolAliasMainSym {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) &&
		  UNDEF_OR_NONEXIST_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
	action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSymbol {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") &&
		  LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SYMBOL_NEW(CONCAT("PLTELEMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltSymbolDynLinkEntry {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SYMBOL_NEW(CONCAT("PLT2:", SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) + OFFSET(0x6) }
}

DynSymGot {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action  { ADD_SUBSECTION("Linker", ".got.plt", CONCAT("DYNAMIC_GOTELEM:",SUBSTRING(MATCHED_NAME(), 10, 0)), DATA, 4, 4) }
	section { RELOCATED32(0x0, CONCAT("PLT2:", SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0,"S00A00+\l*w\s0000$")  }
	address { READ_LINKED_ADDR32(SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0)))) }
}

DynSymGotSymbol {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
        action { ADD_SYMBOL_NEW(CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_GOTELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

PltRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.plt", CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 8) }
	section {
		RELOCATED32(0x0, CONCAT("DYNAMIC_GOT_ENTRY:",SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0,"S00A00+\l*w\s0000$"),
		RELOCATED32(0x7, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 10, 0))) }
}

PltRelSymbol {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("JUMP_SLOT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SYMBOL_NEW(CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* The case where we allocate some data */

/* I386_COPY relocs exist for static data that exists in a dynamic library and that must be
 * copied into the executable. There may not be a reference from inside the executable to
 * this data (all uses may be in the dynamic library), so the symbol may not exist rather
 * than be undefined -> "undefine" it so that we create data for it.
 */

I386CopyRelNonExisting {
	trigger { LINKED_SYMBOL_EXISTS("I386_COPY:*") && !SYMBOL_EXISTS(SUBSTRING(MATCHED_NAME(),10,0))}
	action { ADD_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0), SYMBOL_TYPE_NOTYPE) }
	symbol { UNDEFINED() }
}


DynSymData {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff000f) != 2) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),13,0)) }
	action  { ADD_SUBSECTION("Linker", ".bss", CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(), 13, 0)), BSS, 4, READ_LINKED_ADDR32(MATCHED_SYMBOL_VALUE()+8)) }
	section { RELOCATED32(0x0, CONCAT("I386_COPY:", SUBSTRING(MATCHED_NAME(),13,0)), 0, 0,0, "S00\*\s0000$")  }
	address { READ_LINKED_ADDR32(MATCHED_SYMBOL_VALUE()+4) }
}

/* add ANYDYNSYMSYM:* alias used in rules that apply to both weak and regular dynamic symbols (both .bss and .data/.rodata -> no undefined check) */
/*
DynSymNoJumpSlotSymbol {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) }
	action { ADD_SYMBOL_NEW(CONCAT("ANY",MATCHED_NAME()), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}
*/

DynSymDataSymbol {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0xffff000f) != 2) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
        action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* weak symbols that are not explicitly defined should stay undefined so they
 * can dynamically bind against a symbol in a dynamic library if that exists
 * at run time, except in case of a copy relocation, in which case it must
 * be bound to the copied data
 */
WeakDynSymDataSymbol {
	trigger { LINKED_SYMBOL_EXISTS("WEAKDYNSYMSYM:*") && ((READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12) & 0x0000000f) != 2) && LINKED_SYMBOL_EXISTS(CONCAT("I386_COPY:",SUBSTRING(MATCHED_NAME(),14,0))) && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
        action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),10,0), 12, SYMBOL_TYPE_NOTYPE) }
        symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* globdat */
GlobDatRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("GLOB_DAT:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),9,0)), 0, 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(0x6, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),9,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("GLOB_DAT:", SUBSTRING(MATCHED_NAME(), 9, 0))) }
}

/* tls_tpoff */
TlsTpOffRel {
	trigger { LINKED_SYMBOL_EXISTS("ANYDYNSYMSYM:*") && LINKED_SYMBOL_EXISTS(CONCAT("TLS_TPOFF:", SUBSTRING(MATCHED_NAME(), 13, 0))) }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
    RELOCATED32(0x0, CONCAT("GOT:",SUBSTRING(MATCHED_NAME(),10,0)), 0, 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(0xe, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { SYMBOL(CONCAT("TLS_TPOFF:", SUBSTRING(MATCHED_NAME(), 10, 0))) }
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


/* I386 COPY REL */

I386CopyRel {
	trigger { LINKED_SYMBOL_EXISTS("I386_COPY:*") }
	action { ADD_SUBSECTION("Linker", ".rel.dyn", CONCAT("DYNREL:",MATCHED_NAME()), RODATA, 4, 8) }
	section {
		RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),10,0), 0, 0, 0,"S00A00+\l*w\s0000$"),
		RELOCATED32(0x5, CONCAT("ANYDYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$")}
	address { MATCHED_SYMBOL_VALUE() }
     }

I386CopySymbol {
	trigger { LINKED_SYMBOL_EXISTS("I386_COPY:*") }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNREL:",MATCHED_NAME())) }
	     }

I386CopyAliases {
        trigger { LINKED_SYMBOL_EXISTS("DIABLOMAPSYM:*") }
        action { ADD_SYMBOL_NEW(STRINGTOKEN(MATCHED_NAME(),":",1), 12, MATCHED_SYMBOL_FLAGS()) }
        symbol { DUPLICATE(STRINGTOKEN(MATCHED_NAME(),":",2)) }
}

ElfResizeHashSection {
  trigger { SECTION_EXISTS(".hash") }
  action { ELF_RESIZE_HASH_SECTION() }
}

/* End of dynamic linking */

/* Sets */

SetVector {
  action { VECTORIZE_BY_NAME("set*") }
  trigger { SUBSECTION_EXISTS("*","set*") }
}

_start_sets {
action  { ADD_SYMBOL("_start_sets",SYMBOL_TYPE_NOTYPE) }
trigger { UNDEFINED_SYMBOL("_start_sets") }
 symbol  { START_OF_SUBSECTION("Linker","VECTOR___set*") }
}
_end_sets {
  action  { ADD_SYMBOL("_end_sets",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_end_sets") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR___set*") }
}

/* slightly adapted gcc/binutils linker script serves as our layout script */
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = 0x08048000 + SIZEOF_HEADERS;

  .note   : {} =0
  .note.ABI-tag   : {} =0
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
  .rel.plt        :
    {
      *(.rel.plt)
      PROVIDE (__rel_iplt_start = .);
      *(.rel.iplt)
      PROVIDE (__rel_iplt_end = .);
    }
  .init           :
  {
    KEEP (*(.init))
  } =0x90909090
  .plt            : { *(.plt) }
  .merged_code	  : {} =0x90909090
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

  . = ALIGN(0x1000) + (. & 0xfff);

  .eh_frame_hdr : { *(.eh_frame_hdr) }

  .  = ALIGN_START_OF_RELRO ;
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .gcc_except_table   : { *(.gcc_except_table .gcc_except_table.*) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */

  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
  PROVIDE (__preinit_array_start = .);
  .preinit_array     : { *(.preinit_array) }
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array     : { *(.init_array) }
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array     : { *(.fini_array) }
  PROVIDE (__fini_array_end = .);
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
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }
  .dtors          :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }
  .jcr            : { KEEP (*(.jcr)) .iplt}
  .data.rel.ro : { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro* .gnu.linkonce.d.rel.ro.*) }
  . = ALIGN_GOT_AFTER_RELRO ;
  .got            : { *(.got) *(.igot) }
  . = ALIGN_DATA_AFTER_RELRO ;
  .got.plt        : { *(.got.plt) *(.got.iplt) }
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  }
  .data1          : { *(.data1) }
  .dynamic        : { *(.dynamic) }
  PUT_REMAINING_SECTIONS(DATA_SECTION);
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
   . = ALIGN(32 / 8);
  }
  . = ALIGN(32 / 8);
  PUT_REMAINING_SECTIONS(BSS_SECTION);
  _end = .;
  PROVIDE (end = .);

  .comment        : { *(.comment) }

}
