#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = { 0 };
    char input[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Connected to the server.\n");

    // Receive menu from server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s", buffer);
    }

    // Order loop
    while (1) {
        printf("Enter your order (item,quantity) or 'done' to finish: ");
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';

        // Send order to server
        send(sock, input, strlen(input), 0);

        if (strcmp(input, "done") == 0) {
            break;
        }

        // Receive response from server
        valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s", buffer);
        }
    }

    // Receive final response with total details
    valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s", buffer);
    }

    close(sock);
    return 0;
}