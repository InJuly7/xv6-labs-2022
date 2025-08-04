#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
    pthread_mutex_t barrier_mutex;
    pthread_cond_t barrier_cond;
    int nthread; // 到达同步点的线程数
    int round;   // 达到同步点的次数
} bstate;

// 初始化 锁, 条件变量
static void barrier_init(void) {
    assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
    assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
    bstate.nthread = 0;
}

// 所有线程都到达同步点, 才会进入下一轮
static void barrier() {

    // 申请持有锁
    pthread_mutex_lock(&bstate.barrier_mutex);
    bstate.nthread++;
    // 当最后一个线程到达, 唤醒其余等待线程
    if (bstate.nthread == nthread) {
        bstate.round++; // 进入下一轮
        bstate.nthread = 0; // 重置计数器
        // 唤醒所有在条件变量 cond 上睡眠的线程
        pthread_cond_broadcast(&bstate.barrier_cond);
    } else {
        // 调用pthread_cond_wait时，必须持有锁状态
        // 在条件变量 cond 上睡眠，释放锁 mutex，唤醒时重新获取锁
        pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    }
    // 释放锁
    pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *thread(void *xa) {
    long n = (long)xa;
    long delay;
    int i;

    for (i = 0; i < 20000; i++) {
        int t = bstate.round;
        assert(i == t);
        barrier();
        // 等待其余线程被唤醒
        usleep(random() % 100);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    pthread_t *tha;
    void *value;
    long i;
    double t1, t0;

    if (argc < 2) {
        fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
        exit(-1);
    }
    nthread = atoi(argv[1]);
    tha = malloc(sizeof(pthread_t) * nthread);
    srandom(0);

    barrier_init();

    for (i = 0; i < nthread; i++) {
        assert(pthread_create(&tha[i], NULL, thread, (void *)i) == 0);
    }
    for (i = 0; i < nthread; i++) {
        assert(pthread_join(tha[i], &value) == 0);
    }
    printf("OK; passed\n");
}
