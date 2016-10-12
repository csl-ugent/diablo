#include <diablotilinker.h>
/* Node GNU ld constructor: TODO rename {{{ */
/*! 
 * parse a line from an arm map-file, returns a t_map_node.  
 * Memory allocated, don't forget to free! 
 */

t_map_node *
TiLinkerNode (t_string name, t_string mapped_name, t_address start, t_address size, t_string file, t_string lib)
{
  t_map_node *ret = Calloc (1, sizeof (t_map_node));
  ret->base_addr = start;
  ret->sec_size = size;
  if ((name[0] != '(') || (name[strlen(name)-1] != ')'))
  {
    FATAL (("Expected sections to be inside braces"));
    /* t_string dup = StringDup (file);
       t_string lib = strtok (dup, "(");
       t_string obj = strtok (NULL, ")");
       t_string Name = NULL;
       ASSERT (obj, ("Illegal objectname in map: %s", file));
       ASSERT (lib, ("Illegal objectname in map: %s", file));
       Free (dup);
       ret->object = Name; */
  }
  else
  {
    name[strlen(name)-1]='\0';
    ret->sec_name = StringDup (name+1);
  }

  if (lib)
  {
    ret->object = StringConcat3 (lib, ":", file);
  }
  else
  {
    ret->object = StringDup (file);
  }

  ret->mapped_sec_name = StringDup (mapped_name);
  VERBOSE (2, ("Section name= %s, starts at %x, size=%x, from file %s", name,
            start, size, file));
  return ret;
}

/* }}} */


t_bool IsTiLinkerMap(FILE * fp)
{
  int fpos=ftell(fp);
  t_string parse_line=FileGetLine(fp);
  while ((parse_line) && ((*parse_line == '\n')||(strncmp("********************",parse_line,20)==0))) /* Skip empty lines */
  {
    Free(parse_line);   /* avoids leaks */
    parse_line=FileGetLine(fp);
    continue;
  }

  if (!parse_line) return FALSE;

  if ((!(StringPatternMatch("*TMS320C6x*COFF*Linker*PC*",parse_line)))
  && (!(StringPatternMatch("*TMS470*COFF*Linker*PC*",parse_line)))
  && (!(StringPatternMatch("*TMS470*ELF*Linker*Unix*",parse_line))))
  {
    fseek(fp,fpos,SEEK_SET);
    Free(parse_line);
    return FALSE;
  }

  Free(parse_line);

  fseek(fp,fpos,SEEK_SET);
  return TRUE;
}
extern FILE * ParseMapTiLinkerin;
extern t_map *  tilinker_parser_map;
extern void ParseMapTiLinkerparse();

void TiLinkerReadMapF(FILE * ifp, t_map * map)
{
	tilinker_parser_map=map;
	ParseMapTiLinkerin=ifp;
	ParseMapTiLinkerparse(); 
}

void DiabloTiLinkerInit(int argc, char ** argv)
{
  MapHandlerAdd("TILINKER",  IsTiLinkerMap, TiLinkerReadMapF);
}

void DiabloTiLinkerFini(int argc, char ** argv)
{
  
}
