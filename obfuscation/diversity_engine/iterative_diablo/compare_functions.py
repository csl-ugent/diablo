#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import sys
from   xml.etree.ElementTree import ElementTree

import diablo
import utils

# ./compare_functions.py 
# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py dyninstsoplex_iterated_binary_0._functiondump  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.0   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.0

# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/soplex_iterated_binary_0.idb.ida_funs--full  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.0   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.0

# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/soplex_iterated_binary_0.idb.ida_funs--simple  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.0   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.0

# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/soplex_iterated_binary_0.idb.ida_funs  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.0   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.0

# SOPLEX 20:
# 
# IDA SIMPLE
# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/soplex_iterated_binary_20.idb.ida_funs--simple  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.20   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.20 | tail

# IDA EXTENDED
# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/soplex_iterated_binary_20.idb.ida_funs--full  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.20   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.20 | tail

# DYNINST
# /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/compare_functions.py dyninstsoplex_iterated_binary_20._functiondump  //media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/mapping.xml.20   /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.20 | tail

class NotFound:
  def __init__(self, reason):
	self.reason = reason
  def __repr__(self):
	return self.reason

def open_functionsdump(file):
  addr_to_set = {}
  for line in open(file, "r"):
	addrs = line.split(" ")
	s = set()
	fun = int(addrs[0], base=16)
	assert addrs[1] == '' # there is a spurious space
	for addr in addrs[2:]:
	  a = int(addr, base=16)
	  s.add(a)
	addr_to_set[fun] = s
  return addr_to_set

def function_for_orig_addr(file):
  a_to_f = {}
  for line in open(file, "r"):
	fs = line.split(",")
	addr = int(fs[0], base=16)
	fun = fs[1]
	a_to_f[addr] = fun
  return a_to_f

def sets_of_insts_to_sets_of_funs(sets, funmap):
  nm = {}
  for fun in sets:
	fm = set()
	for inst in sets[fun]:
	  if inst in funmap:
		fm.add(funmap[inst])
	  else:
		print "NOT FOUND IN FUNMAP: %x" % inst  # fm.add(NotFound("in funmap %s" % str(inst))) # TODO
	nm[fun] = fm
  return nm

def transformed_to_orig_map(mapfile): # the one in diablo. package is WRONG? TODO TODO
  result = {}
  
  tree = ElementTree()
  tree.parse(mapfile)
  
  for ins in tree.iter("ins"):
	fileaddr = int(ins.attrib['address'], base=16)
	for orig_addr_text in ins.text.split(" "):
	  if orig_addr_text != "":
		orig_addr = int(orig_addr_text, base=16)
		# utils.add_to_map_of_list(result, fileaddr, orig_addr)
		# TODO TODO
		result[fileaddr] = orig_addr

  return result


def map_all_to_orig(fset, to_orig):
  r = {}
  for fun in fset:
	new_set = set()
	for ins in fset[fun]:
	  if ins in to_orig:
		new_set.add(to_orig[ins])
	  else:
		print "NOT FOUND IN ORIG: %x" % ins # new_set.add(NotFound("in to_orig: %x" % ins))
	# print "FUN IS %x" % fun
	if fun not in to_orig:
	  print "ERROR NOT IN ORIG %x" % fun
	  #assert False
	else:
	  r[to_orig[fun]] = new_set
  return r

def group_functions(addr_to_fun):
  result = {}
  for addr in addr_to_fun:
	utils.add_to_map_of_list(result, addr_to_fun[addr], addr)
  return result

def inst_to_its_function_base(map_base_to_insts):
  result = {}
  for base in map_base_to_insts:
	for addr in map_base_to_insts[base]:
	  result[addr] = base #utils.add_to_map_of_list(result, addr, base) <- maps PER INSTRUCTION -> its single base
  return result

functions_set_transformed = open_functionsdump(sys.argv[1])
transformed_to_orig_map = transformed_to_orig_map(sys.argv[2])
function_names_orig = function_for_orig_addr(sys.argv[3])

functions_set_orig = map_all_to_orig(functions_set_transformed, transformed_to_orig_map)

funs_computed_into_actual_funcs_orig = sets_of_insts_to_sets_of_funs(functions_set_orig, function_names_orig)
 
maybecorrect = 0
wrong = 0
for fun in funs_computed_into_actual_funcs_orig:
  if len(funs_computed_into_actual_funcs_orig[fun]) == 1:
	maybecorrect+=1
  else:
	wrong+=1

print "MAYBE CORRECT: %i" % maybecorrect
print "WRONG: %i" % wrong

function_to_set_of_orig = group_functions( function_names_orig)
computed_functions_entrypts_orig = inst_to_its_function_base(functions_set_orig)

ok = 0
few = 0
many = 0
processed = 0
missing = 0
both = 0

for fun in function_to_set_of_orig:
  correct_set_orig = set(function_to_set_of_orig[fun])
  base_addr_orig = min(correct_set_orig)# TODO
  if base_addr_orig not in computed_functions_entrypts_orig:
	missing += 1
	continue
  computed_set_orig = functions_set_orig[computed_functions_entrypts_orig[base_addr_orig]]
  
  _few = len(correct_set_orig - computed_set_orig) > 0
  _many = len(computed_set_orig - correct_set_orig) > 0
  if _few:
	#print "A:"
	# print correct_set_orig - computed_set_orig
	#print "Actual:"
	#print correct_set_orig
	#print "Computed:"
	#print computed_set_orig
	few += 1
  if _many:
	#print "B:"
	#print computed_set_orig - correct_set_orig
	many += 1
  if _few and _many:
	both += 1
  if len(computed_set_orig | correct_set_orig) == len(computed_set_orig):
	ok += 1
  #print base_addr_orig

  #SYS.EXIT()
  processed += 1
  
print "Processed: %i" % processed
print "TOO MANY: %i" % many
print "TOO FEW: %i" % few
print "BOTH: %i" % both
print "OK: %i" % ok
print "MISSING: %i" % missing

#print functions_set_orig[computed_functions_entrypts_orig[0x813afd0]]

#B:
#set([134796620]) - 0x808D54C
#134977232 - 0x80B96D0

#<ins address="-->0x808d54c<--">0x80b96d0 </ins>
#[bcoppens@ecthelion src]$ grep -i 80b96d0 /media/1cf273de-b073-441c-9c4e-330c60320e4a/data/diversity/experiments/normalization/exploratory/soplex_iterative/7_soplex_1call_nocallee_yesbranchfun_newrules_improved/functionsdump.0
#80b96d0,/home/bcoppens/diversity/newtoolchain/gcc-4.6-stdcpp/gcc-4.6.2/install/lib/gcc/x86_64-unknown-linux-gnu/4.6.2/../../../../lib/libstdc++.a:locale-inst.o_ZNSt18__moneypunct_cacheIcLb1EE8_M_cacheERKSt6locale


#A:
#Actual:
#set([135507969, 135507972, 135507975, 135507978, 135507984, 135507987, 135507993, 135507996, 135507999, 135508002, 135508008, 135508014, 135508019, 135508021, 135508023, 135508026, 135508028, 135508030, 135508033, 135508036, 135508038, 135508044, 135508046, 135508052, 135508055, 135508057, 135508063, 135508065, 135508067, 135508073, 135508078, 135508083, 135508089, 135508091, 135508096, 135508098, 135508100, 135508102, 135508108, 135508111, 135508114, 135508116, 135508119, 135508125, 135508128, 135508131, 135508137, 135508139, 135508141, 135508144, 135508146, 135508149, 135508152, 135508155, 135508161, 135508163, 135508168, 135508171, 135508174, 135508176, 135508178, 135508184, 135508187, 135508190, 135508192, 135508199, 135508207, 135508213, 135508217, 135508222, 135508228, 135508234, 135508240, 135508246, 135508252, 135508258, 135508261, 135508262, 135508264, 135508267, 135508273, 135508275, 135508280, 135508286, 135508293, 135508299, 135508302, 135508306, 135508312, 135508316, 135508322, 135508326, 135508332, 135508336, 135508342, 135508349, 135508356, 135508359, 135508366, 135508373, 135508377, 135508382, 135508388, 135508390, 135508393, 135508395, 135508397, 135508403, 135508405, 135508411, 135508414, 135508416, 135508419, 135508422, 135508426, 135508429, 135508433, 135508436, 135508441, 135508447, 135508451, 135508457, 135508461, 135508467, 135508471, 135508477, 135508481, 135508487, 135508491, 135508494, 135508499, 135508501, 135508503, 135508509, 135508512, 135508518, 135508520, 135508522, 135508528, 135508530, 135508536, 135508538, 135508541, 135508545, 135508547, 135508550, 135508553, 135508559, 135508564, 135508569, 135508575, 135508577, 135508578, 135508579, 135508580, 135508581, 135508582, 135508584, 135508586, 135508589, 135508591, 135508597, 135508599, 135508605, 135508607, 135508613, 135508619, 135508625, 135508627, 135508630, 135508632, 135508635, 135508637, 135508640, 135508642, 135508644, 135508650, 135508656, 135508662, 135508664, 135508670, 135508672, 135508674, 135508676, 135508678, 135508684, 135508689, 135508692, 135508695, 135508696, 135508702, 135508708, 135508714, 135508720, 135508722, 135508725, 135508728, 135508732, 135508735, 135508737, 135508740, 135508743, 135508747, 135508750, 135508755, 135508761, 135508764, 135508770, 135508771, 135508774, 135508776, 135508779, 135508781, 135508784, 135508790, 135508792, 135508795, 135508797, 135508799, 135508802, 135508808, 135508810, 135508816, 135508822, 135508828, 135508830, 135508832, 135508834, 135508836, 135508839, 135508845, 135508847, 135508849, 135508852, 135508855, 135508858, 135508861, 135508865, 135508868, 135508873, 135508875, 135508881, 135508887, 135508893, 135508899, 135508901, 135508903, 135508906, 135508908, 135508914, 135508916, 135508922, 135508924, 135508930, 135508932, 135508938, 135508940, 135508946, 135508948, 135508950, 135508952, 135508958, 135508960, 135508962, 135508964, 135508966, 135508968, 135508970, 135508971, 135508973, 135508979, 135508981, 135508983, 135508984, 135508990, 135508997, 135508999, 135509002, 135509003, 135509008, 135509011, 135509014, 135509016, 135509018, 135509023, 135509026, 135509031, 135509037, 135509043, 135509045, 135509051, 135509053, 135509055, 135509057, 135509059, 135509064, 135509069, 135509075, 135509078, 135509082, 135509088, 135509092, 135509098, 135509102, 135509108, 135509112, 135509118, 135509122, 135509125, 135509130, 135509133, 135509136, 135509140, 135509142, 135509145, 135509148, 135509152, 135509155, 135509158, 135509163, 135509168, 135509170, 135509172, 135509175, 135509181, 135509187, 135509192, 135509195, 135509198, 135509204, 135507920, 135507921, 135507923, 135507924, 135507925, 135507930, 135507931, 135507937, 135507943, 135507946, 135507949, 135507951, 135507954, 135507960, 135507963])
#Computed:
#set([135507920])
#135507920



