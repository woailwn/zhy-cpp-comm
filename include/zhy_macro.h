#ifndef __ZHY_MACRO_H__
#define __ZHY_MACRO_H__

#define ZHY_MAX_ERROR_STR 2048 //显示错误信息最大数组长度

//方便多次复制，指向拷贝后的位置
#define zhy_memcpy(dst,src,n) (((char*)memcpy(dst,src,n)) + (n))
#define zhy_min(val1,val2)  ((val1 < val2) ? (val1) : (val2))

//数字
#define ZHY_MAX_UINT32_VALUE (uint32_t)0xffffffff 
#define ZHY_INT64_LEN (sizeof("-9223372036854775808") - 1)

//日志级别
#define ZHY_LOG_STDERR 0 // 控制台错误【stderr】日志不仅写文件，且尝试向标准输出输出错误信息
#define ZHY_LOG_EMERG 1  // 紧急 【emerg】
#define ZHY_LOG_ALERT 2  // 警戒 【alert】
#define ZHY_LOG_CRIT 3   // 严重 【crit】
#define ZHY_LOG_ERR 4    // 错误 【error】
#define ZHY_LOG_WARN 5   // 警告 【warn】
#define ZHY_LOG_NOTICE 6 // 注意 【notice】
#define ZHY_LOG_INFO 7   // 信息 【info】
#define ZHY_LOG_DEBUG 8  // 调试 【debug】

//默认存放日志级别的路径
#define ZHY_ERROR_LOG_PATH "error.log"
#endif