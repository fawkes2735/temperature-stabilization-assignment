#include "common.h"

static int listen_fd = -1;

static int setup_server_socket(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt(SO_REUSEADDR)"); exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(fd, MAX_CLIENTS) < 0) {
        perror("listen"); exit(1);
    }
    return fd;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <initial_central_temp>\n", argv[0]);
        return 1;
    }
    double central = atof(argv[1]);

    listen_fd = setup_server_socket();
    printf("[server] listening on %s:%d | initial central=%.6f\n",
           SERVER_ADDR, SERVER_PORT, central);
    fflush(stdout);

    int client_fds[MAX_CLIENTS] = {-1,-1,-1,-1};
    int client_idx[MAX_CLIENTS] = {0,0,0,0};
    double ext_prev[MAX_CLIENTS] = {NAN,NAN,NAN,NAN};
    double ext_curr[MAX_CLIENTS] = {0,0,0,0};


    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = accept(listen_fd, NULL, NULL);
        if (client_fds[i] < 0) { perror("accept"); exit(1); }

        Msg m;
        if (recv_all(client_fds[i], &m, sizeof(m)) < 0) { perror("recv initial"); exit(1); }
        if (m.type != MSG_TEMP || m.idx < 1 || m.idx > 4) {
            fprintf(stderr, "[server] invalid initial message\n");
            exit(1);
        }
        client_idx[i] = m.idx;
        ext_curr[i] = m.temp;
        printf("[server] connected client idx=%d | init ext=%.6f (iter=%d)\n",
               client_idx[i], ext_curr[i], m.iter);
        fflush(stdout);
    }

    int iter = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) ext_prev[i] = ext_curr[i];

    while (1) {
        double sum = 0.0;
        for (int i = 0; i < MAX_CLIENTS; i++) sum += ext_curr[i];
        double new_central = (2.0*central + sum) / 6.0;

        int stable = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            double delta = fabs(ext_curr[i] - ext_prev[i]);
            if (delta >= EPS) { stable = 0; break; }
        }

        if (stable && iter > 0) {
            printf("[server] stabilized at iter=%d | central=%.6f\n", iter, central);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                Msg done = { .type = MSG_DONE, .idx = 0, .iter = iter, .temp = central };
                if (send_all(client_fds[i], &done, sizeof(done)) < 0) {
                    perror("send DONE");
                }
            }
            break;
        }

        Msg upd = { .type = MSG_CENTRAL, .idx = 0, .iter = iter, .temp = new_central };
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (send_all(client_fds[i], &upd, sizeof(upd)) < 0) {
                perror("send central"); exit(1);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) ext_prev[i] = ext_curr[i];

        for (int i = 0; i < MAX_CLIENTS; i++) {
            Msg m;
            if (recv_all(client_fds[i], &m, sizeof(m)) < 0) { perror("recv TEMP"); exit(1); }
            if (m.type != MSG_TEMP) {
                fprintf(stderr, "[server] expected TEMP from client %d\n", client_idx[i]);
                exit(1);
            }
            ext_curr[i] = m.temp;
        }

        central = new_central;
        iter++;

        printf("[server] iter=%d central=%.6f | ext:", iter, central);
        for (int i = 0; i < MAX_CLIENTS; i++) printf(" [c%d]=%.6f", client_idx[i], ext_curr[i]);
        printf("\n"); fflush(stdout);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) close(client_fds[i]);
    close(listen_fd);
    printf("[server] done.\n");
    return 0;
}
