# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import logging
import subprocess

import multiplatform
from   settings import settings

# TODO: quoting, etc
# TODO: on VSC
# TODO: if we're already on linux....
def call(exe, args=[], pwd=None, outfile=None, append=False, logerror=True, env=None):
  if not settings.onWindows:
    o = subprocess.check_output([exe] + args, env = env, cwd=pwd)
    if outfile is not None:
      # TODO logerror
      if append:
        f = open(outfile, "a")
      else:
        f = open(outfile, "w")
      f.write(o)
    return
  linuxscript = open(settings.scriptfile, "w")
  print "A"
  if pwd is not None:
	linuxscript.write("cd %s\n" % pwd)

  if env is not None:
	for e in env:
	  linuxscript.write("export %s=%s\n" % (e, env[e]))
	linuxscript.write("\n")

  cmd = exe + " " + " ".join(args)
  
  if outfile is not None:
	if append:
	  cmd += " >> " + outfile
	else:
	  cmd += " > " + outfile

	if logerror:
	  cmd += " 2> %s.err" % outfile

  linuxscript.write(cmd)
  linuxscript.close()
  print "B"
  logging.debug("Running on linux '%s'" % cmd)
  print "C"
  subprocess.call([settings.putty, settings.remoteHost, "-i", settings.privateKeyFile, "-m", settings.scriptfile])
  print "D"

def python(commands, imports, pwd=None, outfile=None, append=False, logerror=True):
  scriptfile  = "\n".join(map (lambda i: "import %s" % i, imports)) + "\n"
  scriptfile += "\n".join(commands)

  if pwd is not None:
	pythonscript_f = "%s/%s" % (pwd, settings.python_scriptfile)
  else:
	pythonscript_f = settings.python_scriptfile
  pythonscript = open(multiplatform.asLocal(pythonscript_f), "w")
  pythonscript.write(scriptfile)
  pythonscript.close()
  
  
  call("python", [pythonscript_f], pwd, outfile, append, logerror, env = { "PYTHONPATH": settings.python_path })
  
