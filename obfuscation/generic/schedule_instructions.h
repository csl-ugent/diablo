/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef GEN_SCHEDULE_INSTRUCTIONS_H
#define GEN_SCHEDULE_INSTRUCTIONS_H

#include <obfuscation/obfuscation_transformation.h>

#include <map>
#include <set>
#include <vector>

struct t_scheduler_dag;

class ScheduleInstructionsTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "scheduleinstructions";
  t_scheduler_dag*  SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set);
protected:
  virtual bool modifiesStackPointer(t_ins* ins) const = 0;
  /* InsHasSideEffects from the architecture-description also includes whether if is a store, and if it modifies the stack pointer.
   * That is a bit of an overkill here, so we separate those */
  virtual bool hasSideEffects(t_ins* ins) const = 0; /* Side-effects OTHER than isStore and modifiesStackPointer! */
public:
  ScheduleInstructionsTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }
  virtual bool transformationIsAvailableInRandomizedList() { return false; }
};

extern FILE* L_OBF_SI;

#endif /* GEN_SCHEDULE_INSTRUCTIONS_H */
