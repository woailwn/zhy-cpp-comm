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

//��Ա����ָ��
typedef bool (CLogicSocket::*handler)(
    lpzhy_connection_t pConn,        //���ӳص�����
    LPSTRUC_MSG_HEADER pMsgHeader,   //��Ϣͷ
    char* pPkgBody,                  //����
    unsigned short iBodyLength);     //���峤��

//������Ϣͷ����ŵ�����Ӧ�ķ������ 1-��¼  2-ping...
static const handler statusHandler[]={
    &CLogicSocket::_HandlePing,
    NULL,
    NULL,
    NULL,
    NULL,

    //����ҵ��
    &CLogicSocket::_HandleRegister, //[5] ע��
    &CLogicSocket::_HandleLogIn,    //[6] ��¼
};

CLogicSocket::CLogicSocket(){}

CLogicSocket::~CLogicSocket(){}

bool CLogicSocket::Initialize(){
    bool mParentInit=CSocket::Initialize();
    return mParentInit;
}

// �����շ�����
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
