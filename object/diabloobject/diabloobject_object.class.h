/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS object
#define object_field_select_prefix OBJECT
#define object_function_prefix Object
#endif

/*!
 * \brief Class used to represent relocatable object files and executables 
 * 
 * Objects form the first layer of abstracton used in Diablo. Everything in
 * Diablo start with creating one or more objects. All supported objectfile
 * formats can be read in (the prefered constructor to do this is \ref
 * ObjectGet), transformed to a generic object (done automatically) and then
 * written back to an objectfile (use \ref ObjectWrite for this). This makes
 * it possible to apply the same analyses and optimizations to objects stored
 * in different objectfile formats. Currently the following objectfile
 * formats are in different stages of developement:
 *
 * - \ref ELF
 * - \ref ECOFF
 * - \ref PE
 *
 * Objects consist of the following information:
 *
 * - The program data and code, stored in sections (t_section)
 * - One or two symbol tables, that labels the information in these sections
 * (t_symbol)
 * - A reloc table, that describes the relationship between different
 * structures in the object (t_reloc)
 *
 *
 * This file holds the typedefs, types, function, and defines needed to
 * manipulate and investigate objects. */

DIABLO_CLASS_BEGIN
MEMBER(t_object *, equal, EQUAL)
MEMBER(t_object *, next, NEXT)
MEMBER(t_object *, prev, PREV)
MEMBER(t_object *, mapped_first, MAPPED_FIRST)
MEMBER(t_object *, mapped_last, MAPPED_LAST)
MEMBER(t_object *, parent, PARENT)
MEMBER(t_address_type, address_size, ADDRESS_SIZE)
MEMBER(t_hash_table *, subobject_cache, SUBOBJECT_CACHE)
MEMBER(t_bool, dynamic, DYNAMIC)
MEMBER(t_bool, library, LIBRARY)
MEMBER(t_bool, switched_endian, SWITCHED_ENDIAN)
MEMBER(t_uint32, streampos, STREAMPOS)
MEMBER(t_uint32, size, SIZE)
MEMBER(t_string, name, NAME)
MEMBER(t_address, entry, ENTRY)
MEMBER(t_uint32, ncodes, NCODES)
MEMBER(t_section **, code, CODE)
MEMBER(t_uint32, ndatas, NDATAS)
MEMBER(t_section **, data, DATA)
/*! The number of thread local data sections */
MEMBER(t_uint32, ntlsdatas, NTLSDATAS)
/*! The thread local data sections. Thread local storage data sections are handled differently from normal data
 * sections. Multiple copies of this section exist, and the address of this
 * section IS NOT FIXED! Furthermore they need their own segment.... */
MEMBER(t_section **, tlsdata, TLSDATA)
MEMBER(t_uint32, nrodatas, NRODATAS)
MEMBER(t_section **, rodata, RODATA)
MEMBER(t_uint32, nnotes, NNOTES)
MEMBER(t_section **, note, NOTE)
MEMBER(t_uint32, nbsss, NBSSS)
MEMBER(t_section **, bss, BSS)
/*! The number of thread local bss sections */
MEMBER(t_uint32, ntlsbsss, NTLSBSSS)
/*! The thread local bss sections. Thread local storage bss sections are handled differently from normal bss 
 * sections. Multiple copies of this section exist, and the address of this
 * section IS NOT FIXED!  Furthermore they need their own segment.... */
MEMBER(t_section **, tlsbss, TLSBSS)
MEMBER(t_uint32, ndebug, NDEBUGS)
MEMBER(t_section **, debug, DEBUG)
MEMBER(t_uint32, nattrib, NATTRIBS)
MEMBER(t_section **, attrib, ATTRIB)
MEMBER(t_segment *, segment_first, SEGMENT_FIRST)
MEMBER(t_segment *, segment_last, SEGMENT_LAST)
MEMBER(t_map *, map, MAP)
MEMBER(t_reloc_table *, reloc_table, RELOC_TABLE)
MEMBER(t_symbol_table *, symbol_table, SYMBOL_TABLE)
MEMBER(t_symbol_table *, dynamic_symbol_table, DYNAMIC_SYMBOL_TABLE)
MEMBER(t_symbol_table *, sub_symbol_table, SUB_SYMBOL_TABLE)
MEMBER(t_hash_table *, section_table, SECTION_TABLE)
MEMBER(t_address, gp, GP)
MEMBER(t_uint32, n_orig_codes, N_ORIG_CODES)
MEMBER(t_section_descriptor *, orig_codes, ORIG_CODES)
MEMBER(t_overlay *, overlays, OVERLAYS)
MEMBER(t_layout_script *, layout_script, LAYOUT_SCRIPT)
MEMBER(t_object_handler *, object_handler, OBJECT_HANDLER)
MEMBER(t_section *, undef_section, UNDEF_SECTION)
MEMBER(t_section *, abs_section, ABS_SECTION)
MEMBER(t_manager, cfg_manager, CFG_MANAGER)
MEMBER(t_manager, section_manager, SECTION_MANAGER)
MEMBER(char *, compressed_subobjects, COMPRESSED_SUBOBJECTS)
/*! flags read from the binary/library such that they be transferred to the rewritten binary */
MEMBER(t_uint32, gnu_stack_flags, GNU_STACK_FLAGS)
MEMBER(t_uint32, relro_caddress, RELRO_CADDRESS)
MEMBER(t_uint32, relro_csize, RELRO_CSIZE)
MEMBER(t_uint32, relro_newaddress, RELRO_NEW_ADDRESS)
MEMBER(t_uint32, relro_newsize, RELRO_NEW_SIZE)
MEMBER(t_bool,got_in_relro, GOT_IN_RELRO)
MEMBER(t_bool,compiler_generated,COMPILER_GENERATED)
/*! A pointer to the control flow graph. This is a void * because we cannot 
 * have dependencies on diabloflowgraph in diabloobject. diabloflowgraph will
 * define convenience macros OBJECT_{SET_}CFG() that do the casting for us */
MEMBER(void *, vcfg, VCFG)
/* ! Type of this (linked) object: executable, dynamic library, ... */
MEMBER(t_object_type, type, TYPE)
/* ! COMDAT section groups in this object: a group of sections
 * with a string signature, which behave like one common section
 * (if there are multiple COMDAT section groups with the same
 *  signature, only one is retained -- right now, we just mark
 *  all sections that are memberof such a group as common, so
 *  this is done automatically; later, we'll have to implement
 *  group-specific logic such as either keeping or discarding
 *  all sections in a group) */
MEMBER(t_uint32, ncomdat_section_groups, NCOMDAT_SECTION_GROUPS)
MEMBER(t_section_group **, comdat_section_groups, COMDAT_SECTION_GROUPS)

/* on some platforms, code symbol addresses must be masked before they are used
 * in relocations. E.g. on ARM, the lowest bit must be masked out. This
 * field must be set to the *inverse* of that mask (i.e. "0" means "nothing
 * gets masked -- which is the default)
 */
MEMBER(t_uint32, code_symbol_address_inverse_mask, CODE_SYMBOL_ADDRESS_INVERSE_MASK)

/*! Basic constructor for an object (t_object). You should probably never use this. Use
 * ObjectNewCached instead, as it initializes a number of structures that will
 * not be initialized by ObjectNew. 
 *
 * \return a new (zero-initialized) t_object * structure */
FUNCTION0 (t_CLASS *, New)
/*! Static initializer, that needs to be called before the first t_object is
 * created. It is called from within DiabloObjectInit() */
FUNCTION0 (void, Init)
/*! Static de-initializer, needs to be called once all t_object's are freed to
 * avoid memory leaks. It is called from within DiabloObjectFini() */
FUNCTION0 (void, Fini)
/*!
 * \param p1 The object to free
 *
 * Free a t_object structure (and all substructures) */
FUNCTION1 (void, Free, t_CLASS const *)
/*!
 * \param p1 an open filedescriptor to the objectfile
 * \param p2 an allocated t_object structure that will be filled with the information read
 * \param p3 A boolean that determines if debug information is loaded or not
 *
 * This function reads an objectfile from a stream
 *
 * \warning This is an internal function and should not be used directly, use \ref ObjectGet instead
 *
 * \todo Fix parameter order */
FUNCTION3 (void, ReadF, FILE *, t_CLASS *, t_bool)
/* \param p1 The filename of the object to read.
 * \param p2 A boolean that determines if debug information is loaded or not
 *
 * Read an object. The name should be the complete path to the object!
 *
 * \warning This is an internal function and should not be used directly, use \ref ObjectGet instead */
FUNCTION3 (t_CLASS *, Read, t_const_string, t_object *, t_bool)
/*!
 * \param p1 The name of the object to load (for object files in libraries use the format libraryname:objectname)
 * \param p2 The parent object in case we're loading a sub object
 * \param p3 A boolean that determines if debug information is loaded or not
 * \param p4 The name of the symbol/section being loaded. Used as a hint for the Microsoft toolchain.
 * \return the loaded object
 *
 * Default method to open an objectfile, has a built in cache to avoid loading an object 2 times. */
FUNCTION4 (t_CLASS *, Get, t_const_string, t_CLASS *, t_bool, t_const_string)

/*!
 * \param p1 The object in which we will look for a subobject
 * \param p2 The address of something we are looking for
 * \return The subobject that contains a section in which the address is found
 *
 * Function to retrieve the subobject in which some address is found, useful to
 * get the original object of some piece of data */
FUNCTION2 (t_CLASS *, GetSubobjectContainingAddress, t_CLASS const *, t_address)

     DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
