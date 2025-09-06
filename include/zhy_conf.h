#ifndef __ZHY_CONF_H__
#define __ZHY_CONF_H__
#include <vector>
#include "zhy_global.h"

//���������ļ���
class CConfig{
    public:
        ~CConfig();
        static CConfig* getInstance(){
            static CConfig m_instance;
            return &m_instance;
        }

        //���������ļ�
        bool Load(const char* m_filename);

        //��ȡ�����ļ���
        const char* GetString(const char* p_itemname);

        //��ȡ������������(�ṩĬ��ֵ)
        int GetIntDefault(const char* p_itemname,const int defaultvalue);

        // struct CG{
        //     ~CG(){
        //         if(CConfig::m_instance){
        //             delete CConfig::m_instance;
        //             CConfig::m_instance=nullptr;
        //         }
        //     }
        // };
    private:
        CConfig();
        static CConfig* m_instance;
        std::vector<LPCConfItem> m_ConfigItemList;  //������洢
};
#endif