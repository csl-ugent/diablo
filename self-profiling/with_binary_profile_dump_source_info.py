#!/usr/bin/python
import struct
import sys

# call file as follows:
# Profile information per function and basic block: with_binary_profile_dump_source_info.py profiling_section.b.out source_info <b.out.src_profile_info>
# Weighted metrics:                                 with_binary_profile_dump_source_info.py profiling_section.b.out metrics <b.out.metrics>

# returns map[address] -> count
def read_profile(fn):
  result = {}

  f = open(fn)

  data = f.read(16)
  while data != "":
    (address, count) = struct.unpack('QQ', data)

    result[address] = count

    data = f.read(16)

  return result

def add_to_map(map, item, count):
  if item not in map:
    map[item] = count
  else:
    map[item] += count

def max_to_map(map, item, count):
  if item not in map:
    map[item] = count
  else:
    map[item] = max(map[item], count)


def read_source_info(fn, profile):
  # <BBL OLD ADDRESS> <SRC FILE> <SRC LINE NUMBER> <NR. INS> <FUNCTION NAME> <BBL IS ENTRY FUNCTION?>
  entry_set = set()
  bbls_seen = set()
  line_to_bbl_old = {}
  total_weight = 0

  functions_weight = {}

  times_called = {}

  for line in open(fn):
    # Skip comments
    if line.startswith("#"):
      continue

    s = line.split("\t")
    
    old_addr = int(s[0], base=16)
    src_file = s[1]
    src_line = s[2]
    nr_ins   = int(s[3])
    function = s[4]
    is_entry = int(s[5]) == 1

    exec_count = profile[old_addr]
    weight     = nr_ins * exec_count

    if src_line != '-' and src_file != '-':
      line = src_file + ':' + src_line

      max_to_map(line_to_bbl_old, (line, function), exec_count)

    # To compute the weight of a function, we only take into account its BBLs once:
    if old_addr not in bbls_seen:
      bbls_seen.add(old_addr)

      add_to_map(functions_weight, function, weight)

      total_weight += weight

      if is_entry:
        entry_set.add(old_addr)
        add_to_map(times_called, function, exec_count)

  return { "lines":            line_to_bbl_old,
           "functions_weight": functions_weight,
           "functions_called": times_called,
           "total_weight"    : total_weight }

def align_to(str, to):
  return str + (to - len(str)) * " "

def max_width(l):
  return reduce(max, l, 0)

def print_info(info, print_zero_weights=False):
  # hot lines
  print "# Max. Execution count of lines:"
  print "# FILE:LINE\tMAX EXEC COUNT (FUNCTION NAME):"

  width = max_width( map( lambda ( (line, function), count ): len(line), info["lines"].items() ) ) + 2

  for ( (line, function) , count) in sorted(info["lines"].items(), key=lambda (l, c): c, reverse=True):
    if print_zero_weights or count > 0:
      print "%s\t%i (%s)" % (align_to(line, width), count, function)

  width = max_width( map( lambda (function, count): len(function), info["functions_weight"].items() ) ) + 2
  print "# Weight of functions:"
  print "# FUNCTION\tWEIGHT IN %% OF TOTAL\tTIMES CALLED"
  for (function, count) in sorted(info["functions_weight"].items(), key=lambda (f, c): c, reverse=True):
    if print_zero_weights or count > 0:
      print "%s\t%f%%\t%i" % (align_to(function, width), float(count) / info["total_weight"], info["functions_called"][function])

def print_total_weighted_metrics(metricsfile, profile):
  vectors = None
  for line in open(metricsfile):
    if line == "\n" or line == "" or line.startswith("#"):
      continue
    s = line.split("\t")

    if vectors is None:
      vectors = (len(s) - 1) * [0] # The first row is the bbl address

    bbl = int(s[0], base=16)
    exec_count = profile[bbl]

    for idx, val in enumerate(s[1:]):
      vectors[idx] += exec_count * int(val)

  print "#Weighted metrics are:"
  print "\t".join(map(str, vectors))

def print_usage():
  print "Usage:"
  print "Profile information per function and basic block: with_binary_profile_dump_source_info.py profiling_section.b.out source_info <b.out.src_profile_info>"
  print "Weighted metrics:                                 with_binary_profile_dump_source_info.py profiling_section.b.out metrics <b.out.metrics>"
  sys.exit(0)

if len(sys.argv) <= 3:
  print_usage()

profile = read_profile(sys.argv[1])

if sys.argv[2] == "source_info":
  info = read_source_info(sys.argv[3], profile)
  print_info(info, print_zero_weights=False)
elif sys.argv[2] == "metrics":
  print_total_weighted_metrics(sys.argv[3], profile)
else:
  print_usage()
