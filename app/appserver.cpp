#ifndef WIN32
  #include <unistd.h>
  #include <cstdlib>
  #include <cstring>
  #include <netdb.h>
#else
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <wspiapi.h>
#endif
#include <iostream>
#include <udt.h>
#include "cc.h"
#include "test_util.h"

using namespace std;

#ifndef WIN32
void* recvdata(void*);
#else
DWORD WINAPI recvdata(LPVOID);
#endif

int main(int argc, char* argv[]) {
  if ((1 != argc) && ((2 != argc) || (0 == atoi(argv[1]))) && ((3 != argc) || strcmp(argv[2], "-1"))) {
    cout << "usage: appserver [server_port] [-1]" << endl;
    return 0;
  }

  // Automatically start up and clean up UDT module.
  UDTUpDown _udt_;

  addrinfo hints;
  addrinfo* res;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  string service("9000");
  if (2 == argc) service = argv[1];

  if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res)) {
    cerr << "illegal port number or port is busy." << endl;
    return -1;
  }

  UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen)) {
    cerr << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
    return -1;
  }

  freeaddrinfo(res);

  cout << "server is ready at port: " << service << endl;

  if (UDT::ERROR == UDT::listen(serv, 10)) {
    cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
    return 0;
  }

  sockaddr_storage clientaddr;
  int addrlen = sizeof(clientaddr);

  UDTSOCKET recver;

  while (true) {
    if (UDT::INVALID_SOCK == (recver = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen))) {
      cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
    }

    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];
    getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
    cout << "new connection: " << clienthost << ":" << clientservice << endl;

    #ifndef WIN32
      pthread_t rcvthread;
      pthread_create(&rcvthread, NULL, recvdata, new UDTSOCKET(recver));
      pthread_detach(rcvthread);
    #else
      CreateThread(NULL, 0, recvdata, new UDTSOCKET(recver), 0, NULL);
    #endif
  }

  UDT::close(serv);

  return 0;
}

#ifndef WIN32
void* recvdata(void* usocket)
#else
DWORD WINAPI recvdata(LPVOID usocket)
#endif
{
  UDTSOCKET recver = *(UDTSOCKET*)usocket;
  delete (UDTSOCKET*)usocket;

  char* data;
  int size = 100000;
  data = new char[size];

  while (true) {
    int rsize = 0;
    int rs;
    while (rsize < size) {
      int rcv_size;
      int var_size = sizeof(int);
      UDT::getsockopt(recver, 0, UDT_RCVDATA, &rcv_size, &var_size);
      if (UDT::ERROR == (rs = UDT::recv(recver, data + rsize, size - rsize, 0))) {
        cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
        break;
      }

      rsize += rs;
    }

    if (rsize < size) break;
  }

  delete [] data;

  UDT::close(recver);

  #ifndef WIN32
    return NULL;
  #else
    return 0;
  #endif
}
