#include "TcpServer.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstdio>
#include <functional>
#include <string>
#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"

using std::placeholders::_1;
using std::placeholders::_2;

TcpServer::TcpServer(EventLoop *base_loop, const InetAddress &listenAddr, const std::string &app_name)
    : m_base_loop(base_loop),
      m_listen_name(listenAddr.toIpPort()),
      m_app_name(app_name),
      m_acceptor(new Acceptor(base_loop, listenAddr)),
      m_threadPool(new EventLoopThreadPool(base_loop, app_name)),
      m_connectionCallback(),
      m_messageCallback(),
      m_started(0),
      m_nextConnId(0) {
  m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {}

void TcpServer::setThreadNums(int numThreads) { m_threadPool->setThreadNum(numThreads); }
void TcpServer::start() {
  // load ??
  if (m_started++ == 0) {
    m_threadPool->start(m_threadInitCallback);
    // 开启监听
    m_base_loop->runInloop(std::bind(&Acceptor::listen, m_acceptor.get()));
  }
}

// accept channel中read 发生回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  auto ioLoop = m_threadPool->getNextLoop();

  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "%s_%d", m_listen_name.c_str(), m_nextConnId);
  m_nextConnId++;

  std::string connName = m_app_name + buf;
  LOG_INFO("TcpServer::newConnection - new connection[%s] from %s", connName.c_str(), peerAddr.toIpPort().c_str());

  sockaddr_in local;
  socklen_t addrLen = sizeof(local);

  bzero(&local, addrLen);
  // 获取已连接套接字的本地地址(服务器地址)
  if (::getsockname(sockfd, reinterpret_cast<sockaddr *>(&local), &addrLen) < 0) {
    LOG_ERROR("getLocalAddr %d", errno);
  }

  InetAddress localAddr(local);

  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

  m_connections[connName] = conn;

  conn->setConnectionCallback(m_connectionCallback);
  conn->setMessageCallback(m_messageCallback);
  conn->setWriteCompleteCallback(m_writeCompleteCallback);

  // 使用者对close不关注,替他做了
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, conn));

  // 连接建立
  ioLoop->runInloop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
  m_base_loop->runInloop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
  LOG_DEBUG("TcpServer::removeConnectionInLoop [%s]", conn->name().c_str());
  m_connections.erase(conn->name());

  EventLoop *ioLoop = conn->getLoop();  // base loop处理也无不可吧
  ioLoop->queueInloop(std::bind(&TcpConnection::connectDestroyed, conn));
}
