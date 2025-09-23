#include <stdio.h>
#include <unistd.h>

#include "zhy_func.h"
#include "zhy_conf.h"
//????
void test_conf(){
    CConfig::getInstance()->Load("../zhy_nginx.conf");
    printf("ListenPort=%d\n",CConfig::getInstance()->GetIntDefault("ListenPort",1000));

    printf("ListenIp=%s\n",CConfig::getInstance()->GetString("ListenIp"));
    printf("ProcName=%s\n",CConfig::getInstance()->GetString("ProcName"));
}

//????????
void test_environ(){
    for(int i=0;environ[i];i++){
        printf("environ[%d]??=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]??=%s\n",i,environ[i]);
    }
    printf("------------------------------------");
    zhy_init_setproctitle();
    for(int i=0;environ[i];i++){
        printf("environ[%d]??=%x",i,(unsigned int)((unsigned long)environ[i]));
        printf("environ[%d]??=%s\n",i,environ[i]);
    }
}

int main(){
    // ???????
    test_log();
    
    // ?????????????
    // test_conf();
    // test_environ();
}