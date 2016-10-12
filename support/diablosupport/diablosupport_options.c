/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef STAGE
#include <opt_gen_handler.h>
#define FUNNAME(x) OptGen##x
#else
#include <diablosupport.h>
#define FUNNAME(x) Option##x
#endif
void 
FUNNAME(Defaults) (t_option * option_list)
{
  t_option *iter = option_list;

  while (iter->type != OPTION_NONE)
  {
    if ((iter->set) && (!(*iter->set)))
    {
      switch (iter->type)
      {
#ifndef STAGE
        case OPTION_PATH:
          {
            t_string tmp;

            if (!(iter->defaults.default_string))
              break;

            tmp = iter->defaults.default_string;
            *(iter->variable.path) = PathNew (tmp, PATH_DELIMITER);
            break;
          }
#endif
        case OPTION_STRING:
          {
            if (!(iter->defaults.default_string))
              break;
            *(iter->variable.string) = iter->defaults.default_string;
            break;
          }
        case OPTION_STRING_ARRAY:
          {
            /* Just do nothing... */
            break;
          }
        case OPTION_BOOL:
          *(iter->variable.boolean) = (iter->defaults.default_bool);
          break;
        case OPTION_COUNT:
        case OPTION_INT:
          *iter->variable.count =
            (iter->defaults.default_count);
          break;
        default:
          /* keep the compiler happy */
          break;
      }
    }
    iter++;
  }
}

/*!
 * This functions prints the usage information for the application */

void
UsageOptionMessage (t_option * iter)
{
  char out[500] = "\0";

  if (iter->hidden)
    return;
  if (iter->required != 1)
    sprintf (out, "[");

  if (iter->short_option)
    sprintf (out + strlen (out), "-%s", iter->short_option);

  if ((iter->short_option) && (iter->long_option))
    sprintf (out + strlen (out), "|");

  if (iter->long_option)
    sprintf (out + strlen (out), "--%s", iter->long_option);

  if (((iter->short_option) || (iter->long_option))
      && (iter->type != OPTION_COUNT) && (iter->type != OPTION_USAGE) &&
      (iter->type != OPTION_VERSION))
    sprintf (out + strlen (out), " ");

  switch (iter->type)
  {
    case OPTION_PATH:
      sprintf (out + strlen (out), "<path>");
      break;
    case OPTION_STRING:
      sprintf (out + strlen (out), "<string>");
      break;
    case OPTION_STRING_ARRAY:
      sprintf (out + strlen (out), "<string_array>");
      break;
    case OPTION_FILE:
      sprintf (out + strlen (out), "<file>");
      break;
    case OPTION_BOOL:
      sprintf (out + strlen (out), "[on|off]");
      break;
    case OPTION_COUNT:
    case OPTION_USAGE:
      break;
    case OPTION_INT:
      sprintf (out + strlen (out), "<number>");
      break;
    default:
      /* keep the compiler happy */
      break;
  }

  if (iter->required != 1)
    sprintf (out + strlen (out), "]: ");
  else
    sprintf (out + strlen (out), ": ");

  printf (" %s", out);
  if (iter->description)
    printf ("%s\n", iter->description);
  else
    printf ("\n");
}

#ifndef STAGE
typedef struct _t_option_group
{
  t_hash_table_node n;
  struct _t_option_group *next;

} t_option_group;
#endif

void
Usage (t_option * option_list)
{
  t_option *iter = option_list;

#ifndef STAGE
  t_option_group *f = NULL;
  t_option_group *g;
  t_hash_table *h =
    HashTableNew (7, 0, (t_hash_func) StringHash, (t_hash_cmp)strcmp, NULL);
#endif
#ifndef STAGE
  VERBOSE(0, ("\nUsage:"));
#else
  printf ("\nUsage:\n");
#endif
#ifndef STAGE
  while (iter->type != OPTION_NONE)
  {
    if (iter->hidden)
    {
      iter++;
      continue;
    }
    if (iter->group && (!HashTableLookup (h, iter->group)))
    {
      g = malloc (sizeof (t_option_group));
      HASH_TABLE_NODE_SET_KEY(&(g->n), StringDup (iter->group));
      g->next = f;
      f = g;
      HashTableInsert (h, &(g->n));
    }
    iter++;
  }

  printf ("The following options exist:\n\n");
  for (g = f; g != NULL; g = g->next)
  {
    printf (" %s options:\n\n", (char *) (HASH_TABLE_NODE_KEY(&(g->n))));
    iter = option_list;
    while (iter->type != OPTION_NONE)
    {
      if (iter->group && (strcmp (iter->group, HASH_TABLE_NODE_KEY(&(g->n))) == 0))
        UsageOptionMessage (iter);
      iter++;
    }
    printf (" \n\n");
  }

  printf (" Normal options: \n\n");
#endif
  iter = option_list;
  while (iter->type != OPTION_NONE)
  {
#ifndef STAGE
    if (!(iter->group))
#endif
      UsageOptionMessage (iter);
    iter++;
  }
  printf (" \n\n");

}

/*!
 *
 * fills in the options that can be change via the environment */

void 
FUNNAME(GetEnvironment) (t_option * option_list)
{
  char *ret;
  t_option *iter = option_list;

  while (iter->type != OPTION_NONE)
  {
    if ((iter->set) && (!(*iter->set)) && (iter->enviromental_option))
    {
      ret = getenv (iter->enviromental_option); /* The search-path for libraries */
      if (ret)
      {
        switch (iter->type)
        {
#ifndef STAGE
          case OPTION_PATH:
            *iter->variable.path = PathNew (ret, PATH_DELIMITER);
            break;
#endif
          default:
          case OPTION_STRING:
          case OPTION_STRING_ARRAY:
          case OPTION_FILE:
          case OPTION_BOOL:
          case OPTION_COUNT:
            FATAL(("Implement"));
        }
        *iter->set = TRUE;
      }
    }
    iter++;
  }
}

/*!
 * \param argc The argument count
 * \param argv The list of arguments
 *
 * fills in the options that are changed on the commandline
 */

void
FUNNAME(ParseCommandLine) (t_option * option_list, t_uint32 argc,
                           char **argv, t_bool final)
{
  t_uint32 tel;

  for (tel = 1; tel < argc; tel++)
  {
    if (argv[tel] != NULL)
    {
      t_bool skip = FALSE, found = FALSE;
      t_option *iter = option_list;

      while (iter->type != OPTION_NONE)
      {
        if (((argv[tel][0] == '-') && (argv[tel][1] != '-')
             && (iter->short_option)
             && (strcmp (iter->short_option, &argv[tel][1]) == 0))
            || ((argv[tel][0] == '-') && (argv[tel][1] == '-')
                && (iter->long_option)
                && (strcmp (iter->long_option, &argv[tel][2]) == 0))
            || ((argv[tel][0] != '-') && (!iter->long_option)
                && (!iter->short_option)))
        {
          if ((!iter->set) ||
              (!(*iter->set)) ||
              (iter->type == OPTION_COUNT))
          {
            if (iter->set)
              (*iter->set) = TRUE;
            switch (iter->type)
            {
              case OPTION_PATH:
                {
#ifndef STAGE
                  t_string tmp;

                  if ((tel + 1) >= argc)
                  {
                    Usage (option_list);
                    FATAL(("-%s expects a path!",
                           iter->short_option));
                  }

                  tmp = StringDup (argv[tel + 1]);

                  *iter->variable.path = PathNew (tmp, PATH_DELIMITER);
                  Free (tmp);
                  skip = TRUE;
                  found = TRUE;
#else
                  FATAL(("Paths not available in STAGE1"));
#endif
                }
                break;
              case OPTION_STRING:
                if ((iter->short_option || iter->long_option)
                    && ((tel + 1) >= argc))
                {
                  Usage (option_list);
                  FATAL(("-%s expects a string!",
                         iter->short_option));
                }
                if (iter->short_option || iter->long_option)
                {
                  *iter->variable.string = StringDup (argv[tel + 1]);
                  skip = TRUE;
                }
                else
                  *iter->variable.string = StringDup (argv[tel]);
                found = TRUE;
                break;
              case OPTION_STRING_ARRAY:
                {
#ifndef STAGE
                  t_uint32 tel2 = tel;

                  if (iter->short_option || iter->long_option)
                  {
                    if ((tel + 1) >= argc)
                    {
                      Usage (option_list);
                      FATAL(("-%s expects one or more strings!",
                             iter->short_option));
                    }

                    tel2++;
                    skip = TRUE;
                  }

                  *iter->variable.sarray = StringArrayNew ();

                  for ( ; tel2 < argc; tel2++)
                  {
                    if (!argv[tel2])
                      break;
                    if (argv[tel2][0] == '-')
                      break;
                    StringArrayAppendString (*iter->variable.sarray, argv[tel2]);
                  }

                  tel = tel2 - 1;
                  if (skip) tel--;
                  found = TRUE;

                  break;
#else
                  FATAL (("String arrays not available in STAGE1"));
#endif
                }
              case OPTION_COUNT:
                found = TRUE;
                (*iter->variable.count)++;
                break;
              case OPTION_BOOL:
                if ((tel + 1) >= argc)
                {
                  found = TRUE;
                  *iter->variable.boolean =
                    !iter->defaults.default_bool;
                }
                else
                {
                  if (argv[tel + 1]
                      && strcmp (argv[tel + 1], "on") == 0)
                  {
                    found = TRUE;
                    skip = TRUE;
                    *iter->variable.boolean = TRUE;
                  }
                  else if (argv[tel + 1]
                           && strcmp (argv[tel + 1], "off") == 0)
                  {
                    found = TRUE;
                    skip = TRUE;
                    *iter->variable.boolean = FALSE;
                  }
                  else
                  {
                    found = TRUE;
                    *iter->variable.boolean =
                      !(iter->defaults.default_bool);
                  }
                }
                break;

              case OPTION_INT:
                {
                  t_string tmp;

                  if ((tel + 1) >= argc)
                  {
                    Usage (option_list);
                    FATAL(("-%s expects a numeric argument!",
                           iter->short_option));
                  }

                  tmp = StringDup (argv[tel + 1]);
                  *iter->variable.count = atoi (tmp);
                  Free (tmp);
                  skip = TRUE;
                  found = TRUE;
                }
                break;
              case OPTION_USAGE:
                {
                  Usage (option_list);
                  if (final)
                    exit (0);
                  else
                    return;
                }
                break;
              case OPTION_VERSION:
                {
                  iter->variable.function();
                  if (final)
                    exit (0);
                  else
                    return;
                }
                break;
              case OPTION_FILE:
                {
                  if ((iter->short_option || iter->long_option)
                      && ((tel + 1) >= argc))
                  {
                    Usage (option_list);
                    FATAL(("-%s expects a filename!",
                           iter->short_option));
                  }
                  if (iter->short_option || iter->long_option)
                  {
#ifndef STAGE
                    *iter->variable.string =
                      FileNameNormalize (argv[tel + 1]);
#else
                    *iter->variable.string = StringDup (argv[tel + 1]);
#endif
                    skip = TRUE;
                  }
                  else
#ifndef STAGE
                    *iter->variable.string =
                      FileNameNormalize (argv[tel]);
#else
                    *iter->variable.string = StringDup (argv[tel]);
#endif
                  found = TRUE;
                }

              default:
                /* keep the compiler happy */
                break;
            }
          }
          else
          {
            FATAL(("Option %s ignored (duplicate set)",
                   argv[tel]));
          }
        }
        if (found)
          break;
        iter++;

      }
      if (!found)
      {
        if (final)
        {
          Usage (option_list);
          FATAL(("Unknown option %s!", argv[tel]));
        }
        else
        {
          /* TODO: REMEMBER THE OPTION STRUCT! */
        }
      }
      else
      {
        argv[tel] = NULL;
      }

      if (skip)
      {
        tel++;
        argv[tel] = NULL;
        continue;
      }
    }
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
