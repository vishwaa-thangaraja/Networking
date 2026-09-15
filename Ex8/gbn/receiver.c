#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8202
#define MSG_FRAME 1
#define MSG_EXIT 2

typedef struct
{
    char sourceIP[16], destinationIP[16], data[17];
    int seqNo, parityBit;
} Frame;

typedef struct FrameNode
{
    Frame frame;
    struct FrameNode *next;
} FrameNode;

typedef struct
{
    int type, mode, totalFrames, messageLength;
    Frame frame;
} Message;

typedef struct
{
    int ackNo;
} Ack;

void binaryValue(int value,char binary[])
{
    int i;

    for(i=7;i>=0;i--)
        binary[7-i]=(value>>i)&1?'1':'0';

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

    for(i=0;i<16;i++)
        if(data[i]=='1')
            ones++;

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

        for(j=0;j<8;j++)
            value=value*2+(data[i*8+j]-'0');

        ch=(char)value;

        printf("%c",ch);
    }

    printf("\n");
}

void addFrame(FrameNode **head,Frame frame)
{
    FrameNode *newNode;
    FrameNode *last;

    newNode=(FrameNode *)malloc(sizeof(FrameNode));

    newNode->frame=frame;
    newNode->next=NULL;

    if(*head==NULL)
    {
        *head=newNode;
        return;
    }

    last=*head;

    while(last->next!=NULL)
        last=last->next;

    last->next=newNode;
}

FrameNode *getFrameNode(FrameNode *head,int index)
{
    int i=0;

    while(head!=NULL && i<index)
    {
        head=head->next;
        i++;
    }

    return head;
}

void freeFrames(FrameNode *head)
{
    FrameNode *temp;

    while(head!=NULL)
    {
        temp=head;
        head=head->next;
        free(temp);
    }
}

int main()
{
    int sockfd,newfd,expected,receivedFrames;
    int i,totalFrames,messageLength,mode;
    int mode2Lost,mode2Skip;

    char receivedData[1600];

    FrameNode *receivedHead=NULL;

    Message msg;
    Ack ack;

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

        newfd=accept(sockfd,
                     (struct sockaddr*)&client,
                     &clientLength);

        expected=0;
        receivedFrames=0;
        mode2Lost=0;
        mode2Skip=0;

        receivedHead=NULL;

        while(recv(newfd,&msg,sizeof(msg),0)>0)
        {
            if(msg.type==MSG_EXIT)
                break;

            if(msg.type!=MSG_FRAME)
                continue;

            totalFrames=msg.totalFrames;
            messageLength=msg.messageLength;
            mode=msg.mode;

            printf("\nFrame %d Received\n",msg.frame.seqNo);

            printf("| Source IP | Destination IP | Seq | 16-bit Data | Parity |\n");

            printf("| %s | %s | %d | %s | %d |\n",
                   msg.frame.sourceIP,
                   msg.frame.destinationIP,
                   msg.frame.seqNo,
                   msg.frame.data,
                   msg.frame.parityBit);

            if(!checkParity(msg.frame.data,msg.frame.parityBit))
            {
                printf("Frame %d Error Detected\n",
                       msg.frame.seqNo);

                printf("Frame %d Discarded\n",
                       msg.frame.seqNo);

                continue;
            }

            if(msg.frame.seqNo==expected)
            {
                addFrame(&receivedHead,msg.frame);

                strcpy(receivedData+receivedFrames*16,
                       msg.frame.data);

                receivedFrames++;
                expected++;

                if(mode==2 &&
                   msg.frame.seqNo==0 &&
                   mode2Lost==0)
                {
                    printf("ACK 0 Lost\n");

                    mode2Lost=1;
                    mode2Skip=1;

                    continue;
                }

                if(mode==2 &&
                   msg.frame.seqNo==2 &&
                   mode2Skip==1)
                {
                    mode2Skip=0;

                    continue;
                }

                ack.ackNo=msg.frame.seqNo;

                send(newfd,&ack,sizeof(ack),0);

                printf("ACK %d Sent\n",ack.ackNo);
            }
            else if(msg.frame.seqNo<expected)
            {
                ack.ackNo=msg.frame.seqNo;

                send(newfd,&ack,sizeof(ack),0);

                printf("ACK %d Sent\n",ack.ackNo);
            }
            else
            {
                printf("Frame %d Out of Order\n",
                       msg.frame.seqNo);

                printf("Frame %d Discarded\n",
                       msg.frame.seqNo);
            }

            if(receivedFrames==totalFrames)
                binaryToText(receivedData,messageLength);
        }

        freeFrames(receivedHead);
        receivedHead=NULL;

        close(newfd);
    }

    close(sockfd);

    return 0;
}