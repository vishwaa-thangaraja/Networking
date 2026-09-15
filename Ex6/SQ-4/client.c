#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8300

int main()
{
    int client;
    struct sockaddr_in server;
    int choice;

    client = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server, sizeof(server));

    while (1)
    {
        printf("\n===== LIBRARY MANAGEMENT =====\n");
        printf("1. Search Book by Title\n");
        printf("2. Search Book by Author\n");
        printf("3. Check Availability\n");
        printf("4. Issue Book\n");
        printf("5. Return Book\n");
        printf("6. View Issued Book Details\n");
        printf("7. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        send(client, &choice, sizeof(choice), 0);

        if (choice == 1)
        {
            char title[50];
            char buffer[1000];

            printf("Enter title: ");
            scanf("%s", title);

            send(client, title, sizeof(title), 0);

            recv(client, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 2)
        {
            char author[50];
            char buffer[2000];

            printf("Enter author: ");
            scanf("%s", author);

            send(client, author, sizeof(author), 0);

            recv(client, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 3)
        {
            int bookId;
            char buffer[200];

            printf("Enter book ID: ");
            scanf("%d", &bookId);

            send(client, &bookId, sizeof(bookId), 0);

            recv(client, buffer, sizeof(buffer), 0);

            printf("%s\n", buffer);
        }

        else if (choice == 4)
        {
            char student[30];
            int bookId;
            char buffer[300];

            printf("Enter student name: ");
            scanf("%s", student);

            printf("Enter book ID: ");
            scanf("%d", &bookId);

            send(client, student, sizeof(student), 0);
            send(client, &bookId, sizeof(bookId), 0);

            recv(client, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 5)
        {
            int issueId;
            char buffer[300];

            printf("Enter issue ID: ");
            scanf("%d", &issueId);

            send(client, &issueId, sizeof(issueId), 0);

            recv(client, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 6)
        {
            char buffer[2000];

            recv(client, buffer, sizeof(buffer), 0);

            printf("%s\n", buffer);
        }

        else if (choice == 7)
        {
            break;
        }

        else
        {
            char buffer[100];

            recv(client, buffer, sizeof(buffer), 0);

            printf("%s\n", buffer);
        }
    }

    close(client);

    return 0;
}
