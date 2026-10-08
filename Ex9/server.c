#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>

#define PORT 8089
#define MAX_NUMBERS 100

struct Request
{
    char option[10];
    int count;
    int numbers[MAX_NUMBERS];
};

int create_socket()
{
    int sock;
    sock=socket(AF_INET,SOCK_DGRAM,0);
    if(sock<0)
    {
        perror("socket");
        exit(1);
    }
    return sock;
}

void create_address(struct sockaddr_in *server)
{
    memset(server,0,sizeof(*server));
    server->sin_family=AF_INET;
    server->sin_addr.s_addr=INADDR_ANY;
    server->sin_port=htons(PORT);
}

void bind_socket(int sock,struct sockaddr_in *server)
{
    if(bind(sock,(struct sockaddr *)server,sizeof(*server))<0)
    {
        perror("bind");
        close(sock);
        exit(1);
    }
}

void unmarshal(char *buffer,struct Request *request)
{
    memcpy(request,buffer,sizeof(*request));
}

void marshal(int result,char *buffer)
{
    memcpy(buffer,&result,sizeof(result));
}

int add(struct Request request)
{
    int result=0;
    int i;

    for(i=0;i<request.count;i++)
        result+=request.numbers[i];

    return result;
}

int sub(struct Request request)
{
    return request.numbers[0]-request.numbers[1];
}

int mul(struct Request request)
{
    int result=1;
    int i;

    for(i=0;i<request.count;i++)
        result*=request.numbers[i];

    return result;
}

int divi(struct Request request)
{
    return request.numbers[0]/request.numbers[1];
}

int server_call(struct Request request)
{
    if(strcmp(request.option,"add")==0)
        return add(request);

    if(strcmp(request.option,"sub")==0)
        return sub(request);

    if(strcmp(request.option,"mul")==0)
        return mul(request);

    if(strcmp(request.option,"div")==0)
        return divi(request);

    return 0;
}

void send_result(int sock,struct sockaddr_in *client,
                 socklen_t length,struct Request request)
{
    int result;
    char buffer[sizeof(int)];
    int i;

    result=server_call(request);

    printf("[SERVER] RPC procedure executed.\n");
    printf("[SERVER] %s(",request.option);

    for(i=0;i<request.count;i++)
    {
        printf("%d",request.numbers[i]);

        if(i<request.count-1)
            printf(",");
    }

    printf(") = %d\n",result);

    marshal(result,buffer);

    if(sendto(sock,buffer,sizeof(buffer),0,
              (struct sockaddr *)client,length)<0)
    {
        perror("sendto");
        return;
    }

    printf("[SERVER] Result sent to client.\n");
}

void start_server()
{
    int sock;
    struct sockaddr_in server,client;
    struct Request request;
    char buffer[sizeof(struct Request)];
    socklen_t length;

    sock=create_socket();
    create_address(&server);
    bind_socket(sock,&server);

    printf("=====================================\n");
    printf("          RPC CALCULATOR SERVER\n");
    printf("=====================================\n");
    printf("[SERVER] Server started.\n");
    printf("[SERVER] Listening on port %d...\n",PORT);
    printf("-------------------------------------\n");

    while(1)
    {
        length=sizeof(client);

        if(recvfrom(sock,buffer,sizeof(buffer),0,
                    (struct sockaddr *)&client,&length)<0)
        {
            perror("recvfrom");
            continue;
        }

        unmarshal(buffer,&request);

        if(strcmp(request.option,"exit")==0)
        {
            printf("\n[SERVER] EXIT request received.\n");
            printf("[SERVER] Server shutting down.\n");
            close(sock);
            exit(0);
        }

        printf("\n[SERVER] RPC request received.\n");

        if(fork()==0)
        {
            send_result(sock,&client,length,request);
            close(sock);
            exit(0);
        }
    }
}

int main()
{
    start_server();
    return 0;
}
