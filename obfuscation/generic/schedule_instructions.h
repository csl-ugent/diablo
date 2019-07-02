/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef GEN_SCHEDULE_INSTRUCTIONS_H
#define GEN_SCHEDULE_INSTRUCTIONS_H

#include <list>

/*Type and Function definitions{{{*/
enum t_scheduler_dependency
{
  DEP_RAW = 0,
  DEP_WAW,
  DEP_ORDERED,
  DEP_WAR,
  DEP_WEAKLY_ORDERED,
  DEP_RAR
};

struct t_scheduler_dagnode : public t_node
{
  t_ins * ins;
  t_uint32 dependency_count; /* The node of predecessors that are unscheduled */
  t_uint32 flags;
  /* We should add the resources here, in an arch independend manner */
  
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * corr;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * next;
  /*added for counting all possible schedules*/
  t_bool is_sink;
  /*added for counting all possible schedules*/
  t_bool is_source;
};

typedef std::list<t_scheduler_dagnode*> t_scheduler_list;

struct t_scheduler_dag
{
  t_graph graph;
  
  t_bbl * bbl;

  /* Maybe add some specific fields here? */
  t_scheduler_dagnode * source;
  t_scheduler_dagnode * sink;
 
  /*The schedule under construction*/
  t_scheduler_list schedule;

  /*The nodes that are ready for scheduling*/
  t_scheduler_list ready;

  /*added for counting all possible schedules*/
  t_scheduler_dagnode * first;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * last;
  /*added for counting all possible schedules*/
  t_uint32 nr_scheduled;
};

class ScheduleInstructionsTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "scheduleinstructions";

  typedef int (*slice_id_fn)(t_ins *);
  typedef void (*set_slice_id_fn)(t_ins *, int);
  typedef t_uint16 (*get_ins_fingerprint_fn)(t_ins *);

protected:
  virtual bool modifiesStackPointer(t_ins* ins) const = 0;
  /* InsHasSideEffects from the architecture-description also includes whether if is a store, and if it modifies the stack pointer.
   * That is a bit of an overkill here, so we separate those */
  virtual bool hasSideEffects(t_ins* ins) const = 0; /* Side-effects OTHER than isStore and modifiesStackPointer! */
public:
  ScheduleInstructionsTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  void doSliceTransform(t_bbl *bbl, t_scheduler_dag *dag, set_slice_id_fn);
  void doCanonicalizeTransform(t_bbl *bbl, t_scheduler_dag *dag, get_ins_fingerprint_fn);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }
  virtual bool transformationIsAvailableInRandomizedList() { return false; }

  t_scheduler_dag*  SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set, bool reverse = false);
  void SchedulerDagDump(t_scheduler_dag *dag, std::string filename);
};

void SchedulerDagFree(t_scheduler_dag * dag);

extern LogFile* L_OBF_SI;

#endif /* GEN_SCHEDULE_INSTRUCTIONS_H */
