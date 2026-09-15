#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8300

typedef struct
{
    int id;
    char title[50];
    char author[50];
    char category[30];
    int available;
} Book;

typedef struct
{
    int issueId;
    int bookId;
    char student[30];
} Issue;

Book books[5] =
{
    {1, "CProgramming", "Dennis", "Programming", 1},
    {2, "Java", "Herbert", "Programming", 1},
    {3, "ComputerNetworks", "Tanenbaum", "Networking", 1},
    {4, "OperatingSystems", "Galvin", "OS", 1},
    {5, "DatabaseSystems", "Korth", "Database", 1}
};

Issue issues[20];

int issueCount = 0;
int nextIssueId = 1001;

void searchTitle(int client)
{
    char title[50];
    char buffer[1000];
    int i, found = 0;

    recv(client, title, sizeof(title), 0);

    strcpy(buffer, "");

    for (i = 0; i < 5; i++)
    {
        if (strcmp(books[i].title, title) == 0)
        {
            sprintf(buffer,
                    "Book ID: %d\nTitle: %s\nAuthor: %s\nCategory: %s\nAvailability: %s",
                    books[i].id,
                    books[i].title,
                    books[i].author,
                    books[i].category,
                    books[i].available ? "Available" : "Not Available");

            found = 1;
            break;
        }
    }

    if (!found)
        strcpy(buffer, "Book not found.");

    send(client, buffer, strlen(buffer) + 1, 0);
}

void searchAuthor(int client)
{
    char author[50];
    char buffer[2000];
    int i, found = 0;

    recv(client, author, sizeof(author), 0);

    strcpy(buffer, "");

    for (i = 0; i < 5; i++)
    {
        if (strcmp(books[i].author, author) == 0)
        {
            char temp[300];

            sprintf(temp,
                    "Book ID: %d | Title: %s | Category: %s | Availability: %s\n",
                    books[i].id,
                    books[i].title,
                    books[i].category,
                    books[i].available ? "Available" : "Not Available");

            strcat(buffer, temp);
            found = 1;
        }
    }

    if (!found)
        strcpy(buffer, "No books found for this author.");

    send(client, buffer, strlen(buffer) + 1, 0);
}

void checkAvailability(int client)
{
    int bookId;
    char buffer[200];

    recv(client, &bookId, sizeof(bookId), 0);

    if (bookId < 1 || bookId > 5)
    {
        strcpy(buffer, "Invalid book ID.");
    }
    else
    {
        sprintf(buffer,
                "%s is %s.",
                books[bookId - 1].title,
                books[bookId - 1].available ?
                "Available" : "Not Available");
    }

    send(client, buffer, strlen(buffer) + 1, 0);
}

void issueBook(int client)
{
    int bookId;
    char student[30];
    char buffer[300];

    recv(client, student, sizeof(student), 0);
    recv(client, &bookId, sizeof(bookId), 0);

    if (bookId < 1 || bookId > 5)
    {
        strcpy(buffer, "Invalid book ID.");
    }
    else if (!books[bookId - 1].available)
    {
        strcpy(buffer, "Book is not available.");
    }
    else
    {
        issues[issueCount].issueId = nextIssueId;
        issues[issueCount].bookId = bookId;
        strcpy(issues[issueCount].student, student);

        books[bookId - 1].available = 0;

        sprintf(buffer,
                "Book issued successfully!\n"
                "Issue ID: %d\n"
                "Book: %s\n"
                "Student: %s",
                nextIssueId,
                books[bookId - 1].title,
                student);

        issueCount++;
        nextIssueId++;
    }

    send(client, buffer, strlen(buffer) + 1, 0);
}

void returnBook(int client)
{
    int issueId;
    char buffer[300];
    int i;

    recv(client, &issueId, sizeof(issueId), 0);

    for (i = 0; i < issueCount; i++)
    {
        if (issues[i].issueId == issueId)
        {
            int bookId = issues[i].bookId;

            books[bookId - 1].available = 1;

            sprintf(buffer,
                    "Book returned successfully.\nBook: %s",
                    books[bookId - 1].title);

            issues[i] = issues[issueCount - 1];
            issueCount--;

            send(client, buffer, strlen(buffer) + 1, 0);
            return;
        }
    }

    strcpy(buffer, "Issue ID not found.");

    send(client, buffer, strlen(buffer) + 1, 0);
}

void issuedDetails(int client)
{
    char buffer[2000];
    int i;

    strcpy(buffer, "\n--- ISSUED BOOK DETAILS ---\n");

    if (issueCount == 0)
    {
        strcpy(buffer, "No books are currently issued.");
    }
    else
    {
        for (i = 0; i < issueCount; i++)
        {
            char temp[300];

            sprintf(temp,
                    "Issue ID: %d | Book: %s | Student: %s\n",
                    issues[i].issueId,
                    books[issues[i].bookId - 1].title,
                    issues[i].student);

            strcat(buffer, temp);
        }
    }

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

    printf("Library Server started...\n");

    while (1)
    {
        client = accept(server, NULL, NULL);

        printf("Client connected.\n");

        while (1)
        {
            if (recv(client, &choice, sizeof(choice), 0) <= 0)
                break;

            if (choice == 1)
                searchTitle(client);

            else if (choice == 2)
                searchAuthor(client);

            else if (choice == 3)
                checkAvailability(client);

            else if (choice == 4)
                issueBook(client);

            else if (choice == 5)
                returnBook(client);

            else if (choice == 6)
                issuedDetails(client);

            else if (choice == 7)
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
