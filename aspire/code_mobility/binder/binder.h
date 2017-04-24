/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef BINDER_H
#define BINDER_H

/* C-standard headers */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* Linux-specific headers */
#include <pthread.h>
#include <sys/mman.h>

typedef void* t_address;

extern t_address DIABLO_Mobility_DownloadByIndex(uint32_t index, size_t *len);

typedef struct
{
  t_address addr;
  t_address downloaded;
  pthread_mutex_t mutex;
  size_t len;
} MobileEntry;

/* We declare these variables extern as they have to be filled in by Diablo, but
 * set their visibility to hidden so they won't end up in the GOT. This is more efficient.
 * For information about this pragma, see https://gcc.gnu.org/wiki/Visibility .
 */
#pragma GCC visibility push(hidden)
extern uint32_t DIABLO_Mobility_binary[];
extern MobileEntry DIABLO_Mobility_global_mobile_redirection_table[];
extern uint32_t DIABLO_Mobility_GMRT_size;
#pragma GCC visibility pop

#ifdef LINKIN_AFTER
void DIABLO_Mobility_Init ();
#else
void DIABLO_Mobility_Init () __attribute__((constructor(101)));
#endif

t_address DIABLO_Mobility_Resolve (uint32_t index);
void binder_softvm(uint32_t index, char** retVmImage, uint32_t* retSizeVmImage);

void EraseMobileBlock (uint32_t index, bool code);
void EraseAllMobileBlocks ();

#endif
