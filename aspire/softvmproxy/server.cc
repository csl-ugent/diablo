#include "api.h"
#include "common.h"

/* C-library */
#include <cstring>
#include <dlfcn.h>
#include <signal.h>

#include <string>

#undef ASSERT
#define ASSERT(x, y) assert2(x, y)

using namespace std;

static void *isl_handle = NULL;
static Bin2Vm* isl_session = NULL;
static Bin2Vm* phase2_session = NULL;
static bin2vm_status_code_t b2vStatus;

bin2vm_create_iselect_t Bin2VmCreateInstructionSelector = NULL;
bin2vm_create_t Bin2VmCreate = NULL;
bin2vm_setTargetVmType_t Bin2VmSetTargetVmType = NULL;
bin2vm_setFlagTestMemory_t Bin2VmSetFlagTestMemory = NULL;
bin2vm_destroy_t Bin2VmDestroy = NULL;
bin2vm_check_arm_instruction_t Bin2VmCheckArmInstruction = NULL;
bin2vm_diablo_phase2_t Bin2VmDiabloPhase2 = NULL;
bin2vm_free_vmimages_arm_t Bin2VmFreeVmImagesArm = NULL;
bin2vm_setMobileCodeOutputDir_t Bin2VmSetMobileCodeOutputDir = NULL;
bin2vm_setIRoptimization_t Bin2vmSetIRoptimization = NULL;

static
void my_handler(int s) {
  auto x = SoftVMProxy(true);
  x.SendCommand(SoftVMProxyCommand::Close);

  /* we only get here when the 'close' command has been received */
  debug("exit!");

  /* Close all DL handlers */
  b2vStatus = Bin2VmDestroy(&isl_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not destroy instruction selector session %d", b2vStatus));

  b2vStatus = Bin2VmDestroy(&phase2_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not destroy phase-2 session %d", b2vStatus));

  dlclose(isl_handle);
  debug("bye");
  
  exit(0);
}

static
void InstallSignalHandler() {
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = my_handler;
  sigemptyset(&(sigIntHandler.sa_mask));
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
}

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

  Bin2vmSetIRoptimization = (bin2vm_setIRoptimization_t)dlsym(isl_handle, "bin2vm_setIRoptimization");
  ASSERT(Bin2vmSetIRoptimization, ("Unable to load symbol bin2vm_setIRoptimization: %s", dlerror()));
}

bool SoftVMProxyIsInstructionSupported(t_uint32 ins, t_uint32 size)
{
  bool isSupported = false;

  debug("checking: " << hex << "0x" << ins << dec);
  b2vStatus = Bin2VmCheckArmInstruction( isl_session, reinterpret_cast<unsigned char *>(&ins), size, &isSupported );
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not check wether instruction 0x%08x is supported or not: %d", ins, b2vStatus));
  debug("ok? " << isSupported);

  return isSupported;
}

void SoftVMProxySetMobileCodeOutputDir(t_string str)
{
  debug("setting mobile code output directory to " << str);
  Bin2VmSetMobileCodeOutputDir(phase2_session, str);
}

bin2vm_list_vmimages_arm *SoftVMProxyDiabloPhase2(t_string json_string, t_string outfile)
{
  bin2vm_list_vmimages_arm *vmImages = NULL;

  debug("server-side phase 2: " << json_string);

  /* invoke the second phase pass of the X-translator (i.e., the post-linker fixups) */
  b2vStatus = Bin2VmDiabloPhase2(phase2_session, json_string, strlen(json_string),
                                 outfile,
                                 NULL, &vmImages);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("Something went wrong in the X-translator during the phase-2 pass (%d)", b2vStatus));

  return vmImages;
}

void SoftVMProxyFreeVMImages(bin2vm_list_vmimages_arm *vmImages)
{
  b2vStatus = Bin2VmFreeVmImagesArm(&vmImages);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("Something went wrong in the X-translator when freeing the list of VM images"));
}

int main(int argc, char** argv)
{
  /* parameter processing */
  ASSERT(argc == 2, ("need the path to the instruction selector library as the first argument"));
  char* library = argv[1];

  isl_handle = dlopen(library, RTLD_LAZY);
  ASSERT(isl_handle, ("could not load instruction selector library (%s)", dlerror()));

  LoadIslSymbols(isl_handle);

  /* open up the phase-2 session */
  b2vStatus = Bin2VmCreate(&phase2_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not create phase-2 session %d", b2vStatus));

  /* open up the instruction selector session */
  b2vStatus = Bin2VmCreateInstructionSelector(&isl_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not create instruction selector session %d", b2vStatus));

  /* select the VM type */
  Bin2VmSetTargetVmType(isl_session, BIN2VM_VM_TYPE_GENERATED);
  Bin2VmSetTargetVmType(phase2_session, BIN2VM_VM_TYPE_GENERATED);

  /* don't do IR optimizations */
  Bin2vmSetIRoptimization(isl_session, false);
  Bin2vmSetIRoptimization(phase2_session, false);

  /* install the signal handler */
  InstallSignalHandler();

  /* start the proxy daemon */
  SoftVMProxy(false).Start();

  return 0;
}
