/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_H
#define DIABLOSOFTVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <diabloflowgraph.h>

#define TYPEDEFS
#include "diablosoftvm_vmchunk.class.h"
#undef TYPEDEFS
#define TYPES
#include "diablosoftvm_vmchunk.class.h"
#undef TYPES
#define DEFINES
#include "diablosoftvm_vmchunk.class.h"
#undef DEFINES
#define DEFINES2
#include "diablosoftvm_vmchunk.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diablosoftvm_vmchunk.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diablosoftvm_vmchunk.class.h"
#undef CONSTRUCTORS

#ifdef __cplusplus
}
#endif

#include "diablosoftvm_cmdline.h"
#include "diablosoftvm_jsonprinter.h"
#include "diablosoftvm_json.h"
#include "aspire_options.h"

void AspireSoftVMInit();
void AspireSoftVMPreFini();
void AspireSoftVMFini(t_ptr_array *chunks);
int AspireSoftVMMarkAndSplit(t_cfg *cfg, t_ptr_array *chunks, t_randomnumbergenerator *rng);
void AspireSoftVMExport(t_cfg *cfg, t_ptr_array *chunks);
void AspireSoftVMFixups(t_cfg *cfg, t_ptr_array *chunks);
void AspireSoftVMInsertGlueJumps(t_cfg *cfg, t_ptr_array *chunks);
void AspireSoftVMReadExtractorOutput(t_cfg * cfg, t_ptr_array *unordered_chunks, t_ptr_array * chunks);

void CfgEdgeKillSoftVM(t_cfg_edge *edge);
void BblKillSoftVM(t_bbl *bbl);

#define SVM "[SoftVM] "
#define SOFTVM_VERBOSITY_LEVEL 0

extern FILE *L_SOFTVM;

#endif
