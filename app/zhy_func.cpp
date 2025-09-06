#include "zhy_func.h"
#include <stdio.h>
#include <string.h>

//去除尾部空格
void Rtrim(char* string)
{
    if(string==NULL){
        return;
    }
    size_t len=strlen(string);
    while(len>0 && string[len-1]==' '){
        string[--len]=0;
    }
    return;
}

void Ltrim(char* string)
{
     if (!string) return;

    char* p = string;
    while (*p == ' ') p++; // 找到第一个非空格

    if (p != string) {         // 有前导空格才需要移动
        char* dst = string;
        while (*p) {
            *dst++ = *p++;
        }
        *dst = '\0';
    }
}