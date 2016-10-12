/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>

#include <obfuscation/obfuscation_architecture_backend.h>
#include "opaque_predicate.h"
#include "opaque_predicate_opt.h"

FILE* L_OBF_OOP = NULL;

OpaquePredicateTransformation::OpaquePredicateTransformation() {
  AddOptionsListInitializer(obfuscation_opaque_predicate_option_list); OpaquePredicateOptInit();
  
  RegisterTransformationType(this, _name);
}

/* Since, for now, we'll simply split BBLs, same preconditions as branch function... */
bool OpaquePredicateTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_opaque_predicate_options.opaque_predicates && !AllObfuscationsEnabled())
    return false;
  
  t_function* fun = BBL_FUNCTION(bbl);
  if (!fun || FUNCTION_IS_HELL(fun) || !BBL_INS_LAST(bbl))
    return false;
  
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (CFG_EDGE_CAT(edge) == ET_SWITCH)
      return false;
  }
  
  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }

  return BBL_NINS(bbl) > 1;
}
