#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import sqlite3    as     sql
import sys
from   xml.etree.ElementTree import ElementTree

def file_to_orig(f):
  xml = ElementTree()
  xml.parse(f)
  insns = list(xml.iter("ins"))

  m = {}

  for ins in insns:
	file_address = int(ins.attrib['address'], base=16)
	orig_address = int(ins.text, base=16)
	if orig_address != 0:
	  if orig_address not in m:
		m[orig_address] = [file_address]
	  else:
		m[orig_address].append(file_address)

  return m


def ground_truth_list(map1, map2):
  file1_to_orig = file_to_orig(map1)
  file2_to_orig = file_to_orig(map2)
  truth = []

  for orig in file1_to_orig:
	for file1 in file1_to_orig[orig]:
	  if orig in file2_to_orig: # Is possible that instructions get remove, like a call
		for file2 in file2_to_orig[orig]:
		  truth.append( (file1, file2) )

  return truth

def ground_truth_map_2_to_1(map1, map2):
  file1_to_orig = file_to_orig(map1)
  file2_to_orig = file_to_orig(map2)
  truth = {}
  truthsize = 0

  for orig in file2_to_orig:
	for file2 in file2_to_orig[orig]:
	  if orig in file1_to_orig: # Is possible that instructions get removed, like a call
		file1 = file1_to_orig[orig]
		truthsize += 1
		if file2 in truth:
		  truth[file2].append( file1 )
		else:
		  truth[file2] = file1

  return truth, truthsize

def add_ground_truth_to_BinDiff(truth, bindiff):
  con = None
  try:
	con = sql.connect(bindiff)
  
	cursor = con.cursor()
	cursor.execute("DROP TABLE IF EXISTS ground_truth;")
	cursor.execute("CREATE TABLE 'ground_truth' (address1 BIGINT, address2 BIGINT);")
	
	cursor.executemany("INSERT INTO ground_truth (address1, address2) VALUES (?, ?)", truth)
	con.commit()

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()

def check_ground_truth(map2to1, bindiff):
  con = None
  algomap_ok = {}
  algomap_notok = {}
  
  seen_pairs = set()

  try:
	con = sql.connect(bindiff)
  
	cursor = con.cursor()
	cursor.execute("SELECT functionalgorithm.id, instruction.address1, instruction.address2 FROM instruction LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id ORDER BY functionalgorithm.id;")
	rows = cursor.fetchall()
	
	def mapadd(algo, algomap):
	  if algo in algomap:
		algomap[algo] += 1
	  else:
		algomap[algo] = 1
  
  
	bd_map = {}
	for row in rows:
	  algo  = row[0]
	  addr1 = row[1]
	  addr2 = row[2]
	  
	  pair = (addr1, addr2)
	  if pair in seen_pairs:
		continue
	  
	  seen_pairs.add(pair)
	  
	  if addr2 not in bd_map:
		bd_map[addr2] = {}
	  bd_map[addr2][addr1] = algo

	for addr2 in map2to1:
	  if addr2 in bd_map:
		# Compare mappings
		for addr1 in map2to1[addr2]:
		  if addr1 in bd_map[addr2]:
			mapadd(bd_map[addr2][addr1], algomap_ok) # True mappings:
		  else:
			mapadd(22, algomap_notok)                # 22 is 'special: not matched due to missing mapping'
		  # False positives:
		for addr1 in bd_map[addr2]:
		  if addr1 not in map2to1[addr2]:
			mapadd(bd_map[addr2][addr1], algomap_notok)

	  else:
		# False negative due to missing instruction
		for addr1 in map2to1[addr2]:
		  mapadd(21, algomap_notok)                 # 21 is 'special: not matched - no BD instruction'

	return algomap_ok, algomap_notok

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()

def print_stats_if_in_ground_truth(truthmap, bindiff, correctmatches=True, only_in_actual_binary=True):
  con = None
  algomap_ok = {}
  seen_pairs = set()

  try:
	con = sql.connect(bindiff)
  
	cursor = con.cursor()
	if only_in_actual_binary:
	  cursor.execute("SELECT functionalgorithm.id, instruction.address1, instruction.address2 FROM all_info LEFT JOIN instruction ON all_info.address = instruction.address2 LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id ;")
	else:
	  cursor.execute("SELECT functionalgorithm.id, instruction.address1, instruction.address2 FROM instruction LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id ;")
	rows = cursor.fetchall()
	
	def mapadd(algo, algomap):
	  if algo in algomap:
		algomap[algo] += 1
	  else:
		algomap[algo] = 1  
  
	bd_map = {}
	for row in rows:
	  algo  = row[0]
	  addr1 = row[1]
	  addr2 = row[2]
	  
	  pair = (addr1, addr2)
	  if pair in seen_pairs:
		continue
	  
	  seen_pairs.add(pair)
	  
	  if correctmatches:
		if addr2 in truthmap and addr1 in truthmap[addr2]:
		  mapadd(algo, algomap_ok)
	  else:
		mapadd(algo, algomap_ok)

	return algomap_ok

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()

# Do the function names match? We ALWAYS USE MAP_ORIG FOR V1
def file_to_orig_REAL(f): # itt de file_to_orig die eigk orig_to_file is???
  xml = ElementTree()
  xml.parse(f)
  insns = list(xml.iter("ins"))

  m = {}

  for ins in insns:
	file_address = int(ins.attrib['address'], base=16)
	orig_address = int(ins.text, base=16)
	if orig_address != 0:
	  if file_address not in m:
		m[file_address] = [orig_address]
	  else:
		m[file_address].append(orig_address)

  return m

def is_map_correct_fun(inst_to_fun_orig, inst_to_fun_patched, map_orig, map_v2, addr_v1, addr_v2):
  # Function name of v1: We first map the address to the original binary from the diabloed address. Then we map that to a function name
  
  #if addr_v1 == 134526001: # 134527200
  #print "Hallo"
  
  if addr_v1 not in map_orig:
	return False
 
  addrs_orig = map_orig[addr_v1]
  # print addrs_orig
  
  # We can have multiple instruction mappings; as soon as we have found 1 that is correct, say it's OK
  for addr_orig in addrs_orig:
	if int(addr_orig) == 0:
	  continue
	if addr_orig not in inst_to_fun_orig:
	  continue

	fun_v1 = inst_to_fun_orig[addr_orig]
  
	# Look up v2
	if addr_v2 not in map_v2:
	  continue

	addrs_patched = map_v2[addr_v2]
	
	for addr_patched in addrs_patched:
	  if int(addr_patched) == 0:
		continue
	  if addr_patched not in inst_to_fun_patched:
		continue

	  fun_v2 = inst_to_fun_patched[addr_patched]

	  if fun_v1 == fun_v2:
		return True
	  #else:
	  #print "%s <<<<>>>>  %s" % (fun_v1, fun_v2)

  # We didn't find a correct function mapping :(
  return False

def print_stats_if_function_matched(inst_to_fun_orig, inst_to_fun_patched, map_orig, map_v2, bindiff, whereclause):
  con = None
  algomap_ok = {}
  seen_pairs = set()
  if whereclause is not None:
	where = whereclause
  else:
	where = ""

  try:
	con = sql.connect(bindiff)
  
	cursor = con.cursor()
	if only_in_actual_binary:
	  cursor.execute("SELECT functionalgorithm.id, instruction.address1, instruction.address2 FROM all_info LEFT JOIN instruction ON all_info.address = instruction.address2 LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id ;")
	else:
	  cursor.execute("SELECT functionalgorithm.id, instruction.address1, instruction.address2 FROM instruction LEFT JOIN basicblock ON instruction.basicblockid = basicblock.id LEFT JOIN function ON basicblock.functionid = function.id LEFT JOIN functionalgorithm ON function.algorithm = functionalgorithm.id " + where + " ORDER BY functionalgorithm.id;")
	rows = cursor.fetchall()
	
	def mapadd(algo, algomap):
	  if algo in algomap:
		algomap[algo] += 1
	  else:
		algomap[algo] = 1  
  
	bd_map = {}
	for row in rows:
	  algo  = row[0]
	  addr1 = row[1]
	  addr2 = row[2]
	  
	  pair = (addr1, addr2)
	  if pair in seen_pairs:
		continue
	  
	  seen_pairs.add(pair)
	  
	  if correctmatches:
		if is_map_correct_fun(inst_to_fun_orig, inst_to_fun_patched, map_orig, map_v2, addr1, addr2):
		  mapadd(algo, algomap_ok)
	  else:
		mapadd(algo, algomap_ok)

	return algomap_ok

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()


def orig_instruction_to_function_map(mapfile):
  m = {}
  for line in open(mapfile, "r"):
	(addr_16, fun) = line.split(",")
	m[int(addr_16, base=16)] = fun
  return m

def transform_to_confidence(reason):
  if reason >= 1 and reason <= 3:
	return 1 # Very Good
  if reason <= 7:
	return 2 # Good
  if reason == 11 or reason <= 9:
	return 3 # Medium
  # if reason == : # loop count matching = 'Poor'
  return 4 # Very poor/poor

#map1    = sys.argv[1] # Map of ORIGINAL
#map2    = sys.argv[2] # Map of PATCHED
#bindiff = sys.argv[3] # V1 to V2
##outok   = 

#correctmatches        = sys.argv[4] == "correct"
#only_in_actual_binary = sys.argv[5] == "inbinary"
#coarse                = sys.argv[6] == "coarse"

# TODO decent commandline parsing
if sys.argv[1] == "only_semantic_changes":
  changes_file = sys.argv[2]
  map_v2       = sys.argv[4]
  semchanges   = True

  whereclause = " WHERE instruction.address2 IN ( "
  addrs_transformed = []
  patched_to_transformed_map = file_to_orig(map_v2)

  for line in open(changes_file, "r"):
	addr_patched = int(line, base=16)
	if addr_patched in patched_to_transformed_map:
	  for addr_v2 in patched_to_transformed_map[addr_patched]:
		addrs_transformed.append(str(addr_v2))

  whereclause = whereclause + ", ".join(addrs_transformed) + ") "
  argv = sys.argv[2:]
  #print whereclause

else:
  argv = sys.argv
  whereclause = None
  semchanges   = False

map_orig    = argv[1] # Map of ORIGINAL, mapping P0 -> bzip2_orig
map_v2      = argv[2] # Map of V2, maps diablo(p_I) -> bzip2_patched

bindiff     = argv[3] # orig to V2

inst_to_fun_orig    = argv[4]
inst_to_fun_patched = argv[5]

##outok   = 

correctmatches        = argv[6] == "correct"
only_in_actual_binary = argv[7] == "inbinary"
coarse                = argv[8] == "coarse"
use_funm_for_correct  = argv[9] == "use_functions"

#add_ground_truth_to_BinDiff(ground_truth(map1, map2), bindiff)

# truthmap, truthsize = ground_truth_map_2_to_1(map1, map2)
# ok_raw = print_stats_if_in_ground_truth(truthmap, bindiff, correctmatches=correctmatches, only_in_actual_binary=only_in_actual_binary)

ok_raw = print_stats_if_function_matched(orig_instruction_to_function_map(inst_to_fun_orig), orig_instruction_to_function_map(inst_to_fun_patched),
										 file_to_orig_REAL(map_orig), file_to_orig_REAL(map_v2), bindiff, whereclause=whereclause)

#ok, notok = check_ground_truth(truthmap, bindiff)

if coarse:
  ok = {}
  for a in range(1, 5):
	ok[a] = 0
  for a in range(1, 21):
	if a in ok_raw:
	  ok[ transform_to_confidence(a) ] += ok_raw[a]
 
else:
  ok = ok_raw

seen = 0
for a in range(1, 21):
	if a in ok:
	  print ok[a]
	  seen += ok[a]
	else:
	  print 0

# Extra row, that contains the number of instructions that had no (correct) match
if semchanges:
  #print "-"
  print len(addrs_transformed) - seen

#if outok:
  #for a in range(1, 23):
	#if a in ok:
	  #print float(ok[a]) / float(truthsize)
	#else:
	  #print 0
#else:
  #for a in range(1, 23):
	#if a in notok:
	  #print float(notok[a]) / float(truthsize)
	#else:
	  #print 0


#print truthsize

