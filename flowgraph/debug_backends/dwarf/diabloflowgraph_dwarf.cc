/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloflowgraph_dwarf.h"
#include "diabloobject_dwarf.h"

#include "diabloflowgraph_dwarf_cfg.h"

static DwarfSections* dwarf_sections = NULL;
static t_object* dwarf_object;
static DwarfInfo dwarf_info;
static bool is_lineinfo = false;
static t_cfg* dwarf_cfg;

static void InitializeDwarf(t_object* obj)
{
  dwarf_sections = new DwarfSections();
  dwarf_object = obj;

  dwarf_info = ObjectGetDwarfInfo(obj, dwarf_sections);
}

static void AssociateDwarfWithCfg(t_object* obj)
{
	ASSERT(dwarf_sections, ("dwarf sections not yet initialised"));
  t_cfg* cfg = OBJECT_CFG(obj);

  InsInitLineinfo(cfg);
  CfgAssociateLineInfoWithIns(cfg, dwarf_sections, dwarf_info);

  is_lineinfo = true;
  dwarf_cfg = cfg;
}

static void FinalizeDwarf()
{
  /* We only need to free memory when there is any in use. If there are multiple deflowgraphs for example
   * it should only be freed once.
   */
  if(dwarf_sections)
  {
    DwarfSectionsFreeAll(&dwarf_info);
    ObjectFreeDebugSections(dwarf_object);

    CfgFileNameCacheFree();

    delete dwarf_sections;
    dwarf_sections = NULL;
  }

  if (is_lineinfo)
    InsFiniLineinfo(dwarf_cfg);
}

void DwarfFlowgraphInit()
{
  /* Some DWARF-related broker calls */
  DiabloBrokerCallInstall("ObjectFlowgraphBefore", "t_object *", (void*)InitializeDwarf, FALSE);
  DiabloBrokerCallInstall("ObjectFlowgraphAfter", "t_object *", (void*)AssociateDwarfWithCfg, FALSE);
  DiabloBrokerCallInstall("AfterDeflowgraph", "", (void*)FinalizeDwarf, FALSE);
}
