#include <string.h>
#include <diabloobject.h>
#include <diablotru64linker.h>

t_bool
IsTru64LinkerMap (FILE * fp)
{
  int fpos = ftell (fp);
  t_string parse_line = FileGetLine (fp);
  while ((parse_line) && (*parse_line == '\n'))	/* Skip empty lines */
  {
    Free (parse_line);	/* avoid leaks */
    parse_line = FileGetLine (fp);
    continue;
  }

  if (!parse_line) return FALSE;

  if (!(StringPatternMatch ("*LINK EDITOR MEMORY MAP*", parse_line)))
  {
    fseek (fp, SEEK_SET, fpos);
    Free (parse_line);
    return FALSE;
  }

  Free (parse_line);
  fseek (fp, SEEK_SET, fpos);
  return TRUE;
}

/*!
 * The following function parses a line from a map file produced by the Tru64/Digital Unix linker 
 *
 * \param line The line to parse
 *
 * \return the parsed node 
 */

t_map_node *
Tru64LinkerMapParseLine (t_string line)
{
  /* static t_uint16 tel = 0; */
  t_map_node *new_node = NULL;
  t_string_array *array = StringDivide (line, " \t\n", FALSE, FALSE);
  t_string joined = StringArrayJoin (array, ":");
  static t_string last = NULL;
  t_uint64 sec_start, sec_size;
  t_uint64 tmp;
  t_uint16 size = StringArrayNStrings (array);

  ASSERT ((size == 4) || (size == 3) || (size == 6), ("Alpha Map file parse error! Line= %s has a wrong number of fields (%d)", line, size));
  
  if (strcmp (joined, "LINK:EDITOR:MEMORY:MAP") == 0)
    return NULL;
  if (strcmp (joined, "output:input:virtual") == 0)
    return NULL;
  if (strcmp (joined, "section:section:address:size") == 0)
    return NULL;

  if (size == 6)
  {
    if ((strcmp (array->first->next->next->next->string, "exception") == 0) 
        && (strcmp (array->first->next->next->next->next->string, "gp") == 0) 
        && (strcmp (array->first->next->next->next->next->next->string, "info") == 0))
    {
      WARNING (("Exception gp info... Not supported... Maybe this object has multiple gp's? Or is it dynamically linked?"));
      return NULL;
    }
    else
    {
      FATAL (("Expected exception gp info: %s", line));
    }

  }
  if (size == 3)
  {
    if (last)
      if (strcmp (last, array->first->string) == 0)
        FATAL (("Parse error in %s", joined));
    last = StringDup (array->first->string);    
    sec_start = strtoll (array->first->next->string, NULL, 16);	/* WARNING! On Linux, use strtolll */
    sec_size = strtoll (array->first->next->next->string, NULL, 16);
    return NULL;
  }
  else
  {
    new_node = (t_map_node *) Calloc (1, sizeof (t_map_node));

    new_node->obj = NULL;
    if (!last)
      FATAL (("Parse error in %s", joined));
    if (strcmp (last, array->first->string) != 0)
      FATAL (("Parse error in %s", joined));
    /* Field 1: The name of the original section */
    new_node->sec_name = StringDup (array->first->string);
    new_node->mapped_sec_name = StringDup (last);
    if ((strcmp (new_node->sec_name, ".text") == 0) || (strcmp (new_node->sec_name, ".init") == 0) || (strcmp (new_node->sec_name, ".fini") == 0))
    {
      new_node->type = Code;
    }
    else if ((strcmp (new_node->sec_name, ".data") == 0) || (strcmp (new_node->sec_name, ".xdata") == 0) || (strcmp (new_node->sec_name, ".rdata") == 0)
             || (strcmp (new_node->sec_name, ".rconst") == 0) || (strcmp (new_node->sec_name, ".pdata") == 0) 
             || (strcmp (new_node->sec_name, ".lita") == 0) || (strcmp (new_node->sec_name, ".sdata") == 0))
    {
      new_node->type = Data;
    }
    else if ((strcmp (new_node->sec_name, ".bss") == 0) || (strcmp (new_node->sec_name, ".sbss") == 0))
    {
      new_node->type = Zero;
    }
    else
      FATAL (("Unknown section %s", last));

    /* Field 2: The start of the section */
    tmp = strtoll (array->first->next->string, NULL, 16);
    new_node->base_addr = AddressNew64 (tmp);
    if (tmp < 0x140000000LL)
    {
      new_node->attr = RO;
    }
    else
    {
      new_node->attr = RW;
    }
    /* Field 3: The size of the section */
    tmp = strtoll (array->first->next->next->string, NULL, 16);
    new_node->sec_size = AddressNew64 (tmp);
    /* Field 4: The object name */
    if ((array->first->next->next->next->string[0] != '(') && (StringPatternMatch ("*(*)", array->first->next->next->next->string)))
    {
      t_string dup = StringDup (array->first->next->next->next->string);
      t_string lib = strtok (dup, "(");
      t_string obj = strtok (NULL, ")");
      t_string Name = NULL;
      ASSERT (obj, ("Illegal objectname in map: %s", array->first->next->next->next->string));
      ASSERT (lib, ("Illegal objectname in map: %s", array->first->next->next->next->string));
      Name = StringConcat3 (lib, ":", obj);
      Free (dup);
      new_node->object = Name;
    }
    else
    {
      new_node->object = StringDup (array->first->next->next->next->string);
    }
    return new_node;

  }
  /* exit(0); *//* Should be impossible ! */
}

/*}}}*/

/*! 
 * \param obj The object for which we want to read the map file
 *
 * \param infile a string containing the name of the object 
 *
 * his function parses the file `infile.map', and stores it into 
 * obj->map. If no map if found obj->map is NULL, and a warning is printed.
 */

void
Tru64LinkerReadMapF (FILE * ifp, t_map * map)
{
  t_map_node *parsed_node;
  t_string parse_line = NULL;

  STATUS(START, ("Parsing Tru64linker map file"));

  fseek (ifp, 0, SEEK_END);
  ASSERT (ftell (ifp), ("Map file %s is an empty file! Remove and add a correct map file"));
  fseek (ifp, 0, SEEK_SET);

  while ((parse_line = FileGetLine (ifp)) != NULL)	/* while EOF not reached */
  {
    if (StringPatternMatch ("*LINK*EDITOR*MEMORY*MAP*", parse_line))
      break;
    Free (parse_line);	/*  avoid leaks */
  }

  if (!parse_line)
    FATAL (("Did not find any real dat in this Tru64Linker map file (or did not encounter the LINK EDITOR MEMORY MAP marker line. Map file probably corrupted!"));
  Free (parse_line);

  while ((parse_line = FileGetLine (ifp)) != NULL)	/* while EOF not reached */
  {
    if (*parse_line == '\n') /* Skip empty lines */
    {
      Free (parse_line);	/*  avoid leaks */
      continue;
    }

    /* Parse one line */
    parsed_node = Tru64LinkerMapParseLine (parse_line);
    if (parsed_node)
    {
      if (parsed_node->type != Dbug)
        MapInsertNode (map, parsed_node);
      else
        MapNodeFree (parsed_node);	/* If this is a line, with debugging 
                                           info, we delete the node, because we
                                           do not need this information */
    }
    Free (parse_line);	/* avoid leaks */
  };

  STATUS(STOP, ("Parsing Tru64linker map file"));
}

static int tru64linker_module_usecount = 0;

void
DiabloTru64LinkerInit (int argc, char **argv)
{
  if (!tru64linker_module_usecount)
  {
    MapHandlerAdd ("TRU64LINKER", IsTru64LinkerMap, Tru64LinkerReadMapF);
  }

  tru64linker_module_usecount++;
}

void
DiabloTru64LinkerFini()
{
  tru64linker_module_usecount--;
}
