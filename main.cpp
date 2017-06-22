#include "charles_server.h"

void request_handler(void *req) {
    request_t *data = (request_t *)req;
    const char *msg = "You just said: ";
    int msglen = strlen(msg);
    char *reply;
    int count = 0;
    while (true) { /* packets event may be combined in epoll */
        int length = *((int *)data->buffer + count);
        xlog("data length => %d", length);
        reply = (char *)(length + msglen + 1);
        memset(reply, 0, length + msglen + 1);
        memcpy(reply, msg, msglen);
        memcpy(reply + msglen, data->buffer + 4, length);
        response(data->connection, reply, strlen(reply));
        delete reply;
        count += 4 + length;
        if (count >= data->length)
            break;
    }

    free_request(data);
}

void response_cb(void *arg) {
    LOG_INFO("response finished!");
}

int main(int argc, char **argv) {
    start_server(request_handler, response_cb);

    return 0;
}
