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


#ifndef S_APP_AERO_TCP
#define S_APP_AERO_TCP

typedef struct app_logger app_logger_t;
typedef void ( * fp_queue_call_back)( uint8_t * Data, int tIndex);

typedef struct app_ep_stack__tcp_server app_ep_stack__tcp_server_t;
typedef struct app_ep_stack__tcp_client app_ep_stack__tcp_client_t;
typedef struct app_ep_stack__tcp_buffer app_ep_stack__tcp_buffer_t;

typedef int (*app_ep_stack__tcp_read_cb)( app_ep_stack__tcp_client_t *, app_ep_stack__tcp_buffer_t *); 

#pragma pack(4)
typedef struct app_ep_stack__tcp_buffer 
{
	struct app_ep_stack__tcp_buffer * Next;
	
	app_ep_stack__tcp_client_t * client;
	uint8_t * buffer;
	int Length;

} app_ep_stack__tcp_buffer_t;

#pragma pack(4)
typedef struct app_ep_stack__tcp_client 
{
	struct app_ep_stack__tcp_client * Next;

	struct sockaddr clientAddr;
	int isActive;
	int fd;
	long int ReceivedMessages;
	long int SentMessages;
	sem_t semp;
	
	app_ep_stack__tcp_server_t * tcp_server;


	char IP[20];
	int Port;

	int ReadType;
	int ReadLength;
	
	app_ep_stack__tcp_buffer_t * bHead;
	app_ep_stack__tcp_buffer_t * bCurrent;
	pthread_mutex_t bLock;
	int bAvailable;
	int bTotal;
	
	app_ep_stack__tcp_read_cb tcp_read_cb;
	
} app_ep_stack__tcp_client_t;


#define APP_EP_STACK__TCP_READ_TYPE__FIXED_LENGTH  		1
#define APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK  	2


#pragma pack(4)
typedef struct app_ep_stack__tcp_server 
{
	struct app_ep_stack__tcp_server * Next;

	char IP[20];
	int Port;
	
	app_ep_stack__tcp_client_t * cHead;
	app_ep_stack__tcp_client_t * cCurrent;
	pthread_mutex_t cLock;
	int cCount;
	
	int ReadType;
	int ReadLength;
	
	app_ep_stack__tcp_buffer_t * bHead;
	app_ep_stack__tcp_buffer_t * bCurrent;
	pthread_mutex_t bLock;
	int bAvailable;
	int bTotal;

	app_ep_stack__tcp_read_cb tcp_read_cb;

} app_ep_stack__tcp_server_t;

typedef struct app_Queue app_iQueue;

typedef struct app_ep_stack__tcp_stack
{
	app_ep_stack__tcp_server_t * tcp_server_head;
	app_ep_stack__tcp_server_t * tcp_server_current;
	int tcp_server_count;
	
	app_iQueue * q;
	app_logger_t * appLogger;

} app_ep_stack__tcp_stack_t;

app_ep_stack__tcp_stack_t * app_tcp_stack;

void app_ep_stack__tcp_stack_init( fp_queue_call_back cb);
void app_ep_stack__start_server( char * ip, int port, int iReadType, int ReadLength, app_ep_stack__tcp_read_cb tcp_read_cb, int poolSize, int bufferSize);
int app_ep_stack__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer, int ReadLength);
app_ep_stack__tcp_client_t * app_ep_stack__tcp_start_client( char * ip, int port, int iReadType, int ReadLength, app_ep_stack__tcp_read_cb tcp_read_cb, int poolSize, int bufferSize);

void app_ep_stack__tcp_stack_set_logger( app_logger_t * appLogger);
int app_ep_stack__tcp_start_client_isconnected( app_ep_stack__tcp_client_t * client);

int app_ep_stack__send_message( app_ep_stack__tcp_client_t * client, uint8_t * buffer, int len);
void app_ep_stack__release_tcp_buffer( app_ep_stack__tcp_buffer_t * tcp_buffer);

#endif


