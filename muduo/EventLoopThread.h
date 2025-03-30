#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include "Thread.h"
#include "noncopyable.h"

class EventLoop;

class EventLoopThread : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
  ~EventLoopThread();

  // 返回自身的 m_loop
  EventLoop *startLoop();

 private:
  // 传递给Thread的执行体
  void ThreadFunc();

 private:
  EventLoop *m_loop;
  Thread m_thread;

  std::mutex m_mutex;
  std::condition_variable m_cond;

  ThreadInitCallback m_threadInitCallback;
};