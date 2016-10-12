#!/usr/bin/python
import sys

tomerge = sys.argv[1:]

profile = {}

def add_profile(addr, count):
  if addr in profile:
	profile[addr] += count
  else:
	profile[addr] = count

for f in tomerge:
  for l in open(f):
	(addr, count) = l.split(" ")
	add_profile(addr, int(count))

for addr in profile:
  print "%s %i" % (addr, profile[addr])
