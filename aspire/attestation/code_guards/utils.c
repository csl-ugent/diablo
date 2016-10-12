/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "utils.h"

static inline Area* GetNextArea(const Area* current)
{
  return (Area*)((void*)current + sizeof(Area) + sizeof(Block) * current->nr_of_blocks);
}

const Area* GetAreaById(const ADS* ads, uint64_t id)
{
  const Area* area = ads->areas;
  for(uint32_t iii = 0; iii < ads->nr_of_areas; iii++)
  {
    if (area->id == id)
      return area;

    area = GetNextArea(area);
  }

  return NULL;
}

void WalkArea(const Area* area, uintptr_t base_address, AttestBlockFun* fun, uint32_t nonce)
{
  for (uint32_t iii = 0; iii < area->nr_of_blocks; iii++)
  {
    Block blk = area->blocks[iii];
    fun((char*)(base_address + (uintptr_t)blk.offset), blk.size);
  }
}
