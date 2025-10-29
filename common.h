#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_PORT 9090
#define SERVER_ADDR "127.0.0.1"
#define MAX_CLIENTS 4
#define EPS 1e-3

typedef enum {
    MSG_TEMP = 1,
    MSG_CENTRAL = 2,
    MSG_DONE = 3
} msg_type_t;

typedef struct {
    int32_t type;
    int32_t idx;
    int32_t iter;
    double temp;
} __attribute__((packed)) Msg;


static inline int send_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t*)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static inline int recv_all(int fd, void *buf, size_t len) {
    uint8_t *p = (uint8_t*)buf;
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        recvd += (size_t)n;
    }
    return 0;
}

#endif // COMMON_H
