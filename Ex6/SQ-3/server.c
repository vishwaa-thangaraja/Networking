#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8200

typedef struct
{
    int id;
    char name[50];
    char timing[20];
    int seats;
    float price;
} Movie;

typedef struct
{
    int bookingId;
    char customer[30];
    int movieId;
    int tickets;
    float amount;
} Booking;

Movie movies[3] =
{
    {1, "Avatar", "10:00 AM", 50, 200},
    {2, "Avengers", "2:00 PM", 40, 180},
    {3, "Interstellar", "6:00 PM", 30, 220}
};

Booking bookings[20];
int bookingCount = 0;
int nextBookingId = 1001;

void sendMovies(int client)
{
    char buffer[2000];
    int i;

    strcpy(buffer, "\n--- MOVIE LIST ---\n");

    for (i = 0; i < 3; i++)
    {
        char temp[200];

        sprintf(temp,
                "ID: %d | Movie: %s | Time: %s | Seats: %d | Price: %.2f\n",
                movies[i].id,
                movies[i].name,
                movies[i].timing,
                movies[i].seats,
                movies[i].price);

        strcat(buffer, temp);
    }

    send(client, buffer, strlen(buffer) + 1, 0);
}

void checkSeats(int client)
{
    int movieId;
    char buffer[200];

    recv(client, &movieId, sizeof(movieId), 0);

    if (movieId < 1 || movieId > 3)
    {
        strcpy(buffer, "Invalid movie ID.");
    }
    else
    {
        sprintf(buffer,
                "Available seats for %s: %d",
                movies[movieId - 1].name,
                movies[movieId - 1].seats);
    }

    send(client, buffer, strlen(buffer) + 1, 0);
}

void bookTickets(int client)
{
    int movieId, tickets;
    char customer[30];
    char buffer[300];

    recv(client, customer, sizeof(customer), 0);
    recv(client, &movieId, sizeof(movieId), 0);
    recv(client, &tickets, sizeof(tickets), 0);

    if (movieId < 1 || movieId > 3)
    {
        strcpy(buffer, "Invalid movie ID.");
    }
    else if (tickets <= 0)
    {
        strcpy(buffer, "Invalid number of tickets.");
    }
    else if (tickets > movies[movieId - 1].seats)
    {
        strcpy(buffer, "Booking failed. Not enough seats.");
    }
    else
    {
        bookings[bookingCount].bookingId = nextBookingId;
        strcpy(bookings[bookingCount].customer, customer);
        bookings[bookingCount].movieId = movieId;
        bookings[bookingCount].tickets = tickets;
        bookings[bookingCount].amount =
            tickets * movies[movieId - 1].price;

        movies[movieId - 1].seats -= tickets;

        sprintf(buffer,
                "Booking successful!\nBooking ID: %d\nMovie: %s\nTickets: %d\nAmount: %.2f",
                nextBookingId,
                movies[movieId - 1].name,
                tickets,
                bookings[bookingCount].amount);

        bookingCount++;
        nextBookingId++;
    }

    send(client, buffer, strlen(buffer) + 1, 0);
}

void cancelBooking(int client)
{
    int bookingId;
    char buffer[300];
    int i;

    recv(client, &bookingId, sizeof(bookingId), 0);

    for (i = 0; i < bookingCount; i++)
    {
        if (bookings[i].bookingId == bookingId)
        {
            int movieId = bookings[i].movieId;

            movies[movieId - 1].seats += bookings[i].tickets;

            sprintf(buffer,
                    "Booking %d cancelled successfully.",
                    bookingId);

            bookings[i] = bookings[bookingCount - 1];
            bookingCount--;

            send(client, buffer, strlen(buffer) + 1, 0);
            return;
        }
    }

    strcpy(buffer, "Booking ID not found.");
    send(client, buffer, strlen(buffer) + 1, 0);
}

void bookingDetails(int client)
{
    int bookingId;
    char buffer[500];
    int i;

    recv(client, &bookingId, sizeof(bookingId), 0);

    for (i = 0; i < bookingCount; i++)
    {
        if (bookings[i].bookingId == bookingId)
        {
            sprintf(buffer,
                    "\n--- BOOKING DETAILS ---\n"
                    "Booking ID: %d\n"
                    "Customer: %s\n"
                    "Movie: %s\n"
                    "Timing: %s\n"
                    "Tickets: %d\n"
                    "Amount: %.2f",
                    bookings[i].bookingId,
                    bookings[i].customer,
                    movies[bookings[i].movieId - 1].name,
                    movies[bookings[i].movieId - 1].timing,
                    bookings[i].tickets,
                    bookings[i].amount);

            send(client, buffer, strlen(buffer) + 1, 0);
            return;
        }
    }

    strcpy(buffer, "Booking ID not found.");
    send(client, buffer, strlen(buffer) + 1, 0);
}

int main()
{
    int server, client;
    struct sockaddr_in address;
    int option = 1;
    int choice;

    server = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server, (struct sockaddr *)&address, sizeof(address));
    listen(server, 5);

    printf("Movie Ticket Server started...\n");

    while (1)
    {
        client = accept(server, NULL, NULL);

        printf("Client connected.\n");

        while (1)
        {
            if (recv(client, &choice, sizeof(choice), 0) <= 0)
                break;

            if (choice == 1)
                sendMovies(client);

            else if (choice == 2)
                checkSeats(client);

            else if (choice == 3)
                bookTickets(client);

            else if (choice == 4)
                cancelBooking(client);

            else if (choice == 5)
                bookingDetails(client);

            else if (choice == 6)
                break;

            else
            {
                char buffer[] = "Invalid choice.";
                send(client, buffer, strlen(buffer) + 1, 0);
            }
        }

        close(client);
        printf("Client disconnected.\n");
    }

    close(server);

    return 0;
}
