#pragma once
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace CurrentThread {
extern thread_local pid_t t_cacheTid;
void cacheTid();

inline pid_t tid() {
  // 分支预测优化
  // 预期 t_cachedTid == 0 的概率较低
  if (__builtin_expect(t_cacheTid == 0, 0)) {
    cacheTid();
  }
  return t_cacheTid;
}
}  // namespace CurrentThread