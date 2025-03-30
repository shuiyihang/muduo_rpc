#include "EventLoopThreadPool.h"
#include <cstdio>
#include "Channel.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *base_loop, const std::string &app_name)
    : m_base_loop(base_loop), m_app_name(app_name), m_numThreads(0), m_next(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
  for (int i = 0; i < m_numThreads; i++) {
    char buf[m_app_name.size() + 32];
    snprintf(buf, sizeof(buf), "%s_%d", m_app_name.c_str(), i);
    EventLoopThread *t = new EventLoopThread(cb, buf);
    m_threads.emplace_back(t);
    m_loops.emplace_back(t->startLoop());
  }

  if (m_numThreads == 0 && cb) {
    cb(m_base_loop);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  EventLoop *loop = m_base_loop;
  if (!m_loops.empty()) {
    loop = m_loops[m_next];
    m_next++;
    if (m_next >= m_loops.size()) {
      m_next = 0;
    }
  }
  return loop;
}
