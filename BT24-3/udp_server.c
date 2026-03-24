#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8082

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(sock, (struct sockaddr*)&server, sizeof(server));

    char buffer[1024];
    struct sockaddr_in client;
    socklen_t len;

    while (1) {
        len = sizeof(client);

        int n = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                         (struct sockaddr*)&client, &len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        printf("[%s:%d] %s\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port),
               buffer);

        sendto(sock, buffer, n, 0,
               (struct sockaddr*)&client, len);
    }

    close(sock);
}