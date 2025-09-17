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

        bool Create(int threadNum);     //创建
        void StopAll();                 //退出线程池的所有线程

        void inMsgRecvQueueAndSignal(char* buf); //入消息队列并通知
        void clearMsgRecvQueue();

        void Call();

        int getRecvMsgQueueCount() {return m_iRecvMsgQueueCount;}

    private:
        static void* ThreadFunc(void* threadData);          //线程回调函数

    private:
        struct ThreadItem{
            pthread_t _Handle;      //线程句柄
            CThreadPool* _pThis;    //线程池指针
            bool isrunning;         //是否启动

            ThreadItem(CThreadPool* pthis):_pThis(pthis),isrunning(false){}
            ~ThreadItem();
        };  //对应单个线程

    private:
        static pthread_mutex_t m_pthreadMutex;
        static pthread_cond_t m_pthreadCond;
        static bool m_shutdown;                 //线程退出标志

        int m_iThreadNum;                       //需要创建的线程数

        std::atomic<int> m_iRunningThreadNum;   //运行中的线程数
        time_t m_iLastEmgTime;                  //上一次线程不够用报警的时间，防止频繁报日志
        
        std::vector<ThreadItem*> m_threadVector;    //线程容器

        //消息队列
        std::list<char*> m_MsgRecvQueue;
        int m_iRecvMsgQueueCount;

};
#endif 

