#include "Channel.h"
#include <memory>
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : m_loop(loop),
      m_fd(fd),
      m_events(0),
      m_revents(0),
      m_index(-1),  // 未添加
      m_tied(false)

{}

void Channel::tie(const std::shared_ptr<void> &obj) {
  m_tie = obj;
  m_tied = true;
}

void Channel::handleEvent(Timestamp recvTime) {
  if (m_tied) {
    std::shared_ptr<void> obj = m_tie.lock();  // conn还没有被remove
    if (obj) {
      handleEventWithGuard(recvTime);
    }
  } else {
    handleEventWithGuard(recvTime);
  }
}

void Channel::handleEventWithGuard(Timestamp recvTime) {
  // LOG_DEBUG("%s handleEvent", __FILE_NAME__);

  // 当远程端关闭了连接时，会触发 EPOLLHUP
  if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)) {
    if (m_close_callback) {
      m_close_callback();
    }
  }

  if (m_revents & EPOLLERR) {
    if (m_error_callback) {
      m_error_callback();
    }
  }
  // EPOLLPRI 通知描述符上有紧急数据可读
  if (m_revents & (EPOLLIN | EPOLLPRI)) {
    if (m_read_callback) {
      m_read_callback(recvTime);
    }
  }

  if (m_revents & EPOLLOUT) {
    if (m_write_callback) {
      m_write_callback();
    }
  }
}

void Channel::remove() {
  // 调用loop的remove
  m_loop->removeChannel(this);
}
void Channel::update() {
  // 调用loop的update
  m_loop->updateChannel(this);
}