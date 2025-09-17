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

//�����������������˺����ᱻ����
void CSocket::zhy_read_request_handler(lpzhy_connection_t c){
    bool isflood=false;

    ssize_t reco=recvproc(c,c->precvbuf,c->irecvlen);
    if(reco<=0){
        return;
    }

    if(c->curStat==_PKG_HD_INIT){
        if(reco==m_iLenPkgHeader){
            //ǡ���յ�������ͷ
            zhy_wait_request_handler_proc_head(c,isflood);
        }else{
            //û���յ�
            c->curStat=_PKG_HD_RECVING;
            c->precvbuf=c->precvbuf+reco;
            c->irecvlen=c->irecvlen-reco;
        }
    }else if(c->curStat==_PKG_HD_RECVING){
        //��ͷ������������������
        if(c->irecvlen==reco){
            //������������
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

//���տͻ�������
ssize_t CSocket::recvproc(lpzhy_connection_t c,char* buff,ssize_t buflen){
    ssize_t n;
    n=recv(c->fd,buff,buflen,0);
    if(n==0){               //�Զ˹ر�
        ClosesocketProc(c);
        return -1;
    }

    if(n<0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            zhy_log_stderr(errno, "CSocket::recvproc()��errno == EAGAIN || errno == EWOULDBLOCK����!");
            return -1;
        }
        if (errno == EINTR) {
            zhy_log_stderr(errno, "CSocket::recvproc()��errno == EINTR����!");
            return -1;
        }
        if (errno == ECONNRESET) {
            // �ͻ��˷������˳��������ͷ����Ӽ���
        } else {
            if (errno == EBADF) 
            {
                // ��Ϊ���̣߳�ż����ɵ�socket�����Բ��ų������������Ŀ�����
            } else {
                zhy_log_stderr(errno, "CSocket::recvproc()�з�������");
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
    pPkgHeader=(LPCOMM_PKG_HEADER)c->dataHeadInfo;          //�հ�Ҫ����������հ�ͷ��������buffֱ�Ӿ���dataHeadInfo

    unsigned short e_pkgLen;
    e_pkgLen=ntohs(pPkgHeader->pkgLen);

    if(e_pkgLen<m_iLenMsgHeader){
        //�ж϶���,�ָ��հ�״̬
        c->curStat=_PKG_HD_INIT;
        c->precvbuf=c->dataHeadInfo;
        c->irecvlen=m_iLenMsgHeader;
    }else if(e_pkgLen>(_PKG_MAX_LENGTH-1000)){
        c->curStat=_PKG_HD_INIT;
        c->precvbuf=c->dataHeadInfo;
        c->irecvlen=m_iLenPkgHeader;
    }else{
        //�Ϸ�
        char *pTmpBuffer=(char*)p_memory->AllocMemory(m_iLenMsgHeader+e_pkgLen,false);
        c->precvMemPointer=pTmpBuffer;

        //������Ϣͷ
        LPSTRUC_MSG_HEADER ptmpMsgHeader=(LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn=c;
        ptmpMsgHeader->isCurrsequence=c->iCurrsequence;

        //�����ͷ
        pTmpBuffer+=m_iLenMsgHeader;
        memcpy(pTmpBuffer,pPkgHeader,m_iLenPkgHeader);
        if(e_pkgLen==m_iLenPkgHeader){
            //����� ���ް����а�ͷ
            zhy_wait_request_handler_proc_total(c,isflood);
        }else{
            //��ʼ���հ���
            c->curStat=_PKG_BD_INIT;
            c->precvbuf = pTmpBuffer + m_iLenPkgHeader;
            c->irecvlen = e_pkgLen - m_iLenPkgHeader;
        }
    }
    return;
}

//�����괦��
void CSocket::zhy_wait_request_handler_proc_total(lpzhy_connection_t c,bool& isflood){
    if(isflood==false){
        g_threadpool.inMsgRecvQueueAndSignal(c->precvMemPointer);
    }else{
        CMemory* p_memory=CMemory::GetInstance();
        p_memory->FreeMemory(c->precvMemPointer);
    }

    //������һ����
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

        if(errno==EAGAIN){              //���ͻ�������
            return -1;
        }

        if(errno==EINTR){
            zhy_log_stderr(errno,"CSocket::sendproc()��send()ʧ��");
        }else{
            return -2;
        }
    }
    return 0;
}

//epoll֪ͨ�Ŀ�д�¼�
void CSocket::zhy_write_request_handler(lpzhy_connection_t pConn){
    CMemory* p_memory=CMemory::GetInstance();

    ssize_t sendsize=sendproc(pConn,pConn->psendbuf,pConn->isendlen);

    if (sendsize > 0 && sendsize != pConn->isendlen) {
        pConn->psendbuf = pConn->psendbuf + sendsize;
        pConn->isendlen = pConn->isendlen - sendsize;
        return;
    }else if(sendsize==-1){
        zhy_log_stderr(errno,"CSocket::zhy_write_request_handler()ʱif(sendsize == -1)����");
        return;
    }

    if(sendsize>0 && sendsize==pConn->isendlen){
        if (zhy_epoll_oper_event(pConn->fd, EPOLL_CTL_MOD, EPOLLOUT, 1, pConn) == -1) { //1�����Ƴ�
            zhy_log_stderr(errno, "CSocket::zhy_write_request_handler()��zhy_epoll_oper_event()ʧ�ܡ�");
        }
    }

    p_memory->FreeMemory(pConn->psendMemPointer);
    pConn->psendMemPointer=NULL;
    --pConn->iThrowsendCount;
    return;
}