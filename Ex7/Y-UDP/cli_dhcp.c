#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    int sockfd;
    int port;

    struct sockaddr_in server_addr;

    char ip_block[50];
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    int subnet_count;

    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n",
               argv[0]);
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

    socklen_t addr_len =
        sizeof(server_addr);

    printf("Connected to DHCP Server %s:%d\n\n",
           argv[1],
           port);

    /* ---------------- GET IP BLOCK ---------------- */

    printf("Enter IP block: ");
    scanf("%49s", ip_block);

    printf("Enter number of subnets: ");
    scanf("%d", &subnet_count);

    /*
     * Example:
     * SETUP 192.168.1.0/24 4
     */

    sprintf(message,
            "SETUP %s %d",
            ip_block,
            subnet_count);

    if (sendto(sockfd,
               message,
               strlen(message),
               0,
               (struct sockaddr *)&server_addr,
               addr_len) < 0)
    {
        perror("sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* Receive subnet information */

    ssize_t bytes_received =
        recvfrom(sockfd,
                 response,
                 BUFFER_SIZE - 1,
                 0,
                 NULL,
                 NULL);

    if (bytes_received < 0)
    {
        perror("recvfrom failed");

        close(sockfd);

        exit(EXIT_FAILURE);
    }

    response[bytes_received] = '\0';

    printf("\n%s\n", response);

    /* ---------------- IP ALLOCATION ---------------- */

    while (1)
    {
        int subnet_number;
        int required_ips;

        printf("Enter subnet number (0 to exit): ");
        scanf("%d", &subnet_number);

        /* EXIT */

        if (subnet_number == 0)
        {
            strcpy(message, "EXIT");

            sendto(sockfd,
                   message,
                   strlen(message),
                   0,
                   (struct sockaddr *)&server_addr,
                   addr_len);

            bytes_received =
                recvfrom(sockfd,
                         response,
                         BUFFER_SIZE - 1,
                         0,
                         NULL,
                         NULL);

            if (bytes_received >= 0)
            {
                response[bytes_received] = '\0';

                printf("\n%s\n",
                       response);
            }

            break;
        }

        printf("Enter number of IP addresses to allot: ");
        scanf("%d", &required_ips);

        /*
         * Example:
         * ALLOCATE 3 1
         */

        sprintf(message,
                "ALLOCATE %d %d",
                subnet_number,
                required_ips);

        if (sendto(sockfd,
                   message,
                   strlen(message),
                   0,
                   (struct sockaddr *)&server_addr,
                   addr_len) < 0)
        {
            perror("sendto failed");
            break;
        }

        /* Receive allocation result */

        bytes_received =
            recvfrom(sockfd,
                     response,
                     BUFFER_SIZE - 1,
                     0,
                     NULL,
                     NULL);

        if (bytes_received < 0)
        {
            perror("recvfrom failed");
            break;
        }

        response[bytes_received] = '\0';

        printf("\nDHCP Server Response:\n");

        printf("%s\n\n",
               response);
    }

    close(sockfd);

    return 0;
}