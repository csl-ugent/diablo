/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/** \file
 *
 * Sections are the containers used to hold data and code in object files.
 * This file holds the typedefs, types, function, and defines needed to handle
 * and manipulate them.
 */

/* Section Typedefs {{{ */
#ifndef DIABLOOBJECT_SECTION_TYPEDEFS
#define DIABLOOBJECT_SECTION_TYPEDEFS
typedef struct _t_section_callbacks t_section_callbacks;
typedef struct _t_section_group t_section_group;
#endif /* }}} Section Typedefs */
/* Section Defines {{{ */
#ifndef DIABLOOBJECT_SECTION_DEFINES
#define DIABLOOBJECT_SECTION_DEFINES
/*! A normal code section,
 * datasize= alignment for this section,
 * data = the raw data in this section,
 * tmp_buf = not used
 * tmp32 = TODO: the size of an instruction for this architecture
 */
#define CODE_SECTION 'C'
/*! Temporary state, busy disassembling,
 * datasize = size of instruction,
 * data = raw_data,
 * tmp_buf = disassemble buffer,
 * tmp32 = instruction struct size (should be set at the start of disassembling) */
#define DISASSEMBLING_CODE_SECTION 'd'
/*! A disassembled code section,
 * datasize = size of instruction struct,
 * data = linear list of instructions
 * tmp_buf = not used
 * tmp32 = not used */
#define DISASSEMBLED_CODE_SECTION 'I'
/*! Temporary state, busy flowgraphing,
 * datasize = size of instruction,
 * data = raw_data,
 * tmp_buf = disassemble buffer,
 * tmp32 = instruction struct size */
#define FLOWGRAPHING_CODE_SECTION 'f'
/*! A flowgraph for a section,
 * datasize = size of instruction struct,
 * data = Flowgraph information (entries, functions, blocks not in a function, graph)
 * tmp_buf = not used
 * tmp32 = not used */
#define FLOWGRAPHED_CODE_SECTION 'F'
/*! Temporary state, busy deflowgraphing,
 * datasize = TODO
 * data = TODO
 * tmp_buf = TODO
 * tmp32 = TODO */
#define DEFLOWGRAPHING_CODE_SECTION 'p'
/*! Temporary state, busy assembling, */
#define ASSEMBLING_CODE_SECTION 'a'

#define RODATA_SECTION 'R'
#define DATA_SECTION 'D'
#define TLSDATA_SECTION 't'
#define NOTE_SECTION 'N'
#define BSS_SECTION 'B'
#define TLSBSS_SECTION 'b'
#define DEBUG_SECTION 'G'
#define SPECIAL_SECTION 'S'
#define ATTRIB_SECTION 'A'

#define MAP_SECTION(sec,name,psec) do { if (SECTION_IS_MAPPED(sec)) FATAL(("Section is already mapped!")); SectionReparent (sec, psec); } while (0)
/*! Convert type to section */
#define T_SECTION(section) ((t_section *) (section))
/*! Get the parent object for this section */
/*! true if section is a datasection */
#define SECTION_IS_DATA(section) (section->type == DATA_SECTION || section->type == RODATA_SECTION || section->type == BSS_SECTION)
/*! true if section is a codesection */
#define SECTION_IS_CODE(section) (!SECTION_IS_DATA(section) && SECTION_TYPE(section) != NOTE_SECTION && SECTION_TYPE (section) != DEBUG_SECTION)
/*! true if the section is mapped to a parent object */
#define SET_SECTION_NINS(section, mnins) do { if (SECTION_IS_MAPPED(section)) FATAL(("Section is mapped!")); section->mapped_or_not.not_mapped.nins=mnins; } while(0)
#define SET_SECTION_TMP_BUF(section, tmp) do { if (SECTION_IS_MAPPED(section)) FATAL(("Section is mapped!")); section->mapped_or_not.not_mapped.tmp_buf=tmp; } while(0)
#define SET_SECTION_DATASIZE(section, size) do { if (SECTION_IS_MAPPED(section)) FATAL(("Section is mapped!")); section->mapped_or_not.not_mapped.data_size=size; } while(0)
#define SECTION_FLAG_KEEP 1 /* do not remove this section, even though it
                               appears unconnected in the graph */
#define SECTION_FLAG_LINKED_IN 2 /* this section was linked in after the parent
                                    object was read */
#define SECTION_FLAG_IN_OVERLAY 4 /* this section is part of an overlay */
#define SECTION_FLAG_EXPORT_SECTION 0x10
#define SECTION_FLAG_IMPORT_SECTION 0x20
#define SECTION_FLAG_DYNLINK_MASK 0xF0
#define SECTION_FLAG_EXIDX_NEEDS_RELAXATION 0x100 /* the linker map indicated that the section was relaxed (i.e., a can't unwind entry was added at the end */

#define AddressNullForSection(sec) AddressNullForObject(SECTION_OBJECT(sec))
#define AddressNewForSection(sec,a) AddressNewForObject(SECTION_OBJECT(sec),a)
#define SECTION_END_ADDRESS(section) AddressAdd(SECTION_CADDRESS(section),SECTION_CSIZE(section))
#define SECTION_OLD_END_ADDRESS(section) AddressAdd(SECTION_OLD_ADDRESS(section),SECTION_OLD_SIZE(section))
#endif /* }}} Section Defines */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_TYPES
/* Section Types {{{ */
#ifndef DIABLO_SECTION_TYPES
#define DIABLO_SECTION_TYPES
struct _t_section_callbacks
{
  t_uint64 (*SectionRecalculateSize) (t_section *, t_relocatable_address_type);
};

struct _t_section_group
{
  t_const_string signature;
  /* number of live alloc sections part of this group
   *
   * If there are no more live alloc sections, all sections part of this group
   * can be discarded
   */
  int nallocmembers;
  t_section **allocmembers;
  int nnoallocmembers;
  t_section **noallocmembers;
};

#endif /* }}} Section Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Section Functions {{{ */
#ifndef DIABLOOBJECT_SECTION_FUNCTIONS
#define DIABLOOBJECT_SECTION_FUNCTIONS
t_section *SectionCreateForObject (t_object *, char, t_section *, t_address, t_const_string);
t_section *SectionGetFromObjectByAddress (const t_object *, t_address);
t_section *SubsectionGetFromObjectByAddress (const t_object * obj, t_address address);
t_section *SectionGetFromObjectByName (const t_object *, t_const_string);
t_section *SectionGetFromObjectByIndex (const t_object *, t_int32);
void SectionLinkToParent(t_section* sec, t_section* parent);
void SectionUnlinkFromParent(t_section* sec);
void SectionReparent (t_section *, t_section *);
void SectionHashTableNodeFree (const void *, void *);
void SectionInsertInObject (t_section *, t_object* obj);
void SectionRemoveFromObject (t_section *);
void SectionKill (const t_section *);

#define SectionGetData64(sec,offset) RealSectionGetData64(__FILE__,__LINE__,sec,offset)
#define SectionGetData32Ignore(sec,offset,ignored) RealSectionGetData32(__FILE__,__LINE__,sec,offset,TRUE,ignored)
#define SectionGetData32(sec,offset) RealSectionGetData32(__FILE__,__LINE__,sec,offset,FALSE,NULL)
#define SectionGetData32By16bit(sec,offset) (((t_uint32)SectionGetData16(sec,offset) << 16) | SectionGetData16(sec,(offset)+2))
#define SectionGetData16(sec,offset) RealSectionGetData16(__FILE__,__LINE__,sec,offset)
#define SectionGetData8(sec,offset) RealSectionGetData8(__FILE__,__LINE__,sec,offset)
#define SectionGetULEB128(sec,offset,len) RealSectionGetULEB128(__FILE__,__LINE__,sec,offset,len)
#define SectionGetSLEB128(sec,offset,len) RealSectionGetSLEB128(__FILE__,__LINE__,sec,offset,len)
t_uint64 RealSectionGetData64 (t_const_string, int, const t_section *, t_address);
t_uint32 RealSectionGetData32 (t_const_string, int, const t_section *, t_address, t_bool ignore_out_of_section, t_bool * out_of_section_ignored);
t_uint16 RealSectionGetData16 (t_const_string, int, const t_section *, t_address);
t_uint8 RealSectionGetData8 (t_const_string, int, const t_section *, t_address);
t_int64 RealSectionGetSLEB128 (t_const_string, int, const t_section *, t_address, int*);
t_uint64 RealSectionGetULEB128 (t_const_string, int, const t_section *, t_address, int*);

/** Redefined as a function to keep the compiler happy */
t_address SectionGetDataAsAddress(const t_section* sec, t_address offset, t_address size);
void SectionSetDataAsAddress(t_section* sec, t_address offset, t_address val, t_address size);

#define SectionSetData64(sec,offset,val) RealSectionSetData64(__FILE__,__LINE__,sec,offset,val)
#define SectionSetData32(sec,offset,val) RealSectionSetData32(__FILE__,__LINE__,sec,offset,val)
#define SectionSetData16(sec,offset,val) RealSectionSetData16(__FILE__,__LINE__,sec,offset,val)
#define SectionSetData8(sec,offset,val) RealSectionSetData8(__FILE__,__LINE__,sec,offset,val)
void RealSectionSetData64 (t_const_string, int, t_section *, t_address, t_uint64 val);
void RealSectionSetData32 (t_const_string, int, t_section *, t_address, t_uint32 val);
void RealSectionSetData16 (t_const_string, int, t_section *, t_address, t_uint16 val);
void RealSectionSetData8 (t_const_string, int, t_section *, t_address, t_uint8 val);

t_section **SectionGetSubsections (const t_section * sec, t_uint32 * nsubs_ret);
t_section * ObjectNewSubsection(t_object * obj, t_address size, char section_type);

void MarkRegionAsData(t_object *obj,t_const_string start_label,t_const_string end_label);

t_section_group* SectionGroupNew(t_const_string signature);
void SectionGroupAddSection(t_section_group *group, t_section *sec, t_bool alloc);
void SectionGroupFree(const t_section_group *group);
t_section *MergeAllSubsections(t_section *main_section, t_string merged_section_name);
#endif /* }}} Section Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
