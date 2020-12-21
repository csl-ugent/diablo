#ifndef DIABLOFLOWGRAPH_HPP
#define DIABLOFLOWGRAPH_HPP
/* This header contains all C++ expansions of diabloflowgraph */

extern "C" {
#include <diabloflowgraph.h>
#include <diablosupport.h>
#include "diabloflowgraph_new_target_selector_cmdline.h"
}

/* C++ standard headers */
#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "diabloflowgraph_STL.hpp"
#include "diabloflowgraph_object_tracking.hpp"
#include "diabloflowgraph_transformation.hpp"
#include "diabloflowgraph_complexity.hpp"
#include "diabloflowgraph_fake_edge_cycles.hpp"

/* If the transformation classes have not been included, don't include the headers that build on them */
#ifdef DIABLOFLOWGRAPH_TRANSFORMATION_CLASS
#include "diabloflowgraph_new_target_selector.hpp"
#endif

void ReadSequenceData(t_cfg * cfg, t_string name, t_address before_address, BblSet& before, t_address after_address, BblSet& after);

#endif
