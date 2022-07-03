/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-20 10:06:28
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoop.h
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-13 14:36:39
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoop.h
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-04 16:31:05
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoop.h
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-03 20:08:25
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoop.h
 * @===================
 */

#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "Channel.h"
#include "Epoll.h"
#include "Util.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"
#include "base/Thread.h"
using namespace std;

class EventLoop {
 public:
  typedef std::function<void()> Functor;
  EventLoop();
  ~EventLoop();
  void loop();
  void quit();
  void runInLoop(Functor&& cb);
  void queueInLoop(Functor&& cb);
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  /**
   * @brief
   * 断言 这个looop是否在 loopPOLL中，实际上是 是否阻塞了
   *
   */
  void assertInLoopThread() { assert(isInLoopThread()); }

  /**
   * 拥有一个epoll对象，一个channel对象， channel 不closefd
   * poller 并不拥有 channel， 所以在其中添加的channel
   * 在析构之前需要，removeFromPoller
   */
  void shutdown(shared_ptr<Channel> channel) { shutDownWR(channel->getFd()); }

  void removeFromPoller(shared_ptr<Channel> channel) {
    // shutDownWR(channel->getFd());
    poller_->epoll_del(channel);
  }
  void updatePoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_mod(channel, timeout);
  }
  void addToPoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_add(channel, timeout);
  }

 private:
  // 声明顺序 wakeupFd_ > pwakeupChannel_
  bool looping_;
  shared_ptr<Epoll> poller_;  // epoll 对象，用于接收多个epoll事件
  int wakeupFd_;
  bool quit_;
  bool eventHandling_;

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_;

  bool callingPendingFunctors_;
  const pid_t threadId_;
  shared_ptr<Channel> pwakeupChannel_;

  void wakeup();
  void handleRead();
  void doPendingFunctors();
  void handleConn();
};
