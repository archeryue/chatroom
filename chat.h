//coder : archer
#ifndef __CHAT_H__
#define __CHAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include "rb_tree.h"

#define MAXBUF 1000
#define MAXBUFARG 1024
#define MAXEPOLLSIZE 100
#define MAXPOOLSIZE 50
#define SERPORT 8888
#define SERIP "172.21.9.40"
#define CLIPORT 7777
#define HASHNUM 1003

typedef struct iplist {
    char ip[16];
    struct iplist * next;
} iplist;

typedef struct roomlist {
    char roomname[8];
    struct roomlist * next;
} roomlist;

typedef struct room_t {
    char roomname[8];
    int num;
    node_t * roommate;
    struct room_t * next;
    iplist * firstip;
    pthread_rwlock_t rblock;
    pthread_rwlock_t listlock;
} room_t;

typedef struct {
    room_t * head;
    int num;
    roomlist * firstroom;
    pthread_rwlock_t roomlock;
} chat_t;

#endif
