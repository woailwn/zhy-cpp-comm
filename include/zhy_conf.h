#ifndef __ZHY_CONF_H__
#define __ZHY_CONF_H__
#include <vector>
#include "zhy_global.h"

//处理配置文件类
class CConfig{
    public:
        ~CConfig();
        static CConfig* getInstance(){
            static CConfig m_instance;
            return &m_instance;
        }

        //加载配置文件
        bool Load(const char* m_filename);

        //获取配置文件项
        const char* GetString(const char* p_itemname);

        //获取数字型配置项(提供默认值)
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
        std::vector<LPCConfItem> m_ConfigItemList;  //配置项存储
};
#endif