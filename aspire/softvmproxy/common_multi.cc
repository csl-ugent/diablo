#include "common.h"
#include "api.h"

/* system headers */
#include <sys/socket.h>
#include <sys/un.h> /* struct sockaddr_un */
#include <sys/time.h> /* FD_SET, FD_ISSET, FD_ZERO */

/* C-library */
#include <unistd.h>

using namespace std;

#define SOCKET_FILE "/tmp/softvmproxy.socket"

#define MAX_CLIENTS 10
static int client_sockets[MAX_CLIENTS] = {0};
static fd_set read_fds;
#define MAX_PENDING_CONNECTIONS 3

static t_string rxbuf[2] = {nullptr};
static t_uint32 rxbuf_len[2] = {0};

SoftVMProxy::SoftVMProxy(bool client)
{
  is_client = client;

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

    /* clear data structure */
    for (int i = 0; i < MAX_CLIENTS; i++)
      client_sockets[i] = 0;

    /* try opening a socket */
    socket_fd = OpenSocket();

    ASSERT(::bind(socket_fd, (const struct sockaddr *) &name, sizeof(name)) > -1, ("bind error"));
  }
}

void SoftVMProxy::Start() {
  if (is_client)
    return;

  /* SERVER */

  /* prepare for accepting connections */
  ASSERT(listen(socket_fd, MAX_PENDING_CONNECTIONS) > -1, ("listen error"));

  while (true) {
    /* clear socket set */
    FD_ZERO(&read_fds);

    /* add master socket to set */
    FD_SET(socket_fd, &read_fds);
    int max_sd = socket_fd;

    /* add client sockets to set */
    for (int i = 0; i < MAX_CLIENTS; i++) {
      /* socket descriptor for this client */
      int sd = client_sockets[i];

      if (sd > 0)
        FD_SET(sd, &read_fds);

      if (sd > max_sd)
        max_sd = sd;
    }

    /* wait for activity on one of the sockets in the set */
    ASSERT((select(max_sd + 1, &read_fds, NULL, NULL, NULL) > -1) && (errno != EINTR), ("select error"));

    /* event on master socket */
    if (FD_ISSET(socket_fd, &read_fds)) {
      int new_socket = accept(socket_fd, NULL, NULL);
      ASSERT(new_socket > -1, ("accept error"));

      /* add to list of clients */
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
          client_sockets[i] = new_socket;
          debug("CLIENT[" << new_socket << "]: open connection #" << i);
          break;
        }
      }
    }

    /* event on client socket */
    for (int i = 0; i < MAX_CLIENTS; i++) {
      my_socket = client_sockets[i];

      if (FD_ISSET(my_socket, &read_fds)) {
        SoftVMProxyCommand cmd = ReceiveCommand();

        bool end = false;
        if (cmd == SoftVMProxyCommand::End) {
          debug("CLIENT[" << my_socket << "]: end of transmission");
          end = true;
        }
        else
          end |= ProcessCommand(cmd);

        if (end) {
          debug("CLIENT[" << my_socket << "]: close connection");
          close(my_socket);

          client_sockets[i] = 0;
        }
      }
    }
  }
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
    debug("CLIENT[" << my_socket << "]: command SendString");
    t_uint32 idx = ReceiveU32();

    /* include null character here */
    t_uint32 x = ReceiveU32();
    debug("CLIENT[" << my_socket << "]: of length " << x);

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
      debug("CLIENT[" << my_socket << "]: not allocating any string!");
  }
    break;

  case SoftVMProxyCommand::IsInstructionSupported:
  {
    debug("CLIENT[" << my_socket << "]: command IsInstructionSupported");
    t_uint32 ins = ReceiveU32();
    t_uint32 size = ReceiveU32();

    bool result = false;
    if (size > 0)
      result = SoftVMProxyIsInstructionSupported(ins, size);

    SendBool(result);
  }
    break;

  case SoftVMProxyCommand::SetMobileCodeOutputDir:
    debug("CLIENT[" << my_socket << "]: command SetMobileCodeOutputDir");
    SoftVMProxySetMobileCodeOutputDir(rxbuf[0]);
    break;

  case SoftVMProxyCommand::DiabloPhase2:
  {
    debug("CLIENT[" << my_socket << "]: command DiabloPhase2");
    auto data = SoftVMProxyDiabloPhase2(rxbuf[0], rxbuf[1]);
    SendVMImages(data);
  }
    break;

  case SoftVMProxyCommand::Close:
    assert2(!is_client, ("client is not allowed to close!"));
    debug("CLIENT[" << my_socket << "]: command Close");
    result = true;
    break;

  default:
    debug("CLIENT[" << my_socket << "]: command UNSUPPORTED!");
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
  debug("CLIENT[" << my_socket << "]: tx command " << static_cast<t_uint32>(cmd));
  Tx(&cmd, sizeof(SoftVMProxyCommand));
}

SoftVMProxyCommand SoftVMProxy::ReceiveCommand()
{
  debug("CLIENT[" << my_socket << "]: waiting for command...");
  SoftVMProxyCommand result;
  bool received = Rx(&result, sizeof(SoftVMProxyCommand));

  if (received) {
    debug("CLIENT[" << my_socket << "]: rx command " << static_cast<t_uint32>(result));
  } else {
    result = SoftVMProxyCommand::End;
  }

  return result;
}

void SoftVMProxy::SendString(char* buffer, t_uint32 idx)
{
  debug("CLIENT[" << my_socket << "]: tx string ");
  SendCommand(SoftVMProxyCommand::SendString);
  SendU32(idx);

  auto len = buffer ? strlen(buffer) : 0;
  SendU32(len);

  if (len > 0)
  {
    debug("CLIENT[" << my_socket << "]: tx DATA");
    Tx(buffer, len);
  }
}

t_string SoftVMProxy::ReceiveString(t_uint32 idx)
{
  auto len = rxbuf_len[idx];
  Rx(rxbuf[idx], len);
  debug("CLIENT[" << my_socket << "]: rx DATA");
  rxbuf[idx][len] = '\0';
  debug("CLIENT[" << my_socket << "]: rx string #" << idx << " of length " << len);

  return rxbuf[idx];
}

void SoftVMProxy::SendU32(t_uint32 x)
{
  debug("CLIENT[" << my_socket << "]: tx u32 " << x);
  Tx(&x, sizeof(t_uint32));
}

t_uint32 SoftVMProxy::ReceiveU32()
{
  t_uint32 result = 0;
  Rx(&result, sizeof(t_uint32));
  debug("CLIENT[" << my_socket << "]: rx u32 " << result);

  return result;
}

void SoftVMProxy::SendBool(bool b)
{
  debug("CLIENT[" << my_socket << "]: tx bool " << b);
  Tx(&b, sizeof(bool));
}

bool SoftVMProxy::ReceiveBool()
{
  bool result = 0;
  Rx(&result, sizeof(bool));
  debug("CLIENT[" << my_socket << "]: rx bool " << result);

  return result;
}

void SoftVMProxy::SendData(void *buffer, t_uint32 length)
{
  debug("CLIENT[" << my_socket << "]: tx data " << length);
  Tx(buffer, length);
}

void SoftVMProxy::ReceiveData(void *buffer, t_uint32 length)
{
  Rx(buffer, length);
  debug("CLIENT[" << my_socket << "]: rx data " << length);
}

void SoftVMProxy::SendVMImages(bin2vm_list_vmimages_arm *data)
{
  debug("CLIENT[" << my_socket << "]: tx images")
  bin2vm_list_vmimages_arm *it = data;

  /* first get the number of images to be transfered */
  t_uint32 counter = 0;
  while (it)
  {
    counter++;
    it = it->next;
  }
  debug("CLIENT[" << my_socket << "]: got " << counter << " images");
  SendU32(counter);

  /* then send the images one after another */
  it = data;
  while (it)
  {
    debug("CLIENT[" << my_socket << "]: tx image");
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
    debug("CLIENT[" << my_socket << "]: rx image " << i << "/" << nr_images);
    /* receive one image */
    bin2vm_list_vmimages_arm *new_item = new bin2vm_list_vmimages_arm();
    new_item->size = ReceiveU32();
    debug("CLIENT[" << my_socket << "]: receiving image of size " << new_item->size);
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
    debug("CLIENT[" << my_socket << "]:       tx'ed " << result << " bytes");
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
    debug("CLIENT[" << my_socket << "]:       rx'ed " << result << " bytes");
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
