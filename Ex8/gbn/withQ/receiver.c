#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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
    int front, rear;
} Queue;
void enqueue(Queue *q,Frame frame)
{
    q->frames[q->rear++]=frame;
}
Frame dequeue(Queue *q)
{
    return q->frames[q->front++];
}
void binaryValue(int value,char binary[])
{
    int i;
    for(i=7;i>=0;i--) binary[7-i]=(value>>i)&1?'1':'0';
    binary[8]='\0';
}
void printIPBinary(char ip[])
{
    int a,b,c,d;
    char binary[9];
    sscanf(ip,"%d.%d.%d.%d",&a,&b,&c,&d);
    printf("%d.%d.%d.%d : ",a,b,c,d);
    binaryValue(a,binary);
    printf("%s ",binary);
    binaryValue(b,binary);
    printf("%s ",binary);
    binaryValue(c,binary);
    printf("%s ",binary);
    binaryValue(d,binary);
    printf("%s\n",binary);
}
int checkParity(char data[],int parityBit)
{
    int i,ones=0;
    for(i=0;i<16;i++) if(data[i]=='1') ones++;
    return (ones%2)==parityBit;
}
void binaryToText(char data[],int length)
{
    int i,j,value;
    char ch;
    printf("Received Message: ");
    for(i=0;i<length;i++)
    {
        value=0;
        for(j=0;j<8;j++) value=value*2+(data[i*8+j]-'0');
        ch=(char)value;
        printf("%c",ch);
    }
    printf("\n");
}
int main()
{
    int sockfd,newfd,expected,receivedFrames;
    int i,totalFrames,messageLength,mode;
    int mode2Lost,mode2Skip;
    char receivedData[1600];
    Message msg;
    Ack ack;
    Queue receivedQueue;
    Frame frame;
    struct sockaddr_in server,client;
    socklen_t clientLength;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    server.sin_family=AF_INET;
    server.sin_port=htons(PORT);
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    bind(sockfd,(struct sockaddr*)&server,sizeof(server));
    listen(sockfd,5);
    while(1)
    {
        clientLength=sizeof(client);
        newfd=accept(sockfd,(struct sockaddr*)&client,&clientLength);
        expected=0;
        receivedFrames=0;
        mode2Lost=0;
        mode2Skip=0;
        receivedQueue.front=0;
        receivedQueue.rear=0;
        while(recv(newfd,&msg,sizeof(msg),0)>0)
        {
            if(msg.type==MSG_EXIT) break;
            if(msg.type!=MSG_FRAME) continue;
            totalFrames=msg.totalFrames;
            messageLength=msg.messageLength;
            mode=msg.mode;
            enqueue(&receivedQueue,msg.frame);
            frame=dequeue(&receivedQueue);
            printf("\nFrame %d Received\n",frame.seqNo);
            printf("| Source IP | Destination IP | Seq | 16-bit Data | Parity |\n");
            printf("| %s | %s | %d | %s | %d |\n",frame.sourceIP,frame.destinationIP,frame.seqNo,frame.data,frame.parityBit);
            if(!checkParity(frame.data,frame.parityBit))
            {
                printf("Frame %d Error Detected\n",frame.seqNo);
                printf("Frame %d Discarded\n",frame.seqNo);
                continue;
            }
            if(frame.seqNo==expected)
            {
                strcpy(receivedData+receivedFrames*16,frame.data);
                receivedFrames++;
                expected++;
                if(mode==2 && frame.seqNo==0 && mode2Lost==0)
                {
                    printf("ACK 0 Lost\n");
                    mode2Lost=1;
                    mode2Skip=1;
                    continue;
                }
                if(mode==2 && frame.seqNo==2 && mode2Skip==1)
                {
                    mode2Skip=0;
                    continue;
                }
                ack.ackNo=frame.seqNo;
                send(newfd,&ack,sizeof(ack),0);
                printf("ACK %d Sent\n",ack.ackNo);
            }
            else if(frame.seqNo<expected)
            {
                ack.ackNo=frame.seqNo;
                send(newfd,&ack,sizeof(ack),0);
                printf("ACK %d Sent\n",ack.ackNo);
            }
            else
            {
                printf("Frame %d Out of Order\n",frame.seqNo);
                printf("Frame %d Discarded\n",frame.seqNo);
            }
            if(receivedFrames==totalFrames)
                binaryToText(receivedData,messageLength);
        }
        close(newfd);
    }
    close(sockfd);
    return 0;
}