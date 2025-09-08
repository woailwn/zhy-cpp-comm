#ifndef __ZHY_GLOBAL_H__
#define __ZHY_GLOBAL_H__
#include <signal.h>


//������
typedef struct ConfItem{
    char ItemName[50];
    char ItemContent[500];

}CConfItem,*LPCConfItem;

typedef struct{
    int level;  //��־����
    int fd;     //��־�ļ�������
} zhy_log_t;

extern char** g_os_argv;         // main �������� argv
extern int g_os_argc;            // ������������
extern size_t g_envneedmem;      // ��ػ��������ܴ�С
extern size_t g_argvneedmem;      // ���������ڴ��С
extern char* gp_envmem;          // ���������ڴ���λ��
extern int g_daemonize;          // �Ƿ����ػ�����


extern pid_t zhy_pid;         // ��ǰ���� id
extern pid_t zhy_parent;      // ��ǰ���̸�����id
extern zhy_log_t zhy_log;     // ��־�����Ϣ
extern int zhy_process;       // ��ʶ��������
extern sig_atomic_t zhy_reap; // ��ʶ�ӽ���״̬�仯
#endif