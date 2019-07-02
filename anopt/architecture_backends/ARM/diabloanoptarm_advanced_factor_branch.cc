#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

//#define DISABLE

//#define DEBUG_FLIP
#define DEBUG_FLIP_DEBUGCOUNTER diablosupport_options.debugcounter

typedef std::map<t_cfg_edge *, DummyEdgeData> DummyEdgeDataMap;

static DummyEdgeDataMap dummy_edge_map;
static NewTargetSelector *selector = nullptr;

void GlobalRedirect(t_cfg_edge *e, DummyEdgeData data) {
#ifdef DISABLE
  if (data.dispatch_type == DummyEdgeData::DispatchType::SwitchOffset)
    return;

  CfgEdgeChangeTail(e, CFG_EDGE_HEAD(e));
#else
  if (selector == nullptr)
    selector = dynamic_cast<NewTargetSelector*>(GetTransformationsForType("newtargetselector:globalpostpass").at(0));

  /* need to find a destination */
  t_bbl *destination = selector->doTransform(CFG_EDGE_HEAD(e), af_rng_redirect);

  /* redirect relocation, if any.
   * Here we assume that the first to-relocatable needs to be redirected. */
  t_reloc *reloc = CFG_EDGE_REL(e);
  if (reloc) {
    ASSERT(AddressIsNull(RELOC_TO_RELOCATABLE_OFFSET(reloc)[0]), ("to offset 0 not null (@G)! @R", RELOC_TO_RELOCATABLE_OFFSET(reloc)[0], reloc));
    RelocSetToRelocatable(reloc, 0, T_RELOCATABLE(destination));
  }

  /* change TAIL of edge */
  CfgEdgeChangeTail(e, destination);
#endif
}

t_bbl *GetGlobalRedirectRelocTo(t_cfg *cfg) {
#ifdef DISABLE
  if (selector == nullptr)
    selector = dynamic_cast<NewTargetSelector*>(GetTransformationsForType("newtargetselector:infiniteloop").at(0));

  /* need to select a random parent BBL */
  t_bbl *bbl = NULL;
  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    bbl = FUNCTION_BBL_FIRST(fun);
    break;
  }

  ASSERT(bbl, ("invalid BBL!"));
  return selector->doTransform(bbl, af_redirect);
#else
  if (selector == nullptr)
    selector = dynamic_cast<NewTargetSelector*>(GetTransformationsForType("newtargetselector:globalpostpass").at(0));

  return selector->doTransform(cfg, af_rng_redirect);
#endif
}

t_bool CanModifyJumpEdgeAF(t_cfg_edge *e) {
	t_bbl *tail = CFG_EDGE_TAIL(e);
	t_function *tail_function = BBL_FUNCTION(tail);

	/* only do destinations in AF functions */
	if (!tail_function
			|| !FunctionIsAF(tail_function))
		return true;

	/* add-ldr */
	t_arm_ins *last_ft = NULL;
	t_cfg_edge *ft_edge;
	BBL_FOREACH_PRED_EDGE(tail, ft_edge)
		if (CFG_EDGE_CAT(ft_edge) == ET_FALLTHROUGH) {
			last_ft = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(ft_edge)));
			break;
		}

	t_arm_ins *first = T_ARM_INS(BBL_INS_FIRST(tail));
	if (first
			&& (ARM_INS_ATTRIB(first) & IF_SWITCHJUMP)
			&& ARM_INS_OPCODE(first) == ARM_ADD
			&& last_ft
			&& ARM_INS_OPCODE(last_ft) == ARM_LDR
			&& ARM_INS_REGB(last_ft) == ARM_REG_R15)
		return false;

	return true;
}

static
bool FlipBranchesCheckFallthroughLoop(t_bbl *from, vector<t_cfg_edge *>& real_ft_vector, vector<t_cfg_edge *>& fake_ft_vector)
{
	/* if the FROM is marked, this is a fallthrough loop! */
	if (BblIsMarked(from))
		return true;

	/* mark this BBL as visited */
	BblMark(from);

	/* lookup the fallthrough edge, if any */
	t_cfg_edge *ft_edge = ArmGetFallThroughEdge(from);
	if (!ft_edge) {
		/* maybe an outgoing CALL edge? */
		t_cfg_edge *e;
		BBL_FOREACH_SUCC_EDGE(from, e)
			if (CFG_EDGE_CAT(e) == ET_CALL
					&& CFG_EDGE_CORR(e)) {
				ft_edge = CFG_EDGE_CORR(e);
				break;
			}
	}

	auto HasOutgoingCallEdge = [] (t_bbl *bbl) {
		t_cfg_edge *e;
		BBL_FOREACH_SUCC_EDGE(bbl, e)
			if (CFG_EDGE_CAT(e) == ET_CALL)
				return true;

		return false;
	};

	/* if no fallthrough destination is found, no loop is possible */
	if (!ft_edge)
		return false;

	if (!(CFG_EDGE_CAT(ft_edge) == ET_RETURN
				|| (CFG_EDGE_CAT(ft_edge) == ET_FALLTHROUGH
						&& FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_HEAD(ft_edge))))
				|| (CFG_EDGE_CAT(ft_edge) == ET_FALLTHROUGH
						&& HasOutgoingCallEdge(from)))) {
		if (CfgEdgeIsFake(ft_edge))
			fake_ft_vector.push_back(ft_edge);
		else
			real_ft_vector.push_back(ft_edge);
	}

	return FlipBranchesCheckFallthroughLoop(CFG_EDGE_TAIL(ft_edge), real_ft_vector, fake_ft_vector);
}

void ArmFlipBranchesFakeEdges(t_cfg *cfg)
{
  if (diabloanoptarm_options.af_fake_fallthrough_condbranch_chance == 0)
    return;

  t_randomnumbergenerator *rng = RNGCreateChild(RNGGetRootGenerator(), "fakeflip");

	t_uint32 nr_transformations = 0;
	STATUS(START, ("Branch Flipping, fake edge specialisation"));

  /* keep a list of flipped branches */
  BblVector flipped;

	t_bbl *bbl;
	CFG_FOREACH_BBL(cfg, bbl)
	{
#ifdef DEBUG_FLIP
		if (DEBUG_FLIP_DEBUGCOUNTER == nr_transformations)
			break;
#endif

		if (CanModifyBranchConditional(bbl)) {
			/* flipping conditional branch */
			t_cfg_edge *jump_edge;
			t_cfg_edge *ft_edge;
			if (!CanFlipBranch(bbl, &ft_edge, &jump_edge))
				continue;

			/* both edges are (not) fake */
			if (!(CfgEdgeIsFake(jump_edge) ^ CfgEdgeIsFake(ft_edge)))
				continue;

			/* fallthrough edge is fake */
			if (CfgEdgeIsFake(ft_edge)) {
				CfgDrawFunctionGraphsWithHotness(cfg, "fake");
				FATAL(("FT is already fake! @E @eiB @eiB", ft_edge, CFG_EDGE_HEAD(ft_edge), CFG_EDGE_TAIL(ft_edge)));
			}

			if (RNGGeneratePercent(rng) <= static_cast<t_uint32>(diabloanoptarm_options.af_fake_fallthrough_condbranch_chance)) {
				/* jump edge is fake, fallthrough edge is not.
				 * Do a swap, but don't take into account multiple incoming FT edges for now. */

	#ifdef DEBUG_FLIP
			if (DEBUG_FLIP_DEBUGCOUNTER == nr_transformations+1)
				CfgDrawFunctionGraphs(cfg, "flip-before");
	#endif

				DoFlipBranch(bbl, ft_edge, jump_edge);
				flipped.push_back(bbl);

	#ifdef DEBUG_FLIP
			if (DEBUG_FLIP_DEBUGCOUNTER == nr_transformations+1)
				CfgDrawFunctionGraphs(cfg, "flip-after");
	#endif

				nr_transformations++;
			}
		}
		else if (CanModifyBranch(bbl)) {
			/* modify unconditional branch */
			t_cfg_edge *e = BBL_SUCC_FIRST(bbl);
			if (!CfgEdgeIsFake(e))
				continue;

			if (!CanModifyJumpEdge(e))
				continue;

			bool switch_related = false;
			t_cfg_edge *ee;
			BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(BBL_PRED_FIRST(bbl)), ee) {
				if (CFG_EDGE_CAT(ee) == ET_SWITCH
						|| CFG_EDGE_CAT(ee) == ET_IPSWITCH) {
					switch_related = true;
					break;
				}
			}
			if (BBL_NINS(bbl) == 1
					&& switch_related) {
				/* this BBL contains only a branch instruction AND is part of a switch construction,
				 * let's assume that this is the case of a branch-based switch table */
				continue;
			}

			if (RNGGeneratePercent(rng) <= static_cast<t_uint32>(diabloanoptarm_options.af_fake_fallthrough_uncondbranch_chance)) {
	#ifdef DEBUG_FLIP
			if (DEBUG_FLIP_DEBUGCOUNTER == nr_transformations+1)
				CfgDrawFunctionGraphs(cfg, "flip-before");
	#endif

				DoUncondBranchToFallthrough(bbl);
				flipped.push_back(bbl);

	#ifdef DEBUG_FLIP
			if (DEBUG_FLIP_DEBUGCOUNTER == nr_transformations+1)
				CfgDrawFunctionGraphs(cfg, "flip-after");
	#endif

				nr_transformations++;
			}
		}
	}

  /* check fallthrough loops */
  for (auto bbl : flipped) {
    BblMarkInit();
    vector<t_cfg_edge *> real_ft_edges;
    vector<t_cfg_edge *> fake_ft_edges;
    bool ft_loop = FlipBranchesCheckFallthroughLoop(bbl, real_ft_edges, fake_ft_edges);

    if (ft_loop) {
			/* pick a suitable list of options, at least one of them should be non-empty.
			 * We prefer to split a REAL edge to a FAKE edge. */
			auto options = real_ft_edges;
			bool fake = false;
			if (options.size() == 0) {
				options = fake_ft_edges;
				fake = true;
			}

			ASSERT(options.size() > 0, ("what? @eiB", bbl));

			/* pick a random edge */
			auto it = options.begin();
  		advance(it, RNGGenerateWithRange(rng, 0, options.size() - 1));
			t_cfg_edge *chosen = *it;

			t_bbl *head = CFG_EDGE_HEAD(chosen);
			t_bbl *tail = CFG_EDGE_TAIL(chosen);
			bool was_fake = CfgEdgeIsFake(chosen);

			if (BBL_IS_HELL(head)) {
				CfgDrawFunctionGraphs(BBL_CFG(head), "boem");
				FATAL(("boem! @eiB -> @eiB, @eiB", head, tail, bbl));
			}

			/* create a new block and add an unconditional branch to it */
			t_bbl *new_bbl = BblNew(cfg);
			BBL_SET_EXEC_COUNT(new_bbl, 0);
			if (BBL_FUNCTION(head))
				BblInsertInFunction(new_bbl, BBL_FUNCTION(head));
			BblCopyExecInformationFromEdge(chosen, new_bbl);

			t_arm_ins *branch_ins;
			ArmMakeInsForBbl(UncondBranch, Append, branch_ins, new_bbl, FALSE);

			/* kill the existing edge */
			if (CFG_EDGE_CORR(chosen))
				CfgEdgeKill(CFG_EDGE_CORR(chosen));
			CfgEdgeKill(chosen);

			/* create new edges */
			t_cfg_edge *ft_edge = CfgEdgeCreate(cfg, head, new_bbl, ET_FALLTHROUGH);
			if (was_fake)
				CfgEdgeMarkFake(ft_edge);
			EdgeMakeInterprocedural(ft_edge);
			BblCopyExecInformationToEdge(new_bbl, ft_edge);

			t_cfg_edge *jp_edge = CfgEdgeCreate(cfg, new_bbl, tail, ET_JUMP);
			if (was_fake)
				CfgEdgeMarkFake(jp_edge);
			EdgeMakeInterprocedural(jp_edge);
			BblCopyExecInformationToEdge(new_bbl, jp_edge);
    }
  }

	/* handle BBLs with multiple incoming fallthrough edges */
	CFG_FOREACH_BBL(cfg, bbl) {
		if (BBL_IS_HELL(bbl))
			continue;

		/* count the number of incoming FT edges: real and fake */
		int nr_real = 0;
		int nr_fake = 0;
		int nr_return = 0;
		t_bbl *ft_call = NULL;

		t_cfg_edge *e;
		BBL_FOREACH_PRED_EDGE(bbl, e) {
			if (CFG_EDGE_CAT(e) == ET_RETURN) {
				ft_call = CFG_EDGE_HEAD(CFG_EDGE_CORR(e));
				nr_return++;
			}
			else {
				if (!CfgEdgeIsFallThrough(e))
					continue;

				if (CfgEdgeIsFake(e))
					nr_fake++;
				else
					nr_real++;
			}
		}
		ASSERT(nr_return <= 1, ("expected only one return edge @eiB", bbl));

		/* only look at problematic cases,
		 * i.e., BBLs with multiple incoming FT edges */
		if ((nr_fake + nr_real + nr_return) <= 1)
			continue;

		/* this case should be solved! */
		bool have_ft = FALSE;
		t_cfg_edge *temp;
		BBL_FOREACH_PRED_EDGE_SAFE(bbl, e, temp) {
			if (!CfgEdgeIsFallThrough(e))
				continue;

			/* skip conditional edge from call */
			if (CFG_EDGE_HEAD(e) == ft_call)
				continue;

			/* this is a FT edge */
			bool need_jump = FALSE;

			/* do we need to jump? */
			if (nr_return > 0 || ft_call)
				need_jump = TRUE;
			else {
				if (have_ft)
					need_jump = TRUE;
				else {
					/* we don't have a FT edge yet */
					if (CfgEdgeIsFake(e)) {
						/* we're looking at a fake edge */
					}
					else {
						/* we're looking at a real edge */
						if (nr_fake > 0)
							need_jump = TRUE;
					}
				}
			}

			/* if so, append an unconditional branch instruction at the end of the HEAD */
			if (need_jump) {
				t_bbl *new_bbl = BblNew(cfg);

				t_function *fun = BBL_FUNCTION(bbl);
				if (fun)
					BblInsertInFunction(new_bbl, FunctionIsAF(fun) ? BBL_FUNCTION(CFG_EDGE_HEAD(e)) : fun);

				t_arm_ins *branch_ins;
				ArmMakeInsForBbl(UncondBranch, Append, branch_ins, new_bbl, FALSE);

				/* execution information */
				BblCopyExecInformationFromEdge(e, new_bbl);

				/* kill the existing edge */
				t_bbl *head = CFG_EDGE_HEAD(e);
				t_bbl *tail = CFG_EDGE_TAIL(e);
				if (CFG_EDGE_CORR(e))
					CfgEdgeKill(CFG_EDGE_CORR(e));
				bool was_fake = CfgEdgeIsFake(e);
				CfgEdgeKill(e);

				/* create new edges */
				t_cfg_edge *ft_edge = CfgEdgeCreate(cfg, head, new_bbl, ET_FALLTHROUGH);
				if (was_fake)
					CfgEdgeMarkFake(ft_edge);
				EdgeMakeInterprocedural(ft_edge);
				BblCopyExecInformationToEdge(new_bbl, ft_edge);

				t_cfg_edge *jp_edge = CfgEdgeCreate(cfg, new_bbl, tail, ET_JUMP);
				if (was_fake)
					CfgEdgeMarkFake(jp_edge);
				EdgeMakeInterprocedural(jp_edge);
				BblCopyExecInformationToEdge(new_bbl, jp_edge);
			}
			else
				have_ft = TRUE;
		}
	}

	STATUS(STOP, ("Branch Flipping, fake edge specialisation (%u flips)", nr_transformations));
}
