#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <string.h>
//#define INADDR_ANY 127.0.0.1
#define PORT 7007


int recv_msg(int new_socket,struct sockaddr_in cli_addr,socklen_t len)
{
   //char buffer[100];// = "hi server";
   
   int buffer;
   int ret = recvfrom(new_socket,buffer,sizeof(buffer),0,(struct sockaddr*)&cli_addr,&len);
   if(ret<0)
   {
     perror("send to failed\n");
     exit(0);
   }
   else
   {
    printf("send success\n");
   }
   printf("buffer =%d\n",buffer);
   
   
      /* int ret = recv(sockfd, buffer, 100, 0); // Use recv instead of recvfrom
    if (ret < 0) {
        perror("recv failed\n");
        exit(0);
    } else {
        printf("recv success\n");
    }
    printf("buffer = %s\n", buffer);*/

	return buffer;
}


//int main()
int server()
{
	int sockfd,new_socket;
	char buffer[100];
	struct sockaddr_in server_addr,cli_addr;
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	
	if(sockfd<0)
	{
	  printf("socket not created\n");
	  exit(0);
	}
	else
	{
		printf("socket created\n"); 
	}
	
	server_addr.sin_family = AF_INET;
 	server_addr.sin_addr.s_addr = INADDR_ANY;// inet_addr("10.0.2.15");
 	server_addr.sin_port = htons(7008);
 	
 	/*if((bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0)
 	{
 		printf("bind failed\n");
 		exit(0);
 	}*/
 	 if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
 	 {
		perror("Bind failed");
		exit(0);
  	  }

 	else
 	{
 		printf("bind succeffull\n");
 	}
 	
 	
 	if((listen(sockfd,3))<0)
 	{
 		perror("listen failed\n");
 		exit(1);
 	}
 	else
 	{
 		printf("listen succeffull\n");
 		
 	}
 	socklen_t len = sizeof(cli_addr);
 	
 	
 	if((new_socket=accept(sockfd,(struct sockaddr*)&cli_addr,&len))<0)
 	{
 	  perror("not accpet  cliaddr\n");
 	  exit(0);
 	}
 	else
 	{	
 	printf("accpeted cliaddr\n");
 	printf("%d\n",cli_addr.sin_addr.s_addr);
 	}
 	
 	int num=recv_msg(new_socket,cli_addr,len);
 	
 	/*int ret = recv(new_socket, buffer, 100, 0); // Use recv instead of recvfrom
    if (ret < 0) {
        perror("recv failed\n");
        exit(0);
    } else {
        printf("recv success\n");
    }
    printf("buffer = %s\n", buffer);
    memset(buffer,0,sizeof(buffer));*/
	

 	//close(sockfd); // close connection
 	return num;
}
