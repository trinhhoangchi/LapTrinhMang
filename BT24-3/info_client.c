#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int send_all(int sock, const void *buf, int len) {
    int total = 0;
    while (total < len) {
        int n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

int send_int(int sock, int value) {
    value = htonl(value);
    return send_all(sock, &value, sizeof(value));
}

int send_long(int sock, long value) {
    return send_all(sock, &value, sizeof(value));
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };

    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        return 1;
    }

    // gửi path
    char cwd[256];
    getcwd(cwd, sizeof(cwd));

    int path_len = strlen(cwd);
    send_int(sock, path_len);
    send_all(sock, cwd, path_len);

    // đếm file
    DIR *dir = opendir(".");
    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode))
            count++;
    }
    closedir(dir);

    send_int(sock, count);

    // gửi file
    dir = opendir(".");
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            int len = strlen(entry->d_name);

            send_int(sock, len);
            send_all(sock, entry->d_name, len);
            send_long(sock, st.st_size);
        }
    }

    close(sock);
}