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

#ifndef __FLOW_DESC_H
#define __FLOW_DESC_H

#define __FLOW_DESC_PERM__PERMIT	1
#define __FLOW_DESC_PERM__DENY		2

#define __FLOW_DESC_DIRE__IN	1
#define __FLOW_DESC_DIRE__OUT	2


#pragma pack(4)
typedef struct __flow_desc_info 
{
	struct __flow_desc_info * Next;
	
	uint16_t perm;
	uint16_t dire;
	char source[32];
	char dest[32];
	uint16_t source_port;
	uint16_t dest_port;
	uint16_t proto;

} __flow_desc_info_t;


#endif


