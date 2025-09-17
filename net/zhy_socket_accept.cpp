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

//新连接接入处理函数
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
            //accept4获得非阻塞连接
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
                //软件引起的连接终止，客户端发送RST包，忽略
                level=ZHY_LOG_ERR;
            }else if(err==EMFILE || err==ENFILE){       //进程fd用完
                level=ZHY_LOG_CRIT;
            }

            if(use_accept4 && err==ENOSYS){     //没有accept4函数
                use_accept4=0;
                continue;
            }
            if(err==ECONNABORTED){
            }
            return;
        }
        
        if(m_onlineUserCount>=m_worker_connections){        //用户连接过多
            zhy_log_stderr( 0, "超出系统允许的最大连入用户数(最大允许连入数%d)，关闭连入请求(%d)。", m_worker_connections, s);
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
                zhy_log_error_core(ZHY_LOG_ALERT,errno,"CSocket::zhy_event_accept()中close(%d)失败!", s);
            }
            return;
        }
        //成功获取连接
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

        //加入到epoll监控
        if(zhy_epoll_oper_event(s,EPOLL_CTL_ADD,EPOLLIN|EPOLLRDHUP,0,newc)==-1){
            //立即回收
            zhy_close_connection(newc);
            return;
        }
        ++m_onlineUserCount;
        break;
    }while(1);
    return;
}
