#include "EventLoop.h"
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>
#include "Channel.h"
#include "CurrentThread.h"
#include "EPollPoller.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

const int kPollTimeMs = 10000;

thread_local EventLoop *loopInThisThread_ptr = nullptr;

static int createEventfd() {
  int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (fd < 0) {
    LOG_FATAL("create wakeup fd error %d", errno);
  }
  return fd;
}

EventLoop::EventLoop()
    : m_quit(false),
      m_callingPendingFunctors(false),
      m_threadId(CurrentThread::tid()),
      m_poller(EPollPoller::newDefaultPoller(this)),
      m_wakeupFd(createEventfd()),
      m_wakeupChannel(new Channel(this, m_wakeupFd)) {
  LOG_DEBUG("EventLoop %p created in thread %d", this, m_threadId);
  if (loopInThisThread_ptr) {
    LOG_FATAL("one thread should one loop");
  } else {
    loopInThisThread_ptr = this;
  }

  m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleWakeupRead, this));
  m_wakeupChannel->enableReading();  // 不使用 m_poller->updateChannel(m_wakeupChannel.get())
}
EventLoop::~EventLoop() {
  m_wakeupChannel->disableAll();
  m_wakeupChannel->remove();  // 从poller中移除channel
  close(m_wakeupFd);
  loopInThisThread_ptr = nullptr;
}

void EventLoop::loop() {
  while (!m_quit) {
    m_activeChannels.clear();

    Timestamp recvTime = m_poller->poll(kPollTimeMs, &m_activeChannels);

    for (auto &channel : m_activeChannels) {
      channel->handleEvent(recvTime);
    }

    doPendingFuntors();
  }
}

void EventLoop::quit() {
  m_quit = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::doPendingFuntors() {
  std::vector<Functor> pendingFun;
  m_callingPendingFunctors = true;  // 原子操作
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_pendingFuntors.swap(pendingFun);  // 妙处！
  }
  for (const auto &fun : pendingFun) {
    fun();
  }
  m_callingPendingFunctors = false;
}

void EventLoop::runInloop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInloop(cb);
  }
}
void EventLoop::queueInloop(Functor cb) {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    // 将任务加入待处理vector中
    m_pendingFuntors.emplace_back(cb);
  }

  // m_callingPendingFunctors是因为，m_pendingFuntors已经被swap取走了
  if (!isInLoopThread() || m_callingPendingFunctors) {
    wakeup();
  }
}

void EventLoop::handleWakeupRead() {
  uint64_t one;
  ssize_t n = read(m_wakeupFd, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR("EventLoop::handleWakeupRead() read %lu bytes instead of 8", n);
  }
}
void EventLoop::wakeup() {
  // 唤醒就是给poll随便发点东西，让poll不阻塞向下执行
  uint64_t one = 1;
  size_t n = write(m_wakeupFd, &one, sizeof(uint64_t));
  if (n != sizeof(uint64_t)) {
    LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8", n);
  }
}

void EventLoop::updateChannel(Channel *channel) { m_poller->updateChannel(channel); }
void EventLoop::removeChannel(Channel *channel) { m_poller->removeChannel(channel); }
bool EventLoop::hasChannel(Channel *channel) { return m_poller->hasChannel(channel); }