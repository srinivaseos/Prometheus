

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

#include "flow_description.h"


int flow_description__parse( __flow_desc_info_t * flow_1, char * info)
{
	if(!info) 
		return -1;
	
	int ilen = strlen(info) + 1;
	
	if( ilen == 1) 
		return -2;
	
	char info_1[ilen];
	info_1[ilen-1] = 0;
	memcpy( info_1, info, ilen-1);
	
	char * saveptr = NULL;
	
	char * datap = strtok_r( info_1, " ", &saveptr);
	
	if( datap)
	{
		printf( "datap=%s  -- saveptr=%s\n", datap, saveptr);

		while(saveptr)
		{
			datap = strtok_r( saveptr, " ", &saveptr);
			
			if(!datap)
				break;
			
			printf( "datap=%s  -- saveptr=%s\n", datap, saveptr);

			//break;
		}
	}
	
	
	
	
	
	return 0;
}



// gcc -o fd flow_description.c
int main( int argc, char* argv[])
{
	printf("fd begin ------------------------------------------------\n");
	
	__flow_desc_info_t flow_1;
	memset( &flow_1, 0, sizeof(__flow_desc_info_t));
	
	flow_description__parse( &flow_1, "permit in 6 from 10.98.254.0/24 5061 to 10.98.0.0/24 5060");
	
	printf("fd end   ------------------------------------------------\n");
	
	return 0;	
}


/*
permit in 6 from 10.98.254.0/24 5061 to 10.98.0.0/24 5060
permit out 6 from 10.98.254.0/24 5060 to 10.98.0.0/24 5061

permit in 6 from any 80 to 172.16.1.1 80
permit out 6 from 172.16.1.1 80 to any 80

permit in 17 from 10.98.254.0/24 50000-60100 to 10.98.0.0/24 50000-60100
permit out 17 from 10.98.254.0/24 50000-60100 to 10.98.0.0/24 50000-60100

permit in 17 from 10.98.254.0/24 5061, 5064 to 10.98.0.0/24  5061, 5064
permit out 17 from 10.98.254.0/24 5061, 5064 to 10.98.0.0/24  5061, 5064

permit in 17 from 172.16.0.0/16 50000-60100, 5061, 5064 to 172.16.0.0/16  50000-60100, 5061, 5064
permit out 17 from 172.16.0.0/16 50000-60100, 5061, 5064 to 172.16.0.0/16  50000-60100, 5061, 5064
*/


