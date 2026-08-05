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


#ifndef S_APP_DEF
#define S_APP_DEF

#include "app_stack.h"
#include "app_endpoint.h"
#include "pfcp_stack.h"

typedef struct dp_application dp_application_t;
dp_application_t * __dp_application_getInstance();
pfcp_node_t * dp__find_node( app_ep_udp_message_t * msg);
void dpiadc__init( char * fileName, int defaultRGId);

#endif