#include "meta_api.h"

static
t_section *MetaAPI_PltRelForSymbol(t_object *obj, t_string name) {
  t_object *lo = ObjectGetLinkerSubObject(obj);

  t_string pltrel_name = StringConcat2("PLTREL:", name);
  t_section *result = SectionGetFromObjectByName(lo, pltrel_name);
  Free(pltrel_name);

  return result;
}

t_section *CallDynamicSymbol(t_cfg *cfg, t_string name, t_ins *ins, t_bbl *call_site, t_bbl *return_site)
{
  t_bbl *dyncall = CfgGetDynamicCallHell(cfg, name);
  CfgEdgeCreateCall(cfg, call_site, dyncall, return_site, FunctionGetExitBlock(BBL_FUNCTION(dyncall)));

  t_object *obj = CFG_OBJECT(cfg);
  t_object * lo = ObjectGetLinkerSubObject(obj);

  /* this BL instruction needs to jump to the PLTELEM:<symbol name> entry in the PLT */
  t_string pltelem_name = StringConcat2("PLTELEM:", name);
  t_section *sec = SectionGetFromObjectByName(lo, pltelem_name);
  Free(pltelem_name);

  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                  AddressNullForObject(obj),
                                  T_RELOCATABLE(ins), AddressNullForObject(obj),
                                  T_RELOCATABLE(sec), AddressNullForObject(obj),
                                  FALSE, NULL, NULL, NULL,
                                  "R00P-s0008-A00+" "\\" "= s0002 & % s0002 > i00ffffff &=l iff000000 &| R00M?i10000000|ifeffffff& } s0017 < |: }* ! w\\l i00ffffff &-$");

  /* need to keep this PLT entry alive */
  t_section *rel  =  MetaAPI_PltRelForSymbol(obj, name);
  ASSERT(rel, ("Relocation section (.plt.rel) for %s not found", name));
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                  AddressNullForObject(obj),
                                  T_RELOCATABLE(ins), AddressNullForObject(obj),
                                  T_RELOCATABLE(rel), AddressNullForObject(obj),
                                  FALSE, NULL, NULL, NULL,
                                  "R00A00+\\*\\s0000$");

  return rel;
}
