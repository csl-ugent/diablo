/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloobject.h>

unsigned int
process_uleb128 (const unsigned char *buf, uleb128 * result)
{
  unsigned int len = 0;
  unsigned char byte;

  if (result)
    (*result) = 0;

  //  printf("start processing uleb128 at %x ... ",buf);
  //  fflush(stdout);
  //  fflush(stderr);



  do
  {
    byte = *(buf + len);
        //      printf("%x ",byte&0xff);
        //  fflush(stdout);
        //  fflush(stderr);

    int msb = 1;
    unsigned char tmp = byte;
    while (tmp >>= 1)
      msb++;
    if ((len * 7 + msb)  > (sizeof (uleb128) * 8))
      FATAL(("Overflow in uleb128"));
    /* each chunk encodes 7 bits of the decoded number, we simply
     * discard the highest bit and shift if over 7 positions */
    if (result)
      *result |= ((uleb128) (byte & 0x7fU)) << (len * 7);
    len++;
  }
  /* Highest bit of the chunk is set for all chunks, except for the
   * terminating chunk */
  while ((byte & 0x80) != 0);
  //  printf(" ... done\n");
  //  fflush(stdout);
  //  fflush(stderr);

  return len;
}

unsigned int
encode_uleb128(uleb128 val, unsigned char *result)
{
  unsigned int len = 0;
  unsigned char byte;
  do
  {
    byte = val & 0x7f;
    val >>= 7;
    if (val)
      byte |= 0x80;
    if (result)
    {
      *result = byte;
      result++;
    }
    len++;
  } while (val != 0);
  return len;
}

unsigned int
process_sleb128 (const unsigned char *buf, sleb128 * result)
{
  unsigned int len = 0;
  unsigned char byte;

  if (result)
    (*result) = 0;
  do
  {
    byte = *(buf + len);
    int msb = 1;
    unsigned char tmp = byte;
    while (tmp >>= 1)
      msb++;
    if ((len * 7 + msb)  > (sizeof (sleb128) * 8))
      FATAL(("Overflow in sleb128"));
    if (result)
      *result |= ((sleb128) (byte & 0x7fU) << len * 7);
    len++;
  }
  while ((byte & 0x80) != 0);

  if (((len * 7 < (sizeof (uleb128) * 8))) && (byte & 0x40))
  {
    if (result)
      *result |= -(1 << len * 7);
  }
  return len;
}

unsigned int
encode_sleb128(sleb128 val, unsigned char *result)
{
  unsigned int len = 0;
  unsigned char byte;
  unsigned char size = sizeof(sleb128) * 8;
  t_int64 asign;
  t_bool neg = val < 0;
  t_bool more = TRUE;

  do
  {
    byte = val & 0x7f;
    val >>= 7;
    if (neg)
    {
      asign = 1;
      asign <<= (size - 7);
      val -= asign;
    }
    if (((val == 0) &&
         ((byte & 0x40) == 0)) ||
        ((val == -1) &&
         ((byte & 0x40) != 0)))
      more = FALSE;
    else
      byte |= 0x80;
    if (result)
    {
      *result = byte;
      result++;
    }
    len++;
  } while (more);
  return len;
}

unsigned int
decode_pointer32 (const t_object * obj, const unsigned char *buf, t_address pc, unsigned char type,
                  t_address * result)
{
  unsigned int ret = 0;
  t_bool sign = FALSE;
  t_address value = AddressNullForObject(obj);

  if (type == 0xff) /* omitted */
  {
    value = AddressNewForObject(obj, -1);
    ret = 0;
  }
  else
  {
    switch (type & 0xf)
    {
      case 0: /* absolute */
        if (OBJECT_ADDRESS_SIZE(obj)==ADDRSIZE32)
        {
          value = AddressNewForObject(obj,*((t_uint32 *) buf));
          ret = 4;
        }
        else
        {
          value = AddressNewForObject(obj,*((t_uint64 *) buf));
          ret = 8;
        }
        break;
      case 0x1: /* uleb128 */
      case 0x2: /* udata2 */
      case 0x3: /* udata4 */
        FATAL(("Implement pointer encoding type 0x%x\n", type));
        break;
      case 0x4: /* udata8 */
        value = AddressNewForObject(obj,*((t_uint64 *) buf));
        ret = 8;
        break;
      case 0x8: /* signed */
      case 0x9: /* sleb128 */
      case 0xa: /* sdata2 */
        FATAL(("Implement pointer encoding type 0x%x\n", type));
        break;
      case 0xb: /* sdata4 */
        sign = TRUE;
        value = AddressNewForObject(obj, (*((t_int32 *) buf)));
        ret = 4;
        break;
      case 0xc: /* sdata8 */
        FATAL(("Implement pointer encoding type 0x%x\n", type));
        break;
      default:
        FATAL(("Unknown pointer encoding type 0x%x\n", type));
    }
    switch (type & 0x70)
    {
      case 0x0: /* absolute */
        break;
      case 0x10: /* pcrel */
        if (sign)
        {
          value = AddressAdd(pc, value);
        }
        else
        {
          value = AddressAdd(pc, value);
        }
        break;
      case 0x20: /* textrel */
      case 0x30: /* datarel */
      case 0x40: /* funcrel */
      case 0x50: /* aligned */
        FATAL(("Implement relative pointer encoding type 0x%x\n", type));
    }

    if (type & 0x80) /* Indirect pointer */
    {
    }

  }

  if (result)
    *result = value;
  return ret;

}

/* #define VERBOSE_EH_FRAME */

void
DoEhFrame (t_object * obj, const t_section * sec)
{
  t_uint32 run = 0, run_save = 0, this_start = 0, this_size = 0, run_aug_data = 0;

  t_object *tmp2, *sub;
  t_section *subsec;
  int i;  

  typedef struct _t_cie
  {
    t_uint32 offset;
    t_bool fde_has_ldsa;
    char fde_pointer_encoding;

    struct _t_cie *next;
  } t_cie;

  t_cie *cache = NULL, *this_cie;

  OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp2)
  {
    OBJECT_FOREACH_SECTION(sub, subsec, i)
    {
      if (SECTION_PARENT_SECTION(subsec) != sec)
        continue;

      run = 0;
      run_save = 0;
      this_start = 0;
      this_size = 0;
      run_aug_data = 0;
#ifdef VERBOSE_EH_FRAME
      printf ("Have eh_frame for %s of size %x %x\n\n", OBJECT_NAME(sub),OBJECT_SIZE(sub),SECTION_CSIZE(subsec));
#endif

      while (((char *) SECTION_DATA(subsec))
             && AddressExtractUint32 (SECTION_CSIZE(subsec)) > run)
      {
        uleb128 utmp;
        sleb128 tmp;

        run_save = this_start = run;
        this_size = *((unsigned int *) (((char *) SECTION_DATA(subsec)) + run));
#ifdef VERBOSE_EH_FRAME
        printf ("%8.8x %8.8x ", run, this_size);
#endif
        /* length does not contain the 4 length field bytes */
        run_save += 4 + this_size;
        run += 4;
        if (this_size == 0)
        {
#ifdef VERBOSE_EH_FRAME
          printf ("ZERO Terminator");
#endif
        }
        else if ((*((unsigned int *) (((char *) SECTION_DATA(subsec)) + run))) == 0)
        {
          this_cie = Malloc (sizeof (t_cie));
          this_cie->offset = this_start;
          this_cie->fde_has_ldsa = 0;
          this_cie->fde_pointer_encoding = 0;
          this_cie->next = cache;
          cache = this_cie;
#ifdef VERBOSE_EH_FRAME
          printf ("00000000 CIE\n");
#endif
          run += 4;
#ifdef VERBOSE_EH_FRAME
          printf (" Version: %x\n",
                  *((unsigned char *) (((unsigned char *) SECTION_DATA(subsec)) + run)));
#endif
          run += 1;
#ifdef VERBOSE_EH_FRAME
          printf (" Augmentation: \"");
#endif
          run_aug_data =
            run + strlen (((char *) (((char *) SECTION_DATA(subsec)) + run))) + 1;
          run_aug_data +=
            process_uleb128 (((unsigned char *) (((unsigned char *) SECTION_DATA(subsec)) +
                                                 run_aug_data)), NULL);
          run_aug_data +=
            process_sleb128 (((unsigned char *) (((unsigned char *) SECTION_DATA(subsec)) +
                                                 run_aug_data)), NULL);
          run_aug_data++;

          while (*((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)))
          {
#ifdef VERBOSE_EH_FRAME
            printf ("%c", *((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)));
#endif
            switch (*((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)))
            {
              /* CIE and FDEs have augmentation body length */
              case 'z':
                run_aug_data +=
                  process_uleb128 (((unsigned char *) (((char *) SECTION_DATA(subsec)) +
                                                       run_aug_data)),
                                   &utmp);
                break;
                /* FDE holds pointer to LSDA */
              case 'L':
                this_cie->fde_has_ldsa = 1;
                break;
                /* CIE holds a pointer to a personality routine */
              case 'P':
                {
                  char type =
                    (*
                     ((unsigned char *) (((char *) SECTION_DATA(subsec)) +
                                         run_aug_data)));

                  run_aug_data += 1;
                  run_aug_data +=
                    decode_pointer32 (obj, (((unsigned char *) SECTION_DATA(subsec)) + run_aug_data),
                                      AddressAddUint32(SECTION_CADDRESS(subsec), run),
                                      type, NULL);
                  break;
                }
                /* Code pointers in each FDE have a non standard encoding */
              case 'R':
                this_cie->fde_pointer_encoding =
                  (*
                   ((unsigned char *) (((char *) SECTION_DATA(subsec)) + run_aug_data)));
                run_aug_data += 1;

                break;
              default:
#ifdef VERBOSE_EH_FRAME
                printf (": Unknown augmentation byte!\n");
#endif
                FATAL(("Unknown augmentation byte"));
            }
            run += 1;
          }
#ifdef VERBOSE_EH_FRAME
          printf ("\"\n");
#endif
          run += 1;
          run +=
            process_uleb128 (((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)),
                             &utmp);
#ifdef VERBOSE_EH_FRAME
          printf (" Code alignment factor: %d\n", utmp);
#endif
          run +=
            process_sleb128 (((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)),
                             &tmp);
#ifdef VERBOSE_EH_FRAME
          printf (" Data alignment factor: %d\n", tmp);
          printf (" Return address column: %x\n",
                  *((unsigned char *) (((char *) SECTION_DATA(subsec)) + run)));
#endif
          run += 1;

        }
        else if (this_size != 0)
        {
          t_uint32 pcrun = 0;
          t_cie *corr_cie = cache;
          t_address pc; 
          t_uint32 cie;

          cie = (*((unsigned int *) (((char *) SECTION_DATA(subsec)) + run)));
#ifdef VERBOSE_EH_FRAME
          printf ("%8.8x FDE ", cie);
          printf ("cie=%8.8x ", 4 + this_start - cie);
#endif
          while (corr_cie && (corr_cie->offset != 4 + this_start - cie))
            corr_cie = corr_cie->next;
          if (!corr_cie)
            FATAL(("CIE for FDE not found"));
          run += 4;

          pcrun = run;
          run +=
            decode_pointer32 (obj, (((unsigned char *) SECTION_DATA(subsec)) + run),
                              AddressAddUint32(SECTION_CADDRESS(subsec), run),
                              corr_cie->fde_pointer_encoding, &pc);

          /* Some binutils versions remove the pc field is the code corresponding to this CDE is skipped */
          /*if (pc == 0) */
          {
            t_reloc_ref *rr;

            for (rr = SECTION_REFERS_TO(subsec); rr; rr = RELOC_REF_NEXT(rr))
            {
              t_uint32 reloff =
                AddressExtractUint32 (RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)));

              if (reloff == pcrun)
                break;
            }

            if (rr)
            {
			  VERBOSE(0, ("REMOVING @R", RELOC_REF_RELOC(rr)));
              RelocTableRemoveReloc (OBJECT_RELOC_TABLE(SECTION_OBJECT(subsec)), RELOC_REF_RELOC(rr));
            }
            else
              FATAL(("Vodden2"));
          }
        }
#ifdef VERBOSE_EH_FRAME
        printf ("\n\n");
#endif
        run = run_save;
      }
    }
  }

  while(cache)
  {
	  t_cie * tmp=cache;
	  cache=cache->next;

	  Free(tmp);
  }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
