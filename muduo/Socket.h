#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable {
 public:
  explicit Socket(int sockfd) : m_sockfd(sockfd) {}

  ~Socket() = default;

  int fd() { return m_sockfd; }

  void bindAddress(const InetAddress &addr);
  void listen();
  int accept(InetAddress *peerAddr);

  void shutdownWrite();
  void setKeepAlive(bool on);

 private:
  const int m_sockfd;
};