/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef DIABLOSUPPORT_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef DIABLOSUPPORT_HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifdef DIABLOSUPPORT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef DIABLOSUPPORT_HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#include <stdlib.h>
#include <errno.h>

static t_string home = NULL;
static char cp[PATH_MAX] = "\0";

/* TODO */
#include <errno.h>
#define ErrorClearErrno() errno=0
#define ErrorGetErrno() errno

extern void mkdir_recursive(t_const_string p_path);

/* Paths */
void
PathAddDirectory(t_path *path, t_const_string dir)
{
  /* Check whether the new directory has already been added. If yes, return without adding it again. */
  t_uint32 iii;
  for (iii = 0; iii < path->nelems; iii++)
    if(!strcmp(path->dirs[iii], dir))
      return;

  path->nelems++;
  path->dirs = (char**) Realloc(path->dirs, path->nelems * sizeof(char*));
  path->dirs[path->nelems - 1] = StringDup(dir);
}

void
PathInit (void)
{
  t_string ret = getenv ("HOME");

  if (ret)
  {
    home = StringDup (ret);
  }
  else
  {
    home = NULL;
  }
#ifdef DIABLOSUPPORT_HAVE_GETCWD
  if (!getcwd (cp, PATH_MAX))
  {
    FATAL(("Could not get current working directory"));
  }
#else
  strcpy (cp, CUR_DIR);
#endif
}

void
PathFini (void)
{
  if (home)
    Free (home);
  home = NULL;
}

t_path *
RealPathNew (FORWARD_MALLOC_FUNCTION_DEF t_const_string path, t_const_string delim)
{
  t_path *ret = (t_path*)RealMalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (t_path));
  t_string_array *array = StringDivide (path, delim, FALSE, FALSE);
  t_string_array_elem *elem;

#ifdef DIABLOSUPPORT_HAVE_SYS_STAT_H
  struct stat st;
#endif

  t_uint32 i;
  t_bool found = FALSE;

  ret->dirs = NULL;
  ret->nelems = 0;

  STRING_ARRAY_FOREACH_ELEM(array, elem)
  {
    t_string s = FileNameNormalize (elem->string);

    if (diablosupport_options.prepend &&
        strlen(diablosupport_options.prepend) != 0)
    {
      t_string tmp = StringConcat2 (diablosupport_options.prepend, s);
      Free (s);
      s = tmp;
    }

#ifdef DIABLOSUPPORT_HAVE_SYS_STAT_H
    if (stat (s, &st))
    {
      /* TODO
         if ((!util_options.restore) && (util_options.restore_multi==-1)) FATAL(("Couldn't stat %s",s));
         else
         {
         }
         */
    }
#ifndef STAT_MACROS_BROKEN
    else if (!(S_ISDIR(st.st_mode)))
    {
      WARNING(("%s is not a directory", s));
    }
#endif
#endif
    ErrorClearErrno ();

    found = FALSE;
    for (i = 0; i < ret->nelems; i++)
    {
      if (strcmp (ret->dirs[i], s) == 0)
      {
        found = TRUE;
        break;
      }
    }
    if (!found)
    {
      ret->dirs =
        (t_string *) RealRealloc (FORWARD_MALLOC_FUNCTION_USE ret->dirs,
                                  sizeof (t_string) * (ret->nelems + 1));
      ret->dirs[ret->nelems] = RealStringDup (FORWARD_MALLOC_FUNCTION_USE s);
      ret->nelems++;
    }
    Free (s);
  }

  StringArrayFree (array);

  return ret;
}

void
PathFree (const t_path * path)
{
  t_uint32 tel;

  for (tel = 0; tel < path->nelems; tel++)
  {
    Free (path->dirs[tel]);
  }
  Free (path->dirs);
  Free (path);
}

/* Utilities */
t_string
FileNameBase (t_const_string name)
{
  t_string ret;
  const char *end_of_base, *start_of_base;

  if (name == NULL || *name == '\0')
  {
    return StringDup (".");
  }

  end_of_base = name + strlen (name) - 1;
  while (end_of_base > name && CHR_ISA_PATH_SEPARATOR(*end_of_base))
    end_of_base--;

  if (end_of_base == name && CHR_ISA_PATH_SEPARATOR(*end_of_base))
  {
    return StringDup (PATH_SEPARATOR_STR);
  }

  start_of_base = end_of_base;
  while (start_of_base > name && !CHR_ISA_PATH_SEPARATOR(*(start_of_base - 1)))
    start_of_base--;

  ret = (t_string)Malloc (end_of_base - start_of_base + 2);
  (void) strncpy (ret, start_of_base, end_of_base - start_of_base + 1);
  ret[end_of_base - start_of_base + 1] = '\0';
  return ret;
}

t_string
RealFileNameNormalize (FORWARD_MALLOC_FUNCTION_DEF t_const_string inname)
{
  t_string tmp = RealStringDup (FORWARD_MALLOC_FUNCTION_USE inname);
  t_string ret;
  t_uint32 i;
  t_uint32 len = strlen(tmp);

  i = len - 1;
  while ((i != 0)
         && ((CHR_ISA_PATH_SEPARATOR(tmp[i]))
             || ((i > 0) && (tmp[i] == '.') && (CHR_ISA_PATH_SEPARATOR(tmp[i - 1])))))
    tmp[i--] = 0; /* Strip pending / and . */
  len = i+1;

  if ((tmp[0] == '~') && ((CHR_ISA_PATH_SEPARATOR(tmp[1])) || (tmp[1] == '\0')))
  {
    if (!home)
      FATAL(("HOME-directory not found! Use absolute paths!"));
    if (len > 2)
      ret = RealStringConcat3 (FORWARD_MALLOC_FUNCTION_USE home, PATH_SEPARATOR_STR, tmp + 2);
    else
      ret = RealStringConcat2 (FORWARD_MALLOC_FUNCTION_USE home, PATH_SEPARATOR_STR);
    Free (tmp);
  }
  else if ((tmp[0] == '.') && ((CHR_ISA_PATH_SEPARATOR(tmp[1])) || (tmp[1] == '\0')))
  {
    if (len > 2)
      ret = RealStringConcat3 (FORWARD_MALLOC_FUNCTION_USE cp, PATH_SEPARATOR_STR, tmp + 2);
    else
      ret = RealStringConcat2 (FORWARD_MALLOC_FUNCTION_USE cp, PATH_SEPARATOR_STR);
    Free (tmp);
  }
  else if ((tmp[0] == '.') && (tmp[1] == '.') && ((CHR_ISA_PATH_SEPARATOR(tmp[2])) || (tmp[2] == '\0')))
  {
    ret = RealStringConcat3 (FORWARD_MALLOC_FUNCTION_USE cp, PATH_SEPARATOR_STR, tmp);
    Free (tmp);
  }
  else
  {
    ret = tmp;
  }

  /* remove double slashes */
  i = 0;
  while (i < len-1)
  {
    if (CHR_ISA_PATH_SEPARATOR(ret[i]) && CHR_ISA_PATH_SEPARATOR(ret[i+1]))
    {
      memmove(ret+i+1, ret+i+2, len-(i+1));
      --len;
    }
    else
      ++i;
  }

  /* TODO: We should also avoid stupidity like /usr/a/../b/./c/ (should be
   * /usr/b/c) */

  return ret;
}

/* Directories */
void
DirMake (t_const_string path, t_bool tmp)
{
#ifdef DIABLOSUPPORT_HAVE_STAT
  struct stat st;

  if (stat (path, &st))
  {
    if (ErrorGetErrno () == ENOENT)
    {
#endif

#ifdef DIABLOSUPPORT_HAVE_MKDIR
      mkdir_recursive (path);
#else
      int ret = 1;
      ASSERT(!ret, ("Could not make directory %s", path));
#endif

#ifdef DIABLOSUPPORT_HAVE_STAT
    }
    else
    {
      FATAL(("Unexpected error assuring directory %s\n", path));
    }
  }
  else
  {
#ifndef STAT_MACROS_BROKEN
    ASSERT(S_ISDIR(st.st_mode),
           ("%s is not a directory, remove regular file!", path));
#endif
  }
  if (tmp)
  {
    /* TODO: Add it to the tmp list */
  }
#endif
  ErrorClearErrno ();
}

void
DirDel (t_const_string dir)
{
#ifdef _MSC_VER
  /** It would have been nice if we could use the POSIX compliant _rmdir function here.
   *  Unfortunately rmdir does not support recursive removal!
   */
  DeleteDirectory(dir);
#else
  pid_t pid;

  if ((pid = fork ()) == 0)
  {
    execlp ("rm", "rm", "-R", dir, NULL);
    exit (0);
  }
  else
  {
    waitpid (pid, NULL, 0);
  }
#endif
}

/* Files */
t_string
FileFind (const t_path * path, t_const_string name)
{
  t_uint32 tel;
  t_string tmp;
  t_bool free_name = FALSE;

#ifdef DIABLOSUPPORT_HAVE_STAT
  struct stat s;
#else
  FILE *fp;
#endif

  if (!path)
    return NULL;

  if (diablosupport_options.prepend &&
      strlen(diablosupport_options.prepend) != 0)
  {
    tmp = StringConcat2 (diablosupport_options.prepend, name);
  }
  else if (diablosupport_options.prepend_abs_path && 
           strlen(diablosupport_options.prepend_abs_path) != 0 &&
           CHR_ISA_PATH_SEPARATOR(name[0]))
  {
    tmp = StringConcat2 (diablosupport_options.prepend_abs_path, name);
  }
  else
    tmp = StringDup (name);

  /* Try to find the file now. 
   * If the filename is a full path, then try to open it directly.
   */
  if ((CHR_ISA_PATH_SEPARATOR(name[0]))
#ifdef _MSC_VER    
    || (strlen(name) > 4
      && name[1] == ':'
      && CHR_ISA_PATH_SEPARATOR(name[2]))    
#endif
    )
  {    
#ifdef DIABLOSUPPORT_HAVE_STAT
    if (!stat (tmp, &s))
#else
    if (fp = fopen (tmp, "r")) != NULL)
#endif
    {
#ifndef DIABLOSUPPORT_HAVE_STAT
      fclose (fp);
#endif

      ErrorClearErrno ();
      return tmp;
    }
    /* File not found at the full path  
     * => Look for it in the specified paths
     */
    else
    {
      Free(tmp);
      {
        char* tmp = FileNameBase(name);
        name = tmp;
        free_name = TRUE;
      }
    }
  }
  else
  {
    Free(tmp);
    ErrorClearErrno();
  }
  
  for (tel = 0; tel < path->nelems; tel++)
  {
    tmp = StringConcat3 (path->dirs[tel], PATH_SEPARATOR_STR, name);
#ifdef DIABLOSUPPORT_HAVE_STAT
    if (!stat (tmp, &s))
#else
    if ((fp = fopen (tmp, "r")) != NULL)
#endif
    {
#ifndef DIABLOSUPPORT_HAVE_STAT
      fclose (fp);
#endif
      ErrorClearErrno ();

      if (free_name) Free((void *)name);
      return tmp;
    }
    else
    {
      ErrorClearErrno ();
      Free (tmp);
    }
  }  

  ErrorClearErrno ();
  if (free_name) Free((void *)name);
  return NULL;
}

void
FileCopy (t_const_string from, t_const_string to)
{  
  t_string full_dir = StringDup ("");
  t_string_array *array = StringDivide (to, PATH_SEPARATOR_STR, FALSE, FALSE);
  t_string_array_elem *elem = NULL;

  STRING_ARRAY_FOREACH_ELEM(array, elem)
  {
    t_string sub_dir = StringConcat3 (full_dir, elem->string, PATH_SEPARATOR_STR);

    if (elem->next)
      DirMake (sub_dir, FALSE);
    Free (full_dir);
    full_dir = sub_dir;
  }

  StringArrayFree (array);
  Free (full_dir);

  if (!FileExists (to))
  {
    #ifdef _MSC_VER
      CopyFileWin(from, to);
    #else
    pid_t pid;
	  if ((pid = fork ()) == 0)
    {
      execlp ("cp", "cp", from, to, NULL);
      exit (0);
    }
    else
    {
      waitpid (pid, NULL, 0); /* Else we could overrun the max process limit */
      /* TODO: Add checks here */
    }
    #endif
  }
}

t_bool
FileExists (t_const_string name)
{
#ifdef DIABLOSUPPORT_HAVE_STAT
  struct stat st;
#else
  FILE *fp;
#endif
#ifdef DIABLOSUPPORT_HAVE_STAT
  if (stat (name, &st))
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
#else
  if (fp = fopen (name, "r"))
  {
    fclose (fp);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
#endif
}

#define CONFIG_MAX_FILE_LINE_LEN 1000

t_string
RealFileGetLine (FORWARD_MALLOC_FUNCTION_DEF FILE * ifp)
{
  char buffer[CONFIG_MAX_FILE_LINE_LEN];
  t_int16 i = 0;
  t_string ret;
  int getc_ret;

  getc_ret = getc (ifp);
  buffer[i] = getc_ret;
  while ((getc_ret != EOF) && (!feof (ifp)))
  {
    i++;
    if (buffer[i - 1] == '\n')
      break;
    ASSERT(i != CONFIG_MAX_FILE_LINE_LEN, ("Increase parsebuffersize!, strings is %s", buffer));
    getc_ret = getc (ifp);
    buffer[i] = getc_ret;
  }
  if (i == 0)
    return NULL;
  buffer[i] = '\0';
  ret = RealStringDup (FORWARD_MALLOC_FUNCTION_USE buffer);
  return ret;
}

t_string
RealFileGetString (FORWARD_MALLOC_FUNCTION_DEF FILE * ifp, char delim)
{
  char buffer[CONFIG_MAX_FILE_LINE_LEN];
  t_int16 i = 0;
  t_string ret;
  int getc_ret;

  getc_ret = getc (ifp);
  buffer[i] = getc_ret;
  while ((getc_ret != EOF) && (!feof (ifp)))
  {
    i++;
    if (buffer[i - 1] == delim)
      break;
    ASSERT(i != CONFIG_MAX_FILE_LINE_LEN, ("Increase parsebuffersize!, strings is %s", buffer));
    getc_ret = getc (ifp);
    buffer[i] = getc_ret;
  }
  if (i == 0)
    return NULL;
  buffer[i] = '\0';
  ret = RealStringDup (FORWARD_MALLOC_FUNCTION_USE buffer);
  return ret;
}

/* Tar */
void
Tar (t_const_string dir, t_const_string tar)
{
#ifdef _MSC_VER
  FATAL(("Implement dir to tar for windows"));
#else
  pid_t pid;

  if ((pid = fork ()) == 0)
  {
    execlp ("tar", "tar", "-cf", tar, dir, NULL);
    exit (0);
  }
  else
  {
    waitpid (pid, NULL, 0);
  }

  DirDel (dir);
#endif
}

void
Untar (t_const_string tar)
{
#ifdef _MSC_VER
  FATAL(("Implement untar for windows (untarring file: %s)\n", tar));
#else
  pid_t pid;

  if ((pid = fork ()) == 0)
  {
    execlp ("tar", "tar", "-xf", tar, NULL);
    exit (0);
  }
  else
  {
    waitpid (pid, NULL, 0);
  }
#endif
}

t_string
IoModifierPath (t_const_string modifiers, va_list * ap)
{
  t_path *path = va_arg (*ap, t_path *);
  t_string_array *array = StringArrayNew ();
  t_string ret;
  t_uint32 i;

  for (i = 0; i < path->nelems; i++)
    StringArrayAppendString (array, StringDup (path->dirs[i]));

  if (i > 0)
    ret = StringArrayJoin (array, PATH_DELIMITER);
  else
    ret = StringDup ("");
  StringArrayFree (array);
  return ret;
}

/* FileRegionsAddFileRegion {{{ */
void 
FileRegionsAddFileRegion(t_file_regions * regions, t_uint32 offset, t_uint32 size, char * data, char * id)
{
  regions->region_array = (t_file_region*)Realloc(regions->region_array, sizeof(t_file_region)*(regions->nregions+1));
  regions->region_array[regions->nregions].offset = offset;
  regions->region_array[regions->nregions].size = size;
  regions->region_array[regions->nregions].data = data;
  regions->region_array[regions->nregions].id = id;
  regions->nregions++;
}
/* }}} */

/* FileRegionsWrite {{{ */
void 
FileRegionsWrite(const t_file_regions * regions, FILE * fp)
{
  t_uint32 i;
  t_uint32 last = 0;
  for (i=0; i<regions->nregions; i++)
  {
    if (last< regions->region_array[i].offset)
    {
      t_uint32 j;
      char c;
      /* printf("0x%08x -> 0x%08x: Padding\n", last, regions->region_array[i].offset); */
      for (j=0; j<regions->region_array[i].offset-last; j++)
      {
        fwrite(&c, 1, 1, fp);
      }
    }
    else if (last > regions->region_array[i].offset)
    {
      FATAL(("Could not write file: file region %s overlaps with file region %s", regions->region_array[i-1].id, regions->region_array[i].id));
    }
    /* printf("0x%08x -> 0x%08x: %s\n", regions->region_array[i].offset, regions->region_array[i].offset+regions->region_array[i].size, regions->region_array[i].id); */
    fwrite(regions->region_array[i].data, 1, regions->region_array[i].size, fp);
    last = regions->region_array[i].offset+regions->region_array[i].size;
  }
}
/* }}} */

/* FileRegionsFindGap {{{ */
t_uint32 
FileRegionsFindGap(const t_file_regions * regions, t_uint32 size)
{
  t_uint32 i;
  t_uint32 last = 0;
  for (i=0; i<regions->nregions; i++)
  {
    /* printf("Next section = 0x%08x -> 0x%08x: %s, looking for gap of size %x, current gap = %x\n", regions->region_array[i].offset, regions->region_array[i].offset+regions->region_array[i].size, regions->region_array[i].id, size, (regions->region_array[i].offset-last));  */
    if (last< regions->region_array[i].offset)
    {
      if ((regions->region_array[i].offset-last)>=size) return last;
    }
    else if (last > regions->region_array[i].offset)
    {
      FATAL(("Elf File Corrupt: file region %s overlaps with file region %s", regions->region_array[i-1].id, regions->region_array[i].id));
    }
    last = regions->region_array[i].offset+regions->region_array[i].size;
  }
  return last;
}
/* }}} */

/* FileRegionsNew {{{ */
t_file_regions * 
FileRegionsNew()
{
  t_file_regions * ret = (t_file_regions*)Calloc(1,sizeof(t_file_regions));
  return ret;
}
/* }}} */

/* FileRegionsFree {{{ */
void 
FileRegionsFree(const t_file_regions * regions)
{
  t_uint32 i;
  for (i=0; i<regions->nregions; i++)
  {
    Free(regions->region_array[i].id);
  }
  Free(regions->region_array);
  Free(regions);
}
/* }}} */

/* FileRegionsSort {{{ */
static int 
sort_regions(const void * a, const void * b)
{
  t_file_region * fa = (t_file_region *) a;
  t_file_region * fb = (t_file_region *) b;

  return ((fa->offset<fb->offset)?-1:((fa->offset==fb->offset)?0:1));
}

void
FileRegionsSort(t_file_regions * regions)
{
   diablo_stable_sort(regions->region_array, regions->nregions, sizeof(t_file_region), sort_regions);
}

t_string FileDirectory(t_string x) {
  return dirname(x);
}
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
