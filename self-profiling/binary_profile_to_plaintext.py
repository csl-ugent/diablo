#!/usr/bin/python
import os
import struct
import sys

# Expects one (or two) arguments:
# Argument 1: Path to the profile.
# Argument 2 (optional): The ID of the run to process (default: 0).

def seek_run(f, ID):
    # Keep reading metadata and going to the next entry until we hit the first entry past the one we want, then return the position of the previous one
    currID = -1
    data = f.read(16)
    while data != "":
        (nr_of_bbls, writeID) = struct.unpack('QQ', data)
        # Check if this is the first entry of a new run
        if writeID == 0:
            currID += 1

        # If we hit the run past the one we want, we can quit
        if currID == ID + 1:
            break

        # Get the position of this entry and go to the next
        pos = f.tell() - 16
        f.seek(16 * nr_of_bbls, os.SEEK_CUR)
        data = f.read(16)

    assert currID >= ID, "Didn't find the requested ID in the file."
    return pos

# Print the run situated at this position
def print_run(f, pos):
    f.seek(pos)
    data = f.read(16)
    (nr_of_bbls, writeID) = struct.unpack('QQ', data)
    for _ in range(nr_of_bbls):
        data = f.read(16)
        (address, count) = struct.unpack('QQ', data)
        print "0x%x %i" % (address, count)

def main():
    # Find out which run we have to print out, and find it in the file
    runID = 0 if len(sys.argv) == 2 else int(sys.argv[2])

    # Open the file, seek to the run, and print it
    with open(sys.argv[1]) as f:
        pos = seek_run(f, runID)
        print_run(f, pos)

if __name__ == "__main__": main()
