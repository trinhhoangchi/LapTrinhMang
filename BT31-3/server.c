#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int state; // 0: name, 1: mssv
    char name[100];
} Client;

Client clients[MAX_CLIENTS];

// trim newline + carriage return
void trim_newline(char *str) {
    str[strcspn(str, "\r\n")] = 0;
}

// kiểm tra MSSV hợp lệ (8-9 số)
int valid_mssv(char *mssv) {
    int len = strlen(mssv);
    if (len < 8 || len > 9) return 0;
    for (int i = 0; i < len; i++) {
        if (!isdigit(mssv[i])) return 0;
    }
    return 1;
}

// tạo email
void generate_email(char *fullname, char *mssv, char *email) {
    char temp[100];
    strcpy(temp, fullname);
    char *words[10];
    int count = 0;
    char *token = strtok(temp, " ");
    while (token != NULL && count < 10) {
        words[count++] = token;
        token = strtok(NULL, " ");
    }
    if (count == 0) {
        snprintf(email, 100, "Invalid name\n");
        return;
    }
    // Tên (last word)
    char *ten = words[count - 1];
    // Initials họ + đệm
    char initials[20] = "";
    for (int i = 0; i < count - 1; i++) {
        initials[i] = toupper(words[i][0]);
    }
    initials[count - 1] = '\0';
    char *mssv_cut = mssv + 2; // bỏ 2 ký tự đầu
    // format email
    snprintf(email, 100, "%s.%s%s@sis.hust.edu.vn\n", ten, initials, mssv_cut);
}

int main() {
    int server_fd, new_socket, max_sd, sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    // init clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
        clients[i].state = 0;
    }
    // create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    printf("Server dang chay port %d...\n", PORT);
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        // add client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].fd;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }
        select(max_sd + 1, &readfds, NULL, NULL, NULL);
        // có client mới
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].state = 0;
                    send(new_socket,
                         "Nhap ho ten:\n",
                         strlen("Nhap ho ten:\n"), 0);

                    break;
                }
            }
        }
        // xử lý client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].fd;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int len = read(sd, buffer, BUFFER_SIZE - 1);
                if (len <= 0) {
                    close(sd);
                    clients[i].fd = 0;
                    continue;
                }
                buffer[len] = '\0';
                trim_newline(buffer);
                // STATE 0: nhập tên
                if (clients[i].state == 0) {
                    strncpy(clients[i].name, buffer,
                            sizeof(clients[i].name) - 1);
                    clients[i].name[sizeof(clients[i].name) - 1] = '\0';
                    send(sd,
                         "Nhap MSSV:\n",
                         strlen("Nhap MSSV:\n"), 0);
                    clients[i].state = 1;
                }
                // STATE 1: nhập MSSV
                else {
                    if (!valid_mssv(buffer)) {
                        send(sd,
                             "MSSV khong hop le (8-9 so)\n",
                             strlen("MSSV khong hop le (8-9 so)\n"), 0);
                        continue;
                    }
                    char email[100];
                    generate_email(clients[i].name, buffer, email);
                    send(sd, email, strlen(email), 0);
                }
            }
        }
    }
    return 0;
}