# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

# These should be changed to your own paths

# Running code on a remote Linux host
python_path    = "/home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/" # Path to this directory on Linux

# Linux Binaries
bindiff_linux = "/opt/zynamics/BinDiff/bin/differ"
ida_linux     = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/software/idalinux/ida-6.6/idal"
# Linux Binaries

# Different diablo binaries, choose one for the settings...
linux_diablo_regular = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/aspire/obfuscation-improvements/test-diablo-regression/build/obfuscation/diablo-diversity-ARM"

# Directories and paths
# WARNING: when on Windows, the final path cannot be long, Windows cannot deal with long file names and path names
experiments_binary_path_base = "/home/bcoppens/demo_iterative/output"

homedrive_data    = "/"
homedrive_scripts = "/home/bcoppens/private/software/diversity/experiments/diversity/iterative_diablo/iterative_diablo/startscripts/"









# You probably should not look beyond this line
onWindows         = False

remoteHost     = "bcoppens@192.168.56.1" # "bcoppens@ecthelion.elis.ugent.be" # remote host (from the VBox) on which we will run code
privateKeyFile = "E:\\home\\bcoppens\\diversity\\ida_vbox_additional_files\\ecthelion.ppk" # Private key file to connect to the remote host


# Windows Binaries
### Beware! Don't put putty on a shared folder on VBox, see andrewbevitt.com/2010/02/04/putty-gethostbyname-unknown-error/
putty      = "C:\\Users\\Administrator\\Desktop\\putty.exe" # putty executable
if onWindows:
  bindiff_exe = "C:\\Program Files (x86)\\zynamics\\BinDiff 4.0\\bin\\BinDiff_Deluxe.exe"
  ida_binary = "C:\\Program Files (x86)\\IDA 6.6\\idaq.exe"
else:
  bindiff_exe = bindiff_linux
  ida_binary  = ida_linux

if onWindows:
  homedrive_data_    = "E:"
  homedrive_scripts_ = "Y:"

# Normalization Diablo, if you don't know what these are for, better not touch them

# diablo binary for normalization
normalization_diablo = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/research/diablo/tmp/test-diota-rebase/build/diota/diablo-diota"

# Force-relayout diablo:
force_relayout_diablo = "/media/1cf273de-b073-441c-9c4e-330c60320e4a/research/diablo/force-layout-to-thwart-matching/build/diversity/diablodiversity/diablo-diversity"

# Diota directory for normalization
normalization_diota_dir = "/afs/elis/group/csl/security/research/diablo-read-diabloed/diota/thomas/context_dyncfg"

# Regular diota dir TODO: also integrate with the above
regular_diota_dir = "/home/bcoppens/private/software/diversity/diota/trunk_orig/trunk/"

# Different linker files, one which keeps the rodata on the place of the original binary, another one moves them to see the effect (TODO: maybe just make different settings files here)
linkerfile_keep_rodata_in_place = "/home/bcoppens/private/software/diversity/experiments/diversity/normalization/diablodiota/test/ELF-i386-BINUTILS_LD.ld" # TODO, see above
linkerfile_move_rodata = "/home/bcoppens/private/software/diversity/experiments/diversity/normalization/diablodiota/test/ELF-i386-BINUTILS_LD--rodata-moved-upward.ld"


#########################################
# These should probably be left unchanged
scriptfile = "putty_scriptfile" # In the local directory
python_scriptfile = "onlinux_scriptfile.py" # In the local directory
#########################################

#########################################
#           CONFIGURATIONS
#########################################



# Class that makes you a nice object with all the right settings, they can be overridden by calling the constructor with
# different *named* arguments
class BaseSettings:
  def __init__(self,
               python_path = python_path,
               remoteHost = remoteHost,
               privateKeyFile = privateKeyFile,
               putty = putty,
               scriptfile = scriptfile,
               python_scriptfile = python_scriptfile,
               normalization_diablo = normalization_diablo,
               normalization_diota_dir = normalization_diota_dir,
               linkerfile_place_correct = linkerfile_keep_rodata_in_place,
               linkerfile_move_rodata = linkerfile_move_rodata,
               linux_diablo = linux_diablo_regular,
               regular_diota_dir = regular_diota_dir,
               experiments_binary_path_base = experiments_binary_path_base,
               onWindows = onWindows,
               ida_binary = ida_binary,
               homedrive_data = homedrive_data,
               homedrive_scripts = homedrive_scripts,
               bindiff_exe = bindiff_exe):
	arguments = locals()
	for argument in arguments:
	  if argument != 'self':
		self.__dict__[argument] = arguments[argument]
  
settings = BaseSettings()
settings_forcelayout = BaseSettings(linux_diablo = force_relayout_diablo)

def set_settings(s):
  global settings
  settings = s
