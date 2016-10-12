

AddSymbolFor__SYSMEM_SIZE {
action { ADD_SYMBOL("__SYSMEM_SIZE",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("__SYSMEM_SIZE") }
symbol { DUPLICATE_ORIG("__SYSMEM_SIZE") }
}

AddSymbolFor__STACK_SIZE {
action { ADD_SYMBOL("__STACK_SIZE",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("__STACK_SIZE") }
symbol { DUPLICATE_ORIG("__STACK_SIZE") }
}

AddSymbolForbinit {
action { ADD_SYMBOL("binit",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("binit") }
symbol { DUPLICATE_ORIG("binit") }
}

AddSymbolForpinit {
action { ADD_SYMBOL("pinit",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("pinit") }
symbol { DUPLICATE_ORIG("pinit") }
}

AddSymbolForcinit {
action { ADD_SYMBOL("cinit",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("cinit") }
symbol { DUPLICATE_ORIG("cinit") }
}

AddSymbolFor___c_args__ {
action { ADD_SYMBOL("___c_args__",SYMBOL_TYPE_NOTYPE) }
trigger{ UNDEFINED_SYMBOL("___c_args__") }
symbol { DUPLICATE_ORIG("___c_args__") }
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

