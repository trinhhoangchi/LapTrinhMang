#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENT  FD_SETSIZE
#define BUF_SIZE 1024

typedef struct {
    int fd;
    char id[50];
    int registered;
} Client;

Client clients[MAX_CLIENT];

void broadcast(int sender_fd, char *msg) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd != -1 && clients[i].fd != sender_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

void init_clients() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = -1;
        clients[i].registered = 0;
    }
}

int main() {
    int server_fd, new_fd, max_fd;
    struct sockaddr_in addr;
    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    init_clients();

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd)
                    max_fd = clients[i].fd;
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // new connection
        if (FD_ISSET(server_fd, &readfds)) {
            new_fd = accept(server_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENT; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_fd;
                    send(new_fd, "Nhap client_id: client_name\n", 30, 0);
                    break;
                }
            }
        }

        // handle clients
        for (int i = 0; i < MAX_CLIENT; i++) {
            int fd = clients[i].fd;
            if (fd == -1) continue;

            if (FD_ISSET(fd, &readfds)) {
                char buf[BUF_SIZE] = {0};
                int n = recv(fd, buf, BUF_SIZE, 0);

                if (n <= 0) {
                    close(fd);
                    clients[i].fd = -1;
                    continue;
                }

                buf[strcspn(buf, "\n")] = 0;

                // chưa đăng ký
                if (!clients[i].registered) {
                    char *token = strchr(buf, ':');
                    if (token) {
                        *token = 0;
                        strcpy(clients[i].id, buf);
                        clients[i].registered = 1;
                        send(fd, "Dang ky thanh cong\n", 20, 0);
                    } else {
                        send(fd, "Sai cu phap!\n", 15, 0);
                    }
                } else {
                    char msg[BUF_SIZE];
                    // snprintf(msg, BUF_SIZE, "%s: %s\n", clients[i].id, buf);
                    snprintf(msg, BUF_SIZE, "%.50s: %.900s\n", clients[i].id, buf);
                    broadcast(fd, msg);
                }
            }
        }
    }
}