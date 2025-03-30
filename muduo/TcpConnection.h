#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include "Buffer.h"
#include "Callbacks.h"
#include "Channel.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Timestamp.h"
#include "noncopyable.h"

// 继承 std::enable_shared_from_this<TcpConnection>，
// TcpConnection 类可以调用 shared_from_this() 方法，获得一个指向自身的 std::shared_ptr

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
 public:
  enum class ConnectState { kDisconnecting, kDisconnected, kConnecting, kConnected };

  explicit TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                         const InetAddress &peerAddr);

  ~TcpConnection();

  void send(const std::string &buf);  // 通过建立的连接发送数据
  void shutdown();

  void connectEstablished();
  void connectDestroyed();

  const std::string &name() const { return m_name; }
  bool connected() { return m_state == ConnectState::kConnected; }
  const InetAddress &localAddress() { return m_localAddr; }
  const InetAddress &peerAddrress() { return m_peerAddr; }

  EventLoop *getLoop() const { return m_loop; }

  void setConnectionCallback(const ConnectionCallback &cb) { m_connectionCallback = cb; }
  void setMessageCallback(const MessageCallback &cb) { m_messageCallback = cb; }
  void setCloseCallback(const CloseCallback &cb) { m_closeCallback = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { m_writeCompleteCallback = cb; }
  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark) {
    m_highWaterMarkCallback = cb;
    m_highWaterMark = highWaterMark;
  }

 private:
  void setState(ConnectState state) { m_state = state; }

  // 注册给channel使用
  void handleRead(Timestamp recvTime);
  void handleWrite();  // 必然是outbuff向外写数据
  void handleClose();
  void handleError();

  // 重要的逻辑
  // 先尝试直接发送，如果无法全部发送，将数据缓存到buff中，等epoll通知可写时候，buff向外写
  void sendInLoop(const void *msg, size_t len);

  void shutdownInLoop();

 private:
  const std::string m_name;
  std::atomic<ConnectState> m_state;

  EventLoop *m_loop;
  std::unique_ptr<Socket> m_socket;
  std::unique_ptr<Channel> m_channel;

  const InetAddress m_localAddr;
  const InetAddress m_peerAddr;

  size_t m_highWaterMark;

  ConnectionCallback m_connectionCallback;
  CloseCallback m_closeCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  MessageCallback m_messageCallback;
  HighWaterMarkCallback m_highWaterMarkCallback;

  Buffer m_inputBuffer;
  Buffer m_outputBuffer;
};