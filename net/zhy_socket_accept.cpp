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

//�����ӽ��봦����
void CSocket::zhy_event_accept(lpzhy_connection_t oldc){
    struct sockaddr mysockaddr;
    socklen_t socklen;
    int err;
    int level;
    int s;
    static int use_accept4=-1;
    lpzhy_connection_t newc;

    socklen=sizeof(mysockaddr);
    do{
        if(use_accept4){
            //accept4��÷���������
            s=accept4(oldc->fd,&mysockaddr,&socklen,SOCK_NONBLOCK);
        }else{
            s=accept(oldc->fd,&mysockaddr,&socklen);
        }

        if(s==-1){
            err=errno;
            if(err==EAGAIN){
                return;
            }
            level=ZHY_LOG_ALERT;
            if(err==ECONNABORTED){
                //��������������ֹ���ͻ��˷���RST��������
                level=ZHY_LOG_ERR;
            }else if(err==EMFILE || err==ENFILE){       //����fd����
                level=ZHY_LOG_CRIT;
            }

            if(use_accept4 && err==ENOSYS){     //û��accept4����
                use_accept4=0;
                continue;
            }
            if(err==ECONNABORTED){
            }
            return;
        }
        
        if(m_onlineUserCount>=m_worker_connections){        //�û����ӹ���
            zhy_log_stderr( 0, "����ϵͳ�������������û���(�������������%d)���ر���������(%d)��", m_worker_connections, s);
            close(s);
            return;
        }

        if(m_connectionList.size() > (m_worker_connections*5)){
            if(m_freeconnectionList.size()<m_worker_connections){
                close(s);
                return;
            }
        }

        newc=zhy_get_connection(s);
        if(newc==NULL){
            if(close(s)==-1){
                zhy_log_error_core(ZHY_LOG_ALERT,errno,"CSocket::zhy_event_accept()��close(%d)ʧ��!", s);
            }
            return;
        }
        //�ɹ���ȡ����
        memcpy(&newc->s_sockaddr,&mysockaddr,socklen);
        
        if(!use_accept4){
            if(setnonblocking(s)==false){
                zhy_close_connection(newc);
                return;
            }
        }

        newc->listening=oldc->listening;
        newc->rhandler=&CSocket::zhy_read_request_handler;
        newc->whandler=&CSocket::zhy_write_request_handler;

        //���뵽epoll���
        if(zhy_epoll_oper_event(s,EPOLL_CTL_ADD,EPOLLIN|EPOLLRDHUP,0,newc)==-1){
            //��������
            zhy_close_connection(newc);
            return;
        }
        ++m_onlineUserCount;
        break;
    }while(1);
    return;
}
