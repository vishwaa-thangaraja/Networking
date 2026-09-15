#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
int main()
{
    int sockfd;
    char message[1024],buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("Connected to DHCP Server\n\n");
    char ip[50];
    int subnets;
    printf("Enter IP block: ");
    scanf("%49s", ip);
    printf("Enter number of subnets: ");
    scanf("%d", &subnets);
    sprintf(message,"SETUP %s %d",ip,subnets);
    sendto(sockfd,message,strlen(message),0,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
    int n;
    n = recvfrom(sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr *)&serverAddr,&addrLen);
    if (n < 0)
    {
        printf("Receive failed\n");
        close(sockfd);
        return 1;
    }
    buffer[n] = '\0';
    printf("\n%s\n", buffer);
    while (1)
    {
        int subnet, required;
        printf("Enter subnet number (0 to exit): ");
        scanf("%d", &subnet);
        if (subnet == 0)
        {
            strcpy(message, "EXIT");
            sendto(sockfd,message,strlen(message),0,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
            n = recvfrom(sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr *)&serverAddr,&addrLen);
            if (n >= 0)
            {
                buffer[n] = '\0';
                printf("\n%s\n", buffer);
            }
            break;
        }
        printf("Enter number of IP addresses to allot: ");
        scanf("%d", &required);
        sprintf(message,"ALLOCATE %d %d",subnet,required);
        sendto(sockfd,message,strlen(message),0,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
        n = recvfrom(sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr *)&serverAddr,&addrLen);
        if (n < 0)
        {
            printf("Receive failed\n");
            break;
        }
        buffer[n] = '\0';
        printf("\nDHCP Server Response:\n");
        printf("%s\n", buffer);
    }
    close(sockfd);
    return 0;
}