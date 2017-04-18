#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy
import os.path
import shutil
import sys
from   xml.etree.ElementTree import ElementTree

import diablo
import diota
import multiplatform
import onlinux
from   settings import settings

class NormalizationConf(diablo.SimpleConf):
  def __init__(self, trace_prefix, dyninst_prefix=None, verbosity=2):
	diablo.SimpleConf.__init__(self)

	self.trace_prefix = trace_prefix

	self.reset_transforms()
	
	self.verbosity = verbosity
	
	self.dyninst_prefix = dyninst_prefix

  def set_transform(self, transform, value):
	if transform == "PlaceDataCorrectly":
	  self.data_placed_correctly = value
	  return
	
	if transform not in self.transformations:
	  raise Exception("Transformation not recognized: %s" % transform)

	self.transformations[transform] = value

	return self

  def enable(self, transform):
	return self.set_transform(transform, True)

  def disable(self, transform):
	return self.set_transform(transform, False)

  def reset_transforms(self):
	self.transformations = copy.deepcopy(NormalizationConf._transformations)
	self.data_placed_correctly = True


  _transformations = {
	"RemoveNotExecuted":       False,
	"RemoveDataSections":      False,
	"NormalizeJcc":            False,
	"DynamicCalls":            False,
	"DynamicJumps":            False,
	"JumpFunctions":           False,
	"JumpFunctionEdgesCalls":  False,
	"JumpFunctionRemovePush":  False,
	"FixLayout":               False,
	"NormalizeInstructions":   False,
	"MultipleIfTarges":        False,
	"OpaquePredicates":        False,
	"Optimize":                False,
	"Liveness":                False,
	"SortDynamicTargets":      False,
	"BranchElimintation":      False,
	"MergeBBLs":               False,
	"Inlining":                False,
	"Peephole":                False
	# "PlaceDataCorrectly": SPECIAL
  }

### DIABLO CONFIG ###
diablo_transforms_mapper = {
  "RemoveNotExecuted":       "-rmdata",
  "RemoveDataSections":      "-rmdase",
  "NormalizeJcc":            "-njcc",
  "DynamicCalls":            "-dync",
  "DynamicJumps":            "-dynj",
  "JumpFunctions":           "-jufu",
  "JumpFunctionEdgesCalls":  "-jfgl",
  "JumpFunctionRemovePush":  "-puju",
  "FixLayout":               "-lay",
  "NormalizeInstructions":   "-nins",
  "MultipleIfTarges":        "-jie",
  "OpaquePredicates":        "-opaak",
  "Optimize":                "-optim",
  "Liveness":                "-live",
  "SortDynamicTargets":      "-sort",
  "BranchElimintation":      "-belim",
  "MergeBBLs":               "-merge",
  "Inlining":                "-inline",
  "Peephole":                "-peep"
}

# TODO: ook input & output
def get_config_arguments(config):
  result = []

  # Transformations
  for transform in config.transformations:
	if config.transformations[transform]:
	  result.append(diablo_transforms_mapper[transform])

  result = result + [ "-dtp", config.trace_prefix ]
  
  if config.dyninst_prefix is not None:
	result = result + [ "-dytp", config.dyninst_prefix ]
  
  if config.data_placed_correctly:
	result = result + [ "-T", settings.linkerfile_place_correct ]
  else:
	result = result + [ "-T", settings.linkerfile_move_rodata ]

  result = result + ["-v"] * config.verbosity

  return result


def runNormalization(experiment, in_binary, out_binary, suffix, useDiabloOutFile=True):
  try:
	outfilename = "%s.diablo.out" % out_binary
	diabloout = open(outfilename, "w")

	args = get_config_arguments(experiment.config) + [ "-o", out_binary, in_binary ]

	diabloout.write("# Called diablo with arguments:\n#%s\n" % " ".join(args))
	
	print args
	if useDiabloOutFile:
	  stdout=diabloout
	else:
	  stdout=None

	try:
	  os.remove(multiplatform.dir_entry(experiment.path, out_binary))
	except OSError, e:
	  pass


	diabloout.close() ### 
	print settings.normalization_diablo
	onlinux.call(settings.normalization_diablo, args, pwd=experiment.path, outfile=outfilename)

	# shutil.move("mapping.xml", "mapping.xml.%s" % suffix)
	shutil.move("mapping.xml", "mapping.xml.%s" % suffix)
	
	if not os.path.isfile(multiplatform.dir_entry(experiment.path, out_binary)):
	  raise Exception("Diablo Failed!!!!")
    
  except OSError, e:
	print "Error:"
	print e
	sys.exit(1)

# Clean diota output first: sed -i -e '/^=.*/d' main-diabloed.dyncfg.dyncfg.xml 


def runDiota(experiment, exe, xml=None, outfile=None, inputs="test"):
  # TODO only first input?
  # diota.diota(exe, "dyncfg", pwd, stdoutfile=outfile, args=experiment.benchmark.inputs["test"][0], diota_home=normalization_diota_dir)

  # TODO switch between linux native or not?
  diota_args = []
  testarg = experiment.benchmark.inputs["test"][0]
  args = { "exe": exe, "analysis": "dyncfg", "pwd": experiment.path, "xml": xml, "stdoutfile": outfile, "args": testarg, "diota_home": settings.normalization_diota_dir }
  for arg in args:
	diota_args.append("%s = %s" % (arg, repr(args[arg])))

  
  onlinux.python(["diota.diota(%s)" % ",".join(diota_args)], imports=["diota"], pwd=experiment.path, outfile=outfile, append=True, logerror=True)



def dump_bbl(tree, base):
  out = open("%s.bbls" % base, "w")

  for bbl in tree.iter("bbl"):
        start = int(bbl.attrib['from_bbl'], base=16)
        out.write("0x%x\n" % (start))
  out.close()

  out = open("%s.bblsnr" % base, "w")
  for bbl in tree.iter("bbl"):
        nr = int(bbl.attrib['nr'])
        out.write("%d\n" % (nr))
  out.close()


def dump_insts(tree, base):
  out = open("%s.insts" % base, "w")
  for ins in tree.iter("ins"):
        start = int(ins.attrib['address'], base=16)
        out.write("0x%x\n" % start)
  out.close()


def dump_edges(tree, base):
  out = open("%s.edges" % base, "w")
  
  # Okay, somewhat fugly: the edges file from_edge is the final byte of the edge's instruction,
  # so we make a map from all end addresses of instructions to all start addresses of instructions.
  # We make this map by observing the start of instructions, and by how long the hexadecimal string representation of the instruction is. YUCK
  # Maybe I should patch diota after all :)
  
  ins_end_to_begin = { }
  for ins in tree.iter("ins"):
        start = int(ins.attrib['address'], base=16)
        byte_length = len(ins.text) / 2 # 2 nibbles per byte
        stop = start + byte_length - 1 # Last byte *starts* at this address, ends one byte later...
        ins_end_to_begin[stop] = start

  for edge in tree.iter("edge"):
        start = int(edge.attrib['from_edge'], base=16)
        stop  = int(edge.attrib['to_edge'], base=16)
        previous = int(edge.attrib['previous'], base=16)
        if previous in ins_end_to_begin:
              out.write("0x%x 0x%x 0x%x\n" % (ins_end_to_begin[start], stop, ins_end_to_begin[previous]))
        else:
              out.write("0x%x 0x%x 0x0\n" % (ins_end_to_begin[start], stop))
  out.close()

def generate_dynamic_info(experiment, exe):
  out = "%s%s.normalization_dyncfg.out" % (experiment.path, exe)
  xml = "%s%s.normalization_dyncfg.xml" % (experiment.path, exe)

  runDiota(experiment, exe, outfile=out, xml=xml, inputs="test")
  
  base = multiplatform.dir_entry(experiment.path, experiment.config.trace_prefix)
  
  tree = ElementTree()
  tree.parse(multiplatform.asLocal(xml))

  dump_bbl(tree, base)
  dump_insts(tree, base)
  dump_edges(tree, base)
