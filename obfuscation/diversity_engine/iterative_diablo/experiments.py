# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

from settings import settings

#binary_path_base = "/home/bcoppens/diversity/iterative/experiments/"
#binary_path_base = "/home/bcoppens/diversity/iterative/experiments/multipleruns/"
#binary_path_base = "/home/bcoppens/diversity/iterative/experiments/monotonic_decreasing/"

binary_path_base = settings.experiments_binary_path_base

def setGlobalBinaryPathBase(path):
  global binary_path_base
  binary_path_base = path

class Experiment:
  def __init__(self, config, benchmark, as_binary_filename):
	self.config = config
	self.benchmark = benchmark
	self.as_binary_filename = as_binary_filename

	""" path is the directory in which the results of an experiment will be stored, and where we have to chdir to to run diablo. This is still in Unix-style directories"""
	self.path = binary_path_base + config.experiment_dir + benchmark.local_subdir()
	self.version = -1

	self.binary_suffix = ""
	
	self.name = ""
	self.shortname = ""

  def initialize_dir(self, version, config):
	self.benchmark.initialize_experiment_dir(version, self.path, config)

class Version:
  def __init__(self, iteration, round):
	self.iteration = iteration
	self.round = round
	self.suffix = ""
  def __str__(self):
	return "%s_%s%s" % (str(self.iteration), str(self.round), str(self.suffix))
