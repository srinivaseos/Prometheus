#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

int login_details();

void msg_func(int sockfd,struct sockaddr_in cli_addr,socklen_t len) 
{
   char buffer[100] = "hi server";
   
   int ret = sendto(sockfd,"hi_server",100,0,(struct sockaddr*)&cli_addr,len);
   if(ret<0)
   {
     perror("send to failed\n");
     exit(0);
   }
   else
   {
    printf("send success\n");
   }
   // printf("buffer =%s\n",buffer);
}


int main()
{
 	int sockfd;
 	struct sockaddr_in cli_addr;
 	//socklen_t len = sizeof(cli_addr);
 	sockfd = socket(AF_INET,SOCK_STREAM,0);
 	if(sockfd < 0)
 	{
 		printf("sockfd not created\n");
 		exit(1);
 	}
 	else
	{
		printf("socket created\n");
	}
 	cli_addr.sin_family = AF_INET;
 	cli_addr.sin_addr.s_addr =  INADDR_ANY;//inet_addr("10.0.2.15");
 	cli_addr.sin_port = htons(7008);
 	
 	if((connect(sockfd,(struct sockaddr*)&cli_addr,sizeof(cli_addr)))<0)
 	{
 		printf("connection failed\n");
 		exit(1);
 	}
 	printf("connection successfull\n");
 	
 	// char buffer[100] = "hi_server";
    
    int buffer = login_details();
    int ret = send(sockfd, buffer, sizeof(buffer), 0);
    if (ret < 0)
    {
        perror("send failed\n");
        exit(1);
    }
    else if (ret == 0)
    {
        printf("Connection closed by peer\n");
    }
    else
    {
        printf("send success\n");
    }
 	//msg_func(sockfd,cli_addr,len);
 	
 	//close(sockfd); // close connection
 	return 0;
}
