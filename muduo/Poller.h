#pragma once
#include <unordered_map>
#include <vector>
#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop;
class Channel;

class Poller : noncopyable {
 public:
  using ChannelList = std::vector<Channel *>;

  Poller(EventLoop *loop);
  virtual ~Poller() = default;

  virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
  virtual void updateChannel(Channel *channel) = 0;
  virtual void removeChannel(Channel *channel) = 0;

  bool hasChannel(Channel *channel);

  // 通过该接口获取epoller
  static Poller *newDefaultPoller(EventLoop *loop);

 protected:
  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap m_channels;  // 一个poll管理多个channel
 private:
  EventLoop *m_owner_loop;
};