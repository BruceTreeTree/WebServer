/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-19 19:24:52
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/EventLoopThreadPool.h
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include <vector>

#include "EventLoopThread.h"
#include "base/Logging.h"
#include "base/noncopyable.h"

class EventLoopThreadPool : noncopyable {
 public:
  EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

  ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }
  void start();
  EventLoop* getNextLoop();

 private:
  EventLoop* baseLoop_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::shared_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};