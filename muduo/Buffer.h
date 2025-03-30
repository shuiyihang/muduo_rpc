#pragma once

#include <cstddef>
#include <string>
#include <vector>
class Buffer {
 public:
  const static size_t kCheapPrepend = 8;
  const static size_t kInitialSize = 1024;

  explicit Buffer(size_t initSize = kInitialSize)
      : m_buffer(kCheapPrepend + initSize), m_writerIndex(kCheapPrepend), m_readerIndex(kCheapPrepend) {}

  size_t readableBytes() const;
  size_t writeableBytes() const;

  // 前面已经不可写的部分
  size_t prependableBytes() const;
  // 可读数据的起始地址
  const char *peek() const;

  // 更新read游标
  void retrieve(size_t len);
  void retrieveAll();

  std::string retrieveAsString(size_t len);
  std::string retrieveAllAsString();

  char *beginWrite();

  // 重要
  void ensureWriteableBytes(size_t len);
  void append(const char *src, size_t len);

  size_t readFd(int fd, int *saveErrno);
  size_t writeFd(int fd, int *saveErrno);

 private:
  // vector数组的首元素地址
  char *begin();
  const char *begin() const;

  void makeSpace(size_t len);

 private:
  std::vector<char> m_buffer;
  size_t m_readerIndex;
  size_t m_writerIndex;
};