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

    client = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server, sizeof(server));

    while (1)
    {
        printf("\n===== MOVIE TICKET BOOKING =====\n");
        printf("1. View Movies\n");
        printf("2. Check Seat Availability\n");
        printf("3. Book Tickets\n");
        printf("4. Cancel Booking\n");
        printf("5. Booking Details\n");
        printf("6. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        send(client, &choice, sizeof(choice), 0);

        if (choice == 1)
        {
            char buffer[2000];

            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }

        else if (choice == 2)
        {
            int movieId;
            char buffer[200];

            printf("Enter movie ID: ");
            scanf("%d", &movieId);

            send(client, &movieId, sizeof(movieId), 0);

            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }

        else if (choice == 3)
        {
            char customer[30];
            int movieId, tickets;
            char buffer[300];

            printf("Enter customer name: ");
            scanf("%s", customer);

            printf("Enter movie ID: ");
            scanf("%d", &movieId);

            printf("Enter number of tickets: ");
            scanf("%d", &tickets);

            send(client, customer, sizeof(customer), 0);
            send(client, &movieId, sizeof(movieId), 0);
            send(client, &tickets, sizeof(tickets), 0);

            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }

        else if (choice == 4)
        {
            int bookingId;
            char buffer[300];

            printf("Enter booking ID: ");
            scanf("%d", &bookingId);

            send(client, &bookingId, sizeof(bookingId), 0);

            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }

        else if (choice == 5)
        {
            int bookingId;
            char buffer[500];

            printf("Enter booking ID: ");
            scanf("%d", &bookingId);

            send(client, &bookingId, sizeof(bookingId), 0);

            recv(client, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        }

        else if (choice == 6)
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
