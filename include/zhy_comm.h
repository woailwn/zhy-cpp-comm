#ifndef __ZHY_COMM_H__
#define __ZHY_COMM_H__

#define _PKG_MAX_LENGTH 30000 // 包头+包体

//收包状态
#define _PKG_HD_INIT 0       //初始状态，准备接收包体
#define _PKG_HD_RECVING 1    //接收包头中
#define _PKG_BD_INIT 2    //包头接收完，准备接收包体
#define _PKG_BD_RECVING 3    //接收包体中，包体不完整，继续接收，处理完毕后直接回到0状态

#define _DATA_BUFSIZE_ 20    //包头数据数组大小


#pragma pack(1)     //字节对齐  这里设为1结构体成员内存之间不添加额外字节，一个挨着一个

//包头结构
typedef struct _COMM_PKG_HEADER{
    unsigned short pkgLen; //包头+包体总长
    unsigned short msgCode; //消息类型
}COMM_PKG_HEADER,*LPCOMM_PKG_HEADER;

#pragma pack()

#endif // !__ZHY_COMM_H__