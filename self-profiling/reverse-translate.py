#!/usr/bin/python

import getopt
import sys

import listfile
import binutils
import profile

f_map_a = ''
f_profile = ''
f_list = ''
f_map_c = ''
f_out_profile = ''

def parse_options(argv):
    global f_map_a
    global f_profile
    global f_list
    global f_map_c
    global f_out_profile

    try:
        opts, args = getopt.getopt(argv, "l:m:n:p:q:", ["list=", "frommap=", "tomap=", "inprofile=", "outprofile="])
    except getopt.GetoptError:
        print 'ERROR: -l <list file> -m <from map file> -p <from execution profile> -n <to map file> -q <output execution profile>'
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-l", "--list"):
            f_list = arg
        elif opt in ("-m", "--frommap"):
            f_map_a = arg
        elif opt in ("-n", "--tomap"):
            f_map_c = arg
        elif opt in ("-p", "--inprofile"):
            f_profile = arg
        elif opt in ("-q", "--outprofile"):
            f_out_profile = arg

if __name__ == "__main__":
    parse_options(sys.argv[1:])

    # read input files
    b_profile = profile.read(f_profile)

    x_profile = None
    if f_list != '':
        # translate execution profile for B from new to old addresses using list file
        b_list = listfile.read(f_list)
        x_profile = profile.translate_list(b_profile, b_list)
    else:
        # passthrough
        x_profile = b_profile

    z_profile = None
    if f_map_a != '':
        (a_map, a_file2functions) = binutils.read_map(f_map_a)
        (c_map, c_file2functions) = binutils.read_map(f_map_c)

        # translate execution file using map files
        z_profile = profile.translate_map(x_profile, a_map, a_file2functions, c_map, c_file2functions)
    else:
        # passthrough
        z_profile = x_profile

    # write output
    profile.write(z_profile, f_out_profile)
