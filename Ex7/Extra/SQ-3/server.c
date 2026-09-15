#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8200

void *client_handler(void *arg)
{
    int client = *(int *)arg;
    int choice;
    char filename[100];
    char buffer[1024];
    FILE *fp;
    int n;

    free(arg);

    while (1)
    {
        if (recv(client, &choice, sizeof(choice), 0) <= 0)
            break;

        if (choice == 1)
        {
            strcpy(buffer,
                   "Available Files:\n"
                   "1. file1.txt\n"
                   "2. file2.txt\n"
                   "3. file3.txt");

            send(client, buffer, sizeof(buffer), 0);
        }

        else if (choice == 2)
        {
            recv(client, filename, sizeof(filename), 0);

            fp = fopen(filename, "rb");

            if (fp == NULL)
            {
                strcpy(buffer, "File not found.");
                send(client, buffer, sizeof(buffer), 0);
            }
            else
            {
                strcpy(buffer, "FILE_FOUND");
                send(client, buffer, sizeof(buffer), 0);

                while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
                {
                    send(client, buffer, n, 0);
                }

                strcpy(buffer, "FILE_END");
                send(client, buffer, sizeof(buffer), 0);

                fclose(fp);
            }
        }

        else if (choice == 3)
        {
            break;
        }

        else
        {
            strcpy(buffer, "Invalid choice.");
            send(client, buffer, sizeof(buffer), 0);
        }
    }

    close(client);

    return NULL;
}

int main()
{
    int server, client;
    struct sockaddr_in address;
    pthread_t thread;

    server = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server, (struct sockaddr *)&address, sizeof(address));

    listen(server, 5);

    printf("File Sharing Server started...\n");

    while (1)
    {
        client = accept(server, NULL, NULL);

        printf("Client connected.\n");

        int *p = malloc(sizeof(int));
        *p = client;

        pthread_create(&thread, NULL, client_handler, p);
        pthread_detach(thread);
    }

    close(server);

    return 0;
}
