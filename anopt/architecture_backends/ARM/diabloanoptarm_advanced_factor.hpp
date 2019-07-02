#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_HPP
#define DIABLOANOPTARM_ADVANCED_FACTOR_HPP

extern "C" {
#include <diabloarm.h>
#include <diabloanoptarm.h>
}
#include <diabloflowgraph.hpp>
#include <diabloanopt.hpp>

#include <diabloannotations.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

//#define AF_VERBOSE

#define AF_VERBOSITY_LEVEL 0
#define AF_VERBOSITY_LEVEL2 0

#define AF_COPY_ANALYSIS 1

#define AF_FUZZY_REGISTERS

/* defined = don't transform the factoring possibilities */
//#define AF_DONT_TRANSFORM
/* defined = don't consider unexecuted BBLs for factorisation */
//#define DONT_FACTOR_NONEXECUTED_BBLS
/* defined = set is useless if it doesn't cover the master and slave region */
//#define NONCROSSED_SETS_ARE_USELESS
/* defined = set is useless if it doesn't contain a master and slave slice that are both executed */
//#define NEED_AT_LEAST_ONE_EXEC_MS
/* defined = set is useless if it doesn't contain two executed slices (looser variant of the previous definition) */
//#define NEED_AT_LEAST_TWO_EXEC_SLICE

extern t_randomnumbergenerator *af_rng;
extern t_randomnumbergenerator *af_rng_transformation;
extern t_randomnumbergenerator *af_rng_dispatcher;
extern t_randomnumbergenerator *af_rng_dummy;
extern t_randomnumbergenerator *af_rng_randomswitch;
extern t_randomnumbergenerator *af_rng_redirect;
extern t_randomnumbergenerator *af_rng_distributed;
extern t_randomnumbergenerator *af_rng_shuffle;
extern t_randomnumbergenerator *af_rng_compare;

extern bool af_dynamic_member_init;

class Slice;
struct SliceCompare;
typedef std::set<Slice *, SliceCompare> SliceSet;
typedef std::vector<Slice *> SliceVector;
typedef std::vector<t_bbl *> BblVector;
typedef std::vector<t_cfg_edge *> CfgPath;

typedef t_uint8 AfFlags;

#define AF_POSSIBILITY_INDIRECTBRANCH           1<<0
#define AF_POSSIBILITY_CONDITIONALJUMP          1<<1
#define AF_POSSIBILITY_DISTRIBUTED              1<<2
#define AF_POSSIBILITY_SWITCHOFFSET             1<<3
#define AF_POSSIBILITY_SWITCHBRANCH             1<<4
#define AF_POSSIBILITY_INTERNAL_CONDITIONALJUMP 1<<5
#define AF_POSSIBILITY_BOUNDS_CHECK             1<<6
typedef t_uint32 t_possibility_flags;

#define AF "[AF] "

#include "advanced_factoring_libraries/score.h"

#include "diabloanoptarm_advanced_factor_gpregisters.h"
#include "diabloanoptarm_advanced_factor_debug.h"
#include "diabloanoptarm_advanced_factor_action.h"
#include "diabloanoptarm_advanced_factor_bookkeeping.h"
#include "diabloanoptarm_advanced_factor_regions.h"
#include "diabloanoptarm_advanced_factor_abstract_instruction.h"
#include "diabloanoptarm_advanced_factor_fingerprint.h"
#include "diabloanoptarm_advanced_factor_slice.h"
#include "diabloanoptarm_advanced_factor_priority.h"
#include "diabloanoptarm_advanced_factor_slice_hash.h"
#include "diabloanoptarm_advanced_factor_analysis.h"
#include "diabloanoptarm_advanced_factor_state.h"
#include "diabloanoptarm_advanced_factor_branch.h"
#include "diabloanoptarm_advanced_factor_factoring.h"
#include "diabloanoptarm_advanced_factor_liveness.h"
#include "diabloanoptarm_advanced_factor_statistics.h"
#include "diabloanoptarm_advanced_factor_sliceset.h"
#include "diabloanoptarm_advanced_factor_dispatch.h"
#include "diabloanoptarm_advanced_factor_combine.h"
#include "diabloanoptarm_advanced_factor_nonzero_analysis.h"
#include "diabloanoptarm_advanced_factor_callgraph.h"
#include "diabloanoptarm_advanced_factor_distributed.h"
#include "diabloanoptarm_advanced_factor_eq_immediates.h"
#include "diabloanoptarm_advanced_factor_eq_registers.h"
#include "diabloanoptarm_advanced_factor_canonicalize.h"
#include "diabloanoptarm_advanced_factor_constant_analysis.h"
#include "diabloanoptarm_advanced_factor_backup.h"
#include "diabloanoptarm_advanced_factor_obfuscation.h"

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(slices, SLICES, Slices, SliceSet*, {*valp = NULL;}, {if (*valp) delete *valp;}, {*valp = NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(incoming_for_slice, INCOMING_FOR_SLICE, IncomingForSlice, SliceSet*, {*valp = NULL;}, {if (*valp) delete *valp;}, {*valp = NULL;});
INS_DYNAMIC_MEMBER_GLOBAL_BODY(slices, SLICES, Slices, SliceSet*, {*valp = NULL;}, {if (*valp) delete *valp;}, {*valp = NULL;});
INS_DYNAMIC_MEMBER_GLOBAL(order, ORDER, Order, int, -1);
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(incoming, INCOMING, Incoming, std::vector<CfgPath>*, {*valp=NULL;}, {if (*valp) delete *valp;}, {*valp=NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL(can_transform, CAN_TRANSFORM, CanTransform, bool, false);
EDGE_DYNAMIC_MEMBER_GLOBAL(af_corr, AF_CORR, AfCorr, t_cfg_edge *, NULL);
EDGE_DYNAMIC_MEMBER_GLOBAL_BODY(slice_information, SLICE_INFORMATION, SliceInformation, TransformedSliceInformation *, {*valp=NULL;}, {if (*valp) delete *valp;}, {*valp = NULL;});
EDGE_DYNAMIC_MEMBER_GLOBAL(equations, EQUATIONS, Equations, t_equations, NULL);
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(table_id_vector, TABLE_ID_VECTOR, TableIdVector, std::set<size_t>*, {*valp=NULL;}, {if (*valp) delete *valp;}, {*valp=NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL(original_id, ORIGINAL_ID, OriginalId, t_uint32, 0);
BBL_DYNAMIC_MEMBER_GLOBAL(is_landing_site, IS_LANDING_SITE, IsLandingSite, bool, false);
FUNCTION_DYNAMIC_MEMBER_GLOBAL(is_af, IS_AF, IsAf, bool, false);
FUNCTION_DYNAMIC_MEMBER_GLOBAL(af_flags, AF_FLAGS, AfFlags, AfFlags, 0);
BBL_DYNAMIC_MEMBER_GLOBAL(prodprop, PRODPROP, ProdProp, t_uint32, 0);
FUNCTION_DYNAMIC_MEMBER_GLOBAL(explicitely_saved, EXPLICITELY_SAVED, ExplicitelySaved, t_regset*, NULL);

/* AF flags */
#define AF_FLAG_DIRTY_SP      (1<<0)
#define AF_FLAG_SP_ALMOST_OK  (1<<1)
void BblSetAFFlag(t_bbl *bbl, int flag);
bool BblGetAFFlag(t_bbl *bbl, int flag);
void BblSetAFDispatchRegister(t_bbl *bbl, t_reg reg);
t_reg BblGetAFDispatchRegister(t_bbl *bbl);
void BblSetAFDispatchType(t_bbl *bbl, DispatcherType dispatcher);
DispatcherType BblGetAFDispatchType(t_bbl *bbl);
t_regset FunctionExplicitelySavedRegs(t_function *fun);
bool FunctionIsAF(t_function *fun);

extern FunctionUID af_function_uid;

extern std::set<TransformationID> all_af_tf_ids;

extern std::map<DispatcherType, F_DispatchGenerator> dispatcher_type_to_generator_map;

#endif
