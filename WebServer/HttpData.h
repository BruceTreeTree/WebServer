// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <sys/epoll.h>
#include <unistd.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "Timer.h"

class EventLoop;
class TimerNode;
class Channel;

enum ProcessState {
  STATE_PARSE_URI = 1,
  STATE_PARSE_HEADERS,
  STATE_RECV_BODY,
  STATE_ANALYSIS,
  STATE_FINISH
};

enum URIState {
  PARSE_URI_AGAIN = 1,
  PARSE_URI_ERROR,
  PARSE_URI_SUCCESS,
};

enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ParseState {
  H_START = 0,
  H_KEY,
  H_COLON,
  H_SPACES_AFTER_COLON,
  H_VALUE,
  H_CR,
  H_LF,
  H_END_CR,
  H_END_LF
};

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

enum HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum HttpVersion { HTTP_10 = 1, HTTP_11 };

/***
 * @description:
 * 用于描述http协议中数据的类型
 * @param {*}
 * @return {*}
 */
class MimeType {
 private:
  // ? 为什么设置为 静态的呢？
  static void init();
  static std::unordered_map<std::string, std::string> mime;
  MimeType();
  MimeType(const MimeType &m);

 public:
  static std::string getMime(const std::string &suffix);

 private:
  static pthread_once_t once_control;
};

/**
 * @brief
 * enable_shared_from_this
 * 说明这个类的对象会交给一个智能指针管理。
 * 且这个类的成员函数里，需要把当前对象，作为参数，传递给其他函数，需要一个指向自身的shared_ptr
 *
 *
 */
class HttpData : public std::enable_shared_from_this<HttpData> {
 public:
  HttpData(EventLoop *loop, int connfd);
  ~HttpData() { close(fd_); }
  void reset();
  void seperateTimer();
  /**
   * @brief
   * TimeNode对象中封装了HttpData对象， 使用weak_ptr
   *
   * 把此TimeNode对象 ，同时变为HttpData对象的成员对象，使得可以相互访问
   * @param mtimer
   */
  void linkTimer(std::shared_ptr<TimerNode> mtimer) {
    // shared_ptr重载了bool, 但weak_ptr没有
    timer_ = mtimer;
  }

  /**
   * @brief Get the Channel object
   * 有管道的意思，使用文件描述符进行通信的对象，相当于对文件描述符的封装
   * @return std::shared_ptr<Channel>
   */
  std::shared_ptr<Channel> getChannel() { return channel_; }

  /**
   * @brief Get the Loop object
   * ! 不知道啥意思
   * @return EventLoop*
   */
  EventLoop *getLoop() { return loop_; }
  /**
   * @brief 主要功能是从epoll中删除文件描述符 Channl
   *
   */
  void handleClose();
  /**
   * @brief 设置新的监听事件
   * 1、首先讲监听描述符，设置为默认的 监听读， 监听边沿触发，oneshot
   * 2、然后注册新的文件描述符
   */
  void newEvent();

 private:
  EventLoop *loop_;  // 这里为啥就不用 智能指针了呢   是不是忘记了
  std::shared_ptr<Channel> channel_;  // 指向文件描述符的智能指针
  int fd_;                            // 客户端文件描述符
  std::string inBuffer_;  // 输入缓冲区一个客户端一个，用于从fd中读入数据
  std::string outBuffer_;  // 输出缓冲区
  bool error_;  // 对象中是否出错已放弃使用代替使用connectionState_
  ConnectionState connectionState_;  // 客户端连接状态
  HttpMethod method_;                // 表示请求的方法 get  post
  HttpVersion HTTPVersion_;          // http版本号
  std::string fileName_;
  std::string path_;
  int nowReadPos_;  // 表示在inbuff中  已经读取完成的 位置指针，
  ProcessState state_;
  ParseState hState_;  // 用于记录解析Header 的过程中的 状态信息
  bool keepAlive_;
  std::map<std::string, std::string> headers_;
  std::weak_ptr<TimerNode> timer_;

  void handleRead();
  void handleWrite();
  void handleConn();
  void handleError(int fd, int err_num, std::string short_msg);
  /**
   * @brief 解析客户端请求信息中的请求行
   * 1、从buff中拿到 请求行，
   * 2、解析请求方法GET， POST， HEAD
   *
   * @return URIState
   */
  URIState parseURI();
  HeaderState parseHeaders();
  AnalysisState analysisRequest();
};

/**
 * @brief HTTPdata 是对Channel的进一步封装， 添加了协议文本处理的内容
 *
 */