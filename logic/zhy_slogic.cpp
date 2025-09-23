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
#include <time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_crc32.h"
#include "zhy_memory.h"
#include "zhy_slogic.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_logiccomm.h"
#include "zhy_macro.h"

//成员函数指针
typedef bool (CLogicSocket::*handler)(
    lpzhy_connection_t pConn,        //连接池的连接
    LPSTRUC_MSG_HEADER pMsgHeader,   //消息头
    char* pPkgBody,                  //包体
    unsigned short iBodyLength);     //包体长度

//根据消息头的序号调用相应的服务比如 1-登录  2-ping...
static const handler statusHandler[]={
    &CLogicSocket::_HandlePing,
    NULL,
    NULL,
    NULL,
    NULL,

    //具体业务
    &CLogicSocket::_HandleRegister, //[5] 注册
    &CLogicSocket::_HandleLogIn,    //[6] 登录
};

#define AUTH_TOTAL_COMMANDS sizeof(statusHandler)/sizeof(handler)   //逻辑个数
CLogicSocket::CLogicSocket(){}

CLogicSocket::~CLogicSocket(){}

bool CLogicSocket::Initialize(){
    bool mParentInit=CSocket::Initialize();
    return mParentInit;
}

// 数据收发函数
void CLogicSocket::SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, unsigned short iMsgCode){

}

bool CLogicSocket::_HandleRegister(lpzhy_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength){

}
bool CLogicSocket::_HandleLogIn(lpzhy_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength){

}
bool CLogicSocket::_HandlePing(lpzhy_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength){

}

//处理收到的数据包
void CLogicSocket::threadRecvProcFunc(char *pMsgBuf){
    LPSTRUC_MSG_HEADER pMsgHeader=(LPSTRUC_MSG_HEADER)pMsgBuf;
    LPCOMM_PKG_HEADER pPkgHeader=(LPCOMM_PKG_HEADER)(pMsgBuf+m_iLenMsgHeader);
    void* pPkgBody=NULL;
    unsigned short pkgLen=ntohs(pPkgHeader->pkgLen);

    //获得包体，校验
    if(m_iLenMsgHeader==pkgLen){        //只有包头
            if(pPkgHeader->crc32!=0){
                //包头crc=0
                return;
            }
            pPkgBody=NULL;
    }else{
        pPkgHeader->crc32=ntohl(pPkgHeader->crc32);
        pPkgBody=(void*)(pMsgBuf+m_iLenMsgHeader+m_iLenPkgHeader);

        //包体校验
        int calcrc=CCRC32::GetInstance()->Get_CRC((unsigned char*)pPkgBody,pkgLen-m_iLenPkgHeader);
        if (calcrc != pPkgHeader->crc32) {
            zhy_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中CRC错误，丢弃数据!");
            return;
        }
    }

    unsigned short imsgCode=ntohs(pPkgHeader->msgCode);
    lpzhy_connection_t p_Conn=pMsgHeader->pConn;

    if(p_Conn->iCurrsequence!=pMsgHeader->isCurrsequence){
        return;
    }

    //过滤恶意包
    if(imsgCode>=AUTH_TOTAL_COMMANDS){
        zhy_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!", imsgCode);
        return;
    }
    if(statusHandler[imsgCode]==NULL){
        zhy_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!", imsgCode);
        return;
    }

    (this->*statusHandler[imsgCode])(p_Conn,pMsgHeader,(char*)pPkgBody,pkgLen-m_iLenPkgHeader);
    return;
}
