#ifndef DIABLOSUPPORT_HPP
#define DIABLOSUPPORT_HPP
/* This header contains all C++ expansions of diabloflowgraph */

extern "C" {
#include <diablosupport.h>
}

#include "diablosupport_bitset.hpp"
#include "diablosupport_shared_library.hpp"
#include "diablosupport_fork.hpp"

#include <string>
#include <random>
#include <set>

std::string GetTransformationIdString();

typedef std::mt19937 t_RNG;
t_RNG &RNGGetGenerator(t_randomnumbergenerator *rng);

std::set<FILE *> GetOpenedLogFiles();

#endif
