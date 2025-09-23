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
#include <memory>
#include "zhy_comm.h"
#include "zhy_threadpool.h"

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

    pthread_mutex_t logicProcMutex;         //逻辑处理互斥量

    //发包相关
    std::atomic<int> iThrowsendCount;       //发送缓冲区满了，就是一个标志，标志增加了一个可写时间到epoll红黑树节点上，意味着数据未发完
    char* psendMemPointer;                  //发送完成后释放
    char* psendbuf;                         //发送数据的缓冲区的头指针
    unsigned int isendlen;                  //要发送多少数据

    time_t lastPingTime;                    //心跳包检测
    time_t inRecyTime;                      //回收时间

    std::atomic<int>  iSendCount;           //发送队列中的数据条目数

    zhy_connection_t  next;                 //后继指针，把空闲的连接池对象构成一个单向链表，方便使用
};

//消息头
typedef struct _STRUC_MSG_HEADER{
    lpzhy_connection_t pConn;       //记录连接
    uint64_t isCurrsequence;        //收到数据包时记录对应的序号
    /*
        解释一下有什么用这个序号
        当取连接会让该值+1，释放连接会继续让该值+1
        当我们服务端处理客户端的事件时，如果客户端断线，服务端通过比对这个消息头的序号与连接池中的序号是否相同如果不相同我们就不继续处理，
        如果相同则证明连接还在
    */
} STRUC_MSG_HEADER,*LPSTRUC_MSG_HEADER;

//Socket相关类
class CSocket{
public:
    CSocket();
    virtual ~CSocket();

    //Socket初始化
    virtual bool Initialize();
    virtual bool Initialize_subproc();
    virtual void Shutdown_subproc();

    //epoll操作
    int zhy_epoll_init();       //初始化
    int zhy_epoll_oper_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,lpzhy_connection_t pConn);
    int zhy_epoll_process_events(int timer);

    virtual void threadRecvProcFunc(char* pMsgBuf);     //处理客户端请求
    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg,time_t cur_time);

protected:
    void sendMsg(char* sendbuf);
    void ClosesocketProc(lpzhy_connection_t p_Conn);            //主动关闭一个连接调用
private:
    void ReadConf();                        //读配置项
    bool setnonblocking(int sockfd);        //套接字设置非阻塞
    bool zhy_open_listening_sockets();      //监听端口
    void zhy_close_listening_sockets();     //关闭监听套接字

    //业务处理函数
    void zhy_event_accept(lpzhy_connection_t oldc);             // 建立新连接
    void zhy_write_request_handler(lpzhy_connection_t pConn);   // 数据发送时的写处理函数
    void zhy_read_request_handler(lpzhy_connection_t c);        // 来数据时的读处理函数
    void zhy_close_connection(lpzhy_connection_t c);            // 关闭连接释放资源

    ssize_t recvproc(lpzhy_connection_t c,char* buff,ssize_t buflen);           //接收从客户端来的数据
    void zhy_wait_request_handler_proc_head(lpzhy_connection_t c,bool& isflood);//包头收完整后的处理
    void zhy_wait_request_handler_proc_total(lpzhy_connection_t c,bool& isflood);//收到一个完整包后的处理
    
    void clearMsgSendQueue();                                           //清除发送队列
    ssize_t sendproc(lpzhy_connection_t c,char* buff,ssize_t size);     //发送数据到客户端

    // 获取对端信息
    size_t zhy_sock_ntop(struct sockaddr* sa, int port, char* text, size_t len);
    
    //连接池操作
    void initConnection();       //初始化连接池
    void clearConnection();      //回收连接池

    lpzhy_connection_t zhy_get_connection(int isock);
    void zhy_free_connection(lpzhy_connection_t c);

    //收集待回收连接
    void inRecyConnectQueue(lpzhy_connection_t pConn);      //将要延迟回收的连接加入延迟回收列表

    //线程处理函数
    static void* ServerSendQueueThread(void* threadData);      // 发送数据的线程
    static void* ServerRecyConnectionThread(void* threadData); // 回收连接的线程
    static void* ServerTimerQueueMonitorThread(void* threadData); // 时间队列监视线程，处理到期不发心跳包的用户
protected:
    size_t m_iLenPkgHeader;         //sizeof(COMM_PKG_HEADER)  包头
    size_t m_iLenMsgHeader;         //sizeof(STRUC_MSG_HEADER) 消息头
private:
    struct ThreadItem
    {
        pthread_t _Handle;   // 线程句柄
        CSocket *_pThis;    
        bool isrunning;      // 是否启动

        ThreadItem(CSocket *pthis) : _pThis(pthis), isrunning(false) {}
        ~ThreadItem();
    }; // 对应单个线程

    int m_worker_connections;   //epoll最大连接数
    int m_ListenPortCount;     //监听的端口数量
    int m_epollhandle;          //epoll_create返回的句柄

    //连接池相关
    std::list<lpzhy_connection_t> m_connectionList;             //连接池
    std::list<lpzhy_connection_t> m_freeconnectionList;         //空闲连接列表
    std::atomic<int> m_total_connection_n;                      //总连接数
    std::atomic<int> m_free_connection_n;                       //空闲连接数

    std::vector<lpzhy_listening_t> m_ListenSocketList; // 监听套接字队列
    struct epoll_event m_events[ZHY_MAX_EVENTS];       // 存储epoll_wait()返回的事件

    std::list<char*>  m_MsgSendQueue;              //发送数据消息队列
    std::atomic<int> m_iSendMsgQueueCount;

    std::vector<ThreadItem*> m_threadVector;
    pthread_mutex_t m_sendMessageQueueMutex;
    sem_t m_semEventSendQueue;                      //处理发消息线程相关的信号量

    std::list<lpzhy_connection_t> m_recyconnectionList;    //待释放的连接
    std::atomic<int> m_total_recyconnection_n;
    int m_RecyConnectionWaitTime;                           //等待释放时间

    pthread_mutex_t m_connectionMutex;          //连接互斥量
    pthread_mutex_t m_recyconnqueueMutex;       //连接回收队列的互斥量

    //控制用户连接数
    std::atomic<int> m_onlineUserCount;

    int m_iDiscardSendPkgCount;             //丢弃的发送数据包的数量
};
#endif