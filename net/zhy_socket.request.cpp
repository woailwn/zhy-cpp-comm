#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_memory.h"
#include "zhy_socket.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"
#include "zhy_threadpool.h"

//当连接有数据来，此函数会被调用
void CSocket::zhy_read_request_handler(lpzhy_connection_t c){
    bool isflood=false;

    ssize_t reco=recvproc(c,c->precvbuf,c->irecvlen);
    if(reco<=0){
        return;
    }

    if(c->curStat==_PKG_HD_INIT){
        if(reco==m_iLenPkgHeader){
            //恰好收到完整包头
            zhy_wait_request_handler_proc_head(c,isflood);
        }else{
            //没有收到
            c->curStat=_PKG_HD_RECVING;
            c->precvbuf=c->precvbuf+reco;
            c->irecvlen=c->irecvlen-reco;
        }
    }else if(c->curStat==_PKG_HD_RECVING){
        //包头不完整，继续接收中
        if(c->irecvlen==reco){
            //包被完整接收
            zhy_wait_request_handler_proc_total(c,isflood);
        }else{
            c->curStat=_PKG_BD_RECVING;
            c->precvbuf=c->precvbuf+reco;
            c->irecvlen=c->irecvlen-reco;
        }
    }else if(c->curStat==_PKG_BD_RECVING){
        if(c->irecvlen==reco){
            zhy_wait_request_handler_proc_total(c,isflood);
        }else{
            c->precvbuf=c->precvbuf+reco;
            c->irecvlen=c->irecvlen-reco;
        }
    }

    if(isflood){

    }
    return;
}

//接收客户端数据
ssize_t CSocket::recvproc(lpzhy_connection_t c,char* buff,ssize_t buflen){
    ssize_t n;
    n=recv(c->fd,buff,buflen,0);
    if(n==0){               //对端关闭
        ClosesocketProc(c);
        return -1;
    }

    if(n<0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            zhy_log_stderr(errno, "CSocket::recvproc()中errno == EAGAIN || errno == EWOULDBLOCK成立!");
            return -1;
        }
        if (errno == EINTR) {
            zhy_log_stderr(errno, "CSocket::recvproc()中errno == EINTR成立!");
            return -1;
        }
        if (errno == ECONNRESET) {
            // 客户端非正常退出，后续释放连接即可
        } else {
            if (errno == EBADF) 
            {
                // 因为多线程，偶尔会干掉socket，所以不排除产生这个错误的可能性
            } else {
                zhy_log_stderr(errno, "CSocket::recvproc()中发生错误！");
            }
        }
        ClosesocketProc(c);
        return -1;
    }
    return n;
}

void CSocket::zhy_wait_request_handler_proc_head(lpzhy_connection_t c,bool& isflood){
    CMemory* p_memory=CMemory::GetInstance();

    LPCOMM_PKG_HEADER pPkgHeader;
    pPkgHeader=(LPCOMM_PKG_HEADER)c->dataHeadInfo;          //收包要先来这里，先收包头，收数据buff直接就是dataHeadInfo

    unsigned short e_pkgLen;
    e_pkgLen=ntohs(pPkgHeader->pkgLen);

    if(e_pkgLen<m_iLenMsgHeader){
        //判断恶意,恢复收包状态
        c->curStat=_PKG_HD_INIT;
        c->precvbuf=c->dataHeadInfo;
        c->irecvlen=m_iLenMsgHeader;
    }else if(e_pkgLen>(_PKG_MAX_LENGTH-1000)){
        c->curStat=_PKG_HD_INIT;
        c->precvbuf=c->dataHeadInfo;
        c->irecvlen=m_iLenPkgHeader;
    }else{
        //合法
        char *pTmpBuffer=(char*)p_memory->AllocMemory(m_iLenMsgHeader+e_pkgLen,false);
        c->precvMemPointer=pTmpBuffer;

        //保存消息头
        LPSTRUC_MSG_HEADER ptmpMsgHeader=(LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn=c;
        ptmpMsgHeader->isCurrsequence=c->iCurrsequence;

        //保存包头
        pTmpBuffer+=m_iLenMsgHeader;
        memcpy(pTmpBuffer,pPkgHeader,m_iLenPkgHeader);
        if(e_pkgLen==m_iLenPkgHeader){
            //特殊包 有无包体有包头
            zhy_wait_request_handler_proc_total(c,isflood);
        }else{
            //开始接收包体
            c->curStat=_PKG_BD_INIT;
            c->precvbuf = pTmpBuffer + m_iLenPkgHeader;
            c->irecvlen = e_pkgLen - m_iLenPkgHeader;
        }
    }
    return;
}

//接受完处理
void CSocket::zhy_wait_request_handler_proc_total(lpzhy_connection_t c,bool& isflood){
    if(isflood==false){
        g_threadpool.inMsgRecvQueueAndSignal(c->precvMemPointer);
    }else{
        CMemory* p_memory=CMemory::GetInstance();
        p_memory->FreeMemory(c->precvMemPointer);
    }

    //接收下一个包
    c->precvMemPointer=NULL;
    c->curStat=_PKG_HD_INIT;
    c->precvbuf=c->dataHeadInfo;
    c->irecvlen=m_iLenPkgHeader;

    return;
}

ssize_t CSocket::sendproc(lpzhy_connection_t c,char* buff,ssize_t size){
    ssize_t n;
    for(;;){
        n=send(c->fd,buff,size,0);
        if(n>0){
            return n;
        }

        if(n==0){
            return 0;
        }

        if(errno==EAGAIN){              //发送缓冲区满
            return -1;
        }

        if(errno==EINTR){
            zhy_log_stderr(errno,"CSocket::sendproc()中send()失败");
        }else{
            return -2;
        }
    }
    return 0;
}

//epoll通知的可写事件
void CSocket::zhy_write_request_handler(lpzhy_connection_t pConn){
    CMemory* p_memory=CMemory::GetInstance();

    ssize_t sendsize=sendproc(pConn,pConn->psendbuf,pConn->isendlen);

    if (sendsize > 0 && sendsize != pConn->isendlen) {
        pConn->psendbuf = pConn->psendbuf + sendsize;
        pConn->isendlen = pConn->isendlen - sendsize;
        return;
    }else if(sendsize==-1){
        zhy_log_stderr(errno,"CSocket::zhy_write_request_handler()时if(sendsize == -1)成立");
        return;
    }

    if(sendsize>0 && sendsize==pConn->isendlen){
        if (zhy_epoll_oper_event(pConn->fd, EPOLL_CTL_MOD, EPOLLOUT, 1, pConn) == -1) { //1代表移除
            zhy_log_stderr(errno, "CSocket::zhy_write_request_handler()中zhy_epoll_oper_event()失败。");
        }
    }

    p_memory->FreeMemory(pConn->psendMemPointer);
    pConn->psendMemPointer=NULL;
    --pConn->iThrowsendCount;
    return;
}