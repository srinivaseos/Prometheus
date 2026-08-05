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
#include <ctype.h>


void dpdk__initexit()
{
}

void dpe__init( char * configfile) 
{
}

void dpe__init_engine( int argc, char **argv)
{
}

void dpe__init_engine_test( int argc, char **argv)
{
}

int dpe__get_recv_worker()
{
	return 0;
}

//gcc proxyeng.c -shared -fPIC -o libeng.so
