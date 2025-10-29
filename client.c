#include "common.h"

static int connect_to_server(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <client_index[1-4]> <initial_external_temp>\n", argv[0]);
        return 1;
    }
    int idx = atoi(argv[1]);
    if (idx < 1 || idx > 4) {
        fprintf(stderr, "client_index must be 1..4\n");
        return 1;
    }
    double external = atof(argv[2]);

    int fd = connect_to_server();
    int iter = 0;

    printf("[client %d] connected | init external=%.6f\n", idx, external);
    fflush(stdout);

    Msg m = { .type = MSG_TEMP, .idx = idx, .iter = iter, .temp = external };
    if (send_all(fd, &m, sizeof(m)) < 0) { perror("send init"); return 1; }

    while (1) {
        Msg resp;
        if (recv_all(fd, &resp, sizeof(resp)) < 0) {
            perror("recv"); return 1;
        }

        if (resp.type == MSG_DONE) {
            printf("[client %d] DONE at iter=%d | final external=%.6f | final central=%.6f\n",
                   idx, resp.iter, external, resp.temp);
            break;
        } else if (resp.type == MSG_CENTRAL) {
            double central = resp.temp;
            double new_external = (3.0*external + 2.0*central) / 5.0;
            iter = resp.iter + 1;

            Msg reply = { .type = MSG_TEMP, .idx = idx, .iter = iter, .temp = new_external };
            if (send_all(fd, &reply, sizeof(reply)) < 0) { perror("send TEMP"); return 1; }

            external = new_external;
            printf("[client %d] iter=%d | central=%.6f -> external=%.6f\n",
                   idx, iter, central, external);
            fflush(stdout);
        } else {
            fprintf(stderr, "[client %d] unexpected message type=%d\n", idx, resp.type);
            return 1;
        }
    }

    close(fd);
    printf("[client %d] exit.\n", idx);
    return 0;
}
