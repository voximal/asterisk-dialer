// This file is a derivative work of Rob Tougher's
// c++ sockets implementation.  Please see socket.LICENSE
// and AUTHORS for details.

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

#ifndef SOCKET
#define SOCKET

// Don't forget to make sure MAXCONNECTIONS is ALWAYS high enough...




#include "SocketFunctions.h"

class ClientSocket:public Socket
{
  public:

  ClientSocket(std::string host, int port)
  {
    Socket::create();
    Socket::connect(host, port);
    timeout = 0;
  }
  ~ClientSocket()
  {
  };

  void setRecvTimeout(int milliseconds)
  {
    timeout = milliseconds;
  }

  const ClientSocket & operator <<(const std::string & s) const
  {
    Socket::send(s);
#ifdef DBG_TRACE
    DBG_TRACE(DBG_TCP, (0, "> %s", s.c_str()));
#endif
    return *this;

  }

  const ClientSocket & operator >>(std::string & s) const
  {
    Socket::recv(s);
#ifdef DBG_TRACE
    DBG_TRACE(DBG_TCP, (0, "< %s", s.c_str()));
#endif
    return *this;
  }

};

#endif
