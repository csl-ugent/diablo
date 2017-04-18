#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import sqlite3    as     sql
import sys
from   xml.etree.ElementTree import ElementTree

def add_cfg_values_to_BinDiff(cfg_values, table, bindiff):
  con = None
  try:
	con = sql.connect(bindiff)
  
	cursor = con.cursor()
	
	# Set up new table
	# TODO! just add extra flag to instruction table?
	# TODO: make this a table with ALL instructions, and then flags for executed, in IDA, etc?
	cursor.execute("DROP TABLE IF EXISTS %s;" % table) # Clean it
	cursor.execute("CREATE TABLE %s (address BIGINT, file INT);" % table) # IF NOT EXISTS 

	# add values
	cursor.executemany("INSERT INTO %s ('address', 'file') VALUES (?, ?)" % table, map(lambda x: (x, 2), cfg_values))

	con.commit()

  except sql.Error, e:
	  print "SQLError %s:" % e.args[0]
	  sys.exit(1)

  finally:
	  if con:
		  con.close()


def get_cfg_values_diota(cfgfile):
  tree = ElementTree()
  tree.parse(cfgfile)
  insns = list(tree.iter("ins"))
  return map(lambda i: int(i.attrib['address'], base=16), insns)

def get_cfg_values_pin(cfgfile):
  insts = []
  for line in open(cfgfile):
	insts.append(int(line, base=16))

  return insts

cfg     = sys.argv[1]
bindiff = sys.argv[2]
what    = sys.argv[3]

if what == "all":
  add_cfg_values_to_BinDiff(get_cfg_values_diota(cfg), "all_info", bindiff) # both map and diota just need the address value of <ins>
else:
  add_cfg_values_to_BinDiff(get_cfg_values_pin(cfg), "executed_info", bindiff)

