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

#ifndef S_APP_AERO_CLIENT
#define S_APP_AERO_CLIENT

#pragma pack(4)
typedef struct app_aerospike_client
{
	char IP[20];
	int Port;
	
	app_ep_stack__tcp_client_t * client;
	app_logger_t * appLogger;

} app_aerospike_client_t;

app_aerospike_client_t * __appAeroClient;


#endif

