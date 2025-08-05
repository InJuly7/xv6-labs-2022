#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_PARAMS_LEN 100

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(2, "xargs argc = %d", argc);
        exit(1);
    }
    char *para[MAXARG] = {0};
    char buffer[MAXARG][MAX_PARAMS_LEN] = {{0}};

    int argc1 = 0;
    // 将所有参数拷贝到 para
    for (argc1 = 0; argc1 < argc - 1; argc1++) {
        para[argc1] = argv[argc1 + 1];
    }

    // 将执行的 命令 拷贝到 command
    char command[MAX_PARAMS_LEN] = {0};
    memmove(command, para[0], strlen(para[0]) + 1); // '\0'

    int flag = 1; // 读取结束标志
    while (1) {
        char c;
        int pipe_argc = 0;
        char *p = buffer[argc1 + pipe_argc];

        // 读取第一行数据
        while ((flag = read(0, &c, sizeof(char))) > 0 && c != '\n') {
            if (c == ' ') {
                *p = '\0';
                para[argc1 + pipe_argc] = buffer[argc1 + pipe_argc];
                pipe_argc++;
                p = buffer[argc1 + pipe_argc];
            } else
                *p++ = c;
        }
        if (flag == 0)
            break;

        // 处理最后一个参数
        if (p != buffer[argc1 + pipe_argc]) { // 确保有内容
            *p = '\0';
            para[argc1 + pipe_argc] = buffer[argc1 + pipe_argc];
            pipe_argc++;
        }

        // 设置参数数组结束标记
        para[argc1 + pipe_argc] = 0;

        if (fork() == 0) {
            exec(command, para);
            exit(0);
        } else {
            wait(0);
        }
    }

    exit(0);
}