#include "CurrentThread.h"

namespace CurrentThread {
// 定义一个线程局部变量，用于缓存线程 ID
// 它的值在一个线程内共享，但在不同线程间是独立的，互不影响。
thread_local pid_t t_cacheTid = 0;

void cacheTid() {
  if (t_cacheTid == 0) {
    t_cacheTid = static_cast<pid_t>(syscall(SYS_gettid));
  }
}
}  // namespace CurrentThread