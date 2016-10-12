#include <diabloobject.h>
#include <diabloarmads.h>
/* include <diabloarmads_cmdline.h>  -- ArmAds cmdline options file is empty */

t_bool
IsArmAdsMap(FILE * fp)
{
  int fpos = ftell (fp);
  t_string parse_line = FileGetLine (fp);
  while ((parse_line) && ((*parse_line == '\n')||(StringPatternMatch("ARM Linker, RVCT*", parse_line))||(StringPatternMatch("Your license for*", parse_line))))
  {
    Free (parse_line);	/* avoid leaks */
    parse_line = FileGetLine (fp);
    continue;
  }

  if (!parse_line) return FALSE;

  if (strncmp(parse_line,"==============",14)!=0) 
  {
    fseek (fp, fpos, SEEK_SET);
    Free (parse_line);
    return FALSE;
  }

  Free (parse_line);
  fseek (fp, fpos, SEEK_SET);
  return TRUE;
}

/*!
 * The following function parses a line from an Arm ADS map file. 
 *
 * \param region The name of the parent section (for ADS binaries this is
 * ER_RO, ER_RW, or ER_ZI).  
 *
 * \param line The line to parse
 *
 * \return the parsed node 
 */

t_map_node *
ArmAdsMapParseLine (t_string region, t_string line)
{
  t_map_node * new_node = (t_map_node*) Calloc(1, sizeof(t_map_node));
  t_string_array *array = StringDivide (line, " \t\n", FALSE, FALSE);
  
  t_bool is_eight_fielder; 
  t_uint32 tmp=0; 
  t_string_array_elem * iter=NULL;

  ASSERT (region, ("Could not parse ARM ADS map file: in ArmAdsMapParseLine: region not set. This typically means there is data in the map file we could not parse!"));
  
  new_node->mapped_sec_name=StringDup(region);
  /* there should be 7 or 8 fields exept when there is padding in the map
   * file. Then there are 3 fields, and the last field is "PAD" */

  if ((StringArrayNStrings(array)==3) && (strcmp(array->first->next->next->string,"PAD")==0))
  {
    new_node->type=Dbug; /* This makes the reader throw away this element */
    new_node->object=StringDup("PAD");
    new_node->sec_name=StringDup("PAD");
	  
    StringArrayFree(array); /* Cleanup */
    return new_node;
  }

  /* the other should all have 7 or 8 fields */
  ASSERT (((StringArrayNStrings(array)==8)||(StringArrayNStrings(array)==7)),("Parse error in map file at line %s",line));

  /* Field 1: the start of the section in the final object */
  iter=array->first;
  ASSERT(((iter->string[0]=='0')||(iter->string[1]=='x')), ("Parse error in map file at line %s: first field should be a hex-number",line)); 
  
  tmp=strtol(iter->string, NULL, 16);
  new_node->base_addr=AddressNew32(tmp);
  /* Field 2: the size of the section in the final object */
  iter=iter->next;
  ASSERT(((iter->string[0]=='0')||(iter->string[1]=='x')), ("Parse error in map file at line %s: second field should be a hex-number",line));
  tmp=strtol(iter->string, NULL, 16);
  new_node->sec_size=AddressNew32(tmp);
  
  is_eight_fielder=(StringArrayNStrings(array)==8);

  /* Field 3: the type of the section in the final object */
  iter=iter->next;
  if (strcmp(iter->string,"Code")==0) new_node->type=Code;
  else if (strcmp(iter->string,"Ven")==0) new_node->type=Dbug;
  else if (strcmp(iter->string,"Data")==0) new_node->type=Data;
  else if (strcmp(iter->string,"Dbug")==0) new_node->type=Dbug;
  else if (strcmp(iter->string,"Zero")==0) new_node->type=Zero;
  else FATAL(("Unknown type for block (%s)!",iter->string));

  /* Field 4: the attributes of the section in the final object */
  iter=iter->next;
  if (strcmp(iter->string,"RO")==0) new_node->attr = RO;
  else if (strcmp(iter->string,"RW")==0) new_node->attr = RW;
  else FATAL(("Unknown attribute for block!"));

  /* Field 5: An index, which we ignore (unless --armads-map-use-indexes is specified) */

  iter=iter->next;
  if (diabloobject_options.use_map_indexes)
  {
    new_node->sec_idx=strtol(iter->string, NULL, 10);
    new_node->has_sec_idx=TRUE;
  }

  /* Field 6(is_eight_fielder) : Marks an entry */ 
  if (is_eight_fielder)
  {
    iter=iter->next;
    ASSERT((iter->string[0]=='*')&&(iter->string[1]=='\0'), ("Parse error, if fields==8, then the 5 element should be a star! in %s",line));
  }

  /* Field 6(is_seven_fielder),7(is_eight_fielder): The sections original name */
  iter=iter->next;
  new_node->sec_name=StringDup(iter->string);
  
  /* Field 7(is_seven_fielder),8(is_eight_fielder): The objects original name */
  iter=iter->next;

  if (StringPatternMatch("*(*)(*)",iter->string))
   {
      t_string dup=StringDup(iter->string);
      t_string lib=strtok(dup,"(");
      t_string obj=strtok(NULL,")");
      t_string Name=StringConcat3(lib,":",obj);
      ASSERT(obj,("Illegal objectname in map: %s",iter->string));
      ASSERT(lib,("Illegal objectname in map: %s",iter->string));
      Free(dup);
      new_node->object=Name;
   }
  else if (StringPatternMatch("*(*)",iter->string))
   {
      t_string dup=StringDup(iter->string);
      t_string obj=strtok(dup,"(");
      t_string lib=strtok(NULL,")");
      t_string Name, tmp;
      
      /* The ARM guys are having fun with this it seems:
       *   a) ADS 2.1-4.0: objname(libname)
       *   b) ADS 4.1 build 462: libname(libname)(objname) (handled above)
       *   c) ADS 4.1 build 713: libname(objname)
       *  -> use heuristics to discern between cases a) and c)
       */
      if (StringPatternMatch("*.o",lib) &&
          (StringPatternMatch("*.l",obj) ||
           StringPatternMatch("*.a",obj)))
      {
        tmp = lib;
        lib = obj;
        obj = tmp;
      }
      Name=StringConcat3(lib,":",obj);
      ASSERT(obj,("Illegal objectname in map: %s",iter->string));
      ASSERT(lib,("Illegal objectname in map: %s",iter->string));
      Free(dup);
      new_node->object=Name;
   }
   else
   {
      new_node->object=StringDup(iter->string);
   }


  StringArrayFree(array); /* Cleanup */
  new_node->obj=NULL;
  return new_node;
}

/*! 
 * \param obj The object for which we want to read the map file
 *
 * \param infile The name of the mapfile (without the .map extension)
 *
 * Parses the file `infile.map', and stores the parsed information in the
 * `map' field of the given object. If no map file exists map is NULL.  This
 * function actually only parses map files generated by the ADS. If the map
 * file was generated by gcc, this function will detect it and a generic GCC
 * parser will be called.
 */

void
ArmAdsReadMapF (FILE * ifp, t_map * map)
{
  t_map_node *parsed_node;
  t_string region = NULL;
  t_string parse_line = NULL;

  fseek (ifp, 0, SEEK_END);
  ASSERT (ftell (ifp), ("Map file %s is an empty file! Remove and add a correct map file"));
  fseek (ifp, 0, SEEK_SET);

  while ((parse_line = FileGetLine (ifp)) != NULL)	/* while EOF not reached */
  {
    if (*parse_line == '\n') /* Skip empty lines */
    {
      Free (parse_line);	/*  avoid leaks */
      continue;
    }

    if (StringPatternMatch("*Execution Region*",parse_line))
    {
      t_string_array * array=StringDivide(parse_line," \t\n",FALSE,FALSE);
      if (region) Free(region);
      region=StringDup(array->first->next->next->string);
      Free(parse_line);   /* avoid leaks */
      StringArrayFree(array);
      continue;
    }
    /* tricky test to see if this line is a line containing section-info,
     * relies on the fact that the address field start at the fifth character
     * of the line */
    else if ((strstr((const char*)parse_line, "0x") - (char *) parse_line) != 4)
    {
      /* It's not section info! */
      Free(parse_line);   /* avoid leaks */
      continue;
    }

    /* we only get here if the line is actually containing real map data */
    /* Parse one line */
    parsed_node = ArmAdsMapParseLine (region, parse_line);
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

  if (region) Free(region);
  VERBOSE (1, ("Parsing done"));
}

static int armads_module_usecount = 0;

void
DiabloArmAdsInit (int argc, char **argv)
{
  if (!armads_module_usecount)
  {
    MapHandlerAdd ("ADS_ARMLINK",  IsArmAdsMap, ArmAdsReadMapF);
  }

  armads_module_usecount++;
}

void
DiabloArmAdsFini()
{
  armads_module_usecount--;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
