#ifndef _CHARLES_SERVER_H
#define _CHARLES_SERVER_H
/**
 * Description: charles server header file
 * File: charles_server.h
 * Author: Charles,Liu. 2017-6-22
 * Mailto: charlesliu.cn.bj@gmail.com
 */
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <inttypes.h>
#include <time.h>
#include <sys/ioctl.h>
#include "threadpool.h"
#include "charles_log.h"
#include "config.h"

#define _8K 8 * 1024
#define _16K 16 * 1024
#define _64K 64 * 1024
#define _128K _64K * 2
#define _256K _128K * 2
#define _512K _256K * 2
#define _1M 1024 * 1024

#define IPLEN 32

#define ss_malloc(size) malloc(size)

#define abort(fmt, ...) do {\
    printf(fmt, ##__VA_ARGS__);\
    if (errno != 0)\
        printf(" (%s)", strerror(errno));\
    printf("\n");\
    exit(1);\
} while(0)

typedef struct request_t {
    uint32_t length;
    void *buffer;
    void *connection;
} request_t;

typedef request_t response_t;

typedef struct ep_data_t {
    int epfd;
    int eventfd;
    void (*read_callback) (void *);
    void (*write_callback) (void *);
    void *self;
    pthread_mutex_t ep_mtx;
    void *extra;
} ep_data_t;

typedef struct server_t {
    threadpool_t *read_threadpool;
    threadpool_t *write_threadpool;
    threadpool_t *listen_threadpool;
    threadpool_t *error_threadpool;
    threadpool_t *worker_threadpool;
} server_t;

typedef void (*request_callback)(void *);
typedef void (*response_callback)(void *);

void start_server(request_callback, response_callback);
void stop_server();
void response(void *connection, void *resp, uint32_t length);
void free_request(request_t *request);

#endif
