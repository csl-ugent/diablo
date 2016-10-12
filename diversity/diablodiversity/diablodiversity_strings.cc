/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2012 Bart Coppens, Ghent University
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

extern "C" {
#include <diabloanopti386.h>
#include <diablodiversity.h>
#include "diablodiversity_cmdline.h"
}

t_bool ContainsProbableStringReference(t_bbl* bbl) {
  return FALSE;
}

t_arraylist* DiversityRandomizeStringAccessesCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (bbl && ContainsProbableStringReference(bbl)) {
    return SimpleCanTransform(bbl, NULL, 1);
  }
  return NULL;
}

t_diversity_options DiversityRandomizeStringAccessesDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  return diversity_options_null;
}
