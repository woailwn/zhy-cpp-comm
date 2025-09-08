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
char** g_os_argv;         // ԭʼ�����в�������
int g_os_argc;            // ������������
size_t g_argvneedmem = 0; // ���������ڴ��С
size_t g_envneedmem = 0;  // ��ػ��������ܴ�С
char* gp_envmem = NULL;   // ���������ڴ���λ��

pid_t zhy_pid;            //�ӽ���id
pid_t zhy_parent;         //������id
int g_daemonize=0;        //�Ƿ����ػ����̷�ʽ����
int zhy_process;      // ��������
sig_atomic_t hps_reap;    //��ʶ�ӽ���״̬�仯

int main(int argc,char* argv[]){
    int exit_code=0; //0���� 1 -1 �쳣 2 �Ҳ����ļ�

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
    
    do{ //������ǰ�˳����ͷ���Դ
        CConfig* p_config=CConfig::getInstance();
        if(p_config->Load("zhy_nginx.conf")==false){
            zhy_log_init();
            zhy_log_stderr(0,"�����ļ�[%s]����ʧ��,�˳�! ","zhy_nginx.conf");
            exit_code=2;
            break;
        }

        zhy_log_init();

        zhy_init_setproctitle();
    }while(false);

    zhy_log_stderr(0,"�����˳�");
    free_resource();
    return exit_code;
}

void free_resource(){
    //�����ƶ����������ڴ�
    if(gp_envmem){
        delete [] gp_envmem;
        gp_envmem=NULL;
    }

    //�ر���־�ļ�
    if(zhy_log.fd!=STDERR_FILENO && zhy_log.fd!=-1){
        close(zhy_log.fd);
        zhy_log.fd=-1;
    }
    return;
}