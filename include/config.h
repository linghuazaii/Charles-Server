#ifndef _SERVER_CONFIG_H
#define _SERVER_CONFIG_H
/**
 * Description: server config file
 * File: config.h
 * Author: Charles,Liu. 2017-6-22
 * Mailto: charlesliu.cn.bj@gmail.com
 */

#define PORT 19920 /* server port */

#define LISTEN_BACKLOG 1024 /* backlog for listen */

#define LISTEN_POOL_SIZE 1 /* listen pool size */

#define READ_POOL_SIZE 8 /* read pool size */

#define WRITE_POOL_SIZE 8 /* write pool size */

#define WORKER_POOL_SIZE 8 /* worker pool size */

#define ERROR_POOL_SIZE 1 /* error pool size */

#define MAX_EVENTS 3000 /* max events for epoll */

#define LISTEN_QUEUE_SIZE 3000 /* listen queue size */

#define READ_QUEUE_SIZE 3000 /* read queue size */

#define WRITE_QUEUE_SIZE 3000 /* write queue size */

#define WORKER_QUEUE_SIZE 3000 /* worker queue size */

#define ERROR_QUEUE_SIZE 1000 /* error queue size */

#define SERVER_LOGGING "./conf/log_conf"

#endif
