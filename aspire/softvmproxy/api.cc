#include "api.h"
#include "common.h"

#include <iostream>
#include <string>
using namespace std;

static SoftVMProxy *conn;

bool SoftVMProxyIsInstructionSupported(t_uint32 ins, t_uint32 size)
{
  debug("  sending command IsInstructionSupported");
  conn->SendCommand(SoftVMProxyCommand::IsInstructionSupported);
  debug("  sending binary data");
  conn->SendU32(ins);
  debug("  sending size");
  conn->SendU32(size);

  return conn->ReceiveBool();
}

void SoftVMProxySetMobileCodeOutputDir(t_string str)
{
  debug("  sending string [" << string(str) << "]");
  conn->SendString(str);
  debug("  sending command SetMobileCodeOutputDir");
  conn->SendCommand(SoftVMProxyCommand::SetMobileCodeOutputDir);
}

bin2vm_list_vmimages_arm *SoftVMProxyDiabloPhase2(t_string json, t_string outfile)
{
  debug("  sending string 0 [JSON]");
  conn->SendString(json, 0);
  debug("  sending string 1 [" << outfile << "]");
  conn->SendString(outfile, 1);
  debug("  sending command DiabloPhase2");
  conn->SendCommand(SoftVMProxyCommand::DiabloPhase2);

  debug("  receiving image data");
  return conn->ReceiveVMImages();
}

void SoftVMProxyFreeVMImages(bin2vm_list_vmimages_arm *vmImage)
{
  while (vmImage)
  {
    delete[] vmImage->data;

    auto tmp = vmImage->next;
    delete vmImage;

    vmImage = tmp;
  }
}

void SoftVMProxyInit()
{
  conn = new SoftVMProxy();
}

void SoftVMProxyDestroy()
{
  delete conn;
}
