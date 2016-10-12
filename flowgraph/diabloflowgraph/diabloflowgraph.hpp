#ifndef DIABLOFLOWGRAPH_HPP
#define DIABLOFLOWGRAPH_HPP
/* This header contains all C++ expansions of diabloflowgraph */

extern "C" {
#include <diabloflowgraph.h>
}

/* C++ standard headers */
#include <set>
#include <vector>

#include "diabloflowgraph_STL.hpp"
#include "diabloflowgraph_complexity.hpp"
#include "diabloflowgraph_transformation.hpp"

/* If the transformation classes have not been included, don't include the headers that build on them */
#ifdef DIABLOFLOWGRAPH_TRANSFORMATION_CLASS
#include "diabloflowgraph_new_target_selector.hpp"
#endif

#endif
