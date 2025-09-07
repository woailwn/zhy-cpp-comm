#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_func.h"
#include "zhy_global.h"
char** g_os_argv;         // 原始命令行参数数组
int g_os_argc;            // 启动参数个数
size_t g_argvneedmem = 0; // 启动参数内存大小
size_t g_envneedmem = 0;  // 相关环境变量总大小
char* gp_envmem = NULL;   // 环境变量内存新位置

pid_t zhy_pid=getpid();
pid_t zhy_parent;
int g_daemonize=0;

// int main(int argc,char* argv[]){

//     zhy_pid=getpid();
//     zhy_parent=getppid();

//     g_argvneedmem=0;
//         for (int i = 0; i < argc; ++i) {
//         g_argvneedmem += strlen(argv[i] + 1);
//     }
//     for (int i = 0; environ[i]; ++i) {
//         g_envneedmem += strlen(environ[i]) + 1;
//     }
//     g_os_argc=argc;
//     g_os_argv=(char**)argv;

//     zhy_log.fd=-1;
    
//     do{ //方便提前退出，释放资源
//         CConfig* p_config=CConfig::getInstance();
//         if(p_config->Load("zhy_nginx.conf")){
//             zhy_log_init();
//             zhy_log_stderr(0,"配置文件[%s]载入失败,退出! ","zhy_nginx.conf");

//         }
//     }while(false);
// }