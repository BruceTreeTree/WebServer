/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-03 16:50:39
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/Server.cpp
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>

#include "Util.h"
#include "base/Logging.h"

Server::Server(EventLoop *loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
      started_(false),
      acceptChannel_(new Channel(loop_)),
      port_(port),
      listenFd_(socket_bind_listen(port_)) {
  acceptChannel_->setFd(listenFd_);
  handle_for_sigpipe();
  // 非阻塞监听连接TCP，，因为需要和ET模式来配合，不然会出问题
  if (setSocketNonBlocking(listenFd_) < 0) {
    perror("set socket non block failed");
    abort();
  }
}

void Server::start() {
  eventLoopThreadPool_->start();
  // acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);?????
  // 为啥不需要oneshot
  // oneshot实在多线程的时候用的，再触发该fd的时候，就开始从epoll重视喊出该fd，然后就不会再出发了，多线程的时候用
  // listenFd不需要oneshot
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
  acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
  loop_->addToPoller(acceptChannel_, 0);  // 时间0  ，持续阻塞
  started_ = true;
}

void Server::handNewConn() {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;

  // 由于fd被设置为了非阻塞的，所以会持续循环，直到  读取不到了，跳出循环
  /**
   * @brief
   * 从 loop线程池中选取，一个空闲线程，使用这个loop线程监听这个fd的 读写事件
   *
   */
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    EventLoop *loop = eventLoopThreadPool_->getNextLoop();
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
        << ntohs(client_addr.sin_port);
    // cout << "new connection" << endl;
    // cout << inet_ntoa(client_addr.sin_addr) << endl;
    // cout << ntohs(client_addr.sin_port) << endl;
    /*
    // TCP的保活机制默认是关闭的
    int optval = 0;
    socklen_t len_optval = 4;
    getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
    cout << "optval ==" << optval << endl;
    */
    // 限制服务器的最大并发连接数
    if (accept_fd >= MAXFDS) {
      close(accept_fd);
      continue;
    }
    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    /**
     * @brief Set the Socket Nodelay object
     * 使用拥塞避免算法
     */
    setSocketNodelay(accept_fd);
    // setSocketNoLinger(accept_fd);

    // 收到Http连接
    shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
    req_info->getChannel()->setHolder(
        req_info);  // 循环引用使用 弱智能指针 ？？
    // 这里需要把Channel的监听事件信息，以及将描述符添加到 监听的 epoll中
    loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    // 拥有权 转移到了 epoll中保存，delete的时候 就reset 没然就 完美析构
  }
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);  // 不太需要的，确定下一下也好
}