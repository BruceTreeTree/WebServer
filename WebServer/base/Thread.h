/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-19 20:15:40
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/base/Thread.h
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

#include "CountDownLatch.h"
#include "noncopyable.h"

class Thread : noncopyable {
 public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(const ThreadFunc&, const std::string& name = std::string());
  ~Thread();
  void start();  // 使用CountDown 对需要运行的线程函数进行计数，当数为0
                 // 的时候表示 线程函数正在执行，可以返回了，否则会出错？
  int join();
  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return name_; }

 private:
  void setDefaultName();
  bool started_;
  bool joined_;
  pthread_t pthreadId_;  // 一个线程标识的结构体，包含pid
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  CountDownLatch latch_;
};