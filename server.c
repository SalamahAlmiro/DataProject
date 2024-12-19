#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

const char* menu = "Menu:\n"
"1. Burger - $5\n"
"2. Pizza - $8\n"
"3. Salad - $4\n"
"4. Makluba - $10\n"
"5. Mandi - $12\n"
"6. Kuru Fasulye - $6\n"
"7. Adana Kebap - $15\n"
"8. Tavuk Doner - $7\n"
"9. Tantuni - $8\n"
"10. Kunefe - $5\n"
"Enter 'item,quantity' to order or 'done' to finish.\n";

const char* get_item_name(const char* item) {
    if (strcmp(item, "1") == 0) return "Burger";
    if (strcmp(item, "2") == 0) return "Pizza";
    if (strcmp(item, "3") == 0) return "Salad";
    if (strcmp(item, "4") == 0) return "Makluba";
    if (strcmp(item, "5") == 0) return "Mandi";
    if (strcmp(item, "6") == 0) return "Fasulye";
    if (strcmp(item, "7") == 0) return "Kebap";
    if (strcmp(item, "8") == 0) return "Doner";
    if (strcmp(item, "9") == 0) return "Tantuni";
    if (strcmp(item, "10") == 0) return "Kunefe";
    return NULL; // Invalid order
}

int get_price(const char* item) {
    if (strcmp(item, "1") == 0) return 5;
    if (strcmp(item, "2") == 0) return 8;
    if (strcmp(item, "3") == 0) return 4;
    if (strcmp(item, "4") == 0) return 10;
    if (strcmp(item, "5") == 0) return 12;
    if (strcmp(item, "6") == 0) return 6;
    if (strcmp(item, "7") == 0) return 15;
    if (strcmp(item, "8") == 0) return 7;
    if (strcmp(item, "9") == 0) return 8;
    if (strcmp(item, "10") == 0) return 5;
    return -1; // Invalid order
}

void save_order_to_file(int order_number, const char* order_details, int total_price) {
    char filename[BUFFER_SIZE];
    snprintf(filename, BUFFER_SIZE, "%d.txt", order_number);
    FILE* file = fopen(filename, "w");
    if (file) {
        fprintf(file, "------------------------------------------------\n");
        fprintf(file, "Item\t\t,Quantity\t,Price\t,Total\n");
        fprintf(file, "------------------------------------------------");
        fprintf(file, "\n%s------------------------------------------------\nTotal price: $%d\n", order_details, total_price);
        fprintf(file, "------------------------------------------------");
        fclose(file);
    }
    else {
        perror("Failed to create order file");
    }
}

void* handle_client(void* arg) {
    static int order_count = 0;
    int client_socket = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char order_details[BUFFER_SIZE] = "";
    int total_price = 0;
    int total_orders = 0;

    // Increment the order count for the current session
    int order_number = ++order_count;

    // Send menu to client
    send(client_socket, menu, strlen(menu), 0);

    while (1) {
        // Receive order from client
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';

            if (strcmp(buffer, "done") == 0) {
                snprintf(response, BUFFER_SIZE, "Order complete. Total items: %d, Total price: $%d\n", total_orders, total_price);
                send(client_socket, response, strlen(response), 0);

                save_order_to_file(order_number, order_details, total_price);
                printf("Order %d has been confirmed. Total price: $%d \n", order_number, total_price);
                break;
            }

            char item[BUFFER_SIZE], quantity[BUFFER_SIZE];
            if (sscanf(buffer, "%[^,],%s", item, quantity) != 2) {
                snprintf(response, BUFFER_SIZE, "Invalid format. Use 'item,quantity' or 'done' to finish.\n");
                send(client_socket, response, strlen(response), 0);
                continue;
            }

            const char* item_name = get_item_name(item);
            int price = get_price(item);
            if (item_name == NULL || price == -1) {
                snprintf(response, BUFFER_SIZE, "Invalid item: %s\n", item);
                send(client_socket, response, strlen(response), 0);
                continue;
            }

            int qty = atoi(quantity);
            if (qty <= 0) {
                snprintf(response, BUFFER_SIZE, "Invalid quantity: %s\n", quantity);
                send(client_socket, response, strlen(response), 0);
                continue;
            }

            total_price += price * qty;
            total_orders += qty;

            // Add to order details
            char order_line[BUFFER_SIZE];
           // snprintf(order_line, BUFFER_SIZE, "%s-%-10s\t, %d\t, $%d\t, $%d\n",item, item_name, quantity, price, qty * price);
            snprintf(order_line, BUFFER_SIZE, "%s-%-10s\t,%-10s\t,$%d\t,$%d\n",item, item_name, quantity,price,qty*price);
            strcat(order_details, order_line);

            snprintf(response, BUFFER_SIZE, "Order added. Total so far: %d items, $%d\n", total_orders, total_price);
            send(client_socket, response, strlen(response), 0);
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
        new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        printf("New connection established.\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, new_socket) != 0) {
            perror("Could not create thread");
            free(new_socket);
        }
        pthread_detach(thread_id);
    }

    return 0;
}