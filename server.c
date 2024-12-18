#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

const char* menu = "Menu:\n1. Burger - $5\n2. Pizza - $8\n3. Salad - $4\nType the item number to order:\n";

int get_price(const char* order) {
    if (strcmp(order, "1") == 0) return 5;
    if (strcmp(order, "2") == 0) return 8;
    if (strcmp(order, "3") == 0) return 4;
    return -1; // Invalid order
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg); // Free the memory allocated for the socket descriptor
    char buffer[BUFFER_SIZE];

    // Send menu to client
    send(client_socket, menu, strlen(menu), 0);

    // Receive order from client
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Order received: %s\n", buffer);

        int price = get_price(buffer);
        if (price == -1) {
            snprintf(buffer, BUFFER_SIZE, "Invalid order. Please try again.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            close(client_socket);
            return NULL;
        }

        snprintf(buffer, BUFFER_SIZE, "Your order costs $%d. Type OK to confirm or Cancel to cancel.\n", price);
        send(client_socket, buffer, strlen(buffer), 0);

        // Receive confirmation from client
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            if (strcmp(buffer, "OK") == 0) {
                snprintf(buffer, BUFFER_SIZE, "Order confirmed. Thank you!\n");
            }
            else {
                snprintf(buffer, BUFFER_SIZE, "Order canceled.\n");
            }
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, * new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        new_socket = malloc(sizeof(int)); // Allocate memory for the socket descriptor
        if ((*new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            free(new_socket); // Free memory in case of an error
            continue;
        }

        printf("New connection established.\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, new_socket) != 0) {
            perror("Could not create thread");
            free(new_socket); // Free memory if thread creation fails
        }
        pthread_detach(thread_id); // Detach the thread to handle cleanup automatically
    }

    return 0;
}
