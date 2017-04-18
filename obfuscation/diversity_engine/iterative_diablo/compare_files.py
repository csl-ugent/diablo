# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging

import benchmarks
import diablo
import diablo_iterative
import experiments
import feedback_round
from   feedback_round import FeedbackRound, singleRound, untilRound
import ida
import instruction_counts
import iterative_rounds
import multiplatform
import os

seed  = 11
#extra = "_suffix_2"
extra = "_better_cost_tracking_hipeac_rules_faster_start_oom"

# matchers = [ ida.BinaryDiffer, ida.TurboDiff, ida.BinDiff, ida.PatchDiff2 ]
# matchers  = [ ida.BinaryDiffer, ida.TurboDiff, ida.BinDiff ]
matchers  = [ ida.BinDiff ]
analyzers = matchers

config_simple = diablo.SimpleConf()
config_simple.experiment_dir = "exploratory/feedback_driven_scripted"
config_simple.extend_IDA_analysis = False

config_extended = diablo.SimpleConf()
config_extended.extend_IDA_analysis = True

rounds = [
		   FeedbackRound( (benchmarks.bzip2, 1), singleRound(0), [],    None),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(1), [0],   None),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(2), [0],   None),
		   FeedbackRound( (benchmarks.bzip2, 2), untilRound(20),  [0],  None),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(11), [0,10], None),
		   FeedbackRound( (benchmarks.bzip2, 2), untilRound(20), [0,10], None)
		 ]

# REDEFINE
rounds = iterative_rounds.regular_soplex


benchmark_0                    = rounds[0].benchmark
config_simple.experiment_dir  += "/" + str(seed) + "_" + benchmark_0.exe + extra
config_extended.experiment_dir = config_simple.experiment_dir

experiment_simple   = experiments.Experiment(config_simple,   benchmark_0, diablo_iterative.binary_iteration_filename)
experiment_extended = experiments.Experiment(config_extended, benchmark_0, diablo_iterative.binary_iteration_filename)


#experiments = [ experiment_simple, experiment_extended ]
#iterations  = range(0, 2)

### ###
experiment_extended.binary_suffix = "_bindiff_oom"

experiments = [ experiment_extended ]
iterations  = [0, 18] #range(18, 19)

localDir = multiplatform.asLocal(experiment_simple.path)
os.chdir(localDir)
logging.basicConfig(filename='compare_files.log', format='%(asctime)s %(levelname)s %(message)s', level=logging.DEBUG)

def get_round_and_fix_experiment(current_iteration, rounds, experiment):
  feedbackRound = feedback_round.getFirstMatchingFeedbackRoundFromList(current_iteration, rounds)
  # This is somewhat fugly ###
  experiment.benchmark = feedbackRound.benchmark
  experiment.benchmark.version = feedbackRound.benchmarkVersion
  
  return feedbackRound
  

def match( experiments, iterations, rounds ):
  for experiment in experiments:
	for current_iteration in iterations:
	  feedbackRound = get_round_and_fix_experiment(current_iteration, rounds, experiment)

	  logging.info("Dumping functions for iteration %i", current_iteration)
	  # ### ida.dump_functions(experiment, current_iteration)

	  for matcher in matchers:
		logging.info("Potentially doing analysis for matcher %s on iteration %i", str(matcher), current_iteration)
		matcher.analyze(experiment, current_iteration)

		for orig in feedbackRound.compareWith:
		  logging.info("Matching %i with %i using %s", orig, current_iteration, str(matcher))

		  matcher.match(experiment, orig, current_iteration)

		  logging.debug("... Matched")


def find_changes( experiments, iterations, rounds ):
  for experiment in experiments:
	for current_iteration in iterations:
	  feedbackRound = get_round_and_fix_experiment(current_iteration, rounds, experiment)

  	  if len(feedbackRound.compareWith) == 0:
		continue

	  helper = ida.matcher.BinaryMatchHelper(experiment, current_iteration)

	  for matcher in matchers:
		for orig in feedbackRound.compareWith:
		  logging.info("Looking at feedback of comparing %i with %i using %s", orig, current_iteration, str(matcher))

		  found = ida.matcher.semantic_changes_found_count_filters(helper, matcher, experiment, orig, current_iteration, matcher.filters)

		  logging.debug("... Done")

		  for chain in found:
			log = "%s maps to: %i total, %i semantic changes" % (chain, found[chain].found_all,  found[chain].found_changes)
			print log
			logging.info(log)

ida.run_Ida("soplex_iterated_binary_0_bindiff_oom", extend_analysis=True, path=experiments[0].path)

match( experiments, iterations, rounds )

#find_changes( experiments, iterations, rounds )

#instruction_counts.traceBenchmark(diablo_iterative.binary_iteration_filename(experiment_simple, 5), benchmark_0, "train")
