#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>


#include "eng.h"
#include "app.h"
#include "app_stack.h"
#include "jansson.h"
#include "pfcp_stack.h"
#include "app_endpoint.h"
#include "pfcp_stack.h"
#include "app_command.h"
#include "pfcp_metric.h" 

#define PORT 8805
#define MAXLINE 1024

void successful_task_request(  const char *method,int data);
void successful_task_responce(  const char *method,int data);

void metrics_prometheus(char * ipv4,int port); 
typedef struct panel
{
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


int app_config__get_int( json_t * json_config, char * key, int defval);
char * app_config__get_str( json_t * json_config, char * key);
void app_ep__init();


/*
int app_config__get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}


char * app_config__get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}*/




void upfcmd__udp_client_message( uint8_t * Data, int tIndex)
{
}












//cc -o prog prog.c -ljansson
//int app_promethes_perf_stats()
//gcc -g3 upf_stats.c ../dplane/app_stack.c ../dplane/app_endpoint.c -I../dplane -o upfstats -lpthread -ljansson
// gcc -g3 upf_stats.c pfcp_metric.c ../dplane/app_stack.c ../dplane/app_endpoint.c -I../dplane -o upfstats -lmicrohttpd -lpromhttp -lprom -lpthread -ljansson -DENABLE_PROMETHEUS
/*
int app_promethes_perf_stats()
{
    json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( "upf_stats.json", 0, &error);
			
    panel_t *buffer;
		if( json_config)
		{
			json_t * sessionInfoObj = json_object_get( json_config, "UPFSTATS");


			if( sessionInfoObj)
			{
				char * ueIP 				= app_config__get_str( sessionInfoObj, "IP");
				int port					= app_config__get_int( sessionInfoObj, "Port", 2);			
				
				printf("UPF stats ueip=%s port=%d\n", ueIP, port);
				
				
				app_ep__init();
				app_ep_udp_client_t * udpclient = app_ep__create_udpv4_client( ueIP,  port,1024, 1, upfcmd__udp_client_message);
				
				int sts = app_ep__udp_send_by_client( udpclient,(char *)&buffer, sizeof(panel_t));
				
				//metrics_prometheus("192.168.144.11", 9521 );
				printf("upf_stats: successful .. sts=%d\n", sts);
            }
		}
		else
		{
			printf("error in json  file at line=%d column=%d position=%d|||%s|%d\n", error.line, error.column, error.position,__FILE__,__LINE__);
			exit(0);
		}
	
    return 0;

}*/



typedef struct udp_client_connect
{
    int sockfd;
    struct sockaddr_in servaddr;
}udp_client_t;

typedef struct pfcpmsg
{
    uint8_t type ;
    char message[30];// = "Hello Server from Client!";
}pfcpmsg_t;



udp_client_t * udpclient= NULL;

void pfcp_Stats_handler(int msgtype)
{
    
    //printf("inside s_handler=%d\n",msgtype);
    switch(msgtype)
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

    }
}



//udp_client_t * udpclient = NULL;
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
    udpclient->servaddr.sin_addr.s_addr = inet_addr("192.168.144.15"); // Server IP address 



        printf("connected = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
     // Send message to server
    //sendto(udpclient->sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
   // printf("sendto = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
   // printf("Message sent to server.\n");
    return 1;
}

void udp_send_msgto_server(udp_client_t * udpclient)
{
    pfcpmsg_t *pfcp =(struct pfcpmsg*)malloc(sizeof(struct pfcpmsg));

    strcpy(pfcp->message,"Hello Server from Client!");
    pfcp->type=1;
   size_t len = sizeof(struct pfcpmsg);
   printf("--len=%ld\n",len);
  //  sendto(udpclient->sockfd, (const char *)message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
    int bytes_sent = sendto(udpclient->sockfd, (const void*)pfcp, len, MSG_CONFIRM, (const struct sockaddr *)&udpclient->servaddr, sizeof(udpclient->servaddr));
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

void udp_recv_msgby_server(udp_client_t * udpclient)
{
    // Receive response from server
     char buffer[MAXLINE];
    socklen_t len;
    len = sizeof(udpclient->servaddr);
    int n = recvfrom(udpclient->sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&udpclient->servaddr, &len);
    buffer[n] = '\0';
    printf("RECVF = %s:%d\n", inet_ntoa(udpclient->servaddr.sin_addr), ntohs(udpclient->servaddr.sin_port));
    printf("Server : %s\n", buffer);

}



int app_promethes_perf_stats() {

   // metrics_prometheus("192.168.144.11", 9521 );
   
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
   udp_client_t * udpclient=(udp_client_t*)malloc(sizeof(udp_client_t));
   int sts = udp_client_conn(udpclient);
    if(sts)
    {    
        printf("------------sts=%d-------\n",sts);
        udp_send_msgto_server(udpclient);
    }
    udp_recv_msgby_server(udpclient);

   // close(sockfd);

    return 0;
}