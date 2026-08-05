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


#ifndef S_APP_ENDPOINT_DEF
#define S_APP_ENDPOINT_DEF

#include "app_stack.h"

#pragma pack(4)
typedef struct app_ep_udp_server
{
	struct app_ep_udp_server * Next;

	char name[100];	
	int port;
	int fd;
	int ipv;	//4||6
	char ip[100];
	int udp_bufferSize;
	app_iQueue * queue;
	int start;

	uint64_t SendCount;
	uint64_t RecvCount;
	uint64_t DropCount;
	uint64_t LastSendCount;
	uint64_t LastRecvCount;	
	uint64_t LastDropCount;	
	pthread_mutex_t SendCountLock;
	pthread_mutex_t RecvCountLock;	
	pthread_mutex_t DropCountLock;		
	
} app_ep_udp_server_t;

#pragma pack(4)
typedef struct app_ep_udp_client
{
	struct app_ep_udp_client * Next;

	char name[100];	
	int port;
	int fd;
	int ipv;	//4||6
	char ip[100];
	int udp_bufferSize;
	app_iQueue * queue;	
	
	uint8_t * obj1;

	uint64_t SendCount;
	uint64_t RecvCount;
	uint64_t DropCount;
	uint64_t LastSendCount;
	uint64_t LastRecvCount;	
	uint64_t LastDropCount;	
	pthread_mutex_t SendCountLock;
	pthread_mutex_t RecvCountLock;	
	pthread_mutex_t DropCountLock;	
	
} app_ep_udp_client_t;

#pragma pack(4)
typedef struct app_ep_tcp_server
{
	struct app_ep_tcp_server * Next;
	
	char name[100];
	int port;
	int fd;
	int ipv;	//4||6
	char ip[100];
	int udp_bufferSize;
	app_iQueue * queue;
	int start;
} app_ep_tcp_server_t;


typedef struct app_ep_udp_message app_ep_udp_message_t;
void app_ep__set_udp_message( app_ep_udp_message_t ** msg, uint8_t * buffer, uint16_t len);

uint32_t app_ep__get_u32_ipv4( char * ip);
uint16_t app_ep__get_len( app_ep_udp_message_t * udp_m);
uint8_t * app_ep__get_buffer( app_ep_udp_message_t * udp_m);
int app_ep__get_ipv( app_ep_udp_message_t * udp_m);
struct sockaddr_in * app_ep__get_ip4addr( app_ep_udp_message_t * udp_m);
struct sockaddr_in6 * app_ep__get_ip6addr( app_ep_udp_message_t * udp_m);
int app_ep__get_fd( app_ep_udp_message_t * udp_m);
void app_ep__get_ipaddr( app_ep_udp_message_t * udp_m, int * fd, uint8_t ** addr, int * len);
int app_ep__udp_sendbymsg( app_ep_udp_message_t * req_msg, app_ep_udp_message_t * res_msg);
int app_ep__udp_sendbymsg2( app_ep_udp_message_t * req_msg, unsigned char * buffer, int len);

void app_ep__encode__msglen( app_ep_udp_message_t * bObj);
void app_ep__encode__setAt( app_ep_udp_message_t * bObj, uint16_t index, uint16_t val);

void app_ep__encode__u8( app_ep_udp_message_t * bObj, uint8_t val);
void app_ep__encode__u16( app_ep_udp_message_t * bObj, uint16_t val);
void app_ep__encode__u24( app_ep_udp_message_t * bObj, uint32_t val);
void app_ep__encode__u32( app_ep_udp_message_t * bObj, uint32_t val);
void app_ep__encode__u16toc( char * buffer, uint32_t val);
void app_ep__encode__u24toc( char * buffer, uint32_t val);
void app_ep__encode__u32toc( char * buffer, uint32_t val);
void app_ep__encode__u40toc( char * buffer, uint64_t val);
void app_ep__encode__u64toc( char * buffer, uint64_t val);
void app_ep__encode__u64( app_ep_udp_message_t * bObj, uint64_t val);
void app_ep__encode__str( app_ep_udp_message_t * bObj, uint8_t * data, uint16_t len);

uint16_t app_ep__get_u16( unsigned char * buff);
uint32_t app_ep__get_u24( unsigned char * buff);
uint32_t app_ep__get_u32( unsigned char * buff);
uint32_t app_ep__get_u32_ntoh( unsigned char * buff);
uint64_t app_ep__get_u40( unsigned char * buff);
uint64_t app_ep__get_u64( unsigned char * buff);
uint64_t app_ep__get_u64_ntoh( unsigned char * buff);


void app_ep__init_udp( int bufferSize, int capacity);
app_ep_udp_message_t * app_ep__allocate_udp_message();
void app_ep__free_udp_message( app_ep_udp_message_t * udp_m);

app_ep_udp_server_t * app_ep__create_udpv4_server( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb);
app_ep_udp_server_t * app_ep__create_udpv6_server( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb);
int app_ep__udp_send( int fd, struct sockaddr * saddr, int slen, char * buff, int len);
int app_ep__udp_send2( app_ep_udp_server_t * udp_server, struct sockaddr * saddr, int slen, char * buff, int len);
int app_ep__udp_sendto_v4( int fd, char * ip, int port, char * buff, int len);

app_ep_udp_client_t * app_ep__create_udpv4_client( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb);
app_ep_udp_client_t * app_ep__create_udpv6_client( char * ip, int port, int udp_bufferSize, int iDecodeThreadCount, fp_queue_call_back cb);

void app_ep__udp_client_set_obj( app_ep_udp_client_t * client, uint8_t * obj, uint32_t i);
uint8_t * app_ep__udp_client_get_obj( app_ep_udp_client_t * client, uint32_t i);
app_ep_udp_client_t * app_ep__udp_message_get_client( app_ep_udp_message_t * udpPtr);
uint8_t * app_ep__udp_message_get_client_obj( app_ep_udp_message_t * udpPtr, uint32_t i);

void app_ep__get_strip( app_ep_udp_message_t * udp_m, char * data);
void app_ep__get_str_ipv4( uint32_t ip, char * data);
void app_ep__get_str_ipv6( char * ipv6, char * data);

void app_ep__perflog( app_logger_t * logger, int log_buffers);
void app_ep__udp_server_start( app_ep_udp_server_t * udp_server);

int app_ep__udp_send_by_client( app_ep_udp_client_t * client, char * buff, int len);
uint8_t * app_ep__get_u( app_ep_udp_message_t * udp_m);
int app_ep__udp_send_by_sockaddrv4( int fd, struct sockaddr_in * clientaddr, char * buff, int len);
uint32_t app_ep__u32ip( char * ip);

int app_ep__udpv4_client_bind( app_ep_udp_client_t * udp_client);

#endif


