#include <stdio.h>
#include <unistd.h>

#include "zhy_func.h"
#include "zhy_conf.h"
//��������
void test_conf(){
    CConfig::getInstance()->Load("../zhy_nginx.conf");
    printf("ListenPort=%d\n",CConfig::getInstance()->GetIntDefault("ListenPort",1000));

    printf("ListenIp=%s\n",CConfig::getInstance()->GetString("ListenIp"));
    printf("ProcName=%s\n",CConfig::getInstance()->GetString("ProcName"));
}

//���Ի���������ַ

//��־����
void test_log(){
    CConfig* p_config=CConfig::getInstance();
    if(p_config->Load("../zhy_nginx.conf")==false){
        zhy_log_stderr(0,"�����ļ�[%s]����ʧ��!");
        return;
    }
    
    // ��ʼ����־ϵͳ
    zhy_log_init();
    
    // д��һ��������־
    zhy_log_stderr(0,"��־ϵͳ��ʼ���ɹ���������־���!");
}
void test_environ(){
    for(int i=0;environ[i];i++){
        printf("environ[%d]��ַ=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]����=%s\n",i,environ[i]);
    }
    printf("------------------------------------");
    zhy_init_setproctitle();
    for(int i=0;environ[i];i++){
        printf("environ[%d]��ַ=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]����=%s\n",i,environ[i]);
    }
}

int main(){
    // ��ʼ����־ϵͳ
    test_log();
    
    // �������Ժ����������������
    // test_conf();
    // test_environ();
}