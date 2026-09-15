#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    char buffer[BUFFER_SIZE];
    int port;

    /* Validate command line arguments */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);

    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* Set up server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Bind socket */
    if (bind(sockfd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Chat Server listening on port %d...\n", port);
    printf("Server is ready for multiple clients.\n\n");

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);

        /* Receive message from any client */
        ssize_t bytes_received = recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr *)&client_addr,
            &addr_len
        );

        if (bytes_received < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        buffer[bytes_received] = '\0';

        /* Display client information */
        printf("Client [%s:%d]: %s\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buffer);

        /* Client wants to exit */
        if (strcmp(buffer, "exit") == 0)
        {
            printf("Client ended the chat.\n");
            printf("Waiting for another client...\n\n");

            /* Server continues running */
            continue;
        }

        /* Get reply from server user */
        printf("You: ");

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        {
            strcpy(buffer, "exit");
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        /* Send reply to the same client */
        if (sendto(sockfd,
                   buffer,
                   strlen(buffer),
                   0,
                   (struct sockaddr *)&client_addr,
                   addr_len) < 0)
        {
            perror("sendto failed");
            continue;
        }

        /* Server wants to end this particular chat */
        if (strcmp(buffer, "exit") == 0)
        {
            printf("Chat with this client ended.\n");
            printf("Waiting for another client...\n\n");
        }

        printf("\n");
    }

    close(sockfd);

    return 0;
}