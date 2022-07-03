/*** 
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-05-19 14:32:24
 * @LastEditors: Lin xiaohu
 * @Description: 
 * @FilePath: /WebServer/WebServer/Main.cpp
 * @===================
 */
/***
 * @Author: Lin xiaohu
 * @Date: 2022-04-29 16:48:27
 * @LastEditTime: 2022-04-29 17:55:14
 * @LastEditors: Lin xiaohu
 * @Description:
 * @FilePath: /WebServer/WebServer/Main.cpp
 * @===================
 */
// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include <getopt.h>

#include <string>

#include "EventLoop.h"
#include "Server.h"
#include "base/Logging.h"

int main(int argc, char *argv[]) {
  int threadNum = 4;
  int port = 80;
  std::string logPath = "./WebServer.log";

  // parse args
  int opt;
  const char *str = "t:l:p:";
  while ((opt = getopt(argc, argv, str)) != -1) {
    switch (opt) {
      case 't': {
        threadNum = atoi(optarg);
        break;
      }
      case 'l': {
        logPath = optarg;
        if (logPath.size() < 2 || optarg[0] != '/') {
          printf("logPath should start with \"/\"\n");
          abort();
        }
        break;
      }
      case 'p': {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }

  Logger::setLogFileName(logPath);
// STL库在多线程上应用
#ifndef _PTHREADS
  LOG << "_PTHREADS is not defined !";  // 宏  定义   #define LOG
                                        // Logger(__FILE__, __LINE__).stream()
#endif
  EventLoop mainLoop;  // 主线程的eventloop
  Server myHTTPServer(&mainLoop, threadNum, port);
  myHTTPServer.start();
  mainLoop.loop();
  return 0;
}
