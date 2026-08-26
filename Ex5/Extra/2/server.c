#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 5000
#define MAX_LOCATIONS 100
struct Weather
{
    char location[50];
    int last_seq;
    int count;
    float total_temp;
    float total_humidity;
    float total_pressure;
    float total_wind;
};
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
    struct sockaddr_in clientAddr;
    socklen_t addrLen;
    struct Packet packet;
    struct Ack ack;
    struct Weather locations[MAX_LOCATIONS];
    int location_count = 0;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed.\n");
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (bind(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)) < 0)
    {
        printf("Bind failed.\n");
        return 1;
    }
    printf("Weather Server started.\n");
    printf("Waiting for weather data...\n\n");
    while (1)
    {
        addrLen = sizeof(clientAddr);
        recvfrom(sockfd,
                 &packet,
                 sizeof(packet),
                 0,
                 (struct sockaddr *)&clientAddr,
                 &addrLen);
        if (strcmp(packet.type, "QUERY") == 0)
        {
            printf("\nWeather statistics requested.\n");
            printf("\n----- Weather Statistics -----\n");
            if (location_count == 0)
            {
                printf("No weather data available.\n");
            }
            for (int i = 0; i < location_count; i++)
            {
                printf("\nLocation: %s\n",
                       locations[i].location);
                printf("Average Temperature : %.2f\n",
                       locations[i].total_temp /
                       locations[i].count);
                printf("Average Humidity    : %.2f\n",
                       locations[i].total_humidity /
                       locations[i].count);
                printf("Average Pressure    : %.2f\n",
                       locations[i].total_pressure /
                       locations[i].count);
                printf("Average Wind Speed  : %.2f\n",
                       locations[i].total_wind /
                       locations[i].count);
            }
            printf("-------------------------------\n");
            strcpy(ack.type, "STATS");
            ack.seq = 0;
            sendto(sockfd,
                   &ack,
                   sizeof(ack),
                   0,
                   (struct sockaddr *)&clientAddr,
                   addrLen);

            continue;
        }
        if (packet.checksum != calculate_checksum(packet))
        {
            printf("ERROR: Corrupted packet received.\n");
            strcpy(ack.type, "ERROR");
            ack.seq = packet.seq;
            sendto(sockfd,
                   &ack,
                   sizeof(ack),
                   0,
                   (struct sockaddr *)&clientAddr,
                   addrLen);

            continue;
        }
        printf("Received packet:\n");
        printf("Location   : %s\n", packet.location);
        printf("Sequence   : %d\n", packet.seq);
        printf("Temperature: %.2f\n", packet.temperature);
        printf("Humidity   : %.2f\n", packet.humidity);
        printf("Pressure   : %.2f\n", packet.pressure);
        printf("Wind Speed : %.2f\n", packet.wind_speed);
        int index = -1;
        for (int i = 0; i < location_count; i++)
        {
            if (strcmp(locations[i].location,packet.location) == 0)
            {
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            index = location_count;
            strcpy(locations[index].location,
                   packet.location);
            locations[index].last_seq = 0;
            locations[index].count = 0;
            locations[index].total_temp = 0;
            locations[index].total_humidity = 0;
            locations[index].total_pressure = 0;
            locations[index].total_wind = 0;
            location_count++;
            printf("New location registered.\n");
        }

        /* Detect missing packet */

        if (packet.seq > locations[index].last_seq + 1)
        {
            printf("WARNING: Missing packet(s) detected.\n");
            printf("Expected sequence: %d\n",
                   locations[index].last_seq + 1);

            printf("Received sequence: %d\n",
                   packet.seq);
        }

        /* Detect duplicate */

        if (packet.seq <= locations[index].last_seq)
        {
            printf("DUPLICATE packet detected: %d\n",
                   packet.seq);

            /* Still send ACK so client stops retransmitting */

            strcpy(ack.type, "ACK");
            ack.seq = packet.seq;

            sendto(sockfd,
                   &ack,
                   sizeof(ack),
                   0,
                   (struct sockaddr *)&clientAddr,
                   addrLen);

            continue;
        }

        /* Store weather data */

        locations[index].last_seq = packet.seq;

        locations[index].count++;

        locations[index].total_temp +=
            packet.temperature;

        locations[index].total_humidity +=
            packet.humidity;

        locations[index].total_pressure +=
            packet.pressure;

        locations[index].total_wind +=
            packet.wind_speed;

        /* Send ACK */

        strcpy(ack.type, "ACK");

        ack.seq = packet.seq;

        sendto(sockfd,
               &ack,
               sizeof(ack),
               0,
               (struct sockaddr *)&clientAddr,
               addrLen);

        printf("ACK sent for sequence %d.\n\n",
               packet.seq);
    }

    close(sockfd);

    return 0;
}
