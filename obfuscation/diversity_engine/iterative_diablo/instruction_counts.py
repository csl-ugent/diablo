#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import os
import stat
from   subprocess import call
import sys
import time

import benchmarks
import onlinux
import utils

pin = "/home/bcoppens/diversity/pin/pin-2.11-49306-gcc.3.4.6-ia32_intel64-linux/pin"
count_so = "/home/bcoppens/diversity/pin/pin-2.11-49306-gcc.3.4.6-ia32_intel64-linux/source/tools/IterativeDiablo/obj-ia32/countins.so"

def filename(exe, input, iteration, what):
  return "%s_%s_%s.%s" % (exe, input, what, str(iteration))

def doTrace(exe, output = "inscount.out", args = []):
  os.chmod(exe, stat.S_IREAD | stat.S_IWRITE | stat.S_IEXEC)

  call([pin, "-t", count_so, "-o", output, "--", exe] + args)

def mergeTraces(exe, input, range):
  merged = {}
  for i in range:
	for line in open(filename(exe, input, i, "trace"), "r"):
	  s = line.split(",")
	  utils.add_or_initialize_to(merged, s[0], long(s[1]))

  out = open(filename(exe, input, "all", "trace"), "w")
  for a in merged:
	out.write("%s,%s\n" % (a, str(merged[a])))
  out.close()

def traceBenchmark(exe, benchmark, input):
  i = 0
  for args in benchmark.inputs[input]:
	doTrace(exe, output = filename(exe, input, i, "trace"), args = args)
	i = i + 1

  mergeTraces(exe, input, range(0, i))

#bzip2_train_args = [ ["input.program", "10"], ["byoudoin.jpg", "5"], ["input.combined", "80"] ]

#i = 0
#for args in bzip2_train_args:
  #doTrace(sys.argv[1], "%s_train_trace_%i" % (sys.argv[1], i), args)
  #i = i + 1

# TODO: move to another module?
# TODO aaa beter! (hardcoded :'()
def execution_time(exe, experiment, input, iterations = 1):
  # There is a WEIRD ISSUE where when I ssh, the file doesn't exist! See if sleeping helps
  onlinux.call("chmod a+x %s; /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/benchmarks/run_benchmark.sh /home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/benchmarks/soplex/soplex_train.sh ./%s" % (exe, exe), pwd=experiment.path, outfile=filename(exe, "train", "all", "time"), append=False)

def traceRemote(exe, experiment):
  onlinux.call("/afs/elis.ugent.be/usr/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/instruction_counts.py ./%s" % exe, pwd=experiment.path, outfile="pin.out.%s" % exe)

# TODO ### even MORE AAAAA BETER (hardcoded? :'()
if __name__ == "__main__":
    #traceBenchmark(sys.argv[1], benchmarks.bzip2, "train")
    
    os.chdir("/home/bcoppens/diversity/compiled/soplexO3_mergesort/O3/")
    traceBenchmark("./soplex", benchmarks.soplex, "train")
    
    os.chdir("/home/bcoppens/diversity/compiled/bzip2O3_patched/O3/")
    traceBenchmark("./bzip2", benchmarks.bzip2, "train")
