#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<errno.h>
#include<limits.h>

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
    server->sin_port=htons(PORT);
    if(inet_pton(AF_INET,"127.0.0.1",&server->sin_addr)<=0)
    {
        perror("inet_pton");
        exit(1);
    }
}

void marshal(struct Request request,char *buffer)
{
    memcpy(buffer,&request,sizeof(request));
}

void unmarshal(char *buffer,int *result)
{
    memcpy(result,buffer,sizeof(*result));
}

int valid_integer(char *str,int *value)
{
    char *end;
    long number;

    errno=0;
    number=strtol(str,&end,10);

    if(str==end || *end!='\0' || errno==ERANGE ||
       number<INT_MIN || number>INT_MAX)
        return 0;

    *value=(int)number;
    return 1;
}

int parse_input(char *input,struct Request *request)
{
    char *token;
    char *copy;
    char *saveptr;
    int count=0;

    copy=malloc(strlen(input)+1);
    if(copy==NULL)
        return 0;

    strcpy(copy,input);

    token=strtok_r(copy," \t",&saveptr);

    if(token==NULL)
    {
        free(copy);
        return 0;
    }

    if(strcmp(token,"add")!=0 &&
       strcmp(token,"mul")!=0 &&
       strcmp(token,"sub")!=0 &&
       strcmp(token,"div")!=0)
    {
        free(copy);
        return 0;
    }

    strcpy(request->option,token);

    while((token=strtok_r(NULL," \t",&saveptr))!=NULL)
    {
        if(count>=MAX_NUMBERS)
        {
            free(copy);
            return 0;
        }

        if(!valid_integer(token,&request->numbers[count]))
        {
            free(copy);
            return 0;
        }

        count++;
    }

    free(copy);

    if(strcmp(request->option,"add")==0 ||
       strcmp(request->option,"mul")==0)
    {
        if(count<2)
            return 0;
    }
    else
    {
        if(count!=2)
            return 0;
    }

    request->count=count;

    if(strcmp(request->option,"div")==0 && request->numbers[1]==0)
        return 0;

    return 1;
}

int remote_call(int sock,struct sockaddr_in *server,struct Request *request)
{
    char buffer[sizeof(struct Request)];
    char result_buffer[sizeof(int)];
    int result;

    marshal(*request,buffer);

    if(sendto(sock,buffer,sizeof(buffer),0,
              (struct sockaddr *)server,sizeof(*server))<0)
    {
        perror("sendto");
        exit(1);
    }

    if(recvfrom(sock,result_buffer,sizeof(result_buffer),0,NULL,NULL)<0)
    {
        perror("recvfrom");
        exit(1);
    }

    unmarshal(result_buffer,&result);

    return result;
}

void start_client()
{
    int sock;
    struct sockaddr_in server;
    struct Request request;
    char input[1024];
    int result;
    int i;

    sock=create_socket();
    create_address(&server);

    printf("=====================================\n");
    printf("          RPC CALCULATOR CLIENT\n");
    printf("=====================================\n");
    printf("Examples:\n");
    printf("add 2 or more numbers\n");
    printf("mul 2 or more numbers\n");
    printf("sub only 2 numbers\n");
    printf("div only 2 numbers\n");
    printf("Type exit to close.\n");
    printf("-------------------------------------\n");

    while(1)
    {
        printf("Enter command: ");

        if(fgets(input,sizeof(input),stdin)==NULL)
            break;

        input[strcspn(input,"\n")]='\0';

        if(strcmp(input,"exit")==0)
        {
            char buffer[sizeof(struct Request)];

            memset(&request,0,sizeof(request));
            strcpy(request.option,"exit");

            marshal(request,buffer);

            sendto(sock,buffer,sizeof(buffer),0,
                   (struct sockaddr *)&server,sizeof(server));

            printf("[CLIENT] EXIT request sent.\n");
            break;
        }

        if(!parse_input(input,&request))
        {
            if(strncmp(input,"div",3)==0 &&
               request.count>=2 &&
               request.numbers[1]==0)
                printf("[CLIENT] Error: Division by zero is not allowed.\n");
            else
                printf("[CLIENT] Invalid input.\n");

            continue;
        }

        result=remote_call(sock,&server,&request);

        if(strcmp(request.option,"add")==0)
        {
            printf("[CLIENT] Result: ");

            for(i=0;i<request.count;i++)
            {
                printf("%d",request.numbers[i]);

                if(i<request.count-1)
                    printf(" + ");
            }

            printf(" = %d\n",result);
        }
        else if(strcmp(request.option,"mul")==0)
        {
            printf("[CLIENT] Result: ");

            for(i=0;i<request.count;i++)
            {
                printf("%d",request.numbers[i]);

                if(i<request.count-1)
                    printf(" * ");
            }

            printf(" = %d\n",result);
        }
        else if(strcmp(request.option,"sub")==0)
        {
            printf("[CLIENT] Result: %d - %d = %d\n",
                   request.numbers[0],
                   request.numbers[1],
                   result);
        }
        else if(strcmp(request.option,"div")==0)
        {
            printf("[CLIENT] Result: %d / %d = %d\n",
                   request.numbers[0],
                   request.numbers[1],
                   result);
        }
    }

    close(sock);
    printf("[CLIENT] Client closed.\n");
}

int main()
{
    start_client();
    return 0;
}
