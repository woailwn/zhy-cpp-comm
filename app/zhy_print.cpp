#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zhy_func.h"
#include "zhy_global.h"
#include "zhy_macro.h"

static char* zhy_sprintf_num(char* buf,char* last,uint64_t ui64,char defaultv,uintptr_t isHex,uintptr_t width);

//�ṩ�˻������Ľ���λ��
char* zhy_snprintf(char* buf, size_t max, const char* fmt, ...)
{
    char* p;
    va_list args;
    
    va_start(args,fmt);
    p=zhy_vslprintf(buf,buf+max,fmt,args);
    va_end(args);
    return p;
}

//��װzhy_vslprintf
char* zhy_slprintf(char* buf, char* last, const char* fmt, ...)
{
    va_list args;
    char* p;

    va_start(args,fmt); //argsָ����ʼ�Ĳ���
    p=zhy_vslprintf(buf,last,fmt,args);
    va_end(args);
    return p;
}


/// @brief ��ʽ�����
/// @param buf �������
/// @param last 
/// @param fmt �ɱ����
/// @param args %d %xd %s %f %p
/// @return char*
char* zhy_vslprintf(char* buf, char* last, const char* fmt, va_list args)
{
    char defaultv; //Ĭ������
    uintptr_t width,sign,hex,frac_width,scale,n;

    int64_t d;    //%d��Ӧ�Ĳ���
    uint64_t ud;  //%ud ��Ӧ�Ŀɱ����
    char* p;      //%s
    double f;     //%f
    uint64_t frac;//����%.2fȡ�ú���λ

    while(*fmt && buf<last){
        if(*fmt=='%'){ //%d %s...
            defaultv=(char)((*++fmt=='0')?'0':' '); //%05d

            sign=1;     //1 �з��� 0 �޷���(%u)
            width=0;    //��ʽ������ַ����ĳ���
            hex=0;      //�Ƿ�ʮ������
            frac_width=0;//С��������� %.5f frac_width:5
            d=0;
            ud=0;
            
            //%16d  ѭ�������λ���֣��ۼ�
            while(*fmt>='0' && *fmt<='9'){
                width=width*10+(*fmt++-'0');
            }

            //����������ţ�u��X,x,'.'
            while(true){
                switch(*fmt){
                    case 'u':
                        sign=0;
                        fmt++;
                        continue;
                    case 'X':
                        hex=2;//��д16����
                        sign=0;
                        fmt++;
                        continue;
                    case 'x':
                        hex=1;//Сд16����
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


            //��������ʽ�ַ�
            switch(*fmt){
                case '%'://%% ���һ��%
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
                    hex=2; //��д
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
                    ud=(int64_t)f; //��������
                    frac=0;        //С������
                    if(frac_width){
                        scale=1;
                        for(n=frac_width;n;n--) scale*=10;
                        frac=(uint64_t)((f-(double)ud)*scale+0.5); //��������
                        if(frac==scale){ud++;frac=0;}//���������λ  12.999->13  0.999*100(������λ)+0.5=100.4->100
                    }
                    //�������������
                    buf=zhy_sprintf_num(buf,last,ud,defaultv,0,width);
                    //���С������
                    if(frac_width){*buf++='.';buf=zhy_sprintf_num(buf,last,frac,'0',0,frac_width);}
                    fmt++;
                    continue;
                default:
                    *buf++=*fmt++;
                    continue;
            }

            //���%d����
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

/// @brief ��ʽ���������
/// @param buf     ���ִ��λ��
/// @param last    ���ݲ��ܳ���last
/// @param ui64    ��ʾ������
/// @param defaultv ��ֵ���Ȳ���ָ�����ȣ������ַ�
/// @param isHex   �Ƿ���16�������
/// @param width   ����ĳ���
/// @return 
static char* zhy_sprintf_num(char* buf,char* last,uint64_t ui64,char defaultv,uintptr_t isHex,uintptr_t width)
{
    char *p,temp[ZHY_INT64_LEN+1];
    size_t len;
    uint32_t ui32;

    static char hex[]="0123456789abcdef"; //%xd
    static char HEX[]="0123456789ABCDEF"; //%Xd
    
    p=temp+ZHY_INT64_LEN; //ָ�����һ��Ԫ��λ��

    if(isHex==0){ //��16������ʾ
        //�������<=32λ��uint32_t����
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
    }else if(isHex==1){ //Сд16����
        do{
            *--p=hex[(uint32_t)(ui64 & 0xf)];
        }while(ui64>>=4);
    }else{
         do{
            *--p=HEX[(uint32_t)(ui64 & 0xf)];
        }while(ui64>>=4);
    }
    len=(temp+ZHY_INT64_LEN)-p; //ת�������ֵĳ���
    while(len++<width && buf<last){
        *buf++=defaultv;
    }

    //�����ȫ��������������ܿ������پͿ�������
    len=(temp+ZHY_INT64_LEN)-p; //��ԭ
    if(buf+len>=last){
        len=last-buf;
    }
    return zhy_memcpy(buf,p,len);
}