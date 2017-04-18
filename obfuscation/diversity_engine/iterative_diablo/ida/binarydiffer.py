# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

from   subprocess            import call

import ida
import ida.matcher
import logging
import settings
import utils

binarydiffer = settings.homedrive_scripts + "\\BinaryDiffer_Custom.exe"

class BDMatch(ida.matcher.Match):
  """ We match BB against BB, so one match will match a BB1 against all instructions in BB2 """
  def __init__(self, bb_1, inst_2, percentage):
	self.bb_1   = bb_1
	self.inst_2 = inst_2
	self.percentage = percentage

  def address2(self):
	return self.inst_2

def bb_matches(bb_to_inst, split_line):
  #                              0         1          2       3          4              5             6            7      8
  # line is formatted as 'date/function,addr_bb1,addr_bb2,match kind,match subkind,match_origin1,match_origin2,percentage,?', and comes in as a split with ,
  addr_bb2 = int(split_line[2], base=16)
  matches  = []
  
  if addr_bb2 in bb_to_inst:
	for inst_2 in bb_to_inst[addr_bb2]:
	  matches.append(BDMatch(int(split_line[1], base=16), inst_2, split_line[7]))

  return matches

class BinaryDiffer(ida.matcher.Matcher):
  matcher_suffix = ".binarydiffer"

  def match(self, experiment, v1, v2):
	exe1   = experiment.as_binary_filename(experiment, v1)
	exe2   = experiment.as_binary_filename(experiment, v2)
	suffix = ida.get_suffix(experiment)
  
	file_base = self.filename_base(experiment, v1, v2)

	ida.selectIdaSuffix(exe1, suffix, experiment.path)
	ida.selectIdaSuffix(exe2, suffix, experiment.path)
  
	ida_call = '"%s" -A -S%s\\%s ' % (ida.ida_binary, ida.ida_scriptdir, "binarydiffer.idc")

	logging.info("Running BinaryDiffer %s against %s", exe1, exe2)
	call([binarydiffer, exe1, exe2, self.full_filename(experiment, v1, v2), ida_call])


  def iterate_and_filter_matches(self, experiment, v1, v2, callback):
	for line in utils.grep(" match ,", self.full_filename(experiment, v1, v2)):
	  for match in bb_matches(callback.helper.bb_to_inst, line.split(",")):
		callback.process(match)

  only100percent      = ida.matcher.MatchFilter("only100percent",      lambda match: match.percentage == 100)
  only100and99percent = ida.matcher.MatchFilter("only100and99percent", lambda match: match.percentage >= 99)
  allmatches          = ida.matcher.MatchFilter("all",                 lambda match: True)

  filter_chains       = [ [ida.matcher.AllInsts], [ida.matcher.ExecInsts] ]
  final_filters       = [ only100percent, only100and99percent, allmatches ]

  filters             = [ ( chain, final_filters ) for chain in filter_chains ]
