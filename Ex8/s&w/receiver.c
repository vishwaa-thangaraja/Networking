#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8200
#define MSG_FRAME 1
#define MSG_EXIT 2
typedef struct
{
    char sourceIP[16];
    char destinationIP[16];
    int seqNo;
    char data[17];
    int parityBit;
} Frame;
typedef struct
{
    int type;
    int mode;
    int totalFrames;
    int messageLength;
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
int main()
{
    int serverfd,clientfd;
    int Rn=0;
    int firstTimeout=1;
    int totalFrames=0;
    int receivedFrames=0;
    int messageLength=0;
    int receivedBits=0;
    char binary[801];
    char message[101];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    serverfd=socket(AF_INET,SOCK_STREAM,0);
    if(serverfd<0)
    {
        perror("Socket creation failed");
        return 1;
    }
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(PORT);
    if(bind(serverfd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0)
    {
        perror("Bind failed");
        close(serverfd);
        return 1;
    }
    if(listen(serverfd,1)<0)
    {
        perror("Listen failed");
        close(serverfd);
        return 1;
    }
    printf("=============================================\n");
    printf("      STOP-AND-WAIT ARQ - RECEIVER\n");
    printf("=============================================\n");
    printf("\nFRAME STRUCTURE\n");
    printf("---------------------------------------------\n");
    printf("| Source IP | Destination IP | Seq | 16-bit Data | Parity |\n");
    printf("---------------------------------------------\n");
    printf("\nWaiting for sender on port %d...\n",PORT);
    client_len=sizeof(client_addr);
    clientfd=accept(serverfd,(struct sockaddr *)&client_addr,&client_len);
    if(clientfd<0)
    {
        perror("Accept failed");
        close(serverfd);
        return 1;
    }
    printf("Sender connected.\n");
    printf("Rn = 0\n");
    printf("\nSOURCE IP\n");
    printIPBinary("127.0.0.1");
    printf("DESTINATION IP\n");
    printIPBinary("127.0.0.1");
    while(1)
    {
        Message msg;
        Ack ack;
        int n,sum=0,i;
        memset(&msg,0,sizeof(msg));
        n=recv(clientfd,&msg,sizeof(msg),0);
        if(n<=0)
        {
            printf("Sender disconnected.\n");
            break;
        }
        if(msg.type==MSG_EXIT)
        {
            printf("\nEXIT signal received.\n");
            break;
        }
        if(msg.type!=MSG_FRAME)
            continue;
        if(receivedFrames==0)
        {
            totalFrames=msg.totalFrames;
            messageLength=msg.messageLength;
            Rn=0;
            if(msg.mode==2)
                firstTimeout=1;
            memset(binary,0,sizeof(binary));
            memset(message,0,sizeof(message));
            receivedBits=0;
        }
        printf("\n--------------------------------------------\n");
        printf("Frame %d RECEIVED\n",msg.frame.seqNo);
        printf("Data     : %s\n",msg.frame.data);
        printf("Parity Bit : %d\n",msg.frame.parityBit);
        sum=0;
        for(i=0;i<16;i++)
            if(msg.frame.data[i]=='1')
                sum++;
        sum%=2;
        if(sum!=msg.frame.parityBit)
        {
            printf("PARITY BIT ERROR.\n");
            printf("Frame %d DISCARDED.\n",msg.frame.seqNo);
            printf("No ACK sent.\n");
            printf("Waiting for retransmission...\n");
            continue;
        }
        printf("Parity Bit Verified.\n");
        if(msg.frame.seqNo==Rn)
        {
            printf("Frame %d is EXPECTED.\n",msg.frame.seqNo);
            for(i=0;i<16;i++)
            {
                binary[receivedBits]=msg.frame.data[i];
                receivedBits++;
            }
            receivedFrames++;
            if(msg.mode==2&&firstTimeout==1)
            {
                printf("ACK %d LOST.\n",(Rn+1)%2);
                Rn=(Rn+1)%2;
                firstTimeout=0;
                continue;
            }
            Rn=(Rn+1)%2;
            ack.ackNo=Rn;
            send(clientfd,&ack,sizeof(ack),0);
            printf("ACK %d SENT\n",ack.ackNo);
        }
        else
        {
            printf("Frame %d is NOT expected.\n",msg.frame.seqNo);
            printf("Expected Frame %d.\n",Rn);
            printf("Duplicate frame discarded.\n");
            ack.ackNo=Rn;
            send(clientfd,&ack,sizeof(ack),0);
            printf("ACK %d SENT\n",ack.ackNo);
        }
        if(receivedFrames==totalFrames)
        {
            int byteValue,bit,position;
            printf("\n=============================================\n");
            printf("ALL FRAMES RECEIVED\n");
            printf("=============================================\n");
            for(i=0;i<messageLength;i++)
            {
                byteValue=0;
                position=i*8;
                for(bit=0;bit<8;bit++)
                {
                    byteValue=byteValue*2;
                    if(binary[position+bit]=='1')
                        byteValue=byteValue+1;
                }
                message[i]=byteValue;
            }
            message[messageLength]='\0';
            printf("Binary converted back to characters.\n");
            printf("MESSAGE : %s\n",message);
            receivedFrames=0;
            receivedBits=0;
            totalFrames=0;
            messageLength=0;
            memset(binary,0,sizeof(binary));
            memset(message,0,sizeof(message));
        }
    }
    close(clientfd);
    close(serverfd);
    printf("Connection closed.\n");
    return 0;
}