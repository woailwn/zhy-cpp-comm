#ifndef __ZHY_COMM_H__
#define __ZHY_COMM_H__

#define _PKG_MAX_LENGTH 30000 // ��ͷ+����

//�հ�״̬
#define _PKG_HD_INIT 0       //��ʼ״̬��׼�����հ���
#define _PKG_HD_RECVING 1    //���հ�ͷ��
#define _PKG_BD_INIT 2    //��ͷ�����꣬׼�����հ���
#define _PKG_BD_RECVING 3    //���հ����У����岻�������������գ�������Ϻ�ֱ�ӻص�0״̬

#define _DATA_BUFSIZE_ 20    //��ͷ���������С


#pragma pack(1)     //�ֽڶ���  ������Ϊ1�ṹ���Ա�ڴ�֮�䲻��Ӷ����ֽڣ�һ������һ��

//��ͷ�ṹ
typedef struct _COMM_PKG_HEADER{
    unsigned short pkgLen; //��ͷ+�����ܳ�
    unsigned short msgCode; //��Ϣ����
}COMM_PKG_HEADER,*LPCOMM_PKG_HEADER;

#pragma pack()

#endif // !__ZHY_COMM_H__