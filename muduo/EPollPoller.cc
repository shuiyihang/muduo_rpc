#include "EPollPoller.h"
#include <asm-generic/errno-base.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

// channel在poller中的状态
const int kNew = -1;     // 未添加到poll中
const int kAdded = 1;    // 已添加
const int kDeleted = 2;  // 从poll中删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), m_epoll_fd(epoll_create1(EPOLL_CLOEXEC)), m_events(kInitEventListSize) {
  if (m_epoll_fd < 0) {
    LOG_FATAL("epoll fd create error:%d", errno);
  }
}

EPollPoller::~EPollPoller() { close(m_epoll_fd); }

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  int numEvents = epoll_wait(m_epoll_fd, &*m_events.begin(), m_events.size(), timeoutMs);
  int saveErrno = errno;  // 保存可能的错误

  if (numEvents > 0) {
    LOG_DEBUG("%d events happend", numEvents);
    fillActiveChannels(numEvents, activeChannels);
    // 1. 自动扩容
    // 2. 使用resize，而不是reserve，因为reserve不会改变size大小
    if (numEvents == m_events.size()) {
      m_events.resize(numEvents * 2);  // 精华
    }
  } else if (numEvents == 0) {
    LOG_DEBUG("%s timeout", __FUNCTION__);
  } else {
    // epoll_wait() returns -1 and errno is set appropriately.
    if (saveErrno != EINTR) {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll err %d", errno);
    }
  }
  return Timestamp::now();
}
void EPollPoller::updateChannel(Channel *channel) {
  const int state = channel->index();

  LOG_DEBUG("func=%s fd=%d events=%d index=%d", __FUNCTION__, channel->fd(), channel->events(), state);
  if (state == kNew || state == kDeleted) {
    if (state == kNew) {
      int fd = channel->fd();
      m_channels[fd] = channel;
    }

    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);

  } else {
    // channel已经被添加到了epoll上
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}
void EPollPoller::removeChannel(Channel *channel) {
  int fd = channel->fd();
  m_channels.erase(fd);  // 擦除键

  int state = channel->index();
  if (state == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  // 设置成new，因为已经擦除key
  channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
  for (int i = 0; i < numEvents; i++) {
    auto channel = static_cast<Channel *>(m_events[i].data.ptr);
    channel->set_revents(m_events[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::update(int op, Channel *channel) {
  epoll_event event;
  bzero(&event, sizeof(epoll_event));

  int fd = channel->fd();
  // 感兴趣的事件
  event.events = channel->events();
  event.data.ptr = channel;  // 存入ptr

  if (epoll_ctl(m_epoll_fd, op, fd, &event) < 0) {
    if (op == EPOLL_CTL_DEL) {
      LOG_ERROR("epoll ctl del error");
    } else {
      LOG_FATAL("epoll ctl add/mod error:%d", errno);
    }
  }
}