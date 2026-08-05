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

#include "app_stack.h"
#include "app_aero.h"
#include "aero_tcp.h"
#include "jansson.h"
#include "app_jet_message.h"

int app_aero__json_get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

char * app_aero__json_get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}

void app_aero__json_set_str( json_t * json_config, char * key, char * vItem, int maxlen)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		int kv = json_string_length( jObj);
		const char * v = json_string_value( jObj);
		if( kv < maxlen)
		{
			memcpy( vItem, v, kv);
		}
		else
		{
			memcpy( vItem, v, maxlen);
		}
	}
}


int app_aero_message__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * buffer);

aerospike as;
as_config config;

app_aerospike_t * __appAero = NULL;


aerospike * app_aero__as_connection() 
{
	return __appAero->as;
}


void app_aero__connect()
{
	memset( __appAero->as, 0, sizeof(aerospike));
	memset( &config, 0, sizeof(as_config));

	as_config_init( &config);
	
	
	int i = 0;
	
	for( i = 0; i < __appAero->AerospikeIPCount; i++)
	{
		printf( "adding config %s  port=%d\n", __appAero->AerospikeIP[i], __appAero->AerospikePort);
		
		if (!as_config_add_hosts( &config, __appAero->AerospikeIP[i], __appAero->AerospikePort)) 
		{
			printf("Invalid host(s) %s\n", __appAero->AerospikeIP[i]);
			as_event_close_loops();
			exit(-1);
		}
	}
	
	
	
	aerospike_init( __appAero->as, &config);
	
	
	as_error err;

	if (aerospike_connect( __appAero->as, &err) != AEROSPIKE_OK) 
	{
		printf("aerospike_connect() failed returned %d - [%s]\n", err.code, err.message);
		app_logger__log( __appAero->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike_connect() failed   err.code=%d err.message=%s  %s|%s|%d", err.code, err.message, __FILE__, __FUNCTION__, __LINE__);
		
		as_event_close_loops();
		aerospike_destroy( __appAero->as);
		exit(-1);
	} 
	else
	{
		printf("aerospike_connect() succesfull Primary-IP=%s Port=%d  %d - [%s]\n", 
			__appAero->AerospikeIP[0], __appAero->AerospikePort, err.code, err.message);
			
		app_logger__log( __appAero->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike_connect() succesfull  Primary-IP=%s Port=%d   err.code=%d err.message=%s  %s|%s|%d", 
			__appAero->AerospikeIP[0], __appAero->AerospikePort, 
			err.code, err.message, __FILE__, __FUNCTION__, __LINE__);
		
		__appAero->as_connected = 1;
	}
}

int app_aero__is_connected()
{
	if( __appAero->as_connected == 1) {
		return aerospike_cluster_is_connected( __appAero->as);
	} else {
		return 0;
	}		
}

void app_aero__destroy()
{
	if( app_aero__is_connected())
	{
		__appAero->as_connected = 0;
		aerospike_destroy( __appAero->as);
	}	
}


void app_aero__reset()
{
	as_error err;
	aerospike_close( __appAero->as, &err);
	aerospike_destroy( __appAero->as);

	app_aero__connect();
}






void app_aero__init_config( json_t * json_config)
{
	char * ip = app_aero__json_get_str( json_config, "IP");
	
	if(!ip)
	{
		printf("please configure application IP=%s\n", ip);
		exit(0);
	}
	
	int port = app_aero__json_get_int( json_config, "Port", 0);
	
	if(port == 0)
	{
		printf("please configure application Port\n");
		exit(0);
	}

	
	json_t * jAerospike = json_object_get( json_config, "Aerospike");
	
	if(!jAerospike)
	{
		printf("please configure Aerospike Section\n");
		exit(0);
	}
	
	json_t * jAerospikeIPs = json_object_get( jAerospike, "IP");
	
	if(!json_is_array(jAerospikeIPs))
	{
		printf("Aerospike IP should be an array\n");
		exit(0);
	}
	
	int iAerospikePort = app_aero__json_get_int( jAerospike, "Port", 0);
	
	if( iAerospikePort == 0)
	{
		printf("please configure Aerospike Port\n");
		exit(0);
	}
	
	
	app_aero__json_set_str( json_config, "IP", __appAero->IP, 20);
	__appAero->Port = port;
	
	printf("\n");
	printf( "App IP=%s Port=%d\n", __appAero->IP, __appAero->Port);
	
	
	int j = 0;
	int aipc = json_array_size(jAerospikeIPs);
	json_t * jIPItem = NULL;
	char * cIP = NULL;
	int iplen = 0;
	
	for( j = 0; j < aipc; j++)
	{
		jIPItem = json_array_get( jAerospikeIPs, j);
		
		if( jIPItem)
		{
			cIP = (char *)json_string_value( jIPItem);
			iplen = json_string_length( jIPItem);
		
			if( iplen < 20)
			{
				memcpy( __appAero->AerospikeIP[__appAero->AerospikeIPCount], cIP, iplen);
				
				printf("Aerospike IP=%s\n", __appAero->AerospikeIP[__appAero->AerospikeIPCount]);
				__appAero->AerospikeIPCount++;
			}
		}	
	}
	
	__appAero->AerospikePort = iAerospikePort;
	app_aero__json_set_str( jAerospike, "Namespace", __appAero->AerospikeNamespace, 20);
	
	printf( "Aerospike Namespace=%s Port=%u\n", __appAero->AerospikeNamespace, __appAero->AerospikePort);
	
	
	
	
	json_t * jMessages = json_object_get( json_config, "Messages");
	
	if( jMessages)
	{
		app_jet__init( jMessages, 0);
	}
}




void app_aero_message__execute( jet_message_buffer_t * msg_buf);
void app_ep_stack__release_tcp_buffer( app_ep_stack__tcp_buffer_t * tcp_buffer);

void app_aero__tcp_qhandler( uint8_t * Data, int tIndex)
{
	app_ep_stack__tcp_buffer_t * tcp_buffer = (app_ep_stack__tcp_buffer_t *)Data;

	//printf("tcp_buffer=%p   %s|%s|%d\n", tcp_buffer, __FILE__, __FUNCTION__, __LINE__);
	
	jet_message_buffer_t * mbuff = app_jet_message__decode( tcp_buffer);
	app_aero_message__execute( mbuff);
	// app_printf_buf( "buff", tcp_buffer->buffer, tcp_buffer->Length);
	
	
	app_ep_stack__release_tcp_buffer( tcp_buffer);	
}

uint32_t app_ep__get_u32( unsigned char * buff);


int app_aero__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	int sts = app_ep_stack__tcp_read( client, tcp_buffer, 12);
	
	//printf( "sts=%d isActive=%d  %s|%d\n", sts, client->isActive, __FILE__, __LINE__);
	
	if( sts <= 0)
		return sts;
	
	uint32_t u32 = app_ep__get_u32( tcp_buffer->buffer);

	//printf( "u32=%d sts=%d isActive=%d   %s|%d\n", u32, sts, client->isActive, __FILE__, __LINE__);
	//app_printf_buf( "", tcp_buffer->buffer, 12);
	
	if( u32 > 12)
	{
		sts = app_ep_stack__tcp_read( client, tcp_buffer, u32 - 12);
		//printf( "u32=%d sts=%d isActive=%d   %s|%d\n", u32, sts, client->isActive, __FILE__, __LINE__);
		
		if( sts <= 0)
			return sts;
	}
	
	return 1;
}


void app_aero_message__set_logger( app_logger_t * logger);

int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./aero.json\n", argv[0]);
		exit(0);
	}

	
	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( argv[1], 0, &error);
	
	if(!json_config)
	{
		printf("error in json %s file at line=%d column=%d position=%d\n", argv[1], error.line, error.column, error.position);
		exit(0);
	}

	
	__appAero = (app_aerospike_t *)malloc( sizeof(app_aerospike_t));
	memset( __appAero, 0, sizeof(app_aerospike_t));

	__appAero->as				= &as;
	__appAero->as_connected 	= 0;


	app_aero__init_config( json_config);
	printf("configuration initalized\n");
	
	__appAero->appLogger = app_logger__create( "./logs/", "AERO_SERVER_", ".log", 500000, 5);
	
	app_timer__init( 2000);
	
	app_aero__connect();
	printf( "aerospike connected=%d\n", aerospike_cluster_is_connected( __appAero->as));
	
	app_ep_stack__tcp_stack_init( app_aero__tcp_qhandler);
	app_ep_stack__tcp_stack_set_logger( __appAero->appLogger);
	app_ep_stack__start_server( __appAero->IP, __appAero->Port, APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK, 0, app_aero__tcp_read, 5000, 1024);
	
	
	app_logger__log( __appAero->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Started Aero-Server  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
	app_aero_message__set_logger( __appAero->appLogger);
	
	while(1)
	{
		sleep(1);
	}
	
	return 0;
}