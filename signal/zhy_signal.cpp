#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"

//信号结构体
typedef struct{
    int signo;              //编号
    const char* signame;    //名称

    //信号处理函数
    void (*handler)(int signo,siginfo_t* siginfo,void* ucontext);
}zhy_signal_t;


//信号处理函数
static void zhy_signal_handler(int signo,siginfo_t* siginfo,void* ucontext);

// 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程(父进程处理SIGCHILD信号)
static void zhy_process_get_status(void);

//处理的相关信号
zhy_signal_t signals[] = {
    {SIGHUP, "SIGHUP", zhy_signal_handler},
    {SIGINT, "SIGINT", zhy_signal_handler},
    {SIGTERM, "SIGTERM", zhy_signal_handler},
    {SIGCHLD, "SIGCHLD", zhy_signal_handler}, // 子进程退出时，父进程会收到此信号
    {SIGQUIT, "SIGQUIT", zhy_signal_handler},
    {SIGIO, "SIGIO", zhy_signal_handler},     // 通用异步I/O信号
    {SIGSYS, "SIGSYS, SIG_IGN", NULL},        // 无效系统调用，不忽略该进程会被操作系统kill掉
    {0, NULL, NULL}                           // 特殊标记
};

//注册信号
int zhy_init_signals(){
    zhy_signal_t* sig;
    struct sigaction sa; 

    for(sig=signals;sig->signo!=0;++sig)
    {
        memset(&sa,0,sizeof(struct sigaction));

        if(sig->handler){
            sa.sa_sigaction=sig->handler;
            sa.sa_flags=SA_SIGINFO; //使信号处理函数生效
        }else{
            sa.sa_handler=SIG_IGN;  //忽略信号
        }

        sigemptyset(&sa.sa_mask);   //清空信号集合，表示不阻塞任何信号
        //绑定信号
        if(sigaction(sig->signo,&sa,NULL)==-1){
            zhy_log_error_core(ZHY_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame);
            return -1;
        }
        return 0;
    }
}

//信号处理函数
static void zhy_signal_handler(int signo,siginfo_t* siginfo,void* ucontext)
{
    zhy_signal_t* sig;
    char* action;

    for(sig=signals;sig->signo!=0;++sig){
        if(sig->signo==signo) break;
    }

    action=(char*)"";
    if(zhy_process==ZHY_PROCESS_MASTER){
        //master线程
        switch(signo){
            case SIGCHLD:   //子进程退出
                zhy_reap=1; //标记子进程状态
                break;

            default:
                break;
        }
    }else if(zhy_process==ZHY_PROCESS_WORKER){

    }else{

    }

    if(siginfo && siginfo->si_pid){ //发送信号的进程
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"signal %d (%s) received from %P%s",signo,sig->signame,siginfo->si_pid,action);
    }else{
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"signal %d (%s) received %s",signo,sig->signame,action);
    }

    if(signo==SIGCHLD){
        zhy_process_get_status(); //处理子进程
    }
    return ;
}


//防止子进程变为僵尸进程
static void zhy_process_get_status(void)
{
    pid_t pid;
    int status;
    int err;
    int one=0;

    while(true){
        pid=waitpid(-1,&status,WNOHANG);  //三个参数意思：等待任何子进程，status保存子进程状态信息，不要阻塞，立即返回

        if(pid==0){//没有收集到子进程信息
            return;
        }
        if(pid==-1){
            err=errno;
            if(err==EINTR){ //调用被信号中断
                continue;
            }
            if(err==ECHILD && one){ //当前进程没有可等待的子进程
                return;
            }
            if(err==ECHILD){
                zhy_log_error_core(ZHY_LOG_INFO,err,"waitpid() failed!");
            }
            zhy_log_error_core(ZHY_LOG_ALERT,err,"waitpid() failed!");
            return;
        }
        //收集到了子进程id信息
        one=1;
        //如果子进程被信号杀死的
        if(WTERMSIG(status)){
            zhy_log_error_core(ZHY_LOG_ALERT,0,"pid=%P exited on signal %d!",pid,WTERMSIG(status));
        }else{  //子进程正常退出
           zhy_log_error_core(ZHY_LOG_NOTICE,0,"pid=%P exited with code %d!",pid,WEXITSTATUS(status));
        } 
    }
    return;
}