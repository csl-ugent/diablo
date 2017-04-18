# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import diablo
import multiplatform

class Benchmark:
  def __init__(self, exe, dir_template, versions, inputs={}, profile_file="output.prof", isThumb2=False):
	"""Versions is a map from integers starting at 1 that maps to a string that should be spliced into dir_template (in unix format) at the %s. """
	self.exe          = exe
	self.dir_template = dir_template
	self.versions     = versions
	self.inputs       = inputs
	self.profile_file = profile_file
	self.isThumb2     = isThumb2
	
  def version_dir(self, version, asLocal=True):
	"""The version is an integer that gets mapped to the string representation of the"""
	d = self.dir_template % self.versions[version]
	if asLocal:
	  return multiplatform.asLocal(d)
	else:
	  return d

  def local_subdir(self):
	"""Some benchmarks, after copying, have their main files relative to a subdirectory (like libpng/benchmark), return that here"""
	return ""

  def initialize_experiment_dir(self, version, experiment_dir, config):
	"""Populates the target (experiment) directory with the files selected from the original files in the benchmark's version directory.
	   The experiment_dir already has added local_subdir. """
	#multiplatform.copy_single_dir(self.version_dir(version), experiment_dir)
	config.directory = self.version_dir(version, asLocal=False)

  def __str__(self):
	return self.exe

  def semantic_change_instructions(self, t2o, version):
	insts = []

	for line in open(multiplatform.dir_entry(self.version_dir(version), "semantic_changes")):
	  insts = insts + t2o[int(line, base=16)] # This returns a list
	return insts

bzip2     = Benchmark("bzip2",      "/home/bcoppens/diversity/compiled/bzip2O3_%s/O3/",
					  { 1: "original", 2: "patched"},
					  {  "test":  [ [ "dryer.jpg", "2" ] ],
					     "train": [ [ "input.program", "10" ], [ "byoudoin.jpg", "5" ], [ "input.combined", "80" ] ] } )
soplex    = Benchmark("soplex",     "/home/bcoppens/diversity/compiled/soplexO3_%s/O3/",
					  { 1: "original", 2: "mergesort" },
					  { "test": [ [ "test.mps" ] ],
					    "train": [ [ "pds-20.mps" ], [ "train.mps" ] ] } )

pngbeta   = Benchmark("pngtest", "/home/bcoppens/diversity/compiled/png-full_%s/O3/libpng/build/",      { 1: "v1", 2: "v2" } )
pngdebian = Benchmark("pngtest", "/home/bcoppens/diversity/compiled/png-debian_%s/O3/libpng/build/",    { 1: "v1", 2: "v2" } )


openssl   = Benchmark("openssl", "/home/bcoppens/diversity/compiled/openssl_%s/O3/apps/",
					  { 1: "0.9.8u", 2: "0.9.8v", 3: "0.9.8w" },
					  { "train": [ [ "crl", "-inform", "DER", "-in", "bcbe.crl.der" ] ] } )

openssl_ssltest = Benchmark("ssltest", "/home/bcoppens/diversity/compiled/openssl_%s/O3/apps/",
					  { 1: "0.9.8u", 2: "0.9.8v", 3: "0.9.8w" },
					  { "train": [ [ "" ] ] } )


mcrypt = Benchmark("mcrypt", "/home/bcoppens/diversity/vulnerability_samples/mcrypt/%s/builds/diablogcc-4.6.2-32/mcrypt-2.6.8/src/",
					  { 1: "CVE-2012-4409/before_redone", 2: "CVE-2012-4409/after" },
					  { "train": [ [ "" ] ] } )

bzip2_arm     = Benchmark("bzip2",      "/media/1cf273de-b073-441c-9c4e-330c60320e4a/aspire/diabloregression/common/regression-main/bzip2-iterative-arm-src-222-481-thumb2-O2/vdynamic%s/",
                      { 1: "a", 2: "b"},
                      {  "test":  [ [ "dryer.jpg", "2" ] ],
                         "train": [ [ "input.program", "10" ], [ "byoudoin.jpg", "5" ], [ "input.combined", "80" ] ] },
                      isThumb2=True)

