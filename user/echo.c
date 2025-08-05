#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int i;
    // 从argv[1]开始遍历所有参数（argv[0]是程序名"echo"）
    for (i = 1; i < argc; i++) {
        // 输出当前参数到文件描述符1（标准输出）
        write(1, argv[i], strlen(argv[i]));
        // 如果不是最后一个参数，输出空格分隔
        if (i + 1 < argc) {
            write(1, " ", 1);
        }
        // 如果是最后一个参数，输出换行符
        else {
            write(1, "\n", 1);
        }
    }
    exit(0);
}