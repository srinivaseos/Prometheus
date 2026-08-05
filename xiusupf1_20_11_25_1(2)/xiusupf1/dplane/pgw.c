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

#include "app.h"
#include "app_stack.h"
#include "jansson.h"
#include "pfcp_stack.h"
#include "app_endpoint.h"
#include "pfcp_stack.h"
#include "app_command.h"



typedef struct pgw_session
{
	struct pgw_session * Next;
	
	uint64_t imsi;
	uint64_t imei;
	
	uint32_t ue_ipv4;
	uint64_t ue_ipv6;
	
	struct 
	{
		uint64_t allocatedQuota;
		uint64_t usedQuota;
		uint64_t allocatedQuotaTotal;
		uint64_t usedQuotaTotal;
		uint32_t RGId;
		uint32_t IsActive;
	} RG[10];
	
	unsigned char chf_session_id[100];
	unsigned char pcrf_session_id[100];
	uint64_t pfcp_pgw_seid;
	uint64_t pfcp_sgw_seid;
	uint64_t pfcp_upf_seid;
	uint32_t pfcp_index;			//upf_instance
} pgw_session_t;


typedef struct ueip_s ueip_t;

typedef struct ueip_address
{
	struct ueip_address * next;
	uint32_t ueip;
	ueip_t * parent;
} ueip_address_t;

typedef struct ueip_s
{
	struct ueip_s * next;
	char plmn[4];
	ueip_address_t * head;
	ueip_address_t * current;
	int count;
	pthread_mutex_t lock;
} ueip_t;

typedef struct xiuspgw
{
	json_t * config;
	
	struct 
	{
		char ipv4[20];
		char ipv6[50];
		int port;
		int fd;
	} gtp_server;

	struct 
	{
		char ipv4[20];
		char ipv6[50];
		int port;
		int fd;
	} pfcp_server;

	struct 
	{
		char ipv4[20];
		char ipv6[50];
		int port;
		int fd;
	} pfcp_client[10];
	int pfcp_client_count;
	
	int MaxSessions;
	
	ueip_t * ueip_head;
	ueip_t * ueip_current;
	int ueip_count;
	pthread_mutex_t lock;
	
	pgw_session_t * session_head;
	pgw_session_t * session_current;
	pthread_mutex_t session_lock;
	int session_total;
	int session_available;

	app_data_region_t * app_mregion;
	aht_t * imsit;
} xiuspgw_t;

xiuspgw_t * pgw = NULL;




void app__signalhandler( int signum)
{
	switch( signum)
	{	
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
		{
			
		}
		break;
	}
	
	int secs = 5;
	printf("\nCaught signal %d, coming out...in %d seconds.\n", signum, secs);
	sleep(secs);
	exit(0);
}

int  dpdk_session__ipv4_count()
{
	return 0;
}

int  dpdk_session__ipv4_add(  uint32_t ipv4, void *  data, int deleteIfExists)
{
	return 0;
}



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
}

void app_config__set_str( json_t * json_config, char * key, char * vItem, int maxlen)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		int kv = json_string_length( jObj);
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


void pgw_config__gtp_server()
{
	json_t * gtpServer = json_object_get( pgw->config, "GTPServer");
	
	if( gtpServer)
	{
		app_config__set_str( gtpServer, "IPv4", pgw->gtp_server.ipv4, 20);
		app_config__set_str( gtpServer, "IPv6", pgw->gtp_server.ipv6, 50);
		
		pgw->gtp_server.port = app_config__get_int( gtpServer, "Port", 2152);
		
		printf("GTPv2 Server: IPv4:%s IPv6:%s Port:%d\n", pgw->gtp_server.ipv4, pgw->gtp_server.ipv6, pgw->gtp_server.port);
	}
}

void pgw_config__pfcp_server()
{
	json_t * pfcpServer = json_object_get( pgw->config, "PFCPServer");
	
	if( pfcpServer)
	{
		app_config__set_str( pfcpServer, "IPv4", pgw->pfcp_server.ipv4, 20);
		app_config__set_str( pfcpServer, "IPv6", pgw->pfcp_server.ipv6, 50);
		
		pgw->pfcp_server.port = app_config__get_int( pfcpServer, "Port", 8805);
		
		printf("PFCP Server: IPv4:%s IPv6:%s Port:%d\n", pgw->pfcp_server.ipv4, pgw->pfcp_server.ipv6, pgw->pfcp_server.port);
	}	
}

void pgw_config__pfcp_clients()
{
	json_t * pfcpClients = json_object_get( pgw->config, "PFCPUserPlane");

	if( pfcpClients)
	{
		json_t * pfcpClient = NULL;
		int isize = json_array_size( pfcpClients);
		int i = 0;
		
		for( i = 0; i < isize; i++)
		{
			pfcpClient = json_array_get( pfcpClients, i);
			
			if( pfcpClient)
			{
				app_config__set_str( pfcpClient, "IPv4", pgw->pfcp_client[pgw->pfcp_client_count].ipv4, 20);
				app_config__set_str( pfcpClient, "IPv6", pgw->pfcp_client[pgw->pfcp_client_count].ipv6, 50);
				pgw->pfcp_client[pgw->pfcp_client_count].port = app_config__get_int( pfcpClient, "Port", 8805);
				
				printf("PFCPUserPlane[%d]: IPv4:%s IPv6:%s Port:%d\n", 
					pgw->pfcp_client_count,
					pgw->pfcp_client[pgw->pfcp_client_count].ipv4, 
					pgw->pfcp_client[pgw->pfcp_client_count].ipv6, 
					pgw->pfcp_client[pgw->pfcp_client_count].port
				);
				
				pgw->pfcp_client_count++;
			}
		}
	}
}

void pgw_config__buffers()
{
	pgw->app_mregion = app_region__create();
	json_t * buffers = json_object_get( pgw->config, "Buffers");
	
	if( buffers)
	{
		if( json_is_array( buffers))
		{
			json_t * bobj = NULL;
			int bCount = json_array_size( buffers);
			int i = 0;
			
			char * key = NULL;
			int Count = 0;
			int Size = 0;
			
			for( i = 0; i < bCount; i++)
			{
				bobj = json_array_get( buffers, i);
				key = app_config__get_str( bobj, "Name");
				Count = app_config__get_int( bobj, "Count", 0);
				Size = app_config__get_int( bobj, "Size", 0);
				
				//printf("Key=%s Count=%d\n", key, Count);
				
				if( Size > 0 && Count > 0)
				{
					app_region__add_pool( pgw->app_mregion, key, Size, Count);
				}
			}
		}
	}	
}

pgw_session_t * pgw_session__allocate()
{
	pgw_session_t * sess = NULL;
	pthread_mutex_lock( &pgw->session_lock);
	
	if(pgw->session_head)
	{
		sess = pgw->session_head;
		pgw->session_head = sess->Next;
		sess->Next = NULL;
		pgw->session_available--;
	}

	pthread_mutex_unlock( &pgw->session_lock);
	return sess;
}

void pgw_session__free( pgw_session_t * sess)
{
	pthread_mutex_lock( &pgw->session_lock);
	
	if(!pgw->session_head) 
	{
		pgw->session_head = pgw->session_current = sess;
	} 
	else 
	{
		pgw->session_current->Next = sess;
		pgw->session_current = sess;
	}
	pgw->session_available++;
	
	pthread_mutex_unlock( &pgw->session_lock);
}

void pgw_init( json_t * json_config)
{
	if(!pgw)
	{
		pgw = (xiuspgw_t*)malloc(sizeof(xiuspgw_t));
		memset( pgw, 0, sizeof(xiuspgw_t));
		pgw->config = json_config;

		pgw->session_head = NULL;
		pgw->session_current = NULL;
		pthread_mutex_init( &pgw->session_lock, NULL);
		pgw->session_total = 0;
		pgw->session_available = 0;

		pgw_config__gtp_server();
		pgw_config__pfcp_server();
		pgw_config__pfcp_clients();
		pgw->MaxSessions = app_config__get_int( pgw->config, "MaxSessions", 50000);
		pgw_config__buffers();
		
		printf("Server MaxSessions: %d\n", pgw->MaxSessions);

		pgw_session_t * sess = NULL;
		uint8_t * mPtr = (uint8_t *)malloc( sizeof(pgw_session_t) * pgw->MaxSessions);
		memset( mPtr, 0, sizeof(pgw_session_t) * pgw->MaxSessions);
		
		int i = 0;
		for( i = 0; i < pgw->MaxSessions; i++)
		{
			sess = (pgw_session_t *)mPtr;
			
			if(!pgw->session_head) 
			{
				pgw->session_head = pgw->session_current = sess;
			} 
			else 
			{
				pgw->session_current->Next = sess;
				pgw->session_current = sess;
			}
			
			sess += sizeof(pgw_session_t);
		}
		
		sess = NULL;
		
		pgw->session_total = pgw->MaxSessions;
		pgw->session_available = pgw->MaxSessions;
	}
}


int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./dpnc.json\n", argv[0]);
		exit(0);
	}
	
	signal( SIGINT, app__signalhandler);		// Ctrl + C
	signal( SIGQUIT, app__signalhandler);		// Ctrl + \ 			//
	signal( SIGTERM, app__signalhandler);		// shell command kill generates SIGTERM by default

	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( argv[1], 0, &error);
	
	if(!json_config)
	{
		printf("error in json %s file at line=%d column=%d position=%d\n", argv[1], error.line, error.column, error.position);
		exit(0);
	}
	
	pgw_init( json_config);

	while(1) 
	{
		usleep( 999999);
	}
	
	return 0;
}





