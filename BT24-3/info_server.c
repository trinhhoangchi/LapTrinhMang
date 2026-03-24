#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int recv_all(int sock, void *buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(sock, buf + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

int recv_int(int sock) {
    int val;
    recv_all(sock, &val, sizeof(val));
    return ntohl(val);
}

long recv_long(int sock) {
    long val;
    recv_all(sock, &val, sizeof(val));
    return val;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(server_fd, 5);

    int client = accept(server_fd, NULL, NULL);

    // nhận path
    int path_len = recv_int(client);
    char path[256];
    recv_all(client, path, path_len);
    path[path_len] = '\0';

    printf("Directory: %s\n", path);

    int count = recv_int(client);

    for (int i = 0; i < count; i++) {
        int name_len = recv_int(client);

        char name[256];
        recv_all(client, name, name_len);
        name[name_len] = '\0';

        long size = recv_long(client);

        printf("%s - %ld bytes\n", name, size);
    }

    close(client);
    close(server_fd);
}