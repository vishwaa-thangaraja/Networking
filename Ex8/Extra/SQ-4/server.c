#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8300
#define MAX 10

int clients[MAX] = {0};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *client(void *arg)
{
    int sock = *(int *)arg;
    int i;
    int n;
    char message[1024];

    free(arg);

    pthread_mutex_lock(&lock);

    for (i = 0; i < MAX; i++)
    {
        if (clients[i] == 0)
        {
            clients[i] = sock;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    while (1)
    {
        n = recv(sock, message, sizeof(message), 0);

        if (n <= 0)
            break;

        message[n] = '\0';

        pthread_mutex_lock(&lock);

        for (i = 0; i < MAX; i++)
        {
            if (clients[i] != 0 && clients[i] != sock)
            {
                send(clients[i], message, strlen(message) + 1, 0);
            }
        }

        pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);

    for (i = 0; i < MAX; i++)
    {
        if (clients[i] == sock)
        {
            clients[i] = 0;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    close(sock);

    printf("Client disconnected.\n");

    return NULL;
}

int main()
{
    int server, sock;
    struct sockaddr_in address;
    pthread_t thread;

    server = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server, (struct sockaddr *)&address, sizeof(address));

    listen(server, 5);

    printf("Chat Server started...\n");

    while (1)
    {
        sock = accept(server, NULL, NULL);

        printf("Client connected.\n");

        int *p = malloc(sizeof(int));
        *p = sock;

        pthread_create(&thread, NULL, client, p);
        pthread_detach(thread);
    }

    close(server);

    return 0;
}
