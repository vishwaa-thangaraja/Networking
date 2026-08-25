#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000 // Updated to match server port
#define BUFFER_SIZE 1024
int main()
{
    int sockfd, n;
    struct sockaddr_in server_addr;
    char ip_addr[50], buffer[BUFFER_SIZE];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(sockfd);
        exit(1);
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        exit(1);
    }
    printf("Connected to ARP server.\n");
    printf("Enter IP address: ");
    scanf("%s", ip_addr);
    send(sockfd, ip_addr, strlen(ip_addr), 0);
    n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) printf("No response from server.\n");
    else
    {
        buffer[n] = '\0';
        printf("Server Response: %s\n", buffer);
    }
    close(sockfd);
    return 0;
}
