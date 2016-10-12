/*=============================================================================
  Diablo PE/COFF back-end.
  Based on Microsoft's PE/COFF Specification v8.2 (September 2010)

  (c) 2011-2013 Stijn Volckaert (svolckae@elis.ugent.be)

  Current Progress for the M$ Back-ends:
  * MS Library Back-end (diabloar.c): DONE!
  * MS Linker Map Back-end (diablomsil.c): DONE!
  * COFF Support:
    + Identifying and reading sections: DONE!
    + Reading and adding symbols from the symbol table: DONE!
    + Reading and adding relocations: 75%
  * COFF Import Support:
    + Reading import name/library: DONE!
    + Construct fake import objects: DONE!
    + Build .idata sections: 75%
  * PE Support:
    + Identifying and reading sections: DONE!
    + Reading and adding dynamic symbols from import table: DONE!
    + Reading and adding static symbols from export table: 0%
    + Write support: 0%

  TODO:
  * Must-haves (in order of importance):
    + Resource support:
      Virtually every exe file has at least one embedded resource
      (the manifest file)
    + TLS Support
    + Dynamic base support
    + Safe SEH support
    + Lib constructor/destructor support

  * Nice to haves (in order):
    + Signed binary support
    + Copyright header support
    + Export forwarding (seldom used. Only in system dlls afaik)

  * Won't bother:
    + Exception directory support (not used on x86, x86_64)
    + Managed binaries (this would be a back-end on its own)
    + Debug info
    + Bound imports (ancient)
    + Delayed imports (ancient)
    + Global Ptr (probably only needed if you want to run windows
      on a gameboy)
=============================================================================*/

#include <diabloobject.h>
#include "diablopecoff.h"

/*-----------------------------------------------------------------------------
  PeCoffIsValidMachineType - Used for COFF-file identification
-----------------------------------------------------------------------------*/
t_bool
PeCoffIsValidMachineType(WORD M)
{
  if (M == IMAGE_FILE_MACHINE_I386
    || M == IMAGE_FILE_MACHINE_ARM
    || M == IMAGE_FILE_MACHINE_THUMB
    || M == IMAGE_FILE_MACHINE_AMD64)
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------------------
  PeCoffIsValidCharacteristics - Used for COFF-file identification
-----------------------------------------------------------------------------*/
t_bool
PeCoffIsValidCharacteristics(WORD C)
{
  /* check if any PE-only flags are set */
  if ((C & IMAGE_FILE_RELOCS_STRIPPED)
    || (C & IMAGE_FILE_EXECUTABLE_IMAGE)
    /* technically the following 2 flags are valid COFF flags but they have been deprecated for a long time and should therefore be unflagged */
    || (C & IMAGE_FILE_LINE_NUMS_STRIPPED)
    || (C & IMAGE_FILE_LOCAL_SYMS_STRIPPED)
    /* Deprecated since Win2K. Should not be flagged */
    || (C & IMAGE_FILE_AGGRESIVE_WS_TRIM)
    /* Reserved flag */
    || (C & 0x0040)
    /* Deprecated */
    || (C & IMAGE_FILE_BYTES_REVERSED_LO)
    || (C & IMAGE_FILE_BYTES_REVERSED_HI)
    /* Image only */
    || (C & IMAGE_FILE_DEBUG_STRIPPED))
    return FALSE;
  return TRUE;
}

/*-----------------------------------------------------------------------------
  IsPe - Checks for the DOS and NT headers and signatures

  PE File lay-out:
  +-----------------+      +--> +-----------------+
  | DOS Header      |      |    | PE Signature    | => 'P' 'E' '\0' '\0'
  | ...             |      |    +-----------------+
  +-----------------+ -----+    | COFF Header     | => aka _IMAGE_FILE_HEADER
  | NT Headers      |           | ...             |
  | ...             |           +-----------------+
  +-----------------+ -----+    | Optional Header | => aka _IMAGE_OPTIONAL_HEADER
  | Section Headers |      |    | ...             |
  . ...             .      |    | DataDirectories |
  .                 .      +--> | ...             |
-----------------------------------------------------------------------------*/
t_bool
IsPe(FILE* fp)
{
  t_uint32  file_size       = 0;
  void*     file_buffer     = NULL;
  t_bool    result          = FALSE;
  t_uint32  stored_pointer  = ftell(fp);

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp) - stored_pointer;
  fseek(fp, stored_pointer, SEEK_SET);

  file_buffer = Malloc(file_size);
  fread(file_buffer, 1, file_size, fp);

  if (file_size > sizeof(IMAGE_DOS_HEADER))
  {
    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)file_buffer;
    if (pDOSHeader->e_magic == IMAGE_DOS_SIGNATURE
      && pDOSHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) < file_size)
    {
      PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)file_buffer + pDOSHeader->e_lfanew);
      if (pNTHeader->Signature == LOWORD(IMAGE_NT_SIGNATURE)
        && pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC)
      {
        if (PeCoffIsValidMachineType(pNTHeader->FileHeader.Machine)
          && pNTHeader->FileHeader.NumberOfSymbols >= 0
          && pNTHeader->FileHeader.PointerToSymbolTable + sizeof(IMAGE_SYMBOL)*pNTHeader->FileHeader.NumberOfSymbols <= file_size
          && pNTHeader->FileHeader.NumberOfSections >= 0)
        {
          result = TRUE;
        }
      }
    }
  }

  fseek(fp, stored_pointer, SEEK_SET);
  Free(file_buffer);
  return result;
}

/*-----------------------------------------------------------------------------
  IdentifyCoff - Checks for the COFF signature

  returns:
  * 0 if not COFF
  * 1 if normal COFF
  * 2 if COFF long import
-----------------------------------------------------------------------------*/
int
IdentifyCoff(FILE* fp)
{
  t_uint32  file_size       = 0;
  void*     file_buffer     = NULL;
  int       result          = 0;
  t_uint32  stored_pointer  = ftell(fp);

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp) - stored_pointer;
  fseek(fp, stored_pointer, SEEK_SET);

  file_buffer = Malloc(file_size);
  fread(file_buffer, 1, file_size, fp);

  if (file_size > sizeof(IMAGE_FILE_HEADER))
  {
    PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)file_buffer;
    if (PeCoffIsValidMachineType(pFileHeader->Machine)
      && PeCoffIsValidCharacteristics(pFileHeader->Characteristics)
      && pFileHeader->NumberOfSymbols >= 0
      && pFileHeader->PointerToSymbolTable + sizeof(IMAGE_SYMBOL)*pFileHeader->NumberOfSymbols <= file_size
      && pFileHeader->NumberOfSections >= 0)
    {
      /* this is definitely COFF, now check if it's a normal COFF object or a COFF LONG IMPORT object */
      /* check if there are .idata sections! */
      t_uint32 i = 0;
      PIMAGE_SECTION_HEADER section_header = (PIMAGE_SECTION_HEADER)((unsigned int)file_buffer + sizeof(IMAGE_FILE_HEADER));

      result = 1;

      for (i = 0; i < pFileHeader->NumberOfSections; ++i, ++section_header)
      {
        if (section_header->Characteristics & IMAGE_SCN_LNK_REMOVE
          || section_header->Characteristics & IMAGE_SCN_MEM_DISCARDABLE
          || section_header->Name[0] == '/')
        {
          continue;
        }

        if (section_header->Name[0] == '.'
          && section_header->Name[1] == 'i'
          && section_header->Name[2] == 'd'
          && section_header->Name[3] == 'a'
          && section_header->Name[4] == 't'
          && section_header->Name[5] == 'a'
          && section_header->Name[6] == '$'
          && section_header->Name[7] != '3')
        {
          result = 2;
          break;
        }
      }
    }
  }

  fseek(fp, stored_pointer, SEEK_SET);
  Free(file_buffer);
  return result;
}

/*-----------------------------------------------------------------------------
  IsCoff -
-----------------------------------------------------------------------------*/
t_bool
IsCoff(FILE* fp)
{
  return (IdentifyCoff(fp) == 1) ? TRUE : FALSE;
}

/*-----------------------------------------------------------------------------
  IsCoffShortImport - Checks if the object is a COFF Import Object
-----------------------------------------------------------------------------*/
t_bool
IsCoffShortImport(FILE* fp)
{
  t_uint32              stored_pointer = ftell(fp);
  IMPORT_OBJECT_HEADER  import_header;

  int ret = fread(&import_header, sizeof(IMPORT_OBJECT_HEADER), 1, fp);
  fseek(fp, stored_pointer, SEEK_SET);

  if (ret != 1)
    return FALSE;

  if (import_header.Sig1 == IMAGE_FILE_MACHINE_UNKNOWN
    && import_header.Sig2 == IMPORT_OBJECT_HDR_SIG2
    && import_header.Reserved == 0)
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------------------
  IsCoffLongImport - Checks for the COFF signature
-----------------------------------------------------------------------------*/
t_bool
IsCoffLongImport(FILE* fp)
{
  return (IdentifyCoff(fp) == 2) ? TRUE : FALSE;
}

/*-----------------------------------------------------------------------------
  IsPeCoff - Checks if the file should be handled by this object back-end
-----------------------------------------------------------------------------*/
t_bool
IsPeCoff(FILE* fp)
{
  if (IsPe(fp))
    return TRUE;
  if (IsCoff(fp))
    return TRUE;
  if (IsCoffShortImport(fp))
    return TRUE;
  if (IsCoffLongImport(fp))
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------------------
  CoffFindImportObject - We associate one dummy object with every imported
  library. This dummy object should be generated on the fly, rather than
  generating it the first time we read the linker map.

  If we generate on the fly, Diablo will not attempt to read the object.
-----------------------------------------------------------------------------*/
t_object*
CoffFindImportObject(t_string import_lib_name, t_object* parent_pe_object)
{
  t_object* obj         = NULL;
  t_object* tmp1        = NULL;
  t_object* tmp2        = NULL;
  t_string  object_name = NULL;

  ASSERT(parent_pe_object, ("Couldn't find a parent object!"));

  object_name = (t_string)Malloc(strlen(import_lib_name) + strlen("$$import_object$") + 1);
  sprintf(object_name, "$%s$import_object$", import_lib_name);
  StringToUpper(object_name);

  /* see if we have already generated the import object */
  _(OBJECT_FOREACH_SUBOBJECT(parent_pe_object, tmp1, tmp2)
  {
    if (!strcmp(OBJECT_NAME(tmp1), object_name))
    {
      obj = tmp1;
      break;
    }
  })

  /* generate on the fly and add as subobject */
  if (!obj)
  {
    VERBOSE(0, ("Creating fake import object: %s", object_name));
    obj = ObjectNewCached (object_name, parent_pe_object);
    if (OBJECT_MAPPED_LAST(parent_pe_object) == NULL)
    {
      OBJECT_SET_MAPPED_LAST(parent_pe_object, obj);
      OBJECT_SET_MAPPED_FIRST(parent_pe_object, obj);
    }
    else
    {
      OBJECT_SET_NEXT(OBJECT_MAPPED_LAST(parent_pe_object), obj);
      OBJECT_SET_MAPPED_LAST(parent_pe_object, obj);
    }
    OBJECT_SET_NEXT(obj, NULL);
    OBJECT_SET_RELOC_TABLE(obj, RelocTableNew (obj));
    OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew (obj));
    OBJECT_SET_SWITCHED_ENDIAN(obj, OBJECT_SWITCHED_ENDIAN(parent_pe_object));
    OBJECT_SET_UNDEF_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*UNDEF*"));
  }

  Free(object_name);
  return obj;
}

/*-----------------------------------------------------------------------------
  CoffGetSectionType -
-----------------------------------------------------------------------------*/
void
CoffGetSectionType(DWORD characteristics, char* section_type, t_bool* section_is_ro)
{
  if (characteristics & IMAGE_SCN_CNT_CODE)
  {
    *section_type  = CODE_SECTION;
    *section_is_ro = TRUE;
  }
  else if (characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
  {
    if (!characteristics & IMAGE_SCN_MEM_WRITE)
    {
      *section_type   = RODATA_SECTION;
      *section_is_ro  = TRUE;
    }
    else
    {
      *section_type   = DATA_SECTION;
      *section_is_ro  = FALSE;
    }
  }
  else if (characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
  {
    *section_type   = BSS_SECTION;
    *section_is_ro  = FALSE;
  }
}

/*-----------------------------------------------------------------------------
  CoffGetSectionAlignment -
-----------------------------------------------------------------------------*/
void
CoffGetSectionAlignment(DWORD characteristics, t_uint32* section_alignment)
{
  characteristics = characteristics & IMAGE_SCN_ALIGN_MASK;
  *section_alignment = 1 << ((characteristics >> 20) - 1);
}

/*-----------------------------------------------------------------------------
  CoffRead - Used to read VC++ generated object files. Note that VC++ will NOT
  generate proper COFF files when the VC Link-Time Optimizer is enabled!

  COFF File lay-out:
  +-----------------+
  | COFF Header     |
  | ...             |
  +-----------------+
  | Section Headers |
  | ...             |
  +-----------------+
  | Raw Data        |
  | ...             |
  .                 .
-----------------------------------------------------------------------------*/
void
CoffRead(FILE * fp, t_object * obj, t_bool read_debug)
{
  t_uint32              file_size         = 0;
  void*                 file_buffer       = NULL;
  t_uint32              stored_pointer    = ftell(fp);
  t_uint32              section_count     = 0;
  t_uint32              symbol_count      = 0;
  t_uint32              i                 = 0;
  t_uint32              j                 = 0;
  PIMAGE_FILE_HEADER    coff_header       = NULL;
  PIMAGE_SECTION_HEADER section_header    = NULL;
  PIMAGE_SYMBOL         symbol_entry      = NULL;
  t_section*            undef             = NULL;
  t_section*            abs               = NULL;
  t_section*            sec               = NULL;
  t_section*            parent_section    = NULL;
  t_symbol**            symtab            = NULL;
  t_symbol*             sym               = NULL;
  t_string              symbol_name       = NULL;
  t_tristate            symbol_search     = NO;
  t_tristate            symbol_dup        = NO;
  t_uint32              symbol_offset     = 0;
  t_int32               symbol_order      = 0;
  t_uint32              symtab_index      = 0;
  t_uint32              string_count      = 0;
  t_uint32              string_table_sz   = 0;
  t_string              string_table      = NULL;
  t_string              next_string       = NULL;
  char                  section_type      = '\0';
  t_bool                section_is_ro     = FALSE;
  t_uint32              section_alignment = 0;
  t_string              section_name      = NULL;
  t_uint32              tel;

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp) - stored_pointer;
  fseek(fp, stored_pointer, SEEK_SET);

  file_buffer = Malloc(file_size);
  fread(file_buffer, 1, file_size, fp);
  fseek(fp, stored_pointer, SEEK_SET);

  STATUS(START, ("Reading COFF Object: %s", OBJECT_NAME(obj)));

  /* we've already determined that this is a valid COFF file */
  coff_header = (PIMAGE_FILE_HEADER)file_buffer;

  /* initialize the object */
  OBJECT_SET_ADDRESS_SIZE(obj, ADDRSIZE32);
  OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew(obj));
  OBJECT_SET_RELOC_TABLE(obj, RelocTableNew(obj));

  /* Should never happen... Unless there is some platform that can execute COFF files directly. */
  ASSERT(OBJECT_PARENT(obj), ("This COFF object does not seem to have a parent."));
  undef = SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*UNDEF*");
  abs   = SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*ABS*");
  OBJECT_SET_UNDEF_SECTION(obj, undef);
  OBJECT_SET_ABS_SECTION(obj, abs);

  /* Begin processing tables */
  section_count     = coff_header->NumberOfSections;
  symbol_count      = coff_header->NumberOfSymbols;
  string_table      = (t_string)((unsigned int)file_buffer + coff_header->PointerToSymbolTable + symbol_count*sizeof(IMAGE_SYMBOL) + 4);
  string_table_sz   = *(t_uint32*)((unsigned int)string_table - 4);

  /* In COFF files, the section headers immediately follow the file header. No alignment or padding */
  section_header  = (PIMAGE_SECTION_HEADER)((unsigned int)file_buffer + sizeof(IMAGE_FILE_HEADER));

  for (i = 0; i < section_count; ++i)
  {
    if (section_header->Characteristics & IMAGE_SCN_LNK_REMOVE          /* extra linker information. Does not end up in the image file. */
      || section_header->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)   /* used for debugging information */
    {
      VERBOSE(2, ("> Found LNK_REMOVE or DEBUG section. Skipping..."));
      section_header++;
      continue;
    }

    /* decode name */
    if (section_header->Name[0] != '/')
    {
      section_name = (t_string)Malloc(9);
      memcpy(section_name, section_header->Name, 8);
      section_name[8] = '\0';
    }
    else
    {
      t_uint32 string_table_offset = atoi((const char*)section_header->Name + 1);
      ASSERT(string_table_offset < string_table_sz + 4,
        ("Section %d in this object has a name with an out of bounds reference to the string table.", i));
      section_name = StringDup(string_table + string_table_offset - 4);
    }

    /* determine section characteristics */
    CoffGetSectionType(section_header->Characteristics, &section_type, &section_is_ro);
    CoffGetSectionAlignment(section_header->Characteristics, &section_alignment);

    VERBOSE(0, ("Adding section: %s (IDX: %d)", section_name, i+1));
    sec = ObjectAddSectionFromFile(
      obj,
      section_type,
      section_is_ro,
      fp,
      section_header->PointerToRawData + stored_pointer,
      0,                                  /* no addresses are known yet... */
      section_header->SizeOfRawData,
      section_alignment,
      section_name,
      i+1);

    section_header++;
  }

  symbol_entry = (PIMAGE_SYMBOL)((unsigned int)file_buffer + coff_header->PointerToSymbolTable);
  symtab = (t_symbol**)Malloc(symbol_count*sizeof(t_symbol*));
  memset(symtab, 0, symbol_count*sizeof(t_symbol*));

  /* we should add COFF symbols as sections in the subobject... */
  for (i = 0; i < symbol_count; ++i)
  {
    /* if the first 4 bytes are zeroed out, the second 4 bytes
    are an offset into the string table */
    if (!symbol_entry->N.Name.Short)
      symbol_name = StringDup(string_table + symbol_entry->N.Name.Long - 4);
    else
    {
      symbol_name = (t_string)Malloc(9);
      symbol_name[8] = '\0';
      memcpy(symbol_name, symbol_entry->N.ShortName, 8);
    }

    /* COFF section numbers start at 1. If the section number is 0,
    the symbol is not contained within this object! */
    if (symbol_entry->SectionNumber && symbol_entry->SectionNumber != -1)
    {
      /* look up parent section */
      parent_section = NULL;
      _(OBJECT_FOREACH_SECTION(obj, sec, tel)
      {
        if (SECTION_INDEX_IN_OBJ(sec) == symbol_entry->SectionNumber)
          parent_section = sec;
      })
      symbol_order = 10;
      symbol_search = NO;
      symbol_dup = NO;
      symbol_offset = symbol_entry->Value;
    }
    else
    {
      parent_section = undef;
      symbol_order = 0;
      symbol_search = YES;
      symbol_dup = PERHAPS;
      symbol_offset = 0;
    }

    /* we're mainly interested in external symbols here...
    Information about symbols of other storage classes can be read elsewhere */
    if (symbol_entry->StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
    {
      /* calculate effective index in the symbol table...
      Auxiliary symbol records each have their own symtab idx */
      symtab_index = ((t_uint32)symbol_entry - (t_uint32)file_buffer - coff_header->PointerToSymbolTable)/sizeof(IMAGE_SYMBOL);

      ASSERT(parent_section, ("Couldn't find parent section for symbol: %s", section_name));

      VERBOSE(0, ("Adding COFF symbol: %s (symtab idx: %d) to parent_section: %s",
        symbol_name, symtab_index, SECTION_NAME(parent_section)));

      symtab[symtab_index] = SymbolTableAddSymbol(
        OBJECT_SYMBOL_TABLE(obj),
        symbol_name,
        "R00A00+$",
        symbol_order, symbol_dup,
        symbol_search,
        T_RELOCATABLE(parent_section),
        symbol_offset,
        0,
        NULL,
        0,
        0);
    }
    else if (symbol_entry->StorageClass == IMAGE_SYM_CLASS_STATIC)
    {
      if (symbol_entry->SectionNumber != IMAGE_SYM_ABSOLUTE
        && parent_section)
      {
        symtab_index = ((t_uint32)symbol_entry - (t_uint32)file_buffer - coff_header->PointerToSymbolTable)/sizeof(IMAGE_SYMBOL);
        VERBOSE(0, ("Adding static symbol: %s %d", symbol_name, symbol_entry->SectionNumber));
        /* add pseudo-symbol corresponding with section */
        symtab[symtab_index] = SymbolTableAddSymbol(
          OBJECT_SYMBOL_TABLE(obj),
          symbol_name,
          "R00A00+$",
          -1, NO,
          NO,
          T_RELOCATABLE(parent_section),
          symbol_offset,
          0,
          NULL,
          0,
          0);
      }
    }

    symbol_count -= symbol_entry->NumberOfAuxSymbols;
    symbol_entry += symbol_entry->NumberOfAuxSymbols;
    symbol_entry++;
    Free(symbol_name);
  }

  STATUS(START, ("Processing COFF Relocs"));
  section_header  = (PIMAGE_SECTION_HEADER)((unsigned int)file_buffer + sizeof(IMAGE_FILE_HEADER));
  for (i = 0; i < section_count; ++i)
  {
    if (section_header->Characteristics & IMAGE_SCN_LNK_REMOVE || section_header->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
    {
      section_header++;
      continue;
    }

    sec = SectionGetFromObjectByIndex(obj, i+1);
    ASSERT(sec, ("Couldn't find section idx %d in obj %s", i+1, OBJECT_NAME(obj)));

    for (j = 0; j < section_header->NumberOfRelocations; ++j)
    {
      PIMAGE_RELOCATION reloc =
        (PIMAGE_RELOCATION)((t_uint32)file_buffer
        + section_header->PointerToRelocations
        + j * sizeof(IMAGE_RELOCATION));

      sym = symtab[reloc->SymbolTableIndex];
      ASSERT(sym, ("Couldn't find corresponding symbol for reloc (symtab idx: %d)", reloc->SymbolTableIndex));

      switch(reloc->Type)
      {
        /* equivalent of ELF's R_I386_32 - Direct VA*/
        case IMAGE_REL_I386_DIR32:
        {
          RelocTableAddRelocToSymbol(
            OBJECT_RELOC_TABLE(obj),
            OBJECT_GP(OBJECT_PARENT(obj)),
            T_RELOCATABLE(sec),
            reloc->DUMMYUNIONNAME.VirtualAddress,
            sym,
            TRUE,
            NULL,
            NULL,
            NULL,
            "S00A00+\\l*w\\s0000$");

          break;
        }

        /* Direct RVA */
        case IMAGE_REL_I386_DIR32NB:
        {
          break;
        }

        /* Relative to the program counter - equivalent of ELF's R_I386_PC32 */
        case IMAGE_REL_I386_REL32:
        {
          /*
          VERBOSE(0, ("Found PC-Rel Reloc to Sym: %s (symtab idx: %d) - should write at: %s (offset: %02x)",
            SYMBOL_NAME(sym), reloc->SymbolTableIndex, SECTION_NAME(sec), reloc->DUMMYUNIONNAME.VirtualAddress));
          */
          /* S00 = value of the to symbol (e.g. instruction address) */
          /* P = value of the from symbol (i.e. from_section+offset) */
          /* PC relative => PC points to instr after this address => add 4 */
          RelocTableAddRelocToSymbol(
            OBJECT_RELOC_TABLE(obj),
            4,
            T_RELOCATABLE(sec),
            reloc->DUMMYUNIONNAME.VirtualAddress,
            sym,
            TRUE,
            NULL,
            NULL,
            NULL,
            "S00P-A00-\\l*w\\s0000$");

          break;
        }

        /* Unknown? */
        default:
        {
          FATAL(("Unknown relocation type found in reloc table for section: %s\n"
            "Reloc index: %d, VA: 0x%08x, symtab idx: %d, type: 0x%02x",
            SECTION_NAME(sec), j, reloc->DUMMYUNIONNAME.VirtualAddress,
            reloc->SymbolTableIndex, reloc->Type));
        }
      }
    }

    section_header++;
  }
  STATUS(STOP, ("Processing COFF Relocs"));
  STATUS(STOP, ("Reading COFF Object: %s", OBJECT_NAME(obj)));

  Free(symtab);
  Free(file_buffer);
}

/*-----------------------------------------------------------------------------
  CoffImportNormalizeSymbolName
-----------------------------------------------------------------------------*/
t_string
CoffImportNormalizeSymbolName(t_string in)
{
  t_string result = in;

  if (!in)
    return NULL;

  /* undecorate C-Style strings */
  if (StringPatternMatch("_*@*", in))
  {
    t_string tmp = StringDup(in);
    t_uint16 i = 0;

    while (tmp[i++] != '@');
    tmp[i-1] = '\0';

    result = (t_string)Malloc(strlen(tmp));
    memcpy(result, (void*)((t_uint32)tmp + 1), strlen(tmp));
    Free(tmp);
  }

  return result;
}

/*-----------------------------------------------------------------------------
  CoffImportAddSymbolImport
-----------------------------------------------------------------------------*/
void
CoffImportAddSymbolImport(t_object* fake_object, t_string symbol_name)
{
  VERBOSE(0, ("Adding Symbol: %s to Object: %s", symbol_name, OBJECT_NAME(fake_object)));
  SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), symbol_name, "R00$",
    1, PERHAPS, YES, T_RELOCATABLE(OBJECT_UNDEF_SECTION(fake_object)), 0, 0, NULL, 0, 0);
}

/*-----------------------------------------------------------------------------
  CoffImportSetActualImportedModuleName
-----------------------------------------------------------------------------*/
void
CoffImportSetActualImportedModuleName(t_object* fake_object, t_string actual_name)
{
  t_string symbol_name = (t_string)Malloc(strlen(actual_name) + strlen("$module$") + 1);
  sprintf(symbol_name, "$module$%s", actual_name);
  CoffImportAddSymbolImport(fake_object, symbol_name);
  Free(symbol_name);
}

/*-----------------------------------------------------------------------------
  CoffImportAddPseudoImport - adds a pseudo import (e.g. for null thunks)
-----------------------------------------------------------------------------*/
void
CoffImportAddPseudoImport(t_object* fake_object, t_string symbol_name)
{
  if (symbol_name[0] == 127)
  {
    t_string new_symbol_name = (t_string)Malloc(strlen(symbol_name) + 4);
    memcpy(new_symbol_name + 3, symbol_name, strlen(symbol_name) + 1);
    new_symbol_name[0] = '\\';
    new_symbol_name[1] = '1';
    new_symbol_name[2] = '7';
    new_symbol_name[3] = '7';
    symbol_name = new_symbol_name;
    CoffImportAddSymbolImport(fake_object, symbol_name);
    Free(symbol_name);
  }
  else
  {
    CoffImportAddSymbolImport(fake_object, symbol_name);
  }
}

/*-----------------------------------------------------------------------------
  CoffImportAddImportByOrdinal
-----------------------------------------------------------------------------*/
void
CoffImportAddImportByOrdinal(t_object* fake_object, t_uint16 ordinal)
{
  t_string import_name = (t_string)Malloc(7);
  sprintf(import_name, "$ord$%02X", ordinal);
  CoffImportAddSymbolImport(fake_object, import_name);
  Free(import_name);
}

/*-----------------------------------------------------------------------------
  CoffUpdateImportTableLayout -
-----------------------------------------------------------------------------*/
void
CoffUpdateImportTableLayout(t_object* obj)
{
  t_object**  import_objects          = NULL;
  t_object*   tmp                     = NULL;
  t_object*   sub_obj                 = NULL;
  t_uint32    i                       = 0;
  t_uint32    imported_lib_num        = 0;
  t_uint32    fake_objects_finalized  = 0;
  t_symbol*   symptr                  = NULL;
  t_uint32    idt_pos                 = 0;
  t_uint32    ilt_pos                 = 0;
  t_uint32    int_pos                 = 0;
  t_section*  tmp_section             = NULL;

  for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
  {
    if (StringPatternMatch("__IMPORT_DESCRIPTOR*", SYMBOL_NAME(symptr)))
      imported_lib_num++;
    else if (!strcmp(".idata$2$range$", SYMBOL_NAME(symptr)))
      idt_pos = RELOCATABLE_OLD_ADDRESS(SYMBOL_BASE(symptr));
    else if (!strcmp(".idata$4$range$", SYMBOL_NAME(symptr)))
      ilt_pos = RELOCATABLE_OLD_ADDRESS(SYMBOL_BASE(symptr));
    else if (!strcmp(".idata$6$range$", SYMBOL_NAME(symptr)))
      int_pos = RELOCATABLE_OLD_ADDRESS(SYMBOL_BASE(symptr));
  }

  import_objects = (t_object**)Malloc(imported_lib_num * sizeof(t_object*));

  for(tmp=OBJECT_MAPPED_FIRST(obj); tmp!=NULL; tmp=OBJECT_NEXT(tmp))
  {
    for (sub_obj=tmp; sub_obj!=NULL; sub_obj=OBJECT_EQUAL(sub_obj))
    {
      if (StringPatternMatch("*$IMPORT_OBJECT$", OBJECT_NAME(sub_obj)))
      {
        if (SectionGetFromObjectByName(sub_obj, ".idata$5"))
          import_objects[fake_objects_finalized++] = sub_obj;
      }
    }
  }

  if (fake_objects_finalized != imported_lib_num)
  {
    Free(import_objects);
    return;
  }

  STATUS(START, ("Calculating full import table layout"));

  while (fake_objects_finalized > 0)
  {
    t_uint32  lowest_iat_start = 0x7fffffff;
    t_uint32  lowest_iat_start_object_offset = 0;
    t_object* lowest_iat_start_object = NULL;

    for (i = 0; i < imported_lib_num; ++i)
    {
      if (import_objects[i])
      {
        t_section* iat_section = SectionGetFromObjectByName(import_objects[i], ".idata$5");
        if (SECTION_CADDRESS(iat_section) < lowest_iat_start)
        {
          lowest_iat_start = SECTION_CADDRESS(iat_section);
          lowest_iat_start_object_offset = i;
        }
      }
    }

    /* found the next section... now lay it out */
    lowest_iat_start_object = import_objects[lowest_iat_start_object_offset];
    import_objects[lowest_iat_start_object_offset] = NULL;

    VERBOSE(0, ("Laying out object: %s", OBJECT_NAME(lowest_iat_start_object)));

    /* lay out IDT section */
    tmp_section = SectionGetFromObjectByName(lowest_iat_start_object, ".idata$2");
    VERBOSE(0, ("Section: %s => Address: %08x", SECTION_NAME(tmp_section), idt_pos));
    SECTION_SET_OLD_ADDRESS(tmp_section, idt_pos);
    SECTION_SET_CADDRESS(tmp_section, idt_pos);
    idt_pos += SECTION_CSIZE(tmp_section);

    /* lay out ILT section */
    tmp_section = SectionGetFromObjectByName(lowest_iat_start_object, ".idata$4");
    VERBOSE(0, ("Section: %s => Address: %08x", SECTION_NAME(tmp_section), ilt_pos));
    SECTION_SET_OLD_ADDRESS(tmp_section, ilt_pos);
    SECTION_SET_CADDRESS(tmp_section, ilt_pos);
    ilt_pos += SECTION_CSIZE(tmp_section);

    /* lay out INT section */
    tmp_section = SectionGetFromObjectByName(lowest_iat_start_object, ".idata$6");
    VERBOSE(0, ("Section: %s => Address: %08x", SECTION_NAME(tmp_section), int_pos));
    SECTION_SET_OLD_ADDRESS(tmp_section, int_pos);
    SECTION_SET_CADDRESS(tmp_section, int_pos);
    int_pos += SECTION_CSIZE(tmp_section);

    fake_objects_finalized--;
  }

  Free(import_objects);
  STATUS(STOP, ("Calculating full import table layout"));
}

/*-----------------------------------------------------------------------------
  CoffImportFinalizeFakeObject - Lay-out of the fake object:

  \TODO: Add hints to the PIMAGE_IMPORT_BY_NAME structs. Ideally, we'd load
  the actual imported library to look at their export tables. In reality
  it might be better to just copy the hints from the parent PE file...
-----------------------------------------------------------------------------*/
void
CoffImportFinalizeFakeObject(t_object* fake_object)
{
  t_section*  tmp                   = NULL;
  t_section*  parent_section        = NULL;
  t_section*  iat_section           = NULL;
  t_section*  ilt_section           = NULL;
  t_section*  int_section           = NULL;
  t_section*  idt_section           = NULL;
  t_symbol*   symptr                = NULL;
  t_symbol*   iat_sym               = NULL;
  t_symbol*   ilt_sym               = NULL;
  t_symbol*   int_sym               = NULL;
  t_uint32    import_cnt            = 0;
  t_uint32    actual_import_cnt     = 0;
  t_uint32    import_by_ordinal_cnt = 0;
  t_uint32    iat_size              = 0;
  t_uint32    int_size              = 0;
  t_uint32    int_pos               = 0;
  t_uint32    sym_num               = 0;
  t_string    actual_module_name    = NULL;
  t_string    null_thunk_name       = NULL;
  void*       tmp_data              = NULL;
  void*       ilt_data              = NULL;
  void*       int_data              = NULL;

  STATUS(START, ("Finalizing Import Object: %s", OBJECT_NAME(fake_object)));

  /* collect symbols */
  _(OBJECT_FOREACH_SYMBOL(fake_object, symptr)
  {
    if (StringPatternMatch("$module$*", SYMBOL_NAME(symptr)))
    {
      actual_module_name = (t_string)Malloc(strlen(SYMBOL_NAME(symptr))-7);
      strcpy(actual_module_name, (t_string)((t_uint32)SYMBOL_NAME(symptr)+8));

      /* contrary to the rest of the .idata$6 section, the imported module name
      is not an actual PIMAGE_IMPORT_BY_NAME struct. It might however require
      padding so that ALL PIMAGE_IMPORT_BY_NAME are 2-byte aligned... */
      int_size += strlen(SYMBOL_NAME(symptr))-7;
      AddressAlign(2, int_size);
    }
    else if (StringPatternMatch("*IMPORT_DESCRIPTOR*", SYMBOL_NAME(symptr)))
    {
    }
    else if (StringPatternMatch("*NULL_THUNK*", SYMBOL_NAME(symptr)))
    {
      null_thunk_name = StringDup(SYMBOL_NAME(symptr));
      import_cnt++;
    }
    else if (StringPatternMatch("$ord$*", SYMBOL_NAME(symptr)))
    {
      import_cnt++;
      actual_import_cnt++;
      import_by_ordinal_cnt++;
    }
    else
    {
      t_string normalized_name = CoffImportNormalizeSymbolName(SYMBOL_NAME(symptr)+6);
      VERBOSE(0, ("Regular Import: %s", normalized_name));

      int_size += strlen(normalized_name)+1+2;
      AddressAlign(2, int_size);

      actual_import_cnt++;
      import_cnt++;
      Free(normalized_name);
    }
  })

  VERBOSE(0, ("Found %d imports from module: %s", actual_import_cnt, actual_module_name));

  /* .idata$5 (IAT) and .idata$4 (ILT) section sizes depend only on nr of imports */
  /* .idata$6 (INT) section size is variable. This section contains the name of the
  imported module + one PIMAGE_IMPORT_BY_NAME struct for every non-ordinal import */
  /* .idata$2 (IDT) section size is fixed. This section contains the import descriptor */
  iat_size = import_cnt*(4 << OBJECT_ADDRESS_SIZE(fake_object));

  OBJECT_SET_RODATA(fake_object, (t_section**)Realloc (OBJECT_RODATA(fake_object), sizeof (t_section *) * 4));

  /* create sections - we create them as special sections so diablo doesn't allocate memory for them */
  VERBOSE(0, ("Creating .idata$5 section (Import Address Table) - size: %d bytes", iat_size));
  iat_section = OBJECT_RODATA(fake_object)[0] =
    SectionCreateForObject(fake_object,
      SPECIAL_SECTION,
      SectionGetFromObjectByName(OBJECT_PARENT(fake_object), ".rdata"),
      iat_size,
      ".idata$5");

  VERBOSE(0, ("Creating .idata$4 section (Import Lookup Table) - size: %d bytes", iat_size));
  ilt_section = OBJECT_RODATA(fake_object)[1] =
    SectionCreateForObject(fake_object,
      SPECIAL_SECTION,
      SectionGetFromObjectByName(OBJECT_PARENT(fake_object), ".rdata"),
      iat_size,
      ".idata$4");

  VERBOSE(0, ("Creating .idata$6 section (Import Name Table) - size: %d bytes", int_size));
  int_section = OBJECT_RODATA(fake_object)[2] =
    SectionCreateForObject(fake_object,
      SPECIAL_SECTION,
      SectionGetFromObjectByName(OBJECT_PARENT(fake_object), ".rdata"),
      int_size,
      ".idata$6");

  VERBOSE(0, ("Creating .idata$2 section (Import Descriptor Table) - size: %d bytes", sizeof(IMAGE_IMPORT_DESCRIPTOR)));
  idt_section = OBJECT_RODATA(fake_object)[3] =
    SectionCreateForObject(fake_object,
      SPECIAL_SECTION,
      SectionGetFromObjectByName(OBJECT_PARENT(fake_object), ".rdata"),
      sizeof(IMAGE_IMPORT_DESCRIPTOR),
      ".idata$2");

  /* now it's safe to flag them as RODATA */
  SECTION_SET_TYPE(iat_section, RODATA_SECTION);
  SECTION_SET_TYPE(ilt_section, RODATA_SECTION);
  SECTION_SET_TYPE(int_section, RODATA_SECTION);
  SECTION_SET_TYPE(idt_section, RODATA_SECTION);
  OBJECT_SET_NRODATAS(fake_object, 4);

  /* we have to determine the exact original location of this
  object's IAT table since the code WILL reference it */
  for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(OBJECT_PARENT(fake_object))); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
  {
    if (!strcmp(SYMBOL_NAME(symptr), null_thunk_name))
    {
      SECTION_SET_OLD_ADDRESS(iat_section, RELOCATABLE_OLD_ADDRESS(SYMBOL_BASE(symptr))
        + SYMBOL_OFFSET_FROM_START(symptr) - actual_import_cnt * (4 << OBJECT_ADDRESS_SIZE(fake_object)));
      SECTION_SET_CADDRESS(iat_section, SECTION_OLD_ADDRESS(iat_section));
      SECTION_SET_OLD_SIZE(iat_section, iat_size);
      SECTION_SET_CSIZE(iat_section, iat_size);
      break;
    }
  }

  /* initialize IAT section */
  tmp_data = Malloc(iat_size);
  memset(tmp_data, 0, iat_size);
  SECTION_SET_DATA(iat_section, tmp_data);

  iat_sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), ".idata$5", "R00$", 0,
    YES, NO, T_RELOCATABLE(iat_section), 0, 0, NULL, iat_size, 0);

  /* initialize ILT section */
  ilt_data = Malloc(iat_size);
  memset(ilt_data, 0, iat_size);
  SECTION_SET_DATA(ilt_section, ilt_data);

  ilt_sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), ".idata$4", "R00$", 0,
    YES, NO, T_RELOCATABLE(ilt_section), 0, 0, NULL, iat_size, 0);

  /* initialize INT section and process symbols */
  int_data = Malloc(int_size);
  memset(int_data, 0, int_size);
  SECTION_SET_DATA(int_section, int_data);

  int_sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), ".idata$6", "R00$", 0,
    YES, NO, T_RELOCATABLE(int_section), 0, 0, NULL, int_size, 0);

  /* Now process symbols and write the import lookup and name tables ... */
  memcpy(int_data, actual_module_name, strlen(actual_module_name));
  int_pos += strlen(actual_module_name);
  AddressAlign(2, int_pos);

  for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(fake_object)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
  {
    if (StringPatternMatch("__imp_*", SYMBOL_NAME(symptr)))
    {
      t_string normalized;

      /* add symbol to the IAT entry first. This is the symbol to
      which relocations from within the code section point */
      //VERBOSE(0, ("ADDING IAT SYMBOL: %s", SYMBOL_NAME(symptr)));
      SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), SYMBOL_NAME(symptr),
        "R00$", 10, NO, NO, T_RELOCATABLE(iat_section),
        sym_num * (4 << OBJECT_ADDRESS_SIZE(fake_object)),
        0, NULL, (4 << OBJECT_ADDRESS_SIZE(fake_object)), 0);

      /* write the name into the INT */
      normalized = CoffImportNormalizeSymbolName(SYMBOL_NAME(symptr)+6);
      memcpy((void*)((t_uint32)int_data + int_pos + 2), normalized, strlen(normalized));

      /* now add a relocation to the ILT entry */
      /*RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(fake_object),
        int_pos,
        T_RELOCATABLE(int_section),


        );*/

      int_pos += 2 + strlen(normalized);
      AddressAlign(2, int_pos);
      Free(normalized);

      //"R00i00000001>i10000000|$"

      sym_num++;
    }
    else if (StringPatternMatch("$ord$*", SYMBOL_NAME(symptr)))
    {
      /* add symbol to the IAT entry first. This is the symbol to
      which relocations from within the code section point */
      SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(fake_object), SYMBOL_NAME(symptr),
        "R00$", 10, NO, NO, T_RELOCATABLE(iat_section),
        sym_num * (4 << OBJECT_ADDRESS_SIZE(fake_object)),
        0, NULL, (4 << OBJECT_ADDRESS_SIZE(fake_object)), 0);
      sym_num++;
    }
  }

  if (null_thunk_name)
    Free(null_thunk_name);
  Free(actual_module_name);
  STATUS(STOP, ("Finalizing Import Object: %s", OBJECT_NAME(fake_object)));

  CoffUpdateImportTableLayout(OBJECT_PARENT(fake_object));
}

/*-----------------------------------------------------------------------------
  CoffShortImportRead
-----------------------------------------------------------------------------*/
void
CoffShortImportRead(FILE * fp, t_object * obj, t_bool read_debug)
{
  t_uint32              stored_pointer  = ftell(fp);
  t_uint32              section_len     = 0;
  t_string              import_name     = NULL;
  t_string              library_name    = NULL;
  t_string              symbol_name     = NULL;
  t_section*            tmp_section     = NULL;
  void*                 tmp_data        = NULL;
  t_object*             fake_object     = NULL;
  IMPORT_OBJECT_HEADER  import_header;

  STATUS(START, ("Reading COFF Short Import Object: %s", OBJECT_NAME(obj)));

  /* basic object initialization to keep diablo happy */
  fread(&import_header, sizeof(IMPORT_OBJECT_HEADER), 1, fp);
  import_name   = FileGetString(fp, '\0');
  library_name  = FileGetString(fp, '\0');
  OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew(obj));
  OBJECT_SET_RELOC_TABLE(obj, RelocTableNew(obj));
  OBJECT_SET_ADDRESS_SIZE(obj, ADDRSIZE32);
  SectionCreateForObject(obj, RODATA_SECTION, NULL, 0, ".idata$5");

  VERBOSE(0, ("Found import info for symbol: %s in lib: %s", import_name, library_name));

  fake_object = CoffFindImportObject(library_name, OBJECT_PARENT(obj));

  symbol_name = (t_string)Malloc(strlen(import_name) +  9);
  sprintf(symbol_name, "__imp_%s", import_name);

  CoffImportAddSymbolImport(fake_object, symbol_name);

  Free(symbol_name);
  fseek(fp, stored_pointer, SEEK_SET);
  STATUS(STOP,  ("Reading COFF Short Import Object: %s", OBJECT_NAME(obj)));
}

/*-----------------------------------------------------------------------------
  CoffLongImportRead
-----------------------------------------------------------------------------*/
void
CoffLongImportRead(FILE * fp, t_object * obj, t_bool read_debug)
{
  t_uint32              i                 = 0;
  t_uint32              obj_name_len      = 0;
  t_uint32              last_colon        = 0;
  t_uint32              last_semicolon    = 0;
  t_uint32              file_size         = 0;
  t_uint32              stored_pointer    = ftell(fp);
  t_uint32              section_count     = 0;
  t_uint32              symbol_count      = 0;
  t_uint32              string_table_sz   = 0;
  void*                 file_buffer       = NULL;
  PIMAGE_FILE_HEADER    coff_header       = NULL;
  PIMAGE_SECTION_HEADER section_header    = NULL;
  PIMAGE_SYMBOL         symbol_entry      = NULL;
  t_section*            undef             = NULL;
  t_section*            abs               = NULL;
  t_section*            sec               = NULL;
  t_string              string_table      = NULL;
  t_string              imported_lib_name = NULL;
  t_string              section_name      = NULL;
  t_string              symbol_name       = NULL;
  t_string              actual_name       = NULL;
  t_object*             import_object     = NULL;

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp) - stored_pointer;
  fseek(fp, stored_pointer, SEEK_SET);

  file_buffer = Malloc(file_size);
  fread(file_buffer, 1, file_size, fp);
  fseek(fp, stored_pointer, SEEK_SET);

  STATUS(START, ("Reading COFF Long Import Object: %s", OBJECT_NAME(obj)));

  /* find or create a fake import library */
  obj_name_len = strlen(OBJECT_NAME(obj));
  for (i = 0; i < obj_name_len; ++i)
  {
    if (OBJECT_NAME(obj)[i] == ':')
      last_colon = i;
    else if (OBJECT_NAME(obj)[i] == ';')
      last_semicolon = i;
  }
  if (last_colon && last_semicolon)
  {
    imported_lib_name = (t_string)Malloc(last_semicolon-last_colon);
    memcpy(imported_lib_name, (void*)((t_uint32)OBJECT_NAME(obj) + last_colon + 1), last_semicolon - last_colon - 1);
    imported_lib_name[last_semicolon-last_colon-1] = '\0';
  }

  ASSERT(imported_lib_name, ("Couldn't find the name of the imported library"));
  import_object = CoffFindImportObject(imported_lib_name, OBJECT_PARENT(obj));
  ASSERT(import_object, ("Couldn't find or create import object"));

  /* Even though we will leave the t_object associated with this COFF long import object untouched,
  it still needs at least some basic initialization */
  coff_header = (PIMAGE_FILE_HEADER)file_buffer;
  OBJECT_SET_ADDRESS_SIZE(obj, ADDRSIZE32);
  OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew(obj));
  OBJECT_SET_RELOC_TABLE(obj, RelocTableNew(obj));

  /* Begin processing tables */
  section_count     = coff_header->NumberOfSections;
  symbol_count      = coff_header->NumberOfSymbols;
  string_table      = (t_string)((unsigned int)file_buffer + coff_header->PointerToSymbolTable + symbol_count*sizeof(IMAGE_SYMBOL) + 4);
  string_table_sz   = *(t_uint32*)((unsigned int)string_table - 4);

  /* In COFF files, the section headers immediately follow the file header. No alignment or padding */
  section_header  = (PIMAGE_SECTION_HEADER)((unsigned int)file_buffer + sizeof(IMAGE_FILE_HEADER));
  for (i = 0; i < section_count; ++i, ++section_header)
  {
    if (section_header->Characteristics & IMAGE_SCN_LNK_REMOVE || section_header->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
      continue;

    /* decode name */
    if (section_header->Name[0] != '/')
    {
      section_name = (t_string)Malloc(9);
      memcpy(section_name, section_header->Name, 8);
      section_name[8] = '\0';
    }
    else
    {
      t_uint32 string_table_offset = atoi((const char*)section_header->Name + 1);
      ASSERT(string_table_offset < string_table_sz + 4,
        ("Section %d in this object has a name with an out of bounds reference to the string table.", i));
      section_name = StringDup(string_table + string_table_offset - 4);
    }

    /* we just create an empty section for this object to keep Diablo happy */
    /* the actual data for this section will end up in the import object */
    VERBOSE(0, ("Adding section: %s (IDX: %d)", section_name, i+1));
    sec = SectionCreateForObject(obj, RODATA_SECTION, NULL, 0, section_name);

    if (!strcmp(section_name, ".idata$6"))
    {
      /* .idata$6 contains the ACTUAL name of the imported module. */
      /* Technically, it would be possible that this actual name does not */
      /* match the name of the import object!!! */
      /* ==> read the section and add the actual name as a pseudo-symbol in the fake object */
      actual_name = StringDup((t_string)((t_uint32)coff_header + section_header->PointerToRawData));
      CoffImportSetActualImportedModuleName(import_object, actual_name);
      Free(actual_name);
    }
  }

  symbol_entry = (PIMAGE_SYMBOL)((unsigned int)file_buffer + coff_header->PointerToSymbolTable);

  for (i = 0; i < symbol_count; ++i, ++symbol_entry)
  {
    /* if the first 4 bytes are zeroed out, the second 4 bytes
    are an offset into the string table */
    if (!symbol_entry->N.Name.Short)
      symbol_name = StringDup(string_table + symbol_entry->N.Name.Long - 4);
    else
    {
      symbol_name = (t_string)Malloc(9);
      symbol_name[8] = '\0';
      memcpy(symbol_name, symbol_entry->N.ShortName, 8);
    }

    if (!symbol_entry->SectionNumber || symbol_entry->SectionNumber == IMAGE_SYM_ABSOLUTE
      || symbol_entry->StorageClass != IMAGE_SYM_CLASS_EXTERNAL)
      continue;

    VERBOSE(0, ("Found symbol: %s", symbol_name));

    CoffImportAddPseudoImport(import_object, symbol_name);

    if (StringPatternMatch("*IMPORT_DESCRIPTOR*", symbol_name))
    {
      VERBOSE(0, ("Found the import descriptor!!! We can finalize the fake object now"));
      CoffImportFinalizeFakeObject(import_object);
      break;
    }
  }

  STATUS(STOP, ("Reading COFF Long Import Object: %s", OBJECT_NAME(obj)));
}

/*-----------------------------------------------------------------------------
  PeGetMapName -
-----------------------------------------------------------------------------*/
t_string PeGetMapName(const t_object* obj)
{
  t_string result = StringDup(OBJECT_NAME(obj));
  t_uint32 len = strlen(result);

  // we're assuming that the extension is 3 characters long here
  sprintf(result + len - 3, "%s", "map");

  VERBOSE(0, ("Linker map name: %s", result));

  return result;
}

/*-----------------------------------------------------------------------------
  PeReadExportTable -
-----------------------------------------------------------------------------*/
void
PeReadExportTable(t_object* obj, void* file_buffer)
{
  PIMAGE_DOS_HEADER       dos_header  = NULL;
  PIMAGE_NT_HEADERS       nt_header   = NULL;
  PIMAGE_EXPORT_DIRECTORY export_dir  = NULL;

  /* no header validation here. Should be checked by this function's caller */
  dos_header  = (PIMAGE_DOS_HEADER)(file_buffer);
  nt_header   = (PIMAGE_NT_HEADERS)((unsigned int)file_buffer + dos_header->e_lfanew);
  export_dir  = (PIMAGE_EXPORT_DIRECTORY)((unsigned int)file_buffer
    + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
}

/*-----------------------------------------------------------------------------
  PeReadImportTable - The import table is where most of the linker "magic"
  happens. In normal dynamically linked binaries/libraries, the import table
  will consist of 3 types of structures. They are linked together as follows

  FYI:
  * PIMAGE_IMPORT_DESCRIPTORS are in .idata$2
  * the NULL_IMPORT_DESCRIPTOR is in .idata$3 and is appended to the .idata$2 sections
  * PIMAGE_THUNK_DATAS are in .idata$4
  * IMPORT_DESCRIPTOR.Name is in .idata$6
  * IAT entries are in .idata$5


  nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
  |
  |
  +-----> +-----------------------------------+
          | PIMAGE_IMPORT_DESCRIPTOR entry    |
          | one of these per library we're    |     +-----------------> +----------------------------------+
          | linking with. One nulled out      |     |                   | PIMAGE_THUNK_DATA entry          |
          | entry at the end of the table     |     |                   | one of these per imported func   |
          |                                   |     |                   |                                  |
          | Fields:                           |     |                   | Does NOT appear in linker map!   |
          | 0000-0004: OriginalFirstThunk     | ----+                   |                                  |
          | 0004-0008: NULL                   |                         | Contains ONE 32-bit address      |
          | 0008-000C: Forwarder Chain        |                         | If the MSB is set, the next 15   |
          | 000C-0010: Name                   | ----+                   | bytes are zeroed and the         |    +------> +-----------------------------------+
  +------ | 0010-0014: FirstThunk             |     |                   | remaining 16 bytes contain an    |    |        | PIMAGE_IMPORT_BY_NAME entry       |
  |       |                                   |     |                   | ordinal number.                  |    |        |                                   |
  |       | Forwarder chains are uncommon and |     |                   |                                  |    |        | Does NOT appear in linker map!    |
  |       | unsupported for now...            |     |                   | If the MSB is cleared, the lower |    |        |                                   |
  |       +-----------------------------------+     |                   | 31 bits are an RVA that points   |    |        | Fields:                           |
  |       | PIMAGE_IMPORT_DESCRIPTOR entry    |     |                   | to an IMPORT_BY_NAME struct      | ---+        | 0000-0002: Hint                   |
  |       .                                   .     |                   +----------------------------------+             | 0002-????: Name                   |
  |       +-----------------------------------+     |                   | PIMAGE_THUNK_DATA                |             | ????-????: PADDING                |
  |       | NULL                              |     |                   .                                  .             |                                   |
  |       .                                   .     |                   +----------------------------------+             | The Hint is an index into the ENT |
  |       +-----------------------------------+     |                   | NULL                             |             | (Export Name Table).              |
  |                                                 |                   .                                  .             | This field is optional but should |
  |                                                 |                   +----------------------------------+             | be filled in if the index of the  |
  +---> +--------------------------------+          |                                                                    | imported name in the imported     |
        | IAT entry                      |          V                                                                    | module's export table is known... |
        | one of these per imported      |  +-----------------------------+                                              |                                   |
        | function, with one nulled out  |  | Linker generated string     |                                              | The length of the Name field is   |
        | entry at the end               |  | this string is NULL-        |                                              | Variable but the field must be    |
        |                                |  | terminated and it does NOT  |                                              | padded to an even address bound-  |
        | IAT entries ARE in the MS      |  | appear in the linker map!!  |                                              | ary...                            |
        | linker map but they are simply |  +-----------------------------+                                              +-----------------------------------+
        | zeroed out until the image has |
        | been bound (at run-time)       |
        +--------------------------------+
        | IAT entry                      |
        .                                .
        +--------------------------------+
        | NULL                           |
        .                                .
        +--------------------------------+

-----------------------------------------------------------------------------*/
void
PeReadImportTable(t_object* obj, void* file_buffer)
{
  PIMAGE_DOS_HEADER         dos_header  = NULL;
  PIMAGE_NT_HEADERS         nt_header   = NULL;
  PIMAGE_IMPORT_DESCRIPTOR  idt         = NULL;
  PIMAGE_THUNK_DATA32       thunk       = NULL;
  t_section*                tmp_sec     = NULL;
  t_section*                idata_sec   = NULL;
  t_address                 idata_off   = NULL;
  t_uint32                  tel         = 0;
  t_address                 idesc_off   = 0;

  STATUS(START, ("Reading Import Table"));

  ASSERT(OBJECT_SYMBOL_TABLE(obj), ("Object doesn't have a symbol table!"));

  /* no header validation here. Should be checked by this function's caller */
  dos_header  = (PIMAGE_DOS_HEADER)(file_buffer);
  nt_header   = (PIMAGE_NT_HEADERS)((unsigned int)file_buffer + dos_header->e_lfanew);
  idt         = (PIMAGE_IMPORT_DESCRIPTOR)((unsigned int)file_buffer
    + PeRvaToOffset(file_buffer, nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));

  /* find the section in which the import table resides */
  _(OBJECT_FOREACH_SECTION(obj, tmp_sec, tel)
  {
    if (SECTION_CADDRESS(tmp_sec) <= nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
      && SECTION_CADDRESS(tmp_sec) + SECTION_CSIZE(tmp_sec) > nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      idata_sec = tmp_sec;
      break;
    }
  })

  ASSERT(idata_sec, ("Couldn't find section in which the import table resides"));
  idata_off = PeRvaToOffset(file_buffer, SECTION_CADDRESS(idata_sec));

  while (idt->FirstThunk)
  {
    /* Add the Import Descriptor as a symbol */
    t_string descriptor_name;
    t_string module_name = (t_string)((t_uint32)file_buffer + PeRvaToOffset(file_buffer, idt->Name));
    t_uint32 len, i;

    len = strlen(module_name) + strlen("__IMPORT_DESCRIPTOR_") + 1;
    descriptor_name = (t_string)Malloc(len);
    sprintf(descriptor_name, "__IMPORT_DESCRIPTOR_%s", module_name);
    StringToUpper(descriptor_name);
    for (i = 0; i < len; ++i)
      if (descriptor_name[i] == '.')
        descriptor_name[i] = '\0';

    idesc_off = (unsigned int)idt - (unsigned int)file_buffer - idata_off;
    VERBOSE(0, ("Adding Import Descriptor: %s - Base Section: %s - Offset: 0x%08x - Size: %d bytes",
      descriptor_name, SECTION_NAME(idata_sec), idesc_off, sizeof(*idt)));
    /*SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),
      descriptor_name,
      "R00A00+$",
      0, NO,
      NO,
      T_RELOCATABLE(idata_sec),
      idesc_off, 0,
      NULL,
      sizeof(*idt),
      0);*/

    /* walk the INT and IOT */
    thunk = (PIMAGE_THUNK_DATA32)((unsigned int)file_buffer
      + PeRvaToOffset(file_buffer, idt->DUMMYUNIONNAME.OriginalFirstThunk));

    while (thunk->u1.AddressOfData)
    {
      /* check import type */
      if ((thunk->u1.AddressOfData & ~IMAGE_ORDINAL_FLAG32) == thunk->u1.AddressOfData)
      {
        /* ordinal bit not set => import by name */
        t_string import_name_string;
        PIMAGE_IMPORT_BY_NAME import_name = NULL;

        if (thunk->u1.AddressOfData)
        {
          import_name = (PIMAGE_IMPORT_BY_NAME)((unsigned int)file_buffer +
            PeRvaToOffset(file_buffer, thunk->u1.AddressOfData));
          import_name_string = StringDup((char*)import_name->Name);
        }
        else
        {
          import_name_string = StringDup("NULL");
        }
      }
      else
      {
        t_uint16 import_ordinal = IMAGE_ORDINAL32(thunk->u1.Ordinal);

        FATAL(("Imports by ordinal are not supported yet..."));
      }

      thunk++;
    }

    Free(descriptor_name);
    idt++;
  }

  /* add the NULL descriptor... */
  VERBOSE(0, ("Adding Import Descriptor: %s - Base Section: %s - Offset: 0x%08x - Size: %d bytes",
      "__NULL_IMPORT_DESCRIPTOR", SECTION_NAME(idata_sec), idesc_off + sizeof(*idt), sizeof(*idt)));
  /*SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),
    "__NULL_IMPORT_DESCRIPTOR",
    "R00A00+$",
    0, NO,
    NO,
    T_RELOCATABLE(idata_sec),
    idesc_off+sizeof(*idt), 0,
    NULL,
    sizeof(*idt),
    0);*/

  STATUS(STOP, ("Reading Import Table"));
}

/*-----------------------------------------------------------------------------
  PeRead - This is used to create the parent object!
-----------------------------------------------------------------------------*/
void
PeRead(FILE * fp, t_object * obj, t_bool read_debug)
{
  PIMAGE_DOS_HEADER         dos_header      = NULL;
  PIMAGE_NT_HEADERS         nt_header       = NULL;
  PIMAGE_OPTIONAL_HEADER32  opt_header      = NULL;
  PIMAGE_SECTION_HEADER     section_header  = NULL;
  t_uint32                  stored_pointer  = ftell(fp);
  t_uint32                  file_size       = 0;
  void*                     file_buffer     = NULL;
  t_uint32                  i               = 0;
  char                      section_type    = '\0';
  t_bool                    section_is_ro   = FALSE;
  t_section*                tmp_section     = NULL;

  STATUS(START, ("Parsing PE File"));

  /* First Check if the object is valid */
  if (!obj)
    FATAL(("Object == NULL"));

  /* Dump the entire PE file into a buffer for easy navigation */
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp) - stored_pointer;
  fseek(fp, stored_pointer, SEEK_SET);

  file_buffer = Malloc(file_size);
  fread(file_buffer, 1, file_size, fp);
  fseek(fp, stored_pointer, SEEK_SET);

  /* Process DOS Header */
  dos_header = (PIMAGE_DOS_HEADER)(file_buffer);
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    FATAL(("Invalid PE DOS Header!"));

  /* Check NT Header */
  nt_header = (PIMAGE_NT_HEADERS)((DWORD)file_buffer + dos_header->e_lfanew);
  if (nt_header->Signature != LOWORD(IMAGE_NT_SIGNATURE))
    FATAL(("Invalid PE NT Header!"));

  /* Get Optional Header */
  opt_header = &nt_header->OptionalHeader;
  if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    FATAL(("PE32+ Files are not supported for now!"));
  else if (opt_header->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    FATAL(("Invalid PE Optional Header!"));

  /* We have the pointers we need, now start setting up the t_object */
  OBJECT_SET_ENTRY(obj, AddressNew32(/*nt_header->OptionalHeader.ImageBase + */nt_header->OptionalHeader.AddressOfEntryPoint));
  OBJECT_SET_GP(obj, nt_header->OptionalHeader.ImageBase);
  /* ok for PE32, should be 64 for PE32+ */
  OBJECT_SET_ADDRESS_SIZE(obj, ADDRSIZE32);
  /* we need this section for linker generated symbols */
  OBJECT_SET_ABS_SECTION(obj, SectionCreateForObject(obj, SPECIAL_SECTION, NULL, AddressNullForObject(obj), "*ABS*"));
  /* we need a reloc table for EVERY object */
  OBJECT_SET_RELOC_TABLE(obj, RelocTableNew(obj));

  /* Create a normal symbol table (for exported symbols, if any)
   * and a dynamic symbol table (for imported symbols).
   *
   * In statically linked executables, the dynamic symbol table would
   * be empty. In windows however, you cannot statically link with kernel32
   * or ntdll.
   */
  OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew (obj));
  OBJECT_SET_DYNAMIC_SYMBOL_TABLE(obj, SymbolTableNew(obj));

  /* STEP 1: read and add all the sections to the parent object */
  section_header = IMAGE_FIRST_SECTION(nt_header);
  for (i = 0; i < nt_header->FileHeader.NumberOfSections; ++i)
  {
    /* Technically, the section headers are the same as in COFF.
     * PE does NOT use per-section relocations (only the linker needs these)
     * or line numbers (deprecated) though.
     */
    t_string tmp = (t_string)Malloc(IMAGE_SIZEOF_SHORT_NAME+1);
    tmp[IMAGE_SIZEOF_SHORT_NAME] = '\0';
    memcpy(tmp, section_header->Name, IMAGE_SIZEOF_SHORT_NAME);

    VERBOSE(0, ("SECTION %d: %s\t(CHARACTERISTICS: 0x%08X)\t(ADDRESS: 0x%08X)", i, tmp, section_header->Characteristics, section_header->VirtualAddress));

    CoffGetSectionType(section_header->Characteristics, &section_type, &section_is_ro);

    tmp_section = ObjectAddSectionFromFile(
      obj,                                                                  /* object to which the section should be added */
      section_type,                                                         /* section type */
      section_is_ro,                                                        /* read-only section? */
      fp,                                                                   /* file from which the section should be read */
      PeRvaToOffset(file_buffer, section_header->VirtualAddress),           /* offset of the section within the file */
      section_header->VirtualAddress,                                       /* virtual address of the file -- the VirtualAddress field in the section header actually contains the RELATIVE Virtual Address! */
      section_header->SizeOfRawData,                                        /* size of the section */
      nt_header->OptionalHeader.SectionAlignment,                           /* section alignment */
      tmp,                                                                  /* name of the section */
      i + 1);                                                               /* section number - PE section numbers start from 1 */

    section_header++;
  }

  /* walk the export table and add all elements to the symbol table */
  if (opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
    PeReadExportTable(obj, file_buffer);

  /* walk the import table and add all elements to the dynamic symbol table */
  if (opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    PeReadImportTable(obj, file_buffer);

  Free(file_buffer);

  STATUS(STOP, ("Parsing PE File"));
}

/*-----------------------------------------------------------------------------
  DiabloPeCoffInit - Registers the back-end
-----------------------------------------------------------------------------*/
void DiabloPeCoffInit(int argc, char ** argv)
{
  ObjectHandlerAdd("PECOFF", NULL, IsPeCoff, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  ObjectHandlerAdd("PECOFF", "i386", IsPe, PeRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, PeGetMapName);
  ObjectHandlerAdd("PECOFF", "COFF", IsCoff, CoffRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  ObjectHandlerAdd("PECOFF", "COFFSHORTIMPORT", IsCoffShortImport, CoffShortImportRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  ObjectHandlerAdd("PECOFF", "COFFLONGIMPORT", IsCoffLongImport, CoffLongImportRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/*-----------------------------------------------------------------------------
  DiabloPeCoffFini - Should unregister the back-end but in reality, this
  function is only called when Diablo is closing down.
-----------------------------------------------------------------------------*/
void DiabloPeCoffFini()
{
}
