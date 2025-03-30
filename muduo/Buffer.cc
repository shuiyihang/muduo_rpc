#include "Buffer.h"
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <string>

size_t Buffer::readableBytes() const { return m_writerIndex - m_readerIndex; }
size_t Buffer::writeableBytes() const { return m_buffer.size() - m_writerIndex; }

size_t Buffer::prependableBytes() const { return m_readerIndex; }
// 可读数据的起始地址
const char *Buffer::peek() const {
  // const方法中只能调用const方法
  return this->begin() + m_readerIndex;
}

// 更新read游标
void Buffer::retrieve(size_t len) {
  if (len < readableBytes()) {
    m_readerIndex += len;
  } else {
    retrieveAll();
  }
}
void Buffer::retrieveAll() { m_readerIndex = m_writerIndex = kCheapPrepend; }

std::string Buffer::retrieveAsString(size_t len) {
  // string的这种截取用法
  // std::string s6("C-style string", 7); --> C-style
  std::string result(peek(), len);
  retrieve(len);
  return result;
}
std::string Buffer::retrieveAllAsString() { return retrieveAsString(readableBytes()); }

char *Buffer::beginWrite() { return begin() + m_writerIndex; }

// 确保有len空间来写
void Buffer::ensureWriteableBytes(size_t len) {
  if (writeableBytes() < len) {
    makeSpace(len);
  }
}
void Buffer::append(const char *src, size_t len) {
  ensureWriteableBytes(len);
  std::copy(src, src + len, beginWrite());
  m_writerIndex += len;
}

char *Buffer::begin() { return &*m_buffer.begin(); }

const char *Buffer::begin() const { return &*m_buffer.begin(); }

void Buffer::makeSpace(size_t len) {
  if (prependableBytes() - kCheapPrepend + writeableBytes() < len) {
    // 调整为刚好可以放下
    m_buffer.resize(m_writerIndex + len);
  } else {
    // 挪动一下数据
    size_t readable = readableBytes();
    std::copy(begin() + m_readerIndex, begin() + m_writerIndex, begin() + kCheapPrepend);
    m_readerIndex = kCheapPrepend;
    m_writerIndex = m_readerIndex + readable;
  }
}

size_t Buffer::readFd(int fd, int *saveErrno) {
  // 64M的栈上空间
  char extraBuf[65536] = {0};

  struct iovec vec[2];
  const size_t writeable = writeableBytes();

  vec[0].iov_base = beginWrite();
  vec[0].iov_len = writeable;

  vec[1].iov_base = extraBuf;
  vec[1].iov_len = sizeof(extraBuf);

  const int iovcnt = (writeable < sizeof(extraBuf)) ? 2 : 1;

  size_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) {
    // 出现了错误
    *saveErrno = errno;
  } else if (n <= writeable) {
    // 不需要使用额外栈空间读
    m_writerIndex += n;
  } else {
    // 妙极
    append(extraBuf, n - writeable);
  }

  return n;
}
size_t Buffer::writeFd(int fd, int *saveErrno) {
  size_t n = ::write(fd, peek(), readableBytes());
  if (n < 0) {
    *saveErrno = errno;
  } else {
    retrieve(n);
  }

  return n;
}