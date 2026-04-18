#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        memset(buff, 0, MAX);
        printf("Enter the string: ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        
        // Send message to server
        write(sockfd, buff, sizeof(buff));
        
        // Read response from server
        memset(buff, 0, MAX);
        read(sockfd, buff, sizeof(buff));
        printf("From Server: %s", buff);
        
        // Check if client wants to exit
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    
    memset(&servaddr, 0, sizeof(servaddr));
    
    // Set server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    
    // Connect to server
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connection with the server failed...\n");
        exit(0);
    }
    else
        printf("Connected to the server..\n");
    
    // Start communication with server
    func(sockfd);
    
    // Close socket
    close(sockfd);
    
    return 0;
}
