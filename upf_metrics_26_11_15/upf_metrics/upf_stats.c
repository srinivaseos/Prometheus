#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
//#include "pfcp_metric.h"
#include "pfcp_metric.h" 

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



typedef struct server
{
    int listenfd;
    struct sockaddr_in servaddr, cliaddr;
}server_t;

typedef struct pfcpmsg
{
    uint8_t type ;
    char message[30];// = "Hello Server from Client!";
}pfcpmsg_t;

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

panel_t *ppi=NULL;

typedef void pfcp_Stats_handler_t(panel_t *msgtype);

server_t *server_conn=NULL;

void display(struct panel *ppi)
{
    printf("--display -- sea=%ld\n",ppi->sea_total);
    printf("--display -- sma=%ld\n",ppi->sma_total);
}


void pfcp_Stats_handler(struct panel *ppi)
{
    int i=0;
    while(i<6)
    {
    printf("inside s_handler=%d:%d\n",ppi->dataype[i],i);
    switch( ppi->dataype[i])
    {
        case PFCP_SESSION_ESTABLISHMENT_REQUEST:

            successful_task_request( "POST",1);
            break;
        case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
            successful_task_responce( "POST",1);    
            break;
        case PFCP_SESSION_MODIFICATION_REQUEST:
            successful_task_request( "POST",2);
            break;
        case PFCP_SESSION_MODIFICATION_RESPONSE:
            successful_task_responce( "POST",2);
            break;
        case PFCP_SESSION_DELETION_REQUEST:
            successful_task_request( "POST",4);                                         
            break;
        case PFCP_SESSION_DELETION_RESPONSE:
            successful_task_responce( "POST",4);    
            break;
        default :
                printf("not valind=%d\n",ppi->dataype[i]);
                break;
    }
    printf("increase\n");
      i++;
    }

    sleep(100);
}



int  udp_server_send_msg(struct server *server_conn,struct panel *ppi)
{
    uint8_t *buffer;
    // send the response
   // char *message = "Hello Client";
    ssize_t len=sizeof(buffer);
   int sts = sendto(server_conn->listenfd, (const void *)buffer, len, 0,(struct sockaddr*)&server_conn->cliaddr, sizeof(server_conn->cliaddr));
   printf("sento --  sts=%d\n",sts);
}

int udp_recv_from_msg(struct server *server_conn,struct panel *ppi,pfcp_Stats_handler_t cb)
{
    //pfcp_perf_incerment(ppi);
    //uint8_t buffer[MAXLINE];
     uint8_t *buffer = malloc(sizeof(panel_t));
    if (!buffer) {
        perror("malloc");
        exit(-1);
    }
    int listenfd;
    
    printf("panel_t = %ld bytes\n", sizeof(panel_t));
    //receive the datagram
    socklen_t len = sizeof(server_conn->cliaddr);
    int n = recvfrom(server_conn->listenfd,buffer, sizeof(panel_t),  0, (struct sockaddr*)&server_conn->cliaddr,&len); //receive message from serv
   
    printf("\nbuffer=%ld\n", sizeof(panel_t));
    //buffer[n] = '\0';
    ppi = (struct panel *)buffer;
    
    if(n > 0)
    {
           printf("----sda=%ld:%d:%d:%d\n",ppi->sea_total,n,ppi->dataype[0],ppi->dataype[1]);
         
    }
    else
    {
        printf("n=%d\n",n);

         printf("error client2 = %s:%d:%d\n", inet_ntoa(server_conn->cliaddr.sin_addr),ntohs(server_conn->cliaddr.sin_port),server_conn->cliaddr.sin_port);
        perror("recv");
        exit(-1);
    }
  
      printf("client2 = %s:%d:%d\n", inet_ntoa(server_conn->cliaddr.sin_addr),ntohs(server_conn->cliaddr.sin_port),server_conn->cliaddr.sin_port);
      cb(ppi);
      printf("retunr to function\n");
       free(buffer);
    return 1;
}

void udp_Server_connection(struct server *server_conn)
{
     
    bzero(&server_conn->servaddr, sizeof(server_conn->servaddr));

    // Create a UDP Socket
    server_conn->listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server_conn->servaddr.sin_addr.s_addr = INADDR_ANY;// inet_addr("192.168.144.12");// htonl(INADDR_ANY);
    server_conn->servaddr.sin_port = htons(PORT);
    server_conn->servaddr.sin_family = AF_INET;

    // bind server address to socket descriptor
    if(bind(server_conn->listenfd, (struct sockaddr*)&server_conn->servaddr, sizeof(server_conn->servaddr)) != -1)
    {
         printf("client = %s:%d\n", inet_ntoa(server_conn->servaddr.sin_addr), ntohs(server_conn->servaddr.sin_port));

    }
    else
   {
     printf("\nbind failure\n");
     perror("binfailed");
     printf("EXITING client = %s:%d\n", inet_ntoa(server_conn->servaddr.sin_addr), ntohs(server_conn->servaddr.sin_port)); 
     exit(EXIT_FAILURE);
     exit(0);
   }


   /* char buffer[100];
    int len;
    //receive the datagram
    len = sizeof(server_conn->cliaddr);
    int n = recvfrom(server_conn->listenfd, buffer, sizeof(buffer),0, (struct sockaddr*)&server_conn->cliaddr,&len); //receive message from serv
  
    buffer[n] = '\0';
    puts(buffer);
      printf("client2 = %s:%d:%d\n", inet_ntoa(server_conn->cliaddr.sin_addr),ntohs(server_conn->cliaddr.sin_port),server_conn->cliaddr.sin_port);*/
   printf("connectiong\n");
}


int main() {

    metrics_prometheus("10.0.2.15", 9521 );
   
  /*  int i=50;
   for(int j=0;j<30;j++)
    {
        //printf("insidelop1\n");
        if(i == 56)
        {
            //printf("j iteration j=%d\n",j);
            sleep(10);
            i=50;
        }
       // pfcp_Stats_handler(i);
        i++;

    }*/
   server_t *server_conn=(struct server*)malloc(sizeof(struct server));

   udp_Server_connection(server_conn);
   
   panel_t  *ppi  = (struct panel*)malloc(sizeof(struct panel));
      display(ppi);
   int sts = udp_recv_from_msg(server_conn,ppi,pfcp_Stats_handler);
         printf("-----sts=%d\n---", sts);
    if(sts)
    {
        //pfcp_Stats_handler(ppi);
    }
    else
    {
         printf("failed -----sts=%d--\n", sts);
    }
      display(ppi);
   //udp_server_send_msg(server_conn,ppi);


    free(server_conn);
    free(ppi);
   // close(sockfd);

    return 0;
}