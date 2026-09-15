#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define BUFFER_SIZE 1024
#define TABLE_SIZE 10
typedef struct Node
{
    char domain[100];
    char ip[50];
    struct Node *next;
} Node;
Node *hashTable[TABLE_SIZE];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct ClientData
{
    int sockfd;
    char domain[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
};
int hashFunction(char *domain)
{
    int hash = 0, i;
    for (i = 0; domain[i] != '\0'; i++) hash = (hash + domain[i]) % TABLE_SIZE;
    return hash;
}
void insert(char *domain, char *ip)
{
    int index;
    Node *newNode;
    index = hashFunction(domain);
    newNode = (Node *)malloc(sizeof(Node));
    strcpy(newNode->domain, domain);
    strcpy(newNode->ip, ip);
    newNode->next = hashTable[index];
    hashTable[index] = newNode;
}
char *search(char *domain)
{
    int index;
    Node *current;
    index = hashFunction(domain);
    current = hashTable[index];
    while (current != NULL)
    {
        if (strcmp(current->domain, domain) == 0) return current->ip;
        current = current->next;
    }
    return NULL;
}
void *handleClient(void *arg)
{
    struct ClientData *data;
    char response[BUFFER_SIZE], *ip;
    data = (struct ClientData *)arg;
    pthread_mutex_lock(&lock);
    printf("Client [%s:%d] requested: %s\n",inet_ntoa(data->clientAddr.sin_addr),
           ntohs(data->clientAddr.sin_port),data->domain);
    ip = search(data->domain);
    if (ip != NULL) sprintf(response,"Domain: %s\nIP Address: %s",data->domain,ip);
    else strcpy(response,"Domain not found");
    printf("Result: %s\n\n", response);
    sendto(data->sockfd,response,strlen(response),0,(struct sockaddr *)&data->clientAddr,data->addrLen);
    pthread_mutex_unlock(&lock);
    free(data);
    return NULL;
}
int main()
{
    int sockfd, i;
    char domain[BUFFER_SIZE];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen;
    for (i = 0; i < TABLE_SIZE; i++) hashTable[i] = NULL;
    insert("google.com", "142.250.195.14");
    insert("youtube.com", "142.250.72.206");
    insert("facebook.com", "157.240.241.35");
    insert("example.com", "93.184.216.34");
    insert("amazon.com", "98.137.11.163");
    insert("github.com", "140.82.114.4");
    insert("wikipedia.org", "208.80.154.224");
    insert("instagram.com", "157.240.241.174");
    insert("microsoft.com", "20.112.250.133");
    insert("apple.com", "17.253.144.10");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5050);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
    {
        printf("Bind failed\n");
        close(sockfd);
        return 1;
    }
    printf("DNS UDP Concurrent Server is running...\nHash table size: %d\n", TABLE_SIZE);
    printf("Waiting for clients...\n\n");
    while (1)
    {
        int n;
        struct ClientData *data;
        pthread_t thread;
        addrLen = sizeof(clientAddr);
        n = recvfrom(sockfd,domain,sizeof(domain) - 1,0,(struct sockaddr *)&clientAddr,&addrLen);
        if (n < 0)
        {
            printf("Receive failed\n");
            continue;
        }
        domain[n] = '\0';
        data = malloc(sizeof(struct ClientData));
        data->sockfd = sockfd;
        strcpy(data->domain, domain);
        data->clientAddr = clientAddr;
        data->addrLen = addrLen;
        pthread_create(&thread,NULL,handleClient,data);
        pthread_detach(thread);
    }
    close(sockfd);
    return 0;
}