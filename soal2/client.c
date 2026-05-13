#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 4096

int main() {

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1",
              &server_addr.sin_addr);

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("Connection failed");
        return 1;
    }

    printf("Connected to server on port %d\n", PORT);

    while (1) {

        printf("db > ");

        memset(buffer, 0, BUFFER_SIZE);

        fgets(buffer, BUFFER_SIZE, stdin);

        if (strncmp(buffer, "EXIT", 4) == 0)
            break;

        send(sock, buffer, strlen(buffer), 0);

        memset(response, 0, BUFFER_SIZE);

        int bytes = recv(sock,
                         response,
                         BUFFER_SIZE,
                         0);

        if (bytes <= 0) {
            printf("Server disconnected\n");
            break;
        }

        printf("%s\n", response);
    }

    close(sock);

    return 0;
}
