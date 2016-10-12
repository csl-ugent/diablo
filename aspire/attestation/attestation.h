/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef ATTESTATION_H
#define ATTESTATION_H

/* Include used C++ headers */
#include <cstdio>
#include <set>
#include <string>

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
  #include <diabloobject.h>
  #include <diablosupport.h>
}
#include <diabloannotations.h>

#ifdef ATTESTATION_INTERNAL
#include "attestation_area.h"
struct Attestator
{
  private:
    t_symbol* checksum_sym;
    t_address checksum_size;

  public:
    std::vector<Attestation::Area> areas;/* The areas protected by this attestator */
    std::vector<std::string> area_names;/* The names of the areas (which are a modified concatenation of the names of the regions in the area */

    /* We keep a map of all attestators, with their label as key */
    typedef std::map<std::string, Attestator> OrderedMap;
    static OrderedMap attestators;

    /* This are the functions we'll use to create or get instances of this class */
    static Attestator* Create(std::string& label);
    static Attestator* Create(std::string& label, std::vector<std::string>& regions, t_uint32* area_id);

    /* Create and associate the regions with their attestators. This only happens for Code Guard regions, because these annotations only
     * contain a region label and we have to find out their attestators by using the attestator annotations.
     */
    static void AssociateRegionsWithAttestators(t_cfg* cfg);
    static void CalculateChecksums(t_object* obj);
    void ReserveChecksumSpace(t_object* obj, t_const_string name);
};
#endif

#include "attestation_json.h"

void AttestationInit(t_object* obj, t_const_string AID_string);

#endif
