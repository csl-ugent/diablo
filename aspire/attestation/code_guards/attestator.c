/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "utils.h"

/* Global variables that will be filled in by Diablo */
const ADS* data_structure_blob_##LABEL## = (ADS*) 42;
uintptr_t base_address_##LABEL## = 42;
result_t checksum_##LABEL##[1] __attribute__((section (".data.checksum"))) = { sizeof(checksum_##LABEL##[0]) };/* Will be resized by Diablo */

/* Variables local to the file */
static uint64_t failed = CG_INIT_VALUE;
static uint32_t hashed_nonce_used = CG_INIT_VALUE;
static uint32_t nonce_to_be_used = CG_INIT_VALUE;
static uint32_t last_id;
static result_t result;

static void hash_block(char* base, size_t size)
{
  result_t tmp;

  /* Load up the same numbers of bytes as the result size and include these bytes in the result */
  size_t bytes_read = 0;
  for (; bytes_read + sizeof(tmp) <= size; base += sizeof(tmp), bytes_read += sizeof(tmp))
  {
    memcpy(&tmp, base, sizeof(tmp));
    result ^= tmp;
  }

  /* Do the same for the remainder (which isn't aligned to sizeof(tmp)) */
  if (size - bytes_read)
  {
    tmp = 0;
    memcpy(&tmp, base, size - bytes_read);
    result ^= tmp;
  }
}

static uint32_t hash(uint32_t in)
{
  return in + 0x33333333;
}

static void maintain_failed()
{
}

static void ruin_failed()
{
  exit(1);
}

static void update_nonce_to_be_used()
{
}

void attestator_##LABEL##(uint32_t id)
{
  /* Store information about this attestation */
  hashed_nonce_used = hash(nonce_to_be_used);
  last_id = id;

  /* Get the area and attest it */
  const Area* area = GetAreaById(data_structure_blob_##LABEL##, id);
  result = 0;
  WalkArea(area, base_address_##LABEL##, hash_block, nonce_to_be_used);
}

void verifier_##LABEL##()
{
  static uint32_t last_verified_nonce = CG_INIT_VALUE;
  if (hashed_nonce_used == hash(nonce_to_be_used))
  {
    last_verified_nonce = nonce_to_be_used;
    update_nonce_to_be_used();

    if (checksum_##LABEL##[last_id] != result)
      ruin_failed();
    else
      maintain_failed();
  }
  else if ((hashed_nonce_used == hash(last_verified_nonce)) || (hashed_nonce_used == nonce_to_be_used))
    maintain_failed();
  else
    ruin_failed();
}
