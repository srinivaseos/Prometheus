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
#include "aero_tcp.h"
#include "jansson.h"
#include "app_jet_message.h"
#include "app_aero_client.h"

app_aerospike_client_t * __appAeroClient = NULL;



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


void app_aero_client__init_config( json_t * json_config)
{
	char * ip = app_aero__json_get_str( json_config, "ASServerIP");
	
	if(!ip)
	{
		printf("please configure application IP\n");
		exit(0);
	}
	
	int port = app_aero__json_get_int( json_config, "ASServerPort", 0);
	
	if(port == 0)
	{
		printf("please configure application Port\n");
		exit(0);
	}

	app_aero__json_set_str( json_config, "ASServerIP", __appAeroClient->IP, 20);
	__appAeroClient->Port = port;
	
	printf("\n");
	printf( "App IP=%s Port=%d\n", __appAeroClient->IP, __appAeroClient->Port);
	
	

	json_t * jMessages = json_object_get( json_config, "Messages");
	
	if( jMessages)
	{
		app_jet__init( jMessages, 0);
	}
}



void app_aero_client__send_message()
{
	jet_message_t * msg_def = app_jet__find_message( 1001);
	
	uint32_t tval = (uint32_t)time(0);
	char cval[25];
	memset( cval, 0, sizeof(cval));
	char cval2[50];
	memset( cval2, 0, sizeof(cval2));
	sprintf( cval, "data-%u", tval);
	
	uint32_t tval2 = tval + 1;
	uint64_t tval3 = tval + 2;
	
	sprintf( cval2, "%s-%u-%lu", cval, tval2, tval3);
	
	if( msg_def)
	{
		//printf("msg_def=%p\n", msg_def);
		
		uint8_t buffer[1024];
		memset( buffer, 0, sizeof(buffer));
		
		jet_message_buffer_t * msgBuff = app_jet_message__init( 1001, buffer);
		
		if( msgBuff)
		{
			jet_element_t * eItem = msg_def->eHead;
			
			//printf( "1 eItem=%p len=%d pos=%d\n", eItem, msgBuff->len, msgBuff->pos);
			
			while( eItem)
			{
				switch( eItem->ID)
				{
					case 1:			//JET_ELEMENT_TYPE__UINT32:
						app_jet__add_u32( eItem, msgBuff, tval);
						break;
					case 2:			//JET_ELEMENT_TYPE__CHAR:
						app_jet__add_char( eItem, msgBuff, cval, strlen(cval));
						break;
					case 3:
						app_jet__add_u32( eItem, msgBuff, tval2);
						break;
					case 4:
						app_jet__add_u64( eItem, msgBuff, tval3);
						break;
					case 5:
						app_jet__add_char( eItem, msgBuff, cval2, strlen(cval2));
						break;
					default:
						break;
				}

				//printf( "2 eItem=%p len=%d pos=%d\n", eItem, msgBuff->len, msgBuff->pos);
				eItem = eItem->Next;
			}
			

			int sentBytes = app_ep_stack__send_message( __appAeroClient->client, msgBuff->buffer , msgBuff->pos);
			printf( "sentBytes=%d pos=%d len=%d\n", sentBytes, msgBuff->pos, msgBuff->len);
			
			
			//printf( "msg-buff pos=%d len=%d\n", msgBuff->pos, msgBuff->len);
			//app_printf_buf( "buffer", msgBuff->buffer, msgBuff->len);
			
			// jet_message_buffer_t * mb2 = app_jet_message__decode_buffer( msgBuff->buffer);
			// printf( "len=%d id=%d opc=%d pos=%d  %s|%d\n", mb2->len, mb2->id, mb2->opc, mb2->pos, __FILE__, __LINE__);
			// jet_element_data_t edata;
			// while( mb2->pos < mb2->len)
			// {
				// app_jet_message__decode_element_data( mb2, &edata);
				// printf("ID=%d Type=%u Length=%u Val=%lu CVal=%s\n", edata.ID, edata.Type, edata.Length, edata.Value, edata.CData);
			// }
			
		}
	}
}


int app_aero_message__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer);
void app_ep_stack__release_tcp_buffer( app_ep_stack__tcp_buffer_t * tcp_buffer);
uint32_t app_ep__get_u32( unsigned char * buff);

int app_aero_client__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	int sts = app_ep_stack__tcp_read( client, tcp_buffer, 12);
	
	//printf( "sts=%d %s|%d\n", sts, __FILE__, __LINE__);
	
	
	if( sts < 0)
		return sts;
	
	uint32_t u32 = app_ep__get_u32( tcp_buffer->buffer);
	
	sts = app_ep_stack__tcp_read( client, tcp_buffer, u32 - 12);
	
	//printf( "u32=%d sts=%d %s|%d\n", u32, sts, __FILE__, __LINE__);
	
	
	if( sts < 0)
		return sts;
	
	return 1;
}



void app_aero_client__tcp_qhandler( uint8_t * Data, int tIndex)
{
	app_ep_stack__release_tcp_buffer( (app_ep_stack__tcp_buffer_t *)Data);
}





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

	__appAeroClient = (app_aerospike_client_t*)malloc(sizeof(app_aerospike_client_t));
	memset( __appAeroClient, 0, sizeof(app_aerospike_client_t));


	app_aero_client__init_config( json_config);
	printf("configuration initalized\n");
	
	__appAeroClient->appLogger = app_logger__create( "./logs/", "AERO_CLIENT_", ".log", 500000, 5);
	
	
	app_timer__init( 2000);
	
	app_ep_stack__tcp_stack_init( app_aero_client__tcp_qhandler);
	app_ep_stack__tcp_stack_set_logger( __appAeroClient->appLogger);
	
	__appAeroClient->client = app_ep_stack__tcp_start_client( __appAeroClient->IP, __appAeroClient->Port, 
		APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK, 0, app_aero_client__tcp_read, 2000, 1024);

	app_logger__log( __appAeroClient->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Started Aero-Client  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
	
	
	while(1)
	{
		if( app_ep_stack__tcp_start_client_isconnected( __appAeroClient->client))
		{
			app_aero_client__send_message();
		}
		
		sleep(1);
	}
	
	return 0;
}

