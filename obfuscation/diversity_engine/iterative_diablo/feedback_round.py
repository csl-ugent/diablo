# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy

class FeedbackRound:
  def __init__(self, benchmarkTuple, whichIteration, compareWith, transformConfig):
	"""benchmark is the benchmark we will be transforming this round
	   benchmarkVersion is the version of this benchmark to be transformed
	   whichRound is a function that returns True on iterations that this function shound be applied
	   compareWith is the (possibly empty) list of iterations
	   transformConfig is a *function* that takes in a setting, and returns a (possibly) different new setting"""
	self.benchmarkTuple                     = benchmarkTuple
	(self.benchmark, self.benchmarkVersion) = benchmarkTuple
	self.whichIteration                     = whichIteration
	self.compareWith                        = compareWith
	self.transformConfig                    = transformConfig

def getFirstMatchingFeedbackRoundFromList(iteration, listOfRounds):
  for r in listOfRounds:
	if r.whichIteration(iteration):
	  return r

  return None

def singleRound(i):
  return lambda r: r == i

def untilRound(i):
  return lambda r: r <= i

def baseSettings(settings, seedBase):
  return settings

class CallableSettings():
  def transform(self, config, seedBase):
	pass

  def __call__(self, *args):
	newConfig = copy.deepcopy(args[0])
	seedBase = args[1]
	self.transform(newConfig, seedBase)
	return newConfig

  def __str__(self):
	return self.__class__.__name__


class layoutSettings(CallableSettings):
  def __init__(self, seed):
	self.seed = seed

  def transform(self, config, seedBase):
	#config.instruction_seed = self.seed
	config.instruction_seed = seedBase

class iterativeSettings(CallableSettings):
  def __init__(self, layoutSeed, diversitySeed, costs, transforms):
	self.layoutSeed    = layoutSeed
	self.diversitySeed = diversitySeed
	self.costs         = costs
	self.transforms    = transforms

  def transform(self, config, seedBase):
	#config.initial_seed     = self.diversitySeed
	#config.instruction_seed = self.layoutSeed
	config.initial_seed     = seedBase
	config.instruction_seed = seedBase

	config.ifStillMappedThenRetransformFromScratch = True
	config.usesTransformPerFunctionFile            = True
  
	for transform in self.transforms:
	  config.transformations[transform] = True
