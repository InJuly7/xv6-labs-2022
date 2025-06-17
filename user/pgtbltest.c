#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

void ugetpid_test();
void pgaccess_test();

int main(int argc, char *argv[]) {
    ugetpid_test(); // 测试用户态获取PID (lab3-1)
    pgaccess_test(); // 测试页面访问追踪 (lab3-3)
    printf("pgtbltest: all tests succeeded\n");
    exit(0);
}

char *testname = "???";

void err(char *why) {
    printf("pgtbltest: %s failed: %s, pid=%d\n", testname, why, getpid());
    exit(1);
}

// 测试 ugetpid() 系统调用是否与 getpid() 返回相同结果
void ugetpid_test() {
    int i;

    printf("ugetpid_test starting\n");
    testname = "ugetpid_test";

    for (i = 0; i < 64; i++) {
        int ret = fork();
        if (ret != 0) {
            wait(&ret);
            if (ret != 0)
                exit(1);
            continue;
        }
        // 子进程执行：比较两个获取PID的系统调用结果
        if (getpid() != ugetpid())
            err("missmatched PID");
        exit(0);
    }
    printf("ugetpid_test: OK\n");
}

void pgaccess_test() {
    char *buf;
    unsigned int abits;
    printf("pgaccess_test starting\n");
    testname = "pgaccess_test";
    // 分配32个页面的内存
    buf = malloc(32 * PGSIZE);

    // 第一次调用pgaccess，此时页面未被访问
    // 起始虚拟地址
    // 要检查的连续页面数量（不是整个地址空间）
    // 返回这32个页面的访问位掩码
    if (pgaccess(buf, 32, &abits) < 0)
        err("pgaccess failed");
    
    // 访问特定页面
    buf[PGSIZE * 1] += 1;
    buf[PGSIZE * 2] += 1;
    buf[PGSIZE * 30] += 1;

    // 第二次调用pgaccess，检查访问位
    if (pgaccess(buf, 32, &abits) < 0)
        err("pgaccess failed");

    // 验证返回的访问位是否正确
    if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
        err("incorrect access bits set");
    free(buf);
    printf("pgaccess_test: OK\n");
}
