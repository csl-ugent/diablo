/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLO_CODE_MOBILITY_H
#define DIABLO_CODE_MOBILITY_H

/* Include used C++ headers */
#include <string>
#include <vector>

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloanopt.h>
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
  #include <diabloobject.h>
  #include <diablosupport.h>
}

#include <gmrt_transformer.h>

/* Include other headers from this folder */
#include "code_mobility_cmdline.h"
#include "code_mobility_json.h"

class CodeMobilityTransformer : public GMRTTransformer
{
  private:
    t_symbol* resolve_sym;/* The symbol of the resolve routine */
    static const t_uint32 binder_version = 1;

    /*** FUNCTIONS ***/
    /* Helper functions */
    t_bbl* CreateGMRTStub (t_bbl* entry_bbl);
    void PrepareCfg (t_cfg* cfg);

  public:
    void AddForceReachables (std::vector<std::string>& reachable_vector);
    void FinalizeTransform ();
    void ReserveEntries(t_uint32 nr);
    void TransformObject ();

    /* Constructor and destructor */
    CodeMobilityTransformer (t_object* obj, t_const_string output_name);
    ~CodeMobilityTransformer ();

    /* Function that can be called by other protections to find out whether the a certain object was created by code mobility */
    static bool IsMobileObject(t_object* obj);
};


#endif /* DIABLO_CODE_MOBILITY_H */
