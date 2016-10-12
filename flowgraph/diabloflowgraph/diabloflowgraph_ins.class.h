/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h>
#include <diablosupport_logging.h>

#ifndef CLASS
#define CLASS ins
#define ins_field_select_prefix INS
#define ins_function_prefix Ins
#define MANAGER_TYPE t_cfg *
#define MANAGER_NAME cfg
#define MANAGER_FIELD ins_manager
#endif

#ifdef INS_NEXT
#undef INS_NEXT
#endif

#ifdef INS_PREV
#undef INS_PREV
#endif

/*! \brief This class is used to represent instructions.
 *
 * It extends t_relocatable, so it can be used as the target (or origin)
 * or relocations.
 * */
DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
/*! The cfg in which this instruction resides. Mind that instructions are
 * always stored in a cfg, even during disassembly and assembly */
MEMBER(t_cfg *, cfg, CFG)
/*! The next instruction in the list of all instructions in the cfg. Mind that
 * next was already taken, as the t_node class has a field next, and
 * t_relocatable extends t_node */
MEMBER(t_CLASS *, inext, INEXT)
/*! The previous instruction in the list of all instructions in the cfg. Mind
 * that prev was already taken, as the t_node class has a field prev, and
 * t_relocatable extends t_node */
MEMBER(t_CLASS *, iprev, IPREV)
/*! This field holds a pointer to a copy of this instruction. This is used by
 * some optimizations and during flowgraph construction */
MEMBER(t_CLASS *, copy, COPY)
/*! The registers that are defined by this instruction */
MEMBER(t_regset, regs_def, REGS_DEF)
/*! The registers that are used by this instruction */
MEMBER(t_regset, regs_use, REGS_USE)
/*! The section in which this instruction resides.
 *
 * \todo By incorporation field cfg in t_ins, this field is overhead. Replace
 * all used of this field by uses of the cfg field.*/
MEMBER(t_section *, section, SECTION)
/*! The attributes of the instruction
 *
 * \todo Add documentation on instruction attributes and a link to this
 * documentation */
MEMBER(t_instruction_flags, attrib, ATTRIB)
/*! The type of the instruction
 *
 * \todo Add documentation on instruction types and a link to this
 * documentation */
MEMBER(t_uint16, type, TYPE)
/*! The bbl in which this instruction resides */
MEMBER(t_bbl *, bbl, BBL)
/*! Temporary field
 *
 * \todo remove this field, use dynamic members instead 
 * \deprecated use dynamic members instead */
MEMBER(void *, tmp, TMP)
/*! The execution count of this instruction (according to a profile passed to
 * diablo) */
MEMBER(t_int64, exec_count, EXEC_COUNT)

/*Auxiliary data that will be set whenever an instruction is created in some phase of Diablo */
MEMBER(t_uint32, phase, PHASE)

/* Corresponding line and file in source code */
MEMBER(t_uint32, src_line, SRC_LINE)

/* Corresponding line and file in source code */
MEMBER(t_string, src_file, SRC_FILE)

/*! Basic function constructor.
 * \warning Don't use this function, use InsNewForBbl or InsNewForSec instead */
CONSTRUCTOR(
            {
              INS_SET_RELOCATABLE_TYPE(ret, RT_INS);
              INS_SET_PHASE(ret,GetDiabloPhase());
              INS_SET_SRC_FILE(ret,NULL);
              INS_SET_SRC_LINE(ret,0);
            }
           )

/*!
 * \brief Duplicate an instruction
 *
 * \param ins The instruction to duplicate
 *
 * \return t_ins * A duplicated instruction
 */

DUPLICATOR(
           {
             const t_architecture_description * desc;
             t_section * sec;
             t_object * obj;
             t_reloc_ref * refs = INS_REFERS_TO(to_dup);

             sec = INS_SECTION(to_dup);
             obj = SECTION_OBJECT(sec);
             if (SECTION_TYPE(sec) != FLOWGRAPHING_CODE_SECTION && SECTION_TYPE(sec) != FLOWGRAPHED_CODE_SECTION && SECTION_TYPE(sec) != DEFLOWGRAPHING_CODE_SECTION)
             {
               FATAL(("Cannot duplicate instructions in fase %c", SECTION_TYPE(sec)));}
               Free (ret); 
               ret = InsNewForSec (sec); 
               if (INS_BBL(to_dup)) 
                 desc = CFG_DESCRIPTION(BBL_CFG(INS_BBL(to_dup)));
               else
                 desc = CFG_DESCRIPTION(OBJECT_CFG(obj)); 
               memcpy (ret, to_dup, desc->decoded_instruction_size); 
               INS_SET_OLD_ADDRESS(ret, AddressNullForIns (to_dup)); 
               INS_SET_CADDRESS(ret, AddressNullForIns (to_dup)); 
               INS_SET_OLD_SIZE(ret, AddressNullForIns (to_dup)); 
               INS_SET_CSIZE(ret, INS_CSIZE(to_dup)); 
               INS_SET_REFED_BY(ret, NULL); 
               INS_SET_IPREV(ret, NULL); 
               INS_SET_INEXT(ret, NULL); 
               INS_SET_REFERS_TO(ret, NULL); 
               INS_SET_REFED_BY(ret, NULL); 
               INS_SET_REFED_BY_SYM(ret, NULL); 
               INS_SET_COPY(to_dup, ret); 
               INS_SET_EXEC_COUNT(ret, INS_EXEC_COUNT(to_dup)); 
               INS_SET_PHASE(ret,GetDiabloPhase());
               while (refs)
               { /* Duplicate the reference */
                 t_reloc * copy = RelocTableDupReloc (OBJECT_RELOC_TABLE(obj), RELOC_REF_RELOC(refs)); 
                 ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(copy)) == RT_INS, ("Illegal reference with instruction @I: @R!", to_dup, copy)); 
                 RelocSetFrom (copy, T_RELOCATABLE(ret)); refs = RELOC_REF_NEXT(refs);
               }
               if (desc->InsDupDynamic) 
                 desc->InsDupDynamic (ret, to_dup);
           }
          )

/*! Basic instruction destructor.
 *
 * \warning This function is too basic for most uses. Use InsKill instead. */
DESTRUCTOR(
           {
             if (CFG_DESCRIPTION(cfg)->InsCleanup) CFG_DESCRIPTION(cfg)->InsCleanup ((t_CLASS *)to_free); InsFreeReferedRelocs ((t_CLASS *)to_free);
           }
          )
/*!
 * \param p1 The instruction to kill
 *
 * InsKill unlinks the instruction and then calls InsFree
 */
FUNCTION1 (void, Kill, t_CLASS *)
/*! \brief Utility function.
 * \warning Do not use unless you know what you're doing. */
FUNCTION1 (void, KillDelayed, t_CLASS *)
/*! \brief Find next instruction in layout chain */
FUNCTION1 (t_CLASS *, NextInChain, t_CLASS *)
/*! \brief Find previous instruction in layout chain */
FUNCTION1 (t_CLASS *, PrevInChain, t_CLASS *)
/*! \brief allocate a new instruction
 *
 * \param p1 The bbl for which we create an instruction
 *
 * \return A new (void -casted) instruction
 *
 * The instruction is inserted in the control flow graph
 * that contains the basic block, but it is not inserted in
 * the basic block itself! For this use either t_ins::InsAppendToBbl
 * or t_ins::InsPrependToBbl or t_ins::InsInsertBefore or t_ins::InsInsertAfter
 * 
 */
FUNCTION1 (t_CLASS *, NewForBbl, t_bbl *)
/*! \brief allocate a new instruction
 * \param p1 The cfg for which we create an instruction
 * 
 * \return A new (void -casted) instruction
 *
 * You still have to insert the instruction into a basic block
 * after it is created. See t_ins::InsInsertBefore or t_ins::InsInsertAfter
 * or t_ins::InsAppendToBbl or t_ins::InsPrependToBbl.
 */
FUNCTION1 (t_CLASS *, NewForCfg, t_cfg *)
/*! \brief allocate a new instruction
 *
 * \param p1 The section for which we create an instruction
 * \return A new (void -casted) instruction
 *
 * \todo Add buffer overrun check
 * \todo If we can garantee that no pointers are created we could reallocate
 * the buffers used for storing instructions (much more efficient)
 */
FUNCTION1 (t_CLASS *, NewForSec, t_section *)
/*! \brief Swap two instructions in a bbl.
 * \param p1 The first instruction 
 * \param p2 The second instruction 
 * \warning The instructions should be in the same basic block! */
FUNCTION2 (void, Swap, t_CLASS *, t_CLASS *)
/*! \brief Append an instruction at the end of a bbl 
 * \param p1 The instruction to append
 * \param p2 The basic block to append it to */
FUNCTION2 (void, AppendToBbl, t_CLASS *, t_bbl *)
/*! \brief Prepend an instruction at the front of a bbl 
 * \param p1 The instruction to prepend
 * \param p2 The basic block to prepend it to */
FUNCTION2 (void, PrependToBbl, t_CLASS *, t_bbl *)
/*! \brief Insert an instruction after another instruction 
 * \param p1 The instruction to insert
 * \param p2 The instruction after which the insert will happen
 * \warning This will only work after the control flow graph is constructed */
FUNCTION2 (void, InsertAfter, t_CLASS *, t_CLASS *)
/*! \brief Insert an instruction before another instruction 
 * \param p1 The instruction to insert
 * \param p2 The instruction before which the insert will happen 
 * \warning This will only work after the control flow graph is constructed */
FUNCTION2 (void, InsertBefore, t_CLASS *, t_CLASS *)
/*! \brief Compute the registers live after an instruction
 * \param p1 The instruction after which liveness information is computed
 * \return t_regset The live registers */
FUNCTION1 (t_regset, RegsLiveAfter, t_CLASS *)
/*! \brief Compute the registers live before an instruction
 * \param p1 The instruction before which liveness information is computed
 * \return t_regset The live registers */
FUNCTION1 (t_regset, RegsLiveBefore, t_CLASS *)
/*! \brief Utility function.
 * \warning Do not use unless you know what you're doing. */
FUNCTION1 (void, FreeReferedRelocsDelayed, t_CLASS *)
/*! \brief Utility function.
 * \warning Do not use unless you know what you're doing. */
FUNCTION1 (void, FreeReferedRelocs, t_CLASS *)
/*! \brief Make a noop instruction
 *
 * Architecture-independent wrapper around the architecture-specific callbacks
 * for InsMakeNoop
 * \param p1 The instruction to turn into a noop */
PFUNCTION1 (void, MakeNoop, t_CLASS *)
/*!
 * \brief Check if the instruction has a side effect
 *
 * Architecture-independent wrapper around the architecture specific callbacks
 * for InsHasSideEffect
 *
 * \param p1 The instruction to check
 *
 * \return t_bool TRUE if the instruction has a side effect, FALSE otherwise
 */
PFUNCTION1 (t_bool, HasSideEffect, t_CLASS *)
/*! \brief Utility function.
 * \warning Do not use unless you know what you're doing. */
FUNCTION1 (void, FreeAllReferedRelocsBase, t_CLASS *)
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS relocatable
#include <diabloobject_relocatable.class.h>
#undef BASECLASS

#define INS_NEXT(x) ({ FATAL(("Do not use INS_NEXT: Use INS_INEXT instead")); NULL; })
#define INS_PREV(x) ({ FATAL(("Do not use INS_PREV: Use INS_IPREV instead")); NULL; })
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
