#ifndef SOFTVMPROXY_API_H
#define SOFTVMPROXY_API_H

#include "diablo.h"
#include "diablosoftvm/diablosoftvm_wandivm_interface.h"

void SoftVMProxyInit();
void SoftVMProxyDestroy();
void SoftVMProxyFreeVMImages(bin2vm_list_vmimages_arm *vmImage);
void SoftVMProxySetMobileCodeOutputDir(t_string str);

bool SoftVMProxyIsInstructionSupported(t_uint32 ins, t_uint32 size);

bin2vm_list_vmimages_arm *SoftVMProxyDiabloPhase2(t_string json, t_string outfile);

#endif /* SOFTVMPROXY_API_H */
