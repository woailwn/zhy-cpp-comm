#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zhy_conf.h"
#include "zhy_func.h"
#include "zhy_macro.h"

int zhy_daemon() {
    // fork �ɹ���ĸ������˳����ӽ��̳�Ϊ���� master ����
    switch (fork()) {
    case -1:
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��fork()ʧ��!");
        return -1;
    case 0:
        // �ӽ��̣�����ִ��
        break;
    default:
        return 1;
    }

    zhy_parent = zhy_pid;
    zhy_pid = getpid();

    if (setsid() == -1) {
        // �����»Ự�������ն�
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��setsid()ʧ��!");
        return -1;
    }

    umask(0); // �������Ʋ����ļ�

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��open(\"/dev/null\")ʧ��!");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) { // ����������
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��dup2(STDIN)ʧ��!");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) { // ������κζ���
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��dup2(STDOUT)ʧ��!");
        return -1;
    }
    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()��close(fd)ʧ��!");
            return -1;
        }
    }

    return 0;
}
