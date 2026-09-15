#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#define BUFFER_SIZE 1024
#define MAX_SUBNETS 10
#define MAX_IPS 256
int subnet_count, ip1, ip2, ip3, ip4;
int total_ips[MAX_SUBNETS], used[MAX_SUBNETS][MAX_IPS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct ClientData
{
    int sockfd;
    char message[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
};
void *handleClient(void *arg)
{
    struct ClientData *data;
    char response[BUFFER_SIZE];
    data = (struct ClientData *)arg;
    pthread_mutex_lock(&lock);
    if (strncmp(data->message, "SETUP", 5) == 0)
    {
        int total, required_subnets, i, j, base, remainder;
        if (sscanf(data->message, "SETUP %d %d",&total, &required_subnets) != 2)
        {
            strcpy(response, "Invalid SETUP format.");
            sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
            pthread_mutex_unlock(&lock);
            free(data);
            return NULL;
        }
        if (required_subnets < 1 || required_subnets > MAX_SUBNETS)
        {
            strcpy(response, "Invalid number of blocks.");
            sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr, data->addrLen);
            pthread_mutex_unlock(&lock);
            free(data);
            return NULL;
        }
        if (total <= 0 || total > MAX_IPS)
        {
            strcpy(response, "Invalid total number of IP addresses.");
            sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
            pthread_mutex_unlock(&lock);
            free(data);
            return NULL;
        }
        subnet_count = required_subnets;
        ip1 = 192;ip2 = 168;ip3 = 1 + rand() % 254;ip4 = 0;
        base = total / subnet_count;
        remainder = total % subnet_count;
        for (i = 0; i < subnet_count; i++)
        {
            total_ips[i] = base;
            if (i < remainder) total_ips[i]++;
            for (j = 0; j < MAX_IPS; j++) used[i][j] = 0;
        }
        sprintf(response,"DHCP Server Setup\n\n""Random IP Block: %d.%d.%d.%d\n""Total IP Addresses: %d\n"
                "Number of Blocks: %d\n\n",ip1, ip2, ip3, ip4,total, subnet_count);
        for (i = 0; i < subnet_count; i++)
        {
            char temp[200];
            sprintf(temp,"Block %d:\n""Available IP Addresses: %d\n\n",i + 1,total_ips[i]);
            strcat(response, temp);
        }
        printf("\nClient: %s\n",inet_ntoa(data->clientAddr.sin_addr));
        printf("%s", response);
        sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    }
    else if (strncmp(data->message, "ALLOCATE", 8) == 0)
    {
        int subnet, required, i, count = 0, start;
        if (sscanf(data->message, "ALLOCATE %d %d",&subnet, &required) != 2)strcpy(response, "Invalid ALLOCATE format.");
        else if (subnet < 1 || subnet > subnet_count)strcpy(response, "Invalid block number.");
        else if (required <= 0)strcpy(response, "Invalid number of IP addresses.");
        else
        {
            subnet--;
            if (required > total_ips[subnet])strcpy(response,"Not enough IP addresses available in this block.");
            else
            {
                strcpy(response,"IP addresses allotted successfully:\n\n");
                start = 0;
                for (i = 0;i < total_ips[subnet] && count < required;i++)
                    if (used[subnet][i] == 0)
                    {
                        char temp[50];
                        used[subnet][i] = 1;
                        sprintf(temp,"%d.%d.%d.%d\n",ip1,ip2,ip3,subnet * 50 + i + 1);
                        strcat(response, temp);
                        count++;
                    }
            }
        }
        printf("\nClient: %s\n",inet_ntoa(data->clientAddr.sin_addr));
        printf("%s\n", response);
        sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    }
    else if (strcmp(data->message, "EXIT") == 0)
    {
        strcpy(response, "DHCP client disconnected.");
        sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
        printf("\nClient disconnected\n");
    }
    else
    {
        strcpy(response, "Invalid request.");
        sendto(data->sockfd, response, strlen(response), 0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    }
    pthread_mutex_unlock(&lock);
    free(data);
    return NULL;
}
int main()
{
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen;
    srand(time(NULL));
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5500);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Bind failed\n");
        close(sockfd);
        return 1;
    }
    printf("UDP Concurrent DHCP Server is running...\n");
    while (1)
    {
        int n;
        addrLen = sizeof(clientAddr);
        n = recvfrom(sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr *)&clientAddr,&addrLen);
        if (n < 0)
        {
            printf("Receive failed\n");
            continue;
        }
        buffer[n] = '\0';
        struct ClientData *data;
        data = malloc(sizeof(struct ClientData));
        data->sockfd = sockfd;
        strcpy(data->message, buffer);
        data->clientAddr = clientAddr;
        data->addrLen = addrLen;
        pthread_t thread;
        pthread_create(&thread,NULL,handleClient,data);
        pthread_detach(thread);
    }
    close(sockfd);
    return 0;
}