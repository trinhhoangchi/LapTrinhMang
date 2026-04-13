#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9090
#define BUF_SIZE 1024
#define MAX_CLIENT 50

typedef struct {
    int fd;
    int logged_in;
} Client;

Client clients[MAX_CLIENT];

// check user/pass từ file
int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
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

int main() {
    int server_fd, new_socket, max_fd;
    struct sockaddr_in address;
    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    for (int i = 0; i < MAX_CLIENT; i++) clients[i].fd = 0;

    printf("Telnet server running on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENT; i++) {
            int sd = clients[i].fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_fd) max_fd = sd;
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // client mới
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENT; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].logged_in = 0;
                    send(new_socket, "Nhap user pass:\n", 18, 0);
                    break;
                }
            }
        }

        // xử lý client
        for (int i = 0; i < MAX_CLIENT; i++) {
            int sd = clients[i].fd;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[BUF_SIZE] = {0};
                int valread = read(sd, buffer, BUF_SIZE);

                if (valread <= 0) {
                    close(sd);
                    clients[i].fd = 0;
                    continue;
                }

                buffer[valread] = '\0';

                // ===== FIX TELNET RÁC =====
                // bỏ ký tự telnet control (0xff)
                if ((unsigned char)buffer[0] == 0xff) {
                    continue;
                }

                // remove \r\n
                buffer[strcspn(buffer, "\r\n")] = 0;

                // nếu chuỗi rỗng thì bỏ qua
                if (strlen(buffer) == 0) continue;

                // ===== LOGIN =====
                if (!clients[i].logged_in) {
                    char user[50], pass[50];

                    if (sscanf(buffer, "%s %s", user, pass) != 2) {
                        send(sd, "Sai format (user pass)\n", 25, 0);
                        continue;
                    }

                    if (check_login(user, pass)) {
                        clients[i].logged_in = 1;
                        send(sd, "Login OK\n", 9, 0);
                    } else {
                        send(sd, "Login FAIL\n", 11, 0);
                    }
                }

                // ===== COMMAND =====
                else {
                    char cmd[200];

                    if (strcmp(buffer, "ls") == 0) {
                        strcpy(cmd, "ls > out.txt");
                    }
                    else if (strcmp(buffer, "pwd") == 0) {
                        strcpy(cmd, "pwd > out.txt");
                    }
                    else if (strcmp(buffer, "date") == 0) {
                        strcpy(cmd, "date > out.txt");
                    }
                    else {
                        send(sd, "Lenh khong hop le\n", 19, 0);
                        continue;
                    }

                    system(cmd);

                    FILE *f = fopen("out.txt", "r");
                    if (f) {
                        char line[BUF_SIZE];
                        while (fgets(line, sizeof(line), f)) {
                            send(sd, line, strlen(line), 0);
                        }
                        fclose(f);
                    }
                }
            }
        }
    }
}