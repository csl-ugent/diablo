linker="ARM ADS 1.2"
fileformats="ELF"
architectures="ARM"

RegionTable1
{
	action { ADD_SUBSECTION("Linker","ER_RO","ArmRegionTable",CODE,4,36) }
	trigger { UNDEFINED_SYMBOL("Region$$Table$$Base") && (!LINKED_SYMBOL_EXISTS("BuildAttributes$$*EABIv2")) }
	section { CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0),CONST32(0) }
	address { SYMBOL("Region$$Table$$Base") }
}


RegionTable2 
{
   action { ADD_SUBSECTION("Linker","ER_RO","ArmRegionTable",CODE,4,16) }
   trigger { UNDEFINED_SYMBOL("Region$$Table$$Base") && LINKED_SYMBOL_EXISTS("BuildAttributes$$*EABIv2") && LINKED_SYMBOL_EXISTS("Region$$Table$$Base") &&  SECTION_EXISTS("ER_ZI") }
   section { START_OF_SECTION("ER_ZI"),
             START_OF_SECTION("ER_ZI"),
             SIZE_OF_SECTION("ER_ZI"),
             /* this address for some reason must not be adjusted for Thumb, if necessary that's done in the __scatterload code */
             RELOCATED32(0x0,"__scatterload_zeroinit",0,0,0,"S00A00+=M-\l*w\s0000$")
   } 
   address { SYMBOL("Region$$Table$$Base") } 
}

 
RegionTableDummy
{
   action { ADD_SUBSECTION("Linker","ER_RW","ArmRegionTable",DATA,4,0) }
   trigger { UNDEFINED_SYMBOL("Region$$Table$$Base") && LINKED_SYMBOL_EXISTS("BuildAttributes$$*EABIv2") && LINKED_SYMBOL_EXISTS("Region$$Table$$Base") && !SECTION_EXISTS("ER_ZI") }
   address {  SYMBOL("Region$$Table$$Base") }
}

RegionTableBase_Data_Symbol
{
   action { ADD_LOCAL_SYMBOL("$d", SYMBOL_TYPE_NOTYPE)
   }
   trigger { UNDEFINED_SYMBOL("Region$$Table$$Base") 
   && SUBSECTION_EXISTS("Linker","ArmRegionTable") }
   symbol  {  START_OF_SUBSECTION("Linker","ArmRegionTable") } 
}
RegionTableBase 
{
   action { ADD_SYMBOL("Region$$Table$$Base",SYMBOL_TYPE_NOTYPE)
   }
   trigger { UNDEFINED_SYMBOL("Region$$Table$$Base") 
   && SUBSECTION_EXISTS("Linker","ArmRegionTable") }
   symbol  {  START_OF_SUBSECTION("Linker","ArmRegionTable") } 
}


BuildAttributes
{
  action { ADD_SYMBOL(MATCHED_NAME()) }
  trigger { LINKED_SYMBOL_EXISTS("BuildAttributes$$*") }
  symbol { ABS(0x0) }
}


RegionTableLimit {
   action  { ADD_SYMBOL("Region$$Table$$Limit",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("Region$$Table$$Limit") && SUBSECTION_EXISTS("Linker","ArmRegionTable") }
   symbol  { END_OF_SUBSECTION("Linker","ArmRegionTable") } 
}


ZISectionTable {
   action { ADD_SUBSECTION("Linker","ER_RO","ArmZISectionTable",CODE,4,24) }
   trigger { UNDEFINED_SYMBOL("ZISection$$Table$$Base") }
   section { CONST32(0),CONST32(0),CONST32(0),CONST32(0),START_OF_SECTION("ER_ZI"),SIZE_OF_SECTION("ER_ZI") }
   address { SYMBOL("ZISection$$Table$$Base") }
}


ZISectionTable_Data_Symbol {
  action { ADD_LOCAL_SYMBOL("$d", SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("ZISection$$Table$$Base") && SUBSECTION_EXISTS("Linker","ArmZISectionTable") }
 symbol  { START_OF_SUBSECTION("Linker","ArmZISectionTable") }
}


ZISectionTableBase {
 action  { ADD_SYMBOL("ZISection$$Table$$Base",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("ZISection$$Table$$Base") && SUBSECTION_EXISTS("Linker","ArmZISectionTable") }
 symbol  { START_OF_SUBSECTION("Linker","ArmZISectionTable") }
}

ZISectionTableLimit {
 action  { ADD_SYMBOL("ZISection$$Table$$Limit",SYMBOL_TYPE_NOTYPE) }
 trigger { UNDEFINED_SYMBOL("ZISection$$Table$$Limit") && SUBSECTION_EXISTS("Linker","ArmZISectionTable") }
 symbol  { END_OF_SUBSECTION("Linker","ArmZISectionTable") }
}

ImageRWLimit  {
   action  { ADD_SYMBOL("Image$$RW$$Limit",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("Image$$RW$$Limit") && SECTION_EXISTS("ER_ZI") } 
   symbol  { END_OF_SECTION("ER_ZI") }
}

ImageZILimit  {
   action  { ADD_SYMBOL("Image$$ZI$$Limit",SYMBOL_TYPE_NOTYPE) }
   trigger { WEAK_UNDEFINED_SYMBOL("Image$$ZI$$Limit") && SECTION_EXISTS("ER_ZI") } 
   symbol  { END_OF_SECTION("ER_ZI") }
}

ImageRWLimitnoZI  {
   action  { ADD_SYMBOL("Image$$RW$$Limit",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("Image$$RW$$Limit") && SECTION_EXISTS("ER_RW") } 
   symbol  { END_OF_SECTION("ER_RW") }
}

ImageRWLimitnoRW  {
   action  { ADD_SYMBOL("Image$$RW$$Limit",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("Image$$RW$$Limit")   }
   symbol  { END_OF_SECTION("ER_RO")  }
}

dsohandle  {
   action  { ADD_SYMBOL_NEW("__dso_handle",20) }
   trigger { LINKED_SYMBOL_EXISTS("__dso_handle") }
   symbol  { START_OF_SECTION("ER_RO")  }
}



ImageZILimit  {
	action  { ADD_SYMBOL("Image$$ZI$$Limit",SYMBOL_TYPE_NOTYPE) }
	trigger { UNDEFINED_SYMBOL("Image$$ZI$$Limit") && SECTION_EXISTS("ER_ZI") }
	symbol  { END_OF_SECTION("ER_ZI") }
}

AvoidGetTypeIdErrors {
  action  { ADD_SYMBOL("__get_typeid",SYMBOL_TYPE_FUNCTION) }
  trigger { UNDEFINED_SYMBOL("__get_typeid") }
  symbol  { ABS(0x0) }
}

AvoidDynamicCastErrors {
  action  { ADD_SYMBOL("__dynamic_cast",SYMBOL_TYPE_FUNCTION) }
  trigger { UNDEFINED_SYMBOL("__dynamic_cast") }
  symbol  { ABS(0x0) }
}

AvoidAebUnwindCppStuff {
  action  { ADD_SYMBOL("__aeabi_unwind_cpp_pr0",SYMBOL_TYPE_FUNC) }
  trigger { UNDEFINED_SYMBOL("__aeabi_unwind_cpp_pr0") }
  symbol  { ABS(0x0) }
}

Avoidcxa_call_unexpectedStuff {
  action  { ADD_SYMBOL("__cxa_call_unexpected",SYMBOL_TYPE_FUNC) }
  trigger { UNDEFINED_SYMBOL("__cxa_call_unexpected") }
  symbol  { ABS(0x0) }
}

DefaultSignalHandler {
  action  { ADD_SYMBOL("__SIG_DFL",SYMBOL_TYPE_NOTYPE) }
  trigger { UNDEFINED_SYMBOL("__SIG_DFL") }
  symbol  { ABS(0xffffffff) }
}

ErrorSignalHandler {
   action  { ADD_SYMBOL("__SIG_ERR",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__SIG_ERR") }
   symbol  { ABS(0xfffffffe) }
}

IgnoreSignalHandler {
   action  { ADD_SYMBOL("__SIG_IGN",SYMBOL_TYPE_NOTYPE) }
   trigger { UNDEFINED_SYMBOL("__SIG_IGN") }
   symbol  { ABS(0xfffffffd) }
}



InitArrayVectors {
   action  { VECTORIZE_BY_NAME(".init_array") }
   trigger { SUBSECTION_EXISTS("*",".init_array") } 
}

InitArrayVectorsBase {
  action  { ADD_SYMBOL(".init_array$$Base",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

InitArrayVectorsBase2 {
  action  { ADD_SYMBOL("SHT$$INIT_ARRAY$$Base",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

InitArrayVectorsLimit {
  action  { ADD_SYMBOL(".init_array$$Limit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

InitArrayVectorsLimit2 {
  action  { ADD_SYMBOL("SHT$$INIT_ARRAY$$Limit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___.init_array") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR___.init_array") }
}

ConstructorVectors {
   action  { VECTORIZE_BY_NAME("C$$pi_ctorvec") }
   trigger { SUBSECTION_EXISTS("*","C$$pi_ctorvec") } 
}

ConstructorVectorsBase {
  action  { ADD_SYMBOL("C$$pi_ctorvec$$Base",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___C$$pi_ctorvec") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___C$$pi_ctorvec") }
}

ConstructorVectorsBase {
  action  { ADD_SYMBOL("C$$pi_ctorvec$$Limit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___C$$pi_ctorvec") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR___C$$pi_ctorvec") }
}


DestructorVectors {
   action  { VECTORIZE_BY_NAME("C$$pi_dtorvec") }
   trigger { SUBSECTION_EXISTS("*","C$$pi_dtorvec") } 
}

DestructorVectorsBase {
  action  { ADD_SYMBOL("C$$pi_dtorvec$$Base",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___C$$pi_dtorvec") }
  symbol  { START_OF_SUBSECTION("Linker","VECTOR___C$$pi_dtorvec") }
}

DestructorVectorsBase {
  action  { ADD_SYMBOL("C$$pi_dtorvec$$Limit",SYMBOL_TYPE_NOTYPE) }
  trigger { SUBSECTION_EXISTS("Linker","VECTOR___C$$pi_dtorvec") }
  symbol  { END_OF_SUBSECTION("Linker","VECTOR___C$$pi_dtorvec") }
}

ATVeneerLLong {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$L$$*") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) != 0xe51ff004) }
  action { ADD_SUBSECTION("Linker","ER_RO",CONCAT(MATCHED_NAME(),"_section"),CODE,4,12) }
  section { CONST32(0xe59fc000), 
            CONST32(0xe12fff1c),
	    RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),11,0) , 0, 0, 0, "S00A00+S00M|\l*w\s0000$")
  }
  address { SYMBOL(MATCHED_NAME()) }
}

/* 0xe51ff004 == ldr pc, [pc,#-4] */
ATVeneerLShort {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$L$$*") && (READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()) == 0xe51ff004) }
  action { ADD_SUBSECTION("Linker","ER_RO",CONCAT(MATCHED_NAME(),"_section"),CODE,4,8) }
  section { CONST32(0xe51ff004), 
	    RELOCATED32(0x0, SUBSTRING(MATCHED_NAME(),11,0) , 0, 0, 0, "S00A00+S00M|\l*w\s0000$")
  }
  address { SYMBOL(MATCHED_NAME()) }
}


ATVeneerLDataSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$L$$*") }
  action { ADD_LOCAL_SYMBOL("$d", SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) + OFFSET(0x8)}
}

ATVeneerI {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$I$$*") }
  action { ADD_SUBSECTION("Linker","ER_RO",CONCAT(MATCHED_NAME(),"_section"),CODE,4,8) }
  section { CONST32(0xe28fc001),
            CONST32(0xe12fff1c)
  }
  address { SYMBOL(MATCHED_NAME()) }
}

ATVeneerSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$*") }
  action { ADD_SYMBOL(MATCHED_NAME()) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) }

}

ATVeneerArmSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$AT$*") }
  action { ADD_LOCAL_SYMBOL("$a", SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) }
}

TAVeneerI {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$TA$I$$*") }
  action { ADD_SUBSECTION("Linker","ER_RO",CONCAT(MATCHED_NAME(),"_section"),CODE,4,4) }
  section { CONST32(0x46c04778) }
  address { SYMBOL(MATCHED_NAME()) }

}

TAVeneerS {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$TA$S$$*") }
  action { ADD_SUBSECTION("Linker","ER_RO",CONCAT(MATCHED_NAME(),"_section"),CODE,4,8) }
  section { CONST32(0x46c04778),
  	    RELOCATED32(0xeaffffff, SUBSTRING(MATCHED_NAME(),11,0), 0, 0, 0, "S00A00+P-s0008-\= s0002 & % s0002 > i00ffffff &=l iff000000 &| S00M?i10000000|ifeffffff& } s0017 < |: }* ! w\l i00ffffff &-$")
  }
  address { SYMBOL(MATCHED_NAME()) }

}

TAVeneerSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$TA$*") }
  action { ADD_SYMBOL(MATCHED_NAME()) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) }

}
TAVeneerThumbSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$TA$*") }
  action { ADD_LOCAL_SYMBOL("$t", SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) }
}
TAVeneerSArmSymbol {
  trigger { LINKED_SYMBOL_EXISTS("$Ven$TA$S$$*") }
  action { ADD_LOCAL_SYMBOL("$a", SYMBOL_TYPE_NOTYPE) }
  symbol { START_OF_SUBSECTION("Linker",CONCAT(MATCHED_NAME(),"_section")) + OFFSET(0x4) }
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
