# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

def grep(pattern, f):
  ret = []
  for line in open(f):
	if pattern in line:
	  ret.append(line)
  return ret


def add_to_map_of_list(map, key, value):
  if key in map:
	map[key].append(value)
  else:
	map[key] = [value]

### TODO efficiency!!!
def remove_if_exist(list, elements):
  for element in elements:
	while element in list:
	  list.remove(element)
  return list

def add_or_initialize_to(map, key, to_add):
  if key in map:
	map[key] += to_add
  else:
	map[key] = to_add

def maybe_initialize_map(map, key, init):
  if key not in map:
	map[key] = init

def map_entry_or_default(map, key, default):
  if key in map:
	return map[key]
  else:
	return default
