/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
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

#include <diablodiversity.h>

t_diversity_options DiversityFactorFunctions(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);
t_diversity_options DiversityFactorEpilogues(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);
t_diversity_options DiversityFactorBbls(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase);
void RegisterBblFactoring(t_bbl * new_bbl, t_bbl * orig_bbl);
void RegisterInsFactoring(t_ins * orig_ins, t_ins * new_ins);
void RegisterFunFactoring(t_function * fun);
