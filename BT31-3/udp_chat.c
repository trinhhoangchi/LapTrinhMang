#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s port_s ip_d port_d\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr_s, addr_d;

    // bind local
    addr_s.sin_family = AF_INET;
    addr_s.sin_port = htons(port_s);
    addr_s.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&addr_s, sizeof(addr_s));

    // dest
    addr_d.sin_family = AF_INET;
    addr_d.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &addr_d.sin_addr);

    fd_set readfds;

    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);      // stdin
        FD_SET(sock, &readfds);   // socket

        select(sock + 1, &readfds, NULL, NULL, NULL);

        // nhập từ bàn phím → gửi
        if (FD_ISSET(0, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);

            sendto(sock, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&addr_d, sizeof(addr_d));
        }

        // nhận từ socket
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);

            recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);

            printf("Nhan: %s", buffer);
        }
    }

    return 0;
}