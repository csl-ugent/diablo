/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_WANDIVM_INTERFACE_H
#define DIABLOSOFTVM_WANDIVM_INTERFACE_H

#include "bin2vm/wandivm/bin2vm.h"
#include "bin2vm/wandivm/bin2vm_status_codes.h"
#include "bin2vm/wandivm/bin2vm_list_vmimages_arm.h"

typedef enum bin2vm_status_codes bin2vm_status_code_t;

typedef bin2vm_status_code_t (*bin2vm_create_iselect_t)( Bin2Vm** retSelf );
typedef bin2vm_status_code_t (*bin2vm_create_t)( Bin2Vm** pSelf );
typedef bin2vm_status_code_t (*bin2vm_setTargetVmType_t)( Bin2Vm* self, enum bin2vm_vm_type vmType );
typedef bin2vm_status_code_t (*bin2vm_setFlagTestMemory_t)( Bin2Vm* self, bool useTestMemory );
typedef bin2vm_status_code_t (*bin2vm_destroy_t)( Bin2Vm** pSelf );
typedef bin2vm_status_code_t (*bin2vm_check_arm_instruction_t)( Bin2Vm* self, unsigned char* armInstruction, unsigned int sizeArmInstruction, bool* retIsSupported );
typedef bin2vm_status_code_t (*bin2vm_diablo_phase2_t)(Bin2Vm* self,
                                                       const char* jsonBuffer, unsigned int sizeJsonBuffer,
                                                       const char *writeAsmFilename,
                                                       const char *dbgFilenameBody,
                                                       struct bin2vm_list_vmimages_arm** retVmImages);
typedef bin2vm_status_code_t (*bin2vm_free_vmimages_arm_t)( struct bin2vm_list_vmimages_arm** pList);
typedef bin2vm_status_code_t (*bin2vm_setMobileCodeOutputDir_t)(Bin2Vm* self, const char * mobileCodeOutputDir);

bin2vm_create_iselect_t Bin2VmCreateInstructionSelector = NULL;
bin2vm_create_t Bin2VmCreate = NULL;
bin2vm_setTargetVmType_t Bin2VmSetTargetVmType = NULL;
bin2vm_setFlagTestMemory_t Bin2VmSetFlagTestMemory = NULL;
bin2vm_destroy_t Bin2VmDestroy = NULL;
bin2vm_check_arm_instruction_t Bin2VmCheckArmInstruction = NULL;
bin2vm_diablo_phase2_t Bin2VmDiabloPhase2 = NULL;
bin2vm_free_vmimages_arm_t Bin2VmFreeVmImagesArm = NULL;
bin2vm_setMobileCodeOutputDir_t Bin2VmSetMobileCodeOutputDir = NULL;

void LoadIslSymbols(void *isl_handle) {
  Bin2VmCreateInstructionSelector = (bin2vm_create_iselect_t)dlsym(isl_handle, "bin2vm_create_iselect");
  ASSERT(Bin2VmCreateInstructionSelector, ("Unable to load symbol bin2vm_create_iselect: %s", dlerror()));

  Bin2VmCreate = (bin2vm_create_t)dlsym(isl_handle, "bin2vm_create");
  ASSERT(Bin2VmCreate, ("Unable to load symbol bin2vm_create: %s", dlerror()));

  Bin2VmSetTargetVmType = (bin2vm_setTargetVmType_t)dlsym(isl_handle, "bin2vm_setTargetVmType");
  ASSERT(Bin2VmSetTargetVmType, ("Unable to load symbol bin2vm_setTargetVmType: %s", dlerror()));

  Bin2VmSetFlagTestMemory = (bin2vm_setFlagTestMemory_t)dlsym(isl_handle, "bin2vm_setFlagTestMemory");
  ASSERT(Bin2VmSetFlagTestMemory, ("Unable to load symbol bin2vm_setFlagTestMemory: %s", dlerror()));

  Bin2VmDestroy = (bin2vm_destroy_t)dlsym(isl_handle, "bin2vm_destroy");
  ASSERT(Bin2VmDestroy, ("Unable to load symbol bin2vm_destroy: %s", dlerror()));

  Bin2VmCheckArmInstruction = (bin2vm_check_arm_instruction_t)dlsym(isl_handle, "bin2vm_check_arm_instruction");
  ASSERT(Bin2VmCheckArmInstruction, ("Unable to load symbol bin2vm_check_arm_instruction: %s", dlerror()));

  Bin2VmDiabloPhase2 = (bin2vm_diablo_phase2_t)dlsym(isl_handle, "bin2vm_diablo_phase2");
  ASSERT(Bin2VmDiabloPhase2, ("Unable to load symbol bin2vm_diablo_phase2: %s", dlerror()));

  Bin2VmFreeVmImagesArm = (bin2vm_free_vmimages_arm_t)dlsym(isl_handle, "bin2vm_free_vmimages_arm");
  ASSERT(Bin2VmFreeVmImagesArm, ("Unable to load symbol bin2vm_free_vmimages_arm: %s", dlerror()));

  Bin2VmSetMobileCodeOutputDir = (bin2vm_setMobileCodeOutputDir_t)dlsym(isl_handle, "bin2vm_setMobileCodeOutputDir");
  ASSERT(Bin2VmSetMobileCodeOutputDir, ("Unable to load symbol bin2vm_setMobileCodeOutputDir: %s", dlerror()));
}

void SetTargetVmType(Bin2Vm *vm, bool use_new_version) {
	Bin2VmSetTargetVmType(vm, (use_new_version) ? BIN2VM_VM_TYPE_NEXTGEN : BIN2VM_VM_TYPE_CLASSIC);
}

#endif /* DIABLOSOFTVM_WANDIVM_INTERFACE_H */
