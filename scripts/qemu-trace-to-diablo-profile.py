#!/usr/bin/python

import sys
import collections
import os

def help():
  print 'Usage: ',sys.argv[0],' qemu_logfile new_diablo_profile_file'
  print 'The logfile must have been generated with qemu-user -d exec,in_asm'
  print 'Note that for Qemu releases before 1.5.0, you have to create a debug build or otherwise the exec tracing will silently do nothing.'
  print 'The resulting profile can be used both for as basic block (-pb) and instruction trace (-pi) profile input for diablo.'
  sys.exit(1)


if len(sys.argv) != 3:
  help()

mapping = {}
profile = collections.Counter()
with open(sys.argv[1],'r') as fin, open(sys.argv[2],'w+') as fout:

  stop = False;
  while True:
    s = fin.readline()
    if not s:
      break

    if s.startswith('IN:'):
      # a translated pseudo-bbl
      addrline = fin.readline()
      # can be empty if it has already been translated before
      if addrline.strip() != '':
        # add all addresses part of the bbl to a list. Format of lines:
        # 0x00008130:  e59fc024      ldr  ip, [pc, #36]   ; 0x815c
        addr = addrline.split(':')[0]
        addrlist = [addr]
        while True:
          addrline = fin.readline()
          if not addrline:
            stop = True;
            break
          if addrline.strip() == '':
            break
          endaddr = addrline.split(':')[0]
          addrlist = addrlist + [endaddr]
        # print 'Mapping ',addr, ' to ',addrlist
        mapping[addr]=addrlist

      if stop:
        break

    if s.startswith('Trace '):
      # Executed bbl, increase counter for every instruction in this bbl
      # Trace 0x6028cac0 [00008130]
      addr = '0x'+s.split('[')[1].split(']')[0]
      # can fail for kernel-mapped pages
      if mapping.has_key(addr):
        # print 'Increasing exec count for ',addr
        for addrs in mapping[addr]: 
          profile[addrs] += 1

  fout.truncate()
  for addr,count in sorted(profile.items()):
    fout.write( addr+' '+str(count) + os.linesep)
  fout.close()

