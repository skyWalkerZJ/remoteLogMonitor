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
