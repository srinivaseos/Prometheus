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

#include "eng.h"
#include "app.h"
#include "app_stack.h"
#include "jansson.h"
#include "pfcp_stack.h"
#include "app_endpoint.h"
#include "pfcp_stack.h"
#include "app_command.h"


typedef struct app_upf_cmd
{
	uint32_t size;
	uint32_t cmd;				// 1 pcap, 			2 log    			3 sess  4 pkt_log 5 slf_1
	uint32_t operation;			// 1 start/enable   2 stop/disable
	uint32_t interface;			// 1024 or any number > 0  					
	uint32_t uplink;
	uint32_t downlink;
	char command[36];			//4  ue_ip	
} app_upf_cmd_t;

/*
	cmd 3 sess
	oper 1 del-all, 2 - del-by-cp-ip	, 3 del-by-up_seid,  4 log session info
	ip | up_seid
*/


int app_config__get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

char * app_config__get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}


void upfcmd__udp_client_message( uint8_t * Data, int tIndex)
{
}

void app_ep__init();

// export LD_LIBRARY_PATH=.
// gcc -g3 upfcmd.c ../dplane/app_stack.c ../dplane/app_endpoint.c -I../dplane -o upfcmd -lpthread -ljansson
int main( int argc, char* argv[])
{
	if( argc == 1)
	{
		// pcap capture
		printf( "%s  pcap start 1024\n", argv[0]);
		printf( "%s  pcap start 1\n", argv[0]);
		printf( "%s  pcap start 2\n", argv[0]);
		printf( "%s  pcap stop 1024\n", argv[0]);
		printf( "%s  pcap stop 1\n", argv[0]);
		printf( "%s  pcap stop 2\n", argv[0]);
		
		// record usage
		printf( "%s usage upf-seid-id<int> rg<int> uplink<int> downlink<int> \n", argv[0]);
		printf( "%s quota upf-seid-id<int>\n", argv[0]);
		printf( "%s qlogs enable<int-0|1>\n", argv[0]);
		printf( "%s plogs enable<int-0|1>\n", argv[0]);
		
		printf("\n");
		//printf( "%s  log stop 2\n", argv[0]);
		
		exit(0);
	}
	
	printf("You have entered %d arguments:\n", argc);
 
    /*for (int i = 1; i < argc; i++) 
	{
        printf("argv[%d].%s\n",i, argv[i]);
    }*/
	
	
	/*
		./upfcmd sess del-all
		./upfcmd sess del-by-ip "192.168.144.21"
		./upfcmd sess del-by-up-seid "12345"
		./upfcmd sess log-by-up-seid "12345"
	*/
	app_upf_cmd_t upf_cmd;
	memset( &upf_cmd , 0, sizeof(app_upf_cmd_t));
	
	upf_cmd.size =  sizeof(app_upf_cmd_t);
	
	int i = 0;
	
	for (i = 1; i < argc; i++) 
	{
        printf("argv[%d] = %s\n",i, argv[i]);
		
		if( strcmp(argv[i],"pcap") == 0)
		{
			upf_cmd.cmd = 1;
		}
		else if( strcmp(argv[i],"log") == 0)
		{
			upf_cmd.cmd = 2;
		}
		else if( strcmp(argv[i],"sess") == 0)
		{
			upf_cmd.cmd = 3;
		}
		else if( strcmp(argv[i],"pkt_log") == 0)
		{
			upf_cmd.cmd = 4;
		}
		else if( strcmp(argv[i],"slf_1") == 0)
		{
			upf_cmd.cmd = 5;
		}
		else if( strcmp(argv[i],"usage") == 0)
		{
			if( argc == 6)
			{
				upf_cmd.cmd 		= 6;
				upf_cmd.operation 	= atoi(argv[2]);
				upf_cmd.interface	= atoi(argv[3]);
				upf_cmd.uplink		= atoi(argv[4]);
				upf_cmd.downlink	= atoi(argv[5]);
				
				printf( "cmd=%d  seid=%d  rg=%d  uplink=%d  downlink=%d\n", 
					upf_cmd.cmd, upf_cmd.operation, upf_cmd.interface, upf_cmd.uplink, upf_cmd.downlink);
			}
			else
			{
				printf("insufficient paramaters for usage\n");
				exit(0);
			}
			
			break;
		}
		else if( strcmp(argv[i],"quota") == 0)
		{
			if( argc == 3)
			{
				upf_cmd.cmd 		= 7;
				upf_cmd.operation 	= atoi(argv[2]);
				
				printf( "cmd=%d  seid=%d\n", upf_cmd.cmd, upf_cmd.operation);
			}
			else
			{
				printf("insufficient paramaters for quota\n");
				exit(0);				
			}
			break;
		}
		else if( strcmp(argv[i],"qlogs") == 0)
		{
			if( argc == 3)
			{
				upf_cmd.cmd 		= 8;
				upf_cmd.operation 	= atoi(argv[2]);
				
				printf( "cmd=%d  enable=%d\n", upf_cmd.cmd, upf_cmd.operation);
			}
			else
			{
				printf("insufficient paramaters for quota\n");
				exit(0);				
			}
			break;			
		}
		else if( strcmp(argv[i],"plogs") == 0)
		{
			if( argc == 3)
			{
				upf_cmd.cmd 		= 9;
				upf_cmd.operation 	= atoi(argv[2]);
				
				printf( "cmd=%d  enable=%d\n", upf_cmd.cmd, upf_cmd.operation);
			}
			else
			{
				printf("insufficient paramaters for quota\n");
				exit(0);				
			}
			break;			
		}
		else if( strcmp(argv[i],"start") == 0 || strcmp(argv[i],"enable") == 0 )
		{
			upf_cmd.operation = 1;
		}
		else if( strcmp(argv[i],"stop") == 0 || strcmp(argv[i],"disable") == 0 )
		{
			upf_cmd.operation = 2;
		}
		else if( upf_cmd.cmd == 3 && strcmp(argv[i],"del-all") == 0)
		{
			upf_cmd.operation = 1;
		}
		else if( upf_cmd.cmd == 3 && strcmp(argv[i],"del-by-ip") == 0)
		{
			upf_cmd.operation = 2;
		}
		else if( upf_cmd.cmd == 3 && strcmp(argv[i],"del-by-up-seid") == 0)
		{
			upf_cmd.operation = 3;
		}
		else if( upf_cmd.cmd == 3 && strcmp(argv[i],"log-by-up-seid") == 0)
		{
			upf_cmd.operation = 4;
		}
		else if( upf_cmd.cmd == 3 && strcmp(argv[i],"log-by-ue-ip") == 0)
		{
			upf_cmd.operation = 5;
		}
		else if( upf_cmd.cmd == 4 && upf_cmd.operation > 0)
		{
			strcpy( upf_cmd.command, argv[i]);
		}
		else if( upf_cmd.cmd == 5 && upf_cmd.operation > 0)
		{
			upf_cmd.operation = 6;
		}
		else 
		{
			upf_cmd.interface = atoi( argv[i]);
		}
    }
	
	// if( upf_cmd.cmd == 4) 
	// {
		// strcpy( upf_cmd.command, argv[ i - 1]);
	// }
	
	
	if(upf_cmd.cmd != 1 && upf_cmd.cmd != 2 && upf_cmd.cmd != 3  && upf_cmd.cmd != 4 && upf_cmd.cmd != 5 && upf_cmd.cmd != 6 && upf_cmd.cmd != 7 && upf_cmd.cmd != 8 && upf_cmd.cmd != 9)
	{
		printf("upf_cmd.cmd=%d\t upf_cmd.operation=%d\t upf_cmd.interface=%d\n",upf_cmd.cmd,upf_cmd.operation,upf_cmd.interface);
		printf("cmd: invalid argument  %d\n", __LINE__);
		return 0;
	}
	else if( (upf_cmd.operation != 1 && upf_cmd.operation != 2 && upf_cmd.operation != 3 && upf_cmd.operation != 4 && upf_cmd.operation != 6) && (upf_cmd.cmd != 6 && upf_cmd.cmd != 7 && upf_cmd.cmd != 8 && upf_cmd.cmd != 9))
	{
		printf("upf_cmd.cmd=%d\t upf_cmd.operation=%d\t upf_cmd.interface=%d\n",upf_cmd.cmd,upf_cmd.operation,upf_cmd.interface);
		printf("operation: invalid argument  %d\n", __LINE__);
		return 0;
	}
	else if( upf_cmd.interface == 0 && upf_cmd.cmd == 1)
	{
		printf("upf_cmd.cmd=%d\t upf_cmd.operation=%d\t upf_cmd.interface=%d\n",upf_cmd.cmd,upf_cmd.operation,upf_cmd.interface);
		printf("interface: invalid argument  %d\n", __LINE__);
		return 0;
	}
	else
	{
		json_error_t error;
		memset( &error, 0, sizeof( json_error_t));
		json_t * json_config = json_load_file( "upfcmd.json", 0, &error);
				
		if( json_config)
		{
			json_t * sessionInfoObj = json_object_get( json_config, "UPFCLI");


			if( sessionInfoObj)
			{
				char * ueIP 				= app_config__get_str( sessionInfoObj, "IP");
				int port					= app_config__get_int( sessionInfoObj, "Port", 2);			
				
				printf("upf_cmd.cmd=%d\t upf_cmd.operation=%d\t upf_cmd.interface=%d\nupf_cmd.command=%s\n", upf_cmd.cmd, upf_cmd.operation, upf_cmd.interface, upf_cmd.command);
				
				
				app_ep__init();
				app_ep_udp_client_t * udpclient = app_ep__create_udpv4_client( ueIP,  port,1024, 1, upfcmd__udp_client_message);
				
				int sts = app_ep__udp_send_by_client( udpclient, (char *)&upf_cmd, upf_cmd.size);
				
				
				printf("upf_cmd: successful .. sts=%d\n", sts);
			}
		}
		else
		{
			printf("error in json %s file at line=%d column=%d position=%d\n", argv[1], error.line, error.column, error.position);
			exit(0);
		}
	
	}
	
	
	
	return 0;
}