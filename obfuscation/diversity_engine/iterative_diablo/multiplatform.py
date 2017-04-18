# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import os
import shutil
# TODO: see if we run on windows automatically

from   settings import settings

if settings.onWindows:
  pathseparator = "\\"
else:
  pathseparator = "/"

def asLocalSubdir(path):
  if settings.onWindows:
	return path.replace('/', '\\')
  return path

def asWindows(path, data=True):
  if data:
	return settings.homedrive_data + path.replace('/', '\\')
  else:
	return settings.homedrive_scripts + path.replace('/', '\\')

def asLocal(path):
  if settings.onWindows:
	return asWindows(path)
  else:
	return path

def dir_entry(path, entry):
  return path + pathseparator + entry


def local_dir_entry(path, entry):
  return asLocal(path) + pathseparator + entry



def copy_single_dir(src, dst):
  for f in os.listdir(src):
	shutil.copy(dir_entry(src, f), dir_entry(dst, f))

def copy_nondir_entries(src, dst):
  for f in os.listdir(src): # no asLocal?
	ff = dir_entry(src, f)
	if not os.path.isdir(ff):
	  shutil.copy(ff, dir_entry(dst, f))

def copy_dir_list(src, dst, subdirs):
  for d in subdirs:
	try:
	  os.mkdir(dir_entry(dst, d))
	except OSError, e:
	  print "Failed to mkdir: " + dir_entry(dst, d)
	  print e
  
  for s in subdirs:
	copy_nondir_entries(dir_entry(src, s), dir_entry(dst, s))
