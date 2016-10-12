/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLO_ABSTRACT_TRANSFORMER_H
#define DIABLO_ABSTRACT_TRANSFORMER_H

/* Include used C++ headers */
#include <string>
#include <vector>

/* Include necessary headers from Diablo core */
extern "C"
{
  #include <diabloarm.h>
  #include <diabloflowgraph.h>
  #include <diabloobject.h>
  #include <diablosupport.h>
}

class AbstractTransformer
{
  protected:
    t_object* obj;/* The object to transform */
    t_cfg* cfg;/* Its CFG */
    const t_uint32 adr_size;/* The address size of the object */
    t_regset possible;/* All general purpose registers that can be used in transformations */
    t_section* binary_base_sec;/* The section that we consider to be the binary base of the object (usually first .text section */

    t_uint32 transform_index;/* The number of transformed functions */
    const t_uint32 transformed_hell_edge_flag;/* Flag to signify this edge was created as a result of a transformation */
    t_const_string transformed_reloc_label;/* The label given to a relocation generated during a transformation */
    FILE* L_TRANSFORMS;/* The handle for the transformation log */

    /* Variables used when splitting off functions into new objects (with new CFGs) */
    t_cfg* new_cfg;
    t_const_string new_obj_name;
    std::vector<t_object*> new_objs;/* A vector of all objects split off */

    /* We can use this table to store all relocations for address producers
     * that might have a 'to' and a 'from' belonging to different objects.
     * These relocations must be killed before the finalization of the program,
     * as they might lead to use after frees during finalization.
     */
    t_reloc_table* address_producer_table;
    t_bool separate_reloc_table;

    /*** FUNCTIONS ***/
    /* These helper functions are used by (and invoke) virtual functions implemented by more derived classes */
  private:
    t_bbl* CreateIndirectionStub (t_bbl* dest);
    void TransformIncomingEdge (t_cfg_edge* edge);
    void TransformIncomingTransformedEdge (t_arm_ins* ins, t_reloc* reloc);
    void TransformOutgoingEdge (t_cfg_edge* edge);
  protected:
    void TransformFunction (t_function* fun, t_bool split_function_from_cfg);
    static void AfterChainsOrdered (t_cfg* split_cfg, t_chain_holder* ch);

    /* Constructor and destructor */
    AbstractTransformer (t_object* obj, t_const_string output_name, t_const_string log_suffix, t_bool srt, t_const_string non, t_const_string trl, t_uint32 edge_flag);
    ~AbstractTransformer ();

    /* Virtual functions that are expected to be implemented by a derived class, but do have a minimal default implementation */
    virtual t_bool CanTransformFunction (t_function* fun) const { return TRUE; }
    virtual void TransformBbl (t_bbl* bbl) {}
    virtual void TransformEntrypoint (t_function* fun) {}
    virtual void TransformExit (t_cfg_edge* edge) {}
    virtual void TransformIncomingEdgeImpl (t_bbl* bbl, t_cfg_edge* edge) {}
    virtual void TransformIncomingTransformedEdgeImpl (t_arm_ins* ins, t_reloc* reloc) {}
    virtual void TransformOutgoingEdgeImpl (t_bbl* bbl, t_cfg_edge* edge, t_relocatable* to) {}

  public:
    /* The functions through which we actually interface with the class */
    virtual void AddForceReachables (std::vector<std::string>& reachable_vector) {};
    virtual void FinalizeTransform () {};/* This function is to be called after deflowgraphing but before assembling of the main object */
    virtual void TransformObject () = 0;
};

#endif
