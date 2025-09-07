#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"
//zhy_log_stderr(err,fmt,"...",...)
//zhy_log_stderr(0,"invalid option: %s , %d","testInfo",326)
zhy_log_t zhy_log; //全局日志信息

static char err_levels[][20] = {
    {"stderr"}, // 0：控制台错误
    {"emerg"},  // 1：紧急
    {"alert"},  // 2：警戒
    {"crit"},   // 3：严重
    {"error"},  // 4：错误
    {"warn"},   // 5：警告
    {"notice"}, // 6：注意
    {"info"},   // 7：信息
    {"debug"}   // 8：调试
};

void zhy_log_stderr(int err,const char* fmt,...)
{
    va_list args;
    char errstr[ZHY_MAX_ERROR_STR+1];
    char* p,*last;
    memset(errstr,0,sizeof(errstr));

    last=errstr+ZHY_MAX_ERROR_STR;  //指向最后一个可写位置
    p=zhy_memcpy(errstr,"ZhyNginx: ",10);

    va_start(args,fmt);
    p=zhy_vslprintf(p,last,fmt,args);
    va_end(args);

    if(err){
        p=zhy_log_errno(p,last,err);
    }

    if(p>=(last-1)){
        p=(last-1)-1;
    }
    *p++='\n';

    write(STDERR_FILENO,errstr,p-errstr);
    if(zhy_log.fd>STDERR_FILENO)
    {
        //打开日志文件
        err=0;
        p--;
        *p=0;
        zhy_log_error_core(ZHY_LOG_STDERR,err,(const char*)errstr);
    }
    return;
}

//写日志文件
void zhy_log_error_core(int level,int err,const char* fmt,...){
    char* last;
    char errstr[ZHY_MAX_ERROR_STR+1];
    memset(errstr,0,sizeof(errstr));

    last=errstr+ZHY_MAX_ERROR_STR;
    struct timeval tv;  //秒和微秒
    struct tm tm; //本地时间
    time_t sec;
    char* p;
    va_list args;
    memset(&tv,0,sizeof(struct timeval));
    memset(&tm,0,sizeof(struct tm));
    gettimeofday(&tv,NULL); //获取当前时间，s+ms
    sec=tv.tv_sec;
    localtime_r(&sec,&tm); //本地时间
    tm.tm_mon++;    //月份从0开始
    tm.tm_year+=1900;
    
    char timestr[40]={0};
    zhy_slprintf(timestr,(char*)-1,
    "%4d/%02d/%02d %02d:%02d:%02d",
    tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    p=zhy_memcpy(errstr,timestr,strlen((const char*)timestr));
    p=zhy_slprintf(p,last," [%s] ",err_levels[level]);
    p=zhy_slprintf(p,last,"%P: ",zhy_pid);

    va_start(args,fmt);
    p=zhy_vslprintf(p,last,fmt,args);
    va_end(args);

    if(err){
        p=zhy_log_errno(p,last,err);
    }
    if(p>=(last-1)){
        p=(last-1)-1;
    }
    *p++='\n';

    ssize_t n;
    do{
        //只记录等级小的
        if(level>zhy_log.level){
            break;
        }

        //写日志文件
        n=write(zhy_log.fd,errstr,p-errstr);
        if(n==-1){
            if(errno==ENOSPC){
            }else{
                if(zhy_log.fd!=STDERR_FILENO){
                    n=write(STDERR_FILENO,errstr,p-errstr);
                }
            }
        }
    }while(false);
    return;
}

//由错误码获取错误信息
char* zhy_log_errno(char* buf, char* last, int err){
    char*  errorinfo=strerror(err);
    size_t len=strlen(errorinfo);

    //组合输出模式
    char lstr[10]={0};
    sprintf(lstr," (%d: ",err);
    size_t llen=strlen(lstr);

    char rstr[]=") ";
    size_t rlen=strlen(rstr);

    size_t helpstr=llen+rlen;
    if((buf+helpstr+len)<last){
        buf=zhy_memcpy(buf,lstr,llen);
        buf=zhy_memcpy(buf,errorinfo,len);
        buf=zhy_memcpy(buf,rstr,rlen);
    }
    return buf;
}

//初始化日志文件
void zhy_log_init(){
    char* log_filename=NULL;
    
    CConfig* p_config=CConfig::getInstance();
    log_filename=(char*)p_config->GetString("Log");
    if(log_filename){
        log_filename=(char*)ZHY_ERROR_LOG_PATH;
    }

    //缺省日志等级
    zhy_log.level=p_config->GetIntDefault("LogLevel",ZHY_LOG_NOTICE);

    zhy_log.fd=open((const char*)log_filename,O_WRONLY|O_APPEND|O_CREAT,0644);
    if(zhy_log.fd==-1){
        zhy_log_stderr(errno,"[alert] could not open error log to log file:open() \"%s\" failed",log_filename);
        zhy_log.fd=STDERR_FILENO; //定位错误输出
    }
    return;
}
