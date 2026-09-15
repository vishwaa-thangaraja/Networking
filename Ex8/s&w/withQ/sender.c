#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#define SERVER_IP "127.0.0.1"
#define PORT 8200
#define MSG_FRAME 1
#define MSG_EXIT 2
typedef struct
{
    char sourceIP[16], destinationIP[16], data[17];
    int seqNo, parityBit;
} Frame;
typedef struct
{
    int type, mode, totalFrames, messageLength;
    Frame frame;
} Message;
typedef struct
{
    int ackNo;
} Ack;
typedef struct
{
    Frame frames[50];
    int front;
    int rear;
} Queue;
void enqueue(Queue *q,Frame frame)
{
    q->frames[q->rear]=frame;
    q->rear++;
}
Frame dequeue(Queue *q)
{
    Frame frame=q->frames[q->front];
    q->front++;
    return frame;
}
void binaryValue(int value,char binary[])
{
    int i;
    for(i=7;i>=0;i--)binary[7-i]=(value>>i)&1?'1':'0';
    binary[8]='\0';
}
void printIPBinary(char ip[])
{
    int a,b,c,d;
    char binary[9];
    sscanf(ip,"%d.%d.%d.%d",&a,&b,&c,&d);
    printf("%d.%d.%d.%d : ",a,b,c,d);
    binaryValue(a,binary);printf("%s ",binary);
    binaryValue(b,binary);printf("%s ",binary);
    binaryValue(c,binary);printf("%s ",binary);
    binaryValue(d,binary);printf("%s\n",binary);
}
int main()
{
    int sockfd,choice,i,j,n,Sn=0,length,totalFrames,first;
    char input[100],binary[801];
    Queue sentQueue;
    struct sockaddr_in server_addr;
    sentQueue.front=0;
    sentQueue.rear=0;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        perror("Socket creation failed");
        return 1;
    }
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    if(inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr)<=0)
    {
        perror("Invalid address");
        close(sockfd);
        return 1;
    }
    if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0)
    {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }
    printf("=============================================\n");
    printf("       STOP-AND-WAIT ARQ - SENDER\n");
    printf("=============================================\n");
    printf("\nFRAME STRUCTURE\n");
    printf("---------------------------------------------\n");
    printf("| Source IP | Destination IP | Seq | 16-bit Data | Parity |\n");
    printf("---------------------------------------------\n");
    printf("\nSOURCE IP\n");printIPBinary("127.0.0.1");
    printf("DESTINATION IP\n");printIPBinary("127.0.0.1");
    while(1)
    {
        printf("\n=============================================\n");
        printf("                 MAIN MENU\n");
        printf("=============================================\n");
        printf("1. Normal propagation\n");
        printf("2. Time-out (ACK lost)\n");
        printf("3. Frame lost\n");
        printf("4. Error in frame\n");
        printf("5. Exit\n");
        printf("---------------------------------------------\n");
        printf("Enter choice: ");
        scanf("%d",&choice);
        getchar();
        if(choice==5)
        {
            Message msg;
            memset(&msg,0,sizeof(msg));
            msg.type=MSG_EXIT;
            send(sockfd,&msg,sizeof(msg),0);
            printf("\nEXIT sent.\n");
            break;
        }
        if(choice<1||choice>4)
        {
            printf("Invalid choice.\n");
            continue;
        }
        Sn=0;
        sentQueue.front=0;
        sentQueue.rear=0;
        printf("\nEnter data: ");
        fgets(input,sizeof(input),stdin);
        input[strcspn(input,"\n")]='\0';
        length=strlen(input);
        binary[0]='\0';
        for(i=0;i<length;i++)
        {
            int value=input[i];
            char temp[9];
            for(j=7;j>=0;j--)
            {
                if(value%2==1) temp[j]='1';
                else temp[j]='0';
                value=value/2;
            }
            temp[8]='\0';
            strcat(binary,temp);
        }
        totalFrames=(length+1)/2;
        printf("\nInput : %s\n",input);
        printf("Binary : %s\n",binary);
        printf("Frame size : 16 bits\n");
        printf("Total frames : %d\n",totalFrames);
        for(i=0;i<totalFrames;i++)
        {
            Message msg;Ack ack;
            int k,sum=0;
            struct timeval tv;
            memset(&msg,0,sizeof(msg));
            msg.type=MSG_FRAME;
            msg.mode=choice;
            msg.totalFrames=totalFrames;
            msg.messageLength=length;
            strcpy(msg.frame.sourceIP,"127.0.0.1");
            strcpy(msg.frame.destinationIP,"127.0.0.1");
            msg.frame.seqNo=Sn;
            for(j=0;j<16;j++)
            {
                k=i*16+j;
                if(k<length*8) msg.frame.data[j]=binary[k];
                else msg.frame.data[j]='0';
            }
            msg.frame.data[16]='\0';
            for(j=0;j<16;j++) if(msg.frame.data[j]=='1') sum++;
            msg.frame.parityBit=sum%2;
            first=(i==0);
            if(choice==1||!first)
            {
                printf("\n---------------------------------------------\n");
                printf("Frame %d SENT\n",Sn);
                printf("Data     : %s\n",msg.frame.data);
                printf("Parity Bit : %d\n",msg.frame.parityBit);
                printf("Timer STARTED.\n");
                printf("Waiting for ACK...\n");
                enqueue(&sentQueue,msg.frame);
                send(sockfd,&msg,sizeof(msg),0);
                n=recv(sockfd,&ack,sizeof(ack),0);
                if(n<=0)
                {
                    printf("Connection error.\n");
                    break;
                }
                printf("ACK %d RECEIVED\n",ack.ackNo);
                if(ack.ackNo==((Sn+1)%2))
                {
                    printf("Valid ACK.\n");
                    printf("Stop timer.\n");
                    dequeue(&sentQueue);
                    Sn=(Sn+1)%2;
                }
            }
            else if(choice==2)
            {
                printf("\n========== TIME-OUT / ACK LOST ==========\n");
                printf("Frame %d SENT\n",Sn);
                printf("Data     : %s\n",msg.frame.data);
                printf("Parity Bit : %d\n",msg.frame.parityBit);
                printf("Timer STARTED.\n");
                enqueue(&sentQueue,msg.frame);
                send(sockfd,&msg,sizeof(msg),0);
                tv.tv_sec=5;
                tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
                n=recv(sockfd,&ack,sizeof(ack),0);
                if(n<=0)
                {
                    printf("\nACK %d NOT RECEIVED.\n",(Sn+1)%2);
                    printf("TIME-OUT! No ACK received.\n");
                    printf("Retransmitting Frame %d...\n",Sn);
                    send(sockfd,&msg,sizeof(msg),0);
                    n=recv(sockfd,&ack,sizeof(ack),0);
                    if(n>0)
                    {
                        printf("ACK %d RECEIVED\n",ack.ackNo);
                        if(ack.ackNo==((Sn+1)%2))
                        {
                            printf("Valid ACK.\n");
                            printf("Stop timer.\n");
                            dequeue(&sentQueue);
                            Sn=(Sn+1)%2;
                        }
                    }
                }
                else
                {
                    printf("ACK %d RECEIVED\n",ack.ackNo);
                    if(ack.ackNo==((Sn+1)%2))
                    {
                        printf("Valid ACK.\nStop timer.\n");
                        dequeue(&sentQueue);
                        Sn=(Sn+1)%2;
                    }
                }
                tv.tv_sec=0;tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
            }
            else if(choice==3)
            {
                printf("\n========== FRAME LOST ==========\n");
                printf("Frame %d LOST IN NETWORK\n",Sn);
                printf("Timer STARTED.\nWaiting for ACK...\n");
                enqueue(&sentQueue,msg.frame);
                tv.tv_sec=5;tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
                n=recv(sockfd,&ack,sizeof(ack),0);
                if(n<=0)
                {
                    printf("\nTIME-OUT occurred.\n");
                    printf("Resending Frame %d...\n",Sn);
                    send(sockfd,&msg,sizeof(msg),0);
                    n=recv(sockfd,&ack,sizeof(ack),0);
                    if(n>0)
                    {
                        printf("ACK %d RECEIVED\n",ack.ackNo);
                        if(ack.ackNo==((Sn+1)%2))
                        {
                            printf("Valid ACK.\n");
                            dequeue(&sentQueue);
                            Sn=(Sn+1)%2;
                        }
                    }
                }
                tv.tv_sec=0;tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
            }
            else if(choice==4)
            {
                printf("\n========== PARITY BIT ERROR ==========\n");
                msg.frame.data[0]=(msg.frame.data[0]=='0')?'1':'0';
                printf("Frame %d SENT WITH ERROR\n",Sn);
                printf("Data     : %s\n",msg.frame.data);
                printf("Parity Bit : %d\n",msg.frame.parityBit);
                printf("Timer STARTED.\nWaiting for ACK...\n");
                enqueue(&sentQueue,msg.frame);
                send(sockfd,&msg,sizeof(msg),0);
                tv.tv_sec=5;tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
                n=recv(sockfd,&ack,sizeof(ack),0);
                if(n<=0)
                {
                    printf("\nTIME-OUT.\n");
                    printf("Retransmitting correct Frame %d...\n",Sn);
                    msg.frame.data[0]=(msg.frame.data[0]=='0')?'1':'0';
                    send(sockfd,&msg,sizeof(msg),0);
                    n=recv(sockfd,&ack,sizeof(ack),0);
                    if(n>0)
                    {
                        printf("ACK %d RECEIVED\n",ack.ackNo);
                        if(ack.ackNo==((Sn+1)%2))
                        {
                            printf("Valid ACK.\n");
                            dequeue(&sentQueue);
                            Sn=(Sn+1)%2;
                        }
                    }
                }
                tv.tv_sec=0;tv.tv_usec=0;
                setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
            }
        }
        printf("\nAll frames processed.\n");
    }
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}