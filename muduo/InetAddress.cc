#include "InetAddress.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <cstdio>
#include <cstring>

#include "Logger.h"

InetAddress::InetAddress(uint16_t port, std::string ip) {
  bzero(&m_addr, sizeof m_addr);

  m_addr.sin_family = AF_INET;  // ipv4
  m_addr.sin_port = htons(port);
  m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
  char buf[INET_ADDRSTRLEN] = {0};
  inet_ntop(AF_INET, &m_addr.sin_addr, buf, INET_ADDRSTRLEN);
  return buf;
}
std::string InetAddress::toIpPort() const {
  char buf[64] = {0};
  memcpy(buf, toIp().c_str(), sizeof buf);
  int len = strlen(buf);

  snprintf(buf + len, sizeof(buf) - len, ":%u", toPort());
  return buf;
}

uint16_t InetAddress::toPort() const { return ntohs(m_addr.sin_port); }

// int main()
// {
//     InetAddress addr(9600);
//     LOG_INFO("%s",addr.toIpPort().c_str());
//     return 0;
// }