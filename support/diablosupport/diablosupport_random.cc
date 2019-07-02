/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

extern "C" {
#include <diablosupport.h>
}

#include <random>
#include <string>
#include <map>
#include <fstream>
#include <iostream>

using namespace std;

#define RANDOM_VERBOSITY 10
#define RANDOM_VERBOSITY_V (RANDOM_VERBOSITY + 1)

enum class RNGOverrideType {
	GenerateNext,
	Seed
};

struct RNGOverride {
	t_uint32 value;
	RNGOverrideType type;
};

typedef uniform_int_distribution<t_uint32> t_dist;
typedef mt19937 t_RNG;
typedef map<string, RNGOverride> t_overrides;
typedef map<string, t_randomnumbergenerator *> t_rngs;

static t_overrides overrides;
static t_rngs random_generators;

static t_randomnumbergenerator *root_rng = NULL;

struct t_randomnumbergenerator_ {
	t_RNG generator;
	t_dist distribution;

	t_uint32 seed;
	t_uint32 min, max;
  t_bool range;

	string name;
};

ofstream dumpfile;

static
void DumpRNG(t_randomnumbergenerator *rng) {
	if (!dumpfile.is_open())
		return;

	/*dumpfile << "# '" << rng->name << "', seed " << rng->seed;
	if (rng->range)
		dumpfile << ", range " << rng->min << "-" << rng->max;
	dumpfile << endl;*/

	dumpfile << rng->name << " " << rng->seed << endl;
}

t_RNG &RNGGetGenerator(t_randomnumbergenerator *rng) {
	return rng->generator;
}

static
string concatNameRegion(t_const_string name, t_const_string region_name) {
	string ret = "";

	if (name)
		ret += string(name);

	if (region_name
	    && string(region_name).length() > 0) {
		/* need to append region name */
		if (ret.length() > 0)
			ret += "_";

		ret += "region_" + string(region_name);
	}

	return ret;
}

static
string getFullRNGName(const string name) {
	/* no root RNG was defined yet */
	if (!root_rng)
		return name;

	/* no name given for the root RNG instance */
	if (root_rng->name.length() == 0)
		return name;

	/* the given name already starts with the root name, just return the given name */
	if (name.find(root_rng->name + "_") == 0)
		return name;

	/* we are looking up the root RNG itself */
	if (name == root_rng->name)
		return name;

	return root_rng->name + "_" + name;
}

void RNGInitialise() {
	if (diablosupport_options.rng_dump_generators)
		dumpfile.open(diablosupport_options.rng_dump_generators);
}

void RNGFinalise() {
	if (dumpfile.is_open())
		dumpfile.close();
}

void RNGInit(t_randomnumbergenerator *rng, t_uint32 seed) {
	rng->seed = seed;
	rng->generator.seed(seed);

	rng->min = 0;
	rng->max = 0;
  rng->range = FALSE;

	rng->name = "";
}

t_randomnumbergenerator *RNGCreate(t_randomnumbergenerator *parent, t_uint32 seed, t_const_string name, t_const_string region_name) {
	t_randomnumbergenerator *ret = new t_randomnumbergenerator();

	/* try to find the region-specific override first */
	t_overrides::iterator it = overrides.find(concatNameRegion(name, region_name));

	/* override the seed value if an override is specified */
	if (it != overrides.end()) {
		VERBOSE(RANDOM_VERBOSITY_V, ("  overriding default seed value"));

		/* advance the parent RNG if one is defined */
		t_uint32 parentNext = (parent) ? RNGGenerate(parent) : 0;

		/* depending on the specified type, determine the seed value */
		switch (it->second.type) {
		case RNGOverrideType::GenerateNext:
		{
			VERBOSE(RANDOM_VERBOSITY_V, ("  overriding by skipping %d parent numbers", it->second.value));
			ASSERT(parent, ("can't skip random numbers for non-inherited RNG initialisation"));

			/* preserve the state of the parent RNG by first creating a child and advancing that one */
			auto rng = RNGCreateBySeed(parentNext, "");
			rng->distribution = parent->distribution;
			rng->min = parent->min;
			rng->max = parent->max;

			/* skip the next N random numbers */
      for (t_uint32 i = 0; i < it->second.value; i++)
				RNGGenerate(rng);

			/* overridden seed value */
			seed = RNGGenerate(rng);

			/* memory cleanup */
			RNGDestroy(rng);
		}
			break;

		case RNGOverrideType::Seed:
			/* just use the provided value */
			VERBOSE(RANDOM_VERBOSITY_V, ("  overriding using raw seed value"));
			seed = it->second.value;
			break;

		default:
			FATAL(("unsupported override type %d!", it->second.type));
		}
	}
	else {
		/* no override is specified, check whether we need to inherit from a parent or not */
		seed = (parent) ? RNGGenerate(parent) : seed;
	}

	/* intialise the rng with the specified seed */
	VERBOSE(RANDOM_VERBOSITY_V, ("  initialising %p with seed %d", ret, seed));
	RNGInit(ret, seed);

	/* store the name for further use (e.g., child inheritance):
	 *  - parent name if a parent is specified
	 *  - own name if specified */
	string rng_name = concatNameRegion(name, region_name);

	if (rng_name.length() > 0) {
		/* only now a name is useful */
		if (parent
		    && parent->name.length() > 0) {
		  rng_name = parent->name + "_" + rng_name;
		}
	}

	ret->name = rng_name;

	/* add the newly created rng to the map if necessary */
	if (rng_name.length() > 0) {
		if (random_generators.find(rng_name) != random_generators.end())
			VERBOSE(0, ("  a random generator with name \"%s\" already exists; overwriting the old pointer (was %p)", rng_name.c_str(), random_generators[rng_name]));

		random_generators[rng_name] = ret;
	}

	VERBOSE(RANDOM_VERBOSITY, ("created rng with name \"%s\"", rng_name.c_str()));

	DumpRNG(ret);
	return ret;
}

t_randomnumbergenerator *RNGCreateBySeedForRegion(t_uint32 seed, t_const_string name, t_const_string region_name) {
	VERBOSE(RANDOM_VERBOSITY_V, ("creating new RNG: seed=%d, name=\"%s\", region_name=\"%s\"", seed, name, region_name));
	return RNGCreate(NULL, seed, name, region_name);
}

t_randomnumbergenerator *RNGCreateChildForRegion(t_randomnumbergenerator *parent, t_const_string name, t_const_string region_name) {
	VERBOSE(RANDOM_VERBOSITY_V, ("creating new RNG: parent=%p, name=\"%s\", region_name=\"%s\"", parent, name, region_name));
	return RNGCreate(parent, 0, name, region_name);
}

t_randomnumbergenerator *RNGCreateBySeed(t_uint32 seed, t_const_string name) {
	VERBOSE(RANDOM_VERBOSITY_V, ("creating new RNG: seed=%d, name=\"%s\"", seed, name));
	return RNGCreateBySeedForRegion(seed, name, NULL);
}

t_randomnumbergenerator *RNGCreateChild(t_randomnumbergenerator *parent, t_const_string name) {
	VERBOSE(RANDOM_VERBOSITY_V, ("creating new RNG: parent=%p, name=\"%s\"", parent, name));
	return RNGCreateChildForRegion(parent, name, NULL);
}

void RNGDestroy(const t_randomnumbergenerator *rng) {
	if (rng->name.length() > 0
	    && random_generators.find(getFullRNGName(rng->name)) != random_generators.end())
		random_generators.erase(getFullRNGName(rng->name));

	delete rng;
}

void RNGSetRange(t_randomnumbergenerator *rng, t_uint32 min, t_uint32 max) {
	VERBOSE(RANDOM_VERBOSITY, ("RNG %p now generates numbers in range [%d, %d]", rng, min, max));

	rng->min = (max > min) ? min : max;
	rng->max = (max > min) ? max : min;

  rng->range = TRUE;
	rng->distribution = t_dist(rng->min, rng->max);
}

t_uint32 RNGGenerate(t_randomnumbergenerator *rng) {
	t_uint32 ret = 0;

  /* If a range was set, generate a number from within this range */
	if (rng->range)
		ret = rng->distribution(rng->generator);
	else
		ret = static_cast<t_uint32>(rng->generator());

	VERBOSE(RANDOM_VERBOSITY, ("RNG %p (%s) generated %d", rng, rng->name.c_str(), ret));

	return ret;
}

/* Have the RNG generate a number outside of its usual range. It is preferable to simply set a range on an RNG
 * and always use RNGGenerate, but sometimes a number with a different range has to be generated. Continuously
 * changing the range would result in hard to read code, and creating loads of new child RNG's is not optimal either.
 */
t_uint32 RNGGenerateWithRange(t_randomnumbergenerator *rng, t_uint32 min, t_uint32 max)
{
  t_dist range = t_dist((max > min) ? min : max, (max > min) ? max : min);
  return range(rng->generator);
}

t_bool RNGGenerateBool(t_randomnumbergenerator *rng) {
	return ((RNGGenerate(rng) % 2) == 1) ? TRUE : FALSE;
}

t_uint32 RNGGeneratePercent(t_randomnumbergenerator *rng) {
	return RNGGenerateWithRange(rng, 1, 100);
}

void RNGReset(t_randomnumbergenerator *rng) {
	RNGInit(rng, rng->seed);
}

t_randomnumbergenerator *RNGGetByName(t_const_string name) {
	if (!name)
		return NULL;

	if (random_generators.find(getFullRNGName(string(name))) == random_generators.end())
		return NULL;

	return random_generators[getFullRNGName(string(name))];
}

t_randomnumbergenerator *RNGGetByNameForRegion(t_const_string name, t_const_string region_name) {
	return RNGGetByName(concatNameRegion(name, region_name).c_str());
}

void RNGSetRootGenerator(t_randomnumbergenerator *rng) {
	root_rng = rng;
}

t_randomnumbergenerator *RNGGetRootGenerator() {
	return root_rng;
}

/* Read in a file containing random generator seed overrides.
 * The syntax of this file is as follows:
 *
 *  Lines beginning with a '#' are ignored.
 *  Other lines are supposed to be one of two patterns:
 *    <rng generator name> <rng generator seed>
 *    <rng generator name> +<rng generator skip number>
 *
 * Where <rng generator name> is either just the name of a random number generator
 * or the name of a random number generator + "_region_" + the name of a region.
 */
#define WHITESPACE " \t"
void RNGReadOverrides(t_const_string file) {
	ifstream input(file);
	VERBOSE(RANDOM_VERBOSITY, ("reading in RNG overrides file \"%s\"", file));

	string line;
	int line_nr = 0;
	while (getline(input, line)) {
		line_nr++;

		size_t first_char = line.find_first_not_of(WHITESPACE);

		/* skip empty lines */
		if (first_char == string::npos)
			continue;

		/* skip comments */
		if (line[first_char] == '#')
			continue;

		size_t first_space = line.find_first_of(WHITESPACE, first_char);
		ASSERT(first_space != string::npos, ("expected line %d in %s to be of the form \"<name> <int>\", but got \"%s\"", line_nr, file, line.c_str()));

		size_t second_char = line.find_first_not_of(WHITESPACE, first_space);
		ASSERT(second_char != string::npos, ("expected line %d in %s to be of the form \"<name> <int>\", but got \"%s\"", line_nr, file, line.c_str()));

		string name = line.substr(first_char, first_space-first_char);
		string value_ = line.substr(second_char, line.length()-second_char);
		VERBOSE(RANDOM_VERBOSITY, ("  line %d defines override seed for RNG \"%s\": %s", line_nr, name.c_str(), value_.c_str()));

		RNGOverrideType type;
		size_t value_offset = 0;
		if (value_[0] == '+') {
			type = RNGOverrideType::GenerateNext;
			value_offset = 1;
		}
		else
			type = RNGOverrideType::Seed;

		t_uint32 value = stoul(value_.substr(value_offset), NULL, 0);

		RNGOverride override{value, type};
		overrides[name] = override;
	}
}
