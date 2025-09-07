#ifndef __ZHY_FUNC_H__
#define __ZHY_FUNC_H__
#include <stdarg.h>
#include <stddef.h>
//?????
void Rtrim(char* string);
void Ltrim(char* string);

//?????????????
void zhy_init_setproctitle();             // ????????
void zhy_setproctitle(const char* title); // ?????

//??
void zhy_log_init();                                            //???
void zhy_log_stderr(int err,const char* fmt,...);               //??
void zhy_log_error_core(int level,int err,const char* fmt,...); //?

//??????????
char* zhy_log_errno(char* buf, char* last, int err);

//???????
char* zhy_snprintf(char* buf, size_t max, const char* fmt, ...);
char* zhy_slprintf(char* buf, char* last, const char* fmt, ...);
char* zhy_vslprintf(char* buf, char* last, const char* fmt, va_list args);
#endif