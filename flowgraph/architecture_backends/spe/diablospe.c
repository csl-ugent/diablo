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
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */


#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diablospe.h>

#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int spe_module_usecount = 0;

void
DiabloSpeInit (int argc, char **argv)
{
    if (!spe_module_usecount)
    {
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
        DiabloBinutilsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
        DiabloElfInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
        DiabloArInit (argc, argv);
#endif

        DiabloSpeFlowgraphCmdlineInit ();
        OptionParseCommandLine (diablospeflowgraph_option_list, argc, argv, FALSE);
        OptionGetEnvironment (diablospeflowgraph_option_list);
        DiabloSpeFlowgraphCmdlineVerify ();
        OptionDefaults (diablospeflowgraph_option_list);

        SpeDescriptionInit ();
        ArchitectureHandlerAdd ("SPE", &spe_description, ADDRSIZE32);

        SpeOpcodesInit ();

#if 0
        /* We will add some "special" symbols for traceback and switch tables for
         * later faster lookup of those. */
        DiabloBrokerCallInstall ("CfgCreated", "t_object *, t_cfg *", DiabloFlowgraphSpeCfgCreated, FALSE);
#endif
    }

    spe_module_usecount++;
}

    void 
DiabloSpeFini()
{
    spe_module_usecount--;

    if (!spe_module_usecount)
    {
        SpeOpcodesFini ();

        ArchitectureHandlerRemove ("SPE");

        DiabloSpeFlowgraphCmdlineFini ();

#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
        DiabloArFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
        DiabloElfFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
        DiabloBinutilsFini ();
#endif
    }
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
