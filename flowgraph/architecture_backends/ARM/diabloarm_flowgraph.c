/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/* Options {{{ */
#define ARMFG_REMOVE_PC24RELOCS
#define ARMFG_OPTIMIZE_WEAK_CALLS
/*#define ARMFG_ASSUME_SWI_ADS*/
#define KEEP_INFO
t_int32 pcgreatness;
/* }}} */
/* Static function declarations {{{ */
static t_bbl * find_close_bbl_by_address(t_bbl * bbl_start, t_address addr);
static t_arm_ins * ArmFindConditionalInstructionThatDefinesRegisterInBbl(t_arm_ins * start, t_regset* reg, t_arm_condition_code cn);
static t_arm_ins * ArmFindPCRelLdrThatDefinesRegisterInBbl(t_arm_ins * start, t_regset* reg);
static t_bool ArmFindLoadImmThatDefinesRegisterInBbl(t_arm_ins * start, t_reg reg, t_uint32 * immvalue);
static t_uint32 ArmPatchCallsToWeakSymbols(t_object * obj);
static t_uint32 ArmFindBBLLeaders(t_object * obj);
static t_uint32 ArmAddBasicBlockEdges(t_object *obj);
static void ArmOptimizeSwitches(t_cfg *cfg);
/* }}} */

/*!
 * Removes the relocs from branches, as they are not strictly necessary and
 * tend to complicate the rest of the flowgraph construction
 *
 * \param obj A disassembled object
 *
 * \return t_uint32 the number of relocs removed
*/
/* ArmRemovePC24Relocs {{{ */
t_uint32 ArmRemovePC24Relocs(t_object * obj)
{
  t_reloc * rel, *tmp;
  t_uint32 ret=0;
  OBJECT_FOREACH_RELOC_SAFE(obj,rel,tmp)
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_INS)
      if (ARM_INS_TYPE(T_ARM_INS(RELOC_FROM(rel))) == IT_BRANCH)
      {
	RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),rel);
	ret++;
      }
  return ret;
}
/* }}} */

/*!
 *  code in RVCT __init_cpu:
 *   db8c:       e3a07f91        mov     r7, #580        ; 0x244
 *   db90:       e1a0600f        mov     r6, pc
 *   db94:       e1560007        cmp     r6, r7
 *   db98:       ba000005        blt     dbb4 <__init_cpu+0x28>
 *   db9c:       e28f5014        add     r5, pc, #20     ; 0x14
 *   dba0:       e3a06000        mov     r6, #0  ; 0x0
 *   dba4:       e4954004        ldr     r4, [r5], #4
 *   dba8:       e4864004        str     r4, [r6], #4
 *   dbac:       e2577004        subs    r7, r7, #4      ; 0x4
 *   dbb0:       1afffffb        bne     dba4 <__init_cpu+0x18>
 *
 * This code copies a bunch of other code to address 0. The
 * problem is that this code is not stored as data, but as plain
 * code (so Diablo will optimise and reorganise it). So duplicate
 * all code that has to be copied as data, and copy that instead.
 *
 * \param sec    Section containing all involved code (copying and copied)
 * \param pc_mov The instruction moving pc into another register
 * \param pc_use The instruction using that other register
 *
 * HandleCodeCopyLoop {{{ */
static
t_bool HandleCodeCopyLoop(t_section *sec, t_arm_ins *pc_mov, t_arm_ins *pc_use)
{
  t_arm_ins *prev, *addr_load, *iter, *first_copied_ins, *last_copied_ins;
  t_reg count_reg;
  t_uint32 copied_code_size;
  t_address copied_code_start, copied_code_end;
  t_bool found_load, found_counter_dec, found_back_branch;

  if (ARM_INS_OPCODE(pc_use) != ARM_CMP)
  {
    VERBOSE(2,("Could not find use and not a compare"));
    return FALSE;
  }

  if (ARM_INS_FLAGS(pc_use) & FL_THUMB)
  {
    VERBOSE(2,("The code copy loop detection only works for ARM, not for THUMB")
);
    return FALSE;
  }

  /* find the definition of the register which does not contain
   * the pc (= size of code) in prev
   * {{{*/
  if (ARM_INS_REGB(pc_use) == ARM_INS_REGA(pc_mov))
    count_reg = ARM_INS_REGC(pc_use);
  else
    count_reg = ARM_INS_REGB(pc_use);
  prev = ARM_INS_IPREV(pc_use);
  while(prev)
  {
    if ( RegsetIn(ARM_INS_REGS_DEF(prev),count_reg)) break;
    prev=ARM_INS_IPREV(prev);
  }
  if (!prev ||
      (ARM_INS_OPCODE(prev) != ARM_MOV) ||
      !(ARM_INS_FLAGS(prev) & FL_IMMED))
  {
    VERBOSE(2,("Load of code copy loop count register not with a constant"));
    return FALSE;
  }
  /*}}}*/
  copied_code_size = ARM_INS_IMMEDIATE(prev);

  /* check for the blt {{{*/
  iter=ARM_INS_INEXT(pc_use);
  if (!iter ||
      (ARM_INS_OPCODE(iter) != ARM_B) ||
      (ARM_INS_CONDITION(iter) == ARM_CONDITION_AL))
  {
    VERBOSE(2,("No conditional branch after code size compare @I",pc_use));
    return FALSE;
  }
  /*}}}*/

  /* next comes the loop header
   * it should contain a definition of register with a
   * pc-relative address: the source of the copy
   * {{{*/
  iter = ARM_INS_INEXT(iter);
  /* also stop when we reach a load/store, that means we've entered the loop
   * already
   */
  while (iter &&
         (((ARM_INS_OPCODE(iter) != ARM_ADD) &&
           (ARM_INS_OPCODE(iter) != ARM_SUB)) ||
          (ARM_INS_REGB(iter) != ARM_REG_R15) ||
          !(ARM_INS_FLAGS(iter) & FL_IMMED)) &&
         (ARM_INS_OPCODE(iter) != ARM_LDR) &&
         (ARM_INS_OPCODE(iter) != ARM_STR))
  {
    iter = ARM_INS_INEXT(iter);
  }

  if (!iter ||
      ((ARM_INS_OPCODE(iter) != ARM_ADD) &&
       (ARM_INS_OPCODE(iter) != ARM_SUB)))
  {
    VERBOSE(2,("Could not find instruction calculating start of to be copied code region in code copy loop header"));
    return FALSE;
  }
  addr_load = iter;
  /*}}}*/
  if (ARM_INS_OPCODE(addr_load) == ARM_ADD)
    copied_code_start = AddressAddInt32(ARM_INS_CADDRESS(addr_load),ARM_INS_IMMEDIATE(addr_load)+(ARM_INS_FLAGS(addr_load) & FL_THUMB)?4:8);
  else
    copied_code_start = AddressSubInt32(ARM_INS_CADDRESS(addr_load),ARM_INS_IMMEDIATE(addr_load)+(ARM_INS_FLAGS(addr_load) & FL_THUMB)?4:8);

  /* and next, the loop proper. Just perform some sanity checks:
   *  - there must be a load with writeback from the register
   *    defined by addr load (source address0
   *  - there must be a sub #4 from the register used in the compare
   *    (size of the code)
   *  - it has to end with a backwards jump
   * {{{*/
  found_load = FALSE;
  found_counter_dec = FALSE;
  found_back_branch = FALSE;
  iter = ARM_INS_INEXT(addr_load);
  while (iter)
  {
    if ((ARM_INS_OPCODE(iter) == ARM_LDR) &&
	ArmInsWriteBackHappens(iter) &&
	(ARM_INS_FLAGS(iter) & FL_IMMED) &&
	(ARM_INS_IMMEDIATE(iter) == 4) &&
	(ARM_INS_REGB(iter) == ARM_INS_REGA(addr_load)))
      found_load = TRUE;
    if ((ARM_INS_OPCODE(iter) == ARM_SUB) &&
	((ARM_INS_FLAGS(iter) & (FL_S | FL_IMMED)) == (FL_S | FL_IMMED)) &&
	(ARM_INS_IMMEDIATE(iter) == 4) &&
	(ARM_INS_REGA(iter) == count_reg) &&
	(ARM_INS_REGB(iter) == count_reg))
      found_counter_dec = TRUE;
    if ((ARM_INS_OPCODE(iter) == ARM_B) &&
	(ARM_INS_CONDITION(iter) != ARM_CONDITION_AL) &&
	(ARM_INS_IMMEDIATE(iter) < 0) &&
        AddressIsGt(AddressAddInt32(ARM_INS_CADDRESS(iter),ARM_INS_IMMEDIATE(iter)+(ARM_INS_FLAGS(iter) & FL_THUMB)?4:8),ARM_INS_CADDRESS(addr_load)))
      {
        found_back_branch = TRUE;
      }
    if (ARM_INS_TYPE(iter) == IT_BRANCH)
      break;
    iter = ARM_INS_INEXT(iter);
  }
  if (!found_load)
  {
    VERBOSE(2,("Did not find appropriate load in code copy loop after @I",addr_load));
    return FALSE;
  }

  if (!found_counter_dec)
  {
    VERBOSE(2, ("Did not find decrement of counter reg in code copy loop after @I",addr_load));
    return FALSE;
  }

  if (!found_back_branch)
  {
    VERBOSE(2, ("Did not find appropriate backwards branch in code copy loop after @I",addr_load));
    return FALSE;
  }
  /*}}}*/

  /* find the start and end of the copied code, and ascertain that there are
   * instructions to be copied
   * {{{*/
  copied_code_end = AddressAddUint32(copied_code_start,copied_code_size);
  first_copied_ins = T_ARM_INS(SecGetInsByAddress(sec,copied_code_start));
  if (!first_copied_ins ||
      !AddressIsEq(ARM_INS_OLD_ADDRESS(first_copied_ins),copied_code_start))
  {
    VERBOSE(2,("Could not find ins at @G in section @T to copy code",copied_code_start,sec));
    return FALSE;
  }

  last_copied_ins = T_ARM_INS(SecGetInsByAddress(sec,AddressSubInt32(copied_code_end,4)));
  if (!last_copied_ins ||
      !AddressIsEq(ARM_INS_OLD_ADDRESS(last_copied_ins),AddressSubInt32(copied_code_end,4)))
  {
     VERBOSE(2,("Cannot find instruction at @G in section @T as the end of the to be copied region",AddressAddInt32(copied_code_start,copied_code_size-4),sec));
     return FALSE;
  }
  /*}}}*/

  /* Everything is fine, now convert all instructions into data {{{*/
  {
    t_arm_ins *stop_ins;

    stop_ins = ARM_INS_INEXT(last_copied_ins);

    VERBOSE(0,("  Marking code from @G to @G as data",copied_code_start,AddressSubInt32(copied_code_end,1)));
    for (iter = first_copied_ins; iter != stop_ins; iter = ARM_INS_INEXT(iter))
    {
      t_uint32 tmp;

      ArmAssembleOne(iter, (char *)&tmp);
      ArmInsMakeData(iter, tmp);
    }
  }
  /*}}}*/

 /* success! */
 return TRUE;
}
/*}}}*/


/*!
 * Walks over all code sections and looks for potential starts of code
 * copying a code region. If it finds one, it hands it over to a
 * specialised handler which will convert the copied code into data
 *
 * ChangeCopiedCodeToData{{{*/
void ChangeCopiedCodeToData(t_object *obj)
{
  t_section *sec;
  t_arm_ins *ins, *next_ins;
  t_uint32 i;

  STATUS(START,("Converting copied code regions into data"));

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    SECTION_FOREACH_ARM_INS(sec, ins)
    {
       /* look for the following pattern:
        *    mov  rX, pc
        *    ... (no use/def of rX)
        *    cmp  rX, rY / cmp  rY, rX
        */
       if ((ARM_INS_OPCODE(ins) != ARM_MOV) ||
	   (ARM_INS_REGC(ins) != ARM_REG_R15))
	 continue;
       /* found a "mov rX, pc", look for the cmp */
       next_ins = ARM_INS_INEXT(ins);
       while (next_ins &&
              (ARM_INS_TYPE(ins) != IT_BRANCH) &&
              !RegsetIn(ARM_INS_REGS_USE(next_ins),ARM_INS_REGA(ins)) &&
              !RegsetIn(ARM_INS_REGS_DEF(next_ins),ARM_INS_REGA(ins)))
         next_ins = ARM_INS_INEXT(next_ins);
       /* if we also found the cmp, perform the rest of the checks and possible
        * transformation in HandleCodeCopyLoop()
        */
       if (next_ins &&
           (ARM_INS_OPCODE(next_ins) == ARM_CMP) &&
           ((ARM_INS_REGB(next_ins) == ARM_INS_REGA(ins)) ||
            (ARM_INS_REGC(next_ins) == ARM_INS_REGA(ins))) &&
           !(ARM_INS_FLAGS(next_ins) & FL_IMMED))
         HandleCodeCopyLoop(sec,ins,next_ins);
    }
  }
  STATUS(STOP,("Converting copied code regions into data"));
}
/*}}}*/

/*!
 * Probably the most important function in the backend: Create a flowgraph from
 * a list of disassembled arm instructions. Works in 3 big steps:
 * - Does leader detection on the list of instructions to identify basic blocks
 * - Converts all position-dependent instructions to pseudo instructions
 * - Draws edges between the basic blocks
 *
 * \todo Needs only one parameter
 * 
 * \param obj
 * \param section The (disassembled) section to flowgraph
 *
 * \return void 
*/
/* ArmFlowgraph {{{ */
void ArmFlowgraph(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);
  OBJECT_SET_ENTRY(obj, AddressAnd(OBJECT_ENTRY(obj),AddressNot(AddressNewForObject(obj,1))));

#ifdef ARMFG_REMOVE_PC24RELOCS
  /* Remove the PC24 relocs as they aren't strictly necessary and cause a 
   * great deal of trouble afterwards */
  STATUS(START,("Removing PC24 relocs from branches"));
  ArmRemovePC24Relocs(obj);
  STATUS(STOP,("Removing PC24 relocs from branches"));
#endif

#ifdef ARMFG_OPTIMIZE_WEAK_CALLS
  /* Remove the calls created by weak symbols */
  STATUS(START,("Patch calls to weak symbols")); 
  ArmPatchCallsToWeakSymbols(obj);
  STATUS(STOP,("Patch calls to weak symbols"));
#endif

  /* Find the leaders in the instruction list */
  STATUS(START,("Leader detection"));
  ArmFindBBLLeaders(obj);
  STATUS(STOP,("Leader detection"));

  /* Create the basic blocks (platform independent) */
  STATUS(START,("Creating Basic Blocks"));
  CfgCreateBasicBlocks(obj);
  STATUS(STOP,("Creating Basic Blocks"));

  STATUS(START,("Detecting switch tables"));
  ArmOptimizeSwitches(cfg);
  STATUS(STOP,("Detecting switch tables"));
 
  /* Create the edges between basic blocks */
  STATUS(START,("Creating Basic Block graph"));
  ArmAddBasicBlockEdges(obj);
  STATUS(STOP,("Creating Basic Block graph"));

  /* Align the basic blocks (ARM-spcific) */
  STATUS(START,("Aligning special Basic Blocks"));
  ArmAlignSpecialBasicBlocks(cfg);
  STATUS(STOP,("Aligning special Basic Blocks"));

  /*! Load store forwarding. Removes unnecessary loads/stores */
  DiabloBrokerCallInstall("ArmPeepholeOptimizations", "const t_cfg *" , ArmPeepholeOptimizations,TRUE, cfg);
  DiabloBrokerCallInstall("ArmReviveFromThumbStubs", "const t_cfg *", ArmReviveFromThumbStubs, TRUE, cfg);
}
/* }}} */


/*!
 * Split bbl at ins_split into two new bbl's.
 *
 * Does not really need the bbl parameter, but having
 * it makes things clearer.
 *
 * \param bbl
 * \param ins_split
 *
 * \return t_bbl the new bbl (last part of the old bbl, starting with ins_split
 */
/* BblSplitAtIns {{{*/
t_bbl *
BblSplitAtIns(t_bbl *bbl, t_ins *ins_split)
{
  t_bbl *tmp;
  t_cfg *cfg;
  t_ins *first;
  t_ins *iter;

  cfg = BBL_CFG(bbl);
  first = INS_COPY(ins_split);

  ASSERT(bbl == INS_BBL(ins_split),("Cannot split a bbl at an instruction not part of it"));

  /* Allocate a new block, gets appended to the list of basic blocks */
  tmp = BblNew(cfg);
  /* Remove it from the list of basic blocks */
  CFG_SET_NODE_LAST(cfg,  BBL_PREV(tmp));
  BBL_SET_NEXT(CFG_NODE_LAST(cfg), NULL);

  /* Insert it after the block we want to split */
  BBL_SET_PREV(tmp,  bbl);
  BBL_SET_NEXT(tmp,  BBL_NEXT(bbl));
  if (BBL_NEXT(tmp)) 
    BBL_SET_PREV(BBL_NEXT(tmp),  tmp);
  BBL_SET_NEXT(bbl,  tmp);
  /* Set the address */
  BBL_SET_OLD_ADDRESS(tmp, INS_OLD_ADDRESS(ins_split));
  BBL_SET_CADDRESS(tmp, INS_CADDRESS(ins_split));

  /* Remove the instruction and all its successors from the
   * original basic block and move the list of instructions to the
   * new basic block  */
  INS_SET_INEXT(INS_IPREV(first), NULL);
  BBL_SET_INS_LAST(tmp,  BBL_INS_LAST(bbl));
  BBL_SET_INS_LAST(bbl,  INS_IPREV(first));
  INS_SET_IPREV(first, NULL);
  BBL_SET_INS_FIRST(tmp, T_INS(first));


  /* Set the bbl field in the moved instructions and update the
   * instruction count */
  BBL_FOREACH_INS(tmp,iter)
  {
    INS_SET_BBL(iter,  tmp);
    BBL_SET_NINS(tmp,  BBL_NINS(tmp)+1);
    BBL_SET_NINS(bbl,  BBL_NINS(bbl)-1);
    BBL_SET_CSIZE(bbl, AddressSub(BBL_CSIZE(bbl),INS_CSIZE(iter)));
    BBL_SET_CSIZE(tmp, AddressAdd(BBL_CSIZE(tmp),INS_CSIZE(iter)));

  }

  /* Do the same for the original instructions */ 
  iter=ins_split;
  while(!(INS_ATTRIB(iter) & IF_BBL_LEADER))
  {
    INS_SET_BBL(iter,  tmp);
    iter=INS_INEXT(iter);
  }

  INS_SET_ATTRIB(ins_split,   INS_ATTRIB(ins_split)|IF_BBL_LEADER);

  return tmp;
}
/*}}}*/


/*!
 * Get the value loaded by a PC-relative ldr/ldrh
 * \param ins
 * \param ret_address if non-NULL, will return the address at which the loaded value resides
 * \param rel if non-NULL, will return the relocation, if any, associated with the loaded value
 * \param failed must be non-NULL, returns TRUE if we failed to load the value for some reason
 *
 * \return the loaded value
 */
/* GetPcRelLoadedValue {{{*/
static t_uint64
GetPcRelLoadedValue(t_object *obj, t_arm_ins *ins, t_address *ret_address, t_reloc **rel, t_bool *failed)
{
  t_address load_address;
  t_address ins_address;
  t_bbl *bbl2;
  t_arm_ins *iter;
  t_uint64 value = 0;

  *failed = FALSE;

  /* align address to 4 byte boundary, as that is what the ldr will use */
  ins_address = AddressInverseMaskUint32(ARM_INS_CADDRESS(ins),3);

  /* Calculate the address from where we are loading */
  /* Calculate the load address + 2*ins_size (pc=(pc+2*inssize) & ~3) */
  load_address=AddressAnd(AddressAdd(ins_address,AddressNew32(((ARM_INS_FLAGS(ins) & FL_THUMB)?4:8))),AddressNew32(~3));
  if (!(ARM_INS_FLAGS(ins) & FL_DIRUP))
  {
    /* Calculate the load address + 2*ins_size (pc=(pc+2*ins_size) & ~3) */
    load_address=AddressSub(load_address,ARM_INS_IMMEDIATE(ins));
  }
  else
  {
    load_address=AddressAdd(load_address,ARM_INS_IMMEDIATE(ins));
  }

  /* Perform  the load, and get the relocation at that address */
  bbl2=find_close_bbl_by_address(ARM_INS_BBL(ins),load_address);
  if (!bbl2)
  {
    VERBOSE(5,("BBL NOT FOUND!"));
    *failed = TRUE;
    return (t_uint64)-1;
  }
  BBL_FOREACH_ARM_INS(bbl2,iter)
  {
    if (AddressIsEq(ARM_INS_CADDRESS(iter),load_address))
      break;
  }
  if (!iter)
  {
    VERBOSE(5,("No ins found!"));
    *failed = TRUE;
    return (t_uint64)-1;
  }

  if (ARM_INS_TYPE(iter)!=IT_DATA) {
  {
    VERBOSE(5,("Ins is not data!\nBbl: @eiB\nPCRel Load Ins: @I\nCaculated Addres: %x",
                bbl2, ins, load_address));
    *failed = TRUE;
    return (t_uint32)-1;
  }
  }

  if (ARM_INS_OPCODE(ins)==ARM_LDRD)
  {
    ASSERT(ARM_INS_INEXT(iter), ("expected next data instruction after @I for @I, but found none", iter, ins));

    value = ARM_INS_IMMEDIATE(ARM_INS_INEXT(iter));
    value <<= 32;
  }

  value|=ARM_INS_IMMEDIATE(iter);
  if (ARM_INS_OPCODE(ins)==ARM_LDRH) 
  {
    value &=0xffff;
    if (ARM_INS_REFERS_TO(iter))
      FATAL(("loading relocatable halfword !!!"));
  }

  if (ret_address)
  {
    *ret_address = load_address;
  }

  if (rel)
  {
    if (ARM_INS_REFERS_TO(iter))
      *rel=RELOC_REF_RELOC(ARM_INS_REFERS_TO(iter));
    else
      *rel=NULL;
  }
  return value;
}
/*}}}*/

/*!
 * Get the previous instruction for a jump table construct
 * Skips any intermediate data blocks.
 * \param ins
 *
 * \return Previous instruction if it would be in the same bbl in case case there were no databbl, otherwise NULL
 */
/* ArmGetPrevIns {{{*/
static t_arm_ins *
ArmGetPrevIns(t_object *obj, t_arm_ins *ins)
{
  t_arm_ins *prev, *jump_target;
  t_bbl *bbl;
  t_address target_address;

  /* normal case (depending on when we call this, ARM_INS_IPREV may
   * already wander into previous bbl's or not) */
  prev = ARM_INS_IPREV(ins);
  if (prev &&
      (ARM_INS_OPCODE(prev) != ARM_DATA))
    return prev;

  /* check if we are preceded by a data bbl (BBL_IS_DATA attrib is not
   * necessarily already set at this point) */
  bbl = BBL_PREV(ARM_INS_BBL(ins));
  if (!bbl ||
      (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) != ARM_DATA))
    return NULL;  

  /* skip the databbl */
  bbl = BBL_PREV(bbl);

  /* does it end in an unconditional branch to the bbl after the data bbl? */
  prev = T_ARM_INS(BBL_INS_LAST(bbl));
  if (!prev ||
      (ARM_INS_OPCODE(prev) != ARM_B) ||
      (ARM_INS_CONDITION(prev) != ARM_CONDITION_AL))
    return NULL;
    
  /* get the instruction we're jumping to */
  target_address=AddressAddUint32(ARM_INS_CADDRESS(prev),((ARM_INS_FLAGS(prev) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(prev));
  jump_target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));

  if (!jump_target)
    return NULL;

  /* does it jump to the instruction we're checking? */
  if (!AddressIsEq(ARM_INS_CADDRESS(jump_target),ARM_INS_CADDRESS(ins)))
    return NULL;

  /* ok, return the instruction preceding the jump as the previous
   * instruction
   */
  return ARM_INS_IPREV(prev);
}
/*}}}*/

static t_address ArmCalculateBranchDestination(t_arm_ins * ins)
{
        return AddressAddUint32(ARM_INS_CADDRESS(ins),((ARM_INS_FLAGS(ins) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(ins));
}

static t_bbl * ArmCheckForUpperBoundBbl(t_bbl * bbl, t_bbl * branch_bbl)
{
  t_arm_ins * last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
  if (last_ins && (ARM_INS_OPCODE(last_ins)==ARM_B || RegsetIn(ARM_INS_REGS_DEF(last_ins),ARM_REG_R15)))
  {
    /* check target (moet na bbl springen) */
    if (((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_T2TBB || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_T2TBH))
        && !AddressIsEq(BBL_CADDRESS(branch_bbl), ArmCalculateBranchDestination(last_ins)))
    {
      //DEBUG(("target not to TBB/TBH, continue @I\n @G\n @I\n @G", BBL_INS_LAST(branch_bbl), ARM_INS_CADDRESS(BBL_INS_LAST(branch_bbl)), last_ins, AddressAddUint32(ARM_INS_CADDRESS(last_ins),((ARM_INS_FLAGS(last_ins) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(last_ins))));
      return NULL;
    }
    else if (BBL_NINS(bbl)==1
        && ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_LAST(bbl)))
        && BBL_PREV(bbl)
        && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(BBL_PREV(bbl))))==ARM_B
        && ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(BBL_PREV(bbl))))!=ARM_CONDITION_AL)
    {
      bbl = BBL_PREV(bbl);
    }

    /* the BBL could be changed */
    last_ins = T_ARM_INS(BBL_INS_LAST(bbl));

    /* check LS condition */
    if (ARM_INS_CONDITION(last_ins)!=ARM_CONDITION_LS && ARM_INS_CONDITION(last_ins)!=ARM_CONDITION_HI) return NULL;

    if (ARM_INS_CONDITION(last_ins) == ARM_CONDITION_HI
        && BBL_NEXT(bbl)
        && BBL_INS_LAST(BBL_NEXT(bbl))
        && ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_LAST(BBL_NEXT(bbl))))
        && AddressIsEq(BBL_CADDRESS(branch_bbl), ArmCalculateBranchDestination(T_ARM_INS(BBL_INS_LAST(BBL_NEXT(bbl))))))
      return bbl;

    if (!(ARM_INS_CONDITION(last_ins) == ARM_CONDITION_LS
        && AddressIsEq(BBL_CADDRESS(branch_bbl), ArmCalculateBranchDestination(last_ins))))
        return NULL;

    /* if so */
    //    DEBUG(("found range-defining BBL: @iB", bbl));

    return bbl;
  }

  return NULL;
}

/* This function propagates a register from one instruction,
 * across stack loads/stores up to an instruction in the same BBL. */
static t_reg ArmPropagateRegBackwardStack(t_reg prop, t_arm_ins * from, t_arm_ins * till)
{
        t_arm_ins * i_ins = from;
        t_arm_ins * load_instr = NULL;

        while (i_ins)
        {
                /* don't go past the ADD LSL instruction */
                if (i_ins == till) break;

                if (RegsetIn(ARM_INS_REGS_DEF(i_ins), ARM_REG_R13))
                        load_instr = NULL;

                /* maybe the register is loaded from a stack location? */
                if (RegsetIn(ARM_INS_REGS_DEF(i_ins), prop))
                {
                        if (!load_instr
                                && ARM_INS_OPCODE(i_ins)==ARM_LDR
                                && ARM_INS_REGB(i_ins)==ARM_REG_R13
                                && (ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
                                load_instr = i_ins;
                        else break;
                }

                /* perhaps we'v found the corresponding STORE for a stack-relative load */
                if (load_instr
                        && ARM_INS_OPCODE(i_ins)==ARM_STR
                        && ARM_INS_REGB(i_ins)==ARM_REG_R13
                        && (ARM_INS_FLAGS(i_ins) & FL_PREINDEX)
                        && (ARM_INS_FLAGS(i_ins) & FL_DIRUP)==(ARM_INS_FLAGS(load_instr) & FL_DIRUP)
                        && ARM_INS_IMMEDIATE(i_ins)==ARM_INS_IMMEDIATE(load_instr))
                {
                        load_instr = NULL;
                        prop = ARM_INS_REGA(i_ins);
                }

                i_ins = ARM_INS_IPREV(i_ins);
        }

        return prop;
}

static t_bool ArmCountEntriesInSwitchTable(t_bbl * branch_bbl, t_uint32 * nr_entries)
{
  t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(branch_bbl));
  //t_uint32 chunk_size = 1; unused

  if (ARM_INS_OPCODE(branch_ins) == ARM_T2TBB || ARM_INS_OPCODE(branch_ins) == ARM_T2TBH)
  {
    t_bbl * data_bbl = NULL;
    t_arm_ins * i_ins = NULL;
    t_uint32 counter = 0;

    if (ARM_INS_REGB(branch_ins) != ARM_REG_R15)
    {
      VERBOSE(3, ("base register is not the PC, not detecting switch table size @I", branch_ins));
      return FALSE;
    }

    data_bbl = BBL_NEXT(branch_bbl);
    //    chunk_size = (ARM_INS_OPCODE(branch_ins) == ARM_T2TBB) ? 1 : 2;

    BBL_FOREACH_ARM_INS(data_bbl, i_ins)
    {
      t_reloc_ref * ref = NULL;

      /* a 32-bit DATA should contain TWO relocations */
      ref = ARM_INS_REFERS_TO(i_ins);
      while (ref)
      {
        counter++;
        ref = RELOC_REF_NEXT(ref);
      }
    }

    if (nr_entries)
      *nr_entries = counter;

    VERBOSE(1, ("detected switch table size for @I: %d", branch_ins, counter));
    return TRUE;
  }

  return FALSE;
}

bool ArmFindDefinitionOfRegister(t_arm_ins * start, t_reg reg, t_uint32 * immvalue) {
  t_uint32 counter = 0;
  t_bbl *to_bbl = ARM_INS_BBL(start);
  t_bbl *from_bbl = BBL_PREV(to_bbl);

  while (counter < 10) {
    counter++;

    t_arm_ins *ins_last = T_ARM_INS(BBL_INS_LAST(from_bbl));
    if (!ArmInsIsUnconditionalBranch(ins_last)) {
      from_bbl = BBL_PREV(from_bbl);
      continue;
    }

    t_address target = AddressAddUint32(ARM_INS_CADDRESS(ins_last),
                                        ARM_INS_IMMEDIATE(ins_last) + ((ARM_INS_FLAGS(ins_last) & FL_THUMB)?4:8));

    if (AddressIsEq(target,ARM_INS_CADDRESS(T_ARM_INS(BBL_INS_FIRST(to_bbl))))) {
      // found the BBL branching to the destination
      t_arm_ins *ins;
      BBL_FOREACH_ARM_INS_R(from_bbl, ins) {
        if (RegsetIn(INS_REGS_DEF(T_INS(ins)), reg)) {
          if ((ARM_INS_OPCODE(ins) == ARM_MOV && ARM_INS_FLAGS(ins) & FL_IMMED)
              || (ARM_INS_OPCODE(ins) == ARM_MOVW && ARM_INS_FLAGS(ins) & FL_IMMEDW)) {
            *immvalue = ARM_INS_IMMEDIATE(ins);
            return TRUE;
          }
          else
            return FALSE;
        }
      }

      to_bbl = from_bbl;
      from_bbl = BBL_PREV(from_bbl);
    }
    else {
      from_bbl = BBL_PREV(from_bbl);
    }
  }

  return FALSE;
}

#define UPPERBOUND_VERBOSITY 3
static t_bool ArmFindSwitchTableUpperBoundCmp(t_cfg *cfg, t_bbl *bbl, t_reg cmp_reg, t_uint32 *upper_bound, t_bbl * branch_bbl, t_arm_ins * start_ins)
{
  t_arm_ins *ins;

  t_arm_ins *stackrel_load = NULL;
  t_bool stackrel_load_corr_store_found = FALSE;
  //  t_bool looking_for_corr_store = FALSE;

  /* save the last encountered branch instruction, so we are able to determine
     if the immediate value of the compare instruction is range-inclusive or range-exclusive */
  t_arm_ins *last_branch_ins = NULL;
  t_arm_ins *last_ins = NULL;

  t_uint32 multiplier = 1;
  t_uint32 shifter = 0;

  t_bool detected_size = FALSE;
  t_uint32 detected_count = 0;

  t_bool valid_upper_bound_by_cmp = FALSE;

  t_bool found = FALSE;

  t_bool seen_other_conditional_instructions_since_last_branch = TRUE;
  t_reg found_cmp_reg = ARM_REG_NONE;
  t_arm_ins * found_cmp_ins = NULL;
  t_bool found_cmp = FALSE;

  t_bool conditional_branch_to_branch_bbl = FALSE;

  detected_size = ArmCountEntriesInSwitchTable(branch_bbl, &detected_count);

  /* check for a possible stack-relative load of the cmp_reg register
   * NOTE: this is not necessarily a STACK-based load (e.g. spec2006, gcc, compiled with LLVM 3.4, O0)
         595ea:       2808            cmp     r0, #8
         595ec:       f8c6 c17c       str.w   ip, [r6, #380]  ; 0x17c
         595f0:       f8c6 8178       str.w   r8, [r6, #376]  ; 0x178
         595f4:       f8c6 9174       str.w   r9, [r6, #372]  ; 0x174
         595f8:       f8c6 a170       str.w   sl, [r6, #368]  ; 0x170
         595fc:       f8c6 b16c       str.w   fp, [r6, #364]  ; 0x16c
         59600:       f8c6 0168       str.w   r0, [r6, #360]  ; 0x168
         59604:       d839            bhi.n   5967a <emit_library_call_value_1+0x132>
         59606:       f8d6 1168       ldr.w   r1, [r6, #360]  ; 0x168
         5960a:       e8df f001       tbb     [pc, r1]
   */
  BBL_FOREACH_ARM_INS_R(branch_bbl, ins)
  {
    t_bool can_fatal = TRUE;

    /* skip the last instruction of the branch bbl as this is the branch instruction */
    if (ins == T_ARM_INS(BBL_INS_LAST(branch_bbl))) continue;

    /* if we start looking in the middle of a BBL, skip every instruction after start_ins */
    if (start_ins
        && ARM_INS_BBL(start_ins) == ARM_INS_BBL(ins)
        && ARM_INS_OLD_ADDRESS(ins) >= ARM_INS_OLD_ADDRESS(start_ins))
        continue;

    if (ARM_INS_OPCODE(ins)==ARM_CMP && ARM_INS_REGB(ins)==cmp_reg)
    {
      /* we have found the cmp, so no need to look further for other registers to replace it with */
      break;
    }

    if (ARM_INS_OPCODE(ins)==ARM_LDR
        && ARM_INS_REGA(ins)==cmp_reg
        /*&& ARM_INS_REGB(ins)==ARM_REG_R13*/
        && (ARM_INS_FLAGS(ins) & FL_IMMED))
    {
      /* we have found a stack-relative load-immediate instruction
       * that defines the cmp-register */
      stackrel_load = ins;
      break;
    }

    if (ARM_INS_OPCODE(ins)==ARM_MOV
        && ARM_INS_REGA(ins)==cmp_reg
        && !(ARM_INS_FLAGS(ins) & FL_IMMED))
    {
      /* shift instructions (e.g., LSL r0, r0, #2) are also included here  */
      cmp_reg = ARM_INS_REGC(ins);
      can_fatal = FALSE;

      /* do not necessarily break here, e.g. in a chain of moves */
    }

    if (ARM_INS_OPCODE(ins) == ARM_ADD
        && ARM_INS_REGA(ins) == cmp_reg
        && ARM_INS_FLAGS(ins) & FL_IMMED
        && ARM_INS_IMMEDIATE(ins) == 0)
    {
        cmp_reg = ARM_INS_REGB(ins);
    }

    if (RegsetIn(ARM_INS_REGS_DEF(ins), cmp_reg) && can_fatal)
        FATAL(("did not expect an instruction defining the compare register here @I", ins));
  }

  /* on thumb, the conditional branch to the target may have been inverted
   * because of offset limitations, and/or may be followed by a bunch of
   * unrelated branch trampolines -> skip all unconditional branches
   */
  /*while ((BBL_NINS(bbl)==1) &&
         ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_LAST(bbl))) &&
         BBL_PREV(bbl))
    bbl=BBL_PREV(bbl);*/

  /* Handle a specific case for LLVM, where the default case of a TBH switch statement
   * is in front of the TBH instruction. This case can contain unconditional branches.
   * As such, the algorithm that patternmatches the CMP/B<c> instruction sequences will
   * fail. Here, we look back in the instruction list, per BBL, for a BBL ending in a
   * BLS instruction. */
   last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
   found = TRUE;
  if (last_ins &&
        (ArmInsIsUnconditionalBranch(last_ins) || ( RegsetIn(ARM_INS_REGS_DEF(last_ins),ARM_REG_R15) && ARM_INS_CONDITION(last_ins)==ARM_CONDITION_AL)
         || ARM_INS_TYPE(last_ins) == IT_DATA))
  {
    t_bbl * tmp_bbl = bbl;
    found = FALSE;

    while ((tmp_bbl = BBL_PREV(tmp_bbl)))
    {
      t_bbl * t = NULL;

      /* do not go past switch BBL's (i.e. nested switch tables) */
      if (ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(tmp_bbl))) & IF_SWITCHJUMP) break;

      t = ArmCheckForUpperBoundBbl(tmp_bbl, branch_bbl);
      if (t)
      {
        found = TRUE;
        bbl = t;
        break;
      }
    };

    /* we did not find a preceeding BLS instruction, maybe the check is done elsewhere in the program.
     * Since we really have no clue where this can be, go over every BBL; */
    if (!found)
    {
      VERBOSE(UPPERBOUND_VERBOSITY, ("did not find a BLS instruction for @iB in @iB, looking in the CFG for a BBL that ends in a BLS instruction",branch_bbl,bbl));
      CFG_FOREACH_BBL(cfg, tmp_bbl)
      {
        t_bbl * t = ArmCheckForUpperBoundBbl(tmp_bbl, branch_bbl);
        if (t)
        {
          found = TRUE;
          bbl = t;
          break;
        }
      }
    }

    if (!detected_size)
      ASSERT(found,("switch table with unconditional branch without detected preceeding BLS: @iB",bbl));
  }


  /* Sometimes the cmp we are looking for is conditional, with the same condition as the
     conditional branch to the switch bbl itself.
     Rather than skipping that instruction, we should than treat it as unconditional in
     our search for the cmp.
  */

  {
    t_arm_ins * ins_last = T_ARM_INS(BBL_INS_LAST(bbl));
    
    if (ARM_INS_OPCODE(ins_last)==ARM_B && 
        ARM_INS_CONDITION(ins_last) != ARM_CONDITION_AL
        )
      {
        t_address target = AddressAddUint32(ARM_INS_CADDRESS(ins_last),ARM_INS_IMMEDIATE(ins_last)+((ARM_INS_FLAGS(ins_last) & FL_THUMB)?4:8));
        if (AddressIsEq(target,ARM_INS_CADDRESS(T_ARM_INS(BBL_INS_FIRST(branch_bbl)))))
          conditional_branch_to_branch_bbl = TRUE;
      }
  }
  

  if (found)
  {
    BBL_FOREACH_ARM_INS_R(bbl,ins)
    {
      if (ArmInsIsNOOP(ins)) continue;

      if (ArmIsControlflow(ins))
      {
        if (!detected_size)
          ASSERT(ARM_INS_CONDITION(ins) != ARM_CONDITION_AL, ("can't have unconditional branches between compare and switch table @I", ins));

        seen_other_conditional_instructions_since_last_branch = FALSE;
        last_branch_ins = ins;
      }
      else if(last_branch_ins && ARM_INS_CONDITION(ins)!=ARM_INS_CONDITION(last_branch_ins))
      {
        seen_other_conditional_instructions_since_last_branch = TRUE;
      }

      /* Skip the last instructiom as this is the branch instruction */
      if (ins==T_ARM_INS(BBL_INS_LAST(bbl))) continue;

      if (!conditional_branch_to_branch_bbl)
        if (last_branch_ins && ARM_INS_CONDITION(ins)==ARM_INS_CONDITION(last_branch_ins))
          continue;

      /* have we found a MOV instruction which redefines the compare register? */
      if (found_cmp_reg != ARM_REG_NONE
          && ARM_INS_OPCODE(ins)==ARM_MOV
          && ARM_INS_REGA(ins)==cmp_reg
          && ARM_INS_REGC(ins)==found_cmp_reg)
      {
        found_cmp = TRUE;
        break;
      }

      if (found_cmp_reg != ARM_REG_NONE
          && ARM_INS_OPCODE(ins)==ARM_MOV
          && ARM_INS_REGA(ins)==found_cmp_reg
          && ARM_INS_REGC(ins)==cmp_reg
          && ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSR_IMM
          /*&& ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_T2TBH*/)
      {
        found_cmp = TRUE;
        multiplier = 1<<ARM_INS_SHIFTLENGTH(ins);
        shifter = ARM_INS_SHIFTLENGTH(ins);
        VERBOSE(UPPERBOUND_VERBOSITY, ("found shift instruction, multiplier set to %d, shifter set to %d", multiplier, shifter));
        break;
      }

      /* For example:
          cbf2:       980a            ldr     r0, [sp, #40]   ; 0x28
          cbf4:       990a            ldr     r1, [sp, #40]   ; 0x28
          cbf6:       290e            cmp     r1, #14
          cbf8:       9009            str     r0, [sp, #36]   ; 0x24
          cbfa:       f200 80c7       bhi.w   cd8c <quantum_objcode_put+0x1e8>
          cbfe:       9909            ldr     r1, [sp, #36]   ; 0x24
          cc00:       e8df f001       tbb     [pc, r1]
       */
      if (found_cmp_reg != ARM_REG_NONE)
      {
        /* we have already found a compare instruction */
        t_arm_ins * ins_defining_cmp_reg = NULL;
        t_arm_ins * ins_defining_found_cmp_reg = NULL;

        /* try to look for two equivalent LDR instructions */
        t_arm_ins * tmp = ins;
        while (tmp && !(ins_defining_cmp_reg && ins_defining_found_cmp_reg))
        {
          if (!ins_defining_cmp_reg
              && ARM_INS_OPCODE(tmp)==ARM_LDR
              && (ARM_INS_FLAGS(tmp) & FL_IMMED)
              && ARM_INS_REGA(tmp)==cmp_reg)
          {
            ins_defining_cmp_reg = tmp;
          }
          else if (!ins_defining_found_cmp_reg
              && ARM_INS_OPCODE(tmp)==ARM_LDR
              && (ARM_INS_FLAGS(tmp) & FL_IMMED)
              && ARM_INS_REGA(tmp)==found_cmp_reg)
          {
            ins_defining_found_cmp_reg = tmp;
          }

          /* try the next instruction */
          tmp = ARM_INS_IPREV(tmp);
        }

        /* have we found two stack-relative loads */
        if (ins_defining_cmp_reg && ins_defining_found_cmp_reg)
        {
          if (ARM_INS_IMMEDIATE(ins_defining_cmp_reg)==ARM_INS_IMMEDIATE(ins_defining_found_cmp_reg)
              && (ARM_INS_FLAGS(ins_defining_cmp_reg) & FL_DIRUP)==(ARM_INS_FLAGS(ins_defining_found_cmp_reg) & FL_DIRUP)
              && ARM_INS_REGB(ins_defining_cmp_reg) == ARM_INS_REGB(ins_defining_found_cmp_reg))
          {
            /* the two loads are equivalent */
            found_cmp = TRUE;
            break;
          }
        }
      }

      if (stackrel_load
          && RegsetIn(ARM_INS_REGS_DEF(ins), ARM_INS_REGB(stackrel_load)))
      {
        /* the stack pointer is redefined, we do not know anything about the stack relative load anymore */

        if (stackrel_load)
          VERBOSE(UPPERBOUND_VERBOSITY, ("invalidating the load instruction @I since the base register is redefined", stackrel_load));

        stackrel_load = NULL;
      }

      if (!stackrel_load_corr_store_found
          && stackrel_load
          && ARM_INS_OPCODE(ins)==ARM_STR
          /*&& ARM_INS_REGB(ins)==ARM_REG_R13*/
          && ARM_INS_REGB(ins)==ARM_INS_REGB(stackrel_load)
          && ARM_INS_IMMEDIATE(ins)==ARM_INS_IMMEDIATE(stackrel_load)
          && (ARM_INS_FLAGS(ins) & FL_DIRUP)==(ARM_INS_FLAGS(stackrel_load) & FL_DIRUP))
      {
        /* here we have a STORE that writes to the same location as the stack-relative load */
        VERBOSE(UPPERBOUND_VERBOSITY, ("Found the corresponding STORE for a LOAD\n  @I\n  @I\n  Adding register r%d to possible compare registers", ins, stackrel_load, ARM_INS_REGA(ins)));
        cmp_reg = ARM_INS_REGA(ins);
        stackrel_load = NULL;
        stackrel_load_corr_store_found = TRUE;
      }

      if ((ARM_INS_REGC(ins)==cmp_reg) ||
          (ARM_INS_REGA(ins)==cmp_reg))
      {
        /* here we haven't found a CMP instruction just yet.
         * However, a MOV instruction may occur. */
        if (found_cmp_reg == ARM_REG_NONE
            && ARM_INS_OPCODE(ins)==ARM_MOV
            && ARM_INS_REGA(ins)==cmp_reg
            && !(ARM_INS_FLAGS(ins) & FL_IMMED))
        {
          cmp_reg = ARM_INS_REGC(ins);
        }
        else
        {
          VERBOSE(UPPERBOUND_VERBOSITY, ("A or C is the compare register cmp_reg R%d", cmp_reg));
          if (ARM_INS_OPCODE(ins)!=ARM_STR && ARM_INS_OPCODE(ins)!=ARM_CMP
              && ARM_INS_OPCODE(ins)!=ARM_STRB)
          {
            if (seen_other_conditional_instructions_since_last_branch || (last_branch_ins && ARM_INS_CONDITION(ins)!=ARM_INS_CONDITION(last_branch_ins)))
            {
              if (!detected_size)
                FATAL(("Strange switch! @iB", bbl));
              else
                break;
            }
          }
        }
      }

      if (ARM_INS_OPCODE(ins)==ARM_CMP && found_cmp_reg == ARM_REG_NONE)
      {
        found_cmp_ins = ins;
        found_cmp_reg = ARM_INS_REGB(ins);

        stackrel_load = NULL;
        stackrel_load_corr_store_found = FALSE;
      }

      if (found_cmp_reg==cmp_reg)
      {
        found_cmp = TRUE;
        break;
      }
    }
  }

  if (!found_cmp && !detected_size)
  {
    t_arm_ins * switch_ins = T_ARM_INS(BBL_INS_LAST(branch_bbl));
    if ((ARM_INS_OPCODE(switch_ins) == ARM_ADD || ARM_INS_OPCODE(switch_ins) == ARM_LDR)
        && !(ARM_INS_FLAGS(switch_ins) & FL_IMMED)
        && !(ARM_INS_FLAGS(switch_ins) & FL_THUMB))
    {
      VERBOSE(0, ("Could not determine size of switch table for @I, going to HELL", switch_ins));
      return FALSE;
    }
    FATAL(("No switch check found for switch and the table size could not be determined! @I", BBL_INS_LAST(branch_bbl)));
  }

  if (found_cmp
            && last_branch_ins)
  {
    if (ARM_INS_REGC(found_cmp_ins)==ARM_REG_NONE)
    {
      *upper_bound = ARM_INS_IMMEDIATE(found_cmp_ins);
      valid_upper_bound_by_cmp = TRUE;
    }
    else
    {
      t_regset search;
      t_arm_ins *ldr;
      t_reloc *rel;
      t_bool failed;

      search = RegsetNew();
      RegsetSetAddReg(search,ARM_INS_REGC(found_cmp_ins));
      ldr = ArmFindPCRelLdrThatDefinesRegisterInBbl(found_cmp_ins,&search);
      if (ldr)
      {
        *upper_bound=GetPcRelLoadedValue(CFG_OBJECT(cfg),ldr,NULL,&rel,&failed);
        valid_upper_bound_by_cmp = TRUE;

        if (rel)
        {
          if (!detected_size)
            FATAL(("Upper bound of switch loaded by @I is relocated value @R",ldr,rel));
          else
            valid_upper_bound_by_cmp = FALSE;
        }
        if (failed)
        {
          if (!detected_size)
            FATAL(("Cannot find upper bound of switch via @I",ldr));
          else
            valid_upper_bound_by_cmp = FALSE;
        }
      }
      else if (ArmFindLoadImmThatDefinesRegisterInBbl(found_cmp_ins,ARM_INS_REGC(found_cmp_ins),upper_bound))
      {
        /* upper_bound has already been set by ArmFindLoadImmThatDefinesRegisterInBbl */
        valid_upper_bound_by_cmp = TRUE;
      }
      else
      {
        if (!detected_size) {
          if (ArmFindDefinitionOfRegister(found_cmp_ins, ARM_INS_REGC(found_cmp_ins), upper_bound)) {
            valid_upper_bound_by_cmp = TRUE;
          } else
          FATAL(("Cannot find PC-relative ldr loading upper bound of switch table for cmp in @ieB",ARM_INS_BBL(found_cmp_ins)));
        } else
          valid_upper_bound_by_cmp = FALSE;
      }
    }

    if (valid_upper_bound_by_cmp)
    {
      *upper_bound = (*upper_bound) * multiplier + (multiplier - 1);

      switch(ARM_INS_CONDITION(last_branch_ins))
      {
      case ARM_CONDITION_HI:
        *upper_bound = (*upper_bound)+1;
        break;

      case ARM_CONDITION_LS:
        /* in case of TBB/TBH/MOV, the last branch ins was as a BLS into the tbb, so we need to add 1 */
        /* in the other case, the last branch ins was the LDRLS switch statement itself, in which case no such thing should be done */
        
        if (ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_CONDITION_AL)
          //        if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_T2TBB || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_T2TBH || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(branch_bbl)))==ARM_MOV)
           *upper_bound = (*upper_bound)+1;
        
        //        DEBUG(("ALARM @iB",branch_bbl));
        break;

      default:
        FATAL(("unsupported condition in conditional branch @I", last_branch_ins));
      }
    }
  }

  if (valid_upper_bound_by_cmp && detected_size)
  {
#if 0
    /* TODO first: make the detection algorithm correct first, since now this check only gives false positives which clutter Diablo output on stdout */
    /* TODO this should be made an ASSERT equal */    
    if (detected_count != *upper_bound)
      WARNING(("detected table size and calculated table size are not equal @eiB!\n  Detected: %d, Calculated (via CMP): %d", branch_bbl, detected_count, *upper_bound));
#endif
  }
  else if (!valid_upper_bound_by_cmp)
  {
    VERBOSE(UPPERBOUND_VERBOSITY, ("could not detect table size by looking for CMP instruction pattern, using the detected size of %d", detected_count));
    *upper_bound = detected_count;
  }

  return TRUE;
}

static t_bool IsThumbTableBranch(t_bbl *bbl, t_uint32 *table_entry_size)
{
  t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(bbl));

  if (ARM_INS_OPCODE(last) == ARM_T2TBB)
  {
    *table_entry_size = 1;
    return TRUE;
  }
  else if (ARM_INS_OPCODE(last) == ARM_T2TBH)
  {
    *table_entry_size = 2;
    return TRUE;
  }

  return FALSE;
}

static t_bool IsThumb1GnuCaseDispatch(t_object * obj, t_bbl *bbl, t_uint32 *table_entry_size, t_uint32 *multiplier, t_bool *is_sign_extend)
{
  if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_BL
      && ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(bbl))) & FL_THUMB)
  {
    t_arm_ins * switch_ins = T_ARM_INS(BBL_INS_LAST(bbl));
    t_address sym_address = AddressAddUint32(ARM_INS_CADDRESS(switch_ins), 4 + ARM_INS_IMMEDIATE(switch_ins)) | 1;
    t_symbol * sym = SymbolTableGetFirstSymbolByAddress(OBJECT_SUB_SYMBOL_TABLE(obj), sym_address);

    while (sym)
    {
      if (   !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_sqi")
          || !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_uqi"))
      {
        *table_entry_size = 1;
        *multiplier = 2;
        *is_sign_extend = !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_sqi");
        break;
      }
      else if (   !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_shi")
               || !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_uhi"))
      {
        *table_entry_size = 2;
        *multiplier = 2;
        *is_sign_extend = !strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_shi");
        break;
      }
      else if (!strcmp(SYMBOL_NAME(sym), "__gnu_thumb1_case_si"))
      {
        *table_entry_size = 4;
        *multiplier = 1;
        *is_sign_extend = FALSE;
        break;
      }

      sym = SymbolTableGetNextSymbolByAddress(sym, sym_address);
    }

    return sym != NULL;
  }

  return FALSE;
}

static t_bool IsThumbSwitchDispatch(t_bbl *bbl, t_uint32 *table_entry_size)
{
  t_arm_ins *last, *prev = NULL, *prev2 = NULL, *prev3 = NULL;

  /* Thumb switch table generated by rvds with -Otime (with -Osize, an (unsupported) helper is used)
   *   cmp     r0, #<limit>
   *   bcs.n   <default>
   *   mov     r1, r0
   *   add     r1, pc       -> address of the adds below
   *   ldrb/h  r1, [r1, #4] -> load offset/2 from .dword table1/table2/...
   *   adds    r1, r1, r1   -> double offset (thumb 16 bit aligned); in case previous instruction was ldrb, this one is "lsl #1" for some reason
   *   add     pc, r1       -> switch
   *   .dword table1        -> lowest byte == (offset between add & L1) >> 1, etc
   *   .dword table2
   *   ...
   * L1:
   */ 
  last = T_ARM_INS(BBL_INS_LAST(bbl));
  if (last)
    prev = ARM_INS_IPREV(last);
  if (prev)
    prev2 = ARM_INS_IPREV(prev);
  if (prev2)
    prev3 = ARM_INS_IPREV(prev2);
  if ((ARM_INS_FLAGS(last) & FL_THUMB) &&
      (ARM_INS_OPCODE(last) == ARM_ADD) &&
      (ARM_INS_REGA(last) == ARM_REG_R15) &&
      (((ARM_INS_OPCODE(prev) == ARM_ADD) &&
        (ARM_INS_REGA(prev) == ARM_INS_REGB(prev)) &&
        (ARM_INS_REGA(prev) == ARM_INS_REGC(prev))) ||
       ((ARM_INS_OPCODE(prev) == ARM_MOV) &&
	(ARM_INS_SHIFTTYPE(prev) == ARM_SHIFT_TYPE_LSL_IMM) &&
	(ARM_INS_SHIFTLENGTH(prev) == 1))) &&
      (ARM_INS_REGA(prev) == ARM_INS_REGC(last)) &&
      (prev2 != prev) &&
      ((ARM_INS_OPCODE(prev2) == ARM_LDRB) ||
       (ARM_INS_OPCODE(prev2) == ARM_LDRH)) &&
      (ARM_INS_IMMEDIATE(prev2) == 4) &&
      (ARM_INS_REGB(prev2) == ARM_INS_REGC(prev)) &&
      (ARM_INS_REGA(prev2) == ARM_INS_REGC(prev)) &&
      prev3 != NULL &&
      (ARM_INS_OPCODE(prev3) == ARM_ADD) &&
      (ARM_INS_REGA(prev3) == ARM_INS_REGC(prev)) &&
      (ARM_INS_REGB(prev3) == ARM_INS_REGC(prev)) &&
      (ARM_INS_REGC(prev3) == ARM_REG_R15)
     )
    {
      if (ARM_INS_OPCODE(prev2) == ARM_LDRB)
	*table_entry_size=1;
      else
	*table_entry_size=2;
      return TRUE;
    }
  return FALSE;
}

/*!
 * When we find an address to function we normally add a hell edge to that
 * function, because we do not know where the address will be used. For switch
 * tables we know where the addresses are used. That is why we detect these
 * tables first and avoid that hell edges are created.
 *
 * \todo Needs only one parameter
 * 
 * \param obj
 * \param sec The section we are creating the flowgraph for, with basic blocks
 * created (but without edges)
 *
 * \return void 
 */
/* ArmOptimizeSwitches {{{ */
static void ArmOptimizeSwitches(t_cfg *cfg)
{
  t_uint32 tel;

  t_bbl * bbl, * databbl, *dest, * bbl2;
  t_arm_ins * data;
  t_object *obj = CFG_OBJECT(cfg);
  t_uint32 upper_bound,table_entry_size = 0, table_entry_size2 = 0;
  t_uint32 multiplier = 0;
  t_bool is_sign_extend = FALSE;
  
  CFG_FOREACH_BBL_SAFE(cfg,bbl,bbl2)
  {
    table_entry_size = 0;
    table_entry_size2 = 0;
    multiplier = 0;
    is_sign_extend = FALSE;
    upper_bound = 0;
           
    if ((BBL_INS_LAST(bbl)&&(ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(bbl)))&IF_SWITCHJUMP)))
    {
      if ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_LDR) 
	  && (ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_REG_R15) 
	  && (ARM_INS_REGB(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_REG_R15))
      {
        /* Jump table in memory {{{*/
	/* VERBOSE(0,("JUMP TABLE @I\n",BBL_INS_LAST(bbl)));*/
	if (ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)))!=2) FATAL(("UStrange switch %d!"));
	if (ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))!=ARM_SHIFT_TYPE_LSL_IMM) FATAL(("UStrange switch %d!"));
        ArmFindSwitchTableUpperBoundCmp(cfg,bbl,ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))),&upper_bound, bbl, NULL);
       /* printf("Valid values : %d\n",upper_bound); */

	databbl=BBL_NEXT(bbl);
	if (!databbl)
          FATAL(("Databbl not found!"));
	databbl=BBL_NEXT(databbl);
	if (!databbl)
          FATAL(("Databbl not found!"));

	/*	VERBOSE(0,("@iB\n",databbl)); */

	if (!AddressIsEq(AddressAddUint32(ARM_INS_CADDRESS(T_ARM_INS(BBL_INS_LAST(bbl))),(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(bbl))) & FL_THUMB)?4:8),BBL_CADDRESS(databbl)))
	  FATAL(("Datablock <-> Switch mismatch"));

	if (BBL_NINS(databbl)<upper_bound)
	  FATAL(("Found only part of switch table!"));

        /* TODO: extend for Thumb switch tables, that should be aligned on bytes or halfwords */
        BBL_SET_ALIGNMENT(databbl,0);
        BBL_SET_ALIGNMENT_OFFSET(databbl,0);

	for (tel=0, data=T_ARM_INS(BBL_INS_FIRST(databbl)); tel<=upper_bound; tel++, data=ARM_INS_INEXT(data))
	{
	  t_reloc_ref * ref=ARM_INS_REFERS_TO(data);

	  if (!ref) FATAL(("NO REFS!"));
          ASSERT((RELOC_N_TO_RELOCATABLES(RELOC_REF_RELOC(ref))==1), ("Unexpected complex relocation in switch table: @R",RELOC_REF_RELOC(ref)));
	  /*VERBOSE(0,("Looking for address %x\n",ARM_INS_IMMEDIATE(data))); */
	  dest=find_close_bbl_by_address(databbl, AddressNew32(ARM_INS_IMMEDIATE(data)));

	  if (T_RELOCATABLE(dest)!=RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ref))[0]) 
	  {
	    t_reloc * rel = RELOC_REF_RELOC(ref);

	    RelocSetToRelocatable(rel, 0, T_RELOCATABLE(dest));
	    Free(RELOC_CODE(rel));
	    RELOC_SET_CODE(rel, StringDup("R00\\l*w\\s0000$"));
	    WARNING(("Relocations for switch table do not adhere the conventions used in Diablo. Compensating for now, but other relocs might also be wrong!"));  
	  }

	  if (!dest) FATAL(("Destination bbl for switch not found!"));

	  RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);

	  {
	    t_cfg_edge * edge = CfgEdgeCreate(cfg,bbl,dest,ET_SWITCH);
	    CFG_EDGE_SET_SWITCHVALUE(edge, tel);
	    CFG_EDGE_SET_REL(edge,  RELOC_REF_RELOC(ref));
	    RELOC_SET_SWITCH_EDGE(RELOC_REF_RELOC(ref), edge);
	  }

	}
        /*}}}*/
      }
      else if ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_LDR)
          && (ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_REG_R15))
      {
        t_arm_ins * last = T_ARM_INS(BBL_INS_LAST(bbl));
        t_bbl * databbl = BBL_NEXT(bbl);
        t_arm_ins * ins_to_start_search_for_cmp = NULL;
        t_reg bound_reg;
        if (ARM_INS_SHIFTTYPE(last) == ARM_SHIFT_TYPE_LSL_IMM
            && ARM_INS_SHIFTLENGTH(last) == 2)
          bound_reg = ARM_INS_REGC(last);
        else
          {
            /* look back for an LSL instruction */
            t_arm_ins *defB = NULL, *defC = NULL;
            
            t_arm_ins *it = ARM_INS_IPREV(last);
            while (it)
              {
                if (!defB && RegsetIn(ARM_INS_REGS_DEF(it), ARM_INS_REGB(last)))
                  defB = it;
                if (!defC && RegsetIn(ARM_INS_REGS_DEF(it), ARM_INS_REGC(last)))
                  defC = it;
		
                if (defB && defC)
                  break;
		
                it = ARM_INS_IPREV(it);
              }
            
            if (!defB || !defC)
              {
                CfgEdgeCreate(cfg, bbl, CFG_HELL_NODE(cfg), ET_IPJUMP);
                continue;
              }

            ASSERT(defB, ("no instruction found defining reg B of @I", last));
            ASSERT(defC, ("no instruction found defining reg C of @I", last));
            
            /* per definition, one of the two instructions is an LSL */
            if (ARM_INS_OPCODE(defB) == ARM_MOV
                && ARM_INS_SHIFTTYPE(defB) == ARM_SHIFT_TYPE_LSL_IMM
                && ARM_INS_SHIFTLENGTH(defB) == 2)
              {
                ins_to_start_search_for_cmp = defB;
                bound_reg = ARM_INS_REGC(defB);
              }
            else if (ARM_INS_OPCODE(defC) == ARM_MOV
                     && ARM_INS_SHIFTTYPE(defC) == ARM_SHIFT_TYPE_LSL_IMM
                     && ARM_INS_SHIFTLENGTH(defC) == 2)
              {
                ins_to_start_search_for_cmp = defC;
                bound_reg = ARM_INS_REGC(defC);
              }
            else
              FATAL(("strange switch @I", last));
          }
      
        ArmFindSwitchTableUpperBoundCmp(cfg, BBL_PREV(bbl), bound_reg, &upper_bound, bbl,  ins_to_start_search_for_cmp);
        if (G_T_UINT32(AddressAnd(ARM_INS_OLD_ADDRESS(last), 0x3)))
          databbl = BBL_NEXT(databbl);

        ASSERT(databbl, ("expected data block following switch statement @eiB", bbl));
        ASSERT(BBL_NINS(databbl) >= upper_bound, ("found only part of switch table @eiB at @eiB", bbl, databbl));

        BBL_SET_ALIGNMENT(databbl, 4);
        BBL_SET_ALIGNMENT_OFFSET(databbl, 0);

        for (tel = 0, data = T_ARM_INS(BBL_INS_FIRST(databbl)); tel < upper_bound; tel++, data = ARM_INS_INEXT(data))
        {
          t_reloc_ref * ref = ARM_INS_REFERS_TO(data);

          ASSERT(ref, ("no references to data @I", data));
          ASSERT(RELOC_N_TO_RELOCATABLES(RELOC_REF_RELOC(ref)) == 1, ("only 1 relocation expected to @I: @R", data, RELOC_REF_RELOC(ref)));

          dest = find_close_bbl_by_address(databbl, AddressNew32(ARM_INS_IMMEDIATE(data)));

          if (T_RELOCATABLE(dest)!=RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ref))[0])
          {
            t_reloc * rel = RELOC_REF_RELOC(ref);

            RelocSetToRelocatable(rel, 0, T_RELOCATABLE(dest));
            Free(RELOC_CODE(rel));
            RELOC_SET_CODE(rel, StringDup("R00\\l*w\\s0000$"));
            WARNING(("Relocations for switch table do not adhere the conventions used in Diablo. Compensating for now, but other relocs might also be wrong!"));
          }

          if (!dest) FATAL(("Destination bbl for switch not found!"));

          RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);

          {
            t_cfg_edge * edge = CfgEdgeCreate(cfg,bbl,dest,ET_SWITCH);
            CFG_EDGE_SET_SWITCHVALUE(edge, tel);
            CFG_EDGE_SET_REL(edge,  RELOC_REF_RELOC(ref));
            RELOC_SET_SWITCH_EDGE(RELOC_REF_RELOC(ref), edge);
          }
        }
      }
      else if ((ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_ADD)
	  && (ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_REG_R15)
	  && (ARM_INS_REGB(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_REG_R15))
      {
        /* Jump table based on data processing operation on R15 {{{*/
	t_arm_ins * prev=T_ARM_INS(ArmGetPrevIns(obj,T_ARM_INS(BBL_INS_LAST(bbl))));
	t_arm_ins * prev2 = NULL, * prev3 = NULL , * prev4= NULL;
	if (prev)
	  prev2=ArmGetPrevIns(obj,prev);
	if (prev2)
	  prev3=ArmGetPrevIns(obj,prev2);
	if (prev3)
	  prev4=ArmGetPrevIns(obj,prev3);


	while(prev)
	{
	  if ( RegsetIn(ARM_INS_REGS_DEF(prev),ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))))) break;
	  if ((ARM_INS_OPCODE(prev)==ARM_CMP)
	      &&(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))!=ARM_CONDITION_AL)
	      &&((ARM_INS_REGB(prev)==ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))))
		||(ARM_INS_REGC(prev)==ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl)))) )) break;
	  prev=ArmGetPrevIns(obj,prev);
	}

	if (!prev) FATAL(("No definition found for @I!",BBL_INS_LAST(bbl)));


	if (  (ARM_INS_OPCODE(prev)==ARM_CMP)
	    &&(ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_SHIFT_TYPE_LSL_IMM)
	    &&(ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)))==2)
	    &&((ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_LS)
	       || (ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_CC)
               || (ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_LT))
              //	    &&(ARM_INS_FLAGS(prev)&FL_IMMED)
	    )
	{
          /* Will be handled in ArmAddBasicBlockEdges */
	}
	/* rsb  rX, rY, #N
	 * add  r15, r15, rX lsl #2
	 */
	/* Jump table ADD type 1 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_RSB)
	         &&(ARM_INS_FLAGS(prev)&FL_IMMED)
	         &&(ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_SHIFT_TYPE_LSL_IMM)
	         &&(ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)))==2)
	         &&(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_AL))
	{
	  int i;
	  int j=0;
	  t_arm_ins * block_end = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(BBL_INS_LAST(bbl)))); 
	  
	  if (!block_end) FATAL(("no block end"));
	  if (!ARM_INS_INEXT(block_end)) FATAL(("no next ins"));
	  block_end=ARM_INS_INEXT(block_end);
	  i=ARM_INS_IMMEDIATE(prev)+1;
	  while (i--)
	  {
	    t_bbl * tmp;
	    t_arm_ins * ins_split=ARM_INS_INEXT(block_end);
	    t_bbl * jumpdest=ARM_INS_BBL(ins_split);
            t_cfg_edge * edge;

	    if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))  
	    {
	      tmp = BblSplitAtIns(jumpdest, T_INS(ins_split));
	    }
	    else
	    {
	      FATAL(("Implement block was already split....\n"));
	    }
	    edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
	    CFG_EDGE_SET_SWITCHVALUE(edge, j);
	    j++;
	    
	    block_end=ARM_INS_INEXT(block_end);
	  }

	 /* FATAL(("Implement adds @I\n",BBL_INS_LAST(bbl))); */
	}
	/*}}}*/
	/* sub  rY, r*, #1
	 * and  rX, rY, imm1
	 * eor  rX, rX, imm1
	 * add  rX, rX, rX lsl #1
	 * add  r15, r15, rX lsl #M
	 */
	/* Jump table ADD type 2 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_ADD)
	         &&(ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_SHIFT_TYPE_LSL_IMM)
	         &&(ARM_INS_SHIFTTYPE(prev)==ARM_SHIFT_TYPE_LSL_IMM)
	         &&(ARM_INS_SHIFTLENGTH(prev)==1)
	         &&(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_AL)
	         &&(ARM_INS_CONDITION(prev)==ARM_CONDITION_AL)
		 && ARM_INS_REGA(prev)==ARM_INS_REGB(prev) 
		 && ARM_INS_REGA(prev)==ARM_INS_REGC(prev) 
		 && ARM_INS_REGA(prev)==ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl)))
		 && prev2 != NULL
		 && (ARM_INS_OPCODE(prev2)==ARM_EOR)
		 && ARM_INS_REGA(prev2)==ARM_INS_REGB(prev2)
		 && ARM_INS_REGA(prev2)==ARM_INS_REGC(prev)
		 && prev3 != NULL
		 && (ARM_INS_OPCODE(prev3)==ARM_AND)
		 && ARM_INS_REGA(prev3)==ARM_INS_REGB(prev2)
		 && ARM_INS_IMMEDIATE(prev3)==ARM_INS_IMMEDIATE(prev2)
		 && prev4 != NULL
		 && (ARM_INS_OPCODE(prev4)==ARM_SUB)
		 && ARM_INS_REGA(prev4)==ARM_INS_REGB(prev3)
		 && ARM_INS_IMMEDIATE(prev4)==1
	    )
	    {
	    t_uint32 nr_of_iterations = ARM_INS_IMMEDIATE(prev3)+1;
	    t_bbl * noop_bbl = BBL_NEXT(bbl);
	    t_bbl * target_bbl = BBL_NEXT(noop_bbl);
	    t_bbl * next_bbl = target_bbl;
	    t_uint32 i,j;
	    t_uint32 nr_of_ins_per_iteration = 1;
	    t_arm_ins * ins_split;
	    t_bbl * tmp;
	    t_cfg_edge * new_edge;

	    for (i=0;i<ARM_INS_SHIFTLENGTH(prev);i++)
	      nr_of_ins_per_iteration *=2;
	    nr_of_ins_per_iteration+=1;
	    for (i=0;i<ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)));i++)
	      nr_of_ins_per_iteration *=2;
	    nr_of_ins_per_iteration /= 4;
	    
	    ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(BBL_INS_FIRST(next_bbl))));

	    for (i=0;i<nr_of_iterations;i++)
	      {
		t_cfg_edge * edge;
		
		if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))  
                  tmp = BblSplitAtIns(next_bbl, T_INS(ins_split));
		else
		  tmp = ARM_INS_BBL(ins_split);

		edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
		CFG_EDGE_SET_SWITCHVALUE(edge, i);

		for (j=0;j<nr_of_ins_per_iteration;j++)
		  ins_split = ARM_INS_INEXT(ins_split);

		next_bbl = tmp;
	      }

	    BBL_FOREACH_SUCC_EDGE(bbl,new_edge)
	      VERBOSE(10,("SUCC @ieB\n",CFG_EDGE_TAIL(new_edge)));

	    //	    printf("iterations %d nr_of_ins_per_iteration %d\n",nr_of_iterations, nr_of_ins_per_iteration);
	    //	    FATAL(("AHA Implement adds @I\ntarget @ieB\n",BBL_INS_LAST(bbl),target_bbl));
	  }
	/*}}}*/
	/* sub  rY, r*, #1
	 * and  rX, rY, imm1
	 * rsb  rX, rX, imm1
	 * add  rX, rX, rX lsl #1
	 * add  r15, r15, rX lsl #M
	 */
	/* Jump table ADD type 3 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_ADD)
	         &&(ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_SHIFT_TYPE_LSL_IMM)
	         &&(ARM_INS_SHIFTTYPE(prev)==ARM_SHIFT_TYPE_LSL_IMM)
	         &&(ARM_INS_SHIFTLENGTH(prev)==1)
	         &&(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_AL)
	         &&(ARM_INS_CONDITION(prev)==ARM_CONDITION_AL)
		 && ARM_INS_REGA(prev)==ARM_INS_REGB(prev) 
		 && ARM_INS_REGA(prev)==ARM_INS_REGC(prev) 
		 && ARM_INS_REGA(prev)==ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl)))
		 && prev2 != NULL
		 && (ARM_INS_OPCODE(prev2)==ARM_RSB)
		 && ARM_INS_REGA(prev2)==ARM_INS_REGB(prev2)
		 && ARM_INS_REGA(prev2)==ARM_INS_REGC(prev)
		 && prev3 != NULL
		 && (ARM_INS_OPCODE(prev3)==ARM_AND)
		 && ARM_INS_REGA(prev3)==ARM_INS_REGB(prev2)
		 && ARM_INS_IMMEDIATE(prev3)==ARM_INS_IMMEDIATE(prev2)
		 && prev4 != NULL
		 && (ARM_INS_OPCODE(prev4)==ARM_SUB)
		 && ARM_INS_REGA(prev4)==ARM_INS_REGB(prev3)
		 && ARM_INS_IMMEDIATE(prev4)==1
		 )
	  {
	    t_uint32 nr_of_iterations = ARM_INS_IMMEDIATE(prev3)+1;
	    t_bbl * noop_bbl = BBL_NEXT(bbl);
	    t_bbl * target_bbl = BBL_NEXT(noop_bbl);
	    t_bbl * next_bbl = target_bbl;
	    t_uint32 i,j;
	    t_uint32 nr_of_ins_per_iteration = 1;
	    t_arm_ins * ins_split;
	    t_bbl * tmp;
	    t_cfg_edge * new_edge;

	    for (i=0;i<ARM_INS_SHIFTLENGTH(prev);i++)
	      nr_of_ins_per_iteration *=2;
	    nr_of_ins_per_iteration+=1;
	    for (i=0;i<ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)));i++)
	      nr_of_ins_per_iteration *=2;
	    nr_of_ins_per_iteration /= 4;
	    
	    ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(BBL_INS_FIRST(next_bbl))));

	    for (i=0;i<nr_of_iterations;i++)
	      {
		t_cfg_edge * edge;
		
		if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))
                  tmp = BblSplitAtIns(next_bbl, T_INS(ins_split));
		else
		  tmp = ARM_INS_BBL(ins_split);
		   
		edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
		CFG_EDGE_SET_SWITCHVALUE(edge, i);

		for (j=0;j<nr_of_ins_per_iteration;j++)
		  ins_split = ARM_INS_INEXT(ins_split);


		next_bbl = tmp;

	      }


	    BBL_FOREACH_SUCC_EDGE(bbl,new_edge)
	      VERBOSE(10,("SUCC @ieB\n",CFG_EDGE_TAIL(new_edge)));


	    //	    FATAL(("AHA Implement adds @I\ntarget @ieB\n",BBL_INS_LAST(bbl),target_bbl));
	  }
	/*}}}*/
	/* (note: prev == prev2 can be because prev is reset separately
         *  in a loop after prev2/3/4 is set to the first previous
         *  instruction defining the index reg used in the add)
         *
	 * sub  rY, r*, #1
	 * sub  rX, rY, imm1
	 * rsb  rX, rX, imm1
	 * add  rX, rX, rX lsl #1
	 * add  r15, r15, rX lsl #X
	 */ 
	/* Jump table ADD type 4 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_ADD)
	    && (ARM_INS_SHIFTTYPE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_SHIFT_TYPE_LSL_IMM)
	    && (ARM_INS_SHIFTTYPE(prev)==ARM_SHIFT_TYPE_LSL_IMM)
	    && (ARM_INS_SHIFTLENGTH(prev)==1)
	    && (ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_INS_CONDITION(prev))
	    && (ARM_INS_REGA(prev)==ARM_INS_REGB(prev))
	    && (ARM_INS_REGA(prev)==ARM_INS_REGC(prev))
	    && (ARM_INS_REGA(prev)==ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))))
	    && prev2 == prev
	    && prev3 != NULL
	    && (ARM_INS_OPCODE(prev3)==ARM_RSB)
	    && (ARM_INS_IMMEDIATE(prev3)!=0)
	    && (ARM_INS_REGA(prev3) == ARM_INS_REGC(prev))
	    && prev4 != NULL
	    && (ARM_INS_OPCODE(prev4)==ARM_SUB)
	    && (ARM_INS_REGA(prev4)==ARM_INS_REGB(prev3))
	    )
	  /* You can encounter these unrolled loops on a gcc4glibc2.3.6 toolchain for the xscale */
	{
	  t_uint32 nr_of_iterations = ARM_INS_IMMEDIATE(prev3)+1;
	  t_bbl * noop_bbl = BBL_NEXT(bbl);
	  t_bbl * next_bbl = noop_bbl;
	  t_uint32 i,j;
	  t_uint32 nr_of_ins_per_iteration = 1;
	  t_arm_ins * ins_split;
	  t_bbl * tmp;

/*          VERBOSE(0,("Noopbbl: @eiB",noop_bbl));*/
	  for (i=0;i<ARM_INS_SHIFTLENGTH(prev);i++)
	    nr_of_ins_per_iteration *=2;
	  nr_of_ins_per_iteration+=1;
	  for (i=0;i<ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)));i++)
	    nr_of_ins_per_iteration *=2;
	  nr_of_ins_per_iteration /= 4;

	  if (!ArmInsIsNOOP(T_ARM_INS(BBL_INS_FIRST(next_bbl))))
	    FATAL(("We expect a NOOP in this pattern, but it is @I", BBL_INS_FIRST(next_bbl)));

	  ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(INS_INEXT(BBL_INS_FIRST(next_bbl)))));

	  for (i=0;i<nr_of_iterations;i++)
	  {
	    t_cfg_edge * edge;

	    if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))
              tmp = BblSplitAtIns(next_bbl, T_INS(ins_split));
	    else
	      tmp = ARM_INS_BBL(ins_split);

	    edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
	    CFG_EDGE_SET_SWITCHVALUE(edge, i);

	    for (j=0;j<nr_of_ins_per_iteration;j++)
	      ins_split = ARM_INS_INEXT(ins_split);


	    next_bbl = tmp;

	  }
	}
	/*}}}*/
	/* sub  rX, r*, *
	 * rsb  rX, rX, #imm1 (!= 0) 
	 * add  r15, r15, rX lsl #M
	 */
	/* Jump table ADD type 5 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_RSB)
	    && prev2 != NULL
	    && (ARM_INS_OPCODE(prev2) == ARM_SUB)
	    && (ARM_INS_REGA(prev) == ARM_INS_REGB(prev))
	    && (ARM_INS_REGA(prev2) == ARM_INS_REGB(prev))
	    && (ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_INS_REGB(prev))
	    && (ARM_INS_IMMEDIATE(prev)!=0)
	    )
	  /* You can encounter these unrolled loops on a gcc4glibc2.3.6 toolchain for the xscale */
	{
	  t_uint32 nr_of_iterations = ARM_INS_IMMEDIATE(prev)+1;
	  t_bbl * noop_bbl = BBL_NEXT(bbl);
	  t_bbl * next_bbl = noop_bbl;
	  t_uint32 i,j;
	  t_uint32 nr_of_ins_per_iteration = 1;
	  t_arm_ins * ins_split;
	  t_bbl * tmp;

	  nr_of_ins_per_iteration <<= (ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl)))) - 2;

	  ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(INS_INEXT(BBL_INS_FIRST(next_bbl)))));

	  for (i=0;i<nr_of_iterations;i++)
	  {
	    t_cfg_edge * edge;

	    if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))
              tmp = BblSplitAtIns(next_bbl,T_INS(ins_split));
	    else
	      tmp = ARM_INS_BBL(ins_split);

	    edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
	    CFG_EDGE_SET_SWITCHVALUE(edge, i);

	    for (j=0;j<nr_of_ins_per_iteration;j++)
	      ins_split = ARM_INS_INEXT(ins_split);


	    next_bbl = tmp;

	  }
        }
	/*}}}*/
	/*   ands rX, r*, #28
	 *   rsb  rX, rX, #32 
	 *   addne  r15, r15, rX
	 *   b   .L0
	 *   nop
	 *   ldr
	 *   ldr
	 *   ldr
	 *   ldr
	 *   ldr
	 *   ldr
	 *   ldr
	 *   add  r15, 15, rX
	 *   nop
	 *   nop
	 *   str
	 *   ... (5 more stores)
	 *   str
	 * L0:
	 */
	/* Jump table ADD type 6 {{{*/
	else if ((ARM_INS_OPCODE(prev)==ARM_RSB)
	    && prev2 != NULL
	    && (ARM_INS_OPCODE(prev2) == ARM_AND)
            && (ARM_INS_FLAGS(prev2) & FL_S)
	    && (ARM_INS_REGA(prev) == ARM_INS_REGB(prev))
	    && (ARM_INS_REGA(prev2) == ARM_INS_REGB(prev))
	    && (ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_INS_REGB(prev))
	    && (ARM_INS_IMMEDIATE(prev)==32)
	    && (ARM_INS_IMMEDIATE(prev2)==28)
            && (ARM_INS_SHIFTLENGTH(T_ARM_INS(BBL_INS_LAST(bbl))) == 0) 
	    && (ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_CONDITION_NE)
	    )
	  /* You can encounter these in the unrolled memset/memcpy loops
           * of glibc 2.7 + ports add-on. Could be generalised for any
           * immediate operands of prev and prev2, as long as
           *  o prev_imm & 3 == 0
           *  o prev2_imm == prev_imm + (~prev2_imm & (prev_imm-1)) + 1
           */
	{
	  t_uint32 nr_of_iterations = 8;
	  t_bbl * noop_bbl = BBL_NEXT(bbl);
	  t_bbl * target_bbl = BBL_NEXT(noop_bbl);
	  t_bbl * next_bbl = target_bbl;
	  t_uint32 i,j,switchcount;
	  t_uint32 nr_of_ins_per_iteration = 1;
	  t_arm_ins * ins_split;
	  t_bbl * tmp;
	  t_cfg_edge * new_edge;
	  ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(BBL_INS_FIRST(next_bbl))));
          

	  for (switchcount=0; switchcount < 2; switchcount++)
	  {
            ARM_INS_SET_ATTRIB(T_ARM_INS(BBL_INS_LAST(bbl)), ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(bbl))) | IF_SWITCHJUMP_FIXEDCASESIZE);

	    for (i=0;i<nr_of_iterations;i++)
	    {
	      t_cfg_edge * edge;

	      if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))  
		tmp = BblSplitAtIns(next_bbl,T_INS(ins_split));
	      else
		tmp = ARM_INS_BBL(ins_split);

	      edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
	      CFG_EDGE_SET_SWITCHVALUE(edge, i);

	      for (j=0;j<nr_of_ins_per_iteration;j++)
		ins_split = ARM_INS_INEXT(ins_split);


	      next_bbl = tmp;

	    }

            VERBOSE(10,("Switch part %d", switchcount));
	    BBL_FOREACH_SUCC_EDGE(bbl,new_edge)
	      VERBOSE(10,("SUCC @ieB\n",CFG_EDGE_TAIL(new_edge)));

            if (switchcount == 0)
            {
              t_arm_ins *check_ins;
              /* we've processed the load switch, now do the store switch */

              check_ins = ARM_INS_INEXT(T_ARM_INS(BBL_INS_FIRST(next_bbl)));
              if ((ARM_INS_OPCODE(check_ins) != ARM_ADD) ||
                  (ARM_INS_CONDITION(check_ins) != ARM_CONDITION_AL) ||
                  (ARM_INS_REGA(check_ins) != ARM_REG_R15) ||
                  (ARM_INS_REGB(check_ins) != ARM_REG_R15) ||
                  (ARM_INS_REGC(check_ins) != ARM_INS_REGA(prev)))
		FATAL(("Expected another switch following the first one at @I in @ieB",check_ins,next_bbl));
              bbl = next_bbl;
	      target_bbl = BBL_NEXT(next_bbl);
	      next_bbl = target_bbl;
	      ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, INS_CADDRESS(INS_INEXT(BBL_INS_FIRST(next_bbl)))));
              /* and also skip this BBL for further switch detection, since
               * no definition will be found for this add
               */
              while (bbl2 != next_bbl)
                bbl2 = BBL_NEXT(bbl2);
              bbl2 = BBL_NEXT(bbl2);
	    }
	  }
	    //	    FATAL(("AHA Implement adds @I\ntarget @ieB\n",BBL_INS_LAST(bbl),target_bbl));
	}
	/*}}}*/
	/* Jump table ADD type 7 {{{*/
	else if (IsThumbSwitchDispatch(bbl,&table_entry_size)) {
	  t_uint32 offset, curdataval, dataentrymask,dataentryshift;
	  t_uint32 i, bytes_left_in_data;
	  t_arm_ins * ins_split, *curdata;
	  t_bbl * tmp, * checkbbl, * databbl;//, * insbbl;
          t_address base_address;
          t_reg switchcmpreg;
          t_bbl **switchbbls;

          checkbbl = BBL_PREV(bbl);
          if ((prev4 != NULL) &&
              (ARM_INS_OPCODE(prev4) == ARM_MOV) &&
              (ARM_INS_REGA(prev4) == ARM_INS_REGC(prev)))
          {
            ASSERT(ARM_INS_REGC(prev4) != ARM_REG_NONE,("Switch table reg loaded with constant? @I",prev4));
            switchcmpreg = ARM_INS_REGC(prev4);
          }
          else
          {
            switchcmpreg = ARM_INS_REGC(prev); 
          }
          ArmFindSwitchTableUpperBoundCmp(cfg, checkbbl, switchcmpreg, &upper_bound, bbl, NULL);
          databbl = BBL_NEXT(bbl);
          BBL_SET_ALIGNMENT(databbl,0);
          BBL_SET_ALIGNMENT_OFFSET(databbl,0);

          if ((G_T_UINT32(BBL_CSIZE(databbl)) < upper_bound) ||
              (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(databbl))) != ARM_DATA))
            FATAL(("Did not find thumb switch table data bbl after @iB",bbl));
          //          insbbl = BBL_NEXT(databbl);
          /* "add pc, r1" in thumb -> pc + 4 */
          base_address = AddressAddUint32(INS_CADDRESS(BBL_INS_LAST(bbl)),4);
          /* keep track of location switch bbls so we can add relocations for
           * them in the switch table
           */
          switchbbls = Calloc(sizeof(t_bbl*),upper_bound);

          curdata = T_ARM_INS(BBL_INS_FIRST(databbl));
          curdataval = ARM_INS_IMMEDIATE(curdata);
          bytes_left_in_data = G_T_UINT32(ARM_INS_CSIZE(curdata));
	  if (table_entry_size==1)
	  {
	    dataentryshift=8;
	    dataentrymask=0xff;
	  }
	  else
	  {
	    dataentryshift=16;
	    dataentrymask=0xffff;
	  }

	  for (i=0;i<upper_bound;i++)
	  {
	    t_cfg_edge * edge;
            t_reloc *rel;
            char reloc_code[200], loadop, storeop;
            t_uint32 mask;

            offset = (curdataval & dataentrymask) * 2;
            curdataval >>= dataentryshift;
            ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, AddressAddUint32(base_address,offset)));

	    if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))
            {
	      VERBOSE(1,("splitting thumb switch bbl at @G: @iB",ARM_INS_OLD_ADDRESS(ins_split),ARM_INS_BBL(ins_split)));
              tmp = BblSplitAtIns(ARM_INS_BBL(ins_split), T_INS(ins_split));
            }
	    else
	      tmp = ARM_INS_BBL(ins_split);
            switchbbls[i]=tmp;
            /* add relocation to (re)calculate offset in switch table for this bbl. Since there
             * are no byte load/store instructions in the relocation stack engine, we have to
             * some masking and shifting
             */
            /* depending on the size of the current data entry we load 16 or 32 bits */
            switch (ARM_INS_CSIZE(curdata))
            {
              case 2:
                loadop='k';
                storeop='v';
                break;
              case 4:
                loadop='l';
                storeop='w';
                break;
              default:
                FATAL(("Unsupported data size in thumb switch table: %d of @I",ARM_INS_CSIZE(curdata),curdata));
                break;
            }
            /* mask out the byte we want to change */
            mask=~(dataentrymask << ((ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8));
            /* components:
             *  o begin: calculate the difference between the "pc" value calculated by the last
             *    instruction of the bbl, and the target label. Since the used "pc" value of
             *    the instruction will be pc+4, and since R01Z01+ == pc+2, subtract an extra 2.
             *    The >>1 is because the table contains the offsets divided by 2.
             *  o s%.4x<:  shift the calculated value to the right position in the table constant
             *  o %c: load the constant (16 or 32 bits)
             *  o i%.8x&|: mask out the byte we calculated and or with the new value
             *  o %c: write the new constant (16 or 32 bits)
             *  o check part: make sure the result is even and fits in one byte after >>1
             */
            snprintf(reloc_code,sizeof(reloc_code),"R00R01Z01+-s0002-\\=s0001>s%.4x<%ci%.8x&|%c\\i%.8x&$",
                (ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8,
                loadop,
                mask,
                storeop,
		~(dataentrymask<<1));

            rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(curdata), AddressNullForObject(obj), T_RELOCATABLE(tmp), AddressNullForObject(obj), FALSE, NULL, NULL, NULL, reloc_code);
            RelocAddRelocatable(rel,T_RELOCATABLE(bbl),AddressNullForBbl(bbl));

            bytes_left_in_data-=table_entry_size;
            if (!bytes_left_in_data)
            {
              curdata = ARM_INS_INEXT(curdata);
	      if (!curdata)
	      {
		ASSERT(i==upper_bound-1,("Ran out of switch table entries for @iB",bbl));
	      }
	      else
	      {
		bytes_left_in_data = ARM_INS_CSIZE(curdata);
		curdataval = ARM_INS_IMMEDIATE(curdata);
	      }
            }

	    edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
	    CFG_EDGE_SET_SWITCHVALUE(edge, i);
	    RELOC_SET_HELL(rel, FALSE);
	    CFG_EDGE_SET_REL(edge, rel);
	    RELOC_SET_SWITCH_EDGE(rel, edge);
	  }
	}
	else
	  {
	    FATAL(("Implement adds @I\n@I",BBL_INS_LAST(bbl),prev));
	  }
	/*}}}*/
      }
      else if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_ADD
               && ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_REG_R15
               && !(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(bbl))) & FL_THUMB))
      {
        t_bool detected = TRUE;
        t_arm_ins * last = T_ARM_INS(BBL_INS_LAST(bbl));
        t_bbl * databbl = BBL_NEXT(bbl);

        /* find instructions defining REG_B and REG_C */
        t_arm_ins * tmp = NULL;
        t_arm_ins * ins_defining_regb = NULL;
        t_arm_ins * ins_defining_regc = NULL;

        t_arm_ins * load_c = NULL;
        t_bool looking_for_corr_store_c = FALSE;

        t_reg regb = ARM_INS_REGB(last);
        t_reg regc = ARM_INS_REGC(last);

        BBL_FOREACH_ARM_INS_R(bbl, tmp)
        {
          /* skip switch instruction */
          if (tmp == last) continue;

          if (RegsetIn(ARM_INS_REGS_DEF(tmp), regb)
              && !ins_defining_regb)
            ins_defining_regb = tmp;

          if (RegsetIn(ARM_INS_REGS_DEF(tmp), regc)
              && !ins_defining_regc
              && !looking_for_corr_store_c)
          {
            if (ARM_INS_OPCODE(tmp) == ARM_LDR
                && ARM_INS_FLAGS(tmp) & FL_IMMED
                && ARM_INS_FLAGS(tmp) & FL_PREINDEX)
            {
              /* stack-relative load */
              looking_for_corr_store_c = TRUE;
              load_c = tmp;
            }
            else
            {
              looking_for_corr_store_c = FALSE;
              ins_defining_regc = tmp;
            }
          }

          if (looking_for_corr_store_c
              && ARM_INS_OPCODE(tmp) == ARM_STR
              && ARM_INS_REGB(tmp) == ARM_INS_REGB(load_c)
              && ARM_INS_FLAGS(load_c) & FL_IMMED
              && (ARM_INS_FLAGS(tmp) & FL_DIRUP) == (ARM_INS_FLAGS(load_c) & FL_DIRUP)
              && ARM_INS_FLAGS(tmp) & FL_PREINDEX
              && ARM_INS_IMMEDIATE(tmp) == ARM_INS_IMMEDIATE(load_c))
          {
            regc = ARM_INS_REGA(tmp);
            looking_for_corr_store_c = FALSE;
          }
        }
        ASSERT(ins_defining_regb && ins_defining_regc, ("unrecognised switch: @I", last));

        /* find semantics of these two instructions */
        t_arm_ins * table_addr_prod = NULL;
        t_arm_ins * offset_calc_instr = NULL;
        if (ARM_INS_OPCODE(ins_defining_regb) == ARM_ADD
            && ARM_INS_REGB(ins_defining_regb) == ARM_REG_R15
            && ARM_INS_FLAGS(ins_defining_regb) & FL_IMMED)
        {
          table_addr_prod = ins_defining_regb;
          offset_calc_instr = ins_defining_regc;
        }
        else if (ARM_INS_OPCODE(ins_defining_regc) == ARM_ADD
            && ARM_INS_REGB(ins_defining_regc) == ARM_REG_R15
            && ARM_INS_FLAGS(ins_defining_regc) & FL_IMMED)
        {
          table_addr_prod = ins_defining_regc;
          offset_calc_instr = ins_defining_regb;
        }
        else
          FATAL(("expected to find address producer for @I, but did not find one", last));

        /* sanity checks */
        ASSERT(AddressAddUint32(
                AddressAddUint32(ARM_INS_CADDRESS(table_addr_prod), 8),
                ARM_INS_IMMEDIATE(table_addr_prod)), ("address producer @I does not produce address of expected data BBL @eiB", table_addr_prod, databbl));

        ASSERT(ARM_INS_OPCODE(offset_calc_instr) == ARM_LDR
               && ARM_INS_FLAGS(offset_calc_instr) & FL_PREINDEX
               && !(ARM_INS_FLAGS(offset_calc_instr) & FL_IMMED), ("did not expect this kind of offset calculation for @I: @I", last, offset_calc_instr));

        /* find index register */
        t_reg index_reg = (ARM_INS_REGB(offset_calc_instr) == ARM_INS_REGA(table_addr_prod)) ? ARM_INS_REGC(offset_calc_instr) : ARM_INS_REGB(offset_calc_instr);

        /* now try to find the upper bound of this switch table */
        detected = ArmFindSwitchTableUpperBoundCmp(cfg, BBL_PREV(bbl), index_reg, &upper_bound, bbl, offset_calc_instr);

        /* process data-BBL */
        BBL_SET_ALIGNMENT(databbl, 4);
        BBL_SET_ALIGNMENT_OFFSET(databbl, 0);

        if (detected)
        {
          /* iterate over elements in data-BBL */
          t_uint32 tel;
          t_arm_ins * data = NULL;
          for (tel = 0, data = T_ARM_INS(BBL_INS_FIRST(databbl)); tel < upper_bound; tel++, data = ARM_INS_INEXT(data))
          {
            t_reloc_ref * ref = ARM_INS_REFERS_TO(data);

            t_address dest_address = AddressAdd(BBL_CADDRESS(databbl),
                                        ARM_INS_IMMEDIATE(data));

            t_bbl * dest_bbl = find_close_bbl_by_address(databbl, dest_address);
            ASSERT(dest_bbl, ("unable to find destination BBL for @I, case %d (@I)", last, tel, data));

            RELOC_SET_HELL(RELOC_REF_RELOC(ref), FALSE);

            t_cfg_edge * edge = CfgEdgeCreate(cfg, bbl, dest_bbl, ET_SWITCH);
            CFG_EDGE_SET_SWITCHVALUE(edge, tel);
            CFG_EDGE_SET_REL(edge, RELOC_REF_RELOC(ref));
            RELOC_SET_SWITCH_EDGE(RELOC_REF_RELOC(ref), edge);
          }
        }
        else
        {
          /* In case the size of this switch table could not be determined, create a jump to HELL.
           * The reason for this is that we currently don't support
           * complicated nested switch tables, e.g., where index registers
           * alias across different switch statements. */
          CfgEdgeCreate(cfg, bbl, CFG_HELL_NODE(cfg), ET_IPJUMP);
        }
      }
        else if (IsThumbTableBranch(bbl, &table_entry_size)
                 || IsThumb1GnuCaseDispatch(obj, bbl, &table_entry_size2, &multiplier, &is_sign_extend))
        {
          t_uint32 offset, curdataval, dataentrymask,dataentryshift;
          t_uint32 i, bytes_left_in_data;
          t_arm_ins * ins_split, *curdata;
          t_bbl * tmp, * checkbbl, * databbl;//, * insbbl;
          t_address base_address;
          t_reg switchcmpreg;
          t_bbl **switchbbls;

          t_arm_ins * tbins = T_ARM_INS(BBL_INS_LAST(bbl));
          ASSERT((table_entry_size != 0) ^ (table_entry_size2 != 0), ("did not detect a switch table where one is expected for @I (%d %d)", tbins, table_entry_size, table_entry_size2));

          /* take into account the fact that we're handling multiple cases here */
          t_bool is_thumb1 = FALSE;
          t_uint32 sign_bit = 0;

          if (table_entry_size2)
          {
            is_thumb1 = TRUE;
            table_entry_size = table_entry_size2;
            switchcmpreg = ARM_REG_R0;

            if (is_sign_extend){
              sign_bit = table_entry_size * 8;
              ARM_INS_SET_FLAGS(tbins, ARM_INS_FLAGS(tbins) | FL_SWITCHEDBL_SIGNEXT);
            }
            ARM_INS_SET_FLAGS(tbins, ARM_INS_FLAGS(tbins) | (table_entry_size >> 1) << FL_SWITCHEDBL_SZ_SHIFT);
          }
          else
          {
            is_thumb1 = FALSE;
            multiplier = 2;
            switchcmpreg = ARM_INS_REGC(tbins);
            ASSERT(ARM_INS_REGB(tbins) == ARM_REG_R15, ("TBB/TBH switch table detected with base register different from the PC @I", tbins));
          }

          checkbbl = BBL_PREV(bbl);
          ArmFindSwitchTableUpperBoundCmp(cfg, checkbbl, switchcmpreg, &upper_bound, bbl, NULL);

          databbl = BBL_NEXT(bbl);
          BBL_SET_ALIGNMENT(databbl,0);
          BBL_SET_ALIGNMENT_OFFSET(databbl,0);
          //                insbbl = BBL_NEXT(databbl);
          //DEBUG(("First instruction after table @I", BBL_INS_FIRST(insbbl)));

          /* "add pc, r1" in thumb -> pc + 4 */
          base_address = AddressAddUint32(INS_CADDRESS(BBL_INS_LAST(bbl)),4);
          /* keep track of location switch bbls so we can add relocations for
          * them in the switch table
          */
          switchbbls = Calloc(sizeof(t_bbl*),upper_bound);

          curdata = T_ARM_INS(BBL_INS_FIRST(databbl));
          curdataval = ARM_INS_IMMEDIATE(curdata);
          bytes_left_in_data = G_T_UINT32(ARM_INS_CSIZE(curdata));
          if (table_entry_size == 1)
          {
            dataentryshift = 8;
            dataentrymask = 0xff;
          }
          else if (table_entry_size == 2)
          {
            dataentryshift = 16;
            dataentrymask = 0xffff;
          }
          else if (table_entry_size == 4)
          {
            dataentryshift = 32;
            dataentrymask = 0xffffffff;

            ASSERT(AddressAnd(BBL_OLD_ADDRESS(databbl), AddressNew32(0x3)) == 0, ("expected 4-byte aligned data BBL for 4-byte switched BL @I, got @G", tbins, BBL_OLD_ADDRESS(databbl)));
          }
          else
            FATAL(("Unsupported table entry size %d for @I", table_entry_size, tbins));

          for (i=0;i<upper_bound;i++)
          {
            t_cfg_edge * edge;
            t_reloc *rel;
            char reloc_code[200], loadop, storeop;
            t_uint32 mask;
            t_reloc_ref * ref;

            offset = (curdataval & dataentrymask) * multiplier;
            if (is_thumb1 && is_sign_extend)
              offset = Uint32SignExtend(offset, sign_bit);
            curdataval >>= dataentryshift;
            ins_split = T_ARM_INS(ObjectGetInsByAddress(obj, AddressAddUint32(base_address,offset)));
            ASSERT(ins_split, ("oops, ins_split should not be NULL for entry %d out of %d!", i, upper_bound-1));

            if (!(ARM_INS_ATTRIB(ins_split) & IF_BBL_LEADER))
            {
              VERBOSE(1,("splitting thumb switch bbl at @G: @eiB",ARM_INS_OLD_ADDRESS(ins_split),ARM_INS_BBL(ins_split)));
              tmp = BblSplitAtIns(ARM_INS_BBL(ins_split), T_INS(ins_split));
            }
            else
              tmp = ARM_INS_BBL(ins_split);
            switchbbls[i]=tmp;
            /* add relocation to (re)calculate offset in switch table for this bbl. Since there
            * are no byte load/store instructions in the relocation stack engine, we have to
            * some masking and shifting
            */
            /* depending on the size of the current data entry we load 16 or 32 bits */
            switch (ARM_INS_CSIZE(curdata))
            {
            case 2:
              loadop='k';
              storeop='v';
              break;
            case 4:
              loadop='l';
              storeop='w';
              break;
            default:
              FATAL(("Unsupported data size in thumb switch table: %d of @I",ARM_INS_CSIZE(curdata),curdata));
              break;
            }
            /* mask out the byte we want to change */
            mask=~(dataentrymask << ((ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8));

            /* get the right sanity check code */
            if (is_thumb1)
            {
              if (table_entry_size == 4)
              {
                /* any relocation value is valid */
                snprintf(reloc_code,sizeof(reloc_code),"R00R01Z01+i0000babe+i0000babe--\\=s0001>s%.4x<%ci%.8x&|%c\\i00000000&$",
                (ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8,
                loadop,
                mask,
                storeop);
              }
              else
              {
                /* only support relocation values within specified range, take into account possible sign extension */
                snprintf(reloc_code,sizeof(reloc_code),"R00R01Z01+i0000babe+i0000babe--\\=s0001>s%.4x<%ci%.8x&|%c\\i%.8x+i%.8x&$",
                (ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8,
                loadop,
                mask,
                storeop,
                (is_sign_extend) ? 1<<(dataentryshift) : 0,
                ~(dataentrymask<<1));
              }
            }
            else
            {
              /* components:
              *  o begin: calculate the difference between the "pc" value calculated by the last
              *    instruction of the bbl, and the target label. Since the used "pc" value of
              *    the instruction will be pc+4, and since R01Z01+ == pc+2, subtract an extra 2.
              *    The >>1 is because the table contains the offsets divided by 2.
              *  o s%.4x<:  shift the calculated value to the right position in the table constant
              *  o %c: load the constant (16 or 32 bits)
              *  o i%.8x&|: mask out the byte we calculated and or with the new value
              *  o %c: write the new constant (16 or 32 bits)
              *  o check part: make sure the result is even and fits in one byte after >>1
              */
              snprintf(reloc_code,sizeof(reloc_code),"R00R01Z01+i0000babe+i0000babe--\\=s0001>s%.4x<%ci%.8x&|%c\\i%.8x&$",
              (ARM_INS_CSIZE(curdata)-bytes_left_in_data)*8,
              loadop,
              mask,
              storeop,
              ~(dataentrymask<<1));
            }

            ref=BBL_REFED_BY(tmp);
            while (ref)
            {
              t_reloc_ref * next = RELOC_REF_NEXT(ref);
              if (StringPatternMatch("R00A01+R01A02+-i00000002/\\l*w\\s0000$", RELOC_CODE(RELOC_REF_RELOC(ref))))
              {
                //                  DEBUG(("removing @R",RELOC_REF_RELOC(ref)));
                RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(ref));
              }
              ref=next;
            }

            rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(curdata), AddressNullForObject(obj), T_RELOCATABLE(tmp), AddressNullForObject(obj), FALSE, NULL, NULL, NULL, reloc_code);
            RelocAddRelocatable(rel,T_RELOCATABLE(bbl),AddressNullForBbl(bbl));
            //            DEBUG(("added @R",rel));

            bytes_left_in_data-=table_entry_size;
            if (!bytes_left_in_data)
            {
              curdata = ARM_INS_INEXT(curdata);
              if (!curdata)
              {
                ASSERT(i==upper_bound-1,("Ran out of switch table entries for @iB",bbl));
              }
              else
              {
                bytes_left_in_data = ARM_INS_CSIZE(curdata);
                curdataval = ARM_INS_IMMEDIATE(curdata);
              }
            }

            edge=CfgEdgeCreate(cfg,bbl,tmp,ET_SWITCH);
            CFG_EDGE_SET_SWITCHVALUE(edge, i);
            RELOC_SET_HELL(rel, FALSE);
            CFG_EDGE_SET_REL(edge, rel);
            RELOC_SET_SWITCH_EDGE(rel, edge);
          }

          Free(switchbbls);
        }
        else if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl)))==ARM_MOV)
        {
                /* step 1: count branch instructions following ARM_MOV */
                t_uint32 nr_targets = 0;
                t_uint32 tel = 0;
                t_bbl *branch_bbl = BBL_NEXT(bbl);
                t_arm_ins * i_ins = NULL;
                t_arm_ins * addlsl_ins = NULL;
                t_arm_ins * addrprod_ins = NULL;
                t_address addrprod_tgt = AddressNew32(0);
                t_address addrprod_pc = AddressNew32(0);
                t_uint32 upper_bound = 0;

                while (BBL_NINS(branch_bbl) == 1 && ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_FIRST(branch_bbl))) && ARM_INS_CSIZE(T_ARM_INS(BBL_INS_FIRST(branch_bbl)))==4)
                {
                        nr_targets++;
                        branch_bbl = BBL_NEXT(branch_bbl);
                }

                /* step 2: check ADD LSL ins */
                BBL_FOREACH_ARM_INS_R(bbl, i_ins)
                {
                        /* look for the instruction that calculates the branch address */
                        if (ARM_INS_OPCODE(i_ins) == ARM_ADD
                                && ARM_INS_SHIFTTYPE(i_ins) == ARM_SHIFT_TYPE_LSL_IMM
                                && ARM_INS_SHIFTLENGTH(i_ins) == 2
                                && ARM_INS_REGA(i_ins) == ArmPropagateRegBackwardStack(ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(bbl))), T_ARM_INS(BBL_INS_LAST(bbl)), i_ins))
                        {
                                /* ADD LSL found */
                                addlsl_ins = i_ins;
                        }

                        /* look for the address producer that calculates the base address of the branch table */
                        if (addlsl_ins
                                && ARM_INS_OPCODE(i_ins) == ARM_ADD
                                && ARM_INS_REGA(i_ins) == ARM_INS_REGB(addlsl_ins)
                                && ARM_INS_REGB(i_ins) == ARM_REG_R15
                                && (ARM_INS_FLAGS(i_ins) & FL_IMMED))
                        {
                                /* address producer found */
                                addrprod_ins = i_ins;
                        }
                }
                ASSERT(addlsl_ins, ("recognized branch table @I, but could not find an ADD-LSL-immediate instruction", BBL_INS_LAST(bbl)));
                ASSERT(addrprod_ins, ("recognized branch table @I, but could not find its address producer", BBL_INS_LAST(bbl)));

                /* step 3: check address producer of branch table */
                addrprod_pc = AddressAnd(ARM_INS_OLD_ADDRESS(addrprod_ins), AddressNew32(~0x3));
                addrprod_tgt = AddressAddUint32(AddressAddUint32(addrprod_pc, ARM_INS_IMMEDIATE(addrprod_ins)),
                                                        (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(branch_bbl))) & FL_THUMB) ? 4 : 8);
                ASSERT(AddressIsEq(BBL_OLD_ADDRESS(BBL_NEXT(bbl)), addrprod_tgt),
                        ("unexpected target of address producer for @iB (address producer @I): expected @G, but got @G", bbl, addrprod_ins, BBL_OLD_ADDRESS(BBL_NEXT(bbl)), addrprod_tgt));

                /* step 4: check upper bound (CMP-instruction) */
                ArmFindSwitchTableUpperBoundCmp(cfg,BBL_PREV(bbl),ARM_INS_REGC(addlsl_ins),&upper_bound, bbl, addlsl_ins);
                ASSERT(upper_bound == nr_targets, ("number of found targets (%d) is not equal to the number of calculated targets, via CMP (%d) for @iB",nr_targets,upper_bound,bbl));

                branch_bbl = BBL_NEXT(bbl);
                while (BBL_NINS(branch_bbl) == 1 && ArmInsIsUnconditionalBranch(T_ARM_INS(BBL_INS_FIRST(branch_bbl))) && ARM_INS_CSIZE(T_ARM_INS(BBL_INS_FIRST(branch_bbl)))==4)
                {
                        t_cfg_edge * edge = CfgEdgeCreate(cfg,bbl,branch_bbl,ET_SWITCH);
                        CFG_EDGE_SET_SWITCHVALUE(edge, tel++);
                        branch_bbl = BBL_NEXT(branch_bbl);
                }
        }

      else FATAL(("Unknown switch jump statement @I!",BBL_INS_LAST(bbl)));
    }
  }
}
/* }}} */

t_bool DidNotFindPredecessingCopyOfAddressIntoR14(t_bbl * bbl)
{
#if 1
  t_arm_ins * ins;
  t_arm_ins * block_end = T_ARM_INS(BBL_INS_LAST(bbl));

  BBL_FOREACH_ARM_INS_R(bbl,ins)
    if (ins!=block_end)
      {
	if (ARM_INS_OPCODE(ins) == ARM_LDM && ARM_INS_REGB(ins) == ARM_REG_R13 &&
	    RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R14))
	  break;
	else if (ARM_INS_OPCODE(ins) == ARM_LDR && ARM_INS_REGB(ins) == ARM_REG_R13 &&
	    RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R14))
	  break;
	else if (/*ARM_INS_OPCODE(ins) == ARM_MOV &&*/ ARM_INS_REGA(ins) == ARM_REG_R14)
	  {
            if (!block_end || !(ARM_INS_ATTRIB(block_end)&IF_SWITCHJUMP))
              VERBOSE(0,("OOPS SHOULD NOT HAPPEN ON KERNEL @iB\n",ARM_INS_BBL(ins)));
	    return FALSE;
	  }
      }
#endif
  return TRUE;
}
  

/*!
 * Checks whether we can successfully interpret a Thumb instruction sequence
 * that starts with "mov rx, pc"
 *
 * \param ins the "mov rx, pc" instruction
 * \param leader if found/relevant, the bbl leader instruction referred to
 *   by the sequence starting with ins will be returned in this parameter
 *   (can be NULL on input)
 *
 * \returns TRUE if the sequence has been successfully interpreted and the
 *    relevant basic block leaders have been marked
 *    
 */
/* ThumbCheckMovPcLeader {{{ */
static t_bool
ThumbCheckMovPcLeader(t_object *obj, t_arm_ins *ins, t_arm_ins **leader)
{
  t_arm_ins *iter, *target;
  t_address target_address;
  t_bool finished, found;

  /* find whether the destination register is still modified later
   */
  iter=ins;
  finished=FALSE;
  found=FALSE;
  while (!finished &&
         ((iter=ARM_INS_INEXT(iter))))
  {
    if (RegsetIn(ARM_INS_REGS_DEF(iter),ARM_INS_REGA(ins)))
    {
      switch (ARM_INS_OPCODE(iter))
      {
        case ARM_SUB:
	case ARM_ADD:
	{
	  t_int32 addvalue;
	  ASSERT(ARM_INS_FLAGS(iter) & FL_IMMED,("Non-constant offset calculation from pc mov '@I': @I",ins,iter));
	  addvalue=ARM_INS_IMMEDIATE(iter);
	  if (ARM_INS_OPCODE(iter)==ARM_SUB)
	    addvalue=-addvalue;
	  target_address=AddressAddUint32(ARM_INS_CADDRESS(ins),4+addvalue);
	  target=T_ARM_INS(ObjectGetInsByAddress(obj,target_address));
	  ASSERT(target,("Data processing operation produced an address that lies out of the section:\n@I",ins));
          if ((!(ARM_INS_ATTRIB(target) & IF_DATA)))
	  {
	    VERBOSE(2,("Making @I Thumb-add/sub leader",target));
            ARM_INS_SET_ATTRIB(target,  ARM_INS_ATTRIB(target) | IF_BBL_LEADER);
	    found=TRUE;
	  }
	  if (leader)
	    *leader=target;
	  finished=TRUE;
	  break;
	}
        default:
	  finished=TRUE;
	  break;
      }
    }
  }
  return found;
}
/* }}} */
/*!
 * Add edges between the newly created basic blocks. Both the lineair (array)
 * representation of the instructions and the basic block representation are
 * available: you can access the (copy) basic block by using ARM_INS_BBL of a
 * leader in the lineair representation.
 *
 * Although connecting the blocks isn't that hard, it could be difficult to
 * find out if a given jump instructions is a call or just a jump (without
 * return). Getting this completly safe is probably not what you want: if you
 * implement this completly safe, you'll probably end up with one big
 * function, which will degrade the performance of the rest of diablo or hell
 * nodes after each jump which will degrade the performance even further.
 *
 * On the other hand, if you get this wrong (e.g add a jump edge where there
 * should have been a call edge), then chances are that your flowgraph (and
 * thus the produced binary) is wrong (the successor block of the call block
 * will have no incoming (return) edge, and thus this block is dead). 
 *
 * The solution is to look at code that is produced by your compiler(s).
 * It'll probably have only a few possibilties to produce calls. If you
 * handle them as safe as possible, than you're off the hook. 
 *
 * \param section The disassembled section, with basic blocks already created
 * we want to flowgraph
 *
 * \return t_uint32 
*/

/* ArmAddBasicBlockEdges {{{ */
static t_uint32 ArmAddBasicBlockEdges(t_object *obj)
{
  t_arm_ins *ins, * iter, *stop, * block_end, * block_start, * jump_target;
  t_bbl * block, * ft_block;
  t_bbl * bbbl;
  t_cfg_edge * edge, * j_edge;
  t_cfg * cfg = OBJECT_CFG(obj);
  t_bool flag;
  t_reloc_ref * ref;
  t_uint32 nedges=0;
  t_section *section;
  t_uint32 i;

  OBJECT_FOREACH_CODE_SECTION(obj, section, i)
  {
    /* walk through the original(!) array (we need offsets) and add 
     * all possible successors to every node */

    SECTION_FOREACH_ARM_INS(section, ins)
    {
      /* we're only interested in the first and last instructions of a basic block */

      if (!(ARM_INS_ATTRIB(ins) & IF_BBL_LEADER)) 
	continue;

      block_start = ins;
      block=ARM_INS_BBL(block_start);
      block_end=block_start;
      for (iter = ARM_INS_INEXT(block_start); iter && !(ARM_INS_ATTRIB(iter) & IF_BBL_LEADER); iter = ARM_INS_INEXT(iter))
      {
	block_end = iter;
      }


      /* block_end now points to the last "real" instruction of the block in the linear list,
       * iter points to the first instruction of the next block (or NULL if there is no next block) */
      ft_block = iter ? ARM_INS_BBL(iter) : NULL;


      /* 1. ADD EDGES FOR RELOCATIONS: 
       * 
       * each relocation gets its own edge. These edges will be automatically
       * deleted when the relocation is removed. There's no need to add an edge
       * for the obvious relocations (such as the relocations that are present
       * for the calculation of the offsets in intermodular jumps), since they
       * are completly described by the edge itself (the reason for the edge is
       * that there is a jump, not that there is a relocation) 
       *
       * {{{ */

      for (ref=BBL_REFED_BY(block); ref!=NULL; ref=RELOC_REF_NEXT(ref))
      {
	t_bool add_edge = FALSE;
	t_reloc *rel = RELOC_REF_RELOC (ref);

	if (ARM_INS_TYPE(block_start) != IT_DATA)
	{
#define expanded_pc24	"R00A01+P-A00+\\= s0002 & % s0002 > i00ffffff &=l iff000000 &| R00A01+M?i10000000|ifeffffff& } s0017 < |: }* ! w\\l i00ffffff &-$"

	  if (RELOC_HELL(rel))
	    add_edge = TRUE;
	  else
	  {
	    if (!strcmp(RELOC_CODE(rel), expanded_pc24))
	      if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel))
		    == RT_INS &&
		    ARM_INS_TYPE(T_ARM_INS(RELOC_FROM(rel)))
		    == IT_DATA) ||
		  (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel))
		   == RT_SUBSECTION))
	      {
		/* this is a special case added for kernel optimization, but
		 * it might be useful more generally as well: this makes sure
		 * hell edges get added for PC24 relocations coming from data
		 * blocks. Normally you don't expect these relocations in data
		 * blocks, but the only way we have to handle self-modifying code
		 * is to treat it as a data block, and this results in this kind
		 * of weirdness. */
		add_edge = TRUE;
		RELOC_SET_HELL(rel, TRUE);
	      }
	  }
	}
#undef expanded_pc24

	/* Skip the obvious relocations and reloctions to data */

	if (add_edge)
	{
	  /* Some extra checks if the relocation is from an instruction */
	  if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel))==RT_INS)
	  {
	    t_cfg_edge * found=NULL;
	    t_cfg_edge * i_edge;
	    if (RELOC_REF_NEXT(RELOCATABLE_REFERS_TO(RELOC_FROM(rel))))
	    {
	      VERBOSE(1,("1. @R\n",RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(RELOC_FROM(rel)))));
	      VERBOSE(1,("2. @R\n",RELOC_REF_RELOC(RELOC_REF_NEXT(RELOCATABLE_REFERS_TO(RELOC_FROM(rel))))));
	      VERBOSE(1,("Multiple relocs from instruction?"));
	    }
	    //if ((RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(RELOC_FROM(rel))))!=rel) FATAL(("Relocation wrong"));


	    BBL_FOREACH_PRED_EDGE(block,i_edge)
	    {
	      if ((CFG_EDGE_HEAD(i_edge)==CFG_HELL_NODE(cfg)) && (CFG_EDGE_CAT(i_edge) == ET_CALL))
	      {
		found=i_edge;
		break;
	      }
	    }

	    if (found)
	    {
	      RELOC_SET_EDGE(rel, found);
	      CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found)+1);
	      /* also increment the refcount of the corresponding return edge!!! */
	      ASSERT(CFG_EDGE_CORR(found),("Call edge @E does not have a corresponding edge!",found));
	      CFG_EDGE_SET_REFCOUNT( CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT( CFG_EDGE_CORR(found)) + 1);
	    }
	    else
	    {
	      RELOC_SET_EDGE(rel, CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), block,NULL,NULL));
	      nedges++;
	    }
	    /* Some comments on this */
	    VERBOSE(1,("Adding INS arrow from hell! (relocation @R, @B)",rel,block));
	  }
	  else
	  {
	    t_cfg_edge * found=NULL;
	    t_cfg_edge * i_edge;
	    /* Add the edge */
	    BBL_FOREACH_PRED_EDGE(block,i_edge)
	    {
	      if ((CFG_EDGE_HEAD(i_edge)==CFG_HELL_NODE(cfg)) && (CFG_EDGE_CAT(i_edge) == ET_CALL))
	      {
		found=i_edge;
		break;
	      }
	    }

	    if (found)
	    {
	      RELOC_SET_EDGE(rel, found);
	      CFG_EDGE_SET_REFCOUNT(found, CFG_EDGE_REFCOUNT(found)+1);
	      /* also increment the refcount of the corresponding return edge!!! */
	      ASSERT(CFG_EDGE_CORR(found),("Call edge @E does not have a corresponding edge!",found));
	      CFG_EDGE_SET_REFCOUNT( CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT( CFG_EDGE_CORR(found)) + 1);
	    }
	    else
	    {

	      RELOC_SET_EDGE(rel, CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), block,NULL,NULL));
	      nedges++;
	    }
	    VERBOSE(1,("Adding arrow from hell! (relocation @R, @B)",rel,block));
	  }
	}
      }
      /* }}} */

      /* 2. ADD NORMAL EDGES */
      /* now determine the successors of the block and their types */

      switch (ARM_INS_TYPE(block_end)) 
      {
	case IT_BRANCH:
	  { /* {{{ */
	    /* If the branch is conditional, add a fall-through edge */
	    if ((ft_block != NULL && !IS_DATABBL(ft_block) &&
                (ARM_INS_CONDITION(block_end) != ARM_CONDITION_AL || ARM_INS_OPCODE(block_end)==ARM_T2CBZ || ARM_INS_OPCODE(block_end)==ARM_T2CBNZ)))
	    {
	      CfgEdgeCreate(cfg, block,ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }

	    /* Add edge for the jump, dependent on the type of jump: BL adds
	     * calls (unless return is really impossible), B adds jumps */
	    switch (ARM_INS_OPCODE(block_end)) 
	    {
	       case ARM_T2CBZ:
               case ARM_T2CBNZ:
              case ARM_B:
		{
		  /* get the instruction we're jumping to */
		  t_address target_address=AddressAddUint32(ARM_INS_CADDRESS(block_end),((ARM_INS_FLAGS(block_end) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(block_end));
		  jump_target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));

		  if (jump_target)
		  {
		    CfgEdgeCreate(cfg, block, ARM_INS_BBL(jump_target),ET_JUMP);
		    nedges++;
		  }
		  else 
		  {
		    t_object * lo = ObjectGetLinkerSubObject(obj);	
		    t_section * sec = SectionGetFromObjectByAddress(lo, target_address);

		    if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
		    {
		      t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
		      t_section * rel  =  SectionGetFromObjectByName(lo, relname);
		      t_bbl *dyncallhell = CfgGetDynamicCallHell(cfg, SECTION_NAME(sec) + 8);
		      Free(relname);
		      ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));
		      CfgEdgeCreate(cfg, block, dyncallhell, ET_JUMP);
		      nedges++;
		    }
		    else
		    {
		      FATAL( ("Jump target not found for @I", block_end));
		    }
		  }
		}
                break;

	      case ARM_BL:
		{

		  /* get the instruction we're jumping to */
		  t_address target_address=AddressAddUint32(ARM_INS_CADDRESS(block_end),((ARM_INS_FLAGS(block_end) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(block_end));
		  jump_target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));

		  if (!jump_target)
		  {
		    t_object * lo = ObjectGetLinkerSubObject(obj);
		    t_section * sec = SectionGetFromObjectByAddress(lo, target_address);

		    if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
		    {
		      t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
		      t_section * rel  =  SectionGetFromObjectByName(lo, relname);
		      t_bbl *dyncallhell = CfgGetDynamicCallHell(cfg, SECTION_NAME(sec) + 8);
		      Free(relname);
		      ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));
		      CfgEdgeCreateCall(cfg, block, dyncallhell, ft_block, NULL);
		    }
		    else
		    {
		      WARNING(("Unrecognised call into non-code section from @I, changing into regular call hell",block_end));
		      CfgEdgeCreateCall(cfg, block, CFG_HELL_NODE(cfg), ft_block, NULL);
		    }
		    nedges++;
		  }
		  else
		  {
		    if ((ft_block != NULL)&&(!IS_DATABBL(ft_block)))
		    {
		      CfgEdgeCreateCall(cfg, block,ARM_INS_BBL(jump_target),ft_block,NULL);
		      nedges++;
		    }
		    else
		    {
		      CfgEdgeCreate(cfg, block, ARM_INS_BBL(jump_target),ET_JUMP);
		      nedges++;
		    }
		  }
		}
		break;

	      case ARM_BX:
		if (ARM_INS_REGB(block_end)==ARM_REG_R14)
		{
		  ArmInsMakeMov(block_end,ARM_REG_R15,ARM_REG_R14,0,ARM_INS_CONDITION(block_end));
		  CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
		  nedges++;
		}
		else if (ARM_INS_REGB(block_end)==ARM_REG_R15)
		{
		  t_address target_address=AddressAddUint32(ARM_INS_CADDRESS(block_end),((ARM_INS_FLAGS(block_end) & FL_THUMB)?4:8));
		  jump_target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));
		  if (ARM_INS_FLAGS(block_end) & FL_THUMB)
		  {
		    CfgEdgeCreate(cfg, block, ARM_INS_BBL(jump_target), ET_FALLTHROUGH);
		    nedges++;
		    ARM_INS_SET_OPCODE(block_end,  TH_BX_R15);
		    ARM_INS_SET_REGB(block_end,  ARM_REG_NONE);
		    ARM_INS_SET_REGS_USE(block_end,  ArmUsedRegisters(block_end));
		    if(ARM_INS_COPY(block_end))
		    {
		      ARM_INS_SET_OPCODE(ARM_INS_COPY(block_end),  TH_BX_R15);
		      ARM_INS_SET_REGB(ARM_INS_COPY(block_end),  ARM_REG_NONE);
		      ARM_INS_SET_REGS_USE(ARM_INS_COPY(block_end),  ArmUsedRegisters(block_end));
		    }
		  }
		  else
		  {
		    CfgEdgeCreate(cfg, block, ARM_INS_BBL(jump_target), ET_JUMP);
		    nedges++;
		    if (ARM_INS_COPY(block_end) != NULL)
		    {
		      ARM_INS_SET_FLAGS(ARM_INS_COPY(block_end),   ARM_INS_FLAGS(ARM_INS_COPY(block_end)) | FL_IMMED);
		      ARM_INS_SET_OPCODE(ARM_INS_COPY(block_end),  ARM_B);
		      ARM_INS_SET_REGB(ARM_INS_COPY(block_end),  ARM_REG_NONE);
		      ARM_INS_SET_IMMEDIATE(ARM_INS_COPY(block_end),  Uint32SignExtend(0xffc, 10));
		    }
		    ARM_INS_SET_FLAGS(block_end,  ARM_INS_FLAGS(block_end)| FL_IMMED);
		    ARM_INS_SET_OPCODE(block_end,  ARM_B);
		    ARM_INS_SET_REGB(block_end,  ARM_REG_NONE);
		    ARM_INS_SET_IMMEDIATE(block_end,  Uint32SignExtend(0xffc, 10));
		  }
		}
		else
		{
		  t_arm_ins * backtrack = ARM_INS_COPY(block_end);
		  if (!backtrack) FATAL(("Instruction datastructures corrupt"));
		  backtrack = ARM_INS_IPREV(backtrack);

		  while (backtrack)
                    {
		    if (ARM_INS_REGA(backtrack)==ARM_REG_R14) break;
                    else if (RegsetIn(ARM_INS_REGS_DEF(backtrack),ARM_REG_R14))
                      backtrack = NULL; // we did not find the instruction we were looking for, but some other that surely voids the search ...
		    else
		      backtrack = ARM_INS_IPREV(backtrack);
		  }
                  
                  /* when a value is loaded into R14, for sure the goal of the BX is not a call */
                  if (backtrack && ARM_INS_OPCODE(backtrack)==ARM_LDR)
                    backtrack = NULL;

		  if (backtrack && !(ARM_INS_CONDITION(backtrack)==ARM_CONDITION_AL) && !(ARM_INS_CONDITION(backtrack)==ARM_INS_CONDITION(block_end)) && ARM_INS_IPREV(backtrack) != NULL)
		  {
		    t_arm_ins * inverse = ARM_INS_IPREV(backtrack);
		    while (!(ARM_INS_REGA(inverse)==ARM_REG_R14 || ARM_INS_IPREV(inverse) == NULL))
		      inverse = ARM_INS_IPREV(inverse);
		    if (ARM_INS_IPREV(inverse) == NULL)
		      FATAL(("Instruction @I is conditional and shouldn't be", backtrack));
		    if (!(ARM_INS_CONDITION(backtrack)==ArmInvertCondition(ARM_INS_CONDITION(inverse))))
		      FATAL(("Instructions @I and @I have overlapping conditions!", backtrack, inverse));
		  }
		  if (backtrack && ARM_INS_REGA(backtrack)==ARM_REG_R14)
		  {
                    //                    DEBUG(("HOLA: created call for ins @I backtrack @I",block_end,backtrack)); 
                    CfgEdgeCreateCall(cfg, block, CFG_HELL_NODE(cfg), ft_block, NULL);
		    nedges++;
		    break;
		  }
		  backtrack = ARM_INS_IPREV(ARM_INS_COPY(block_end));	/* Look for POP {rx}, BX rx pair */
		  while (backtrack)
		  {
		    if (ARM_INS_OPCODE(backtrack) == ARM_LDM &&
			ARM_INS_TYPE(backtrack) == IT_LOAD_MULTIPLE &&
			ARM_INS_REGB(backtrack) == ARM_REG_R13 &&
			(ARM_INS_IMMEDIATE(backtrack) >> ARM_INS_REGB(block_end)) & 1)
		      break;
		    else
		      backtrack = ARM_INS_IPREV(backtrack);
		  }
		  if (backtrack && ARM_INS_IPREV(backtrack) != NULL)
                    {
                      //                      DEBUG(("HOLA: created return for ins @I backtrack @I",block_end,backtrack)); 
                      CfgEdgeCreate(cfg, block, CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
                    }
		  else
		  {
                    //                    DEBUG(("HOLA: created jump for ins @I",block_end)); 
		    CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		  }
		  nedges++;
		}
		break;
	      case ARM_BLX:
		if (ARM_INS_REGB(block_end) != ARM_REG_NONE)
		{
		  CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
		  nedges++;
                  VERBOSE(10,("jump from @I, regb: %d",block_end, ARM_INS_REGB(block_end)));
		}
		else
		{
		  t_address target_address=AddressAddUint32(ARM_INS_CADDRESS(block_end),((ARM_INS_FLAGS(block_end) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(block_end));
		  jump_target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));
		  if (!jump_target)
		  {
		    /* call to e.g. PLT entry */
		    CfgEdgeCreateCall(cfg, block, CFG_HELL_NODE(cfg), ft_block, NULL);
		    nedges++;
		  }
		  else if ((ft_block != NULL)&&(!IS_DATABBL(ft_block)))
		  {
		    CfgEdgeCreateCall(cfg, block,ARM_INS_BBL(jump_target),ft_block,NULL);
		    nedges++;
		  }
		  else
		  {
		    CfgEdgeCreate(cfg, block, ARM_INS_BBL(jump_target),ET_JUMP);
		    nedges++;
		  }

		}
		break;
	      default:
		break;
	    }
	    break;
	  } /* }}} */

	  /* SWI's go to SWI-hell */
	case IT_SWI:
	  /* {{{ */
#ifdef ARMFG_ASSUME_SWI_ADS
	  if (ft_block != NULL)
	  {
	    CfgEdgeCreate(cfg, block,ft_block, ET_FALLTHROUGH);
	    nedges++;
	  }
#else
	  /* If the SWI is conditional, we add a fall-throug edge */
	  if ((ft_block != NULL)&&(!IS_DATABBL(ft_block))&& (ARM_INS_CONDITION(block_end) != ARM_CONDITION_AL))
	  {
	    CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
	    nedges++;
	  }
	  /* since we don't know where a SWI jumps to, we'll add hell as a successor */
	  CfgEdgeCreateSwi(cfg, block, ft_block);
	  nedges++;
#endif
	  break;
	  /* }}} */

	  /* Dataprocessing instructions can be used as jumps on architectures with explicit program counters */
	case IT_DATAPROC:
	  /* {{{ */
	  if (ARM_INS_REGA(block_end) == ARM_REG_R15)
	  {
	    if ((ft_block != NULL)&&(!IS_DATABBL(ft_block))&& (ARM_INS_CONDITION(block_end) != ARM_CONDITION_AL))
	    {
	      CfgEdgeCreate(cfg, block,ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }

	    /* A dataprocessing instruction that define R15 (= the Program
	     * counter) => control flow altering instruction.  there are
	     * different possibilities: 
	     *
	     * 1) MOV pc,r14 is (probably) a function return, although THIS IS
	     * NOT SAFE (but a safe enough assumption)
	     *  
	     * 2) Any other operation is either a jump (or even a switch) or a
	     * function call depending on whether r14 holds the return address or
	     * not.
	     */

	    if ((ARM_INS_OPCODE(block_end) == ARM_MOV) 
		&& (ARM_INS_REGC(block_end) == ARM_REG_R14) 
		&& (ARM_INS_REGB(block_end) == ARM_REG_NONE) 
		&& (ARM_INS_IMMEDIATE(block_end)==0)
		&& DidNotFindPredecessingCopyOfAddressIntoR14(ARM_INS_BBL(block_end))
	       )
	    {
	      /* mov pc,r14: function return */

	      /* BJORN: this turns out not to be the case in manually
		 written Linux kernel assembler code */

	      CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
	      nedges++;
	    }
	    else if (ARM_INS_OPCODE (block_end) == ARM_SUB
		&& ARM_INS_REGB (block_end) == ARM_REG_R14
		&& (ARM_INS_FLAGS (block_end) & FL_IMMED)
		&& (ARM_INS_FLAGS (block_end) & FL_S))
	    {
	      /* return to lower privilege mode (e.g. interrupt handler return) */
	      CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
	      nedges++;
	    }
	    else if (ARM_INS_OPCODE (block_end) == ARM_SUB
		&& ARM_INS_REGB (block_end) == ARM_REG_R15
		&& (ARM_INS_FLAGS (block_end) & FL_IMMED)
		&& ARM_INS_IMMEDIATE (block_end) == 4)
	    {
	      /* jump to next instruction: clears the pipeline
	       * this is a system instruction, not a control flow instruction */
	      CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }
	    else
	    {
	      t_regset search=ARM_INS_REGS_USE(block_end);
	      RegsetSetSubReg(search,ARM_REG_R15);
	      RegsetSetSubReg(search,ARM_REG_CPSR);
	      stop = ARM_INS_IPREV(block_start);
	      /* skip data bbl if necessary */
	      if (stop &&
		  (ARM_INS_OPCODE(stop) == ARM_DATA))
	      {
		t_arm_ins *prev;
		prev = ArmGetPrevIns(obj,block_start);
		if (prev)
		  stop = T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(prev)));
		if (prev)
		  stop = ARM_INS_IPREV(prev);
	      }

	      if (ARM_INS_OPCODE(block_end) == ARM_ADD
		  && ARM_INS_SHIFTTYPE(block_end) == ARM_SHIFT_TYPE_LSL_IMM
		  && (ARM_INS_CONDITION(block_end) == ARM_CONDITION_LS
		    || ARM_INS_CONDITION(block_end) == ARM_CONDITION_CC
                    || ARM_INS_CONDITION(block_end) == ARM_CONDITION_LT))
	      {
		/* {{{ it's a switch */
		t_uint32 ncases = 0;

		for (iter = block_end; iter != stop; iter = ArmGetPrevIns(obj,iter))
		{
                  if (RegsetIn(search ,ARM_INS_REGB(iter)) && (ARM_INS_OPCODE(iter)==ARM_CMP))
		    break;
		}

		if (iter==stop) FATAL(("Bound not found! Switch approx at @I",iter));

                /* Iter contains the instruction that calculates the bounds check. We will now add switch edges for every switch */
                if (ARM_INS_FLAGS(iter) & FL_IMMED)
                      ncases = ARM_INS_IMMEDIATE(iter);
                else
                {
                        /* this is a CMP rX, rY instruction */
                        t_arm_ins * cmp_resolver;
                        t_reg propagated_reg = ARM_INS_REGC(iter);

                        /* WARNING WARNING WARNING
                         * This is a nasty hack.
                         *
                         * ----- BBL1
                         * movw ip, 0xXXX
                         * ...
                         * ----- BBL2
                         * ldr ...
                         * cmp r1, ip
                         * ADDLS pc, pc, r1 LSL 2
                         *
                         * If we want to be truly conservative here, we should not cross the basic block boundary.
                         * However, if the last instruction of the previous basic block is not a control flow instruction,
                         * we can assume that the compiler intends the transition between BBL1 and BBL2 to be a fallthrough path.
                         * So, under these circumstances, we can safely propagate backwards.
                         *
                         * This was an issue for the NAGRA use case with the Wandi diversified VM.
                         */
                        for(cmp_resolver = ArmGetPrevIns(obj,iter); !ArmIsControlflow(cmp_resolver); cmp_resolver = ArmGetPrevIns(obj,cmp_resolver))
                        {
                                if (RegsetIn(ARM_INS_REGS_DEF(cmp_resolver), propagated_reg)
                                    && ARM_INS_OPCODE(cmp_resolver)==ARM_MOVW)
                                {
                                        ncases = ARM_INS_IMMEDIATE(cmp_resolver);
                                        break;
                                }
                        }

                        if (cmp_resolver==stop) FATAL(("Bound not found (after register propagation)! Switch approx at @I",iter));
                }
                ASSERT(ncases > 0, ("what? @I", block_end));

		if (ARM_INS_CONDITION(block_end) == ARM_CONDITION_LS)
		  ncases++;

		CFG_FOREACH_BBL(cfg,bbbl)
		{
		  if ((G_T_UINT32(BBL_CADDRESS(bbbl))>G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins)))+BBL_NINS(ARM_INS_BBL(ins))*4)
		      && (G_T_UINT32(BBL_CADDRESS(bbbl))<=G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins)))+ncases*4+BBL_NINS(ARM_INS_BBL(ins))*4))
		  {
		    t_cfg_edge * e=CfgEdgeCreate(cfg, block,  bbbl,ET_SWITCH);
		    CFG_EDGE_SET_SWITCHVALUE(e, G_T_UINT32(AddressSub(BBL_CADDRESS(bbbl),ARM_INS_CADDRESS(block_end)))/4 - 2);
		    nedges++;
		  }
		}
		/* }}} */
	      }
	      else if (!(ARM_INS_ATTRIB(block_end)&IF_SWITCHJUMP))
	      {
		/* Find the instruction that defines r14 (= the return register)  */
		jump_target = NULL;
		for (iter = block_end; iter != stop; iter = ARM_INS_IPREV(iter))
		{
		  /* is r14 possibly just used like any other register in the calculation
		   * of a jump table address?
		   */
		  if (RegsetIn(ARM_INS_REGS_USE(iter),ARM_REG_R14) &&
		      !jump_target)
		    jump_target=iter;
		  if (RegsetIn(ARM_INS_REGS_DEF(iter),ARM_REG_R14))
		    break;
		}

		if ((iter != stop) && /* We found the definition of r14 */
		    /* check if r14 is used as any other register to load an index into
		     * a switch table (that was not detected as a switch table by diablo)
		     */
		    ((ARM_INS_OPCODE(block_end) != ARM_MOV) ||
		     !jump_target ||
		     !RegsetIn(ARM_INS_REGS_DEF(jump_target),ARM_INS_REGC(block_end))))
		{
		  /* if iter == ARM_INS_IPREV(block_end) and *iter = mov pc, r14 => function call
		   * if iter != ARM_INS_IPREV(block_end) and *iter = add pc, constant => function call
		   * if iter defines r14 independently from the pc => jump
		   * else FATAL!
		   */
		  if (!RegsetIn(ARM_INS_REGS_USE(iter) , ARM_REG_R15)) 
		  {
		    /* This is something really odd: r14 is defined, which indicates we're looking at an indirect function
		     * call, but the value of r14 isn't derived from the program counter, implicating that 
		     * function return will not bring us to the next instruction...
		     * Usually we want to come down with a FATAL in this situation but there is one exception:
		     * the indirect call could be an optimised tail call, in which case the definition of r14
		     * should have been a load from the stack.
		     */

		    if (((ARM_INS_OPCODE(iter) == ARM_LDR) || (ARM_INS_OPCODE(iter) == ARM_LDM)) && (ARM_INS_REGB(iter) == ARM_REG_R13))
		    {
		      /* tail call. should be viewed as a jump (will become interprocedural later on) */
		      /* the destination is the hell node, because we can't know the destination until after constant propagation */
		      CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		      nedges++;
		    }
		    else
		    {
		      t_arm_ins * iter2;

		      /* Some checks to see if the offset is loaded, in which case
		       * there shouldn't be a problem because we will have relocations pointing 
		       * to the possible targets */
		      for (iter2 = block_end; iter2 != stop; iter2 = ARM_INS_IPREV(iter2))
		      {
			if(RegsetIn(ARM_INS_REGS_DEF(iter2),ARM_INS_REGC(block_end)))
			  break;
		      }
		      /* This happens when using multiple computed gotos in
		       * a single function. This check could be enabled again
		       * if we start recognising that pattern.
		       if(ARM_INS_OPCODE(iter2) != ARM_LDR)
		       FATAL(("Check if this happens! This would mean we would return to somewhere else.... Quite dangerous I think.\n Instruction: @I\nBlock @iB\nLrins: @I\nIter2:@I",block_end,block,iter,iter2)); */
		      /* jump */
		      /* the destination is the hell node, because we can't know the destination until after constant propagation */
		      CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		      nedges++;
		    }
		  }
		  else if (((t_arm_ins *)iter) == ARM_INS_IPREV(block_end) 
		      && ARM_INS_OPCODE(iter) == ARM_MOV 
		      && ARM_INS_REGC(iter) == ARM_REG_R15)
		  {
		    /* function call */
		    if (!IS_DATABBL(ft_block))
		      /* In case code follows this instruction, we can assume it's a funtion call */
		    {
		      CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
		      nedges += 2;
		    }
		    else
		      /* otherwise it's certainly not a function call */
		    {
		      CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		      nedges++;
		    }
		  }
		  else if (ARM_INS_OPCODE(iter) == ARM_ADD
		      && ARM_INS_REGB(iter) == ARM_REG_R15
		      && (ARM_INS_FLAGS(iter) & FL_IMMED))
		  {
                    t_arm_ins *dest = T_ARM_INS(ObjectGetInsByAddress(obj, AddressAddUint32(
                                                                                AddressAddUint32(ARM_INS_CADDRESS(iter), ARM_INS_IMMEDIATE(iter)),
                                                                                (ARM_INS_FLAGS(iter) & FL_THUMB) ? 4 : 8)));

		    if (dest == ARM_INS_INEXT(block_end))
		    {
		      /* regular indirect function call */
		      CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
		      nedges += 2;
		    }
		    else
		    {
		      /* fucked up function call: does not return to the
		       * instruction following the call. add the link edge 
		       * to the appropriate block, but make a lot of noise
		       * about this so we know where to look when things 
		       * go wrong later on. */
		      if (!(ARM_INS_ATTRIB(dest) & IF_BBL_LEADER))
			FATAL(("Missed bbl leader because of fucked up function call"));
		      CfgEdgeCreateCall(cfg,block,CFG_HELL_NODE(cfg),ARM_INS_BBL(dest),NULL);
		      nedges += 2;
		      VERBOSE(0,("FREAKY FUNCTION CALL:\ncall @I\nset r14 @I\nreturn site @I",block_end,iter,dest));
		    }
		  }
		  else
		  {
		    WARNING(("Encountered something (@I) that looks like an indirect function call but the link register (defined in @I) is wrong! (block %x)",block_end,iter,G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(block_end)))));
		    CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		    if ((ARM_INS_OPCODE(iter)==ARM_SUB)
			&& (ARM_INS_REGB(iter)==ARM_REG_R15))
		    {
		      t_int32 address=G_T_UINT32(ARM_INS_CADDRESS(iter))-ARM_INS_IMMEDIATE(iter)+((ARM_INS_FLAGS(iter) & FL_THUMB)?4:8);
		      t_bbl * dest=find_close_bbl_by_address(ARM_INS_BBL(iter), AddressNew32(address));
		      CfgEdgeCreate(cfg, CFG_HELL_NODE(cfg), dest, ET_JUMP);
		    }
		    else
		      FATAL(("This is bizar"));
		    nedges++;
		  }
		}
		else /* We did not find the definition for R14 */
		{
		  /* no r14-defining instruction found within the block, this
		   * must be a jump. THIS IS NOT SAFE! WE COULD HAVE MISSED IT!
		   * But we assume a compiler does not produce this kind of code
		   * if it wants to implement a call.  The destination is the
		   * hell node, because we can't know the destination until after
		   * constant propagation */

		  CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		  nedges++;
		}
	      }
	    }
	  }
	  else
	  {
	    /* this instruction does NOT alter control flow, it just happens to be the end of a bbl */
	    if (ft_block != NULL) 
	    {
	      CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }
	  }
	  break;
	  /* }}} */

	  /* Loads can be used as jump instructions on architectures with explicit program counters */
	case IT_LOAD:
	  /* {{{ */
	  if (ARM_INS_REGA(block_end) == ARM_REG_R15)
	  {
	    /* control flow altering instruction.  there are different
	     * possibilities: LDR pc,r13,... is interpreted as a function return
	     * otherwise the operation is either a jump or a function call
	     * depending on whether r14 is designated the return address in the
	     * same basic block or not.
	     */
	    if ((ARM_INS_CONDITION(block_end) != ARM_CONDITION_AL) && (ARM_INS_INEXT(block_end) != NULL))
	    {
	      CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }

	    if (ARM_INS_REGB(block_end) == ARM_REG_R13)
            {
              if (ARM_INS_IPREV(block_end) && 
                  ARM_INS_CONDITION(block_end)==ARM_INS_CONDITION(ARM_INS_IPREV(block_end)) &&
                  ARM_INS_OPCODE(block_end) == ARM_LDR && 
                  ARM_INS_OPCODE(ARM_INS_IPREV(block_end)) == ARM_MOV &&  
                  ARM_INS_REGA(ARM_INS_IPREV(block_end)) == ARM_REG_R14 && 
                  ARM_INS_REGC(ARM_INS_IPREV(block_end)) == ARM_REG_R15 && 
                  !(ARM_INS_FLAGS(block_end) & FL_WRITEBACK)
                  )
                {
                  /* regular indirect function call because next instruction was just copied into return address register */
                  CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
                  nedges += 2;
                }
              else
                {
                  /* function return */
                  CfgEdgeCreate(cfg, block, CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
                  nedges++;
                }
            }
	    else if (RegsetIn(ARM_INS_REGS_USE(block_end) , ARM_REG_R15) &&
		!RegsetIsMutualExclusive(ARM_INS_REGS_USE(block_end) , ARM_ALL_BUT_PC_AND_COND) )
	    {
	      if (ARM_INS_OPCODE(block_end) == ARM_LDR &&
		  ARM_INS_SHIFTTYPE(block_end)==ARM_SHIFT_TYPE_LSL_IMM &&
		  ARM_INS_CONDITION(block_end)==ARM_CONDITION_LS)
	      {
		/* This is a switch of the form:
		 *	ldrls pc,[pc,rBase lsl 2]
		 *	b <fallthrough case>
		 * <$d>
		 *	<address of case 0>
		 *	<address of case 1>
		 *	...
		 *
		 * We take a conservative approach to this kind of switch:
		 * 	- add a jump edge to the hell node --- No longer, do switch detection
		 * 	- because the true code for the different cases has relocations 
		 * 	  pointing to it from the data block, incoming hell edges to
		 * 	  this code will automatically be added
		 * 	- in the previous step, a relocation was added to block_end that 
		 * 	  points to the data block, in order not to lose this block during
		 * 	  dead data removal
		 * 	- during layout, the switch table block is automatically linked to the
		 * 	  fallthrough case block, so the switch construct stays together in the
		 * 	  final layout
		 */
	      }
	      else
	      {
		FATAL(("Should not get here @I",block_end));
	      }
	    }
	    else if (ARM_INS_ATTRIB(block_end) & IF_SWITCHJUMP
				  && ARM_INS_CONDITION(block_end) == ARM_CONDITION_AL
				  && BBL_SUCC_FIRST(ARM_INS_BBL(block_end))
				  && CFG_EDGE_CAT(BBL_SUCC_FIRST(ARM_INS_BBL(block_end))) == ET_SWITCH)
		{
		  /* this is a switch block */
		}
	    else
	    {
	      stop = ARM_INS_IPREV(block_start);
	      for (iter = block_end; iter && iter != stop; iter = ARM_INS_IPREV(iter))
	      {
		if (RegsetIn(ARM_INS_REGS_DEF(iter) , ARM_REG_R14))
		  break;
	      }

	      if (iter && iter != stop)
	      {
		/* if iter == ARM_INS_IPREV(block_end) and *iter = MOV pc, r14 => function call
		 * if iter != ARM_INS_IPREV(block_end) and *iter = ADD pc, constant => function call
		 * if iter defines r14 independently from the pc => jump
		 * else FATAL!
		 */
		if (!RegsetIn(ARM_INS_REGS_USE(iter), 15))
		{
                  VERBOSE(1,("Check if this happens!\n@I\nThis would mean we would return to somewhere else.... Quite dangerous I think.",iter));
		  /* jump */
		  /* the destination is the hell node, because we can't know the destination until after constant propagation */
		  CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
		  nedges++;
		}
		else
		{
		  if (((t_arm_ins *)iter) == ARM_INS_IPREV(block_end)
		      && ARM_INS_OPCODE(iter) == ARM_MOV
		      && ARM_INS_REGC(iter) == ARM_REG_R15)
		  {
		    /* function call */
		    CfgEdgeCreateCall(cfg, block,CFG_HELL_NODE(cfg),ft_block,NULL);
		    nedges++;
		    nedges++;
		  }
		  else if (ARM_INS_OPCODE(iter) == ARM_ADD
		      && ARM_INS_REGB(iter) == ARM_REG_R15
		      && (ARM_INS_FLAGS(iter) & FL_IMMED))
		  {
                    /* Jump table of the form
                     *  ADD rX, r15, #4                 <--- iter
                     *  LDR r15, [rX, rY, lsl #2]       <--- block_end
                     *  NOP (optional; only if DATA is not 4-byte aligned by default)
                     *  .word 0x........
                     *  .word 0x........
                     *   ...
                     * This is also valid for Thumb. */
                    if (ARM_INS_FLAGS(iter) & FL_THUMB)
                    {
                        t_uint32 expected_imm = 0;
                        t_address data_addr = ARM_INS_OLD_ADDRESS(iter);

                        /* The ADD-instruction can either be 2- or 4-bytes large */
                        data_addr = AddressAdd(data_addr, ARM_INS_CSIZE(iter));
                        data_addr = AddressAdd(data_addr, ARM_INS_CSIZE(block_end));

                        /* In Thumb, instructions have to be aligned at 2-byte boundaries.
                         * However, 32-bit datawords should be 4-byte aligned. As such,
                         * 2-byte NOP's can be inserted after the LDR-instruction if necessary. */
                        if(AddressAnd(data_addr, 0x2))
                        {
                                t_arm_ins * ins_after_ldr = ARM_INS_INEXT(block_end);
                                ASSERT((ARM_INS_OPCODE(ins_after_ldr) == ARM_T2NOP) && (ARM_INS_CSIZE(ins_after_ldr) == 2), ("expected aligning NOP after 2-byte aligned LDR @eiB", ARM_INS_BBL(iter)));

                                data_addr = AddressAdd(data_addr, 2);
                        }

                        /* verify the expected immediate */
                        expected_imm = G_T_UINT32(AddressSub(AddressSub(data_addr, 4), AddressAnd(ARM_INS_OLD_ADDRESS(iter), ~0x3)));
                        ASSERT(ARM_INS_IMMEDIATE(iter) == expected_imm, ("expected immediate of %x in Thumb! @eiB", expected_imm, ARM_INS_BBL(iter)));
                    }

                    /* here we use an array of instructions */
                    t_arm_ins *dest = T_ARM_INS(ObjectGetInsByAddress(obj, AddressAddUint32(
                                                                                AddressAddUint32(ARM_INS_CADDRESS(iter), ARM_INS_IMMEDIATE(iter)),
                                                                                (ARM_INS_FLAGS(iter) & FL_THUMB) ? 4 : 8)));
                    t_arm_ins * next = ARM_INS_INEXT(block_end);

                    if ((G_T_UINT32(ARM_INS_CADDRESS(next)) & 0x3) && G_T_UINT32(ARM_INS_CSIZE(next))==0x2)
                        next = ARM_INS_INEXT(next);

		    if (dest == next)
		    {
                      if (RegsetIn(ARM_INS_REGS_USE(block_end), ARM_REG_R14))
                        {
                          VERBOSE(2, ("TEMPORARY HACK: ASSUMING @I IS SWITCH INS, NOT CALL INS", block_end));
                          CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
                          nedges++;
                        }
                      else
                        {
                          CfgEdgeCreateCall(cfg, block, CFG_HELL_NODE(cfg),ft_block,NULL);
                          nedges++;
                          nedges++;
                        }
		    }
		    else
		    {
		      /* fucked up function call: it will not return to the instruction 
		       * after the call. Add a link edge to the appropriate return block
		       * but make enough noise so we can find this if there are problems
		       * later on */
		      CfgEdgeCreateCall(cfg,block,CFG_HELL_NODE(cfg),ARM_INS_BBL(dest),NULL);
		      nedges += 2;
		      VERBOSE(0,("FREAKY FUNCTION CALL:\ncall @I\nset r14 @I\nreturn site @I",block_end,iter,dest));
		      if (!(ARM_INS_ATTRIB(dest) & IF_BBL_LEADER))
			FATAL(("Missed leader thanks to fucked up function call"));
		    }
		  }
		  else if (ARM_INS_OPCODE(iter) == ARM_SUB
		      && ARM_INS_REGB(iter) == ARM_REG_R15
		      && (ARM_INS_FLAGS(iter) & FL_IMMED))
		  {
		    /* fucked up function call */
		    t_arm_ins *dest = (t_arm_ins *) &(T_ARM_INS(iter)[(8-ARM_INS_IMMEDIATE(iter))/4]);
                    FATAL(("Fucked-up function call from @I to @I", iter, dest));
		    CfgEdgeCreateCall(cfg,block,CFG_HELL_NODE(cfg),ARM_INS_BBL(dest),NULL);
		    nedges += 2;
		    VERBOSE(0,("FREAKY FUNCTION CALL:\ncall @I\nset r14 @I\nreturn site @I",block_end,iter,dest));
		    if (!(ARM_INS_ATTRIB(dest) & IF_BBL_LEADER))
		      FATAL(("Missed leader thanks to fucked up function call"));
		  }
		  else
		  {
		    FATAL(("Fucked up fun call:\ncall @I\nr14 def @I",block_end,iter));
		  }
		}
	      }
	      else
	      {
		/* no r14-defining instruction found within the block, this must be a jump */
		/* the destination is the hell node, because we can't know the destination until after constant propagation */
		CfgEdgeCreate(cfg, block,  CFG_HELL_NODE(cfg), ET_JUMP);
		nedges++;
	      }
	    }

	  }
	  else
	  {
	    /* this instruction does NOT alter control flow, it just happens to be the end of a bbl */
	    if (ft_block != NULL) 
	    { 
	      CfgEdgeCreate(cfg, block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }
	  }
	  break;
	  /* }}} */

	case IT_LOAD_MULTIPLE:
	case IT_STORE_MULTIPLE:
	  /* {{{ */
	  if ((ARM_INS_IMMEDIATE(block_end) & (1 << ARM_REG_R15)) && (ARM_INS_OPCODE(block_end) == ARM_LDM))
	  {
	    /* control flow altering instruction.
	     * there are different possibilities: 
	     *   LDR r13,{...,r15} is interpreted as a function return
	     *   LDR r11,{...,r11,r13,r15} is interpreted as a function return as well, as this restore the fp, sp and pc using the fp 
	     *   any other operation is either a jump or a function call depending on whether r14 is designated the return 
	     *   address in the same basic block or not.
	     */
	    if ((ARM_INS_CONDITION(block_end) != ARM_CONDITION_AL) && (ARM_INS_INEXT(block_end) != NULL))
	    {
	      CfgEdgeCreate(cfg,  block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }

	    if (ARM_INS_REGB(block_end) == ARM_REG_R13)
	    {
	      /* function return */
	      CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
	      nedges++;
	    }
	    else if (ARM_INS_REGB(block_end) == ARM_REG_R11 && (ARM_INS_IMMEDIATE(block_end) & (1 << ARM_REG_R13)) && (ARM_INS_IMMEDIATE(block_end) & (1 << ARM_REG_R11)))
	    {
	      /* function return */
	      CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
	      nedges++;
	    }
	    else
	    {
	      stop = ARM_INS_IPREV(block_start);
	      for (iter = block_end; iter != stop; iter = ARM_INS_IPREV(iter))
	      {
		if (RegsetIn(ARM_INS_REGS_DEF(iter) , 14))
		  break;
	      }

	      if (iter != stop)
	      {
		/* if iter == ARM_INS_IPREV(block_end) and *iter = MOV pc, r14 => function call
		 * if iter != ARM_INS_IPREV(block_end) and *iter = ADD pc, constant => function call
		 * if iter defines r14 independently from the pc => jump
		 * else FATAL!
		 */
		if (!RegsetIn(ARM_INS_REGS_USE(iter) , 15))
		{
		  FATAL(("Check if this happens! This would mean we would return to somewhere else.... Quite dangerous I think."));
		  /* jump */
		  /* the destination is the hell node, because we can't know the destination until after constant propagation */
		  CfgEdgeCreate(cfg,  block,  CFG_HELL_NODE(cfg), ET_JUMP);
		  nedges++;
		}
		else if (((t_arm_ins *) iter == ARM_INS_IPREV(block_end))&&((ARM_INS_OPCODE(iter) == ARM_MOV) && (ARM_INS_REGC(iter) == ARM_REG_R15)))
		{
		  /* function call */
		  CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
		  nedges++;
		  nedges++;
		}
		else if (((ARM_INS_OPCODE(iter) == ARM_ADD) && (ARM_INS_REGB(iter) == ARM_REG_R15) && (ARM_INS_FLAGS(iter) & FL_IMMED))&&((t_arm_ins *) &(T_ARM_INS(iter)[(ARM_INS_IMMEDIATE(iter)+8)/4]) == ARM_INS_INEXT(block_end)))
		{
                  FATAL(("Function call using ADD instruction @I", iter));
		  /* yep, it's a function call */
		  CfgEdgeCreateCall(cfg,  block, CFG_HELL_NODE(cfg),ft_block,NULL);
		  nedges++;
		  nedges++;
		}
		else
		{
		  FATAL(("Encountered something that looks like an indirect function call but the link register is wrong! (block %x)",G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(block_end)))));
		}
	      }
	      else
	      {
		/* no r14-defining instruction found within the block, this must be a jump */
		/* the destination is the hell node, because we can't know the destination until after constant propagation */
		CfgEdgeCreate(cfg,  block,  CFG_HELL_NODE(cfg), ET_JUMP);
		nedges++;
	      }
	    }
	  }
	  else
	  {
	    /* this instruction does NOT alter control flow, it just happens to be the end of a bbl */
	    if (ft_block != NULL)
	    {
	      CfgEdgeCreate(cfg,  block, ft_block, ET_FALLTHROUGH);
	      nedges++;
	    }
	  }
	  break;
	  /* }}} */

	  /* Ignore data */
	case IT_DATA:
	  break;

	default:
	  /* instructions that end up here do not alter the control flow, they just happen to be */
	  /* the end of a basic block */
	  if (RegsetIn(ARM_INS_REGS_DEF(block_end) , 15)) 
	  {
	    /* consistency check */
	    FATAL(("pc-defining instruction @I not recognized as such!",block_end));
	  }

	  if (ARM_INS_INEXT(block_end) != NULL)
	  {
	    CfgEdgeCreate(cfg,  block, ft_block, ET_FALLTHROUGH);
	    nedges++;
	  }
      }
    }
  }

  /* {{{ mark switch tables and their corresponding edges */
  CFG_FOREACH_BBL(cfg,block)
  {
    flag = FALSE;
    BBL_FOREACH_PRED_EDGE(block,edge)
      if (CFG_EDGE_CAT(edge)==ET_SWITCH)
      {
	flag = TRUE;
	break;
      }

    if (flag 
	&& BBL_NINS(block) == 1 
	&& ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(block))) == ARM_B
	&& ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(block))) == ARM_CONDITION_AL)
    {
      /* mark the jump edges out of ADS-style switches */
      BBL_FOREACH_SUCC_EDGE(block,j_edge)
	if (CFG_EDGE_CAT(j_edge)==ET_JUMP)
	{
	  CFG_EDGE_SET_FLAGS(j_edge,    CFG_EDGE_FLAGS(j_edge) |EF_FROM_SWITCH_TABLE);
	}
    }
    else
    {
      BBL_FOREACH_SUCC_EDGE(block,j_edge)
	if (CFG_EDGE_CAT(j_edge)==ET_JUMP)
	{
	  CFG_EDGE_SET_FLAGS(j_edge,   CFG_EDGE_FLAGS(j_edge) & (~EF_FROM_SWITCH_TABLE));
	}
    }
  } /* }}} */

  return nedges;
}
/* }}} */
/*!
 * Basic leader-detection algorithm. 
 * ---------------------------------
 *
 * See e.g. the Red Dragon Book ("Compilers: Principles, Techniques and
 * Tools", by Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman) for more
 * details. 
 *
 * The aim of this function is to identify all start-addresses of basic
 * blocks (and mark them). There are four reasons to mark an instructions as
 * a basic block leader:
 * 
 * 1. The instruction is the target of a direct or indirect jump. This is not
 * as easy as it seems, especially when the program counter is explicit:
 * every instruction that changes the pc can be considered as a jump
 * instruction.
 *
 * 2. It's the successor of a control-flow altering instruction (if a jump is
 * conditional, than the next instruction will be the first instruction of the
 * fall-through block, else it is either target of an other jump or a dead
 * block. In the latter case, it will be removed by the unreachable block
 * removal routines, so we just mark it as a basic block leader).
 *
 * 3. There is an address produced to this basic block, this is either 
 *
 *    a. done directly, using the program counter (we assume no real
 *    code-address calculations are present (so not address of function + x) 
 *
 *    b. using a relocation (and not necessary detected when scanning the
 *    instructions) 
 *
 * For both cases we need to assume there will be an indirect jump to this
 * address.
 *
 * 4. The start of data blocks in code or instructions following datablocks
 * are considered basic block leaders.
 *
 *
 * This function will find all basic block leaders that are not part of a
 * switch statement. More switch checking will be done in optimize switches.
 * 
 * \todo needs only one parameter
 *
 * \param obj
 * \param code
 *
 * \return t_uint32 
 */
/* ArmFindBBLLeaders {{{ */
static t_bool
PltSubSecStartWithThumbStub(t_object *obj, t_section *sec)
{
  return AddressIsEq(SECTION_CSIZE(sec),AddressNewForObject(obj,16));
}

/* data structures used in hash table */
/* TODO: really should be included via header file ... */

typedef struct _t_symbol_he t_symbol_he;

struct _t_symbol_he
{
  t_hash_table_node node;
  t_symbol * sym1;
  t_symbol * sym2;
};

static t_uint32 ArmFindBBLLeaders(t_object * obj)
{
  /* When calling this function, all instructions should still be stored
   * sequentially in the array Ains. The last instruction stored can be
   * recognised with INSNEXT ==  NULL */

  t_arm_ins * ins;
  t_uint32  nleaders=0;
  t_section *code;
  t_uint32 i;

  t_hash_table * gnu_thumb1_symbol_ht = SymbolTableCreateHashTableForGetFirstLimitedToNamePattern(OBJECT_SUB_SYMBOL_TABLE(obj),"__gnu_thumb1_*");

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    /* Initialisation: The first instruction of the program is always a leader.
     * The reasoning is equal to that of reason 2: Either there will be a jump to
     * this address or it's the entry point or it is dead. */

    ins = T_ARM_INS(SECTION_DATA(code));
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) | IF_BBL_LEADER);

    SECTION_FOREACH_ARM_INS(code, ins)
    {
      t_bool pc_def_detected_and_resolved = FALSE, pc_use_detected_and_resolved = FALSE;

      /* 1. DATA (reason 4) {{{ */ 
      if (ARM_INS_TYPE(ins) == IT_DATA)
      {
        /* The start of a block of data is considered a basic block leader */
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins)| IF_BBL_LEADER); 
        if(ARM_INS_FLAGS(ins) & FL_THUMB)
        {
          if(G_T_UINT32(ARM_INS_CSIZE(ins)) != 2) FATAL(("Something went wrong during disassembly: thumb instruction does not have size 2 as it should have! @I",ins));
        }

        /* Process all the other data */
        while (ins && (ARM_INS_TYPE(ins) == IT_DATA)) {
          ins = ARM_INS_INEXT(ins); 
        } 

        /* The instruction following the data is a basic block leader */
        if (ins) 
          ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_BBL_LEADER);
        else
          /* If there are no instructions left, we can exit the outer loop
           * (all blocks are marked) */
          break; 
      }
      /* }}} */
      /* 2. NORMAL BRANCHES (reason 1 and 2). For the ARM all branches are
       * direct, except BX. For BX we detect the BX R15 case, and ignore all
       * other cases.
       * The TBB/TBH instructions are handled as switch tables (see lower).
       * {{{ */
      if ((ARM_INS_TYPE(ins) == IT_BRANCH) && (ARM_INS_OPCODE(ins) != ARM_T2TBB) && (ARM_INS_OPCODE(ins) != ARM_T2TBH))
      {
        t_arm_ins * target;
        t_address target_address;
        pc_def_detected_and_resolved = TRUE;

        /* The instruction following the branch instruction is a basic block leader */
        if (ARM_INS_INEXT(ins) != NULL) {
          ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
        }

        /* The BX instruction is always indirect, so in all normal cases, we can
         * ignore it here (the third block in this if then else statement does
         * just that). However, if the pc (R15) is used as the target register,
         * BX can be used to implement some sort of fallthrough to a block of a
         * different mode (arm/thumb), if the correct amount of padding (noops)
         * is inserted. This will not be handled by hell edges, as this statement
         * will not be relocated. We explicitly handle it here */
        if ((ARM_INS_FLAGS(ins) & FL_THUMB) && (ARM_INS_OPCODE(ins) == ARM_BX) && (ARM_INS_REGB(ins) == ARM_REG_R15))
        {
          pc_use_detected_and_resolved = TRUE;
          target_address=AddressAddUint32(ARM_INS_CADDRESS(ins),4);
        }
        /* Normal, direct branches. Mark the target of the branch will as a basic
         * block leader */
        else if (ARM_INS_OPCODE(ins) != ARM_BX)
        {
          if (ARM_INS_FLAGS(ins) & FL_IMMED)
            /* Only branch(-link) immediate instructions should be considered here */
            target_address=AddressAddUint32(ARM_INS_CADDRESS(ins),((ARM_INS_FLAGS(ins) & FL_THUMB)?4:8) + ARM_INS_IMMEDIATE(ins));
          else
            /* Branch(-link) register instructions are handled here: next instruction is a leader */
            target_address=AddressAdd(ARM_INS_CADDRESS(ins), ARM_INS_CSIZE(ins));
        }
        /* Indirect BX... setting the target address to the address of the
         * instruction following the branch instruction = NOOP */
        else if (ARM_INS_REGB(ins) != ARM_REG_R15)
        {
          target_address=AddressAdd(ARM_INS_CADDRESS(ins),ARM_INS_CSIZE(ins));
        }
        else
          FATAL(("Implement BX R15 from ARM code for @I", ins));

        target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));

        if (target) 
        {
          ARM_INS_SET_ATTRIB(target,  ARM_INS_ATTRIB(target) | IF_BBL_LEADER); 

          /* This could be a switch table of the following form:
           *
           * CMP r0, #xx
           * BLS <to BL below> (r0 is within range of switch table) -+
           * ...                                                     |
           * BL <__gnu_thumb1_...>    <------------------------------+
           * .byte xx
           * .byte yy
           * .byte zz
           *
           * Where __gnu_thumb1_... makes use of the LR-register in which the start address of the switch table is stored.
           */

          if (ARM_INS_OPCODE(ins) == ARM_BL
             && ARM_INS_FLAGS(ins) & FL_THUMB
             && ARM_INS_INEXT(ins)
             && ARM_INS_TYPE(ARM_INS_INEXT(ins)) == IT_DATA)
          {
            /* The actual address of the symbol we're searching is ODD because it's a Thumb symbol! */
            t_address sym_address = target_address | 1;
            t_symbol *sym = NULL;
            t_symbol * next_sym = NULL;
            t_bool use_next_sym = FALSE;
            t_symbol_he * node = HashTableLookup(gnu_thumb1_symbol_ht,&sym_address);

            if (node)
              {
                sym = node->sym1;
                next_sym = node->sym2; // per first symbol at an address, the HT holds at most one (i.e. the next) symbol at the same address (if that exists)
                use_next_sym = TRUE; // so rather than searching for the next one, we can reuse the held one at least the first time
              }

            while (sym)
            {
              if (StringPatternMatch("__gnu_thumb1_*", SYMBOL_NAME(sym)))
              {
                ASSERT(ARM_INS_INEXT(ins) != NULL, ("No data for Thumb-1 switch statement @I", ins));
                ASSERT(ARM_INS_TYPE(ARM_INS_INEXT(ins)) == IT_DATA, ("Expected data immediately after @I", ins));

                ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)| IF_SWITCHJUMP);
                ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_SWITCHTABLE);

                t_reloc * rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(ARM_INS_INEXT(ins)),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
                RELOC_SET_LABEL(rel, StringDup("Thumb-1 switch table reloc"));

                break;
              }

              if (use_next_sym)
                {
                  // use the second symbol at the address as stored in the HT
                  sym=next_sym; 
                  // since no third symbol is stored, from now on, the fallback will have to be used of GetNext...ByAddress
                  use_next_sym = FALSE;
                }
              else 
                {
                  sym = SymbolTableGetNextSymbolByAddress(sym, sym_address);
                }
            }
          }
        }
        else 
        {
          t_object * lo = ObjectGetLinkerSubObject(obj);	
          t_section * sec = SectionGetFromObjectByAddress(lo, target_address);

          if ((sec)&&(StringPatternMatch("PLTELEM:*", SECTION_NAME(sec))))
          {
            t_string relname = StringConcat2("PLTREL:", SECTION_NAME(sec) + 8);
            t_section * rel  =  SectionGetFromObjectByName(lo, relname);
            Free(relname);
            ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", SECTION_NAME(sec) + 8));

            VERBOSE(4, ("Call/Branch to PLT @I", ins));

            if (ARM_INS_FLAGS(ins) & FL_THUMB)
            {
              //              t_address offset;
              switch (ARM_INS_OPCODE(ins))
              {
                case ARM_BL:
                case ARM_BLX:
                {
                  t_address arm_plt_entry_offset;
                  /* blx can switch between ARM/Thumb -> always branch to the ARM plt entry */
                  if (PltSubSecStartWithThumbStub(obj,sec))
                    arm_plt_entry_offset = AddressNewForObject(obj,4);
                  else
                    arm_plt_entry_offset = AddressNullForObject(obj);
                  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), arm_plt_entry_offset, T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(sec),AddressNullForObject(obj), FALSE, NULL, NULL, NULL, "R00A00+=M?P:Pifffffffc&!-s0004-" "\\" "R00A00+M%" WRITE_THM_PC22_EXT);
                  break;
                }
                case ARM_B:
                {
                  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(sec),AddressNullForObject(obj), FALSE, NULL, NULL, NULL,
                  /* base offset: destination-PC-4 */
                  "R00P-s0004-"
                  /* this is a thumb instruction and a branch cannot switch between
                   * thumb and ARM state -> jump to thumb switch, which lies at the
                   * start of this PLT switch -> no extra offset needed. Just use
                   * the regular thm_pc24 relocation code
                   */
                  "\\" WRITE_THM_PC24);
                  break;
                }
                default:
                  FATAL(("Add support for control transfer to PLT entry: @I",ins));
                  break;
              }
            }
            else
            {
              /* if there is a thumb stub at the start of this PLT section, the ARM
               * part starts 4 bytes in
               */
              t_address arm_plt_entry_offset;
              //              t_string thumb_plt_sym_name;
              if (PltSubSecStartWithThumbStub(obj,sec))
              {
                arm_plt_entry_offset = AddressNewForObject(obj,4);
              }
              else
              {
                arm_plt_entry_offset = AddressNullForObject(obj);
              }
              RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), arm_plt_entry_offset, T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(sec),AddressNullForObject(obj), FALSE, NULL, NULL, NULL, "R00P-s0008-A00+" "\\" "= s0002 & % s0002 > i00ffffff &=l iff000000 &| R00M?i10000000|ifeffffff& } s0017 < |: }* ! w\\l i00ffffff &-$");
            }
            /* keep the symbol referred by the PLT entry alive */
            RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(rel),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");


          }
          else
          {
            /* This used to be a FATAL, but that probably isn't necessary.  Such a
             * branch usually means that this piece of code never can be executed.
             * We change the branch into something saner: a branch to itself => if
             * the program ever executes this instruction, it hangs instead of
             * dieing on a SEGFAULT */
            ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_BBL_LEADER);
            ARM_INS_SET_IMMEDIATE(ins,  -((ARM_INS_FLAGS(ins) & FL_THUMB)?4:8));
          }
        }
      }
      /* }}} */
      /* {{{ 3. CHANGES TO THE PROGRAM COUNTER WITH A DATAPROCESSING INSTRUCTION (reason 1 and 2) */
      else if (ARM_INS_TYPE(ins) == IT_DATAPROC && ARM_INS_REGA(ins)==ARM_REG_R15)
      {
        pc_def_detected_and_resolved = TRUE;
        /* This instruction preforms a some kind of jump, so the next instruction
         * is a bbl leader */
        if (ARM_INS_INEXT(ins) != NULL) 
        {
          t_arm_ins *next = ARM_INS_INEXT(ins);
          ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next) | IF_BBL_LEADER);
        }

        /* Add an offset to the PC to calculate a new PC. Can be a loaded offset
         * or a calculated offset. In case of a loaded offset (and in case all
         * offsets are relocated) no more processing is needed. Hell edges will
         * be added that conservatively model the switch. However, switch
         * detection can improve this modelling. In case not all offsets are
         * loaded, or if the offset is calculated, more processing is needed to
         * discover the switch targets. We mark this instruction as a special
         * instruction (a switch). Switch detection will try to understand the
         * switch, create more basic block if needed, and bail out if it cannot
         * figure out the switch */
        if (ARM_INS_OPCODE(ins) == ARM_ADD && (ARM_INS_REGB(ins) == ARM_REG_R15 || ARM_INS_REGC(ins) == ARM_REG_R15))
        {
          if (ARM_INS_REGB(ins) == ARM_REG_R12
              && ARM_INS_IPREV(ins)
              && ARM_INS_OPCODE(ARM_INS_IPREV(ins)) == ARM_LDR
              && ARM_INS_REGA(ARM_INS_IPREV(ins)) == ARM_REG_R12
              && (ARM_INS_FLAGS(ARM_INS_IPREV(ins)) & FL_IMMED)
              && ARM_INS_REGB(ARM_INS_IPREV(ins)) == ARM_REG_R15)
          {
            /* This is a special case of PIC code, i.e., the case for PIC from-thumb veneers (as used in Android dynamic libraries):
             *  0003b088 <__pthread_mutex_unlock_from_thumb>:
             *     3b088:       4778            bx      pc
             *     3b08a:       46c0            nop                     ; (mov r8, r8)
             *     3b08c:       e59fc000        ldr     ip, [pc]        ; 3b094 <__pthread_mutex_unlock_from_thumb+0xc>
             *     3b090:       e08cf00f        add     pc, ip, pc
             *     3b094:       fffd2df4        .word   0xfffd2df4
             * Additionally, it is assumed that r12 (ip) is used as an intermediate register. */
          }
          else
            ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
          pc_use_detected_and_resolved = TRUE;
        }
        else if (ARM_INS_OPCODE(ins)==ARM_SUB && ARM_INS_REGB(ins)!=ARM_REG_R15 && ARM_INS_REGC(ins)!=ARM_REG_R15)
        {
          /* Copies an address in the pc, no problem, as there is already an edge
           * to this address (relocation or address producer) */
        }
        else if (ARM_INS_OPCODE(ins)==ARM_MOV && ARM_INS_REGC(ins)!=ARM_REG_R15)
        {
          if (ARM_INS_FLAGS(ins) & FL_THUMB)
            {
              t_reg src_reg_mov = ARM_INS_REGC(ins);
              t_arm_ins * add_ins = ins;

              while ((add_ins=ARM_INS_IPREV(add_ins)))
                {
                  if (ARM_INS_OPCODE(add_ins)==ARM_ADD && ARM_INS_SHIFTTYPE(add_ins)==ARM_SHIFT_TYPE_LSL_IMM && ARM_INS_REGA(add_ins)==ArmPropagateRegBackwardStack(src_reg_mov, ins, add_ins))
                    {
                      ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
                      pc_use_detected_and_resolved = TRUE;

                      break;
                    }

                   if (RegsetIn(ARM_INS_REGS_DEF(add_ins), src_reg_mov)
                        && !(ARM_INS_OPCODE(add_ins) == ARM_LDR
                                && ARM_INS_REGA(add_ins) == src_reg_mov
                                && ARM_INS_REGB(add_ins) == ARM_REG_R13
                                && (ARM_INS_FLAGS(add_ins) & FL_IMMED)
                                && (ARM_INS_FLAGS(add_ins) & FL_PREINDEX)))
                   {
                        break;
                   }
                }
            }

          /* Copies an address in the pc, no problem, as there is already an edge
           * to this address (relocation or address producer) */
        }
        else if (ARM_INS_OPCODE(ins)==ARM_ADD && ARM_INS_FLAGS(ins) & FL_IMMED
            && ARM_INS_REGB(ins)!=ARM_REG_R15)
        {
          if (ARM_INS_IMMEDIATE(ins)==0)
          {
            /* Copies an address in the pc, no problem, as there is already an
             * edge to this address (relocation or address producer) */
            ARM_INS_SET_OPCODE(ins, ARM_MOV);
            ARM_INS_SET_REGC(ins, ARM_INS_REGB(ins));
            ARM_INS_SET_REGB(ins, ARM_REG_NONE);
            ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) & (~FL_IMMED));
          }
          else
          {
            /* This would mean we manipulate code addresses. We assume this
             * cannot happen */
            WARNING(("Handling @I as an indirect jump. Possibly incorrect!",ins));
          }
        }
        else if (ARM_INS_OPCODE(ins) == ARM_SUB && (ARM_INS_FLAGS(ins) & FL_S)
            && ARM_INS_REGA(ins) == ARM_REG_R15
            && ARM_INS_REGB(ins) == ARM_REG_R14
            && (ARM_INS_FLAGS(ins) & FL_IMMED) && ARM_INS_IMMEDIATE(ins) == 4)
        {
          /* return instruction from some interrupt handler */
        }
        else if (ARM_INS_OPCODE(ins) == ARM_SUB
            && ARM_INS_REGA(ins) == ARM_REG_R15
            && ARM_INS_REGB(ins) == ARM_REG_R15
            && (ARM_INS_FLAGS(ins) & FL_IMMED) && ARM_INS_IMMEDIATE(ins) == 4)
        {
          /* jump to next instruction: used as a means of flushing the
           * instruction pipeline (see cpwait macro in linux kernel source).
           * this should be considered a system instruction, not a control flow
           * instruction */
          pc_use_detected_and_resolved = TRUE;
        }
        else if (ARM_INS_OPCODE(ins) == ARM_SUB
            && ARM_INS_REGA(ins) == ARM_REG_R15
            && ARM_INS_REGB(ins) == ARM_REG_R14
            && !(ARM_INS_FLAGS(ins) & FL_IMMED)
            && ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_IMM 
            && ARM_INS_SHIFTLENGTH(ins) == 32)
        {
          /* used in the cpwait_ret macro in the linux kernel: - wait for coproc
           * completion (previous instruction reads some value from the coproc
           * into regC) - flush instruction pipeline (because of jump) - return
           * (sub pc,r14,(anything >> 32) == mov pc,r14)
           */
        }
        else if (ARM_INS_OPCODE(ins) == ARM_ADD
                 && !(ARM_INS_FLAGS(ins) & FL_IMMED))
        {
          ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
        }
        else FATAL(("Implement @I\n!",ins));
      }
      /* }}} */
      /* 4. CREATION OF AN ADDRESS BY A DATAPROCESSING INSTRUCTION USING THE PC (reason 3) {{{ */ 
      else if ((ARM_INS_TYPE(ins) == IT_DATAPROC) && (ARM_INS_REGA(ins)!=ARM_REG_R15)
          && ((ARM_INS_REGB(ins) ==ARM_REG_R15) || (ARM_INS_REGC(ins) ==ARM_REG_R15) )) 
      {
        if ((ARM_INS_OPCODE(ins)==ARM_ADD) && (ARM_INS_FLAGS(ins) & FL_IMMED))
        {
          t_address target_address;
          t_arm_ins * target;
          target_address=AddressAdd(ARM_INS_CADDRESS(ins),AddressAddUint32(AddressMulUint32(ARM_INS_CSIZE(ins),2), ARM_INS_IMMEDIATE(ins)));
          /* VERBOSE(0,( "targetadress 2: @G\n", target_address)); */
          target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));

          if (target)
          {
            if ((!(ARM_INS_ATTRIB(target) & IF_DATA)))
              ARM_INS_SET_ATTRIB(target,  ARM_INS_ATTRIB(target) | IF_BBL_LEADER); 
          }
          else
            FATAL(("Data processing operation produced an address that lies out of the section:\n@I",ins));
          pc_use_detected_and_resolved=TRUE;
        }
        else if ((ARM_INS_OPCODE(ins)==ARM_SUB) && (ARM_INS_FLAGS(ins) & FL_IMMED))
        {

          t_address target_address;
          t_arm_ins * target;
          t_uint32 pc_offset = 2*G_T_UINT32(ARM_INS_CSIZE(ins));

        if (ARM_INS_FLAGS(ins) & FL_THUMB)
                pc_offset = 4;

          target_address=AddressAddInt32(ARM_INS_CADDRESS(ins),pc_offset - ARM_INS_IMMEDIATE(ins));
          /* VERBOSE(0,( "targetadress 3: @G\n", target_address)); */
          target=T_ARM_INS(ObjectGetInsByAddress(obj, target_address));
          if (target)
          {
            if ((!(ARM_INS_ATTRIB(target) & IF_DATA)))
              ARM_INS_SET_ATTRIB(target,  ARM_INS_ATTRIB(target) | IF_BBL_LEADER); 
          }
          else
            FATAL(("Data processing operation produced an address that lies out of the section:\n@I", ins));
          pc_use_detected_and_resolved=TRUE;
        }
        else if ((ARM_INS_OPCODE(ins)==ARM_ADD)) 
        {

          /* This is a PCrelative offset + PC. Potentially dangerous (since
           * it takes a lot of analysis to find out what is calculated
           * here), but it it's probably a REL32 relocation + pc. We'll
           * check and remove these further on (in the address producer
           * construction)  */

          pc_use_detected_and_resolved=TRUE; 
        }
        else if ((ARM_INS_OPCODE(ins)==ARM_MOV) && (ARM_INS_REGB(ins) ==ARM_REG_NONE))
        {
          if(!(ARM_INS_FLAGS(ins) & FL_THUMB))
          {
            /* A copy operation: the instruction + 8 (bytes) could be the target of an indirect jump */
            t_arm_ins * target = NULL;
            if (ARM_INS_INEXT(ins) != NULL) {
              target = ARM_INS_INEXT(ARM_INS_INEXT(ins));
            }
            if (target) 
              ARM_INS_SET_ATTRIB(target,  ARM_INS_ATTRIB(target) | IF_BBL_LEADER); 
            pc_use_detected_and_resolved=TRUE;
          }
          else
          {
            /* A single thumb instruction cannot add/sub something from pc, so
               check for a subsequent add/sub and potentially also an ldr */
            if(!ThumbCheckMovPcLeader(obj,ins,NULL))
              FATAL(("PC use not detected for @I!",ins));
            else pc_use_detected_and_resolved=TRUE;
          }
        }
        else if ((ARM_INS_OPCODE(ins)==ARM_TEQ) && (ARM_INS_REGA(ins)==ARM_REG_NONE)
            && (ARM_INS_REGB(ins)  ==ARM_REG_R15) && (ARM_INS_REGC(ins)  ==ARM_REG_R15))
        {
          /* Test to detect 26/32 bit mode */
          pc_use_detected_and_resolved=TRUE;
        }
        else FATAL(("Dataprocessing instruction @I calculates something from the pc, but we do not understand what it does.",ins));
      }
      /* }}} */
      else if ((ARM_INS_TYPE(ins) == IT_LOAD) && (ARM_INS_REGA(ins) == ARM_REG_R15) && !(ARM_INS_REGB(ins) == ARM_REG_R15) &&
                (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM) && (ARM_INS_SHIFTLENGTH(ins) == 2)
                /* This is not always a switch table!
                 * The following code behaves like a function call:
                 *  ...
                 *  mov lr, pc
                 *  ldr pc, [rX, rY, LSL #imm]
                 *  ...
                 */
                && !(ARM_INS_IPREV(ins)
                      && ARM_INS_OPCODE(ARM_INS_IPREV(ins))==ARM_MOV
                      && ARM_INS_REGA(ARM_INS_IPREV(ins))==ARM_REG_R14
                      && ARM_INS_REGC(ARM_INS_IPREV(ins))==ARM_REG_R15))
      {
        t_reloc * rel;
        t_arm_ins * next = ARM_INS_INEXT(ins);

        ASSERT(next, ("No next ins for switch statement @eiB", ARM_INS_BBL(ins)));

        if (G_T_UINT32(AddressAnd(ARM_INS_OLD_ADDRESS(next), 0x3)))
        {
                ASSERT(ARM_INS_OPCODE(next)==ARM_T2NOP && ARM_INS_CSIZE(next)==2, ("expected 2-byte NOP to align the switch datablock"));

                ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next) | IF_BBL_LEADER);
                next = ARM_INS_INEXT(next);
                ASSERT(next, ("no data found for switch statement @eiB", ARM_INS_BBL(ins)));
        }

        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
        ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next) | IF_BBL_LEADER);
        ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next)|IF_SWITCHTABLE);

        /* Add a relocation to keep the jump table alive.  The relocation is only
         * meant to keep the data block alive as long as the switch instruction
         * is live, it never needs to be relocated, so the write part of the
         * program is empty. */

        rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(next),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
        RELOC_SET_LABEL(rel, StringDup("Switch table reloc"));

        pc_def_detected_and_resolved=TRUE;
      }
      /* 5. LOADS USED AS JUMP INSTRUCTIONS (reason 1 and 2) {{{ */
      else if ((ARM_INS_TYPE(ins) == IT_LOAD) && (ARM_INS_REGA(ins) == ARM_REG_R15) && !(RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R15)))
      {
        /* A load from memory into the pc => the next instruction is a
         * leader.  Since the loaded address is either produced by an other
         * instruction (that is thus converted to an address producer which
         * will have an associated relocation) or produced by the linker (and
         * thus must have a relocation associated) we do not need to worry about
         * the target of this type of jump. It will become a jump to hell,
         * and can be resolved if the load comes from constant memory by the
         * constant propagator. */
		
        /* this could be a switch, but only if no immediate is present */
        if (!(ARM_INS_FLAGS(ins) & FL_IMMED))
          {
            t_arm_ins *defB = NULL, *defC = NULL;
            t_arm_ins *it = ARM_INS_IPREV(ins);
            
            /* look for instructions defining both registers in the load instruction */
            while (it)
              {
                if (!defB && RegsetIn(ARM_INS_REGS_DEF(it), ARM_INS_REGB(ins)))
                  defB = it;
                if (!defC && RegsetIn(ARM_INS_REGS_DEF(it), ARM_INS_REGC(ins)))
                  defC = it;
		
                /* we have found instructions defining both registers; early exit */
                if (defB && defC)
                  break;
                
                /* continue iterating on the next instruction */
                it = ARM_INS_IPREV(it);
              }
            
            if (defB && defC)
              {
                /* check whether this is a switch statement */
                t_arm_ins *addrprod_ins = NULL, *lsl_ins = NULL;
		
                /* one of the two instructions should be an address producer */
                if (ARM_INS_OPCODE(defB) == ARM_ADD
                    && ARM_INS_REGB(defB) == ARM_REG_R15
                    && ARM_INS_FLAGS(defB) & FL_IMMED)
                  {
                    addrprod_ins = defB;
                    lsl_ins = defC;
                  }
                else if (ARM_INS_OPCODE(defC) == ARM_ADD
                         && ARM_INS_REGB(defC) == ARM_REG_R15
                         && ARM_INS_FLAGS(defC) & FL_IMMED)
                  {
                    addrprod_ins = defC;
                    lsl_ins = defB;
                  }
		
                /* check whether the other instruction is an LSL #2 one */
                if (addrprod_ins
                    && ARM_INS_OPCODE(lsl_ins) == ARM_MOV
                    && ARM_INS_SHIFTTYPE(lsl_ins) == ARM_SHIFT_TYPE_LSL_IMM
                    && ARM_INS_SHIFTLENGTH(lsl_ins) == 2)
                  {
                    t_arm_ins *next = ARM_INS_INEXT(ins);
                    
                    /* assume this is a switch statement */
                    ASSERT(!(ARM_INS_FLAGS(addrprod_ins) & FL_THUMB), ("no thumb supported for now! @I", addrprod_ins));
                    
                    t_address data_addr = ARM_INS_OLD_ADDRESS(addrprod_ins);
                    /* PC-compensation */
                    data_addr = AddressAddUint32(data_addr, 8);
                    /* address producer value */
                    data_addr = AddressAddUint32(data_addr, ARM_INS_IMMEDIATE(addrprod_ins));
                    /* sanity check: only support for immediately followed data tables */
                    ASSERT(AddressIsEq(data_addr, ARM_INS_OLD_ADDRESS(next)), ("not supported @I", ins));
                    
                    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) | IF_SWITCHJUMP);
                    ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next) | IF_BBL_LEADER);
                    ARM_INS_SET_ATTRIB(next, ARM_INS_ATTRIB(next) | IF_SWITCHTABLE);
                    
                    pc_use_detected_and_resolved = TRUE;
                    
                    t_reloc *rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(next),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
                    RELOC_SET_LABEL(rel, StringDup("Switch table reloc"));
                  }
              }
          }

        pc_def_detected_and_resolved = TRUE;
        if (ARM_INS_INEXT(ins) != NULL) {
          ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
        }
      }
      else if ((ARM_INS_TYPE(ins) == IT_LOAD) 
          && (ARM_INS_REGA(ins) == ARM_REG_R15) 
          && (ARM_INS_REGB(ins) == ARM_REG_R15)
          && !( /* make sure we don't catch switches here: they are handled in case 9. */
            ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSL_IMM
            && ARM_INS_SHIFTLENGTH(ins)==2
            && ARM_INS_CONDITION(ins)==ARM_CONDITION_LS
            ))
      {
        /* a load from data in the code section into the pc => this should become an address producer 
         * that produces its address directly into the pc */
        pc_use_detected_and_resolved = pc_def_detected_and_resolved = TRUE;
        if (ARM_INS_INEXT(ins) != NULL) {
          ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
        }
      }
      /* }}} */
      /* 6. MULTIPLE LOADS USED AS JUMP INSTRUCTIONS (reason 1 and 2) {{{ */
      else if ((ARM_INS_OPCODE(ins) == ARM_LDM) && (ARM_INS_IMMEDIATE(ins) & (1 << ARM_REG_R15))) 
      {
        /* This alters (loads) the pc, so the next instruction is a leader.
         * Can be a return or a stored address. See normal loads for
         * explanation. */
        pc_def_detected_and_resolved = TRUE;
        if (ARM_INS_INEXT(ins) != NULL) {
          ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
        }
      }
      /* }}} */
      /* 7. STORES THAT CREATE ADDRESSES USING THE PC {{{ */
      else if ((ARM_INS_OPCODE(ins) == ARM_STM) && (ARM_INS_IMMEDIATE(ins) & (1 << ARM_REG_R15))) 
      {
        /* This copies the pc (puts it somewhere in memory). To be real safe
         * we should assume it can be used to return to this address + 4 or 8
         * dependent on the processor implementation.  The only problem with
         * that is that gcc produced binaries put their address on the stack
         * at the start of each procedure.... So we ignore it for now */

        t_arm_ins * saved=ARM_INS_INEXT(ins)?ARM_INS_INEXT(ARM_INS_INEXT(ins)):NULL;
        pc_use_detected_and_resolved = TRUE;
        if (saved)
        {
          /* just to check if it ever really happens outside of the gcc function call code: */
          if (ARM_INS_REGB(ins) != ARM_REG_R13) FATAL(("STM of pc outside of the stack! Check this:\n@I",ins));
        }
      }
      /* }}} */
      /* 8. SYSTEM CALLS {{{ */
      else if (ARM_INS_TYPE(ins) == IT_SWI) 
      {
        /* Actually, we could just consider system calls as normal
         * instructions, if we were sure that they are not long-jump-like
         * system calls. Since we do not know that for sure here, we must
         * assume that is will alter control flow => next instruction is a
         * leader */

        pc_def_detected_and_resolved = TRUE; 
        if (ARM_INS_INEXT(ins) != NULL) {
          ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
        }
      }
      /* }}} */
      /* 9. SWITCH TABLES {{{ */
      else if ((ARM_INS_OPCODE(ins) == ARM_LDR)                                          /* We have a load */
          &&   (ARM_INS_REGB(ins)==ARM_REG_R15)                                          /* from the PC  */
          &&   (!RegsetIsMutualExclusive(ARM_INS_REGS_USE(ins) , ARM_ALL_BUT_PC_AND_COND)) /* + a register (no immediate) */ 
          &&   (ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSL_IMM)                          /* the register is shifted left */
          &&   (ARM_INS_SHIFTLENGTH(ins)==2)                                             /* over two positions */
          &&   (ARM_INS_CONDITION(ins)==ARM_CONDITION_LS)
          )
      {
        t_reloc * rel;

        if (ARM_INS_INEXT(ins) == NULL) FATAL(("No next ins for switch statement")); 
        if (ARM_INS_INEXT(ARM_INS_INEXT(ins)) == NULL) FATAL(("No data for switch statement")); 
        if (ARM_INS_OPCODE(ARM_INS_INEXT(ARM_INS_INEXT(ins)))!=ARM_DATA) FATAL(("No data (2) for switch statement"));

        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
        ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ARM_INS_INEXT(ins)),    ARM_INS_ATTRIB(ARM_INS_INEXT(ARM_INS_INEXT(ins)))|IF_SWITCHTABLE);

        /* Add a relocation to keep the jump table alive.  The relocation is only
         * meant to keep the data block alive as long as the switch instruction
         * is live, it never needs to be relocated, so the write part of the
         * program is empty. */

        rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(ARM_INS_INEXT(ARM_INS_INEXT(ins))),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
        RELOC_SET_LABEL(rel, StringDup("Switch table reloc"));

        pc_use_detected_and_resolved=TRUE;
        pc_def_detected_and_resolved=TRUE;

        /* We should indicate the next instruction (fallthrough) as a bbl leader.
         * The targets of the switch table will be relocated as they are absolute
         * values and thus marked in 12. */
        ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins)) | IF_BBL_LEADER);
      }
      else if ((ARM_INS_OPCODE(ins) == ARM_T2TBB) || (ARM_INS_OPCODE(ins) == ARM_T2TBH))
      {
        t_reloc * rel;
        ASSERT(ARM_INS_INEXT(ins) != NULL, ("No data for TBB/TBH switch statement @I", ins));
        ASSERT(ARM_INS_OPCODE(ARM_INS_INEXT(ins)) == ARM_DATA, ("Data expected immediately after the TBB/TBH instruction @I", ins));

        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)| IF_SWITCHJUMP);
        ARM_INS_SET_ATTRIB(ARM_INS_INEXT(ins), ARM_INS_ATTRIB(ARM_INS_INEXT(ins))| IF_SWITCHTABLE);
        /* unlike in some ARM code, in these thumb2 tables, the data block follows immediately after the tbb */
        rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(ARM_INS_INEXT(ins)),AddressNullForObject(obj),FALSE,NULL,NULL,NULL,"R00A00+\\*\\s0000$");
        RELOC_SET_LABEL(rel, StringDup("TBB/TBH Switch table reloc"));

        pc_use_detected_and_resolved=TRUE;
        pc_def_detected_and_resolved=TRUE;
      }
      /*}}}*/
      /* 10. LOADS USING THE PC {{{ */
      else if ((ARM_INS_TYPE(ins)==IT_LOAD)&&(ARM_INS_REGB(ins)==ARM_REG_R15)&&(ARM_INS_REGA(ins)!=ARM_REG_R15))
      {
        /* We can always ignore loads from the pc, since they will be removed
         * by the const/address producer reduction. If they produce an adress,
         * then it will have an associated relocation (or it is produced in one
         * of the above cases), so the correct basic blocks will be marked. */
        pc_use_detected_and_resolved=TRUE;
      }
      /* }}} */
      /* 11. FLOATING POINT LOADS USING THE PC {{{ */
      else if ((ARM_INS_TYPE(ins)==IT_FLT_LOAD) && (ARM_INS_REGB(ins)==ARM_REG_R15))
      {
        pc_use_detected_and_resolved=TRUE;
      }
      /* }}} */
      /* 12. THE RELOCATIONS (reason 3) {{{ */
      if (ARM_INS_REFED_BY(ins)) 
        ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_BBL_LEADER);
      /* }}} */

      /* check for completeness */
      if ((RegsetIn(ARM_INS_REGS_DEF(ins) , ARM_REG_R15) && !pc_def_detected_and_resolved) || (RegsetIn(ARM_INS_REGS_USE(ins) , ARM_REG_R15) && !pc_use_detected_and_resolved))
      {
        /* so there are pc-defining instructions that we haven't trapped yet */
        FATAL(("Instruction @I defines or uses pc but is not recognized as such!\n",ins));
      }
    }
  }

  /* Now we also mark the beginning of data-blocks as bbl-leaders. We do this by walking the subsections
   * and look for RODATA. The beginning of mapped RODATA subsections are marked as bbl-leaders.
   */
  {
    t_object * tmp, * tmp2;
    t_uint32 tel;
    t_section * sec;
    t_arm_ins * tmp_ins;


    OBJECT_FOREACH_SUBOBJECT(obj,tmp,tmp2)
      OBJECT_FOREACH_SECTION(tmp,sec,tel)
      {
        if (!SECTION_IS_MAPPED(sec)) continue;
        tmp_ins=T_ARM_INS(ObjectGetInsByAddress(obj, SECTION_CADDRESS(sec)));
        if (tmp_ins)
        {
          if (AddressIsEq(ARM_INS_CADDRESS(tmp_ins), SECTION_CADDRESS(sec)))
          {
            ARM_INS_SET_ATTRIB(tmp_ins, ARM_INS_ATTRIB(tmp_ins) | IF_BBL_LEADER); 
          }
        }
      }
  }

  nleaders = 0;
  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    ins = T_ARM_INS(SECTION_DATA(code)); 
    while(ins)
    {
      if (ARM_INS_ATTRIB(ins)&IF_BBL_LEADER) {/*VERBOSE(0,("bblleader: @I\n", ins));fflush(stdout);*/nleaders++;}
      ins=ARM_INS_INEXT(ins);
    }
  }

  HashTableFree(gnu_thumb1_symbol_ht);

  return nleaders;
}
/* }}} */
/*!
 * \todo Document
 *
 * \param bbl_start
 * \param addr
 *
 * \return t_bbl *
*/
/* find_close_bbl_by_address {{{ */
static t_bbl * find_close_bbl_by_address(t_bbl * bbl_start, t_address addr)
{
  t_bbl * bbl=bbl_start;
  t_address base;

  while(bbl)
  {
    if (AddressIsNull(BBL_CSIZE(bbl))) FATAL(("@iB size null",bbl));
    base = BBL_CADDRESS(bbl);
    if (AddressIsLe(base,addr))
    {
      if (AddressIsGt(AddressAdd(base,BBL_CSIZE(bbl)),addr))
      {
	return bbl;
      }
      else
      {
	bbl=BBL_NEXT(bbl);
      }
    }
    else
    {
      bbl=BBL_PREV(bbl);
    }
  }

  return NULL;

}
/* }}} */
/*!
 * \brief 
 *
 * \param start
 * \param reg
 * \param cn
 *
 * \return t_arm_ins *
*/
/* ArmFindConditionalInstructionThatDefinesRegisterInBbl {{{ */
static t_arm_ins * ArmFindConditionalInstructionThatDefinesRegisterInBbl(t_arm_ins * start, t_regset* reg,t_arm_condition_code cn)
{
  t_arm_ins * i_ins=start;
  while((i_ins=ARM_INS_IPREV(i_ins)))
    {
       if ((cn==ARM_INS_CONDITION(i_ins))&&(RegsetIsSubset(ARM_INS_REGS_DEF(i_ins), *reg)))
	return i_ins;
    }
 
  return NULL;
}
/* }}} */
/*!
 * \brief 
 *
 * \param start
 * \param reg
 * \param offset
 *
 * \return t_arm_ins *
*/
/* ArmFindLoadImmThatDefinesRegisterInBbl {{{ */
static t_bool ArmFindLoadImmThatDefinesRegisterInBbl(t_arm_ins * start, t_reg reg, t_uint32 * immvalue)
{
  t_arm_ins * i_ins=start;
  t_arm_ins * last_visited_ins = start;
  *immvalue=0;

  //DEBUG(("-----------------------------------------------------------------------------------------"));
  //DEBUG(("-----------------------------------------------------------------------------------------"));
  //DEBUG(("looking for @I",start));

  while (1)
    {
      
      //DEBUG(("restarted with @I",i_ins));

      while ((i_ins=ARM_INS_IPREV(i_ins)))
        {
          //DEBUG(("went to previous @I",i_ins));

          last_visited_ins = i_ins;
          
          if (RegsetIn(ARM_INS_REGS_DEF(i_ins),reg))
            {
              switch (ARM_INS_OPCODE(i_ins))
                {
                case ARM_ADD:
                case ARM_SUB:
                  /* since it defines reg, ARM_INS_REGA is guaranteed to be reg */
                  if ((ARM_INS_REGB(i_ins) == reg) &&
                      (ARM_INS_FLAGS(i_ins) & FL_IMMED))
                    if (ARM_INS_OPCODE(i_ins)==ARM_ADD)
                      *immvalue+=ARM_INS_IMMEDIATE(i_ins);
                    else
                      *immvalue-=ARM_INS_IMMEDIATE(i_ins);
                  else
                    return FALSE;
                  break;
                case ARM_MOVW:
                  *immvalue+=ARM_INS_IMMEDIATE(i_ins);
                  return TRUE;
                case ARM_MOV:
                  /* since we're going backwards, we may have already processed
                   * add/sub's -> add the immediate rather than just assigning it
                   */
                  if (ARM_INS_FLAGS(i_ins) & FL_IMMED)
                    {
                      *immvalue+=ARM_INS_IMMEDIATE(i_ins);
                      return TRUE;
                    }
                  else
                    return FALSE;
                  break;
                default:
                  return FALSE;
                }
            }
        }
      
      if (BBL_PREV(ARM_INS_BBL(last_visited_ins)) && IS_DATABBL(BBL_PREV(ARM_INS_BBL(last_visited_ins))) && (BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins)))))
        {
          t_arm_ins * new_last_ins = T_ARM_INS(BBL_INS_LAST(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins)))));
          t_uint32 nop_offset = 0;

          if (new_last_ins)
            { 
              //DEBUG(("first new last ins @I",new_last_ins));
              //DEBUG(("in @ieB",ARM_INS_BBL(new_last_ins)));
              //DEBUG(("%d %d %d",ArmInsIsNOOP(new_last_ins),new_last_ins == T_ARM_INS(BBL_INS_FIRST(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins))))),BBL_PREV(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins))))));
            }

          if (new_last_ins && ArmInsIsNOOP(new_last_ins) && new_last_ins == T_ARM_INS(BBL_INS_FIRST(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins))))) && BBL_PREV(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins)))))
            {
              nop_offset = ARM_INS_CSIZE(new_last_ins);
              new_last_ins = T_ARM_INS(BBL_INS_LAST(BBL_PREV(BBL_PREV(BBL_PREV(ARM_INS_BBL(last_visited_ins))))));
              //if (new_last_ins)
              //  DEBUG(("second new last ins @I",new_last_ins));
            }
              
          if (new_last_ins && ARM_INS_OPCODE(new_last_ins)==ARM_B && !ARM_INS_IS_CONDITIONAL(new_last_ins)) 
            {
              //DEBUG(("found unconditional branch @I",new_last_ins));              
              //DEBUG(("last visited ins @I",last_visited_ins));
              t_uint32 data_size = BBL_CSIZE(BBL_PREV(ARM_INS_BBL(last_visited_ins)));
              t_uint32 offset = ARM_INS_IMMEDIATE(new_last_ins);
              t_uint32 extra_offset =  ((ARM_INS_FLAGS(new_last_ins)&FL_THUMB)?4:8)-ARM_INS_CSIZE(new_last_ins);

              //DEBUG(("%d %d %d",data_size,offset,extra_offset));
              if ((data_size+nop_offset)==(offset+extra_offset))
                {
                  i_ins = new_last_ins;
                  //DEBUG(("first continue with @I",new_last_ins));
                  continue;
                }
            }
        }
      
      if (BBL_PREV(ARM_INS_BBL(last_visited_ins)) && !IS_DATABBL(BBL_PREV(ARM_INS_BBL(last_visited_ins))))
        {
          t_arm_ins * new_last_ins = T_ARM_INS(BBL_INS_LAST(BBL_PREV(ARM_INS_BBL(last_visited_ins))));
          if (new_last_ins && (ARM_INS_OPCODE(new_last_ins)==ARM_BL || ARM_INS_OPCODE(new_last_ins)==ARM_BLX) && RegsetIn(arm_description.callee_saved,reg))
            {
              i_ins = new_last_ins;
                //        DEBUG(("second continue with @I",new_last_ins));
              continue;
            }
        }

      break;
    }
  
  return FALSE;
}
/* }}} */
/* ArmFindPCRelLdrThatDefinesRegisterInBbl {{{ */
static t_arm_ins * ArmFindPCRelLdrThatDefinesRegisterInBbl(t_arm_ins * start, t_regset* reg)
{
  t_arm_ins * i_ins=start;
  t_arm_ins *memloc_check = NULL;

  while((i_ins=ARM_INS_IPREV(i_ins)))
  {
    if (memloc_check)
    {
       /* try to figure out where the memory value comes
        * from in case gcc spilled it for some reason
        */

       /* make sure none of the regs used in the memory
        * address we're looking for has been modified
        */
       if (!RegsetIsMutualExclusive(ARM_INS_REGS_DEF(i_ins),ARM_INS_REGS_USE(memloc_check)))
         return NULL;
       /* make sure the location we're tracking is not
        * overwritten
        */
       if (ARM_INS_IS_MEMORY(i_ins))
       {
         if (ARM_INS_TYPE(i_ins) == IT_STORE_MULTIPLE)
           return NULL;
	 if ((ARM_INS_TYPE(i_ins) == IT_STORE) ||
             (ARM_INS_TYPE(i_ins) == IT_FLT_STORE))
         {
	   int store_size;

	   switch (ARM_INS_OPCODE(i_ins))
	   {
	     case ARM_STR:
	       store_size=4;
	       break;
	     case ARM_STRB:
	       store_size=1;
	       break;
	     case ARM_STRH:
	       store_size=2;
	       break;
	     default:
               /* we don't handle differently sized stores */
	       return NULL;
	   }
           /* may be indirect pointer store overwriting the value */
           if (((ARM_INS_REGB(i_ins) != ARM_INS_REGB(memloc_check)) ||
               (ARM_INS_REGC(i_ins) != ARM_INS_REGC(memloc_check)) ||
               (ARM_INS_SHIFTTYPE(i_ins) != ARM_SHIFT_TYPE_NONE)))
             return NULL;
           /* if the offset is the same as that of the load, then it's a
            * store to the same address, otherwise to a different one
            */
           if ((ARM_INS_IMMEDIATE(i_ins) == ARM_INS_IMMEDIATE(memloc_check)) &&
	       (store_size==4))
           {
             /* we have found the definition of the memory location
              * -> start tracking the new register
              */
             RegsetSetAddReg(*reg,ARM_INS_REGA(i_ins));
             memloc_check = NULL;
           }
	   else
	   {
	     /* in case of an overlapping store -> abort */
	     if ((ARM_INS_IMMEDIATE(i_ins) <= ARM_INS_IMMEDIATE(memloc_check)) &&
	         (ARM_INS_IMMEDIATE(i_ins)+store_size > ARM_INS_IMMEDIATE(memloc_check)))
	       return NULL;
	   }
         }
       }
    }

    /* An empty regset is a subset of any other regset,
     * but that's not what we are looking for (*reg
     * can become empty when we are looking for a
     * defining load)
     */
    if (!RegsetIsEmpty(*reg) &&
        RegsetIsSubset(ARM_INS_REGS_DEF(i_ins), *reg))
    {
      if (ARM_INS_OPCODE(i_ins) == ARM_LDR)
      {
	if (ARM_INS_REGB(i_ins) == ARM_REG_R15)
	{
	  if ((ARM_INS_REGC(i_ins) == ARM_REG_NONE) &&
	      (ARM_INS_SHIFTTYPE(i_ins) == ARM_SHIFT_TYPE_NONE))
	    return i_ins;
	  else
	    return NULL;
	}
	else
	{
	  if (ARM_INS_SHIFTTYPE(i_ins) == ARM_SHIFT_TYPE_NONE)
	  {
	    /* stop tracking the overwritten register, and
	     * start looking for the definition of the memory
	     * location
	     */
	    RegsetSetSubReg(*reg,ARM_INS_REGA(i_ins));
	    memloc_check = i_ins;
	  }
	  else
	    return NULL;
	}
      }
      else
        return NULL;
    }
  }

  return NULL;
}
/* }}} */
/*!
 * Turns position-dependent instructions (i.e. instructions that use the pc)
 * into pseudo-operations that no longer depend on the program conter. On the
 * arm the pc is mostly used to produce addresses, hence the name of this
 * function.
 *
 * \param code
 *
 * \return void 
*/
/* ArmMakeAddressProducers {{{ */


static t_bool IsRel32Reloc(t_reloc *rel)
{
  return
    strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+P-A00+!R00A01+M|\\",38)==0 ||
    strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+P-A00+!\\",29)==0 ||
    strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+Z00+P-A00+!R00A01+Z00+M|\\",46)==0 ||
    strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+Z00+P-A00+!\\",33)==0 ||
    strncmp(RELOC_CODE(rel),"R00A01+A00+P-\\",14)==0 ||
    strncmp(RELOC_CODE(rel),"gP-A00+\\",8)==0;
}


static t_string
RelocRel32GetEquivAbs32RelocCode(t_reloc *rel)
{
  t_string newcode;
  /* R00 = target bbl/section, A01 = offset within bbl/section, A00 = addend of relocation */
  if ((strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+P-A00+!R00A01+M|\\",38)==0))
  {
    newcode="R00A01+A00+R00A01+M|\\l*w\\s0000$";
  }
  else if ((strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+P-A00+!\\",29)==0))
  {
    newcode="R00A01+A00+\\l*w\\s0000$";
  }
  else if ((strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+Z00+P-A00+!R00A01+Z00+M|\\",46)==0))
  {
    newcode="R00A01+Z00+A00+R00A01+Z00+M|\\l*w\\s0000$";
  }
  else if ((strncmp(RELOC_CODE(rel),"U00?s0000A00+:R00A01+Z00+P-A00+!\\",33)==0))
  {
    newcode="R00A01+Z00+A00+\\l*w\\s0000$";
  }
  else if((strncmp(RELOC_CODE(rel),"R00A01+A00+P-\\",14)==0))
  {
    newcode="R00A01+A00+\\l*w\\s0000$";
  }
  else if((strncmp(RELOC_CODE(rel),"gP-A00+\\",8)==0))
  {
    newcode="gA00+\\l*w\\s0000$";
  }
  else
  {
    FATAL(("Unsupported supposed REL32 relocation code in @R",rel));
  }
  return newcode;
}


void * BblAddressKey(t_bbl * bbl, int i)
{
  t_uint32 * result = (t_uint32*) Malloc(3*sizeof(t_uint32));
  *result = (G_T_UINT32(BBL_CADDRESS(bbl))/1024) + i;
  *(result+1) = G_T_UINT32(BBL_CADDRESS(bbl));
  *(result+2) = G_T_UINT32(BBL_CSIZE(bbl));

  return result;
}

void BblHeAddrKeyFree(const void * he1, void * data)
{
  t_bbl_he* he = (t_bbl_he* ) he1;
  Free(HASH_TABLE_NODE_KEY(T_HASH_TABLE_NODE(he)));
  Free(he);
}

t_uint32 BblHashAddress(const void * key, const t_hash_table * table)
{
  return (*((t_uint32 *) key)) % HASH_TABLE_TSIZE(table);
}

t_int32 AddressCmp(const void* key, const void* key2)		/*compares fingerprints*/
{
 t_uint32 n1=*((t_uint32 *) key+1);
 t_uint32 n2=*((t_uint32 *) key+2);
 t_uint32 m1=*((t_uint32 *) key2+1);

 if (m1>=n1 && m1<n1+n2)
   return 0;

 return 1;
}


t_bbl *
AddressToBbl(t_hash_table *addresses_to_blocks, t_cfg *cfg, t_address adest)
{
  t_bbl* bdest;
  t_uint64 address_to_look;
  void * address_to_look_ptr = &address_to_look;
  t_address_to_bbl_he * node;

  if (addresses_to_blocks)
  {
    *(t_uint32*)address_to_look_ptr = G_T_UINT32(adest)/1024;
    *(((t_uint32*)address_to_look_ptr)+1) = G_T_UINT32(adest);
    node = HashTableLookup(addresses_to_blocks,&address_to_look);

    if (node)
    {
      /* in this case, the address was the starting address of a block */
      return node->bbl;
    }
  }
  /* not in the hash table (-> middle of a bbl) or no hash table
   * -> the brute force way
   */
    CFG_FOREACH_BBL(cfg,bdest)
    if (AddressIsLe(BBL_CADDRESS(bdest),adest) &&
        AddressIsGt(AddressAdd(BBL_CADDRESS(bdest),BBL_CSIZE(bdest)),adest))
      return bdest;
  return NULL;
}


void ArmMakeAddressProducers(t_cfg *cfg)
{
  t_object *obj = CFG_OBJECT(cfg);
  t_bbl * bbl, * bbl2;
  t_arm_ins * ins, * def;
  t_arm_ins * ins2=NULL;
  t_reloc * rel=NULL;
  t_address address, load_address; 
  t_uint64 value;
  t_hash_table * addresses_to_blocks;
  t_bool getvalue_failed;
  int counter = 0;
  /* When we encounter an address producer we want to rewrite its relocation but can't always do this
   * because other address producers might use the same relocation. Therefore we will duplicate the relocation
   * and subsequently adjust it. This means that at the end the original relocations still exist however.
   * Therefore we will move these relocations to a separate relocation table and free it (with its relocs)
   * at the end. Take care to copy the reloc table's callbacks.
   */
  t_reloc_table* relocs_to_remove = RelocTableNew(NULL);
  RELOC_TABLE_SET_DEL_EDGE_CALLBACK(relocs_to_remove, RELOC_TABLE_DEL_EDGE_CALLBACK(OBJECT_RELOC_TABLE(obj)));
  RELOC_TABLE_SET_DEL_SWITCH_EDGE_CALLBACK(relocs_to_remove, RELOC_TABLE_DEL_SWITCH_EDGE_CALLBACK(OBJECT_RELOC_TABLE(obj)));

  t_ptr_array add_todos;
  PtrArrayInit(&add_todos,FALSE);

  addresses_to_blocks = NULL;

  /* 1. The addition of the PC with a register. We should be able to
   * find out what the value in the register is.  Normally it will be
   * a loaded self-relative value. We must convert this since it
   * introduces a dependency between the datablock in which the
   * self-relative value is stored and the instruction that does the
   * addition to the pc 
   *
   * {{{ */

  CFG_FOREACH_BBL(cfg,bbl)
  {
    if (!IS_DATABBL(bbl))
    {
      address = BBL_CADDRESS(bbl);
      BBL_FOREACH_ARM_INS(bbl,ins)
      {
	t_uint32 table_entry_size;
	address=AddressAdd(address,ARM_INS_CSIZE(ins));

        /* check for thumb compact switch table pattern */
        if ((ARM_INS_OPCODE(ins)==ARM_ADD) && IsThumbSwitchDispatch(bbl,&table_entry_size) && ((ARM_INS_REGB(ins) == ARM_REG_R15) || (ARM_INS_REGC(ins) == ARM_REG_R15) ))
        {
          t_arm_ins *data;
          t_bbl *databbl;

	  if (ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP)  continue; /* Skip switches */
          ins2 = ARM_INS_INEXT(ins);
          if (!ins2 ||
              ((ARM_INS_OPCODE(ins2)!=ARM_LDRB) &&
	       (ARM_INS_OPCODE(ins2)!=ARM_LDRH)) ||
              (ARM_INS_REGA(ins2)!=ARM_INS_REGA(ins)) ||
              (ARM_INS_REGB(ins2)!=ARM_INS_REGA(ins)) ||
              (ARM_INS_IMMEDIATE(ins2) != 4))
            FATAL(("Did not find thumb compact switch table load after @I",ins));
          /* check that the load goes to the next bbl and that it's a data bbl */
          if (!ARM_INS_INEXT(ins2) ||
              !ARM_INS_INEXT(ARM_INS_INEXT(ins2)) ||
              ARM_INS_INEXT(ARM_INS_INEXT(ARM_INS_INEXT(ins2))))
            FATAL(("Did not find data bbl for thumb compact switch table load at @I",ins2));
          databbl = BBL_NEXT(bbl);
          data = T_ARM_INS(BBL_INS_FIRST(databbl));
          if (ARM_INS_OPCODE(data) != ARM_DATA)
            FATAL(("Did not find data bbl for thumb compact switch table load at @I",ins2));
          /* s0002- because the code is:
           *
           * add     r1, pc
           * ldrb    r1, [r1, #4] (or ldrh)
           * adds    r1, r1, r1 (or lsl 1 in case of ldrh, for some reason)
           * add     pc, r1
           * .data1
           * .data2
	   * [.data3 ...]
           *
           * the first instruction makes r1 point to to the adds+orig_r1 (pc+4 is added),
           * so we need 4 more to get to .data1
           * The relocation however encodes the offset in ldrb and hence specifies it as R00.
           * Since it comes 2 bytes after the add, we need 4-2=2
           */
          RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj), T_RELOCATABLE(ins2), AddressNullForObject(obj), T_RELOCATABLE(databbl), AddressNullForObject(obj), FALSE, NULL, NULL, NULL, "R00P-s0002-\\=s0006<ksf83f&|v\\iffffffe0&$");
          /* mark the first add also as a switch jump, so the rest of the code doesn't complain about it */
          ARM_INS_SET_ATTRIB(ins,ARM_INS_ATTRIB(ins)|IF_SWITCHJUMP);
        }
	else if ((ARM_INS_OPCODE(ins)==ARM_ADD) &&  (! (ARM_INS_FLAGS(ins) & FL_IMMED)) && (ARM_INS_TYPE(ins) == IT_DATAPROC) && ((ARM_INS_REGB(ins) == ARM_REG_R15) || (ARM_INS_REGC(ins) == ARM_REG_R15) ))
	{
	  t_regset search=ARM_INS_REGS_USE(ins);
	  
	  RegsetSetSubReg(search,ARM_REG_R15);
	  RegsetSetSubReg(search,ARM_REG_CPSR);

	  /* The cases that we can handle: 
	   *
	   * a. Addition of a value to the pc: ADD pc, pc, reg
	   * if we find the definition of reg
	   * b. Switch statements: ADDLS pc,pc, reg << 8
	   * safe to ignore these, they are handled earlier on */

	  if ((ARM_INS_OPCODE(ins) == ARM_ADD)&&(ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP)) continue; /* Skip switches */
	  if (ARM_INS_OPCODE(ins) != ARM_ADD) FATAL(("Strange... expected and ADD pc, pc, reg or ADD pc, reg, pc"));

	  VERBOSE(1,("Handling         @G: @I",address,ins));

	  /* Find the definition of the register that is added to the pc */

	  if (!ARM_INS_IS_CONDITIONAL(ins)) {
	    def=ArmFindPCRelLdrThatDefinesRegisterInBbl(ins,&search);
	  }
	  else 
	  {
	    RegsetSetSubReg(search,ARM_REG_Q_CONDITION);
	    RegsetSetSubReg(search,ARM_REG_V_CONDITION);
	    RegsetSetSubReg(search,ARM_REG_Z_CONDITION);
	    RegsetSetSubReg(search,ARM_REG_N_CONDITION);
	    RegsetSetSubReg(search,ARM_REG_C_CONDITION);
	    def=ArmFindConditionalInstructionThatDefinesRegisterInBbl(ins,&search,ARM_INS_CONDITION(ins));
	  }

	  /* We must find the definition or we cannot handle this */

	  if (!def)
          {
            /* may still be part of a REL32 construct that we can handle when processing the
             * corresponding LDR; keep track of this instruction to ensure that we do
             * process it later
             */
            PtrArrayAdd(&add_todos,ins);
            continue;
          }

	  VERBOSE(1,("Definition  =    @I", def));

	  /* Check if it is a pc-relative load, if not we cannot handle this */

          if (ARM_INS_OPCODE(def)==ARM_LDR && (ARM_INS_REGB(def)==ARM_REG_R13 || ARM_INS_REGB(def)==ARM_REG_R11) && (ARM_INS_FLAGS(def) & FL_IMMED))
            {
              /* value is spilled to the stack, so we'll need the more global search method to 
                 locate the other part of the address production */
              PtrArrayAdd(&add_todos,ins);
              continue;
            }


	  if ((ARM_INS_OPCODE(def)!=ARM_LDR) || (ARM_INS_REGB(def)!=ARM_REG_R15)) FATAL(("At @G: @I: PC-relative value is not loaded (no PC-relative LDR, the instruction for the definition is @I). This is in experimental, but essential code: mail this binary along with the error message to diablo@@lists.ugent.be",address, ins, def));

	  /* Calculate the address from where we are loading, perform the load and get the relocation */
          value = GetPcRelLoadedValue(obj,def,&load_address,&rel,&getvalue_failed);

          if (getvalue_failed)
          {
            VERBOSE(5,("@I is possibly part of complex pc-rel load, adding result to @I; will try to handle it when looking at the adds",def,ins));
            PtrArrayAdd(&add_todos,ins);
            continue;
          }

	  if (rel) VERBOSE(1,("Loaded reloc =    @R",rel));

	  /* If we do not have a relocation, than we have troubles: 
	   * this is probably weird assembler code
	   * we cannot handle */

	  if (!rel)
          {
            VERBOSE(5,("The relocation (for a PC-relative load) was not found. Will check whether we can handle it via its corresponding add later."));
            PtrArrayAdd(&add_todos,ins);
            continue;
          }

	  /* If it is not a self-relative value, we cannot
	     handle it */

	  if (!StringPatternMatch("*P*",RELOC_CODE(rel)))
          {
	    VERBOSE(5,("Relocation used for PC-relative displacement is not SREL32! Will check whether we can handle it via its corresponding add later. @R",rel));
            PtrArrayAdd(&add_todos,ins);
            continue;
          }

          if (RELOC_N_TO_RELOCATABLES(rel)>1)
          {
            VERBOSE(5,("Complex address producer, will check later whether we can handle it via its corresponding add:  @R\n", rel));
            PtrArrayAdd(&add_todos,ins);
            continue;
          }

	  /* Now let's find out what the value is we want to
	   * produce: There are a number of possibilities:
	   * a. an ABS_SYMBOL. This value should never be
	   * relocated, so we can change it into a constant
	   * producer b. Otherwise its an address, so we
	   * should convert it to an address producer. There
	   * are still 2 posibilities: 1. The symbol is weak,
	   * then it has no real value. In that case we point
	   * to the value we're loading from (self-relativity)
	   * 2. Otherwise we produce the symbol's address */

	  if   ((RELOC_N_TO_RELOCATABLES(rel)==0) || (!((RELOC_TO_RELOCATABLE(rel)[0]==T_RELOCATABLE(OBJECT_ABS_SECTION(obj))) || (RELOC_TO_RELOCATABLE(rel)[0]==T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))))) )  /* b */
	  {
	    /* replace the ADD with produce pointer instruction*/
	    
	    if ((RELOC_N_TO_RELOCATABLES(rel)) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_INS))
	    {
	      FATAL(("Relocation still to instruction!"));
	    }
	    else if ((RELOC_N_TO_RELOCATABLES(rel))
                && RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_SUBSECTION
                && SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(rel)[0])) == CODE_SECTION)
	    {
	      FATAL(("Relocation still to code section in reloc\n@R %p", rel, rel));
	    }
	    else if ((RELOC_N_TO_RELOCATABLES(rel)==0) || (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_BBL) || (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_SUBSECTION) || (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_SECTION))
	    {
	      t_string newcode;

	      /* since we're going to change the reloc, duplicate it because others may also use it
	       * (and they may use it from another address -> will also adjust it, and moreover
	       *  in a different way)
	       */
        RelocMoveToRelocTable(rel, relocs_to_remove);
	      rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), rel);

              newcode=RelocRel32GetEquivAbs32RelocCode(rel);
              Free(RELOC_CODE(rel));
              RELOC_SET_CODE(rel,StringDup(newcode));

              if(ARM_INS_FLAGS(ins) & FL_THUMB)
                RELOC_ADDENDS(rel)[0]=AddressSub(RELOC_ADDENDS(rel)[0],AddressAddUint32(AddressSub(load_address,ARM_INS_CADDRESS(ins)),-4));
              else
                RELOC_ADDENDS(rel)[0]=AddressSub(RELOC_ADDENDS(rel)[0],AddressAddUint32(AddressSub(load_address,ARM_INS_CADDRESS(ins)),-8));
		
	      RelocSetFrom(rel, T_RELOCATABLE(ins));
	      RELOC_SET_FROM_OFFSET(rel,  AddressNullForObject(obj));
	    }
	    else
	    {
	      VERBOSE(5,("%p %p\n",RELOC_TO_RELOCATABLE(rel)[0],OBJECT_ABS_SECTION(obj)));
	      VERBOSE(5,("Cannot handle relocation here, will try when handling ldr for @R",rel));
              PtrArrayAdd(&add_todos,ins);
              continue;
	    }

	    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	    ArmMakeAddressProducer(__FILE__,__LINE__,ins, value + G_T_UINT32(address) + G_T_UINT32(ARM_INS_CSIZE(ins)), rel);

#ifdef KEEP_INFO
	    ARM_INS_SET_INFO(ins, ArmAddrInfoNew());
	    ARM_INS_INFO(ins)->original_type='A';
#endif
	  } 
	  else  /* a */
	  { 
	    if ((RELOC_TO_RELOCATABLE(rel)[0]) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_SECTION) && ((RELOC_TO_RELOCATABLE(rel)[0]==T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))) || (RELOC_TO_RELOCATABLE(rel)[0] == T_RELOCATABLE(OBJECT_ABS_SECTION(obj)))))
            {
              /* case of ABS_SYMBOL */
              /* replace the ADD with an constant producer instruction */
	      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
              ArmMakeConstantProducer(ins,value + G_T_UINT32(address) + G_T_UINT32(ARM_INS_CSIZE(ins)));
	      ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins)| FL_ORIG_CONST_PROD);
#ifdef KEEP_INFO
	    ARM_INS_SET_INFO(ins, ArmAddrInfoNew());
	    ARM_INS_INFO(ins)->original_type='A';
#endif

              VERBOSE(1,("At @G  no reloc (ABS_SYMBOL) (immed=%d,loading from @G) (%x)!",address, ARM_INS_IMMEDIATE(ins), load_address, value)); 
            }
	    else if (strncmp(RELOC_CODE(rel),"PP-A00+\\",8)==0 || strncmp(RELOC_CODE(rel),"PP-A00+PM +\\",12)==0 || strncmp(RELOC_CODE(rel),"P  P-A00+\\",10)==0)
	    {
	      t_bbl * found=NULL;

	      /*VERBOSE(0,("@G = @G = @G - @G\n",rel->addend,AddressAddUint32(AddressSub(load_address,ARM_INS_CADDRESS(ins)),-8),load_address,ARM_INS_CADDRESS(ins)));*/

	      /* symbol is weak undef, and so we must make relocation point 
	       * to load address(load_address), TODO : check this */
	      CFG_FOREACH_BBL(cfg,bbl2)
	      {
		t_address base;
		base = BBL_CADDRESS(bbl2);
		if ((AddressIsLe(base,load_address))&&(AddressIsGt(AddressAddUint32(base,BBL_NINS(bbl2)*G_T_UINT32(ARM_INS_CSIZE(ins))),load_address)))
		{ 
		  found = T_BBL(bbl2); 
		  break; 
		}
	      }

	      if (!found)
              {
                PtrArrayAdd(&add_todos,ins);
                continue;
              }
	     

	      rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj),T_RELOCATABLE(ins), AddressNullForObject(obj), T_RELOCATABLE(found),AddressSub(load_address,BBL_CADDRESS(bbl)), RELOC_HELL(rel),RELOC_EDGE(rel),NULL,NULL,"R00\\l*w\\s0000$");
	      RELOC_SET_LABEL(rel, StringDup("Weak undefined symbol"));
	      
	      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	      ArmMakeAddressProducer(__FILE__,__LINE__,ins, value + G_T_UINT32(address) + G_T_UINT32(ARM_INS_CSIZE(ins)), rel);
	      
#ifdef KEEP_INFO
	    ARM_INS_SET_INFO(ins, ArmAddrInfoNew());
	    ARM_INS_INFO(ins)->original_type='A';
#endif

	    }
	    else
            {
	      VERBOSE(5,("Unrecognized reloc code for pcrel load, will try to handle later @R",rel));
              PtrArrayAdd(&add_todos,ins);
              continue;
            }
	  }
	  /* Replace the load with a no-op. We cannot delete
	     it since this would change addresses, and we
	     still need them! */

	  ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(def));
	  ArmInsMakeNoop(def);
#ifdef KEEP_INFO
	  ARM_INS_SET_INFO(def, ArmAddrInfoNew());
	  ARM_INS_INFO(def)->original_type='L';
	  ARM_INS_INFO(def)->load_address=G_T_UINT32(load_address)-G_T_UINT32(ARM_INS_CADDRESS(def))+G_T_UINT32(ARM_INS_OLD_ADDRESS(def));
	  ARM_INS_INFO(def)->load_value=value;
#endif
	}
      }
    }
  }

  /* end of 1. }}} */


  /* 2. Loads relative to the pc.  This is very similar to the previous case.
   * No need to find any definition. We just need to remove it, since it
   * introduces dependencies between the pc and the address of the loaded value
   * 
   * {{{ */

  CFG_FOREACH_BBL(cfg,bbl)
  {
    if (IS_DATABBL(bbl)) continue; 
    /*address = BBL_CADDRESS(bbl);*/
    BBL_FOREACH_ARM_INS(bbl,ins) /* Check all instructions in the block */
    {
      address = ARM_INS_CADDRESS(ins);

      if ((ARM_INS_TYPE(ins) == IT_LOAD) && (ARM_INS_REGB(ins) == ARM_REG_R15)  /* It has to be a load from the pc, that either  */


	  && ((RegsetIsMutualExclusive(ARM_INS_REGS_USE(ins) , ARM_ALL_BUT_PC_AND_COND)) || /* a. uses no other registers than the pc */ 
	    (!((ARM_INS_OPCODE(ins) == ARM_LDR)&&(ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSL_IMM))) )) /*b. is not a switch */  
      {
        t_bool is_tls_reloc, failed;
	/* We should have a normal load or we cannot handle it */

	if (ARM_INS_OPCODE(ins)!=ARM_LDR && ARM_INS_OPCODE(ins)!=ARM_LDRH && ARM_INS_OPCODE(ins)!=ARM_LDRD) FATAL(("PC-relative value is not loaded (no LDR, but @I). This is in experimental, but essential code: mail this binary along with the error message to bdebus@@elis.ugent.be",ins));

	/* Calculate the address form which we are loading and get the loaded value and relocation */
        value = GetPcRelLoadedValue(obj,ins,&load_address,&rel,&failed);

        /* normally these are already processed by the TLS_IE32 code under b)
         * before we get to them, but due to code reordering we may get to
         * them before that happens -> skip
         */
        if (failed)
          continue;

	/* Two possibilies: 
	 *
	 * a. The load is used to produce a (large) constant value: no
	 * relocation or ABS relocation. It is also possible that we load a
	 * WEAK UNDEFINED value. In this case, 0 is produced sothe load
	 * becomes a constant producer, not an address producer. Finally,
         * we could also be calculating the offset between two addresses
         * in a .tbss/.tdata section. Since we don't remove items from
         * such a section, this is a constant as well.
	 *
	 * b. An address is loaded (most common case) 
	 */

#ifdef KEEP_INFO
	  ARM_INS_SET_INFO(ins, ArmAddrInfoNew());
	  ARM_INS_INFO(ins)->original_type='L';
	  ARM_INS_INFO(ins)->load_address=G_T_UINT32(load_address)-G_T_UINT32(ARM_INS_CADDRESS(ins))+G_T_UINT32(ARM_INS_OLD_ADDRESS(ins));
	  ARM_INS_INFO(ins)->load_value=value;
#endif

        is_tls_reloc = rel &&
            (((strncmp(RELOC_CODE(rel),"R00A01+A00+R01A02+-s0008+\\",26)==0) ||
              (strncmp(RELOC_CODE(rel),"R00A01+A00+R01A02+-\\",20)==0)) &&
             (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[1]) == RT_SECTION) &&
             ((strcmp(SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(rel)[1])),".tdata")==0) ||
              (strcmp(SECTION_NAME(T_SECTION(RELOC_TO_RELOCATABLE(rel)[1])),".tbss")==0)));

	if ((!rel)||((RELOC_N_TO_RELOCATABLES(rel) == 1)&&(RELOC_TO_RELOCATABLE(rel)[0] == T_RELOCATABLE(OBJECT_ABS_SECTION(obj)))) ||
            is_tls_reloc) /* a */
	{
          if (is_tls_reloc)
	  {
      RelocMoveToRelocTable(rel, relocs_to_remove);
	    rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), rel);
	  }

	  /* frees the original relocation */
	  ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	  ArmMakeConstantProducer(ins,value & 0xffffffff);
          if (value & 0xffffffff00000000ULL)
          {
            /* create a second instruction, producing the next half-constant in the next register */
            t_arm_ins *new_ins = ArmInsNewForBbl(ARM_INS_BBL(ins));
            ARM_INS_SET_REGA(new_ins, ARM_INS_REGA(ins)+1);
            ArmInsUnconditionalize(new_ins);
            ArmMakeConstantProducer(new_ins, value >> 32);
            ArmInsInsertAfter(new_ins, ins);
          }

	  if (is_tls_reloc)
	  {
	    /* turn into a dummy relocation to keep the tbss/tdata entry alive */
	    RelocSetFrom(rel, T_RELOCATABLE(ins));
	    RELOC_SET_FROM_OFFSET(rel,  AddressNullForObject(obj));
	    if (RELOC_LABEL(rel)) 
	      Free(RELOC_LABEL(rel));
	    RELOC_SET_LABEL(rel, StringDup("tls keepalive"));
	    Free(RELOC_CODE(rel));
	    RELOC_SET_CODE(rel, StringIo("R01*i%08x\\*\\s0000$",value));
	  }
	  ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) | FL_ORIG_CONST_PROD);
	  VERBOSE(1,("At @G no (real) reloc (immed=%lld,loading from @G) (%x)!",address,ARM_INS_IMMEDIATE(ins), load_address, value)); 
	}
	else  /* b */
	{
          t_address newaddend = AddressNullForObject(obj);

          ASSERT(ARM_INS_OPCODE(ins) != ARM_LDRD, ("did not expect this instruction @I", ins));
          if (strncmp(RELOC_CODE(rel),"R00A01+A00+R01A02+-s0004-\\",26)==0)
          {
            /* Transform TLS_IE32 relocations into ABS32 relocations {{{*/
            t_arm_ins *actual_load, *data_ins;
            t_reloc *nrel;
            /* This relocation is used in code like this:
             * GOTEntry:
             *   offset_from_tls_start_of_TLSVAR
             * ...
             *   LDR r3, [pc,#x]  ; pc+8+#x == addr, load the offset between the next LDR and the GOT entry above
             *  ...
             *   LDR/ADD r4, pc, r3   ; load the GOT entry
             *  ...
             * addr:
             *  "R00A01+A00+R01A02+-s0004-", with R00 = GOT entry and R01 = second LDR
             *    (s0004- both for ARM/Thumb, see "NOTE" at the end of the comment for
             *     the R_ARM_TLS_IE32 relocation in diabloelf_arm.c)
             *
             * We can change the first load (= instruction we are currently processing) into an address
             * producer for the GOT entry (that will be done by the generic code) and change the second
             * one into a non-PC relative load (which is what we'll do here).
             */

             /* Some sanity checks */
             ASSERT((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_SUBSECTION)
                    && (strcmp(SECTION_NAME(SECTION_PARENT_SECTION(T_SECTION(RELOC_TO_RELOCATABLE(rel)[0]))),".got")==0),("TLS_IE32 relocation related to @I not referring to .got section?",ins));
             ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[1])==RT_BBL,("TLS_IE32 relocation related to @I not referring to BBL?",ins));
             /* get the second load (ObjectGetInsByAddress is not possible at this point) */
             {
               t_address load_addr;
               t_bbl *load_bbl;

               load_bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[1]);
               load_addr = AddressAdd(RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[1]),RELOC_TO_RELOCATABLE_OFFSET(rel)[1]);
               if (!(ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(load_bbl))) & FL_THUMB))
               {
                 /* see "NOTE" at the end of the comment for the R_ARM_TLS_IE32 relocation in diabloelf_arm.c */
                 load_addr = AddressSubUint32(load_addr,4);
               }
               /* since an instruction referred by an instruction causes the start
                * of a new bbl (-> loadaddr should correspond to the first instruction
                * of load_bbl), get the bbl containing this new address
                */
               load_bbl = find_close_bbl_by_address(load_bbl,load_addr);
               BBL_FOREACH_ARM_INS(load_bbl,actual_load)
               {
                 if (AddressIsEq(ARM_INS_CADDRESS(actual_load),load_addr))
                   break;
               }
               ASSERT(actual_load,("Could not find second part of GOT load for first LDR @I at @G in BBL @iB",ins,load_addr,load_bbl));
             }
             /* in case this instruction is a load, transform it in a non-PC-relative load */
             if (ARM_INS_OPCODE(actual_load) == ARM_LDR)
             {
               /* check whether it looks like we expect */
               ASSERT((ARM_INS_OPCODE(actual_load) == ARM_LDR) && (ARM_INS_REGB(actual_load) == ARM_REG_R15) && (ARM_INS_REGC(actual_load) == ARM_INS_REGA(ins)), ("Second part of GOT load is not a PC-relative LDR using register of first LDR:\n  First LDR: @I\n  Second LDR: @I",ins,actual_load));
               /* turn it in a plain load */
               ArmInsMakeLdr(actual_load,ARM_INS_REGA(actual_load),ARM_INS_REGC(actual_load),ARM_REG_NONE,0,ARM_INS_CONDITION(actual_load),TRUE,TRUE,FALSE);

             }
             /* otherwise it has to be an add -> turn into a Noop, since
              * our address producer will already calculate the entire address
              */
             else
             {
               /* don't sanity-check REGB of actual_load (in theory the same as REGA of ins),
                * because in some cases the compiler inserts a move to a different register
                * between the load and this add
                */
               ASSERT((ARM_INS_OPCODE(actual_load) == ARM_ADD) && ((ARM_INS_REGB(actual_load) == ARM_REG_R15) || (ARM_INS_REGC(actual_load) == ARM_REG_R15)), ("Second part of GOT load is neither load nor an apprpriate ADD:\n  First LDR: @I\n  Second ins: @I\nregb: %d, regc: %d, oldrega: %d",ins,actual_load,ARM_INS_REGB(actual_load),ARM_INS_REGC(actual_load),ARM_INS_REGA(ins)));
	       ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(actual_load));
               ArmInsMakeNoop(actual_load);
             }

             /* create the new relocation for the data ins */
             data_ins = T_ARM_INS(RELOC_FROM(rel));
             nrel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForIns(data_ins),T_RELOCATABLE(data_ins),AddressNullForIns(data_ins),RELOC_TO_RELOCATABLE(rel)[0],RELOC_TO_RELOCATABLE_OFFSET(rel)[0],RELOC_HELL(rel),RELOC_EDGE(rel),NULL,NULL,"R00A01+A00+\\l*w\\s0000$");
             RELOC_SET_LABEL(nrel, StringDup(RELOC_LABEL(rel)));
             RelocAddAddend(nrel,AddressNullForIns(ins));
             /* since we reuse the edge, make sure it isn't freed */
             RELOC_SET_EDGE(rel,NULL);

             /* update value and rel for the new situation */
             value = G_T_UINT32(AddressAdd(RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[0]),RELOC_TO_RELOCATABLE_OFFSET(rel)[0]));

             /* free the old relocation */
	     RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj), rel);

             rel = nrel;
             /* now let the generic code turn this load into an address producer */
            /*}}}*/
          }
          else if (IsRel32Reloc(rel))
          {
            /* Transform (some) REL32 relocations into ABS32 relocations {{{ */
            t_arm_ins *actual_load = NULL, *data_ins;
            t_reloc *nrel;
            t_bool is_thumb_ins = (ARM_INS_FLAGS(ins) & FL_THUMB) ? 1 : 0;
            /* This relocation is used in code like this:
             *   LDR r3, [pc,#x]  ; pc+8+#x == addr, load the offset between the next LDR/ADD and the target data
             *  ...
             *  .LPIC:
             *   LDR(/ADD) r4, pc, r3   ; load the (address of the) data entry (r3=s-P+(addr-LPIC), with P==addr; pc=LPIC);
             *
             *  addr
             *   "U00?s0000A00+:R00A01+P-A00+!R00A01+M|" with A00 = negative offset between addr and the second LDR/ADD
             *
             * or
             *   LDR r3, [pc,#x]        ; pc+8+#x == addr, load the offset between the next LDR/ADD and the target data
             *  ...
             *   ADD r4, pc, #y         ; load address of address pool entry in r4
             *  ...
             *   LDR/ADD r5, r4, r3     ; load from/calculate final address (r3=g-P+A; r4=P -> r3+r4=g+A)
             *  ...
             * addr
             *   "gP-A00+" with A00 = offset in the GOT
             *
             * We can change the first load (= instruction we are currently processing) into an address
             * producer for the data entry (that will be done by the generic code) and change the second
             * one into a non-PC relative load (which is what we'll do here).
             *
             * Overall, this is fairly similar to the TLS_IE32 code above
             */

             /* Some sanity checks (rel may not point to relocatables in case of R_ARM_GOTPC)  */
             ASSERT(RELOC_N_TO_RELOCATABLES(rel)==0 || RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_SUBSECTION || RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_BBL,("REL32 relocation related to @I not referring to section but to @T",ins,RELOC_TO_RELOCATABLE(rel)[0]));
             data_ins = T_ARM_INS(RELOC_FROM(rel));
             /* get the second load (ObjectGetInsByAddress is not possible at this point) */
             {
               t_address load_addr;
               t_uint32 pc_offset = 8;
               t_bbl *bdest;

               if (is_thumb_ins)
                   pc_offset = 4; // standard value for add as second ins, but not for load, where PC-alignment also needs to be considered ...
               load_addr = AddressSubUint32(AddressSub(ARM_INS_CADDRESS(data_ins),RELOC_ADDENDS(rel)[0]),pc_offset);
               bdest = AddressToBbl(addresses_to_blocks,cfg,load_addr);
               if (bdest)
                 actual_load = T_ARM_INS(BBL_INS_FIRST(bdest));
               while (actual_load &&
                      AddressIsGt(load_addr,ARM_INS_CADDRESS(actual_load)))
                 actual_load = ARM_INS_INEXT(actual_load);
             }
             /* check whether it looks like what we expect */
             if (actual_load &&
                 ((ARM_INS_OPCODE(actual_load) == ARM_LDR) || (ARM_INS_OPCODE(actual_load) == ARM_ADD)) &&
                 /* can't check whether the other register matches the original
                  * loaded register, because it can be different in case of
                  * spilling
                  */
                 ((ARM_INS_REGB(actual_load) == ARM_REG_R15) || (ARM_INS_REGC(actual_load) == ARM_REG_R15)))
             {
               /* this instruction adds the value of the pc to the loaded value -> ok */
             }
             else if (actual_load && (ARM_INS_OPCODE(actual_load) == ARM_MOV) && (ARM_INS_FLAGS(actual_load) & USED_TO_BE_ADD_WITH_PC))
             {
               /* this instruction used to add the value of the pc to the loaded value 
                  as in the previous case, but was already converted for another data_ins -> ok */               
             }
             else
             {
               t_reg loadreg;
               /* second case -> we have to find the "add r4, pc, #y" exhaustively,
                * because there is information in the calculated relocation value
                * that refers back to this instruction. For now, require that this
                * add is in the same bbl as the first ldr
                */
               actual_load = ARM_INS_INEXT(ins);
               loadreg = ARM_INS_REGA(ins);
               /* look for the third instruction above (the second add/ldr) */
               while (actual_load)
               {
                 if (RegsetIn(ARM_INS_REGS_DEF(actual_load),loadreg))
                 {
                   t_regset usedregs;
                   t_reg pcrelreg;
                   t_arm_ins *dataadr;
                   t_address add_calculated_address;
                   t_reloc *newrel;

                   /* must add another register to the loaded reg */
                   usedregs = ARM_INS_REGS_USE(actual_load);
                   ASSERT(RegsetIn(usedregs,loadreg),("GOTPC loadreg not used by found add: @I - @I",ins,actual_load));
                   RegsetSetSubReg(usedregs,loadreg);
                   ASSERT(RegsetCountRegs(usedregs)==1,("Besides register r%d less or more than 1 extra register used in GOTPC add in @I (@X)",loadreg,actual_load,CPREGSET(cfg,usedregs)));
                   /* get the other register */
                   REGSET_FOREACH_REG(usedregs,pcrelreg)
                     break;
                   /* look for the definition of that other register */
                   dataadr = ArmFindConditionalInstructionThatDefinesRegisterInBbl(actual_load,&usedregs,ARM_INS_CONDITION(actual_load));
                   ASSERT(dataadr,("Can't find instruction that defines r%d before @I in the same bbl",pcrelreg,actual_load));
                   ASSERT((ARM_INS_OPCODE(dataadr)==ARM_ADD
                           || ARM_INS_OPCODE(dataadr)==ARM_SUB) && (ARM_INS_REGB(dataadr)==ARM_REG_R15) && (ARM_INS_FLAGS(dataadr) & FL_IMMED),
                       ("Instruction that defines second GOTPC register is not an add reg,pc,#imm: @I",dataadr));
                   /* check that the found add refers to the same data pool entry as the load */
                   if (ARM_INS_OPCODE(dataadr) == ARM_ADD)
                        add_calculated_address = AddressAddUint32(AddressInverseMaskUint32(ARM_INS_CADDRESS(dataadr),3),ARM_INS_IMMEDIATE(dataadr)+(ARM_INS_FLAGS(dataadr)&FL_THUMB?4:8));
                   else
                        add_calculated_address = AddressSubUint32(AddressInverseMaskUint32(ARM_INS_CADDRESS(dataadr),3),ARM_INS_IMMEDIATE(dataadr)-(ARM_INS_FLAGS(dataadr)&FL_THUMB?4:8));
                   ASSERT(AddressIsEq(add_calculated_address,load_address)
                          || (AddressIsEq(AddressAdd(add_calculated_address, RELOC_ADDENDS(rel)[0]), load_address) && strncmp(RELOC_CODE(rel), "U00?s0000A00+:R00A01+P-A00+!", strlen("U00?s0000A00+:R00A01+P-A00+!"))==0),
                          ("Instruction that defined second GOTPC register (@I) does not refer to the same data pool entry (@G) as the load at @I (@G)",dataadr,add_calculated_address,ins,load_address));
                   /* since the addend isn't used in the PIC-construct here, it may contain a value
                    * that we may have to use
                    */
                   newaddend = RELOC_ADDENDS(rel)[0];
		   if (!AddressIsEq(add_calculated_address,load_address) && (AddressIsEq(AddressAdd(add_calculated_address, RELOC_ADDENDS(rel)[0]), load_address) && strncmp(RELOC_CODE(rel), "U00?s0000A00+:R00A01+P-A00+!", strlen("U00?s0000A00+:R00A01+P-A00+!"))==0))
		     {
		       /* addend does not only include offset in to symbol, but also displacement between dataadr and  add_calculated_address */
		       newaddend = AddressAdd(newaddend,AddressSub(add_calculated_address,load_address));
		     }
                   break;
                 }
                 actual_load = ARM_INS_INEXT(actual_load);
               }
               ASSERT(actual_load,("Could not find last instruction of PIC load sequence started by @I loading rel @R",ins,rel));
             }

             if (is_thumb_ins)
               ASSERT(ARM_INS_OPCODE(actual_load) == ARM_ADD || (ARM_INS_OPCODE(actual_load) == ARM_MOV && (ARM_INS_FLAGS(actual_load) & USED_TO_BE_ADD_WITH_PC)),("Need to implement support for REL32 loads in thumb of LDR ... LDR ... sequences for @I",ins));

             /*
               ASSERT(
                    ((ARM_INS_OPCODE(actual_load) == ARM_LDR) || (ARM_INS_OPCODE(actual_load) == ARM_ADD)) &&
                    ((ARM_INS_REGB(actual_load) == ARM_REG_R15 && ARM_INS_REGC(actual_load) == ARM_INS_REGA(ins)) || (ARM_INS_REGC(actual_load) == ARM_REG_R15 && ARM_INS_REGB(actual_load) == ARM_INS_REGA(ins))),
                    ("Second part of REL32 load/add is not a PC-relative LDR/ADD using register of first LDR:\n  First LDR: @I\n  Second LDR: @I, calculated load address: @G",ins,actual_load,AddressSubUint32(AddressSub(ARM_INS_CADDRESS(data_ins),RELOC_ADDENDS(rel)[0]),is_thumb_ins?4:8)));
             */

             if (ARM_INS_OPCODE(actual_load) == ARM_LDR)
               /* turn it in a plain load */
               ArmInsMakeLdr(actual_load,ARM_INS_REGA(actual_load),ARM_INS_REGC(actual_load),ARM_REG_NONE,0,ARM_INS_CONDITION(actual_load),TRUE,TRUE,FALSE);
             else if (ARM_INS_FLAGS(actual_load) & USED_TO_BE_ADD_WITH_PC)
               {
                 /* nothing to do: was already converted to mov!!! */
               }
             else if (ARM_INS_REGA(ins) == ARM_INS_REGA(actual_load) && (ARM_INS_REGA(actual_load)==ARM_INS_REGB(actual_load) || ARM_INS_REGA(actual_load)==ARM_INS_REGC(actual_load)))
               {
                 /* turn it into a nop only if the value was already in the right register
                    the above check for REGB or REGC equaling REGA is necessary because it may occur that 
                    REGA(ins)==REGA(actual_load) but that this is totally irrelevant because the value was 
                    spilled in the meantime and reloaded into another register!
                 */
	         ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(actual_load));
                 ArmInsMakeNoop(actual_load);
               }
             else
               {
               /* turn it into a mov if it's in a different register */
                if (ARM_INS_REGB(actual_load) == ARM_REG_R15)
                  {
                    ArmInsMakeMov(actual_load,ARM_INS_REGA(actual_load),ARM_INS_REGC(actual_load),0,ARM_INS_CONDITION(actual_load));
                    ARM_INS_SET_FLAGS(actual_load,ARM_INS_FLAGS(actual_load) | USED_TO_BE_ADD_WITH_PC);
                  }
                else
                  {
                    ArmInsMakeMov(actual_load,ARM_INS_REGA(actual_load),ARM_INS_REGB(actual_load),0,ARM_INS_CONDITION(actual_load));
                    ARM_INS_SET_FLAGS(actual_load,ARM_INS_FLAGS(actual_load) | USED_TO_BE_ADD_WITH_PC);
                  }
               }

             /* create the new relocation for the data ins */
             if (RELOC_N_TO_RELOCATABLES(rel)>=1)
             {
               nrel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),newaddend,T_RELOCATABLE(data_ins),AddressNullForIns(data_ins),RELOC_TO_RELOCATABLE(rel)[0],RELOC_TO_RELOCATABLE_OFFSET(rel)[0],RELOC_HELL(rel),RELOC_EDGE(rel),NULL,NULL,RelocRel32GetEquivAbs32RelocCode(rel));
               RelocAddAddend(nrel,AddressNullForIns(ins));
             }
             else
             {
               nrel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),newaddend,T_RELOCATABLE(data_ins),AddressNullForIns(data_ins),NULL,AddressNullForIns(ins),RELOC_HELL(rel),RELOC_EDGE(rel),NULL,NULL,RelocRel32GetEquivAbs32RelocCode(rel));
             }
             /* update value for the new situation */
             value = StackExecConst(RELOC_CODE(nrel), nrel, NULL, 0, obj);

             /* since we reuse the edge, make sure it isn't freed */
             RELOC_SET_EDGE(rel,NULL);

             /* free the old relocation */
	     RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj), rel);

             rel = nrel;
             /* now let the generic code turn this load into an address producer */

            /*}}}*/
          }
 

	  /* We only know ABS32 relocations = the absolute value is written at this address */
	  if ((strncmp(RELOC_CODE(rel),"R00A01+A00+\\",12)!=0) 
              && (strncmp(RELOC_CODE(rel),"R00A01+A00+g-\\",14)!=0) 
              && (strncmp(RELOC_CODE(rel),"R00A01+Z00+A00+g-\\",18)!=0) 
              && (strncmp(RELOC_CODE(rel),"R00A01+A00+R00A01+M|\\", 21)!=0) 
              && (strncmp(RELOC_CODE(rel),"R00A01+A00+R00A01+M|g-\\",23)!=0)
              && (strncmp(RELOC_CODE(rel),"R00A01+Z00+A00+R00A01+Z00+M|\\",29)!=0) 
              && (strncmp(RELOC_CODE(rel),"R00A01+Z00+A00+\\",16)!=0)
              && (strncmp(RELOC_CODE(rel),"R00A01+Z00+i00000fff+ifffff000&A00+R00A01+Z00+i00000fff+ifffff000&M|\\",69)!=0)
              && (strncmp(RELOC_CODE(rel),"gA00+\\",6)!=0))
	    FATAL(("Did not expect this relocation for @I %s at @G: @R",ins,RELOC_CODE(rel),load_address,rel));


          ASSERT(RELOC_N_TO_RELOCATABLES(rel)<=1, ("Complex address producer @R\n", rel));

	  /* If it does not already point to a bbl make it point to a bbl when neccessary*/
	  if ((RELOC_N_TO_RELOCATABLES(rel)==1) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])!=RT_BBL))
	  {
	    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_INS) 
	    {
	      FATAL(("Should not happen!"));
	    }
	    else if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])!=RT_SUBSECTION)&&(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])!=RT_SECTION))
	    {
	      FATAL(("Implement @R\n",rel));
	    }
	    else if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(rel)[0])) == CODE_SECTION)
	    {
	      FATAL(("This should be dead code!"));
	    }
	    else
	    {
        RelocMoveToRelocTable(rel, relocs_to_remove);
	      rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), rel);
	      RelocSetFrom(rel, T_RELOCATABLE(ins));
	      RELOC_SET_FROM_OFFSET(rel,  AddressNullForObject(obj));
	      if (RELOC_LABEL(rel)) 
		Free(RELOC_LABEL(rel));
	      RELOC_SET_LABEL(rel, StringDup("ADDR RELATIVE LOAD"));
	      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	      ArmMakeAddressProducer(__FILE__,__LINE__,ins,value , rel);
	    }
	  }
	  else
	  {
      RelocMoveToRelocTable(rel, relocs_to_remove);
            rel = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), rel);
            RelocSetFrom(rel, T_RELOCATABLE(ins));
            RELOC_SET_FROM_OFFSET(rel,  AddressNullForObject(obj));
	    if (RELOC_LABEL(rel)) 
	      Free(RELOC_LABEL(rel));
	    RELOC_SET_LABEL(rel, StringDup("ADDR RELATIVE LOAD (from bbl)"));
	    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	    ArmMakeAddressProducer(__FILE__,__LINE__,ins,value , rel);
	  }
	}
      }
      else if (((ARM_INS_OPCODE(ins)==ARM_LDF)||(ARM_INS_OPCODE(ins)==ARM_FLDS)||(ARM_INS_OPCODE(ins)==ARM_FLDD)||ARM_INS_OPCODE(ins)==ARM_VLDR)&&(ARM_INS_REGB(ins)==ARM_REG_R15))
      {
        t_uint32 pc_offset = 8;
        VERBOSE(1,("transforming VLDR instruction @I to const producer, this has not been tested well\n",ins));

	char * copy;

        if (ARM_INS_FLAGS(ins) & FL_THUMB)
                pc_offset = 4;

        if ((ARM_INS_OPCODE(ins) == ARM_VLDR) && (ARM_INS_REGB(ins) == ARM_REG_R15))
        {
                /* 4-byte alignment of the PC required */
                address = AddressInverseMaskUint32(address,0x3);
        }

	if (!(ARM_INS_FLAGS(ins) & FL_DIRUP))
	{
	  load_address=AddressSubUint32(address,ARM_INS_IMMEDIATE(ins)); /* Calculate the load address + 8 (pc=pc+8) */
	  load_address=AddressAddUint32(load_address,pc_offset); /* Calculate the load address + 8 (pc=pc+8) */
	}
	else
	{
	  load_address=AddressAddUint32(address,ARM_INS_IMMEDIATE(ins)+pc_offset); /* Calculate the load address + 8 (pc=pc+8) */
	}


	bbl2=find_close_bbl_by_address(bbl,load_address);
	if (!bbl2) FATAL(("BBL NOT FOUND!"));
	else 
	{
	  BBL_FOREACH_ARM_INS(bbl2,ins2)
	  {
	    if (AddressIsEq(ARM_INS_CADDRESS(ins2),load_address)) break;
	  }
	  if (!ins2) FATAL(("No ins found!"));


	  if (ARM_INS_TYPE(ins2)!=IT_DATA) FATAL(("Ins is not data!\nLoad: @I\nData: @I\n",ins,ins2));
	  value=ARM_INS_IMMEDIATE(ins2);
	}

	if (ARM_INS_FLAGS(ins)&FL_FLT_PACKED) 
	  /* 96 bits*/
	{
	  FATAL(("Implement packed floats")); 
	  exit(0); /* Keep the compiler happy */
	}
	else if (ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) 
	  /* 96 bits */
	{
	  FATAL(("Implement double extended floats")); 
	  exit(0); /* Keep the compiler happy */
	}
	else if ((ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE) ||
                 (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE))
	  /* 64 bits */
	{
	  int val2;
	  if(ARM_INS_TYPE(ARM_INS_INEXT(ins2)) != IT_DATA) FATAL(("Ins->next is not data!\nData: @I\n",ins2));
	  copy=Malloc(8);/*64);*/
	  memcpy(copy,&value,4);/*64);*/
	  val2=ARM_INS_IMMEDIATE(ARM_INS_INEXT(ins2));
	  memcpy(copy+4,&val2,4);/*64);*/
	  /*if(memcmp(data,copy,8))*/
	}
	else 
	  /* 32 bits */
	{
	  copy=Malloc(4);
	  memcpy(copy,&value,4);
	}

	ArmMakeFloatProducer(ins,copy);
      }

      /*address=AddressAddUint32(address,4);*/
    }
  }

  /* end of 2. }}} */

  /* by now we should have found all adds we couldn't change in part 1 */
  {
    int i;

    for (i=0; i<PtrArrayCount(&add_todos); i++)
    {
      if (RegsetIn(ARM_INS_REGS_USE(T_ARM_INS(PtrArrayGet(&add_todos,i))),ARM_REG_R15))
        FATAL(("Did transform @I into an address producer",T_ARM_INS(PtrArrayGet(&add_todos,i))));
    }
    PtrArrayFini(&add_todos,FALSE);
  }


  /* 3. Add or substract constants to/from the pc. This must be removed since
   * it introduces dependencies between the refered value and this
   * instruction. The only hard thing is finding out what we refer to. The
   * following rules are used:
   *
   * If the calculated address is a data address, we assume it points to
   * somewhere in the corresponding datablock. Otherwise we must find out if
   * it will be used for control-flow or for loads (which is possible, when we
   * know that there will always be a constant offset added to this block).
   *
   * {{{ */

  addresses_to_blocks = HashTableNew(20033,0,
     BblHashAddress,
     AddressCmp,
     BblHeAddrKeyFree);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      int i;
      for (i=0;i<G_T_UINT32(BBL_CSIZE(bbl));i+=1024)
	{
	  t_bbl_he * bblhe = (t_bbl_he*) Malloc(sizeof(t_bbl_he));
	  bblhe->bbl=bbl;
	  HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(bblhe), BblAddressKey(bbl,i/1024));
	  HashTableInsert(addresses_to_blocks,bblhe);
	}
    }

  CFG_FOREACH_BBL(cfg,bbl)
  {
    if (IS_DATABBL(bbl)) continue;

    BBL_FOREACH_ARM_INS(bbl,ins)
    {
      t_address adest;
      t_bbl *bdest;
      t_address pc;

      if (ARM_INS_OPCODE(ins) != ARM_ADD && ARM_INS_OPCODE(ins) != ARM_SUB) continue;
      if (ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP) continue;
      if (!RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R15)) continue;

      /* sanity checks */
      if (!(ARM_INS_FLAGS(ins) & FL_IMMED)) FATAL(("implement @I",ins));
      if (ARM_INS_REGB(ins) != ARM_REG_R15) FATAL(("implement @I",ins));

      pcgreatness = (ARM_INS_FLAGS(ins) & FL_THUMB) ? 4 : 8;
      pc = AddressAnd(ARM_INS_CADDRESS(ins), ~0x3);

      if (ARM_INS_OPCODE(ins) == ARM_ADD)
	adest = AddressAddUint32(pc,ARM_INS_IMMEDIATE(ins)+pcgreatness);
      else
	adest = AddressSubUint32(pc,ARM_INS_IMMEDIATE(ins)-pcgreatness);

      VERBOSE(1,("@I produces @G\n",ins,adest));

      /* find the destination block */

      bdest=AddressToBbl(addresses_to_blocks,cfg,adest);
      if (!bdest) 
      {
	/* {{{ Could not find destination block for this address producer.
	 * There are two possibilities:
	 *   1. the generated address lies somewhere in a data section,
	 *      so we can just make an address producer with a relocation
	 *      to this data section. This is highly unlikely to ever happen,
	 *      because in theory the compiler cannot generate such code.
	 *   2. the generated address lies outside all sections of the 
	 *      program. In this case, our best bet is that it is actually
	 *      some constant address generated in assembly, that isn't
	 *      supposed to change anyway. An example of this is the
	 *      generation of the swapper_pg_dir address in arch/arm/kernel/entry-armv.S
	 *      in the arm linux 2.4 kernel. The best solution here is to 
	 *      make the instruction a constant producer instead of an address
	 *      producer. */
	t_section *sec = ObjectGetSectionContainingAddress(obj,adest);
	if (sec) FATAL(("IMPLEMENT CASE 1."));
	else
	{
	  /* adest lies outside all sections. make a constant producer instead. */
	  ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	  ArmMakeConstantProducer(ins,G_T_UINT32(adest));
	  ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) | FL_ORIG_CONST_PROD);
	} /* }}} */
      }
      else
      {
	t_bool address_is_start_of_block = AddressIsEq(adest,BBL_CADDRESS(bdest));
	if (IS_DATABBL(bdest))
	{
	  /* {{{ data block in the code */
	  rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
	      AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),
	      T_RELOCATABLE(bdest),AddressSub(AddressAnd(adest,AddressNew32(~3)),BBL_CADDRESS(bdest)), 
	      TRUE, NULL, NULL,NULL,
	      "R00A00+\\l*w\\s0000$");
	  RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO DATA"));
	  ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	  ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
	  /* }}} */
	}
	else
	{
	  /* the produced address is a code address */

	  if (ARM_INS_REGA(ins) == ARM_REG_R14)
	  {
	    /* {{{ we're producing a return address */
	    if (!address_is_start_of_block) 
	    {
	      if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(bdest))) & FL_THUMB)
	      {
		/* Look for a pattern:
		 *    add/subNE r14,pc,immediate
		 *    movEQ     r14,pc
		 *
		 * In case the address produced in the first instruction is thumb code, we can no-op the second
		 * instruction because we know that the return address has to be the first one.
		 * 
		 */
		if (ARM_INS_CONDITION(ins) == ARM_CONDITION_NE && ARM_INS_INEXT(ins) && ARM_INS_CONDITION(ARM_INS_INEXT(ins)) == ARM_CONDITION_EQ &&
		    RegsetEquals(ARM_INS_REGS_DEF(ins),ARM_INS_REGS_DEF(ARM_INS_INEXT(ins))))
		{
		  ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
		  VERBOSE(1,("Nooping @I",ARM_INS_INEXT(ins)));
		  ArmInsMakeNoop(ARM_INS_INEXT(ins));
		}
	      }
	      else
		FATAL(("Should not happen!"));
	    }


	    rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
		AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),
		T_RELOCATABLE(bdest),AddressNullForObject(obj),
		FALSE, NULL, NULL,NULL,
		"R00A00+R00M|\\l*w\\s0000$");
	    RELOC_SET_LABEL(rel, StringDup("ADDR RETURN"));
	    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	    ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
	    /* }}} */
	  }
	  else if (ARM_INS_REGA(ins) == ARM_REG_R15)
	  {
	    /* sub pc,pc,#4 is a special case:
	     * it is effectively a noop, but it clears the instruction pipeline.
	     * this is important for supervisor code (e.g. the linux kernel).
	     * this is _not_ an address producer however */
	    if (ARM_INS_OPCODE(ins) == ARM_SUB &&
		ARM_INS_REGA(ins) == ARM_REG_R15 && 
		ARM_INS_REGB(ins) == ARM_REG_R15 &&
		(ARM_INS_FLAGS(ins) & FL_IMMED) &&
		ARM_INS_IMMEDIATE(ins) == 4)
	    {
	      /* do nothing */
	    }
	    else
	      FATAL(("IMPLEMENT ADDRESS PRODUCER STRAIGHT IN PC: @I",ins));
	  }
	  else
	  {
	    /* {{{ we're not producing a return address */
	    t_arm_ins * use;

	    /* try to find the use of the address 
	     * this is actually rather unsafe code */

	    for (use = ARM_INS_INEXT(ins); use; use = ARM_INS_INEXT(use))
	    {
	      if (RegsetIn(ARM_INS_REGS_USE(use),ARM_INS_REGA(ins)))
		break;
	    }

	    if (!use)
	    {
	      /* assume the produced address can only be used for calling
	       * the addressed bbl as a function. this is unsafe, so warn
	       * the user. */
	      /* {{{ */
	      t_cfg_edge *edge = CfgEdgeCreateCall(cfg,CFG_HELL_NODE(cfg),bdest,NULL,NULL);
	      ASSERT(address_is_start_of_block,("@G not start of @iB",adest,bdest));
	      rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
		  AddressNullForObject(obj),(t_relocatable *)ins,AddressNullForObject(obj),
		  (t_relocatable *)bdest,AddressNullForObject(obj),
		  TRUE,edge,NULL,NULL,
		  "R00A00+\\l*w\\s0000$");
	      RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO UNKNOWN CODE"));
	      VERBOSE(2,("DANGEROUS: @I produces code address @G, use not found\n",ins,adest));
	      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
	      ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
	      /* }}} */
	    }
	    else
	    {
	      if (ARM_INS_TYPE(use)==IT_LOAD)
	      {
		/* it appears we're making a data pointer, so we should look for the data block within 
		 * this section: the reloc should point to that block (with the proper addend of course) */
		/* {{{ */

		t_bbl * bbl;
		t_bbl * datablock = NULL;

		/* find the code sub section we're working in right now */
		t_map_node * sub;
		t_section * sub_sec;
		t_address upper, lower;

		/* Dominique: the next line originally used the new calculated address,
		 * changed this to use the old address because that is what is stored 
		 * in the map */
		sub=MapGetSection(OBJECT_MAP(obj),BBL_OLD_ADDRESS(bdest)); 
		sub_sec=SectionGetFromObjectByName(sub->obj,sub->sec_name); 
		lower = SECTION_OLD_ADDRESS(sub_sec);
		upper = AddressAdd(lower,SECTION_OLD_SIZE(sub_sec));

		/* look for the data block (and hope there's only one) */
		CFG_FOREACH_BBL(cfg,bbl)
		{
		  if (!IS_DATABBL(bbl)) continue;
		  if (AddressIsLt(BBL_OLD_ADDRESS(bbl),lower) || AddressIsGe(BBL_OLD_ADDRESS(bbl),upper)) continue;

		  /* is it a padding block for a thumb stub? */
		  if (BBL_NINS(bbl) == 1 &&
		      G_T_UINT32(ARM_INS_CSIZE(T_ARM_INS(BBL_INS_FIRST(bbl)))) == 2 &&
		      ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(bbl))) == IT_DATA &&
		      ARM_INS_IMMEDIATE(T_ARM_INS(BBL_INS_FIRST(bbl))) == 0x0 &&
		      BBL_REFED_BY(bbl) == NULL && 
		      ARM_INS_REFERS_TO(T_ARM_INS(BBL_INS_FIRST(bbl))) == NULL)
		    continue;

		  if (!datablock) 
		    datablock = bbl;
		  else
		  {
		    t_string fbase = FileNameBase(OBJECT_NAME(sub->obj));
		    /* there are two non-trivial data blocks in the code. 
		       we don't know what to do any more. panic. */

		    if ((strcmp(fbase,"f_a_p.l:fdiv.o")==0) ||
			(strcmp(fbase,"fz_a_p.l:fdiv.o")==0) ||
			(strcmp(fbase,"f_t_p.l:fdiv.o")==0) ||
                        (strcmp(fbase,"fz_4s.l:fdiv.o") == 0))
		    {
		      /* special case: hard coded because we know the right answer here: 
			 we need the data with the smallest address */

		      if (AddressIsGt(BBL_OLD_ADDRESS(datablock),BBL_OLD_ADDRESS(bbl)))
			datablock=bbl;

		      VERBOSE(0,("Chose @iB for fdiv\n",datablock));
		    }
		    else
		    {
		      FATAL(("Two data blocks for addr prod @I (load @I)...", ins, use));
		    }
		    Free(fbase);
		  }
		}

		ASSERT(datablock,("Could not find a proper data block in the code sub section"));

		/* now add a relocation and an address producer to the data block */

		rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
		    AddressSub(adest,BBL_CADDRESS(datablock)),
		    T_RELOCATABLE(ins),
		    AddressNullForObject(obj),
		    T_RELOCATABLE(datablock),
		    AddressNullForObject(obj),FALSE, 
		    NULL,NULL,NULL,
		    "R00A00+\\l*w\\s0000$");
		RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO DATA (guessed)"));
		ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
		/* }}} */
	      }
	      else if(ARM_INS_OPCODE(use) == ARM_BX)
	      {
		/* {{{ */
		t_cfg_edge * edge = CfgEdgeCreateCall(cfg,CFG_HELL_NODE(cfg),bdest,NULL,NULL);
		t_reloc * rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),
		    T_RELOCATABLE(ins),AddressNullForObject(obj),
		    T_RELOCATABLE(bdest),AddressNullForObject(obj),
		    TRUE,edge,NULL,NULL,"R00A00+\\l*w\\s0000$");
		RELOC_SET_LABEL(rel, StringDup("Addr from thumb stub to call main"));
		ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest)&0xfffffffe,rel);
		/* }}} */
	      }
	      else if ((ARM_INS_OPCODE(use) == ARM_SUB || ARM_INS_OPCODE(use) == ARM_ADD)
		  && (ARM_INS_FLAGS(use) & FL_IMMED))
	      {
		/* an offset is added to an existing address producer.
		 * this is effectively a new address producer, so we change
		 * the instruction so that it will be handled by the outer
		 * loop (as use is after ins in the bbl, it will be handled
		 * later on in the loop).
		 * the first address producer will be handled as an
		 * "unknown" address producer. 
		 * EXCEPTION: if the newly produced address isn't part of 
		 * any section, we don't really know what happens, and we 
		 * just leave it be, hoping nothing will go wrong. Of course,
		 * we emit a warning for the user. (this is necessary for the
		 * arm linux kernel, where the address of the page tables is
		 * generated this way, even before the MMU is activated and 
		 * virtual addresses exist.) */
		t_address new_dest,offset;
		t_int32 soff;
		t_section *sec;
		t_cfg_edge *edge;
		/* set up the new address producer */
		if (ARM_INS_OPCODE(use) == ARM_ADD)
		  new_dest = AddressAddUint32(adest,ARM_INS_IMMEDIATE(use));
		else
		  new_dest = AddressSubUint32(adest,ARM_INS_IMMEDIATE(use));
		sec = ObjectGetSectionContainingAddress(obj,new_dest);
		if (sec)
		{
		  /* create new address producer */
		  offset = AddressSub(new_dest,AddressAddUint32(ARM_INS_CADDRESS(use),pcgreatness));
		  soff = (t_int32) G_T_UINT32(offset);
		  VERBOSE(0,("AP use generates @G, orig @I",new_dest,use));
		  if (soff < 0)
		  {
		    ARM_INS_SET_OPCODE(use,  ARM_SUB);
		    ARM_INS_SET_REGB(use,  ARM_REG_R15);
		    ARM_INS_SET_IMMEDIATE(use,  -soff);
		  }
		  else
		  {
		    ARM_INS_SET_OPCODE(use,  ARM_ADD);
		    ARM_INS_SET_REGB(use,  ARM_REG_R15);
		    ARM_INS_SET_IMMEDIATE(use,  soff);
		  }
		  ARM_INS_SET_REGS_USE(use,  ArmUsedRegisters(use));
		  ARM_INS_SET_REGS_DEF(use,  ArmDefinedRegisters(use));
		  VERBOSE(0,(" new @I\n",use));
		}
		else
		{
		  /* leave it be, but emit a warning to the user */
		  VERBOSE(0,("ADDRESS PRODUCER @I\nUSE @I, generates address @G out of all known sections... DANGEROUS\n",ins,use,new_dest));
		}

		/* handle the old address producer as an "unknown" address producer */
		edge = CfgEdgeCreateCall(cfg,CFG_HELL_NODE(cfg),bdest,NULL,NULL);
		ASSERT(address_is_start_of_block,("@G not start of @iB",adest,bdest));
		rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
		    AddressNullForObject(obj),(t_relocatable *)ins,AddressNullForObject(obj),
		    (t_relocatable *)bdest,AddressNullForObject(obj),
		    TRUE,edge,NULL,NULL,
		    "R00A00+\\l*w\\s0000$");
		RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO UNKNOWN CODE (intermediate AP)"));
		VERBOSE(2,("DANGEROUS: @I produces code address @G, use not found\n",ins,adest));
		ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
	      }
	      else if (ARM_INS_OPCODE(ins)==ARM_ADD && (ARM_INS_FLAGS(ins) & FL_IMMED) && ARM_INS_IMMEDIATE(ins) == 4 && 
                       ARM_INS_OPCODE(use)==ARM_ORR && !(ARM_INS_FLAGS(use) & FL_THUMB) && (ARM_INS_FLAGS(use) & FL_IMMED) && ARM_INS_IMMEDIATE(use) == 1 && ARM_INS_REGB(use)==ARM_INS_REGA(ins) && ARM_INS_REGA(use)==ARM_INS_REGA(ins) &&
                       ARM_INS_INEXT(use) && ARM_INS_OPCODE(ARM_INS_INEXT(use))==ARM_BX && ARM_INS_REGB(ARM_INS_INEXT(use)) == ARM_INS_REGA(use))
                {
                  /* 
                     205cc:       e28fc004        add     ip, pc, #4
                     205d0:       e38cc001        orr     ip, ip, #1
                     205d4:       e12fff1c        bx      ip
                  */
                  VERBOSE(2,("NEW CASE:\n @I\n @I",ins,use));
                  t_cfg_edge *edge = CfgEdgeCreateCall(cfg,CFG_HELL_NODE(cfg),bdest,NULL,NULL);
                  ASSERT(address_is_start_of_block,("@G not start of @iB",adest,bdest));
                  rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                                        AddressNullForObject(obj),(t_relocatable *)ins,AddressNullForObject(obj),
                                                        (t_relocatable *)bdest,AddressNullForObject(obj),
                                                        TRUE,edge,NULL,NULL,
                                                        "R00A00+\\l*w\\s0000$");
                  RELOC_SET_LABEL(rel, StringDup("addr switch from arm to thumb stub"));
		  ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
                  ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
                }
              else
	      {
		t_cfg_edge *edge = NULL;

                /*
                   1499e:       f20f 000e       addw    r0, pc, #14
                   ...
                   149a2:       eb00 0281       add.w   r2, r0, r1, lsl #2      <-- 'use'
                   ...
                   149ac:       4697            mov     pc, r2                  <-- branch
                 */
                if ((ARM_INS_FLAGS(use) & FL_THUMB)
                        && ARM_INS_SHIFTTYPE(use)==ARM_SHIFT_TYPE_LSL_IMM
                        && ARM_INS_SHIFTLENGTH(use)==2
                        && ARM_INS_REGB(use)==ARM_INS_REGA(ins)

                        && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(use))))==ARM_MOV
                        && ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(use))))==ARM_REG_R15
                        && ArmPropagateRegBackwardStack(ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(use)))), T_ARM_INS(BBL_INS_LAST(bbl)), use)==ARM_INS_REGA(use))
                {
                        /* We have detected that it is the base address of a recognized switch table of branches, so no
                         * need to create additional hell edges */
                        edge = NULL;
                        VERBOSE(1,("Switch table!\n   @I\n   @I\n   @I",ins,use,T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(use)))));
                }
                else
                {
                        /* if we haven't recognized the use of the produced address,
                         * we'll just have to hope it will only be used to call
                         * the block it points to. Just handle it as an "unknown"
                         * address producer */
                        edge = CfgEdgeCreateCall(cfg,CFG_HELL_NODE(cfg),bdest,NULL,NULL);
		        VERBOSE(0,("DANGEROUS: @I produces code address @G, use @I not understood\n",ins,adest,use));
                }

                ASSERT(address_is_start_of_block,("@G not start of @iB",adest,bdest));
                rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                    AddressNullForObject(obj),(t_relocatable *)ins,AddressNullForObject(obj),
                    (t_relocatable *)bdest,AddressNullForObject(obj),
                    TRUE,edge,NULL,NULL,
                    "R00A00+\\l*w\\s0000$");
                RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO UNKNOWN CODE (could not understand use)"));
		VERBOSE(2,("DANGEROUS: @I produces code address @G, use @I not understood\n",ins,adest,use));
		ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(adest),rel);
	      }
	    }

	    /* }}} */
	  }
	}
      }
    }
  }
  /* End of 3. }}} */


  /* 4. Move the pc into a register and add or substract constants to/from that
   * register. This must be removed since
   * it introduces dependencies between the referred value and this
   * instruction. The only hard thing is finding out what we refer to. The
   * following rules are used:
   *
   * If the calculated address is a data address, we assume it points to
   * somewhere in the corresponding datablock else, we must find out if it will
   * be used for control-flow or for loads (which is possible, when we know
   * that there will always be a constant offset added to this block.
   *
   * Looking for code patterns in the form of:
   * MOV reg_x, pc
   * SUB reg_x, immed
   * LD  reg_x, ...
   *
   * {{{ */
  CFG_FOREACH_BBL(cfg,bbl)
  {
    if (!IS_DATABBL(bbl))
    {
      BBL_FOREACH_ARM_INS(bbl,ins)
      {
	if (ARM_INS_REGC(ins) == ARM_REG_R15 &&
	    ARM_INS_OPCODE(ins) == ARM_MOV &&
	    ARM_INS_REGA(ins) != ARM_REG_R14 &&
	    ARM_INS_REGA(ins) != ARM_REG_R15)
	{
	  t_bool found=FALSE;
	  t_arm_ins * i_ins=ins;
	  t_arm_ins * i_i_ins;
	  t_address addr;
	  /* Try to find the operation with the register */
	  address = ARM_INS_CADDRESS(ins);
	  while ((i_ins=ARM_INS_INEXT(i_ins)))
	  {
	    if (RegsetIn(ARM_INS_REGS_USE(i_ins), ARM_INS_REGA(ins)))
	    {
	      found = TRUE;
	      break;
	    }
	  }
	  if (!found) FATAL(("Could not find use of register in this bbl, maybe it is used in the next bbl..."));
	  VERBOSE(1,("INS = @I\nI_INS = @I",ins,i_ins));
	  i_i_ins = ARM_INS_INEXT(i_ins);
	  while (i_i_ins &&
                 !RegsetIn(ARM_INS_REGS_USE(i_i_ins), ARM_INS_REGA(ins)))
	  {
	    if ((i_i_ins=ARM_INS_INEXT(i_i_ins))) continue;
	    found = FALSE;
	    break;
	  }
          if (found && !i_i_ins)
          {
            /* should already be handled in HandleCodeCopyLoop() */
            if (ARM_INS_OPCODE(i_ins) != ARM_CMP)
              FATAL(("Could not find use and not a compare"));
          }
	  else if (!found) FATAL(("Could not find use of register in this bbl, maybe it is used in another bbl"));
	  else
	  {
	    VERBOSE(1,("I_I_INS = @I",i_i_ins));
	    /* Try to find the use of the register */
	    if ((ARM_INS_OPCODE(i_ins)==ARM_ADD || ARM_INS_OPCODE(i_ins)==ARM_SUB) &&
	         ARM_INS_FLAGS(i_ins) & FL_IMMED)
	    {
	      pcgreatness = (ARM_INS_FLAGS(i_ins) & FL_THUMB) ? 4 : 8;
	      if (ARM_INS_OPCODE(i_ins)==ARM_ADD)
		addr=AddressAddInt32(address,ARM_INS_IMMEDIATE(i_ins)+pcgreatness);
	      else
		addr=AddressSubInt32(address,ARM_INS_IMMEDIATE(i_ins)-pcgreatness);
	      VERBOSE(1,("At @G:@I produced @G",address,i_ins,addr));

	      CFG_FOREACH_BBL(cfg,bbl2)
	      {
		t_address base;
		base = BBL_CADDRESS(bbl2);

		/* Search for the basic block that contains this address */

		if ((AddressIsLe(base,addr))&&(AddressIsGt(AddressAdd(base,BBL_CSIZE(bbl2)),addr)))
		{
		  /* Found one: is it data or code ? */
		  if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl2)))==ARM_DATA && ARM_INS_OPCODE(i_i_ins)==ARM_LDM)
		  {
		    addr = AddressNew32(G_T_UINT32(addr) & ~3);
		    rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(bbl2),AddressSub(addr,base), TRUE, NULL, NULL, NULL, "R00A00+\\l*w\\s0000$");
		    RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO DATA"));
		    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		    ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(addr),rel);
		    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(i_ins));
		    ArmInsMakeNoop(i_ins);
		  }
		  else if (ARM_INS_OPCODE(i_i_ins)==ARM_LDR)
		  {
		    if (ARM_INS_FLAGS(i_i_ins) & FL_IMMED)
		    {
		      addr = AddressNew32(G_T_UINT32(addr) & ~3);
		      rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(bbl2),AddressSub(addr,base), TRUE, NULL, NULL, NULL, "R00A00+\\l*w\\s0000$");
		      RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO DATA"));
		      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		      ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(addr),rel);
		      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(i_ins));
		      ArmInsMakeNoop(i_ins);
		    }
		    else /* maybe there is only one datablock, so that is legal too */
		    {
		      t_bbl * bbl;
		      t_bbl * datablock = NULL;

		      /* find the code sub section we're working in right now */
		      t_map_node * sub;
		      t_section * sub_sec;
		      t_address upper, lower;

		      sub=MapGetSection(OBJECT_MAP(obj),addr);
		      sub_sec=SectionGetFromObjectByName(sub->obj,sub->sec_name);
		      lower = SECTION_OLD_ADDRESS(sub_sec);
		      upper = AddressAdd(lower,SECTION_OLD_SIZE(sub_sec));

		      CFG_FOREACH_BBL(cfg,bbl)
		      {
		        if (!IS_DATABBL(bbl)) continue;
			if (AddressIsLt(BBL_OLD_ADDRESS(bbl),lower) || AddressIsGe(BBL_OLD_ADDRESS(bbl),upper)) continue;

			/* is it a padding block for a thumb stub? */
			if (BBL_NINS(bbl) == 1 &&
			    G_T_UINT32(ARM_INS_CSIZE(T_ARM_INS(BBL_INS_FIRST(bbl)))) == 2 &&
			    ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(bbl))) == IT_DATA &&
			    ARM_INS_IMMEDIATE(T_ARM_INS(BBL_INS_FIRST(bbl))) == 0x0 &&
			    BBL_REFED_BY(bbl) == NULL &&
			    ARM_INS_REFERS_TO(T_ARM_INS(BBL_INS_FIRST(bbl))) == NULL)
			  continue;
			if (!datablock)
			  datablock = bbl;
			else
			  FATAL(("Two data blocks for addr producer!"));
		      }
		      ASSERT(datablock,("Could not find a proper data block in the code sub section"));

		      /* now add a relocation and an address producer to the data block */
		      addr = AddressNew32(G_T_UINT32(addr) & ~3);
		      rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),AddressSub(addr,BBL_CADDRESS(datablock)),T_RELOCATABLE(ins),AddressNullForObject(obj),T_RELOCATABLE(datablock),AddressNullForObject(obj),FALSE,NULL,NULL,NULL, "R00A00+\\l*w\\s0000$");
		      RELOC_SET_LABEL(rel, StringDup("ADDR ADD TO DATA"));
		      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		      ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(addr),rel);
		      ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(i_ins));
		      ArmInsMakeNoop(i_ins);
		    }
		  }
		  else if ((ARM_INS_OPCODE(i_i_ins)==ARM_MOV) &&
		           (ARM_INS_REGA(i_i_ins)==ARM_REG_R14))
		  {
		    /* equivalent of "add/sub lr,pc,#const */
		    t_bbl *bdest;

		    /* clear lowest bits in case of Thumb address */
		    bdest=AddressToBbl(addresses_to_blocks,cfg,AddressNew32(G_T_UINT32(addr) & ~3));
		    ASSERT(bdest,("Add support for data reference in mov/addsub/mov code @I -- @I",ins,i_i_ins));
		    ASSERT(!IS_DATABBL(bdest),("Putting address of data in return register @I -- @I",ins,i_i_ins));
		    ASSERT(AddressIsEq(AddressNew32(G_T_UINT32(addr) & ~1),BBL_CADDRESS(bdest)),("Putting code address that's not the start of a bbl in lr @I -- @I",ins,i_i_ins));
		    rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
			AddressNullForObject(obj),T_RELOCATABLE(ins),AddressNullForObject(obj),
			T_RELOCATABLE(bdest),AddressNullForObject(obj),
			FALSE, NULL, NULL,NULL,
			"R00A00+R00M|\\l*w\\s0000$");
		    RELOC_SET_LABEL(rel, StringDup("ADDR RETURN2"));
		    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(ins));
		    ArmMakeAddressProducer(__FILE__,__LINE__,ins,G_T_UINT32(addr),rel);
		    /* the add is folded into the address producer */
		    ComplexityRecordTransformedInstruction(T_INS(ins), T_INS(i_ins));
		    ArmInsMakeNoop(i_ins);
                  }
		  else FATAL(("Serious troubles!!"));
		}
	      }
	    }
	    else FATAL(("Another possibility not yet discovered until now!"));
	  }
	}
      }
    }
  }
  /* End of 4. }}} */


#if 1
  CFG_FOREACH_BBL(cfg,bbl)
    {
      address = BBL_CADDRESS(bbl);
      BBL_FOREACH_ARM_INS(bbl,ins)
      {
	if (RegsetIn(ARM_INS_REGS_USE(ins) , 15))
	{
	  if ((ARM_INS_OPCODE(ins)==ARM_MOV) || (ARM_INS_OPCODE(ins)==ARM_STM))
	  {
	    /* Returns.... No need to handle this */
	  }
	  /* Just a test for the switches */
	  else if ((ARM_INS_OPCODE(ins)==ARM_ADD)&&(ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSL_IMM)&&(ARM_INS_CONDITION(ins)==ARM_CONDITION_LS))
	  {
	  }
	  else if ((ARM_INS_OPCODE(ins)==ARM_LDR)&&(ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_LSL_IMM)&&(ARM_INS_CONDITION(ins)==ARM_CONDITION_LS))
	  {
	  }
	  else if (ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP)
	  {
	    /* Just a switch.... ignore it */ 
	  }
	  else if ((ARM_INS_OPCODE(ins) == ARM_BX) && (ARM_INS_FLAGS(ins) & FL_THUMB))
	  {
	    /* Do nothing, already taken care of */
	  }
	  else if ((ARM_INS_OPCODE(ins) == ARM_B) && (ARM_INS_IMMEDIATE(ins) == 0x7fc))
	  {
	    /* Do nothing, this used to be a 'BX pc' but is changed to 'B 0x2' */
	  }
	  else if (ARM_INS_OPCODE(ins) == ARM_SUB && 
	      ARM_INS_REGA(ins) == ARM_REG_R15 &&
	      ARM_INS_REGB(ins) == ARM_REG_R15 &&
	      (ARM_INS_FLAGS(ins) & FL_IMMED) &&
	      ARM_INS_IMMEDIATE(ins) == 4)
	  {
	    /* sub pc,pc,#4 just flushes the instruction pipeline.
	     * this is sometimes necessary in supervisor mode. 
	     * it is not an address producer however. */
	  }
	  else if ((ARM_INS_OPCODE(ins)==ARM_TEQ) && (ARM_INS_REGA(ins)  ==ARM_REG_NONE) &&  (ARM_INS_REGB(ins)  ==ARM_REG_R15) && (ARM_INS_REGC(ins)  ==ARM_REG_R15))
	  {
	    /* TEQ PC, PC checks if we are in 26 bit or 32 bit mode. Not an
	     * address producer */
	  }
	  /* Drop out if we do not know the type */
	  else
	    FATAL(("Used pc, not handled: @I",ins));
	}
      }
  }
#endif

  /* temporary code to check for edges from datablocks */
  {
    t_cfg_edge * edge;
    VERBOSE(0, ("Checking for stray edges..."));
    CFG_FOREACH_EDGE(cfg,edge) 
    {
      t_bbl * from, * to;
      from = CFG_EDGE_HEAD(edge);
      to = CFG_EDGE_TAIL(edge); 

      if (BBL_INS_FIRST(from))
	if (ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(from))) == IT_DATA) 
	{
	  VERBOSE(1,("edge from data block! (@G)",BBL_CADDRESS(from)));
	}
      if (BBL_INS_FIRST(to))
	if (ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(to))) == IT_DATA)
	{
	  VERBOSE(1,("edge to data block! (@G)",BBL_CADDRESS(to)));
	  if (BBL_PRED_FIRST(from))
	    VERBOSE(1,("edge from possibly reachable code to data block! (@G)\n@eiB",BBL_CADDRESS(to),from));		  
	    if (CFG_EDGE_CAT(edge)==ET_JUMP) 
	    {
	    	VERBOSE(1,("jump edge to data block converted into jump to own block! (@G)",BBL_CADDRESS(to)));
	    	CfgEdgeChangeTail(edge, from);
	    }
	}
    }

    VERBOSE(0, ("Checking for odd blocks..."));
    CFG_FOREACH_BBL(cfg,bbl)
    {
      int data = (BBL_INS_FIRST(bbl) ? (ARM_INS_TYPE(T_ARM_INS(BBL_INS_FIRST(bbl))) == IT_DATA) : 0);
      BBL_FOREACH_ARM_INS(bbl,ins) 
      {
	if (data != (ARM_INS_TYPE(ins) == IT_DATA)) WARNING(("Odd block detected (@G)",BBL_CADDRESS(bbl)));
      }
    }
  }

  /* maybe the following code should move to a separate pass */
  /* anyway, we try to make sure that useless MOVTs are removerd */
  /* and that constants are recogized as such and will be propagated through the code during const prop */

if (!diabloarm_options.nomovwmovtprod)
{
  CFG_FOREACH_BBL(cfg,bbl)
    {
      t_arm_ins * movw;
      BBL_FOREACH_ARM_INS(bbl,movw)
        if (ARM_INS_OPCODE(movw)==ARM_MOVW && !ARM_INS_REFERS_TO(movw))
          {
            t_reg reg = ARM_INS_REGA(movw);
            t_arm_ins *movt = NULL;
            t_arm_ins *next = ARM_INS_INEXT(movw);
            while (next) 
              {
                if (ARM_INS_OPCODE(next)==ARM_MOVT && ARM_INS_REGA(next)==reg && ARM_INS_CONDITION(next)==ARM_INS_CONDITION(movw))
                  {
                    movt = next;
                    break;
                  }
                if (RegsetIn(ARM_INS_REGS_DEF(next),reg)) break;
                if (RegsetIn(ARM_INS_REGS_USE(next),reg)) break;
                next=ARM_INS_INEXT(next);              
              }
            
            if (!movt) 
              {
                //DEBUG(("TRANSFORMED @I without movt",movw));
		ComplexityRecordTransformedInstruction(T_INS(movw), T_INS(movw));
                ArmMakeConstantProducer(movw,ARM_INS_IMMEDIATE(movw));
                //DEBUG(("       INTO @I",movw));
              }
            else if (movt && ((!ARM_INS_REFERS_TO(movt)) || ARM_INS_IMMEDIATE(movw)==0) && ARM_INS_IMMEDIATE(movt)==0)
              {
                //DEBUG(("TRANSFORMED @I",movw));
                //DEBUG(("            @I",movt));
		ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                ArmMakeConstantProducer(movt,ARM_INS_IMMEDIATE(movw));
		ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                ArmInsMakeNoop(movw);
                //DEBUG(("       INTO @I",movw));
                //DEBUG(("            @I",movt));
              }
          }
    }
  CFG_FOREACH_BBL(cfg,bbl)
    {
      t_arm_ins * movw;
      BBL_FOREACH_ARM_INS(bbl,movw)
        if (ARM_INS_OPCODE(movw)==ARM_MOVW && ARM_INS_REFERS_TO(movw))
          {
            t_reg reg = ARM_INS_REGA(movw);
            t_arm_ins *movt = NULL;
            t_arm_ins *next = ARM_INS_INEXT(movw);

            //	    if (counter >= diablosupport_options.debugcounter)
            //              continue;
            
            while (next) 
              {
                if (ARM_INS_OPCODE(next)==ARM_MOVT && ARM_INS_REGA(next)==reg && ARM_INS_CONDITION(next)==ARM_INS_CONDITION(movw))
                  {
                    movt = next;
                    break;
                  }
                if (RegsetIn(ARM_INS_REGS_DEF(next),reg)) break;
                if (RegsetIn(ARM_INS_REGS_USE(next),reg)) break;
                next=ARM_INS_INEXT(next);              
              }            
            if (movt && ARM_INS_REFERS_TO(movt))
              {

                //                DEBUG(("ADDR PROD @I",movw));
                //                DEBUG(("          @I",movt));
                
                /* TODO: add check that they point to the same symbol with same addend etc ... */
                /* TODO: convert this sequence into an address producer */
                
                t_reloc * reloct = RELOC_REF_RELOC(ARM_INS_REFERS_TO(movt));
                t_reloc * relocw = RELOC_REF_RELOC(ARM_INS_REFERS_TO(movw));

                if (strcmp(RELOC_CODE(reloct),"R00A01+A00+R00A01+M|sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0)
                  {
                    t_uint32 value = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    WARNING(("type 1: ins @I and @I should be combined into an address producer for %x", movw, movt,value));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+Z00+A00+R00A01+Z00+M|sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0)
                  {
                    t_uint32 value = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    WARNING(("type 2: ins @I and @I should be combined into an address producer for %x", movw, movt,value));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+A00+s0010>\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0 
                         && strcmp(RELOC_CODE(relocw),"R00A01+A00+R00A01+M|sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0 
                         )
                  {
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    counter++;
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+A00+R00A01+M|\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+Z00+A00+s0010>\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0 
                         && strcmp(RELOC_CODE(relocw),"R00A01+Z00+A00+R00A01+Z00+M|sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0
                         )
                  {
                    counter++;
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+Z00+A00+R00A01+Z00+M|\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+A00+s0010>\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0 
                         && strcmp(RELOC_CODE(relocw),"R00A01+A00+R00A01+M|sffff&\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0
                         )
                  {
                    counter++;
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+A00+R00A01+M|\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                    }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+A00+s0010>\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0
                         && strcmp(RELOC_CODE(relocw),"R00A01+A00+sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0
                         )
                  {
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    counter++;
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+A00+\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                 }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+A00+s0010>\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0
                         && strcmp(RELOC_CODE(relocw),"R00A01+A00+sffff&\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0
                         )
                  {
                    counter++;
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+A00+\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+Z00+A00+s0010>\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0 
                         && strcmp(RELOC_CODE(relocw),"R00A01+Z00+A00+sffff&\\=s0fff& % sf000&s0004<| l ifff0f000& | w \\ s0000$")==0
                         )
                  {
                    counter++;
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+Z00+A00+\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                  }
                else if (strcmp(RELOC_CODE(reloct),"R00A01+Z00+A00+s0010>\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0
                         && strcmp(RELOC_CODE(relocw),"R00A01+Z00+A00+sffff&\\===s00ff&s0010< % s0700&s0014<| % s0800&s0001>| % sf000&s000c>| l i8f00fbf0&| w \\ s0000$")==0
                         )
                  {
                    counter++;
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I",movt));
                    Free(RELOC_CODE(reloct));
                    RELOC_SET_CODE(reloct, StringDup("R00A01+Z00+A00+\\l*w\\s0000$"));
                    RelocSetFrom(reloct,T_RELOCATABLE(movt));
                    RELOC_SET_FROM_OFFSET(reloct,AddressNullForObject(obj));
                    t_uint32 value = StackExecConst (RELOC_CODE(reloct), reloct, NULL, 0, obj);
                    t_uint32 value2 = (ARM_INS_IMMEDIATE(movw) & 0xffff) | (ARM_INS_IMMEDIATE(movt) << 16);
                    if(value != value2)
                      WARNING(("While making a MOVT/MOVW address producer the value calculated through StackExec and the value computed \
from the immediates encoded in the instructions did not match! This suggests the size of some subsection was changed after linker emulation. \
When adding new initialization or finalization routines this is to be expected.\nInstruction: @I\nValue calculated through StackExec: %x\n\
Value calculated from encoded immediates: %x\n", movt, value, value2));
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movw));
                    ArmInsMakeNoop(movw);
		    ComplexityRecordTransformedInstruction(T_INS(movt), T_INS(movt));
                    ArmMakeAddressProducer(__FILE__,__LINE__,movt, value, reloct);
                    //DEBUG(("ADDR PROD @I",movw));
                    //DEBUG(("          @I\n",movt));
                  }
                else
                  WARNING(("STRANGE COMBINATION OF RELOC @I and @I\n@R\n@R", movw, movt,RELOC_REF_RELOC(ARM_INS_REFERS_TO(movw)),RELOC_REF_RELOC(ARM_INS_REFERS_TO(movt))));
              }
          }
    }
}


  HashTableFree(addresses_to_blocks);
  RelocTableFree(relocs_to_remove);
}
/* }}} */
/*!
 * \brief 
 *
 * \param obj
 * \param code
 *
 * \return t_uint32 
*/
/* ArmPatchCallsToWeakSymbols {{{ */
static t_uint32 ArmPatchCallsToWeakSymbols(t_object * obj)
{
  t_arm_ins * ins;
  t_uint32 pc, ncalls=0;
  t_section *code;
  t_uint32 i;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    ins = SECTION_DATA(code); 

    for (pc = 0; ins != NULL; ins = ARM_INS_INEXT(ins), pc += 4)
    {
      if (ARM_INS_TYPE(ins) == IT_BRANCH)
        if (ARM_INS_OPCODE(ins) == ARM_BL)
        {
          if ((ARM_INS_IMMEDIATE(ins) == -4) && !(ARM_INS_FLAGS(ins) & FL_THUMB))
          {
            ncalls++;
            ArmInsMakeNoop(ins);	
          }
          /* The same thing happens in thumb but the immediate is zero instead of 4*/
          if(ARM_INS_IMMEDIATE(ins) == 0 && (ARM_INS_FLAGS(ins) & FL_THUMB))
          {
            ncalls++;
            VERBOSE(1,("Nooping @I",ins));
            ArmInsMakeNoop(ins);	
          }
        }
    }
  }
  return ncalls;
}
/* }}} */


/* {{{ helper function callback for FunctionDuplicate */
void ArmDupSwitchTables (t_function *orig, t_function *new)
{
  t_bbl *bbl, *data, *dup;
  t_object *obj = CFG_OBJECT(FUNCTION_CFG(orig));

  if (strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name, "ARM"))
    return;

  FUNCTION_FOREACH_BBL (new, bbl)
  {
    t_cfg_edge *edge;
    t_ins *ins = BBL_INS_LAST (bbl);
    t_arm_ins *entry;
    t_reloc *rel;
    int i, maxval = -1;

    BBL_FOREACH_SUCC_EDGE (bbl, edge)
      if (CFG_EDGE_CAT(edge) == ET_SWITCH || CFG_EDGE_CAT(edge) == ET_IPSWITCH)
	if (maxval < (int) CFG_EDGE_SWITCHVALUE (edge))
	  maxval = (int) CFG_EDGE_SWITCHVALUE (edge);
    if (maxval == -1) continue;

    if (!ins)
      FATAL (("switch without instructions?"));
    if (!INS_REFERS_TO (ins)) continue;

    rel = RELOC_REF_RELOC(INS_REFERS_TO(ins));
    data = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]);
    ASSERT (IS_DATABBL (data), ("switch table not data?"));
    
    /* do not duplicate the switch table, easier to create a new one instead */
    dup = BblDup (data);
    RelocSetToRelocatable (rel, 0, T_RELOCATABLE(dup));
    i = 0;
    BBL_FOREACH_ARM_INS (dup, entry)
    {
      if (i > maxval)
	break;
      if (ARM_INS_REFERS_TO (entry))
      {
	t_reloc *rel2 = RELOC_REF_RELOC (ARM_INS_REFERS_TO (entry));
	BBL_FOREACH_SUCC_EDGE (bbl, edge)
	  if (CfgEdgeTestCategoryOr (edge, ET_SWITCH|ET_IPSWITCH) &&
	      CFG_EDGE_SWITCHVALUE (edge) == i)
	    break;
	ASSERT (edge, ("Cannot find edge corresponding to jump table entry"));
	ASSERT (RELOC_N_TO_RELOCATABLES (rel2) == 1,
	    ("unexpected reloc for jump table"));
	RelocSetToRelocatable (rel2, 0, T_RELOCATABLE (CFG_EDGE_TAIL (edge)));
      }
      ++i;
    }
    while (entry)
    {
      /* only duplicate the jump table, not any cruft that follows it */
      ins = T_INS (entry);
      entry = ARM_INS_INEXT (entry);
      InsKill (ins);
    }
  }
}

/* Add a call (branch and link) between these BBL's by splitting the from BBL. Is only used by CfgAddSelfProfiling */
void ArmAddCallFromBblToBbl (t_object* obj, t_bbl* from, t_bbl* to)
{
  t_arm_ins* ins;
  t_bool isThumb = ArmBblIsThumb(from);
  t_bbl* split = BblSplitBlock (from, T_INS(BBL_INS_FIRST(from)), TRUE);
  CfgEdgeKill (BBL_SUCC_FIRST(from));/* A fallthrough edge to split has been created by BblSplitBlock, remove it */
  CfgEdgeCreateCall (OBJECT_CFG(obj), from, to, split, NULL);/* Create call edge with corresponding return edge to split */
  ArmMakeInsForBbl(Push, Append, ins, from, isThumb, (1 << ARM_REG_R14), ARM_CONDITION_AL, isThumb);/* Push and pop LR */
  ArmMakeInsForBbl(CondBranchAndLink, Append, ins, from, isThumb, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Pop, Prepend, ins, split, isThumb, (1 << ARM_REG_R14), ARM_CONDITION_AL, isThumb);/* Prepend to following BBL */
}

/* Add instructions for instrumentation at the beginning of the BBL. These instructions
 * increment a certain memory location every time the BBL is executed. Used by CfgAddSelfProfiling
 */
#define COUNTER_SATURATION 1
#define PROFILE_USE_LIVENESS 0
void ArmAddInstrumentationToBbl (t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_address offset)
{
  t_arm_ins* tmp, *ins;
  t_arm_ins* first = T_ARM_INS(BBL_INS_FIRST(bbl));
  t_reloc* rel;
  t_cfg_edge* edge;
  t_reg reg , helper;
#if COUNTER_SATURATION
  t_reg reg_saturation;
#endif
  t_regset available;
  t_uint32 nr_of_dead_regs;
  t_bool isThumb = ArmBblIsThumb(bbl);
  t_uint32 registers = 0;

  /* Initialize regsets containing all possible ARM and thumb registers */
  static t_regset possible;
  static t_regset thumb_possible;
  static t_bool initialized = FALSE;

  if(!initialized)
  {
    RegsetSetEmpty(possible);
    RegsetSetEmpty(thumb_possible);
#if PROFILE_USE_LIVENESS
    for (reg = ARM_REG_R0; reg <= ARM_REG_R12; reg++)
    {
      RegsetSetAddReg(possible, reg);
      if(IS_THUMB_REG(reg))
        RegsetSetAddReg(thumb_possible, reg);
    }
#endif
    initialized = TRUE;
  }

  offset = AddressAddUint32(offset, sizeof(t_uint64));

  /* In thumb code we might encounter from_thumb stubs. These start with a BX PC instruction that must be aligned
   * on four bytes. To avoid problems with alignment we won't profile these BBLs.
   */
  if(isThumb && (ARM_INS_OPCODE(first) == ARM_BX) && (ARM_INS_REGB(first) == ARM_REG_R15))
    return;

  /* In thumb code a nop instruction might be added between a jumptable and the instruction
   * implementing the switch. The nop instruction serves as padding and is alone in its BBL,
   * without predecessor and falling through to the jumptable
   */
  if((isThumb && (BBL_NINS(bbl) == 1) && (ARM_INS_OPCODE(first) == ARM_T2NOP)
    && BBL_SUCC_FIRST(bbl) && IS_DATABBL(T_BBL(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl))))) && (BBL_PRED_FIRST(bbl) == NULL))
  {
    CfgEdgeKill(BBL_SUCC_FIRST(bbl));
    return;
  }

  /* This BBL might be the default case for a switch. This is implemented as a branch after
   * a conditional branch. The jumptable for the switch is placed right behind this BBL. Therefore we can't
   * instrument it as this would mean an increase in size of the BBL and a larger offset to the jumptable.
   * We'll make a new BBL to contain the default branch and instrumentation, and have the old one jump to it.
   */
  BBL_FOREACH_PRED_EDGE(bbl, edge)
    if (CfgEdgeIsFallThrough(edge))
      break;
  if(edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(edge));

    /* If any of the succeeding edges of head is a switch edge */
    BBL_FOREACH_SUCC_EDGE(head, edge)
      if (CfgEdgeTestCategoryOr(edge, ET_SWITCH | ET_IPSWITCH))
        break;
    if(edge)
    {
      t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);
      CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
      ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, isThumb);

      bbl = split;
    }
  }

  /* The BBL might be one of the cases of a switch where each case consists of one instruction and falls
   * through into the next case. This kind of switch is implemented using a simple add instruction. Perhaps
   * switches where each case consists of 2 instructions (or each consists of 3, or...) might also be implemented
   * in this way. I haven't encountered this though and it isn't supported.
   */
  BBL_FOREACH_PRED_EDGE(bbl, edge)
    if (CfgEdgeTestCategoryOr(edge, ET_SWITCH | ET_IPSWITCH))
      break;
  if(edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(edge));
    t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(head));
    if(ARM_INS_OPCODE(last) == ARM_ADD)
    {
      /* The original case is replaced by a BBL containing only a jump to the code in the original BBL.
       * The original BBL will be instrumented. If it only has a fallthrough edge, this edge is modified
       * to a jump back to the fallthrough case.
       */
      if(CfgEdgeIsFallThrough(BBL_SUCC_FIRST(bbl)) && CfgEdgeIsFallThrough(BBL_SUCC_LAST(bbl)))
      {
        t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, isThumb);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(split), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Append, tmp, split, isThumb);
        bbl = split;
      }
      else
      {
        /* In case the cases of the add-switch contain only a jump */
        BBL_FOREACH_SUCC_EDGE(bbl, edge)
          if (CfgEdgeTestCategoryOr(edge, ET_JUMP | ET_IPJUMP))
            break;
        if(edge)
        {
          t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);

          CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
          ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, isThumb);

          bbl = split;
        }   
      }
    }
  }

  /* The regset 'available' now contains all dead registers. If it contains at least two registers
   * we're settled, else at least one register will need to be pushed/popped.
   */
  available = isThumb?RegsetDiff(thumb_possible, BblRegsLiveBefore(bbl)):RegsetDiff(possible, BblRegsLiveBefore(bbl));
  nr_of_dead_regs = RegsetCountRegs(available);

  /* The instrumentation code uses two registers. We can either use two dead registers (if available),
   * use one dead register and a live register that needs to be pushed/popped, or two live registers
   * that need to be pushed/popped.
   */
  if(nr_of_dead_regs == 0)
  {
    /* arbitrarily choose the required registers */
    reg = ARM_REG_R0;
    registers |= (1 << reg);

    helper = ARM_REG_R1;
    registers |= (1 << helper);

#if COUNTER_SATURATION
    reg_saturation = ARM_REG_R2;
    registers |= (1 << reg_saturation);
#endif
  }
  else if(nr_of_dead_regs == 1)
  {
    /* 1 free register, need to find some more */
    REGSET_FOREACH_REG(available, reg)
      break;

    helper = ARM_REG_R0;
    while (helper == reg)
      helper++;

    registers |= (1 << helper);

#if COUNTER_SATURATION
    reg_saturation = ARM_REG_R0;
    while (reg_saturation == reg
           || reg_saturation == helper)
      reg_saturation++;

    registers |= (1 << reg_saturation);
#endif
  }
#if COUNTER_SATURATION
  else if(nr_of_dead_regs == 2)
  {
    /* need to find 1 more register */
    REGSET_FOREACH_REG(available, reg)
      break;
    REGSET_FOREACH_REG(available, helper)
      if (reg != helper) break;

    reg_saturation = ARM_REG_R0;
    while (reg_saturation == reg
           || reg_saturation == helper)
      reg_saturation++;

    registers |= (1 << reg_saturation);
  }
#endif
  else
  {
    REGSET_FOREACH_REG(available, reg)
      break;
    REGSET_FOREACH_REG(available, helper)
      if(reg != helper) break;
#if COUNTER_SATURATION
    REGSET_FOREACH_REG(available, reg_saturation)
      if (reg_saturation != reg
          && reg_saturation != helper) break;
#endif
  }

  /* the following code prepends instructions to the given BBL, adding the profiling support code */

  /* restore used live registers, if any:
   *  POP {...} */
  if (registers != 0)
    ArmMakeInsForBbl(Pop, Prepend, tmp, bbl, isThumb, registers, ARM_CONDITION_AL, isThumb);

  /* store the new counter value:
   *  STR reg, [helper, #0] */
  ArmMakeInsForBbl(Str, Prepend, tmp, bbl, isThumb, reg, helper, ARM_REG_NONE,
    0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, FALSE /* up */, FALSE /* wb */);

  /* if desired, make sure the counter doesn't overflow */
#if COUNTER_SATURATION
  /* subtract one from the counter value, resetting it back to its maximum:
   *  SUB reg, reg, reg_saturation */
  ArmMakeInsForBbl(Sub, Prepend, tmp, bbl, isThumb, reg, reg, reg_saturation, 0, ARM_CONDITION_AL);

  /* shift the number of leading zeros right by 5 bits.
   * The logic here is that if 'reg' is 0, 'reg_saturation' contains '32' (0b100000).
   *  MOV reg_saturation, reg_saturation LSR 5 */
  ArmMakeInsForBbl(Mov, Prepend, tmp, bbl, isThumb, reg_saturation, reg_saturation, 0, ARM_CONDITION_AL);
  ARM_INS_SET_SHIFTTYPE(tmp, ARM_SHIFT_TYPE_LSR_IMM);
  ARM_INS_SET_SHIFTLENGTH(tmp, 5);

  /* count the number of leading zeros in 'reg', and store the result in 'reg_saturation'.
   * We can't use comparison instructions here because the statusflags may not be modified:
   *  CLZ reg_saturation, reg */
  ArmMakeInsForBbl(Clz, Prepend, tmp, bbl, isThumb, reg_saturation, reg, ARM_CONDITION_AL);
#endif

  /* increment the counter value:
   *  ADD reg, reg, #1 */
  ArmMakeInsForBbl(Add, Prepend, tmp, bbl, isThumb, reg, reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);

  /* load the stored counter value (immediate will be filled in by the layout code, when address producers are generated):
   *  LDR reg, [helper, #0] */
  ArmMakeInsForBbl(Ldr, Prepend, tmp, bbl, isThumb, reg, helper, ARM_REG_NONE,
    0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, FALSE /* up */, FALSE /* wb */);

  /* address producer, producing the address at which the counter for this BBL is stored:
   *  ADDRESS_PRODUCER helper, <address> */
  ArmMakeInsForBbl(Mov,  Prepend, ins, bbl, isThumb,  helper, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */
  rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                        AddressNullForObject(obj), /* addend */
                                        T_RELOCATABLE(ins), /* from */
                                        AddressNullForObject(obj), /* from-offset */
                                        T_RELOCATABLE(profiling_sec), /* to */
                                        offset, /* to-offset */
                                        FALSE, /* hell */
                                        NULL, /* edge */
                                        NULL, /* corresp */
                                        NULL, /* sec */
                                        "R00A00+\\l*w\\s0000$");
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  /* back up the used live registers, if any:
   *  PUSH {...} */
  if (registers != 0)
    ArmMakeInsForBbl(Push, Prepend, tmp, bbl, isThumb, registers, ARM_CONDITION_AL, isThumb);
}/* }}} */

void ArmReviveFromThumbStubs(t_cfg *cfg)
{
  return;

  t_bbl * bbl;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (ArmIsStubFromThumb(bbl))
    {
      /* keep this BBL and the following 2 alive */
      BblMark2(bbl);
      BblMark2(BBL_NEXT(bbl));
      BblMark2(BBL_NEXT(BBL_NEXT(bbl)));
    }
  }
}

void ArmAlignSpecialBasicBlocks(t_cfg *cfg)
{
  t_bbl * bbl;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (!BBL_INS_FIRST(bbl)) continue;

    if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) == TH_BX_R15)
    {
      t_bbl * next;

      next = BBL_NEXT(bbl);
      ASSERT(next, ("WTF? We expect a BBL following a BX PC instruction: @eiB", bbl));
      next = BBL_NEXT(next);
      ASSERT(next, ("WTF? We expect a second BBL following a BX PC instruction: @eiB\nFirst BBL: @eiB"));

      VERBOSE(1, ("Set alignment of BBL's @eiB (%p) and @eiB (%p) to 4", bbl, bbl, next, next));
      BBL_SET_ALIGNMENT(bbl, 4);
      BBL_SET_ALIGNMENT(next, 4);
    }
  }
}
/* vim: set shiftwidth=2 foldmethod=marker: */
