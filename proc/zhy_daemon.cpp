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
    // fork 成功后的父进程退出，子进程成为后续 master 进程
    switch (fork()) {
    case -1:
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中fork()失败!");
        return -1;
    case 0:
        // 子进程，继续执行
        break;
    default:
        return 1;
    }

    zhy_parent = zhy_pid;
    zhy_pid = getpid();

    if (setsid() == -1) {
        // 创建新会话，脱离终端
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中setsid()失败!");
        return -1;
    }

    umask(0); // 避免限制操作文件

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中open(\"/dev/null\")失败!");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) { // 不接受输入
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中dup2(STDIN)失败!");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) { // 不输出任何东西
        zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            zhy_log_error_core(ZHY_LOG_EMERG, errno, "zhy_daemon()中close(fd)失败!");
            return -1;
        }
    }

    return 0;
}
