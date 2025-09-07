#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zhy_global.h"

/*
    �����������������͸ý�����ػ����������λ�������ڵģ����޸Ľ�����ʱ
    Ϊ�˱��⻷����������Ҫ�����������ƶ�
*/
//���·���һ���ڴ汣�滷������
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

//�޸Ľ�������
void zhy_setproctitle(const char* title){
    size_t new_title_len=strlen(title);
    size_t total_len=g_argvneedmem + g_envneedmem; //�½������Ʋ��ܳ������������ܺ�+��������
    if(total_len<=new_title_len){
        return; //̫����
    }

    g_os_argv[1]=NULL;

    char* tmp=g_os_argv[0];
    strcpy(tmp,title);
    tmp+=new_title_len;  //����������
    size_t invalid=total_len-new_title_len;
    memset(tmp,0,invalid);
    return;
}