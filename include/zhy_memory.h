#ifndef __ZHY_MEMORY_H__
#define __ZHY_MEMORY_H__

#include <stddef.h>

//ÄÚ´æ³Ø
class CMemory{
public:
    static CMemory* GetInstance(){
        if(m_instance==NULL){
            if(m_instance==NULL){
                m_instance=new CMemory();
                static CGarCollection gc;
            }
        }
        return m_instance;
    }
    
    class CGarCollection{
        public:
            ~CGarCollection(){
                if(CMemory::m_instance){
                    delete CMemory::m_instance;
                    CMemory::m_instance=NULL;
                }
            }
    };

    void* AllocMemory(int memCount,bool ifmemset);
    void* FreeMemory(void* point);
    ~CMemory(){};

private:
    CMemory() {}

    static CMemory* m_instance;
};
#endif