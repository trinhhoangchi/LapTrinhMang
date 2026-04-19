#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9090
#define MAX_CLIENT FD_SETSIZE
#define BUF_SIZE 1024

typedef struct {
    int fd;
    int logged;
    char username[50];
} Client;

Client clients[MAX_CLIENT];

void init_clients() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = -1;
        clients[i].logged = 0;
    }
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("account.txt", "r");
    if (!f) return 0;

    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void execute_command(int fd, char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        send(fd, "Command error\n", 14, 0);
        return;
    }

    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), fp)) {
        send(fd, buf, strlen(buf), 0);
    }
    pclose(fp);
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
                    send(new_fd, "Nhap: user pass\n", 19, 0);
                    break;
                }
            }
        }

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

                buf[strcspn(buf, "\r\n")] = 0;

                if (!clients[i].logged) {
                    char user[50], pass[50];

                    if (sscanf(buf, "%49s %49s", user, pass) != 2) {
                        send(fd, "Nhap dung: user pass\n", 23, 0);
                        continue;
                    }

                    if (check_login(user, pass)) {
                        clients[i].logged = 1;
                        strcpy(clients[i].username, user);
                        send(fd, "Login OK\n$ ", 11, 0);
                    } else {
                        send(fd, "Login Fail\n", 12, 0);
                    }
                } else {
                    if (strlen(buf) == 0) {
                        send(fd, "$ ", 2, 0);
                        continue;
                    }

                    execute_command(fd, buf);
                    send(fd, "\n$ ", 4, 0);
                }
            }
        }
    }
}