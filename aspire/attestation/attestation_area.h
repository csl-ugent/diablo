/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef ATTESTATION_AREA_H
#define ATTESTATION_AREA_H

/* Forward declaration */
namespace Attestation
{
  class Area;
}

#include "attestation.h"

namespace Attestation
{
  /* Typedefs */
  typedef void AttestBlockFun(const uint8_t* base, size_t size);

  class Block {
  friend class Area;/* The members and functions can be accessed from Area */

  private:
    /* Members */
    t_address offset;
    t_address size;

    /* Constructor */
    Block(t_address begin_offset);

    /* Calculate the size of the block from the end_offset (we already know the begin) */
    void calcSize(t_address end_offset);
  };

  class Area {
  private:
    /* Members */
    std::vector<Block> blocks;

  public:
    t_tristate alive;
    uint32_t region_idx;

    /* Constructor */
    Area() : alive(NO) {}

    /* Initializing and finalizing a block */
    void InitializeBlock(t_address begin_offset);
    void FinalizeBlock(t_address end_offset);

    /* Calculate the checksum for the Area */
    void CalculateChecksum(void* base, AttestBlockFun* fun, uint32_t nonce) const;

    /* Check whether the area is empty */
    bool empty() const { return (blocks.size() == 0); }

    /* Function to print metric information about Area */
    void PrintMetric(FILE* fp) const;

    /* This function will append the necessary information for the area to the ADS */
    void WriteAreaToADS(t_section* ads, t_uint16 id) const;
  };
}

#endif
