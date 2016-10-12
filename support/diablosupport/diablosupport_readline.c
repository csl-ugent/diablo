/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <stdio.h>
#include <stdlib.h>

#include <diablosupport.h>


/* This file contains implementations of functions in the
 * GNU Readline library for systems that don't have that
 * library.
 */


/* GetLine() is based on code from http://sourceforge.net/projects/libedit/,
 * a BSD-licensed implementation of the GNU Readline library.
 */
/* GetLine {{{ */
size_t
GetLine(char **lineptr, size_t *n, FILE *stream)
{
#define EXPAND_CHUNK 16

  char *line = *lineptr;
  size_t n_read = 0;
  *n = 0;

  if(lineptr == NULL)
    return -1;
  if(n == NULL)
    return -1;
  if(stream == NULL)
    return -1;

  while(1)
  {
    int c = getc(stream);

    if(c == EOF)
    {
      if(!line)
      {
        *lineptr = malloc(1);
        line = *lineptr;
        line[0] = '\0';
        n_read = 0;
      }
      if(n_read > 0)
        line[n_read] = '\0';
      break;
    }

    if((n_read + 2) >= *n)
    {
      size_t new_size;

      if(*n == 0)
        new_size = 16;
      else
        new_size = (*n * 2);

      if(*n >= new_size) // overflowed size_t
        line = NULL;
      else
        line = (((char*) *lineptr) ? realloc(*lineptr, new_size) : malloc(new_size));

      if(line)
      {
        *lineptr = line;
        *n = new_size;
      }
      else
      {
        if(*n > 0)
        {
          (*lineptr)[*n - 1] = '\0';
          n_read = (*n - 2);
        }
        break;
      }
    }

    line[n_read] = c;
    n_read++;

    if(c == '\n')
    {
      line[n_read] = '\0';
      break;
    }
  }

  return n_read;
}
/* }}} */
