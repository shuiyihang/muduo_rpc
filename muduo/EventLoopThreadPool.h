#pragma once

#include <memory>
#include <string>
#include <vector>
#include "EventLoopThread.h"
#include "noncopyable.h"

class EventLoop;

class EventLoopThreadPool : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *base_loop, const std::string &app_name);
  ~EventLoopThreadPool();

  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  EventLoop *getNextLoop();
  void setThreadNum(int numThreads) { m_numThreads = numThreads; }

 private:
  EventLoop *m_base_loop;

  int m_numThreads;
  int m_next;  // 轮询索引

  std::vector<std::unique_ptr<EventLoopThread>> m_threads;
  std::vector<EventLoop *> m_loops;
  std::string m_app_name;
};