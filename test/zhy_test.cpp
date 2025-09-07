#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "zhy_func.h"
#include "zhy_conf.h"
// char** g_os_argv;         // 原始命令行参数数组
// int g_os_argc;            // 启动参数个数
// size_t g_argvneedmem = 0; // 启动参数内存大小
// size_t g_envneedmem = 0;  // 相关环境变量总大小
// char* gp_envmem = NULL;   // 环境变量内存新位置

//测试配置
void test_conf(){
    CConfig::getInstance()->Load("../../zhy_nginx.conf");
    printf("ListenPort=%d\n",CConfig::getInstance()->GetIntDefault("ListenPort",1000));

    printf("ListenIp=%s\n",CConfig::getInstance()->GetString("ListenIp"));
    printf("ProcName=%s\n",CConfig::getInstance()->GetString("ProcName"));
}

//测试环境变量地址
void test_environ(){
    for(int i=0;environ[i];i++){
        printf("environ[%d]地址=%x\n",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]内容=%s\n",i,environ[i]);
    }
    printf("------------------------------------");
    zhy_init_setproctitle();
    for(int i=0;environ[i];i++){
        printf("environ[%d]地址=%x\n",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]内容=%s\n",i,environ[i]);
    }
    zhy_setproctitle("nginx:master process");
    while(true){sleep(3);}
}

//日志测试
void test_log(){
    CConfig* p_config=CConfig::getInstance();
    if(p_config->Load("../../zhy_nginx.conf")==false){
        zhy_log_init();
        zhy_log_stderr(0,"配置文件[%s]载入失败,退出! ","zhy_nginx.conf");
    }
    zhy_log_init();
    zhy_log_stderr(0,"配置文件[%s]载入成功!","zhy_nginx.conf");
}

int main(int argc,char* argv[]){
    test_conf();


    // g_argvneedmem = 0;
    // for (int i = 0; i < argc; ++i) {
    //     g_argvneedmem += strlen(argv[i] + 1);
    // }
    // for (int i = 0; environ[i]; ++i) {
    //     g_envneedmem += strlen(environ[i]) + 1;
    // }
    // g_os_argc = argc;
    // g_os_argv = (char**)argv;
    // test_environ();

    test_log();
}