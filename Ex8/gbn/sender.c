#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
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

void makeFrames(char text[],FrameNode **head,int totalFrames)
{
    int i,j,k,len,ones;
    char binary[9];
    FrameNode *newNode,*last=NULL;

    len=strlen(text);

    *head=NULL;

    for(i=0;i<totalFrames;i++)
    {
        newNode=(FrameNode *)malloc(sizeof(FrameNode));

        strcpy(newNode->frame.sourceIP,"127.0.0.1");
        strcpy(newNode->frame.destinationIP,"127.0.0.1");

        newNode->frame.seqNo=i;
        k=0;
        ones=0;

        for(j=0;j<2;j++)
        {
            if(i*2+j<len)
                binaryValue(text[i*2+j],binary);
            else
                strcpy(binary,"00000000");

            strcpy(newNode->frame.data+k,binary);
            k+=8;
        }

        newNode->frame.data[16]='\0';

        for(j=0;j<16;j++)
            if(newNode->frame.data[j]=='1')
                ones++;

        newNode->frame.parityBit=ones%2;

        newNode->next=NULL;

        if(*head==NULL)
            *head=newNode;
        else
            last->next=newNode;

        last=newNode;
    }
}

FrameNode *getNode(FrameNode *head,int index)
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
    int sockfd,mode,windowSize,totalFrames,length,base,next;
    int lostFrame,errorFrame;
    int errorFrameNo=-1,errorBitIndex=-1;
    int errorInjected=0;

    char text[100];

    FrameNode *head=NULL;
    FrameNode *current;

    Message msg;
    Ack ack;

    struct sockaddr_in server;
    struct timeval tv;

    while(1)
    {
        printf("\n1. Normal Propagation\n");
        printf("2. Time-out (ACK Lost)\n");
        printf("3. Frame Lost\n");
        printf("4. Error in Frame\n");
        printf("5. Exit\n");
        printf("Enter mode: ");
        scanf("%d",&mode);
        getchar();

        if(mode==5)
        {
            sockfd=socket(AF_INET,SOCK_STREAM,0);

            server.sin_family=AF_INET;
            server.sin_port=htons(PORT);
            server.sin_addr.s_addr=inet_addr("127.0.0.1");

            connect(sockfd,(struct sockaddr*)&server,sizeof(server));

            msg.type=MSG_EXIT;

            send(sockfd,&msg,sizeof(msg),0);

            close(sockfd);

            break;
        }

        printf("Enter message: ");
        fgets(text,sizeof(text),stdin);
        text[strcspn(text,"\n")]='\0';

        length=strlen(text);
        totalFrames=(length+1)/2;

        printf("Total Frames: %d\n",totalFrames);

        if(mode==4)
        {
            makeFrames(text,&head,totalFrames);

            printf("\nFrame Details:\n");
            printf("| Frame No | 16-bit Data       | Parity |\n");

            current=head;

            while(current!=NULL)
            {
                printf("| %8d | %16s | %6d |\n",
                       current->frame.seqNo,
                       current->frame.data,
                       current->frame.parityBit);

                current=current->next;
            }

            printf("\nEnter frame number in which error should be injected (0-%d): ",
                   totalFrames-1);
            scanf("%d",&errorFrameNo);

            while(errorFrameNo<0 || errorFrameNo>=totalFrames)
            {
                printf("Invalid frame number. Enter again (0-%d): ",
                       totalFrames-1);
                scanf("%d",&errorFrameNo);
            }

            printf("Enter bit index to change (0-15): ");
            scanf("%d",&errorBitIndex);

            while(errorBitIndex<0 || errorBitIndex>15)
            {
                printf("Invalid bit index. Enter again (0-15): ");
                scanf("%d",&errorBitIndex);
            }

            printf("\nError Injection Selected:\n");
            printf("Frame Number : %d\n",errorFrameNo);
            printf("Bit Index    : %d\n",errorBitIndex);
            printf("Original Bit : %c\n",
                   getNode(head,errorFrameNo)->frame.data[errorBitIndex]);
            printf("Changed Bit  : %c\n",
                   getNode(head,errorFrameNo)->frame.data[errorBitIndex]=='0'?'1':'0');

            getchar();
        }
        else
        {
            printf("Enter Window Size: ");
            scanf("%d",&windowSize);
            getchar();

            if(windowSize>totalFrames)
                windowSize=totalFrames;

            makeFrames(text,&head,totalFrames);
        }

        if(mode==4)
        {
            printf("Enter Window Size: ");
            scanf("%d",&windowSize);
            getchar();

            if(windowSize>totalFrames)
                windowSize=totalFrames;
        }

        sockfd=socket(AF_INET,SOCK_STREAM,0);

        server.sin_family=AF_INET;
        server.sin_port=htons(PORT);
        server.sin_addr.s_addr=inet_addr("127.0.0.1");

        connect(sockfd,(struct sockaddr*)&server,sizeof(server));

        tv.tv_sec=5;
        tv.tv_usec=0;

        setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

        base=0;
        next=0;
        lostFrame=0;
        errorFrame=0;
        errorInjected=0;

        msg.type=MSG_FRAME;
        msg.mode=mode;
        msg.totalFrames=totalFrames;
        msg.messageLength=length;

        while(base<totalFrames)
        {
            while(next<totalFrames && next<base+windowSize)
            {
                current=getNode(head,next);

                msg.frame=current->frame;

                if(mode==3 && next==0 && lostFrame==0)
                {
                    printf("Frame %d Lost\n",next);
                    lostFrame=1;
                    next++;
                    continue;
                }

                if(mode==4 &&
                   next==errorFrameNo &&
                   errorInjected==0)
                {
                    msg.frame.data[errorBitIndex]=
                        (msg.frame.data[errorBitIndex]=='0')?'1':'0';

                    printf("Frame %d Sent With Error\n",next);

                    send(sockfd,&msg,sizeof(msg),0);

                    errorInjected=1;

                    next++;
                    continue;
                }

                printf("Frame %d Sent\n",next);

                send(sockfd,&msg,sizeof(msg),0);

                next++;
            }

            while(1)
            {
                if(recv(sockfd,&ack,sizeof(ack),0)<=0)
                {
                    printf("Time-out! Retransmitting frames %d to %d\n",
                           base,next-1);

                    next=base;

                    break;
                }

                printf("ACK %d Received\n",ack.ackNo);

                if(mode==2 && ack.ackNo>base)
                {
                    printf("ACK %d Missing\n",base);

                    printf("Retransmitting frames %d to %d\n",
                           base,next-1);

                    next=base;

                    break;
                }

                if(ack.ackNo>=base)
                {
                    base=ack.ackNo+1;

                    break;
                }
            }
        }

        msg.type=MSG_EXIT;

        send(sockfd,&msg,sizeof(msg),0);

        close(sockfd);

        freeFrames(head);
        head=NULL;
    }

    return 0;
}