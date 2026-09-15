#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8300

int client;

void *receive_messages(void *arg)
{
    char message[1024];
    int n;

    while (1)
    {
        n = recv(client, message, sizeof(message), 0);

        if (n <= 0)
        {
            printf("\nServer disconnected.\n");
            exit(0);
        }

        message[n] = '\0';

        printf("\n%s\n", message);
        printf("Enter message: ");
        fflush(stdout);
    }

    return NULL;
}

int main()
{
    struct sockaddr_in server;
    pthread_t thread;
    char message[1024];

    client = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server, sizeof(server));

    pthread_create(&thread, NULL, receive_messages, NULL);

    while (1)
    {
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);

        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "exit") == 0)
            break;

        send(client, message, strlen(message) + 1, 0);
    }

    close(client);

    return 0;
}
