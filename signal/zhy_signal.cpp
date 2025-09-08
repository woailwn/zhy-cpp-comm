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

//�źŽṹ��
typedef struct{
    int signo;              //���
    const char* signame;    //����

    //�źŴ�����
    void (*handler)(int signo,siginfo_t* siginfo,void* ucontext);
}zhy_signal_t;


//�źŴ�����
static void zhy_signal_handler(int signo,siginfo_t* siginfo,void* ucontext);

// ��ȡ�ӽ��̵Ľ���״̬����ֹ����kill�ӽ���ʱ�ӽ��̱�ɽ�ʬ����(�����̴���SIGCHILD�ź�)
static void zhy_process_get_status(void);

//���������ź�
zhy_signal_t signals[] = {
    {SIGHUP, "SIGHUP", zhy_signal_handler},
    {SIGINT, "SIGINT", zhy_signal_handler},
    {SIGTERM, "SIGTERM", zhy_signal_handler},
    {SIGCHLD, "SIGCHLD", zhy_signal_handler}, // �ӽ����˳�ʱ�������̻��յ����ź�
    {SIGQUIT, "SIGQUIT", zhy_signal_handler},
    {SIGIO, "SIGIO", zhy_signal_handler},     // ͨ���첽I/O�ź�
    {SIGSYS, "SIGSYS, SIG_IGN", NULL},        // ��Чϵͳ���ã������Ըý��̻ᱻ����ϵͳkill��
    {0, NULL, NULL}                           // ������
};

//ע���ź�
int zhy_init_signals(){
    zhy_signal_t* sig;
    struct sigaction sa; 

    for(sig=signals;sig->signo!=0;++sig)
    {
        memset(&sa,0,sizeof(struct sigaction));

        if(sig->handler){
            sa.sa_sigaction=sig->handler;
            sa.sa_flags=SA_SIGINFO; //ʹ�źŴ�������Ч
        }else{
            sa.sa_handler=SIG_IGN;  //�����ź�
        }

        sigemptyset(&sa.sa_mask);   //����źż��ϣ���ʾ�������κ��ź�
        //���ź�
        if(sigaction(sig->signo,&sa,NULL)==-1){
            zhy_log_error_core(ZHY_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame);
            return -1;
        }
        return 0;
    }
}

//�źŴ�����
static void zhy_signal_handler(int signo,siginfo_t* siginfo,void* ucontext)
{
    zhy_signal_t* sig;
    char* action;

    for(sig=signals;sig->signo!=0;++sig){
        if(sig->signo==signo) break;
    }

    action=(char*)"";
    if(zhy_process==ZHY_PROCESS_MASTER){
        //master�߳�
        switch(signo){
            case SIGCHLD:   //�ӽ����˳�
                zhy_reap=1; //����ӽ���״̬
                break;

            default:
                break;
        }
    }else if(zhy_process==ZHY_PROCESS_WORKER){

    }else{

    }

    if(siginfo && siginfo->si_pid){ //�����źŵĽ���
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"signal %d (%s) received from %P%s",signo,sig->signame,siginfo->si_pid,action);
    }else{
        zhy_log_error_core(ZHY_LOG_NOTICE,0,"signal %d (%s) received %s",signo,sig->signame,action);
    }

    if(signo==SIGCHLD){
        zhy_process_get_status(); //�����ӽ���
    }
    return ;
}


//��ֹ�ӽ��̱�Ϊ��ʬ����
static void zhy_process_get_status(void)
{
    pid_t pid;
    int status;
    int err;
    int one=0;

    while(true){
        pid=waitpid(-1,&status,WNOHANG);  //����������˼���ȴ��κ��ӽ��̣�status�����ӽ���״̬��Ϣ����Ҫ��������������

        if(pid==0){//û���ռ����ӽ�����Ϣ
            return;
        }
        if(pid==-1){
            err=errno;
            if(err==EINTR){ //���ñ��ź��ж�
                continue;
            }
            if(err==ECHILD && one){ //��ǰ����û�пɵȴ����ӽ���
                return;
            }
            if(err==ECHILD){
                zhy_log_error_core(ZHY_LOG_INFO,err,"waitpid() failed!");
            }
            zhy_log_error_core(ZHY_LOG_ALERT,err,"waitpid() failed!");
            return;
        }
        //�ռ������ӽ���id��Ϣ
        one=1;
        //����ӽ��̱��ź�ɱ����
        if(WTERMSIG(status)){
            zhy_log_error_core(ZHY_LOG_ALERT,0,"pid=%P exited on signal %d!",pid,WTERMSIG(status));
        }else{  //�ӽ��������˳�
           zhy_log_error_core(ZHY_LOG_NOTICE,0,"pid=%P exited with code %d!",pid,WEXITSTATUS(status));
        } 
    }
    return;
}