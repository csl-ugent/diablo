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
#include "kernel/diablo_print.h"
#include "kernel/diablo_liveness.h"
#include "kernel/diablo_object.h"
#include "kernel/diablo_flowgraph.h"
#include "kernel/diablo_constprop.h"
#include "kernel/diablo_opt_misc.h"
#include "kernel/diablo_dominator.h"
#include "kernel/diablo_iterators.h"
#include "kernel/diablo_link.h"
#include "kernel/diablo_programexit.h"
#include "kernel/diablo_save_state.h"
#include "kernel/diablo_copy_analysis.h"
#include "kernel/diablo_profile.h"
#include "kernel/diablo_code_motion.h"
#include "kernel/diablo_pre.h"
#include "kernel/diablo_copy_analysis.h"

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

#include "frontends/diablo_mips_options.h"
/*}}}*/

/* Config variables for DUMP and RESTORE {{{ */
#define DUMP_DIR "./DUMP"
#define VERBOSE_DUMP 
/* }}}*/

/* DumpProgram {{{ */
void DumpProgram(t_object * obj,t_bool multiple_dumps)
{
      FILE * fp;
      t_uint32 tel;
      t_object * i_obj, * tmp; 
      t_string dest=StringConcat2("./DUMP",obj->name);
      t_string tmp_string=StringConcat2(obj->name,".map");
      if (multiple_dumps) Untar("dump.tar");
      FileCopy(obj->name,dest);
      Free(dest);
      dest=StringConcat3("./DUMP",obj->name,".map");
      FileCopy(tmp_string,dest);
      Free(dest);
      Free(tmp_string);


      /* Copy the subobjects {{{ */
      OBJECT_FOREACH_SUBOBJECT(obj, i_obj, tmp)
      {
	 if (strncmp(i_obj->name,"Linker---",9)==0)
	 {
	 }
	 else if (strcmp(i_obj->name,"Linker")==0)
	 {
	 }
	 else if (StringPatternMatch("*:*",i_obj->name))
	 {
	    t_string name2=StringDup(i_obj->name);
	    t_string libn=(t_string)strtok(name2,":");
	    dest=StringConcat2("./DUMP",libn);
	    FileCopy(libn,dest);
	    Free(name2);
	    Free(dest);
	 }
	 else
	 {
	    dest=StringConcat2("./DUMP",i_obj->name);
	    FileCopy(i_obj->name, dest);
	    Free(dest);
	 }
      }
      /* }}} */
      /* Copy the inputs  {{{ */
      if (global_options.input_files_set)
      {
	FATAL(("Implement"));
#if 0
	int tel;
	t_string_array * in=StringDivide(global_options.input_files,";",TRUE,FALSE);

	for (tel=0; tel<in->nstrings; tel++)
	{
	  char * name;
	  name=FileNameNormalize(in->strings[tel]);
	  if (name[0]!='/') FATAL(("Input file is not absolute! Implement"));
	  dest=StringConcat2("./DUMP",name);
	  FileCopy(name,dest);
	  Free(dest); Free(name);
	}
#endif
      }
      /* }}}*/

     /* Copy the outputs {{{ */

      if (global_options.output_files_set) {
	FATAL(("Implement"));
#if 0
	int tel;
	t_string_array * in=StringDivide(global_options.output_files,";",TRUE,FALSE);

	for (tel=0; tel<in->nstrings; tel++)
	{
	  char * name;
	  name=FileNameNormalize(in->strings[tel]);
	  if (name[0]!='/') FATAL(("Output file is not absolute! Implement"));
	  dest=StringConcat3("./DUMP",name,"_ref");
	  FileCopy(name,dest);
	  Free(dest); Free(name);
	}
#endif
      }

      /* }}} */


      if (multiple_dumps)
      {
	fp=fopen("./DUMP/diablo_dump","a");
	fprintf(fp,"=============\n");
      }
      else
	fp=fopen("./DUMP/diablo_dump","w");

      if (!fp) FATAL(("Failed to open diablo_dump, for writing"));
      fprintf(fp,"LPATH: ");
      for (tel=0; tel<global_options.libpath->nelems; tel++)
      {
	 fprintf(fp,"%s",global_options.libpath->dirs[tel]);
	 if (tel+1<global_options.libpath->nelems) fprintf(fp,":");
      }
      fprintf(fp,"\n");

      fprintf(fp,"OPATH: ");
      for (tel=0; tel<global_options.objpath->nelems; tel++)
      {
	 fprintf(fp,"%s",global_options.objpath->dirs[tel]);
	 if (tel+1<global_options.objpath->nelems) fprintf(fp,":");
      }
      fprintf(fp,"\n");

      fprintf(fp,"ARGS: %s\n",global_options.arguments);

      /* Set the input line {{{ */
      if (global_options.input_files_set) {
	FATAL(("Implement"));
#if 0
	int tel;
	t_string_array * in=StringDivide(global_options.input_files,";",TRUE,FALSE);

	fprintf(fp,"INPUTS: ");
	for (tel=0; tel<in->nstrings; tel++)
	{
	  char * name;
	  name=FileNameNormalize(in->strings[tel]);
	  if (name[0]!='/') FATAL(("Input file is not absolute! Implement"));
	  fprintf(fp,"%s",name);
	  if (tel+1<in->nstrings) fprintf(fp,";");
	  Free(name);
	}

	fprintf(fp,"\n");
#endif
      }
      /* }}} */

      /* Set the output line {{{ */
      /* The outputs */
      if (global_options.output_files_set) {
	FATAL(("Implement"));
#if 0
	int tel;
	t_string_array * in=StringDivide(global_options.output_files,";",TRUE,FALSE);

	fprintf(fp,"OUTPUTS: ");
	for (tel=0; tel<in->nstrings; tel++)
	{
	  char * name;
	  name=FileNameNormalize(in->strings[tel]);
	  if (name[0]!='/') FATAL(("Input file is not absolute! Implement"));
	  fprintf(fp,"%s",name);
	  if (tel+1<in->nstrings) fprintf(fp,";");
	  Free(name);
	}
#endif
	fprintf(fp,"\n");
      }

      /* }}} */


      fprintf(fp, "OBJ: %s\n",obj->name);
      fclose(fp);

     
      Tar("./DUMP","dump.tar");
}
/*}}} */

/* Execute the program {{{ */
#ifdef HAVE_IPC
#ifdef HAVE_FILENO
#define HAVE_DIABLO_EXEC
void DiabloExec()
{
     char * execname=FileNameNormalize("./b.out");
     char *f;
     char * instring=StringDup("DIABLO=1");
     FILE * in;
     FILE * out;
     pid_t pid;
     
     if ((global_options.read)&&(!(global_options.dump||global_options.dump_multiple))) chmod("./b.out",0700);
     else execname=FileFind(global_options.objpath,global_options.objectfilename);
     
     printf("------------- Executing %s with args %s\n",execname,global_options.arguments);

     /* Do the inputs */
     if (global_options.input_files_set) {
       int tel;
       t_string_array * in=StringDivide(global_options.input_files,";",TRUE,FALSE);

       for (tel=0; tel<in->nstrings; tel++)
       {
	 char * name;

	 if (diablosupport_options.prepend[0]!=0)
	 {
	   char * tmp;
	   FATAL(("Implement"));
	   //name=StringConcat3(global_options.prepend,"/",tmp=ConvertFileName(in->strings[tel]));
	   Free(tmp);
	 }
	 else
	 {
	   FATAL(("Implement"));
	   //name=ConvertFileName(in->strings[tel]);
	 }
	 
	 if (tel+1<10)
	 {
	   f=instring;
	   instring=StringConcat3(instring,"; Ix=",name);
	   Free(f);
	   sprintf(instring+strlen(instring)-strlen(name)-2,"%d",tel+1);
	 }
	 else
	 {
	   f=instring;
	   instring=StringConcat3(instring,"; Ixx=",name);
	   Free(f);
	   sprintf(instring+strlen(instring)-strlen(name)-3,"%d",tel+1);
	 }
	 *(instring+strlen(instring))='=';
	 Free(name);
	 
       }

       StringArrayFree(in);

     }

     /* Do the outputs */
     if (global_options.output_files_set) {
       int tel;
       t_string_array * in=StringDivide(global_options.output_files,";",TRUE,FALSE);

       for (tel=0; tel<in->nstrings; tel++)
       {
	 char * name;
	 if (diablosupport_options.prepend[0]!=0)
	 {
	   char * tmp;
	   FATAL(("Implement"));
	   //name=StringConcat3(global_options.prepend,"/",tmp=ConvertFileName(in->strings[tel]));
	   Free(tmp);
	 }
	 else
	 {
	   FATAL(("Implement"));
	   //name=ConvertFileName(in->strings[tel]);
	 }
	 if (tel+1<10)
	 {
	   f=instring;
	   instring=StringConcat3(instring,"; Ox=",name);
	   Free(f);
	   sprintf(instring+strlen(instring)-strlen(name)-2,"%d",tel+1);
	 }
	 else
	 {
	   f=instring;
	   instring=StringConcat3(instring,"; Oxx=",name);
	   Free(f);
	   sprintf(instring+strlen(instring)-strlen(name)-3,"%d",tel+1);
	 }
	 *(instring+strlen(instring))='=';
	 Free(name);
       }

       StringArrayFree(in);

     }
     
     if (global_options.arguments_set)
     {
       char * tofree1, * tofree2;
       if (global_options.host_set)
       {
	 /*CreateProcPipe(&pid,&out,&in, NULL, "ssh", global_options.host, tofree1=StringConcat3(tofree2=StringConcat3(instring,";",execname), " ", global_options.arguments),  NULL); */
       }
/*       else
       CreateProcPipe(&pid,&out,&in, NULL, "bash", "-c", tofree1=StringConcat3(tofree2=StringConcat3(instring,";",execname), " ", global_options.arguments),  NULL); */
       Free(tofree1);
       Free(tofree2);


       
     }
     else
     {
/*       CreateProcPipe(&pid,&out,&in, NULL, "bash", "-c", execname,  NULL); */
     }

     while (!feof(out))
     {
       char c;
       fread(&c,1,1,out);
       if (feof(out)) break;
       printf("%c",c);
     }
     Free(instring);

     if (global_options.output_files_set) {
       int tel;
       t_string_array * ina=StringDivide(global_options.output_files,";",TRUE,FALSE);

       for (tel=0; tel<ina->nstrings; tel++)
       {
	 int status;
         char * tmp;	 
	 char * name, *f;
	 if (diablosupport_options.prepend[0]!=0)
	 {
	   FATAL(("Implement"));
	   //name=StringConcat3(global_options.prepend,"/",tmp=ConvertFileName(ina->strings[tel]));
	   Free(tmp);
	 }
	 else
	 {
	   FATAL(("Implement"));
	   //name=ConvertFileName(ina->strings[tel]);
	 }
	 f=name;
	 name=StringConcat3(name," ", name);
	 Free(f);
	 f=name;
	 name=StringConcat3("diff -q -s ",name,"_ref");
	 Free(f);
	 
	 printf("====Exec %s\n",name);
	 /*CreateProcPipe(&pid,&out,&in,NULL, "bash", "-c", name, NULL); */
	 while (!feof(out))
	 {
	   char c;
	   fread(&c,1,1,out);
	   if (feof(out)) break;
	   printf("%c",c);
	 }
	 waitpid(pid,&status,0);

	 /* is non-zero if the child exited normally. */
	 if (WIFEXITED(status))
	 {
	   printf("EXIT STATUS OF DIFF: %d\n",WEXITSTATUS(status));
	 }
	 /*
	 else if ( WIFSIGNALED(status))
	 {
	   WTERMSIG(status)
	 }
	 */
	 else 
	 {
	   FATAL(("Diff crashed!"));
	 }

	 Free(name);
       }

       StringArrayFree(ina);

     }
}
#endif
#endif
/* }}} */


int main(int argc,char ** argv)
{
  t_object * obj;
  /* I. Initialize all structures {{{ */
  DiabloSupportInit(argc,argv); 

  /* Initialize (global) datastructures for archives */
  ArchivesInit();
  /* Initialize (global) datastructures for objects  */
  ObjectInit();
  /* }}} */

  MoreIo();

  OptionParseCommandLine (global_list, argc, argv, FALSE);
  OptionGetEnvironment (global_list);
  GlobalVerify();
  OptionDefaults (global_list);



  DiabloArInit();

  
  /* III. The REAL program {{{ */

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (global_options.restore || (global_options.restore_multi!=-1)) 
    global_options.objectfilename = RestoreDumpedProgram();


  if (global_options.read)
  {
    /* A. Get a binary to work with {{{ */
    
    if (global_options.restoreState || global_options.stegoDecode)
      obj=RestoreState();
    else 
      obj=LinkEmulate(global_options.objectfilename, FALSE);

    /* End Load }}} */


    if (global_options.print_data_layout)
      {
	t_object * subobj;
	t_object * tmp;
	t_section * sec;
	int tel;
	OBJECT_FOREACH_SUBOBJECT(obj,subobj,tmp)
	  OBJECT_FOREACH_SECTION(subobj,sec,tel)
	  {
	    if (SECTION_TYPE(sec)==DATA_SECTION)
	      VERBOSE(0,(".data 0x%x 0x%x\n",SECTION_OLD_ADDRESS(sec),SECTION_CSIZE(sec)));
	    else if (SECTION_TYPE(sec)==BSS_SECTION)
	      VERBOSE(0,(".bss 0x%x 0x%x\n",SECTION_OLD_ADDRESS(sec),SECTION_CSIZE(sec)));
	  }
	exit(0);
      }
    if (global_options.relayout_description)
    {
      ObjectMergeRODataSections(obj);
      ObjectMoveBssToDataSections(obj);
      ObjectRelayoutData(obj,global_options.relayout_description);
    }
    else 
    if ((global_options.disassemble) && (!(global_options.dump || global_options.dump_multiple)))
    {
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */ 
      
      ObjectAction(DISASSEMBLE,obj); 
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
	ObjectAction(FLOWGRAPH,obj);   

	/* Remove unconnected blocks {{{ */
	if (global_options.initcfgopt)
	  {
	    STATUS(START,("Removing unconnected blocks"));
	    CfgRemoveDeadCodeAndDataBlocks(T_CFG(obj->code[0]->data));
	    if (CfgPatchForNonReturningFunctions(T_CFG(obj->code[0]->data)))
	      CfgRemoveDeadCodeAndDataBlocks(T_CFG(obj->code[0]->data));
	    STATUS(STOP,("Removing unconnected blocks"));
	  }

	CfgPatchToSingleEntryFunctions(T_CFG(obj->code[0]->data));
	CfgRemoveDeadCodeAndDataBlocks(T_CFG(obj->code[0]->data));

	if (diabloflowgraph_options.blockprofilefile)
	  {   
	    CfgReadBlockExecutionCounts(T_CFG(obj->code[0]->data),diabloflowgraph_options.blockprofilefile);
	    printf("START WEIGHT %d\n",CfgComputeWeight(T_CFG(obj->code[0]->data)));
	    if (diabloflowgraph_options.insprofilefile)
	      {   
		CfgReadInsExecutionCounts(T_CFG(obj->code[0]->data),diabloflowgraph_options.insprofilefile);
		CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.90);
	      }
	  }



	/*	VERBOSE(0,("@eB\n@eB\n@eB\n@eB\n",T_CFG(obj->code[0]->data)->hell_node,T_CFG(obj->code[0]->data)->exit_hell_node,T_CFG(obj->code[0]->data)->call_hell_node,T_CFG(obj->code[0]->data)->exit_call_hell_node));
		exit(-1); */

	/*}}}*/
	/* Dump graphs prior to optimization {{{ */ 
	if (global_options.generate_dots)
	{
	  if (diabloflowgraph_options.blockprofilefile)
	    CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.90);
	  else
	    if(global_options.annotate_loops)
	    {
	      ComDominators(T_CFG(obj->code[0]->data));
	      Export_Flowgraph(T_BBL(T_CFG(obj->code[0]->data)->entries->entry_bbl), 0xffff, "./dots/flowgraph_loop.dot");
	    }

	  CfgDrawFunctionGraphsWithHotness(OBJECT_CFG(obj),"./dots");
	  CgBuild(T_CFG(obj->code[0]->data));
	  CgExport(T_CFG(obj->code[0]->data)->cg,"./dots/callgraph.dot");
	}
	/* }}} */
	/* {{{ dominator analysis */
	if (global_options.dominator) 
	  {
	    ComDominators(T_CFG(obj->code[0]->data));
	  }
	/* Export dots after dominator */
	if (global_options.dominator && global_options.generate_dots)
	{
	  t_function * function;


	  DirMake("./dots-dominator",FALSE);

	  Export_FunctionDominator((T_CFG(obj->code[0]->data))->unique_entry_node, 0xffff, "./dots-dominator/flowgraph.dot");

	  CFG_FOREACH_FUN(OBJECT_CFG(obj), function)
	  {
	    char * fname, *fname2;
	    if (function->bbl_first)
	    {
	      fname=sDiabloPrint("./dots-dominator/@G.func-%s.dot",BBL_OLD_ADDRESS(function->bbl_first),FUN_NAME(function));
	    }
	    else
	    {
	      fname=sDiabloPrint("./dots-dominator/0x0.func-%s.dot",FUN_NAME(function));
	    }
	    Export_FunctionDominator(function->bbl_first, ~ET_INTERPROC, fname);
	    Free(fname);
	  }
	}
	if (global_options.dominator) DominatorCleanup(T_CFG(obj->code[0]->data));
	/* }}} */
	/*{{{ ia64 stuff */
	/* }}}*/

#ifdef THUMB_SUPPORT
	if(global_options.thumbtoarm)
	  ThumbToArm(T_CFG(obj->code[0]->data));
#endif
	
	if (global_options.factoring && global_options.optimize)
	  WholeFunctionFactoring(T_CFG(obj->code[0]->data));



	/* Analyses && Optimizations  {{{ */
	for (global_optimization_phase = 0; global_optimization_phase < 3; global_optimization_phase++)
	  if (global_options.optimize)
	  {
	    int dead_count=0;
	    t_cfg * cfg = T_CFG(obj->code[0]->data);
	    int loopcount;
	    {
	      t_bool flag =  global_options.rely_on_calling_conventions;
	      global_options.rely_on_calling_conventions = global_optimization_phase < 2;
	      if (global_options.rely_on_calling_conventions!=flag)
		{
		  printf("CALLING CONVENTIONS CHANGED!!!\n");
		}


	    }



	    if (!global_options.rely_on_calling_conventions && global_options.factoring)
	    {
	      FunctionEpilogueFactoring(cfg);
	      CfgPatchToSingleEntryFunctions(cfg);
	      BblFactoring(cfg);
	    }

	      

	    CfgCreateExitBlockList(cfg);

    
	    /* always do this to make sure the live out sets of bbls are properly
	     * initialized, even if liveness is turned off */

	    CfgComputeLiveness(cfg,TRIVIAL);
	    
	    ObjectAction(CONVERT_DATA_TO_BLOCKS,obj);

	    loopcount = 0;

	    do
	    {
	      dead_count=0;

	      loopcount++;
	      if (global_options.remove_unconnected) 
		CfgRemoveDeadCodeAndDataBlocks(cfg);
	      
	      
	      if (!global_options.rely_on_calling_conventions)
		if (global_options.inlining)
		  {
		    DiabloBrokerCall("InlineTrivial",cfg);
		  }


	      /*ComputeDefinedRegs(cfg);*/



	      if (global_options.liveness) 
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

	      CfgDetectLeaves(cfg);

	      if (global_options.constant_propagation) 
	      {

		ConstantPropagation(cfg, CONTEXT_SENSITIVE);
		OptUseConstantInformation(cfg, CONTEXT_SENSITIVE);
		FreeConstantInformation(cfg);
	      }


	      if (global_options.liveness)
			  dead_count += CfgKillUselessInstructions (cfg);

     
	      if (global_options.dominator && global_options.loop_invariant_code_motion) 
		{
		  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
		  CfgComputeSavedChangedRegisters(*(obj->code));
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		  
		  if (diabloflowgraph_options.blockprofilefile)
		    {
		      CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.90);
		      CfgEstimateEdgeCounts(cfg);
		      
		      ComDominators(T_CFG(obj->code[0]->data));
		      CfgHoistConstantProducingCode(T_CFG(obj->code[0]->data));

#ifdef SPEEDUP_DOMINATORS
		      if (dominator_info_correct == FALSE)
#endif
			DominatorCleanup(T_CFG(obj->code[0]->data));
		      CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		      CfgEstimateEdgeCounts(cfg);
		    }

#ifdef SPEEDUP_DOMINATORS
		  if (dominator_info_correct == FALSE)
#endif
		    ComDominators(T_CFG(obj->code[0]->data));
		  CfgHoistConstantProducingCode3(T_CFG(obj->code[0]->data));
#ifdef SPEEDUP_DOMINATORS
		  if (dominator_info_correct == FALSE)
#endif
		    DominatorCleanup(T_CFG(obj->code[0]->data));
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		  
#ifdef SPEEDUP_DOMINATORS
		  if (dominator_info_correct == FALSE)
#endif
		    ComDominators(T_CFG(obj->code[0]->data));
		  DetectLoopStackSubAdds(cfg);
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		  DetectLoopInvariants(cfg);
#ifdef SPEEDUP_DOMINATORS
		  if (dominator_info_correct == FALSE)
#endif
		    DominatorCleanup(T_CFG(obj->code[0]->data));
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		  
		  if (diabloflowgraph_options.blockprofilefile)
		    {
		      CfgEstimateEdgeCounts(cfg);
		      CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.85);
#ifdef SPEEDUP_DOMINATORS
		      if (dominator_info_correct == FALSE)
#endif
			ComDominators(T_CFG(obj->code[0]->data));
		      LoopUnrollingSimple(T_CFG(obj->code[0]->data));
		      DominatorCleanup(T_CFG(obj->code[0]->data));
		      CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		    }
		}

	      if (global_options.copy_analysis) CopyAnalysis(*(obj->code));

	      if (global_options.copy_propagation) DiabloBrokerCall("OptCopyPropagation",cfg); /* only does something on the ARM */


	      TraceBblAtAddress(cfg,global_options.traceadr,"na_copy_propagatie");



	      if (global_options.loadstorefwd) DiabloBrokerCall("ArmLoadStoreFwd",cfg);
	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");
	      if (global_options.copy_analysis) CopyAnalysisFini(*(obj->code));

	      CfgRemoveUselessConditionalJumps(cfg);

	      CfgRemoveEmptyBlocks(cfg);


	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");
	      if (global_options.remove_unconnected) CfgRemoveDeadCodeAndDataBlocks(cfg);
	      CfgMoveInsDown(cfg);
	      CfgComputeSavedChangedRegisters(*(obj->code));
	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");



	      if (global_options.liveness) 
		{
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
		}
	      if (global_options.remove_unconnected) CfgRemoveDeadCodeAndDataBlocks(cfg);

	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");
	      if (global_options.branch_elimination) DiabloBrokerCall("BranchForwarding",cfg);
	      if (global_options.branch_elimination) DiabloBrokerCall("BranchElimination",cfg);
	      if (global_options.mergebbls) DiabloBrokerCall("MergeBbls",cfg);

	      if (global_options.pre)
		{
		  if (diabloflowgraph_options.blockprofilefile)
		    {
		      CfgEstimateEdgeCounts(cfg);
		      PartialRedundancyElimination1(cfg);
		      PartialRedundancyElimination1ForCalls(cfg);
		      PartialRedundancyElimination1ForReturns(cfg);
		    } 

		  CfgComputeLiveness(cfg,TRIVIAL);
		  CfgComputeSavedChangedRegisters(*(obj->code));
		  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
		  CfgComputeSavedChangedRegisters(*(obj->code));
		  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

		  ReplaceTriangleWithConditionalMove(cfg);

		  if (diabloflowgraph_options.blockprofilefile)
		    {
		      PartialRedundancyElimination2(cfg);
		      PartialRedundancyElimination3(cfg);

		      if (diabloflowgraph_options.insprofilefile)
			{   
			  CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.95);
			  DetectColdCodeBundles(T_CFG(obj->code[0]->data));
			}
		      CfgEstimateEdgeCounts(cfg);
		  
		      CfgBranchSwitch(cfg);
		      CfgBranchSwitch2(cfg);
		      CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.90);
		      CfgEstimateEdgeCounts(cfg);
		    }
		}
	      
	      if (!global_options.rely_on_calling_conventions)
		{
		  if (global_options.inlining)
		    {
		      DiabloBrokerCall("GeneralInlining",cfg); /* Arm only */
		      DiabloBrokerCall("I386InlineSimple",cfg);
		      /*DiabloBrokerCall("I386InlineFunctionsWithOneCallSite",cfg);*/
		    }
		}

		      

	      DetectStackAliases(T_CFG(obj->code[0]->data));

	      if (global_options.peephole) DiabloBrokerCall("I386PeepHoles",T_CFG(obj->code[0]->data));


	      if (global_options.peephole) DiabloBrokerCall("ArmPeepholeOptimizations",T_CFG(obj->code[0]->data));
	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");
	      
      
	      if (global_options.stack) 
		DiabloBrokerCall("OptimizeStackLoadAndStores",cfg); /* only for the ARM */
	      /*DiabloBrokerCall("InstructieOptimalisatie",cfg); |+ only for the MIPS +|*/
	      CfgComputeSavedChangedRegisters(*(obj->code));

	      if (global_options.liveness)
		dead_count += CfgKillUselessInstructions (cfg);

	      TraceBblAtAddress(cfg,global_options.traceadr,"HIER");
	      
	    } while(dead_count);
	  }

	{
	  dominator_info_correct = FALSE;
	  DominatorCleanup(T_CFG(obj->code[0]->data));
	}


	/* End of Analyses && Optimizations  }}} */
 
	if (diabloflowgraph_options.blockprofilefile)
	  printf("END WEIGHT %d\n",CfgComputeWeight(T_CFG(obj->code[0]->data)));

	/* Export dots after optimzation {{{  */
	if (global_options.generate_dots)
	{
	  if (diabloflowgraph_options.blockprofilefile)
	    CfgComputeHotBblThreshold(T_CFG(obj->code[0]->data),0.90);
	  CfgDrawFunctionGraphsWithHotness(OBJECT_CFG(obj),"./dots-final");
	  CgBuild(T_CFG(obj->code[0]->data));
	  CgExport(T_CFG(obj->code[0]->data)->cg,"./dots-final/callgraph.dot");
	}
	/* }}} */

	
	/* Back to the section representation (still disassembled though) */
  
	if (1)
	  {
	    int nr_ins = 0;
	    t_bbl * bbl;
	    CFG_FOREACH_BBL(T_CFG(obj->code[0]->data),bbl)
	      nr_ins+=BBL_NINS(bbl);
	    printf("nr_ins %d\n",nr_ins);
	  }
      
	
	ObjectAction(DEFLOWGRAPH,obj);  

	/* rebuild the layout of the data sections
	 * so that every subsection sits at it's new address */
	ObjectRebuildSectionsFromSubsections(obj);
      } /*  }}} */

      if (global_options.saveState)
	SaveState(obj,SAVESTATEDISASSEMBLED);

      ObjectAction(ASSEMBLE,obj); 
      /* End Transform and optimize }}} */ 
    }
    else if (global_options.dump || global_options.dump_multiple)
    {
#ifdef HAVE_DIABLO_EXEC
      if (global_options.exec) DiabloExec();
#else
      if (global_options.exec) FATAL(("Exec not compiled!"));
#endif
      DumpProgram(obj,global_options.dump_multiple);
      exit(0);
    }
    else if (global_options.saveState)
    {
      SaveState(obj,SAVESTATENOTDISASSEMBLED); 
    }



    ObjectAction(WRITE,obj,global_options.output_name);  

#ifdef DIABLOSUPPORT_HAVE_STAT
  /* make the file executable */
  chmod(global_options.output_name,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
#endif

  }
  /* END REAL program }}} */

  if (global_options.exec)
  {
#ifdef HAVE_DIABLO_EXEC
    DiabloExec();
#else
    FATAL(("Exec not compiled!"));
#endif
  }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP");*/
  /* Deallocate global error variables (we can still use
   * Error (e.g. for leak detection, but the error
   * message will no longer contain the program name */ 
  /* Free all the paths that are stored in the options and
   * the default objectname: The rest of the options (like
   * the verbose-level remains the same*/
  /* Free the archive cache */
  ArchivesFini();
  /* Free the object cache */
  /* XXX: this is an ugly hack, but turning off ObjectFini if we saved the state or haven't flowgraphed fixes a messy segfault*/
  ObjectFini();
  /* Free initializers used in path expansion (such as the
   * users home directory */
  PathFini();   
  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */ 

#ifdef DEBUG_MALLOC
  global_options.verbose++;
  PrintRemainingBlocks();
#endif
  /* End Fini }}} */
  return 0;
}
/* vim: set shiftwidth=2 foldmethod=marker:*/

