#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 5000
#define MAX_RETRIES 3

struct Packet
{
    char type[10];

    char location[50];

    int seq;

    float temperature;
    float humidity;
    float pressure;
    float wind_speed;

    int checksum;
};

struct Ack
{
    char type[10];
    int seq;
};

int calculate_checksum(struct Packet p)
{
    int sum = 0;

    for (int i = 0; p.location[i] != '\0'; i++)
        sum += p.location[i];

    sum += p.seq;
    sum += (int)p.temperature;
    sum += (int)p.humidity;
    sum += (int)p.pressure;
    sum += (int)p.wind_speed;

    return sum;
}

int main()
{
    int sockfd;

    struct sockaddr_in serverAddr;

    struct Packet packet;
    struct Ack ack;

    socklen_t addrLen;

    char location[50];

    int choice;
    int seq = 1;

    float temperature;
    float humidity;
    float pressure;
    float wind_speed;

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

    printf("Enter location: ");
    scanf("%s", location);

    /* Timeout = 2 seconds */

    struct timeval timeout;

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    setsockopt(sockfd,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &timeout,
               sizeof(timeout));

    while (1)
    {
        printf("\n1. Send Weather Data\n");
        printf("2. Request Weather Statistics\n");
        printf("3. Exit\n");

        printf("Enter choice: ");
        scanf("%d", &choice);

        /* Send weather data */

        if (choice == 1)
        {
            printf("\nEnter Temperature: ");
            scanf("%f", &temperature);

            printf("Enter Humidity: ");
            scanf("%f", &humidity);

            printf("Enter Pressure: ");
            scanf("%f", &pressure);

            printf("Enter Wind Speed: ");
            scanf("%f", &wind_speed);

            memset(&packet, 0, sizeof(packet));

            strcpy(packet.type, "DATA");

            strcpy(packet.location, location);

            packet.seq = seq;

            packet.temperature = temperature;
            packet.humidity = humidity;
            packet.pressure = pressure;
            packet.wind_speed = wind_speed;

            packet.checksum =
                calculate_checksum(packet);

            int success = 0;

            /* Retransmission */

            for (int attempt = 1;
                 attempt <= MAX_RETRIES;
                 attempt++)
            {
                printf("\nSending packet %d (Attempt %d)...\n",
                       packet.seq,
                       attempt);

                sendto(sockfd,
                       &packet,
                       sizeof(packet),
                       0,
                       (struct sockaddr *)&serverAddr,
                       sizeof(serverAddr));

                addrLen = sizeof(serverAddr);

                int n = recvfrom(sockfd,
                                 &ack,
                                 sizeof(ack),
                                 0,
                                 (struct sockaddr *)&serverAddr,
                                 &addrLen);

                if (n > 0)
                {
                    if (strcmp(ack.type, "ACK") == 0 &&
                        ack.seq == packet.seq)
                    {
                        printf("ACK received for packet %d.\n",
                               packet.seq);

                        success = 1;
                        break;
                    }
                    else if (strcmp(ack.type, "ERROR") == 0)
                    {
                        printf("Server detected packet error.\n");
                    }
                }
                else
                {
                    printf("Timeout! No ACK received.\n");
                }
            }

            if (success)
            {
                seq++;
            }
            else
            {
                printf("Packet %d failed after %d attempts.\n",
                       packet.seq,
                       MAX_RETRIES);

                seq++;
            }
        }

        /* Request statistics */

        else if (choice == 2)
        {
            memset(&packet, 0, sizeof(packet));

            strcpy(packet.type, "QUERY");

            sendto(sockfd,
                   &packet,
                   sizeof(packet),
                   0,
                   (struct sockaddr *)&serverAddr,
                   sizeof(serverAddr));

            addrLen = sizeof(serverAddr);

            int n = recvfrom(sockfd,
                             &ack,
                             sizeof(ack),
                             0,
                             (struct sockaddr *)&serverAddr,
                             &addrLen);

            if (n > 0)
            {
                if (strcmp(ack.type, "STATS") == 0)
                {
                    printf("\nStatistics request received by server.\n");
                    printf("Check the server terminal for averages.\n");
                }
            }
            else
            {
                printf("Server did not respond.\n");
            }
        }

        /* Exit */

        else if (choice == 3)
        {
            break;
        }

        else
        {
            printf("Invalid choice.\n");
        }
    }

    close(sockfd);

    return 0;
}
