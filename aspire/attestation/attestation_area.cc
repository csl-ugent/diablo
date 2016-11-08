/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "attestation_area.h"

using namespace Attestation;
using namespace std;

Block::Block(t_address begin_offset)
: offset(begin_offset)
{}

void Block::calcSize (t_address end_offset)
{
  this->size = AddressSub(end_offset, offset);
}

void Area::InitializeBlock(t_address begin_offset)
{
  blocks.push_back(Block(begin_offset));
}

void Area::FinalizeBlock(t_address end_offset)
{
  blocks.back().calcSize(end_offset);
}

void Area::CalculateChecksum(void* base, AttestBlockFun* fun, uint32_t nonce) const
{
  for(const auto& block : blocks)
  {
    uint8_t* block_base = static_cast<uint8_t*>(AddressAddDispl(base, block.offset));
    fun(block_base, block.size);
  }
}

void Area::PrintMetric(FILE* fp) const
{
  const t_uint32 nr_of_blocks = blocks.size();
  uint32_t nr_of_guarded_bytes = 0;
  for(const auto& block : blocks)
  {
    nr_of_guarded_bytes += AddressExtractUint32(block.size);
  }

  fprintf(fp, "%u,%u,%u,%u\n", region_idx, nr_of_blocks * (uint32_t)(sizeof(t_uint64) + sizeof(t_uint32)), nr_of_blocks, nr_of_guarded_bytes);
}

void Area::WriteAreaToADS(t_section* ads, t_uint16 id) const
{
  const t_uint32 nr_of_blocks = blocks.size();

  /* Calculate the space needed in the ADS section */
  const t_uint32 needed_space = sizeof(t_uint16) + sizeof(t_uint32) + nr_of_blocks * (sizeof(t_uint64) + sizeof(t_uint32));
  const t_address old_size = SECTION_CSIZE(ads);
  SECTION_SET_CSIZE(ads, AddressAddUint32(old_size, needed_space));
  SECTION_SET_DATA(ads, Realloc(SECTION_DATA(ads), AddressExtractUint64(SECTION_CSIZE(ads))));

  /* Add the area information to the ADS section */
  SectionSetData16(ads, old_size, id);
  SectionSetData32(ads, AddressAddUint32(old_size, sizeof(t_uint16)), nr_of_blocks);

  t_address offset = AddressAddUint32(old_size, sizeof(t_uint16) + sizeof(t_uint32));
  for(const auto& block : blocks)
  {
    SectionSetData64(ads, offset, AddressExtractUint64(block.offset));
    SectionSetData32(ads, AddressAddUint32(offset, sizeof(t_uint64)), AddressExtractUint32(block.size));
    offset = AddressAddUint32(offset, sizeof(t_uint64) + sizeof(t_uint32));
  }
}
