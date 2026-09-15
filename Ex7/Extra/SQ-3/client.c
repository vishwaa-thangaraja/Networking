#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8200

int main()
{
    int client;
    struct sockaddr_in server;
    int choice;
    char filename[100];
    char buffer[1024];
    FILE *fp;
    int n;

    client = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server, sizeof(server));

    while (1)
    {
        printf("\n===== FILE SHARING =====\n");
        printf("1. View Files\n");
        printf("2. Download File\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        send(client, &choice, sizeof(choice), 0);

        if (choice == 1)
        {
            recv(client, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 2)
        {
            printf("Enter filename: ");
            scanf("%s", filename);

            send(client, filename, sizeof(filename), 0);

            recv(client, buffer, sizeof(buffer), 0);

            if (strcmp(buffer, "File not found.") == 0)
            {
                printf("%s\n", buffer);
            }
            else
            {
                printf("File found. Downloading...\n");

                fp = fopen(filename, "wb");

                while (1)
                {
                    n = recv(client, buffer, sizeof(buffer), 0);

                    if (n <= 0)
                        break;

                    if (strcmp(buffer, "FILE_END") == 0)
                        break;

                    fwrite(buffer, 1, n, fp);
                }

                fclose(fp);

                printf("File downloaded successfully.\n");
            }
        }

        else if (choice == 3)
        {
            break;
        }

        else
        {
            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }
    }

    close(client);

    return 0;
}
