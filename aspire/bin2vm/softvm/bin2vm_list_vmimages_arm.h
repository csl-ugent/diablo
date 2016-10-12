#ifndef BIN2VM_LIST_VMIMAGES_ARM_H
#define BIN2VM_LIST_VMIMAGES_ARM_H

#include <bin2vm/softvm/bin2vm.h>

typedef void * SoftVmCallDataARM;

//Use bin2vm_free_vmimages_arm to free the list.
struct bin2vm_list_vmimages_arm
{
  unsigned char* data;
  unsigned int   size;
  struct bin2vm_list_testcases_vm_image_arm* testcases; /* List of testcases for a single vmimage. */
  struct bin2vm_list_vmimages_arm* next; //For the last item in the list, next is NULL.
};

struct bin2vm_list_testcases_vm_image_arm
{
  SoftVmCallDataARM input;
  SoftVmCallDataARM expectedOutput;
  struct bin2vm_list_testcases_vm_image_arm* next;
};

#endif /* END BIN2VM_LIST_VMIMAGES_ARM_H */
