linker="ppc GNU LD"
fileformats="ELF"
architectures="ppc"

/* NULLGOT if static {{{*/
NullGotStatic {
  action  { ADD_SUBSECTION("Linker",".got","NULLGOT",DATA,4,16  ) }
  trigger { LINKED_SYMBOL_EXISTS("_GLOBAL_OFFSET_TABLE_") && SECTION_EXISTS(".got") && !SECTION_EXISTS(".dynamic") }
  section {
    CONST32(0x4e800021),
    CONST32(0x0), /* Reserved entry */
    CONST32(0x0), /* Reserved entry */
    CONST32(0x0)} /* Reserved entry */
  address  { SYMBOL("_GLOBAL_OFFSET_TABLE_") - 4 }
}
/*}}}*/

/* Vectorize sections {{{*/

EhFrameVector {
  action {  VECTORIZE(".eh_frame") }
  trigger { SUBSECTION_EXISTS("*",".eh_frame") }
}

EhFrameVector2 {
  action {  VECTORIZE(".gcc_except_table") }
  trigger { SUBSECTION_EXISTS("*",".gcc_except_table") }
}

CtorVector {
  action { VECTORIZE(".ctors") }
  trigger { SUBSECTION_EXISTS("*",".ctors") }
}

DtorVector {
  action { VECTORIZE(".dtors") }
  trigger { SUBSECTION_EXISTS("*",".dtors") }
}

AtExitVector {
  action { VECTORIZE("__libc_atexit") }
  trigger { SUBSECTION_EXISTS("*","__libc_atexit") }
}

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
/*}}}*/

/* Start/End Symbols {{{*/

StartOfAtExit {
  action  { ADD_SYMBOL("__start___libc_atexit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR_____libc_atexit") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR_____libc_atexit") }
}

StopOfAtExit {
  action  { ADD_SYMBOL("__stop___libc_atexit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR_____libc_atexit") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR_____libc_atexit") }
}

StartSubfreeres {
  action  { ADD_SYMBOL("__start___libc_subfreeres",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_subfreeres") }
  symbol  { START_OF_SECTION("__libc_subfreeres") }
}

StopSubfreeres {
  action  { ADD_SYMBOL("__stop___libc_subfreeres",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_subfreeres") }
  symbol  { END_OF_SECTION("__libc_subfreeres") }
}

StartFreeres_ptrs {
  action  { ADD_SYMBOL("__start___libc_freeres_ptrs",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_freeres_ptrs") }
  symbol  { START_OF_SECTION("__libc_freeres_ptrs") }
}

StopFreeres_ptrs {
  action  { ADD_SYMBOL("__stop___libc_freeres_ptrs",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_freeres_ptrs") }
  symbol  { END_OF_SECTION("__libc_freeres_ptrs") }
}

StartFreeres_fn {
  action  { ADD_SYMBOL("__start___libc_freeres_fn",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_freeres_fn") }
  symbol  { START_OF_SECTION("__libc_freeres_fn") }
}

StopFreeres_fn {
  action  { ADD_SYMBOL("__stop___libc_freeres_fn",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS("__libc_freeres_fn") }
  symbol  { END_OF_SECTION("__libc_freeres_fn") }
}

bssstart {
  action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".sbss") }
  symbol  { START_OF_SECTION(".sbss") } 
}

bssstart_fallback_1 {
  action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("__bss_start") &&  SECTION_EXISTS(".plt") }
  symbol  { START_OF_SECTION(".plt") } 
}

bssstart_fallback_2 {
  action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("__bss_start") && SECTION_EXISTS(".bss") }
  symbol  { START_OF_SECTION(".bss") } 
}

datastart {
  action  { ADD_SYMBOL("data_start",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".data") }
  symbol  { START_OF_SECTION(".data") } 
}

underscore_datastart {
  action  { ADD_SYMBOL("__data_start",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("__data_start") && SECTION_EXISTS(".data")}
  symbol  { START_OF_SECTION(".data") } 
}

underscore_End {
  action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
  trigger { /*UNDEFINED_SYMBOL("__bss_start") && */ SECTION_EXISTS(".bss") }
  symbol  { END_OF_SECTION(".bss") } 
}

/*underscore_End_fallback_1 {
  action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_end") &&  SECTION_EXISTS(".plt") }
  symbol  { START_OF_SECTION(".plt") } 
}

underscore_End_fallback_2 {
  action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_end") && SECTION_EXISTS(".bss") }
  symbol  { START_OF_SECTION(".bss") } 
}*/

/*}}}*/

/* Dynamic Link support {{{*/

/* Specific and support SYMBOLs {{{*/

/* Add a _DYNAMIC symbol used by the dynamic linker */
DynamicSymbol {
  action { ADD_SYMBOL("_DYNAMIC",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".dynamic") && UNDEFINED_SYMBOL("_DYNAMIC") }
  symbol { START_OF_SECTION(".dynamic") }
}

/*}}}*/

/* Add sections needed for dynamic linking {{{*/

/* If the object for which we are emulating the link holds an .interp section
 * we need to recreate this section. The interp section lists the dynamic
 * loader that is to be used to load this dynamically linked program. In all
 * currently supported dynamically linked programs it will hold the string
 * /lib/ld.so.1. Mind that we must explicitly add the trailing zero when
 * using STRING filler */
Interp {
        trigger { SECTION_EXISTS(".interp") }
        action  { ADD_SUBSECTION("Linker",".interp",".interp", RODATA,4, 13) }
        section {
                  STRING("/lib/ld.so.1"),
                  CONST8(0x00)
                }
        address { START_OF_SECTION(".interp") }
}

/* Initialize the GOT table */
NullGotDynamic {
  action  { ADD_SUBSECTION("Linker",".got","NULLGOT",DATA,4,16  ) }
  trigger { LINKED_SYMBOL_EXISTS("_GLOBAL_OFFSET_TABLE_") && SECTION_EXISTS(".got") && SECTION_EXISTS(".dynamic") }
  section {
    CONST32(0x4e800021),
    START_OF_SECTION(".dynamic"),
    CONST32(0x0), /* Reserved entry */
    CONST32(0x0)} /* Reserved entry */
  address  { SYMBOL("_GLOBAL_OFFSET_TABLE_") - 4 }
}

/* GOT table symbol {{{*/
GotTableStartSym {
  trigger { SUBSECTION_EXISTS("Linker","NULLGOT") && UNDEFINED_SYMBOL("_GLOBAL_OFFSET_TABLE_") && SECTION_EXISTS(".got") }
  action  { ADD_SYMBOL("_GLOBAL_OFFSET_TABLE_",SYMBOL_TYPE_NOTYPE) }
  symbol  { START_OF_SUBSECTION_OFFSET("Linker","NULLGOT",4) }
}

/*}}}*/



/* The plt starts with 72 empty bytes reserved for use by the
 * dynamic linker*/
Plt {
	trigger {  SECTION_EXISTS(".plt") }
	action { ADD_SUBSECTION("Linker",".plt","INITPLT", DATA, 4, 72) }
	section {
    CONST32(0x0),CONST32(0x0),CONST32(0x0),CONST32(0x0),
    CONST32(0x0),CONST32(0x0),CONST32(0x0),CONST32(0x0),
    CONST32(0x0),CONST32(0x0),CONST32(0x0),CONST32(0x0),
    CONST32(0x0),CONST32(0x0),CONST32(0x0),CONST32(0x0),
    CONST32(0x0),CONST32(0x0)
	} 
	address { START_OF_SECTION(".plt") }
}

PltSym  {
  action  { ADD_SYMBOL("INITPLT",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".plt") }
  symbol  { START_OF_SUBSECTION("Linker", "INITPLT") }
}


/* don't use END_OF_SECTION(".text"), because it has to come after
   the .fini section. Don't use END_OF_SECTION(".fini") because if
   the .fini section is removed, you will get a wrong result.
   So we use the start of one of the sections coming after fini
   (and hope that at least one of them exists)
 */
TextEnd {
  action  { ADD_SYMBOL("etext",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("etext") && SECTION_EXISTS(".rodata") }
  symbol  { START_OF_SECTION(".rodata") }
}

TextEnd {
  action  { ADD_SYMBOL("etext",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("etext") && SECTION_EXISTS(".sdata2") }
  symbol  { START_OF_SECTION(".sdata2") }
}

TextEnd {
  action  { ADD_SYMBOL("etext",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("etext") && SECTION_EXISTS(".sbss2") }
  symbol  { START_OF_SECTION(".sbss2") }
}

TextEnd {
  action  { ADD_SYMBOL("etext",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("etext") && SECTION_EXISTS(".eh_frame_hdr") }
  symbol  { START_OF_SECTION(".eh_frame_hdr") }
}


/* A symbol table needs a leading '\0' in the string table as name of the null
 * symbol. We add it here. */
NullDynStr {
	trigger { SECTION_EXISTS(".dynstr") }
	action { ADD_SUBSECTION("Linker",".dynstr","NULLDYNSTR", RODATA,1, 1) }
	section {
	CONST8(0x0)
	} 
	address { START_OF_SECTION(".dynstr") }
}

/* A symbol table needs to start with a null symbol. We add it here. */
NullDynSym {
	trigger {  SECTION_EXISTS(".dynsym") }
	action { ADD_SUBSECTION("Linker",".dynsym","NULLDYNSYM", RODATA,4, 16) }
	section {
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0)
	} 
	address { START_OF_SECTION(".dynsym") }
}


/*}}}*/

/*}}}*/


/* Dynamic Linking {{{ */


/* Helper Symbols {{{ */

/* Create a helper symbol to mark the start of the .fini section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

FiniStart  {
   action  { ADD_SYMBOL("__fini_start",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".fini") }
   symbol  { START_OF_SECTION(".fini") }
}

/* Create a helper symbol to mark the start of the .dynstr section. This symbol
 * will be used in RELOCATED32 statements, which only can use symbols (not
 * sections) */

DynStrStart {
        action  { ADD_SYMBOL("$dynstr_start",SYMBOL_TYPE_NOTYPE) }
        trigger {  SECTION_EXISTS(".dynstr") }
        symbol  { START_OF_SECTION(".dynstr") }
}


DynSymStart {
        action  { ADD_SYMBOL("$dynsym_start",SYMBOL_TYPE_NOTYPE) }
        trigger {  SECTION_EXISTS(".dynsym") }
        symbol  { START_OF_SECTION(".dynsym") }
}

GotStart {
        action  { ADD_SYMBOL("$got_start",SYMBOL_TYPE_NOTYPE) }
        trigger {  SECTION_EXISTS(".got") }
        symbol  { START_OF_SECTION(".got") }
}

PltStart {
        action  { ADD_SYMBOL("$plt_start",SYMBOL_TYPE_NOTYPE) }
        trigger {  SECTION_EXISTS(".plt") }
        symbol  { START_OF_SECTION(".plt") }
}

RelaDynStart {
  action  { ADD_SYMBOL("$reladyn_start",SYMBOL_TYPE_NOTYPE) }
  trigger {  SECTION_EXISTS(".rela.dyn") }
  symbol  { START_OF_SECTION(".rela.dyn") }
}

RelaPltStart {
  action  { ADD_SYMBOL("$relaplt_start",SYMBOL_TYPE_NOTYPE) }
  trigger {  SECTION_EXISTS(".rela.plt") }
  symbol  { START_OF_SECTION(".rela.plt") }
}

/* }}} */


/* Each dynamic entry found in the object for which we are emulating the link
 * gets marked with a special symbol by the object backend. For each of these
 * symbols we need to recreate the dynamic entry in a form that Diablo
 * understands */

/* DtInit: points to the start of the .init section */

DtInit {
	trigger { LINKED_SYMBOL_EXISTS("$dt_init") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.INIT", RODATA,4, 8) }
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
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FINI", RODATA,4, 8) }
	section { 
		/* DT_FINI */
		CONST32(0xd),
		RELOCATED32(0x0,"_fini",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_fini") }
}

DtStrtab {
	trigger { LINKED_SYMBOL_EXISTS("$dt_strtab") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRTAB", RODATA,4, 8) }
	section { 
		CONST32(0x5),
		RELOCATED32(0x0,"$dynstr_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_strtab") }
}

DtStrsize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_strsize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRSZ", RODATA,4, 8) }
	section { 
		CONST32(0xa),
		SIZE_OF_SECTION(".dynstr")
	}
	address { SYMBOL("$dt_strsize") }
}

DtHash {
	trigger { LINKED_SYMBOL_EXISTS("$dt_hash") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.HASH", RODATA,4, 8) }
	section { 
		CONST32(0x4),
		START_OF_SECTION(".hash")
	}
	address { SYMBOL("$dt_hash") }
}

DtPltGot {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltgot") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTGOT", RODATA,4, 8) }
	section { 
		CONST32(0x3),
		RELOCATED32(0x0,"$plt_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_pltgot") }
}

DtJmpRel {
	trigger { LINKED_SYMBOL_EXISTS("$dt_jmprel") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.JMPREL", RODATA,4, 8) }
	section { 
		CONST32(0x17),
		RELOCATED32(0x0,"$relaplt_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_jmprel") }
}

DtPltRelSize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltrelsize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTRELSZ", RODATA,4, 8) }
	section { 
		CONST32(0x2),
		SIZE_OF_SECTION(".rela.plt") 
	}
	address { SYMBOL("$dt_pltrelsize") }
}

DtSymtab {
	trigger { LINKED_SYMBOL_EXISTS("$dt_symtab") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMTAB", RODATA,4, 8) }
	section { 
		CONST32(0x6),
		RELOCATED32(0x0,"$dynsym_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_symtab") }
}

DtRelEnt {
	trigger { LINKED_SYMBOL_EXISTS("$dt_relent") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELENT", RODATA,4, 8) }
	section { 
		CONST32(0x13),
		CONST32(0x8)
	}
	address { SYMBOL("$dt_relent") }
  
}

DtRela {
	trigger { LINKED_SYMBOL_EXISTS("$dt_rela") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELA", RODATA,4, 8) }
	section { 
		CONST32(0x7),
		RELOCATED32(0x0,"$reladyn_start",0,0,0,"S00A00+\l*w\s0000$")
	}
	address { SYMBOL("$dt_rela") }
}

DtRelaSize {
	trigger { LINKED_SYMBOL_EXISTS("$dt_relasize") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELASZ", RODATA,4, 8) }
	section { 
		CONST32(0x8),
		RELOCATED32(0x0, "$reladyn_start" , 0, "$relaplt_start",0, "Z00Z01+\l*w\s0000$")
/*		SIZE_OF_SECTION(".rela.dyn")+SIZE_OF_SECTION(".rela.plt") */
	}
	address { SYMBOL("$dt_relasize") }
}

DtRelaEnt {
	trigger { LINKED_SYMBOL_EXISTS("$dt_relaent") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELAENT", RODATA,4, 8) }
	section { 
		CONST32(0x9),
		CONST32(0xc)
	}
	address { SYMBOL("$dt_relaent") }
  
}

DtPltRel {
	trigger { LINKED_SYMBOL_EXISTS("$dt_pltrel") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTREL", RODATA,4, 8) }
	section { 
		CONST32(0x14),
		CONST32(0x7)
	}
	address { SYMBOL("$dt_pltrel") }
  
}

DtDebug {
	trigger { LINKED_SYMBOL_EXISTS("$dt_debug") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.DEBUG", RODATA,4, 8) }
	section { 
		CONST32(0x15),
		CONST32(0x0)
	}
	address { SYMBOL("$dt_debug") }
  
}

DtSymEnt {
	trigger { LINKED_SYMBOL_EXISTS("$dt_syment") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMENT", RODATA,4, 8) }
	section { 
		CONST32(0xb),
		CONST32(0x10)
	}
	address { SYMBOL("$dt_syment") }
  
}

DtNeededString {
	trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),11, 0))) }
	section { STRING(SUBSTRING(MATCHED_NAME(), 11, 0)), CONST8(0x0) }
	address { READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+ABS(4)) + START_OF_SECTION(".dynstr")}
}

DtNeededStringSymbol {
	trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
	action { ADD_SYMBOL_NEW(CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
	 }


DtNeeded {
	trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("NEEDED:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 4, 8) }
	section { 
		CONST32(0x1),
		RELOCATED32(0x0, CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$")

	}
	address { SYMBOL(MATCHED_NAME()) }
}

DtRPathString {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),10, 0))) }
  section { STRING(SUBSTRING(MATCHED_NAME(), 10, 0)), CONST8(0x0) }
  address { READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME())+ABS(4)) + START_OF_SECTION(".dynstr")}
}

DtRPathStringSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action { ADD_SYMBOL_NEW(CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 12, SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0))) }
}


DtRPath {
  trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
  action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("RPATH:",SUBSTRING(MATCHED_NAME(),10, 0)), RODATA, 4, 16) }
  section {
		/* DT_NEEDED */
    CONST32(0xf),
    RELOCATED32(0x0, CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$")
    
  }
  address { SYMBOL(MATCHED_NAME()) }
}



/* Create the name data of the symbol */
DynSymString { 
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 1, STRLEN(SUBSTRING(MATCHED_NAME(),10,0)) + 1) }
	section { STRING(SUBSTRING(MATCHED_NAME(),10,0)) , CONST8(0) }
	address { READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) + START_OF_SECTION(".dynstr") }
}

/* Make a symbol for the name data */
DynSymStringSymbol {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* Create the dynamic symbols. First the ones with a non-NULL address:
 * jump slots (.plt entries) and copied symbols from shared libraries
 * (.sbss entries)
 */
DynSymNonNull
{
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") /* && !LINKED_SYMBOL_EXISTS(CONCAT("PPC_GLOB_DAT:", SUBSTRING(MATCHED_NAME(), 10, 0))) */ }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 16) }
	section { 
    RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */ 
    RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),10,0), 0, 0,0, "S00\l*w\s0000$"), /* The address of the plt/sbss elem or function */
    CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+8)) /* The symbol size */,
    CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+12)) /* The symbol type */ 
	}
	address { MATCHED_SYMBOL_VALUE()  }
}

/* For each R_PPC_JMP_SLOT relocation, we create a PPC_JMP_SLOT
 * symbol. These are the .plt entries. Every .plt entry gets
 * a dummy relocation referring to its corresponding .rela.plt
 * entry (the PLTREL:* references), so that the the .plt and
 * .rela.plt entries reference each other and hence both will
 * either be live or dead depending on whether or not the .plt
 * entry is referenced
 */
PpcJmpSlot {
  trigger { LINKED_SYMBOL_EXISTS("PPC_JMP_SLOT:*") }
  action { ADD_SUBSECTION("Linker", ".plt", CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),13,0)), DATA, 4, 8) }
	section {
		RELOCATED32(0x0, CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),13,0)), 0 , 0, 0, "S00 S00-\\$"),
		CONST32(0x0)
  }
  address { READ_LINKED_ADDR32(MATCHED_SYMBOL_VALUE()) }
}


/* add a JMP_SLOT:* symbol at the start of each JMP_SLOT:*
 * section created above
 */
PpcJmpSlotSym {
  trigger { LINKED_SYMBOL_EXISTS("PPC_JMP_SLOT:*") }
	action { ADD_SYMBOL_NEW(CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),13,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),13,0))) }
}


/* add the .rela.plt entry for the .plt entry created above:
 * first the address of the .plt entry, then the address of
 * the dynamic symbol, finally an addend of 0
 */
PpcJmpSlotRel {
	trigger { LINKED_SYMBOL_EXISTS("PPC_JMP_SLOT:*") }
	action { ADD_SUBSECTION("Linker", ".rela.plt", CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 12) }
	section {
		RELOCATED32(0x0,  CONCAT("JMP_SLOT:", SUBSTRING(MATCHED_NAME(),13,0)) , 0 , 0, 0, "S00A00+\l*w\s0000$"),
		RELOCATED32(21, CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynsym_start" , 0,"S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$"),
    CONST32(0x0)
	}
	address { MATCHED_SYMBOL_VALUE() }
}


/* add the symbol for the .rela.plt entry (so it can be referenced from
 * the .plt entry as described above)
 */
TagPpcJmpSlotRel {
	trigger  { LINKED_SYMBOL_EXISTS("PPC_JMP_SLOT:*") }
	action { ADD_SYMBOL( CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),13,0))) }
	symbol { START_OF_SUBSECTION("Linker",  CONCAT("PLTREL:",SUBSTRING(MATCHED_NAME(),13,0))) }
}


/* finally associate the plain symbol name for which the
 * .plt entry was created also with the .plt entry in
 * case that symbol wasn't defined yet.
 * TODO: check what should be done in case a symbol with
 *   that name is already declared (as plain function symbol
 *   because the function is defined in an object file),
 *   but it's referenced via a R_PPC_PLTREL24 relocation
 */
JmpSymbol2
{
	trigger { SYMBOL_EXISTS("JMP_SLOT:*") && (UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),9,0))) }
	action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),9,0), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { DUPLICATE(MATCHED_NAME()) } 
}



/* R_PPC_COPY relocations are used when symbols exist both
 * in a shared library and in the linked binary
 * We use the same trick to link the .rela.dyn and the
 * .sbss entry as with did with the .plt and .rela.plt
 *  entries for jump slots above. The rest is similar as
 *  well.
 */
DynSymCopyData {
	trigger { LINKED_SYMBOL_EXISTS("PPC_COPY:*") }
	action  { ADD_SUBSECTION("Linker", ".sbss", CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(), 9, 0)), BSS, 4, READ_LINKED_ADDR32(SYMBOL(CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),9,0)))+8)) }
	section { RELOCATED32(0x0, CONCAT("DYNREL:", SUBSTRING(MATCHED_NAME(),9,0)), 0, 0,0, "S00\*\s0000$")  }
	address {  READ_LINKED_ADDR32(SYMBOL(CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),9,0)))+4) }
}

DynSymCopyDataSymbol {
	trigger { LINKED_SYMBOL_EXISTS("PPC_COPY:*") }
  action { ADD_SYMBOL_NEW(SUBSTRING(MATCHED_NAME(),9,0), 12, SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNAMIC_BSSELEM:",SUBSTRING(MATCHED_NAME(),9,0))) }
}

PpcCopyRel {
	trigger { LINKED_SYMBOL_EXISTS("PPC_COPY:*") }
	action { ADD_SUBSECTION("Linker", ".rela.dyn", CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),9,0)), RODATA, 4, 12) }
	section {
		RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),9,0), 0, 0, 0,"S00\l*w\s0000$"),
    RELOCATED32(19, CONCAT("DYNSYMSYM:",SUBSTRING(MATCHED_NAME(),9,0)), 0, "$dynsym_start", 0, "S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$"),
    CONST32(0x0)
  }
	address { MATCHED_SYMBOL_VALUE() }
}

PpcCopySymbol {
	trigger { LINKED_SYMBOL_EXISTS("PPC_COPY:*") }
	action { ADD_SYMBOL( CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),9,0))) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),9,0))) } 
}




/* PPC_GLOB_DAT relocations are for addresses of shared library symbols in the .got
   Most of their relocation is already insterted using a tentative symbol in
   diabloelf_powerpc.c when the got entry is loaded using load with a
   R_PPC_GOT16 relocation */
PpcGlobDatRel {
	trigger { LINKED_SYMBOL_EXISTS("PPC_GLOB_DAT:*") }
	action { ADD_SUBSECTION("Linker", ".rela.dyn", CONCAT("rela.dyn.DYNREL:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 12) }
	section {
		RELOCATED32(0x0,  CONCAT("GOTELEM:", SUBSTRING(MATCHED_NAME(),13,0)) , 0 , 0, 0, "S00\l*w\s0000$"),
		RELOCATED32(20, CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynsym_start" , 0,"S00S01 - i00000004 > \ i00000008 < l i000000ff & | w \ s0000$"),
    CONST32(0x0)
	}
	address { MATCHED_SYMBOL_VALUE() }
}

TagGlobDatRel {
	trigger  { LINKED_SYMBOL_EXISTS("PPC_GLOB_DAT:*") }
	action { ADD_SYMBOL( CONCAT("GOTREL:",SUBSTRING(MATCHED_NAME(),13,0))) }
	symbol { START_OF_SUBSECTION("Linker",  CONCAT("rela.dyn.DYNREL:",SUBSTRING(MATCHED_NAME(),13,0))) }
}

/* Jump slot in the .plt section occupy at first 8 bytes per entry.
 * After all these 8 byte entries, we need an equal amount of empty 4 byte entries for
 * use by the dynamic linker. We don't have to allocate this space manually however,
 * because even though the .plt section does not appear in any object file, it's
 * still mentioned in the link map and diablo reads the (correct) final size
 * from there.
 *
 * Note that as of entry 2^13, onl the even entry indexes can be used (so each entry
 * becomes 16 bytes large), but this is not yet implemented. We'll give a FATAL in
 * the PPC32_CHECK_PLT_SIZE callback if that happens. The callback also checks whether
 * our constructed plt's size matches the original one.
 */
CheckPltSize {
  action  { PPC32_CHECK_PLT_SIZE() }
}


DynSymSym {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}


/* The dynamic section is terminated by a DT_NULL entry. We add it here */

EndDynamic {
	trigger {  SECTION_EXISTS(".dynamic") }
	action { ADD_SUBSECTION("Linker",".dynamic",".dynamic.END", RODATA,4, 8) }
	section {
		CONST32(0x0),
		CONST32(0x0)
  }
  address { END_OF_SECTION(".dynamic") - ABS(0x8) }
}

/* }}} */


/* Other Link-time symbols {{{*/

/*========= Start of preinit ========= */
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
  action  { ADD_SYMBOL("_init",SYMBOL_TYPE_NOTYPE) }
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
  action  { ADD_SYMBOL("_fini",SYMBOL_TYPE_NOTYPE) }
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


/* The are some symbols in the GLIBC_2.3.3 that are defined with a 
 * versioned symbol. We add them manually. */
AddSymbolForVersionedsetjmp {
  action { ADD_SYMBOL("_setjmp",SYMBOL_TYPE_NOTYPE) }
  trigger{ UNDEFINED_SYMBOL("_setjmp") && SYMBOL_EXISTS("_setjmp@@GLIBC_2.3.4*") }
  symbol { DUPLICATE("_setjmp@@GLIBC_2.3.4") }
}

AddSymbolForVersionedlongjmp {
  action { ADD_SYMBOL("longjmp",SYMBOL_TYPE_NOTYPE) }
  trigger{ UNDEFINED_SYMBOL("longjmp") && SYMBOL_EXISTS("longjmp@@GLIBC_2.3.4*") }
  symbol { DUPLICATE("longjmp@@GLIBC_2.3.4") }
}

EhFrameBegin {
  action  { ADD_SYMBOL("__EH_FRAME_BEGIN__",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("__EH_FRAME_BEGIN__") && SECTION_EXISTS(".eh_frame") }
  symbol  { START_OF_SECTION(".eh_frame") }
}

JcrLIST {
  action  { ADD_SYMBOL("__JCR_LIST__",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".jcr") }
  symbol  { START_OF_SECTION(".jcr") } 
}

_edata {
  action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".sdata") }
  symbol  { END_OF_SECTION(".sdata") } 
}

_edata {
  action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".sbss") && !SECTION_EXISTS(".sdata") }
  symbol  { START_OF_SECTION(".sbss") } 
}

_edata {
  action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".plt") && !SECTION_EXISTS(".sbss") && !SECTION_EXISTS(".sdata") }
  symbol  { START_OF_SECTION(".plt") } 
}

/* Add a _SDA_BASE_ symbol at the start of the sdata section if .sdata exists*/
SDABASEWithSdata {
  action  { ADD_SYMBOL("_SDA_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_SDA_BASE_") && SECTION_EXISTS(".sdata") }
  symbol  { START_OF_SECTION(".sdata") + OFFSET(0x8000) } 
}

/* Otherwise at the start of the sbss section if .sbss exists (needed for Cell binaries) */
SDABASEWithSBSS {
  action  { ADD_SYMBOL("_SDA_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_SDA_BASE_") && SECTION_EXISTS(".sbss") }
  symbol  { START_OF_SECTION(".sbss") + OFFSET(0x8000) } 
}

/* If no .sdata exists, _SDA_BASE_ symbol must be 0.  */
SDABASEWithNoSdata{
  action  { ADD_SYMBOL("_SDA_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_SDA_BASE_") }
  symbol  { ABS(0) } 
}

/* Add a _SDA2_BASE_ symbol to the start of the sdata section if .sdata2 exists*/
SDA2BASEWithSdata {
  action  { ADD_SYMBOL("_SDA2_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".sdata2") }
  symbol  { START_OF_SECTION(".sdata2") + OFFSET(0x8000) } 
}

/* Otherwise at the start of the sbss2 section if .sbss2 exists */
SDA2BASEWithSBSS2 {
  action  { ADD_SYMBOL("_SDA2_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_SDA2_BASE_") && SECTION_EXISTS(".sbss2") }
  symbol  { START_OF_SECTION(".sbss2") + OFFSET(0x8000) } 
}

/* If no .sdata2 exists, _SDA_BASE_ symbol must be 0.  */
SDA2BASEWithNoSdata{
  action  { ADD_SYMBOL("_SDA2_BASE_",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("_SDA2_BASE_") }
  symbol  { ABS(0) } 
}/*}}}*/



/*==================================================*/

/* Read-only sections, merged into text segment: */
/*  PROVIDE (__executable_start = 0x10000000); . = 0x10000000 + SIZEOF_HEADERS;*/

SECTIONS
{
  PROVIDE (__executable_start = 0x10000000); . = 0x10000000 + SIZEOF_HEADERS;
  .interp         : { *(.interp) }
  PUT_REMAINING_SECTIONS(NOTE_SECTION);
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
      *(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)
      *(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)
      *(.rel.sdata2 .rel.sdata2.* .rel.gnu.linkonce.s2.*)
      *(.rel.sbss2 .rel.sbss2.* .rel.gnu.linkonce.sb2.*)
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
      *(.rela.got1)
      *(.rela.got2)
      *(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)
      *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)
      *(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)
      *(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)
      *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
  }
  .rel.plt        : { *(.rel.plt) }
  .rela.plt       : { *(.rela.plt) }
  .init           :
  {
    KEEP (*(.init))
  } =0
  .text           :
  {
    *(.text .stub .text.* .gnu.linkonce.t.*)
      /* .gnu.warning sections are handled specially by elf32.em.  */
      *(.gnu.warning)
  } =0
  .fini           :
  {
    KEEP (*(.fini))
  } =0
  PUT_REMAINING_SECTIONS(CODE_SECTION);
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  PUT_REMAINING_SECTIONS(RO_DATA_SECTION);
  .sdata2         : { *(.sdata2 .sdata2.* .gnu.linkonce.s2.*) }
  .sbss2          : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) }
  .eh_frame_hdr : { *(.eh_frame_hdr) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN (0x10000) - ((0x10000 - .) & (0x10000 - 1)); 
  /* . = DATA_SEGMENT_ALIGN (0x10000, 0x1000);*/
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  . = ALIGN(32 / 8);
  PROVIDE (__preinit_array_start = .);
  .preinit_array     : { *(.preinit_array) }
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array     : { *(.init_array) }
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array     : { *(.fini_array) }
  PROVIDE (__fini_array_end = .);
  .data1          : { *(.data1) }
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .gcc_except_table   : { *(.gcc_except_table) }
  .got1           : { *(.got1) }
  .got2           : { *(.got2) }
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
  .jcr            : { KEEP (*(.jcr)) }
  .data.rel.ro    : {} 
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
      SORT(CONSTRUCTORS)
  }
  .got            : { *(.got.plt) *(.got) }
  PUT_REMAINING_SECTIONS(DATA_SECTION);
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata          :
  {
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  }
  _edata = .;
  PROVIDE (edata = .);
  __bss_start = .;
  .sbss           :
  {
    PROVIDE (__sbss_start = .);
    PROVIDE (___sbss_start = .);
    *(.dynsbss)
      *(.sbss .sbss.* .gnu.linkonce.sb.*)
      *(.scommon)
      PROVIDE (__sbss_end = .);
    PROVIDE (___sbss_end = .);
  }
  .plt            : { *(.plt) }
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
  /*. = DATA_SEGMENT_END (.);*/
  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /DISCARD/	: { *(.fixup) }
  /DISCARD/ : { *(.note.GNU-stack) }
}

/* vim: set foldmethod=marker : */
