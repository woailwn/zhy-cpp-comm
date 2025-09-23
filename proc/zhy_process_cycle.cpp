#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_func.h"
#include "zhy_macro.h"

// master 创建 worker 子进程，并初始化子进程
static void zhy_start_worker_processes(int threadnums);
static int zhy_spawn_process(int threadnums, const char* pprocname);
static void zhy_worker_process_cycle(int inum, const char* pprocname);
static void zhy_worker_process_init(int inum);

static char master_process_name[]="master process";

void zhy_master_process_cycle(){
    sigset_t set;
    sigemptyset(&set);

    // 屏蔽以下信号，防止执行代码时被信号中断
    //如创建子进程时被中断而无法成功创建
    sigaddset(&set, SIGCHLD);  // 子进程状态改变
    sigaddset(&set, SIGALRM);  // 定时器超时
    sigaddset(&set, SIGIO);    // 异步I/O
    sigaddset(&set, SIGINT);   // 终端中断符
    sigaddset(&set, SIGHUP);   // 连接断开
    sigaddset(&set, SIGUSR1);  // 用户定义信号
    sigaddset(&set, SIGUSR2);  // 用户定义信号
    sigaddset(&set, SIGWINCH); // 终端窗口大小改变
    sigaddset(&set, SIGTERM);  // 终止
    sigaddset(&set, SIGQUIT);  // 终端退出符

    //一次性屏蔽这些信号
    if(sigprocmask(SIG_BLOCK,&set,NULL)==-1){
        zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_master_process_cycle中的sigprocmask执行失败!");
    }

    //修改master进程标题
    size_t size;
    size=sizeof(master_process_name);
    size+=g_argvneedmem;
    if(size<1000){
        char title[1000]={0};
        strcpy(title,(const char*)master_process_name);
        zhy_setproctitle(title);
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"%s %P [master] 进程启动并执行......",title,zhy_pid);
    }

    CConfig* p_config=CConfig::getInstance();
    //获取工作线程数
    int workprocess=p_config->GetIntDefault("WorkerProcesses",1);

    //开启线程
    zhy_start_worker_processes(workprocess);

    sigemptyset(&set);
    /*
        暂停(阻塞到sigsuspend调用行)
        指定新的屏蔽信号，暂时取代旧的屏蔽信号
    */
    for(;;){
        sigsuspend(&set);  //收到信号，恢复之前的信号屏蔽，执行相应的信号处理程序，处理完后，sigsuspend返回，继续向下执行
        sleep(1);
    }
    return;
}

static void zhy_start_worker_processes(int threadnums)
{
    for(int i=0;i<threadnums;i++){
        zhy_spawn_process(i,"worker process");
    }
    return;
}

//创建一个具体子进程
static int zhy_spawn_process(int threadnums, const char* pprocname)
{
    pid_t pid;
    pid=fork();

    switch(pid){
        case -1:
            zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_spawn_process() fork()产生子进程num=%d,name=\"%s\"失败!",i,pprocname);
            return -1;
        case 0://子进程分支
            zhy_parent=zhy_pid;
            zhy_pid=getpid();
            zhy_worker_process_cycle(threadnums,pprocname);//子进程业务
            break;
        default:
            break;
    }
    return pid;
}

static void zhy_worker_process_cycle(int inum, const char* pprocname){
    zhy_process=ZHY_PROCESS_WORKER;

    zhy_worker_process_init(inum);
    zhy_setproctitle(pprocname);
    zhy_log_error_core(ZHY_LOG_NOTICE,0,"%s %P [master] 进程启动并执行......!",pprocname,zhy_pid);

    for(;;){
        zhy_process_events_and_timers();    //子进程主逻辑
    }
    
    //回收资源
    g_threadpool.StopAll();
    g_socket.Shutdown_subproc();
    return;
}

static void zhy_worker_process_init(int inum){
    //信号，配置文件读取
    sigset_t set;
    sigemptyset(&set);

    if(sigprocmask(SIG_SETMASK,&set,NULL)==-1){
        //取消信号屏蔽
        zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_worker_process_init()中sigprocmask()失败！");
    }

    //线程池
    CConfig* p_config=CConfig::getInstance();
    int tmpthreadsnums=p_config->GetIntDefault("ProMsgRecvWorkThreadCount",5);
    if(g_threadpool.Create(tmpthreadsnums)==false){
        exit(-2);
    }
    if(g_socket.Initialize_subproc()==false){
        exit(-2);
    }
    g_socket.zhy_epoll_init();
}