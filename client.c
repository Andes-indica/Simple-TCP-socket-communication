#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

static int parse_port(int argc, char *argv[])
{
    char *end_ptr = NULL;
    long port = DEFAULT_PORT;

    if (argc >= 2) {
        port = strtol(argv[1], &end_ptr, 10);
        if (argv[1][0] == '\0' || *end_ptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    return (int)port;
}

int main(int argc, char *argv[])
{
    int client_socket;
    int port = parse_port(argc, argv);
    struct sockaddr_in server_addr;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(client_socket);
        return EXIT_FAILURE;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("Connected to %s:%d\n", SERVER_IP, port);
    printf("Type a message and press Enter. Type 'exit' to close the client.\n");

    while (fgets(send_buffer, sizeof(send_buffer), stdin) != NULL) {
        size_t message_length = strlen(send_buffer);

        if (send(client_socket, send_buffer, message_length, 0) < 0) {
            perror("send failed");
            break;
        }

        if (strcmp(send_buffer, "exit\n") == 0) {
            break;
        }

        memset(recv_buffer, 0, sizeof(recv_buffer));
        ssize_t bytes_received = recv(client_socket, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("recv failed");
            break;
        }

        if (bytes_received == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        recv_buffer[bytes_received] = '\0';
        printf("Server echoed: %s", recv_buffer);
    }

    close(client_socket);
    return EXIT_SUCCESS;
}
