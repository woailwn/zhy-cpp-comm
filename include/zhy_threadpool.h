#ifndef __ZHY_THREADPOOL_H__
#define __ZHY_THREADPOOL_H__

#include <atomic>
#include <list>
#include <pthread.h>
#include <vector>

class CThreadPool{
    public:
        CThreadPool();
        ~CThreadPool();

        bool Create(int threadNum);     //����
        void StopAll();                 //�˳��̳߳ص������߳�

        void inMsgRecvQueueAndSignal(char* buf); //����Ϣ���в�֪ͨ
        void clearMsgRecvQueue();

        void Call();

        int getRecvMsgQueueCount() {return m_iRecvMsgQueueCount;}

    private:
        static void* ThreadFunc(void* threadData);          //�̻߳ص�����

    private:
        struct ThreadItem{
            pthread_t _Handle;      //�߳̾��
            CThreadPool* _pThis;    //�̳߳�ָ��
            bool isrunning;         //�Ƿ�����

            ThreadItem(CThreadPool* pthis):_pThis(pthis),isrunning(false){}
            ~ThreadItem();
        };  //��Ӧ�����߳�

    private:
        static pthread_mutex_t m_pthreadMutex;
        static pthread_cond_t m_pthreadCond;
        static bool m_shutdown;                 //�߳��˳���־

        int m_iThreadNum;                       //��Ҫ�������߳���

        std::atomic<int> m_iRunningThreadNum;   //�����е��߳���
        time_t m_iLastEmgTime;                  //��һ���̲߳����ñ�����ʱ�䣬��ֹƵ������־
        
        std::vector<ThreadItem*> m_threadVector;    //�߳�����

        //��Ϣ����
        std::list<char*> m_MsgRecvQueue;
        int m_iRecvMsgQueueCount;

};
#endif 

