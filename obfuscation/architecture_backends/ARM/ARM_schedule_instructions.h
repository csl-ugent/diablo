/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef ARM_SCHEDULE_INSTRUCTIONS_H
#define ARM_SCHEDULE_INSTRUCTIONS_H

#include <obfuscation/generic/schedule_instructions.h>

class ARMScheduleInstructionsTransformation : public ScheduleInstructionsTransformation {
protected:
  bool modifiesStackPointer(t_ins* ins) const;
  bool hasSideEffects(t_ins* ins) const;
};

void ShouldKeepInsCombination(t_ins *last_ins, t_ins *prev_ins, bool *b);

#endif /* ARM_SCHEDULE_INSTRUCTIONS_H */
