#ifndef __ZHY_MACRO_H__
#define __ZHY_MACRO_H__

#define ZHY_MAX_ERROR_STR 2048 //��ʾ������Ϣ������鳤��

//�����θ��ƣ�ָ�򿽱����λ��
#define zhy_memcpy(dst,src,n) (((char*)memcpy(dst,src,n)) + (n))
#define zhy_min(val1,val2)  ((val1 < val2) ? (val1) : (val2))

//����
#define ZHY_MAX_UINT32_VALUE (uint32_t)0xffffffff 
#define ZHY_INT64_LEN (sizeof("-9223372036854775808") - 1)

//��־����
#define ZHY_LOG_STDERR 0 // ����̨����stderr����־����д�ļ����ҳ������׼������������Ϣ
#define ZHY_LOG_EMERG 1  // ���� ��emerg��
#define ZHY_LOG_ALERT 2  // ���� ��alert��
#define ZHY_LOG_CRIT 3   // ���� ��crit��
#define ZHY_LOG_ERR 4    // ���� ��error��
#define ZHY_LOG_WARN 5   // ���� ��warn��
#define ZHY_LOG_NOTICE 6 // ע�� ��notice��
#define ZHY_LOG_INFO 7   // ��Ϣ ��info��
#define ZHY_LOG_DEBUG 8  // ���� ��debug��

//Ĭ�ϴ����־�����·��
#define ZHY_ERROR_LOG_PATH "error.log"
#endif