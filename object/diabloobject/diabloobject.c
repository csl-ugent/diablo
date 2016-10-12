/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define GENERATE_CLASS_CODE
#include <diabloobject.h>

/* Relocatable {{{ */
t_string
IoModifierRelocatable (t_const_string modifiers, va_list * ap)
{
  t_string ret;
  t_string_array *array;
  t_relocatable *relocatable = va_arg (*ap, t_relocatable *);

  array = StringArrayNew ();

  StringArrayAppendString (array, StringDup ("Relocatable "));

  switch (RELOCATABLE_RELOCATABLE_TYPE(relocatable))
  {
    case RT_SECTION:
      StringArrayAppendString (array, StringDup ("Type=RT_SECTION Name="));
      StringArrayAppendString (array, StringDup (SECTION_NAME(T_SECTION(relocatable))));
      StringArrayAppendString (array, StringDup ("in object"));
      StringArrayAppendString (array, StringDup (OBJECT_NAME(SECTION_OBJECT(T_SECTION(relocatable)))));
      break;
    case RT_SUBSECTION:
      StringArrayAppendString (array, StringDup ("Type=RT_SUBSECTION Name="));
      StringArrayAppendString (array, StringDup (SECTION_NAME(T_SECTION(relocatable))));

      switch (SECTION_TYPE(T_SECTION(relocatable)))
      {
        case CODE_SECTION:
          StringArrayAppendString (array, StringDup (" (type = CODE)"));
          break;
        case RODATA_SECTION:
          StringArrayAppendString (array, StringDup (" (type = RODATA)"));
          break;
        case DATA_SECTION:
          StringArrayAppendString (array, StringDup (" (type = DATA)"));
          break;
        case BSS_SECTION:
          StringArrayAppendString (array, StringDup (" (type = BSS)"));
          break;
      }

      StringArrayAppendString (array, StringDup ("in object"));
      StringArrayAppendString (array, StringDup (OBJECT_NAME(SECTION_OBJECT(T_SECTION(relocatable)))));
      break;
    case RT_INS:
      StringArrayAppendString (array, StringDup ("Type=RT_INS"));
      break;
    case RT_CODEBYTE:
      StringArrayAppendString (array, StringDup ("Type=RT_CODEBYTE"));
      break;
    case RT_BBL:
      StringArrayAppendString (array, StringDup ("Type=RT_BBL"));
      break;
    default:
      FATAL(("Trying to print a relocatable with unknown relocatable type %d (0x%x)!", RELOCATABLE_RELOCATABLE_TYPE(relocatable), RELOCATABLE_RELOCATABLE_TYPE(relocatable)));
  }

  StringArrayAppendString (array, StringIo ("CADDRESS=@G OLD_ADDRESS=@G", RELOCATABLE_CADDRESS(relocatable), RELOCATABLE_OLD_ADDRESS(relocatable)));

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

/* }}} */
/* Relocs {{{ */
t_string
IoModifierReloc (t_const_string modifiers, va_list * ap)
{
  t_string ret;
  t_string_array *array;
  t_reloc *reloc = va_arg (*ap, t_reloc *);
  t_uint32 i;

  array = StringArrayNew ();

  StringArrayAppendString (array, StringIo ("Reloc\nFrom    : @T\n",RELOC_FROM(reloc)));
  StringArrayAppendString (array, StringIo ("FromOff    : @G\n\n", RELOC_FROM_OFFSET(reloc)));


  for (i=0; i<RELOC_N_TO_SYMBOLS(reloc); i++)
  {
    StringArrayAppendString (array, StringIo ("ToSym %2d: ",i));
    StringArrayAppendString (array, StringDup (SYMBOL_NAME(RELOC_TO_SYMBOL(reloc)[i]) ? SYMBOL_NAME(RELOC_TO_SYMBOL(reloc)[i]) : "<nameless symbol>"));
    StringArrayAppendString (array, StringDup (" Offset= "));
    StringArrayAppendString (array, StringIo ("@G\n", SYMBOL_OFFSET_FROM_START(RELOC_TO_SYMBOL(reloc)[i])));
  }
  
  for (i=0; i<RELOC_N_TO_RELOCATABLES(reloc); i++)
  {
    StringArrayAppendString (array, StringIo ("ToRel %2d: @T\n", i, RELOC_TO_RELOCATABLE(reloc)[i]));
    StringArrayAppendString (array, StringIo ("ToOff %2d: @G\n", i, RELOC_TO_RELOCATABLE_OFFSET(reloc)[i]));
  }

  if (RELOC_EDGE(reloc))
    StringArrayAppendString (array, StringDup ("Has edge"));

  StringArrayAppendString (array, StringConcat3 ("\nCODE=",RELOC_CODE(reloc),"\n"));

  for (i=0; i<RELOC_N_ADDENDS(reloc); i++)
  {
    StringArrayAppendString (array, StringIo ("Addend %2d: @G\n", i, RELOC_ADDENDS(reloc)[i]));
  }
  if (RELOC_HELL(reloc)) StringArrayAppendString (array, StringConcat2 ("\nHELL\n",RELOC_CODE(reloc)));
  else  StringArrayAppendString (array, StringConcat2 ("\nNOHELL\n",RELOC_CODE(reloc)));

  ret = StringArrayJoin (array, "");
  StringArrayFree (array);
  return ret;
}

/* }}} */
/* Symbols {{{ */
t_string
IoModifierSymbol (t_const_string modifiers, va_list * ap)
{
  t_symbol *symbol = va_arg (*ap, t_symbol *);
  t_string_array *array = StringArrayNew ();
  t_string ret;

  if ((SYMBOL_ORDER(symbol)<0)&&(SYMBOL_SEARCH(symbol)==FALSE))
    StringArrayAppendString (array, StringDup ("local"));
  else
    StringArrayAppendString (array, StringDup ("global"));

  StringArrayAppendString (array, StringDup ("symbol"));
  StringArrayAppendString (array, SYMBOL_NAME(symbol) ? StringDup (SYMBOL_NAME(symbol)) : StringDup ("<nameless symbol>"));
  
  StringArrayAppendString (array, StringDup (SYMBOL_CODE(symbol)));

  if (SYMBOL_DUP(symbol)==TRUE)
    StringArrayAppendString (array, StringDup ("can have duplicates"));

  StringArrayAppendString (array, StringIo ("search %d",SYMBOL_SEARCH(symbol)));
  StringArrayAppendString (array, StringIo ("order %d",SYMBOL_ORDER(symbol)));
  StringArrayAppendString (array, StringIo ("dup %d",SYMBOL_DUP(symbol)));
  StringArrayAppendString (array, StringIo ("Base = @T", SYMBOL_BASE(symbol)));
  StringArrayAppendString (array, StringIo ("Offset = @G", SYMBOL_OFFSET_FROM_START(symbol)));
  StringArrayAppendString (array, StringIo ("Flags = 0x%x", SYMBOL_FLAGS(symbol)));
  
  if (SYMBOL_TENTATIVE(symbol))
    StringArrayAppendString (array, StringIo("TENTATIVE = %s",SYMBOL_TENTATIVE(symbol)));

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

/* }}} */

static int object_module_usecount = 0;

void
DiabloObjectCmdlineVersion ()
{
  VERBOSE(0, ("DiabloObject version %s", DIABLOOBJECT_VERSION));
#ifdef BIT32ADDRSUPPORT
#ifdef BIT64ADDRSUPPORT
  VERBOSE(0, ("  with support for both 32-bit and 64-bit architectures"));
#else
  VERBOSE(0, ("  with support for 32-bit architectures"));
#endif
#else
  VERBOSE(0, ("  with support for 64-bit architectures"));
#endif
  VERBOSE(0, ("  build linker modules:"));
#ifdef DIABLOOBJECT_ARMADSSUPPORT
  VERBOSE(0, ("    armads"));
#endif
#ifdef DIABLOOBJECT_BINUTILSSUPPORT
  VERBOSE(0, ("    binutils"));
#endif
#ifdef DIABLOOBJECT_TILINKERSUPPORT
  VERBOSE(0, ("    tilinker"));
#endif
  VERBOSE(0, ("  build archive modules:"));
#ifdef DIABLOOBJECT_ARSUPPORT 
  VERBOSE(0, ("    ar"));
#endif
  VERBOSE(0, ("  build object file modules:"));
#ifdef DIABLOOBJECT_ELFSUPPORT 
  VERBOSE(0, ("    ELF"));
#endif
#ifdef DIABLOOBJECT_TICOFFSUPPORT 
  VERBOSE(0, ("    TICOFF"));
#endif
}

void
DiabloObjectInit (int argc, char **argv)
{
  if (!object_module_usecount)
  {
    DiabloSupportInit (argc, argv);
    DiabloObjectCmdlineInit ();
    OptionParseCommandLine (diabloobject_option_list, argc, argv, FALSE);
    OptionGetEnvironment (diabloobject_option_list);
    DiabloObjectCmdlineVerify ();
    OptionDefaults (diabloobject_option_list);
    IoModifierAdd ('@', 'R', "", IoModifierReloc);
    IoModifierAdd ('@', 'S', "", IoModifierSymbol);
    IoModifierAdd ('@', 'T', "", IoModifierRelocatable);

    ObjectInit ();
    SymbolInit ();
    ArchivesInit ();
  }
  object_module_usecount++;
}

void
DiabloObjectFini ()
{
  object_module_usecount--;
  if (!object_module_usecount)
  {
    ArchivesFini ();
    SymbolFini ();
    ObjectFini ();
    MapTableFree ();
    CompressedMapTableFree ();
    DiabloObjectCmdlineFini ();
    DiabloSupportFini ();
  }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
