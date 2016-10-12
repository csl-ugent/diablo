/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLO_GMRT_TRANSFORMER_H
#define DIABLO_GMRT_TRANSFORMER_H

/* Include used C++ headers */
#include <initializer_list>
#include <set>

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
  #include <diabloobject.h>
  #include <diablosupport.h>
}

#include "abstract_transformer.h"

#define MOBILITY_PREFIX "Mobility_"
#define GMRT_IDENTIFIER_PREFIX LINKIN_IDENTIFIER_PREFIX MOBILITY_PREFIX
#define PREFIX_FOR_LINKED_IN_GMRT_OBJECT "LINKED_IN_GMRT_OBJECT_"
#define FINAL_PREFIX_FOR_LINKED_IN_GMRT_OBJECT PREFIX_FOR_LINKED_IN_GMRT_OBJECT GMRT_IDENTIFIER_PREFIX

struct sectioncmp {
  bool operator() (const t_section* lhs, const t_section* rhs) const
  {return SECTION_CADDRESS(lhs) < SECTION_CADDRESS(rhs);}
};
typedef std::set<t_section*, sectioncmp> OrderedSectionSet;

/* Transforms functions to only be accessed indirectly through a Global Mobile Redirection Table */
class GMRTTransformer : public AbstractTransformer
{
  private:
    static t_const_string transform_label;

  protected:
    t_symbol* init_sym;/* The symbol of the init routine */
    t_symbol* gmrt_size_sym;/* The symbol for the variable holding the size of the GMRT */
    t_section* gmrt_sec;/* The GMRT subsection */
    t_uint32 gmrt_entry_size;/* The size of a GMRT entry */
    t_bbl* binary_base_bbl;/* The binary base BBL of the function being transformed */
    OrderedSectionSet mobile_sections;/* The data sections to be made mobile */

    /*** FUNCTIONS ***/
    /* Helper functions */
  private:
    static void RelocIsRelative (t_reloc* rel, t_bool* is_relative);
    void TransformAddressProducer (t_arm_ins* ins);
  protected:
    void SelectMobileDataForFunction(const t_function* fun);

    /* Implement the virtual functions */
  protected:
    t_bool CanTransformFunction (t_function* fun) const;
    void TransformBbl (t_bbl* bbl);
    void TransformEntrypoint (t_function* fun);
    void TransformIncomingEdgeImpl (t_bbl* bbl, t_cfg_edge* edge);
    void TransformIncomingTransformedEdgeImpl (t_arm_ins* ins, t_reloc* reloc);
    void TransformOutgoingEdgeImpl (t_bbl* bbl, t_cfg_edge* edge, t_relocatable* to);

    /* Constructor */
    GMRTTransformer (t_object* obj, std::initializer_list<t_const_string> objs_needed, t_string lib_needed, t_const_string output_name, t_const_string log_suffix, t_bool srt, t_const_string non, t_const_string trl, t_uint32 edge_flag);
};

#endif
