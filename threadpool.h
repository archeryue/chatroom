//coder : archer
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include "chat.h"

typedef struct worker {
    void * (* process)(void *arg);
    void * arg;
    struct worker * next;
} mythread_worker;

typedef struct {
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    mythread_worker * queue_head;

    int is_shutdown;

    pthread_t *threadid;

    int max_thread_num;
    int cur_queue_size;
} mythread_pool;

typedef struct arg_t {
    int fd;
    char buf[MAXBUFARG+1];
} arg_t;

int pool_add_worker(void *(process)(void *arg),void *arg);

void *thread_routine(void *arg);

void pool_init(int max_thread_num);

int pool_destroy();
#endif
