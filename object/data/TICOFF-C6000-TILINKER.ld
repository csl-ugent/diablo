

AddSymbolForCLKStart {
action { ADD_SYMBOL("_CLK_start",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_CLK_start") && SYMBOL_EXISTS("_CLK_start6x") }
symbol { DUPLICATE("_CLK_start6x") }
}

AddSymbolForTSK_VCREATEFXN {
action { ADD_SYMBOL("TSK_VCREATEFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("TSK_VCREATEFXN") && SYMBOL_EXISTS("_FXN_F_nop") }
symbol { DUPLICATE("_FXN_F_nop") }
}

AddSymbolForTSK_VDELETEFXN {
action { ADD_SYMBOL("TSK_VDELETEFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("TSK_VDELETEFXN") && SYMBOL_EXISTS("_FXN_F_nop") }
symbol { DUPLICATE("_FXN_F_nop") }
}

AddSymbolForTSK_VEXITFXN {
action { ADD_SYMBOL("TSK_VEXITFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("TSK_VEXITFXN") && SYMBOL_EXISTS("_FXN_F_nop") }
symbol { DUPLICATE("_FXN_F_nop") }
}

AddSymbolFor_encoderSwi {
action { ADD_SYMBOL("_encoderSwi",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_encoderSwi") && SYMBOL_EXISTS("encoderSwi") }
symbol { DUPLICATE("encoderSwi") }
}

AddSymbolFor_decoderSwi {
action { ADD_SYMBOL("_decoderSwi",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_decoderSwi") && SYMBOL_EXISTS("decoderSwi") }
symbol { DUPLICATE("decoderSwi") }
}

AddSymbolFor_cancelerSwi {
action { ADD_SYMBOL("_cancelerSwi",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_cancelerSwi") && SYMBOL_EXISTS("cancelerSwi") }
symbol { DUPLICATE("cancelerSwi") }
}

PRD_A_TABBEG {
   action  { ADD_SYMBOL("PRD_A_TABBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("PRD_A_TABBEG")   && SECTION_EXISTS(".prd") }
   symbol  { START_OF_SECTION(".prd") } 
}

PRD_A_TABLEN {
   action  { ADD_SYMBOL("PRD_A_TABLEN",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("PRD_A_TABLEN")   && SECTION_EXISTS(".prd") }
   symbol  { ABS(SIZE_OF_SECTION(".prd")/OFFSET(32)) } 
}
GBL_stackbeg {
   action  { ADD_SYMBOL("GBL_stackbeg",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("GBL_stackbeg")   && SECTION_EXISTS(".stack") }
   symbol  { START_OF_SECTION(".stack") } 
}

ISRAM_B {
   action  { ADD_SYMBOL("ISRAM$B",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("ISRAM$B")   && SECTION_EXISTS(".ISRAM$heap") }
   symbol  { START_OF_SECTION(".ISRAM$heap") } 
}

STACK_SIZE {
   action  { ADD_SYMBOL("__STACK_SIZE",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__STACK_SIZE")  }
   symbol  { ABS(0x800) } 
}

ISRAM_B {
   action  { ADD_SYMBOL("ISRAM$B",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("ISRAM$B")  }
   symbol  { ABS(0x200) } 
}

ISRAM_L {
   action  { ADD_SYMBOL("ISRAM$L",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("ISRAM$L")  }
   symbol  { ABS(0x8000) } 
}

HWI_A_VECS {
   action  { ADD_SYMBOL("HWI_A_VECS",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("HWI_A_VECS")   && SECTION_EXISTS(".hwi_vec") }
   symbol  { START_OF_SECTION(".hwi_vec") } 
}

CLK_A_TABBEG {
   action  { ADD_SYMBOL("CLK_A_TABBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("CLK_A_TABBEG")   && SECTION_EXISTS(".clk") }
   symbol  { START_OF_SECTION(".clk") } 
}

GBL_stackend {
   action  { ADD_SYMBOL("GBL_stackend",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("GBL_stackend")   && SECTION_EXISTS(".stack") }
   symbol  { END_OF_SECTION(".stack") } 
}
AddSymbolGBLBoot {
action { ADD_SYMBOL("GBL_boot",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("GBL_boot") }
symbol { ABS(0x1) }
}

AddSymboldivu {
action { ADD_SYMBOL("__divu",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("__divu") }
symbol { ABS(0x0) }
}


AddSymbolFor_HWI_STKTOP {
action { ADD_SYMBOL("_HWI_STKTOP",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_HWI_STKTOP") && SYMBOL_EXISTS("GBL_stackbeg") }
symbol { DUPLICATE("GBL_stackbeg") }
}

AddSymbolFor_HWI_STKBOTTOM {
action { ADD_SYMBOL("_HWI_STKBOTTOM",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_HWI_STKBOTTOM") && SYMBOL_EXISTS("GBL_stackbeg") }
symbol { DUPLICATE("GBL_stackbeg") + OFFSET(0x7f8)}
}

bss {
   action  { ADD_SYMBOL("$bss",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("$bss")   && SECTION_EXISTS(".bss") }
   symbol  { START_OF_SECTION(".bss") } 
}

cinit {
   action  { ADD_SYMBOL("cinit",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("cinit")   && SECTION_EXISTS(".cinit") }
   symbol  { START_OF_SECTION(".cinit") } 
}

HST_A_TABBEG {
   action  { ADD_SYMBOL("HST_A_TABBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("HST_A_TABBEG")   && SECTION_EXISTS(".hst") }
   symbol  { START_OF_SECTION(".hst") } 
}

IDL_A_TABBEG {
   action  { ADD_SYMBOL("IDL_A_TABBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("IDL_A_TABBEG")   && SECTION_EXISTS(".idl") }
   symbol  { START_OF_SECTION(".idl") } 
}

HST_A_TABEND {
   action  { ADD_SYMBOL("HST_A_TABEND",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("HST_A_TABEND")   && SECTION_EXISTS(".hst") }
   symbol  { END_OF_SECTION(".hst") } 
}

___pinit__ {
   action  { ADD_SYMBOL("___pinit__",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("___pinit__")   && SECTION_EXISTS(".pinit") }
   symbol  { START_OF_SECTION(".pinit") } 
}

_SYS_PUTCBEG {
   action  { ADD_SYMBOL("_SYS_PUTCBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_SYS_PUTCBEG")   && SECTION_EXISTS(".trace") }
   symbol  { START_OF_SECTION(".trace") } 
}

_SYS_PUTCEND {
   action  { ADD_SYMBOL("_SYS_PUTCEND",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("_SYS_PUTCEND")   && SECTION_EXISTS(".trace") }
   symbol  { END_OF_SECTION(".trace") - OFFSET(0x1) } 
}

PIP_A_TABBEG {
   action  { ADD_SYMBOL("PIP_A_TABBEG",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("PIP_A_TABBEG")   && SECTION_EXISTS(".pip") }
   symbol  { START_OF_SECTION(".pip")  } 
}

PIP_A_TABLEN {
   action  { ADD_SYMBOL("PIP_A_TABLEN",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("PIP_A_TABLEN")   && SECTION_EXISTS(".pip") }
   symbol  { ABS(SIZE_OF_SECTION(".pip") / OFFSET(100)) } 
}

AddSymbolFor_KNL_swi {
action { ADD_SYMBOL("_KNL_swi",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_KNL_swi") && SYMBOL_EXISTS("KNL_swi") }
symbol { DUPLICATE("KNL_swi") }
}
AddSymbolFor_trace {
action { ADD_SYMBOL("_trace",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_trace") && SYMBOL_EXISTS("trace") }
symbol { DUPLICATE("trace") }
}
AddSymbolFor_CLK_REGS {
action { ADD_SYMBOL("_CLK_REGS",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_CLK_REGS") && SYMBOL_EXISTS("CLK_REGS") }
symbol { DUPLICATE("CLK_REGS") }
}
AddSymbolFor_PIP_A_TABLEN {
action { ADD_SYMBOL("_PIP_A_TABLEN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_PIP_A_TABLEN") && SYMBOL_EXISTS("PIP_A_TABLEN") }
symbol { DUPLICATE("PIP_A_TABLEN") }
}
AddSymbolFor_CLK_TIMERNUM {
action { ADD_SYMBOL("_CLK_TIMERNUM",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_CLK_TIMERNUM") && SYMBOL_EXISTS("CLK_TIMERNUM") }
symbol { DUPLICATE("CLK_TIMERNUM") }
}

AddSymbolFor_CLK_USETIMER {
action { ADD_SYMBOL("_CLK_USETIMER",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_CLK_USETIMER") && SYMBOL_EXISTS("CLK_USETIMER") }
symbol { DUPLICATE("CLK_USETIMER") }
}

AddSymbolFor_CLK_PRD {
action { ADD_SYMBOL("_CLK_PRD",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("_CLK_PRD") && SYMBOL_EXISTS("CLK_PRD") }
symbol { DUPLICATE("CLK_PRD") }
}

AddSymbolForCLK_F_gethtime {
action { ADD_SYMBOL("CLK_F_gethtime",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("CLK_F_gethtime") && SYMBOL_EXISTS("CLK_F_getshtime") }
symbol { DUPLICATE("CLK_F_getshtime") }
}

AddSymbolForCLK_TIMEFXN {
action { ADD_SYMBOL("CLK_TIMEFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("CLK_TIMEFXN") && SYMBOL_EXISTS("CLK_F_getshtime") }
symbol { DUPLICATE("CLK_F_getshtime") }
}

AddSymbolForIDL_CALIBRFXN {
action { ADD_SYMBOL("IDL_CALIBRFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("IDL_CALIBRFXN") && SYMBOL_EXISTS("IDL_F_stub") }
symbol { DUPLICATE("IDL_F_stub") }
}

AddSymbolForSWI_RUNFXN {
action { ADD_SYMBOL("SWI_RUNFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("SWI_RUNFXN") && SYMBOL_EXISTS("SWI_F_run") }
symbol { DUPLICATE("SWI_F_run") }
}

AddSymbolForSWI_EXECFXN {
action { ADD_SYMBOL("SWI_EXECFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("SWI_EXECFXN") && SYMBOL_EXISTS("SWI_F_iexec") }
symbol { DUPLICATE("SWI_F_iexec") }
}

AddSymbolForSWI_EHOOKFXN {
action { ADD_SYMBOL("SWI_EHOOKFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("SWI_EHOOKFXN") && SYMBOL_EXISTS("GBL_NULL") }
symbol { DUPLICATE("GBL_NULL") }
}

AddSymbolForSWI_IHOOKFXN {
action { ADD_SYMBOL("SWI_IHOOKFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("SWI_IHOOKFXN") && SYMBOL_EXISTS("GBL_NULL") }
symbol { DUPLICATE("GBL_NULL") }
}


AddSymbolForCLK_HOOKFXN {
action { ADD_SYMBOL("CLK_HOOKFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("CLK_HOOKFXN") && SYMBOL_EXISTS("HWI_F_dispatch") }
symbol { DUPLICATE("HWI_F_dispatch") }
}

IDLCalVectors {
   action  { VECTORIZE_BY_NAME(".idlcal") }
   trigger { SUBSECTION_EXISTS("*",".idlcal") } 
}

IDLCalVectorsBase {
  action  { ADD_SYMBOL("IDL_A_CALBEG",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___.idlcal") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___.idlcal") }
}

AddSymbolForPRD_THOOKFXN {
action { ADD_SYMBOL("PRD_THOOKFXN",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("PRD_THOOKFXN") && SYMBOL_EXISTS("_KNL_tick") }
symbol { DUPLICATE("_KNL_tick") }
}


/* embedded layout script */
SECTIONS
{
	. = 0x8000;
ER_RO : {}
	PUT_REMAINING_SECTIONS(CODE_SECTION);
	PUT_REMAINING_SECTIONS(RO_DATA_SECTION);
ER_RW : {}
	PUT_REMAINING_SECTIONS(DATA_SECTION);
ER_ZI : {. = ALIGN(4);}
	PUT_REMAINING_SECTIONS(BSS_SECTION);
}

