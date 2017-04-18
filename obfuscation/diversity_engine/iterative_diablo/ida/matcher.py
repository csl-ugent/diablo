# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging

import diablo
import ida
import utils


def bb_to_instructions(experiment, version):
  # file format is: 'function',[bb_addrs]
  bb_to_inst = {}
  for line in open(ida.functions_and_bbs_file(experiment, version), "r"):
	addrs = utils.remove_if_exist(line.split(","), ["", "\n"])
	bb_to_inst[int(addrs[1], base=16)] = map(lambda i: int(i, base=16), addrs[1:])
  return bb_to_inst

class Matcher:
  def filename_base(self, experiment, v1, v2):
	#return experiment.benchmark.exe + "_%i_to_%i" % (v1, v2) # Gets a simple/extended suffix too!
	return "%s_to_%s" % (experiment.as_binary_filename(experiment, v1), experiment.as_binary_filename(experiment, v2))

  def full_filename(self, experiment, v1, v2):
	suffix = ida.get_suffix(experiment)
	return ida.script_filename_raw(self.filename_base(experiment, v1, v2), suffix, self.matcher_suffix)

  def analyze(self, experiment, version):
	pass

  def match(self, experiment, v1, v2):
	pass

  def iterate_and_filter_matches(self, experiment, v1, v2, callback):
	pass

class Match:
  def address2(self):
	pass

def match_index_list(match_address, list):
  for item in list:
	if item.address2() == match_address:
	  return item
  return None

class BinaryMatchHelper:
  """ Keeps track of some precomputed stuff, like all instructions, maps, and all executed instructions (if available), that can be re-used across matchers and filters """
  def __init__(self, experiment, v2):
	self.bb_to_inst       = ida.matcher.bb_to_instructions(experiment, v2)
	# TODO: reuse the same single map reading?
	self.all_insts        = diablo.all_insts(diablo.map_filename(v2))
	self.t2o              = diablo.orig_to_transformed_map(diablo.map_filename(v2))
	self.semantic_changes = experiment.benchmark.semantic_change_instructions(self.t2o, experiment.benchmark.version)
	# TODO: executed instructions
	self.exec_insts       = []
	
	if experiment.config.extend_IDA_analysis:
	  self.name_prefix    = "%s_extended" % str(experiment.benchmark)
	else:
	  self.name_prefix    = "%s_simple" % str(experiment.benchmark)

class MatchFilter:
  def __init__(self, name, keep = None):
	self.name   = name
	if keep is not None:
	  self.keep = keep

  def __str__(self):
	return self.name

AllInsts  = MatchFilter("all",  lambda h, ins: ins in h.all_insts)
ExecInsts = MatchFilter("exec", lambda h, ins: ins in h.exec_insts)

def chain_name(name_prefix, chain_filters, final_filter):
  return "%s_%s_%s" % (name_prefix, "_".join(map (lambda c: str(c), chain_filters)), str(final_filter))

class FoundChanges:
  """ Keeps track of the number of changes in total (found_all), and the number of correctly found semantic changes (found_changes) """
  def __init__(self, name):
	self.name          = name
	self.found_all     = 0
	self.found_changes = 0
  def __str__(self):
	return str( (self.found_all, self.found_changes) )

def initialize_filters_map(name_prefix, chain_prefix, filters):
  m = {}

  for filter in filters:
	name    = chain_name(name_prefix, chain_prefix, filter)
	m[name] = FoundChanges(name)
  return m


class FilterCallback:
  def __init__(self, helper, chains):
	self.helper = helper
	self.chains = chains
	self.maps   = {}

	for chain in self.chains:
	  # These should all return distinct names
	   self.maps.update(initialize_filters_map(helper.name_prefix, chain[0], chain[1]))

  def process(self, match):
	v2_addr = match.address2()
	
	for chain in self.chains:
	  keep = True
	  chain_filters = chain[0]
	  final_filters = chain[1]

	  for chain_filter in chain_filters:
		if not chain_filter.keep(self.helper, v2_addr):
		  keep = False

	  # This chain of filters kept this instruction, look it up in the matcher, and look if our matcher's filters filter them away
	  if keep:
		for final_filter in final_filters:
		  if final_filter.keep(match):
			name    = chain_name(self.helper.name_prefix, chain_filters, final_filter)
			changes = self.maps[name]

			changes.found_all += 1

			if v2_addr in self.helper.semantic_changes:
			  changes.found_changes += 1

def semantic_changes_found_count_filters(helper, matcher, experiment, v1, v2, chains):
  logging.debug("Semantic changes: %i v %i", v1, v2)
  
  callback = FilterCallback(helper, chains)
  matcher.iterate_and_filter_matches(experiment, v1, v2, callback)
  
  logging.debug("All matches found")
  
  return callback.maps
