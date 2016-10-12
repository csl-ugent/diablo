/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
/******** Various ********/

/* CharStringPatternMatch */
t_bool
CharStringPatternMatch (t_const_string pattern, char match)
{
  t_uint32 tel = 0;
  t_uint32 len = strlen (pattern);
  t_uint16 prev = 256;
  t_bool negate = (pattern[0] == '^');

  if (negate)
    tel++;

  for (; tel < len; tel++)
  {
    if (pattern[tel] == '-')
    {
      if ((prev != 256) && (tel + 1 < len))
      {
        if ((match >= prev) && (match <= pattern[tel + 1]))
        {
          return (!negate);
        }
      }
      else
      {
        if (match == '-')
          return (!negate);
      }
    }
    else
    {
      if (match == pattern[tel])
        return (!negate);
    }
    prev = pattern[tel];
  }
  return negate;
}

/* StringToUint32 */
t_uint32
StringToUint32 (t_const_string str, t_uint32 maxlen)
{
  t_uint32 val = 0, tel = 0, base = 10;

  if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))
  {
    base = 16;
    tel = 2;
  }

  while ((!maxlen) || (tel < maxlen))
  {
    if (base == 10)
    {
      if ((str[tel] >= '0') && (str[tel] <= '9'))
        val = val * 10 + (str[tel] - '0');
      else
        break;
    }
    else
    {
      if ((str[tel] >= '0') && (str[tel] <= '9'))
        val = val * 16 + (str[tel] - '0');
      else if ((str[tel] >= 'a') && (str[tel] <= 'f'))
        val = val * 16 + 10 + (str[tel] - 'a');
      else if ((str[tel] >= 'A') && (str[tel] <= 'F'))
        val = val * 16 + 10 + (str[tel] - 'A');
      else
        break;
    }
    tel++;
  }

  if ((!maxlen) && (str[tel] != 0))
    FATAL(("Trailing characters in string!"));

  return val;

}

/* StringToUint64 */
t_uint64
StringToUint64 (t_const_string str, t_uint32 maxlen)
{
  t_uint64 val = 0, tel = 0, base = 10;

  if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))
  {
    base = 16;
    tel = 2;
  }

  while ((!maxlen) || (tel < maxlen))
  {
    if (base == 10)
    {
      if ((str[tel] >= '0') && (str[tel] <= '9'))
        val = val * 10 + (str[tel] - '0');
      else
        break;
    }
    else
    {
      if ((str[tel] >= '0') && (str[tel] <= '9'))
        val = val * 16 + (str[tel] - '0');
      else if ((str[tel] >= 'a') && (str[tel] <= 'f'))
        val = val * 16 + 10 + (str[tel] - 'a');
      else if ((str[tel] >= 'A') && (str[tel] <= 'F'))
        val = val * 16 + 10 + (str[tel] - 'A');
      else
        break;
    }
    tel++;
  }

  if ((!maxlen) && (str[tel] != 0))
    FATAL(("Trailing characters in string! %s - %s",str,&str[tel]));

  return val;

}

/******** Strings ********/
/* StringDup */
t_string
RealStringDup (FORWARD_MALLOC_FUNCTION_DEF t_const_string in)
{
  char *ret;

  if (in == NULL)
    return NULL;
  ret =
    RealCalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (char), (strlen (in) + 1));
  strcpy (ret, in);
  return ret;
}

t_string
RealStringnDup(FORWARD_MALLOC_FUNCTION_DEF t_const_string in, int length)
{
  char *ret;

  if (in == NULL)
    return NULL;
  ret =
    RealCalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (char), (length + 1));
  strncpy (ret, in, length);
  return ret;
}

/* StringCmp */
t_int32
StringCmp (t_const_string in1, t_const_string in2)
{
  return strcmp (in1, in2);
}

/* StringChop */
t_bool
StringChop (t_string in, char c)
{
  int len = strlen (in);

  if (in[len - 1] == c)
    in[len - 1] = '\0';
  else
    return FALSE;
  return TRUE;
}

/* StringChomp */
void
StringChomp (t_string in)
{
  int len = strlen (in);

  if (in[len - 1] == '\n')
    in[len - 1] = '\0';
}

/* StringTrim */
void
StringTrim (t_string string)
{
  int i;

  for (i = strlen (string) - 1; i >= 0; i--)
    if (!isspace (string[i]))
      break;
  string[i + 1] = '\0';
}

/* StringConcat2 */
t_string
RealStringConcat2 (FORWARD_MALLOC_FUNCTION_DEF t_const_string in1,
                   t_const_string in2)
{
  t_string out =
    (t_string) RealMalloc (FORWARD_MALLOC_FUNCTION_USE strlen (in1) +
                           strlen (in2) + 1);

  sprintf (out, "%s%s", in1, in2);
  return out;

}

/* StringConcat3 */
t_string
RealStringConcat3 (FORWARD_MALLOC_FUNCTION_DEF t_const_string in1,
                   t_const_string in2, t_const_string in3)
{
  t_string out =
    (t_string) RealMalloc (FORWARD_MALLOC_FUNCTION_USE strlen (in1) +
                           strlen (in2) + strlen (in3) + 1);

  sprintf (out, "%s%s%s", in1, in2, in3);
  return out;
}

/* StringConcat4 */
t_string
RealStringConcat4 (FORWARD_MALLOC_FUNCTION_DEF t_const_string in1,
                   t_const_string in2, t_const_string in3, t_const_string in4)
{
  t_string out =
    (t_string) RealMalloc (FORWARD_MALLOC_FUNCTION_USE strlen (in1) +
                           strlen (in2) + strlen (in3) + strlen(in4) + 1);

  sprintf (out, "%s%s%s%s", in1, in2, in3, in4);
  return out;
}

/* StringPatternMatch*/
t_uint32
StringPatternMatch (t_const_string mask, t_const_string s)
{
  t_const_string cp = 0, mp = 0;

  for (; *s && *mask != '*'; mask++, s++)
    if (*mask != *s && *mask != '?')
      return 0;
  for (;;)
  {
    if (!*s)
    {
      while (*mask == '*')
        mask++;
      return !*mask;
    }
    if (*mask == '*')
    {
      if (!*++mask)
        return 1;
      mp = mask;
      cp = s + 1;
      continue;
    }
    if (*mask == *s || *mask == '?')
    {
      mask++, s++;
      continue;
    }
    mask = mp;
    s = cp++;
  }
}


/* Two different string hash functions */

#if 0
/* This is much slower than the following */
t_uint32
StringHash (t_string vkey, t_hash_table * table)
{
  char *key = (char *) vkey;
  t_uint32 n = 0;

  while (*key != 0)
    n = ((n + n + *(key++)) % table->tsize);
  return n;
}
#else
t_uint32
StringHash (t_const_string vkey, const t_hash_table * table)
{
  char *key = (char *) vkey;
  t_uint32 n = 0;

  ASSERT(key, ("Key not set in string hash!"));

  while (*key != 0)
    n = ((n * 131 + *(key++)));

  n %= HASH_TABLE_TSIZE(table);
  return n;
}
#endif

void StringToUpper(t_string in)
{
  t_uint32 len = strlen(in);
  t_uint32 i;

  for (i = 0; i < len; ++i)
  {
    if (in[i] >= 97 && in[i] <= 122)
      in[i] -= 32;
  }
}

void StringToLower(t_string in)
{
  t_uint32 len = strlen(in);
  t_uint32 i;

  for (i = 0; i < len; ++i)
  {
    if (in[i] >= 65 && in[i] <= 90)
      in[i] += 32;
  }
}

/* StringArrays {{{ */

/* StringDivide */
t_string_array *
RealStringDivide (FORWARD_MALLOC_FUNCTION_DEF t_const_string in,
                  t_const_string separators, t_bool strict, t_bool keep_sep)
{
  int len = strlen (in);
  int tel, sublength = 0;
  t_bool prev_is_separator = TRUE;
  t_bool this_is_separator = FALSE;
  t_const_string start_prev = in;
  t_string_array *ret = RealStringArrayNew (FORWARD_MALLOC_ONLY_FUNCTION_USE);

  for (tel = 0; tel < len; tel++)
  {
    this_is_separator = CharStringPatternMatch (separators, in[tel]);
    if ((this_is_separator) && ((strict) || (!prev_is_separator)))
    {
      t_string tmp =
        RealMalloc (FORWARD_MALLOC_FUNCTION_USE sublength + 1);

      strncpy (tmp, start_prev, sublength);
      tmp[sublength] = '\0';
      RealStringArrayAppendString (FORWARD_MALLOC_FUNCTION_USE ret, tmp);
      start_prev += sublength - (keep_sep ? 1 : 0);
      sublength = keep_sep ? 1 : 0;
    }
    if (!this_is_separator)
    {
      sublength++;
    }
    else
    {
      start_prev++;
      sublength = keep_sep ? 1 : 0;
    }

    prev_is_separator = this_is_separator;
  }
  if (sublength)
  {
    t_string tmp = RealMalloc (FORWARD_MALLOC_FUNCTION_USE sublength + 1);

    strncpy (tmp, start_prev, sublength);
    tmp[sublength] = '\0';
    RealStringArrayAppendString (FORWARD_MALLOC_FUNCTION_USE ret, tmp);
    start_prev += sublength;
    sublength = 0;
  }
  return ret;
}


/* StringArrayNew */
t_string_array *
RealStringArrayNew (FORWARD_MALLOC_ONLY_FUNCTION_DEF)
{
  t_string_array *ret =
    RealCalloc (FORWARD_MALLOC_FUNCTION_USE 1, sizeof (t_string_array));

  return ret;
}

/* StringArrayAppendString */
void
RealStringArrayAppendString (FORWARD_MALLOC_FUNCTION_DEF t_string_array *
                             array, t_string string)
{
  t_string_array_elem *elem =
    RealMalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (t_string_array_elem));

  elem->string = string;
  elem->prev = array->last;
  if (array->last)
    array->last->next = elem;
  else
    array->first = elem;
  elem->next = NULL;
  array->last = elem;
  array->nstrings++;
}

/* StringArrayPrependString */
void
RealStringArrayPrependString (FORWARD_MALLOC_FUNCTION_DEF t_string_array *
                              array, t_string string)
{
  t_string_array_elem *elem =
    RealMalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (t_string_array_elem));

  elem->string = string;
  elem->next = array->first;
  if (array->first)
    array->first->prev = elem;
  else
    array->last = elem;
  elem->prev = NULL;
  array->first = elem;
  array->nstrings++;
}

/* StringArrayNStrings */
t_uint16
StringArrayNStrings (const t_string_array * array)
{
  return array->nstrings;
}

/* StringArrayJoin */
t_string
RealStringArrayJoin (FORWARD_MALLOC_FUNCTION_DEF const t_string_array * array,
                     t_const_string separator)
{
  t_string_array_elem *iter;
  t_string ret;
  int nstr = 0;
  int len = 0;

  STRING_ARRAY_FOREACH_ELEM(array, iter)
  {
    len += strlen (iter->string);
    nstr++;
  }

  len += ((nstr - 1) * strlen (separator)) + 1;

  ret = RealMalloc (FORWARD_MALLOC_FUNCTION_USE len);

  len = 0;

  STRING_ARRAY_FOREACH_ELEM(array, iter)
  {
    sprintf (ret + len, "%s", iter->string);
    len += strlen (iter->string);
    nstr--;
    if (nstr != 0)
    {
      sprintf (ret + len, "%s", separator);
      len += strlen (separator);
    }
  }
  ret[len] = '\0';
  return ret;

}

/* StringArrayFree */
void
StringArrayFree (const t_string_array * to_free)
{
  t_string_array_elem *elem, *elem2;

  STRING_ARRAY_FOREACH_ELEM_SAFE(to_free, elem, elem2)
  {
    Free (elem->string);
    Free (elem);
  }
  Free (to_free);
}

/* }}} */

/* Strtab {{{ */
t_strtab_hash *
StrtabHashNew(t_const_string key, t_uint32 offset)
{
  t_strtab_hash * ret = Malloc(sizeof(t_strtab_hash));
  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(ret), StringDup (key));
  ret->offset = offset;

  return ret;
}

void
StrtabHashFree(const t_strtab_hash * tf, void * ignored)
{
  Free(HASH_TABLE_NODE_KEY(T_HASH_TABLE_NODE(tf)));
  Free(tf);
}

t_strtab * 
StrtabNew()
{
  t_strtab * ret = Calloc(1, sizeof(t_strtab));
  ret->ht = HashTableNew (1024, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, (t_hash_node_free) StrtabHashFree);

  return ret;
}

char * 
StrtabFindString(const t_strtab * strtab,  t_const_string string)
{
  t_strtab_hash * h = HashTableLookup(strtab->ht, string);

  if (h) 
  {
    return strtab->strtab + h->offset;
  }
  else
  {
    return NULL;
  }
}

t_uint32 
StrtabAddString (t_strtab * strtab, t_const_string string)
{
  t_uint32 offset;
  char * found = StrtabFindString(strtab, string);

  if (found)
  {
    offset = found - (strtab->strtab);
  }
  else
  {
    offset = strtab->len;
    strtab->len += strlen(string) + 1;
    strtab->strtab = Realloc(strtab->strtab, strtab->len);
    strcpy(strtab->strtab + offset, string);

    HashTableInsert(strtab->ht, StrtabHashNew(string, offset));
  }

  return offset;
}

void 
StrtabFree (const t_strtab * strtab)
{
  Free(strtab->strtab);
  HashTableFree(strtab->ht);
  Free(strtab);
}
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
