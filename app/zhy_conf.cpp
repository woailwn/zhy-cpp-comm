#include "zhy_conf.h"
#include "zhy_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
CConfig* CConfig::m_instance=nullptr;


CConfig::~CConfig()
{
    for(auto* p_item:m_ConfigItemList){
        delete p_item;
    }
    // m_ConfigItemList.clear()
    // m_ConfigItemList.shrink_to_fit();
    std::vector<LPCConfItem>().swap(m_ConfigItemList);
}

CConfig::CConfig()
{
    
}
bool CConfig::Load(const char *m_filename)
{
    if(m_filename==nullptr){
        return false;
    }
    FILE* fp=fopen(m_filename,"r");
    if(fp==nullptr){
        return false;
    }

    char lines[500];
    while(!feof(fp)){
        if(fgets(lines,500,fp)==nullptr){ //读取一行
            continue;
        }
        if(lines[0]==0){ //空行
            continue;
        }

        //处理注释行
        if(*lines==';' || *lines==' ' || *lines=='#' || *lines=='\t' || *lines=='\n'){
            continue;
        }

        //处理末尾的换行，回车，空格
       while (strlen(lines) > 0) {
            if (lines[strlen(lines) - 1] == 10 || lines[strlen(lines) - 1] == 13 ||
                lines[strlen(lines) - 1] == 32) {
                lines[strlen(lines) - 1] = 0;
            } else {
                break;
            }
        }

        //处理完什么都没了(空行)
        if(lines[0]==0){
            continue;
        }
        
        if(*lines=='['){
            continue;
        }

        char* temp=strchr(lines,'=');
        if(temp!=nullptr){
            LPCConfItem conf_item=new CConfItem();
            memset(conf_item,0,sizeof(conf_item));
            strncpy(conf_item->ItemName,lines,(int)(temp-lines));
            strcpy(conf_item->ItemContent,temp+1);

            //去除空格
            Rtrim(conf_item->ItemName);
            Ltrim(conf_item->ItemName);
            Rtrim(conf_item->ItemContent);
            Ltrim(conf_item->ItemContent);

            m_ConfigItemList.push_back(conf_item);
        }
    }
    fclose(fp);
    return true;
}

const char *CConfig::GetString(const char *p_itemname)
{
    for(auto* p_item:m_ConfigItemList){
        if(strcasecmp(p_item->ItemName,p_itemname)==0){
            return p_item->ItemContent;
        }
    }
    return nullptr;
}

int CConfig::GetIntDefault(const char *p_itemname, const int defaultvalue)
{
    for(auto* p_item:m_ConfigItemList){
        if(strcasecmp(p_item->ItemName,p_itemname)==0){
            return atoi(p_item->ItemContent);
        }
    }
    return defaultvalue;
}
