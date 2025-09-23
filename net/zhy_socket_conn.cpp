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
#include <mutex>

#include "zhy_conf.h"
#include "zhy_memory.h"
#include "zhy_socket.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"
#include "zhy_lockmutex.h"

zhy_connection_s::zhy_connection_s(){
    iCurrsequence=0;
    pthread_mutex_init(&logicProcMutex,NULL);
    return;
}

zhy_connection_s::~zhy_connection_s(){
    return;
}

void zhy_connection_s::getOneToUse(){
    ++iCurrsequence;                //每次调用序号都+1
    fd=-1;                      
    curStat=_PKG_HD_INIT;           //收包状态处于初始状态，准备接收数据包头
    precvbuf=dataHeadInfo;          //收包数据首先到这里来
    irecvlen=sizeof(COMM_PKG_HEADER);   //收数据的长度，要求收包头这么长的数据

    precvMemPointer=NULL;           //内存地址指向NULL
    psendMemPointer=NULL;           //发送数据头指针记录    
    events=0;                       //epoll事件设为0
    iThrowsendCount=0;
    lastPingTime=time(NULL);

    iSendCount=0;
    return;
}

void zhy_connection_s::putOneToFree(){
    ++iCurrsequence;                //序号+1  

    if(precvMemPointer!=NULL){              //我们曾为该连接分配的空间要释放
        CMemory::GetInstance()->FreeMemory(precvMemPointer);
        precvMemPointer=NULL;
    }

    if(psendMemPointer!=NULL){              //清除发送缓冲区的数据
        CMemory::GetInstance()->FreeMemory(psendMemPointer);
        psendMemPointer=NULL;
    }

    iThrowsendCount=0;
}

void CSocket::initConnection(){
        lpzhy_connection_t p_Conn;
        CMemory* p_memory=CMemory::GetInstance();

        int ilenconnpool=sizeof(zhy_connection_t);
        for(int i=0;i<m_worker_connections;i++){
            p_Conn=(lpzhy_connection_t)p_memory->AllocMemory(ilenconnpool,true);
            p_Conn=new (p_Conn) zhy_connection_t();
            p_Conn->getOneToUse();
            m_connectionList.push_back(p_Conn);             //所有连接都放这里
            m_freeconnectionList.push_back(p_Conn);         //空闲连接放这里
        }
        m_free_connection_n=m_total_connection_n=m_connectionList.size();
        return;
}

void CSocket::clearConnection(){
    lpzhy_connection_t p_Conn;
    CMemory* p_memory=CMemory::GetInstance();

    while(!m_connectionList.empty()){
        p_Conn=m_connectionList.front();
        m_connectionList.pop_front();
        p_Conn->~zhy_connection_s();
        p_memory->FreeMemory(p_Conn);
    }
}

void CSocket::inRecyConnectQueue(lpzhy_connection_t pConn){
    CLock lock(&m_recyconnqueueMutex);

    bool iffind=false;
    CLock lock(&m_recyconnqueueMutex);
    for(auto* conn:m_recyconnectionList){
        if(conn==pConn){
            iffind=true;
            break;
        }
    }
    if(iffind){
        return;
    }

    pConn->inRecyTime=time(NULL);
    ++pConn->iCurrsequence;
    m_recyconnectionList.push_back(pConn);
    ++m_total_connection_n;
    --m_onlineUserCount;
    return;
}

void* CSocket::ServerRecyConnectionThread(void* threadData){

}

lpzhy_connection_t CSocket::zhy_get_connection(int isock){
    CLock lock(&m_connectionMutex);
    //有空闲连接
    if(!m_freeconnectionList.empty()){
        lpzhy_connection_t p_Conn=m_freeconnectionList.front();
        m_freeconnectionList.pop_front();
        p_Conn->getOneToUse();
        --m_free_connection_n;
        p_Conn->fd=isock;
        return p_Conn;
    }

    //没有空闲连接
    CMemory* p_memory=CMemory::GetInstance();
    lpzhy_connection_t p_Conn=(lpzhy_connection_t)p_memory->AllocMemory(sizeof(zhy_connection_t),true);
    p_Conn=new (p_Conn) zhy_connection_t();
    p_Conn->getOneToUse();
    m_connectionList.push_back(p_Conn);
    ++m_total_connection_n;
    p_Conn->fd=isock;
    return p_Conn;
}

void CSocket::zhy_free_connection(lpzhy_connection_t pConn){
    //防止其他线程可能要动连接池的连接
    CLock lock(&m_connectionMutex);

    pConn->putOneToFree();

    m_freeconnectionList.push_back(pConn);

    ++m_free_connection_n;
    return;
}

void* CSocket::ServerRecyConnectionThread(void* threadData){
    ThreadItem* pThread=static_cast<ThreadItem*>(threadData);
    CSocket* pSocketObj=pThread->_pThis;
    int err;

    time_t currtime;
    lpzhy_connection_t p_Conn;

    std::list<lpzhy_connection_t>::iterator pos,posend;

    while(1){
        usleep(200*1000);

        if(pSocketObj->m_total_connection_n>0){
            currtime=time(NULL);
            err=pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);
            if(err!=0)
                zhy_log_stderr(
                    err, "CSocket::ServerRecyConnectionThread()中pthread_mutex_lock()失败，返回的错误码为%d!", err);
            
            pos=pSocketObj->m_recyconnectionList.begin();
            posend=pSocketObj->m_recyconnectionList.end();
            for(;pos!=posend;pos++){
                p_Conn=(*pos);
                if((p_Conn->inRecyTime+pSocketObj->m_RecyConnectionWaitTime)>currtime && (g_stopEvent==0)){
                    continue;
                }

                if (p_Conn->iThrowsendCount > 0) {
                    zhy_log_stderr(
                        0, "CSocket::ServerRecyConnectionThread()中到释放时间却发现p_Conn.iThrowsendCount>"
                           "0,这个不该发生");
                }

                --pSocketObj->m_total_connection_n;
                pSocketObj->m_recyconnectionList.erase(pos);

                pSocketObj->zhy_free_connection(p_Conn);
            }
            err=pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex);
            if(err!=0)
                zhy_log_stderr(
                    err, "CSocket::ServerRecyConnectionThread()pthread_mutex_unlock()失败，返回的错误码为%d!", err);
        }

        if(g_stopEvent)  //退出整个程序
        {
            if (pSocketObj->m_total_connection_n > 0)
            {
                err = pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);
                if (err != 0)
                    zhy_log_stderr(
                        err, "CSocket::ServerRecyConnectionThread()中pthread_mutex_lock2()失败，返回的错误码为%d!",
                        err);
                
                pos = pSocketObj->m_recyconnectionList.begin();
                posend = pSocketObj->m_recyconnectionList.end();
                for (; pos != posend; ++pos) {
                    p_Conn = (*pos);
                    --pSocketObj->m_total_connection_n;
                    pSocketObj->m_recyconnectionList.erase(pos);
                    pSocketObj->zhy_free_connection(p_Conn);
                }
                 err = pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex);
                 if (err != 0)
                    zhy_log_stderr(
                        err, "CSocket::ServerRecyConnectionThread()pthread_mutex_unlock2()失败，返回的错误码为%d!",
                        err);
            }
            break;
        }
    }
    return (void*)0;
}

void* CSocket::ServerSendQueueThread(void* threadData){
    ThreadItem* pThread=static_cast<ThreadItem*>(threadData);
    CSocket* pSocketObj=pThread->_pThis;
    int err;

    std::list<char*>::iterator pos,pos2,posend;

    char* pMsgBuf;
    LPSTRUC_MSG_HEADER pMsgHeader;
    LPCOMM_PKG_HEADER pPkgHeader;
    lpzhy_connection_t p_Conn;
    unsigned short itmp;
    ssize_t sendsize;

    CMemory* p_memory=CMemory::GetInstance();
    while(g_stopEvent==0)
    {
        if(sem_wait(&pSocketObj->m_semEventSendQueue)==-1){
             if (errno != EINTR)
                zhy_log_stderr(
                    errno, "CSocket::ServerSendQueueThread()中sem_wait(&pSocketObj->m_semEventSendQueue)失败.");
        }

        if(g_stopEvent!=0)  //已经停止
            break;
        
        if(pSocketObj->m_iSendMsgQueueCount>0){
            err=pthread_mutex_lock(&pSocketObj->m_sendMessageQueueMutex);
            if (err != 0)
                zhy_log_stderr(
                    err, "CSocket::ServerSendQueueThread()中pthread_mutex_lock()失败，返回的错误码为%d!", err);
            
            pos=pSocketObj->m_MsgSendQueue.begin();
            posend=pSocketObj->m_MsgSendQueue.end();

            while(pos!=posend){
                pMsgBuf=(*pos);
                pMsgHeader=(LPSTRUC_MSG_HEADER)pMsgBuf;
                pPkgHeader=(LPCOMM_PKG_HEADER)(pMsgBuf+pSocketObj->m_iLenMsgHeader);
                p_Conn=pMsgHeader->pConn;

                //过滤过期包（客户端已经掉线)
                if(p_Conn->iCurrsequence!=pMsgHeader->isCurrsequence){
                    //本包序列号与p_Conn中的实际序列号已经不同，丢弃消息(被回收)
                    /*
                        pMsgHeader->isCurrsequence 服务器收到客户端发送来的注册请求包时的连接序列号
                        p_Conn->iCurrsequence 此刻的连接序列号值
                        如果客户端在处理业务中途断线了p_Conn->iCurrsequence值就会被+1
                    */
                    pos2=pos;       //这里用pos2是为了防止迭代器失效
                    pos++;
                    pSocketObj->m_MsgSendQueue.erase(pos2);
                    --pSocketObj->m_iSendMsgQueueCount;
                    p_memory->FreeMemory(pMsgBuf);
                    continue;
                }

                if(p_Conn->iThrowsendCount>0){
                    pos++;
                    continue;
                }

                --p_Conn->iSendCount;

                p_Conn->psendMemPointer=pMsgBuf;
                pos2=pos;
                pos++;
                pSocketObj->m_MsgSendQueue.erase(pos2);
                --pSocketObj->m_iSendMsgQueueCount;
                p_Conn->psendbuf=(char*)pPkgHeader;
                itmp=ntohs(pPkgHeader->pkgLen);
                p_Conn->isendlen=itmp;

                //即将发送的数据isendlen
                sendsize=pSocketObj->sendproc(p_Conn,p_Conn->psendbuf,p_Conn->isendlen);
                if(sendsize>0){
                    if (sendsize == p_Conn->isendlen)
                    { // 发完
                        p_memory->FreeMemory(p_Conn->psendMemPointer);
                        p_Conn->psendMemPointer = NULL;
                        p_Conn->iThrowsendCount = 0;
                    }
                    else
                    { // 没发完
                        p_Conn->psendbuf = p_Conn->psendbuf + sendsize;
                        p_Conn->isendlen = p_Conn->isendlen - sendsize;

                        ++p_Conn->iThrowsendCount;      
                        // 没发完还要继续发，注册可写事件
                        if (pSocketObj->zhy_epoll_oper_event(p_Conn->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, p_Conn) == -1)
                        {
                            zhy_log_stderr(errno, "CSocket::ServerSendQueueThread()hps_epoll_oper_event()失败.");
                        }
                    }
                    continue;
                }else if(sendsize==0){
                    p_memory->FreeMemory(p_Conn->psendMemPointer);
                    p_Conn->psendMemPointer = NULL;
                    p_Conn->iThrowsendCount = 0;
                    continue;
                }else if(sendsize==-1){
                    //发送缓冲区满
                    ++p_Conn->iThrowsendCount;
                    if (pSocketObj->zhy_epoll_oper_event(p_Conn->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, p_Conn) == -1) {
                        zhy_log_stderr(errno, "CSocket::ServerSendQueueThread()中hps_epoll_add_event()_2失败.");
                    }
                    continue;
                }else{
                    p_memory->FreeMemory(p_Conn->psendMemPointer);
                    p_Conn->psendMemPointer = NULL;
                    p_Conn->iThrowsendCount = 0;
                    continue;
                }
            }

            err = pthread_mutex_unlock(&pSocketObj->m_sendMessageQueueMutex);
            if (err != 0)
                zhy_log_stderr(
                    err, "CSocket::ServerSendQueueThread()pthread_mutex_unlock()失败，返回的错误码为%d!", err);
        }
    }
    return (void*)0;
}

void CSocket::zhy_close_connection(lpzhy_connection_t c){
    zhy_free_connection(c);
    if(c->fd!=-1){
        close(c->fd);
        c->fd=-1;
    }
    return ;
}


