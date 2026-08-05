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


#ifndef S_APP_COMMAND_DEF
#define S_APP_COMMAND_DEF

typedef struct app_cmd_usage
{
	uint32_t cmd;
	uint64_t cp_seid;
	uint32_t rgid;
	uint32_t uplink;
	uint32_t downlink;
} app_cmd_usage_t;


void app_cmd__init( int ipv, char * ip, int port, int wThreads);
void app_cmd__send_usage( int fd, char * ip, int port, uint64_t up_f_seid, uint32_t rgid, uint32_t uplink, uint32_t downlink);

#endif























