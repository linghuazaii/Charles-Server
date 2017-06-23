#include "charles_server.h"

void request_handler(void *req) {
    request_t *data = (request_t *)req;
    const char *msg = "You just said: ";
    int msglen = strlen(msg);
    char *reply;
    int count = 0;
    while (true) { /* packets event may be combined in epoll */
        int length = *((int *)data->buffer + count);
        reply = (char *)malloc(length + msglen + 1);
	if (reply == NULL ) {
}
        memset(reply, 0, length + msglen + 1);
        memcpy(reply, msg, msglen);
        memcpy(reply + msglen, data->buffer + 4, length);
        response(data->connection, (void *)reply, strlen(reply));
        delete reply;
        count += 4 + length;
        if (count >= data->length)
            break;
    }

    free_request(data);
}

void http_request(void *req) {
    request_t *data = (request_t *)req;
    const char *http_response = "\
                                 HTTP/1.1 200 OK\r\n\
                                 Server: nginx\r\n\
                                 Date: Fri, 23 Jun 2017 02:28:43 GMT\r\n\
                                 Connection: keep-alive\r\n\
                                 \r\n";
    response(data->connection, (void *)http_response, strlen(http_response));
    free_request(data);
}

void response_cb(void *arg) {
    LOG_INFO("response finished!");
}

int main(int argc, char **argv) {
    //start_server(request_handler, response_cb);
    start_server(http_request, response_cb);

    return 0;
}
