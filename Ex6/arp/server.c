#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#define PORT 5500
#define SIZE 20
struct ARP
{
    char ip[40];  
    char mac[20]; 
};
struct ARP table[SIZE];
int hash(char ip[])
{
    int sum = 0, i;
    for (i = 0; ip[i] != '\0'; i++) sum += ip[i];
    return sum % SIZE;
}
int main()
{
    int server_fd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char ip_req[40], mac_res[100], out_buf[2048]; 
    FILE *fp;
    char line[200], ip[40], mac[20]; 
    int idx, n, i, opt = 1;
    srand(time(NULL));
    fp = popen("ip neigh", "r");
    if (fp == NULL)
    {
        printf("Could not read ARP table.\n");
        return 1;
    }
    while (fgets(line, sizeof(line), fp))
    {
        sscanf(line, "%s", ip);
        if (strstr(line, "lladdr") != NULL)
        {
            sscanf(strstr(line, "lladdr") + 7, "%s", mac);
            idx = hash(ip);
            strcpy(table[idx].ip, ip);
            strcpy(table[idx].mac, mac);
        }
    }
    pclose(fp);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server_fd);
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }
    printf("ARP Server running on port %d...\n", PORT);
    while (1)
    {
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_sock < 0) continue;
        n = recv(new_sock, ip_req, sizeof(ip_req) - 1, 0);
        if (n <= 0)
        {
            close(new_sock);
            continue;
        }
        ip_req[n] = '\0';
        if (strcmp(ip_req, "DISPLAY_ALL") == 0)
        {
            printf("Client requested full ARP table data dump.\n");
            sprintf(out_buf, "\n-------------------------------\nIP Address\tMAC Address\n-------------------------------\n");
            for (i = 0; i < SIZE; i++)
                if (strlen(table[i].ip) > 0)
                {
                    char row[100];
                    sprintf(row, "%s\t%s\n", table[i].ip, table[i].mac);
                    strcat(out_buf, row);
                }
            strcat(out_buf, "-------------------------------\n");
            send(new_sock, out_buf, strlen(out_buf), 0);
        }
        else 
        {
            printf("Requested IP: %s\n", ip_req);
            idx = hash(ip_req);
            if (strcmp(table[idx].ip, ip_req) == 0)
                send(new_sock, table[idx].mac, strlen(table[idx].mac), 0);
            else
            {
                sprintf(mac, "AA:BB:CC:DD:EE:%02X", rand() % 256);
                strcpy(table[idx].ip, ip_req);
                strcpy(table[idx].mac, mac);
                sprintf(mac_res, "NOT FOUND. Assigned Mac -> %s", mac);
                send(new_sock, mac_res, strlen(mac_res), 0);
            }
        }
        close(new_sock);
    }
    close(server_fd);
    return 0;
}
