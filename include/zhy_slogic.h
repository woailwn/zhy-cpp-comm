#ifndef  __ZHY_SLOGIC_H__
#define  __ZHY_SLOGIC_H__

#include <sys/socket.h>
#include "zhy_socket.h"

//�����߼���ͨѶ������
class CLogicSocket:public CSocket{
    public:
        CLogicSocket();
        virtual ~CLogicSocket();
        bool Initialize() override;

        bool _HandleRegister(lpzhy_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char* pPkgBody,unsigned short iBodyLength);
        bool _HandleLogIn(lpzhy_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);
        bool _HandlePing(lpzhy_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);

        //�����շ�����
        void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader,unsigned short iMsgCode);


        void threadRecvProcFunc(char* pMsgBuf) override;
};      
#endif 