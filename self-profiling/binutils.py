import re

from collections import OrderedDict

# x, file2func = read_map_file('file_path_and_name')
#
# Result:
#   x[address] = ['object_file_path_and_name', 'function_name_or_dollar']
#   file2func['object_file_path_and_name'] = ['function_name', 'function_name', ...]
def read_map(filename):
    result = OrderedDict()
    file_to_function_map = {}

    #  .text          0x00000000000089d8      0xa60 spec.o
    ptn_textsection = re.compile("^\s*\.(?:(?:text.*)|(?:init)|(?:fini))\s+(0x[0-9a-fA-F]+)\s+0x[0-9a-fA-F]+\s+(\S+)$");
    #                 0x00000000000089d8                ran
    ptn_funinobject = re.compile("^\s+(0x[0-9a-fA-F]+)\s+(\S+)$");
    #                 0x0000000000012ea4                PROVIDE (etext, .)
    ptn_textend     = re.compile("^\s+(0x[0-9a-fA-F]+)\s+PROVIDE\s*\(_*etext,\s*\.\)$");

    fileobject = open(filename, "r")
    try:
        seen_end_of_text = False
        objectfile = ''
        objectfile_address = 0
        added_dummy = False

        line = fileobject.readline()
        while line:
            line = line.rstrip('\n')

            matched = ptn_textsection.match(line)
            if matched:
                # found new object file text section
                address = matched.group(1)
                objectfile = matched.group(2)
                objectfile_address = int(address, 0)
                added_dummy = False

            else:
                matched = ptn_funinobject.match(line)
                if matched and objectfile != '':
                    # found new function definition for current_objectfile
                    address = int(matched.group(1), 0)
                    function = matched.group(2)

                    # store this function in the current object file datastructure
                    if (address > objectfile_address) and not added_dummy:
                        # an additional global entry for the object file should be added first
                        result[objectfile_address] = [objectfile, '$']
                    added_dummy = True

                    if (not result.has_key(address)):
                        result[address] = [objectfile, [function]]
                    else:
                        result[address][1].append(function)

                    if not file_to_function_map.has_key(objectfile):
                        file_to_function_map[objectfile] = []
                    file_to_function_map[objectfile].append(function)
                else:
                    matched = ptn_textend.match(line)
                    if matched:
                        address = matched.group(1)

                        result[int(address, 0)] = ['etext', '']
                        seen_end_of_text = True
                    else:
                        objectfile = ''

            line = fileobject.readline()

        if not seen_end_of_text:
            print 'ERROR while parsing map file',filename,': no PROVIDE(etext) symbol found!'
            sys.exit(2)
    finally:
        fileobject.close()

    # for address, data in result.items():
    #   print hex(address),data
    return (result, file_to_function_map)
