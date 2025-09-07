#include <stdio.h>
#include <unistd.h>

#include "zhy_func.h"
#include "zhy_conf.h"
//测试配置
void test_conf(){
    CConfig::getInstance()->Load("../zhy_nginx.conf");
    printf("ListenPort=%d\n",CConfig::getInstance()->GetIntDefault("ListenPort",1000));

    printf("ListenIp=%s\n",CConfig::getInstance()->GetString("ListenIp"));
    printf("ProcName=%s\n",CConfig::getInstance()->GetString("ProcName"));
}

//测试环境变量地址

//日志测试
void test_log(){
    CConfig* p_config=CConfig::getInstance();
    if(p_config->Load("../zhy_nginx.conf")==false){
        zhy_log_stderr(0,"配置文件[%s]载入失败!");
        return;
    }
    
    // 初始化日志系统
    zhy_log_init();
    
    // 写入一条测试日志
    zhy_log_stderr(0,"日志系统初始化成功，测试日志输出!");
}
void test_environ(){
    for(int i=0;environ[i];i++){
        printf("environ[%d]地址=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]内容=%s\n",i,environ[i]);
    }
    printf("------------------------------------");
    zhy_init_setproctitle();
    for(int i=0;environ[i];i++){
        printf("environ[%d]地址=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]内容=%s\n",i,environ[i]);
    }
}

int main(){
    // 初始化日志系统
    test_log();
    
    // 其他测试函数可以在这里调用
    // test_conf();
    // test_environ();
}