#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "Acceptor.h"
#include "Callbacks.h"
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "noncopyable.h"

class TcpServer : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  TcpServer(EventLoop *base_loop, const InetAddress &listenAddr, const std::string &app_name);

  ~TcpServer();

  void setThreadInitCallback(const ThreadInitCallback &cb) { m_threadInitCallback = cb; }

  void setConnectionCallback(const ConnectionCallback &cb) { m_connectionCallback = cb; }
  void setCloseCallback(const CloseCallback &cb) { m_closeCallback = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { m_writeCompleteCallback = cb; }
  void setMessageCallback(const MessageCallback &cb) { m_messageCallback = cb; }

  void setThreadNums(int numThreads);
  void start();

 private:
  void newConnection(int sockfd, const InetAddress &peerAddr);

  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

 private:
  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

  std::string m_app_name;
  EventLoop *m_base_loop;
  std::unique_ptr<Acceptor> m_acceptor;
  std::unique_ptr<EventLoopThreadPool> m_threadPool;

  ThreadInitCallback m_threadInitCallback;

  ConnectionCallback m_connectionCallback;
  CloseCallback m_closeCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  MessageCallback m_messageCallback;

  std::atomic<int> m_started;

  const std::string m_listen_name;
  int m_nextConnId;  // 只有main loop 执行accept，所以线程安全

  ConnectionMap m_connections;
};