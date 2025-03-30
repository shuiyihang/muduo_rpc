#include "EventLoopThread.h"
#include <functional>
#include <mutex>
#include "Channel.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : m_loop(nullptr),
      m_thread(std::bind(&EventLoopThread::ThreadFunc, this), name),
      m_threadInitCallback(cb)

{}
EventLoopThread::~EventLoopThread() {
  if (m_loop != nullptr) {
    m_loop->quit();  // 退出执行体
    m_thread.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  m_thread.start();  // 启动新线程，调用ThreadFunc

  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_loop == nullptr) {
      m_cond.wait(lock);  // wait之后会自动释放锁，结束等待之后会重新获取锁
    }
    loop = m_loop;
  }

  return loop;
}

// 最底层Thread调用的执行体
void EventLoopThread::ThreadFunc() {
  EventLoop loop;

  if (m_threadInitCallback) {
    m_threadInitCallback(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = &loop;
    m_cond.notify_one();
  }
  // 进入poll循环监听
  m_loop->loop();

  std::unique_lock<std::mutex> lock(m_mutex);
  m_loop = nullptr;  // 这个m_loop是可能在别的线程调用的，需要mutex保护
}