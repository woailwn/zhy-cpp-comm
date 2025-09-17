#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_socket.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"


//socket�󶨵ĵ�ַת��Ϊ�ı���ʽ
//sa �Զ���Ϣ       text��ϵ��ַ���    return ��Ϣ�ַ�������
size_t CSocket::zhy_sock_ntop(struct sockaddr* sa, int port, char* text, size_t len){
    struct sockaddr_in * sin;
    char* p;
    
    switch(sa->sa_family){
        case AF_INET:
            sin = (struct sockaddr_in*)sa;
            p = (char*)&sin->sin_addr;
            if (port) {
                p = zhy_snprintf(text, len, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
            } else {
                p = zhy_snprintf(text, len, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
            }
        return (p - text);
        break;
        default:
        break;
    }
}