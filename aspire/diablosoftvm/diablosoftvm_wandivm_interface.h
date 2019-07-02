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
typedef bin2vm_status_code_t (*bin2vm_setIRoptimization_t)( Bin2Vm* self, bool optimizeIR );

#endif /* DIABLOSOFTVM_WANDIVM_INTERFACE_H */
