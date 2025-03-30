#include "Thread.h"
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "CurrentThread.h"

std::atomic<int> Thread::m_numCreated(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : m_started(false), m_joined(false), m_tid(0), m_func(std::move(func)), m_name(name) {
  setDefaultName();
}

Thread::~Thread() {
  if (m_started && !m_joined) {
    // 分离的线程在完成后，系统会自动回收它的资源，无需显式调用join
    m_thread->detach();
  }
}

void Thread::start() {
  m_started = true;

  std::mutex mtx;
  std::condition_variable cond;

  m_thread = std::unique_ptr<std::thread>(new std::thread([this, &cond]() {
    // this只能值捕获
    this->m_tid = CurrentThread::tid();
    cond.notify_one();

    this->m_func();  // 开启的新线程和Thread共存亡
  }));

  std::unique_lock<std::mutex> lock(mtx);
  cond.wait(lock);
}
void Thread::join() {
  m_joined = true;
  m_thread->join();
}

void Thread::setDefaultName() {
  int num = numCreated();
  if (m_name.empty()) {
    char buf[32] = {0};
    snprintf(buf, 32, "Thread_%d", num);
    m_name = buf;
  }
}