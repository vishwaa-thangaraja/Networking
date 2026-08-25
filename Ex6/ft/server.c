#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int main()
{
    int server_fd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char filename[100];
    char buffer[BUFFER_SIZE];
    FILE *fp;
    int n, opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // FIX: Prevents "Address already in use" bind errors
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
    printf("Server is listening on port %d...\n", PORT);
    while (1)
    {
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_sock < 0)
        {
            perror("Accept failed");
            continue;
        }
        printf("\nClient connected.\n");
        n = recv(new_sock, filename, sizeof(filename) - 1, 0);
        if (n <= 0)
        {
            close(new_sock);
            continue;
        }
        filename[n] = '\0';
        printf("Requested file: %s\n", filename);
        fp = fopen(filename, "rb");

        if (fp == NULL)
        {
            printf("File not found.\n");
            send(new_sock, "NO", 2, 0);
            close(new_sock);
            continue;
        }

        send(new_sock, "OK", 2, 0);
        while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
        {
            send(new_sock, buffer, n, 0);
        }

        printf("File sent successfully.\n");
        fclose(fp);
        close(new_sock);
        printf("Client disconnected.\n");
    }

    close(server_fd);
    return 0;
}
