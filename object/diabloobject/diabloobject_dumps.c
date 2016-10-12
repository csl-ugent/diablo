/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloobject.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef DIABLOOBJECT_HAVE_SYS_WAIT_H
  #include <sys/wait.h>
#endif
#include <string.h>
#include <unistd.h>

/* Config variables for DUMP and RESTORE {{{ */
#define DUMP_DIR "./DUMP"
#define VERBOSE_DUMP
/* }}}*/

/* RestoreDumpedProgram {{{ */
t_string
RestoreDumpedProgram ()
{
  /* Variables used when restoring a dump from a multiple dump */
  t_bool multi = FALSE;
  t_uint32 multi_tel = 0;

  /* File pointer to the dump definition file */
  FILE *fp_def;

  /* Holds the lines from the dump definition file */
  t_string line;
  t_bool input = FALSE;
  t_bool output = FALSE;
  t_bool arguments = FALSE;
  t_string objectfilename = NULL;

  if (diabloobject_options.input_files_set)
    input = TRUE;
  if (diabloobject_options.output_files_set)
    output = TRUE;
  if (diabloobject_options.arguments_set)
    arguments = TRUE;

#ifdef VERBOSE_DUMP
  printf ("RESTORE!\n");
#endif

  Free (diablosupport_options.prepend);
  diablosupport_options.prepend = FileNameNormalize (DUMP_DIR);

  /* Untar the dump, and open the dump definition file {{{ */
  Untar ("dump.tar");
  fp_def = fopen ("./DUMP/diablo_dump", "r");
  if (!fp_def)
    FATAL(("Cannot restore: dump not found"));
  /* }}} */
  /* Locate the dump in the file (in case of multiple dumps and set the corresponding variable {{{ */
  while (!feof (fp_def))
  {
    line = FileGetLine (fp_def);

    if (!feof (fp_def))
    {
      if (StringPatternMatch ("LPATH: *", line))
      {
        if (diabloobject_options.libpath)
          PathFree (diabloobject_options.libpath);
        diabloobject_options.libpath = PathNew (line + 7, ":\n");
      }
      else if (StringPatternMatch ("OPATH: *", line))
      {
        if (diabloobject_options.objpath)
          PathFree (diabloobject_options.objpath);
        diabloobject_options.objpath = PathNew (line + 7, ":\n");
      }
      else if (StringPatternMatch ("ARGS: *", line))
      {
        StringChomp (line);
        if (!arguments)
        {
          if (diabloobject_options.arguments)
            Free (diabloobject_options.arguments);
          diabloobject_options.arguments = StringDup (line + 6);
        }
        diabloobject_options.arguments_set = TRUE;
      }
      else if (StringPatternMatch ("INPUTS: *", line))
      {
        StringChomp (line);
        if (!input)
        {
          if (diabloobject_options.input_files)
            Free (diabloobject_options.input_files);
          diabloobject_options.input_files = StringDup (line + 8);
        }
        diabloobject_options.input_files_set = TRUE;
      }
      else if (StringPatternMatch ("OUTPUTS: *", line))
      {
        StringChomp (line);
        if (!output)
        {
          if (diabloobject_options.output_files)
            Free (diabloobject_options.output_files);
          diabloobject_options.output_files = StringDup (line + 9);
        }
        diabloobject_options.output_files_set = TRUE;
      }
      else if (StringPatternMatch ("OBJ: *", line))
      {
        StringChomp (line);
        if (objectfilename)
          Free (objectfilename);
        objectfilename = StringDup (line + 5);
        if (!objectfilename)
          FATAL(("Couldn't find %s\n", line + 5));
      }
      else if (strcmp (line, "=============\n") == 0)
      {
        if (diabloobject_options.restore_multi == -1)
        {
          if (!multi)
          {
            printf ("This file contains the following dumps:\n");
          }
          printf ("%3d: %s\n", multi_tel, objectfilename);
        }
        else if (multi_tel == diabloobject_options.restore_multi)
        {
          printf ("%d %d\n", diabloobject_options.restore_multi, diabloobject_options.restore_multi_set);
          printf ("Selecting %d: %s from dump\n", multi_tel, objectfilename);
          Free (line);
          return objectfilename;
        }
        multi = TRUE;
        multi_tel++;
      }
      else
        FATAL(("Unrecognized dump line %s\n", line));
      Free (line);
    }
  }

  /* }}} */

  if (multi_tel == diabloobject_options.restore_multi)
  {
    printf ("Selected %d: %s from dump\n", multi_tel, objectfilename);
    return objectfilename;
  }

  if ((multi) && (diabloobject_options.restore_multi == -1))
  {
    printf ("%3d: %s\n", multi_tel, objectfilename);
    DirDel ("./DUMP");
    FATAL(("Must select a dump from the file!", diabloobject_options.restore_multi));
  }
  else if (multi)
  {
    DirDel ("./DUMP");
    FATAL(("Must select a valid dump from the dump-file!\n Use objdump -r to get a list. %d is not a valid dump", diabloobject_options.restore_multi));

  }
  /* FATAL(("Should not get here!"));*/

  return objectfilename;
}

/* }}} */
/* DumpProgram {{{ */
void
DumpProgram (const t_object * obj, t_bool multiple_dumps)
{
  FILE *fp;
  t_uint32 tel;
  t_object *i_obj, *tmp;
  t_string dest = StringConcat2 ("./DUMP", OBJECT_NAME(obj));
  t_string tmp_string = StringConcat2 (OBJECT_NAME(obj), ".map");

  if (multiple_dumps)
    Untar ("dump.tar");
  FileCopy (OBJECT_NAME(obj), dest);
  Free (dest);
  dest = StringConcat3 ("./DUMP", OBJECT_NAME(obj), ".map");
  FileCopy (tmp_string, dest);
  Free (dest);
  Free (tmp_string);

  /* Copy the subobjects {{{ */
  OBJECT_FOREACH_SUBOBJECT(obj, i_obj, tmp)
  {
    if (strncmp (OBJECT_NAME(i_obj), "Linker---", 9) == 0)
    {
    }
    else if (strcmp (OBJECT_NAME(i_obj), "Linker") == 0)
    {
    }
    else if (StringPatternMatch ("*:*", OBJECT_NAME(i_obj)))
    {
      t_string name2 = StringDup (OBJECT_NAME(i_obj));
      t_string libn = (t_string) strtok (name2, ":");

      dest = StringConcat2 ("./DUMP", libn);
      FileCopy (libn, dest);
      Free (name2);
      Free (dest);
    }
    else
    {
      dest = StringConcat2 ("./DUMP", OBJECT_NAME(i_obj));
      FileCopy (OBJECT_NAME(i_obj), dest);
      Free (dest);
    }
  }
  /* }}} */
  /* Copy the inputs {{{ */
  if (diabloobject_options.input_files_set)
  {
    t_string_array_elem *tmp;
    t_string_array *in = StringDivide (diabloobject_options.input_files, ";", TRUE, FALSE);

    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      name = FileNameNormalize (tmp->string);
      if (name[0] != '/')
        FATAL(("Input file is not absolute! Implement"));
      dest = StringConcat2 ("./DUMP", name);
      FileCopy (name, dest);
      Free (dest);
      Free (name);
    }
  }
  /* }}} */

  /* Copy the outputs {{{ */

  if (diabloobject_options.output_files_set)
  {
    t_string_array_elem *tmp;
    t_string_array *in = StringDivide (diabloobject_options.output_files, ";", TRUE, FALSE);

    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      name = FileNameNormalize (tmp->string);
      if (name[0] != '/')
        FATAL(("Output file is not absolute! Implement"));
      dest = StringConcat3 ("./DUMP", name, "_ref");
      FileCopy (name, dest);
      Free (dest);
      Free (name);
    }
  }

  /* }}} */

  if (multiple_dumps)
  {
    fp = fopen ("./DUMP/diablo_dump", "a");
    fprintf (fp, "=============\n");
  }
  else
    fp = fopen ("./DUMP/diablo_dump", "w");

  if (!fp)
    FATAL(("Failed to open diablo_dump, for writing"));
  fprintf (fp, "LPATH: ");
  for (tel = 0; tel < diabloobject_options.libpath->nelems; tel++)
  {
    fprintf (fp, "%s", diabloobject_options.libpath->dirs[tel]);
    if (tel + 1 < diabloobject_options.libpath->nelems)
      fprintf (fp, ":");
  }
  fprintf (fp, "\n");

  fprintf (fp, "OPATH: ");
  for (tel = 0; tel < diabloobject_options.objpath->nelems; tel++)
  {
    fprintf (fp, "%s", diabloobject_options.objpath->dirs[tel]);
    if (tel + 1 < diabloobject_options.objpath->nelems)
      fprintf (fp, ":");
  }
  fprintf (fp, "\n");

  fprintf (fp, "ARGS: %s\n", diabloobject_options.arguments);

  /* Set the input line {{{ */
  if (diabloobject_options.input_files_set)
  {
    t_string_array_elem *tmp;
    t_string_array *in = StringDivide (diabloobject_options.input_files, ";", TRUE, FALSE);

    fprintf (fp, "INPUTS: ");
    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      name = FileNameNormalize (tmp->string);
      if (name[0] != '/')
        FATAL(("Input file is not absolute! Implement"));
      fprintf (fp, "%s", name);
      if (tel + 1 < in->nstrings)
        fprintf (fp, ";");
      Free (name);
    }

    fprintf (fp, "\n");
  }
  /* }}} */

  /* Set the output line {{{ */
  /* The outputs */
  if (diabloobject_options.output_files_set)
  {
    t_string_array_elem *tmp;
    t_string_array *in = StringDivide (diabloobject_options.output_files, ";", TRUE, FALSE);

    fprintf (fp, "OUTPUTS: ");
    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      name = FileNameNormalize (tmp->string);
      if (name[0] != '/')
        FATAL(("Input file is not absolute! Implement"));
      fprintf (fp, "%s", name);
      if (tel + 1 < in->nstrings)
        fprintf (fp, ";");
      Free (name);
    }
    fprintf (fp, "\n");
  }

  /* }}} */

  fprintf (fp, "OBJ: %s\n", OBJECT_NAME(obj));
  fclose (fp);


  Tar ("./DUMP", "dump.tar");
}

/*}}} */

/* Execute the program {{{ */
void CreateProcPipe (int *retpid, FILE ** read_pipe, FILE ** write_pipe, char **argv, char *exec, ...);

#define HAVE_DIABLO_EXEC
void
DiabloExec ()
{
#ifndef _MSC_VER
  char *execname = FileNameNormalize ("./b.out");
  char *f;
  char *instring = StringDup ("DIABLO=1");
  FILE *in;
  FILE *out;
  pid_t pid;


  // if ((diabloobject_options.read)&&(!(diabloobject_options.dump||diabloobject_options.dump_multiple))) chmod("./b.out",0700);
  // else execname=FileFind(diabloobject_options.objpath,diabloobject_options.objectfilename);

  printf ("------------- Executing %s with args %s\n", execname, diabloobject_options.arguments);

  /* Do the inputs */
  if (diabloobject_options.input_files_set)
  {
    t_string_array_elem *tmp;
    t_string_array *in = StringDivide (diabloobject_options.input_files, ";", TRUE, FALSE);

    int tel = 0;


    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      if (diablosupport_options.prepend[0] != 0)
      {
        char *tf;

        name = StringConcat3 (diablosupport_options.prepend, "/", tf = FileNameNormalize (tmp->string));
        Free (tf);
      }
      else
      {
        name = FileNameNormalize (tmp->string);
      }

      if (tel + 1 < 10)
      {
        f = instring;
        instring = StringConcat3 (instring, "; Ix=", name);
        Free (f);
        sprintf (instring + strlen (instring) - strlen (name) - 2, "%d", tel + 1);
      }
      else
      {
        f = instring;
        instring = StringConcat3 (instring, "; Ixx=", name);
        Free (f);
        sprintf (instring + strlen (instring) - strlen (name) - 3, "%d", tel + 1);
      }
      *(instring + strlen (instring)) = '=';
      Free (name);
      tel++;
    }

    StringArrayFree (in);

  }

  /* Do the outputs */
  if (diabloobject_options.output_files_set)
  {
    int tel = 0;
    t_string_array *in = StringDivide (diabloobject_options.output_files, ";", TRUE, FALSE);
    t_string_array_elem *tmp;

    STRING_ARRAY_FOREACH_ELEM(in, tmp)
    {
      char *name;

      if (diablosupport_options.prepend[0] != 0)
      {
        char *tf;

        name = StringConcat3 (diablosupport_options.prepend, "/", tf = FileNameNormalize (tmp->string));
        Free (tf);
      }
      else
      {
        name = FileNameNormalize (tmp->string);
      }
      if (tel + 1 < 10)
      {
        f = instring;
        instring = StringConcat3 (instring, "; Ox=", name);
        Free (f);
        sprintf (instring + strlen (instring) - strlen (name) - 2, "%d", tel + 1);
      }
      else
      {
        f = instring;
        instring = StringConcat3 (instring, "; Oxx=", name);
        Free (f);
        sprintf (instring + strlen (instring) - strlen (name) - 3, "%d", tel + 1);
      }
      *(instring + strlen (instring)) = '=';
      Free (name);
      tel++;
    }

    StringArrayFree (in);

  }

  if (diabloobject_options.arguments_set)
  {
    char *tofree1, *tofree2;

    if (diabloobject_options.host_set)
    {
      CreateProcPipe (&pid, &out, &in, NULL, "ssh", diabloobject_options.host, tofree1 = StringConcat3 (tofree2 = StringConcat3 (instring, ";", execname), " ", diabloobject_options.arguments), NULL);
    }
    else
      CreateProcPipe (&pid, &out, &in, NULL, "bash", "-c", tofree1 = StringConcat3 (tofree2 = StringConcat3 (instring, ";", execname), " ", diabloobject_options.arguments), NULL);
    Free (tofree1);
    Free (tofree2);


  }
  else
  {
    CreateProcPipe (&pid, &out, &in, NULL, "bash", "-c", execname, NULL);
  }

  while (!feof (out))
  {
    char c;

    IGNORE_RESULT(fread (&c, 1, 1, out));
    if (feof (out))
      break;
    printf ("%c", c);
  }
  Free (instring);

  if (diabloobject_options.output_files_set)
  {
    t_string_array_elem *sa;
    t_string_array *ina = StringDivide (diabloobject_options.output_files, ";", TRUE, FALSE);

    STRING_ARRAY_FOREACH_ELEM(ina, sa)
    {
      int status;
      char *tmp;
      char *name, *f;

      if (diablosupport_options.prepend[0] != 0)
      {
        name = StringConcat3 (diablosupport_options.prepend, "/", tmp = FileNameNormalize (sa->string));
        Free (tmp);
      }
      else
      {
        name = FileNameNormalize (sa->string);
      }
      f = name;
      name = StringConcat3 (name, " ", name);
      Free (f);
      f = name;
      name = StringConcat3 ("diff -q -s ", name, "_ref");
      Free (f);

      printf ("====Exec %s\n", name);
      CreateProcPipe (&pid, &out, &in, NULL, "bash", "-c", name, NULL);
      while (!feof (out))
      {
        char c;

        IGNORE_RESULT(fread (&c, 1, 1, out));
        if (feof (out))
          break;
        printf ("%c", c);
      }
      waitpid (pid, &status, 0);

      /* is non-zero if the child exited normally. */
      if (WIFEXITED(status))
      {
        printf ("EXIT STATUS OF DIFF: %d\n", WEXITSTATUS(status));
      }
      /*
         else if ( WIFSIGNALED(status))
         {
         WTERMSIG(status)
         }
         */
      else
      {
        FATAL(("Diff crashed!"));
      }

      Free (name);
    }

    StringArrayFree (ina);

  }
#endif
}

/*#endif*/
/*#endif*/

void
CreateProcPipe (int *retpid, FILE ** read_pipe, FILE ** write_pipe, char **argv, char *exec, ...)
{
#ifndef _MSC_VER
  int filedes[2]; /* pipe */
  int file_rd = fileno (stdin); /* filedescriptor = 0 */
  int file_wr = fileno (stdout); /* filedescriptor = 1 */
  int into_proc, used_byproc_in, outof_proc, used_byproc_out;
  va_list val;
  int ret;

  ret = pipe (filedes); /* Create process in */
  into_proc = filedes[1];
  used_byproc_in = filedes[0];

  ret = pipe (filedes); /* Create process out */
  outof_proc = filedes[0];
  used_byproc_out = filedes[1];

  if (!((*retpid) = fork ()))
  {
    char *tmp;
    int nargs = 0;

    /* child */
    /* stdin replaced by pipe output */
#ifdef ALPHA
    if (!isastream (used_byproc_out))
      FATAL(("Seems like we are using a non-STREAMS-based pipe function\nThis happens on Tru64 Unix if you do not link with the sys5 library!"));
#endif

    /* close unused */

    close (outof_proc);
    close (into_proc);

    ret = dup2 (used_byproc_in, file_rd);
    /* stdout replaced by pipe input */
    dup2 (used_byproc_out, file_wr);

#ifdef ALPHA
    /* Provide terminal functionality on pipe.... Sometimes this is neccesary */
    ret = ioctl (used_byproc_out, I_PUSH, "ptem");
    if (ret < 0)
      FATAL(("Pseudo Terminal EMulation (output) failed...."));
    ret = ioctl (file_wr, I_PUSH, "ldterm");
    if (ret < 0)
      FATAL(("Line Discipline module (output) failed"));
    ret = ioctl (file_rd, I_PUSH, "ptem");
    if (ret < 0)
      FATAL(("Pseudo Terminal EMulation (input) failed...."));
    ret = ioctl (file_rd, I_PUSH, "ldterm");
    if (ret < 0)
      FATAL(("Line Discipline module (input) failed"));
#endif

    setbuf (stdin, NULL);
    setbuf (stdout, NULL);
    if (!argv)
    {
      argv = (char **) realloc (argv, sizeof (char *) * (nargs + 1));
      argv[nargs] = exec;
      nargs++;
      va_start (val, exec);
      while ((tmp = va_arg (val, char *)))
      {
        argv = (char **) realloc (argv, sizeof (char *) * (nargs + 1));
        argv[nargs] = tmp;
        nargs++;
      }
      argv = (char **) realloc (argv, sizeof (char *) * (nargs + 1));
      argv[nargs] = NULL;
      nargs++;
    }

    /* Run the program */
    execvp (exec, argv);
    FATAL(("Execl %s failed!", exec));
  }
  else
  {

    /* close unused */

    close (used_byproc_in);
    close (used_byproc_out);

    if (read_pipe)
    {
      *read_pipe = fdopen (outof_proc, "r");
      setbuf (*read_pipe, NULL);
      ASSERT(((*read_pipe) != NULL), ("Failed to open FILE descriptor for %s", exec));
    }

    if (write_pipe)
    {
      *write_pipe = fdopen (into_proc, "w");
      setbuf (*write_pipe, NULL);
      ASSERT(((*write_pipe) != NULL), ("Failed to open FILE descriptor for %s", exec));
    }

  }
#endif
}

/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
