#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENT 100
#define BUF_SIZE 1024

typedef struct {
    int fd;
    char id[50];
    int authenticated;
} Client;

Client clients[MAX_CLIENT];

void broadcast(int sender_fd, char *msg) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd != 0 && clients[i].fd != sender_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

void get_time(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 64, "%Y/%m/%d %H:%M:%S", t);
}

int main() {
    int server_fd, new_socket, max_fd, activity;
    struct sockaddr_in address;
    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = 0;
    }

    printf("Chat server running on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENT; i++) {
            int sd = clients[i].fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_fd) max_fd = sd;
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENT; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].authenticated = 0;
                    send(new_socket, "Nhap: client_id: client_name\n", 35, 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            int sd = clients[i].fd;

            if (FD_ISSET(sd, &readfds)) {
                char buffer[BUF_SIZE] = {0};
                int valread = read(sd, buffer, BUF_SIZE);

                if (valread <= 0) {
                    close(sd);
                    clients[i].fd = 0;
                    continue;
                }

                buffer[valread] = '\0';

                // chưa login
                if (!clients[i].authenticated) {
                    char *p = strstr(buffer, ":");
                    if (p) {
                        sscanf(buffer, "%[^:]:", clients[i].id);
                        clients[i].authenticated = 1;
                        send(sd, "OK\n", 3, 0);
                    } else {
                        send(sd, "Sai format\n", 11, 0);
                    }
                } else {
                    char msg[BUF_SIZE];
                    char timebuf[64];
                    get_time(timebuf);
                    snprintf(msg, sizeof(msg), "%s %s: %.900s", timebuf, clients[i].id, buffer);
                    broadcast(sd, msg);
                }
            }
        }
    }
}