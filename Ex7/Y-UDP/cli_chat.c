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
    char buffer[BUFFER_SIZE];
    int port;

    /* Validate command line arguments */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[2]);

    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
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

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid server IP address: %s\n", argv[1]);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(server_addr);

    printf("Connected to chat server at %s:%d\n",
           argv[1], port);

    printf("Type 'exit' to end the chat.\n\n");

    while (1)
    {
        /* Get message from client */
        printf("You: ");

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        {
            strcpy(buffer, "exit");
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        /* Send message to server */
        if (sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr,
                   addr_len) < 0)
        {
            perror("sendto failed");
            break;
        }

        /* Client wants to exit */
        if (strcmp(buffer, "exit") == 0)
        {
            printf("You ended the chat.\n");
            break;
        }

        /* Receive reply from server */
        ssize_t bytes_received = recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr *)&server_addr,
            &addr_len
        );

        if (bytes_received < 0)
        {
            perror("recvfrom failed");
            break;
        }

        buffer[bytes_received] = '\0';

        printf("Server: %s\n", buffer);

        /* Server wants to end the chat */
        if (strcmp(buffer, "exit") == 0)
        {
            printf("Server ended the chat.\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}