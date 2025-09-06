#include "zhy_func.h"
#include <stdio.h>
#include <string.h>

//ȥ��β���ո�
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
    while (*p == ' ') p++; // �ҵ���һ���ǿո�

    if (p != string) {         // ��ǰ���ո����Ҫ�ƶ�
        char* dst = string;
        while (*p) {
            *dst++ = *p++;
        }
        *dst = '\0';
    }
}