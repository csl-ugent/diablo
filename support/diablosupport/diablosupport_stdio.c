/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define _GNU_SOURCE
#include <diablosupport.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

t_uint32 io_wrapper_type = 0;
t_const_string io_wrapper_file = 0;
t_uint32 io_wrapper_lnno = 0;
t_uint32 io_wrapper_status = 0;

static char escapes[256] = "\0";

t_io_modifier *io_modifiers = NULL;
t_io_handler *io_handlers = NULL;

static t_io_modifier *
IoModifierFind (char escape, char command)
{
  t_io_modifier *iter;

  for (iter = io_modifiers; iter != NULL; iter = iter->next)
  {
    if ((iter->escape == escape)
        && ((iter->command == command) || (iter->command == 0)))
    {
      return iter;
    }
  }
  return NULL;
}

static t_io_handler *
IoHandlerFind (t_uint32 id)
{
  t_io_handler *iter;

  for (iter = io_handlers; iter != NULL; iter = iter->next)
  {
    if (iter->id == id)
    {
      return iter;
    }
  }
  return NULL;
}

static t_string
sInternalIo (t_const_string message, va_list * ap)
{

  t_string_array *array = StringDivide (message, escapes, TRUE, TRUE);
  t_string_array_elem *elem;
  t_bool first = TRUE;
  t_string ret;

  STRING_ARRAY_FOREACH_ELEM(array, elem)
  {
    t_uint32 i = 0;
    t_io_modifier *mod = NULL;

    if (first)
    {
      first = FALSE;
      if (!CharStringPatternMatch (escapes, elem->string[0]))
        continue;
    }
    first = FALSE;
    if (strlen (elem->string) == 1)
    {
      if (!elem->next)
        FATAL(("Illegal modifier (end of message)"));
      else if (elem->next->string[0] == elem->string[0])
      {
        t_string new = StringDup (elem->next->string + 1);

        Free (elem->next->string);
        elem->next->string = new;
        first = TRUE;
        continue;
      }
      else
        FATAL(("Illegal combination %c%c\n", elem->next->string[0],
               elem->string[0]));
    }
    for (i = 1; i < strlen (elem->string); i++)
    {
      if ((mod = IoModifierFind (elem->string[0], elem->string[i])))
        break;
    }
    if (mod)
    {
      t_string modifiers = StringDup (elem->string + 1);
      t_string new;

      if (mod->valid_modifiers)
      {
        t_string tf;

        modifiers[i - 1] = 0;
        new =
          StringConcat2 (tf = mod->function (modifiers, ap), elem->string + i + 1);
        Free (tf);
      }
      else
        new = mod->function (modifiers, ap);

      Free (modifiers);
      Free (elem->string);
      elem->string = new;
    }
  }

  ret = StringArrayJoin (array, "");
  StringArrayFree (array);
  return ret;
}

t_string
StringIo (t_const_string message, ...)
{
  t_string out;
  va_list ap;

  va_start (ap, message);
  out = sInternalIo (message, &ap);
  AllocOverride (out);
  return out;
}

t_string
vStringIo (t_const_string message, va_list * ap)
{
  t_string out;

  out = sInternalIo (message, ap);
  AllocOverride (out);
  return out;
}

void
FileIo (FILE * fp, t_const_string message, ...)
{
  t_string out;
  va_list ap;

  va_start (ap, message);
  out = sInternalIo (message, &ap);
  fprintf (fp, "%s", out);
  Free (out);
}

void
InternalIo (t_uint32 type, t_const_string message, va_list * ap)
{
  t_string out;
  t_io_handler *han = IoHandlerFind (type);

  out = sInternalIo (message, ap);

  if (han)
  han->function (type, out);
  else {
    printf("<IO handlers uninitialised> ");
    vprintf(message, *ap);
    printf("\n");
  }
  Free (out);
}

void
Io (t_uint32 type, t_string message, ...)
{
  va_list ap;

  va_start (ap, message);
  InternalIo (type, message, &ap);
}

void
IoWrapper (t_const_string message, ...)
{
  va_list ap;

  va_start (ap, message);
  InternalIo (io_wrapper_type, message, &ap);
}

t_io_modifier *
IoModifierAdd (char escape, char command, t_string modifiers,
               t_string (*function) (t_const_string modifiers, va_list * ap))
{
  t_io_modifier *mod = Malloc (sizeof (t_io_modifier));

  if (!(CharStringPatternMatch (escapes, escape)))
  {
    t_uint32 pos = strlen (escapes);

    escapes[pos] = escape;
    escapes[pos + 1] = '\0';
  }

  mod->escape = escape;
  mod->command = command;
  mod->valid_modifiers = modifiers;
  mod->function = function;
  mod->next = io_modifiers;
  io_modifiers = mod;
  return mod;
}

void
IoModifiersFree ()
{
  while (io_modifiers)
  {
    t_io_modifier *tf = io_modifiers;

    io_modifiers = io_modifiers->next;
    Free (tf);
  }
}

void
IoHandlersFree ()
{
  while (io_handlers)
  {
    t_io_handler *tf = io_handlers;

    io_handlers = io_handlers->next;
    Free (tf);
  }
}

t_io_handler *
IoHandlerAdd (t_uint32 id, void (*function) (t_uint32 id, t_const_string out))
{
  t_io_handler *han = Malloc (sizeof (t_io_handler));

  han->id = id;
  han->function = function;
  han->next = io_handlers;
  io_handlers = han;
  return han;
}

#ifndef DIABLOSUPPORT_HAVE_VASPRINTF
t_uint32
vasprintf (char **ret, const char *fmt, va_list ap)
{
  static t_uint32 len = 0;
  static char *buffer = NULL;
  t_bool restart = FALSE;
  t_uint32 sn_ret;

  if (len == 0)
  {
    len = 100;
    buffer = (char*)Malloc(len * sizeof(char));
  }

  do
  {
    restart = FALSE;
    sn_ret = vsnprintf (buffer, len, fmt, ap);

    /* vsnprintf is defined by both C89 and C99 but the definition is incompatible:
     * + C99 (GCC): if the buffer is too small, vsnprintf will return the number
     *   of bytes the function _WOULD_ have written if the buffer had been big enough.
     * + C89 (Visual Studio): if the buffer is too small, vsnprintf will return -1
     *   (0xffffffff) to indicate that the output has been truncated.
     *
     * As a result of this AWESOME incompatibility, using a vasprintf based on vsnprintf
     * will work fine with C99-compliant compilers. Using this function in windows however
     * will result in sn_ret ultimately increasing to 0xffffffff. This means that the
     * application will slowly eat all available memory and ultimately it will crash.
     */
#if __STDC_VERSION__ >= 199901L
    while (sn_ret >= len)
#else
    if (sn_ret >= len)
#endif
    {
      len += 100;
      restart = TRUE;
    }
    if (restart)
      buffer = (char*)Realloc (buffer, sizeof (char) * len);
  }
  while (restart);

  *ret = strdup (buffer);

  return strlen(*ret);
}
#endif

t_string
IoModifierPrintf (t_const_string modifiers, va_list * ap)
{
  t_string fmt = NULL;
  t_string ret = NULL;
  t_string rret = NULL;
  t_uint32 tel;
  t_uint32 size = 0;
  va_list tmp;
  int tmpi;
  long int tmpli;
  long long int tmplli;
  double tmpd;
  void * tmpv;

  fmt = StringConcat2 ("%", modifiers);

  va_copy (tmp, (*ap));
  
  IGNORE_RESULT(vasprintf (&ret, fmt, tmp));

  for (tel = 0; tel < strlen (modifiers); tel++)
  {
    if (CharStringPatternMatch ("l", modifiers[tel]))
    {
      size++;
    }
    else if (CharStringPatternMatch ("diouxXc", modifiers[tel]))
    {
      if (size == 0)
        tmpi = va_arg (*ap, int);

      else if (size == 1)
        tmpli = va_arg (*ap, long int);

      else if (size == 2)
        tmplli = va_arg (*ap, long long int);

      break;
    }
    else if (CharStringPatternMatch ("eEfFgGaA", modifiers[tel]))
    {
      tmpd = va_arg (*ap, double);

      break;
    }
    else if (CharStringPatternMatch ("sp", modifiers[tel]))
    {
      tmpv = va_arg (*ap, void *);

      break;
    }
    else if (CharStringPatternMatch ("n", modifiers[tel]))
    {
      break;
    }
  }
  rret = StringDup (ret);
  free (ret);
  Free (fmt);
  return rret;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
