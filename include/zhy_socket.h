#ifndef __ZHY_SOCKET_H__
#define __ZHY_SOCKET_H__

#include <atomic>
#include <list>
#include <map>
#include <pthread.h>    
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

#include "zhy_comm.h"

#define ZHY_LISTEN_BACKLOG 511   //全连接队列
#define ZHY_MAX_EVENTS 512       //epoll_wait 一次最多接收数据

typedef struct zhy_listening_s zhy_listening_t,*lpzhy_listening_t;
typedef struct zhy_connection_s zhy_connection_t,*lpzhy_connection_t;
typedef class CSocket CSocket;

typedef void (CSocket::*zhy_event_handler_pt)(lpzhy_connection_t c);    //事件
//监听端口信息
struct zhy_listening_s{
    int port;
    int fd;

    lpzhy_connection_t connection; //连接池中的一条连接
};


//一条tcp连接信息
struct zhy_connection_s{
    zhy_connection_s();
    virtual ~zhy_connection_s();
    void getOneToUse();
    void putOneToFree();

    int fd;
    lpzhy_listening_t listening;            //对应的监听端口信息

    uint64_t iCurrsequence;                 //分配出去就+1
    struct sockaddr s_sockaddr;             //保存对方地址信息

    zhy_event_handler_pt rhandler;          //读事件处理
    zhy_event_handler_pt whandler;          //写事件处理

    uint32_t events;                        //epoll事件相关

    //收包相关信息
    unsigned char curStat;                  //当前收包状态
    char dataHeadInfo[_DATA_BUFSIZE_];      //存放包头信息
    char* precvbuf;                         //接收缓冲区头指针
    unsigned int irecvlen;                  //需要接受多少数据
    char* precvMemPointer;

    //发包相关
    std::atomic<int> iThrowsendCount;       //发送缓冲区满了
    char* psendMemPointer;                  //发送完成后释放
    char* psendbuf;                         //发送数据的缓冲区的头指针
    unsigned int isendlen;                  //要发送多少数据

    time_t lastPingTime;                    //心跳包检测
    time_t inRecyTime;                      //回收时间

    std::atomic<int>  iSendCount;           //发送队列中的数据条目数

    zhy_connection_t  next;                 //后继指针，把空闲的连接池对象构成一个单向链表，方便使用
};


//Socket相关类
class CSocket{
public:
    CSocket();
    virtual ~CSocket();

    //Socket初始化
    virtual bool Initialize();

    //epoll操作
    int zhy_epoll_init();       //初始化
    int zhy_epoll_oper_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,lpzhy_connection_t pConn);
    int zhy_epoll_process_events(int timer);
private:
    void ReadConf();                        //读配置项
    bool setnonblocking(int sockfd);        //套接字设置非阻塞
    bool zhy_open_listening_sockets();      //监听端口
    void zhy_close_listening_sockets();     //关闭监听套接字

    //业务处理函数
    void zhy_event_accept(lpzhy_connection_t oldc); //建立新连接
    void zhy_close_connection(lpzhy_connection_t c);
    //连接池操作
    void initConnection();       //初始化连接池
    void clearConnection();      //回收连接池
private:
    int m_worker_connections;   //epoll最大连接数
    int m_ListenPortCount;     //监听的端口数量
    int m_epollhandle;          //epoll_create返回的句柄

    //连接池相关
    std::list<lpzhy_connection_t> m_connectionList;             //连接池
    std::list<lpzhy_connection_t> m_freeconnectionList;         //空闲连接列表
    std::atomic<int> m_total_connection_n;                      //总连接数
    std::atomic<int> m_free_connection_n;                       //空闲连接数

    lpzhy_connection_t zhy_get_connection(int isock);
    void zhy_free_connection(lpzhy_connection_t c);

    std::vector<lpzhy_listening_t> m_ListenSocketList; // 监听套接字队列
    struct epoll_event m_events[ZHY_MAX_EVENTS];       // 存储epoll_wait()返回的事件

    std::list<char*>  m_MsgSendQueue;              //发送数据消息队列
    std::atomic<int> m_iSendMsgQueueCount;
    std::vector<lpzhy_connection_t> m_recyconnectionList;    //待释放的连接
    std::atomic<int> m_total_recyconnection_n;

};
#endif