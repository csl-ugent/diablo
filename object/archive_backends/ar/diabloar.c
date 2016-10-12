/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Archive (arch/ar) handler backend. This backend handles (or should handle):
 *
 * - GNU AR files (produced by binutils ar, armar and sometimes code composer) 
 * - BSD AR files (detected but not handled)
 * - ECOFF AR files (produced by Tru64Unix ar)
 * - CodeComposer LIB files (incomplete)
 * - MS VC++ AR files (produced by Visual Studio)
 *
 * As the arch format has never been standardized, one of the biggest problems
 * is distinguishing between these different fileformats. As an example,
 * CodeComposer LIB files appear to be valid GNU AR files, however, the archive
 * header is padded with an extra '\0'. If we would process them as GNU AR
 * files, all files will start with this '\0', which of course, we do not want. 
 * 
 * Currently, the following algorithm is used to distinguish between different
 * formats:
 *
 * 1. ECOFF AR files are handled by a separate function, and we distinguish
 * between these files and the other AR files by looking at the first archive
 * header. For ECOFF this is ________64E. 
 *
 * 2. If ar_gid and ar_uid are blanked out (all spaces), we're dealing with an
 * MS VC++ file.
 *
 * 3. If the ar_uid == 0 for all archive headers or there is a file called
 * <filenames> and all archive headers are padded with a trailing '\0' we
 * conclude that we are working on a CodeComposer LIB file.
 *
 * 4. Otherwise we have either a GNU or BSD AR file. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <diabloobject.h>
#include <diabloar.h>

t_bool
IsArchAr (FILE * fp)
{
  /* Buffer to hold the magic string and the name of the first archive header.
   * The name of the first archive header will be used to differentiate between
   * GNU/BSD AR files and ECOFF AR files */
  char buffer[SARMAG + 40];	
  long fpos = ftell (fp);

  /* Read the magic string and the name of the first archive header */
  int ret = fread (buffer, SARMAG + 40, 1, fp);
  fseek (fp, fpos, SEEK_SET);
  
  /* If the file is smaller then SARMAG + 16 it certainly is no AR file */
  if (ret != 1)
    return FALSE;

  /* If the first name in the archive is "________64E" we are dealing with an
   * the ECOFF AR variant, not normal AR */
  if (strncmp ("________64E", buffer + SARMAG, 11) == 0)
    return FALSE;
 
  /* It's not ALPHA but we could still be dealing with MS or GNU/BSD */
  if (strncmp (ARMAG, buffer, SARMAG) == 0)
  {
    /* if gid and uid are blanked out, we're dealing with an MS archive */
    if (strncmp("      ", buffer + SARMAG + 16 + 12, 6) == 0
      && strncmp("      ", buffer + SARMAG + 16 + 12 + 6, 6) == 0)
      return FALSE;
    /* if not it's GNU/BSD! */
    return TRUE;
  }

  return FALSE;
}

t_bool
IsArchAlphaAr (FILE * fp)
{
  /* Buffer to hold the magic string and the name of the first archive header.
   * The name of the first archive header will be used to differentiate between
   * GNU/BSD AR files and ECOFF AR files */
  char buffer[SARMAG + 16];
  long fpos = ftell (fp);
  
  /* Read the magic string and the name of the first archive header */
  int ret = fread (buffer, SARMAG + 16, 1, fp);
  fseek (fp, fpos, SEEK_SET);
  
  /* If the file is smaller then SARMAG + 16 it certainly is no AR file */
  if (ret != 1)
    return FALSE;
  
  /* If the first name in the archive is "________64E" we are dealing with an
   * the ECOFF AR variant, if the magic string is correct we have an ECOFF AR
   * file */
  if ((strncmp ("________64E", buffer + SARMAG, 11) == 0)
      && (strncmp (ARMAG, buffer, SARMAG) == 0))
    return TRUE;
  return FALSE;
}

/* 
 * MS Archives are very similar to GNU archives. There's a few differences however:
 *
 * 1) MS Archives contain _TWO_ "Linker Members". These linker members have similar
 * semantics to GNU's "/" member. They contain information about the symbol table and
 * the offsets of the archive members. In fact, the first MS linker member has identical
 * semantics to the GNU "/" member. The second MS linker member is an extended version
 * of the first.
 *
 * 2) MS Archives may contain multiple archive members with the same name. 
 *
 * 3) MS Archives may contain "import members" that contain information needed to build
 * the import tables of a new PE binary.
 */
t_bool
IsArchMSAr (FILE * fp)
{
  char buffer[SARMAG + 40];
  long fpos = ftell (fp);

  int ret = fread(buffer, SARMAG + 40, 1, fp);
  fseek(fp, fpos, SEEK_SET);
  
  if (ret != 1)
    return FALSE;

  /* check for alpha */
  if (strncmp("________64E", buffer + SARMAG, 11) == 0)
    return FALSE;

  /* if uid and/or gid aren't blanked out, it's not an MS archive */
  if (strncmp("      ", buffer + SARMAG + 16 + 12, 6) != 0
    || strncmp("      ", buffer + SARMAG + 16 + 12 + 6, 6) != 0)
    return FALSE;

  /* check if the signature is valid */
  if (strncmp(ARMAG, buffer, SARMAG) == 0)
    return TRUE;

  return FALSE;
}

t_string 
ArchArDecodeObjectNameGnu(t_archive * ar, t_ar_archive_header * tmp,  char *strtbl, t_uint32 stblsize)
{
  char *rname = NULL;

  if (tmp->ar_name[0] != '/')
  {
    long size = 0;

    /* GNU AR uses a '/' to mark the end of the filename instead of a
     * space. This allows for the use of spaces inside a filename
     * without the use of an extended filename. */
    while (tmp->ar_name[size] != '/')
    {
      size++;
    }

    rname = (char *) Malloc (size + 1);
    strncpy (rname, tmp->ar_name, size);
    rname[size] = 0;
  }
  /* Filenames starting with a '/' are GNU AR's way of storing extended
   * filenames. */
  else if (tmp->ar_name[1] != ' ')
  {
    t_uint32 size = 0;
    t_uint32 start = StringToUint32 (tmp->ar_name + 1, 15);

    ASSERT (strtbl != NULL,
            ("Long name, but no string table found in %s",
             ar->name));
    while ((strtbl[start + size] != '/')
           && (stblsize > start + size))
    {
      size++;
    }

    rname = (char *) Malloc (size + 1);
    strncpy (rname, strtbl + start, size);
    rname[size] = 0;
  }
  return rname;
}

t_string
ArchMSNormalizeName(t_string name)
{
  if (name[0] == 127)
  {
    t_string result = (t_string)Malloc(strlen(name) + 4);
    memcpy(result + 3, name, strlen(name) + 1);
    result[0] = '\\';
    result[1] = '1';
    result[2] = '7';
    result[3] = '7';
    return result;
  }
  else
    return StringDup(name);
}

/* this is a huge hack! */
t_string
ArchMSGetDllThunkName(t_ar_archive_header* tmp, char* base_name, t_uint32 base_name_length, FILE* fp)
{
  char* result = NULL;
  t_uint32 cur_pos = ftell(fp);
  char* tmp_object = (char*)Malloc(atoi(tmp->ar_size));
  IGNORE_RESULT(fread(tmp_object, 1, atoi(tmp->ar_size), fp));

  if (!tmp_object[0] && !tmp_object[1] && ((unsigned char)tmp_object[2] == 0xFF) && ((unsigned char)tmp_object[3] == 0xFF))
  {
    /* short import format */
    char tmp_import_name[4096];
    t_string import_name;

    sscanf((tmp_object + 20), "%s %*s", tmp_import_name);
    import_name = ArchMSNormalizeName(tmp_import_name);

    result = (char*)Malloc(base_name_length + 1 + strlen(import_name) + 1 + 6);
    memcpy(result, base_name, base_name_length);
    result[base_name_length] = 0;
    strcat(result, ";__imp_");
    strcat(result, import_name);
    Free(import_name);
  }
  else
  {
    /* COFF import format */
    t_uint32 symtab_ptr = *(t_uint32*)&tmp_object[8];
    t_uint32 symtab_sz  = *(t_uint32*)&tmp_object[12];
    t_uint32 strtab_ptr = symtab_ptr + 18*symtab_sz;
    t_uint32 sym_start  = symtab_ptr;
    t_uint32 syms_last_found_start = 0;
    t_uint32 syms_found = 0;
    t_uint32 i;
    char sym_name[9];
    
    /* iterate over COFF symbols. we should only find ONE symbol that is defined in one of the COFF sections!!! */
    for (i = 0; i < symtab_sz; ++i)
    {
      t_uint8 sym_class = tmp_object[sym_start + 16];
      t_uint16 sym_section = *(t_uint16*)&tmp_object[sym_start + 12];
      t_uint8 sym_aux = *(t_uint8*)&tmp_object[sym_start + 17];

      if (sym_class == 2 /* external */
        && sym_section != 0 /* must be defined in one of the COFF sections */
        )
      {
        syms_found++;
        syms_last_found_start = sym_start;
      }

      /* jump over aux syms */
      symtab_sz -= sym_aux;
      sym_start += 18*(sym_aux + 1);      
    }

    ASSERT(syms_found == 1, ("Found multiple defined external syms in a long import format member!"));
    
    memcpy(sym_name, &tmp_object[syms_last_found_start], 8);
    sym_name[8] = 0;
    if (!sym_name[0] && !sym_name[1] && !sym_name[2] && !sym_name[3])
    {
      /* name is longer than 8 bytes */
      t_uint16 str_offset = *(t_uint16*)&sym_name[4];
      char real_sym_name[4096];
      t_string import_name;

      sscanf(&tmp_object[strtab_ptr + str_offset], "%s", real_sym_name);
      import_name = ArchMSNormalizeName(real_sym_name);      
      
      result = (char*)Malloc(base_name_length + 1 + strlen(import_name) + 1);
      memcpy(result, base_name, base_name_length);
      result[base_name_length] = 0;
      strcat(result, ";");
      strcat(result, import_name);
      Free(import_name);
    }
    else
    {
      t_string import_name = ArchMSNormalizeName(sym_name);
      result = (char*)Malloc(base_name_length + 1 + strlen(import_name) + 1);
      memcpy(result, base_name, base_name_length);
      result[base_name_length] = 0;
      strcat(result, ";");
      strcat(result, import_name);
      Free(import_name);
    }
  }    

  fseek(fp, cur_pos, SEEK_SET);
  Free(tmp_object);
  return result;
}

t_string 
ArchMSArDecodeObjectName(t_archive * ar, t_ar_archive_header * tmp,  char *strtbl, t_uint32 stblsize, t_uint32 header_offset, FILE* fp)
{
  char *rname = NULL;

  /* regular names end with a slash */
  if (tmp->ar_name[0] != '/')
  {
    long size = 0;

    while (tmp->ar_name[size] != '/')
      size++;

    /* if the object name ends with .dll, the object contains import information.
     * In MS archives, there may be thousands of import information entries with
     * the same object name => we cannot use the object name as the key for the
     * hash table.
     * 
     * luckily, each of these objects contain exactly ONE symbol =>
     * we can append the symbol name to the object name
     */
    if (size > 4 && strncasecmp(tmp->ar_name + size - 4, ".dll", 4) == 0)
    {
      return ArchMSGetDllThunkName(tmp, tmp->ar_name, size, fp);
    }
    /* MS archives contain full pathnames for embedded obj files. Linker maps only
     * contain filenames though... We can discard the path.
     */
    else if (size > 4 && strncasecmp(tmp->ar_name + size - 4, ".obj", 4) == 0)
    {
      t_uint32 last_backslash = 0;
      t_uint32 i = 0;

      for (i = 0; i < size; ++i)
        if (tmp->ar_name[i] == '\\')
          last_backslash = i;
      
      if (last_backslash > 0)
      {
        rname = (t_string) Malloc (size - last_backslash);
        strncpy(rname, tmp->ar_name + last_backslash + 1, size - (last_backslash + 1));
        rname[size - (last_backslash + 1)] = '\0';
        return rname;
      }
    }
    
    rname = (t_string) Malloc (size + 1);
    strncpy (rname, tmp->ar_name, size);
    rname[size] = 0;
  }  
  else if (tmp->ar_name[1] != ' ')
  {
    t_uint32 size = 0;
    t_uint32 start = StringToUint32 (tmp->ar_name + 1, 15);

    ASSERT (strtbl != NULL,
            ("Long name, but no string table found in %s",
             ar->name));
    while ((strtbl[start + size] != '\0')
           && (stblsize > start + size))
      size++;

    /* again, we need to check if it's a dll */
    if (size > 4 && strncasecmp(strtbl + start + size - 4, ".dll", 4) == 0)
    {      
      return ArchMSGetDllThunkName(tmp, strtbl + start, size, fp);
    }
    else if (size > 4 && strncasecmp(strtbl + start + size - 4, ".obj", 4) == 0)
    {
      t_uint32 last_backslash = 0;
      t_uint32 i = 0;

      for (i = 0; i < size; ++i)
        if (strtbl[start + i] == '\\')
          last_backslash = i;
      
      if (last_backslash > 0)
      {
        rname = (t_string) Malloc (size - last_backslash);
        strncpy(rname, strtbl + start + last_backslash + 1, size - (last_backslash + 1));
        rname[size - (last_backslash + 1)] = '\0';
        return rname;
      }
    }

    rname = (t_string) Malloc (size + 1);
    strncpy (rname, strtbl + start, size);
    rname[size] = 0;
  }
  return rname;
}

void
ArchArOpen (FILE * fp, t_archive * ar)
{
  /* For CodeComposer LIB files, this becomes the filename array if we
   * encounter a <filenames> file in the archive */
  char * code_composer_filenames = NULL;
  char *strtbl = NULL;
  t_ar_archive_header tmp;
  t_uint32 max;
  t_uint32 stblsize = 0;
  t_archive_object_hash_table_node *aohn = NULL; 
  
  /* Used to store the symbol lookup table */

  t_uint32 nsyms=0;
  t_uint32 * symbol_offsets=NULL;
  t_string * symbol_names=NULL;
  
  /* Eventually, we'll need subtypes since different arch's exist but normally
   * they'll only differ in symbol table lookup functions and stuff ... (BSD vs
   * GNU AR). Currently only GNU AR is supported! BSD AR files will pass fine as
   * long as no extended filenames are used.... */

  ar->open_fp = fp;
  ar->objects = HashTableNew (47, 0,
			      (t_hash_func) StringHash,
			      (t_hash_cmp) StringCmp,
			      ArchiveObjectHashTableNodeFree);
  ar->symbols = NULL;
  fseek (fp, SARMAG, SEEK_SET);
  
  while (1)
  {
    if (fread (&tmp, sizeof (t_ar_archive_header), 1, fp) != 1)
      break;
    
    ASSERT (strncmp (tmp.ar_fmag, ARFMAG, 2) == 0, ("Corrupted archive or unsupported archivetype %s, ar_fmag = 0x%x 0x%x (%d %d)", ar->name,*tmp.ar_fmag,*(tmp.ar_fmag+1),*tmp.ar_fmag,*(tmp.ar_fmag+1)));    

    /* GNU AR variant extended filenames are stored in the data of a special
     * pseudo objectfile called "//" (archive string table). If we encounter
     * such an entry we read the stringtable and store it for later use... 
     * 
     * The MS AR variant has a longnames member with identical semantics to the
     * GNU variant. 
     */
    if (strncmp (tmp.ar_name, "//", 2) == 0)
    {
      strtbl = (char *) Malloc (StringToUint32 (tmp.ar_size, 10) + 1);
      IGNORE_RESULT(fread (strtbl, StringToUint32 (tmp.ar_size, 10), 1, fp));
      stblsize = StringToUint32 (tmp.ar_size, 10);
      fseek (fp, stblsize % 2, SEEK_CUR);	/* Hey, it does the trick.... Needed this on linux. */
    }
    /* The BSD AR variant uses the special name "#1/" for files that need an extended
     * filename (not implemented) */
    else if (strncmp (tmp.ar_name, "#1/", 3) == 0)
    {
      FATAL(("BSD ar not implemented"));
    }
    /* The GNU AR variant uses "/" as a name padded with spaces to denote the
     * symbol lookup table (also called armap). We simply read it here. Decoding
     * is done later (to assure we already read the string table which is needed
     * for decoding object file names) */
    else if (strncmp (tmp.ar_name, "/ ", 2) == 0)
    {
      t_uint32 i;
      t_uint32 start_of_symbol_table=ftell(fp);

      /* 
       * GNU AR symbol lookup format:
       *
       * number of syms (4 bytes long stored big endian)           
       * 
       * symbol offsets (number of syms times 4 bytes offset in ar file stored
       * big endian) 
       *
       * symbol names   (number of syms times a zero delimited string) */
   
      max = StringToUint32 (tmp.ar_size, 10);

      IGNORE_RESULT(fread(&nsyms, sizeof(t_uint32), 1, fp));

#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
      nsyms = Uint32SwapEndian(nsyms); /* Yeez.... number is always in BIG endian */
#endif

      symbol_offsets=Malloc(sizeof(t_uint32)*nsyms);
      symbol_names=Malloc(sizeof(t_string)*nsyms);
      IGNORE_RESULT(fread(symbol_offsets, sizeof(t_uint32)*nsyms, 1, fp));

      for (i=0; i<nsyms; i++)        
        symbol_names[i]=FileGetString(fp,'\0');                

      fseek (fp, start_of_symbol_table + max + max % 2, SEEK_SET);
    }
    /* Code Composer LIB files, almost arch format, but filenames are in
     * another array */
    else if (strncmp(tmp.ar_name, "<filenames>",11) == 0)
    {
      max = StringToUint32 (tmp.ar_size, 10);
      code_composer_filenames = Malloc(sizeof(char)*max);
      IGNORE_RESULT(fread(code_composer_filenames, sizeof(char)*max, 1, fp));
      fseek (fp, max % 2, SEEK_CUR);	/* Hey, it does the trick.... Needed this on linux. */
    }
    /* Regular AR members */
    else
    {
      t_string rname;
      if ((code_composer_filenames)&&(StringToUint32(tmp.ar_uid,6)!=0))
      {
        long size = 0;

        /* CodeComposer uses a '/' to mark the end of the filename instead of a
         * space. This allows for the use of spaces inside a filename without
         * the use of an extended filename. */
        while ((code_composer_filenames+StringToUint32(tmp.ar_uid,6))[size] != '/')
        {
          size++;
        }

        rname = (char *) Malloc (size + 1);
        strncpy (rname, code_composer_filenames+StringToUint32(tmp.ar_uid,6), size);
        rname[size] = 0;
      }
      else
        rname=ArchArDecodeObjectNameGnu(ar, &tmp,  strtbl, stblsize);
      aohn = Malloc (sizeof (t_archive_object_hash_table_node));
      HASH_TABLE_NODE_SET_KEY(&aohn->node,  rname);
      aohn->fpos = ftell (fp);
      aohn->size = StringToUint32 (tmp.ar_size, 10);
      aohn->flags = 0;
      HashTableInsert (ar->objects, (t_hash_table_node *) aohn);
      max = StringToUint32 (tmp.ar_size, 10);
      fseek (fp, max + max % 2, SEEK_CUR);	/* Hey, it does the trick.... Needed this on linux. */      
    }
  }


  if (symbol_offsets)
  {
    t_uint32 i;
    t_uint32 pos=ftell(fp);
    t_archive_symbol_hash_table_node *ashn = NULL;

    ar->symbols = HashTableNew (47, 0,
                                (t_hash_func) StringHash,
                                (t_hash_cmp) StringCmp,
                                ArchiveSymbolHashTableNodeFree);

    for (i=0; i<nsyms; i++)
    {
      t_ar_archive_header tmp2;
      ashn = Malloc (sizeof (t_archive_symbol_hash_table_node));
      HASH_TABLE_NODE_SET_KEY(&ashn->node,  symbol_names[i]);
#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
      ashn->fpos = Uint32SwapEndian(symbol_offsets[i]) + sizeof(t_ar_archive_header);
#else
      ashn->fpos = symbol_offsets[i] + sizeof(t_ar_archive_header);
#endif
      fseek(fp, ashn->fpos - sizeof(t_ar_archive_header), SEEK_SET);

      if (fread (&tmp2, sizeof (t_ar_archive_header), 1, fp) != 1) 
        FATAL(("Symbol lookup table (armap) of GNU ar archive %s is corrupt\n", ar->name));
      ashn->name = ArchArDecodeObjectNameGnu(ar, &tmp2,  strtbl, stblsize);
      HashTableInsert (ar->symbols, (t_hash_table_node *) ashn);
    }

    fseek(fp, pos, SEEK_SET);
  }

  if (symbol_offsets)
    Free(symbol_offsets);

  if (symbol_names) 
    Free(symbol_names);

  if (code_composer_filenames) 
    Free(code_composer_filenames);

  if (strtbl)
    Free (strtbl);
}

void
ArchArClose (t_archive * ar)
{
  fclose(ar->open_fp);
  ar->open_fp = NULL;
}

void
ArchAlphaArOpen (FILE * fp, t_archive * ar)
{
  /* OK, Tru64Unix archives have support for compressed objects and stuff, 
   * Need to handle that different */
  char *strtbl = NULL;
  t_ar_archive_header tmp;
  t_uint32 max;
  t_uint32 stblsize = 0;
  t_archive_object_hash_table_node *aohn = NULL;

  ar->open_fp = fp;
  ar->objects = HashTableNew (47, 0,
			      (t_hash_func) StringHash,
			      (t_hash_cmp) StringCmp,
			      ArchiveObjectHashTableNodeFree);
  ar->symbols = NULL;
  fseek (fp, SARMAG, SEEK_SET);
  IGNORE_RESULT(fread (&tmp, 1, sizeof (t_ar_archive_header), fp));
  max = StringToUint32 (tmp.ar_size, 10);
  fseek (fp, max, SEEK_CUR);
  while (1)
    {
      ASSERT ((strncmp (tmp.ar_fmag, ARFZMAG, 2) == 0)
	      || (strncmp (tmp.ar_fmag, ARFMAG, 2) == 0),
	      ("Corrupted archive or unsupported archivetype %s (magix=%.35s)",
	       ar->name, tmp.ar_fmag));
      if (fread (&tmp, sizeof (t_ar_archive_header), 1, fp) != 1)
	break;
      if (strncmp (tmp.ar_name, "//", 2) == 0)
	{
	  strtbl = (char *) Malloc (StringToUint32 (tmp.ar_size, 10) + 1);
	  IGNORE_RESULT(fread (strtbl, StringToUint32 (tmp.ar_size, 10), 1, fp));
	  stblsize = StringToUint32 (tmp.ar_size, 10);
	}
      else
	{
	  char *rname = NULL;
	  if (tmp.ar_name[0] != '/')
	    {
	      long size = 0;
	      while (tmp.ar_name[size] != ' ')
		{
		  size++;
		}

	      rname = (char *) Malloc (size + 1);

	      strncpy (rname, tmp.ar_name, size);
	      rname[size] = 0;
	    }
	  else
	    {
	      t_uint32 size = 0;
	      t_uint32 start = StringToUint32 (tmp.ar_name + 1, 15);

	      ASSERT (strtbl != NULL,
		      ("Long name, but no string table found in %s",
		       ar->name));
	      while ((strtbl[start + size] != '/')
		     && (stblsize > start + size))
		{
		  size++;
		}

	      rname = (char *) Malloc (size + 1);
	      strncpy (rname, strtbl + start, size);
	      rname[size] = 0;
	    }


	  aohn = Malloc (sizeof (t_archive_object_hash_table_node));
	  HASH_TABLE_NODE_SET_KEY(&aohn->node,  rname);
	  aohn->fpos = ftell (fp);
	  aohn->size = StringToUint32 (tmp.ar_size, 10);
	  if (strncmp (tmp.ar_fmag, ARFZMAG, 2) == 0)
	    aohn->flags = COMPRESSED_ALPHA;
	  else if (strncmp (tmp.ar_fmag, ARFMAG, 2) == 0)
	    aohn->flags = 0;
	  else
	    FATAL (("MAGIC IS WRONG!"));
	  HashTableInsert (ar->objects, (t_hash_table_node *) aohn);
	  max = StringToUint32 (tmp.ar_size, 10);
	  fseek (fp, aohn->fpos + 2 * (max / 2 + max % 2), SEEK_SET);	/* Hey, it does the trick.... Needed this on linux and Alpha */
	}
    }
  if (strtbl)
    Free (strtbl);
}

void
ArchAlphaArClose (t_archive * ar)
{
  fclose(ar->open_fp);
  ar->open_fp = NULL;
}

/* Very similar to the GNU AR open routine but can handle duplicate names... */
void
ArchMSArOpen (FILE * fp, t_archive * ar)
{  
  char *strtbl = NULL;
  t_ar_archive_header tmp;
  t_uint32 max;
  t_uint32 stblsize = 0;
  t_archive_object_hash_table_node *aohn = NULL; 
  t_uint32 linker_members = 0;
  
  /* Used to store the symbol lookup table */

  t_uint32 nsyms=0;
  t_uint32 * symbol_offsets=NULL;
  t_string * symbol_names=NULL;

  ar->open_fp = fp;
  ar->objects = HashTableNew (47, 0,
			      (t_hash_func) StringHash,
			      (t_hash_cmp) StringCmp,
			      ArchiveObjectHashTableNodeFree);
  ar->symbols = NULL;
  fseek (fp, SARMAG, SEEK_SET);
  
  while (1)
  {
    t_uint32 curpos = ftell(fp);
    if (fread (&tmp, sizeof (t_ar_archive_header), 1, fp) != 1)
      break;
    
    ASSERT (strncmp (tmp.ar_fmag, ARFMAG, 2) == 0, ("Corrupted archive or unsupported archivetype %s, ar_fmag = 0x%x 0x%x (%d %d)", ar->name,*tmp.ar_fmag,*(tmp.ar_fmag+1),*tmp.ar_fmag,*(tmp.ar_fmag+1)));    

    /* "//" = the LONGNAMES member. Identical semantics to GNU's string table */
    if (strncmp (tmp.ar_name, "//", 2) == 0)
    {
      strtbl = (char *) Malloc (StringToUint32 (tmp.ar_size, 10) + 1);
      IGNORE_RESULT(fread (strtbl, StringToUint32 (tmp.ar_size, 10), 1, fp));
      stblsize = StringToUint32 (tmp.ar_size, 10);
      fseek (fp, stblsize % 2, SEEK_CUR);	/* Hey, it does the trick.... Needed this on linux. */
    }
    /* "/" = LINKER MEMBER. */
    else if (strncmp (tmp.ar_name, "/ ", 2) == 0)
    {
      t_uint32 i;
      t_uint32 start_of_symbol_table=ftell(fp);
      linker_members++;
   
      max = StringToUint32 (tmp.ar_size, 10);

      /* we ignore the second linker member for now. 
       * It contains the same information as the first linker 
       * member but it also has an index table that can be used for faster symbol lookups.
       */
      if (linker_members == 1)
      {
        IGNORE_RESULT(fread(&nsyms, sizeof(t_uint32), 1, fp));

  #ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
        nsyms = Uint32SwapEndian(nsyms); /* Yeez.... number is always in BIG endian */
  #endif

        symbol_offsets  = (t_uint32*) Malloc(sizeof(t_uint32)*nsyms);
        symbol_names    = (t_string*) Malloc(sizeof(t_string)*nsyms);
        IGNORE_RESULT(fread(symbol_offsets, sizeof(t_uint32)*nsyms, 1, fp));

        for (i=0; i<nsyms; i++)        
          symbol_names[i]=FileGetString(fp,'\0');                
      }

      fseek (fp, start_of_symbol_table + max + max % 2, SEEK_SET);
    }    
    /* Regular AR members */
    else
    {
      t_string rname;
      rname       = ArchMSArDecodeObjectName(ar, &tmp,  strtbl, stblsize, curpos, fp);
      aohn        = (t_archive_object_hash_table_node*) Malloc (sizeof (t_archive_object_hash_table_node));      
      aohn->fpos  = ftell (fp);
      aohn->size  = StringToUint32 (tmp.ar_size, 10);
      aohn->flags = 0;
      HASH_TABLE_NODE_SET_KEY(&aohn->node, rname);
      //VERBOSE(0, ("> Inserting AR member: %s", rname));
      HashTableInsert (ar->objects, (t_hash_table_node *) aohn);
      max = StringToUint32 (tmp.ar_size, 10);
      fseek (fp, max + max % 2, SEEK_CUR);	/* Hey, it does the trick.... Needed this on linux. */      
    }
  }

  if (symbol_offsets)
  {
    t_uint32 i;
    t_uint32 pos=ftell(fp);
    t_archive_symbol_hash_table_node *ashn = NULL;

    ar->symbols = HashTableNew (47, 0,
                                (t_hash_func) StringHash,
                                (t_hash_cmp) StringCmp,
                                ArchiveSymbolHashTableNodeFree);

    for (i=0; i<nsyms; i++)
    {
      t_ar_archive_header tmp2;      
      ashn = Malloc (sizeof (t_archive_symbol_hash_table_node));
      HASH_TABLE_NODE_SET_KEY(&ashn->node,  symbol_names[i]);
#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
      ashn->fpos = Uint32SwapEndian(symbol_offsets[i]) + sizeof(t_ar_archive_header);
#else
      ashn->fpos = symbol_offsets[i] + sizeof(t_ar_archive_header);
#endif
      fseek(fp, ashn->fpos - sizeof(t_ar_archive_header), SEEK_SET);

      if (fread (&tmp2, sizeof (t_ar_archive_header), 1, fp) != 1) 
        FATAL(("Symbol lookup table of MS ar archive %s is corrupt\n", ar->name));
      ashn->name = ArchMSArDecodeObjectName(ar, &tmp2,  strtbl, stblsize, ashn->fpos - sizeof(t_ar_archive_header), fp);
      /*VERBOSE(0, ("> Inserting AR symbol: %s - In member: %s", symbol_names[i], ashn->name));*/
      HashTableInsert (ar->symbols, (t_hash_table_node *) ashn);
    }

    fseek(fp, pos, SEEK_SET);
  }

  if (symbol_offsets)
    Free(symbol_offsets);

  if (symbol_names) 
    Free(symbol_names);

  if (strtbl)
    Free (strtbl);
}

void
ArchMSArClose (t_archive * ar)
{
  fclose(ar->open_fp);
  ar->open_fp = NULL;
}

t_const_string
ArchGetObjectNameForSymbol(const t_archive * ar, t_const_string name)
{
  if (ar->symbols)
  {
    t_archive_symbol_hash_table_node *hn =
      (t_archive_symbol_hash_table_node *) HashTableLookup (ar->symbols, name);

    if (!hn)
      return NULL;

    return hn->name;
  }

  return NULL;
}

t_object * 
ArchGetObject(const t_archive * ar, t_const_string name, t_object * parent, t_bool read_debug)
{
  t_archive_object_hash_table_node *hn =
    (t_archive_object_hash_table_node *) HashTableLookup (ar->objects, name);
  t_object *obj;

  if (!hn)
    return NULL;
  else
    {
      if (hn->flags == COMPRESSED_ALPHA)
	{
#ifndef HAVE_OBJZ_SUPPORT
	  FATAL (("Compressed alpha archives not supported on this architecture (or not compiled in), define -DHAVE_OBJZ_SUPPORT when compiling the source on Tru64 Unix systems and link with libmld. This should be autoconfigured. Contact me (bdebus at elis.ugent.be) if you have information about the ObjZ compression algoritm"));
	  return NULL;		/* keep the compiler happy */
#else
	  t_string tmp = StringConcat3 (ar->name, ":", hn->node.key);
	  FILE *out;
	  t_uint64 uncomp_size;
	  t_uint64 pad;
	  unsigned char *data = NULL;
	  int ret;
	  ecoff_file_hdr fhdr;
	  char data_tmp[8192];

	  int cur_pos;


	  if (fseek (ar->open_fp, hn->fpos, SEEK_SET))
	    FATAL (("Fseek failed"));
	  ret = fread (&fhdr, 1, sizeof (ecoff_file_hdr), ar->open_fp);
	  if (ret < 0)
	    FATAL (("Could not read file!"));
	  if ((!(fhdr.f_magic == 0x183)) && (!(fhdr.f_magic == 0x188)))
	    {
	      FATAL (("Expected an object, file %s in archive %s has magic %x, fhdrs= %d!", name, ar->name, fhdr.f_magic, fhdr.f_nscns));
	    }
	  else
	    {
	      IGNORE_RESULT(fread (&uncomp_size, sizeof (t_uint64), 1, ar->open_fp));
	      IGNORE_RESULT(fread (&pad, sizeof (t_uint64), 1, ar->open_fp));
	    }
	  cur_pos = lseek (fileno (ar->open_fp), 0, SEEK_CUR);
	  lseek (fileno (ar->open_fp), hn->fpos, SEEK_SET);
	  data = (unsigned char *) Calloc (1, uncomp_size);
	  obj_Zcopytocore (fileno (ar->open_fp), data, hn->size);
	  lseek (fileno (ar->open_fp), cur_pos, SEEK_SET);

	  out = fopen ("/tmp/tmp_obj2.o", "wb");
	  fwrite (data, uncomp_size, 1, out);
	  fclose (out);
	  Free (data);
	  obj = ObjectReadWithFakeName ("/tmp/tmp_obj2.o", tmp);
	  unlink ("/tmp/tmp_obj2.o");

	  return obj;
#endif
	}
      else
	{
	  t_string tmp = StringConcat3 (ar->name, ":", (t_string)HASH_TABLE_NODE_KEY(&hn->node));
	  fseek (ar->open_fp, hn->fpos, SEEK_SET);
	  obj = ObjectGetFromStream (tmp, ar->open_fp, parent, read_debug);
	  Free (tmp);
	  return obj;
	}
    }
}

t_object * 
ArchMSGetObject(const t_archive * ar, t_const_string name, t_object * parent, t_bool read_debug)
{
  t_archive_object_hash_table_node *hn =
    (t_archive_object_hash_table_node *) HashTableLookup (ar->objects, name);
  t_object *obj;

  if (!hn)
    return NULL;
  else
  {
    t_string tmp = StringConcat3 (ar->name, ":", (t_string)HASH_TABLE_NODE_KEY(&hn->node));
    fseek (ar->open_fp, hn->fpos, SEEK_SET);
    obj = ObjectGetFromStream (tmp, ar->open_fp, parent, read_debug);
    Free (tmp);
    return obj;
  }
}

void
ArchMSCheckSymbolName(void* node, void* data)
{
  t_archive_symbol_hash_table_node* hn = (t_archive_symbol_hash_table_node*)node;  
  
  if (StringCmp((t_string)HASH_TABLE_NODE_KEY(&hn->node), (t_string)((void**)data)[1]) == 0)
  {
    *(void**)(((void**)data)[0]) = node;
  }
}

t_object *
ArchMSGetObjectBySymbol(const t_archive * ar, t_const_string symbol_name, t_object * parent, t_bool read_debug)
{  
  /* map the symbol onto an object name */  
  t_archive_symbol_hash_table_node *hn =
   NULL;

  /* stupid hack... */
  void* data[2];
  data[0] = &hn;
  data[1] = (void*)symbol_name;

  HashTableWalk(ar->symbols, ArchMSCheckSymbolName, &data);

  /* might have to restart the search using the undecorated version of the name... */
  if (!hn)
  {        
    if (StringPatternMatch("_*@*", symbol_name))
    {
      t_string undecorated_symbol_name = (t_string)Malloc(strlen(symbol_name));
      t_uint32 i;
      strcpy(undecorated_symbol_name, symbol_name+1);
      for (i = 0; i < strlen(symbol_name) - 1; ++i)
      {
        if (undecorated_symbol_name[i] == '@')
        {
          undecorated_symbol_name[i] = '\0';
          break;
        }
      }
      data[1] = undecorated_symbol_name;
      HashTableWalk(ar->symbols, ArchMSCheckSymbolName, &data);
      Free(undecorated_symbol_name);
    }
    /* NULL'ed out thunk data members are prefixed with \177 */
    else if (StringPatternMatch("\\177*", symbol_name))
    {
      /* whyyyyyyyy would you do this? */
      t_string tmpstring = StringDup(symbol_name);
      tmpstring[3] = 127;
      data[1] = (tmpstring + 3);
      HashTableWalk(ar->symbols, ArchMSCheckSymbolName, &data);
      Free(tmpstring);
    }
  }
  
  if (hn)
    return ArchMSGetObject(ar, hn->name, parent, read_debug);
  return NULL;
}

static int ar_module_usecount = 0;

void
DiabloArInit (int argc, char ** argv)
{
  if (!ar_module_usecount)
  {
    ArchiveHandlerAdd ("AR", IsArchAr, ArchArOpen, ArchGetObject, NULL, ArchArClose);
    ArchiveHandlerAdd ("TRU64AR", IsArchAlphaAr, ArchAlphaArOpen, ArchGetObject, NULL, ArchAlphaArClose);
    ArchiveHandlerAdd ("MSAR", IsArchMSAr, ArchMSArOpen, ArchMSGetObject, ArchMSGetObjectBySymbol, ArchMSArClose);
  }
  
  ar_module_usecount++;
}

void DiabloArFini()
{
  ar_module_usecount--;
}
