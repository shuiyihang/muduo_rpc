#pragma once

#include <functional>
#include <memory>
#include <utility>
#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop;

class Channel : noncopyable {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;  // 读事件回调多一个时间戳?

  Channel(EventLoop *loop, int fd);
  ~Channel() = default;

  void handleEvent(Timestamp recvTime);

  void setReadCallback(ReadEventCallback cb) { m_read_callback = std::move(cb); }
  void setWriteCallback(EventCallback cb) { m_write_callback = std::move(cb); }
  void setCloseCallback(EventCallback cb) { m_close_callback = std::move(cb); }
  void setErrorCallback(EventCallback cb) { m_error_callback = std::move(cb); }

  // 设置fd状态
  void enableReading() {
    m_events |= kReadEvent;
    update();
  }
  void disableReading() {
    m_events &= ~kReadEvent;
    update();
  }

  void enableWriting() {
    m_events |= kWriteEvent;
    update();
  }

  void disableWriting() {
    m_events &= ~kWriteEvent;
    update();
  }

  void disableAll() {
    m_events = kNoneEvent;
    update();
  }

  bool isNoneEvent() { return m_events == kNoneEvent; }

  bool isWriting() { return m_events & kWriteEvent; }

  bool isReading() { return m_events & kReadEvent; }

  int index() { return m_index; }
  void set_index(int index) { m_index = index; }

  EventLoop *ownerLoop() { return m_loop; }

  void remove();

  void tie(const std::shared_ptr<void> &obj);

  int fd() const { return m_fd; }
  int events() const { return m_events; }
  void set_revents(int revt) { m_revents = revt; }

 private:
  void update();

  void handleEventWithGuard(Timestamp recvTime);

 private:
  EventLoop *m_loop;  // channel挂在哪一个事件循环上

  const int m_fd;
  int m_events;  // 监听的事件

  int m_revents;  // 发生的事件

  int m_index;  // channel状态

  std::weak_ptr<void> m_tie;
  bool m_tied;

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  ReadEventCallback m_read_callback;
  EventCallback m_write_callback;
  EventCallback m_close_callback;
  EventCallback m_error_callback;
};