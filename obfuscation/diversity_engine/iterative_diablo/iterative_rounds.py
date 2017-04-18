# This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
# The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

import benchmarks
import feedback_round
from   feedback_round import FeedbackRound, singleRound, untilRound

#mySeed = 7
mySeed = 12

costs = {}
costs["InitialMaxCost"] = 1
costs["MaxWithCost"]    = 1
costs["CostIncrease"]   = 1

### TODO REMOVE
#mySeed = 12 ## 18, 19
#mySeed = 14 ## 20

costs2 = {}
costs2["InitialMaxCost"] = 1
costs2["MaxWithCost"]    = 1
costs2["CostIncrease"]   = 1

iterativeSettings = feedback_round.iterativeSettings(mySeed + 1, mySeed, costs, [ "FlipBranches", "TwoWayPredicates", "FlattenFunctions", "DisassemblyThwarting" ])
iterativeSettings2 = feedback_round.iterativeSettings(mySeed + 1, mySeed + 1, costs2, [ "FlipBranches", "TwoWayPredicates", "FlattenFunctions", "DisassemblyThwarting" ])


rounds_bzip_multiple = [
		   FeedbackRound( (benchmarks.bzip2, 1), singleRound(0), [],    feedback_round.baseSettings),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(1), [0],   feedback_round.baseSettings),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(2), [0],   feedback_round.layoutSettings(mySeed)),
		   FeedbackRound( (benchmarks.bzip2, 2), untilRound(17),  [0],   iterativeSettings),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(18), [0], feedback_round.layoutSettings(mySeed + 1)),
		   FeedbackRound( (benchmarks.bzip2, 2), untilRound(33), [0], iterativeSettings2),
		   FeedbackRound( (benchmarks.bzip2, 2), singleRound(34), [0,17], iterativeSettings2)
		 ]

rounds_mcrypt_multiple = [
		   FeedbackRound( (benchmarks.mcrypt, 1), singleRound(0), [],    feedback_round.baseSettings),
		   FeedbackRound( (benchmarks.mcrypt, 2), singleRound(1), [0],   feedback_round.baseSettings),
		   FeedbackRound( (benchmarks.mcrypt, 2), singleRound(2), [0],   feedback_round.layoutSettings(mySeed)),
		   FeedbackRound( (benchmarks.mcrypt, 2), untilRound(10),  [0],   iterativeSettings),
		   FeedbackRound( (benchmarks.mcrypt, 3), singleRound(11), [0,10], feedback_round.baseSettings),
		   FeedbackRound( (benchmarks.mcrypt, 3), singleRound(12), [0,10], feedback_round.layoutSettings(mySeed + 1)),
		   FeedbackRound( (benchmarks.mcrypt, 3), untilRound(20), [0,10], iterativeSettings2)
		 ]


def rounds_regular(benchmark, until = 2000):
  return [ FeedbackRound( (benchmark, 1), singleRound(0), [],    feedback_round.baseSettings),
		   FeedbackRound( (benchmark, 2), singleRound(1), [0],   feedback_round.baseSettings),
		   FeedbackRound( (benchmark, 2), singleRound(2), [0],   feedback_round.layoutSettings(mySeed)),
		   FeedbackRound( (benchmark, 2), untilRound(until),  [0],   iterativeSettings)
		 ]

regular_bzip2     = rounds_regular(benchmarks.bzip2)

bzip2_arm = rounds_regular(benchmarks.bzip2_arm)

regular_soplex     = rounds_regular(benchmarks.soplex)
regular_openssl_uv = rounds_regular(benchmarks.openssl)
openssl_ssltest_uv = rounds_regular(benchmarks.openssl_ssltest)

### TODO REMOVE
### mySeed = 11
