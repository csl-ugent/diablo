/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablobinutils.h>
/* Node GNU ld constructor: TODO rename {{{ */
/*! 
 * parse a line from an arm map-file, returns a t_map_node.  
 * Memory allocated, don't forget to free! 
 */

t_map_node *
BinutilsNode (t_string name, t_string mapped_name, t_address start, t_address size, t_string file, t_bool builtin)
{
  t_map_node *ret = Calloc (1, sizeof (t_map_node));
  ret->base_addr = start;
  ret->sec_size = size;

  /* Check whether the object is part of a library */
  if ((file[0] != '(') && (StringPatternMatch ("*(*)", file)))
    {
      t_string dup = StringDup (file);
      t_string lib = strtok (dup, "(");
      t_string obj = strtok (NULL, ")");
      t_string Name = NULL;
      char* loc;
      ASSERT (obj, ("Illegal objectname in map: %s", file));
      ASSERT (lib, ("Illegal objectname in map: %s", file));
      Name = StringConcat3 (lib, ":", obj);
      ret->object = Name;

      /* Find the directory and add it to the library path */
      loc = strrchr(lib, '/');
      /* in case there is no /, it means the library is in the current
       * directory, which is already searched by diablo by default
       */
      if (loc)
      {
        *loc = '\0';
        PathAddDirectory (diabloobject_options.libpath, lib);
      }

      Free (dup);
    }
  else
    {
      ret->object = StringDup (file);
    }
  ret->sec_name = StringDup (name);
  ret->mapped_sec_name = StringDup (mapped_name);
  ret->builtin = builtin;
  VERBOSE (2,
	   ("Section name= %s, starts at @G, size=@G, from file %s", name,
	    start, size, file));
  return ret;
}

/* }}} */


t_bool IsBinutilsMap(FILE * fp)
{
  int fpos=ftell(fp);
  t_string parse_line;
  parse_line = FileGetLine(fp);

  while ((parse_line) && (*parse_line == '\n')) /* Skip empty lines */
  {
    Free(parse_line);   /* avoids leaks */
    parse_line=FileGetLine(fp);
    continue;
  }

  if (!parse_line) return FALSE;

  if (strncmp(parse_line,"Archive member",14)!=0 &&
      strncmp(parse_line,"Memory Configuration",20)!=0 &&
      strncmp(parse_line,"Allocating common symbols",25)!=0 &&
      strncmp(parse_line,"Discarded input sections",24)!=0)
  {
    fseek(fp,fpos,SEEK_SET);
    Free(parse_line);
    return FALSE;
  }

  Free(parse_line);

  fseek(fp,fpos,SEEK_SET);
  return TRUE;
}
extern FILE * ParseMapBinutilsin;
extern t_map *  binutils_parser_map;
extern void ParseMapBinutilsparse();
#ifdef DIABLOOBJECT_FLEX_HAS_DESTROY
extern int ParseMapBinutilslex_destroy ();
#endif

void BinutilsReadMapF(FILE * ifp, t_map * map)
{
	binutils_parser_map=map;
	ParseMapBinutilsin=ifp;
	ParseMapBinutilsparse(); 
	ParseMapBinutilsin=NULL;
#ifdef DIABLOOBJECT_FLEX_HAS_DESTROY
	ParseMapBinutilslex_destroy();
#endif
}

static int binutils_module_usecount = 0;

void DiabloBinutilsInit(int argc, char ** argv)
{
  if (!binutils_module_usecount)
  {
    MapHandlerAdd("BINUTILS_LD",  IsBinutilsMap, BinutilsReadMapF);
  }
  binutils_module_usecount++;
}

void DiabloBinutilsFini(int argc, char ** argv)
{
  binutils_module_usecount--;
}
