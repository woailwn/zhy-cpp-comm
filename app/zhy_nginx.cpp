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
char** g_os_argv;         // ԭʼ�����в�������
int g_os_argc;            // ������������
size_t g_argvneedmem = 0; // ���������ڴ��С
size_t g_envneedmem = 0;  // ��ػ��������ܴ�С
char* gp_envmem = NULL;   // ���������ڴ���λ��

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
    
//     do{ //������ǰ�˳����ͷ���Դ
//         CConfig* p_config=CConfig::getInstance();
//         if(p_config->Load("zhy_nginx.conf")){
//             zhy_log_init();
//             zhy_log_stderr(0,"�����ļ�[%s]����ʧ��,�˳�! ","zhy_nginx.conf");

//         }
//     }while(false);
// }