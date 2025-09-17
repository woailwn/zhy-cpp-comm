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

#include "zhy_comm.h"

#define ZHY_LISTEN_BACKLOG 511   //ȫ���Ӷ���
#define ZHY_MAX_EVENTS 512       //epoll_wait һ������������

typedef struct zhy_listening_s zhy_listening_t,*lpzhy_listening_t;
typedef struct zhy_connection_s zhy_connection_t,*lpzhy_connection_t;
typedef class CSocket CSocket;

typedef void (CSocket::*zhy_event_handler_pt)(lpzhy_connection_t c);    //�¼�
//�����˿���Ϣ
struct zhy_listening_s{
    int port;
    int fd;

    lpzhy_connection_t connection; //���ӳ��е�һ������
};


//һ��tcp������Ϣ
struct zhy_connection_s{
    zhy_connection_s();
    virtual ~zhy_connection_s();
    void getOneToUse();
    void putOneToFree();

    int fd;
    lpzhy_listening_t listening;            //��Ӧ�ļ����˿���Ϣ

    uint64_t iCurrsequence;                 //�����ȥ��+1
    struct sockaddr s_sockaddr;             //����Է���ַ��Ϣ

    zhy_event_handler_pt rhandler;          //���¼�����
    zhy_event_handler_pt whandler;          //д�¼�����

    uint32_t events;                        //epoll�¼����

    //�հ������Ϣ
    unsigned char curStat;                  //��ǰ�հ�״̬
    char dataHeadInfo[_DATA_BUFSIZE_];      //��Ű�ͷ��Ϣ
    char* precvbuf;                         //���ջ�����ͷָ��
    unsigned int irecvlen;                  //��Ҫ���ܶ�������
    char* precvMemPointer;                  

    pthread_mutex_t logicProcMutex;         //�߼���������

    //�������
    std::atomic<int> iThrowsendCount;       //���ͻ���������
    char* psendMemPointer;                  //������ɺ��ͷ�
    char* psendbuf;                         //�������ݵĻ�������ͷָ��
    unsigned int isendlen;                  //Ҫ���Ͷ�������

    time_t lastPingTime;                    //���������
    time_t inRecyTime;                      //����ʱ��

    std::atomic<int>  iSendCount;           //���Ͷ����е�������Ŀ��

    zhy_connection_t  next;                 //���ָ�룬�ѿ��е����ӳض��󹹳�һ��������������ʹ��
};

//��Ϣͷ
typedef struct _STRUC_MSG_HEADER{
    lpzhy_connection_t pConn;       //��¼����
    uint64_t isCurrsequence;        //�յ����ݰ�ʱ��¼��Ӧ�����
    /*
        ����һ����ʲô��������
        ��ȡ���ӻ��ø�ֵ+1���ͷ����ӻ�����ø�ֵ+1
        �����Ƿ���˴���ͻ��˵��¼�ʱ������ͻ��˶��ߣ������ͨ���ȶ������Ϣͷ����������ӳ��е�����Ƿ���ͬ�������ͬ���ǾͲ���������
        �����ͬ��֤�����ӻ���
    */
} STRUC_MSG_HEADER,*LPSTRUC_MSG_HEADER;

//Socket�����
class CSocket{
public:
    CSocket();
    virtual ~CSocket();

    //Socket��ʼ��
    virtual bool Initialize();

    //epoll����
    int zhy_epoll_init();       //��ʼ��
    int zhy_epoll_oper_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,lpzhy_connection_t pConn);
    int zhy_epoll_process_events(int timer);

    virtual void threadRecvProcFunc(char* pMsgBuf);     //����ͻ�������
    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg,time_t cur_time);

protected:
    void sendMsg(char* sendbuf);
    void ClosesocketProc(lpzhy_connection_t p_Conn);            //�����ر�һ�����ӵ���
private:
    void ReadConf();                        //��������
    bool setnonblocking(int sockfd);        //�׽������÷�����
    bool zhy_open_listening_sockets();      //�����˿�
    void zhy_close_listening_sockets();     //�رռ����׽���

    //ҵ������
    void zhy_event_accept(lpzhy_connection_t oldc);             // ����������
    void zhy_write_request_handler(lpzhy_connection_t pConn);   // ���ݷ���ʱ��д������
    void zhy_read_request_handler(lpzhy_connection_t c);        // ������ʱ�Ķ�������
    void zhy_close_connection(lpzhy_connection_t c);            // �ر������ͷ���Դ

    ssize_t recvproc(lpzhy_connection_t c,char* buff,ssize_t buflen);           //���մӿͻ�����������
    void zhy_wait_request_handler_proc_head(lpzhy_connection_t c,bool& isflood);//��ͷ��������Ĵ���
    void zhy_wait_request_handler_proc_total(lpzhy_connection_t c,bool& isflood);//�յ�һ����������Ĵ���

    void clearMsgSendQueue();                                           //������Ͷ���
    ssize_t sendproc(lpzhy_connection_t c,char* buff,ssize_t size);     //�������ݵ��ͻ���

    // ��ȡ�Զ���Ϣ
    size_t zhy_sock_ntop(struct sockaddr* sa, int port, char* text, size_t len);
    
    //���ӳز���
    void initConnection();       //��ʼ�����ӳ�
    void clearConnection();      //�������ӳ�

    lpzhy_connection_t zhy_get_connection(int isock);
    void zhy_free_connection(lpzhy_connection_t c);

protected:
    size_t m_iLenPkgHeader;         //sizeof(COMM_PKG_HEADER)  ��ͷ
    size_t m_iLenMsgHeader;         //sizeof(STRUC_MSG_HEADER) ��Ϣͷ
private:
    struct ThreadItem
    {
        pthread_t _Handle;   // �߳̾��
        CThreadPool *_pThis; // �̳߳�ָ��
        bool isrunning;      // �Ƿ�����

        ThreadItem(CThreadPool *pthis) : _pThis(pthis), isrunning(false) {}
        ~ThreadItem();
    }; // ��Ӧ�����߳�

    int m_worker_connections;   //epoll���������
    int m_ListenPortCount;     //�����Ķ˿�����
    int m_epollhandle;          //epoll_create���صľ��

    //���ӳ����
    std::list<lpzhy_connection_t> m_connectionList;             //���ӳ�
    std::list<lpzhy_connection_t> m_freeconnectionList;         //���������б�
    std::atomic<int> m_total_connection_n;                      //��������
    std::atomic<int> m_free_connection_n;                       //����������

    std::vector<lpzhy_listening_t> m_ListenSocketList; // �����׽��ֶ���
    struct epoll_event m_events[ZHY_MAX_EVENTS];       // �洢epoll_wait()���ص��¼�

    std::list<char*>  m_MsgSendQueue;              //����������Ϣ����
    std::atomic<int> m_iSendMsgQueueCount;

    std::vector<ThreadItem*> m_threadVector;
    pthread_mutex_t m_sendMessageQueueMutex;
    sem_t m_semEventSendQueue;

    std::vector<lpzhy_connection_t> m_recyconnectionList;    //���ͷŵ�����
    std::atomic<int> m_total_recyconnection_n;

    //�����û�������
    std::atomic<int> m_onlineUserCount;
};
#endif