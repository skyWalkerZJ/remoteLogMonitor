#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unordered_map>
#include <unordered_set>
#include "logPayload.h"
using namespace std;
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define MAX_CON_NUMS 10
#define EPOLL_SIZE 30
string WATCHDOG_IP = "192.168.239.1";
struct logClient{
    int sockfd;
    int pid;
    string ip;
    int port;
};
int sigtype = 0;
int ifexit = 0;


int WatchDogFd = -1;

void sig_int(int signo)
{
	sigtype = signo;
	ifexit = 1;
}
void sig_pipe(int signo)
{
	sigtype = signo;
}
void sig_chld(int signo)
{
	sigtype = signo;
	pid_t pid_chld;
	int stat;
	while ((pid_chld = waitpid(-1, &stat, WNOHANG)) > 0);
}

int setnonblocking(int fd) {
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}
int main()
{
	struct sigaction sigpipe1, sigpipe2;
	sigemptyset(&sigpipe1.sa_mask);
	sigpipe1.sa_handler=sig_pipe;
	sigpipe1.sa_flags=0;
	sigpipe1.sa_flags|=SA_RESTART;
	sigaction(SIGPIPE,&sigpipe1,&sigpipe2);
	//注册sigchld信号处理器
	struct sigaction sigchld1, sigchld2;
	sigemptyset(&sigchld1.sa_mask);
	sigchld1.sa_handler=sig_chld;
	sigchld1.sa_flags=0;
	sigchld1.sa_flags|=SA_RESTART;
	sigaction(SIGCHLD, &sigchld1, &sigchld2);
	//注册sigint信号处理器
	struct sigaction sigint1, sigint2;
	sigemptyset(&sigint1.sa_mask);
	sigint1.sa_flags = 0;
	sigint1.sa_handler = &sig_int;
	sigaction(SIGINT, &sigint1, &sigint2);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd < 0)
    {
        printf("ERROR(line 27): create listernfd failed!\n");
        exit(-1);
    }
    printf("create listenfd successfully!\n");

    if(bind(listenfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
    {
        printf("ERROR(line 34): bind listenfd failed!\n");
        exit(-1);
    }

    int res = listen(listenfd,MAX_CON_NUMS);
    if(res < 0 )
    {
        printf("ERROR(line 42): listern failed!\n");
    }
    printf("start to listen on IP：%s PORT: %d============\n",SERVER_IP, SERVER_PORT);

    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        printf("ERROR(line 50): epfd created failed!\n");
    }
    printf("epfd created successfully!\n");

    struct epoll_event events[EPOLL_SIZE];

    struct epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    
    setnonblocking(listenfd);

    unordered_set<int> Log_CLI_Fds;
    unordered_map<int,LogPayload*> LogCLIs;
    while(!ifexit)
    {
        int epoll_event_counts = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(epoll_event_counts < 0)
        {
            printf("ERROR(line 68): epoll_wait failed!\n");
            break;
        }

        printf("There are %d Epoll_events!\n",epoll_event_counts);

        for(int i=0; i < epoll_event_counts; i++)
        {
            int sockfd = events[i].data.fd;

            if(sockfd == listenfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addrLen = sizeof(struct sockaddr_in);
                int clientfd = accept(listenfd,(struct sockaddr*)&client_addr,&client_addrLen);

                printf("get client connection from IP: %s PORT: %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

                if (WATCHDOG_IP == string(inet_ntoa(client_addr.sin_addr)))
                {
                    WatchDogFd = clientfd;
                    printf("get connection from log watchdog IP: %s\n",WATCHDOG_IP.data());
                    printf("start receive log from watchdog\n");
                }else{
                    Log_CLI_Fds.emplace(clientfd);
                    printf("A new CLI get connection,there are %ld CLI Now\n",Log_CLI_Fds.size());
                }

                struct epoll_event ep_ev;
                ep_ev.data.fd = clientfd;
                ep_ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ep_ev);
                setnonblocking(clientfd);

                printf("add Clientfd %d to epoll\n",clientfd);
            }else if(sockfd == WatchDogFd)
            {
                char buffer[BUF_SIZE];
                bzero(&buffer,BUF_SIZE);
                int len = 0;int readed;
                string log;
                while((readed = recv(sockfd,buffer,BUF_SIZE,0) > 0))
                {
                    len += readed;
                    log += string(buffer);   
                }
                for(auto iter = Log_CLI_Fds.begin(); iter!= Log_CLI_Fds.end(); iter++)
                {
                    //封装为json再发送
                    LogPayload* payload = new LogPayload();
                    payload->setData(log);
                    send(*iter,payload->toJsonString().data(),payload->toJsonString().size(),0);
                    printf("push log update information to %d",*iter);
                }
            }else if(Log_CLI_Fds.count(sockfd) > 0)
            {
                //来自CLI的控制信息。解析json
                //包括CLI的退出等操作。
                char buffer[BUF_SIZE];
                bzero(&buffer,BUF_SIZE);
                int len = -1;string jsonFromCLI;
                while((len = recv(sockfd,buffer,BUF_SIZE,0)) > 0)
                {
                    jsonFromCLI += string(buffer);
                }
                LogPayload* payload = new LogPayload();
                payload->parseJsonToClass(jsonFromCLI);
                if(payload->getInstruction() == "exit")
                {
                    LogCLIs.erase(sockfd);
                    //解除epoll注册
                    close(sockfd);
                }else if(payload->getInstruction() == "terminal logging")
                {
                    LogCLIs.emplace(sockfd,payload);
                }else if(payload->getInstruction() == "show terminal logging")
                {
                       
                }
            }
        }
    }
}