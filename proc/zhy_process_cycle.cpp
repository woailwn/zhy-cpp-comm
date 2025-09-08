#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_func.h"
#include "zhy_macro.h"

// master ���� worker �ӽ��̣�����ʼ���ӽ���
static void zhy_start_worker_processes(int threadnums);
static int zhy_spawn_process(int threadnums, const char* pprocname);
static void zhy_worker_process_cycle(int inum, const char* pprocname);
static void zhy_worker_process_init(int inum);

static char master_process_name[]="master process";

void zhy_master_process_cycle(){
    sigset_t set;
    sigemptyset(&set);

    // ���������źţ���ִֹ�д���ʱ���ź��ж�
    //�紴���ӽ���ʱ���ж϶��޷��ɹ�����
    sigaddset(&set, SIGCHLD);  // �ӽ���״̬�ı�
    sigaddset(&set, SIGALRM);  // ��ʱ����ʱ
    sigaddset(&set, SIGIO);    // �첽I/O
    sigaddset(&set, SIGINT);   // �ն��жϷ�
    sigaddset(&set, SIGHUP);   // ���ӶϿ�
    sigaddset(&set, SIGUSR1);  // �û������ź�
    sigaddset(&set, SIGUSR2);  // �û������ź�
    sigaddset(&set, SIGWINCH); // �ն˴��ڴ�С�ı�
    sigaddset(&set, SIGTERM);  // ��ֹ
    sigaddset(&set, SIGQUIT);  // �ն��˳���

    //һ����������Щ�ź�
    if(sigprocmask(SIG_BLOCK,&set,NULL)==-1){
        zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_master_process_cycle�е�sigprocmaskִ��ʧ��!");
    }

    //�޸�master���̱���
    size_t size;
    size=sizeof(master_process_name);
    size+=g_argvneedmem;
    if(size<1000){
        char title[1000]={0};
        strcpy(title,(const char*)master_process_name);
        zhy_setproctitle(title);
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"%s %P [master] ����������ִ��......",title,zhy_pid);
    }

    CConfig* p_config=CConfig::getInstance();
    //��ȡ�����߳���
    int workprocess=p_config->GetIntDefault("WorkerProcesses",1);

    //�����߳�
    zhy_start_worker_processes(workprocess);

    sigemptyset(&set);
    /*
        ��ͣ(������sigsuspend������)
        ָ���µ������źţ���ʱȡ���ɵ������ź�
    */
    for(;;){
        sigsuspend(&set);  //�յ��źţ��ָ�֮ǰ���ź����Σ�ִ����Ӧ���źŴ�����򣬴������sigsuspend���أ���������ִ��
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

//����һ�������ӽ���
static int zhy_spawn_process(int threadnums, const char* pprocname)
{
    pid_t pid;
    pid=fork();

    switch(pid){
        case -1:
            zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_spawn_process() fork()�����ӽ���num=%d,name=\"%s\"ʧ��!",i,pprocname);
            return -1;
        case 0://�ӽ��̷�֧
            zhy_parent=zhy_pid;
            zhy_pid=getpid();
            zhy_worker_process_cycle(threadnums,pprocname);//�ӽ���ҵ��
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
    zhy_log_error_core(ZHY_LOG_NOTICE,0,"%s %P [master] ����������ִ��......!",pprocname,zhy_pid);

    for(;;){
        zhy_process_events_and_timers();    //�ӽ������߼�
    }
    
    //������Դ
    return;
}

static void zhy_worker_process_init(int inum){
    //�źţ������ļ���ȡ
    sigset_t set;
    sigemptyset(&set);

    if(sigprocmask(SIG_SETMASK,&set,NULL)==-1){
        //ȡ���ź�����
        zhy_log_error_core(ZHY_LOG_ALERT,errno,"zhy_worker_process_init()��sigprocmask()ʧ�ܣ�");
    }

    //...
}