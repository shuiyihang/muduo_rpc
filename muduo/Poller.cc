#include "Poller.h"
#include "Channel.h"
#include "EventLoop.h"

bool Poller::hasChannel(Channel *channel) {
  auto itr = m_channels.find(channel->fd());
  return itr != m_channels.end() && itr->second == channel;  // 有默认的比较操作==
}

Poller::Poller(EventLoop *loop) : m_owner_loop(loop) {}