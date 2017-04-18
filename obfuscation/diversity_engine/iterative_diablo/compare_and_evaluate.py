#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import os
import sys

import benchmarks
import diablo
import diablo_normalization
import experiments
import multiplatform
import ida

# Code to evaluate the framework for normalization

# Specific paths for these experiments:

# experiments.setGlobalBinaryPathBase("/home/bcoppens/private/software/diversity/experiments/diversity/normalization/diablodiota/test/")
base_path_proteus = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/bzip2/"
base_path_glaucus = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/"
experiments.setGlobalBinaryPathBase(base_path_glaucus)
#experiments.setGlobalBinaryPathBase(base_path_proteus)

settings.set_settings(settings_forcelayout)

matchers  = [ ida.BinDiff ]
analyzers = matchers

#tmp_benchmark = benchmarks.Benchmark("bzip2", "path_to_original_sources", { 1: "o22", 2: "p22" }, { } )
tmp_benchmark = benchmarks.Benchmark("bzip2", "path_to_original_sources", { 1: "soplex_iterated_binary_0", 2: "soplex_iterated_binary_20" }, { "test": [ [ "test.mps" ] ] } )

logging.basicConfig(filename='compare_files_extensive_iterative_inlining_soplex.log', format='%(asctime)s %(levelname)s %(message)s', level=logging.DEBUG)

def binary_iteration_filename(experiment, iteration):
  #return "%s_%i" % (experiment.benchmark.exe, iteration)
  #return experiment.benchmark.versions[iteration]
  return "%s_v%i" % (experiment.shortname, iteration)

def unnormalized_filename(experiment, iteration):
  return experiment.benchmark.versions[iteration]

def match(experiment, v1, v2, matchers):
  localDir = multiplatform.asLocal(experiment.path)
  os.chdir(localDir)
  
  
  for i in [v1, v2]:
	exe = binary_iteration_filename( experiment, i )
	ida.run_Ida(exe, extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)

  matchers[0].match(experiment, v1, v2)

def evaluate(experiment, v1, v2, matchers):
	matches_v2 = matchers[0].get_instructions(experiment, v1, v2, which_version=v2)
	
	exe_v2 = binary_iteration_filename( experiment, v2 )
	instructions_v2 = diablo.all_insts(multiplatform.dir_entry(experiment.path, "%s.xml" % exe_v2))
	
	#full_name = "%s, %s" % (setting, experiment.name)
	full_name = experiment.name
	logging.info("===> SIZE FOR EXPERIMENT %s: %i" % (full_name, len(matches_v2)))
	logging.info("Instructions in binary: %i, matched: %i" % (len(instructions_v2), len(matches_v2)))
	logging.info("===> PRUNING RATE FOR EXPERIMENT %s: %f (tot. abs. insts. count: %i)" % (full_name, (float(len(matches_v2)) / float(len(instructions_v2))), len(instructions_v2)))


options = [ "RemoveNotExecuted", "RemoveDataSections", "NormalizeJcc", "DynamicCalls",
			"JumpFunctions", "JumpFunctionRemovePush", "Liveness", "OpaquePredicates", "SortDynamicTargets",
			"Optimize",  "MergeBBLs",
			#"Inlining", "BranchElimintation", "DynamicJumps", "MultipleIfTarges"  ] #, "NormalizeInstructions", "Peephole", "FixLayout" ]
			"Inlining", "DynamicJumps", "MultipleIfTarges",  "Peephole"  ] #, "NormalizeInstructions", "Peephole", "FixLayout" ]


config_simple = diablo_normalization.NormalizationConf(trace_prefix="")
config_simple.extend_IDA_analysis = False
config_simple.experiment_dir = ""

config_extended = diablo_normalization.NormalizationConf(trace_prefix="")
config_extended.extend_IDA_analysis = True
config_extended.experiment_dir = ""

def get_traces(vs):
  experiment = experiments.Experiment(config_simple,   tmp_benchmark, binary_iteration_filename)
  for v in vs:
	experiment.config.trace_prefix = "trace%s" % unnormalized_filename( experiment, v )
	diablo_normalization.generate_dynamic_info(experiment, unnormalized_filename( experiment, v ))

# get_traces([1,2]) # For now, we just experiment with different normalizations on fixed 2 soplex binaries

name = ""
smallname = ""
binary_pair = 0

only_do = None
#only_do = [ 15 ]
#only_do = [ 11, 12, 13, 14, 15, 16, 17 ]
#only_do = [ 13, 14, 15, 16, 17, 18 ]
#only_do = range(26, 36)

for option in options:
  name += option
  smallname += diablo_normalization.diablo_transforms_mapper[option]
  
  config_simple.enable(option)
  config_extended.enable(option)
  
  #for data_ok in [ True ]: # [ True, False ]:
  data_ok = True
  for doDyninst in [ False, True ]:
	if data_ok:
	  currentname = name + "PlaceDataCorrectly"
	  currentsmallname = smallname + "-dataok"
	  config_simple.enable("PlaceDataCorrectly")
	  config_extended.enable("PlaceDataCorrectly")
	else:
	  currentname = name + "PlaceDataInCorrectly"
	  config_simple.disable("PlaceDataCorrectly")
	  config_extended.disable("PlaceDataCorrectly")
	  currentsmallname = smallname + "-datanok"
	
	if doDyninst:
	  currentname = name + "DYNINST"
	  currentsmallname = smallname + "-DYNINST"
	else:
	  currentname = name + "NOdyninst"
	  currentsmallname = smallname + "-NOdyninst"

	
	binary_pair += 1
	logging.info("BINARYPAIR,%i,%s" % (binary_pair, currentname))

	if only_do is not None and binary_pair not in only_do:
	  logging.info("Skipping actual code generation")
	  continue

	experiment_simple   = experiments.Experiment(config_simple,   tmp_benchmark, binary_iteration_filename)
	experiment_simple.shortname = "exp%i_basic" % binary_pair # currentsmallname + "_basic"
	experiment_simple.name = currentname + "BasicDisassembly"
	experiment_extended = experiments.Experiment(config_extended, tmp_benchmark, binary_iteration_filename)
	experiment_extended.name = currentname + "ExtendedDisassembly"
	experiment_extended.shortname = "exp%i_extended" % binary_pair # = currentsmallname + "_extended"
  
	experimentslist = [ experiment_extended ]
	
	for experiment in experimentslist:
	  for v in [ 1, 2 ]:
		out_binary = binary_iteration_filename(experiment, v)
		experiment.config.trace_prefix = "trace%s" % unnormalized_filename( experiment, v )
		
		# TODO: run dyninst!
		if doDyninst:
		  experiment.config.dyninst_prefix = "dyninst%s" % unnormalized_filename( experiment, v )
		else:
		  experiment.config.dyninst_prefix = None

		diablo_normalization.runNormalization(experiment, unnormalized_filename( experiment, v ), out_binary, out_binary)

	  match(experiment, 1, 2, matchers)
	  evaluate(experiment, 1, 2, matchers)



sys.exit(0)

#def binary_iteration_filename(experiment, iteration):
  ##return "%s_%i" % (experiment.benchmark.exe, iteration)
  #return experiment.benchmark.versions[iteration]


for setting in [ "stringsfout", "stringsjuist" ]:
  config_simple.experiment_dir = setting
  config_extended.experiment_dir = config_simple.experiment_dir

  experiment_simple   = experiments.Experiment(config_simple,   tmp_benchmark, binary_iteration_filename)
  experiment_simple.name = "Basic Disassembly"
  experiment_extended = experiments.Experiment(config_extended, tmp_benchmark, binary_iteration_filename)
  experiment_extended.name = "Extended Disassembly"
  
  experimentslist = [ experiment_simple, experiment_extended ]
  
  for experiment in experimentslist:
	localDir = multiplatform.asLocal(experiment.path)
	os.chdir(localDir)

	#for matcher in matchers:
	  #logging.info("Potentially doing analysis for matcher %s on iteration %i", str(matcher), current_iteration)
	  #matcher.analyze(experiment, current_iteration)

	  #for orig in feedbackRound.compareWith:
		#logging.info("Matching %i with %i using %s", orig, current_iteration, str(matcher))

		#matcher.match(experiment, orig, current_iteration)

		#logging.debug("... Matched")
		#found = ida.matcher.semantic_changes_found_count_filters(helper, matcher, experiment, orig, current_iteration, matcher.filters)

		#logging.debug("... Done")

		#for chain in found:
		  #log = "%s maps to: %i total, %i semantic changes" % (chain, found[chain].found_all,  found[chain].found_changes)

	for i in [1, 2]:
	  exe = binary_iteration_filename( experiment, i )
	  ida.run_Ida(exe, extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)

	matchers[0].match(experiment, 1, 2)
	
	matches_v2 = matchers[0].get_instructions(experiment, 1, 2, which_version=2)
	
	v2 = binary_iteration_filename( experiment, 2 )
	instructions_v2 = diablo.all_insts(multiplatform.dir_entry(experiment.path, "%s.xml" % v2))
	
	full_name = "%s, %s" % (setting, experiment.name)
	logging.info("===> SIZE FOR EXPERIMENT %s: %i" % (full_name, len(matches_v2)))
	logging.info("Instructions in binary: %i, matched: %i" % (len(instructions_v2), len(matches_v2)))
	logging.info("===> PRUNING RATE FOR EXPERIMENT %s: %f (tot. abs. insts. count: %i)" % (full_name, (float(len(matches_v2)) / float(len(instructions_v2))), len(instructions_v2)))
