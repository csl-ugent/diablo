/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

/*!
 * Layouts a flowgraph, and builds a linear list of instructions that can be executed.
 *
 * \todo needs only one parameter
 *
 * \param obj Remove this parameter
 * \param code The section to layout (we actually take the flowgraph from this
 * section)
 *
 * \return void
*/
/* ArmDeflowgraph {{{ */
void ArmDeflowgraph(t_object *obj)
{
  t_cfg * cfg = OBJECT_CFG(obj);

  ArmCfgLayout(cfg,0);
}
/* }}} */


/* slack_for_alignment is a dynamically updated amount of slack that needs to be 
   taken into account when trying to find places for literal pools for address producers.
   We start with ORIG_SLACK_FOR_ALIGNMENT whenever we try find a location, because 8-byte alignment is quite 
   common given that we generate 8-byte-aligned literals ourselves.
   The amount of needed slack is updated in the find forward and backward functions: if 
   blocks are iterated over that have higher alignment requirements, the slack is increased
   accordingly. */

#define ORIG_SLACK_FOR_ALIGNMENT 8
int slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;

#define FW_LOAD_OFFSET (4096+8-slack_for_alignment)
#define BW_LOAD_OFFSET	(4096-8-4-slack_for_alignment)
/* -2 because of possible extra alignment requirement later on */
#define FW_LOAD_OFFSET_THUMB    (255*4-2)



static t_arm_ins *
ArmInsNextInChainSplittable(t_arm_ins * ins)
{
  /* we can't split IT-blocks.
   * Possible locations to split, are:
   *  - after regular instructions, except for IT-instructions;
   *  - after the last instruction of an IT-block. */
  t_arm_ins * ret = ArmInsNextInChain(ins);

  while (ret)
  {
    t_bool is_last_ins_in_block = FALSE;
    t_arm_ins * owning_it = ArmInsFindOwningIT(ret, &is_last_ins_in_block, NULL);

    if (!owning_it && (ARM_INS_OPCODE(ret) != ARM_T2IT)) return ret;
    else if (is_last_ins_in_block) return ret;

    ret = ArmInsNextInChain(ret);
  }

  return NULL;
}

/* return how many bits of offset can we encode in the (V)LDR that will implement the address of const producer */
static t_uint32 OffsetBitsAvailable(t_arm_ins * ins, t_bool thumb_only_in_2_bytes)
{
  if ((ARM_INS_FLAGS(ins) & FL_THUMB && (ARM_INS_CSIZE(ins)==2 || thumb_only_in_2_bytes)) || ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER)
    return 8;
  else
    return 12;
  
}

static t_uint32 NegativeOffsetBitsAvailable(t_arm_ins * ins)
{
  if ((ARM_INS_FLAGS(ins) & FL_THUMB && ARM_INS_CSIZE(ins)==2))
    return 0;
  else if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER)
    return 8;
  else
    return 12;
}


/* compute the base address (from the pc) to which an offset will have to be added */ 
static t_address BaseLdrAddress(t_arm_ins * ins)
{
  t_uint32  pc_offset = (ARM_INS_FLAGS(ins) & FL_THUMB) ? 4 : 8;
  t_address base_address= AddressAdd(AddressAnd(ARM_INS_CADDRESS(ins),AddressNew32(~0x3)),pc_offset);
  return base_address;
}

/* check what offset can be bridged from the LdrProducer */
/*
static t_int32 RangeLdrProducer(t_arm_ins * ins, t_bool thumb_only_in_2_bytes)
{
  t_uint32 offset_bits_available = OffsetBitsAvailable(ins, thumb_only_in_2_bytes);
  if (offset_bits_available == 12)
    return 4096 - 8; //to allow for extra alignment bytes at start of data pool
  else 
    return 1024 - 8; //to allow for extra alignment bytes at start of data pool
}
*/

/* check if the ins (a const or addr producer) is close enough to pool_address to encode the offset */
static t_bool single_in_fw_range(t_arm_ins * ins, t_address pool_address, t_bool thumb_only_in_2_bytes)
{
  t_uint32 offset_bits_available = OffsetBitsAvailable(ins, thumb_only_in_2_bytes);
  t_uint32 max_offset;
  t_uint32 pc_alignment = 0; // address of pc in thumb instructions is aligned, so might be decreased with 2
  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    pc_alignment = 2;

  if (offset_bits_available == 12)
    max_offset = 4096 - slack_for_alignment - pc_alignment; //to allow for extra alignment bytes at start of data pool
  else 
    max_offset = 1024 - slack_for_alignment - pc_alignment; //to allow for extra alignment bytes at start of data pool

  /* when a block ends with a producer, the base value (e.g., pc + 8) might be higher than 
     the address of the next ins. to avoid running into trouble with this corner case
     we fall back on the more conservative test that uses the address of the ins instead of the itself */
  //return AddressIsLt(AddressSub(pool_address,BaseLdrAddress(ins)),AddressNew32(max_offset));
  return AddressIsLt(AddressSub(pool_address,ARM_INS_CADDRESS(ins)),AddressNew32(max_offset));
}

static t_bool single_in_bw_range(t_arm_ins * ins, t_address pool_address)
{
  t_uint32 negative_offset_bits_available = NegativeOffsetBitsAvailable(ins);
  t_uint32 max_offset;
  t_uint32 data_size = 4;
  if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER)
    data_size += 4;

  if (negative_offset_bits_available == 12)
    max_offset = 4096 - data_size - slack_for_alignment; //to allow for extra alignment bytes after data pool
  else if (negative_offset_bits_available == 8)
    max_offset = 1024 - data_size - slack_for_alignment; //to allow for extra alignment bytes after data pool
  else
    return FALSE;

  return AddressIsLt(AddressSub(BaseLdrAddress(ins),pool_address),AddressNew32(max_offset));
}



static t_bool both_in_fw_range(t_arm_ins * dominant_imm12_ins, t_uint32 dominant_imm12_ins_pool_offset,
                                 t_arm_ins * dominant_imm8_ins, t_uint32 dominant_imm8_ins_pool_offset,
                               t_address pool_start_address, t_bool thumb_only_in_2_bytes)
{    
  if (dominant_imm12_ins)
    {
      t_address target_address = 
        AddressAddUint32(pool_start_address,dominant_imm12_ins_pool_offset);
      if (!single_in_fw_range(T_ARM_INS(dominant_imm12_ins),target_address, thumb_only_in_2_bytes))
        return FALSE;
    }

  if (dominant_imm8_ins)
    {
      t_address target_address = 
        AddressAddUint32(pool_start_address,dominant_imm8_ins_pool_offset);
      if (!single_in_fw_range(T_ARM_INS(dominant_imm8_ins),target_address, thumb_only_in_2_bytes))
        return FALSE;
      }
  return TRUE;
}

/* check if the ins_to_check (a const or addr prod to become an LDR), when it would access its data from the end of the current table
   would involve a greater PC-relative offset than the already foreseen such instruction with the greatest offset 
   and do so separately for instructions that can store 12 bit immediates and 8 bits immediates */

static void 
UpdateMostDistantProducers(t_arm_ins *ins_to_check, t_uint32 pool_size, t_address pool_start_address, 
                           t_arm_ins **dominant_imm12_ins, t_uint32 *dominant_imm12_ins_pool_offset,
                           t_arm_ins **dominant_imm8_ins, t_uint32 *dominant_imm8_ins_pool_offset,
                           t_bool thumb_only_in_2_bytes)
{

  t_arm_ins ** dominant_ins;
  t_uint32 * dominant_ins_pool_offset;
  t_uint32 offset_bits_available = OffsetBitsAvailable(ins_to_check, thumb_only_in_2_bytes);
  /* first we decide which type of most distant ins we need to update */
  if (offset_bits_available == 8)
    {
      /* can only encode 8 bits in immediate operand */
      dominant_ins = dominant_imm8_ins;
      dominant_ins_pool_offset = dominant_imm8_ins_pool_offset;      
    }
  else 
    {
      /* can encode 12 bits in immediate operand */
      dominant_ins = dominant_imm12_ins;
      dominant_ins_pool_offset = dominant_imm12_ins_pool_offset;
    }

  /* then we will do the updating by comparing */

  if (! *dominant_ins)
    {
      *dominant_ins = ins_to_check;
      *dominant_ins_pool_offset = pool_size;
    }
  else
  {
    t_address load_address_dominant_ins = BaseLdrAddress(*dominant_ins);
    t_address distance_dominant_ins = AddressAddUint32(AddressSub(pool_start_address,load_address_dominant_ins),*dominant_ins_pool_offset);

    t_address load_address = BaseLdrAddress(ins_to_check);
    t_address pool_end_address = AddressAddUint32(pool_start_address, pool_size);
    t_address new_distance = AddressSub(pool_end_address,load_address);

    if (AddressIsLt(distance_dominant_ins,new_distance))
    {
      *dominant_ins = ins_to_check;
      *dominant_ins_pool_offset = pool_size;
    }
  }
}


/* for all blocks in a bbl, when adding the entries necessary for their const and data producers to a data pool
   that would be placed right after this block, update the needed pool size, and keep track of which 
   instructions are furthest away from their data in the pool, for two categories of instructions:
   instructions that can store 8 bits for their immediate value and instructions that can store 12 bits */
static void ComputeMostDistantProducersInFwRange(t_bbl * bbl, t_uint32 *pool_size, t_uint32 * pool_alignment, 
                                                 t_arm_ins ** dominant_imm12_ins, t_uint32 * dominant_imm12_ins_pool_offset,
                                                 t_arm_ins ** dominant_imm8_ins, t_uint32 * dominant_imm8_ins_pool_offset,
                                                 t_bool thumb_only_in_2_bytes
                                                 )
{
  t_arm_ins *ins;
  t_address pool_start_address;                    
  
  pool_start_address = AddressAdd(BBL_CADDRESS(bbl), BBL_CSIZE(bbl));  
  BBL_FOREACH_ARM_INS(bbl, ins)
    {
      if ((ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_CONSTANT_PRODUCER) ||
          (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_ADDRESS_PRODUCER) || 
          (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER)
          )
        {
          /* insert noop if necessary to align 8-byte data */
          if ((ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER) && (ARM_INS_FLAGS(T_ARM_INS(ins)) & FL_VFP_DOUBLE))
            {
              *pool_alignment = 8;
              if ((*pool_size & 0x7) != 0)
                *pool_size += 4;
            }
          /* check if this could ins could become the dominant ins */
          UpdateMostDistantProducers(T_ARM_INS(ins), *pool_size, pool_start_address, 
                                     dominant_imm12_ins, dominant_imm12_ins_pool_offset,
                                     dominant_imm8_ins, dominant_imm8_ins_pool_offset,
                                     thumb_only_in_2_bytes
                                     );
          /* and assign space for the data */
          if (ARM_INS_OPCODE(T_ARM_INS(ins))==ARM_VFPFLOAT_PRODUCER && ARM_INS_FLAGS(T_ARM_INS(ins)) & FL_VFP_DOUBLE)
            *pool_size += 8;
          else 
            *pool_size += 4;
        }
    }
}

t_bbl * FindPreferredPoolBackward(t_arm_ins *ins)
{
  t_bbl *iter;
  t_bbl *preferred_candidate = NULL;
  t_bbl *pool;
  t_bbl * last_iter;
  t_uint32 already_passed_data_pools = 0;

  iter = ARM_INS_BBL (ins);

  last_iter = iter;

  slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;

  iter = BBL_PREV_IN_CHAIN(iter);
  
  if (!iter) return NULL;

  /* in case we would try to add something after an existing preferred data pool (which would then be placed after a forward search, because otherwise it is not
     preferred, use the start address of that data pool to check range. This models the fact that even though the data pool has been inserted in the chains,
     the addresses of instructions following later in the chain have not yet been updated to reflect that insertion */
  for (; iter && single_in_bw_range(ins,AddressSubUint32(BBL_CADDRESS (last_iter),already_passed_data_pools));
       iter = BBL_PREV_IN_CHAIN (iter))
    {

      /* we need to make sure that we reduce the search window because alignment might suddenly add more bytes in between blocks */
      if (BBL_ALIGNMENT(iter)>slack_for_alignment)
	slack_for_alignment = BBL_ALIGNMENT(iter);


      //DEBUG(("checked it for 0x%x and 0x%x",ARM_INS_CADDRESS(ins),BBL_CADDRESS(last_iter)));
      if ((BBL_ATTRIB(iter) & BBL_IS_PREFERRED_FOR_DATA_POOLS) && ArmBblCanInsertAfter(iter) && ((BBL_ATTRIB(iter) & BBL_FORWARD_DATAPOOL) || !(BBL_PRED_FIRST(iter)==NULL && BBL_SUCC_FIRST(iter)==NULL)))
      {
        //DEBUG(("found it with already passed data pool size = 0x%x",already_passed_data_pools));
        //DEBUG(("last iter size 0x%x @ieB",G_T_UINT32(BBL_CSIZE(last_iter)),last_iter));
        //DEBUG(("iter size 0x%x @ieB",G_T_UINT32(BBL_CSIZE(iter)),iter));
        preferred_candidate = iter;
        break;
      }

      if (BBL_ATTRIB(iter) & BBL_DATA_POOL)
        already_passed_data_pools += G_T_UINT32(BBL_CSIZE(iter)) + 4; // for possible padding bytes

      last_iter = iter;
    }
  
  if (!preferred_candidate)
    {
      //      DEBUG(("DID NOT FIND PREFERRED BACKWARD POOL FOR @I",ins));
      return NULL;
    }

  /* insert pool in chain */
  pool = BblNew (BBL_CFG (preferred_candidate));
  BBL_SET_ATTRIB(pool, BBL_DATA_POOL);
  if (BBL_ATTRIB(preferred_candidate) & BBL_FORWARD_DATAPOOL)
    // to make sure that the correct offset is computed in later backward searches
    // the reason is that the addresses following the already inserted data pool have not yet
    // been incremented with that pool's size
    // so if we assign the end address of that data pool to this block, the offset checking in later
    // backward searches will go wrong
    BBL_SET_CADDRESS (pool, BBL_CADDRESS (preferred_candidate)); 
  else
    BBL_SET_CADDRESS (pool,
                      AddressAdd (BBL_CADDRESS (preferred_candidate), BBL_CSIZE (preferred_candidate)));
  BBL_SET_ALIGNMENT(pool,4);
  BBL_SET_ALIGNMENT_OFFSET(pool,0);
  BblInsertInChainAfter(pool,preferred_candidate);

  //DEBUG(("FOUND PREFERRED BACKWARD POOL FOR @I after @B\n",ins,preferred_candidate));
  /*
  {
    t_bbl * bbl_it;
    for (bbl_it=preferred_candidate;bbl_it && bbl_it!=ARM_INS_BBL(ins);bbl_it=BBL_NEXT_IN_CHAIN(bbl_it))
      DEBUG(("   bbl with size 0x%x in chain: @ieB",BBL_CSIZE(bbl_it),bbl_it));
  }
  DEBUG(("   bbl in chain: @ieB",ARM_INS_BBL(ins)));
  */
  return pool;
}


t_bbl * FindPreferredPoolForward (t_arm_ins *ins, t_bool thumb_only_in_2_bytes)
{
  t_bbl *iter;
  t_bbl *preferred_candidate = NULL;
  t_bbl *pool;
  t_uint32 pool_size = 0;
  t_uint32 pool_alignment = 4;
  t_arm_ins * dominant_imm12_ins = NULL;
  t_arm_ins * dominant_imm8_ins = NULL;
  t_uint32 dominant_imm12_ins_pool_offset = 0;
  t_uint32 dominant_imm8_ins_pool_offset = 0;
  t_uint32 thumb_misc_offset_adjustment = 0;

  slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;

  iter = ARM_INS_BBL (ins);

  for (; iter && single_in_fw_range(ins,AddressAdd (BBL_CADDRESS (iter), BBL_CSIZE (iter)), thumb_only_in_2_bytes);
       iter = BBL_NEXT_IN_CHAIN (iter))
  {
    t_cfg_edge *edge;

    /* we need to make sure that we reduce the search window because alignment might suddenly add more bytes in between blocks */
    if (BBL_ALIGNMENT(iter)>slack_for_alignment)
      slack_for_alignment = BBL_ALIGNMENT(iter);

    ComputeMostDistantProducersInFwRange(iter,&pool_size,&pool_alignment,
                                         &dominant_imm12_ins,&dominant_imm12_ins_pool_offset,
                                         &dominant_imm8_ins,&dominant_imm8_ins_pool_offset,
                                         thumb_only_in_2_bytes
                                         );
    BBL_FOREACH_PRED_EDGE(iter,edge)
      {
        t_bbl *head = CFG_EDGE_HEAD(edge);
        t_arm_ins * last_ins = T_ARM_INS(BBL_INS_LAST(head));
        if (!last_ins) continue;
        if (BBL_IS_HELL(head))
          continue;
        
        if (INS_CSIZE(T_INS(last_ins)) == 2 &&
            ARM_INS_OPCODE(last_ins) == ARM_B && 
            ARM_INS_CONDITION(last_ins) != ARM_CONDITION_AL
            )
          {
            thumb_misc_offset_adjustment +=4;
            break;
          }
        
        if (INS_CSIZE(T_INS(last_ins)) == 2 && 
            (ARM_INS_OPCODE(last_ins) == ARM_T2CBZ || ARM_INS_OPCODE(last_ins) == ARM_T2CBNZ)
            )
          {
            thumb_misc_offset_adjustment +=4;  
            break;
          }
      }

    if (!both_in_fw_range(dominant_imm12_ins,dominant_imm12_ins_pool_offset,
                          dominant_imm8_ins,dominant_imm8_ins_pool_offset,
                          AddressAdd(BBL_CADDRESS (iter), BBL_CSIZE (iter)),
                          thumb_only_in_2_bytes
                          )
        )
      break;

    if (ArmBblCanInsertAfter(iter))
      {
        /* TODO: we should also make sure in Thumb2 that no pools are inserted
           after TBB or ... instructions */
        if (!(BBL_PRED_FIRST(iter)==NULL && BBL_SUCC_FIRST(iter)==NULL))
          {
            if (BBL_ATTRIB(iter) & BBL_IS_PREFERRED_FOR_DATA_POOLS)
              preferred_candidate = iter;
          }
      }
  }

  if (!preferred_candidate)
    {
      //DEBUG(("RUN %d: DID NOT FIND FORWARD BACKWARD POOL FOR @I",thumb_only_in_2_bytes,ins));
      return NULL;
    }

  /* insert pool in chain */
  pool = BblNew (BBL_CFG (preferred_candidate));
  BBL_SET_ATTRIB(pool, BBL_DATA_POOL | BBL_IS_PREFERRED_FOR_DATA_POOLS | BBL_FORWARD_DATAPOOL);
  /* once we access a pool from in front of it, we should no longer allow others to put something in front of it */
  BBL_SET_ATTRIB(preferred_candidate, BBL_ATTRIB(preferred_candidate) & ~BBL_IS_PREFERRED_FOR_DATA_POOLS);
  BBL_SET_CADDRESS (pool,
      AddressAdd (BBL_CADDRESS (preferred_candidate), BBL_CSIZE (preferred_candidate)));
  BBL_SET_ALIGNMENT(pool,4);
  BBL_SET_ALIGNMENT_OFFSET(pool,0);
  BblInsertInChainAfter(pool,preferred_candidate);

  //DEBUG(("RUN %d FOUND PREFERRED FORWARD POOL FOR @I",thumb_only_in_2_bytes,ins));

  return pool;
}

t_bbl * FindForwardPool (t_arm_ins *ins)
{
  t_bbl *iter;
  t_bbl *candidate = NULL;
  t_bbl *pool;
  t_cfg *cfg = ARM_INS_CFG (ins);
  t_uint32 pool_size = 0;
  t_uint32 pool_alignment = 4;
  t_arm_ins * dominant_imm12_ins = NULL;
  t_arm_ins * dominant_imm8_ins = NULL;
  t_uint32 dominant_imm12_ins_pool_offset = 0;
  t_uint32 dominant_imm8_ins_pool_offset = 0;
  t_uint32 thumb_misc_offset_adjustment = 0;

  slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;

  iter = ARM_INS_BBL (ins);

  for (; iter && single_in_fw_range(ins,AddressAdd (BBL_CADDRESS (iter), BBL_CSIZE (iter)),FALSE);
       iter = BBL_NEXT_IN_CHAIN (iter))
  {
    t_cfg_edge *edge;

    /* we need to make sure that we reduce the search window because alignment might suddenly add more bytes in between blocks */
    if (BBL_ALIGNMENT(iter)>slack_for_alignment)
      slack_for_alignment = BBL_ALIGNMENT(iter);

    ComputeMostDistantProducersInFwRange(iter,&pool_size,&pool_alignment,
                                         &dominant_imm12_ins,&dominant_imm12_ins_pool_offset,
                                         &dominant_imm8_ins,&dominant_imm8_ins_pool_offset, 
                                         FALSE
                                        );
    BBL_FOREACH_PRED_EDGE(iter,edge)
      {
        t_bbl *head = CFG_EDGE_HEAD(edge);
        t_arm_ins * last_ins = T_ARM_INS(BBL_INS_LAST(head));
        if (!last_ins) continue;
        if (BBL_IS_HELL(head))
          continue;
        
        if (INS_CSIZE(T_INS(last_ins)) == 2 &&
            ARM_INS_OPCODE(last_ins) == ARM_B && 
            ARM_INS_CONDITION(last_ins) != ARM_CONDITION_AL
            )
          {
            thumb_misc_offset_adjustment +=4;
            break;
          }
        
        if (INS_CSIZE(T_INS(last_ins)) == 2 && 
            (ARM_INS_OPCODE(last_ins) == ARM_T2CBZ || ARM_INS_OPCODE(last_ins) == ARM_T2CBNZ)
            )
          {
            thumb_misc_offset_adjustment +=4;  
            break;
          }
      }

    if (!both_in_fw_range(dominant_imm12_ins,dominant_imm12_ins_pool_offset,
                          dominant_imm8_ins,dominant_imm8_ins_pool_offset,
                          AddressAdd(BBL_CADDRESS (iter), BBL_CSIZE (iter)),
                          FALSE
                          )
        )
      break;

    if (ArmBblCanInsertAfter(iter))
      {
        /* TODO: we should also make sure in Thumb2 that no pools are inserted
           after TBB or ... instructions */
        if (!(BBL_PRED_FIRST(iter)==NULL && BBL_SUCC_FIRST(iter)==NULL))
          {
            candidate = iter;
            break; // don't try to put as many as possible in a table: try small tables instead
          }
      }
  }

  ASSERT (candidate || iter, ("chain ends in fallthrough path: @ieB",
	BBL_LAST_IN_CHAIN (ARM_INS_BBL (ins))));

  if (candidate)
    {
      //DEBUG(("FOUND NON-PREFERRED FORWARD POOL FOR @I",ins));
    }
  else
    { 
      //      DEBUG(("DID NOT EVEN FIND NON-PREFERRED FORWARD POOL IN FIRST ROUND FOR @I",ins));
    }
  
  if (!candidate)
  {
    t_arm_ins *branch;
    t_cfg_edge *edge;
    t_bool is_interproc = FALSE, has_corr = FALSE;

    /* split chain to create space a place for the data pool */
    /* same hack with PRED and SUCC edges as before to avoid that we put data pools
       right after the no-ops inserted after LDR r15, r15, LSL ... switch tables */
    while (!both_in_fw_range(dominant_imm12_ins,dominant_imm12_ins_pool_offset,
                          dominant_imm8_ins,dominant_imm8_ins_pool_offset,
                             AddressAddUint32(BBL_CADDRESS (iter), 4), FALSE)
           || ArmBblInSwitchStatement(iter,FALSE) 
           //           || (!(BBL_ATTRIB(iter) & BBL_IS_PREFERRED_FOR_DATA_POOLS))
           || (BBL_PRED_FIRST(iter)==NULL && BBL_SUCC_FIRST(iter)==NULL)
           /* here also need to make sure TBB blocks are not causing problems */
           )
      {
        if (AddressIsLt(BBL_CADDRESS (iter), ARM_INS_CADDRESS(ins)))
          {
            iter = NULL;
            break;
          }
        iter = BBL_PREV_IN_CHAIN (iter);
      }
    
    if (iter == ARM_INS_BBL(ins))
      iter = NULL;
    
   if (iter && AddressIsLt(BBL_CADDRESS (iter), ARM_INS_CADDRESS(ins)))
     iter = NULL;

    if (iter)
      {
        //        DEBUG((" found second round! @B",iter));

        candidate = iter;
        iter = BblSplitBlock (candidate, BBL_INS_FIRST (candidate), TRUE);
        edge = BBL_SUCC_FIRST (candidate);
        is_interproc = CFG_EDGE_CAT (edge) == ET_IPFALLTHRU;
        if (CFG_EDGE_CORR (edge))
          {
            CfgEdgeKill (CFG_EDGE_CORR (edge));
            has_corr = TRUE;
          }
        CfgEdgeKill (edge);
        edge = CfgEdgeCreate (cfg, candidate, iter, is_interproc?ET_IPJUMP:ET_JUMP);
        if (has_corr)
          CfgEdgeCreateCompensating (cfg, edge);
        branch = T_ARM_INS(InsNewForBbl (candidate));
        if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(iter))) & FL_THUMB)
          {
            ARM_INS_SET_FLAGS(branch, ARM_INS_FLAGS(branch) |  FL_THUMB);
            ARM_INS_SET_CSIZE(branch, AddressNew32(2));
          }
        ArmInsAppendToBbl (branch, candidate);
        ArmInsMakeUncondBranch (branch);
        BBL_SET_ATTRIB(candidate,BBL_ATTRIB(candidate) & ~BBL_IS_PREFERRED_FOR_DATA_POOLS); // to make sure this block is not seen as a potential preferred spot in a following backward search
      }
    else
    {
      t_arm_ins * split_at, * j_ins;
      t_address pool_start_address;

      dominant_imm12_ins = NULL;
      dominant_imm8_ins = NULL;

      if (OffsetBitsAvailable(ins,FALSE) == 12)
        {
          dominant_imm12_ins = ins;
        }
      else 
        {
          dominant_imm8_ins = ins;
        }

      pool_size = AddressNew32(0);

      for (split_at=ArmInsNextInChainSplittable(ins),
             pool_start_address = AddressAdd (ARM_INS_CADDRESS(split_at),
                                              AddressMulUint32 (ARM_INS_CSIZE(split_at), 3));
           both_in_fw_range(dominant_imm12_ins,dominant_imm12_ins_pool_offset,
                            dominant_imm8_ins,dominant_imm8_ins_pool_offset,
                            pool_start_address,FALSE);
           split_at=ArmInsNextInChainSplittable(split_at),
             pool_start_address = AddressAdd (ARM_INS_CADDRESS(split_at),
                                              AddressMulUint32 (ARM_INS_CSIZE(split_at), 3)))
        {
          UpdateMostDistantProducers(split_at, pool_size, pool_start_address,
                                     &dominant_imm12_ins,&dominant_imm12_ins_pool_offset,
                                     &dominant_imm8_ins,&dominant_imm8_ins_pool_offset,
                                     FALSE
                                     );

          if (!both_in_fw_range(dominant_imm12_ins,dominant_imm12_ins_pool_offset,
                           dominant_imm8_ins,dominant_imm8_ins_pool_offset,
                                pool_start_address, FALSE))
            break;

          if (ARM_INS_OPCODE(T_ARM_INS(split_at))==ARM_VFPFLOAT_PRODUCER && ARM_INS_FLAGS(T_ARM_INS(split_at)) & FL_VFP_DOUBLE)
            {
              if (pool_alignment<8)
                {
                  pool_size +=4 ; // for possible alignment bytes that need to be added at the start of the pool
                  //DEBUG(("1: size + 4 = %d for @I",pool_size,split_at));
                } 
              pool_alignment = 8;
              if ((pool_size & 0x7) != 0)
                {
                  pool_size += 4;
                  //DEBUG(("2: size + 4 = %d for @I",pool_size,split_at));
                }
              pool_size += 8;
              //DEBUG(("3: size + 4 = %d for @I",pool_size,split_at));
            }
          else if ((ARM_INS_OPCODE(T_ARM_INS(split_at)) == ARM_CONSTANT_PRODUCER) ||
                   (ARM_INS_OPCODE(T_ARM_INS(split_at)) == ARM_ADDRESS_PRODUCER))
            {
              pool_size += 4;
              //DEBUG(("4: size + 4 = %d for @I",pool_size,split_at));
            }
      };

      //DEBUG((" found third round with estimated pool size %d!",pool_size));
      j_ins = (t_arm_ins *)InsNewForCfg(cfg);
      if (ARM_INS_FLAGS(split_at) & FL_THUMB)
        {
          ARM_INS_SET_FLAGS(j_ins, ARM_INS_FLAGS(j_ins) | FL_THUMB);
          ARM_INS_SET_CSIZE(j_ins, AddressNew32(2));
        }
      
      if (ARM_INS_OPCODE(split_at)==ARM_B || ARM_INS_OPCODE(split_at)==ARM_BL)
        {
          split_at = ArmInsNextInChain(split_at);
          ArmInsInsertBefore(j_ins,split_at);
          ArmInsMakeUncondBranch(j_ins);
          BblSplitBlock(ARM_INS_BBL(split_at),T_INS(j_ins),FALSE);
          BBL_SET_ATTRIB(ARM_INS_BBL(j_ins),BBL_ATTRIB(ARM_INS_BBL(j_ins)) & ~BBL_IS_PREFERRED_FOR_DATA_POOLS); // to make sure this block is not seen as a potential preferred spot in a following backward search
          CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(ARM_INS_BBL(j_ins)),  ET_JUMP);
          BblSetAddressSuper(ARM_INS_BBL(j_ins), BBL_CADDRESS(ARM_INS_BBL(j_ins)));
          AssignAddressesInChain (ARM_INS_BBL(j_ins), BBL_CADDRESS(ARM_INS_BBL(j_ins)));
          candidate=ARM_INS_BBL(j_ins);
        }
      else /* TODO: refactor code: split_at can be replaced with j_ins in last lines, which then become shared with then case */
        {
          ArmInsInsertAfter(j_ins,split_at);
          ArmInsMakeUncondBranch(j_ins);
          BblSplitBlock(ARM_INS_BBL(split_at),T_INS(j_ins),FALSE);
          BBL_SET_ATTRIB(ARM_INS_BBL(j_ins),BBL_ATTRIB(ARM_INS_BBL(j_ins)) & ~BBL_IS_PREFERRED_FOR_DATA_POOLS); // to make sure this block is not seen as a potential preferred spot in a following backward search
          CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(ARM_INS_BBL(split_at)),  ET_JUMP);
          BblSetAddressSuper(ARM_INS_BBL(split_at), BBL_CADDRESS(ARM_INS_BBL(split_at)));
          AssignAddressesInChain (ARM_INS_BBL(split_at), BBL_CADDRESS(ARM_INS_BBL(split_at)));
          candidate=ARM_INS_BBL(split_at);
        }
    }
  }

  VERBOSE(2,("Using pool at @G for instruction @I", AddressAdd (BBL_CADDRESS (candidate), BBL_CSIZE (candidate)), ins));

  /* insert pool in chain */
  pool = BblNew (BBL_CFG (candidate));
  BBL_SET_ATTRIB(pool, BBL_DATA_POOL | BBL_FORWARD_DATAPOOL);// | BBL_IS_PREFERRED_FOR_DATA_POOLS);
  BBL_SET_CADDRESS (pool,
      AddressAdd (BBL_CADDRESS (candidate), BBL_CSIZE (candidate)));
  BBL_SET_ALIGNMENT(pool,4);
  BBL_SET_ALIGNMENT_OFFSET(pool,0);
  BblInsertInChainAfter(pool,candidate);
  return pool;
}

static void ArmSetPreferredDataPoolPoints(t_bbl * chain_head)
{
  t_bbl * bbl;
  t_arm_ins * ins;

  CHAIN_FOREACH_BBL(chain_head,bbl)
    {
      BBL_SET_ATTRIB(bbl,BBL_ATTRIB(bbl) | BBL_IS_PREFERRED_FOR_DATA_POOLS);
    }
  
  bbl = chain_head;
  
  while (bbl)
    {
      ins = T_ARM_INS(BBL_INS_LAST(bbl));
      if (ins && (ARM_INS_OPCODE(ins)==ARM_T2CBZ || ARM_INS_OPCODE(ins)==ARM_T2CBNZ))
        {
          t_bbl * tmp = bbl;
          t_cfg_edge *  edge; 
          t_address tail_address;
          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            {
              if (CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
                break;
            }
          tail_address = BBL_CADDRESS(CFG_EDGE_TAIL(edge));
          while (tmp && AddressIsLt(BBL_CADDRESS(tmp),tail_address))
            {
              BBL_SET_ATTRIB(tmp,BBL_ATTRIB(tmp) &~ BBL_IS_PREFERRED_FOR_DATA_POOLS);
              tmp = BBL_NEXT_IN_CHAIN(tmp);
            }
        }
      else if (ins && (ARM_INS_OPCODE(ins)==ARM_T2TBB || ARM_INS_OPCODE(ins)==ARM_T2TBH || ArmInsIsSwitchedBL(ins)))
        {
          t_cfg_edge *  edge; 
          BBL_FOREACH_SUCC_EDGE(bbl,edge)
            {
              /* for switched BL-instructions, skip the call edge */
              if (CFG_EDGE_CAT(edge) != ET_SWITCH)
                continue;

              t_bbl * tmp = bbl;
              t_address tail_address = BBL_CADDRESS(CFG_EDGE_TAIL(edge));
              while (tmp && AddressIsLt(BBL_CADDRESS(tmp),tail_address))
                {
                  BBL_SET_ATTRIB(tmp,BBL_ATTRIB(tmp) &~ BBL_IS_PREFERRED_FOR_DATA_POOLS);
                  tmp = BBL_NEXT_IN_CHAIN(tmp);
                }
            }
        }
      
      bbl = BBL_NEXT_IN_CHAIN(bbl);
    }
}

static t_arm_ins * AppendDataToPool(t_arm_ins * ins, t_bbl * pool)
{
  t_arm_ins *data = NULL;

  if (ARM_INS_OPCODE(ins)==ARM_ADDRESS_PRODUCER || ARM_INS_OPCODE(ins)==ARM_CONSTANT_PRODUCER)
    {
      data=T_ARM_INS(InsNewForBbl (pool));
      ARM_INS_SET_CSIZE(data, AddressNew32(4));
      ARM_INS_SET_OLD_ADDRESS(data,ARM_INS_OLD_ADDRESS(ins)+1);
      ARM_INS_SET_CADDRESS(data, AddressAdd(BBL_CADDRESS(pool), BBL_CSIZE(pool)));
      ArmInsAppendToBbl (data, pool);
      ArmInsMakeData (T_ARM_INS (data), ARM_INS_IMMEDIATE (ins));
      ARM_INS_SET_ATTRIB(data,  ARM_INS_ATTRIB(data) | IF_ADDRESS_POOL_ENTRY);
      VERBOSE(1,("Added entry to pool @ieB for @I",pool,ins));
    }
  else
    {
      int existing_alignment_pool = BBL_ALIGNMENT(pool);
      /* TODO: for the time being, assume 8-byte alignement is needed*/
      /* TODO: we should compute the needed alignment based on the needs
         of the producing ins instead */
      int needed_alignment = 4; /* TODO: depends on s or d register of VLDR */
      
      if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
        needed_alignment = 8;
      
      char * value = ARM_INS_DATA(ins);
      if (existing_alignment_pool<needed_alignment)
        {
          //          DEBUG(("BBL SET ALIGNMENT TO %d",needed_alignment));
          BBL_SET_ALIGNMENT(pool,needed_alignment);
        }
      
      while ((G_T_UINT32(BBL_CSIZE(pool)) % needed_alignment) != 0)
        {
          data=T_ARM_INS(InsNewForBbl (pool));
          ARM_INS_SET_CSIZE(data, AddressNew32(4));
          ARM_INS_SET_CADDRESS(data, AddressAdd(BBL_CADDRESS(pool), BBL_CSIZE(pool)));
          ArmInsAppendToBbl (data, pool);
          /* TODO: we should actually mark the inserted data as not used, such that it can be filled
             with useful data later on */
          ArmInsMakeData (T_ARM_INS (data), 0);
          ARM_INS_SET_ATTRIB(data,  ARM_INS_ATTRIB(data) | IF_ADDRESS_POOL_ENTRY);
        }
      
      data=T_ARM_INS(InsNewForBbl (pool));
      ARM_INS_SET_OLD_ADDRESS(data,ARM_INS_OLD_ADDRESS(ins)+1);
      ARM_INS_SET_CSIZE(data, AddressNew32(4));
      ARM_INS_SET_CADDRESS(data, AddressAdd(BBL_CADDRESS(pool), BBL_CSIZE(pool)));
      ArmInsAppendToBbl (data, pool);
      ArmInsMakeData (T_ARM_INS (data), *((t_uint32 *) value));
      ARM_INS_SET_ATTRIB(data,  ARM_INS_ATTRIB(data) | IF_ADDRESS_POOL_ENTRY);
      
      t_uint32 ins_address = G_T_UINT32(ARM_INS_CADDRESS(ins)) & 0xfffffffc;
      t_int32 offset=G_T_UINT32(ARM_INS_CADDRESS(data))-ins_address-((ARM_INS_FLAGS(ins) & FL_THUMB)?4:8);
      
      ASSERT(offset<1024,("offset %d too big in @I\n",offset,ins));
      
      if ((ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE) || (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE) || (ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(ins)&FL_FLT_PACKED))
        {
          t_arm_ins * extra_data = T_ARM_INS(InsNewForBbl (pool));
          value += 4;
          ARM_INS_SET_CSIZE(extra_data, AddressNew32(4));
          ARM_INS_SET_CADDRESS(extra_data, AddressAdd(BBL_CADDRESS(pool), BBL_CSIZE(pool)));
          ArmInsAppendToBbl (extra_data, pool);
          ArmInsMakeData (T_ARM_INS (extra_data), *((t_uint32 *) value));
        }
      
      if ((ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(ins)&FL_FLT_PACKED))
        {
          t_arm_ins * extra_data = T_ARM_INS(InsNewForBbl (pool));
          value += 4;
          ARM_INS_SET_CSIZE(extra_data, AddressNew32(4));
          ARM_INS_SET_CADDRESS(extra_data, AddressAdd(BBL_CADDRESS(pool), BBL_CSIZE(pool)));
          ArmInsAppendToBbl (extra_data, pool);
          ArmInsMakeData (T_ARM_INS (extra_data), *((t_uint32 *) value));
        }
      /* TODO: 4th word for quad register VLDR? */
    }
  return data;
}

static void ConvertProducerToLoad(t_object * obj, t_arm_ins * ins, t_arm_ins * data, t_bool dir_up)
{
  static char * code_arm_ldr_down = "Ps0008+R00-\\=s0fff&lifffff000&|w\\ifffff000&$";
  static char * code_arm_ldr_up   = "R00P-s0008-\\=s0fff&lifffff000&|w\\ifffff000&$";
  static char * code_thumb_ldr4_down = "Pifffffffc&s0004+R00-\\=s0fff&s0010<lif000ffff&|w\\ifffff000&$";
  static char * code_thumb_ldr4_up   = "R00Pifffffffc&s0004+-\\=s0fff&s0010<lif000ffff&|w\\ifffff000&$";
  static char * code_thumb_ldr2_up   = "R00Pifffffffc&s0004+-\\=s0002>s00ff&liffffff00&|w\\ifffffc03&$";
  static char * code_arm_vldr_down = "Ps0008+R00-\\=s0002>s00ff&liffffff00&|w\\ifffffc03&$";
  static char * code_arm_vldr_up   = "R00P-s0008-\\=s0002>s00ff&liffffff00&|w\\ifffffc03&$";
  static char * code_thumb_vldr_down = "Pifffffffc&s0004+R00-\\=s0002>s00ff&s0010<liff00ffff&|w\\ifffffc03&$";
  static char * code_thumb_vldr_up   = "R00Pifffffffc&-s0004-\\=s0002>s00ff&s0010<liff00ffff&|w\\ifffffc03&$";

  if (ARM_INS_REFERS_TO (ins))
    RelocSetFrom (RELOC_REF_RELOC(ARM_INS_REFERS_TO (ins)), T_RELOCATABLE (data));
  
  if (ARM_INS_OPCODE(ins)==ARM_CONSTANT_PRODUCER || ARM_INS_OPCODE(ins)==ARM_ADDRESS_PRODUCER)
    ArmInsMakeLdr (ins, ARM_INS_REGA (ins), ARM_REG_R15, ARM_REG_NONE, 0x0, ARM_INS_CONDITION (ins), TRUE, dir_up, FALSE);
  else
    {
      ARM_INS_SET_OPCODE(ins,  ARM_VLDR);
      ARM_INS_SET_REGB(ins,  15);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(ins,  0);
      if (dir_up)
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED | FL_PREINDEX | FL_DIRUP);
      else
        ARM_INS_SET_FLAGS(ins, (ARM_INS_FLAGS(ins) | FL_IMMED | FL_PREINDEX) & ~FL_DIRUP);
      ARM_INS_SET_TYPE(ins,  IT_FLT_LOAD);
      ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
      ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
    }
  
  ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_ADDRESS_PRODUCER);
  
  char * code;

  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    {
      if (ARM_INS_OPCODE(ins)==ARM_LDR)
        {
          if (dir_up)
            code = code_arm_ldr_up;
          else
            code = code_arm_ldr_down;
        }
      else /* vldr for VFPFLOAT_PRODUCER */
        {
          if (dir_up)
            code = code_arm_vldr_up;
          else
            code = code_arm_vldr_down;
        }
    }
  else
    {
      if ((ARM_INS_CSIZE(ins)==2))
        {
          code = code_thumb_ldr2_up ;
        }
      else if (ARM_INS_OPCODE(ins)==ARM_LDR)
        {
          if (dir_up)
            code = code_thumb_ldr4_up;
          else
            code = code_thumb_ldr4_down;
        }
      else /* vldr for VFPFLOAT_PRODUCER */
        {
          if (dir_up)
            code = code_thumb_vldr_up;
          else
            code = code_thumb_vldr_down;
        }
    }

  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), 
                                  AddressNullForObject(obj), 
                                  T_RELOCATABLE(ins), 
                                  AddressNullForObject(obj), 
                                  T_RELOCATABLE(data), 
                                  AddressNullForObject(obj), 
                                  FALSE, NULL, NULL, NULL, 
                                  code
                                  );
}

void check_sizes(t_bbl * chain, int id)
{
  t_bbl * bbl;
  t_arm_ins * ins;

  CHAIN_FOREACH_BBL(chain,bbl)
    {
      t_uint32 bbl_size = G_T_UINT32(BBL_CSIZE(bbl));
      t_uint32 ins_size = 0;
      BBL_FOREACH_ARM_INS(bbl, ins)
        {
          ins_size += G_T_UINT32(ARM_INS_CSIZE(ins));
        }

      if (bbl_size!=ins_size)
        DEBUG(("LINE %d,BBL SIZE %d != %d SUM INS SIZE FOR BBL @iB",id,bbl_size,ins_size,bbl));
    }

}

static void
RealAddressProducersForChain (t_bbl *chain, t_bool optimize, t_bool do_ldr_opt)
{
  t_arm_ins *start_range;
  t_object * obj=CFG_OBJECT(BBL_CFG(chain));
  t_bbl * bbl;
  t_arm_ins * ins;

  /* when generating PIC code, we can't do the ldr optimisation
   * (because it assumes the code is loaded at a fixed address)
   */
  if ((OBJECT_TYPE(obj)==OBJTYP_EXECUTABLE_PIC) ||
      (OBJECT_TYPE(obj)==OBJTYP_SHARED_LIBRARY_PIC))
    do_ldr_opt = FALSE;

  /* preprocess all constant producers: the simple ones should never
     be stored in constant pools in the first place */

  CHAIN_FOREACH_BBL (chain, bbl)
  {
        BBL_FOREACH_ARM_INS(bbl, ins)
        {
                if (((ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER) ||
                    (ARM_INS_OPCODE(ins) == ARM_ADDRESS_PRODUCER) ||
                    (ARM_INS_OPCODE(ins) == ARM_VFPFLOAT_PRODUCER)) &&
                    (ARM_INS_CSIZE(ins) == 2))
                {
                        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
                        ARM_INS_SET_CSIZE(ins, 4);

                        BBL_SET_CSIZE(bbl, BBL_CSIZE(bbl)+2);
                }
        }
  }

  /* TODO: the function called in the loop below probably has to be updated to support Thumb2 moves of different widths */
  CHAIN_FOREACH_BBL (chain, bbl)
    BBL_FOREACH_ARM_INS (bbl, ins)
      if (ARM_INS_OPCODE (ins) == ARM_CONSTANT_PRODUCER)
        GenerateInstructionsForConstProdIfPossible(ins, ARM_INS_IMMEDIATE(ins));

  /* TODO: the function probably also needs to be adapted for Thumb2 code, not sure though */
  ArmGenerateFloatProducers(obj);

  /* FROM HERE ON, ALL INSTRUCTIONS SHOULD HAVE A FIXED WIDTH */

  AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  ArmSetPreferredDataPoolPoints(chain);

  start_range = (t_arm_ins *) BBL_INS_FIRST (chain);

  bbl = chain;
  while (!start_range)
  {
    bbl = BBL_NEXT_IN_CHAIN(bbl);
    ASSERT(bbl, ("Chains are corrupt. Could not find initial instruction!"));
    start_range = (t_arm_ins *) BBL_INS_FIRST (bbl);
  }

  STATUS(START,("Converting const/address producers to LDRs"));

  while (1)
  {
    t_arm_ins *ins;
    t_bbl *pool;

    /* find next address producer */
    for (; start_range; start_range = ArmInsNextInChain (start_range))
      if (ARM_INS_OPCODE (start_range) == ARM_ADDRESS_PRODUCER
	  || ARM_INS_OPCODE (start_range) == ARM_CONSTANT_PRODUCER
          || ARM_INS_OPCODE (start_range) == ARM_VFPFLOAT_PRODUCER
          )
	break;

    if (!start_range)
      break;

    //DEBUG(("STARTING FOR INS @I",start_range));

    pool = FindPreferredPoolForward(start_range, TRUE);
    if (!pool)
      {    
        pool = FindPreferredPoolForward(start_range, FALSE);
        if (!pool)
          {    
            pool = FindPreferredPoolBackward(start_range);
            if (pool)
              {
                /* only add one element */
                t_arm_ins * ins = start_range;
                t_arm_ins * data = AppendDataToPool(ins,pool);
                ConvertProducerToLoad(obj,ins,data,FALSE);
                continue;
              }
            pool = FindForwardPool(start_range);
          }
      }
    
    //    if (pool)
    //      DEBUG(("FOUND POOL @B",pool));

    /* create address producers for instructions up to pool */
    for (ins = start_range; ins && (ARM_INS_BBL(T_ARM_INS(ins)) != pool); ins = ArmInsNextInChain (ins))
    {
      t_arm_ins *data = NULL;

      if (ARM_INS_OPCODE (ins) != ARM_ADDRESS_PRODUCER
          && ARM_INS_OPCODE (ins) != ARM_CONSTANT_PRODUCER
          && ARM_INS_OPCODE (ins) != ARM_VFPFLOAT_PRODUCER
          )
	continue;

      /* first try to reuse a value in the pool */
      /* for the time being, we do not try to reuse a value for VFP_FLOAT producers */
      /* TODO: implement reuse for VFP_FLOAT_PRODUCERS */
      //      DEBUG(("   HANDLING @I",ins));

      BBL_FOREACH_ARM_INS(pool, data)
      {
	if (ARM_INS_REFERS_TO (ins))
        {
          if (ARM_INS_REFERS_TO(data))
          {
            if (!RelocCmp(RELOC_REF_RELOC(ARM_INS_REFERS_TO (ins)),
                  RELOC_REF_RELOC(ARM_INS_REFERS_TO (data)), FALSE))
              break;
          }
        }
	else if (!ARM_INS_REFERS_TO(data))
	{
          if (ARM_INS_OPCODE(ins)==ARM_CONSTANT_PRODUCER)
            if (ARM_INS_IMMEDIATE(data)==ARM_INS_IMMEDIATE(ins))
              break;

          if (ARM_INS_OPCODE(ins)==ARM_VFPFLOAT_PRODUCER && !(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE))
            if (ARM_INS_IMMEDIATE(data)==*((t_uint32 *) ARM_INS_DATA(ins)))
              break;
	}
      }

      if (!data)
        data = AppendDataToPool(ins,pool);
      else
        VERBOSE(1,("Reused data entry @I for @I",data,ins));

      ConvertProducerToLoad(obj,ins,data,TRUE) ;
      VERBOSE(1,("Constant/Address producer transformed into @I refering to @I",ins,data));
    }

    ASSERT(ins, ("Address producer algorithm failed (probably caused by bad choice for pool - should not happen. This is a bug in FindForwardPool)"));

    //    AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));

    if (!BBL_NEXT_IN_CHAIN (pool))
      break;

    if (ins)
      {

        /* make sure no backward blocks will be inserted before forward inserted pool */
        t_bbl * tmp = pool;
        t_bbl * start_bbl = ARM_INS_BBL(start_range);
        tmp = BBL_PREV_IN_CHAIN(tmp);
        while (tmp)
          {
            BBL_SET_ATTRIB(tmp,BBL_ATTRIB(tmp) &~ BBL_IS_PREFERRED_FOR_DATA_POOLS);
            if (tmp==start_bbl)
              break;
            tmp = BBL_PREV_IN_CHAIN(tmp);
          }
      }


    start_range = ins;
  }

  AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  STATUS(STOP,("Converting const/address producers to LDRs"));
}


static void
RealOptimizeAddressProducersForChain (t_bbl *chain, t_bool optimize, t_bool do_ldr_opt)
{
  t_object * obj=CFG_OBJECT(BBL_CFG(chain));
  t_bbl * bbl;
  t_arm_ins * ins, * tmp;

  if (!optimize)
    return;

  /* ALL OPTIMIZATIONS BELOW WILL NEED TO BE FIXED FOR thumb2. But first, they can simply be skipped by returning
     non-conditionally */

  STATUS(START,("Optimizing LDRs of const/address producers"));

  if (!diabloarm_options.noaddraddsub)
  {
    /* Optimization number 1: turn loads into add/sub from pc */
    CHAIN_FOREACH_BBL (chain, bbl)
    {
      BBL_FOREACH_ARM_INS(bbl, ins)
      {
        if (ARM_INS_ATTRIB(ins) & IF_ADDRESS_PRODUCER)
        {
    t_bool to_intra_chain_bbl = TRUE;
    t_uint32 i;
    t_reloc_ref *to_ref = ARM_INS_REFERS_TO(T_ARM_INS(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[0]));
    t_reloc *apreloc = to_ref ? RELOC_REF_RELOC(to_ref) : NULL;

    if (to_ref)
    {
      for (i=0; i<RELOC_N_TO_RELOCATABLES(apreloc); i++)
      {
        if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(apreloc)[i]) == RT_BBL) &&
      ((BBL_FIRST_IN_CHAIN(T_BBL(RELOC_TO_RELOCATABLE(apreloc)[i]))) == chain))
        {

        }
        else
        {
          to_intra_chain_bbl = FALSE;
        }
      }
    }

          /* Don't try to convert producers referring to more than one relocatable.
          * For producers referring to more relocatables, the distance between the two relocatables
          * is calculated. At this point in the code, this distance can still change (e.g., by removing
          * address pool entries further on in this function). As a consequence, it can happen that the
          * converted instruction is encodable at this point in time, but not after all address pool
          * optimizations have been done because the distance between the two relocatables has been
          * enlarged. */
          if (apreloc
              && RELOC_N_TO_RELOCATABLES(apreloc) > 1)
            continue;

          if ((to_ref)&&(to_intra_chain_bbl))
          {
      /* address producer to basic block in same chain */
            t_int32 range = (ARM_INS_FLAGS(ins) & FL_THUMB)? -1: 0x3ff;
            t_address relval=StackExecConst(RELOC_CODE(apreloc), apreloc, NULL, 0, obj);
            t_int32 offset=G_T_UINT32(AddressSub(relval,ARM_INS_CADDRESS(ins)))-8;
            t_bbl * i_bbl = bbl;

            if (range>0)
              {
                /* we have to make the range smaller whenever there are aligned data blocks in the range
                  because those might lead to padding words being inserted, causing the target to be
                  moved out of the range */
                t_bbl * data = INS_BBL(T_INS(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[0]));

                if (offset>0)
                  {
                    i_bbl = BBL_NEXT_IN_CHAIN(i_bbl);
                    while (i_bbl)
                      {
                        t_address al = BBL_ALIGNMENT(i_bbl);
                        if (al>4)
                          {
                            range -= al - 4;
                          }
                        if (i_bbl==data) break;
                        i_bbl = BBL_NEXT_IN_CHAIN(i_bbl);
                      }
                  }
                if (offset<0)
                  {
                    
                    i_bbl = BBL_PREV_IN_CHAIN(i_bbl);
                    while (i_bbl)
                      {
                              t_address al = BBL_ALIGNMENT(i_bbl);
                        if (al>4)
                          {
                            range -= al - 4;
                          }
                        if (i_bbl==data) break;
                        i_bbl = BBL_PREV_IN_CHAIN(i_bbl);
                      }
                  }
              }

            /* For Thumb instructions, range == -1 */
            if ((0<=offset && offset<range)&&((offset & 0xfffffffc)==offset))
            {
        /* make add rX, pc, offset */
              t_reloc * copy=RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), apreloc);
              t_string_array * sa=StringDivide(RELOC_CODE(apreloc), "\\", TRUE, TRUE);
              Free(RELOC_CODE(copy));

              /* Some magic happens here in the relocation code:
              *  * We calculate the immediate we want to encode in the instruction;
              *  * We shift that immediate right by 2 bits;
              *  * By setting bits 8-11 to 1, the ArmExpandImm pseudo-function (see ARM ARM on modified immediates),
              *    the encoded immediate will be rotated right by 30 bits (2 x 15), which is the same as a left shift
              *    by 2. This results in the original immeidate, which we want to encode.
              */
              RELOC_SET_CODE(copy, StringIo("%sP-s0008-\\s0002>s0f00|lifffff000&|w\\s0000$",sa->first->string));
              ArmInsMakeAdd(ins, ARM_INS_REGA(ins), ARM_REG_R15, ARM_REG_NONE, 0x0, ARM_INS_CONDITION(ins));
              RelocSetFrom(copy, T_RELOCATABLE(ins));
              StringArrayFree(sa);
              ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
            }
            /* there is no sub rX, pc, offset thumb instruction */
            else if ((-range<=offset && offset<0)&&((offset & 0xfffffffc)==offset) &&
                    !(ARM_INS_FLAGS(ins) & FL_THUMB))
            {
        /* make sub rX, pc, offset */
              t_reloc * copy = RelocTableDupReloc(OBJECT_RELOC_TABLE(obj), apreloc);
              t_string_array * sa = StringDivide(RELOC_CODE(apreloc), "\\", TRUE, TRUE);
              Free(RELOC_CODE(copy));

              /* See comment for the "add rX, pc, offset" case above. */
              RELOC_SET_CODE(copy, StringIo("P%s-s0008+\\s0002>s0f00|lifffff000&|w\\s0000$", sa->first->string));
              ArmInsMakeSub(ins, ARM_INS_REGA(ins), ARM_REG_R15, ARM_REG_NONE, 0x0, ARM_INS_CONDITION(ins));
              RelocSetFrom(copy, T_RELOCATABLE(ins));
              StringArrayFree(sa);
              ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
            }
          }
    else if (!to_ref)
          {
      /* constant producer  or FVPFLOAT producer which we cannot optimize*/
            //        if (ARM_INS_OPCODE(ins)==ARM_LDR)
            //        GenerateInstructionsForConstProdIfPossible(ins, ARM_INS_IMMEDIATE(T_ARM_INS(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[0])));
          }
        }
      }
    }
  }


  /* kill superfluous address pool entries */
  CHAIN_FOREACH_BBL (chain, bbl)
  {
    if (BBL_ALIGNMENT(bbl)<=4) /* not to destroy alignment of double data */
    BBL_FOREACH_ARM_INS_SAFE(bbl, ins,tmp)
    {
      if (ARM_INS_ATTRIB(ins) & IF_ADDRESS_POOL_ENTRY)
      {
        if (ARM_INS_REFED_BY(ins)==NULL)
        {
          ArmInsKill(ins);
        }
      }
    }
  }

  AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  /* NEXT  OPTIMIZATION: try to replace 32-bit thumb loads with 16-bit loads */

  CHAIN_FOREACH_BBL (chain, bbl)
    {
      t_uint32 start_address = G_T_UINT32(BBL_CADDRESS(bbl));
      static char * code_thumb_ldr2_up   = "R00Pifffffffc&s0004+-\\=s0002>s00ff&liffffff00&|w\\ifffffc03&$";

      BBL_FOREACH_ARM_INS(bbl, ins)
        {
          t_arm_ins * data1;
          t_arm_ins * data2;
          t_bbl * data_bbl;
          t_uint32 data_address;
          if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
            break;

          if (!(ARM_INS_ATTRIB(ins) & IF_ADDRESS_PRODUCER) || ARM_INS_OPCODE(ins)==ARM_VLDR || ARM_INS_REGA(ins)>=ARM_REG_R8)
            {
              start_address += G_T_UINT32(ARM_INS_CSIZE(ins));
              continue;
            }

          data1=T_ARM_INS(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[0]);
          data_bbl =ARM_INS_BBL(data1);
              
          data_address = G_T_UINT32(BBL_CADDRESS(data_bbl));

          if (data_address<=start_address) {
            start_address += G_T_UINT32(ARM_INS_CSIZE(ins));
            continue;
          }
                    
          BBL_FOREACH_ARM_INS(data_bbl, data2)
            {
              if (data1!=data2)
                data_address+= G_T_UINT32(ARM_INS_CSIZE(data2));
              else break;
            }

          if (data_address>start_address+1000) {
            start_address += G_T_UINT32(ARM_INS_CSIZE(ins));
            continue;
          }

          //          DEBUG(("CONVERTED @I",ins));
          //          DEBUG(("       to @I",data1));
          ARM_INS_SET_CSIZE(ins,AddressNew32(2));
          BBL_SET_CSIZE(bbl, AddressSubUint32(BBL_CSIZE(bbl), 2));

          RelocTableRemoveReloc (OBJECT_RELOC_TABLE (obj),
                                 RELOC_REF_RELOC (ARM_INS_REFERS_TO (ins)));

          RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), 
                                          AddressNullForObject(obj), 
                                          T_RELOCATABLE(ins), 
                                          AddressNullForObject(obj), 
                                          T_RELOCATABLE(data1), 
                                          AddressNullForObject(obj), 
                                          FALSE, NULL, NULL, NULL, 
                                          code_thumb_ldr2_up
                                          );
          //          DEBUG(("into      @I",ins));
          start_address += 2;
        }
    }

  AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);


  /* Optimization 2: eliminate duplicate address pool entries */
  CHAIN_FOREACH_BBL (chain, bbl)
  {
    BBL_FOREACH_ARM_INS(bbl, ins)
    {
      if (ARM_INS_ATTRIB(ins) & IF_ADDRESS_PRODUCER)
      {
        t_arm_ins * data1=T_ARM_INS(RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)))[0]);
	t_arm_ins * data2;
	t_int32 offset;
        t_bool ok=FALSE;
        t_uint32 max_offset;

        if (!ARM_INS_REFERS_TO(data1)) continue; /* skip const producers */


        /* TODO: now FW_LOAD_OFFSET contains an arbitrary margin of ORIG_SLACK_FOR_ALIGNMENT bytes to make up for padding
           bytes that might be inserted. In fact, check needs to be inserted that computes the
           exact margin needed. With the current code, if the arbitrary margin proves insufficient
           Diablo will FATAL during the assembly process because some immediate operand is too large */
	slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;
        max_offset=(ARM_INS_FLAGS(ins) & FL_THUMB) ? FW_LOAD_OFFSET_THUMB : FW_LOAD_OFFSET;
	/* look forward for data instructions that hold the same address but
	 * are not part of a data pool (i.e. original data blocks) */
	for(data2=ArmInsNextInChain(ins), offset=data2?G_T_UINT32(AddressSub(ARM_INS_CADDRESS(data2),ARM_INS_CADDRESS(ins))):max_offset;
	    offset<max_offset;
	    data2=ArmInsNextInChain(data2), offset=data2?G_T_UINT32(AddressSub(ARM_INS_CADDRESS(data2),ARM_INS_CADDRESS(ins))):max_offset)
	{
                t_address al = 0xdead;
	  if (data2!=data1)
	    if (data2 == T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(data2)))) {
              al = BBL_ALIGNMENT(ARM_INS_BBL(data2));
	      if (al>slack_for_alignment)
		{
		  /* we need to make sure that we reduce the search window because alignment might suddenly add more bytes in between blocks */
		  max_offset+=slack_for_alignment;
		  slack_for_alignment = al;
		  max_offset-=slack_for_alignment;
		}
            }
	  
          if (data2!=data1)
             if (ARM_INS_OPCODE(data2)==ARM_DATA) {
                     if (al == 0xdead) al = BBL_ALIGNMENT(ARM_INS_BBL(data2));
               if (al>=4)
                 if (ARM_INS_REFERS_TO(data2))
                   if (!(ARM_INS_ATTRIB(data2) & IF_ADDRESS_POOL_ENTRY))
                     if (!RelocCmp(RELOC_REF_RELOC(ARM_INS_REFERS_TO (data1)),
                                   RELOC_REF_RELOC(ARM_INS_REFERS_TO (data2)), FALSE))
                       if (!(ARM_INS_FLAGS(ins) & FL_THUMB) && // TODO: this seems not ok for thumb2 yet
                           ((G_T_UINT32(AddressSub(ARM_INS_CADDRESS(data2),ARM_INS_CADDRESS(ins))) & 3) == 0))
                         {
                           if (!(ARM_INS_FLAGS (ins) & FL_DIRUP))
                             {
                               /* in this case, the LDR was from a backward found pool first, and now becomes a forward pool entry */
                               ARM_INS_SET_FLAGS(ins,ARM_INS_FLAGS(ins) | FL_DIRUP);
                               RELOC_SET_CODE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)),StringIo("R00P-s0008-\\=s0fff&lifffff000&|w\\ifffff000&$"));
                             }
                           RelocSetToRelocatable(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)),0, T_RELOCATABLE(data2));
                           ok=TRUE;
                           ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);
                           break;
                         }
             }
	}
	if (!ok &&
            !(ARM_INS_FLAGS(ins) & FL_THUMB))
        {
        /* TODO: now BW_LOAD_OFFSET contains an arbitrary margin of ORIG_SLACK_FOR_ALIGNMENT bytes to make up for padding
           bytes that might be inserted. In fact, check needs to be inserted that computes the
           exact margin needed. With the current code, if the arbitrary margin proves insufficient
           Diablo will FATAL during the assembly process because some immediate operand is too large */
	  slack_for_alignment = ORIG_SLACK_FOR_ALIGNMENT;
	  max_offset = BW_LOAD_OFFSET;

	  /* look backward for data (in address pool or otherwise) holding the
	   * same address as the data pool entry */
          for(data2=ArmInsPrevInChain(ins), offset=data2?G_T_UINT32(AddressSub(ARM_INS_CADDRESS(ins),ARM_INS_CADDRESS(data2))):max_offset;
              offset<max_offset;
              data2=ArmInsPrevInChain(data2), offset=data2?G_T_UINT32(AddressSub(ARM_INS_CADDRESS(ins),ARM_INS_CADDRESS(data2))):max_offset)
          {
	    if (data2!=data1)
	      if (data2 == T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(data2)))) {
                      t_address al = BBL_ALIGNMENT(ARM_INS_BBL(data2));
		if (al>slack_for_alignment)
		  {
		    /* we need to make sure that we reduce the search window because alignment might suddenly add more bytes in between blocks */
		    max_offset+=slack_for_alignment;
		    slack_for_alignment = al;
		    max_offset-=slack_for_alignment;
		  }
                }

            if (data2!=data1)
              if (ARM_INS_OPCODE(data2)==ARM_DATA)
                if (ARM_INS_REFERS_TO(data2))
                  if (!RelocCmp(RELOC_REF_RELOC(ARM_INS_REFERS_TO (data1)),
                        RELOC_REF_RELOC(ARM_INS_REFERS_TO (data2)), FALSE))
                    /* thumb ldr is always preindex */
		    if (!(ARM_INS_FLAGS(ins) & FL_THUMB)) // TODO: improve for thumb2
		    {
                      Free(RELOC_CODE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins))));
		      RELOC_SET_CODE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)), StringDup("Ps0008+R00-\\=s0fff&lifffff000&|w\\ifffff000&$"));

		      RelocSetToRelocatable(RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)),0, T_RELOCATABLE(data2));
		      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_DIRUP);
		    }
          }
        }
      }
    }
  }

  /* remove superfluous address pool entries */
  CHAIN_FOREACH_BBL (chain, bbl)
  {
    if (BBL_ALIGNMENT(bbl)<=4) /* not to destroy alignment of double data */
    BBL_FOREACH_ARM_INS_SAFE(bbl, ins,tmp)
    {
      if (ARM_INS_ATTRIB(ins) & IF_ADDRESS_POOL_ENTRY)
      {
        if (ARM_INS_REFED_BY(ins)==NULL)
        {
          ArmInsKill(ins);
        }
      }
    }
  }

  if (do_ldr_opt)
  {
    /* optimization 3:
     * ADR r1, xxx
     * LDR r2, [r1]
     * =>
     * ADD r1, pc, (xxx & 0xff000)
     * LDR r2, [r1, (xxx & 0x00fff)]!
     */
    AssignAddressesInChain (chain, SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
    ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

    CHAIN_FOREACH_BBL (chain, bbl)
    {
      BBL_FOREACH_ARM_INS (bbl, ins)
      {
        t_arm_ins *mem, *iter;
        t_reg reg;
        t_arm_ins *poolentry;
        t_uint32 i;
        t_reloc *rel, *memrel, *addrel;
        t_address dest, diff;
        t_bool negative;
        t_string_array *split;
        t_string program;
        char relnum[20];


        if (ARM_INS_FLAGS(ins) & FL_THUMB) 
          break;


        if (!(ARM_INS_ATTRIB (ins) & IF_ADDRESS_PRODUCER))
          continue;

        /* thumb ldr doesn't support writeback */

        mem = NULL;
        reg = ARM_INS_REGA (ins);
        for (iter = ARM_INS_INEXT (ins); iter; iter = ARM_INS_INEXT (iter))
        {
          if ((ARM_INS_OPCODE (iter) == ARM_LDR || ARM_INS_OPCODE (iter) == ARM_STR)
            && (ARM_INS_REGB (iter) == reg))
          {
            mem = iter;
            break;
          }

          /* if reg is redefined, stop looking */
          if (RegsetIn (ARM_INS_REGS_DEF (iter), reg))
            break;

          /* if reg is used in something else than a memory access,
           * the optimization cannot be performed */
          if (RegsetIn (ARM_INS_REGS_USE (iter), reg))
            break;
        }

        if (!mem) continue;

        /* cannot do this for instructions like STR r3, [r3] */
        if (ARM_INS_OPCODE (mem) == ARM_STR && ARM_INS_REGA (mem) == reg)
          continue;

        /* conditional instructions are difficult: you'd have to prove
         * that the address producer and the memory operation correspond in
         * all possible cases. Doable, but too much work. */
        if (ArmInsIsConditional (ins) || ArmInsIsConditional (mem)) continue;

        if (ARM_INS_REGC (mem) != ARM_REG_NONE) continue;
        if (!(ARM_INS_FLAGS (mem) & FL_PREINDEX)) continue;
        if (ArmInsWriteBackHappens (mem)) continue;
        if (ARM_INS_IMMEDIATE (mem) != 0)
        {
          /* TODO look at this: if reg is dead afterward, this
           * should not be a problem */
          VERBOSE (10, ("nonzero"));
          continue;
        }

        /* mem is of form ldr xxx, [reg, #0] */
        poolentry = T_ARM_INS (
          RELOC_TO_RELOCATABLE (RELOC_REF_RELOC (ARM_INS_REFERS_TO (ins)))[0]);
        if (!ARM_INS_REFERS_TO (poolentry))
        {
          /* happens for e.g. memory-mapped I/O in the Linux kernel: these
           * addresses are fixed and not relocatable */
          continue;
        }

        rel  = RELOC_REF_RELOC (ARM_INS_REFERS_TO (poolentry));
        dest = StackExecConst (RELOC_CODE (rel), rel, NULL, 0, obj);
        diff = AddressSubUint32 (AddressSub (dest, ARM_INS_CADDRESS (ins)), 8);
        negative = (((t_int32)(G_T_UINT32 (diff))) < 0);
        if (negative)
          diff = AddressSub (AddressNew32 (0), diff);

        /* TODO: also here the maximal offset allowed should be corrected for potential
           padding bytes being inserted later on With the current code, if the arbitrary margin proves insufficient
           Diablo will FATAL during the assembly process because some immediate operand is too large */


        /* check if the offset can be encoded in 20 bits - padding that can 
           be introduced as part of RELRO section generation (see diabloelf_arm.c) 
           and as part of alignment of RW PT_LOAD SEGMENT see ELF-ARM-BINUTILS.ld 
        */
        if (AddressIsGe (diff, AddressNew32 (0x100000-0x8000-0x1000)))
        {
          continue;
        }

        VERBOSE (10, ("PAIR%c\n@I\n@I", negative?'-':'+',ins, mem));
        VERBOSE (10, ("points to @R", rel));
        RelocTableRemoveReloc (OBJECT_RELOC_TABLE (obj),
          RELOC_REF_RELOC (ARM_INS_REFERS_TO (ins)));

        ARM_INS_SET_IMMEDIATE (mem, G_T_UINT32 (diff) & 0xfff);
        if (negative)
          ARM_INS_SET_FLAGS (mem, ARM_INS_FLAGS (mem) & ~FL_DIRUP);
        else
          ARM_INS_SET_FLAGS (mem, ARM_INS_FLAGS (mem) | FL_DIRUP);
        if (ARM_INS_REGA (mem) != reg)
          ARM_INS_SET_FLAGS (mem, ARM_INS_FLAGS (mem) | FL_WRITEBACK);

        if (!negative)
          ArmInsMakeAdd(ins, ARM_INS_REGA(ins), ARM_REG_R15, ARM_REG_NONE,
            0x0, ARM_INS_CONDITION(ins));
        else
          ArmInsMakeSub(ins, ARM_INS_REGA(ins), ARM_REG_R15, ARM_REG_NONE,
            0x0, ARM_INS_CONDITION(ins));

        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) & ~IF_ADDRESS_PRODUCER);

#define encode_imm_operand "i000ff000 &\\ s000c > s0a00 | l ifffff000 & | w\\s0000$"
        split = StringDivide (RELOC_CODE (rel), "\\", FALSE, FALSE);

        if (!negative)
          program =
            StringConcat2 (split->first->string, "P-s0008-" encode_imm_operand);
        else
          program =
            StringConcat2 (split->first->string, "P-s0008-s0000%-" encode_imm_operand);
#undef encode_imm_operand

        addrel = RelocTableAddRelocToRelocatable (
          OBJECT_RELOC_TABLE (obj), RELOC_ADDENDS (rel)[0], T_RELOCATABLE (ins),
          AddressNew32 (0), RELOC_TO_RELOCATABLE (rel)[0],
          RELOC_TO_RELOCATABLE_OFFSET (rel) [0], FALSE,
          NULL, NULL, NULL, program);
        Free (program);

        for (i = 1; i < RELOC_N_ADDENDS (rel); ++i)
          RelocAddAddend (addrel, RELOC_ADDENDS (rel)[i]);
        for (i = 1; i < RELOC_N_TO_RELOCATABLES (rel); ++i)
          RelocAddRelocatable (addrel,
            RELOC_TO_RELOCATABLE (rel)[i],
            RELOC_TO_RELOCATABLE_OFFSET (rel)[i]);

        sprintf (relnum, "R%02d", RELOC_N_TO_RELOCATABLES (rel));

        if (!negative)
          program =
            StringConcat3 (split->first->string, relnum, "-s0008-s0fff&\\lifffff000&|w\\s0000$");
        else
          program =
            StringConcat3 (split->first->string, relnum, "-s0008-s0000%-s0fff&\\lifffff000&|w\\s0000$");

        memrel = RelocTableAddRelocToRelocatable (
          OBJECT_RELOC_TABLE (obj), RELOC_ADDENDS (rel)[0], T_RELOCATABLE (mem),
          AddressNew32 (0), RELOC_TO_RELOCATABLE (rel)[0],
          RELOC_TO_RELOCATABLE_OFFSET (rel) [0], FALSE,
          NULL, NULL, NULL, program);

        for (i = 1; i < RELOC_N_ADDENDS (rel); ++i)
          RelocAddAddend (memrel, RELOC_ADDENDS (rel)[i]);
        for (i = 1; i < RELOC_N_TO_RELOCATABLES (rel); ++i)
          RelocAddRelocatable (memrel,
            RELOC_TO_RELOCATABLE (rel)[i],
            RELOC_TO_RELOCATABLE_OFFSET (rel)[i]);

        RelocAddRelocatable (memrel, T_RELOCATABLE (ins), AddressNew32 (0));

        Free (program);
        StringArrayFree (split);

        VERBOSE (10, ("after\n@I\n@I", ins, mem));
      }
    }

    /* remove superfluous address pool entries */
    CHAIN_FOREACH_BBL (chain, bbl)
    {
      if (BBL_ALIGNMENT(bbl)<=4) /* not to destroy alignment of double data */
      BBL_FOREACH_ARM_INS_SAFE(bbl, ins,tmp)
      {
	      if (ARM_INS_ATTRIB(ins) & IF_ADDRESS_POOL_ENTRY)
	      {
	        if (ARM_INS_REFED_BY(ins)==NULL)
	        {
	          ArmInsKill(ins);
	        }
	      }
      }
    }
  }

  AssignAddressesInChain (chain,SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(BBL_CFG(chain)))[0]));
  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  STATUS(STOP,("Optimizing LDRs of const/address producers"));
}

void
SimpleAddressProducersForChain (t_bbl *chain, t_bool optimize)
{
  RealAddressProducersForChain(chain, optimize, TRUE);
}

void
OptimizeAddressProducersForChain (t_bbl *chain, t_bool optimize)
{
  RealOptimizeAddressProducersForChain(chain, optimize, TRUE);
}

/* added for kcompress: here data sections can move even after the address producers
 * have been generated, so the add+ldr optimization is unsafe. Hence, we don't use it.
 */
void SimpleAddressProducersForChainNoLdrOpt(t_bbl *chain, t_bool optimize)
{
  RealAddressProducersForChain(chain, optimize, FALSE);
}

void OptimizeAddressProducersForChainNoLdrOpt(t_bbl *chain, t_bool optimize)
{
  RealOptimizeAddressProducersForChain(chain, optimize, FALSE);
}

/* vim: set shiftwidth=2 foldmethod=marker : */
