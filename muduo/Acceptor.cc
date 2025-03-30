#include "Acceptor.h"
#include <asm-generic/errno-base.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <functional>
#include "InetAddress.h"
#include "Logger.h"

static int createSocketFd() {
  int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (fd == -1) {
    LOG_FATAL("createSocketFd failed %d", errno);
  }
  return fd;
}
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : m_base_loop(loop),
      m_acceptSocket(createSocketFd()),
      m_acceptChannel(loop, m_acceptSocket.fd()),
      m_listenning(false) {
  // bind
  m_acceptSocket.bindAddress(listenAddr);

  m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor() {
  m_acceptChannel.disableAll();

  // 从poller中删除
  m_acceptChannel.remove();
}
void Acceptor::listen() {
  m_listenning = true;

  // 设置sockfd listen
  m_acceptSocket.listen();
  // 加入poller
  m_acceptChannel.enableReading();
}

void Acceptor::handleRead() {
  InetAddress peerAddr;
  int confd = m_acceptSocket.accept(&peerAddr);
  if (confd >= 0) {
    if (m_newConnectionCallback) {
      m_newConnectionCallback(confd, peerAddr);
    } else {
      ::close(confd);
    }
  } else {
    LOG_ERROR("Acceptor::handleRead accept err %d", errno);
    if (errno == EMFILE) {
      LOG_ERROR("sock fd reached limit");
    }
  }
}