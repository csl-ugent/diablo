/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>

#ifndef DIABLOSUPPORT_RANDOM
#define DIABLOSUPPORT_RANDOM

typedef struct t_randomnumbergenerator_ t_randomnumbergenerator;

/* constructors */
t_randomnumbergenerator *RNGCreateBySeed(t_uint32 seed, t_const_string name);
t_randomnumbergenerator *RNGCreateChild(t_randomnumbergenerator *parent, t_const_string name);
t_randomnumbergenerator *RNGCreateBySeedForRegion(t_uint32 seed, t_const_string name, t_const_string region_name);
t_randomnumbergenerator *RNGCreateChildForRegion(t_randomnumbergenerator *parent, t_const_string name, t_const_string region_name);

/* destructors */
void RNGDestroy(const t_randomnumbergenerator *rng);

/* helpers */
void RNGReset(t_randomnumbergenerator *rng);
void RNGSetRootGenerator(t_randomnumbergenerator *rng);
void RNGReadOverrides(t_const_string file);
void RNGSetRange(t_randomnumbergenerator *rng, t_uint32 min, t_uint32 max);
t_uint32 RNGGenerate(t_randomnumbergenerator *rng);
t_uint32 RNGGenerateWithRange(t_randomnumbergenerator *rng, t_uint32 min, t_uint32 max);
t_bool RNGGenerateBool(t_randomnumbergenerator *rng);

/* lookup */

/* The name is actually sort of a "path" in the RNG tree,
 * where the name of the parent and child are concatenated by inserting a '_' in between.
 * If an RNG is created for a region, additionally '_region_' + region_name is appended to
 * the name of the created RNG.
 *
 * When looking up an RNG by name, the full path must be specified, but if the root RNG is
 * set by calling RNGSetRootGenerator, the name of the root generator may be omitted. */
t_randomnumbergenerator *RNGGetRootGenerator();
t_randomnumbergenerator *RNGGetByName(t_const_string name);
t_randomnumbergenerator *RNGGetByNameForRegion(t_const_string name, t_const_string region_name);

#endif /* DIABLOSUPPORT_RANDOM */
