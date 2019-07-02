#include "common.h"
#include "api.h"

/* system headers */
#include <sys/socket.h>
#include <sys/un.h> /* struct sockaddr_un */

/* C-library */
#include <unistd.h>

using namespace std;

#define SOCKET_FILE "/tmp/softvmproxy.socket"

static t_string rxbuf[2] = {nullptr};
static t_uint32 rxbuf_len[2] = {0};
static int rx_string_idx = -1;

volatile static bool rx_close = false;

SoftVMProxy::SoftVMProxy(bool client)
{
  is_client = client;
  rx_close = false;

  struct sockaddr_un name;
  memset(&name, 0, sizeof(struct sockaddr_un));

  /* bind socket to socket name */
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_FILE, sizeof(name.sun_path) - 1);

  if (is_client)
  {
    debug("opening client");
    /* client-side setup */
    my_socket = OpenSocket();

    bool connected = false;
    for (int i = 0; i < 5; i++)
    {
      auto result = connect(my_socket, (sockaddr *) &name, sizeof(name));
      if (result >= 0) {
        debug("connected!");
        connected = true;
        break;
      }
      sleep(1);
    }
    ASSERT(connected, ("could not connect to socket"));
  }
  else
  {
    /* server-side setup */
    debug("opening server");

    /* remove the socket if it already exists */
    unlink(SOCKET_FILE);

    /* try opening a socket */
    socket_fd = OpenSocket();

    ASSERT(::bind(socket_fd, (const struct sockaddr *) &name, sizeof(name)) > -1, ("could not bind to socket"));
  }
}

void SoftVMProxy::Start() {
  if (is_client)
    return;
    
  debug("accepting connections 1");

  /* prepare for accepting connections */
  ASSERT(listen(socket_fd, 1) > -1, ("could not listen on socket"));

  /* connection handling */

  while (true) {
    debug("accepting connections 2");
    my_socket = accept(socket_fd, NULL, NULL);
    ASSERT(my_socket > -1, ("could not accept connections on socket"));

    debug("processing commands");
    while (!rx_close) {
      SoftVMProxyCommand cmd = ReceiveCommand();
      
      if (cmd == SoftVMProxyCommand::End) {
        debug("end of transmission!");
        break;
      }

      ProcessCommand(cmd);
    }
  }

  close(my_socket);
}

SoftVMProxy::~SoftVMProxy()
{
  if (!is_client)
    close(socket_fd);
}

bool SoftVMProxy::ProcessCommand(SoftVMProxyCommand cmd)
{
  bool result = false;

  switch (cmd)
  {
  case SoftVMProxyCommand::SendString:
  {
    debug("command SendString");
    t_uint32 idx = ReceiveU32();

    /* include null character here */
    t_uint32 x = ReceiveU32();
    debug("of length " << x);

    /* need to allocate a new array */
    rxbuf_len[idx] = x;

    /* destroy the old array if needed */
    if (rxbuf[idx] != nullptr)
    {
      delete[] rxbuf[idx];
      rxbuf[idx] = nullptr;
    }

    /* create a new array */
    if (rxbuf_len[idx] > 0)
    {
      rxbuf[idx] = new char[rxbuf_len[idx] + 1];
      ReceiveString(idx);
    }
    else
      debug("not allocating any string!");
  }
    break;

  case SoftVMProxyCommand::IsInstructionSupported:
  {
    debug("command IsInstructionSupported");
    t_uint32 ins = ReceiveU32();
    t_uint32 size = ReceiveU32();
    
    bool result = false;
    if (size > 0)
      result = SoftVMProxyIsInstructionSupported(ins, size);

    SendBool(result);
  }
    break;

  case SoftVMProxyCommand::SetMobileCodeOutputDir:
    debug("command SetMobileCodeOutputDir");
    SoftVMProxySetMobileCodeOutputDir(rxbuf[0]);
    break;

  case SoftVMProxyCommand::DiabloPhase2:
  {
    debug("command DiabloPhase2");
    auto data = SoftVMProxyDiabloPhase2(rxbuf[0], rxbuf[1]);
    SendVMImages(data);
  }
    break;

  case SoftVMProxyCommand::Close:
    assert2(!is_client, ("client is not allowed to close!"));
    debug("command Close");
    rx_close = true;
    break;

  default:
    debug("command UNSUPPORTED!");
  }

  return result;
}

int SoftVMProxy::OpenSocket()
{
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  ASSERT(fd > -1, ("could not open socket"));

  return fd;
}

void SoftVMProxy::SendCommand(SoftVMProxyCommand cmd)
{
  debug("tx command " << static_cast<t_uint32>(cmd));
  Tx(&cmd, sizeof(SoftVMProxyCommand));
}

SoftVMProxyCommand SoftVMProxy::ReceiveCommand()
{
  debug("waiting for command...");
  SoftVMProxyCommand result;
  bool received = Rx(&result, sizeof(SoftVMProxyCommand));
  
  if (received) {
    debug("rx command " << static_cast<t_uint32>(result));
  } else {
    result = SoftVMProxyCommand::End;
  }

  return result;
}

void SoftVMProxy::SendString(char* buffer, t_uint32 idx)
{
  debug("tx string ");
  SendCommand(SoftVMProxyCommand::SendString);
  SendU32(idx);

  auto len = buffer ? strlen(buffer) : 0;
  SendU32(len);

  if (len > 0)
  {
    debug("tx DATA");
    Tx(buffer, len);
  }
}

t_string SoftVMProxy::ReceiveString(t_uint32 idx)
{
  auto len = rxbuf_len[idx];
  Rx(rxbuf[idx], len);
  debug ("rx DATA");
  rxbuf[idx][len] = '\0';
  debug("rx string #" << idx << " of length " << len);

  return rxbuf[idx];
}

void SoftVMProxy::SendU32(t_uint32 x)
{
  debug("tx u32 " << x);
  Tx(&x, sizeof(t_uint32));
}

t_uint32 SoftVMProxy::ReceiveU32()
{
  t_uint32 result = 0;
  Rx(&result, sizeof(t_uint32));
  debug("rx u32 " << result);

  return result;
}

void SoftVMProxy::SendBool(bool b)
{
  debug("tx bool " << b);
  Tx(&b, sizeof(bool));
}

bool SoftVMProxy::ReceiveBool()
{
  bool result = 0;
  Rx(&result, sizeof(bool));
  debug("rx bool " << result);

  return result;
}

void SoftVMProxy::SendData(void *buffer, t_uint32 length)
{
  debug("tx data " << length);
  Tx(buffer, length);
}

void SoftVMProxy::ReceiveData(void *buffer, t_uint32 length)
{
  Rx(buffer, length);
  debug("rx data " << length);
}

void SoftVMProxy::SendVMImages(bin2vm_list_vmimages_arm *data)
{
  debug("tx images")
  bin2vm_list_vmimages_arm *it = data;

  /* first get the number of images to be transfered */
  t_uint32 counter = 0;
  while (it)
  {
    counter++;
    it = it->next;
  }
  debug("got " << counter << " images");
  SendU32(counter);

  /* then send the images one after another */
  it = data;
  while (it)
  {
    debug("tx image");
    SendU32(it->size);
    SendData(static_cast<void *>(it->data), static_cast<t_uint32>(it->size));

    it = it->next;
  }

  /* we don't need this memory anymore */
  SoftVMProxyFreeVMImages(data);
}

bin2vm_list_vmimages_arm *SoftVMProxy::ReceiveVMImages()
{
  /* first get the number of images transfered */
  t_uint32 nr_images = ReceiveU32();

  bin2vm_list_vmimages_arm *result = NULL;
  bin2vm_list_vmimages_arm *result_end = result;
  for (t_uint32 i = 0; i < nr_images; i++)
  {
    debug("rx image " << i << "/" << nr_images);
    /* receive one image */
    bin2vm_list_vmimages_arm *new_item = new bin2vm_list_vmimages_arm();
    new_item->size = ReceiveU32();
    debug("receiving image of size " << new_item->size);
    new_item->data = new unsigned char[new_item->size];
    ReceiveData(new_item->data, new_item->size);
    /* 'next' is zero-initialised by 'new' */

    /* construct the linked list */
    if (result == NULL)
      result = new_item;
    else
      result_end->next = new_item;

    result_end = new_item;
  }

  return result;
}

void SoftVMProxy::Tx(void *buffer, t_uint32 size)
{
  uint8_t *buf = static_cast<uint8_t *>(buffer);
  while (size > 0)
  {
    /* need to send more data */
    int result = send(my_socket, buf, size, 0);
    debug("      tx'ed " << result << " bytes");
    if (result < 0) {
      printf("could not send data! %s\n", strerror(result));
      return;
    }
    buf += result;
    size -= result;
  }
}

bool SoftVMProxy::Rx(void *buffer, t_uint32 size)
{
  uint8_t *buf = static_cast<uint8_t *>(buffer);
  int times_zero = 10;
  while (size > 0)
  {
    /* need to receive more data */
    int result = recv(my_socket, buf, size, 0);
    debug("      rx'ed " << result << " bytes");
    if (result == 0) times_zero--;
    if (result < 0) {
      printf("could not receive data! %s\n", strerror(result));
      return false;
    }
    buf += result;
    size -= result;

    if (times_zero == 0)
      return false;
  }
  
  return true;
}
