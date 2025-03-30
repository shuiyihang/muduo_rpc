
#include <cstdlib>
#include "EPollPoller.h"
#include "Poller.h"

// 通过该接口获取epoller
Poller *Poller::newDefaultPoller(EventLoop *loop) {
  if (::getenv("USE_POLL")) {
    return nullptr;
  } else {
    return new EPollPoller(loop);
  }
}