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

#ifndef S_INTERFACE__XIDP_M
#define S_INTERFACE__XIDP_M


#pragma pack(4)
typedef struct xidp_message_s {

	uint32_t len;
	uint32_t id;
	uint32_t opc;
	
	uint64_t 	allocated_id;
	char 		imsi[16];
	uint32_t 	result;
	uint32_t 	rnumber;
	
}  xidp_message_t;


#endif