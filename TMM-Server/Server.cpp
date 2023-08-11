#include <iostream>
#include <list>
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
#include <unordered_map>
#include <unordered_set>
using namespace std;
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define WATCH_DOG_IP "127.0.0.1"
#define BUF_SIZE 1024
#define MAX_CON_NUMS 5
#define EPOLL_SIZE 5
int main()
{
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
    int WatchDogFd = -1;
    while(true)
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

                if (WATCH_DOG_IP == inet_ntoa(client_addr.sin_addr))
                {
                    WatchDogFd = clientfd;
                    printf("get connection from log watchdog IP: %s\n",WATCH_DOG_IP);
                    printf("start receive log from watchdog\n");
                }else{
                    Log_CLI_Fds.emplace(clientfd);
                    printf("A new CLI get connection,there are %d CLI Now\n",Log_CLI_Fds.size());
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
                int len = 0; 
                while((len=recv(sockfd,buffer,BUF_SIZE,0) > 0))
                {
                    for(auto iter=Log_CLI_Fds.begin(); iter!= Log_CLI_Fds.end(); iter++)
                    {
                        send(*iter,buffer,len,0);
                        printf("push log update information to %d",*iter);
                    }
                }
                //来自watchdog的日志信息。遍历Log_CLI_Fds，分发日志。
            }else if(Log_CLI_Fds.count(sockfd) > 0)
            {
                //来自CLI的控制信息。解析json
                //包括CLI的退出等操作。
            }      
        }
    }
}