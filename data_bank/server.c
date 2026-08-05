// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
#include "struct_Def.h" 
#define PORT	 8080
#define MAXLINE 1024



allocate_udp_server_t *udptr;


int udp_recv()
{
	char buffer[MAXLINE];
	char *hello = "Hello from server";
	int len, n;
	
	len = sizeof(udptr->cliaddr); //len is value/result
	
	n = recvfrom(udptr->sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, ( struct sockaddr *) &udptr->cliaddr,&len);
	buffer[n] = '\0';
	printf("Client : %s\n", buffer);
	sendto(udptr->sockfd, (const char *)hello, strlen(hello),MSG_CONFIRM, (const struct sockaddr *) &udptr->cliaddr,len);
	printf("Hello message sent.\n");
	return 2;
}
// Driver code
int udp_server()
{
	
	udptr = malloc(sizeof(struct allocate_udp_server));
	
		
	// Creating socket file descriptor
	if ( (udptr->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&udptr->servaddr, 0, sizeof(udptr->servaddr));
	memset(&udptr->cliaddr, 0, sizeof(udptr->cliaddr));
		
	// Filling server information
	udptr->servaddr.sin_family = AF_INET; // IPv4
	udptr->servaddr.sin_addr.s_addr = INADDR_ANY;
	udptr->servaddr.sin_port = htons(PORT);
		
	// Bind the socket with the server address
	if ( bind(udptr->sockfd, (const struct sockaddr *)&udptr->servaddr,sizeof(udptr->servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
		
	
		
	return 0;
}

