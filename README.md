# remoteLogMonitor
远程日志监视
## 项目背景
在数据中心网络中，网络管理员需要通过交换机的命令行CLI去观测交换机的运行日志syslog。待更！

# start
## dependency
```
apt-get install libjsoncpp-dev
```
## problem
```
编译失败可尝试如下命令：
g++ logPayload.cpp -ljsoncpp -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0
```
1. 为什么要做这个，不是可以直接tail -f吗？
2. 三个模块分别位于什么位置，什么时候用TCP什么时候用SOCK_RAW
3. difflog的实现细节，watchdog怎么过滤掉无关的日志信息的
4. 定时器怎么确定cli已经下线，怎么进行保活的，tcp不会自动关闭连接吗？。
5. watchdog和cli的上线顺序是什么，这中间的日志怎么处理。
6. 通信协议怎么设计的。在TMM上怎么存储CLI对象的
7. 怎么处理sigpipe的
8. config视图下进行配置有什么含义？会更新Redis中的哪些table
9. 能不能同时监控多个日志文件
10. TMMServer异常处理：将Exception报错信息保存到/var/log/syslog中，使用tail -f循环查看报错信息; 什么意思？

