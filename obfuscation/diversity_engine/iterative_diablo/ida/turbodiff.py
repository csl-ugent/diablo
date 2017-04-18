# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import os
import shutil
from   subprocess            import call

import ida
import ida.matcher
import multiplatform

class TurboDiff(ida.matcher.Matcher):
  matcher_suffix = ".turbodiffdump"

  def analyze(self, experiment, version):
	exe    = experiment.as_binary_filename(experiment, version)
	suffix = ida.get_suffix(experiment)

	ida.selectIdaSuffix(exe, suffix, experiment.path)
	ida.run_Ida_Script(exe, "analyze_turbodiff.idc", suffix, experiment.path, [".ana", ".dis", ".turbodiffinfo"], "")

  def match(self, experiment, v1, v2):
	exe1   = experiment.as_binary_filename(experiment, v1)
	exe2   = experiment.as_binary_filename(experiment, v2)
	suffix = ida.get_suffix(experiment)
  
	file_base = self.filename_base(experiment, v1, v2)

	options = "turbodiff_bart:%s %s" % (multiplatform.local_dir_entry(experiment.path, exe2) + ".idb", file_base)

	ida.selectIdaSuffix(exe1, suffix, experiment.path)
	ida.selectIdaSuffix(exe2, suffix, experiment.path)
	ida.run_Ida_Script(exe1, "match_turbodiff.idc", suffix, experiment.path, [".turbodiffdump"], options, file_base)

  filters = []
  filter_chains = []
