#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char *msg = "Hello UDP";

    sendto(sock, msg, strlen(msg), 0,
           (struct sockaddr*)&server, sizeof(server));

    char buffer[1024];
    socklen_t len = sizeof(server);

    int n = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                     (struct sockaddr*)&server, &len);

    buffer[n] = '\0';
    printf("Echo: %s\n", buffer);

    close(sock);
}