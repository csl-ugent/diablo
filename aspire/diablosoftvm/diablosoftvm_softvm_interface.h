/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_SOFTVM_INTERFACE_H
#define DIABLOSOFTVM_SOFTVM_INTERFACE_H

#include "bin2vm/softvm/bin2vm.h"
#include "bin2vm/softvm/bin2vm_status_codes.h"
#include "bin2vm/softvm/bin2vm_list_vmimages_arm.h"

typedef enum bin2vm_status_codes bin2vm_status_code_t;

typedef bin2vm_status_code_t (*bin2vm_create_iselect_t)( Bin2Vm** retSelf );
typedef bin2vm_status_code_t (*bin2vm_check_arm_instruction_t)( Bin2Vm* self, unsigned char* armInstruction, unsigned int sizeArmInstruction, bool* retIsSupported );
typedef bin2vm_status_code_t (*bin2vm_create_t)( Bin2Vm** pSelf );
typedef bin2vm_status_code_t (*bin2vm_destroy_t)( Bin2Vm** pSelf );
typedef bin2vm_status_code_t (*bin2vm_diablo_phase2_t)(Bin2Vm* self, const char* jsonBuffer, unsigned int sizeJsonBuffer, const char *file_name, const char *dbgFilenameBody, struct bin2vm_list_vmimages_arm** retVmImages);
typedef bin2vm_status_code_t (*bin2vm_free_vmimages_arm_t)( struct bin2vm_list_vmimages_arm** pList);

bin2vm_create_iselect_t Bin2VmCreateInstructionSelector = NULL;
bin2vm_check_arm_instruction_t Bin2VmCheckArmInstruction = NULL;
bin2vm_create_t Bin2VmCreate = NULL;
bin2vm_destroy_t Bin2VmDestroy = NULL;
bin2vm_diablo_phase2_t Bin2VmDiabloPhase2 = NULL;
bin2vm_free_vmimages_arm_t Bin2VmFreeVmImagesArm = NULL;

void LoadIslSymbols(void *isl_handle) {
  Bin2VmCreateInstructionSelector = (bin2vm_create_iselect_t)dlsym(isl_handle, "bin2vm_create_iselect");
  ASSERT(Bin2VmCreateInstructionSelector, ("Unable to load symbol bin2vm_create_iselect: %s", dlerror()));

  Bin2VmCreate = (bin2vm_create_t)dlsym(isl_handle, "bin2vm_create");
  ASSERT(Bin2VmCreate, ("Unable to load symbol bin2vm_create: %s", dlerror()));

  Bin2VmDestroy = (bin2vm_destroy_t)dlsym(isl_handle, "bin2vm_destroy");
  ASSERT(Bin2VmDestroy, ("Unable to load symbol bin2vm_destroy: %s", dlerror()));

  Bin2VmCheckArmInstruction = (bin2vm_check_arm_instruction_t)dlsym(isl_handle, "bin2vm_check_arm_instruction");
  ASSERT(Bin2VmCheckArmInstruction, ("Unable to load symbol bin2vm_check_arm_instruction: %s", dlerror()));

  Bin2VmDiabloPhase2 = (bin2vm_diablo_phase2_t)dlsym(isl_handle, "bin2vm_diablo_phase2");
  ASSERT(Bin2VmDiabloPhase2, ("Unable to load symbol bin2vm_diablo_phase2: %s", dlerror()));

  Bin2VmFreeVmImagesArm = (bin2vm_free_vmimages_arm_t)dlsym(isl_handle, "bin2vm_free_vmimages_arm");
  ASSERT(Bin2VmFreeVmImagesArm, ("Unable to load symbol bin2vm_free_vmimages_arm: %s", dlerror()));
}

/* this functionality is not present in the old SoftVM */
void SetTargetVmType(Bin2Vm *vm, bool b) {}

#endif /* DIABLOSOFTVM_SOFTVM_INTERFACE_H */
