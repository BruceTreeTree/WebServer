/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-04 16:05:55
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/Epoll.h
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-03 19:02:03
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/Epoll.h
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"

class Epoll {
 public:
  /**
   * @brief Construct a new Epoll object
   * 创建epoll对象，
   * epoll_create(), size参数只要大于0就可以
   * epoll_create1(),
   * 有一个flag参数，flags为0，epoll_create1（）和删除了过时size参数的epoll_create（）相同。
   *  如果 EPOLL_CLOEXEC  会有不同  O_CLOEXEC   意思就是说 在fork的时候
   * 就直接关闭文件描述符
   *
   */
  Epoll();
  ~Epoll();
  void epoll_add(SP_Channel request, int timeout);
  void epoll_mod(SP_Channel request, int timeout);
  void epoll_del(SP_Channel request);

  /**
   * @brief epoll的核心功能，返回Channel活跃的数组
   *
   * @return std::vector<std::shared_ptr<Channel>>
   */
  std::vector<std::shared_ptr<Channel>> poll();
  std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
  void add_timer(std::shared_ptr<Channel> request_data, int timeout);
  int getEpollFd() { return epollFd_; }
  void handleExpired();

 private:
  static const int MAXFDS = 100000;
  int epollFd_;                      // 保存epoll 的fd
  std::vector<epoll_event> events_;  // 保存每个fd 监听到有  反应的对多的 events
  std::shared_ptr<Channel> fd2chan_[MAXFDS];   // 保存Channel对象
  std::shared_ptr<HttpData> fd2http_[MAXFDS];  // 保存Http对象
  TimerManager timerManager_;                  // 计时队列管理对象
};