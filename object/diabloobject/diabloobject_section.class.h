/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS section
#define section_field_select_prefix SECTION
#define section_function_prefix Section
#define MANAGER_TYPE t_object *
#define MANAGER_NAME object
#define MANAGER_FIELD section_manager
#endif

/*! \brief Class used to represent the sections stored in a relocatable object 
 *
 * Data in object files is typically ordered in sections, blocks of data with
 * the same properties. This class provides Diablo's representation of
 * sections.  See also t_object and t_segment.
 * */
DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
MEMBER(t_hash_table_node, parent_t_hash_table_node, PARENT_T_HASH_TABLE_NODE)
MEMBER(t_object *, object, OBJECT)
MEMBER(t_address, load_address, LOAD_ADDRESS)
MEMBER(char, type, TYPE)
MEMBER(t_string, name, NAME)
MEMBER(t_bool, is_subsection, IS_SUBSECTION)
MEMBER(t_bool, is_mapped, IS_MAPPED)
MEMBER(t_bool, is_common_defined, IS_COMMON_DEFINED)
MEMBER(t_bool, is_vectorized, IS_VECTORIZED)
/* the index of the section in the object file from
 * which it was read (-1 if not available/applicable)
 */
MEMBER(t_int32, index_in_obj, INDEX_IN_OBJ)
MEMBER(t_section *, parent_section, PARENT_SECTION)
/* in case the section is part of a group and it's the
 * "main" section of the group (i.e., not a debug or
 * other non-execution-influencing section), keep track
 * of the other sections part of this group that need
 * to be removed along with this one
 */
MEMBER(t_section_group *, section_group, SECTION_GROUP)
MEMBER(t_uint32, nins, NINS)
MEMBER(void *, tmp_buf, TMP_BUF)
MEMBER(void *, data, DATA)
MEMBER(t_uint32, flags, FLAGS)
MEMBER(t_uint32, tmp, TMP)
MEMBER(t_section_callbacks *, callbacks, CALLBACKS)
MEMBER(void **, address_to_ins_map, ADDRESS_TO_INS_MAP)
MEMBER(t_section *, subsec_first, SUBSEC_FIRST)
MEMBER(t_section *, subsec_last, SUBSEC_LAST)
MEMBER(t_section *, subsec_prev, SUBSEC_PREV)
MEMBER(t_section *, subsec_next, SUBSEC_NEXT)
CONSTRUCTOR2(char, type, t_const_string, name,
             {
             ASSERT(name, ("Trying to create a section without a name. This is illegal."));
             SECTION_SET_NAME(ret, StringDup (name));

             SECTION_SET_OBJECT(ret, object);
             SECTION_SET_TYPE(ret, type);
             SECTION_SET_DATA(ret, NULL);
             SECTION_SET_TMP_BUF(ret, NULL);
             SECTION_SET_IS_SUBSECTION(ret, FALSE);
             SECTION_SET_IS_MAPPED(ret, FALSE);
             SECTION_SET_IS_COMMON_DEFINED(ret, FALSE);
             SECTION_SET_INDEX_IN_OBJ(ret, -1);

             SECTION_SET_CALLBACKS(ret, (t_section_callbacks*) Malloc (sizeof (t_section_callbacks)));

             SECTION_CALLBACKS(ret)->SectionRecalculateSize = NULL;
             SECTION_SET_RELOCATABLE_TYPE(ret, RT_SECTION);
             SECTION_SET_CADDRESS(ret, AddressNullForObject (object));
             SECTION_SET_OLD_ADDRESS(ret, AddressNullForObject (object));
             SECTION_SET_CSIZE(ret, AddressNullForObject (object));
             SECTION_SET_OLD_SIZE(ret, AddressNullForObject (object));
             SECTION_SET_ALIGNMENT(ret, AddressNullForObject (object));
             })

DUPLICATOR(
           {
           })

DESTRUCTOR({
           t_section *sub;
           t_section *safe;

           /* The section might already have been removed from the table, so we
            * try to look it up before deleting it.
            */
           t_hash_table* table = OBJECT_SECTION_TABLE(SECTION_OBJECT(to_free));
           if (table && HashTableIsPresent(table, to_free))
             HashTableDelete (table, to_free);

	   if (SECTION_REFED_BY(to_free)) FATAL(("Trying to free section still refered by reloc %s:%s", OBJECT_NAME(SECTION_OBJECT(to_free)), SECTION_NAME(to_free)));

           if (SECTION_TMP_BUF(to_free))
             Free (SECTION_TMP_BUF(to_free));
           if (SECTION_DATA(to_free))
             Free (SECTION_DATA(to_free));
           if (SECTION_NAME(to_free))
             Free (SECTION_NAME(to_free));
           Free (SECTION_CALLBACKS(to_free));
	   while (SECTION_REFED_BY_SYM(to_free))
	   {
	     SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(SECTION_REFED_BY_SYM(to_free)->sym), SECTION_REFED_BY_SYM(to_free)->sym);
	   }

           SECTION_FOREACH_SUBSECTION_SAFE (to_free, sub, safe)
           {
             SECTION_SET_PARENT_SECTION (sub, NULL);
             SECTION_SET_SUBSEC_PREV (sub, NULL);
             SECTION_SET_SUBSEC_NEXT (sub, NULL);
           }

           if (SECTION_PARENT_SECTION (to_free))
             SectionUnlinkFromParent (T_SECTION(to_free));
           })

DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS relocatable
#include <diabloobject_relocatable.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
