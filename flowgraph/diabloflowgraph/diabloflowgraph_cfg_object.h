/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* CfgObject Functions {{{ */
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLOFLOWGRAPH_CFG_OBJECT_FUNCTIONS
#define DIABLOFLOWGRAPH_CFG_OBJECT_FUNCTIONS
t_section *SectionCreateDeflowgraphingFromChain (t_object * obj, t_bbl * chain_head, t_const_string name);
void SectionInitAssembly (t_object * obj, t_section * section, t_uint32 max_ins);
void SectionFiniAssembly (t_object * obj, t_section * section);
void SectionInitDisassembly (t_section *);
void SectionFiniDisassembly (t_object * obj, t_section * section);
void SectionInitFlowgraph (t_section * section);
void SectionFiniFlowgraph (t_section * section);
void SectionInitDeflowgraph (t_object * obj, t_section * section);
void SectionToDisassembled (t_section * sec);
t_architecture_description *ObjectGetArchitectureDescription (t_object * obj);
void ObjectFlowgraph (t_object * obj, t_const_string const * force_leader, t_const_string const * force_reachable, t_bool preserve_functions_by_symbol);
void SectionFiniDeflowgraph (t_section * section);
void ArchitectureHandlerAdd (t_const_string arch_name, t_architecture_description * description, t_address_type addrsize);
void ArchitectureHandlerRemove (t_const_string arch_name);
t_architecture_description *ArchitectureGetDescription (t_const_string arch);
t_address_type ArchitectureGetAddressSize (t_const_string arch);
void ObjectRewrite (t_const_string name, int (*func) (t_cfg *), t_const_string oname);
void DeflowgraphedModus (t_address * x, t_reloc * rel, t_address * out);
void ObjectPrintListing(t_object *obj, t_string output_name);
#endif
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
