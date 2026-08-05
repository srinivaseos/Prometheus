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

#include "app.h"
#include "app_stack.h"
#include "jansson.h"
#include "app_endpoint.h"
#include "app_command.h"

void pfcp_stack___record_usage( uint64_t up_f_seid, uint32_t rgid, uint64_t uplink, uint64_t downlink)
{
}


void app__signalhandler( int signum)
{
	switch( signum)
	{	
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
		{
			
		}
		break;
	}
	
	int secs = 5;
	printf("\nCaught signal %d, coming out...in %d seconds.\n", signum, secs);
	sleep(secs);
	exit(0);
}

int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./dpnc.json\n", argv[0]);
		exit(0);
	}
	
	signal( SIGINT, app__signalhandler);		// Ctrl + C
	signal( SIGQUIT, app__signalhandler);		// Ctrl + \ 			//
	signal( SIGTERM, app__signalhandler);		// shell command kill generates SIGTERM by default
	
	return 0;
}













