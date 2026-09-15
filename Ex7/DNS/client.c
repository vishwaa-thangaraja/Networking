#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUFFER_SIZE 1024
int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in serverAddr;
    char domain[BUFFER_SIZE], response[BUFFER_SIZE];
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n",argv[0]);
        return 1;
    }
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535)
    {
        printf("Invalid port number\n");
        return 1;
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET,argv[1],&serverAddr.sin_addr) <= 0)
    {
        printf("Invalid server IP address\n");
        close(sockfd);
        return 1;
    }
    socklen_t addrLen = sizeof(serverAddr);
    printf("Connected to DNS Server %s:%d\n",argv[1],port);
    printf("Type 'exit' to stop.\n\n");
    while (1)
    {
        printf("Enter Domain Name: ");
        fgets(domain,sizeof(domain),stdin);
        domain[strcspn(domain, "\n")] = '\0';
        if (strcmp(domain, "exit") == 0)
        {
            printf("DNS client terminated.\n");
            break;
        }
        if (sendto(sockfd,domain,strlen(domain),0,(struct sockaddr *)&serverAddr,addrLen) < 0)
        {
            printf("Send failed\n");
            break;
        }
        int n = recvfrom(sockfd,response,sizeof(response) - 1,0,(struct sockaddr *)&serverAddr,&addrLen);
        if (n < 0)
        {
            printf("Receive failed\n");
            break;
        }
        response[n] = '\0';
        printf("\nDNS Server Response:\n");
        printf("%s\n\n", response);
    }
    close(sockfd);
    return 0;
}