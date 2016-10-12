#include <diabloobject.h>

#define TAR_EOF 1
#define TAR_IO_ERROR 2
#define TAR_FORMAT_ERROR 3
#define TAR_DIRECTORY 4
#define TAR_NORMAL_FILE 5
#define TAR_LINK 6
#define TAR_SYMBOLIC_LINK 7

/* #define VERBOSE_TAR  */
typedef struct _t_tar_desc
{
  char * name;
  int size;
} t_tar_desc;


t_uint32
ReadHeader (FILE * fp, t_tar_desc * d)
{
  t_uint32 chksum2 = 0;
  t_uint32 chksum = 0;
  int tel, isize;
  char header[512];
  t_bool allzero = TRUE;
  int zerocount = 0;
  int ret = TAR_FORMAT_ERROR;
  int tel2 = 0;
  int is_ustar = TRUE;

  while (allzero && zerocount < 2)
    {

      if (fread (header, 1, 512, fp) != 512)
	{
	  if (feof (fp))
	    return TAR_IO_ERROR;
	  else
	    return TAR_IO_ERROR;
	}

      for (tel = 0; tel < 148; tel++)
	{
	  if (header[tel])
	    {
	      allzero = FALSE;
	    }
	  chksum2 += header[tel];
	}

      for (tel = 0; tel < 8; tel++)
	{
	  if (header[tel])
	    {
	      allzero = FALSE;
	    }
	  chksum2 += ' ';
	}
      for (tel = 0; tel < 512 - 156; tel++)
	{
	  if (header[tel])
	    {
	      allzero = FALSE;
	    }
	  chksum2 += header[156 + tel];
	}
      zerocount++;
    }

  /* End of tar is indicated by two zero blocks */
  if (zerocount == 2)
    return TAR_EOF;
  else if (zerocount != 1)
    return TAR_FORMAT_ERROR;

  /* In USTAR, '5' indicates directories, but in traditional tar this is
   * done by appending '/' to the filename. This is the only USTAR extension we support. */
  if ((header[156] == '5') || ((*(header + strlen (header) - 1)) == '/'))
    {
#ifdef VERBOSE_TAR
      printf ("====================== Directory ======================\n");
#endif
      ret=TAR_DIRECTORY;
    }
  /* Normal files, exists in USTAR as well as in traditional tar */
  else if ((header[156] == '0') || (header[156] == 0))
    {
#ifdef VERBOSE_TAR
      printf ("===================== Normal file =====================\n");
#endif
      ret=TAR_NORMAL_FILE;
    }
  else if (header[156] == '1')
    {
#ifdef VERBOSE_TAR
      printf ("============== Link to an archived file ===============\n");
#endif
      ret=TAR_LINK;
    }
  else if (header[156] == '2')
    {
#ifdef VERBOSE_TAR
      printf ("=================== Symbolic link =====================\n");
#endif
      ret=TAR_SYMBOLIC_LINK;
    }
#if 0
  else if (header[156] == '3')
    printf ("Character special device\n");
  else if (header[156] == '4')
    printf ("Block special device\n");
  else if (header[156] == '6')
    printf ("FIFO special file\n");
  else if (header[156] == '7')
    printf ("Reserved\n");
#endif
  else
    {
      return TAR_FORMAT_ERROR;
    }

#ifdef VERBOSE_TAR
  printf ("Filename: %.100s\n", header);	/* Filename */
  printf ("Size: %.12s\n", header + 124);	/* Size */
#endif
  isize = strtol (header + 124, NULL, 8);

#ifdef VERBOSE_TAR
  printf ("Size: %.11o\n", isize);	/* Size */
  printf ("Checksum %.8s\n", header + 148);	/* Chksum */
#endif
  chksum = strtol (header + 148, NULL, 8);


  while (((*(header + 257 + tel2)) != ' ')
	 && ((*(header + 257 + tel2)) != '\0') && (tel2 < 6))
    {
      if (*(header + 257 + tel2) != *("ustar" + tel2))
	is_ustar = FALSE;
      tel2++;
    }
  if ((tel2) != 5)
    is_ustar = FALSE;


#ifdef VERBOSE_TAR
  printf ("Magic %s\n", header + 257);	/* Magic */
#endif
  if (is_ustar)
    FATAL (("Implement USTAR extensions"));

  if (chksum != chksum2)
    return TAR_FORMAT_ERROR;

  if (isize & 511)
    isize = ((isize + 512) & ~511);
  if (fseek (fp, isize, SEEK_CUR) == -1)
    {
      return TAR_IO_ERROR;
    }
  d->name=StringDup(header);
  d->size=isize;
  return ret;
}


t_bool
IsArchTar (FILE * fp)
{
  t_uint32 fpos=ftell(fp);
  int eof = 0;
  int count=0;
  t_tar_desc d;

  while ((!eof) && (count<5))
    {
      count ++;
      switch (ReadHeader (fp,&d))
	{
	  case TAR_DIRECTORY:
	  case TAR_LINK:
	  case TAR_SYMBOLIC_LINK:
	  case TAR_NORMAL_FILE:
	      Free(d.name);
	      break;
	  case TAR_EOF:
	      eof = 1;
	      break;
	  case TAR_IO_ERROR:
	  case TAR_FORMAT_ERROR:
	  default:
	      fseek(fp,fpos,SEEK_SET);
	      return FALSE;
	}
    }
  fseek(fp,fpos,SEEK_SET);
  return TRUE;
}



void
ArchTarOpen (FILE * fp, t_archive * ar)
{
  t_tar_desc d;
  int eof = 0;
  t_archive_object_hash_table_node *aohn = NULL;

  ar->open_fp = fp;
  ar->objects = HashTableNew (47, 0,
			      (t_hash_func)StringHash,
			      (t_hash_cmp) StringCmp,
			      ArchiveObjectHashTableNodeFree);
  while (!eof)
    {
      t_uint32 fpos=ftell(fp);

      switch (ReadHeader (fp,&d))
	{
	  case TAR_DIRECTORY:
	      /* Do nothing */
	      Free(d.name);
	      break;
	  case TAR_LINK:
	  case TAR_SYMBOLIC_LINK:
	      FATAL(("We do not support links or symbolic links in our tar files"));
	  case TAR_NORMAL_FILE:
	      aohn = Malloc (sizeof (t_archive_object_hash_table_node));
	      HASH_TABLE_NODE_SET_KEY(&aohn->node,  d.name);
	      aohn->fpos = fpos+512;
	      aohn->size = d.size;
	      aohn->flags = 0;
	      HashTableInsert (ar->objects, (t_hash_table_node *) aohn);
	      break;
	  case TAR_EOF:
	      eof = 1;
	      break;
	  case TAR_IO_ERROR:
	  case TAR_FORMAT_ERROR:
	  default:
	      FATAL (("Error"));
	}
    }
}

void
ArchTarClose (t_archive * ar)
{
  fclose(ar->open_fp);
  ar->open_fp = NULL;
}

t_object * 
TarGetObject(t_archive * ar, t_string name, t_object * parent, t_bool read_debug)
{
  t_archive_object_hash_table_node *hn =
    (t_archive_object_hash_table_node *) HashTableLookup (ar->objects, name);
  t_object *obj;

  if (!hn)
    return NULL;
  else
    {
      t_string tmp = StringConcat3 (ar->name, ":", HASH_TABLE_NODE_KEY(&hn->node));
      fseek (ar->open_fp, hn->fpos, SEEK_SET);
      obj = ObjectGetFromStream (tmp, ar->open_fp, parent, read_debug);
      Free (tmp);
      return obj;
    }
}

static int tar_module_usecount = 0;

void
DiabloTarInit (int argc, char ** argv)
{
  if (!tar_module_usecount)
  {
    ArchiveHandlerAdd ("TAR", IsArchTar, ArchTarOpen, TarGetObject, NULL, ArchTarClose);
  }

  tar_module_usecount++;
}

void DiabloTarFini()
{
  tar_module_usecount--;
}
