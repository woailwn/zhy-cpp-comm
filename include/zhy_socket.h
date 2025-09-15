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
private:
    void ReadConf();                        //��������
    bool setnonblocking(int sockfd);        //�׽������÷�����
    bool zhy_open_listening_sockets();      //�����˿�
    void zhy_close_listening_sockets();     //�رռ����׽���

    //ҵ������
    void zhy_event_accept(lpzhy_connection_t oldc); //����������
    void zhy_close_connection(lpzhy_connection_t c);
    //���ӳز���
    void initConnection();       //��ʼ�����ӳ�
    void clearConnection();      //�������ӳ�
private:
    int m_worker_connections;   //epoll���������
    int m_ListenPortCount;     //�����Ķ˿�����
    int m_epollhandle;          //epoll_create���صľ��

    //���ӳ����
    std::list<lpzhy_connection_t> m_connectionList;             //���ӳ�
    std::list<lpzhy_connection_t> m_freeconnectionList;         //���������б�
    std::atomic<int> m_total_connection_n;                      //��������
    std::atomic<int> m_free_connection_n;                       //����������

    lpzhy_connection_t zhy_get_connection(int isock);
    void zhy_free_connection(lpzhy_connection_t c);

    std::vector<lpzhy_listening_t> m_ListenSocketList; // �����׽��ֶ���
    struct epoll_event m_events[ZHY_MAX_EVENTS];       // �洢epoll_wait()���ص��¼�

    std::list<char*>  m_MsgSendQueue;              //����������Ϣ����
    std::atomic<int> m_iSendMsgQueueCount;
    std::vector<lpzhy_connection_t> m_recyconnectionList;    //���ͷŵ�����
    std::atomic<int> m_total_recyconnection_n;

};
#endif