#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define BUFFER_SIZE 1024
#define MAX_SUBNETS 10
#define MAX_IPS 256
int subnet_count, total_ips[MAX_SUBNETS], used[MAX_SUBNETS][MAX_IPS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct ClientData
{
    int sockfd;
    char message[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
};
int power(int n)
{
    int result = 1, i;
    for (i = 0; i < n; i++) result = result * 2;
    return result;
}
void *handleClient(void *arg)
{
    struct ClientData *data;
    char response[BUFFER_SIZE];
    data = (struct ClientData *)arg;
    pthread_mutex_lock(&lock);
    if (strncmp(data->message, "SETUP", 5) == 0)
    {
        char ip[50];
        int prefix, required_subnets, borrowed_bits = 0, new_prefix, host_bits, addresses, i, j;
        if (sscanf(data->message,"SETUP %49[^/]/%d %d",ip,&prefix,&required_subnets) != 3)
        {
            strcpy(response, "Invalid SETUP format.");
            sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
            pthread_mutex_unlock(&lock);
            free(data);
            return NULL;
        }
        subnet_count = required_subnets;
        while (power(borrowed_bits) < subnet_count) borrowed_bits++;
        new_prefix = prefix + borrowed_bits;
        host_bits = 32 - new_prefix;
        addresses = power(host_bits);
        if (addresses > MAX_IPS) addresses = MAX_IPS;
        for (i = 0; i < subnet_count; i++)
        {
            total_ips[i] = addresses - 2;
            for (j = 0; j < MAX_IPS; j++) used[i][j] = 0;
        }
        sprintf(response,"Subnet calculation:\n\n""IP Block: %s/%d\n""Required Subnets: %d\n""Borrowed Bits: %d\n"
                "New Prefix: /%d\n\n",ip,prefix,subnet_count,borrowed_bits,new_prefix);
        for (i = 0; i < subnet_count; i++)
        {
            int block_size, network, first, last, broadcast;
            block_size = addresses;
            network = i * block_size;
            first = network + 1;
            last = network + block_size - 2;
            broadcast = network + block_size - 1;
            char temp[200];
            sprintf(temp,"Subnet %d:\n""Network: 192.168.1.%d\n""First Host: 192.168.1.%d\n""Last Host: 192.168.1.%d\n"
                    "Broadcast: 192.168.1.%d\n""Usable Hosts: %d\n\n",i + 1,network,first,last,broadcast,total_ips[i]);
            strcat(response, temp);
        }
        printf("\nClient: %s\n", inet_ntoa(data->clientAddr.sin_addr));
        printf("%s", response);
        sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    }
    else if (strncmp(data->message, "ALLOCATE", 8) == 0)
    {
        int subnet, required, i, count = 0;
        if (sscanf(data->message,"ALLOCATE %d %d",&subnet,&required) != 2) strcpy(response, "Invalid ALLOCATE format.");
        else if (subnet < 1 || subnet > subnet_count) strcpy(response, "Invalid subnet number.");
        else if (required <= 0) strcpy(response, "Invalid number of IP addresses.");
        else
        {
            subnet--;
            if (required >total_ips[subnet]) strcpy(response,"Not enough IP addresses available.");
            else
            {
                strcpy(response,"IP addresses allotted successfully:\n");
                for (i = 0;i < total_ips[subnet] &&count < required;i++)
                    if (used[subnet][i] == 0)
                    {
                        char temp[50];
                        used[subnet][i] = 1;
                        sprintf(temp,"192.168.1.%d\n",(subnet * (total_ips[subnet] + 2))+ i + 1);
                        strcat(response, temp);
                        count++;
                    }
            }
        }
        printf("\nClient: %s\n",inet_ntoa(data->clientAddr.sin_addr));
        printf("%s\n", response);
        sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    }
    else if (strcmp(data->message, "EXIT") == 0)
    {
        strcpy(response,"DHCP client disconnected.");
        sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
        printf("\nClient disconnected\n");
    }
    else
    {
        strcpy(response,"Invalid request.");
        sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
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
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
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