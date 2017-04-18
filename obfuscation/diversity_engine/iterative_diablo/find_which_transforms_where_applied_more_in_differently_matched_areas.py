#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import re
import sqlite3    as     sql
import sys
from   xml.etree.ElementTree import ElementTree

import utils

### INPUT PARAMETERS ###
old_mapped=sys.argv[1]
old_log=sys.argv[2]
new_mapped=sys.argv[3]
new_log=sys.argv[4]
mapfile2=sys.argv[5]

# Returns instructions in the original binary
def mapped_insts(f):
  s = set()
  for line in open(f):
	s.add(int(line.split(",")[0], 16))
  return s

def orig_addr_to_diablo_fun(mapfile):
  xml = ElementTree()
  xml.parse(mapfile)
  insns = list(xml.iter("ins"))

  m = {}

  for ins in insns:
	orig_address = int(ins.text, base=16)
	if orig_address != 0:
	  m[orig_address] = int(ins.attrib['orig_function_first'], base=16)

  return m

def function_transforms_applied(logfile):
  transforms = {}

  for line in utils.grep("DEBUGAPPLY", logfile):
	info = line.split(",")
	orig_function = int(info[0].split(":")[2], 16)
	transform = info[4][1:len(info[4])-1] # Chomp of initial space and final \n
	
	utils.maybe_initialize_map(transforms, orig_function, {})
	utils.add_or_initialize_to(transforms[orig_function], transform, 1)

  return transforms

def map_difference(m1, m2):
  for key in set(m1.keys() + m2.keys()):
	print "\t%s: %i" % (key, utils.map_entry_or_default(m2, key, 0) - utils.map_entry_or_default(m1, key, 0))

def print_differences_per_function(funs_bad, counts_old, counts_new):
  for bad in funs_bad:
	if bad not in counts_new:
	  print "%x NOT IN NEW?????" % bad
	else:
	  if bad in counts_old:
		print "%x" % bad
		map_difference(counts_old[bad], counts_new[bad])
	  else:
		print "%x NOT IN OLD!" % bad
		#sys.exit(-1)

def print_difference_aggregated_for_functions(funs_bad, counts_old, counts_new):
  map = {}
  for bad in funs_bad:
	if bad not in counts_new:
	  print "%x NOT IN NEW?????" % bad
	else:
	  if bad in counts_old:
		m1 = counts_old[bad]
		m2 = counts_new[bad]
		for key in set(m1.keys() + m2.keys()):
		  utils.add_or_initialize_to(map, key, utils.map_entry_or_default(m2, key, 0) - utils.map_entry_or_default(m1, key, 0))

  print str(map)

def print_total(funs_bad, counts_new):
  map = {}
  for bad in funs_bad:
	if bad not in counts_new:
	  print "%x NOT IN NEW?????" % bad
	else:
	  if bad in counts_old:
		m2 = counts_new[bad]
		for key in set(m2.keys()):
		  utils.add_or_initialize_to(map, key, utils.map_entry_or_default(m2, key, 0))

  print str(map)


mapped_in_new = mapped_insts(new_mapped)
mapped_in_old = mapped_insts(old_mapped)
orig_to_fun   = orig_addr_to_diablo_fun(mapfile2)

all_functions = set(orig_to_fun.values())

funs_bad = set()

for orig_2 in mapped_in_new:
  if orig_2 not in mapped_in_old:
	# We've found an instruction that was mapped in the second file, but not in the first!
	# Which original function does this belong to?
	if orig_2 in orig_to_fun:
	  #print "%x" % orig_to_fun[orig_2]
	  funs_bad.add(orig_to_fun[orig_2])
	# else: # it's not in the map file! This is probably because it's newly added code, this shouldn't matter much

counts_old = function_transforms_applied(old_log)
counts_new = function_transforms_applied(new_log)

#print_differences_per_function(funs_bad, counts_old, counts_new)
#print_difference_aggregated_for_functions(funs_bad, counts_old, counts_new)
#print_difference_aggregated_for_functions(all_functions, counts_old, counts_new)

#print_total(funs_bad, counts_new)
#print_total(all_functions, counts_new)

print (len(set(all_functions)))
