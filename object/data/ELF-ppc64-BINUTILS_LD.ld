linker="ppc GNU LD"
fileformats="ELF"
architectures="ppc64"

/* Add a virtual GOT table in the case there is no .got section (for manually
 * written assembler) */
FalseGotTable {
  action  { ADD_SYMBOL("_GLOBAL_OFFSET_TABLE_",SYMBOL_TYPE_OBJECT) }
  trigger { !SYMBOL_EXISTS("_GLOBAL_OFFSET_TABLE_") && !SECTION_EXISTS(".got") && SECTION_EXISTS(".data") }
  symbol  { START_OF_SECTION(".data") }
}

/* Section start/stop symbols {{{ */
JcrLIST {
  action  { ADD_SYMBOL("__JCR_LIST__",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".jcr") }
  symbol  { START_OF_SECTION(".jcr") } 
}

JcrEnd {
  action  { ADD_SYMBOL("__JCR_END__",SYMBOL_TYPE_NOTYPE) }
  trigger { SECTION_EXISTS(".jcr") }
  symbol  { END_OF_SECTION(".jcr") }
}
/* }}} */

/* Vectorizations {{{ */
EhFrameVector {
  action {  VECTORIZE(".eh_frame") }
  trigger { SUBSECTION_EXISTS("*",".eh_frame") }
}

EhFrameBegin {
  action  { ADD_SYMBOL_NEW("__EH_FRAME_BEGIN__", 12) }
  trigger { UNDEFINED_SYMBOL("__EH_FRAME_BEGIN__")   && SECTION_EXISTS(".eh_frame") }
  symbol  { START_OF_SECTION(".eh_frame") }
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

/* }}} */

/* List of versioned symbols {{{ */
/* The are some symbols in the GLIBC_2.3.3 that are defined with a 
 * versioned symbol. We add them manually. */

AddSymbolForVersionedlongjmp {
  action { ADD_SYMBOL("longjmp",SYMBOL_TYPE_FUNCTION) }
  trigger{ UNDEFINED_SYMBOL("longjmp") && SYMBOL_EXISTS("longjmp@@GLIBC_2.3.4") }
  symbol { DUPLICATE("longjmp@@GLIBC_2.3.4") }
}

AddSymbolForVersionedlongjmp {
  action { ADD_SYMBOL("siglongjmp",SYMBOL_TYPE_FUNCTION) }
  trigger{ UNDEFINED_SYMBOL("siglongjmp") && SYMBOL_EXISTS("siglongjmp@@GLIBC_2.3.4") }
  symbol { DUPLICATE("siglongjmp@@GLIBC_2.3.4") }
}

AddSymbolForVersionedlongjmp {
  action { ADD_SYMBOL("_longjmp",SYMBOL_TYPE_FUNCTION) }
  trigger{ UNDEFINED_SYMBOL("_longjmp") && SYMBOL_EXISTS("_longjmp@@GLIBC_2.3.4") }
  symbol { DUPLICATE("_longjmp@@GLIBC_2.3.4") }
}

AddSymbolForVersionedlongjmp {
  action { ADD_SYMBOL("__sigsetjmp",SYMBOL_TYPE_FUNCTION) }
  trigger{ UNDEFINED_SYMBOL("__sigsetjmp") && SYMBOL_EXISTS("__sigsetjmp@@GLIBC_2.3.4") }
  symbol { DUPLICATE("__sigsetjmp@@GLIBC_2.3.4") }
}

AddSymbolForVersioned_setjmp {
  action { ADD_SYMBOL("_setjmp",SYMBOL_TYPE_FUNCTION) }
  trigger{ UNDEFINED_SYMBOL("_setjmp") && SYMBOL_EXISTS("_setjmp@@GLIBC_2.3.4") }
  symbol { DUPLICATE("_setjmp@@GLIBC_2.3.4") }
}
/* }}} */

/* TLS {{{ */
StartTbss {
   action  { ADD_SYMBOL("$tls_start",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".tdata") }
   symbol  { START_OF_SECTION(".tdata") }
}

TlsStartTbss {
   action  { ADD_SYMBOL("$tls_start",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".tbss") && !SECTION_EXISTS(".tdata") }
   symbol  { START_OF_SECTION(".tbss") }
}

TlsEndWithTbss {
   action  { ADD_SYMBOL("_etls",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_etls")   && SECTION_EXISTS(".tbss") }
   symbol  { END_OF_SECTION(".tbss") }
}
/* }}} */

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
/* }}} */

EhFrameHdr {
        trigger { SECTION_EXISTS(".eh_frame_hdr") }
        action  { ADD_SUBSECTION("Linker",".eh_frame_hdr",".eh_frame_hdr", RODATA,8, 8) }
        section {
			CONST32(0x0),
			CONST32(0x0)
                }
        address { START_OF_SECTION(".eh_frame_hdr") }
}

/* If the object for which we are emulating the link holds an .interp section
 * we need to recreate this section. The interp section lists the dynamic
 * loader that is to be used to load this dynamically linked program. In all
 * currently supported dynamically linked programs it will hold the string
 * /lib64/ld64.so.1. Mind that we must explicitly add the trailing zero when
 * using STRING filler */

Interp {
        trigger { SECTION_EXISTS(".interp") }
        action  { ADD_SUBSECTION("Linker",".interp",".interp", RODATA,4, 17) }
        section {
                  STRING("/lib64/ld64.so.1"),
                  CONST8(0x00)
                }
        address { START_OF_SECTION(".interp") }
}


NullGot {
	trigger {  SECTION_EXISTS(".got") }
	action { ADD_SUBSECTION("Linker",".got","NULLGOT", RODATA, 8, 8) }
	section {
		RELOCATED64(0x0, "$got_start", 0, 0,0, "S00 s8000+ \L*W\s0000$")
	}
	address { START_OF_SECTION(".got") }
}


/* Each dynamic entry found in the object for which we are emulating the link
 * gets marked with a special symbol by the object backend. For each of these
 * symbols we need to recreate the dynamic entry in a form that Diablo
 * understands.
 *
 * All the DT_ code: {{{ */

DtNeededString {
        trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
        action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),11, 0))) }
        section { STRING(SUBSTRING(MATCHED_NAME(), 11, 0)), CONST8(0x0) }
        address { READ_LINKED_VALUE64(SYMBOL(MATCHED_NAME())+ABS(8)) + START_OF_SECTION(".dynstr")}
}

DtNeededStringSymbol {
        trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
        action { ADD_SYMBOL_NEW(CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 12) }
        symbol { START_OF_SUBSECTION("Linker", CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0))) }
}


DtNeeded {
        trigger { LINKED_SYMBOL_EXISTS("$dt_needed:*") }
        action  { ADD_SUBSECTION("Linker", ".dynamic", CONCAT(".dynamic.NEEDED:",SUBSTRING(MATCHED_NAME(),11, 0)), RODATA, 4, 16) }
        section {
		/* DT_NEEDED */
                CONST32(0x0), CONST32(0x1),
                RELOCATED64(0x0, CONCAT("NEEDEDSTR:",SUBSTRING(MATCHED_NAME(),11, 0)), 0, "$dynstr_start",0, "S00S01-\L*W\s0000$")

        }
        address { SYMBOL(MATCHED_NAME()) }
}


DtRPathString {
        trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
        action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), RODATA, 1, 1 + STRLEN(SUBSTRING(MATCHED_NAME(),10, 0))) }
        section { STRING(SUBSTRING(MATCHED_NAME(), 10, 0)), CONST8(0x0) }
        address { READ_LINKED_VALUE64(SYMBOL(MATCHED_NAME())+ABS(8)) + START_OF_SECTION(".dynstr")}
}

DtRPathStringSymbol {
        trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
        action { ADD_SYMBOL_NEW(CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 12) }
        symbol { START_OF_SUBSECTION("Linker", CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0))) }
}


DtRPath {
        trigger { LINKED_SYMBOL_EXISTS("$dt_rpath:*") }
        action  { ADD_SUBSECTION("Linker", ".dynamic", CONCAT(".dynamic.RPATH:",SUBSTRING(MATCHED_NAME(),10, 0)), RODATA, 4, 16) }
        section {
		/* DT_NEEDED */
                CONST32(0x0), CONST32(0xf),
                RELOCATED64(0x0, CONCAT("RPATHSTR:",SUBSTRING(MATCHED_NAME(),10, 0)), 0, "$dynstr_start",0, "S00S01-\L*W\s0000$")

        }
        address { SYMBOL(MATCHED_NAME()) }
}

/* DtInit: points to the start of the .init section */

DtInit {
        trigger { LINKED_SYMBOL_EXISTS("$dt_init") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.INIT", RODATA,4, 16) }
        section {
                /* DT_INIT */
                CONST32(0x0), CONST32(0xc),
		RELOCATED64(0x0,"_init",0,0,0,"S00A00+\L*W\s0000$") 
        }
        address { SYMBOL("$dt_init") }
}

/* DtFini: points to the start of the .fini section */

DtFini {
        trigger { LINKED_SYMBOL_EXISTS("$dt_fini") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.FINI", RODATA,4, 16) }
        section {
                /* DT_FINI */
                CONST32(0x0), CONST32(0xd),
                RELOCATED64(0x0,"_fini",0,0,0,"S00A00+\L*W\s0000$")
        }
        address { SYMBOL("$dt_fini") }
}

DtStrtab {
        trigger { LINKED_SYMBOL_EXISTS("$dt_strtab") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRTAB", RODATA,4, 16) }
        section {
                CONST32(0x0), CONST32(0x5),
		START_OF_SECTION(".dynstr")
        }
        address { SYMBOL("$dt_strtab") }
}

DtHash {
	trigger { LINKED_SYMBOL_EXISTS("$dt_hash") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.HASH", RODATA,4,16) }
	section {
		  CONST32(0x0),
		  CONST32(0x4),
		  START_OF_SECTION(".hash")
		}
	address { SYMBOL("$dt_hash") }
}

DtSymtab {
        trigger { LINKED_SYMBOL_EXISTS("$dt_symtab") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMTAB", RODATA,4, 16) }
        section {
		CONST32(0x0),
                CONST32(0x6),
		START_OF_SECTION(".dynsym")
        }
        address { SYMBOL("$dt_symtab") }
}

DtSymbolic {
	trigger { LINKED_SYMBOL_EXISTS("$dt_symbolic") }
	action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMBOLIC", RODATA,4,16) }
	section {
		  CONST32(0x0),
		  CONST32(0x10),
                  CONST64(0x0)
		}
	address { SYMBOL("$dt_symbolic") }
}

DtFlags {
    trigger { LINKED_SYMBOL_EXISTS("$dt_flags") }
    action  { ADD_SUBSECTION("Linker", ".dynamic", ".dynamic.FLAGS", RODATA, 4, 16) }
    section { CONST32(0x0), CONST32(0x1e),
        CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME()) + 8)),
        CONST32(READ_LINKED_VALUE32(SYMBOL(MATCHED_NAME()) + 12))
    }
    address { SYMBOL("$dt_flags") }
}

DtStrsize {
        trigger { LINKED_SYMBOL_EXISTS("$dt_strsize") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.STRSZ", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0xa),
                SIZE_OF_SECTION(".dynstr")
        }
        address { SYMBOL("$dt_strsize") }
}

DtSymEnt {
        trigger { LINKED_SYMBOL_EXISTS("$dt_syment") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.SYMENT", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0xb),
                CONST32(0x0),
                CONST32(0x18)
        }
        address { SYMBOL("$dt_syment") }

}


DtDebug {
        trigger { LINKED_SYMBOL_EXISTS("$dt_debug") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.DEBUG", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x15),
                CONST32(0x0),
                CONST32(0x0)
        }
        address { SYMBOL("$dt_debug") }

}


DtRela {
        trigger { LINKED_SYMBOL_EXISTS("$dt_rela") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELA", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x07),
                START_OF_SECTION(".rela.dyn")
        }
        address { SYMBOL("$dt_rela") }
}


DtRelaSize {
        trigger { LINKED_SYMBOL_EXISTS("$dt_relasize") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELASZ", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x08),
                SIZE_OF_SECTION(".rela.dyn")
        }
        address { SYMBOL("$dt_relasize") }
}

DtRelaEnt {
        trigger { LINKED_SYMBOL_EXISTS("$dt_relaent") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELAENT", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x09),
                CONST32(0x0),
                CONST32(0x18)
        }
        address { SYMBOL("$dt_relaent") }

}



DtPltGot {
        trigger { LINKED_SYMBOL_EXISTS("$dt_pltgot") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTGOT", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x3),
                START_OF_SECTION(".plt")
        }
        address { SYMBOL("$dt_pltgot") }
}

DtRelSize {
        trigger { LINKED_SYMBOL_EXISTS("$dt_relsize") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.RELSZ", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x12),
                SIZE_OF_SECTION(".rel.dyn")
        }
        address { SYMBOL("$dt_relsize") }
}


DtPltRelSize {
        trigger { LINKED_SYMBOL_EXISTS("$dt_pltrelsize") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTRELSZ", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x2),
                SIZE_OF_SECTION(".rela.plt")
        }
        address { SYMBOL("$dt_pltrelsize") }
}

DtPltRel {
        trigger { LINKED_SYMBOL_EXISTS("$dt_pltrel") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.PLTREL", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x14),
                CONST32(0x0),
                CONST32(0x7)
        }
        address { SYMBOL("$dt_pltrel") }
}

DtJmpRel {
        trigger { LINKED_SYMBOL_EXISTS("$dt_jmprel") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.JMPREL", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x17),
                START_OF_SECTION(".rela.plt")
        }
        address { SYMBOL("$dt_jmprel") }
}

/* The dynamic section is terminated by a DT_NULL entry. We add it here */

EndDynamic {
	trigger {  SECTION_EXISTS(".dynamic") }
	action { ADD_SUBSECTION("Linker",".dynamic",".dynamic.END", RODATA,4, 16) }
	section {
		CONST32(0x0),
		CONST32(0x0),
		CONST32(0x0),
		CONST32(0x0)
	 }
	 address { END_OF_SECTION(".dynamic") - ABS(0x10) }
}


MakeGlinkStub {
	trigger { LINKED_SYMBOL_EXISTS("$dt_ppc64_glink") } 
	action { ADD_SUBSECTION("Linker", ".text", "__glink", CODE, 4, 64) }
	section {
		CONST32(0x7d8902a6),     /* mfctr   r12*/
		CONST32(0x780b1f24),     /* rldicr  r11,r0,3,60*/
		CONST32(0x34408000),     /* addic.  r2,r0,-32768*/
		CONST32(0x7d8b6050),     /* subf    r12,r11,r12*/
		CONST32(0x7c42fe76),     /* sradi   r2,r2,63*/
		CONST32(0x780b1764),     /* rldicr  r11,r0,2,61*/
		CONST32(0x7c425838),     /* and     r2,r2,r11*/
		CONST32(0x7d8b6050),     /* subf    r12,r11,r12*/
		CONST32(0x7d8c1214),     /* add     r12,r12,r2*/
		RELOCATED32(0x3d8c0000, "$plt_start", 0, "__glink",0, "S00 S01 s0040 + - = s8000 & s0001 < + \  iffff0000 & s0010 >  l iffff0000 & | w \s0000$"),     /* addis   r12,r12,1*/
		RELOCATED32(0xe96c0000, "$plt_start", 0, "__glink",0, "S00 S01 s0040 + - i0000ffff &\ l iffff0000 & | w \s0000$"),    
		RELOCATED32(0x398c0000, "$plt_start", 0, "__glink",0, "S00 S01 s0040 + - i0000ffff &\ l iffff0000 & | w \s0000$"),    
		CONST32(0xe84c0008),     /* ld      r2,8(r12)*/
		CONST32(0x7d6903a6),     /* mtctr   r11*/
		CONST32(0xe96c0010),     /* ld      r11,16(r12)*/
		CONST32(0x4e800420)     /* bctr*/
	}
        address { READ_LINKED_VALUE64(SYMBOL("$dt_ppc64_glink") + 8) - 0x20 }
}


/* Make a symbol to the name data */
MakeGlinkEntry {
	trigger { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
	action { ADD_SUBSECTION("Linker", ".text", CONCAT("__glink_stub:",SUBSTRING(MATCHED_NAME(),15,0)) , CODE, 4, 8) }
	section {
		RELOCATED32(0x38000000, "__glink", 0, 0, 0, "P S00 - s0040 - s0003> i00ffffff & \ l iff000000 & | w \ s0000$"),     /* li      r0,0*/
		RELOCATED32(0x4bffffbc, "__glink", 0, 0, 0, "S00 P- i03fffffc & \ l ifc000003 & | w \ s0000$")     /* b       0x10000720*/
	}
        address { (READ_LINKED_VALUE64(MATCHED_SYMBOL_VALUE())  - SUBSYMBOL("$plt_start") - 0x18)/0x3 + SYMBOL("__glink") + 0x40 }
}

MakeGlinkEntrySym {
	trigger { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
	action { ADD_SYMBOL_NEW(CONCAT("__glink_stub:",SUBSTRING(MATCHED_NAME(),15,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker", CONCAT("__glink_stub:",SUBSTRING(MATCHED_NAME(),15,0))) }
	}

DtGlink {
        trigger { LINKED_SYMBOL_EXISTS("$dt_ppc64_glink") }
	action { ADD_SYMBOL_NEW("__glink", 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker", "__glink") }
} 

DtGlink {
        trigger { LINKED_SYMBOL_EXISTS("$dt_ppc64_glink") }
        action  { ADD_SUBSECTION("Linker",".dynamic",".dynamic.GLINK", RODATA,4, 16) }
        section {
                CONST32(0x0),
                CONST32(0x70000000),
                RELOCATED64(0x0, "__glink", 0, 0,0, "S00 s0020 + \L*W\s0000$")
        }
        address { SYMBOL("$dt_ppc64_glink") }
}
/* }}} */

/* Recreate dynamic symbols {{{ */

/* A symbol table needs a leading '\0' in the string table as name of the null
 * symbol. We add it here. */

NullDynStr {
	trigger {  SECTION_EXISTS(".dynstr") }
	action { ADD_SUBSECTION("Linker",".dynstr","NULLDYNSTR", RODATA,1, 1) }
	section {
	CONST8(0x0)
	} 
	address { START_OF_SECTION(".dynstr") }
}

/* A symbol table needs to start with a null symbol. We add it here. */


NullDynSym {
	trigger {  SECTION_EXISTS(".dynsym") }
	action { ADD_SUBSECTION("Linker",".dynsym","NULLDYNSYM", RODATA,4, 24) }
	section {
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0),
	CONST32(0x0)
	} 
	address { START_OF_SECTION(".dynsym") }
}


/* Create the name data of the symbol */
DynSymString 
{
	        trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
		action  { ADD_SUBSECTION("Linker", ".dynstr", CONCAT("DYNSYMSTR:", SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 1, STRLEN(SUBSTRING(MATCHED_NAME(),10,0)) + 1) }
		section { STRING(SUBSTRING(MATCHED_NAME(),10,0)) , CONST8(0) }
		address { READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) + START_OF_SECTION(".dynstr") }
}

/* Make a symbol to the name data */
DynSymStringSymbol {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0))) }
}

/* Make the symbol */
DynSym 
{
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 24) }
	section { 
		RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */	
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4)), /* 8 bits (Bind + Type) + 8 bits st_other + 16 bits section */
		CONST32(0x0),
		CONST32(0x0),
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+16)), CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+20))  /* Size */
	}
	address { MATCHED_SYMBOL_VALUE()  }
}

DynSym 
{
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") && !UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),10,0)) }
	action  { ADD_SUBSECTION("Linker", ".dynsym", CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0)), RODATA, 4, 24) }
	section { 
		RELOCATED32(0x0, CONCAT("DYNSYMSYMSTR:",SUBSTRING(MATCHED_NAME(),10,0)), 0, "$dynstr_start",0, "S00S01-\l*w\s0000$"), /* The offset in the string table */	
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4)), /* 8 bits (Bind + Type) + 8 bits st_other + 16 bits section */
		RELOCATED64(0x0,  SUBSTRING(MATCHED_NAME(),10,0) , 0 , 0, 0, "S00\L*W\s0000$"),
		CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+16)), CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+20))  /* Size */
	}
	address { MATCHED_SYMBOL_VALUE()  }
}

DynSymSym {
	trigger { LINKED_SYMBOL_EXISTS("DYNSYMSYM:*") }
	action { ADD_SYMBOL_NEW(CONCAT("DYNSYMSYM:",SUBSTRING(MATCHED_NAME(),10,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("DYNSYM:",SUBSTRING(MATCHED_NAME(),10,0))) }
}
/* }}} */



Ppc64JmpSlot {
        trigger { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
        action { ADD_SUBSECTION("Linker", ".plt", CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),15,0)), RODATA, 4, 24) }
	section {
		RELOCATED32(0x0, CONCAT("__glink_stub:",SUBSTRING(MATCHED_NAME(),15,0)), 0 , 0, 0, "S00 S00-\\$"),
		RELOCATED32(0x0, CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),15,0)), 0 , 0, 0, "S00 S00-\\$"),
		CONST32(0x0),
		CONST32(0x0),
		CONST32(0x0),
		CONST32(0x0)
		}
        address { READ_LINKED_VALUE64(MATCHED_SYMBOL_VALUE()) }
}



Ppc64JmpSlotSym {
        trigger { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
	action { ADD_SYMBOL_NEW(CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),15,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("JMP_SLOT:",SUBSTRING(MATCHED_NAME(),15,0))) }
}

Ppc64JmpSlotRel {
	trigger { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
	action { ADD_SUBSECTION("Linker", ".rela.plt", CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),15,0)), RODATA, 4, 24) }
	section {
		RELOCATED64(0x0,  CONCAT("JMP_SLOT:", SUBSTRING(MATCHED_NAME(),15,0)) , 0 , 0, 0, "S00\L*W\s0000$"),
		RELOCATED32(0x0, CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),15,0)), 0, "$dynsym_start" , 0,"S00S01- s0018 / \l*w\s0000$"), 
		CONST32(0x15),
		CONST32(0x0),
		CONST32(0x0)
	}
	address { MATCHED_SYMBOL_VALUE() }
     }

TagPpc64JmpSlotRel {
	trigger  { LINKED_SYMBOL_EXISTS("PPC64_JMP_SLOT:*") }
	action { ADD_SYMBOL( CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),15,0)), SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",  CONCAT("DYNREL:",SUBSTRING(MATCHED_NAME(),15,0))) }
	}

DoLinkerStubsMagic {
  action  { PPC64_ADD_LINKER_STUBS() }
}

/* 
Ppc64Addr64 {
        trigger { LINKED_SYMBOL_EXISTS("PPC64_ADDR64:*") }
        action { ADD_SUBSECTION("Linker", ".got", CONCAT("ADDR64:",SUBSTRING(MATCHED_NAME(),13,0)), RODATA, 4, 8) }
	section {
		CONST32(0x0),
		CONST32(0x0)
		}
        address { READ_LINKED_VALUE64(MATCHED_SYMBOL_VALUE()) }
} */
/* 
Ppc64Addr64Sym {
        trigger { LINKED_SYMBOL_EXISTS("PPC64_ADDR64:*") }
	action { ADD_SYMBOL_NEW(CONCAT("ADDR64:",SUBSTRING(MATCHED_NAME(),13,0)), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { START_OF_SUBSECTION("Linker",CONCAT("ADDR64:",SUBSTRING(MATCHED_NAME(),13,0))) }
} */

Ppc64Addr64Rel {
	trigger { LINKED_SYMBOL_EXISTS("PPC64_ADDR64:*") }
	action { ADD_SUBSECTION("Linker", ".rela.dyn", ".rela.dyn", RODATA, 4, 24) }
	section {
		SECTION_OFFSET(SUBSECTION_BY_ADDRESS(READ_LINKED_VALUE64(MATCHED_SYMBOL_VALUE())), READ_LINKED_VALUE64(MATCHED_SYMBOL_VALUE())),
		RELOCATED32(0x0, CONCAT("DYNSYMSYM:", SUBSTRING(MATCHED_NAME(),13,0)), 0, "$dynsym_start" , 0,"S00S01- s0018 / \l*w\s0000$"), 
		CONST32(0x26),
		CONST32(0x0),
		CONST32(0x0)
	}
	address { MATCHED_SYMBOL_VALUE() }
     }


/* }}} */

NullPlt {
        trigger {  SECTION_EXISTS(".plt") }
        action { ADD_SUBSECTION("Linker",".plt","NULLPLT", BSS, 8, 24) }
        address { START_OF_SECTION(".plt") }
}

SymSymbol 
{
	trigger { SYMBOL_EXISTS(".bl.*") && UNDEFINED_SYMBOL(MATCHED_NAME()) && (!WEAK_UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),4,0))) && (!UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),4,0))) }
	action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, SYMBOL_TYPE_NOTYPE) }
	symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) } 
}

SymSymbol2
{
	trigger { SYMBOL_EXISTS(".bl.*") && UNDEFINED_SYMBOL(MATCHED_NAME()) && (WEAK_UNDEFINED_SYMBOL(SUBSTRING(MATCHED_NAME(),4,0))) }
	action { ADD_WEAK_SYMBOL(MATCHED_NAME(), SYMBOL_TYPE_NOTYPE) }
	symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) } 
}

DynamicVector {
	action {  VECTORIZE(".rela.dyn") }
	trigger { SUBSECTION_EXISTS("*",".rela.dyn") }
}

/*==================================================*/
BssStartWithBss {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".plt")  }
   symbol  { START_OF_SECTION(".plt") } 
}

BssStartWithBss {
   action  { ADD_SYMBOL("__bss_start",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".bss") && UNDEFINED_SYMBOL("__bss_start")  }
   symbol  { START_OF_SECTION(".bss") } 
}

Edata {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".plt")  }
   symbol  { START_OF_SECTION(".plt") } 
}

Edata {
   action  { ADD_SYMBOL("_edata",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".bss") && UNDEFINED_SYMBOL("__edata")  }
   symbol  { START_OF_SECTION(".bss") } 
}

End {
   action  { ADD_SYMBOL("_end",SYMBOL_TYPE_NOTYPE) }
   trigger { SECTION_EXISTS(".bss")  }
   symbol  { END_OF_SECTION(".bss") } 
}

/* .init and .fini can only be vectorized at the end of the script because
 * we first need to make sure all linker stubs are generated */
InitVector2 {
  action {  VECTORIZE(".init") }
  trigger { SUBSECTION_EXISTS("*",".init") }
}

FiniVector2 {
  action {  VECTORIZE(".fini") }
  trigger { SUBSECTION_EXISTS("*",".fini") }
}


SECTIONS
{
  /* Read-only sections, merged into text segment: */
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
      *(.rel.data.rel.ro*)
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
      *(.rela.toc)
      *(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)
      *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)
      *(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)
      *(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)
      *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
    }
  .rel.plt        : { *(.rel.plt) }
  .rela.plt       : { *(.rela.plt) }
  .rela.tocbss	  : { *(.rela.tocbss) }
  .init           :
  {
    KEEP (*(.init))
  } =0x60000000
  .text           :
  {
    *(.text .stub .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.sfpr .glink)
  } =0x60000000
  .fini           :
  {
    KEEP (*(.fini))
  } =0x60000000
  PUT_REMAINING_SECTIONS(CODE_SECTION);
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  .eh_frame_hdr : { *(.eh_frame_hdr) }
  .eh_frame : { *(.eh_frame) }

  PUT_REMAINING_SECTIONS(RO_DATA_SECTION);
  .sdata2         : { *(.sdata2 .sdata2.* .gnu.linkonce.s2.*) }
  .sbss2          : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) }

  . = . + 0x8000;

  /* TODO: not parseable: .eh_frame       : ONLY_IF_RO { KEEP (*(.eh_frame)) } */
  /* TODO: not parseable: .gcc_except_table   : ONLY_IF_RO { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) } */
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  /* TODO: not parseable: . = ALIGN (0x10000) - ((0x10000 - .) & (0x10000 - 1)); . = DATA_SEGMENT_ALIGN (0x10000, 0x1000); */
  /* Exception handling  */
  /* TODO: not parseable: .eh_frame       : ONLY_IF_RW { KEEP (*(.eh_frame)) } */
  /* TODO: not parseable: .gcc_except_table   : ONLY_IF_RW { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) } */
  /* Thread Local Storage sections  */
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  . = ALIGN(64 / 8);
  PROVIDE (__preinit_array_start = .);
  .preinit_array     : { KEEP (*(.preinit_array)) }
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array     : { KEEP (*(.init_array)) }
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array     : { KEEP (*(.fini_array)) }
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
  .data.rel.ro : { *(.data.rel.ro.local) *(.data.rel.ro*) }
  .dynamic        : { *(.dynamic) *(.dynamic.*) }
  /* TODO: not parseable: . = DATA_SEGMENT_RELRO_END (0, .); */
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    KEEP (*(.gnu.linkonce.d.*personality*))
    SORT(CONSTRUCTORS)
  }
  .data1          : { *(.data1) }
  .toc1		 ALIGN(8) : { *(.toc1) }
  .opd		 ALIGN(8) : { KEEP (*(.opd)) }
  .got		ALIGN(8) : { *(.got .toc) }
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
  .tocbss	 ALIGN(8) : { *(.tocbss)}
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
   . = ALIGN(64 / 8);
  }
  . = ALIGN(64 / 8);
  PUT_REMAINING_SECTIONS(BSS_SECTION);
  _end = .;
  PROVIDE (end = .);
  /* TODO: not parseable: . = DATA_SEGMENT_END (.); */
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
  /DISCARD/ : { *(.note.GNU-stack) }
}

/* vim: set foldmethod=marker : */
