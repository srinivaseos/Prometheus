
// server program for udp connection
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<netinet/in.h>
#define PORT 8806
#define MAXLINE 100

//PFCP_SESSION_RELATED_MESSAGES
#define PFCP_SESSION_ESTABLISHMENT_REQUEST                                   (50)
#define PFCP_SESSION_ESTABLISHMENT_RESPONSE                                  (51)
#define PFCP_SESSION_MODIFICATION_REQUEST                                    (52)
#define PFCP_SESSION_MODIFICATION_RESPONSE                                   (53)
#define PFCP_SESSION_DELETION_REQUEST                                        (54)
#define PFCP_SESSION_DELETION_RESPONSE                                       (55)
#define PFCP_SESSION_REPORT_REQUEST                                          (56)
#define PFCP_SESSION_REPORT_RESPONSE                                         (57)

typedef struct panel
{
  int dataype[8];
  uint64_t ser_last;
	uint64_t ser_total;
	

	uint64_t sea_last;
	uint64_t sea_total;
	

	uint64_t smr_last;
	uint64_t smr_total;
	
	
	uint64_t sma_last;
	uint64_t sma_total;
	
	uint64_t sdr_last;
	uint64_t sdr_total;
	
	
	uint64_t sda_last;
	uint64_t sda_total;
	
	
	uint64_t sur_last;
	uint64_t sur_total;
	

	uint64_t sua_last;
	uint64_t sua_total;
	
}panel_t;




typedef struct udp_client_connect
{
    int sockfd;
    struct sockaddr_in servaddr;
}udp_client_t;



void display(struct panel *ppi)
{
    printf("--display -- sea=%ld\n",ppi->sea_total);
    printf("--display -- sma=%ld\n",ppi->sma_total);
}

int i=0;
int pfcp_perf_incerment(struct panel *ppi)
{
  int fun;
    ppi->sea_total = 0;
    ppi->sma_total = 0;
    ppi->sda_total = 0;
    ppi->sur_total = 0 ;

    
    printf("-----next\n");
    static int i=0;
   // printf("enetr msg type num=");
    //scanf("%d",&fun);

    if(  ppi->dataype[0] == PFCP_SESSION_ESTABLISHMENT_REQUEST)
    {
      printf("-----next\n");
     ppi->sea_total =5+i;
    }
    if(ppi->dataype[1]==PFCP_SESSION_MODIFICATION_REQUEST)
    {
      printf("-----next\n");
       ppi->sma_total =5+i;
    }
    if( ppi->dataype[2]==PFCP_SESSION_DELETION_REQUEST)
    {
      printf("-----next\n");
      ppi->sda_total =5+i;
    }
    else{
      printf("-----next\n");
    ppi->sur_total =5+i;
    }
    
    i+=5;
    printf("buffer size=%ld\n",sizeof(struct panel));
    display(ppi);
}





udp_client_t * udpclient = NULL;
int udp_client_conn(udp_client_t * udpclient)
{
    // int sockfd;
    //char buffer[MAXLINE];
    //char *message = "Hello Server from Client!";
    //socklen_t len;

    // Create a UDP socket
    if ((udpclient->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&udpclient->servaddr, 0, sizeof(udpclient->servaddr));

    // Fill server information
    udpclient->servaddr.sin_family = AF_INET;
    udpclient->servaddr.sin_port = htons(PORT);
    udpclient->servaddr.sin_addr.s_addr = inet_addr("10.0.2.15"); // Server IP address 



        printf("connected = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
     // Send message to server
    //sendto(udpclient->sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
   // printf("sendto = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
   // printf("Message sent to server.\n");
    return 1;
}

void udp_send_msgto_server(udp_client_t * udpclient,panel_t *pfcp)
{
   
  uint8_t *buffer= (uint8_t *)pfcp;

    size_t len = sizeof(panel_t);
   printf("--len=%ld\n",len);
  //  sendto(udpclient->sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
    int bytes_sent = sendto(udpclient->sockfd,buffer, len, MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
    if (bytes_sent == -1) {
     perror("sendto failed");
    // Handle error appropriately
    } else {
          perror("sendto success");
    // Handle success
    }
    printf("sendto = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
    printf("Message sent to server.\n");
}

void udp_recv_msgby_server(udp_client_t * udpclient,panel_t *pfcp)
{
    // Receive response from server
    uint8_t *buffer;
    socklen_t len;
    len = sizeof(udpclient->servaddr);
    int n = recvfrom(udpclient->sockfd,(void *)buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&udpclient->servaddr, &len);
   // buffer[n] = '\0';
    struct panel *pfcp1 = (struct panel *)buffer;
      printf("buffer=%ld\n",pfcp1->sda_total);
    printf("RECVF = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
    //printf("Server : %s\n", buffer);
    // pfcp_Stats_handler(pfcp);

}

// Driver code
int main()
{


  udp_client_t * udpclient=(udp_client_t*)malloc(sizeof(udp_client_t));

  panel_t *pfcp =(struct panel*)malloc(sizeof(struct panel));
  pfcp->dataype[0]=50;
  pfcp->dataype[1]=52;
  pfcp->dataype[2]=54;
   int sts = udp_client_conn(udpclient);
  pfcp_perf_incerment(pfcp);
    if(sts)
    {    
        printf("------------sts=%d-------\n",sts);
        udp_send_msgto_server(udpclient,pfcp);
    }
    //udp_recv_msgby_server(udpclient,pfcp);

    
    //close(listenfd);
}

