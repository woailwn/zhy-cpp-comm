#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zhy_global.h"

/*
    进程名及启动参数和该进程相关环境变量存放位置是相邻的，在修改进程名时
    为了避免环境变量，需要将环境变量移动
*/
//重新分配一块内存保存环境变量
void zhy_init_setproctitle(){
    gp_envmem=new char[g_envneedmem];
    memset(gp_envmem,0,g_envneedmem);

    char* tmp=gp_envmem;
    for(int i=0;environ[i];i++){
        size_t size=strlen(environ[i])+1;
        strcpy(tmp,environ[i]);
        environ[i]=tmp;
        tmp+=size;
    }
    return ;
}

//修改进程名称
void zhy_setproctitle(const char* title){
    size_t new_title_len=strlen(title);
    size_t total_len=g_argvneedmem + g_envneedmem; //新进程名称不能超过参数长度总和+环境变量
    if(total_len<=new_title_len){
        return; //太长了
    }

    g_os_argv[1]=NULL;

    char* tmp=g_os_argv[0];
    strcpy(tmp,title);
    tmp+=new_title_len;  //跳过进程名
    size_t invalid=total_len-new_title_len;
    memset(tmp,0,invalid);
    return;
}