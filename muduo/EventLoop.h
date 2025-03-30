#pragma once

#include <sys/types.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "Channel.h"
#include "CurrentThread.h"
#include "Poller.h"
#include "noncopyable.h"

class EventLoop : noncopyable {
 public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  void runInloop(Functor cb);
  void queueInloop(Functor cb);

  void wakeup();

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  bool isInLoopThread() const { return m_threadId == CurrentThread::tid(); }

 private:
  void handleWakeupRead();
  void doPendingFuntors();  // 执行等待的任务

 private:
  using ChannelList = std::vector<Channel *>;

  const pid_t m_threadId;  // 当前loop所在线程ID
  int m_wakeupFd;
  std::unique_ptr<Channel> m_wakeupChannel;
  ChannelList m_activeChannels;

  std::unique_ptr<Poller> m_poller;

  std::mutex m_mutex;  // 保护m_pendingFuntors的操作
  std::vector<Functor> m_pendingFuntors;

  std::atomic<bool> m_quit;
  std::atomic<bool> m_callingPendingFunctors;
};