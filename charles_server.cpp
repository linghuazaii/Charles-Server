#include "charles_server.h"

/**
 * two callback, one for request, another for response.
 */
request_callback charles_server_request_handler = NULL;
response_callback charles_server_response_handler = NULL;
server_t *charles_server = NULL;

int set_nonblock(int sock) {
    int flags = fcntl(sock, F_GETFL);
    if (flags == -1) {
        err("fcntl getfl failed");
        return -1;
    }
    flags |= O_NONBLOCK;
    int ret = fcntl(sock, F_GETFL, flags);
    if (ret == -1) {
        err("fcntl setfl failed");
        return -1;
    }

    return 0;
}

int set_reuseaddr(int sock) {
    int on = 1;
    int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    if (ret == -1) {
        err("setsockopt failed");
        return -1;
    }

    return 0;
}

int set_tcp_defer_accept(int sock) {
    int on = 1;
    int ret = setsockopt(sock, IPPROTO_TCP, TCP_DEFER_ACCEPT, &on, sizeof(int));
    if (ret == -1) {
        err("setsockopt failed");
        return -1;
    }

    return 0;
}

int set_tcp_nodelay(int sock) {
    int on = 1;
    int ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));
    if (ret == -1) {
        err("setsockopt failed");
        return -1;
    }

    return 0;
}

int ss_socket() {
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock == -1)
        abort("create socket error");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    set_reuseaddr(sock);
    set_tcp_defer_accept(sock);

    int ret = bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
        abort("bind socket error");

    ret = listen(sock, LISTEN_BACKLOG);
    if (ret == -1)
        abort("listen socket error");

    return sock;
}

int ss_epoll_create() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
        abort("epoll create error");

    return epoll_fd;
}

int ss_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    int ret = epoll_ctl(epfd, op, fd, event);
    if (ret == -1)
        LOG_ERROR("epoll_ctl error (%s)", strerror(errno));

    return ret;
}

int ss_epoll_wait(int epfd, struct epoll_event *events, int maxevents) {
    int ret = epoll_wait(epfd, events, maxevents, -1);
    if (ret == -1)
        abort("epoll_wait error");

    return ret;
}

void ss_free_imp(void **ptr) {
    free(*ptr);
    *ptr = NULL;
}

void free_request(request_t *request) {
    free(request->buffer);
    free(request);
}

void response(void *connection, char *resp, uint32_t length) {
    ep_data_t *data = (ep_data_t *)connection;
    data->ep_write_buffer.buffer = ss_malloc(length);
    memcpy(data->ep_write_buffer.buffer, resp, length);
    data->ep_write_buffer.length = length;
    struct epoll_event event;
    event.data.fd = data->eventfd;
    event.data.ptr = data;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET; /* we should always allow read */
    ss_epoll_ctl(data->epfd, EPOLL_CTL_MOD, data->eventfd, &event);
}

void do_accept(void *arg) {
    ep_data_t *data = (ep_data_t *)arg;
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    char ip[IPLEN];

    while (true) {
        int conn = accept4(data->eventfd, (struct sockaddr *)&client, &addrlen, SOCK_NONBLOCK);
        if (conn == -1) {
            if (errno == EAGAIN)
                break;
            LOG_ERROR("accpet4 error (%s)", strerror(errno));
            continue;
        }
        inet_ntop(AF_INET, &client.sin_addr, ip, IPLEN);
        LOG_INFO("accept connection from %s, fd: %d", ip, conn);

        if (-1 == set_tcp_nodelay(conn))
            LOG_ERROR("set tcp nodelay failed for %d (%s)", conn, strerror(errno));

        struct epoll_event event;
        //event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP; //shouldn't include EPOLLOUT '
        event.events = EPOLLIN | EPOLLET;
        ep_data_t *ep_data = (ep_data_t *)ss_malloc(sizeof(ep_data_t));
        ep_data->epfd = data->epfd;
        ep_data->eventfd = conn;
        ep_data->read_callback = charles_server_request_handler; /* callback for request */
        ep_data->write_callback = charles_server_response_handler; /* callback for response */
        event.data.ptr = ep_data;
        ss_epoll_ctl(data->epfd, EPOLL_CTL_ADD, conn, &event);
        xlog("add event for fd: %d", conn);
    }
}

void do_close(void *arg) {
    xlog("do close, free data!");
    ep_data_t *data = (ep_data_t *)arg;
    if (data != NULL) {
        ss_epoll_ctl(data->epfd, EPOLL_CTL_DEL, data->eventfd, NULL);
        close(data->eventfd);
        if (data->ep_write_buffer.buffer != NULL)
            ss_free(data->ep_write_buffer.buffer);
        ss_free(data);
    }
}

int get_socket_buffer_length(int fd) {
    int length = 0;
    int ret = ioctl(fd, FIONREAD, &length);
    if (ret == -1) {
        LOG_ERROR("ioctl get buffer length failed (%s)", strerror(errno));
        return ret;
    }

    return length;
}

void do_read(void *arg) {
    xlog("enter do_read");
    ep_data_t *data = (ep_data_t *)arg;
    //while (true) {
        int length = get_socket_buffer_length(data->eventfd);
        xlog("buffer length: %d", length);
        if (length == 0 || length == -1) {
            //do_close(data);
        //    break;
            return;
        }
        request_t *read_buffer = (request_t *)ss_malloc(sizeof(request_t));
        read_buffer->length = length;
        read_buffer->buffer = ss_malloc(length);
        read_buffer->connection = (void *)data;
        int count = read(data->eventfd, read_buffer->buffer, length);
        //if (count == -1) {
        //    if (errno == EAGAIN) {
        //        break; /* no more data to read */
        //    }
        //    LOG_ERROR("read error for %d (%s)", data->eventfd, strerror(errno));
        //    return;
        //}
        threadpool_add(charles_server->worker_threadpool, data->read_callback, (void *)read_buffer, 0);
    //}
}

void reset_epdata(ep_data_t *data) {
    ss_free(data->ep_write_buffer.buffer);
    memset(&data->ep_write_buffer, 0, sizeof(data->ep_write_buffer));
    /* remove write event */
    struct epoll_event event;
    event.data.ptr = (void *)data;
    event.events = EPOLLIN | EPOLLET;
    ss_epoll_ctl(data->epfd, EPOLL_CTL_MOD, data->eventfd, &event);
}

void do_write(void *arg) {
    ep_data_t *data = (ep_data_t *)arg;
    while (true) {
        int count = write(data->eventfd, data->ep_write_buffer.buffer + data->ep_write_buffer.count, data->ep_write_buffer.length - data->ep_write_buffer.count);
        if (count == -1) {
            if (errno == EAGAIN) {
                break; /* wait for next write */
            }
            LOG_ERROR("write error for %d (%s)", data->eventfd, strerror(errno));
            return;
        }
        data->ep_write_buffer.count += count;
        if (data->ep_write_buffer.count < data->ep_write_buffer.length) {
            /* write not finished, wait for next write */
        } else {
            //data->write_callback(data); /* should not block write */
            if (data->write_callback != NULL)
                threadpool_add(charles_server->worker_threadpool, data->write_callback, NULL, 0);
            reset_epdata(data);
            break;
        }
    }
}

void event_loop() {
    int listener = ss_socket();
    int epfd = ss_epoll_create();
    xlog("epollfd: %d\tlistenfd: %d", epfd, listener);
    ep_data_t *listener_data = (ep_data_t *)ss_malloc(sizeof(ep_data_t));
    listener_data->epfd = epfd;
    listener_data->eventfd = listener;
    struct epoll_event ev_listener;
    ev_listener.data.ptr = listener_data;
    ev_listener.events = EPOLLIN | EPOLLET;
    int ret = ss_epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev_listener);
    if (ret == -1)
        abort("can't add listen event to epoll (%s)", strerror(errno));

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        int nfds = ss_epoll_wait(epfd, events, MAX_EVENTS);
        for (int i = 0; i < nfds; ++i) {
            xlog("current eventfd: %d", ((ep_data_t *)events[i].data.ptr)->eventfd);
            if (((ep_data_t *)events[i].data.ptr)->eventfd == listener) {
                xlog("listenfd event.");
                threadpool_add(charles_server->listen_threadpool, do_accept, events[i].data.ptr, 0);
            } else {
                if (events[i].events & EPOLLIN) {
                    xlog("read event.");
                    threadpool_add(charles_server->read_threadpool, do_read, events[i].data.ptr, 0);
                }
                if (events[i].events & EPOLLOUT) {
                    xlog("write event.");
                    threadpool_add(charles_server->write_threadpool, do_write, events[i].data.ptr, 0);
                }
                if (events[i].events & EPOLLERR | events[i].events & EPOLLHUP) {
                    xlog("close event.");
                    threadpool_add(charles_server->error_threadpool, do_close, events[i].data.ptr, 0);
                }
            }
        }
    }
}

void init_server() {
    charles_server = (server_t *)ss_malloc(sizeof(server_t));
    charles_server->read_threadpool = threadpool_create(READ_POOL_SIZE, READ_QUEUE_SIZE, 0);
    charles_server->write_threadpool = threadpool_create(WRITE_POOL_SIZE, WRITE_QUEUE_SIZE, 0);
    charles_server->listen_threadpool = threadpool_create(LISTEN_POOL_SIZE, LISTEN_QUEUE_SIZE, 0);
    charles_server->error_threadpool = threadpool_create(ERROR_POOL_SIZE, ERROR_QUEUE_SIZE, 0);
    charles_server->worker_threadpool = threadpool_create(WORKER_POOL_SIZE, WORKER_QUEUE_SIZE, 0);
    if (charles_server->read_threadpool == NULL || charles_server->write_threadpool == NULL || 
            charles_server->listen_threadpool == NULL || charles_server->error_threadpool == NULL || 
            charles_server->worker_threadpool == NULL) {
        LOG_ERROR("failed to init server pools");
        exit(1);
    }
}

void set_callbacks(request_callback request_cb, response_callback response_cb) {
    charles_server_request_handler = request_cb;
    charles_server_response_handler = response_cb;
}

void start_server(request_callback request_cb, response_callback response_cb) {
    START_CHARLES_LOGGING(SERVER_LOGGING);
    set_callbacks(request_cb, response_cb);
    init_server();
    event_loop();
}

void stop_server() {
    STOP_CHARLES_LOGGING();
}
