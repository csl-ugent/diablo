import re

from collections import OrderedDict

# x = read_list('file_path_and_name')
#
# Result:
#   x[new_address] = old_address
def read(filename):
    result = OrderedDict()

    ptn_insn = re.compile("^\s*New\s+(0x[0-9a-fA-F]+)\s+Old\s+(0x[0-9a-fA-F]+).*")

    with open(filename) as f:
        for line in f:
            matched = ptn_insn.match(line)
            if matched:
                new_address = matched.group(1)
                old_address = matched.group(2)
                result[int(new_address, 0)] = int(old_address, 0)

    return result
