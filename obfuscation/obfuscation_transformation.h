/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef OBFUSCATION_TRANSFORMATION_H
#define OBFUSCATION_TRANSFORMATION_H

#include <vector>
#include <string>
#include <diabloannotations.h>


extern "C" {
#include <diablosupport.h>
#include <diabloflowgraph.h>
}

extern LogFile * L_OBF_OP;
extern LogFile * L_OBF_FLAT;
extern LogFile * L_OBF_BF;

struct BBLObfuscationTransformation : public BBLTransformation {
  BBLObfuscationTransformation();
  virtual t_const_string name() const { return "bbl_obfuscation"; }
};

/* TODO: Actually, we have region-based transformations that can transform functions, because they are a region, and function-specific transformations, that,
 * for example, split off the function entry block. Currently these two are merged, but they should be split.
 * However, sometimes you want a region to a part of a function, for example, I'm not sure how well the current flattening implementation would work with multiple
 * functions's BBLs being merged into a single switch block. */
struct FunctionObfuscationTransformation : public FunctionTransformation {
  FunctionObfuscationTransformation();
  virtual t_const_string name() const { return "function_obfuscation"; }
  
  /* Also possibly transform a region that does not necessarily correspond to a function. See the above TODO */
  virtual bool canTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls) { return false; }
  virtual void doTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls, t_randomnumbergenerator * rng) {}
};

typedef std::pair<t_bbl*, AnnotationIntOptions> AnnotatedBbl;
typedef std::vector<std::vector<AnnotatedBbl>> AnnotatedBblPartition;

int GetAnnotationValueWithDefault(const AnnotatedBbl& abbl, const std::string& key, int d);

struct CompatibilityPartitioner {
  virtual bool compare(const AnnotatedBbl&, const AnnotatedBbl&) const = 0;
};
AnnotatedBblPartition PartitionAnnotatedBbls(const std::vector<AnnotatedBbl>& bbls, const CompatibilityPartitioner& p_compatible);
AnnotatedBblPartition SelectBblsForAllRegionsFor(t_cfg* cfg, /*const TODO */ std::string& obfuscation_string, const CompatibilityPartitioner& p_compatible, t_randomnumbergenerator *rng_obfuscation);

/* If this is disabled (false), you have to manually specify on Diablo's command line which transformations can be applied. In some contexts (for example, obfuscation/diversity scripts),
 * this is somewhat annoying, hence the global enable option (true). The default is false */
bool AllObfuscationsEnabled();
void SetAllObfuscationsEnabled(bool enable);

/* For now, some transformations are only enabled for diversity on i486 */
void SetUseDiversityTransforms(bool enable);
bool DiversityTransformsEnabled();

/* Functions that should not be allowed to be transformed, nor calls to them (for example, because they use the LR, or PC) */
bool FunctionIsBranchFunction(const t_function* fun);
bool DisallowedFunctionToTransform(const t_function* fun);

/* Some generic functionality for registers */
void AddRegisterToLiveOut(t_bbl* bbl, t_reg reg);

void CfgObfuscateRegions(t_cfg* cfg);
void ObjectObfuscate(t_const_string filename, t_object* obj);

#endif /* OBFUSCATION_TRANSFORMATION_H */
