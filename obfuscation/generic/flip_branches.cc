/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>

#include <obfuscation/obfuscation_architecture_backend.h>
#include "flip_branches.h"
#include "flip_branches_opt.h"

/* TODO: factor out into a Policy */

FlipBranchesTransformation::FlipBranchesTransformation() {
  AddOptionsListInitializer(obfuscation_flip_branches_option_list); FlipBranchesOptInit();
  
  RegisterTransformationType(this, _name);
}

bool FlipBranchesTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_flip_branches_options.flip_branches && !AllObfuscationsEnabled())
    return false;

  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }

  t_function* fun = BBL_FUNCTION(bbl);
  if (!fun || FUNCTION_IS_HELL(fun) || !BBL_INS_LAST(bbl))
    return false;

  return true;
}
