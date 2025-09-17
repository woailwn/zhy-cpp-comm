#include <stdarg.h>
#include <unistd.h>

#include "zhy_memory.h"
#include "zhy_global.h"
#include "zhy_macro.h"
#include "zhy_func.h"
#include "zhy_threadpool.h"

pthread_mutex_t CThreadPool::m_pthreadMutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::m_pthreadCond=PTHREAD_COND_INITIALIZER;

bool CThreadPool::m_shutdown=false;

CThreadPool::CThreadPool(){
    m_iRunningThreadNum=0;
    m_iRecvMsgQueueCount=0;
    return;
}

CThreadPool::~CThreadPool(){
    clearMsgRecvQueue();
    return;
}

bool CThreadPool::Create(int threadNum){
    ThreadItem* pNew;
    int err;

    for(int i=0;i<threadNum;i++){
        m_threadVector.push_back(pNew=new ThreadItem(this));
        err=pthread_create(&pNew->_Handle,0,ThreadFunc,pNew);
        if(err!=0){
            zhy_log_stderr(err, "CThreadPool::Create()�����߳�%dʧ��,���صĴ�����Ϊ%d!", i, err);
            return false;
        }
    }

    std::vector<ThreadItem*>::iterator iter;
lblfor:
    for(iter=m_threadVector.begin();iter!=m_threadVector.end();iter++){
        if((*iter)->isrunning==false){
            //�߳�δ����
            sleep(100*1000);
            goto lblfor;
        }
    }
    return true;
}

void CThreadPool::inMsgRecvQueueAndSignal(char* buf){
    int err = pthread_mutex_lock(&m_pthreadMutex);
    if (err != 0) {
        zhy_log_stderr(err, "CThreadPool::inMsgRecvQueueAndSignal()pthread_mutex_lock()ʧ�ܣ����صĴ�����Ϊ%d!", err);
    }

    m_MsgRecvQueue.push_back(buf);
    ++m_iRecvMsgQueueCount;

    err = pthread_mutex_unlock(&m_pthreadMutex);
    if (err != 0) {
        zhy_log_stderr(err, "CThreadPool::inMsgRecvQueueAndSignal()pthread_mutex_unlock()ʧ�ܣ����صĴ�����Ϊ%d!", err);
    }
    Call();//����һ���߳�������
    return;
}

void CThreadPool::clearMsgRecvQueue(){
    char* sTmpMempoint;
    CMemory* p_memory=CMemory::GetInstance();

    while(!m_MsgRecvQueue.empty()){
        sTmpMempoint=m_MsgRecvQueue.front();
        m_MsgRecvQueue.pop_front();
        p_memory->FreeMemory(sTmpMempoint);
    }
    return;
}

//���̴߳���������ִ��
void* CThreadPool::ThreadFunc(void* threadData){
    ThreadItem* pThread=static_cast<ThreadItem*>(threadData);
    CThreadPool* pThreadPoolObj=pThread->_pThis;

    CMemory* p_memory=CMemory::GetInstance();
    int err;

    pthread_t pid=pthread_self();
    while(true){
        err=pthread_mutex_lock(&m_pthreadMutex);
        if(err!=0){
            zhy_log_stderr(err, "CThreadPool::ThreadFunc()pthread_mutex_lock()ʧ�ܣ����صĴ�����Ϊ%d!", err);
        }
        //��ٻ��Ѵ���
        while(pThreadPoolObj->m_MsgRecvQueue.empty() && m_shutdown==false){
            if(pThread->isrunning==false) 
                pThread->isrunning=true;
            pthread_cond_wait(&m_pthreadCond,&m_pthreadMutex);
        }

        if(m_shutdown){
            pthread_mutex_unlock(&m_pthreadMutex);
        }

        char* jobbuf=pThreadPoolObj->m_MsgRecvQueue.front();
        pThreadPoolObj->m_MsgRecvQueue.pop_front();
        --pThreadPoolObj->m_iRecvMsgQueueCount;

        err=pthread_mutex_unlock(&m_pthreadMutex);
        if(err!=0){
            zhy_log_stderr(err, "CThreadPool::ThreadFunc()pthread_mutex_unlock()ʧ�ܣ����صĴ�����Ϊ%d!", err);
        }

        ++pThreadPoolObj->m_iRunningThreadNum;
        //jobbuf��û����˭������д������
        //g_socket.threadRecvProcFunc(jobbuf);
        p_memory->FreeMemory(jobbuf);
        --pThreadPoolObj->m_iRunningThreadNum;
    }
}

void CThreadPool::StopAll(){
    if(m_shutdown){
        return;
    }
    m_shutdown=true;

    int err=pthread_cond_broadcast(&m_pthreadCond);
    if(err!=0){
        zhy_log_stderr(err, "CThreadPool::StopAll()��pthread_cond_broadcast()ʧ�ܣ����صĴ�����Ϊ%d!", err);
        return;
    }

    std::vector<ThreadItem*>::iterator iter;
    for(iter=m_threadVector.begin();iter!=m_threadVector.end();iter++){
        pthread_join((*iter)->_Handle,NULL);
    }

    pthread_mutex_destroy(&m_pthreadMutex);
    pthread_cond_destroy(&m_pthreadCond);

    for(iter=m_threadVector.begin();iter!=m_threadVector.end();iter++){
        if(*iter)
            delete (*iter);
    }

    std::vector<CThreadPool::ThreadItem*>().swap(m_threadVector);
    zhy_log_stderr(0, "CThreadPool::StopAll()�ɹ����أ��̳߳����߳�ȫ����������!");
    return;
}

void CThreadPool::Call(){
    int err = pthread_cond_signal(&m_pthreadCond); // ����һ��������pthread_cond_wait()���߳�
    if (err != 0) {
        zhy_log_stderr(err, "CThreadPool::Call()��pthread_cond_signal()ʧ�ܣ����صĴ�����Ϊ%d!", err);
    }

    if(m_iThreadNum==m_iRunningThreadNum){
        //�̲߳�����
        time_t curtime=time(NULL);
        if(curtime-m_iLastEmgTime>10){
            m_iLastEmgTime=curtime;
            zhy_log_stderr(0, "CThreadPool::Call()�з����̳߳��е�ǰ�����߳�����Ϊ0��Ҫ���������̳߳���!");
        }
    }
}