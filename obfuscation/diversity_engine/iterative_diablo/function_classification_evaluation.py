#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import math
import sys

est_file = sys.argv[1]
ref_file = sys.argv[2]

# returns map[ins]->id
def read_mapping(f):
  m = {}
  for line in open(f):
    s = line.split(',')
    ins = int(s[0], base=16)
    id = int(s[1])
    if id != -1:
      m[ins] = id
  return m

# Gets a map[group_id] -> set(ins)
def make_grouping(m):
  cs = {}
  for ins, id in m.iteritems():
    if id in cs:
      cs[id].add(ins)
    else:
      cs[id] = set()
      cs[id].add(ins)
  return cs

def make_grouping_ida(m):
  cs = {}
  for ins, id in m.iteritems():
    if id == 1:
      continue
    if id in cs:
      cs[id].add(ins)
    else:
      cs[id] = set()
      cs[id].add(ins)
  return cs


# Given a cluster (estimated: set(ins)), get its classes (reference, set(ins))
def classes_for_cluster(cluster, ref_map):
  classes = set()
  for ins in cluster:
    # TODO if ins not in ref_map
    if ins in ref_map:
      classes.add(ref_map[ins])
  return classes

# cluster: set(ins), return: purity(float)
def purity_of_cluster(cluster, ref_map):
  classes = classes_for_cluster(cluster, ref_map)

  m = float(0)
  n_c = float(len(cluster))
  
  for c in classes:
    c_count = float(0)
    for i in cluster:
      if i in ref_map and ref_map[i] == c: # TODO: not in ref_map?
        c_count+=1
    m = max(m, c_count/n_c)
  
  return m

def purity(clusters, ref_map):
  maxes = {}
  
  n = float(len(ref_map))
  p = float(0)
  
  for c in clusters:
    n_c = float(len(clusters[c]))
    
    p += purity_of_cluster(clusters[c], ref_map) * n_c / n

  return p

def entropy_of_cluster(cluster, ref_map):
  classes = classes_for_cluster(cluster, ref_map)

  e = float(0)
  n_c = len(cluster)
  
  for c in classes:
    c_count = float(0)
    for i in cluster:
      if i in ref_map and ref_map[i] == c: # TODO: not in ref_map?
        c_count+=1
    #e += c_count / c_
    e = e + c_count/n_c * math.log(c_count/n_c)
  
  return - e

def entropy(clusters, ref_map):
  maxes = {}
  
  n = len(ref_map)
  e = float(0)
  
  for c in clusters:
    n_c = len(clusters[c])
    
    e += entropy_of_cluster(clusters[c], ref_map) * n_c / n

  return e

def FN(ida_clusters, ida_mapping, truth_clusters):
  seen = set()
  fn = float(0)
  tot = float(0)

  for fun in truth_clusters:
    fun_insts = truth_clusters[fun]
    
    fn_fun = 0
    tot_fun = 0

    for inst in fun_insts:
      if inst in seen:
        continue
      
      seen.add(inst)
      
      if inst in ida_mapping:
        id = ida_mapping[inst]
        if id in ida_clusters:
          ida_fun = ida_clusters[id]
        else:
           ida_fun = set()
      else:
        ida_fun = set()
      
      for inst_j in fun_insts:
        if inst_j in seen:
          continue
        tot_fun += 1
        
        if inst_j not in ida_fun:
          fn_fun += 1
    
    fn += float(fn_fun) / float(len(fun_insts))
    tot += float(tot_fun) / float(len(fun_insts))
  
  return (fn, float(fn)/float(tot))
  
def FP(ida_clusters, truth_clusters, truth_mapping):
  seen = set()
  fp = float(0)
  tot = float(0)
  
  #max_fp = 0
  #start_fp = 0

  for fun in ida_clusters:
    fun_insts = ida_clusters[fun]

    #start_fp = fp
    fp_fun = 0
    tot_fun = 0

    for inst in fun_insts:
      if inst in seen:
        continue
      
      seen.add(inst)
      
      if inst in truth_mapping:
        id = truth_mapping[inst]
        if id in truth_clusters:
          truth_fun = truth_clusters[id]
        else:
          truth_fun = set()
      else:
        truth_fun = set()
      
      for inst_j in fun_insts:
        if inst_j in seen:
          continue
        tot_fun += 1
        
        if inst_j not in truth_fun:
          fp_fun += 1
    
    fp += float(fp_fun) / float(len(fun_insts))
    tot += float(tot_fun) / float(len(fun_insts))
    
    #if fp - start_fp > max_fp:
    #  print "New largest cluster @ %s, size %i" % (str(fun_insts), fp - max_fp)
    #  max_fp = fp - start_fp
  
  #print "tot = %i" % tot
  
  return (fp, float(fp)/float(tot))

def metrics(ref_map, est_map, metric):
  #ref = make_grouping(ref_map)
  clusters = make_grouping(est_map)
  
  print "Number of classes: %i" % len(clusters)
  print "Number of instructions: %i" % len(est_map)
  
  p = metric(clusters, ref_map)
  
  print "The evaluation of the mapping: %f" % p


#reference_mapping = read_mapping("E:\\tmp\\reference_mapping_%s" % f)
#estimated_mapping = read_mapping("E:\\tmp\\estimated_mapping_%s" % f)

reference_mapping = read_mapping(ref_file)
estimated_mapping = read_mapping(est_file)

reference_functions = make_grouping(reference_mapping)
estimated_functions = make_grouping_ida(estimated_mapping)

fn = FN(estimated_functions, estimated_mapping, reference_functions)

print "FN,%i,%f" % (fn[0], fn[1])

#fp = FP(estimated_functions, reference_functions, reference_mapping)

#print "FP,%i,%f" % (fp[0], fp[1])

#print "FP,%i,%f,FN,%i,%f" % (fp[0], fp[1], fn[0], fn[1])

#for m in [purity, entropy]:
  #print "BEGIN %s METRICS: " % str(m)
  #print ""
  #print "reference -> estimated"
  #metrics(reference_mapping, estimated_mapping, m)
  #print ""
  #print "estimated -> reference"
  #metrics(estimated_mapping, reference_mapping, m)
  #print ""
  #print "========="



