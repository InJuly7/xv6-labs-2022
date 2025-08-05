#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    //  创建两个管道 p1: P --> S    p2: S --> P
    int p1[2], p2[2];
    pipe(p1), pipe(p2);
    char *ping = "ping";
    char *pong = "pong";
    char *buffer[10] = {0};

    int pid = fork();
    if (pid == 0) {
        // child process
        int child_pid = getpid();
        close(p1[1]);
        int n = read(p1[0], buffer, sizeof buffer);
        buffer[n] = '\0';
        close(p1[0]);
        printf("%d: received %s\n", child_pid, buffer);
        close(p2[0]);
        write(p2[1], pong, strlen(pong));
        close(p2[1]);
        exit(0);
    } else if (pid > 0) {
        // parents process
        int parents_pid = getpid();
        close(p1[0]);
        write(p1[1], ping, strlen(ping));
        close(p1[1]);

        close(p2[1]);
        int n = read(p2[0], buffer, sizeof buffer);
        buffer[n] = '\0';
        printf("%d: received %s\n", parents_pid, buffer);
        close(p2[0]);
        wait(0);
    }
    exit(0);
}