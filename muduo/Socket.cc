#include "Socket.h"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include "InetAddress.h"
#include "Logger.h"

// socket不是主要用于监听连接的嘛，为什么有一个shutdownwrite

void Socket::bindAddress(const InetAddress &addr) {
  // const对象只能调用const成员函数
  // 成功返回0
  if (0 != bind(m_sockfd, reinterpret_cast<const sockaddr *>(addr.getSockAddr()), sizeof(sockaddr_in))) {
    LOG_FATAL("bind sockfd err %d", errno);
  }
}
void Socket::listen() {
  // The  backlog  argument defines the maximum length to which the queue of pending connections for sockfd may grow.
  if (0 != ::listen(m_sockfd, 1024)) {
    LOG_FATAL("listen sockfd err %d", errno);
  }
}
int Socket::accept(InetAddress *peerAddr) {
  sockaddr_in addr;
  socklen_t len = sizeof(sockaddr_in);
  bzero(&addr, len);

  // 设置连接的描述符未非阻塞
  int confd = ::accept4(m_sockfd, reinterpret_cast<sockaddr *>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

  if (confd >= 0) {
    peerAddr->setSockAddr(addr);
  }

  return confd;
}

void Socket::shutdownWrite() {
  if (::shutdown(m_sockfd, SHUT_WR) < 0) {
    LOG_ERROR("shutdownWrite err %d", errno);
  }
}
void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}