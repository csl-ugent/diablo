# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import os
import shutil
from   subprocess            import call

import ida
import multiplatform

def filename_base(experiment, v1, v2):
  return experiment.benchmark.exe + "_%i_to_%i" % (v1, v2)

def match(experiment, v1, v2):
  exe1   = experiment.as_binary_filename(experiment, v1)
  exe2   = experiment.as_binary_filename(experiment, v2)
  suffix = ida.get_suffix(experiment)
  
  file_base = filename_base(experiment, v1, v2)
  
  options = "patchdiff2_bart:%s %s" % (multiplatform.local_dir_entry(experiment.path, exe2) + ".idb", file_base)

  ida.selectIdaSuffix(exe1, suffix, experiment.path)
  ida.selectIdaSuffix(exe2, suffix, experiment.path)
  ida.run_Ida_Script(exe1, "match_patchdiff2.idc", suffix, experiment.path, [".patchdiff2.identical.out", ".patchdiff2.info", ".patchdiff2.matches", "patchdiff2.unmatched1", ".patchdiff2.unmatched2"], options, file_base)

