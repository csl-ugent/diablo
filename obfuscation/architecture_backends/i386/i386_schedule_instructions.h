/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef I386_SCHEDULE_INSTRUCTIONS_H
#define I386_SCHEDULE_INSTRUCTIONS_H

#include <obfuscation/generic/schedule_instructions.h>

class I386ScheduleInstructionsTransformation : public ScheduleInstructionsTransformation {
protected:
  bool modifiesStackPointer(t_ins* ins) const;
  bool hasSideEffects(t_ins* ins) const;
};

#endif /* I386_SCHEDULE_INSTRUCTIONS_H */
