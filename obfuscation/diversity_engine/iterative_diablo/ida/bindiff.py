# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import os
import shutil
from   subprocess            import call
import sys

import sqlite3 as sql

import ida
import ida.matcher
import multiplatform
import settings

#java="java"

class BinDiff(ida.matcher.Matcher):
  matcher_suffix = ".BinDiff"

  def binExport(self, exe, suffix, path):
	logging.info("BinExport for %s" % exe)
	ida.run_Ida_Script("%s" % exe, "export.idc", suffix, path, [".BinExport"], options="ExporterModule:%s" % multiplatform.asLocal(path), add_idb=True)

  def run_BinDiff_once(self, v1, v2, out_file, suffix, path):
	ida.selectIdaSuffix(v1, suffix, path)
	ida.selectIdaSuffix(v2, suffix, path)
   
	logging.info("Running BinDiff, comparing file %s with %s (with suffix %s)", v1, v2, suffix)
  
	self.binExport(v1, suffix, path)
	self.binExport(v2, suffix, path)

	logging.info("Diffing...")
	# TODO: log output?
	target_bindiff = "%s/%s_vs_%s.BinDiff" % (multiplatform.asLocal(path), v1, v2)
	  
	try:
		os.remove(target_bindiff)
	except OSError, e:
		pass
	  
	call( [settings.bindiff_exe,
		     "-i%s" % multiplatform.dir_entry(path, "%s.BinExport" % v1),
		     "-j%s" % multiplatform.dir_entry(path, "%s.BinExport" % v2),
		     "-o%s" % multiplatform.asLocal(path) ] )
	logging.debug("Done")

	# If we didn't _move_ the file, we have twice as many .BinDiff files taking space, and if we re-run
	shutil.move(target_bindiff, out_file)

  def run_BinDiff(self, v1, v2, experiment):
	""" as_binary_filename is a function that takes 2 arguments: experiment and iteration number, and returns the binary name for this iteration """
	try:
	  f = open("bindiff.input", "w")
	
	  binary_1 = experiment.as_binary_filename(experiment, v1)
	  binary_2 = experiment.as_binary_filename(experiment, v2)

	  f.write("%s#%s#%s" % (binary_1, binary_2, binary_2)) # file1#file2#dirbase
	  f.close()
	
	  ida.run_Ida(binary_2, extend_analysis=experiment.config.extend_IDA_analysis, path=experiment.path)
	
	  out_file = self.full_filename(experiment, v1, v2)

	  self.run_BinDiff_once(binary_1, binary_2, out_file, ida.get_suffix(experiment), experiment.path)
    
	except OSError, e:
	  print "Error in running BinDiff:"
	  print e
	  sys.exit(1)

  def match(self, experiment, v1, v2):
	  logging.debug("Matching BinDiff: %s v %s", str(v1), str(v2))
	  self.run_BinDiff(v1, v2, experiment)
	  logging.debug("... Done! (With Matching BinDiff: %s v %s)", str(v1), str(v2))

  def get_instructions(self, experiment, v1, v2, which_version=2):
	logging.info("Getting instructions, comparing %s versus %s" % (str(v1), str(v2)))
	
	bd = self.full_filename(experiment, v1, v2)
	
	# There is no try! Then we know immediately if and when it failed :)
	con = sql.connect(bd)
	cur = con.cursor()    
	cur.execute("SELECT instruction.address%i FROM instruction;" % which_version)
	
	insts = set()
	
	for row in cur.fetchall():
	  insts.add(row[0])
	  
	con.close()
	
	return insts

  filters = []
  filter_chains = []
