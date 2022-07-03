/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-03 20:02:50
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/Server.h
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>

#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class Server {
 public:
  /**
   * @brief Construct a new Server object
   * 初始化各种信息
   * 1、注册SIGPIPE信号处理
   * 2、将监听描述符设置为非阻塞  非阻塞监听
   * @param loop
   * @param threadNum
   * @param port
   */
  Server(EventLoop *loop, int threadNum, int port);
  ~Server() {}
  EventLoop *getLoop() const { return loop_; }
  void start();
  void handNewConn();
  void handThisConn() { loop_->updatePoller(acceptChannel_); }

 private:
  EventLoop *loop_;
  int threadNum_;
  std::unique_ptr<EventLoopThreadPool>
      eventLoopThreadPool_;  // 独占指针，拥有对那块内存的所有的权限，一旦不指，则RAII释放
  bool started_;
  std::shared_ptr<Channel> acceptChannel_;  // 表示监听的文件描述符
  int port_;                                // Server的 端口
  int listenFd_;                            // 监听的FD
  static const int MAXFDS = 100000;         // 最大可以监听的fd个数
};