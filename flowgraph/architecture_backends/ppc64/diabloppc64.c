/*
 * Copyright (C) 2006 Lluis Vilanova <xscript@gmx.net> {{{
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
 */


#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloppc64.h>

#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int ppc64_module_usecount = 0;

void
DiabloPpc64Init (int argc, char **argv)
{
  if (!ppc64_module_usecount)
  {
    DiabloFlowgraphInit(argc, argv);
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit (argc, argv);
#endif

    Ppc64DescriptionInit ();
    ArchitectureHandlerAdd ("ppc64", &ppc64_description, ADDRSIZE64);

    /* mark .glink stub and entries as data to make sure diablo doesn't modify
     * them */
    DiabloBrokerCallInstall ("ObjectDisassembleBefore", "t_object *", Ppc64MarkGlinkAsData, FALSE);
    /* Handle .opd-related things (set entry point, split .opd section, ... */
    DiabloBrokerCallInstall ("ObjectFlowgraphBefore", "t_object *", Ppc64LinuxProcessOpd, FALSE);
    /* We will add some "special" symbols for traceback and switch tables for
     * later faster lookup of those. */
    DiabloBrokerCallInstall ("CfgCreated", "t_object *, t_cfg *", DiabloFlowgraphPpc64CfgCreated, FALSE);
  }

  ppc64_module_usecount++;
}

void 
DiabloPpc64Fini()
{
  ppc64_module_usecount--;
  if (!ppc64_module_usecount)
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
    ArchitectureHandlerRemove ("ppc64");

    DiabloFlowgraphFini();
  }
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab: */
