/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "binder.h"

/* Global variables, already initialize them so they won't end up in BSS section. The arrays will be resized by
 * Diablo, therefore we initialize them to a size of 1 and make sure each of them exists in a separate subsection.
 */
uint32_t DIABLO_Mobility_binary[1] __attribute__((section (".data.binary"))) = { 42 };
MobileEntry DIABLO_Mobility_global_mobile_redirection_table[1] __attribute__((section (".data.gmrt"))) = {{ (t_address) sizeof DIABLO_Mobility_global_mobile_redirection_table[0] }};
uint32_t DIABLO_Mobility_GMRT_size = 1;
uint32_t DIABLO_Mobility_version __attribute__((section (".data.version")))= 1;

__attribute__ ((noinline)) static void RealInit ()
{
  /* Set up mutexes for GMRT */
  uint32_t size = DIABLO_Mobility_GMRT_size;/* Slight hack to optimize, cache the value of this global variable as it won't change */
  for(uint32_t iii = 0; iii < size; iii++)
  {
    pthread_mutex_init(&(DIABLO_Mobility_global_mobile_redirection_table[iii].mutex), NULL);
  }
}

__attribute__ ((naked)) void DIABLO_Mobility_Init ()
{
  __asm("push {r0, r1, r2, r3, lr}");
  RealInit();
  __asm("pop {r0, r1, r2, r3, lr}");
  __asm("mov pc, lr");
}

/* This function will insert a mobile block (by downloading it) into the code cache and update the metadata. It is assumed the mutex
 * associated to the block has been taken before invocation.
 */
static void InsertMobileBlock (MobileEntry* entry, uint32_t index, bool code)
{
  /* Download the mobile block */
  size_t len;
  t_address start = DIABLO_Mobility_DownloadByIndex(index, &len);

  if (code)
  {
    /* Write the binary base in the beginning of the block */
    *((uint32_t*)start) = (uint32_t) DIABLO_Mobility_binary;

    /* Change the protection for the mobile block to execute only */
    mprotect(start, len, PROT_EXEC);

    /* Now clear the cache, making sure the entire downloaded block is present in the main memory (as opposed to only in the data cache),
     * so we can load it into the instruction cache. Can't do this when linking in after, as this is not a libc function.
     */
#ifndef LINKIN_AFTER
    __clear_cache(start, start + len);
#endif

    /* Put the address of the stub into the downloaded field. We can use this later on to replace the address field with the address of the stub again */
    entry->downloaded = entry->addr;
    entry->len = len;
    entry->addr = (t_address)((uint32_t)start + 8);/* Only change the GMRT address entry at the end, right before unlocking */
  }
  else
  {
    /* Put the start address into the downloaded field, so that it doesn't contain a zero anymore, signifying it's been downloaded */
    entry->downloaded = start;
    entry->len = len;
    entry->addr = start;/* Only change the GMRT address entry at the end, right before unlocking */
  }
}

/* This function will return a MobileEntry for a mobile block (that will be downloaded if it's not present yet */
MobileEntry* GetMobileBlock(uint32_t index, bool code)
{
  MobileEntry* entry = &DIABLO_Mobility_global_mobile_redirection_table[index];
  pthread_mutex_lock(&(entry->mutex));

  /* Check if another thread already downloaded and inserted this mobile code fragment. If not, we'll do it ourselves, else
   * it has already happened and we can use the information in the table.
   */
  if (entry->downloaded == 0)
    InsertMobileBlock(entry, index, code);

  pthread_mutex_unlock(&(entry->mutex));

  return entry;
}

t_address Resolve (uint32_t index)
{
  MobileEntry* entry = GetMobileBlock(index, true);
  return entry->addr;
}

/* Wrapper function that aligns the stack on 8-byte boundary */
__attribute__ ((naked)) t_address DIABLO_Mobility_Resolve (uint32_t index)
{
  __asm("push {r1, r2, r3, r4, r5, fp, ip, lr}");
  __asm("mrs r4, CPSR");
  __asm("vmrs r5, FPSCR");
  __asm("mov fp, sp");
  __asm("and sp, #-8");
  Resolve(index);
  __asm("mov sp, fp");
  __asm("vmsr FPSCR, r5");
  __asm("msr CPSR, r4");
  __asm("pop {r1, r2, r3, r4, r5, fp, ip, pc}");
}

void binder_softvm(uint32_t index, char** retVmImage, uint32_t* retSizeVmImage)
{
  MobileEntry* entry = GetMobileBlock(index, false);

  /* Fill in return values */
  *retVmImage = entry->addr;
  *retSizeVmImage = entry->len;
}

void EraseAllMobileBlocks()
{
  uint32_t i;

  for (i = 0; i < DIABLO_Mobility_GMRT_size; i++) {
    EraseMobileBlock(i, true); // TODO how can I get if it is code or data?
  }
}

void EraseMobileBlock (uint32_t index, bool code)
{
//  printf("EraseMobileBlock(%x, %d)\n", index, code);

  MobileEntry* entry = &DIABLO_Mobility_global_mobile_redirection_table[index];
  pthread_mutex_lock(&(entry->mutex));

  if (code)
  {
//    printf("entry->addr: %x\n", (unsigned int)entry->addr);
//    printf("entry->downloaded: %x\n", (unsigned int)entry->downloaded);
//    printf("entry->len: %d\n", entry->len);

    if (entry->downloaded != 0) {
//      printf("downloaded != 0 branch\n");
      /* release the previously allocated block */
      //free(entry->addr);

      /* reset the len attribute */
      entry->len = 0;

      /* restore the initial stub */
      entry->addr = entry->downloaded;

      /* reset the 'block downloaded' flag so that next access to the
       * block will trigger a new download process */
      entry->downloaded = 0;
    }
  }
  else
  {
//    printf("downloaded == 0 branch\n");
    //free(entry->addr);

    entry->downloaded = 0;
    entry->len = 0;
    entry->addr = 0;
  }

  pthread_mutex_unlock(&(entry->mutex));
}
