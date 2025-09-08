#ifndef __ZHY_GLOBAL_H__
#define __ZHY_GLOBAL_H__
#include <signal.h>


//配置项
typedef struct ConfItem{
    char ItemName[50];
    char ItemContent[500];

}CConfItem,*LPCConfItem;

typedef struct{
    int level;  //日志级别
    int fd;     //日志文件描述符
} zhy_log_t;

extern char** g_os_argv;         // main 函数参数 argv
extern int g_os_argc;            // 启动参数个数
extern size_t g_envneedmem;      // 相关环境变量总大小
extern size_t g_argvneedmem;      // 启动参数内存大小
extern char* gp_envmem;          // 环境变量内存新位置
extern int g_daemonize;          // 是否开启守护进程


extern pid_t zhy_pid;         // 当前进程 id
extern pid_t zhy_parent;      // 当前进程父进程id
extern zhy_log_t zhy_log;     // 日志相关信息
extern int zhy_process;       // 标识进程类型
extern sig_atomic_t zhy_reap; // 标识子进程状态变化
#endif