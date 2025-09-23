#ifndef __ZHY_LOCKMUTEX_H__
#define __ZHY_LOCKMUTEX_H__

#include <pthread.h>

class CLock {
  public:
    CLock(pthread_mutex_t* pMutex) {
        m_pMutex = pMutex;
        pthread_mutex_lock(m_pMutex);
        return;
    }

    ~CLock() {
        pthread_mutex_unlock(m_pMutex);
        return;
    }

  private:
    pthread_mutex_t* m_pMutex;
};  
#endif 