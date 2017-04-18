# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import os
import copy
import re
import shutil                as     shutil
import sqlite3               as     sql
import subprocess
from   subprocess            import call
import sys

import multiplatform
import settings

import ida.binarydiffer
import ida.bindiff
import ida.patchdiff2
import ida.turbodiff

simple_suffix="_simple"
extended_suffix="_extended"

ida_binary    = settings.settings.ida_binary
ida_scriptdir = settings.homedrive_scripts

def removeTempIdaFiles(exe, binary_path, rm_idb=False):
  toRemove = [ exe + ".id0", exe + ".id1", exe + ".nam", exe + ".til" ]
  if rm_idb:
	toRemove = toRemove + [ exe + ".idb" ]

  for f in toRemove:
	try:
	  os.remove(binary_path + "/" + f)
	except OSError, e:
	  print "Error removing file:"
	  print e

def script_filename_raw(base_file, suffix, script_output):
  """ Returns the filename of a script output file with the current suffix """
  return base_file + script_output + suffix + script_output

def run_Ida_Script(exe, script, suffix, path, to_copy_output, options="", output_base=None, add_idb=False):
  """ Runs the given script with IDA. to_copy_output is a list of suffices that will be appended to exe, those outputs will get copied and get a suffix to it """
  # idb file must be removed by callee of this function! (ie run_Ida, so that we can actually re-use this to extend an existing idb)
  auto = True
  print "Running IDA on the binary..."
  
  if options == "":
	options_arg = []
  else:
	options_arg = ["-O" + options]


  if auto:
	auto_arg = ["-A"]
  else:
	auto_arg = []
	
  if output_base is None:
	output_base=exe

  # Some plugins, such as BinDiff's exportermodule, really want the .idb suffix, but then output files without the .idb. Hence this hack:
  if add_idb:
	to_call_binary_file = "%s.idb" % exe
  else:
	to_call_binary_file = exe

  to_call = [ida_binary] + auto_arg + ["-S%s/%s" % (ida_scriptdir, script)] + options_arg + [multiplatform.local_dir_entry(path, to_call_binary_file)]
  
  logging.debug("Running Ida Script: %s", " ".join(to_call)) 
  
  #call(to_call)
  subprocess.call(to_call)
  
  for output in to_copy_output:
	logging.debug("Copying %s" % output)
	logging.debug("%s -> %s" % (multiplatform.local_dir_entry(path, output_base + output), multiplatform.local_dir_entry(path, script_filename_raw(output_base, suffix, output))))
	shutil.copy(multiplatform.local_dir_entry(path, output_base + output), multiplatform.local_dir_entry(path, script_filename_raw(output_base, suffix, output)))
  

def run_Ida(exe, extend_analysis, path):
  # Analyze the new binary with ida! (but remove the idb if it exists!)
  removeTempIdaFiles(exe, multiplatform.asLocal(path), rm_idb=True)

  run_Ida_Script(exe, "auto.idc", simple_suffix, path, [".idb"])

  if extend_analysis:
	print "Extending the analysis"
	run_Ida_Script(exe, "bzip.idc", extended_suffix, path, [".idb"])

def selectIdaSuffix(exe, suffix, path):
  removeTempIdaFiles(exe, multiplatform.asLocal(path))
  shutil.copy(multiplatform.local_dir_entry(path, exe + ".idb" + suffix + ".idb"), multiplatform.local_dir_entry(path, exe + ".idb"))

def get_suffix(experiment):
  if experiment.config.extend_IDA_analysis:
	return extended_suffix
  else:
	return simple_suffix


def functions_and_bbs_file(experiment, version):
  exe = experiment.as_binary_filename(experiment, version)
  return script_filename_raw(exe, get_suffix(experiment), ".idb.functions_and_bbs")

def dump_functions(experiment, version):
  # This script automatically takes its output file from its options
  exe = experiment.as_binary_filename(experiment, version)

  output = functions_and_bbs_file(experiment, version)
  suffix = get_suffix(experiment)
  
  selectIdaSuffix(exe, suffix, experiment.path)
  run_Ida_Script(exe, "dump_function_bbs.idc", suffix, experiment.path, [], "patchdiff2_bart:" + output)


BinaryDiffer = ida.binarydiffer.BinaryDiffer()
BinDiff      = ida.bindiff.BinDiff()
#PatchDiff2   = ida.bindiff.PatchDiff2()
TurboDiff    = ida.turbodiff.TurboDiff()
