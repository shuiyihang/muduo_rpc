#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>
#include <string>

class InetAddress {
 public:
  explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
  explicit InetAddress(const sockaddr_in &addr) : m_addr(addr) {}

  // IP地址字符串
  std::string toIp() const;
  // IP地址 + 端口 字符串
  std::string toIpPort() const;

  // 由网络端口号得到本地端口号
  uint16_t toPort() const;

  const sockaddr_in *getSockAddr() const { return &m_addr; }
  void setSockAddr(const sockaddr_in &addr) { m_addr = addr; }

 private:
  sockaddr_in m_addr;
  sockaddr f;
};