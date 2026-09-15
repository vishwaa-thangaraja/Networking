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
    int port;

    struct sockaddr_in server_addr;

    char domain[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    /* Check command line arguments */
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[2]);

    if (port <= 0 || port > 65535)
    {
        printf("Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Set server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET,
                  argv[1],
                  &server_addr.sin_addr) <= 0)
    {
        printf("Invalid server IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(server_addr);

    printf("Connected to DNS Server %s:%d\n",
           argv[1], port);

    printf("Type 'exit' to stop.\n\n");

    while (1)
    {
        /* Get domain name */
        printf("Enter Domain Name: ");

        if (fgets(domain, BUFFER_SIZE, stdin) == NULL)
        {
            break;
        }

        domain[strcspn(domain, "\n")] = '\0';

        /* Exit */
        if (strcmp(domain, "exit") == 0)
        {
            printf("DNS client terminated.\n");
            break;
        }

        /* Send domain name to server */
        if (sendto(sockfd,
                   domain,
                   strlen(domain),
                   0,
                   (struct sockaddr *)&server_addr,
                   addr_len) < 0)
        {
            perror("sendto failed");
            break;
        }

        /* Receive response */
        ssize_t bytes_received = recvfrom(
            sockfd,
            response,
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

        response[bytes_received] = '\0';

        /* Display result */
        printf("\nDNS Server Response:\n");
        printf("%s\n\n", response);
    }

    close(sockfd);

    return 0;
}