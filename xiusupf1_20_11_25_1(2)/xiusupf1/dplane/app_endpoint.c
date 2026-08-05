#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <resolv.h>
#include <ifaddrs.h>

#include "app_stack.h"
#include "app_endpoint.h"

typedef struct app_ep_udp_message
{
	uint16_t len;
	uint16_t pos;
	int ipv;
	int fd;
	union {
		struct sockaddr_in clientaddr;
		struct sockaddr_in6 clientaddr6;
	} u;
	
	uint8_t * buffer;
	app_ep_udp_client_t * client;
	
} app_ep_udp_message_t;


void app_ep__set_udp_message( app_ep_udp_message_t ** msg, uint8_t * buffer, uint16_t llen)
{
	app_ep_udp_message_t msg1;

	memset( &msg1, 0, sizeof(app_ep_udp_message_t));
	msg1.buffer = buffer;
	msg1.len = llen;
	msg1.pos = 0;

	*msg = &msg1;
}


typedef struct app_ep_stack
{
	int total_server_count;
	int total_client_count;

	uint64_t SendCount;
	uint64_t RecvCount;
	uint64_t DropCount;
	uint64_t LastSendCount;
	uint64_t LastRecvCount;	
	uint64_t LastDropCount;	
	pthread_mutex_t SendCountLock;
	pthread_mutex_t RecvCountLock;	
	pthread_mutex_t DropCountLock;	

	app_ep_udp_server_t * shead;
	app_ep_udp_server_t * scurrent;
	
	app_ep_tcp_server_t * thead;
	app_ep_tcp_server_t * tcurrent;	

	app_data_region_t * app_mregion;
	app_data_pool_t * udp_pool1;
	app_data_pool_t * udp_pool2;
	int udpBufferSize;
	int udp_capacity;
} app_ep_stack_t;

app_ep_stack_t * ep_stack = NULL;

uint16_t app_ep__get_len( app_ep_udp_message_t * udp_m)
{
	return udp_m->len;
}

uint8_t * app_ep__get_buffer( app_ep_udp_message_t * udp_m)
{
	return udp_m->buffer;
}

int app_ep__get_ipv( app_ep_udp_message_t * udp_m)
{
	return udp_m->ipv;
}

int app_ep__get_fd( app_ep_udp_message_t * udp_m)
{
	return udp_m->fd;
}

struct sockaddr_in * app_ep__get_ip4addr( app_ep_udp_message_t * udp_m)
{
	return &udp_m->u.clientaddr;
}

struct sockaddr_in6 * app_ep__get_ip6addr( app_ep_udp_message_t * udp_m)
{
	return &udp_m->u.clientaddr6;
}

uint8_t * app_ep__get_u( app_ep_udp_message_t * udp_m)
{
	return (uint8_t *)&udp_m->u;
}

uint32_t app_ep__u32ip( char * ip)
{
	return inet_addr( ip);
}

void app_ep__get_ipaddr( app_ep_udp_message_t * udp_m, int * fd, uint8_t ** addr, int * len)
{
	*fd = udp_m->fd;
	
	if( udp_m->ipv == 4)
	{
		*addr = (uint8_t*)&udp_m->u.clientaddr;
		*len = sizeof(struct sockaddr_in);
	}
	else
	{	
		*addr = (uint8_t*)&udp_m->u.clientaddr6;
		*len = sizeof(struct sockaddr_in6);
	}
}



void app_ep__get_strip( app_ep_udp_message_t * udp_m, char * data)
{
	memset( data, 0, 100);
	if( udp_m->ipv == 4)
	{
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &udp_m->u.clientaddr.sin_addr, str, INET_ADDRSTRLEN);
		sprintf( data, "%s:%d", str, ntohs( udp_m->u.clientaddr.sin_port));		 
	}
	else
	{	
		char str[INET6_ADDRSTRLEN];
		inet_ntop( AF_INET6, &udp_m->u.clientaddr6.sin6_addr, str, INET6_ADDRSTRLEN);
		sprintf( data, "%s:%d", str, ntohs( udp_m->u.clientaddr6.sin6_port));
	}
}

void app_ep__get_str_ipv4( uint32_t ip, char * data)
{
	//char str[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, (char *)&ip, data, INET_ADDRSTRLEN);
}

void app_ep__get_str_ipv6( char * ipv6, char * data)
{
	//char str[INET_ADDRSTRLEN];
	inet_ntop( AF_INET6, ipv6, data, INET_ADDRSTRLEN);
}

uint32_t app_ep__get_u32_ipv4( char * ip)
{
	return inet_addr( ip);
}

void app_ep__perflog( app_logger_t * logger, int log_buffers)
{
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "UDP     Servers %-6u    Clients %-6u", ep_stack->total_server_count, ep_stack->total_client_count);

	uint64_t SendCount = ep_stack->SendCount - ep_stack->LastSendCount;
	uint64_t RecvCount = ep_stack->RecvCount - ep_stack->LastRecvCount;

	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "UDP     Sent   %-6u    Recv   %-6u    TSent %-10u    TRecv %-10u", 
		SendCount, RecvCount, ep_stack->SendCount, ep_stack->RecvCount);

	ep_stack->LastSendCount = ep_stack->SendCount;
	ep_stack->LastRecvCount = ep_stack->RecvCount;
	
	app_ep_udp_server_t * udp_server = ep_stack->shead;
	
	while( udp_server)
	{
		if(udp_server->queue)
		{
			app_queue__logstats( logger, udp_server->queue);
		}

		udp_server = udp_server->Next;
	}
	
	//if( log_buffers)
	{
		//app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "---------------------------------------------------------------------------------------------");
		app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "UDP Buffers");
		app_region__print_stats( logger, ep_stack->app_mregion);
	}
}


void app_ep__encode__msglen( app_ep_udp_message_t * bObj)
{
	bObj->buffer[2] = ((bObj->len-4) >> 8) & 0xFF;
	bObj->buffer[3] = (bObj->len-4) & 0xFF;	
}

void app_ep__encode__setAt( app_ep_udp_message_t * bObj, uint16_t index, uint16_t val)
{
	bObj->buffer[index + 0] = (val >> 8) & 0xFF;
	bObj->buffer[index + 1] = val & 0xFF;	
}

void app_ep__encode__u8( app_ep_udp_message_t * bObj, uint8_t val)
{
	bObj->buffer[bObj->len + 0] = (val) & 0xFF;
	bObj->len++;
}

void app_ep__encode__u16( app_ep_udp_message_t * bObj, uint16_t val)
{
	bObj->buffer[bObj->len + 0] = (val >> 8) & 0xFF;
	bObj->buffer[bObj->len + 1] = val & 0xFF;	
	
	bObj->len += 2;
}

void app_ep__encode__u24( app_ep_udp_message_t * bObj, uint32_t val)
{
	bObj->buffer[bObj->len + 0] = (val >> 16) & 0xFF;
	bObj->buffer[bObj->len + 1] = (val >> 8) & 0xFF;
	bObj->buffer[bObj->len + 2] = val & 0xFF;	
	
	bObj->len += 3;
}

void app_ep__encode__u32( app_ep_udp_message_t * bObj, uint32_t val)
{
	bObj->buffer[bObj->len + 0] = (val >> 24) & 0xFF;
	bObj->buffer[bObj->len + 1] = (val >> 16) & 0xFF;
	bObj->buffer[bObj->len + 2] = (val >> 8) & 0xFF;
	bObj->buffer[bObj->len + 3] = val & 0xFF;	
	
	bObj->len += 4;
}

void app_ep__encode__u16toc( char * buffer, uint32_t val)
{
	buffer[0] = (val >> 8) & 0xFF;
	buffer[1] = val & 0xFF;	
}

void app_ep__encode__u24toc( char * buffer, uint32_t val)
{
	buffer[0] = (val >> 16) & 0xFF;
	buffer[1] = (val >> 8) & 0xFF;
	buffer[2] = val & 0xFF;	
}

void app_ep__encode__u32toc( char * buffer, uint32_t val)
{
	buffer[0] = (val >> 24) & 0xFF;
	buffer[1] = (val >> 16) & 0xFF;
	buffer[2] = (val >> 8) & 0xFF;
	buffer[3] = val & 0xFF;	
}

void app_ep__encode__u40toc( char * buffer, uint64_t val)
{
	buffer[0] = (val >> 32) & 0xFF;
	buffer[1] = (val >> 24) & 0xFF;
	buffer[2] = (val >> 16) & 0xFF;
	buffer[3] = (val >> 8) & 0xFF;
	buffer[4] = val & 0xFF;	
}

void app_ep__encode__u64toc( char * buffer, uint64_t val)
{
	buffer[0] = (val >> 56) & 0xFF;
	buffer[1] = (val >> 48) & 0xFF;
	buffer[2] = (val >> 40) & 0xFF;
	buffer[3] = (val >> 32) & 0xFF;
	buffer[4] = (val >> 24) & 0xFF;
	buffer[5] = (val >> 16) & 0xFF;
	buffer[6] = (val >> 8) & 0xFF;
	buffer[7] = val & 0xFF;	
}

void app_ep__encode__u64( app_ep_udp_message_t * bObj, uint64_t val)
{
	bObj->buffer[bObj->len + 0] = (val >> 56) & 0xFF;
	bObj->buffer[bObj->len + 1] = (val >> 48) & 0xFF;
	bObj->buffer[bObj->len + 2] = (val >> 40) & 0xFF;
	bObj->buffer[bObj->len + 3] = (val >> 32) & 0xFF;
	bObj->buffer[bObj->len + 4] = (val >> 24) & 0xFF;
	bObj->buffer[bObj->len + 5] = (val >> 16) & 0xFF;
	bObj->buffer[bObj->len + 6] = (val >> 8) & 0xFF;
	bObj->buffer[bObj->len + 7] = val & 0xFF;	
	
	bObj->len += 8;
}

void app_ep__encode__str( app_ep_udp_message_t * bObj, uint8_t * data, uint16_t len)
{
	memcpy( &bObj->buffer[bObj->len], data, len);
	bObj->len += len;
}

uint16_t app_ep__get_u16( unsigned char * buff)
{
	return (uint16_t) (( buff[0] << 8) & 0xFF00) + (buff[1] & 0x00FF);
}

uint32_t app_ep__get_u24( unsigned char * buff)
{
	return (uint32_t) ((buff[0] << 16) & 0x00FF0000) + (( buff[1] << 8) & 0x0000FF00) + (buff[2] & 0x000000FF);
}

uint32_t app_ep__get_u32( unsigned char * buff)
{
	return (uint32_t) (( buff[0] << 24) & 0xFF000000) + ((buff[1] << 16) & 0x00FF0000) + ((buff[2] << 8) & 0x0000FF00) + (buff[3] & 0x000000FF);
}

uint32_t app_ep__get_u32_ntoh( unsigned char * buff)
{
	return (uint32_t) (( buff[3] << 24) & 0xFF000000) + ((buff[2] << 16) & 0x00FF0000) + ((buff[1] << 8) & 0x0000FF00) + (buff[0] & 0x000000FF);
}


uint64_t app_ep__get_u40( unsigned char * buff)
{
	return ((((uint64_t)buff[0] << 32) & 0x000000FF00000000U) + ((buff[1] << 24) & 0x00000000FF000000U) + ((buff[2] << 16) & 0x0000000000FF0000U) + ((buff[3] << 8) & 0x000000000000FF00U) + (buff[4] & 0x00000000000000FFU));
}

uint64_t app_ep__get_u64( unsigned char * buff)
{
	return (
	(((uint64_t)buff[0] << 56) & 0xFF00000000000000U) + (((uint64_t)buff[1] << 48) & 0x00FF000000000000U) + (((uint64_t)buff[2] << 40) & 0x0000FF0000000000U) +
	(((uint64_t)buff[3] << 32) & 0x000000FF00000000U) + ((buff[4] << 24) & 0x00000000FF000000U) + ((buff[5] << 16) & 0x0000000000FF0000U) + ((buff[6] << 8) & 0x000000000000FF00U) 
	+ (buff[7] & 0x00000000000000FFU));
}

uint64_t app_ep__get_u64_ntoh( unsigned char * buff)
{
	return (
	(((uint64_t)buff[7] << 56) & 0xFF00000000000000U) + (((uint64_t)buff[6] << 48) & 0x00FF000000000000U) + (((uint64_t)buff[5] << 40) & 0x0000FF0000000000U) +
	(((uint64_t)buff[4] << 32) & 0x000000FF00000000U) + ((buff[3] << 24) & 0x00000000FF000000U) + ((buff[2] << 16) & 0x0000000000FF0000U) + ((buff[1] << 8) & 0x000000000000FF00U) 
	+ (buff[0] & 0x00000000000000FFU));
}


void app_ep__init()
{
	if(!ep_stack)
	{
		ep_stack = malloc(sizeof( app_ep_stack_t));
		memset( ep_stack, 0, sizeof( app_ep_stack_t));
		
		ep_stack->total_server_count = 0;;
		ep_stack->total_client_count = 0;

		ep_stack->SendCount = 0;
		ep_stack->RecvCount = 0;
		ep_stack->DropCount = 0;
		ep_stack->LastSendCount = 0;
		ep_stack->LastRecvCount = 0;
		ep_stack->LastDropCount = 0;
		
		pthread_mutex_init( &ep_stack->SendCountLock, NULL);
		pthread_mutex_init( &ep_stack->RecvCountLock, NULL);
		pthread_mutex_init( &ep_stack->DropCountLock, NULL);

		ep_stack->shead = NULL;
		ep_stack->scurrent = NULL;
	}
	
	if(!ep_stack->app_mregion)
	{
		ep_stack->app_mregion = app_region__create();
	}	
}


void app_ep__init_udp( int bufferSize, int capacity)
{
	app_ep__init();
	
	if(!ep_stack->udp_pool1)
	{
		ep_stack->udpBufferSize = bufferSize;
		ep_stack->udp_capacity = capacity;
		ep_stack->udp_pool1 = app_region__add_pool( ep_stack->app_mregion, "udpb1", sizeof(app_ep_udp_message_t), capacity);
		ep_stack->udp_pool2 = app_region__add_pool( ep_stack->app_mregion, "udpb2", bufferSize, capacity);
	}
}

app_ep_udp_message_t * app_ep__allocate_udp_message()
{
	uint8_t * bPtr = app_region__allocate_fd( ep_stack->udp_pool1);
	
	if( bPtr)
	{	
		app_ep_udp_message_t * udpPtr = (app_ep_udp_message_t *)(bPtr);
		udpPtr->buffer = app_region__allocate_fd( ep_stack->udp_pool2);
		memset( udpPtr->buffer, 0, ep_stack->udpBufferSize);

		udpPtr->pos = 0;
		udpPtr->len = 0;
		return udpPtr;
	}

	return NULL;
}

void app_ep__free_udp_message( app_ep_udp_message_t * udp_m)
{
	if( udp_m)
	{
		app_region__free( udp_m->buffer);
		udp_m->buffer = NULL;
		app_region__free( (uint8_t *)udp_m);
		udp_m = NULL;
	}	
}

void app_ep__udp_server_start( app_ep_udp_server_t * udp_server)
{
	if( udp_server)
	{
		udp_server->start = 1;
	}	
}

void * app_ep__udp_server_thread( void * args)
{
	app_ep_udp_server_t * udp_server = (app_ep_udp_server_t*)args;
	
	int iPort = udp_server->port;
	int serverSocket = 0;
	struct sockaddr_in cli_addr;
	struct sockaddr_in6 cli_addr6;
	
	if( udp_server->ipv == 4) {	
		serverSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		serverSocket = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	}		
	
	if( serverSocket <= 0)
	{
		printf("UDP Host Server Socket Creation Failed (Host Process) for Port=%d\n", iPort);
		exit(1);
	}

	udp_server->fd = serverSocket;
	printf("start udp server for port %d ipversion=%d fd=%d  (%s|%s|%d) \n", iPort, udp_server->ipv, udp_server->fd, __FUNCTION__, __FILE__, __LINE__);
	
	int yes = 1;
	if( setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		printf("UCP setsockopt SO_REUSEADDR failed\n");
		exit(1);
	}

	int clientlen = sizeof(struct sockaddr_in6);
	
	if( udp_server->ipv == 4)
	{	
		clientlen = sizeof(struct sockaddr_in);
		
		struct sockaddr_in serverAddr;
		memset( &serverAddr, 0, sizeof( struct sockaddr_in));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(iPort);
		
		if( strlen( udp_server->ip) == 0) {
			serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			serverAddr.sin_addr.s_addr = inet_addr( udp_server->ip);
		}
		
		int z = 0;
		
		for( z = 0; z < 4; z++)
		{
			if( bind( serverSocket, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) != 0)
			{
				if( z < 2)
				{
					printf("retrying: Server Bind Failed for UDP-V4 Server Socket = %d Server-IP=%s Port = %d   %s|%s|%d\n", serverSocket, udp_server->ip, iPort, __FILE__, __FUNCTION__, __LINE__);
					printf("------1\n");
					usleep(999999);
				}
				else
				{
					printf("exiting: Server Bind Failed for UDP-V4 Server Socket = %d Server-IP=%s Port = %d   %s|%s|%d\n", serverSocket, udp_server->ip, iPort, __FILE__, __FUNCTION__, __LINE__);
					exit(1);
				}
			}
			else
			{
				printf("---Server Bind Success for UDP-V4 Server Socket = %d Server-IP=%s Port = %d\n", serverSocket, udp_server->ip, iPort);
				break;
			}
		}
		
	}
	else
	{
		struct sockaddr_in6 serverAddr6;
		memset( &serverAddr6, 0, sizeof( struct sockaddr_in6));
		serverAddr6.sin6_family = AF_INET6;
		serverAddr6.sin6_port = htons(iPort);
		serverAddr6.sin6_addr = in6addr_any;

		if( bind( serverSocket, (struct sockaddr *) &serverAddr6, sizeof(struct sockaddr_in6)) != 0)
		{
			perror("bind failed\n");
			printf("Server Bind Failed for UDP-V6 Server Socket = %d Server/Port = %d\n", serverSocket, iPort);
			exit(1);
		}		
	}
	int is = 0;
	int it = 0;
	while( udp_server->start == 0)
	{
		if( is == 0) 
		{
			printf( "udp server with port=%d not started, waiting...flag=%d\n", iPort, udp_server->start);
		}

		is++;
		if( is == 3) 
		{
			is = 0;
			it++;
		}

		usleep( 999999);
		
		if( it > 50)
		{
			printf( "udp server with port=%d not started exiting=%d\n", iPort, udp_server->start);
			exit(0);
		}
	}
	printf("Started udp server with port=%d\n", iPort);
	
	
	struct pollfd fds;	
	fds.fd     = serverSocket;
	fds.events = POLLIN;

	int n = 0, flags = 0, ret;
	
	app_ep_udp_message_t * udpPtr = NULL;
	int i = 0;
	
	while(1)
	{
		
		ret = poll( &fds, 1, 1000);
		
		if( ret <= 0)
		{
			continue;
		}
		
		if( ret > 0)
		{
			udpPtr = app_ep__allocate_udp_message();
			i = 0;
			
			if(!udpPtr)
			{
				while( i < 4)
				{
					usleep( 999999);
					udpPtr = app_ep__allocate_udp_message();
				
					if(udpPtr)
						break;
				
					i++;
				}
			}
			if(udpPtr)
			{
				if( udp_server->ipv == 4)
				{
					udpPtr->ipv = 4;
					udpPtr->len = recvfrom( serverSocket, udpPtr->buffer, udp_server->udp_bufferSize, 0, (struct sockaddr *) &udpPtr->u.clientaddr, &clientlen);
					printf("-------server udpPtr->u.clientaddr=%s:%p:%d\n",inet_ntoa(udpPtr->u.clientaddr.sin_addr),(void *) udpPtr->buffer,udp_server->udp_bufferSize);  
				}
				else
				{
					udpPtr->ipv = 6;
					udpPtr->len = recvfrom( serverSocket, udpPtr->buffer, udp_server->udp_bufferSize, 0, (struct sockaddr *) &udpPtr->u.clientaddr6, &clientlen);
				}
				
				

				if( app_queue__is_limit_reached( udp_server->queue) == 0)
				{
					//printf("queued udp message\n");
					
					udpPtr->fd = udp_server->fd;
					app_queue__enquee( udp_server->queue, (uint8_t *)udpPtr, NULL);
					ep_stack->RecvCount++;
				}
				else
				{
					//printf("droppig udp message\n");
					
					app_queue__dropped( udp_server->queue);
					app_ep__free_udp_message( udpPtr);
					ep_stack->DropCount++;
				}
			}
			else
			{
				printf("Allocation of UDP Buffer failed %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
			}
			
			//printf("Received messaged RecvCount=%ld\n", ep_stack->RecvCount);
		}
	}
	
	return NULL;
}


int app_ep__udp_send( int fd, struct sockaddr * saddr, int slen, char * buff, int len)
{
	int sentBytes = sendto( fd, buff, len, MSG_DONTWAIT, saddr, slen);
	ep_stack->SendCount++;
	return sentBytes;
}


int app_ep__udp_send2( app_ep_udp_server_t * udp_server, struct sockaddr * saddr, int slen, char * buff, int len)
{
	return app_ep__udp_send( udp_server->fd, saddr, slen, buff, len);
}


int app_ep__udp_sendbymsg( app_ep_udp_message_t * req_msg, app_ep_udp_message_t * res_msg)
{
	ep_stack->SendCount++;
	if( req_msg->ipv == 4)
	{
		return sendto( req_msg->fd, res_msg->buffer, res_msg->len, MSG_DONTWAIT, (struct sockaddr *)&req_msg->u.clientaddr, sizeof(struct sockaddr_in));
	}
	else
	{	
		return sendto( req_msg->fd, res_msg->buffer, res_msg->len, MSG_DONTWAIT, (struct sockaddr *)&req_msg->u.clientaddr6, sizeof(struct sockaddr_in6));
	}
}

int app_ep__udp_sendbymsg2( app_ep_udp_message_t * req_msg, unsigned char * buffer, int len)
{
	ep_stack->SendCount++;
	if( req_msg->ipv == 4)
	{
		return sendto( req_msg->fd, buffer, len, MSG_DONTWAIT, (struct sockaddr *)&req_msg->u.clientaddr, sizeof(struct sockaddr_in));
	}
	else
	{	
		return sendto( req_msg->fd, buffer, len, MSG_DONTWAIT, (struct sockaddr *)&req_msg->u.clientaddr6, sizeof(struct sockaddr_in6));
	}
}

int app_ep__udp_sendto_v4( int fd, char * ip, int port, char * buff, int len)
{
	struct sockaddr_in server;
	bzero((char*)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons( port);
	
	return app_ep__udp_send( fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in), buff, len);
}

int app_ep__udp_send_by_client( app_ep_udp_client_t * client, char * buff, int len)
{
	if( client)
	{
		return app_ep__udp_sendto_v4( client->fd, client->ip, client->port, buff, len);
	}
	return -1;
}

int app_ep__udp_send_by_sockaddrv4( int fd, struct sockaddr_in * clientaddr, char * buff, int len)
{
	return app_ep__udp_send( fd, (struct sockaddr *)clientaddr, sizeof(struct sockaddr_in), buff, len);
}



void * app_ep__tcp_server_thread( void * args)
{
	
}


app_ep_tcp_server_t * app_ep__create_tcpv4_server( char * ip, int port, int tcp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb)
{
	app_ep_tcp_server_t * tcp_server = (app_ep_tcp_server_t*)malloc( sizeof(app_ep_tcp_server_t));
	memset( tcp_server, 0, sizeof( app_ep_tcp_server_t));
	
	if(!ep_stack->thead)
	{
		ep_stack->thead = ep_stack->tcurrent = tcp_server;
	}
	else
	{
		ep_stack->tcurrent->Next = tcp_server;
		ep_stack->tcurrent = tcp_server;
	}	

	tcp_server->ipv = 4;
	tcp_server->port = port;
	tcp_server->udp_bufferSize = tcp_bufferSize;
	strcpy( tcp_server->ip, ip);
	tcp_server->queue = app_queue__create( "udpq", ep_stack->udp_capacity, iDecodeThreadCount, cb);
	tcp_server->start = 0;

	int iRet;
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, app_ep__tcp_server_thread, (void *)tcp_server);
	
	if(iRet)
	{
		perror("Error: ");
		printf("unable to create thread udp server thread \n");
		exit(-1);
	}	
	
	ep_stack->total_server_count++;
	
	return tcp_server;
}

app_ep_udp_server_t * app_ep__create_udpv4_server( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb)
{
	app_ep_udp_server_t * udp_server = (app_ep_udp_server_t*)malloc( sizeof(app_ep_udp_server_t));
	memset( udp_server, 0, sizeof( app_ep_udp_server_t));
	
	if(!ep_stack->shead)
	{
		ep_stack->shead = ep_stack->scurrent = udp_server;
	}
	else
	{
		ep_stack->scurrent->Next = udp_server;
		ep_stack->scurrent = udp_server;
	}
	
	udp_server->ipv = 4;
	udp_server->port = port;
	udp_server->udp_bufferSize = udp_bufferSize;
	strcpy( udp_server->ip, ip);
	udp_server->queue = app_queue__create( "udpq", ep_stack->udp_capacity, iDecodeThreadCount, cb);
	udp_server->start = 0;

	udp_server->SendCount = 0;
	udp_server->RecvCount = 0;
	udp_server->DropCount = 0;
	udp_server->LastSendCount = 0;
	udp_server->LastRecvCount = 0;
	udp_server->LastDropCount = 0;
	
	pthread_mutex_init( &udp_server->SendCountLock, NULL);
	pthread_mutex_init( &udp_server->RecvCountLock, NULL);
	pthread_mutex_init( &udp_server->DropCountLock, NULL);
		
	
	int iRet;
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, app_ep__udp_server_thread, (void *)udp_server);
	
	if(iRet)
	{
		perror("Error: ");
		printf("unable to create thread udp server thread \n");
		exit(-1);
	}	
	
	ep_stack->total_server_count++;
	return udp_server;
}

app_ep_udp_server_t * app_ep__create_udpv6_server( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb)
{
	app_ep_udp_server_t * udp_server = (app_ep_udp_server_t*)malloc( sizeof(app_ep_udp_server_t));
	memset( udp_server, 0, sizeof( app_ep_udp_server_t));

	if(!ep_stack->shead)
	{
		ep_stack->shead = ep_stack->scurrent = udp_server;
	}
	else
	{
		ep_stack->scurrent->Next = udp_server;
		ep_stack->scurrent = udp_server;
	}

	udp_server->ipv = 6;
	udp_server->port = port;
	udp_server->udp_bufferSize = udp_bufferSize;
	strcpy( udp_server->ip, ip);
	udp_server->queue = app_queue__create( "udpq", ep_stack->udp_capacity, iDecodeThreadCount, cb);

	int iRet;
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, app_ep__udp_server_thread, (void *)udp_server);
	
	if(iRet)
	{
		perror("Error: ");
		printf("unable to create thread udp server thread \n");
		exit(-1);
	}
	
	ep_stack->total_server_count++;
	return udp_server;
}

void * app_ep__udp_client_thread( void * args)
{
	
	app_ep_udp_client_t * udp_client = (app_ep_udp_client_t*)args;
	int iPort = udp_client->port;
	int clientSocket = 0;
	if( udp_client->ipv == 4) {	
		clientSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		clientSocket = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	}
	
	udp_client->fd = clientSocket;
	printf("start udp client for port %d ipversion=%s fd=%d  (%s|%s|%d) \n", iPort,udp_client->ip, udp_client->fd, __FUNCTION__, __FILE__, __LINE__);

	struct pollfd fds;	
	fds.fd     = udp_client->fd;
	fds.events = POLLIN;

	int n = 0, flags = 0, ret;
	app_ep_udp_message_t * udpPtr = NULL;
	
	int clientlen = sizeof(struct sockaddr_in6);
	
	if( udp_client->ipv == 4) {
		clientlen = sizeof(struct sockaddr_in);

		struct sockaddr_in clientAddr;
		memset( &clientAddr, 0, sizeof( struct sockaddr_in));
		clientAddr.sin_family = AF_INET;
		clientAddr.sin_port = htons(iPort);
		
		if( strlen( udp_client->ip) == 0) {
			clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			clientAddr.sin_addr.s_addr = inet_addr( udp_client->ip);
		}
		
	}
	
	while(1)
	{
		//printf("----recived client from upfstats:%s:%d\n",__FILE__,__LINE__);
		ret = poll( &fds, 1, 1000);
		
		if( ret <= 0)
		{
			continue;
		}

		if( ret > 0)
		{
			udpPtr = app_ep__allocate_udp_message();
			
			if( udp_client->ipv == 4)
			{
				udpPtr->len = recvfrom( udp_client->fd, udpPtr->buffer, udp_client->udp_bufferSize, 0, (struct sockaddr *) &udpPtr->u.clientaddr, &clientlen);
				printf("-----client udpPtr->u.clientaddr=%s:%p\n",inet_ntoa(udpPtr->u.clientaddr.sin_addr), (void*)udpPtr->buffer);

			}
			else
			{
				udpPtr->len = recvfrom( udp_client->fd, udpPtr->buffer, udp_client->udp_bufferSize, 0, (struct sockaddr *) &udpPtr->u.clientaddr6, &clientlen);
			}
			
			udpPtr->client = udp_client;
			app_queue__enquee( udp_client->queue, (uint8_t *)udpPtr, NULL);
			ep_stack->RecvCount++;
		}		
	}

	
	return NULL;
}

app_ep_udp_client_t * app_ep__udp_message_get_client( app_ep_udp_message_t * udpPtr)
{
	return udpPtr->client;
}



void app_ep__udp_client_set_obj( app_ep_udp_client_t * client, uint8_t * obj, uint32_t i)
{
	if( i == 1)
	{
		client->obj1 = obj;
	}	
}


uint8_t * app_ep__udp_client_get_obj( app_ep_udp_client_t * client, uint32_t i)
{
	if( i == 1)
	{
		return client->obj1;
	}	
	return NULL;
}

uint8_t * app_ep__udp_message_get_client_obj( app_ep_udp_message_t * udpPtr, uint32_t i)
{
	app_ep_udp_client_t * c = app_ep__udp_message_get_client( udpPtr);
	
	if(c)
	{
		return app_ep__udp_client_get_obj( c, i);
	}
	return NULL;
}


int app_ep__udpv4_client_bind( app_ep_udp_client_t * udp_client)
{
	// int yes = 1;
	// if( setsockopt( udp_client->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	// {
		// printf("UCP setsockopt SO_REUSEADDR failed\n");
		// exit(1);
	// }
	
	struct sockaddr_in my_addr;
	memset( &my_addr, 0, sizeof( struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr( udp_client->ip);	
	my_addr.sin_port = htons( udp_client->port);

	return bind( udp_client->fd, (struct sockaddr*) &my_addr, sizeof(my_addr));
}

app_ep_udp_client_t * app_ep__create_udpv4_client( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb)
{
	
	app_ep_udp_client_t * udp_client = (app_ep_udp_client_t*)malloc( sizeof(app_ep_udp_client_t));
	
	udp_client->ipv = 4;
	udp_client->port = port;
	udp_client->udp_bufferSize = udp_bufferSize;
	strcpy( udp_client->ip, ip);
	
	if ((udp_client->fd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		exit(1);
	}
	
	
	udp_client->queue = app_queue__create( "udpq", ep_stack->udp_capacity, iDecodeThreadCount, cb);

	int iRet;
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, app_ep__udp_client_thread, (void *)udp_client);
	
	if(iRet)
	{
		perror("Error: ");
		printf("unable to create thread udp server thread  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(-1);
	}
	
	ep_stack->total_client_count++;
	return udp_client;
}

app_ep_udp_client_t * app_ep__create_udpv6_client( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb)
{
	app_ep_udp_client_t * udp_client = (app_ep_udp_client_t*)malloc( sizeof(app_ep_udp_client_t));
	
	udp_client->ipv = 6;
	udp_client->port = port;
	udp_client->udp_bufferSize = udp_bufferSize;
	strcpy( udp_client->ip, ip);
	udp_client->queue = app_queue__create( "udpq", ep_stack->udp_capacity, iDecodeThreadCount, cb);

	int iRet;
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, app_ep__udp_client_thread, (void *)udp_client);
	
	if(iRet)
	{
		perror("Error: ");
		printf("unable to create thread udp server thread  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(-1);
	}
	
	ep_stack->total_client_count++;
	return udp_client;	
}











