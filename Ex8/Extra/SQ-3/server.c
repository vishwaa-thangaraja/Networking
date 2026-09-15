#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8200

void *client(void *arg)
{
    int sock = *(int *)arg;
    int choice;
    char filename[100];
    char buffer[1024];
    FILE *fp;
    int n;
    long size;

    free(arg);

    while (1)
    {
        if (recv(sock, &choice, sizeof(choice), 0) <= 0)
            break;

        if (choice == 1)
        {
            strcpy(buffer,
                   "file1.txt\n"
                   "file2.txt\n"
                   "file3.txt");

            send(sock, buffer, sizeof(buffer), 0);
        }

        else if (choice == 2)
        {
            recv(sock, filename, sizeof(filename), 0);

            fp = fopen(filename, "rb");

            if (fp == NULL)
            {
                size = -1;
                send(sock, &size, sizeof(size), 0);
            }
            else
            {
                fseek(fp, 0, SEEK_END);
                size = ftell(fp);
                rewind(fp);

                send(sock, &size, sizeof(size), 0);

                while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
                {
                    send(sock, buffer, n, 0);
                }

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
            send(sock, buffer, sizeof(buffer), 0);
        }
    }

    close(sock);

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

    printf("File Sharing Server started...\n");

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
