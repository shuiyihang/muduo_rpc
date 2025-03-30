#include "TcpConnection.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <functional>
#include "Callbacks.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

using std::placeholders::_1;
using std::placeholders::_2;

static EventLoop *checkLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL("loop is nullptr");
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : m_loop(checkLoopNotNull(loop)),
      m_name(name),
      m_state(ConnectState::kConnecting),
      m_socket(new Socket(sockfd)),
      m_channel(new Channel(loop, sockfd)),
      m_localAddr(localAddr),
      m_peerAddr(peerAddr),
      m_highWaterMark(64 * 1024 * 1024) {
  m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  LOG_DEBUG("TcpConnection ctor[%s] at fd:%d", m_name.c_str(), sockfd);

  m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG("TcpConnection dtor[%s] at fd:%d state:%d", m_name.c_str(), m_channel->fd(), m_state.load());
}

void TcpConnection::send(const std::string &buf) {
  // TODO TcpConnection方法在哪里调用
  // conn被保存进map和设置回调参数，触发事件时调用
  if (m_loop->isInLoopThread()) {
    sendInLoop(buf.c_str(), buf.size());
  } else {
    m_loop->queueInloop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str()  // 可能有问题，buf如果已经没了
                                  ,
                                  buf.size()));
  }
}

void TcpConnection::sendInLoop(const void *msg, size_t len) {
  ssize_t nwrote = 0;
  ssize_t remaining = len;
  bool faultError = false;

  if (m_state == ConnectState::kDisconnected) {
    LOG_ERROR("disconnected,give up writing");
    return;
  }

  // 先看看能不能直接写
  if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0) {
    nwrote = ::write(m_socket->fd(), msg, len);

    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && m_writeCompleteCallback) {
        m_loop->queueInloop(std::bind(m_writeCompleteCallback, shared_from_this()));
      }
    } else {
      if (errno != EWOULDBLOCK) {
        LOG_ERROR("sendInLoop err %d", errno);
        // SIGPIPE  对端已经关闭连接
        // RESET
        if (errno == EPIPE || errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  if (!faultError && remaining > 0) {
    size_t oldDataLen = m_outputBuffer.readableBytes();
    if (oldDataLen + remaining >= m_highWaterMark && oldDataLen < m_highWaterMark && m_highWaterMarkCallback) {
      // 延迟发送的数据达到水位线，回调
      m_loop->queueInloop(std::bind(m_highWaterMarkCallback, shared_from_this(), oldDataLen + remaining));
    }

    m_outputBuffer.append(reinterpret_cast<const char *>(msg) + nwrote, remaining);
    // 开启写事件
    if (!m_channel->isWriting()) {
      m_channel->enableWriting();
    }
  }
}

// 执行在channel中的各种callback
void TcpConnection::handleRead(Timestamp recvTime) {
  int saveErrno = 0;

  size_t n = m_inputBuffer.readFd(m_channel->fd(), &saveErrno);
  if (n > 0) {
    m_messageCallback(shared_from_this(), &m_inputBuffer, recvTime);
  } else if (n == 0) {
    // 标识对端关闭了连接
    handleClose();
  } else {
    // 发生错误
    errno = saveErrno;
    LOG_ERROR("TcpConnection::handleRead err %d", errno);
    handleError();
  }
}
void TcpConnection::handleWrite() {
  // 将output Buffer中的数据写出去
  if (m_channel->isWriting()) {
    int saveErrno;
    ssize_t n = m_outputBuffer.writeFd(m_channel->fd(), &saveErrno);
    if (n > 0) {
      if (m_outputBuffer.readableBytes() == 0) {
        // 写完了
        m_channel->disableWriting();
        if (m_writeCompleteCallback) {
          m_loop->queueInloop(std::bind(m_writeCompleteCallback, shared_from_this()));
        }

        if (m_state == ConnectState::kDisconnecting) {
          // TODO 没有设置状态，只做了写关闭
          shutdownInLoop();
        }
      }
    } else {
      LOG_ERROR("TcpConnection::handleWrite");
    }
  } else {
    LOG_ERROR("TcpConnection fd:%d is down,no more writing", m_channel->fd());
  }
}
void TcpConnection::handleClose() {
  LOG_DEBUG("TcpConnection::handleClose");
  setState(ConnectState::kDisconnected);
  m_channel->disableAll();

  // 执行用户手动注册的 连接状态改变 事件
  m_connectionCallback(shared_from_this());

  // 执行TCP server 提供的removeConnection
  m_closeCallback(shared_from_this());
}
void TcpConnection::handleError() {
  int optval;
  socklen_t optlen = sizeof(optval);

  int err;
  if (getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    err = errno;
  } else {
    err = optval;
  }
  LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", m_name.c_str(), err);
}

void TcpConnection::shutdown() {
  if (m_state == ConnectState::kConnected) {
    setState(ConnectState::kDisconnecting);  // 等待发送完毕，设置为kDisconnected
    m_loop->runInloop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  // 如果未完成写，就延迟到output buff 写事件回调中
  if (!m_channel->isWriting()) {
    m_socket->shutdownWrite();
  }
}

// 什么时间调用--accept channel 触发read事件之后，建立connection分发完成之后
void TcpConnection::connectEstablished() {
  setState(ConnectState::kConnected);
  m_channel->tie(shared_from_this());  // 绑定了一个连接，这个连接存放到server map[str,conn]
  m_channel->enableReading();

  // 执行新连接建立回调
  m_connectionCallback(shared_from_this());
}
void TcpConnection::connectDestroyed() {
  if (m_state == ConnectState::kConnected) {
    setState(ConnectState::kDisconnected);
    m_channel->disableAll();
    // 连接断开也是这个
    m_connectionCallback(shared_from_this());
  }
  m_channel->remove();
}