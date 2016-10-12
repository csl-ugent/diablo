/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * }}}
 *
 * This file is part of the SPE port of Diablo (Diablo is a better link-time
 * optimizer)
 */


#include <diablospe.h>
#if 0
#include <diabloanopt.h>
#include <diabloanoptspe.h>
#endif
#include "diablo_spe_options.h"

int spe_rewrite (t_cfg *cfg)
{

  CfgDrawFunctionGraphsWithHotness (cfg, "./dots");

  if (diablo_spe_options.initcfgopt)
  {
    STATUS(START, ("Removing unconnected blocks"));
    CfgRemoveDeadCodeAndDataBlocks (cfg);
    STATUS(STOP, ("Removing unconnected blocks"));
  }

  /* split functions so they are all single-entry: some Diablo analyses and
   * optimizations expect this */
  CfgPatchToSingleEntryFunctions (cfg);

  if (diablo_spe_options.remove_unconnected)
  {
    CfgRemoveDeadCodeAndDataBlocks (cfg);
  }

  CfgRemoveUselessConditionalJumps (cfg);
  CfgRemoveEmptyBlocks (cfg);

  if (diablo_spe_options.remove_unconnected)
  {
    CfgRemoveDeadCodeAndDataBlocks (cfg);
  }

  return 0;
}

int
main (int argc, char **argv)
{
  /* initialize the modules you're going to use */
#if 0
  DiabloAnoptInit (argc, argv);
  DiabloAnoptSpeInit (argc, argv);
#else
  DiabloFlowgraphInit (argc, argv);
  DiabloSpeInit (argc, argv);
#endif

  DiabloSpeOptionsInit ();
  OptionParseCommandLine (diablo_spe_option_list, argc, argv, TRUE);
  OptionGetEnvironment (diablo_spe_option_list);
  DiabloSpeOptionsVerify ();
  OptionDefaults (diablo_spe_option_list);

  ObjectRewrite (diablo_spe_options.objectfilename, spe_rewrite, diablo_spe_options.output_name);

  DiabloSpeFini ();
  DiabloFlowgraphFini ();

  PrintRemainingBlocks ();

  return 0;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
