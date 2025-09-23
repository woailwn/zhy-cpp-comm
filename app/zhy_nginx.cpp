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
#include "zhy_macro.h"
#include "zhy_threadpool.h"
#include "zhy_slogic.h"
char** g_os_argv;         // 原始命令行参数数组
int g_os_argc;            // 启动参数个数
size_t g_argvneedmem = 0; // 启动参数内存大小
size_t g_envneedmem = 0;  // 相关环境变量总大小
char* gp_envmem = NULL;   // 环境变量内存新位置

pid_t zhy_pid;            //子进程id
pid_t zhy_parent;         //父进程id
int g_daemonize=0;        //是否以守护进程方式运行
int zhy_process;      // 进程类型

CThreadPool g_threadpool;   //线程池
CLogicSocket g_socket;    // 全局 socket 管理

sig_atomic_t hps_reap;    //标识子进程状态变化
int main(int argc,char* argv[]){
    int exit_code=0; //0正常 1 -1 异常 2 找不到文件

    zhy_pid=getpid();
    zhy_parent=getppid();

    g_argvneedmem=0;
    for (int i = 0; i < argc; ++i) {
        g_argvneedmem += strlen(argv[i] + 1);
    }
    for (int i = 0; environ[i]; ++i) {
        g_envneedmem += strlen(environ[i]) + 1;
    }
    g_os_argc=argc;
    g_os_argv=(char**)argv;

    zhy_log.fd=-1;
    zhy_process=ZHY_PROCESS_MASTER;
    
    do{ //方便提前退出，释放资源
        CConfig* p_config=CConfig::getInstance();
        if(p_config->Load("zhy_nginx.conf")==false){
            zhy_log_init();
            zhy_log_stderr(0,"配置文件[%s]载入失败,退出! ","zhy_nginx.conf");
            exit_code=2;
            break;
        }

        zhy_log_init();

        zhy_init_setproctitle();
    }while(false);

    zhy_log_stderr(0,"程序退出");
    free_resource();
    return exit_code;
}

void free_resource(){
    //回收移动环境变量内存
    if(gp_envmem){
        delete [] gp_envmem;
        gp_envmem=NULL;
    }

    //关闭日志文件
    if(zhy_log.fd!=STDERR_FILENO && zhy_log.fd!=-1){
        close(zhy_log.fd);
        zhy_log.fd=-1;
    }
    return;
}