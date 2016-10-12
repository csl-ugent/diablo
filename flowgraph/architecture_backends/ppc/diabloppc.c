/*
 * Copyright (C) 2005 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
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
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */


#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloppc.h>
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int ppc_module_usecount = 0;

void
DiabloPpcInit (int argc, char **argv)
{
  if (!ppc_module_usecount)
  {
    DiabloFlowgraphInit(argc, argv);
    DiabloPpcCmdlineInit();
    OptionParseCommandLine (diabloppc_option_list, argc, argv, FALSE);
    OptionGetEnvironment (diabloppc_option_list);
    DiabloPpcCmdlineVerify();
    OptionDefaults(diabloppc_option_list);

#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit (argc, argv);
#endif
    ArchitectureHandlerAdd ("ppc", &ppc_description, ADDRSIZE32);
    DiabloBrokerCallInstall ("CfgCreated", "t_object *, t_cfg *", DiabloFlowgraphPpcCfgCreated, FALSE);
  }

  ppc_module_usecount++;
}

void 
DiabloPpcFini()
{
  ppc_module_usecount--;
  if (!ppc_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini ();
#endif
    ArchitectureHandlerRemove ("ppc");
    DiabloPpcCmdlineFini();
    DiabloFlowgraphFini();
  }
}

void
DiabloPpcCmdlineVersion ()
{
  printf ("DiabloPpc version %s\n", "0.5.0");
}
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
