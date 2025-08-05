#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PIPE_WRITE 1
#define PIPE_READ 0

void sieve(int p[]) {
    close(p[PIPE_WRITE]);
    int prime = 0;
    read(p[PIPE_READ], &prime, sizeof(prime));
    if (prime == 0)
        exit(0); // 结束
    printf("prime %d\n", prime);

    int child_pipe[2];
    pipe(child_pipe);

    if (fork() > 0) {
        close(child_pipe[PIPE_READ]);
        int x = -1;
        while (read(p[PIPE_READ], &x, sizeof(x))) {
            if (x % prime != 0) {
                write(child_pipe[PIPE_WRITE], &x, sizeof(int));
            }
        }
        x = 0;
        write(child_pipe[PIPE_WRITE], &x, sizeof(int));
        close(p[PIPE_READ]);
        close(child_pipe[PIPE_WRITE]);
        wait(0);
        exit(0);

    } else {
        sieve(child_pipe);
    }
}

int main() {

    int p[2];
    pipe(p);
    if (fork() == 0) {
        // 子进程
        sieve(p);

    } else {
        // 父进程
        close(p[PIPE_READ]);
        for (int i = 2; i <= 35; i++) {
            write(p[PIPE_WRITE], &i, sizeof(i));
        }
        close(p[PIPE_WRITE]);
        wait(0);
    }
    exit(0);
}