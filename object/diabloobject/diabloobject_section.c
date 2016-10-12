/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Includes {{{ */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <diabloobject.h>
/*}}}*/

void
SectionInsertInObject (t_section * sec, t_object* obj)
{
  char type = SECTION_TYPE(sec);
  SECTION_SET_OBJECT(sec, obj);

  switch (type)
  {
    case CODE_SECTION:
    case FLOWGRAPHING_CODE_SECTION:
    case FLOWGRAPHED_CODE_SECTION:
    case DEFLOWGRAPHING_CODE_SECTION:
    case DISASSEMBLING_CODE_SECTION:
    case DISASSEMBLED_CODE_SECTION:
    case ASSEMBLING_CODE_SECTION:
      OBJECT_SET_CODE(obj, Realloc (OBJECT_CODE(obj), sizeof (t_section *) * ((OBJECT_NCODES(obj)) + 1)));
      OBJECT_CODE(obj)[(OBJECT_NCODES(obj))] = sec;
      OBJECT_SET_NCODES(obj, OBJECT_NCODES(obj) + 1);
      break;
    case RODATA_SECTION:
      OBJECT_SET_RODATA(obj, Realloc (OBJECT_RODATA(obj), sizeof (t_section *) * ((OBJECT_NRODATAS(obj)) + 1)));
      OBJECT_RODATA(obj)[(OBJECT_NRODATAS(obj))] = sec;
      OBJECT_SET_NRODATAS(obj, OBJECT_NRODATAS(obj) + 1);
      break;
    case DATA_SECTION:
      OBJECT_SET_DATA(obj, Realloc (OBJECT_DATA(obj), sizeof (t_section *) * ((OBJECT_NDATAS(obj)) + 1)));
      OBJECT_DATA(obj)[(OBJECT_NDATAS(obj))] = sec;
      OBJECT_SET_NDATAS(obj, OBJECT_NDATAS(obj) + 1);
      break;
    case TLSDATA_SECTION:
      OBJECT_SET_TLSDATA(obj, Realloc (OBJECT_TLSDATA(obj), sizeof (t_section *) * ((OBJECT_NTLSDATAS(obj)) + 1)));
      OBJECT_TLSDATA(obj)[(OBJECT_NTLSDATAS(obj))] = sec;
      OBJECT_SET_NTLSDATAS(obj, OBJECT_NTLSDATAS(obj) + 1);
      break;
    case NOTE_SECTION:
      OBJECT_SET_NOTE(obj, Realloc (OBJECT_NOTE(obj), sizeof (t_section *) * ((OBJECT_NNOTES(obj)) + 1)));
      OBJECT_NOTE(obj)[(OBJECT_NNOTES(obj))] = sec;
      OBJECT_SET_NNOTES(obj, OBJECT_NNOTES(obj) + 1);
      break;
    case BSS_SECTION:
      OBJECT_SET_BSS(obj, Realloc (OBJECT_BSS(obj), sizeof (t_section *) * ((OBJECT_NBSSS(obj)) + 1)));
      OBJECT_BSS(obj)[(OBJECT_NBSSS(obj))] = sec;
      OBJECT_SET_NBSSS(obj, OBJECT_NBSSS(obj) + 1);
      break;
    case TLSBSS_SECTION:
      OBJECT_SET_TLSBSS(obj, Realloc (OBJECT_TLSBSS(obj), sizeof (t_section *) * ((OBJECT_NTLSBSSS(obj)) + 1)));
      OBJECT_TLSBSS(obj)[(OBJECT_NTLSBSSS(obj))] = sec;
      OBJECT_SET_NTLSBSSS(obj, OBJECT_NTLSBSSS(obj) + 1);
      break;
    case DEBUG_SECTION:
      OBJECT_SET_DEBUG(obj, Realloc (OBJECT_DEBUG(obj), sizeof (t_section *) * ((OBJECT_NDEBUGS(obj)) + 1)));
      OBJECT_DEBUG(obj)[(OBJECT_NDEBUGS(obj))] = sec;
      OBJECT_SET_NDEBUGS(obj, OBJECT_NDEBUGS(obj) + 1);
      break;
    case ATTRIB_SECTION:
      OBJECT_SET_ATTRIB(obj, Realloc (OBJECT_ATTRIB(obj), sizeof (t_section *) * ((OBJECT_NATTRIBS(obj)) + 1)));
      OBJECT_ATTRIB(obj)[(OBJECT_NATTRIBS(obj))] = sec;
      OBJECT_SET_NATTRIBS(obj, OBJECT_NATTRIBS(obj) + 1);
      break;
    case SPECIAL_SECTION:
      /* these are not stored in the section arrays */
      break;
    default:
      FATAL(("Unsupported type %c in add!", type));
  }

  /* add the section to the section hash table */
  if (!OBJECT_SECTION_TABLE(obj))
    FATAL(("Object has no section table!"));

  HashTableSetKeyForNode (OBJECT_SECTION_TABLE(obj), sec, StringDup (SECTION_NAME(sec)));
  HashTableInsert (OBJECT_SECTION_TABLE(obj), sec);
}

t_section *
SectionCreateForObject (t_object * obj, char type, t_section * parent,
                        t_address size, t_const_string name)
{
  t_section *ret = SectionNew (obj, type, name);
  SectionInsertInObject (ret, obj);
  SECTION_SET_CSIZE(ret, size);
  SECTION_SET_OLD_SIZE(ret, size);
  if (type != BSS_SECTION && type != SPECIAL_SECTION && !AddressIsNull (size))
    SECTION_SET_DATA(ret, Calloc (1, AddressExtractUint32 (size)));
  if (parent)
  {
    SECTION_SET_RELOCATABLE_TYPE(ret, RT_SUBSECTION);
    SECTION_SET_IS_SUBSECTION(ret, TRUE);
    SectionLinkToParent (ret, parent);
  }

  return ret;
}

void SectionLinkToParent(t_section* sec, t_section* parent)
{
  if (SECTION_SUBSEC_FIRST (parent))
  {
    t_section *last = SECTION_SUBSEC_LAST (parent);
    SECTION_SET_SUBSEC_NEXT (last, sec);
    SECTION_SET_SUBSEC_PREV (sec, last);
    SECTION_SET_SUBSEC_LAST (parent, sec);
  }
  else
  {
    SECTION_SET_SUBSEC_FIRST (parent, sec);
    SECTION_SET_SUBSEC_LAST (parent, sec);
    SECTION_SET_SUBSEC_PREV (sec, NULL);
  }

  SECTION_SET_SUBSEC_NEXT (sec, NULL);
  SECTION_SET_PARENT_SECTION (sec, parent);
  SECTION_SET_IS_MAPPED (sec, TRUE);
}

void SectionUnlinkFromParent(t_section* sec)
{
  t_section* parent = SECTION_PARENT_SECTION(sec);
  t_section *prev = SECTION_SUBSEC_PREV (sec);
  t_section *next = SECTION_SUBSEC_NEXT (sec);
  if (prev)
    SECTION_SET_SUBSEC_NEXT (prev, next);
  else
    SECTION_SET_SUBSEC_FIRST (parent, next);
  if (next)
    SECTION_SET_SUBSEC_PREV (next, prev);
  else
    SECTION_SET_SUBSEC_LAST (parent, prev);
}

void SectionReparent (t_section *sec, t_section *parent)
{
  ASSERT (!SECTION_IS_SUBSECTION(sec), ("Cannot reparent parent sections"));

  /* unlink from previous parent */
  if (SECTION_PARENT_SECTION (sec))
    SectionUnlinkFromParent (sec);

  /* link to new parent */
  SectionLinkToParent (sec, parent);
}

/* Section Constructors {{{ */
/* SectionGetFromObjectByAddress {{{ */

/*! Gets a section at an address in an objectfile. This never returns tls
 * sections (as their final adress is not known).
 *
 * \param obj The object in which we are searching
 * \param address The (generic) address at which we are looking
 *
 * \return t_section * The section at the address if found else NULL
 */

t_section *
SectionGetFromObjectByAddress (const t_object * obj, t_address address)
{
  t_uint32 tel;

  for (tel = 0; tel < OBJECT_NCODES(obj); tel++)
  {
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_CODE(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_CODE(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_CODE(obj)[tel]))))
      {
        return OBJECT_CODE(obj)[tel];
      }
  }

  for (tel = 0; tel < OBJECT_NDATAS(obj); tel++)
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_DATA(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_DATA(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_DATA(obj)[tel]))))
      {
        return OBJECT_DATA(obj)[tel];
      }

  for (tel = 0; tel < OBJECT_NNOTES(obj); tel++)
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_NOTE(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_NOTE(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_NOTE(obj)[tel]))))
      {
        return OBJECT_NOTE(obj)[tel];
      }

  for (tel = 0; tel < OBJECT_NRODATAS(obj); tel++)
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_RODATA(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_RODATA(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_RODATA(obj)[tel]))))
      {
        return OBJECT_RODATA(obj)[tel];
      }

  for (tel = 0; tel < OBJECT_NBSSS(obj); tel++)
  {
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_BSS(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_BSS(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_BSS(obj)[tel]))))
      {
        return OBJECT_BSS(obj)[tel];
      }
  }

  for (tel = 0; tel < OBJECT_NDEBUGS(obj); tel++)
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_DEBUG(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_DEBUG(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_DEBUG(obj)[tel]))))
      {
        return OBJECT_DEBUG(obj)[tel];
      }

  for (tel = 0; tel < OBJECT_NATTRIBS(obj); tel++)
    if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_ATTRIB(obj)[tel])))
      if (AddressIsLt
          (address,
           AddressAdd (SECTION_CSIZE(OBJECT_ATTRIB(obj)[tel]),
                       SECTION_CADDRESS(OBJECT_ATTRIB(obj)[tel]))))
      {
        return OBJECT_ATTRIB(obj)[tel];
      }

  return NULL;
}

/*}}}*/
/*SubsectionGetFromObjectByAddress {{{ */
/*! Returns the subsection at a certain address for an object
 *
 * \param obj The object in which we search
 * \param address The address where the subsection should be
 *
 * \return t_section * A subsection if found, otherwise NULL
 */
t_section *
SubsectionGetFromObjectByAddress (const t_object * obj, t_address address)
{
  t_object *tmp2, *tmp;

  for (tmp2 = OBJECT_MAPPED_FIRST(obj); tmp2 != NULL; tmp2 = OBJECT_NEXT(tmp2))
  {
    for (tmp = tmp2; tmp != NULL; tmp = OBJECT_EQUAL(tmp))
    {
      t_uint32 tel;

      for (tel = 0; tel < OBJECT_NCODES(tmp); tel++)
      {
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_CODE(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_CODE(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_CODE(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_CODE(tmp)[tel]));
            return OBJECT_CODE(tmp)[tel];
          }
      }

      for (tel = 0; tel < OBJECT_NDATAS(tmp); tel++)
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_DATA(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_DATA(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_DATA(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_DATA(tmp)[tel]));
            return OBJECT_DATA(tmp)[tel];
          }

      for (tel = 0; tel < OBJECT_NNOTES(tmp); tel++)
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_NOTE(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_NOTE(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_NOTE(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_NOTE(tmp)[tel]));
            return OBJECT_NOTE(tmp)[tel];
          }

      for (tel = 0; tel < OBJECT_NRODATAS(tmp); tel++)
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_RODATA(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_RODATA(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_RODATA(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_RODATA(tmp)[tel]));
            return OBJECT_RODATA(tmp)[tel];
          }

      for (tel = 0; tel < OBJECT_NBSSS(tmp); tel++)
      {
        VERBOSE(0, ("@G\n", SECTION_CADDRESS(OBJECT_BSS(tmp)[tel])));
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_BSS(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_BSS(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_BSS(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_BSS(tmp)[tel]));
            return OBJECT_BSS(tmp)[tel];
          }
      }

      for (tel = 0; tel < OBJECT_NDEBUGS(tmp); tel++)
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_DEBUG(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_DEBUG(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_DEBUG(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_DEBUG(tmp)[tel]));
            return OBJECT_DEBUG(tmp)[tel];
          }

      for (tel = 0; tel < OBJECT_NATTRIBS(tmp); tel++)
        if (AddressIsGe (address, SECTION_CADDRESS(OBJECT_ATTRIB(tmp)[tel])))
          if (AddressIsLt
              (address,
               AddressAdd (SECTION_CSIZE(OBJECT_ATTRIB(tmp)[tel]),
                           SECTION_CADDRESS(OBJECT_ATTRIB(tmp)[tel]))))
          {
            printf ("Possible %s %s\n", OBJECT_NAME(tmp),
                    SECTION_NAME(OBJECT_ATTRIB(tmp)[tel]));
            return OBJECT_ATTRIB(tmp)[tel];
          }

    }
  }
  return NULL;
}

/*}}}*/
/* ObjectGetSectionFromObjectByName {{{ */

/*!
 * \par obj The object from which we want to get a section
 * \par name The name of the section
 *
 * \ret A section when found, NULL otherwise
 *
 * Get's a section from an objectfile by name.*/

t_section *
SectionGetFromObjectByName (const t_object * obj, t_const_string name)
{
  t_section *hn = (t_section *) HashTableLookup (OBJECT_SECTION_TABLE(obj), name);

  if (hn)
  {
    if (HASH_TABLE_NODE_EQUAL(ClassCast (t_hash_table_node, hn)))
      FATAL(("More than one section for section name in file %s, section name %s", OBJECT_NAME(obj), name));
    return hn;
  }
  else
  {
    return NULL;
  }
}

/*}}}*/
/* ObjectGetSectionFromObjectByIndex {{{ */

/* helper routine/type to get a section by index from an object {{{ */
typedef struct
{
 t_int32 secidx;
 t_section *sec;
} t_secidxstruct;

void TexasRanger(void *sec, void *_ret)
{
  t_secidxstruct *ret = (t_secidxstruct*)_ret;
  
  if (SECTION_INDEX_IN_OBJ((t_section*)sec)==ret->secidx)
  {
    ASSERT(!ret->sec,("Multiple sections with index %d in object file",
                      ret->secidx));
    ret->sec=(t_section*)sec;
  }
}
/* }}} */

/*!
 * \par obj The object from which we want to get a section
 * \par index The index of the section
 *
 * \ret A section when found, NULL otherwise
 *
 * Get's a section from an objectfile by section index.*/

t_section *
SectionGetFromObjectByIndex (const t_object * obj, t_int32 index)
{
  t_secidxstruct findsec;
 
  ASSERT(index>=0,("Section indices < 0 mean that the section was not read from an object file"));

  findsec.secidx=index;
  findsec.sec=NULL;
  HashTableWalk(OBJECT_SECTION_TABLE(obj),TexasRanger,&findsec);
  return findsec.sec;  
}
/*}}}*/

/* t_section * ObjectNewSubsection(t_object * obj,t_uint32 size, char section_type) {{{ */
t_section * ObjectNewSubsection(t_object * obj, t_address size, char section_type)
{
  t_object  *pobj;
  t_section *sec,*added;

  pobj=ObjectGetLinkerSubObject(obj);
  switch (section_type)
  {
    case RODATA_SECTION:
      sec=SectionGetFromObjectByName(obj,".rodata");
      if (!sec)
        sec=SectionGetFromObjectByName(obj,"ER_RO");
      added=SectionCreateForObject(pobj, RODATA_SECTION, sec, size, "__added_by_diablo__");
      break;
    case DATA_SECTION:
      sec=SectionGetFromObjectByName(obj,".data");
      if (!sec)
        sec=SectionGetFromObjectByName(obj,"ER_RW");
      added=SectionCreateForObject(pobj, DATA_SECTION, sec, size, "__added_by_diablo__");
      break;
    case BSS_SECTION:
      sec=SectionGetFromObjectByName(obj,".bss");
      if (!sec)
        sec=SectionGetFromObjectByName(obj,"ER_ZI");
      added=SectionCreateForObject(pobj, BSS_SECTION, sec, size, "__added_by_diablo__");
      break;
    default:
      FATAL(("Only a rodata, data or bss section can be added\n"));
  }

  ASSERT(added,("Adding section failed!"));

  SECTION_SET_ALIGNMENT(added,AddressNewForSection(obj,4));

  return added;
}
/* }}} */
/*}}}*/

/* Load from a section {{{ */
/*!
 * Get's the 32 bits data-word stored at a certain offset in a section.
 * Automatically swaps endian. Calls should go through SectionGetData32
 *
 * \param file The file where the function was called from
 * \param lnno The linenumber where this function was called
 * \param sec The section from which we want to load
 * \param offset The offset in the section
 *
 * \return A 32 bits word.
 */

t_uint64
RealSectionGetData64 (t_const_string file, int lnno, const t_section * sec, t_address offset)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section read at %s %d!", file, lnno));

  t_uint64 val = 0;
  void *load = AddressAddDispl (SECTION_DATA(sec), offset);
  memcpy(&val, load, sizeof(val));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint64SwapEndian (val);
  return val;
}

t_uint32
RealSectionGetData32 (t_const_string file, int lnno, const t_section * sec, t_address offset, t_bool ignore_out_of_section, t_bool * out_of_section_ignored)
{
  t_uint32 bytes_to_read = 4;

  if (AddressIsGe (offset, SECTION_CSIZE(sec))
      || AddressIsGt(AddressAddUint32(offset, 4), SECTION_CSIZE(sec)))
  {
    if (ignore_out_of_section)
    {
      ASSERT(out_of_section_ignored, ("out_of_section_ignored is NULL while it should not!"));
      *out_of_section_ignored = TRUE;
      bytes_to_read = AddressSub(SECTION_CSIZE(sec), offset);
      VERBOSE(1, ("ignoring out of section read, reading %d bytes instead of 4: @G (size @G)", bytes_to_read, offset, SECTION_CSIZE(sec)));
    }
    else
      FATAL(("Out of section read at %s %d!\nobject %s section %s offset @G size @G", file, lnno, OBJECT_NAME (SECTION_OBJECT (sec)), SECTION_NAME (sec), offset, SECTION_CSIZE (sec)));
  }

  if (!SECTION_DATA(sec))
    FATAL(("Trying to get data from .bss-like section at %s %d!\n" , file, lnno)); 

  /* Do the load using a memcpy to handle alignment (the compiler will optimize
   * this). We specialize for reading 4 bytes (which will be optimized to 1
   * instruction) and do a normal memcpy for other values.
   */
  t_uint32 val = 0;
  void *load = AddressAddDispl (SECTION_DATA(sec), offset);
  if (bytes_to_read != 4)
    memcpy(&val, load, bytes_to_read);
  else
    memcpy(&val, load, sizeof(val));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint32SwapEndian (val);
  if (bytes_to_read != 4)
    VERBOSE(1, ("   read value 0x%x", val));
  return val;
}

t_uint16
RealSectionGetData16 (t_const_string file, int lnno, const t_section * sec, t_address offset)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section read at %s %d!", file, lnno));

  t_uint16 val = 0;
  void *load = AddressAddDispl (SECTION_DATA(sec), offset);
  memcpy(&val, load, sizeof(val));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint16SwapEndian (val);
  return val;
}

t_uint8
RealSectionGetData8 (t_const_string file, int lnno, const t_section * sec, t_address offset)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section read at %s %d!", file, lnno));

  void *load = AddressAddDispl (SECTION_DATA(sec), offset);
  return (*((t_uint8 *) load));
}

int
RealSectionGetLEB128 (t_const_string file, int lnno, const t_section *sec, t_address offset, t_uint64 *result)
{
  int shift = 0;
  t_uint8 byte = 0;
  void *load = AddressAddDispl (SECTION_DATA(sec), offset);
  *result = 0;

  do
  {
    /* check if it still fits in our uint64 */
    if (shift > 8)
      FATAL(("No support for uleb128 values bigger than 8 bytes (%s %d)!",
            file, lnno));
    if (AddressIsGe(AddressAddUint32(offset,shift),SECTION_CSIZE(sec)))
      FATAL(("Reading (U)LEB128 at offset @G (started at offset @G) which is past end of section @T",AddressAddUint32(offset,shift),offset,sec));

    /* decode next byte and add it to result */
    byte = *((t_uint8 *) (load + shift));
    *result |= ((byte & 0x7f) << (shift*7));

    /* next? */
    shift++;
    byte >>= 7;
  } while (byte != 0);

  return shift;
}

t_uint64
RealSectionGetULEB128 (t_const_string file, int lnno, const t_section *sec, t_address offset, int* len)
{
  t_uint64 result = 0;
  *len = RealSectionGetLEB128(file, lnno, sec, offset, &result); 
  /*return *((t_uint64 *) result);*/
  return result;
}

t_int64
RealSectionGetSLEB128 (t_const_string file, int lnno, const t_section *sec, t_address offset, int* len)
{
  t_uint64 result = 0;
  int shift = RealSectionGetLEB128(file, lnno, sec, offset, &result); 
  *len = shift;

  /* do we need to sign extend? */
  int sign_bits = -1 << (7*shift);
  if (((sign_bits >> 1) & result) != 0)
    result |= sign_bits;

  return *((t_int64 *) &result);
}
/*}}}*/

/* {{{ Write to a section */
void
RealSectionSetData64 (t_const_string file, int lnno, t_section * sec, t_address offset, t_uint64 val)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section write at %s %d!", file, lnno));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint64SwapEndian (val);

  void* addr = AddressAddDispl (SECTION_DATA(sec), offset);
  memcpy(addr, &val, sizeof(val));
}

void
RealSectionSetData32 (t_const_string file, int lnno, t_section * sec, t_address offset, t_uint32 val)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section write at %s %d!", file, lnno));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint32SwapEndian (val);

  void* addr = AddressAddDispl (SECTION_DATA(sec), offset);
  memcpy(addr, &val, sizeof(val));
}

void
RealSectionSetData16 (t_const_string file, int lnno, t_section * sec, t_address offset, t_uint16 val)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section write at %s %d!", file, lnno));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    val = Uint16SwapEndian (val);

  void* addr = AddressAddDispl (SECTION_DATA(sec), offset);
  memcpy(addr, &val, sizeof(val));
}

void
RealSectionSetData8 (t_const_string file, int lnno, t_section * sec, t_address offset, t_uint8 val)
{
  if (AddressIsGe (offset, SECTION_CSIZE(sec)))
    FATAL(("Out of section read at %s %d!", file, lnno));

  void* addr = AddressAddDispl (SECTION_DATA(sec), offset);
  *((t_uint8 *) addr) = val;
}

/* }}} */

/* FUN : SectionHashTableNodeFree
 * PAR : void * (=the section hashnode to free), void * (=must be zero)
 * RET : nothing
 * DESC: Free's a t_section_hash_table_node structure allocated with
 * SectionHashTableNodeNew */

void
SectionHashTableNodeFree (const void *to_free, void *data)
{
  t_section *s = (t_section *) to_free;

  Free (HASH_TABLE_NODE_KEY(ClassCast (t_hash_table_node, s)));
}

/* SectionGetSubsections {{{ :return an array containing all subsections of a given parent section */

static int
_sec_compare_old_addresses (const t_section * const * s1, const t_section * const * s2)
{
  /* We need to take special care when reordering .got subsections. The NULLGOT must come first, followed by all PLTGOT
   * subsections (starting with DYNAMIC_GOTELEM:), and then all other subsections.
   */
  if(!strcmp(".got", SECTION_NAME(SECTION_PARENT_SECTION(*s1))))
  {
    /* NULLGOT comes first */
    if(!strcmp("NULLGOT", SECTION_NAME(*s1))) return -1;
    if(!strcmp("NULLGOT", SECTION_NAME(*s2))) return 1;

    /* Then all PLTGOT subsections, these must be contiguous */
    if(StringPatternMatch("DYNAMIC_GOTELEM:*", SECTION_NAME(*s1)))
    {
      if(StringPatternMatch("DYNAMIC_GOTELEM:*", SECTION_NAME(*s2)))
      {

        /* If subsections have been added after linker emulation they'll have an old_address of 0, these must come last */
        if(AddressIsNull(SECTION_OLD_ADDRESS(*s1))) return 1;
        if(AddressIsNull(SECTION_OLD_ADDRESS(*s2))) return -1;

        if (AddressIsLt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
          return -1;
        if (AddressIsGt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
          return 1;
      }
      else
        return -1;
    }
    if(StringPatternMatch("DYNAMIC_GOTELEM:*", SECTION_NAME(*s2)))
      return 1;
  }

  /* We need to take care when ordering .dynamic subsections, as the .dynamic.END subsection must come last */
  if (!strcmp(".dynamic", SECTION_NAME(SECTION_PARENT_SECTION(*s1))))
  {
    /* .dynamic.END comes last */
    if (!strcmp(".dynamic.END", SECTION_NAME(*s1))) return 1;
    if (!strcmp(".dynamic.END", SECTION_NAME(*s2))) return -1;
  }

  /* If subsections have been added after linker emulation they'll have an old_address of 0, these must come last */
  if(AddressIsNull(SECTION_OLD_ADDRESS(*s1))) return 1;
  if(AddressIsNull(SECTION_OLD_ADDRESS(*s2))) return -1;

  if (AddressIsLt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
    return -1;
  if (AddressIsGt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
    return 1;
  /* if a section is 0 bytes, order it before sections > 0 bytes,
   * because otherwise ObjectMarkSubsectionGapsAsData will add
   * wrong $d symbols (it will detect a gap between the end
   * of the section of 0 bytes and the next section, which is
   * actually filled by the non-0 bytes section)
   */
  if (AddressIsNull (SECTION_CSIZE(*s2)))
      return 1;
  return -1;
}

t_section **
SectionGetSubsections (const t_section * sec, t_uint32 * nsubs_ret)
{
  t_uint32 j;
  t_section **subs;
  t_uint32 nsubs = 0;
  t_section *subsec;

  /* we want the section to be a parent section */
  ASSERT(SECTION_PARENT_SECTION(sec) == NULL, ("Requested subsections from a section that isn't a parent section!"));

  if (SECTION_TYPE(sec) == CODE_SECTION ||
      SECTION_TYPE(sec) == RODATA_SECTION ||
      SECTION_TYPE(sec) == DATA_SECTION ||
      SECTION_TYPE(sec) == TLSDATA_SECTION ||
      SECTION_TYPE(sec) == BSS_SECTION ||
      SECTION_TYPE(sec) == TLSBSS_SECTION ||
      SECTION_TYPE(sec) == NOTE_SECTION)
  {

    /* count the number of subsections */
    SECTION_FOREACH_SUBSECTION (sec, subsec)
      nsubs++;

    if (nsubs)
    {
      /* store all subsections in an array and sort them */
      subs = (t_section **) Malloc (sizeof (t_section *) * nsubs);
      j = 0;
      SECTION_FOREACH_SUBSECTION (sec, subsec)
        subs[j++] = subsec;
      diablo_stable_sort (subs, nsubs, sizeof (t_section *),
             (int (*)(const void *, const void *))
             _sec_compare_old_addresses);
    }
    else
    {
      subs = NULL;
    }
  }
  else
  {
    /* in this case, no subsections should be returned:
     * subsections are meaningless for a flow graph... */
    nsubs = 0;
    subs = NULL;
  }

  *nsubs_ret = nsubs;
  return subs;
} /* }}} */

void SectionRemoveFromObject (t_section * sec)
{
  t_object * obj = SECTION_OBJECT(sec);
  t_section ** array = NULL;
  t_uint32 nsecs = 0, tel, tel2;

  switch (SECTION_TYPE(sec))
  {
    case CODE_SECTION:
    case FLOWGRAPHING_CODE_SECTION:
    case FLOWGRAPHED_CODE_SECTION:
    case DEFLOWGRAPHING_CODE_SECTION:
    case DISASSEMBLING_CODE_SECTION:
    case DISASSEMBLED_CODE_SECTION:
    case ASSEMBLING_CODE_SECTION:
      array = OBJECT_CODE(obj);
      nsecs = OBJECT_NCODES(obj);
      break;
    case RODATA_SECTION:
      array = OBJECT_RODATA(obj);
      nsecs = OBJECT_NRODATAS(obj);
      break;
    case DATA_SECTION:
      array = OBJECT_DATA(obj);
      nsecs = OBJECT_NDATAS(obj);
      break;
    case TLSDATA_SECTION:
      array = OBJECT_TLSDATA(obj);
      nsecs = OBJECT_NTLSDATAS(obj);
      break;
    case NOTE_SECTION:
      array = OBJECT_NOTE(obj);
      nsecs = OBJECT_NNOTES(obj);
      break;
    case TLSBSS_SECTION:
      array = OBJECT_TLSBSS(obj);
      nsecs = OBJECT_NTLSBSSS(obj);
      break;
    case BSS_SECTION:
      array = OBJECT_BSS(obj);
      nsecs = OBJECT_NBSSS(obj);
      break;
    case DEBUG_SECTION:
      array = OBJECT_DEBUG(obj);
      nsecs = OBJECT_NDEBUGS(obj);
      break;
    case ATTRIB_SECTION:
      array = OBJECT_ATTRIB(obj);
      nsecs = OBJECT_NATTRIBS(obj);
      break;
    default:
      FATAL(("Unsupported type %c in SectionRemoveFromObject!", SECTION_TYPE(sec)));
  }

  for (tel = 0; tel < nsecs; tel++)
  {
    if (array[tel] == sec) break;
  }

  if (tel == nsecs) FATAL(("Trying to delete section from object to which it does not belong!"));

  for (tel2 = tel + 1; tel2 < nsecs; tel2++)
  {
    array[tel2 - 1] = array[tel2];
  }

  nsecs--;
  tel--;

  switch(SECTION_TYPE(sec))
  {
    case CODE_SECTION:
    case FLOWGRAPHING_CODE_SECTION:
    case FLOWGRAPHED_CODE_SECTION:
    case DEFLOWGRAPHING_CODE_SECTION:
    case DISASSEMBLING_CODE_SECTION:
    case DISASSEMBLED_CODE_SECTION:
    case ASSEMBLING_CODE_SECTION:
      OBJECT_SET_CODE(obj, array);
      OBJECT_SET_NCODES(obj, nsecs);
      break;
    case RODATA_SECTION:
      OBJECT_SET_RODATA(obj, array);
      OBJECT_SET_NRODATAS(obj, nsecs);
      break;
    case DATA_SECTION:
      OBJECT_SET_DATA(obj, array);
      OBJECT_SET_NDATAS(obj, nsecs);
      break;
    case TLSDATA_SECTION:
      OBJECT_SET_TLSDATA(obj, array);
      OBJECT_SET_NTLSDATAS(obj, nsecs);
      break;
    case NOTE_SECTION:
      OBJECT_SET_NOTE(obj, array);
      OBJECT_SET_NNOTES(obj, nsecs);
      break;
    case BSS_SECTION:
      OBJECT_SET_BSS(obj, array);
      OBJECT_SET_NBSSS(obj, nsecs);
      break;
    case TLSBSS_SECTION:
      OBJECT_SET_TLSBSS(obj, array);
      OBJECT_SET_NTLSBSSS(obj, nsecs);
      break;
    case DEBUG_SECTION:
      OBJECT_SET_DEBUG(obj, array);
      OBJECT_SET_NDEBUGS(obj, nsecs);
      break;
    case ATTRIB_SECTION:
      OBJECT_SET_ATTRIB(obj, array);
      OBJECT_SET_NATTRIBS(obj, nsecs);
      break;
    default:
      FATAL(("Unsupported type %c in SectionRemoveFromObject!", SECTION_TYPE(sec)));
  }

  /* Remove the section from the object's section table. Can't completely remove
   * its connection with the object (by setting SECTION_OBJECT to NULL) however,
   * as the object is its manager.
   */
  HashTableDelete (OBJECT_SECTION_TABLE(obj), sec);
}

void
SectionKill (const t_section * del)
{
  SectionRemoveFromObject(T_SECTION(del));

  while (SECTION_REFERS_TO(del))
  {
    RelocTableRemoveReloc (RELOC_TABLE(RELOC_REF_RELOC(SECTION_REFERS_TO(del))), RELOC_REF_RELOC(SECTION_REFERS_TO(del)));
  }

  SectionFree (del);
}


/* {{{ Mark some code region as data (so Diablo doesn't screw it up) */
/*#define VERBOSE_DATAIZING*/
void MarkRegionAsData(t_object *obj, t_const_string start_label, t_const_string end_label)
{
  t_symbol *start = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE (obj),start_label);
  t_symbol *end = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE (obj),end_label);
  t_symbol *first = SymbolTableGetFirstSymbolWithName(OBJECT_SUB_SYMBOL_TABLE (obj),"$first");
  t_bool data_before, data_after;
  t_address start_address = StackExecConst(SYMBOL_CODE(start), NULL, start, 0, obj);
  t_address end_address = StackExecConst(SYMBOL_CODE(end), NULL, end, 0, obj);
  t_symbol * sym;
  t_symbol * tmp;

  if (!AddressIsNull(start_address))
  {
    data_before = (ADDRESS_IS_DATA == 
                   SymbolTableGetDataType(obj,AddressSubUint32(start_address, 4)));
  }
  else
  {
    data_before = FALSE;
  }
  data_after = (ADDRESS_IS_DATA == 
      SymbolTableGetDataType(obj,end_address));

#ifdef VERBOSE_DATAIZING
  VERBOSE(0,("Dataizing region between @G and @G\n",start_address, end_address));
  VERBOSE(0,("data before: %c after: %c\n",data_before?'Y':'N',data_after?'Y':'N'));
#endif

  /* remove all existing switch symbols in the region between start and end */
  sym = (t_symbol*)SymbolTableLookup(OBJECT_SUB_SYMBOL_TABLE (obj),"$switch");
  for (tmp = (t_symbol *)SYMBOL_EQUAL(sym); sym; sym = tmp, tmp = sym ? (t_symbol *)SYMBOL_EQUAL(sym) : NULL)
  {
    t_address sym_address = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
    if (AddressIsGe(sym_address, start_address) &&
	AddressIsLe(sym_address, end_address))
    {
#ifdef VERBOSE_DATAIZING
      VERBOSE(0,("Removing switch symbol at @G\n", sym_address));
#endif
      SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE (obj), sym);
    }
  }

  /* add the appropriate switch symbols */
  if (!data_before)
  {
    SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                          start_address, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
     
#ifdef VERBOSE_DATAIZING
    VERBOSE(0,("Adding switch symbol before\n"));
#endif
  }
  if (!data_after)
  {
    SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                          end_address, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
#ifdef VERBOSE_DATAIZING
    VERBOSE(0,("Adding switch symbol after\n"));
#endif
  }

  /* sort the switch symbols */
  SymbolTableSortSymbolsByAddress(OBJECT_SUB_SYMBOL_TABLE (obj),"$switch");

  /* if we're dataizing from the start of the code, make sure the $first
   * symbol is set the right way */
  sym = (t_symbol *) SymbolTableLookup(OBJECT_SUB_SYMBOL_TABLE (obj),"$switch");
  if (AddressIsEq(start_address,StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj)))
  {
#ifdef VERBOSE_DATAIZING
    VERBOSE(0,("Resetting $first symbol\n"));
#endif
    SYMBOL_SET_OFFSET_FROM_START(first,  AddressNullForObject(obj));
  }

  /* do a lookup in the data and code table in order to reset the cached pointer.
   * we messed it up with our reordering of the $switch symbols */
  SymbolTableGetDataType(obj,AddressNullForObject(obj));

}
/* }}} */

t_address SectionGetDataAsAddress(const t_section* sec, t_address offset, t_address size)
{
   t_address _res;
   switch (G_T_UINT32(size))
   {
     case 8:
        _res = AddressNewForSection(sec, SectionGetData8 (sec, offset));
        break;
     case 16:
        _res = AddressNewForSection(sec, SectionGetData16 (sec, offset));
        break;
     case 32:
        _res = AddressNewForSection(sec, SectionGetData32 (sec, offset));
        break;
     case 64:
        _res = AddressNewForSection(sec, SectionGetData64 (sec, offset));
        break;
     default:
        FATAL(("Unsupported section data size: %d", size));
        break;
   }
   return _res;
}

void SectionSetDataAsAddress(t_section* sec, t_address offset, t_address val, t_address size)
{
   switch (G_T_UINT32(size))
   {
     case 8:
        SectionSetData8 (sec, offset, (t_uint8)AddressExtractUint32(val));
        break;
     case 16:
        SectionSetData16 (sec, offset, (t_uint16)AddressExtractUint32(val));
        break;
     case 32:
        SectionSetData32 (sec, offset, AddressExtractUint32(val));
        break;
     case 64:
        SectionSetData64 (sec, offset, AddressExtractUint64(val));
        break;
     default:
        FATAL(("Unsupported section data size: %d", size));
        break;
   }
}

t_section_group* SectionGroupNew(t_const_string signature)
{
  t_section_group* ret = Calloc(1, sizeof(t_section_group));
  ret->signature = StringDup(signature);
  return ret;
}

void SectionGroupAddSection(t_section_group *group, t_section *sec, t_bool alloc)
{
  int *nmembers;
  t_section ***members;

  if (alloc)
  {
    nmembers = &group->nallocmembers;
    members = &group->allocmembers;
  }
  else
  {
    nmembers = &group->nnoallocmembers;
    members = &group->noallocmembers;
  }
  (*nmembers)++;
  *members = Realloc(*members, (*nmembers) * sizeof(**members));
  (*members)[*nmembers-1] = sec;
  /* add support for a section belonging to more than one group
   * if necessary
   */
  ASSERT(SECTION_SECTION_GROUP(sec)==NULL,("IMPLEMENT: section @S belongs to more than one section group",sec));
  SECTION_SET_SECTION_GROUP(sec,group);
}

void SectionGroupFree(const t_section_group *group)
{
  Free(group->signature);
  if(group->allocmembers)
    Free(group->allocmembers);
  if(group->noallocmembers)
    Free(group->noallocmembers);
  Free(group);
}

t_section *MergeAllSubsections(t_section *main_section, t_string merged_section_name)
{
  /* sort all subsections by their old address, as this is not guaranteerd by Diablo */
  t_uint32 nr_subsections = 0;
  t_section **subsections = SectionGetSubsections(main_section, &nr_subsections);

  /* combined size of all subsections */
  t_address total_size = AddressNew32(0);
  for (t_uint32 i = 0; i < nr_subsections; i++)
    total_size = AddressAdd(total_size, SECTION_CSIZE(subsections[i]));

  /* create new section */
  t_section *merged_section = SectionCreateForObject(ObjectGetLinkerSubObject(SECTION_OBJECT(main_section)),
                                                     SECTION_TYPE(subsections[0]),
                                                     main_section, total_size,
                                                     merged_section_name);
  SECTION_SET_CADDRESS(merged_section, SECTION_CADDRESS(main_section));
  SECTION_SET_OLD_ADDRESS(merged_section, SECTION_OLD_ADDRESS(main_section));
  SECTION_SET_ALIGNMENT(merged_section, SECTION_ALIGNMENT(main_section));

  /* offset in the new section */
  t_address current_offset = AddressNew32(0);

  /* iterate over all subsections, migrating all relocations */
  for (t_uint32 i = 0; i < nr_subsections; i++)
  {
    t_section *subsection = subsections[i];

    /* don't support this for now */
    while (SECTION_REFED_BY(subsection))
      FATAL(("@T is refed by", subsection));

    /* all relocations for which the FROM has been set to this subsection */
    while (SECTION_REFERS_TO(subsection))
    {
      t_reloc *rel = RELOC_REF_RELOC(SECTION_REFERS_TO(subsection));
      RelocSetFrom(rel, T_RELOCATABLE(merged_section));
      RELOC_SET_FROM_OFFSET(rel, AddressAdd(current_offset, RELOC_FROM_OFFSET(rel)));
    }

    /* move all subsection data */
    memmove(((char *)SECTION_DATA(merged_section)) + G_T_UINT32(current_offset),
            (char *)SECTION_DATA(subsection),
            SECTION_CSIZE(subsection));

    /* the next subsection should be put after the newly added subsection */
    current_offset = AddressAdd(current_offset, SECTION_CSIZE(subsection));

    SectionKill(subsection);
  }

  subsections = SectionGetSubsections(main_section, &nr_subsections);

  return merged_section;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
