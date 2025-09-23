#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_socket.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"
#include "zhy_memory.h"
#include "zhy_lockmutex.h"

CSocket::CSocket(){
    m_worker_connections=1;     //epoll_init
    m_ListenPortCount=1;        //监听
    m_RecyConnectionWaitTime=60;    //待回收连接需要延迟60s回收

    m_epollhandle=-1;

    m_iLenPkgHeader=sizeof(COMM_PKG_HEADER);
    m_iLenMsgHeader=sizeof(STRUC_MSG_HEADER);

    m_iSendMsgQueueCount=0;
    m_total_recyconnection_n=0;

    m_onlineUserCount=0;
    return;
}

CSocket::~CSocket(){
    for(auto* p_zhy_listen:m_ListenSocketList){
        delete p_zhy_listen;
    }
    std::vector<lpzhy_listening_t>().swap(m_ListenSocketList);
    return;
}

bool CSocket::Initialize(){
    ReadConf();
    return zhy_open_listening_sockets();
}

bool CSocket::Initialize_subproc(){
    // 发消息互斥量初始化
    if (pthread_mutex_init(&m_sendMessageQueueMutex, NULL) != 0) {
        zhy_log_stderr(0, "CSocket::Initialize()中pthread_mutex_init(&m_sendMessageQueueMutex)失败.");
        return false;
    }

    // 连接相关互斥量初始化
    if (pthread_mutex_init(&m_connectionMutex, NULL) != 0) {
        zhy_log_stderr(0, "CSocket::Initialize()中pthread_mutex_init(&m_connectionMutex)失败.");
        return false;
    }

    // 连接回收队列相关互斥量初始化
    if (pthread_mutex_init(&m_recyconnqueueMutex, NULL) != 0) {
        zhy_log_stderr(0, "CSocket::Initialize()中pthread_mutex_init(&m_recyconnqueueMutex)失败.");
        return false;
    }

    if (sem_init(&m_semEventSendQueue, 0, 0) == -1) {
        zhy_log_stderr(0, "CSocket::Initialize()中sem_init(&m_semEventSendQueue,0,0)失败.");
        return false;
    }

    //创建发送线程和回收线程
    int err;
    ThreadItem* pRecyconn;
    m_threadVector.push_back(pRecyconn=new ThreadItem(this));
    err=pthread_create(&pRecyconn->_Handle,NULL,ServerRecyConnectionThread,pRecyconn);
    if(err!=0){
        return false;
    }

    ThreadItem* pSendQueue;
    m_threadVector.push_back(pSendQueue = new ThreadItem(this));
    err = pthread_create(&pSendQueue->_Handle, NULL, ServerSendQueueThread, pSendQueue);
    if (err != 0) {
        zhy_log_stderr(0, "CSocket::Initialize_subproc()中pthread_create(ServerRecyConnectionThread)失败.");
        return false;
    }

    return true;
}

void CSocket::Shutdown_subproc(){
    //让ServerSendQueueThread流程走下来干活
    if (sem_post(&m_semEventSendQueue) == -1) {
        zhy_log_stderr(0, "CSocket::Shutdown_subproc()中sem_post(&m_semEventSendQueue)失败.");
    }
    std::vector<ThreadItem*>::iterator iter;
    for (iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++) {
        pthread_join((*iter)->_Handle, NULL);
    }
    for (iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++) {
        if (*iter)
            delete *iter;
    }
    std::vector<ThreadItem*>().swap(m_threadVector);

    this->clearMsgSendQueue();
    this->clearConnection();

    pthread_mutex_destroy(&m_connectionMutex);
    pthread_mutex_destroy(&m_sendMessageQueueMutex);
    pthread_mutex_destroy(&m_recyconnqueueMutex);
    sem_destroy(&m_semEventSendQueue);
    return;
}

void CSocket::ReadConf(){
    CConfig* p_config=CConfig::getInstance();
    m_worker_connections=p_config->GetIntDefault("worker_connections",m_worker_connections);
    m_ListenPortCount=p_config->GetIntDefault("ListenPortCount",m_ListenPortCount);
}


//监听端口
bool CSocket::zhy_open_listening_sockets(){
    int isock;
    struct sockaddr_in serv_addr;
    int iport;
    char strinfo[100];

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    CConfig* p_config=CConfig::getInstance();
    for(int i=0;i<m_ListenPortCount;i++){
        isock=socket(AF_INET,SOCK_STREAM,0);
        if(isock==-1){
            zhy_log_stderr(errno,"CSocket::Initialize()中socket()失败,i=%d.",i);
            return false;
        }

        int reuseaddr=1;
        if(setsockopt(isock,SOL_SOCKET,SO_REUSEADDR,(const void*)&reuseaddr,sizeof(reuseaddr))==-1){
            zhy_log_stderr(errno,"CSocket::Initialize()中setsockopt(SO_REUSEADDR)失败，i=%d.",i);
            close(isock);
            return false;
        }

        //设置非阻塞
        if(setnonblocking(isock)==false){
            zhy_log_stderr(errno,"CSocket::Initialize()中setnonblocking()失败,i=%d.",i);
            close(isock);
            return false;
        }

        //绑定ip和端口
        strinfo[0]=0;
        sprintf(strinfo,"ListenPort%d",i);
        iport=p_config->GetIntDefault(strinfo,10000);
        serv_addr.sin_port=htons((in_port_t)iport);

        if(bind(isock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
            zhy_log_stderr(errno,"CSocket::Initialize()中bind()失败,i=%d.",i);
            close(isock);
            return false;
        }

        if(listen(isock,ZHY_LISTEN_BACKLOG)==-1){
            zhy_log_stderr(errno,"CSocket::Initialize()中listen失败(),i=%d",i);
            close(isock);
            return false;
        }

        lpzhy_listening_t p_listensocketitem=new zhy_listening_t;
        memset(p_listensocketitem,0,sizeof(zhy_listening_t));
        p_listensocketitem->port=iport;
        p_listensocketitem->fd=isock;
        zhy_log_error_core(ZHY_LOG_INFO,0,"监听%d端口成功",iport);
        m_ListenSocketList.push_back(p_listensocketitem);
    }
    if(m_ListenSocketList.size()<=0)
        return false;
    return true;
}      

bool CSocket::setnonblocking(int sockfd) {
    int nb = 1; // 0 清除, 1 设置
    if (ioctl(sockfd, FIONBIO, &nb) == -1) {
        return false;
    }
    return true;
}

//关闭监听套接字
void CSocket::zhy_close_listening_sockets(){
    for(int i=0;i<m_ListenPortCount;i++){
        close(m_ListenSocketList[i]->fd);
        zhy_log_error_core(ZHY_LOG_INFO,0,"关闭监听端口%d!",m_ListenSocketList[i]->port);
    }
    return;
}    


int CSocket::zhy_epoll_init(){
    m_epollhandle=epoll_create(m_worker_connections);
    if(m_epollhandle==-1){
        zhy_log_stderr(errno,"CSocket::zhy_epoll_init()中epoll_create()失败.");
        exit(2);
    }

    //连接池
    this->initConnection();

    //监听socket绑定一个连接池对象
    std::vector<lpzhy_listening_t>::iterator pos;
    for(pos=m_ListenSocketList.begin();pos!=m_ListenSocketList.end();pos++){
        lpzhy_connection_t p_Conn=zhy_get_connection((*pos)->fd);
        if(p_Conn==NULL){
            zhy_log_stderr(errno,"CSocket::zhy_epoll_init()中zhy_get_connection()失败.");
            exit(2);
        }
        p_Conn->listening=(*pos);       //连接对象和监听对象关联，方便通过连接对象找监听对象
        (*pos)->connection=p_Conn;      //监听对象和连接对象关联，方便通过监听对象找连接对象

        //监听读写事件...
        p_Conn->rhandler=&CSocket::zhy_event_accept;

        //监听事件
        if(zhy_epoll_oper_event((*pos)->fd,EPOLL_CTL_ADD,EPOLLIN|EPOLLRDHUP,0,p_Conn)==-1){
            exit(2);
        }
    }
    return 1;
}

//flag取决于事件类型，bcaction补充标志
int CSocket::zhy_epoll_oper_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,lpzhy_connection_t pConn){
    struct epoll_event ev;
    memset(&ev,0,sizeof(ev));

    if(eventtype==EPOLL_CTL_ADD){
        ev.events=flag;
        pConn->events=flag;
    }else if (eventtype==EPOLL_CTL_MOD){
        ev.events=pConn->events;
        if(bcaction==0){        //增加
            ev.events|=flag;
        }else if(bcaction==1){  //去除
            ev.events&=~flag;
        }else{
            ev.events=flag;     //覆盖
        }
        pConn->events=ev.events;
    }else{
        return 1;
    }

    ev.data.ptr=(void*)pConn;
    if(epoll_ctl(m_epollhandle,eventtype,fd,&ev)==-1){
        zhy_log_stderr(
            errno, "CSocket::zhy_epoll_oper_event()中epoll_ctl(%d,%ud,%ud,%d)失败.", fd, eventtype, flag, bcaction);
            return -1;
    }
    return 1;
}

//获取发生的事件消息
int CSocket::zhy_epoll_process_events(int timer){
    /*
        timer:
            -1 一直阻塞
            0  立即返回

        return
            -1 错误
            0  发生超时
            >0 捕获到的事件数量
    */
    int events=epoll_wait(m_epollhandle,m_events,ZHY_MAX_EVENTS,timer);

    if(events==-1){
        if(errno==EINTR){   //信号导致
            zhy_log_error_core(ZHY_LOG_INFO,errno, "CSocket::hps_epoll_process_events()中epoll_wait()失败!");
            return 1;
        }else{
            zhy_log_error_core(ZHY_LOG_ALERT,errno, "CSocket::hps_epoll_process_events()中epoll_wait()失败!");
            return 0;
        }
    }

    if(events==0){
        if(timer!=-1){
            return 1;           //阻塞时间到
        }
        zhy_log_error_core(ZHY_LOG_ALERT,0,"CSocket::hps_epoll_process_events()中epoll_wait()没超时却没返回任何事件!");
        return 0;
    }

    lpzhy_connection_t p_Conn;
    uint32_t revents;

    for(int i=0;i<events;i++){
        p_Conn=(lpzhy_connection_t)(m_events[i].data.ptr);


        //过滤过期时间
        if(p_Conn->fd==-1){
            zhy_log_error_core(ZHY_LOG_DEBUG, 0, "CSocket::zhy_epoll_process_events()中遇到了fd=-1的过期事件:%p.",
            p_Conn); continue;
        }
        
        revents=m_events[i].events;
        if(revents & EPOLLIN){
            (this->*(p_Conn->rhandler))(p_Conn);
        }

        if(revents & EPOLLOUT){
            if(revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                /*
                    EPOLLERR:连接错误
                    EPOLLHUP:连接被挂起
                    EPOLLRDHUP:TCP连接远端关闭或半关闭连接
                */
                --p_Conn->iThrowsendCount;      //可写事件数-1
            }else{
                //数据没有发送完毕
                (this->*(p_Conn->whandler))(p_Conn);
            }
        }
    }
}

void CSocket::sendMsg(char* sendbuf){
    CMemory* p_memory=CMemory::GetInstance();
    CLock lock(&m_sendMessageQueueMutex);

    if(m_iSendMsgQueueCount>50000){
        m_iDiscardSendPkgCount++;
        p_memory->FreeMemory(sendbuf);
        return;
    }

    LPSTRUC_MSG_HEADER pMsgHeader=(LPSTRUC_MSG_HEADER)sendbuf;
    lpzhy_connection_t p_Conn=pMsgHeader->pConn;
    if(p_Conn->iSendCount>400){
        zhy_log_stderr(0, "CSocket::msgSend()中发现某用户%d积压了大量待发送数据包,切断与他的连接！", p_Conn->fd);
        m_iDiscardSendPkgCount++;
        p_memory->FreeMemory(sendbuf);
        ClosesocketProc(p_Conn);
        return;
    }

    ++p_Conn->iSendCount;
    m_MsgSendQueue.push_back(sendbuf);
    ++m_iSendMsgQueueCount;
    

    if(sem_post(&m_semEventSendQueue)==-1){
        zhy_log_stderr(0, "CSocket::msgSend()中sem_post(&m_semEventSendQueue)失败.");
    }
    return;
}

void CSocket::ClosesocketProc(lpzhy_connection_t p_Conn){
    if(p_Conn->fd!=-1){
        close(p_Conn->fd);
        p_Conn->fd=-1;
    }
    if(p_Conn->iThrowsendCount>0)
        --p_Conn->iThrowsendCount;
    
    return;
}

