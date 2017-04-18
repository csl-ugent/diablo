#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy
import os
import shutil     as     shutil
import sys

#sys.path.append("E:\\home\\bcoppens\\private\\software\\diversity\\experiments\\diversity\\iterative_diablo")
#execfile("diablo_iterative.py")
execfile("E:\\home\\bcoppens\\private\\software\\diversity\\experiments\\diversity\\iterative_diablo\\diablo_iterative.py")

experiment_source = "E:\\home\\bcoppens\\diversity\\compiled\\bzip2O3_original\\O3\\"

class DiversityConfig(DiversityConfigBase):
  def __init__(self):
	#super(DiversityConfig, self).__init__() # Why doesn't this work?
	DiversityConfigBase.__init__(self)
	
	self.initial_seed = 145
	self.experiment_dir = "exploratory\\isolation"

def without_and_with_extend(config, base):
  for extend in [False, True]:
	nr = base + 10 * int(extend)
	config.extend_IDA_analysis = extend
	# Because of the takes_input=False, we will regenerate a new binary every time with the given random seed, there is NO ITERATIVE INPUT!
	single_iteration(current_iteration = nr + 1, just_diablo=False, config=config, takes_input=False)


def single_transform_test(transform, maxcost):
  global compare_with
  config = DiversityConfig()
  baseconfig = DiversityConfig()
  config.experiment_dir += "\\" + transform
  baseconfig.experiment_dir = config.experiment_dir
  baseconfig.extend_IDA_analysis = True
  
  set_binarypath(config)
  try:
	os.makedirs(binary_path)
  except OSError, e:
	print "Exception: %s" % str(e)

  os.chdir(binary_path)

  # Initialize undiversified base file, against we will be comparing! This has both simple and extended.
  ### ### initialize_experiment(experiment_source, binary_path)
  ### ### setup_first_iteration(baseconfig, nr = 0)
  ### ### single_iteration(current_iteration = 1, just_diablo=False, config=baseconfig)

  compare_with = 0

  # Generate base file: this one is NOT diversified, comparing against itself!
  ### ### without_and_with_extend(config, base=100)
  
  # Compare against a version that has been relayouted! (### TODO: ideally this should only be done in ONE benchmark, and then copied to the rest, same as above actually)
  config.instruction_seed = 145
  ### ### without_and_with_extend(config, base=200)

  # Now diversify it with the given transformation (no code relayout yet)
  config.instruction_seed = None
  config.transformations[transform] = True
  config.costs["InitialMaxCost"] = maxcost
  without_and_with_extend(config, base=300)
  
  # Now both diversity and code relayout!
  config.instruction_seed = 145
  without_and_with_extend(config, base=400)
  
parse_regular_argv()

#single_transform_test("FlipBranches", 20000)
#single_transform_test("OpaquePredicates", 20000)
#single_transform_test("InlineFunctions", 5)
#single_transform_test("TwoWayPredicates", 40)
#single_transform_test("DisassemblyThwarting", 40)
#single_transform_test("FlattenFunctions", 20000)
single_transform_test("UnfoldBasicBlocks", 40)

#for f in *BinDiff*; do cp $f `basename $f`_RTest.BinDiff; done
#
# for i in 311 411; do echo $i; chmod a+x ./bzip2_iterated_binary_$i; time ~/diversity/pin/pin-2.11-49306-gcc.3.4.6-ia32_intel64-linux/pin -t /home/bcoppens/diversity/pin/pin-2.11-49306-gcc.3.4.6-ia32_intel64-linux/source/tools/IterativeDiablo/obj-ia32/countins.so -o insexec_$i -- ./bzip2_iterated_binary_$i ~/diversity/iterative/experiments/exploratory/isolation/InlineFunctions/dryer.jpg 2; done;

#time (for t in FlipBranches InlineFunctions OpaquePredicates; do echo $t; cd $t; for i in 0 1 101 111 201 211 301 311 401 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py bzip2_iterated_binary_${i}.dyncfg.xml bzip2_0_to_${i}.BinDiff_simple_RTest.BinDiff ; done; cd ..; done)
#time (for t in FlipBranches InlineFunctions OpaquePredicates; do echo $t; cd $t; for i in 0 1 101 111 201 211 301 311 401 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py bzip2_iterated_binary_${i}.dyncfg.xml bzip2_0_to_${i}.BinDiff_extended_RTest.BinDiff ; done; cd ..; done)
#
#
#
#
# time (for t in TwoWayPredicates DisassemblyThwarting FlattenFunctions; do echo $t; cd $t; for i in 111 211 311 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py insexec_${i} bzip2_0_to_${i}.BinDiff_simple_RTest.BinDiff exec; done; cd ..; done)
# time (for t in TwoWayPredicates DisassemblyThwarting FlattenFunctions; do echo $t; cd $t; for i in 111 211 311 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py insexec_${i} bzip2_0_to_${i}.BinDiff_extended_RTest.BinDiff exec; done; cd ..; done)
# 
# 
# 
# time (for t in TwoWayPredicates DisassemblyThwarting FlattenFunctions; do echo $t; cd $t; for i in 111 211 311 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py mapping.xml.${i} bzip2_0_to_${i}.BinDiff_simple_RTest.BinDiff all; done; cd ..; done)
# time (for t in TwoWayPredicates DisassemblyThwarting FlattenFunctions; do echo $t; cd $t; for i in 111 211 311 411; do echo $i; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/add_cfg_info_to_BinDiff.py mapping.xml.${i} bzip2_0_to_${i}.BinDiff_extended_RTest.BinDiff all; done; cd ..; done)

