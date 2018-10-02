// This file is a derivative work of Rob Tougher's
// c++ sockets implementation.  Please see socket.LICENSE
// and AUTHORS for details.

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>

#include <netinet/tcp.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 1000;
const int MAXRECV = 1024; //4096;

static void catch_empty(int empty)
{
}

class Socket
{
public:
  Socket();
  virtual ~ Socket();

  // Server initialization
  bool create();
  bool bind(const int port);
  bool listen() const;
  bool accept(Socket &) const;

  // Client initialization
  bool connect(const std::string host, const int port);

  // Data Transimission
  bool send(const std::string) const;
  int recv(std::string &) const;
  char* recv() const;

  void set_non_blocking(const bool);

  int get_rcvbuf(void);
  int get_cinq(void);
  int get_sndbuf(void);
  int get_coutq(void);

  bool is_valid() const
  {
    return m_sock != -1;
  }
  int timeout;

private:

  int m_sock;
  sockaddr_in m_addr;

};


Socket::Socket():m_sock(-1)
{
  memset(&m_addr, 0, sizeof(m_addr));
  
	// Set SIGIO to empty function
	{
	  signal(SIGIO,catch_empty);
	}  
}

Socket::~Socket()
{
  if (is_valid())
    ::close(m_sock);
}

bool Socket::create()
{
  m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (!is_valid())
    return false;

  /*
  int on = 1;  
  
  if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on,
      sizeof(on)) == -1)
    return false;
  */
  
  return true;
}



bool Socket::bind(const int port)
{

  if (!is_valid())
  {
    return false;
  }

  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons(port);

  int bind_return =::bind(m_sock, (struct sockaddr *)&m_addr, sizeof(m_addr));

  if (bind_return == -1)
  {
    return false;
  }

  return true;
}

bool Socket::listen() const
{
  if (!is_valid())
  {
    return false;
  }

  int listen_return =::listen(m_sock, MAXCONNECTIONS);


  if (listen_return == -1)
  {
    return false;
  }

  return true;
}

bool Socket::accept(Socket & new_socket) const
{
  int addr_length = sizeof(m_addr);
  new_socket.m_sock =::accept(m_sock, (sockaddr *) & m_addr,
    (socklen_t *) & addr_length);

  if (new_socket.m_sock <= 0)
    return false;
  else
    return true;
}

bool Socket::send(const std::string s) const
{
  int status =::send(m_sock, s.c_str(), s.size(), MSG_NOSIGNAL);
  if (status == -1)
  {
    return false;
  }
  else
  {
    //::flush(m_sock);
    return true;
  }
}

/*
int Socket::recv_old(std::string & s) const
{
  struct timeval tv;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  if (timeout)
  {
    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
        sizeof(tv)) == -1)
      return false;
  }

  char buf[MAXRECV + 1];

  s = "";

  memset(buf, 0, MAXRECV + 1);

  int status =::recv(m_sock, buf, MAXRECV, 0);

  if (status == -1)
  {
    if (timeout == 0)
    {
      //std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
    }
    return 0;
  }

  else if (status == 0)
  {
    return 0;
  }
  else
  {
    s = buf;
    return status;
  }
}
*/

int Socket::recv(std::string & s) const
//static int rtmp_receive_data(struct rtmp_client *client, char *data, int length, int timeout) 
{
  static char buf[MAXRECV + 1];
  int result;
	struct pollfd fds[1];

  memset(fds, 0, sizeof(fds));  
  	
  fds[0].fd = m_sock;
  fds[0].events = POLLIN | POLLERR;
  fds[0].revents = 0;
  
  s= "";  
  memset(buf, 0, MAXRECV + 1);  
	
	/*
  result = 0;
  while(result == 0)
  {
	  result = poll(fds, 1, timeout);
	  	  
	  if (result == 0)
	  return 0;
	  if (result < 0)
	  return -1;
  }	
  */
  
  //std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";  
        	
  result = ::recv(m_sock, buf, MAXRECV, MSG_DONTWAIT | MSG_NOSIGNAL);
  //std::cout << "errno == " << errno << "  in Socket::recv\n";  
 
  if (result <= 0)
  {			
    return -1;
  }  

  s = buf;
  return result;   
}

char* Socket::recv(void) const
//static int rtmp_receive_data(struct rtmp_client *client, char *data, int length, int timeout) 
{
  static char buf[MAXRECV + 1];
  char *s=buf;
  int result;
	struct pollfd fds[1];

  memset(fds, 0, sizeof(fds));  
  	
  fds[0].fd = m_sock;
  fds[0].events = POLLIN | POLLERR;
  fds[0].revents = 0;
  
  memset(buf, 0, MAXRECV + 1);  
	
	/*
  result = 0;
  while(result == 0)
  {
	  result = poll(fds, 1, timeout);
	  	  
	  if (result <= 0)
	  return s;
  }	
  */
  
  //std::cout << "status1 == -1   errno == " << errno << "  in Socket::recv\n";  
        	
  //result = ::recv(m_sock, buf, MAXRECV, MSG_DONTWAIT | MSG_NOSIGNAL);
  result = ::recv(m_sock, buf, MAXRECV, MSG_NOSIGNAL);
  //std::cout << "errno == " << errno << "  in Socket::recv(void)\n";  

  //std::cout << "result == " << result << "  in Socket::recv\n";  
  //std::cout << "status2 == -1   errno == " << errno << "  in Socket::recv\n";  
 
  if (result <= 0)
  {			
    return s;
  }  

  return s;   
}

bool Socket::connect(const std::string host, const int port)
{
  if (!is_valid())
    return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons(port);

  int status = inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);

  if (errno == EAFNOSUPPORT)
    return false;

  status =::connect(m_sock, (sockaddr *) & m_addr, sizeof(m_addr));

  {
    struct timeval tv; 
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
    {
    }
  } 

  {
    int optval = 1;

    /* Set the option active */
    if (setsockopt(m_sock, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval)))
    {
    }
  }

  {
    int tcpbuffer = 100000;

    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &tcpbuffer, sizeof(tcpbuffer)))
    {
    }

    if (setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, &tcpbuffer, sizeof(tcpbuffer)))
    {
    }
  }
  
  if (status == 0)
    return true;
  else
    return false;
}

void Socket::set_non_blocking(const bool b)
{

  int opts = fcntl(m_sock, F_GETFL);

  if (opts < 0)
  {
    return;
  }

  if (b)
    opts = (opts | O_NONBLOCK);
  else
    opts = (opts & ~O_NONBLOCK);

  fcntl(m_sock, F_SETFL, opts);
}


int Socket::get_rcvbuf(void)
{
  int rcvbufsiz = 0;
  socklen_t len = sizeof(rcvbufsiz);

  if (getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &rcvbufsiz, &len) < 0)
  {
  }

  return rcvbufsiz;
}

int Socket::get_cinq(void)
{
  int used = 0;

  if (ioctl(m_sock, SIOCINQ, &used) < 0)
  {
  }

  return used;

}

int Socket::get_sndbuf(void)
{
  int sndbufsiz = 0;
  socklen_t len = sizeof(sndbufsiz);

  if (getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, &sndbufsiz, &len) < 0)
  {
  }

  return sndbufsiz;
}

int Socket::get_coutq(void)
{
  int used = 0;

  if (ioctl(m_sock, SIOCOUTQ, &used) < 0)
  {
  }

  return used;

}
