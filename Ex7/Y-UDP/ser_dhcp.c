#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096
#define MAX_SUBNETS 256
#define MAX_HOSTS 65534

typedef struct
{
    uint32_t network;
    uint32_t first_host;
    uint32_t last_host;
    uint32_t broadcast;
    int total_hosts;
    int allocated;
    int used[MAX_HOSTS];
} Subnet;

/* Convert IP string to integer */
uint32_t ip_to_int(char *ip)
{
    unsigned int a, b, c, d;

    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);

    return (a << 24) | (b << 16) | (c << 8) | d;
}

/* Convert integer to IP string */
void int_to_ip(uint32_t ip, char *str)
{
    sprintf(str, "%u.%u.%u.%u",
            (ip >> 24) & 255,
            (ip >> 16) & 255,
            (ip >> 8) & 255,
            ip & 255);
}

/* Calculate 2^x */
int power_of_two(int x)
{
    int result = 1;

    for (int i = 0; i < x; i++)
    {
        result = result * 2;
    }

    return result;
}

/* Find number of bits to borrow */
int find_bits(int subnet_count)
{
    int x = 0;

    while (power_of_two(x) < subnet_count)
    {
        x++;
    }

    return x;
}

int main(int argc, char *argv[])
{
    int sockfd;
    int port;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    /*
     * static is important here.
     * It prevents the huge array from being created on the stack.
     */
    static Subnet subnets[MAX_SUBNETS];

    int subnet_count = 0;
    int prefix = 0;
    int new_prefix = 0;
    int host_bits = 0;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);

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

    /* Server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Bind socket */
    if (bind(sockfd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("DHCP UDP Server started on port %d\n", port);
    printf("Server is ready for clients.\n\n");

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);

        /* Receive message */
        ssize_t bytes_received = recvfrom(
            sockfd,
            buffer,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr *)&client_addr,
            &addr_len);

        if (bytes_received < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        buffer[bytes_received] = '\0';

        /* ---------------- SETUP ---------------- */

        if (strncmp(buffer, "SETUP", 5) == 0)
        {
            char ip[50];
            int required_subnets;

            /*
             * Example:
             * SETUP 192.168.1.0/24 4
             */

            if (sscanf(buffer,
                       "SETUP %49[^/]/%d %d",
                       ip,
                       &prefix,
                       &required_subnets) != 3)
            {
                strcpy(response, "Invalid SETUP format.");

                sendto(sockfd,
                       response,
                       strlen(response),
                       0,
                       (struct sockaddr *)&client_addr,
                       addr_len);

                continue;
            }

            /* Validate subnet count */
            if (required_subnets <= 0 ||
                required_subnets > MAX_SUBNETS)
            {
                strcpy(response, "Invalid number of subnets.");

                sendto(sockfd,
                       response,
                       strlen(response),
                       0,
                       (struct sockaddr *)&client_addr,
                       addr_len);

                continue;
            }

            subnet_count = required_subnets;

            /* Find borrowed bits */
            int borrowed_bits = find_bits(subnet_count);

            new_prefix = prefix + borrowed_bits;

            /* Validate prefix */
            if (prefix < 0 || prefix > 30 || new_prefix > 30)
            {
                strcpy(response, "Invalid IP prefix.");

                sendto(sockfd,
                       response,
                       strlen(response),
                       0,
                       (struct sockaddr *)&client_addr,
                       addr_len);

                continue;
            }

            /* Host bits */
            host_bits = 32 - new_prefix;

            /* Addresses in each subnet */
            uint32_t addresses_per_subnet =
                power_of_two(host_bits);

            /* Convert IP */
            uint32_t base_ip = ip_to_int(ip);

            printf("Client [%s:%d]\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            printf("IP Block: %s/%d\n",
                   ip,
                   prefix);

            printf("Required Subnets: %d\n",
                   subnet_count);

            printf("Borrowed Bits: %d\n",
                   borrowed_bits);

            printf("New Prefix: /%d\n\n",
                   new_prefix);

            /* Create subnets */
            for (int i = 0; i < subnet_count; i++)
            {
                subnets[i].network =
                    base_ip +
                    (i * addresses_per_subnet);

                subnets[i].broadcast =
                    subnets[i].network +
                    addresses_per_subnet - 1;

                subnets[i].first_host =
                    subnets[i].network + 1;

                subnets[i].last_host =
                    subnets[i].broadcast - 1;

                subnets[i].total_hosts =
                    addresses_per_subnet - 2;

                subnets[i].allocated = 0;

                for (int j = 0;
                     j < subnets[i].total_hosts;
                     j++)
                {
                    subnets[i].used[j] = 0;
                }
            }

            /* Prepare response */
            strcpy(response, "Subnet calculation:\n\n");

            for (int i = 0; i < subnet_count; i++)
            {
                char network[30];
                char first[30];
                char last[30];
                char broadcast[30];

                int_to_ip(subnets[i].network, network);
                int_to_ip(subnets[i].first_host, first);
                int_to_ip(subnets[i].last_host, last);
                int_to_ip(subnets[i].broadcast, broadcast);

                char temp[300];

                sprintf(temp,
                        "Subnet %d: %s/%d\n"
                        "Network: %s\n"
                        "First Host: %s\n"
                        "Last Host: %s\n"
                        "Broadcast: %s\n"
                        "Usable Hosts: %d\n\n",
                        i + 1,
                        network,
                        new_prefix,
                        network,
                        first,
                        last,
                        broadcast,
                        subnets[i].total_hosts);

                /*
                 * Prevent response buffer overflow.
                 */
                if (strlen(response) + strlen(temp) <
                    BUFFER_SIZE)
                {
                    strcat(response, temp);
                }
                else
                {
                    strcat(response,
                           "\nResponse too large to display completely.\n");
                    break;
                }
            }

            printf("%s", response);

            /* Send subnet information */
            sendto(sockfd,
                   response,
                   strlen(response),
                   0,
                   (struct sockaddr *)&client_addr,
                   addr_len);
        }

        /* ---------------- ALLOCATION ---------------- */

        else if (strncmp(buffer, "ALLOCATE", 8) == 0)
        {
            int subnet_number;
            int required_ips;

            /*
             * Example:
             * ALLOCATE 3 2
             */

            if (sscanf(buffer,
                       "ALLOCATE %d %d",
                       &subnet_number,
                       &required_ips) != 2)
            {
                strcpy(response,
                       "Invalid ALLOCATE format.");

                sendto(sockfd,
                       response,
                       strlen(response),
                       0,
                       (struct sockaddr *)&client_addr,
                       addr_len);

                continue;
            }

            printf("Allocation request:\n");
            printf("Subnet: %d\n", subnet_number);
            printf("Required IPs: %d\n",
                   required_ips);

            /* Validate subnet */
            if (subnet_number < 1 ||
                subnet_number > subnet_count)
            {
                strcpy(response,
                       "Invalid subnet number.");
            }

            /* Validate requested IP count */
            else if (required_ips <= 0)
            {
                strcpy(response,
                       "Invalid number of IP addresses.");
            }

            /* Check available IPs */
            else if (required_ips >
                     subnets[subnet_number - 1].total_hosts -
                     subnets[subnet_number - 1].allocated)
            {
                strcpy(response,
                       "Subnet is full or not enough IP addresses are available.");
            }

            else
            {
                Subnet *s =
                    &subnets[subnet_number - 1];

                strcpy(response,
                       "IP addresses allotted successfully:\n");

                int allocated_now = 0;

                for (int i = 0;
                     i < s->total_hosts &&
                     allocated_now < required_ips;
                     i++)
                {
                    if (s->used[i] == 0)
                    {
                        uint32_t ip =
                            s->first_host + i;

                        char ip_string[30];

                        int_to_ip(ip, ip_string);

                        s->used[i] = 1;

                        s->allocated++;

                        char temp[50];

                        sprintf(temp,
                                "%s\n",
                                ip_string);

                        strcat(response, temp);

                        allocated_now++;
                    }
                }
            }

            printf("%s\n\n", response);

            /* Send allocation result */
            sendto(sockfd,
                   response,
                   strlen(response),
                   0,
                   (struct sockaddr *)&client_addr,
                   addr_len);
        }

        /* ---------------- EXIT ---------------- */

        else if (strcmp(buffer, "EXIT") == 0)
        {
            strcpy(response,
                   "DHCP client disconnected.");

            sendto(sockfd,
                   response,
                   strlen(response),
                   0,
                   (struct sockaddr *)&client_addr,
                   addr_len);

            printf("Client disconnected.\n\n");
        }

        /* ---------------- INVALID REQUEST ---------------- */

        else
        {
            strcpy(response,
                   "Invalid request.");

            sendto(sockfd,
                   response,
                   strlen(response),
                   0,
                   (struct sockaddr *)&client_addr,
                   addr_len);
        }
    }

    close(sockfd);

    return 0;
}