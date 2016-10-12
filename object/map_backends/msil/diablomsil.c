/*=============================================================================
  Map Back-End for the Microsoft Incremental Linker (MSIL)

  (c) 2011 Stijn Volckaert (svolckae@elis.ugent.be)
=============================================================================*/

#include <diabloobject.h>
#include "diablomsil.h"

/*-----------------------------------------------------------------------------
  Global Variables
-----------------------------------------------------------------------------*/
static int     msil_module_usecount = 0;
static t_uint8 msil_parser_state    = 0;

/*-----------------------------------------------------------------------------
  IsMsilMap
-----------------------------------------------------------------------------*/
t_bool
IsMsilMap(FILE* fp)
{
  int fpos = ftell (fp);
  t_string parse_line = FileGetLine (fp);
  int found_count = 0;

  /* first line = file name */
  if (parse_line)
  {
    Free(parse_line);
    parse_line = FileGetLine(fp);
  }

  /* next we should find the timestamp and the preferred load address */
  while (parse_line
    && ((*parse_line == '\n')
      || (StringPatternMatch(" Timestamp is*", parse_line))
      || (StringPatternMatch(" Preferred load address is*", parse_line))))
  {
	if (*parse_line != '\n')
	  found_count++;
    Free (parse_line);
    parse_line = FileGetLine (fp);

    continue;
  }

  if (found_count != 2) {
	  fseek(fp, fpos, SEEK_SET);
	  return FALSE;
  }

  VERBOSE(0, ("Identified Microsoft Incremental Linker Map"));
  Free (parse_line);
  fseek (fp, fpos, SEEK_SET);
  return TRUE;
}

/*-----------------------------------------------------------------------------
  MsilChangeParserState
-----------------------------------------------------------------------------*/
void
MsilChangeParserState(t_uint8 new_state)
{
  msil_parser_state = new_state;
}

/*-----------------------------------------------------------------------------
  MsilNode
-----------------------------------------------------------------------------*/
t_map_node*
MsilNode(t_const_string symbol_name, t_section* parent_section, t_const_string object_name,
  t_uint32 symbol_offset, t_bool symbol_is_generated, t_bool is_static)
{
  t_map_node* ret = (t_map_node*)Malloc(sizeof(t_map_node));

  ret->base_addr        = SECTION_CADDRESS(parent_section) + symbol_offset;
  /* we cannot determine the size at this point since the msil linker maps lack this info.
  In some cases we _could_ find an upper bound though... */
  ret->sec_size         = 0;
  ret->object           = StringDup(object_name);
  ret->sec_name         = StringDup(symbol_name);
  ret->mapped_sec_name  = StringDup(SECTION_NAME(parent_section));
  /* We might refine the type and attributes later */
  ret->type             = symbol_is_generated ? Data : Code;
  ret->attr             = RW;
  /* Need to fix this later... */
  ret->builtin          = /*symbol_is_generated*/ FALSE;
  /* Default values */
  ret->sec_idx          = 0;
  ret->obj              = NULL;
  ret->has_sec_idx      = FALSE;

  VERBOSE(0, ("New Node - section: %s - object: %s", symbol_name, object_name));

  return ret;
}

/*-----------------------------------------------------------------------------
  MsilProcessLine
-----------------------------------------------------------------------------*/
void
MsilProcessLine(t_map* map, t_const_string line)
{
  if (strlen(line) > 0)
  {
    /* check for state changes */
    if (!StringCmp(line, " Start         Length     Name                   Class"))
    {
      MsilChangeParserState(1); /* reading sections */
      return;
    }
    else if (!StringCmp(line, "  Address         Publics by Value              Rva+Base       Lib:Object"))
    {
      MsilChangeParserState(2); /* reading public symbols */
      return;
    }
    else if (!StringCmp(line, " Static symbols"))
    {
      MsilChangeParserState(3); /* reading static symbols */
      return;
    }
    else if (!StringCmp(line, " Exports") || !StringCmp(line, "  ordinal    name"))
    {
      MsilChangeParserState(4); /* reading exported symbols */
      return;
    }
    else if (StringPatternMatch(" entry point at*", line))
    {
      /* this info can be read from the PE file as well... */
      MsilChangeParserState(0);
      return;
    }
    else
    {
      switch(msil_parser_state)
      {
        /* (sub)section info */
        case 1:
        {
          t_section*  tmp_section       = NULL;
          t_section*  parent_section    = NULL;
          t_section*  subsection        = NULL;
          t_uint32    i                 = 0;
          t_uint32    parent_section_nr = 0;
          t_uint32    subsection_offset = 0;
          t_uint32    subsection_size   = 0;
          char        subsection_class[1000];
          char        subsection_name[1000];

          sscanf(line, " %04x:%08x %08xH %s %s", &parent_section_nr, &subsection_offset,
            &subsection_size, subsection_name, subsection_class);

          /* unfortunately, the parent sections are only referenced by index in the linker map... */
          OBJECT_FOREACH_SECTION(map->obj, tmp_section, i)
          {
            if (SECTION_INDEX_IN_OBJ(tmp_section) == parent_section_nr)
            {
              parent_section = tmp_section;
              break;
            }
          }

          /* Levine, p152 */
          if (subsection_size > 0)
          {
            VERBOSE(0, ("Adding subsection: %s to parent-section: %s - offset: %08x",
              subsection_name, SECTION_NAME(parent_section), subsection_offset));

            /* then add a pseudo-symbol to the beginning of the new section */
            SymbolTableAddSymbol(
              OBJECT_SYMBOL_TABLE(map->obj),                    /* add to the parent object's symbol table (i.e. the PE file) */
              subsection_name,
              "R00$",
              0, PERHAPS,                                       /* this symbol is NOT a duplicate so the order will be ignored */
              NO,                                               /* no idea what this even means... */
              T_RELOCATABLE(parent_section),
              0,
              0,
              NULL,
              AddressNew32(subsection_size),
              0);

            /* we somehow need to remember what offset the subsection was at
             * => add a new generated symbol
             */
            strcat(subsection_name, "$range$");
            SymbolTableAddSymbol(
              OBJECT_SYMBOL_TABLE(map->obj),
              subsection_name,
              "R00$",
              0, NO,
              NO,
              T_RELOCATABLE(parent_section),
              subsection_offset,
              0,
              NULL,
              subsection_size,
              0);
          }

          break;
        }
        /* symbol info */
        case 2:
        case 3:
        {
          t_section*  parent_section      = NULL;
          t_section*  tmp_section         = NULL;
          t_uint32    i                   = 0;
          t_uint32    section_num         = 0;
          t_uint32    section_offset      = 0;
          t_uint32    virtual_address     = 0;
          t_bool      generated           = TRUE;   /* linker generated symbol? */
          char        symbol_name[2048];
          char        object_name[2048];

          sscanf(line, "%04x:%08x %s %08x %s",
            &section_num, &section_offset, symbol_name, &virtual_address, object_name);

          /* symbols that correspond with code have an f character between the VA and the object name */
          if (!StringCmp(object_name, "f"))
          {
            sscanf(line, "%04x:%08x %s %08x %*s %s",
              &section_num, &section_offset, symbol_name, &virtual_address, object_name);
            generated = FALSE;

            /* there might be an i here too... TODO: Figure out what this means! */
            if (!StringCmp(object_name, "i"))
            {
              sscanf(line, "%04x:%08x %s %08x %*s %*s %s",
                &section_num, &section_offset, symbol_name, &virtual_address, object_name);
            }
          }

          /* section_num 0 is reserved for linker generated symbols, which will
          not resolve to any parent section */
          if (section_num)
          {
            OBJECT_FOREACH_SECTION(map->obj, tmp_section, i)
            {
              if (SECTION_INDEX_IN_OBJ(tmp_section) == section_num)
              {
                parent_section = tmp_section;
                break;
              }
            }
          }

          /* Check if the library has an extension and add it manually if needed */
          if (StringPatternMatch("??*:*", object_name)
            && !StringPatternMatch("??*.lib:*", object_name))
          {
            t_uint32 len              = strlen(object_name);
            t_uint32 last_semicolon   = 0;
            t_string new_object_name  = (t_string)Malloc(len+1+4);

            for (i = 0; i < len; ++i)
            {
              if (object_name[i] == ':')
                last_semicolon = i;
            }

            object_name[last_semicolon] = '\0';
            strcpy(new_object_name, object_name);
            strcat(new_object_name, ".lib:");
            strcat(new_object_name, object_name + last_semicolon + 1);
            strcpy(object_name, new_object_name);
            Free(new_object_name);
          }

          /* append symbol name to the object in case of a dll member */
          if (StringPatternMatch("??*:*.dll", object_name)
            || StringPatternMatch("??*:*.DLL", object_name))
          {
            strcat(object_name, ";");
            strcat(object_name, symbol_name);
          }

          if (!StringPatternMatch("<*>", object_name))
          {
            t_string real_symbol_name;
            t_uint32 real_symbol_name_len;
            t_symbol* symptr;
            t_symbol* tmpptr;
            t_symbol* parent_symbol = NULL;

            /* resolve to subsection */
            OBJECT_FOREACH_SYMBOL_SAFE(map->obj, symptr, tmpptr)
            {
              if (SYMBOL_BASE(symptr) == T_RELOCATABLE(parent_section))
              {
                /* look for the $range$ symbol */
                if (strstr(SYMBOL_NAME(symptr), "$range$")
                  && SYMBOL_OFFSET_FROM_START(symptr) <= section_offset
                  && SYMBOL_OFFSET_FROM_START(symptr) + SYMBOL_SIZE(symptr) > section_offset)
                {
                  parent_symbol = symptr;
                  break;
                }
              }
            }

            ASSERT(parent_symbol, ("Found symbol: %s in map file but could not resolve its parent section", symbol_name));

            real_symbol_name_len = strlen(SYMBOL_NAME(parent_symbol)) - 6;
            real_symbol_name = (t_string)Malloc(real_symbol_name_len);
            memcpy(real_symbol_name, SYMBOL_NAME(parent_symbol), real_symbol_name_len - 1);
            real_symbol_name[real_symbol_name_len-1] = '\0';

            MapInsertNode(map, MsilNode(real_symbol_name, parent_section, object_name,
              AddressNew32(section_offset), FALSE, FALSE));

           SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(map->obj),
              symbol_name,
              "R00$",
              -1, PERHAPS,
              NO,
              T_RELOCATABLE(SYMBOL_BASE(parent_symbol)),
              section_offset,
              0,
              NULL,
              AddressNullForObject(map->obj),
              0); /* the length of the symbol cannot be derived from the linker map */

           Free(real_symbol_name);
          }
          else
          {
            ASSERT(OBJECT_ABS_SECTION(map->obj), ("Found linker generated symbol but parent object has no ABS section"));
            SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(map->obj),
              symbol_name,
              "R00A00+$",
              0, NO,
              NO,
              T_RELOCATABLE(OBJECT_ABS_SECTION(map->obj)),
              AddressNew32(virtual_address),
              AddressNullForObject(map->obj),
              NULL,
              AddressNew32(OBJECT_ADDRESS_SIZE(map->obj)),
              0);
          }
        }
        /* exported symbols - we can just discard this info. */
        /* This can also be read directly from the PE binary */
        case 4:
          break;
      }
    }
  }
}

/*-----------------------------------------------------------------------------
  MsilReadMap
-----------------------------------------------------------------------------*/
void
MsilReadMap(FILE* fp, t_map* map)
{
  t_uint32    i           = 0;
  t_uint32    j           = 0;
  t_int32     file_size   = 0;
  t_int32     bytes_read  = 0;
  t_uint32    line_pos    = 0;
  void*       file_buffer = NULL;
  t_section*  section     = NULL;
  t_section*  subsection  = NULL;

  STATUS(START, ("Reading Microsoft Incremental Linker Map"));

  msil_parser_state = 0;

  /* Shove the entire linker map into one buffer */
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  ASSERT(file_size > 0, ("Could not read linker map. Invalid filesize."));
  file_buffer = Malloc(file_size);
  memset(file_buffer, 0, file_size);
  bytes_read = fread(file_buffer, 1, file_size, fp);

  /* bytes_read might not be equal to filesize. Diablo opens linker maps in text mode */
  ASSERT(bytes_read > 0, ("Could not read from linker map. fread error. bytes read: %d", bytes_read));

  /* Chop it up and start processing lines! */
  for (i = 0; i < file_size; ++i)
  {
    t_uint8 c = ((t_uint8*)file_buffer)[i];
    if (c == 10 || c == 13)
    {
      ((t_uint8*)file_buffer)[i] = 0;
      MsilProcessLine(map, (t_string)((t_string)file_buffer + line_pos));
      line_pos = i + 1;
    }
  }
  if (line_pos < file_size)
  {
    t_string last_line = (t_string)Malloc(file_size - line_pos + 1);
    memcpy(last_line, (const void*)((unsigned int)file_buffer + line_pos), file_size - line_pos);
    last_line[file_size - line_pos] = '\0';
    MsilProcessLine(map, last_line);
    Free(last_line);
  }

  Free(file_buffer);
  STATUS(STOP, ("Reading Microsoft Incremental Linker Map"));
}

/*-----------------------------------------------------------------------------
  DiabloMsilInit
-----------------------------------------------------------------------------*/
void
DiabloMsilInit (int argc, char **argv)
{
  if (!msil_module_usecount)
  {
    MapHandlerAdd ("MSIL",  IsMsilMap, MsilReadMap);
  }

  msil_module_usecount++;
}

/*-----------------------------------------------------------------------------
  DiabloMsilFini
-----------------------------------------------------------------------------*/
void
DiabloMsilFini()
{
  msil_module_usecount--;
}
