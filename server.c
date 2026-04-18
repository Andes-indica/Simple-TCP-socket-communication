#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define LISTEN_BACKLOG 10
#define MAX_CLIENTS 100

typedef struct {
    int *client_socket;
    struct sockaddr_in client_addr;
    unsigned int delay_seconds;
    int drop_rate;
} client_context_t;

static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_connections = 0;
static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    keep_running = 0;
}

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s [port] [delay_seconds] [drop_rate_percent]\n"
            "  port               TCP port to bind on localhost (default: %d)\n"
            "  delay_seconds      Artificial delay before replies (default: 0)\n"
            "  drop_rate_percent  Probability of dropping a received message (0-100, default: 0)\n",
            program_name,
            DEFAULT_PORT);
}

static int parse_int(const char *value, const char *name, int min, int max)
{
    char *end_ptr = NULL;
    long parsed = strtol(value, &end_ptr, 10);

    if (value[0] == '\0' || *end_ptr != '\0' || parsed < min || parsed > max) {
        fprintf(stderr, "Invalid %s: %s\n", name, value);
        exit(EXIT_FAILURE);
    }

    return (int)parsed;
}

static int should_drop_message(int drop_rate)
{
    int random_value;

    if (drop_rate <= 0) {
        return 0;
    }

    pthread_mutex_lock(&rand_mutex);
    random_value = rand() % 100;
    pthread_mutex_unlock(&rand_mutex);

    return random_value < drop_rate;
}

static void update_connection_count(int delta)
{
    pthread_mutex_lock(&connection_mutex);
    active_connections += delta;
    printf("Active connections: %d\n", active_connections);
    pthread_mutex_unlock(&connection_mutex);
}

static void *handle_client(void *arg)
{
    client_context_t *context = (client_context_t *)arg;
    char client_ip[INET_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    inet_ntop(AF_INET, &context->client_addr.sin_addr, client_ip, sizeof(client_ip));

    for (;;) {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(*context->client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received == 0) {
            printf("Client disconnected: %s:%d\n",
                   client_ip,
                   ntohs(context->client_addr.sin_port));
            break;
        }

        if (bytes_received < 0) {
            perror("recv failed");
            break;
        }

        buffer[bytes_received] = '\0';

        if (should_drop_message(context->drop_rate)) {
            printf("Simulated packet drop from %s:%d: %s\n",
                   client_ip,
                   ntohs(context->client_addr.sin_port),
                   buffer);
            continue;
        }

        printf("Received from %s:%d: %s\n",
               client_ip,
               ntohs(context->client_addr.sin_port),
               buffer);

        if (context->delay_seconds > 0) {
            sleep(context->delay_seconds);
        }

        if (send(*context->client_socket, buffer, (size_t)bytes_received, 0) < 0) {
            perror("send failed");
            break;
        }
    }

    close(*context->client_socket);
    free(context->client_socket);
    update_connection_count(-1);
    free(context);
    return NULL;
}

int main(int argc, char *argv[])
{
    int server_socket;
    int opt = 1;
    int port = DEFAULT_PORT;
    int delay_seconds = 0;
    int drop_rate = 0;
    struct sockaddr_in server_addr;

    if (argc > 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 2) {
        port = parse_int(argv[1], "port", 1, 65535);
    }
    if (argc >= 3) {
        delay_seconds = parse_int(argv[2], "delay_seconds", 0, 60);
    }
    if (argc >= 4) {
        drop_rate = parse_int(argv[3], "drop_rate_percent", 0, 100);
    }

    srand((unsigned int)time(NULL));
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, LISTEN_BACKLOG) < 0) {
        perror("listen failed");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server started on %s:%d\n", SERVER_IP, port);
    printf("Listening with backlog %d, max clients %d, delay %d second(s), drop rate %d%%\n",
           LISTEN_BACKLOG,
           MAX_CLIENTS,
           delay_seconds,
           drop_rate);

    while (keep_running) {
        int *client_socket_ptr = NULL;
        pthread_t thread_id;
        client_context_t *context = NULL;
        struct sockaddr_in client_addr;
        char client_ip[INET_ADDRSTRLEN];
        socklen_t client_len = sizeof(client_addr);
        int should_reject = 0;

        client_socket_ptr = malloc(sizeof(*client_socket_ptr));
        if (client_socket_ptr == NULL) {
            perror("malloc failed");
            continue;
        }

        *client_socket_ptr = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket_ptr < 0) {
            free(client_socket_ptr);

            if (!keep_running) {
                break;
            }

            perror("accept failed");
            continue;
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        pthread_mutex_lock(&connection_mutex);
        if (active_connections >= MAX_CLIENTS) {
            should_reject = 1;
        } else {
            active_connections++;
            printf("Client connected: %s:%d\n",
                   client_ip,
                   ntohs(client_addr.sin_port));
            printf("Active connections: %d\n", active_connections);
        }
        pthread_mutex_unlock(&connection_mutex);

        if (should_reject) {
            const char *busy_message = "Server busy: maximum client limit reached.\n";

            printf("Rejected client %s:%d because max clients limit was reached.\n",
                   client_ip,
                   ntohs(client_addr.sin_port));

            if (send(*client_socket_ptr, busy_message, strlen(busy_message), 0) < 0) {
                perror("send failed");
            }

            close(*client_socket_ptr);
            free(client_socket_ptr);
            continue;
        }

        context = malloc(sizeof(*context));
        if (context == NULL) {
            perror("malloc failed");
            close(*client_socket_ptr);
            free(client_socket_ptr);
            update_connection_count(-1);
            continue;
        }

        context->client_socket = client_socket_ptr;
        context->client_addr = client_addr;
        context->delay_seconds = (unsigned int)delay_seconds;
        context->drop_rate = drop_rate;

        if (pthread_create(&thread_id, NULL, handle_client, context) != 0) {
            perror("pthread_create failed");
            close(*client_socket_ptr);
            free(client_socket_ptr);
            free(context);
            update_connection_count(-1);
            continue;
        }

        if (pthread_detach(thread_id) != 0) {
            perror("pthread_detach failed");
        }
    }

    close(server_socket);
    printf("Server shutting down.\n");
    return EXIT_SUCCESS;
}
