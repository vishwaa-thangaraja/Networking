#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000

struct Student
{
    char regno[20];
    char name[50];
    char department[30];
    int semester;
    float cgpa;
};

int main()
{
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen;

    char regno[20];
    char response[500];

    struct Student students[] =
    {
        {"23CSE001", "Arun", "CSE", 5, 8.75},
        {"23CSE002", "Vishwaa", "CSE", 5, 9.10},
        {"23ECE001", "Rahul", "ECE", 5, 8.45},
        {"23EEE001", "Karthik", "EEE", 5, 8.20},
        {"23IT001", "Priya", "IT", 5, 9.25}
    };

    int count = sizeof(students) / sizeof(students[0]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        printf("Socket creation failed.\n");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(sockfd,
             (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        printf("Bind failed.\n");
        return 1;
    }

    printf("Student Information Server started.\n");
    printf("Waiting for requests...\n\n");

    while (1)
    {
        addrLen = sizeof(clientAddr);

        recvfrom(sockfd,
                 regno,
                 sizeof(regno),
                 0,
                 (struct sockaddr *)&clientAddr,
                 &addrLen);

        regno[sizeof(regno) - 1] = '\0';

        printf("Received registration number: %s\n", regno);

        int found = 0;

        for (int i = 0; i < count; i++)
        {
            if (strcmp(regno, students[i].regno) == 0)
            {
                sprintf(response,
                        "Registration Number: %s\n"
                        "Name: %s\n"
                        "Department: %s\n"
                        "Semester: %d\n"
                        "CGPA: %.2f",
                        students[i].regno,
                        students[i].name,
                        students[i].department,
                        students[i].semester,
                        students[i].cgpa);

                found = 1;
                break;
            }
        }

        if (!found)
        {
            strcpy(response,
                   "ERROR: Student registration number not found.");
        }

        sendto(sockfd,
               response,
               strlen(response) + 1,
               0,
               (struct sockaddr *)&clientAddr,
               addrLen);

        printf("Response sent.\n\n");
    }

    close(sockfd);

    return 0;
}
