#pragma once

#include <sys/epoll.h>
#include <vector>
#include "Channel.h"
#include "Poller.h"
#include "Timestamp.h"

class EventLoop;

class EPollPoller : public Poller {
 public:
  EPollPoller(EventLoop *loop);
  ~EPollPoller() override;

  virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
  virtual void updateChannel(Channel *channel) override;
  virtual void removeChannel(Channel *channel) override;

 private:
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
  void update(int op, Channel *channel);

 private:
  static const int kInitEventListSize = 16;
  using EventList = std::vector<epoll_event>;

  int m_epoll_fd;
  EventList m_events;
};