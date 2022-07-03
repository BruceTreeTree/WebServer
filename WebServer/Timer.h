
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <unistd.h>

#include <deque>
#include <memory>
#include <queue>

#include "HttpData.h"
#include "base/MutexLock.h"
#include "base/noncopyable.h"

class HttpData;

class TimerNode {
 public:
  /**
   * @brief Construct a new Timer Node object
   * 初始化当前的请求对象，并设置超时时间
   * 1、计算当前的时间，以毫秒计时
   * 2、当前时间加上，超时时间，赋值给截至时间
   * @param requestData
   * @param timeout
   */
  TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
  /**
   * @brief Destroy the Timer Node object
   * 1、判断当前客户端请求对象是否析构
   * 2、如果没有析构，说明当前的请求对象还有指针指向他，但是此时请求超时了
   * 3、调用HttpData中detete函数，将此连接关闭。
   */
  ~TimerNode();
  /**
   * @brief Construct a new Timer Node object
   * 复制构造函数
   * @param tn
   */
  TimerNode(TimerNode &tn);
  /**
   * @brief 更新超时时间
   * 1、重新计算当前时间
   * 2、把传进来的时间加上
   * @param timeout
   */
  void update(int timeout);
  /**
   * @brief
   * 判断是否超时，超时则设置该对象的dete状态为true 当 访问到头的时候在删除
   *
   * @return true
   * @return false
   */
  bool isValid();
  /**
   * @brief 表示释放HttpData所指向的空间，
   * 并且讲本身对象detete对象设置为true，表示 对象已经被删除，删除该对象
   *
   */
  void clearReq();
  /**
   * @brief Set the Deleted object
   *
   */
  void setDeleted() { deleted_ = true; }
  /**
   * @brief
   * 判断，这个Timenode中的请求对象是否被删除
   * @return true
   * @return false
   */
  bool isDeleted() const { return deleted_; }
  /**
   * @brief Get the Exp Time object
   *
   * @return size_t
   */
  size_t getExpTime() const { return expiredTime_; }

 private:
  bool deleted_;
  size_t expiredTime_;

  std::shared_ptr<HttpData> SPHttpData;
};

struct TimerCmp {
  /**
   * @brief
   * 仿函数，用来比较两个TimeNode对象的超时时间的大小
   */
  bool operator()(std::shared_ptr<TimerNode> &a,
                  std::shared_ptr<TimerNode> &b) const {
    return a->getExpTime() > b->getExpTime();
  }
};

/**
 * @brief
 * 对Timenode进行更上一层的封装，用来管理时间队列
 *
 */
class TimerManager {
 public:
  TimerManager();
  ~TimerManager();

  /**
   * @brief
   * 主要是把请求对象封装到队列中
   * 1、把HttpData对象封装成为 TimeNode对象
   * 2、添加TimeNode对象到优先队列中管理
   * 3、让HttpData对象连接到TimeNode对象，使得可以相互访问。
   *
   * @param SPHttpData 一个指向HTTPDATA 对象的指针
   * @param timeout 设置该请求对象的计时时间
   */
  void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
  /**
   * @brief 处理超时时间队列
   * 这里只在 使用到他的 IO线程中调用，所以不用枷锁
   * 1、访问优先队列头部，如果已经被删除，则出队
   * 2、循环监听
   *
   */
  void handleExpiredEvent();

 private:
  typedef std::shared_ptr<TimerNode>
      SPTimerNode;  // 指向TimeNode类型的指针SPTimerNode

  // 使用优先队列来存储，容器使用双端队列，比较函数，用来存储时间节点对象，判断该对象是否超时，以此来判断是否断开
  std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp>
      timerNodeQueue;
  // MutexLock lock;
};