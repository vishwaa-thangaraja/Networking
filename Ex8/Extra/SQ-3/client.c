#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8200

int main()
{
    int sock;
    struct sockaddr_in server;
    int choice;
    char filename[100];
    char buffer[1024];
    FILE *fp;
    int n;
    long size;
    long received;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    while (1)
    {
        printf("\n===== FILE SHARING =====\n");
        printf("1. View Files\n");
        printf("2. Download File\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        send(sock, &choice, sizeof(choice), 0);

        if (choice == 1)
        {
            recv(sock, buffer, sizeof(buffer), 0);

            printf("\nAvailable Files:\n%s\n", buffer);
        }

        else if (choice == 2)
        {
            printf("Enter filename: ");
            scanf("%s", filename);

            send(sock, filename, sizeof(filename), 0);

            recv(sock, &size, sizeof(size), 0);

            if (size == -1)
            {
                printf("File not found.\n");
            }
            else
            {
                fp = fopen(filename, "wb");

                received = 0;

                while (received < size)
                {
                    n = recv(sock, buffer, sizeof(buffer), 0);

                    if (n <= 0)
                        break;

                    fwrite(buffer, 1, n, fp);

                    received = received + n;
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
            recv(sock, buffer, sizeof(buffer), 0);

            printf("%s\n", buffer);
        }
    }

    close(sock);

    return 0;
}
