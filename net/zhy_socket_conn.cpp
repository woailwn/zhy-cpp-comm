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
#include "zhy_memory.h"
#include "zhy_socket.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"

zhy_connection_s::zhy_connection_s(){
    iCurrsequence=0;
    return;
}

zhy_connection_s::~zhy_connection_s(){
    return;
}

void zhy_connection_s::getOneToUse(){
    ++iCurrsequence;
    fd=-1;
    curStat=_PKG_HD_INIT;
    precvbuf=dataHeadInfo;
    irecvlen=sizeof(COMM_PKG_HEADER);

    precvMemPointer=NULL;
    psendMemPointer=NULL;
    events=0;
    lastPingTime=time(NULL);

    iSendCount=0;
    return;
}

void zhy_connection_s::putOneToFree(){
    ++iCurrsequence;

    if(precvMemPointer!=NULL){
        CMemory::GetInstance()->FreeMemory(precvMemPointer);
        precvMemPointer=NULL;
    }
    if(psendMemPointer!=NULL){
        CMemory::GetInstance()->FreeMemory(psendMemPointer);
        psendMemPointer=NULL;
    }

    iThrowsendCount=0;
    return;
}

void CSocket::initConnection(){
        lpzhy_connection_t p_Conn;
        CMemory* p_memory=CMemory::GetInstance();

        int ilenconnpool=sizeof(zhy_connection_t);
        for(int i=0;i<m_worker_connections;i++){
            p_Conn=(lpzhy_connection_t)p_memory->AllocMemory(ilenconnpool,true);
            p_Conn=new (p_Conn) zhy_connection_t();
            p_Conn->getOneToUse();
            m_connectionList.push_back(p_Conn);
            m_freeconnectionList.push_back(p_Conn);
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
    return;
}

lpzhy_connection_t CSocket::zhy_get_connection(int isock){

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
    pConn->putOneToFree();
    m_freeconnectionList.push_back(pConn);
    ++m_free_connection_n;
    return;
}

void CSocket::zhy_close_connection(lpzhy_connection_t c){
    zhy_free_connection(c);
    if(c->fd!=-1){
        close(c->fd);
        c->fd=-1;
    }
    return ;
}



