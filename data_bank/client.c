// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
#define PORT	 8080
#define MAXLINE 1024

typedef struct  allocate_udp_client
{
	int sockfd;
	struct sockaddr_in	 servaddr;

}allocate_udp_client_t;

allocate_udp_client_t *udptr;


int udp_recv()
{
	
}
// Driver code
int main() {
	allocate_udp_client_t *udptr = malloc(sizeof(struct allocate_udp_client));
	char buffer[MAXLINE];
	char *hello = "Hello from client";
	
	
	// Creating socket file descriptor
	if ( (udptr->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&udptr->servaddr, 0, sizeof(udptr->servaddr));
		
	// Filling server information
	udptr->servaddr.sin_family = AF_INET;
	udptr->servaddr.sin_port = htons(PORT);
	udptr->servaddr.sin_addr.s_addr = INADDR_ANY;
		
	int n, len;
		
	sendto(udptr->sockfd, (const char *)hello, strlen(hello),
		MSG_CONFIRM, (const struct sockaddr *) &udptr->servaddr,
			sizeof(udptr->servaddr));
	printf("Hello message sent.\n");
			
	n = recvfrom(udptr->sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &udptr->servaddr,&len);
	buffer[n] = '\0';
	printf("Server : %s\n", buffer);
	
	//close(sockfd);
	return 0;
}

