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
#include <sys/wait.h>
#include <signal.h>
#include "logPayload.h"
using namespace std;
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024

#define EPOLL_SIZE 30
int sigtype = 0;
int ifexit = 0;

int ifconnected = 0;
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

int addFdToEpoll(int epollfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnonblocking(fd);
}
int main()
{
	struct sigaction sigpipe1, sigpipe2;
	sigemptyset(&sigpipe1.sa_mask);
	sigpipe1.sa_handler = sig_pipe;
	sigpipe1.sa_flags = 0;
	sigpipe1.sa_flags |= SA_RESTART;
	sigaction(SIGPIPE,&sigpipe1,&sigpipe2);
	//注册sigchld信号处理器
	struct sigaction sigchld1, sigchld2;
	sigemptyset(&sigchld1.sa_mask);
	sigchld1.sa_handler = sig_chld;
	sigchld1.sa_flags = 0;
	sigchld1.sa_flags |= SA_RESTART;
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

    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        printf("ERROR(line 50): epfd created failed!\n");
    }
    printf("epfd created successfully!\n");

    struct epoll_event events[EPOLL_SIZE];

    int pipe_fd[2];
    if(pipe(pipe_fd) < 0)
    {
        printf("create pipe failed!\n");
        exit(-1);
    }
    
    addFdToEpoll(epfd,pipe_fd[0]);
    int connfd = -1; 
    int pid = fork();
    if(pid < 0)
    {
        printf("fork failed!\n");
        exit(-1);
    }else if(pid == 0) //子进程将控制信息写入到管道，由父进程做处理后发送到TMM-Server
    {
        close(pipe_fd[0]);
        printf("Please input 'exit' to exit terminal!\n");
        char instruction[50];
        while(!ifexit)
        {
            bzero(&instruction,50);
            cin.getline(instruction,50);

            if(write(pipe_fd[1],instruction,strlen(instruction)) < 0)
            {
                printf("pipe write failed!\n");
            }
        }
        close(pipe_fd[1]);
    }else{
        close(pipe_fd[1]);
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
                
                char buffer[BUF_SIZE];
                bzero(&buffer,BUF_SIZE);
                
                if(sockfd == connfd)
                {
                    //来自TMM-Server的日志更新信息
                    int len = -1;
                    string data;
                    while((len = recv(sockfd,buffer,BUF_SIZE,0)) > 0)
                    {
                        //输入日志到控制台
                        data += string(buffer);
                    }
                    LogPayload payload;
                    payload.parseJsonToClass(data);
                    cout << payload.getData() << endl;
                }else if(sockfd == pipe_fd[0])
                {
                    //来自CLI的控制信息，需要发送到TMM-server

                    int res = read(sockfd,buffer,BUF_SIZE);
                    /*
                    1. exit
                    2. show terminal logging server
                    3. 构建json，构造特定的payload，包含user、pid、ip、port、报文类型、指令、日志信息、timestamp
                    */

                    if(res == 0){
                        printf("read control message from pipe failed!\n");
                        ifexit = 1;
                    }else{
                        
                        string instruction = string(buffer);
                        printf("收到来自子进程的消息!");
                        cout << instruction << endl;
                        if(instruction == "terminal logging")
                        {
                            if(!ifconnected)
                            {
                                connfd = socket(AF_INET,SOCK_STREAM,0);
                                if(connfd < 0)
                                {
                                    printf("ERROR(line 27): create connfd failed!\n");
                                    exit(-1);
                                }
                                printf("create connfd successfully!\n");

                                if(connect(connfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
                                {
                                    printf("ERROR(line 34): connection to server failed!\n");
                                    exit(-1);
                                }

                                addFdToEpoll(epfd,connfd);
                                ifconnected = 1;

                                LogPayload payload;
                                payload.setType(1);//control
                                payload.setInstruction(instruction);
                                send(connfd,payload.toJsonString().data(),BUF_SIZE,0);             
                            }else{
                                printf("already start terminal logging\n");
                            }
                        }else if(instruction == "exit")
                        {
                            LogPayload payload;
                            payload.setType(1);//control
                            payload.setInstruction(instruction);
                            send(connfd,payload.toJsonString().data(),BUF_SIZE,0);
                            
                            close(connfd);  
                            ifexit = 1;
                        }
                    }
                }
            }
        }
        close(pipe_fd[0]);
        close(connfd);
    }
}