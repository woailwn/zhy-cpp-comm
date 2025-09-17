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

void CLogicSocket::threadRecvProcFunc(char *pMsgBuf){

}
