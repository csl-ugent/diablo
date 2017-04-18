# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import sys
from   xml.etree.ElementTree import ElementTree

import benchmarks
import diablo
import diablo_iterative
import experiments
import utils

def readExecCountFromMapping(mapfile):
  tree = ElementTree()
  tree.parse(mapfile)
  map = {}
  
  for ins in tree.iter("ins"):
	orig_function = int(ins.attrib['orig_function_first'], base=16)
	execcnt       = int(ins.attrib['guessed_train_exec_count'])
	if orig_function != 0:
	  utils.add_or_initialize_to(map, orig_function, execcnt)

  return map

def readExecCountFromTrace(tracefile, mapfile):
  tree = ElementTree()

  tree.parse(mapfile)
  addr_to_fun = {}
  
  for ins in tree.iter("ins"):
	orig_function = int(ins.attrib['orig_function_first'], base=16)
	exe_addr      = int(ins.attrib['address'], base=16)
	if orig_function != 0:
	  addr_to_fun[exe_addr] = orig_function

  map = {}
  for line in open(tracefile, "r"):
	s   = line.split(",")
	ins = int(s[0], base=16)
	cnt = int(s[1])
	
	if ins in addr_to_fun:
	  utils.add_or_initialize_to(map, addr_to_fun[ins], cnt)

  return map

def cnt_or_default(map, key, default=0):
  if key in map:
	return map[key]
  else:
	return default

def total_execution_count(tracefile):
  cnt = 0
  for line in open(tracefile, "r"):
	s    = line.split(",")
	cnt += int(s[1])
  return cnt

def compareExecCounts(cnt1, cnt2, totalcnt=None):
  seen = set()
  if totalcnt is not None:
	cnt  = float(totalcnt)

  for fun in cnt1.keys() + cnt2.keys():
	if fun in seen:
	  continue
	seen.add(fun)

	c1 = cnt_or_default(cnt1, fun)
	c2 = cnt_or_default(cnt2, fun)
	
	if c1 != c2:
	  diff = float(abs(c1-c2))
	  if totalcnt is not None:
		print "%i: %f - %f - %i,%i" % (fun, diff/float(max(c1, c2)), diff/cnt, c1, c2)
	  else:
		print "%i: %f - %i,%i" % (fun, diff/float(max(c1, c2)), c1, c2)

class SimpleExperiment:
  def __init__(self, benchmark):
	self.benchmark = benchmark

# Commandline args: dotrace       <benchmark name> <iteration> <trace profile>
#                   comparetrace  <benchmark name> <iteration> <trace profile>
if __name__ == "__main__":
  benchmark  = benchmarks.__dict__[sys.argv[1]]
  iteration1 = int(sys.argv[2])
  iteration2 = int(sys.argv[3])
  
  compareExecCounts(readExecCountFromMapping(diablo.map_filename(iteration1)), readExecCountFromMapping(diablo.map_filename(iteration2)))
  
  sys.exit(0)
  profile    = sys.argv[3]
  
  
  # TODO: configurable experiment names
  binary = diablo_iterative.binary_iteration_filename(SimpleExperiment(benchmark), iteration)
  
  tracefile = "%s_%s_trace.all" % (binary, profile)
  mapfile   = diablo.map_filename(iteration)
  
  fromMap   = readExecCountFromMapping(mapfile)
  fromTrace = readExecCountFromTrace(tracefile, mapfile)
  
  total     = total_execution_count(tracefile)
  
  compareExecCounts(fromMap, fromTrace, total)
