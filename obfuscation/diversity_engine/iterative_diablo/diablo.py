# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import copy
import logging
import re
import os
import shutil                as     shutil
import sqlite3               as     sql
from   subprocess            import call
import sys
import xml.etree
from   xml.etree.ElementTree import ElementTree

import multiplatform
import onlinux
import utils
from   settings import settings

useWindows       = False
useDiabloOutFile = False

class SimpleConf:
  def __init__(self):
	self.experiment_dir = ""
	self.extend_IDA_analysis = True

def map_filename(iteration):
  return "mapping.xml.%i" % iteration

def diablo_fun_addr(instaddr, mapdata, ins2fun):
  # The instruction addresses of what BinDiff thinks are the function starts, in the transformed binary
  f = hex(instaddr)
  
  # That address in the original binary:
  found = re.findall(f + '">([^ ]*)', mapdata)
  
  # It's possible that this instruction was disassembled incorrectly, and thus has no match in the mapping file
  if len(found) == 0:
	return None

  fo=found[0].split('x')[1]

  # The diablo function addresses of that
  return re.findall(fo + " (.*)", ins2fun)[0]


def transformed_to_orig_map(mapfile): ### TODO mwhuu, wrong way around?
  result = {}
  
  tree = ElementTree()
  tree.parse(mapfile)
  
  for ins in tree.iter("ins"):
	origaddr = int(ins.attrib['address'], base=16)
	if origaddr != 0:
	  #result[int(ins.text, base=16)] = origaddr # TODO, multiple source addresses
	  result[origaddr] = int(ins.text, base=16)# TODO, multiple source addresses

  return result

def orig_to_transformed_map(mapfile):
  result = {}
  
  tree = ElementTree()
  tree.parse(mapfile)
  
  for ins in tree.iter("ins"):
	fileaddr = int(ins.attrib['address'], base=16)
	for orig_addr_text in ins.text.split(" "):
	  if orig_addr_text != "":
		orig_addr = int(orig_addr_text, base=16)
		utils.add_to_map_of_list(result, orig_addr, fileaddr)

  return result

def all_insts(mapfile):
  result = []
  
  tree = ElementTree()
  tree.parse(mapfile)
  
  if xml.etree.ElementTree.VERSION.startswith("1.3."):
	it = tree.iter("ins")
  else:
	it = tree.getiterator("ins")
  
  for ins in it:
	fileaddr = int(ins.attrib['address'], base=16)
	result.append(fileaddr)

  return result


# The Files class has a files map, which maps a human-readable shorthand to an actual filename 'pair'. The snd of this pair says if the file gets a 'unique' filename automatically
class DiabloFiles:
  def __init__(self):
	self.files = {
	  "binary": (None, True),
	  "listfile": (None, True),
	  "mapping": ("mapping.xml", False),
	  "logfile": (None, True)
	}


def runIterative(experiment, iinput, ioutput, in_binary, out_binary, suffix):
  try:
	### TODO: -T?

	outfilename = "%s.diablo.out" % out_binary
	diabloout = open(outfilename, "w")

	has_input = []
	if iinput is not None:
	  has_input = ["-dii", iinput]


	args = get_config_arguments(experiment.config) + has_input + ["-dio", ioutput, "-o", out_binary, in_binary]
	
	#if useWindows:
	  #args = args + [ "-T", multiplatform.asWindows(settings.linkerFile) ]
	#else:
	  #args = args + [ "-T", settings.linkerFile ]

	diabloout.write("# Called diablo with arguments:\n#%s\n" % " ".join(args))
	
	#print diablo_binary
	print args
	if useDiabloOutFile:
	  stdout=diabloout
	else:
	  stdout=None
	  
	if useWindows:
	  call([diablo_binary] + args, stdout=stdout)
	else:
	  diabloout.close() ### 
	  print "HALLOO"
	  print settings.linux_diablo
	  onlinux.call(settings.linux_diablo, args, pwd=experiment.path, outfile=outfilename)

	shutil.move("mapping.xml", "mapping.xml.%i" % suffix)
	shutil.move("functionsdump", "functionsdump.%i" % suffix)
	### ### shutil.move("instructions_to_function.mapping", "instructions_to_function.mapping.%i" % suffix)
	shutil.move("extra_iterative_info_file", "extra_iterative_info_file.%i" % suffix)
    
  except OSError, e:
	print "Error:"
	print e
	sys.exit(1)




class DiversityConfigBase:
  def __init__(self):
	self.transformations = copy.deepcopy(DiversityConfigBase._transformations)
	self.instruction_seed = None
	self.costs = copy.deepcopy(DiversityConfigBase._costs)
	self.extend_IDA_analysis = False
	
	self.allowed_transforms_per_function_file        = None
	self.allowed_transforms_per_function_output_file = None
	self.matched_instructions_and_why_file           = None
	self.profile_file                                = None
	
	self.ifStillMappedThenRetransformFromScratch     = True
	self.usesTransformPerFunctionFile                = False
	
	self.transformation_rules                        = None
	
	self.directory                                   = None

  _transformations = {
	"FlipBranches":         False,
	"OpaquePredicates":     False,
	"FlattenFunctions":     False,
	"InlineFunctions":      False,
	"DisassemblyThwarting": False,
	"TwoWayPredicates":     False,
	"UnfoldBasicBlocks":    False,
	"InstructionSelection": False
  }
  
  _costs = {
	"InitialMaxCost":    0,
	"RestartDivider":    2,
	"MaxWithCost":       4,
	"CostIncrease":      1
  }

### DIABLO CONFIG ###
diablo_transforms_mapper = {
  "FlipBranches":         "-dbj",
  "OpaquePredicates":     "-doo",
  "FlattenFunctions":     "-dof",
  "InlineFunctions":      "-dif",
  "TwoWayPredicates":     "-dit",
  "DisassemblyThwarting": "-dot",
  "UnfoldBasicBlocks":    "-dib",
  "InstructionSelection": "-dbi"
}

# TODO: ook input & output
def get_config_arguments(config):
  result = []

  # Transformations
  for transform in config.transformations:
	if config.transformations[transform]:
	  result.append(diablo_transforms_mapper[transform])

  # Costs
  #result = result + [
	  #"-dimc", str(config.costs["InitialMaxCost"]),
	  #"-drd",  str(config.costs["RestartDivider"]),
	  #"-dmwc", str(config.costs["MaxWithCost"]),
	  #"-dci",  str(config.costs["CostIncrease"])
	#]

  if config.instruction_seed is not None:
	result = result + [ "--orderseed", str(config.instruction_seed) ]

  if config.initial_seed is not None:
	result = result + [ "-drs", str(config.initial_seed) ]

  # Input and output files
  if config.allowed_transforms_per_function_file is not None:
	result = result + [ "-datpff", config.allowed_transforms_per_function_file ]

  if config.allowed_transforms_per_function_output_file is not None:
	result = result + [ "-datpfof", config.allowed_transforms_per_function_output_file ]
  
  if config.matched_instructions_and_why_file is not None:
	result = result + [ "-dmiawf", config.matched_instructions_and_why_file ]
	logging.info("DiabloConfig: Mathched Instructions File used is %s", config.matched_instructions_and_why_file)


  if config.ifStillMappedThenRetransformFromScratch:
	result = result + [ "-dismrfs", "on" ]
  else:
	result = result + [ "-dismrfs", "off" ]

  if config.profile_file is not None:
	result = result + [ "-pb", "%s/%s" % (config.directory, config.profile_file), "--rawprofiles", "off" ]

  if config.transformation_rules is not None:
    result = result + [ "-dir", config.transformation_rules ]

  result = result + [ "-off", "on", "-obf", "on", "-oop", "on", "-osofh", "-osoc", "on", "-ofb", "on", "-ostp", "on" ]
  
  if config.isThumb2:
    result = result + [ "--fullthumb2", "on" ]
  
  result = result + [ "-O", config.directory, "-L", config.directory ]

  return result
