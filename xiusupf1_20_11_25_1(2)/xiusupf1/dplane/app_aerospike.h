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


#ifndef S_AEROSPIKE_DEF
#define S_AEROSPIKE_DEF

#include <stddef.h>
#include <stdlib.h>

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_error.h>
#include <aerospike/as_record.h>
#include <aerospike/as_sleep.h>
#include <aerospike/as_status.h>

#include "pfcp_stack.h"

void app__as__init( int ipv, char * ip, int port, char * ns, char * set, app_logger_t * logger);
void app__as__restoresession();
void app__as__pfcpsavesession( pfcp_session_t * session);
void app__as__remove( pfcp_session_t * session);
int  app__as__restore_completed();
int  app__as__restore_records();

#endif