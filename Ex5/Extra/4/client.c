#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000

int main()
{
    int sockfd;
    struct sockaddr_in serverAddr;

    char regno[20];
    char response[500];

    socklen_t addrLen;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        printf("Socket creation failed.\n");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    inet_pton(AF_INET,
              "127.0.0.1",
              &serverAddr.sin_addr);

    while (1)
    {
        printf("\nEnter Registration Number");
        printf(" (or 0 to exit): ");

        scanf("%19s", regno);

        if (strcmp(regno, "0") == 0)
        {
            break;
        }

        /* Basic input validation */

        if (strlen(regno) < 5)
        {
            printf("Invalid registration number.\n");
            continue;
        }

        sendto(sockfd,
               regno,
               strlen(regno) + 1,
               0,
               (struct sockaddr *)&serverAddr,
               sizeof(serverAddr));

        addrLen = sizeof(serverAddr);

        int n = recvfrom(sockfd,
                         response,
                         sizeof(response) - 1,
                         0,
                         (struct sockaddr *)&serverAddr,
                         &addrLen);

        if (n < 0)
        {
            printf("Error receiving response.\n");
            continue;
        }

        response[n] = '\0';

        printf("\n----- Student Details -----\n");
        printf("%s\n", response);
        printf("---------------------------\n");
    }

    close(sockfd);

    return 0;
}
