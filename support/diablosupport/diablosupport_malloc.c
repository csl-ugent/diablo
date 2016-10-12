/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Memory allocation functions
 * ---------------------------
 *
 * if DIABLOSUPPORT_MALLOC_DEBUG is defined the [M|C|Re]alloc functions transparantly
 * allocate some extra structures for leak detection (support for
 * DIABLOSUPPORT_MALLOC_DEBUG
 * is also added in StringDup and StringConcat3)
 *
 * To free data allocate with these functions, use the Free function
 *
 * WARNING: do not use Free on normal malloced data if DIABLOSUPPORT_MALLOC_DEBUG is defined
 * !
 *
 * if a program crashes because of DIABLOSUPPORT_MALLOC_DEBUG, you're probably writing to
 * non allocated memory
 *
 */

/* Debug data structures {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
typedef struct _t_alloc_info
{
  char magic[8];
  struct _t_alloc_info *prev;
  struct _t_alloc_info *next;
  const char *mfname;
  int mlnno;
  int size;
} t_alloc_info;

static t_alloc_info *head = NULL;
static t_alloc_info *tail = NULL;

typedef struct _t_final_alloc_info
{
  const char *name;
  int lnno;
  int sum;
  struct _t_final_alloc_info *next;
} t_final_alloc_info;
#endif /* Malloc Debug */
/* }}} */
/* Malloc {{{ */
void *
RealMalloc (FORWARD_MALLOC_FUNCTION_DEF t_uint32 size)
{
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  void *ret = malloc (size + sizeof (t_alloc_info));
  t_alloc_info *ai = (t_alloc_info *) ret;
#else
  void *ret = malloc (size);
#endif
  if (size == 0)
  {
#ifndef DIABLOSUPPORT_MALLOC_DEBUG
    WARNING(("Mallocing 0 bytes!"));
    return NULL;
#else
    WARNING(("Mallocing 0 bytes! at %s - %d", filename, lnno));
#endif
  }

#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  if (ret == NULL)
  {
    fputs("Malloc failed (out of memory?) in file ", stderr);
    fputs(filename, stderr);
    fputs("\n", stderr);
    exit(-1);
  }
  ai->mfname = filename;
  ai->mlnno = lnno;
  ai->size = size;
  ai->prev = tail;
  ai->magic[0] = 'M';
  ai->magic[1] = 'A';
  ai->magic[2] = 'G';
  ai->magic[3] = 'I';
  ai->magic[4] = 'C';
  ai->magic[5] = '_';
  ai->magic[6] = 'A';
  ai->magic[7] = 'L';
  if (ai->prev)
    ai->prev->next = ai;
  ai->next = NULL;
  tail = ai;
  if (!head)
    head = ai;
#else
  if (ret == NULL)
  {
    fputs("Malloc failed (out of memory?)\n", stderr);
    exit(-1);
  }
#endif
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  return ((char *) ret) + sizeof (t_alloc_info);
#else
  return ret;
#endif

}

/* }}} End Malloc */
/* Calloc {{{ */
void *
RealCalloc (FORWARD_MALLOC_FUNCTION_DEF t_uint32 numelems, t_uint32 elemsize)
{
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  void *ret = calloc (1, elemsize * numelems + sizeof (t_alloc_info));
  t_alloc_info *ai = (t_alloc_info *) ret;
#else
  void *ret = calloc (numelems, elemsize);
#endif
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  if (ret == NULL)
  {
    fputs("Calloc failed (out of memory?) in file ", stderr);
    fputs(filename, stderr);
    fputs("\n", stderr);
    exit(-1);
  }

  ai->mfname = filename;
  ai->mlnno = lnno;
  ai->size = numelems * elemsize;
  ai->prev = tail;
  ai->magic[0] = 'M';
  ai->magic[1] = 'A';
  ai->magic[2] = 'G';
  ai->magic[3] = 'I';
  ai->magic[4] = 'C';
  ai->magic[5] = '_';
  ai->magic[6] = 'A';
  ai->magic[7] = 'L';
  if (ai->prev)
    ai->prev->next = ai;
  ai->next = NULL;
  tail = ai;
  if (!head)
    head = ai;
#else
  if (ret == NULL)
  {
    fputs("Calloc failed (out of memory?)\n", stderr);
    exit(-1);
  }
#endif
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  return ((char *) ret) + sizeof (t_alloc_info);
#else
  return ret;
#endif

}

/* }}} End Calloc */
/* Realloc {{{ */
void *
RealRealloc (FORWARD_MALLOC_FUNCTION_DEF const void *base, t_uint32 size)
{
  void *ret;

#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  t_alloc_info *ai =
    (t_alloc_info *) (((char *) base) - sizeof (t_alloc_info));
#endif
  if (size == 0)
  {
    if (base) Free ((void *) base);
    return NULL;
  }
  else
  {
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
    if (base)
    {
      ASSERT(((ai->magic[0] == 'M') && (ai->magic[1] == 'A')
              && (ai->magic[2] == 'G') && (ai->magic[3] == 'I')
              && (ai->magic[4] == 'C') && (ai->magic[5] == '_')
              && (ai->magic[6] == 'A')
              && (ai->magic[7] == 'L')),
             ("Reallocing non Alloced data (at %s:%d)", filename,
              lnno));
      ai->magic[0] = ai->magic[1] = ai->magic[2] = ai->magic[3] =
        ai->magic[4] = ai->magic[5] = ai->magic[6] = ai->magic[7] = 0;

      ret =
        realloc (((char *) base) - sizeof (t_alloc_info),
                 size + sizeof (t_alloc_info));

      if (ret == NULL)
      {
        fputs("Realloc failed (out of memory?) in file ", stderr);
        fputs(filename, stderr);
        fputs("\n", stderr);
        exit(-1);
      }
      if (ai == tail)
      {
        tail = (t_alloc_info *) ret;
      }

      if (ai == head)
      {
        head = (t_alloc_info *) ret;
      }

      ai = (t_alloc_info *) ret;
      ai->magic[0] = 'M';
      ai->magic[1] = 'A';
      ai->magic[2] = 'G';
      ai->magic[3] = 'I';
      ai->magic[4] = 'C';
      ai->magic[5] = '_';
      ai->magic[6] = 'A';
      ai->magic[7] = 'L';
      if (ai->next)
        ai->next->prev = ai;
      if (ai->prev)
        ai->prev->next = ai;
      ai->size = size;
      ret = ((char *) ret) + sizeof (t_alloc_info);
    }
    else
    {
      ret = RealMalloc (filename, lnno, size);
    }
#else
    ret = realloc ((void *) base, size);
    if (ret == NULL)
    {
      fputs("Realloc failed (out of memory?)\n", stderr);
      exit(-1);
    }
#endif
    return ret;
  }

}

/* }}} End Realloc */
/* Free {{{ */
void
RealFree (FORWARD_MALLOC_FUNCTION_DEF const void *to_free)
{
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  t_alloc_info *ai =
    (t_alloc_info *) (((char *) to_free) - sizeof (t_alloc_info));
#endif
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  ASSERT((to_free != NULL),
         ("Freeing NULL at %s-%d!!!", filename, lnno));
#else
  ASSERT((to_free != NULL), ("Freeing NULL!!!"));
#endif
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  if (!
      (((ai->magic[0] == 'M') && (ai->magic[1] == 'A')
        && (ai->magic[2] == 'G') && (ai->magic[3] == 'I')
        && (ai->magic[4] == 'C') && (ai->magic[5] == '_')
        && (ai->magic[6] == 'A') && (ai->magic[7] == 'L'))))
  {
    int tel;

    printf ("Freein non malloc data at %d in %s\n", lnno, filename);
    fflush (stdout);
    fflush (stderr);
    printf ("Block contains: ");
    for (tel = 0; tel < 10; tel++)
    {
      printf ("%c", ((char *) to_free)[tel]);
      fflush (stdout);
    }

    printf ("\n");
    if (
        (((ai->magic[0] == 'M') && (ai->magic[1] == 'A')
          && (ai->magic[2] == 'G') && (ai->magic[3] == 'I')
          && (ai->magic[4] == 'C') && (ai->magic[5] == '_')
          && (ai->magic[6] == 'F') && (ai->magic[7] == 'R'))))
    {
      printf ("Appears to be freed at %s %d\n", ai->mfname, ai->mlnno);
    }
    else
    {
      printf ("Free info destroyed\n");
    }

  }

  if (!((ai->magic[0] == 'M') && (ai->magic[1] == 'A')
          && (ai->magic[2] == 'G') && (ai->magic[3] == 'I')
          && (ai->magic[4] == 'C') && (ai->magic[5] == '_')
          && (ai->magic[6] == 'A')
          && (ai->magic[7] == 'L')))
    FATAL (("MY-Freeing non MY-alloced data (%lx at %s:%d)", to_free,
            filename, lnno));

  ai->magic[6] = 'F';
  ai->magic[7] = 'R';

  ai->mfname = filename;
  ai->mlnno = lnno;

  if (ai->prev)
    ai->prev->next = ai->next;
  else
    head = ai->next;
  if (ai->next)
    ai->next->prev = ai->prev;
  else
    tail = ai->prev;
  free (((char *) to_free) - sizeof (t_alloc_info));
#else
  free ((void *) to_free);
#endif
} /* }}} End Free */

/* CheckAddress {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
void
RealCheckAddress (FORWARD_MALLOC_FUNCTION_DEF const void *x)
{
  t_alloc_info *tmp = head;
  const char *y = x;

  while (tmp)
  {
    char *data = ((char *) tmp) + sizeof (t_alloc_info);

    ASSERT(((tmp->magic[0] == 'M') && (tmp->magic[1] == 'A')
            && (tmp->magic[2] == 'G') && (tmp->magic[3] == 'I')
            && (tmp->magic[4] == 'C') && (tmp->magic[5] == '_')
            && (tmp->magic[6] == 'A')
            && (tmp->magic[7] == 'L')),
           ("Malloc chtmpn corrupted (at %s:%d)", filename,
            lnno));
    if ((y >= data) && (y < data + tmp->size))
    {
      return;
    }
    tmp = tmp->next;
  }
  FATAL(("Invalid Allocated address (%s:%d))", filename, lnno));
}
#endif /* Debug Malloc */
/* }}} End CheckAddress */
/* PrintRemainingBlocks {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
void
RealPrintRemainingBlocks (void)
{
  t_final_alloc_info *first = NULL;
  t_final_alloc_info *run;
  t_final_alloc_info *run_next;
  t_alloc_info *tmp = head;
  int found;

  if (tmp) printf ("===================== Leak INFO =========================\n");

  while (tmp)
  {
    found = 0;
    for (run = first; run != NULL; run = run->next)
    {
      if ((tmp->mlnno == run->lnno)
          && (strcmp (run->name, tmp->mfname) == 0))
      {
        run->sum += tmp->size;
        found = 1;
        break;
      }
    }
    if (!found)
    {
      run = (t_final_alloc_info*)malloc (sizeof (t_final_alloc_info));
      run->sum = tmp->size;
      run->name = tmp->mfname;
      run->lnno = tmp->mlnno;
      run->next = first;
      first = run;
    }
    tmp = tmp->next;
  }

  for (run = first; run != NULL; run = run_next)
  {
    printf ("LEAK: Total block size %d allocated at %s-%d\n", run->sum,
            run->name, run->lnno);
    run_next = run->next;
    free (run);
  }
}
#endif /* Debug Malloc */
/* }}} End PrintRemainingBlocks */
/* GetMemUse {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
t_uint64
RealGetMemUse ()
{
  t_alloc_info *tmp = head;
  t_uint64 memuse = 0;

  while (tmp)
  {
    memuse += tmp->size;
    tmp = tmp->next;
  }
  return memuse;
}
#endif /* Debug Malloc */
/* }}} */
/* PrintMemUse {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
void
RealPrintMemUse ()
{
  t_uint64 memuse = GetMemUse ();

  printf ("Total Memory Use: %"PRIu64" bytes\n", memuse);
}
#endif /* Debug Malloc */
/* }}} End PrintMemUse */
/* PrintMallocInfo {{{ */
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
void
RealPrintMallocInfo (void *block)
{
  t_alloc_info *ai =
    (t_alloc_info *) (((char *) block) - sizeof (t_alloc_info));

  printf ("Block at %p , ai at %p\n", block, ai);
  if (!
      (((ai->magic[0] == 'M') && (ai->magic[1] == 'A')
        && (ai->magic[2] == 'G') && (ai->magic[3] == 'I')
        && (ai->magic[4] == 'C') && (ai->magic[5] == '_')
        && (ai->magic[6] == 'A') && (ai->magic[7] == 'L'))))
  {
    printf ("This block is not allocated!\n");
    printf ("Block starts with %c %c %c %c %c %c %c %c\n", ai->magic[0],
            ai->magic[1], ai->magic[2], ai->magic[3], ai->magic[4],
            ai->magic[5], ai->magic[6], ai->magic[7]);
  }
  printf ("Block allocated at %s %d\n", ai->mfname, ai->mlnno);
}
#endif
/* }}} End PrintMallocInfo */
void
RealAllocOverride (FORWARD_MALLOC_FUNCTION_DEF void *r)
{
#ifdef DIABLOSUPPORT_MALLOC_DEBUG
  t_alloc_info *ai = (t_alloc_info *) (((char *) r) - sizeof (t_alloc_info));

  ai->mfname = filename;
  ai->mlnno = lnno;
#endif
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
