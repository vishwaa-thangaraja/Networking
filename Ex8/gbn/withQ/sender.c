#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
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
void clearQueue(Queue *q)
{
    q->front=0;
    q->rear=0;
}
void binaryValue(int value,char binary[])
{
    int i;
    for(i=7;i>=0;i--) binary[7-i]=(value>>i)&1?'1':'0';
    binary[8]='\0';
}
void makeFrames(char text[],Frame frames[],int totalFrames)
{
    int i,j,k,len,ones;
    char binary[9];
    len=strlen(text);
    for(i=0;i<totalFrames;i++)
    {
        strcpy(frames[i].sourceIP,"127.0.0.1");
        strcpy(frames[i].destinationIP,"127.0.0.1");
        frames[i].seqNo=i;k=0;ones=0;
        for(j=0;j<2;j++)
        {
            if(i*2+j<len) binaryValue(text[i*2+j],binary);
            else strcpy(binary,"00000000");
            strcpy(frames[i].data+k,binary);
            k+=8;
        }
        frames[i].data[16]='\0';
        for(j=0;j<16;j++) if(frames[i].data[j]=='1') ones++;
        frames[i].parityBit=ones%2;
    }
}
int main()
{
    int sockfd,mode,windowSize,totalFrames,length,base,next,lostFrame,errorFrame;
    char text[100];
    Frame frames[50],temp,frame;
    Message msg;
    Ack ack;
    Queue sentQueue;
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
        printf("Enter Window Size: ");
        scanf("%d",&windowSize);
        getchar();
        if(windowSize>totalFrames) windowSize=totalFrames;
        makeFrames(text,frames,totalFrames);
        sockfd=socket(AF_INET,SOCK_STREAM,0);
        server.sin_family=AF_INET;
        server.sin_port=htons(PORT);
        server.sin_addr.s_addr=inet_addr("127.0.0.1");
        connect(sockfd,(struct sockaddr*)&server,sizeof(server));
        tv.tv_sec=5; tv.tv_usec=0;
        setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
        base=0; next=0; lostFrame=0; errorFrame=0;
        sentQueue.front=0;
        sentQueue.rear=0;
        msg.type=MSG_FRAME;
        msg.mode=mode;
        msg.totalFrames=totalFrames;
        msg.messageLength=length;
        while(base<totalFrames)
        {
            while(next<totalFrames && next<base+windowSize)
            {
                msg.frame=frames[next];
                if(mode==3 && next==0 && lostFrame==0)
                {
                    printf("Frame %d Lost\n",next);
                    lostFrame=1;
                    next++;
                    continue;
                }
                if(mode==4 && next==0 && errorFrame==0)
                {
                    temp=msg.frame;
                    msg.frame.data[0]=(msg.frame.data[0]=='0')?'1':'0';
                    printf("Frame %d Sent With Error\n",next);
                    send(sockfd,&msg,sizeof(msg),0);
                    msg.frame=temp;
                    enqueue(&sentQueue,msg.frame);
                    errorFrame=1;
                    next++;
                    continue;
                }
                printf("Frame %d Sent\n",next);
                send(sockfd,&msg,sizeof(msg),0);
                enqueue(&sentQueue,msg.frame);
                next++;
            }
            while(1)
            {
                if(recv(sockfd,&ack,sizeof(ack),0)<=0)
                {
                    printf("Time-out! Retransmitting frames %d to %d\n",base,next-1);
                    clearQueue(&sentQueue);
                    next=base;
                    break;
                }
                printf("ACK %d Received\n",ack.ackNo);
                if(mode==2 && ack.ackNo>base)
                {
                    printf("ACK %d Missing\n",base);
                    printf("Retransmitting frames %d to %d\n",base,next-1);
                    clearQueue(&sentQueue);
                    next=base;
                    break;
                }
                if(ack.ackNo>=base)
                {
                    if(sentQueue.front<sentQueue.rear)
                    {
                        frame=dequeue(&sentQueue);
                    }
                    base=ack.ackNo+1;
                    break;
                }
            }
        }
        msg.type=MSG_EXIT;
        send(sockfd,&msg,sizeof(msg),0);
        close(sockfd);
    }
    return 0;
}