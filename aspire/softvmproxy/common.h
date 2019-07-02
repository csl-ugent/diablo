#ifndef SOFTVMPROXY_COMMON_H
#define SOFTVMPROXY_COMMON_H

#include <cstdint>
#include <iostream>

#include "diablo.h"
#include "diablosoftvm/diablosoftvm_wandivm_interface.h"

//#define debug(x) cout << x << endl; fflush(stdout);
#define debug(x)

#define assert2(x, y) \
do {\
  if (!(x)) {\
    cout << "assertion failed! " << #x << ": " << y << endl;\
    exit(1);\
  }\
} while (0);
#define assert(x) assert2(x, "(no message)")
#define ASSERT(x, y) assert2(x, y)

enum class SoftVMProxyCommand {
  SendString,
  SendString2,
  IsInstructionSupported,
  SetMobileCodeOutputDir,
  Close,
  DiabloPhase2,
  End
};

class SoftVMProxy {
public:
  SoftVMProxy(bool client = true);
  ~SoftVMProxy();
  
  void Start();

  /* Tx/Rx commands */
  void SendCommand(SoftVMProxyCommand cmd);
  SoftVMProxyCommand ReceiveCommand();

  /* Tx/Rx strings */
  void SendString(t_string buffer, t_uint32 idx = 0);
  t_string ReceiveString(t_uint32 idx);

  /* Tx/Rx 32-bit integers */
  void SendU32(t_uint32 x);
  t_uint32 ReceiveU32();

  /* Tx/Rx boolean values */
  void SendBool(bool b);
  bool ReceiveBool();

  /* Tx/Rx VM images */
  void SendVMImages(bin2vm_list_vmimages_arm *data);
  bin2vm_list_vmimages_arm *ReceiveVMImages();

  /* Tx/Rx data */
  void SendData(void *d, t_uint32 length);
  void ReceiveData(void *buffer, t_uint32 length);

private:
  bool ProcessCommand(SoftVMProxyCommand cmd);
  int OpenSocket();
  void ConnectSocket();

  void Tx(void *buffer, t_uint32 size);
  bool Rx(void *buffer, t_uint32 size);

  int my_socket;
  bool is_client;
  int socket_fd;
};

/* SoftVM API */
bool IsInstructionSupported(t_uint32 ins, t_uint32 size);
void SetMobileCodeOutputDir(t_string str);
bin2vm_list_vmimages_arm *DiabloPhase2(t_string json_string, t_string outfile);
void FreeVMImages(bin2vm_list_vmimages_arm *vmImages);

#endif /* SOFTVMPROXY_COMMON_H */
