#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int sockfd;
int running = 1;

struct sockaddr_in serverAddr;

/* Receives messages from server */
void *receiveMessages(void *arg)
{
    char buffer[BUFFER_SIZE];
    int n;
    socklen_t addrLen;

    while (running)
    {
        addrLen = sizeof(serverAddr);

        n = recvfrom(sockfd,
                     buffer,
                     sizeof(buffer) - 1,
                     0,
                     (struct sockaddr *)&serverAddr,
                     &addrLen);

        if (n < 0)
        {
            printf("Receive failed\n");
            break;
        }

        buffer[n] = '\0';

        printf("\n%s\n", buffer);
        printf("You: ");
        fflush(stdout);
    }

    return NULL;
}

int main()
{
    char buffer[BUFFER_SIZE];

    pthread_t receiveThread;

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

    printf("Connected to Chat Server\n");
    printf("Type 'exit' to leave the chat.\n\n");

    /* Thread for receiving messages */
    pthread_create(&receiveThread,
                   NULL,
                   receiveMessages,
                   NULL);

    while (running)
    {
        printf("You: ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (sendto(sockfd,
                   buffer,
                   strlen(buffer),
                   0,
                   (struct sockaddr *)&serverAddr,
                   sizeof(serverAddr)) < 0)
        {
            printf("Send failed\n");
            break;
        }

        if (strcmp(buffer, "exit") == 0)
        {
            running = 0;
            break;
        }
    }

    close(sockfd);

    return 0;
}