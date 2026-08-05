
// server program for udp connection
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#pragma pack(4)

#define PORT 7999
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

/*
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
*/

typedef struct panel
{
	int elements;
	int spare;
	
	struct {
		int id;
		int value;
	} idv[100];
}panel_t;



typedef struct udp_client_connect
{
    int sockfd;
    struct sockaddr_in servaddr;
}udp_client_t;

typedef struct {
    udp_client_t *udpclient;
    panel_t *pfcp;
    int interval_seconds;  // how often to run
} thread_args_t;


void udp_send_msgto_server(udp_client_t * udpclient,panel_t *pfcp);

void intialize(struct panel *ppi)
{
	struct panel *p = ppi;
	if(!p)
	{
		perror("ppi memory allooction  failed");
        exit(EXIT_FAILURE);
	}
	 // Zero all members
    memset(p, 0, sizeof(panel_t));
	  p->elements=10;
	  p->idv[0].id=50;
	  p->idv[1].id=51;
	  p->idv[2].id=52;
	  p->idv[3].id=53;
	  p->idv[4].id=54;
	  p->idv[5].id=55;
	  p->idv[6].id=56;
	  p->idv[7].id=57;
	  p->idv[8].id=10000;
	  p->idv[9].id=10100;
	  
	  
	  
	  
	  
}

void display(struct panel *ppi)
{
    //printf("--display -- sea=%ld\n",ppi->sea_total);
   // printf("--display -- sma=%ld\n",ppi->sma_total);
}

//int i=0;
int pfcp_perf_incerment(struct panel *p)
{
	static int count=0;
      // if(  ppi->dataype[0] == PFCP_SESSION_ESTABLISHMENT_REQUEST)
      //{
         // printf("pfcp->ser_total=%ld\n",ppi->ser_total);
         // ppi->ser_last=5;
         // ppi->ser_total =5+ppi->ser_last;
    //  } 
	  if( p->idv[0].id == PFCP_SESSION_ESTABLISHMENT_REQUEST)
		{
         //printf("pfcp->ser_total=%ld\n",ppi->value[0]);
          p->idv[0].value=1;
        // ppi->ser_total =5+ppi->ser_last;
		} 
		
      if( p->idv[1].id == PFCP_SESSION_ESTABLISHMENT_RESPONSE)
      {
          //printf("pfcp->ser_total=%ld\n",ppi->sea_last);
           p->idv[1].value=2;
          //ppi->ser_total =5+ppi->sea_last;
      }
      if(  p->idv[2].id == PFCP_SESSION_MODIFICATION_REQUEST)
      {
          //printf("pfcp->ser_total=%ld\n", ppi->smr_last);
           p->idv[2].value=2;
          //ppi->ser_total =5+ppi->smr_last;
      }
      if(p->idv[3].id==PFCP_SESSION_MODIFICATION_RESPONSE)
      {
        //printf("pfcp->ser_total=%ld\n", ppi->sma_last);
		if(count <3)
		{
        p->idv[3].value=1+ p->idv[3].value;
		}
        //ppi->smr_total =5+ppi->sma_last;
      }
      if( p->idv[4].id==PFCP_SESSION_DELETION_REQUEST)
      {
        // printf("pfcp->ser_total=%ld\n", ppi->sdr_last);
        if(count <10)
		{
        p->idv[4].value=5;
		count++;
		}
        // ppi->sdr_total =5+ppi->sdr_last;
      }
      if( p->idv[5].id==PFCP_SESSION_DELETION_RESPONSE)
      {
        //printf("pfcp->ser_total=%ld\n",ppi->sda_last);
         p->idv[5].value=5;
        //ppi->sdr_total =5+ppi->sda_last;
      }
      if( p->idv[6].id==PFCP_SESSION_REPORT_REQUEST)
      {
        //printf("pfcp->ser_total=%ld\n",ppi->sur_last);
         p->idv[6].value=6;
        // ppi->sdr_total =5+ppi->sur_last;
      }
      if( p->idv[7].id==PFCP_SESSION_REPORT_RESPONSE)
      {
        //printf("pfcp->ser_total=%ld\n",ppi->sua_last);
         p->idv[7].value=7;
        // ppi->sdr_total =5+ppi->sua_last;
      }
	
	if( p->idv[8].id==10000)
      {
        //printf("pfcp->10000=%ld\n",ppi->value[8]);
          p->idv[8].value=8;
        // ppi->sdr_total =5+ppi->sua_last;
      }
	  if(p->idv[9].id==10100)
      {
        //printf("pfcp->101000=%ld\n",ppi->value[9]);
         p->idv[9].value=9;
        // ppi->sdr_total =5+ppi->sua_last;
      }
      else{
        printf("-----next\n");
        
      }
   // i+=5;
    //printf("buffer size=%ld\n",sizeof(struct panel));
    //display(ppi);
    
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
   
  ///uint8_t *buffer= (uint8_t *)pfcp;

    size_t len = sizeof(pfcp);
   printf("--len=%ld:%d\n",len,(pfcp->elements + 1) * 8);
  //  sendto(udpclient->sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
    int bytes_sent = sendto(udpclient->sockfd,pfcp, (pfcp->elements + 1) * 8, MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
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
    //uint8_t *buffer;
    socklen_t len;
    len = sizeof(udpclient->servaddr);
    int n = recvfrom(udpclient->sockfd,pfcp, sizeof(panel_t), MSG_WAITALL, (struct sockaddr *)&udpclient->servaddr, &len);
   // buffer[n] = '\0';
   // struct panel *pfcp1 = (struct panel *)buffer;
      printf("buffer=%d\n",pfcp->idv[5].value);
    printf("RECVF = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
    //printf("Server : %s\n", buffer);
    // pfcp_Stats_handler(pfcp);

}

void *thread_message_function(void *args)
{
    //const int interval_seconds = 3;
	thread_args_t *msgs = (thread_args_t *)args;
	udp_client_t * udpmsg = msgs->udpclient;
	panel_t *ppi = msgs->pfcp;
	
	
	time_t start_time = time(NULL);
    time_t last_call_time = start_time;
    while(1) 
    {
        time_t current_time = time(NULL);
        
        if(difftime(current_time,last_call_time) >= msgs->interval_seconds)
        {
            printf("start_time=%ld:%ld  ",start_time,current_time);
            udp_send_msgto_server(udpmsg,ppi);
            last_call_time = current_time;
        }
        usleep(100000); // sleep 0.1 seconds
    }
}

int  requestperseconds(udp_client_t * udpclient,panel_t *pfcp)
{	
	pthread_t thread; 
	int ret;
	
	thread_args_t *args =(thread_args_t *)malloc(sizeof(thread_args_t));
	args->udpclient = udpclient;
	args->pfcp = pfcp;
	args->interval_seconds = 10;
	printf("------------1111sts=%d-------\n",ret);
	ret = pthread_create(&thread, NULL, thread_message_function, (void*) args);
	printf("------------1111ret=%d-------\n",ret);
	
    return 0;
	
}
// Driver code
int main()
{

	
  udp_client_t * udpclient=(udp_client_t*)malloc(sizeof(udp_client_t));

  panel_t *pfcp =(struct panel*)malloc(sizeof(struct panel));
  intialize(pfcp);
   int sts = udp_client_conn(udpclient);
  
    if(sts)
    {    
        printf("------------sts=%d-------\n",sts);
		int ret=requestperseconds(udpclient,pfcp);
       // udp_send_msgto_server(udpclient,pfcp);
       while(1)
       {
		   pfcp_perf_incerment(pfcp);
          //pfcp_perf_incerment(pfcp);
          //sleep(6);
          //printf("\nsleeptime=--\n");
       }

    }
    //udp_recv_msgby_server(udpclient,pfcp);

    free(pfcp);
    //close(listenfd);
}

