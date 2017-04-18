#!/usr/bin/python

# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import benchmarks
import feedback_driven
import iterative_rounds

iterative_arm_demo = benchmarks.Benchmark(
  "fun", # Binary name
  "/home/bcoppens/demo_iterative/%s/", # Base path
  { 1: "unpatched", 2: "patched"}, # -> version 1 of the binary is located in /home/bcoppens/demo_iterative/unpatched/; version 2 in /home/bcoppens/demo_iterative/patched/ (this is based on the base path above)
  isThumb2=False)

# Select how the different iterations behave. Regular behavior:
# * Iteration 0: original binary (diablo without transformations)
# * Iteration 1: patched binary (diablo without transformations)
# * Iteration 2: patched binary + layout randomization
# * Iteration n > 2: iterative diversity, based on information from iteration 2
rounds = iterative_rounds.rounds_regular(iterative_arm_demo)

# Stop after 20 iterations, start with random seed 10
feedback_driven.iterative_diversity(rounds, extra="first_iterative_test", stop=21, seed=10, rules_file="/home/bcoppens/demo_iterative/transformation_rules")
