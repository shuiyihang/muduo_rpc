#pragma once

#include <functional>
#include "Channel.h"
#include "InetAddress.h"
#include "Socket.h"
#include "noncopyable.h"

class EventLoop;

class Acceptor : noncopyable {
 public:
  using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

  explicit Acceptor(EventLoop *loop, const InetAddress &listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback &cb) { m_newConnectionCallback = cb; }

  bool listenning() const { return m_listenning; }
  void listen();

 private:
  // accept channel上有数据时，表示有连接，调用accept获取confd和peerAddr
  void handleRead();

 private:
  EventLoop *m_base_loop;
  Socket m_acceptSocket;
  Channel m_acceptChannel;

  NewConnectionCallback m_newConnectionCallback;
  bool m_listenning;
};