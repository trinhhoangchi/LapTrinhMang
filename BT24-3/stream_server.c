#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8081

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    int client = accept(server_fd, NULL, NULL);

    char buffer[1024];
    char tail[10] = "";
    int count = 0;

    while (1) {
        int n = recv(client, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';

        char data[2048];
        snprintf(data, sizeof(data), "%s%s", tail, buffer);

        char *p = data;
        while ((p = strstr(p, "0123456789")) != NULL) {
            count++;
            p += 1;
        }

        int len = strlen(data);
        if (len >= 9)
            strncpy(tail, data + len - 9, 9);
        else
            strcpy(tail, data);

        tail[9] = '\0';

        printf("Count = %d\n", count);
    }

    close(client);
    close(server_fd);
}