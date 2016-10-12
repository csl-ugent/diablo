/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/***********************************************************
 * Copyright 2001,2002,2003: {{{
 * 
 * Bruno De Bus
 * Bjorn De Sutter
 * Dominique Chanet
 * Ludo Van Put
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Diablo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Diablo; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file contains the main program for DIABLO and CoALA. It can be used as
 * a skeleton for other objectconsumers/editors.
 * 
 * }}}
 **********************************************************/

/* Includes {{{ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif
#include "fileformats/ar/ar_arch.h"
#include <diablosupport.h>
#include <diabloobject.h>
#include "kernel/diablo_print.h"
#include "kernel/diablo_liveness.h"
#include "kernel/diablo_flowgraph.h"
#include "kernel/diablo_constprop.h"
#include "kernel/diablo_opt_misc.h"
#include "kernel/diablo_dominator.h"
#include "kernel/diablo_iterators.h"
#include "kernel/diablo_programexit.h"
#include "kernel/diablo_save_state.h"
#include "kernel/diablo_copy_analysis.h"
#include "kernel/diablo_profile.h"
#include "kernel/diablo_code_motion.h"
#include "kernel/diablo_pre.h"
#include "kernel/diablo_dumps.h"

#ifdef ARM_SUPPORT
#include "arch/arm/arm_flowgraph.h"
#include "arch/arm/arm_copy_propagation.h"
#include "arch/arm/arm_constant_propagation.h"
#include "arch/arm/arm_peephole.h"
#include "arch/arm/arm_ls_fwd.h"
#include "arch/arm/thumb_conversion.h"
#include "kernel/diablo_copy_analysis.h"
#include "kernel/diablo_factor.h"
#endif

#include "frontends/diablo_i386_options.h"
/*}}}*/

#define SELFMOD_PREFIX "__sm_prefix_"

int
main (int argc, char **argv)
{
  t_object *obj;
  DiabloSupportInit (argc, argv);
  DiabloObjectInit (argc, argv);

  MoreIo ();

  OptionParseCommandLine (global_list, argc, argv, FALSE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);



  DiabloArInit ();
  DiabloElfInit (argc, argv);
  MapInit ();


  /* III. The REAL program {{{ */

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (global_options.restore || (global_options.restore_multi != -1))
    global_options.objectfilename = RestoreDumpedProgram ();


  if (global_options.read)
    {
      /* A. Get a binary to work with {{{ */

      if (global_options.restoreState)
	obj = RestoreState ();
      else
	obj = LinkEmulate (global_options.objectfilename, FALSE);

      /*let editor use printf function*/
/*      {
	t_symbol * sym = SymbolTableGetSymbolByName (obj->sub_symbol_table, "printf");
	SymbolTableAddSymbolToSubSection(obj->sub_symbol_table,sym->type, sym->base, AddressAdd(sym->offset_from_start,SECTION_CADDRESS(sym->base)), "__sm_prefix_printf", sym->local);
      }*/
      
      LinkObjectFileNew(obj, StringConcat2(DIABLO_INSTALL_PREFIX,"/frontends/selfmod/sm_editor_source.o"), SELFMOD_PREFIX,FALSE,TRUE, NULL);
      

      /* End Load }}} */


      if (global_options.print_data_layout)
	{
	  t_object *subobj;
	  t_object *tmp;
	  t_section *sec;
	  int tel;
	  OBJECT_FOREACH_SUBOBJECT (obj, subobj, tmp)
	    OBJECT_FOREACH_SECTION (subobj, sec, tel)
	  {
	    if (SECTION_TYPE (sec) == DATA_SECTION)
	      VERBOSE (0,
		       (".data 0x%x 0x%x\n", SECTION_OLD_ADDRESS (sec),
			SECTION_CSIZE (sec)));
	    else if (SECTION_TYPE (sec) == BSS_SECTION)
	      VERBOSE (0,
		       (".bss 0x%x 0x%x\n", SECTION_OLD_ADDRESS (sec),
			SECTION_CSIZE (sec)));
	  }
	  exit (0);
	}
      if (global_options.relayout_description)
	{
	  ObjectMergeRODataSections (obj);
	  ObjectMoveBssToDataSections (obj);
	  ObjectRelayoutData (obj, global_options.relayout_description);
	}
      else
	if ((global_options.disassemble)
	    && (!(global_options.dump || global_options.dump_multiple)))

	  if (obj->mapped_first)
	    ObjectMergeCodeSections (obj);

	  ObjectDisassemble (obj);
	  /* 2. {{{ Create the flowgraph */
	  if (global_options.flowgraph)
	    {
	      t_cfg *cfg;
	      t_string force_reachable_funs[2] = {"__sm_prefix_editor", NULL};
	      ObjectFlowgraph (obj, NULL, force_reachable_funs);
	      cfg = OBJECT_CFG(obj);
	      I386StucknessAnalysis (obj);

	      /* Remove unconnected blocks */
	      if (global_options.initcfgopt)
	      {
		{
		  t_bbl * bbl, *bbl_2;
		  CFG_FOREACH_BBL_SAFE(cfg,bbl,bbl_2)
		  {
		    if(!IS_DATABBL(bbl) && BBL_FUN(bbl)==NULL)
		    {
		      t_cfg_edge * edge, *edge_2;
		      BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge,edge_2)
		      {
			CfgEdgeKill(edge);
		      }
		      BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,edge_2)
		      {
			CfgEdgeKill(edge);
		      }
		      BblKill(bbl);
		    }
		  }
		}
	      }

	      CfgPatchToSingleEntryFunctions (cfg);

	      if (global_options.initcfgopt)
	      {
		{
		  t_bbl * bbl, *bbl_2;
		  CFG_FOREACH_BBL_SAFE(cfg,bbl,bbl_2)
		  {
		    if(!IS_DATABBL(bbl) && BBL_FUN(bbl)==NULL)
		    {
		      t_cfg_edge * edge, *edge_2;
		      BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge,edge_2)
		      {
			CfgEdgeKill(edge);
		      }
		      BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,edge_2)
		      {
			CfgEdgeKill(edge);
		      }
		      BblKill(bbl);
		    }
		  }
		}
	      }

	      if (diabloflowgraph_options.blockprofilefile)
		{
		  CfgReadBlockExecutionCounts (cfg,
					       diabloflowgraph_options.
					       blockprofilefile);
		  printf ("START WEIGHT %d\n", CfgComputeWeight (cfg));
		  if (diabloflowgraph_options.insprofilefile)
		    {
		      CfgReadInsExecutionCounts (cfg,
						 diabloflowgraph_options.
						 insprofilefile);
		      CfgComputeHotBblThreshold (cfg, 0.90);
		    }
		}

	      /* Dump graphs prior to optimization {{{ */
	      if (global_options.generate_dots)
		{
		  if (diabloflowgraph_options.blockprofilefile)
		    CfgComputeHotBblThreshold (cfg, 0.90);
		  else if (global_options.annotate_loops)
		    {
		      ComDominators (cfg);
		      Export_Flowgraph (T_BBL (cfg->entries->entry_bbl),
					0xffff, "./dots/flowgraph_loop.dot");
		    }

		  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots");

		  CgBuild (cfg);
		  CgExport (cfg->cg, "./dots/callgraph.dot");
		}
	      /* }}} */
	      /* {{{ dominator analysis */
	      if (global_options.dominator)
		{
		  ComDominators (cfg);
		}
	      /* Export dots after dominator */
	      if (global_options.dominator && global_options.generate_dots)
		{
		  t_function *function;


		  DirMake ("./dots-dominator", FALSE);

		  Export_FunctionDominator ((cfg)->unique_entry_node, 0xffff,
					    "./dots-dominator/flowgraph.dot");

		  CFG_FOREACH_FUN(OBJECT_CFG(obj), function)
		  {
		    char *fname, *fname2;
		    if (function->bbl_first)
		      {
			fname =
			  sDiabloPrint ("./dots-dominator/@G.func-%s.dot",
					BBL_OLD_ADDRESS (function->bbl_first),
					FUN_NAME (function));
		      }
		    else
		      {
			fname =
			  sDiabloPrint ("./dots-dominator/0x0.func-%s.dot",
					FUN_NAME (function));
		      }
		    Export_FunctionDominator (function->bbl_first,
					      ~ET_INTERPROC, fname);
		    Free (fname);
		  }
		}
	      if (global_options.dominator)
		DominatorCleanup (cfg);
	      /* }}} */

	      if (global_options.factoring && global_options.optimize)
		WholeFunctionFactoring (cfg);

	      /* Analyses && Optimizations  {{{ */
	      for (global_optimization_phase = 0;
		   global_optimization_phase < 3; global_optimization_phase++)
		if (global_options.optimize)
		  {
		    int dead_count = 0;
		    int loopcount;
		    {
		      t_bool flag =
			global_options.rely_on_calling_conventions;
		      global_options.rely_on_calling_conventions =
			global_optimization_phase < 2;
		      if (global_options.rely_on_calling_conventions != flag)
			{
			  printf ("CALLING CONVENTIONS CHANGED!!!\n");
			}


		    }



		    if (!global_options.rely_on_calling_conventions
			&& global_options.factoring)
		      {
			FunctionEpilogueFactoring (cfg);
			CfgPatchToSingleEntryFunctions (cfg);
			BblFactoring (cfg);
		      }



		    CfgCreateExitBlockList (cfg);


		    /* always do this to make sure the live out sets of bbls are properly
		     * initialized, even if liveness is turned off */

		    CfgComputeLiveness (cfg, TRIVIAL);


		    loopcount = 0;

		    do
		      {
			dead_count = 0;

			loopcount++;
			if (global_options.remove_unconnected)
			  CfgRemoveDeadCodeAndDataBlocks (cfg);


			if (!global_options.rely_on_calling_conventions)
			  if (global_options.inlining)
			    {
			      DiabloBrokerCall ("InlineTrivial", cfg);
			    }


			/*ComputeDefinedRegs(cfg); */



			if (global_options.liveness)
			  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

			CfgDetectLeaves (cfg);

			if (global_options.constant_propagation)
			  {

			    ConstantPropagation (cfg, CONTEXT_SENSITIVE);
			    OptUseConstantInformation (cfg,
						       CONTEXT_SENSITIVE);
			    FreeConstantInformation (cfg);
			  }


			if (global_options.liveness)
			  dead_count += CfgKillUselessInstructions (cfg);


			if (global_options.dominator
			    && global_options.loop_invariant_code_motion)
			  {
			    CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
			    CfgComputeSavedChangedRegisters (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

			    if (diabloflowgraph_options.blockprofilefile)
			      {
				CfgComputeHotBblThreshold (cfg, 0.90);
				CfgEstimateEdgeCounts (cfg);

				ComDominators (cfg);
				CfgHoistConstantProducingCode (cfg);

#ifdef SPEEDUP_DOMINATORS
				if (dominator_info_correct == FALSE)
#endif
				  DominatorCleanup (cfg);
				CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
				CfgEstimateEdgeCounts (cfg);
			      }

#ifdef SPEEDUP_DOMINATORS
			    if (dominator_info_correct == FALSE)
#endif
			      ComDominators (cfg);
			    CfgHoistConstantProducingCode3 (cfg);
#ifdef SPEEDUP_DOMINATORS
			    if (dominator_info_correct == FALSE)
#endif
			      DominatorCleanup (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

#ifdef SPEEDUP_DOMINATORS
			    if (dominator_info_correct == FALSE)
#endif
			      ComDominators (cfg);
			    DetectLoopStackSubAdds (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
			    DetectLoopInvariants (cfg);
#ifdef SPEEDUP_DOMINATORS
			    if (dominator_info_correct == FALSE)
#endif
			      DominatorCleanup (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

			    if (diabloflowgraph_options.blockprofilefile)
			      {
				CfgEstimateEdgeCounts (cfg);
				CfgComputeHotBblThreshold (cfg, 0.85);
#ifdef SPEEDUP_DOMINATORS
				if (dominator_info_correct == FALSE)
#endif
				  ComDominators (cfg);
				LoopUnrollingSimple (cfg);
				DominatorCleanup (cfg);
				CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
			      }
			  }

			if (global_options.copy_analysis)
			  CopyAnalysis (*(obj->code));

			if (global_options.copy_propagation)
			  DiabloBrokerCall ("OptCopyPropagation", cfg);	/* only does something on the ARM */


			TraceBblAtAddress (cfg, global_options.traceadr,
					   "na_copy_propagatie");



			if (global_options.loadstorefwd)
			  DiabloBrokerCall ("ArmLoadStoreFwd", cfg);
			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");
			if (global_options.copy_analysis)
			  CopyAnalysisFini (*(obj->code));

			CfgRemoveUselessConditionalJumps (cfg);

			CfgRemoveEmptyBlocks (cfg);


			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");
			if (global_options.remove_unconnected)
			  CfgRemoveDeadCodeAndDataBlocks (cfg);
			CfgMoveInsDown (cfg);
			CfgComputeSavedChangedRegisters (cfg);
			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");



			if (global_options.liveness)
			  {
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
			  }
			if (global_options.remove_unconnected)
			  CfgRemoveDeadCodeAndDataBlocks (cfg);

			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");
			if (global_options.branch_elimination)
			  DiabloBrokerCall ("BranchForwarding", cfg);
			if (global_options.branch_elimination)
			  DiabloBrokerCall ("BranchElimination", cfg);
			if (global_options.mergebbls)
			  DiabloBrokerCall ("MergeBbls", cfg);

			if (global_options.pre)
			  {
			    if (diabloflowgraph_options.blockprofilefile)
			      {
				CfgEstimateEdgeCounts (cfg);
				PartialRedundancyElimination1 (cfg);
				PartialRedundancyElimination1ForCalls (cfg);
				PartialRedundancyElimination1ForReturns (cfg);
			      }

			    CfgComputeLiveness (cfg, TRIVIAL);
			    CfgComputeSavedChangedRegisters (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
			    CfgComputeSavedChangedRegisters (cfg);
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

			    ReplaceTriangleWithConditionalMove (cfg);

			    if (diabloflowgraph_options.blockprofilefile)
			      {
				PartialRedundancyElimination2 (cfg);
				PartialRedundancyElimination3 (cfg);

				if (diabloflowgraph_options.insprofilefile)
				  {
				    CfgComputeHotBblThreshold (cfg, 0.95);
				    DetectColdCodeBundles (cfg);
				  }
				CfgEstimateEdgeCounts (cfg);

				CfgBranchSwitch (cfg);
				CfgBranchSwitch2 (cfg);
				CfgComputeHotBblThreshold (cfg, 0.90);
				CfgEstimateEdgeCounts (cfg);
			      }
			  }

			if (!global_options.rely_on_calling_conventions)
			  {
			    if (global_options.inlining)
			      {
				DiabloBrokerCall ("GeneralInlining", cfg);	/* Arm only */
				DiabloBrokerCall ("I386InlineSimple", cfg);
				/*DiabloBrokerCall("I386InlineFunctionsWithOneCallSite",cfg); */
			      }
			  }



			DetectStackAliases (cfg);

			if (global_options.peephole)
			  DiabloBrokerCall ("I386PeepHoles", cfg);


			if (global_options.peephole)
			  DiabloBrokerCall ("ArmPeepholeOptimizations", cfg);
			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");


			if (global_options.stack)
			  DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);	/* only for the ARM */
			/*DiabloBrokerCall("InstructieOptimalisatie",cfg); |+ only for the MIPS +| */
			CfgComputeSavedChangedRegisters (cfg);

			if (global_options.liveness)
			  dead_count += CfgKillUselessInstructions (cfg);

			TraceBblAtAddress (cfg, global_options.traceadr,
					   "HIER");

		      }
		    while (dead_count);
		  }

	      {
		dominator_info_correct = FALSE;
		DominatorCleanup (cfg);
	      }


	      /* End of Analyses && Optimizations  }}} */

	      if (diabloflowgraph_options.blockprofilefile)
		printf ("END WEIGHT %d\n", CfgComputeWeight (cfg));

	      /* Export dots after optimzation {{{  */
	      if (global_options.generate_dots)
		{
		  if (diabloflowgraph_options.blockprofilefile)
		    CfgComputeHotBblThreshold (cfg, 0.90);
		  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots-final");
		  CgBuild (cfg);
		  CgExport (cfg->cg, "./dots-final/callgraph.dot");
		}
	      /* }}} */


	      /* Back to the section representation (still disassembled though) */

	      if (1)
		{
		  int nr_ins = 0;
		  t_bbl *bbl;
		  CFG_FOREACH_BBL (cfg, bbl) nr_ins += BBL_NINS (bbl);
		  printf ("nr_ins %d\n", nr_ins);
		}


	      SmTryout(obj);

	      /* rebuild the layout of the data sections
	       * so that every subsection sits at it's new address */
	      ObjectRebuildSectionsFromSubsections (obj);
	      
	    }			/*  }}} */

	  if (global_options.saveState)
	    SaveState (obj, SAVESTATEDISASSEMBLED);

	  ObjectAssemble (obj);
	  /* End Transform and optimize }}} */
	}
      else if (global_options.dump || global_options.dump_multiple)
	{
#ifdef HAVE_DIABLO_EXEC
	  if (global_options.exec)
	    DiabloExec ();
#else
	  if (global_options.exec)
	    FATAL (("Exec not compiled!"));
#endif
	  DumpProgram (obj, global_options.dump_multiple);
	  exit (0);
	}
      else if (global_options.saveState)
	{
	  SaveState (obj, SAVESTATENOTDISASSEMBLED);
	}

      ObjectWrite (obj, global_options.output_name);

#ifdef DIABLOSUPPORT_HAVE_STAT
      /* make the file executable */
      chmod (global_options.output_name,
	     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
	     S_IXOTH);
#endif

    }
  /* END REAL program }}} */

  if (global_options.exec)
    {
#ifdef HAVE_DIABLO_EXEC
      DiabloExec ();
#else
      FATAL (("Exec not compiled!"));
#endif
    }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  DiabloObjectFini ();
  DiabloSupportFini ();



  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */

#ifdef DEBUG_MALLOC
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
#endif
  /* End Fini }}} */
  return 0;
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
