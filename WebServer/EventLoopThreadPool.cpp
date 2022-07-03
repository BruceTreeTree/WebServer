/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-19 21:12:15
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoopThreadPool.cpp
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-03 20:29:05
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoopThreadPool.cpp
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0) {
  if (numThreads_ <= 0) {
    LOG << "numThreads_ <= 0";
    abort();
  }
}

void EventLoopThreadPool::start() {
  baseLoop_->assertInLoopThread();  // 主线程中
  started_ = true;
  for (int i = 0; i < numThreads_; ++i) {
    // 新建线程加入 线程池中
    std::shared_ptr<EventLoopThread> t(new EventLoopThread());
    threads_.push_back(t);
    loops_.push_back(t->startLoop());
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  // 如果 线程池中没有线程，那么就用主线程中  eventloopthread  成为单线程循环
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;
  if (!loops_.empty()) {
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;
}