// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "HttpData.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "Channel.h"
#include "EventLoop.h"
#include "Util.h"
#include "time.h"

using namespace std;

// note: 表示类的初始化，在所有的线程中只执行一次
pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;

// note: 类外初始化静态变量
std::unordered_map<std::string, std::string> MimeType::mime;

/**
 * @brief
// 表示客户端线程注册的epoll事件，输入，边沿触发，oneshot事件
 *
 * oneshot：如果不注册这个事件，在多线程中，如果一个线程处理不及时，那么再次相应的时候，另外
 * 一个线程又被分配给这个任务，重复处理，
 * 为了避免这种情况，使用oneshot，就是说在唤醒该fd之后，会从epoll中删除该fd
 * 所以完成之后，需要再次注册一下这个fd
 *
 */
const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

// ! budong
const int DEFAULT_EXPIRED_TIME = 2000;  // ms

/**
 * @brief
 * 代表，一个客户端不活跃的时候，最大的保持事件，然后断开
 *
 */
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms

//  理解了 就是一个图片模拟的
char favicon[555] = {
    '\x89', 'P',    'N',    'G',    '\xD',  '\xA',  '\x1A', '\xA',  '\x0',
    '\x0',  '\x0',  '\xD',  'I',    'H',    'D',    'R',    '\x0',  '\x0',
    '\x0',  '\x10', '\x0',  '\x0',  '\x0',  '\x10', '\x8',  '\x6',  '\x0',
    '\x0',  '\x0',  '\x1F', '\xF3', '\xFF', 'a',    '\x0',  '\x0',  '\x0',
    '\x19', 't',    'E',    'X',    't',    'S',    'o',    'f',    't',
    'w',    'a',    'r',    'e',    '\x0',  'A',    'd',    'o',    'b',
    'e',    '\x20', 'I',    'm',    'a',    'g',    'e',    'R',    'e',
    'a',    'd',    'y',    'q',    '\xC9', 'e',    '\x3C', '\x0',  '\x0',
    '\x1',  '\xCD', 'I',    'D',    'A',    'T',    'x',    '\xDA', '\x94',
    '\x93', '9',    'H',    '\x3',  'A',    '\x14', '\x86', '\xFF', '\x5D',
    'b',    '\xA7', '\x4',  'R',    '\xC4', 'm',    '\x22', '\x1E', '\xA0',
    'F',    '\x24', '\x8',  '\x16', '\x16', 'v',    '\xA',  '6',    '\xBA',
    'J',    '\x9A', '\x80', '\x8',  'A',    '\xB4', 'q',    '\x85', 'X',
    '\x89', 'G',    '\xB0', 'I',    '\xA9', 'Q',    '\x24', '\xCD', '\xA6',
    '\x8',  '\xA4', 'H',    'c',    '\x91', 'B',    '\xB',  '\xAF', 'V',
    '\xC1', 'F',    '\xB4', '\x15', '\xCF', '\x22', 'X',    '\x98', '\xB',
    'T',    'H',    '\x8A', 'd',    '\x93', '\x8D', '\xFB', 'F',    'g',
    '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f',    'v',    'f',    '\xDF',
    '\x7C', '\xEF', '\xE7', 'g',    'F',    '\xA8', '\xD5', 'j',    'H',
    '\x24', '\x12', '\x2A', '\x0',  '\x5',  '\xBF', 'G',    '\xD4', '\xEF',
    '\xF7', '\x2F', '6',    '\xEC', '\x12', '\x20', '\x1E', '\x8F', '\xD7',
    '\xAA', '\xD5', '\xEA', '\xAF', 'I',    '5',    'F',    '\xAA', 'T',
    '\x5F', '\x9F', '\x22', 'A',    '\x2A', '\x95', '\xA',  '\x83', '\xE5',
    'r',    '9',    'd',    '\xB3', 'Y',    '\x96', '\x99', 'L',    '\x6',
    '\xE9', 't',    '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',    '\xA7',
    '\xC4', 'b',    '1',    '\xB5', '\x5E', '\x0',  '\x3',  'h',    '\x9A',
    '\xC6', '\x16', '\x82', '\x20', 'X',    'R',    '\x14', 'E',    '6',
    'S',    '\x94', '\xCB', 'e',    'x',    '\xBD', '\x5E', '\xAA', 'U',
    'T',    '\x23', 'L',    '\xC0', '\xE0', '\xE2', '\xC1', '\x8F', '\x0',
    '\x9E', '\xBC', '\x9',  'A',    '\x7C', '\x3E', '\x1F', '\x83', 'D',
    '\x22', '\x11', '\xD5', 'T',    '\x40', '\x3F', '8',    '\x80', 'w',
    '\xE5', '3',    '\x7',  '\xB8', '\x5C', '\x2E', 'H',    '\x92', '\x4',
    '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g',    '\x98', '\xE9',
    '6',    '\x1A', '\xA6', 'g',    '\x15', '\x4',  '\xE3', '\xD7', '\xC8',
    '\xBD', '\x15', '\xE1', 'i',    '\xB7', 'C',    '\xAB', '\xEA', 'x',
    '\x2F', 'j',    'X',    '\x92', '\xBB', '\x18', '\x20', '\x9F', '\xCF',
    '3',    '\xC3', '\xB8', '\xE9', 'N',    '\xA7', '\xD3', 'l',    'J',
    '\x0',  'i',    '6',    '\x7C', '\x8E', '\xE1', '\xFE', 'V',    '\x84',
    '\xE7', '\x3C', '\x9F', 'r',    '\x2B', '\x3A', 'B',    '\x7B', '7',
    'f',    'w',    '\xAE', '\x8E', '\xE',  '\xF3', '\xBD', 'R',    '\xA9',
    'd',    '\x2',  'B',    '\xAF', '\x85', '2',    'f',    'F',    '\xBA',
    '\xC',  '\xD9', '\x9F', '\x1D', '\x9A', 'l',    '\x22', '\xE6', '\xC7',
    '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15', '\x90', '\x7',  '\x93',
    '\xA2', '\x28', '\xA0', 'S',    'j',    '\xB1', '\xB8', '\xDF', '\x29',
    '5',    'C',    '\xE',  '\x3F', 'X',    '\xFC', '\x98', '\xDA', 'y',
    'j',    'P',    '\x40', '\x0',  '\x87', '\xAE', '\x1B', '\x17', 'B',
    '\xB4', '\x3A', '\x3F', '\xBE', 'y',    '\xC7', '\xA',  '\x26', '\xB6',
    '\xEE', '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
    '\xA',  '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X',    '\x0',  '\x27',
    '\xEB', 'n',    'V',    'p',    '\xBC', '\xD6', '\xCB', '\xD6', 'G',
    '\xAB', '\x3D', 'l',    '\x7D', '\xB8', '\xD2', '\xDD', '\xA0', '\x60',
    '\x83', '\xBA', '\xEF', '\x5F', '\xA4', '\xEA', '\xCC', '\x2',  'N',
    '\xAE', '\x5E', 'p',    '\x1A', '\xEC', '\xB3', '\x40', '9',    '\xAC',
    '\xFE', '\xF2', '\x91', '\x89', 'g',    '\x91', '\x85', '\x21', '\xA8',
    '\x87', '\xB7', 'X',    '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N',
    'N',    'b',    't',    '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
    '\xEC', '\x86', '\x2',  'H',    '\x26', '\x93', '\xD0', 'u',    '\x1D',
    '\x7F', '\x9',  '2',    '\x95', '\xBF', '\x1F', '\xDB', '\xD7', 'c',
    '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF', '\x22', 'J',    '\xC3',
    '\x87', '\x0',  '\x3',  '\x0',  'K',    '\xBB', '\xF8', '\xD6', '\x2A',
    'v',    '\x98', 'I',    '\x0',  '\x0',  '\x0',  '\x0',  'I',    'E',
    'N',    'D',    '\xAE', 'B',    '\x60', '\x82',
};

// note:初始化HTTP支持的所有数据类型
void MimeType::init() {
  mime[".html"] = "text/html";
  mime[".avi"] = "video/x-msvideo";
  mime[".bmp"] = "image/bmp";
  mime[".c"] = "text/plain";
  mime[".doc"] = "application/msword";
  mime[".gif"] = "image/gif";
  mime[".gz"] = "application/x-gzip";
  mime[".htm"] = "text/html";
  mime[".ico"] = "image/x-icon";
  mime[".jpg"] = "image/jpeg";
  mime[".png"] = "image/png";
  mime[".txt"] = "text/plain";
  mime[".mp3"] = "audio/mp3";
  mime["default"] = "text/html";
}

/**
 * @brief
 * 表示根据文件的后缀获取相应的html文件格式
 * 在使用之前需要先初始化，所以调用线程单次初始化函数
 * @param suffix
 * @return std::string
 */
std::string MimeType::getMime(const std::string &suffix) {
  pthread_once(&once_control, MimeType::init);
  if (mime.find(suffix) == mime.end())
    return mime["default"];
  else
    return mime[suffix];
}

HttpData::HttpData(EventLoop *loop, int connfd)
    : loop_(loop),
      channel_(new Channel(loop, connfd)),
      fd_(connfd),
      error_(false),
      connectionState_(H_CONNECTED),
      method_(METHOD_GET),
      HTTPVersion_(HTTP_11),
      nowReadPos_(0),
      state_(STATE_PARSE_URI),
      hState_(H_START),
      keepAlive_(false) {
  // loop_->queueInLoop(bind(&HttpData::setHandlers, this));
  channel_->setReadHandler(bind(&HttpData::handleRead, this));
  channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
  channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

void HttpData::reset() {
  // inBuffer_.clear();
  fileName_.clear();
  path_.clear();
  nowReadPos_ = 0;
  state_ = STATE_PARSE_URI;
  hState_ = H_START;
  headers_.clear();
  // keepAlive_ = false;

  // 由于timer  只在本线程中使用
  if (timer_.lock()) {
    shared_ptr<TimerNode> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();  // 指针置空，但是 饮用量不会减一
  }
}

void HttpData::seperateTimer() {
  // cout << "seperateTimer" << endl;
  if (timer_.lock()) {
    shared_ptr<TimerNode> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

void HttpData::handleRead() {
  __uint32_t &events_ = channel_->getEvents();  //获取监听事件
  do {
    bool zero =
        false;  // 代表没有读取到数据，代表客户端出现异常，断开连接或者怎么滴
    int read_num = readn(fd_, inBuffer_, zero);
    LOG << "Request: " << inBuffer_;
    if (connectionState_ ==
        H_DISCONNECTING) {  //代表  半关闭状态，对方写关闭，只需要处理写事件
      inBuffer_.clear();
      break;
    }
    // cout << inBuffer_ << endl;
    if (read_num < 0) {
      perror("1");
      error_ = true;
      handleError(fd_, 400, "Bad Request");
      break;
    }
    // else if (read_num == 0)
    // {
    //     error_ = true;
    //     break;
    // }
    else if (zero) {
      // 有请求出现但是读不到数据，可能是Request
      // Aborted，或者来自网络的数据没有达到等原因
      // 最可能是对端已经关闭了，统一按照对端已经关闭处理
      // error_ = true;
      connectionState_ = H_DISCONNECTING;
      if (read_num == 0) {
        // error_ = true;
        break;
      }
      // cout << "readnum == 0" << endl;
    }
    // 解析URI  资源定位符
    if (state_ == STATE_PARSE_URI) {
      URIState flag = this->parseURI();
      if (flag == PARSE_URI_AGAIN)
        break;
      else if (flag == PARSE_URI_ERROR) {  // 信息格式解析有问题，不是http协议
                                           // 表示出错 AGAIN我觉得  也是出错
        perror("2");
        LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
        inBuffer_.clear();
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      } else
        state_ = STATE_PARSE_HEADERS;
    }

    if (state_ == STATE_PARSE_HEADERS) {
      HeaderState flag = this->parseHeaders();
      if (flag == PARSE_HEADER_AGAIN)
        break;
      else if (flag == PARSE_HEADER_ERROR) {
        perror("3");
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      }
      if (method_ == METHOD_POST) {
        // POST方法准备
        // cout << "post....................." << endl;
        state_ = STATE_RECV_BODY;
      } else {
        // cout << "get....................." << endl;
        state_ = STATE_ANALYSIS;
      }
    }

    if (state_ == STATE_RECV_BODY) {
      int content_length = -1;
      if (headers_.find("Content-length") != headers_.end()) {
        content_length = stoi(headers_["Content-length"]);
      } else {
        // cout << "(state_ == STATE_RECV_BODY)" << endl;
        error_ = true;
        handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
        break;
      }
      if (static_cast<int>(inBuffer_.size()) < content_length)
        break;  // 在这一部分出错了
      state_ = STATE_ANALYSIS;
    }
    if (state_ == STATE_ANALYSIS) {
      AnalysisState flag = this->analysisRequest();
      if (flag == ANALYSIS_SUCCESS) {
        state_ = STATE_FINISH;
        break;
      } else {
        cout << "state_ == STATE_ANALYSIS" << endl;
        error_ = true;  // 未知错误
        break;
      }
    }
  } while (false);
  // cout << "state_=" << state_ << endl;
  if (!error_) {
    if (outBuffer_.size() > 0) {
      handleWrite();
    }
    // error_ may change
    if (!error_ && state_ == STATE_FINISH) {
      this->reset();
      if (inBuffer_.size() > 0) {
        if (connectionState_ != H_DISCONNECTING)
          handleRead();  //? 可能发生需求粘连，即tcp粘包现象 一次性处理完。
      }
    } else if (!error_ && connectionState_ != H_DISCONNECTED)
      events_ |= EPOLLIN;  //?  因为是 oneshot  所以 需要重新注册eollin
  }
}

void HttpData::handleWrite() {
  if (!error_ && connectionState_ != H_DISCONNECTED) {
    __uint32_t &events_ = channel_->getEvents();
    if (writen(fd_, outBuffer_) < 0) {
      perror("writen");
      events_ = 0;
      error_ = true;
    }
    if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
  }
}

void HttpData::handleConn() {
  seperateTimer();
  __uint32_t &events_ = channel_->getEvents();

  // 表示读解析正常结束   解析状态 对此连接进行相关处理
  if (!error_ && connectionState_ == H_CONNECTED) {
    if (events_ != 0) {
      int timeout = DEFAULT_EXPIRED_TIME;
      if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;

      //? 如果还存在这些事件，表示什么事件都没有监听到，表示对方已经关闭连接，
      if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
        events_ = __uint32_t(0);
        events_ |= EPOLLOUT;
      }

      // events_ |= (EPOLLET | EPOLLONESHOT);   为什么不 oneshot了？
      // 持续存在epoll中，？
      events_ |= EPOLLET;
      loop_->updatePoller(channel_, timeout);

    } else if (keepAlive_) {
      events_ |= (EPOLLIN | EPOLLET);
      // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
      int timeout = DEFAULT_KEEP_ALIVE_TIME;
      loop_->updatePoller(channel_, timeout);
    } else {
      // cout << "close normally" << endl;
      // loop_->shutdown(channel_);
      // loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
      events_ |= (EPOLLIN | EPOLLET);
      // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
      int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
      loop_->updatePoller(channel_, timeout);  // 都是用timer进行关闭
    }
  } else if (!error_ && connectionState_ == H_DISCONNECTING &&
             (events_ & EPOLLOUT)) {
    events_ = (EPOLLOUT | EPOLLET);
  } else {
    // cout << "close with errors" << endl;
    loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
  }
}

URIState HttpData::parseURI() {
  string &str = inBuffer_;
  cout << str << endl;
  string cop = str;
  // 读到完整的请求行再开始解析请求
  size_t pos = str.find('\r', nowReadPos_);

  // npos = -1
  if (pos < 0) {
    return PARSE_URI_AGAIN;  // 表示没有找到完整\r 表示 格式有问题，需要重发
  }

  // 去掉请求行所占的空间，节省空间
  string request_line = str.substr(0, pos);
  if (str.size() > pos + 1)
    str = str.substr(pos + 1);
  else
    str.clear();

  // Method
  int posGet =
      request_line.find("GET");  // 请求指定的页面信息，并返回实体主体。
  int posPost = request_line.find(
      "POST");  // 向指定资源提交数据进行处理请求（例如提交表单或者上传文件）。数据被包含在请求体中。POST
                // 请求可能会导致新的资源的建立和/或已有资源的修改。
  int posHead = request_line.find(
      "HEAD");  // 	类似于 GET
                // 请求，只不过返回的响应中没有具体的内容，用于获取报头

  if (posGet >= 0) {
    pos = posGet;
    method_ = METHOD_GET;
  } else if (posPost >= 0) {
    pos = posPost;
    method_ = METHOD_POST;
  } else if (posHead >= 0) {
    pos = posHead;
    method_ = METHOD_HEAD;
  } else {
    return PARSE_URI_ERROR;
  }

  // filename
  pos = request_line.find("/", pos);
  // 默认就是 index
  if (pos < 0) {
    fileName_ = "index.html";
    HTTPVersion_ = HTTP_11;
    return PARSE_URI_SUCCESS;
  } else {
    size_t _pos = request_line.find(' ', pos);
    if (_pos < 0)
      return PARSE_URI_ERROR;
    else {
      if (_pos - pos > 1) {
        fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
        size_t __pos = fileName_.find('?');
        if (__pos >= 0) {
          fileName_ = fileName_.substr(0, __pos);
        }
      }

      else
        fileName_ = "index.html";
    }
    pos = _pos;
  }
  cout << "fileName_: " << fileName_ << endl;
  // HTTP 版本号
  pos = request_line.find("/", pos);
  if (pos < 0)
    return PARSE_URI_ERROR;
  else {
    if (request_line.size() - pos <= 3)
      return PARSE_URI_ERROR;
    else {
      string ver = request_line.substr(pos + 1, 3);
      if (ver == "1.0")
        HTTPVersion_ = HTTP_10;
      else if (ver == "1.1")
        HTTPVersion_ = HTTP_11;
      else
        return PARSE_URI_ERROR;
    }
  }
  return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
  string &str = inBuffer_;
  int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
  int now_read_line_begin = 0;
  bool notFinish = true;
  size_t i = 0;
  for (; i < str.size() && notFinish; ++i) {
    switch (hState_) {
      case H_START: {
        if (str[i] == '\n' || str[i] == '\r') break;
        hState_ = H_KEY;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
      case H_KEY: {
        if (str[i] == ':') {
          key_end = i;
          if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
          hState_ = H_COLON;
        } else if (str[i] == '\n' || str[i] == '\r')
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_COLON: {
        if (str[i] == ' ') {
          hState_ = H_SPACES_AFTER_COLON;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_SPACES_AFTER_COLON: {
        hState_ = H_VALUE;
        value_start = i;
        break;
      }
      case H_VALUE: {
        if (str[i] == '\r') {
          hState_ = H_CR;
          value_end = i;
          if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
        } else if (i - value_start > 255)
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_CR: {
        if (str[i] == '\n') {
          hState_ = H_LF;
          string key(str.begin() + key_start, str.begin() + key_end);
          string value(str.begin() + value_start, str.begin() + value_end);
          headers_[key] = value;
          now_read_line_begin = i;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_LF: {
        if (str[i] == '\r') {
          hState_ = H_END_CR;
        } else {
          key_start = i;
          hState_ = H_KEY;
        }
        break;
      }
      case H_END_CR: {
        if (str[i] == '\n') {
          hState_ = H_END_LF;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_END_LF: {
        notFinish = false;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
    }
  }
  if (hState_ == H_END_LF) {
    str = str.substr(i);
    return PARSE_HEADER_SUCCESS;
  }
  str = str.substr(now_read_line_begin);
  return PARSE_HEADER_AGAIN;
}

AnalysisState HttpData::analysisRequest() {
  if (method_ == METHOD_POST) {
    // ------------------------------------------------------
    // My CV stitching handler which requires OpenCV library
    // ------------------------------------------------------
    // string header;
    // header += string("HTTP/1.1 200 OK\r\n");
    // if(headers_.find("Connection") != headers_.end() &&
    // headers_["Connection"] == "Keep-Alive")
    // {
    //     keepAlive_ = true;
    //     header += string("Connection: Keep-Alive\r\n") + "Keep-Alive:
    //     timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
    // }
    // int length = stoi(headers_["Content-length"]);
    // vector<char> data(inBuffer_.begin(), inBuffer_.begin() + length);
    // Mat src = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
    // //imwrite("receive.bmp", src);
    // Mat res = stitch(src);
    // vector<uchar> data_encode;
    // imencode(".png", res, data_encode);
    // header += string("Content-length: ") + to_string(data_encode.size()) +
    // "\r\n\r\n";
    // outBuffer_ += header + string(data_encode.begin(), data_encode.end());
    // inBuffer_ = inBuffer_.substr(length);
    // return ANALYSIS_SUCCESS;
  } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
    string header;
    header += "HTTP/1.1 200 OK\r\n";
    if (headers_.find("Connection") != headers_.end() &&
        (headers_["Connection"] == "Keep-Alive" ||
         headers_["Connection"] == "keep-alive")) {
      keepAlive_ = true;
      header += string("Connection: Keep-Alive\r\n") +
                "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) +
                "\r\n";
    }
    int dot_pos = fileName_.find('.');
    string filetype;
    if (dot_pos < 0)
      filetype = MimeType::getMime("default");
    else
      filetype = MimeType::getMime(fileName_.substr(dot_pos));

    // echo test
    if (fileName_ == "hello") {
      outBuffer_ =
          "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
      return ANALYSIS_SUCCESS;
    }

    if (fileName_ == "favicon.ico") {
      header += "Content-Type: image/png\r\n";
      header += "Content-Length: " + to_string(sizeof favicon) + "\r\n";
      header += "Server: Linxiaohu's Web Server\r\n";

      header += "\r\n";
      outBuffer_ += header;
      outBuffer_ += string(favicon, favicon + sizeof favicon);

      return ANALYSIS_SUCCESS;
    }

    struct stat sbuf;
    if (stat(fileName_.c_str(), &sbuf) < 0) {
      header.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    header += "Content-Type: " + filetype + "\r\n";
    header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
    header += "Server: Linxiaohu's Web Server\r\n";
    // 头部结束
    header += "\r\n";
    outBuffer_ += header;

    if (method_ == METHOD_HEAD) return ANALYSIS_SUCCESS;

    int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
    if (src_fd < 0) {
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);
    if (mmapRet == (void *)-1) {
      munmap(mmapRet, sbuf.st_size);
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    char *src_addr = static_cast<char *>(mmapRet);
    outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
    ;
    munmap(mmapRet, sbuf.st_size);
    return ANALYSIS_SUCCESS;
  }
  return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, string short_msg) {
  short_msg = " " + short_msg;
  char send_buff[4096];
  string body_buff, header_buff;
  body_buff += "<html><title>哎~出错了</title>";
  body_buff += "<body bgcolor=\"ffffff\">";
  body_buff += to_string(err_num) + short_msg;
  body_buff += "<hr><em> Linxiaohu's Web Server</em>\n</body></html>";

  header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
  header_buff += "Content-Type: text/html\r\n";
  header_buff += "Connection: Close\r\n";
  header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
  header_buff += "Server: Linxiaohu's Web Server\r\n";
  ;
  header_buff += "\r\n";

  // 错误处理不考虑writen不完的情况
  sprintf(send_buff, "%s", header_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
  sprintf(send_buff, "%s", body_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
}

void HttpData::handleClose() {
  connectionState_ = H_DISCONNECTED;
  // 获取指向自己的指针，只能指针，需要继承enable_shared_form_this 类，
  // 调用此函数返回指向自己的智能指针
  shared_ptr<HttpData> guard(
      shared_from_this());  // 连接断开，此时我们需要释放自己，为啥gard？
  loop_->removeFromPoller(channel_);
}

void HttpData::newEvent() {
  channel_->setEvents(DEFAULT_EVENT);
  loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}
