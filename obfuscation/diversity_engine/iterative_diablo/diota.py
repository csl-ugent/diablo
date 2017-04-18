#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import getpass # for getuser
import os
import shutil     as     shutil
import stat
from   subprocess import call
import sys

from   settings import settings

def diota(exe, analysis, pwd, xml=None, stdoutfile=None, args=None, diota_home=settings.regular_diota_dir):
  env = {}
  env["LD_BIND_NOW"]       = "yes"
  env["DIOTA_NR_BACKENDS"] = "1"

  env["LD_LIBRARY_PATH"]   = diota_home + "/SO:" + diota_home +  "/SO/../client/lib:" + diota_home + "/SO/../static/lib/"
  env["LD_PRELOAD"]        = diota_home + "/SO/" + analysis + ".so"
  env["DIOTAPARAM"]        = "log_dir=" + pwd
  env["PATH"]              = diota_home + "/bin/:."

  os.chdir(pwd)
  
  if stdoutfile is not None:
	stdout = open(stdoutfile, "w")
  else:
	stdout = None
  
  call(["diota", "./%s" % exe] + args, cwd = pwd, stdout = stdout, env = env)
  
  if stdoutfile is not None:
	stdout.close()

  #xmlfile = "%s.%s.xml" % (exe, analysis)
  shutil.move("diota.log-._%s-%s" % (exe, getpass.getuser()), xml)
  
  call(["sed", "-i", "-e", "/^=.*/d", xml]) 
  
  #call(["vim", "-s", VIMDIR + "/clear.vim", xmlfile]) ### TODO CLEAR MANUALLY
  #shutil.move("inp.out", "%s_%s.diota_out" % (exe, analysis))
