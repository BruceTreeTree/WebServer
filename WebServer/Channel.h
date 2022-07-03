
#pragma once
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Timer.h"

class EventLoop;
class HttpData;

class Channel {
 private:
  typedef std::function<void()> CallBack;  // 回调函数定义
  EventLoop *loop_;                        // 事件循环
  int fd_;                                 // 客户端文件描述符
  __uint32_t events_;                      // 表示服务器监听事件
  __uint32_t revents_;                     // 客户端返回的事件
  __uint32_t lastEvents_;                  // 最后一个事件

  std::weak_ptr<HttpData>
      holder_;  // 方便找到上层持有该Channel的对象，指向该holder的持有者
                // 定义一个指向HttpData指针对象，只能指向shared-ptr

 private:
  CallBack readHandler_;   // 用于读取数据的时候使用的回调函数
  CallBack writeHandler_;  // 用于回复数据的时候使用的回调函数
  CallBack errorHandler_;  // 当出现错误的时候使用的回调函数
  CallBack connHandler_;   // 用于处理连接描述符的回调函数

 public:
  /**
   * @brief Construct a new Channel object
   *
   * @param loop
   */
  Channel(EventLoop *loop);
  /**
   * @brief Construct a new Channel object
   *
   * @param loop
   */
  Channel(EventLoop *loop, int fd);
  /**
   * @brief Destroy the Channel object
   *
   */
  ~Channel();
  /**
   * @brief Get the Fd object
   *
   * @return int
   */
  int getFd();
  /**
   * @brief Set the Fd object
   *
   * @param fd
   */
  void setFd(int fd);
  /**
   * @brief Set the Holder object
   * 记录该Channel上层的持有者信息，使用weakptr 避免循环引用引起的内存泄漏
   * @param holder
   */
  void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
  /**
   * @brief Get the Holder object
   *
   * @return std::shared_ptr<HttpData>
   */
  std::shared_ptr<HttpData> getHolder() {
    std::shared_ptr<HttpData> ret(holder_.lock());
    return ret;
  }
  /**
   * @brief Set the Read Handler object
   *
   * @param readHandler
   */
  void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
  /**
   * @brief Set the Write Handler object
   *
   * @param writeHandler
   */
  void setWriteHandler(CallBack &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  /**
   * @brief Set the Error Handler object
   *
   * @param errorHandler
   */
  void setErrorHandler(CallBack &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  /**
   * @brief Set the Conn Handler object
   *
   * @param connHandler
   */
  void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

  /**
   * @brief
   * 处理事务主要函数
   * 1、回复事件先置空
   * 2、EPOLLRDHUP 表示读关闭    EPOLLHUP 表示读写都关闭， 没有写事件
   * 3、客户端出错，表示错误
   * 4、表示 对方有信息过来，或者  对方
   * 读关闭了，或者EPOLLPRI表示对方有紧急信息需要读取
   * 5、表示该文件描述符可写
   * 6、 最后处理handleConn？
   */
  void handleEvents() {
    events_ = 0;  // 监听事件因为是oneshot，自己这边需要更新
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
      events_ = 0;
      return;
    }
    if (revents_ & EPOLLERR) {
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      handleRead();
    }
    if (revents_ & EPOLLOUT) {
      handleWrite();
    }
    // 走到这里的话，最后肯定是要处理这个地方的
    // 先处理读，后处理写，最后处理连接重置问题
    handleConn();
  }
  /**
   * @brief
   * 如果存在处理读事务的 回调函数，则处理
   */
  void handleRead();
  /**
   * @brief
   * 如果存在处理写事务的 回调函数，则处理
   */
  void handleWrite();
  /**
   * @brief
   * 表示服务器出现现错误，
   * 并且发送出现错误的HTTP报文
   * @param fd 文件描述符
   * @param err_num 错误号
   * @param short_msg 概要信息
   */
  void handleError(int fd, int err_num, std::string short_msg);
  /**
   * @brief
   * 处理连接信息
   *
   */
  void handleConn();
  /**
   * @brief Set the Revents object
   *
   * @param ev
   */
  void setRevents(__uint32_t ev) { revents_ = ev; }
  /**
   * @brief Set the Events object
   *
   * @param ev
   */
  void setEvents(__uint32_t ev) { events_ = ev; }
  __uint32_t &getEvents() { return events_; }

  /**
   * @brief 判断是否是最后一个事件，
   *
   * @return true
   * @return false
   */
  bool EqualAndUpdateLastEvents() {
    bool ret = (lastEvents_ == events_);
    lastEvents_ = events_;
    return ret;
  }

  __uint32_t getLastEvents() { return lastEvents_; }
};

typedef std::shared_ptr<Channel> SP_Channel;  // 指向Channel的智能指针

/**
 * @brief 总结Channel类的作用，对一个文件描述符事务处理功能的封装
 *
 * 接收一个  enentloop对象，表示epoll对象
 * fd置为0
 *
 * 或者一个loop对象，外加一个fd
 *
 */