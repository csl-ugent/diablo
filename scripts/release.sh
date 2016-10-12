#!/bin/bash
set -o errexit
set -o pipefail
set -o nounset
#set -o xtrace

# Check the number of parameters
if [ "$#" -ne 1 ]; then
  echo "Illegal number of parameters. Expected path to put the tarball."
  exit 1
fi

# Make a temporary directory and check out the repo to it
tmpdir=$(mktemp -d)
svn checkout https://acavus.elis.ugent.be/svn/diablo/private/privatetrunk $tmpdir/diablo
cd $tmpdir/diablo

# Set the revision (CMake can't rely on the VCS metadata to do this, as it won't be present)
revision=$(svnversion)
sed -i -e "s/add_version_info_from_vcs(\(.*\))/set(\1 svn-r$revision)/g" CMakeLists.txt

# Exclude the parts we don't need by deleting them
rm -rf .svn # The SVN folder
rm scripts/release.sh # This script

# Don't include diversity stuff
rm -rf diversity/ obfuscation/diversity_engine/iterative_diablo

# Create the tarball
cd $tmpdir
tar -cJvf $1/diablo.tar.xz diablo/ || true # Always true, so we always get cleanup

# Cleanup
rm -rf $tmpdir
