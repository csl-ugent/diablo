/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* C standard headers */
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* All structs that describe the ADS and its parts. These have to be packed as we'll overlay this structs
 * on directly on ADS we find in the binary.
 */
typedef struct __attribute__((__packed__)) Block
{
  uint64_t offset;
  uint32_t size;
} Block;

typedef struct __attribute__((__packed__)) Area
{
  uint16_t id;
  uint32_t nr_of_blocks;
  Block blocks[];
} Area;

typedef struct __attribute__((__packed__)) ADS
{
  uint64_t AID_low;
  uint64_t AID_high;
  uint64_t id;
  uint32_t nr_of_areas;
  Area areas[];
} ADS;

/* Typedefs */
typedef void AttestBlockFun(char* base, size_t size);
typedef uint64_t result_t;

/* General defines */
#define CG_INIT_VALUE 1

/* Functions to be used externally */
const Area* GetAreaById(const ADS* ads, uint64_t id);
void WalkArea(const Area* area, uintptr_t base_address, AttestBlockFun* fun, uint32_t nonce);
