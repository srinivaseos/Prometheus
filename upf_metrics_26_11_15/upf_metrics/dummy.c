// server program for udp connection
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>

#define PORT 8805
#define MAXLINE 1000

// Driver code
int main()
{
    char buffer[100];
    char *message = "Hello Client";
    int listenfd, len;
    struct sockaddr_in servaddr, cliaddr;
   // bzero(&servaddr, sizeof(servaddr));

    // Create a UDP Socket
    listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset( &servaddr, 0, sizeof( struct sockaddr_in));
    servaddr.sin_addr.s_addr = inet_addr("192.168.144.15");// htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;
    //sleep(5);
    // bind server address to socket descriptor
   if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != -1)
   {
        
    printf("client = %s:%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
   }
   else
   {
     printf("\nbind failure\n");
      exit(EXIT_FAILURE);
        printf("EXITING client = %s:%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)); 
        exit(0);
   }
   // send the response
    cliaddr.sin_addr.s_addr = inet_addr("192.168.144.11");// htonl(INADDR_ANY);
    cliaddr.sin_port = htons(PORT);
    cliaddr.sin_family = AF_INET;
    sendto(listenfd, message, MAXLINE, 0,(struct sockaddr*)&cliaddr, sizeof(cliaddr));
    //receive the datagram
    len = sizeof(cliaddr);
    int n = recvfrom(listenfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr,&len); //receive message from serv
    printf("----buffer=%s\n",buffer);
    buffer[n] = '\0';
   // puts(buffer);
      printf("client2 = %s:%d:%d\n", inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port),cliaddr.sin_port);
	
    

    
    //close(listenfd);
}


/*
#define PORT 8806
int main()
{
                struct sockaddr_in servaddr,cliaddr;
    int len=sizeof(cliaddr);
        char buffer[50];
         // creation of socket
        int fd=socket(AF_INET,SOCK_DGRAM,0);
        if(fd<0)
        {
                printf("\nSOCKET NOT CREATED");
                exit(1);
        }
        else
        {
                printf("\nSOCKET CREATED");
        }

        // port address

        servaddr.sin_family=AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(PORT);


     if( bind(fd,(const struct sockaddr *)&servaddr, sizeof(servaddr)) ==-1)

        {
                printf("\nbind failure");
      exit(EXIT_FAILURE);
        }

//      printf("\n----------- server listening port %d--------",PORT);

        int t_len = recvfrom(fd,buffer,sizeof(buffer),0,(struct sockaddr *)&cliaddr,&len);
        if(t_len == -1)
        {
                printf("\n recv failed");

        }
        printf("\nrecived :%s",buffer);
        close(fd);
        return 0;
}
*/