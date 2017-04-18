#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy
import logging
import os
import shutil     as     shutil
import sqlite3    as     sql
import sys
import traceback

import diablo
import diablo_iterative

import benchmarks
import experiments
import feedback_round
from   feedback_round import FeedbackRound, singleRound, untilRound
import ida
import instruction_counts
import iterative_rounds
import multiplatform

transformation_rules = "/home/bcoppens/transformation_rules"

class DiversityConfig(diablo.DiversityConfigBase):
  def __init__(self):
	diablo.DiversityConfigBase.__init__(self)
	
	self.initial_seed = 145
	self.experiment_dir = "CHANGEMELATER"

def bindiff_string_to_id(string):
  """The problem is, that the order of entries in BinDiffDeluxe.xml determines the ID it returns in the DB. While we actually use the ID as though it were fixed.
     So we'll use transform the string into a fixed number. """
  table = {
	"function: name hash matching": 1,
	"function: hash matching": 2,
	"function: edges flowgraph MD index": 3,
	"function: edges callgraph MD index": 4,
	"function: MD index matching (flowgraph MD index, top down)": 5,
	"function: MD index matching (flowgraph MD index, bottom up)": 6,
	"function: prime signature matching": 7,
	"function: MD index matching (callGraph MD index, top down)": 8,
	"function: MD index matching (callGraph MD index, bottom up)": 9,
	"function: edges proximity MD index": 10,
	"function: relaxed MD index matching": 11,
	"function: instruction count": 12,
	"function: address sequence": 13,
	"function: string references": 14,
	"function: loop count matching": 15,
	"function: call sequence matching(exact)": 16,
	"function: call sequence matching(topology)": 17,
	"function: call sequence matching(sequence)": 18,
	"function: call reference matching": 19 }

  return table[string]

def save_bindiff_feedback(experiment, iteration, currentRound, compareWith):
  out = open(experiment.config.matched_instructions_and_why_file, "w")
  con = None
  
  logging.info("Saving the information from this round into %s", experiment.config.matched_instructions_and_why_file)

  to_orig = diablo.transformed_to_orig_map(diablo.map_filename(iteration))
  
  logging.info("Log file %s" % diablo.map_filename(iteration))
  logging.info("%s" % str(to_orig))

  bindiffs = diablo_iterative.all_comparison_bindiff_files(experiment, compareWith, iteration, currentRound)

  for bindiff in bindiffs:
	logging.info("Adding the results of BinDiff file %s", bindiff)
	try:
	  con = sql.connect(bindiff)

	  cur = con.cursor()    
	  cur.execute("SELECT functionalgorithm.name, instruction.address2 FROM instruction LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id;")
	  rows = cur.fetchall()
  
	  for row in rows:
            logging.info(str(row))
            if int(int(row[1])) in to_orig:
                  logging.info("Ok")
                  orig = to_orig[int(row[1])]
                  why  = bindiff_string_to_id(row[0])
                  
                  out.write( "0x%x,%i\n" % (orig, why) )

	except sql.Error, e:
	  print "SQLError in save_bindiff_feedback %s:" % str(e)
	  print str(bindiff)
	  traceback.print_exc()
	  sys.exit(1)

	finally:
	  if con:
		con.close()

  out.close()

def clean_feedback_round(experiment, r):
  for f in [ experiment.config.allowed_transforms_per_function_file, experiment.config.matched_instructions_and_why_file ]:
	if f is not None:
	  open(f, "w").close()


def regular_single_feedback_action(experiment, src_iteration, r, feedbackRound, currentRound):
  if r == 0: ### TODO
	clean_feedback_round(experiment, r)

  diablo_iterative.single_iteration_with_comparisons(src_iteration, r, currentRound, experiment, just_diablo=False, compare_with=feedbackRound.compareWith)

  experiment.config.matched_instructions_and_why_file           = "matched_instructions_and_why_file.%i" % (r + 1)

  save_bindiff_feedback(experiment, r, currentRound, feedbackRound.compareWith)


def just_redo_bindiff_action(experiment, src_iteration, r, feedbackRound):
  if r == 0: ### TODO
	clean_feedback_round(experiment, r)
	ida.run_Ida(diablo_iterative.binary_iteration_filename(experiment, r), extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)


  for orig in feedbackRound.compareWith:
	  logging.debug("Matching with: %i", orig)
	  ida.BinDiff.match(experiment=experiment, v1=orig, v2=r)


def just_redo_diablo_feedback_action(experiment, src_iteration, r, feedbackRound):
  # since we re-do just diablo, compare_with should be empty -> otherwise it would try to run BinDiff
  diablo_iterative.single_iteration_with_comparisons(r, experiment, just_diablo=True, compare_with=[], ida_when_no_comparisons=False, copy_output_when_no_comparisons=False)

# infoFromIteration is the source iteration, for example when we need to skip back to the input of an old generation
def single_feedback_round(experiment, r, feedbackRound, feedbackAction, infoFromIteration=None, currentRound=None):
  if infoFromIteration is None:
	infoFromIteration = r # Default

  assert currentRound is not None

  config = experiment.config

  if config.usesTransformPerFunctionFile:
	config.allowed_transforms_per_function_file        = "allowed_transforms_per_function_output_file.%i" % (infoFromIteration)
	config.matched_instructions_and_why_file           = "matched_instructions_and_why_file.%i"           % (infoFromIteration)
  else:
	config.allowed_transforms_per_function_file        = None
	config.matched_instructions_and_why_file           = None

  config.allowed_transforms_per_function_output_file = "allowed_transforms_per_function_output_file.%i" % (r + 1)
  
  feedbackAction(experiment, infoFromIteration, r, feedbackRound, currentRound)

# What happens: we have a list of 'steps', and a round is the 'best' result of a step
class Round:
  @staticmethod
  def filename(r):
	return "round_information.%i" % r

  def __init__(self):
	self.round = None
	self.seed = None
	self.startStep = None
	self.startSeed = None
	self.stopStep = None
	self.stopSeed = None
	self.sameAsIteration = None # In this case, we restart from the mentioned round, but with round_nr increased (if we could not find a better random seed, for example)
  def hasStop(self):
	return self.stopStep is not None and self.stopSeed is not None
  def hasStart(self):
	return self.startStep is not None and self.startSeed is not None

  def isValid(self):
	if not self.hasStart():
	  return False
	logging.info("%s" % str(self.startStep))
	if self.startStep == -1 or self.startSeed == -1 or self.stopStep == -1 or self.stopSeed == -1:
	  return False
	return True

  def writeField(self, file, classField, fileField):
	print classField
	print self.__dict__
	assert classField in self.__dict__
	if self.__dict__[classField] is not None:
	  file.write("%s=%i\n" % (fileField, self.__dict__[classField]))

  def write(self, r):
	file = open(Round.filename(r), "w")
	
	self.writeField(file, "startStep", "start_step")
	self.writeField(file, "startSeed", "start_seed")
	self.writeField(file, "stopStep", "stop_step")
	self.writeField(file, "stopSeed", "stop_seed")
	self.writeField(file, "sameAsIteration", "same_as_iteration")
	
	file.close()

def readRoundInfo(r):
  """ If possible, skips to the end round of this iteration """
  # File format, file is 'round_information.<r>'
  # start_step=<n>
  # start_seed=<n>
  # [stop_step=<n>]
  # [stop_seed=<n>]
  # [same_as_iteration=<n>]
  
  info = Round()
  
  try:
	for line in open(Round.filename(r), "r"):
	  s = line.split("=")
	  if len(s) != 2:
		continue

	  key   = s[0]
	  value = int(s[1])
	  
	  # if a key is in here twice, it is suspicious: remove it!
	  def update(info, var, value): # IEW ICK? CHECK
		if var in info.__dict__ and info.__dict__[var] is not None:
		  info.__dict__[var] = -1 # uhmmm ick
		else:
		  info.__dict__[var] = value

	  if key == "start_step":
		update(info, "startStep", value)
	  if key == "start_seed":
		update(info, "startSeed", value)
	  if key == "stop_step":
		update(info, "stopStep", value)
	  if key == "stop_seed":
		update(info, "stopSeed", value)
	  if key == "same_as_iteration":
		update(info, "sameAsIteration", value)
	logging.info("%s" % str(info.hasStart()))
	if not info.isValid():
	  return None
	info.round = info.stopStep # TODO ditto als hieronder
	info.seed = info.stopSeed # TODO correct als we skippen???
	return info

  except:
	logging.info("Reading failed!")
	return None # reading failed

def skipToRound(r):
  """ Skips to the END round of this iteration """
  # We want to ensure that the path from round 0 to r is actually consisting of valid round infos
  info = None
  for i in range(1, r + 1):
	info = readRoundInfo(r)
	if info is None:
	  logging.info("No correct info for iteration %i" % i)
	  return None
  return info

# This action emulates the old round behavior, that is, there is none: every iteration is executed at most once
def oldRoundAction(experiment, current_iteration, roundInfo):
  assert False # Should not be used
  roundInfo.stopStep = 1
  roundInfo.round = 1
  return False

# TODO WTF DOET DOET?
def getFilteredInstructions(experiment, compare_version, version):
  to_filer = diablo_iterative.getPostProcessInstsToRemove(version)
  insts = ida.BinDiff.get_instructions(experiment, compare_version, version)
  
  return insts - to_filer

# Returns boolean == run with new round? -> False == next iteration please!
def roundActionBasedOnInstructionsMatchedBase(experiment, feedbackRound, current_iteration, roundInfo, maxrounds):
  exe = experiment.as_binary_filename(experiment, experiments.Version(current_iteration, roundInfo.round))
  
  logging.info("Round action: compare with %s" % str(feedbackRound.compareWith))
  
  if feedbackRound.compareWith == []:
	# There are no comparisons to be done!
	roundInfo.stopStep = roundInfo.round
	roundInfo.stopSeed = roundInfo.seed
	return False
  
  # TODO: Option to use other matchers?
  # TODO THIS IS HORRIBLY WRONG WITH > 1 comparison! FIXME
  # TODO what if the comparisons for the previous iteration have changed with the current one???? FIXME
  
  assert len(feedbackRound.compareWith) == 1
  compare_version = experiments.Version(feedbackRound.compareWith[0], "final")
  
  prev_iteration = current_iteration - 1
  
  # FIXME TODO see? it already begins, ugh
  if prev_iteration == 0:
	roundInfo.stopStep = roundInfo.round
	roundInfo.stopSeed = roundInfo.seed
	return False

  
  prev_version = experiments.Version(prev_iteration, "final")

  current_version = experiments.Version(current_iteration, roundInfo.round)
  
  # TODO WTF
  #insts_prev_iteration = getFilteredInstructions(experiment, compare_version, prev_version)
  #insts_cur_round = getFilteredInstructions(experiment, compare_version, current_version)
  
  insts_prev_iteration = ida.BinDiff.get_instructions(experiment, compare_version, prev_version)
  insts_cur_round = ida.BinDiff.get_instructions(experiment, compare_version, current_version)
  
  
  logging.info("Round decision taking: round nr %i, max %i" % (roundInfo.round, maxrounds))
  logging.info("Round decision taking: insts prev iteration: %i, insts current round %i" % (len(insts_prev_iteration), len(insts_cur_round)))
  
  #maxrounds = None
  if maxrounds is not None and roundInfo.round > maxrounds:
	roundInfo.sameAsIteration = prev_iteration
	roundInfo.stopStep = roundInfo.round
	roundInfo.stopSeed = roundInfo.seed
	return False
  if len(insts_prev_iteration) > len(insts_cur_round):
	roundInfo.stopStep = roundInfo.round
	roundInfo.stopSeed = roundInfo.seed
	roundInfo.sameAsIteration = None
	return False

  roundInfo.round += 1 # Next round needed
  roundInfo.seed += 1
  return True
  
def roundActionBasedOnInstructionsMatchedMaxRounds(maxrounds):
  return lambda e, f, c, r: roundActionBasedOnInstructionsMatchedBase(e, f, c, r, maxrounds)

# TODO: rounds is confusingly named with roundAction etc now
def single_feedback_test_raw(maxcost, rounds, start, stop, seed, actions, roundAction, extra="", binary_suffix=""):
  """ stop is not inclusive """
  config = DiversityConfig()
  
  config.experiment_dir = "" # "exploratory/feedback_driven_scripted/hipeac_revision"

  config.extend_IDA_analysis = True
  config.allowed_transforms_per_function_output_file = "allowed_transforms_per_function_output_file.base"

  # We initialize the experiment with the basic config. This will set the path. Later on we might change the config, but the experiment's path will remain.
  benchmark_0            = rounds[0].benchmark
  config.experiment_dir += "/" + str(seed) + "_" + benchmark_0.exe + extra
  experiment             = experiments.Experiment(config, benchmark_0, diablo_iterative.binary_iteration_filename)
  
  config.profile_file    = benchmark_0.profile_file
  config.isThumb2        = benchmark_0.isThumb2

  experiment.binary_suffix = binary_suffix

  localDir = multiplatform.asLocal(experiment.path)
  try:
	os.makedirs(localDir)
  except OSError, e:
	print "Exception: %s" % str(e)

  os.chdir(localDir)

  logging.basicConfig(filename='feedback_driven.log', format='%(asctime)s %(levelname)s %(message)s', level=logging.DEBUG)

  logging.info("Using transformations rule: %s, contents are:" % transformation_rules)
  f = open(transformation_rules)
  for line in f:
    logging.info("%s" % line)
  f.close()
  logging.info("End of transformations rule contents")
  
  config.transformation_rules = transformation_rules

  previous_benchmark = (None, None)

  logging.info("Iterating from %i to %i", start, stop)
  
  # TODO RESTART FROM ROUND! Implement!
  roundInfo = 0
  start = -1
  #roundInfo = 1
  #start = 3 # TODO TODO remove again
  
  if roundInfo == 0:
	roundInfo = Round()
	roundInfo.round = 1
	roundInfo.seed = seed
	roundInfo.startStep = 0
	roundInfo.startSeed = seed
  else:
	roundInfo = skipToRound(start) # Skips to the END of this iteration => we start a NEW iteration after loading this!
	assert roundInfo
	#while roundInfo.same_as_round is not None:
	#  roundInfo = skipToRound(roundInfo.same_as_round)
	#  assert roundInfo

  # TODO REMOVE?
  start += 1

  for current_iteration in range(start, stop):
	
	feedbackRound     = feedback_round.getFirstMatchingFeedbackRoundFromList(current_iteration, rounds)
	current_benchmark = feedbackRound.benchmarkTuple
	
	experiment.benchmark = feedbackRound.benchmark
	
	logging.info("Iteration %i, with settings %s comparing against iterations %s", current_iteration, str(feedbackRound.transformConfig), str(feedbackRound.compareWith) )
	
	if previous_benchmark != current_benchmark:
	  logging.info("Initializing directory for Benchmark %s/%i", str(feedbackRound.benchmark), feedbackRound.benchmarkVersion)
	  experiment.initialize_dir(feedbackRound.benchmarkVersion, config)

	  previous_benchmark = current_benchmark

	# All rounds for this iteration depend on the same iterative input
	if current_iteration > 0: ### TODO
		shutil.copy( feedbackRound.benchmark.exe + ".iterative_output.%i" % (current_iteration - 1), feedbackRound.benchmark.exe + ".iterative_input.%i" % current_iteration )

	doNewRound = True
	roundInfo.round = 1
	roundInfo.startSeed = roundInfo.seed
	src_iteration = current_iteration # Confusingly not with - 1, TODO?
	# Keep the seed over the different iterations, tracking changes due to extra rounds...
	
	while doNewRound:
	  logging.info("Starting round %i" % roundInfo.round)
	  exe = experiment.as_binary_filename(experiment, experiments.Version(current_iteration, roundInfo.round))
	  
	  experiment.config    = feedbackRound.transformConfig(config, roundInfo.seed)

	  for action in actions:
		action(experiment, src_iteration, current_iteration, feedbackRound, exe, roundInfo.round)
	
          logging.info("Before roundaction: %s" % str(roundInfo.seed))
	  doNewRound = roundAction(experiment, feedbackRound, current_iteration, roundInfo)
	  logging.info("After roundaction: %s" % str(roundInfo.seed))
	  logging.info("Should re-run round? %s" % str(doNewRound))
	assert roundInfo.round > 0
	assert roundInfo.stopStep is not None
	roundInfo.write(current_iteration)
	
	# THIS IS FUGLY, REDO! Copy output files, rather than re-running!
	# TODO in case this version is NEVER beter than the previous, copy the previous files, not current ones
	if roundInfo.sameAsIteration is not None:
          logging.info("Roundinfo same as iteration? %s" % str(roundInfo.sameAsIteration))
	  # TODO JUST COPY THE FILES! or refactor!
	  # reset the info to re-create a previous iteration
	  feedbackRound     = feedback_round.getFirstMatchingFeedbackRoundFromList(roundInfo.sameAsIteration, rounds)
	  assert feedbackRound.benchmarkTuple == current_benchmark # Don't support changed benchmarks here
	
	  experiment.benchmark = feedbackRound.benchmark
	  
	  previousRoundInfo = skipToRound(roundInfo.sameAsIteration)
	  assert previousRoundInfo
	  assert previousRoundInfo.stopSeed
	  
	  experiment.config    = feedbackRound.transformConfig(config, previousRoundInfo.stopSeed)

	  exe = experiment.as_binary_filename(experiment, experiments.Version(current_iteration, "final"))
	  for action in actions:
		action(experiment, roundInfo.sameAsIteration, current_iteration, feedbackRound, exe, "final")
		
	  roundInfo = previousRoundInfo # So that we keep the seeds
	else:
          logging.info("Final binary: %s" % str(roundInfo.seed))
	  exe = experiment.as_binary_filename(experiment, experiments.Version(current_iteration, "final"))
	  for action in actions:
		action(experiment, current_iteration, current_iteration, feedbackRound, exe, "final")

	

def single_feedback_test(maxcost, rounds, start, stop, seed, extra="", binary_suffix=""):
  actions = []
  actions.append( lambda experiment, src_iteration, current_iteration, feedbackRound, exe, currentRound: single_feedback_round(experiment, current_iteration, feedbackRound, regular_single_feedback_action, currentRound=currentRound, infoFromIteration=src_iteration) )
  #actions.append( lambda experiment, current_iteration, feedbackRound, exe, currentRound: instruction_counts.execution_time(exe, experiment, "train") ) # TODO make more easily configurable
  #actions.append( lambda experiment, current_iteration, feedbackRound, exe: instruction_counts.traceRemote(exe, experiment) )
  
  roundAction = roundActionBasedOnInstructionsMatchedMaxRounds(maxrounds=3) # TODO put maxrounds elsewhere
  single_feedback_test_raw(maxcost, rounds, start, stop, seed, actions, roundAction, extra, binary_suffix)

def single_feedback_test_redo_diablo(maxcost, rounds, start, stop, seed, extra="", binary_suffix=""):
  actions = []
  actions.append( lambda experiment, current_iteration, feedbackRound, exe: single_feedback_round(experiment, current_iteration, feedbackRound, just_redo_diablo_feedback_action ) )

  roundAction = roundActionBasedOnInstructionsMatchedMaxRounds(maxrounds=5) # TODO put maxrounds elsewhere
  single_feedback_test_raw(maxcost, rounds, start, stop, seed, actions, roundAction, extra, binary_suffix)

def single_feedback_test_redo_bindiff(maxcost, rounds, start, stop, seed, extra="", binary_suffix=""):
  actions = []
  actions.append( lambda experiment, current_iteration, feedbackRound, exe: single_feedback_round(experiment, current_iteration, feedbackRound, just_redo_bindiff_action ) )

  single_feedback_test_raw(maxcost, rounds, start, stop, seed, actions, extra, binary_suffix)


def single_feedback_test_redo_diablo_bindiff(maxcost, rounds, start, stop, seed, extra="", binary_suffix=""):
  actions = []
  actions.append( lambda experiment, current_iteration, feedbackRound, exe: single_feedback_round(experiment, current_iteration, feedbackRound, just_redo_diablo_feedback_action ) )
  actions.append( lambda experiment, current_iteration, feedbackRound, exe: single_feedback_round(experiment, current_iteration, feedbackRound, just_redo_bindiff_action ) )

  single_feedback_test_raw(maxcost, rounds, start, stop, seed, actions, extra, binary_suffix)



seed = iterative_rounds.mySeed

# For easy in-file testing:
# single_feedback_test(maxcost=40, rounds=iterative_rounds.bzip2_arm, extra="first_test", start=0, stop=21, seed=12)

def iterative_diversity(rounds, extra, stop, seed, rules_file):
  global transformation_rules
  transformation_rules = rules_file
  single_feedback_test(maxcost=40, rounds=rounds, extra=extra, start=0, stop=stop, seed=seed)
