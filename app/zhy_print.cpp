#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"

static char* zhy_sprintf_num(char* buf,char* last,uint64_t ui64,char defaultv,uintptr_t isHex,uintptr_t width);

//提供了缓冲区的结束位置
char* zhy_snprintf(char* buf, size_t max, const char* fmt, ...)
{
    char* p;
    va_list args;
    
    va_start(args,fmt);
    p=zhy_vslprintf(buf,buf+max,fmt,args);
    va_end(args);
    return p;
}

//封装zhy_vslprintf
char* zhy_slprintf(char* buf, char* last, const char* fmt, ...)
{
    va_list args;
    char* p;

    va_start(args,fmt); //args指向起始的参数
    p=zhy_vslprintf(buf,last,fmt,args);
    va_end(args);
    return p;
}


/// @brief 格式化输出
/// @param buf 存放数据
/// @param last 
/// @param fmt 可变参数
/// @param args %d %xd %s %f %p
/// @return char*
char* zhy_vslprintf(char* buf, char* last, const char* fmt, va_list args)
{
    char defaultv; //默认填充符
    uintptr_t width,sign,hex,frac_width,scale,n;

    int64_t d;    //%d对应的参数
    uint64_t ud;  //%ud 对应的可变参数
    char* p;      //%s
    double f;     //%f
    uint64_t frac;//根据%.2f取得后两位

    while(*fmt && buf<last){
        if(*fmt=='%'){ //%d %s...
            defaultv=(char)((*++fmt=='0')?'0':' '); //%05d

            sign=1;     //1 有符号 0 无符号(%u)
            width=0;    //格式化输出字符串的长度
            hex=0;      //是否十六进制
            frac_width=0;//小数点的数字 %.5f frac_width:5
            d=0;
            ud=0;
            
            //%16d  循环处理多位数字，累加
            while(*fmt>='0' && *fmt<='9'){
                width=width*10+(*fmt++-'0');
            }

            //处理特殊符号，u，X,x,'.'
            while(true){
                switch(*fmt){
                    case 'u':
                        sign=0;
                        fmt++;
                        continue;
                    case 'X':
                        hex=2;//大写16进制
                        sign=0;
                        fmt++;
                        continue;
                    case 'x':
                        hex=1;//小写16进制
                        sign=0;
                        fmt++;
                        continue;
                    case '.':  //%.10f
                        fmt++;
                        while(*fmt>='0' && *fmt<='9'){
                            frac_width=frac_width*10+(*fmt++-'0');
                        }
                        continue;
                        break;
                    default:
                        break;
                }
                break;
            }


            //处理具体格式字符
            switch(*fmt){
                case '%'://%% 输出一个%
                    *buf++='%';
                    fmt++;
                    continue;
                case 'd':
                    if (sign) d=(int64_t)va_arg(args,int);
                    else ud=(uint64_t)va_arg(args,u_int);
                    break;
                case 'i':
                    if (sign) d=(int64_t)va_arg(args,intptr_t);
                    else ud=(uint64_t)va_arg(args,uintptr_t);
                    break;
                case 'L':
                    if (sign) d=(int64_t)va_arg(args,int64_t);
                    else ud=(uint64_t)va_arg(args,uint64_t);
                    break;
                case 'p':
                    ud=(uintptr_t)va_arg(args,void*);
                    hex=2; //大写
                    sign=0;
                    defaultv='0';
                    width=2*sizeof(void*);
                    break;
                case 's':
                    p=va_arg(args,char*);
                    while(*p && buf<last){
                        *buf++=*p++;
                    }
                    fmt++;
                    continue;
                case 'f':
                    f=va_arg(args,double);
                    if(f<0){*buf++='-';f=-f;}
                    ud=(int64_t)f; //整数部分
                    frac=0;        //小数部分
                    if(frac_width){
                        scale=1;
                        for(n=frac_width;n;n--) scale*=10;
                        frac=(uint64_t)((f-(double)ud)*scale+0.5); //四舍五入
                        if(frac==scale){ud++;frac=0;}//四舍五入进位  12.999->13  0.999*100(保留两位)+0.5=100.4->100
                    }
                    //存放正整数部分
                    buf=zhy_sprintf_num(buf,last,ud,defaultv,0,width);
                    //存放小数部分
                    if(frac_width){*buf++='.';buf=zhy_sprintf_num(buf,last,frac,'0',0,frac_width);}
                    fmt++;
                    continue;
                default:
                    *buf++=*fmt++;
                    continue;
            }

            //存放%d内容
            if(sign){
                if(d<0){
                    *buf='-';
                    ud=(uint64_t)-d;
                }else{
                    ud=(uint64_t)d;
                }
            }
            buf = zhy_sprintf_num(buf, last, ud, defaultv, hex, width);
            fmt++;
        }else{
            *buf++=*fmt++;
        }
    }
    return buf;
}

/// @brief 格式化输出数字
/// @param buf     数字存放位置
/// @param last    数据不能超过last
/// @param ui64    显示的数字
/// @param defaultv 数值长度不足指定长度，填充的字符
/// @param isHex   是否以16进制输出
/// @param width   输出的长度
/// @return 
static char* zhy_sprintf_num(char* buf,char* last,uint64_t ui64,char defaultv,uintptr_t isHex,uintptr_t width)
{
    char *p,temp[ZHY_INT64_LEN+1];
    size_t len;
    uint32_t ui32;

    static char hex[]="0123456789abcdef"; //%xd
    static char HEX[]="0123456789ABCDEF"; //%Xd
    
    p=temp+ZHY_INT64_LEN; //指向最后一个元素位置

    if(isHex==0){ //非16进制显示
        //如果数字<=32位，uint32_t更快
        if(ui64<=(uint64_t)ZHY_MAX_UINT32_VALUE){
            ui32=(uint32_t)ui64;
            do{ //1234567 => [          1234567]
                *--p=(char)(ui32%10 + '0');
            }while(ui32/=10);
        }else{
            do{
                *--p=(char)(ui64%10+'0');
            }while(ui64/=10);
        }
    }else if(isHex==1){ //小写16进制
        do{
            *--p=hex[(uint32_t)(ui64 & 0xf)];
        }while(ui64>>=4);
    }else{
         do{
            *--p=HEX[(uint32_t)(ui64 & 0xf)];
        }while(ui64>>=4);
    }
    len=(temp+ZHY_INT64_LEN)-p; //转换后数字的长度
    while(len++<width && buf<last){
        *buf++=defaultv;
    }

    //如果够全拷贝，如果不够能拷贝多少就拷贝多少
    len=(temp+ZHY_INT64_LEN)-p; //还原
    if(buf+len>=last){
        len=last-buf;
    }
    return zhy_memcpy(buf,p,len);
}