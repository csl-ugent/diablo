/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

extern "C" {
#include <diabloarm.h>
}

#include <diablosoftvm.h>
#include <diabloannotations.h>
#include "diablosoftvm_vmchunk.h"

extern int frontend_id;

using namespace std;

/* Constructor & Destructor {{{ */
t_vmchunk *
VmChunkNew()
{
  t_vmchunk *chunk = (t_vmchunk *) Calloc(1, sizeof(t_vmchunk));
  t_ptr_array *bbls = reinterpret_cast<t_ptr_array *>(Malloc(sizeof(t_ptr_array)));
  t_ptr_array *exits = reinterpret_cast<t_ptr_array *>(Malloc(sizeof(t_ptr_array)));
  t_ins_to_reloc_map *reloc_map = new t_ins_to_reloc_map();
  t_edge_to_edge_map *edge_map = new t_edge_to_edge_map();

  /* init members */
  PtrArrayInit(bbls, FALSE);
  PtrArrayInit(exits, TRUE);

  VMCHUNK_SET_BBLS(chunk, bbls);
  VMCHUNK_SET_EXITS(chunk, exits);
  VMCHUNK_SET_RELOC_MAP(chunk, reloc_map);
  VMCHUNK_SET_EXIT_EDGE_MAP(chunk, reinterpret_cast<void *>(edge_map));
  VMCHUNK_SET_CFG(chunk, NULL);
  VMCHUNK_SET_IP(chunk, FALSE);
  VMCHUNK_SET_INTEGRATED(chunk, FALSE);
  VMCHUNK_SET_MOBILE_ID(chunk, -1);

  return chunk;
}

void
VmChunkFree(t_vmchunk *chunk)
{
  PtrArrayFini(VMCHUNK_BBLS(chunk), FALSE);
  PtrArrayFini(VMCHUNK_EXITS(chunk), FALSE);

  Free((t_ptr_array *)VMCHUNK_BBLS(chunk));
  Free((t_ptr_array *)VMCHUNK_EXITS(chunk));

  if (VMCHUNK_CFG(chunk) != NULL)
  {
    //ObjectFree(CFG_OBJECT(VMCHUNK_CFG(chunk)));
    //CfgFree(VMCHUNK_CFG(chunk));
    CfgFiniRegions(CFG_OBJECT(VMCHUNK_CFG(chunk)));
    BblFiniRegions(VMCHUNK_CFG(chunk));
  }

  t_edge_to_edge_map *e2a_map = reinterpret_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));
  //for (auto it = e2a_map->begin(); it != e2a_map->end(); it++)
  //  CfgEdgeKill(it->first);
  //e2a_map->clear();
  delete e2a_map;

  t_ins_to_reloc_map *i2r_map = reinterpret_cast<t_ins_to_reloc_map *>(VMCHUNK_RELOC_MAP(chunk));
  delete i2r_map;

  Free(chunk);
}
/* }}} */

static t_bbl *BblCopyToCfg(t_vmchunk *chunk, t_bbl *bbl, t_cfg *cfg)
{
  t_arm_ins *ins, *copy;
  t_bbl *newbbl = BblNew(cfg);
  t_ins_to_reloc_map *i2r_map = static_cast<t_ins_to_reloc_map *>(VMCHUNK_RELOC_MAP(chunk));

  BBL_SET_CADDRESS(newbbl, BBL_OLD_ADDRESS(bbl));
  BBL_SET_OLD_ADDRESS(newbbl, G_T_UINT32(AddressSub(BBL_CADDRESS(bbl), BBL_CADDRESS(FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl))))));

  BBL_FOREACH_ARM_INS(bbl,ins)
  {
    copy = ArmInsDup(ins);

    /* copy over instruction properties */
    ARM_INS_SET_OLD_ADDRESS(copy, ARM_INS_OLD_ADDRESS(ins));
    ARM_INS_SET_CFG(copy, cfg);

    ArmInsAppendToBbl(copy,newbbl);

    if (frontend_id == 4)
    {
      /* When the original instruction was copied, the targets (to-relocatables) associated with this instruction
       * were not copied into the new CFG. As this is not necessary (for now), just remove all references in the
       * target to the new instruction. */
      /* keep a list of all relocatables refered to by this instruction */
      vector<t_reloc *> reloc_vector;
      for (t_reloc_ref *rref = ARM_INS_REFERS_TO(copy); rref; rref = RELOC_REF_NEXT(rref))
      {
        t_reloc *rel = RELOC_REF_RELOC(rref);

        /* sanity check: we can't possibly know where the resulting bytecode will be executed in memory.
         * Thus, PC-relative relocations are not supported! */
        ASSERT(!StringPatternMatch("*P*", RELOC_CODE(rel)), ("no support for translation of PC-relative relocations! @I (@R)", copy, rel));

        /* add the relocation to the list to be kept */
        reloc_vector.push_back(rel);
      }

      /* need to associate 'copy' with the vector of relocations here because the original instruction
       * will be killed by dead code removal */
      if (reloc_vector.size() > 0)
        i2r_map->insert(t_ins_to_reloc_map_entry(copy, reloc_vector));
    }
    else
      while (ARM_INS_REFERS_TO(copy))
        /* remove all relocations from the copied instruction to the original CFG */
        RelocatableRemoveAllRefersTo(T_RELOCATABLE(copy));
  }

  /* copy over liveness information */
  BBL_SET_REGS_LIVE_OUT(newbbl, BBL_REGS_LIVE_OUT(bbl));

  return newbbl;
}

/* Recursively copy the chunk CFG to a new CFG */
static t_bbl *
VmChunkCopyBblToCfg(t_vmchunk *chunk, t_bbl *bbl, t_cfg *new_cfg, t_function *to_function, t_bbl *bbl_copy, t_function *original_function)
{
  if (BblIsMarked(bbl))
  {
    /* the BBL has already been copied, just return the copied instance */

    t_bbl *bbl_it;
    bbl_copy = NULL;

    CFG_FOREACH_BBL(VMCHUNK_CFG(chunk), bbl_it)
      if (BBL_CADDRESS(bbl_it) == BBL_OLD_ADDRESS(bbl))
      {
        bbl_copy = bbl_it;
        break;
      }

    ASSERT(bbl_copy, ("somthing went wrong: BBL @iB was marked as copied, but could not find it in the chunk CFG", bbl));
  }
  else
  {
    /* the BBL has not been copied yet, copy it! */
    if (bbl_copy == NULL)
    {
      /* copy over the BBL to the new CFG */
      bbl_copy = BblCopyToCfg(chunk, bbl, new_cfg);

      ASSERT(BBL_FUNCTION(bbl) == original_function, ("Oh no, interprocedural SoftVM transformation stuff is not tested yet! @F <-> @F", BBL_FUNCTION(bbl), original_function));
      BblInsertInFunction(bbl_copy, to_function);
      BblMark(bbl);
    }

    t_edge_to_edge_map *e2a_map = reinterpret_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));

    /* walk over every successor edge of the copied bbl */
    t_cfg_edge *edge;
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
    {
      t_bbl *bbl_it = CFG_EDGE_TAIL(edge);

      int bbl_index = PtrArrayFind(VMCHUNK_BBLS(chunk), reinterpret_cast<void *>(bbl_it));

      /* is this an exit edge or not? */
      if (bbl_index < 0)
      {
        /* yes, this is an exit edge */

        /* sanity check on the edge types we actually support */
        ASSERT(CFG_EDGE_CAT(edge) != ET_FALLTHROUGH, ("did not expect fallthrough edge to be an exit edge @E!", edge));
        ASSERT(CFG_EDGE_CAT(edge) == ET_JUMP || CFG_EDGE_CAT(edge) == ET_IPJUMP, ("did not expect this type of exit edge @E!", edge));

        /* first recreate the equivalent edge, going to HELL */
        t_cfg_edge *edge_copy = CfgEdgeCreate(new_cfg, bbl_copy, CFG_HELL_NODE(new_cfg), ET_IPJUMP);

        /* then save the necessary information about the original edge */
        e2a_map->insert(t_edge_to_edge_map_entry(edge_copy, edge));

        /* also add it to the EXIT_EDGES member array */
        PtrArrayAdd(VMCHUNK_EXITS(chunk), edge);
      }
      else
      {
        /* Not an exit edge: recreate it in the new CFG */
        ASSERT(!CfgEdgeIsInterproc(edge), ("can't have IP edges in chunks for now @E!", edge));

        /* first copy over the tail of the edge */
        t_bbl *next_bbl = reinterpret_cast<t_bbl *>(PtrArrayGet(VMCHUNK_BBLS(chunk), bbl_index));
        t_bbl *next_bbl_copy = VmChunkCopyBblToCfg(chunk, next_bbl, new_cfg, to_function, NULL, original_function);

        /* then create a new edge in the new CFG */
        CfgEdgeCreate(new_cfg, bbl_copy, next_bbl_copy, CFG_EDGE_CAT(edge));
      }
    }
  }

  return bbl_copy;
}

/*
 * Calculate some thing when the chunk is complete. For example, we can only
 * correctly get the exiting edges after all bbl's were added.
 */
void
VmChunkRefresh(t_vmchunk *chunk)
{
  t_bbl *first_bbl = reinterpret_cast<t_bbl *>(PtrArrayGet(VMCHUNK_BBLS(chunk), 0));

  /* create a new object for this chunk */
  t_object *orig_object = CFG_OBJECT(BBL_CFG(first_bbl));
  t_object *new_object = ObjectNew();
  OBJECT_SET_OBJECT_HANDLER(new_object, OBJECT_OBJECT_HANDLER(orig_object));
  OBJECT_SET_ADDRESS_SIZE(new_object, OBJECT_ADDRESS_SIZE(orig_object));
  OBJECT_SET_SWITCHED_ENDIAN(new_object, OBJECT_SWITCHED_ENDIAN(orig_object));

  /* create a new CFG for this chunk */
  t_cfg *orig_cfg = BBL_CFG(first_bbl);
  t_cfg *new_cfg  = CfgCreate(new_object);
  OBJECT_SET_CFG(new_object, new_cfg);
  VMCHUNK_SET_CFG(chunk, new_cfg);

  CfgInitRegions(CFG_OBJECT(new_cfg));
  BblInitRegions(new_cfg);

  /* copy the first bbl in the chunk array */
  t_bbl *first_bbl_copy = BblCopyToCfg(chunk, first_bbl, new_cfg);
  CfgCreateHellNodesAndEntry(new_object, new_cfg, AddressNewForCfg(new_cfg, -1), first_bbl_copy);

  /* create a new function in the newly created CFG */
  t_function *new_function = FunctionMake(first_bbl_copy, FUNCTION_NAME(BBL_FUNCTION(first_bbl)), FT_NORMAL);

  /* copy the chunked part of the CFG to the newly created CFG instance */
  BblMarkInit();
  VmChunkCopyBblToCfg(chunk, first_bbl, new_cfg, new_function, first_bbl_copy, BBL_FUNCTION(first_bbl));

  /* Sanity checks */
  t_bbl *bbl_it;
  CFG_FOREACH_BBL(VMCHUNK_CFG(chunk), bbl_it)
  {
    t_cfg_edge *e;

    if (BBL_IS_HELL(bbl_it)) continue;
    if (BblIsExitBlock(bbl_it)) continue;

    /* predecessor edge checks */
    BBL_FOREACH_PRED_EDGE(bbl_it, e)
    {
      if (CFG_EDGE_HEAD(e) != CFG_UNIQUE_ENTRY_NODE(new_cfg))
        ASSERT(!CfgEdgeIsInterproc(e), ("Can't have IP edges in chunk: @E in @eiB", e, bbl_it));
    }

    /* successor edge checks */
    BBL_FOREACH_SUCC_EDGE(bbl_it, e)
    {
      if (!BBL_IS_HELL(CFG_EDGE_TAIL(e)))
        ASSERT(!CfgEdgeIsInterproc(e), ("Can't have IP edges in chunk: @E in @eiB", e, bbl_it));
    }

    /* BBL sanity */
    ASSERT(BBL_SUCC_FIRST(bbl_it) != NULL, ("BBL @eiB should have at least one outgoing edge", bbl_it));
  }

  CfgVerifyCorrectness(VMCHUNK_CFG(chunk));
}

/*
 * Small wrapper arround the ptr_array add, might be useful later to add extra
 * actions on bbl addition to a chunk or when replacing the array datastructure.
 */
void
VmChunkAddBbl(t_vmchunk *chunk, t_bbl *bbl)
{
    PtrArrayAdd(VMCHUNK_BBLS(chunk), bbl);
}

/*
 * Small wrapper arround the ptr_array find, might be useful later to add extra
 * actions on bbl addition to a chunk or when replacing the array datastructure.
 */
int
VmChunkHasBbl(t_vmchunk *chunk, t_bbl *bbl)
{
    return PtrArrayFind(VMCHUNK_BBLS(chunk), bbl);
}

void
VmChunkPrint(t_vmchunk *chunk)
{
  int i;
  for (i=0; i<PtrArrayCount(VMCHUNK_BBLS(chunk)); i++)
  {
    t_bbl *bbl = reinterpret_cast<t_bbl *>(PtrArrayGet(VMCHUNK_BBLS(chunk), i));

    int nsedges = 0;
    t_cfg_edge * e;
    BBL_FOREACH_SUCC_EDGE(bbl, e)
      nsedges++;
    int npedges = 0;
    BBL_FOREACH_PRED_EDGE(bbl, e)
      npedges++;

    DEBUG(("CHUNK BBL %d: @eiB\n   %s num edges %d in, %d out", i, bbl,(i==0)?"entry":"",npedges,nsedges));
  }
}

int
VmChunkCountInstructions(t_vmchunk *chunk)
{
  int ret = 0;

  for (int i = 0; i < PtrArrayCount(VMCHUNK_BBLS(chunk)); i++)
  {
    t_bbl *bbl = reinterpret_cast<t_bbl *>(PtrArrayGet(VMCHUNK_BBLS(chunk), i));

    ret += BBL_NINS(bbl);
  }

  return ret;
}
