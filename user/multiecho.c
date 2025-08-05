#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    // 输出多行到标准输出
    write(1, "line1\nline2\nline3\n", 18);
    exit(0);
}