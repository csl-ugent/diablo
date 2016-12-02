import os
import struct
import sys

from collections import OrderedDict

# x = read_profile('file_path_and_name')
#
# Result:
#   x[address] = execcount
def read(filename):
    result = OrderedDict()
    fileobject = open(filename, "rb")
    try:
        # data = fileobject.read(16)
        data = fileobject.readline()
        data.strip("\n\r")
        while data != "":
            # process data as two 64-bit numbers
            # extract two little-endian unsigned 64-bit numbers
            # (address, execcount) = struct.unpack("<QQ", data)
            raw = data.split(" ")
            address = int(raw[0], base=16)
            execcount = int(raw[1])

            # store this result
            result[address] = execcount

            # read the next chunk of binary data
            # data = fileobject.read(16)
            data = fileobject.readline()
            data.strip("\n\r")
    finally:
        fileobject.close()

    return result

# write_profile(execution_profile, 'file_path_and_name')
#
# Parameters:
#   execution_profile[address] = execcount
def write(profile, filename):
    fileobject = open(filename, "wb")
    try:
        # iterate over the profile (which is a dictionary)
        for address, execcount in profile:
            #data = struct.pack("<QQ", address, execcount)
            data = "0x%x %i\n" % (address, execcount)
            fileobject.write(data)
    finally:
        fileobject.close()

def extend(profile1, profile2):
    result = profile1

    for orig_address, execcount in profile2.items():
        if not result.has_key(orig_address) or (result[orig_address] == 0):
            result[orig_address] = execcount

    return sorted(result.items())

def merge(profile1, profile2):
    result = profile1

    for orig_address, execcount in profile2.items():
        if not result.has_key(orig_address):
            result[orig_address] = execcount
        else:
            result[orig_address] = result[orig_address] + execcount

    return sorted(result.items())

# x = translate_profile(from_profile_data, from_map_data, from_file_functions, to_map_data, to_file_functions)
#
# Parameters:
#   from_*: input profile and map file data
#   to_*: output map file date
#
# Result:
#   x[address] = execcount
def translate_map(from_profile_data, from_map_data, from_file_functions, to_map_data, to_file_functions):
    result = {}

    orig_map_items = from_map_data.items()
    orig_map_index = 0

    new_map_items = to_map_data.items()
    new_map_index = -1

    orig_base_address = orig_map_items[orig_map_index][0]
    new_base_address = -1

    orig_function = ""
    new_function = ""

    for orig_address, execcount in from_profile_data.items():
        look_for_next_function = False

        # check whether the current address is the start of another function
        while orig_map_index+1 < len(orig_map_items) and orig_map_items[orig_map_index + 1][0] <= orig_address:
            orig_map_index += 1
            orig_base_address = orig_map_items[orig_map_index][0]
            #print "new function",orig_map_items[orig_map_index]
            orig_function = orig_map_items[orig_map_index]
            look_for_next_function = True

        if orig_map_index+1 >= len(orig_map_items):
            print "end of map file:",orig_function
            break

        # if no matching address was determined yet
        #     or if a new function has begun,
        # look for the corresponding new address, given the original address
        if new_base_address == -1 or look_for_next_function:
            # orig_map_items[orig_map_index][0] = address
            # orig_map_items[orig_map_index][1] = [object file, function name]
            #print "looking for original function",orig_function,"in object file",orig_map_items[orig_map_index][1][0]

            # look for this function in the new map file
            new_map_index = -1

            # first of all, look through the file list for a suitable file
            # 1. try to find an exact match
            to_objectfilename = ''
            for objectfilename, functionlist in to_file_functions.items():
                if objectfilename == orig_map_items[orig_map_index][1][0]:
                    to_objectfilename = objectfilename
                    break

            # if that didn't work, try to do kind of a pattern match on the filename
            # This can be needed because the second version of the original binary can be generated from another directory,
            # in which case the paths in the map files don't match.
            if to_objectfilename == '':
                # we only want to use the basename of the file path
                from_basename = os.path.basename(orig_map_items[orig_map_index][1][0])

                for objectfilename, functionlist in to_file_functions.items():
                    to_basename = os.path.basename(objectfilename)
                    if from_basename == to_basename:
                        # possible candidate match; need to verify whether list of functions matches identically
                        if set(functionlist) == set(from_file_functions[orig_map_items[orig_map_index][1][0]]):
                            # found an exact match
                            to_objectfilename = objectfilename
                            break

            # we have found the matching object file name, look for the function
            for index, data in enumerate(new_map_items):
                if data[1][0] == to_objectfilename:
                    # calculate the intersection of the function names associated with this object file
                    intersect = set(data[1][1]).intersection(set(orig_map_items[orig_map_index][1][1]))
                    if intersect != set():
                        #print "found match",hex(data[0]),data[1],intersect
                        new_map_index = index
                        new_base_address = data[0]
                        new_function = data[1]
                        break

            if new_map_index == -1:
                print 'could not find address',hex(orig_address),'in new map data',execcount,'for function original',orig_function
                #sys.exit(2)

        if new_map_index != -1:
            #print new_base_address,hex(new_base_address),orig_address,hex(orig_address),orig_base_address,hex(orig_base_address)
            new_address = new_base_address + (orig_address - orig_base_address)
            #print "old",hex(orig_address),"new",hex(new_address),":",execcount
            result[new_base_address + (orig_address - orig_base_address)] = execcount

    return sorted(result.items())

# x = translate_profile_list(execution_profile, instruction_list)
#
# Parameters:
#   execution_profile[address] = execcount
#   instruction_list[new_address] = old_address
#
# Result:
#   x[old_address] = execcount
def translate_list(profile, inlist):
    result = OrderedDict()

    for new_address, execcount in profile.items():
        # old address is 0; this was not in the original binary
        if inlist[new_address] == 0:
            continue

        print "translating",hex(new_address),execcount
        print "  to",hex(inlist[new_address])
        result[inlist[new_address]] = execcount

    return result
