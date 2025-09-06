#include "zhy_conf.h"
#include <stdio.h>

//²âÊÔÍê³É
void test_conf(){
    CConfig::getInstance()->Load("../zhy_nginx.conf");
    printf("ListPort=%d\n",CConfig::getInstance()->GetIntDefault("ListenPort",1000));
}
int main(){
    test_conf();
}