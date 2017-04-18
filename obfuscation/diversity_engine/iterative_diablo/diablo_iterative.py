#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy
import logging
import os
import re
import shutil                as     shutil
import sqlite3               as     sql
from   subprocess            import call
import sys
from   xml.etree.ElementTree import ElementTree

import diablo
import experiments
import ida
import ida.bindiff
import multiplatform
import utils

### ITERATIVE INPUT ###

class Iterative:
  def __init__(self, fromstring):
	(self.first_address, self.seed, self.transformations_left, self.max_cost, self.times_iterated, self.redo) = fromstring.split(',')
	self.redo = int(self.redo) == 0 # If it is equal to 0 -> output should be 0 again! -> invert

  def __str__(self):
	return self.first_address + "," + self.seed + "," + self.transformations_left + "," + self.max_cost + "," + self.times_iterated + "," + str(int(not self.redo))

def iterative_input(f):
  ret = {}

  for line in open(f):
	info = Iterative(line)
	ret[info.first_address] = info

  return ret

### EVALUATION ###


### Iterate by looking at which functions have been matched, and of those matches what caused them -> we tell the iterative framework to retransform using specific transformations


### Evaluate a single round & update the iterative file for the next round
def set_next_iteration(bindiff_file, iterative_input_file, iterative_output_file, map1, map2, ins2fun1, ins2fun2):
  map1data = open(map1).read()
  map2data = open(map2).read()
  ins2fun1data = open(ins2fun1).read()
  ins2fun2data = open(ins2fun2).read()

  iterative_data = iterative_input(iterative_input_file)

  con = None
  try:
	con = sql.connect(bindiff_file)
  
	cur = con.cursor()    
	cur.execute("SELECT address1, address2 FROM function;")
	rows = cur.fetchall()
  
  
	for row in rows:
	  f1of = diablo_fun_addr(row[0], map1data, ins2fun1data)
	  f2of = diablo_fun_addr(row[1], map2data, ins2fun2data)

	  # If the addresses are the same, the guessed match is actually a correct match in the original program! Redo these...
	  if f2of is not None and f1of == f2of:
		iterative_data[f2of].redo = False

	out = open(iterative_output_file, "w")
	for output in iterative_data:
	  out.write(str(iterative_data[output]) + "\n")
	out.close()

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()

  return iterative_data


def evaluate(iteration, bindiff, iterative_data, appendto):
  to_redo = 0
  for d in iterative_data:
	if iterative_data[d].redo:
	  to_redo += 1
  
  f = open(appendto, "a")
  f.write("After iteration %i, I need to redo %i functions!\n" % (iteration, to_redo) )
  f.close()

def binary_iteration_filename(experiment, v):
  version_string = str(v) # can expand to iteration_round, for example
  return experiment.benchmark.exe + "_iterated_binary_%s_%s"  % (v, experiment.binary_suffix)

def postProcessFile(version):
  return "postprocess.insts.remove.%s" % str(version)

def getPostProcessInstsToRemove(version):
  insts = set()
  for line in open(postProcessFile(version), "r"):
	insts.add(int(line, base=16))
  return insts

def iteration_filenames(experiment, current_iteration, takes_input):
  # exes should have underscores, because IDA cuts off the last '.' as 'extension', and all idbs point to the same

  filenames = {}

  orig_exe = experiment.benchmark.exe
  filenames["orig_exe"] = orig_exe

  if takes_input:
	filenames["inputfile"]    = orig_exe + ".iterative_input.%i"  % current_iteration
  else:
	filenames["inputfile"]    = None  

  filenames["outputfile"]     = orig_exe + ".iterative_output.%i" % current_iteration        # The raw diablo output, which will be evaluated&processed into the next input
  filenames["inputfile_next"] = orig_exe + ".iterative_input.%i"  % (current_iteration + 1)  # The processed diablo output, to be used directly as input for the next round

  filenames["map2"]           = "mapping.xml.%i"                      % current_iteration
  filenames["ins2fun2"]       = "instructions_to_function.mapping.%i" % current_iteration


  return filenames

def single_iteration_diablo(current_iteration, experiment, takes_input=True, compare_with=0):
  filenames = iteration_filenames(experiment, current_iteration, takes_input)

  diablo.runIterative(experiment=experiment, iinput=filenames["inputfile"], ioutput=filenames["outputfile"], in_binary=filenames["orig_exe"], out_binary=filenames["current_binary"], suffix=current_iteration)


def single_iteration(current_iteration, experiment, just_diablo, takes_input=True, map1=None, ins2fun1=None, orig_binary=None, evaluate_and_prepare_next_iteration=True, compare_with=0):
  assert False # NOT USED / NOT ADAPTED FOR ROUNDS
  single_iteration_diablo(current_iteration, experiment, takes_input, compare_with)

  filenames = iteration_filenames(experiment, current_iteration, takes_input)

  if not just_diablo:
	# TODO orig_binary etc
	filenames["bindiff_file"]   = bindiff_filename(experiment, v1, v2)
	ida.bindiff.run_BinDiff(v1=filenames["orig_binary"], v2=filenames["current_binary"], out_file=filenames["bindiff_file"], experiment=experiment)

	print current_iteration

	if experiment.config.extend_IDA_analysis:
	  bindiff_file = bindiff_file + ida.extended_suffix
	else:
	  bindiff_file = bindiff_file + ida.simple_suffix

	if evaluate_and_prepare_next_iteration:
	  iterative_data = set_next_iteration(bindiff_file=bindiff_file, iterative_input_file=current_outputfile, iterative_output_file=next_inputfile,
										  map1=map1, map2=map2, ins2fun1=ins2fun1, ins2fun2=ins2fun2)

	  evaluate(iteration=current_iteration, bindiff=bindiff_file, iterative_data=iterative_data, appendto="./iterative.log")

def all_comparison_bindiff_files(experiment, compare_with, iteration, round):
  files = []
  this_round = experiments.Version(iteration, round)
  for orig in compare_with:
	orig = experiments.Version(orig, "final")
	files.append(ida.BinDiff.full_filename(experiment, orig, this_round))

  return files

# src_iteration is for when we need to re-use the input files from an OLD iteration to produce
# an output for a much later iteration, for example when same_as_iteration with rounds is in use
def single_iteration_with_comparisons(src_iteration, iteration, round, experiment, just_diablo, compare_with, ida_when_no_comparisons=True, copy_output_when_no_comparisons=True):
  do_comparisons = len(compare_with) > 0
  takes_input    = do_comparisons

  src_filenames = iteration_filenames(experiment, src_iteration, takes_input)
  filenames = iteration_filenames(experiment, iteration, takes_input)
  
  current_binary = binary_iteration_filename(experiment, experiments.Version(iteration, round))

  diablo.runIterative(experiment=experiment, iinput=src_filenames["inputfile"], ioutput=filenames["outputfile"], in_binary=filenames["orig_exe"], out_binary=current_binary, suffix=iteration)
  
  #shutil.move("postprocess.insts.remove", postProcessFile(experiments.Version(iteration, round))) # TODO TODO make generic
  
  logging.debug("Comparisons: %s", str(compare_with))
  
  if not do_comparisons:
	# We don't do a comparison, but still might run IDA
	if copy_output_when_no_comparisons:
	  copy_iterative_output_from_to(experiment.benchmark.exe, iteration, iteration + 1)
	if ida_when_no_comparisons:
	  ida.run_Ida(current_binary, extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)
  else:
	for orig in compare_with:
	  logging.debug("Matching with: %i", orig)
	  ida.BinDiff.match(experiment=experiment, v1=experiments.Version(orig, "final"), v2=experiments.Version(iteration, round))

def copy_iterative_output_from_to(exe, fr, to):
  shutil.copy(exe + ".iterative_output.%i" % fr, exe + ".iterative_input.%i" % to)

# The initial mapping, which is the 'undiversified' case we want to improve on:
# - Generate a random binary as version 0
# - Use the output to regenerate _the same binary_ as version 1 (this can serve as sanity check that diablo regenerates the binary correctly)
# - Match version 0 and version 1, evaluate.
# - Use the evaluation for iterating further, as normal
# All steps can be done with the regular iterating mechanism once we generate version 0:
# we just set the input of version 1 to be the _unevaluated_ output of version 0
def setup_first_iteration(experiment, nr = 0):
	single_iteration(nr, experiment, just_diablo=True, takes_input=False, compare_with=0)
	copy_iterative_output_from_to(experiment.benchmark.exe, nr, nr + 1)
	ida.run_Ida(experiment.benchmark.exe + "_iterated_binary_%s" % experiments.Version(nr, "final"), extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)

### MAIN PROGRAM ###
def iterative_diablo_evaluation(current_iteration, max_iteration, experiment, just_diablo, compare_with):
  if current_iteration == 0:
	setup_first_iteration(experiment)
	current_iteration = 1

  while current_iteration < max_iteration:
	single_iteration(current_iteration, experiment, just_diablo=just_diablo, compare_with=0)
	current_iteration += 1

