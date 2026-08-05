#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jansson.h>

#pragma pack(4)

#include "pfcp_metric.h" 

//#define PORT 7999
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
#define PFCP_SESSION_CPKT                                                   (10000)
#define PFCP_SESSION_PENDING_PACKETS                                        (10100)

void metrics_prometheus(char * ipv4,int port); 
void successful_task_request(  const char *method,int data,double count);
void successful_task_responce(  const char *method,int data,double count); 
void packets_proccesed_promo(  const char *method,int data,double count);

typedef struct server
{
    int listenfd;
	char IPv4[20];
	int Port;
    struct sockaddr_in servaddr, cliaddr;
}server_t;


typedef struct app_promo
{
    char pfcp_IPv4[100];
	int pfcp_Port;
    int promotheus_Enabled;
}app_promo_t;

app_promo_t *upf_stats=NULL;


typedef struct pfcpmsg
{
    uint8_t type ;
    char message[30];// = "Hello Server from Client!";
}pfcpmsg_t;

/*typedef struct panel
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



typedef void pfcp_Stats_handler_t( panel_t * panel);//(struct panel *msgtype);



int app_config__get_int( json_t * json_config, char * key, int defval)
{
    //printf("defval=%d key=%s\n",defval,key);
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

const char * app_config__get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}



void app_config__set_str( json_t * json_config, char * key, char * vItem, int maxlen)
{
    //printf("defval=%s key=%s\n",vItem,key);
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		int kv = json_string_length( jObj)+1;
		const char * v = json_string_value( jObj);
		if( kv < maxlen)
		{
			memcpy( vItem, v, kv);
		}
		else
		{
			memcpy( vItem, v, maxlen);
		}
	}
}

server_t *server_conn=NULL;

void display(panel_t * panel)
{
   // printf("--display -- sea=%ld\n",pfcp->value[0]);
    //printf("--display -- sma=%ld\n",pfcp->value[0]);
}


void  pfcp_Stats_handler(panel_t * panel)//(struct panel *pfcp)
{
    //struct panel *pfcp = (struct panel *)buffer;
    int i=0;
    //printf("inside s_handler=%d\n",panel->elements);
	 
    while(i<panel->elements)
    {
		switch( panel->idv[i].id)
		{
			case PFCP_SESSION_ESTABLISHMENT_REQUEST:
				successful_task_request( "POST",1, panel->idv[i].value);
				break;
			case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
				successful_task_responce( "POST",1,panel->idv[i].value);    
				break;
			case PFCP_SESSION_MODIFICATION_REQUEST:
				successful_task_request( "POST",2, panel->idv[i].value);
				break;
			case PFCP_SESSION_MODIFICATION_RESPONSE:
				successful_task_responce( "POST",2, panel->idv[i].value);
				break;
			case PFCP_SESSION_DELETION_REQUEST:
				successful_task_request( "POST",3, panel->idv[i].value);                                         
				break;
			case PFCP_SESSION_DELETION_RESPONSE:
				successful_task_responce( "POST",3,panel->idv[i].value);    
				break;
			  case PFCP_SESSION_REPORT_REQUEST:
				successful_task_request( "POST",4,panel->idv[i].value);
				break;
			case PFCP_SESSION_REPORT_RESPONSE:
				successful_task_responce( "POST",4,panel->idv[i].value);
				break;
			case PFCP_SESSION_CPKT:
				//printf("panel->cpkt_total=%d\n", panel->idv[i].value); 
				successful_task_request( "POST",5, panel->idv[i].value);
				break;
			case PFCP_SESSION_PENDING_PACKETS:
				//printf("panel->total_total=%d\n",panel->idv[i].value);
				successful_task_request( "POST",6,panel->idv[i].value);
				break;
			default :
					printf("not valind=%d\n",panel->idv[i].value);
					break;
		}
    //display(buffer);
    //printf("increase\n");
    i++;
    }
}



int  udp_server_send_msg(struct server *server_conn,struct panel *ppi)
{
    uint8_t *buffer;
    ssize_t len=sizeof(buffer);
   int sts = sendto(server_conn->listenfd, (const void *)buffer, len, 0,(struct sockaddr*)&server_conn->cliaddr, sizeof(server_conn->cliaddr));
   printf("sento --  sts=%d\n",sts);
}

int udp_recv_from_msg(struct server *server_conn,struct panel *ppi,pfcp_Stats_handler_t cb)
{
   panel_t panel;
   memset( &panel, 0, sizeof(panel_t));
    int listenfd;
    
    //receive the datagram
    socklen_t len = sizeof(server_conn->cliaddr);
    int n = recvfrom( server_conn->listenfd,&panel, sizeof(panel_t),  0, (struct sockaddr*)&server_conn->cliaddr, &len); //receive message from serv
    if(n > 0)
    {
           //printf("----sda=%ld:%d:%d:%d\n",pfcp->sea_total,n,pfcp->dataype[0],pfcp->dataype[1]);
         
    }
    else
    {
        //printf("n=%d\n",n);

        printf("error client2 = %s:%d:%d\n", inet_ntoa(server_conn->cliaddr.sin_addr),ntohs(server_conn->cliaddr.sin_port),server_conn->cliaddr.sin_port);
        perror("recv");
        exit(-1);
    }
  
      printf("client2 = %s:%d:%d\n", inet_ntoa(server_conn->cliaddr.sin_addr),ntohs(server_conn->cliaddr.sin_port),ntohs(server_conn->servaddr.sin_port));
      cb(&panel);
      //sleep(100);
      //printf("retunr to function\n");
     //free(buffer);
    return 1; 
}

void udp_Server_connection(struct server *server_conn)
{
     
    //bzero(&server_conn->servaddr, sizeof(server_conn->servaddr));

    // Create a UDP Socket
	int iPort = server_conn->Port;
    server_conn->listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if( server_conn->listenfd <= 0)
	{
		printf("UDP Host Server Socket Creation Failed (Host Process) for Port=%d\n", iPort);
		exit(1);
	}
	server_conn->servaddr.sin_port = htons(iPort);
    server_conn->servaddr.sin_family = AF_INET;
	if( strlen( server_conn->IPv4) == 0)
	{
		server_conn->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		server_conn->servaddr.sin_addr.s_addr = inet_addr( server_conn->IPv4);
	}
    //server_conn->servaddr.sin_addr.s_addr = INADDR_ANY;// inet_addr("192.168.144.12");// htonl(INADDR_ANY);
	
    

	
	
    // bind server address to socket descriptor
    if(bind(server_conn->listenfd, (struct sockaddr*)&server_conn->servaddr, sizeof(server_conn->servaddr)) != -1)
    {
         printf("connection client = %s:%d\n", inet_ntoa(server_conn->servaddr.sin_addr), ntohs(server_conn->servaddr.sin_port));

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
   printf("connecting\n");
}


int main(int argc,char *argv[]) {
	
	json_error_t error;
 	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( argv[1], 0, &error);
	//json_t * json_config = json_load_file( config, 0, &error); //fro object .so file
	if(!json_config)
	{
		printf("error in json  file at line=%d column=%d position=%d\n", error.line, error.column, error.position);
		exit(0);
	}
    else
    {
        printf("json_config=%s\n",(char*)json_config);
    }
    upf_stats=(app_promo_t*)malloc(sizeof(app_promo_t));
    json_t * metricsObj = json_object_get( json_config, "Metrics");
	
	if( metricsObj)
	{
        //printf("finded metricsObj\n");
        upf_stats->promotheus_Enabled = app_config__get_int( metricsObj, "Enable", 0);
        printf("upf_stats->promotheus_Enabled=%d\n",upf_stats->promotheus_Enabled);
        if(upf_stats->promotheus_Enabled ==1)
        {
       
        app_config__set_str( metricsObj, "IPv4", upf_stats->pfcp_IPv4, 20);
        upf_stats->pfcp_Port = app_config__get_int( metricsObj, "Port", 9521);	
        }
    }

    printf("upf_stats->pfcp_IPv4=%s,upf_stats->pfcp_Port=%d\n",upf_stats->pfcp_IPv4,upf_stats->pfcp_Port  );

    //metrics_prometheus("192.168.144.51", 9521 );
    metrics_prometheus(upf_stats->pfcp_IPv4, upf_stats->pfcp_Port );
  
   server_t *server_conn=(struct server*)malloc(sizeof(struct server));
	json_t * pfcpobj = json_object_get( json_config, "Promo");
	
	if( pfcpobj)
	{
        printf("finded pfcpobj\n");
        //printf("server_conn->promotheus_Enabled=%d\n",server_conn->promotheus_Enabled);
        app_config__set_str( pfcpobj, "IPv4", server_conn->IPv4, 20);
        server_conn->Port = app_config__get_int( pfcpobj, "Port", 0);	
        
    }

   udp_Server_connection(server_conn);
   
   panel_t  *ppi  = (struct panel*)malloc(sizeof(struct panel));
     
    while(1)
    {
        int sts = udp_recv_from_msg(server_conn,ppi,pfcp_Stats_handler);
        //printf("-----sts=%d\n---", sts);
    }
    if(1)
    {
        //pfcp_Stats_handler(ppi);
    }
    else
    {
        //printf("failed -----sts=%d--\n", 1);
    }
	
   //udp_server_send_msg(server_conn,ppi);


    free(server_conn);
    free(ppi);
   // close(sockfd);

    return 0;
}
