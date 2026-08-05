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

#include "jansson.h"
#include "app_stack.h"
#include "app_endpoint.h"
#include "aero_tcp.h"
#include "xidp_interface.h"
#include "app_jet_message.h"

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_index.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/aerospike_udf.h>
#include <aerospike/as_bin.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_dir.h>
#include <aerospike/as_error.h>
#include <aerospike/as_config.h>
#include <aerospike/as_key.h>
#include <aerospike/as_operations.h>
#include <aerospike/as_password.h>
#include <aerospike/as_record.h>
#include <aerospike/as_record_iterator.h>
#include <aerospike/as_sleep.h>
#include <aerospike/as_status.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>
#include <aerospike/aerospike_scan.h>


#pragma pack(4)
typedef struct xidp_info_s {
	struct xidp_info_s * Next;
	
	//uint64_t idp;
	char imsi[16];
	uint32_t status;

} xidp_info_t;


#pragma pack(4)
typedef struct __xidp_s {

	char IP[20];	
	int Port;
	
	uint32_t poolsize;
	uint64_t beginNo;
	uint64_t endNo;
	
	char AerospikeIP[5][20];
	int AerospikeIPCount;
	int AerospikePort;
	char AerospikeNamespace[100];
	char AerospikeSet[100];
	
	aerospike * as;
	int as_connected;

	app_logger_t * appLogger;	
	app_rbtree_t * idpTree;
	
	xidp_info_t * r_head;
	xidp_info_t * r_current;
	pthread_mutex_t r_lock;

	xidp_info_t * p_head;
	xidp_info_t * p_current;
	pthread_mutex_t p_lock;
	
	xidp_info_t ** xidps;
	
	uint32_t AERO_APP_MsgId;
	
	char logsFolder[20];
	char logsPrefix[20];
	
} __xidp_t;

__xidp_t * __xidp = NULL;

#pragma pack(4)
typedef struct app_aerospike_client
{
	char IP[20];
	int Port;
	
	app_ep_stack__tcp_client_t * client;
	app_logger_t * appLogger;

} app_aerospike_client_t;

app_aerospike_client_t * __appAeroClient = NULL;


int app_xidp__json_get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

long app_xidp__json_get_long( json_t * json_config, char * key, long defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

char * app_xidp__json_get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}

void app_xidp__json_set_str( json_t * json_config, char * key, char * vItem, int maxlen)
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

uint64_t iRecordCount = 0 ;



aerospike * app_xidp__as_connection() 
{
	return __xidp->as;
}

aerospike as;
as_config config;

void app_xidp__connect()
{
	as_config_init( &config);
	
	
	int i = 0;
	
	for( i = 0; i < __xidp->AerospikeIPCount; i++)
	{
		printf( "adding config %s  port=%d\n", __xidp->AerospikeIP[i], __xidp->AerospikePort);
		
		if (!as_config_add_hosts( &config, __xidp->AerospikeIP[i], __xidp->AerospikePort)) 
		{
			printf("Invalid host(s) %s\n", __xidp->AerospikeIP[i]);
			as_event_close_loops();
			exit(-1);
		}
	}
	
	
	
	aerospike_init( __xidp->as, &config);
	
	
	as_error err;

	if (aerospike_connect( __xidp->as, &err) != AEROSPIKE_OK) 
	{
		printf("aerospike_connect() failed returned %d - [%s]\n", err.code, err.message);
		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike_connect() failed   err.code=%d err.message=%s  %s|%s|%d", err.code, err.message, __FILE__, __FUNCTION__, __LINE__);
		
		as_event_close_loops();
		aerospike_destroy( __xidp->as);
		exit(-1);
	} 
	else
	{
		printf("aerospike_connect() succesfull Primary-IP=%s Port=%d  %d - [%s]\n", 
			__xidp->AerospikeIP[0], __xidp->AerospikePort, err.code, err.message);
			
		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike_connect() succesfull  Primary-IP=%s Port=%d   err.code=%d err.message=%s  %s|%s|%d", 
			__xidp->AerospikeIP[0], __xidp->AerospikePort, 
			err.code, err.message, __FILE__, __FUNCTION__, __LINE__);
		
		__xidp->as_connected = 1;
	}
}


int app_xidp__is_connected()
{
	if( __xidp->as_connected == 1) {
		return aerospike_cluster_is_connected( __xidp->as);
	} else {
		return 0;
	}		
}

void app_xidp__destroy()
{
	if( app_xidp__is_connected())
	{
		__xidp->as_connected = 0;
		aerospike_destroy( __xidp->as);
	}	
}


void app_xidp__reset()
{
	as_error err;
	aerospike_close( __xidp->as, &err);
	aerospike_destroy( __xidp->as);

	app_xidp__connect();
}






void app_aero_client__init_config( json_t * json_config)
{
	char * ip = app_xidp__json_get_str( json_config, "ASServerIP");
	
	if(!ip)
	{
		printf("please configure application IP\n");
		exit(0);
	}
	
	int port = app_xidp__json_get_int( json_config, "ASServerPort", 0);
	
	if(port == 0)
	{
		printf("please configure application Port\n");
		exit(0);
	}

	app_xidp__json_set_str( json_config, "ASServerIP", __appAeroClient->IP, 20);
	__appAeroClient->Port = port;
	
	printf("\n");
	printf( "App IP=%s Port=%d\n", __appAeroClient->IP, __appAeroClient->Port);
	
	

	json_t * jMessages = json_object_get( json_config, "Messages");
	
	if( jMessages)
	{
		app_jet__init( jMessages, 0);
	}
}


void app_xidp__init_config( json_t * json_config)
{
	char * ip = app_xidp__json_get_str( json_config, "IP");
	
	if(!ip)
	{
		printf("please configure application IP=%s\n", ip);
		exit(0);
	}
	
	int port = app_xidp__json_get_int( json_config, "Port", 0);
	
	if(port == 0)
	{
		printf("please configure application Port\n");
		exit(0);
	}

	json_t * jIDP = json_object_get( json_config, "IDP");
	
	if(!jIDP)
	{
		printf("please configure IDP Section\n");
		exit(0);
	}
	
	
	__xidp->beginNo 	= app_xidp__json_get_long( jIDP, "BeginNo", 0);
	__xidp->endNo 		= app_xidp__json_get_long( jIDP, "EndNo", 0);
	__xidp->poolsize 	= app_xidp__json_get_long( jIDP, "PoolSize", 0);
	
	if( __xidp->beginNo == 0 || __xidp->endNo == 0 || __xidp->poolsize == 0)
	{
		exit(0);
	}
	
	if( __xidp->beginNo > __xidp->endNo)
	{
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
	
	int iAerospikePort = app_xidp__json_get_int( jAerospike, "Port", 0);
	
	if( iAerospikePort == 0)
	{
		printf("please configure Aerospike Port\n");
		exit(0);
	}
	
	
	app_xidp__json_set_str( json_config, "IP", __xidp->IP, 20);
	__xidp->Port = port;
	
	printf("\n");
	printf( "App IP=%s Port=%d\n", __xidp->IP, __xidp->Port);
	
	
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
				memcpy( __xidp->AerospikeIP[__xidp->AerospikeIPCount], cIP, iplen);
				
				printf("Aerospike IP=%s\n", __xidp->AerospikeIP[__xidp->AerospikeIPCount]);
				__xidp->AerospikeIPCount++;
			}
		}	
	}
	
	__xidp->AerospikePort = iAerospikePort;
	app_xidp__json_set_str( jAerospike, "Namespace", __xidp->AerospikeNamespace, 20);
	app_xidp__json_set_str( jAerospike, "Set", __xidp->AerospikeSet, 50);
	
	printf( "Aerospike Namespace=%s Port=%u\n", __xidp->AerospikeNamespace, __xidp->AerospikePort);
	
	__xidp->AERO_APP_MsgId = app_xidp__json_get_int( json_config, "AERO_APP_MsgId", 0);
	printf( "AERO_APP_MsgId=%u\n", __xidp->AERO_APP_MsgId);


	char aerospikeConfigFile[100];
	memset( aerospikeConfigFile, 0, sizeof(aerospikeConfigFile));
	
	
	app_xidp__json_set_str( json_config, "ASConfigFile", aerospikeConfigFile, 100);
	app_xidp__json_set_str( json_config, "LogsFolder", __xidp->logsFolder, 20);
	app_xidp__json_set_str( json_config, "LogsPrefix", __xidp->logsPrefix, 20);
	
	if( strlen( aerospikeConfigFile) > 0)
	{
		__appAeroClient = (app_aerospike_client_t*)malloc(sizeof(app_aerospike_client_t));
		memset( __appAeroClient, 0, sizeof(app_aerospike_client_t));
		
		// printf( "__appAeroClient=%p\n", __appAeroClient);
		

		json_error_t error;
		memset( &error, 0, sizeof( json_error_t));
		json_t * aero_json_config = json_load_file( aerospikeConfigFile, 0, &error);
	
		
		// printf( "aero_json_config=%p %d\n", aero_json_config, __LINE__);
	
		if( aero_json_config)
		{
			app_aero_client__init_config( aero_json_config);
			printf( "aero_json_config=%p %d\n", aero_json_config, __LINE__);
			
			
			// printf( "init aero-server completed\n");
		}
		else
		{
			printf("error in aerospike-json %s file at line=%d column=%d position=%d\n", aerospikeConfigFile, error.line, error.column, error.position);
		}
		
		//exit(0);
	}
}




void app_aero__as_putx( uint64_t base_id, uint64_t fv, uint32_t GenId, uint32_t status, char * imsi, uint16_t opc);

void app_xidp__handle_request( xidp_message_t * mess, app_ep_stack__tcp_client_t * client)
{
	// xidp_info_t * item = NULL;
	// pthread_mutex_lock( &__xidp->p_lock);

		// item = __xidp->p_head;
	
		// if( item)
		// {
			// __xidp->p_head = item->Next;
			// item->Next = NULL;
		// }

	// pthread_mutex_unlock( &__xidp->p_lock);


	// if( item)
	// {
		// pthread_mutex_lock( &__xidp->r_lock);
		
		// if(!__xidp->r_head)
		// {
			// __xidp->r_head = __xidp->r_current = item;
		// }
		// else
		// {
			// __xidp->r_current->Next = item;
			// __xidp->r_current = item;
		// }
		
		// pthread_mutex_unlock( &__xidp->r_lock);
	// }
	
	app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
				"received request with opc=%u  xmsg.id=%u  %s|%s|%d", mess->opc, mess->id,
				__FILE__, __FUNCTION__, __LINE__);
				
	
	uint64_t id = 0; 
	uint32_t GenId = 0;
	app_rbnode_t * node = NULL;
	
	if( mess->opc == 1000)
	{
		//allocate
		mess->allocated_id 	= app_rbnode__get_next_idp( __xidp->idpTree, NULL);
		
		if( mess->allocated_id != UINT64_MAX)
		{
			mess->result = 1;
			
			uint64_t base_id = app_rbnode__get_baseid_idp( __xidp->idpTree, mess->allocated_id);
			
			memset( __xidp->xidps[base_id]->imsi, 0, sizeof(__xidp->xidps[base_id]->imsi));
			__xidp->xidps[base_id]->status = 0;
			
			uint32_t imsilen = strlen( mess->imsi);
			
			if( imsilen > 0)
			{
				memcpy( __xidp->xidps[base_id]->imsi, mess->imsi, imsilen);
			}
			
			
			
			
			node = app_rbnode__find( __xidp->idpTree, base_id);
			app_rbnode__get( node, &id, &GenId);
			
			app_aero__as_putx( base_id, mess->allocated_id, GenId, 1, mess->imsi, 1);
		}
		else
		{
			mess->result = 10;
			mess->allocated_id = 0;
		}
	}
	else if( mess->opc == 1100)
	{
		//update
		mess->result = 1;
		
		uint64_t base_id = app_rbnode__get_baseid_idp( __xidp->idpTree, mess->allocated_id);
		uint32_t imsilen = strlen( mess->imsi);
		
		if( (base_id >= 0 && base_id < __xidp->poolsize) && imsilen > 0)
		{
			memcpy( __xidp->xidps[base_id]->imsi, mess->imsi, imsilen);
			mess->result		= 1;
			
			node = app_rbnode__find( __xidp->idpTree, base_id);
			app_rbnode__get( node, &id, &GenId);
			
			//app_aero__as_putx( base_id, id, mess->allocated_id, GenId, 1, mess->imsi, 1);
			app_aero__as_putx( base_id, mess->allocated_id, GenId, 1, mess->imsi, 1);
		}
		else
		{
			mess->result = 20;
		}
	}
	else if( mess->opc == 1200)
	{
		//release
		mess->result = 1;
		
		uint64_t base_id = app_rbnode__get_baseid_idp( __xidp->idpTree, mess->allocated_id);
		
		if( base_id >= 0 && base_id < __xidp->poolsize)
		{
			memset( __xidp->xidps[base_id]->imsi, 0, sizeof(__xidp->xidps[base_id]->imsi));
			app_rbnode__free_idp( __xidp->idpTree, mess->allocated_id);
			
			node = app_rbnode__find( __xidp->idpTree, base_id);
			app_rbnode__get( node, &id, &GenId);
			
			//app_aero__as_putx( base_id, id, mess->allocated_id, GenId, 0, mess->imsi, 1);
			app_aero__as_putx( base_id, mess->allocated_id, GenId, 0, mess->imsi, 1);
		}
	}
	else
	{
		mess->result = 100;
	}

	// printf( "allocated_id=%lu result=%u  base-id=%lu\n", 
			// mess->allocated_id, mess->result,
			// app_rbnode__get_baseid_idp( __xidp->idpTree, mess->allocated_id)
		// );



	if( client)
	{
		uint32_t sz = sizeof(xidp_message_t);
		mess->len 	= ntohl(sz);
		
		int sentBytes = app_ep_stack__send_message( client, (char*)mess, sz);
		
		if( sentBytes < 0)
		{
			app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
				"sending response failed  client=%p client->fd=%d %s|%s|%d", client, client->fd, __FILE__, __FUNCTION__, __LINE__);
		}
	}	
}


void app_ixdp__tcp_qhandler( uint8_t * Data, int tIndex)
{
	app_ep_stack__tcp_buffer_t * tcp_buffer = (app_ep_stack__tcp_buffer_t *)Data;

	//printf("tcp_buffer=%p   %s|%s|%d\n", tcp_buffer, __FILE__, __FUNCTION__, __LINE__);
	
	xidp_message_t * mess = (xidp_message_t *)tcp_buffer->buffer;
	app_xidp__handle_request( mess, tcp_buffer->client);
	
	// jet_message_buffer_t * mbuff = app_jet_message__decode( tcp_buffer);
	// app_aero_message__execute( mbuff);
	// app_printf_buf( "buff", tcp_buffer->buffer, tcp_buffer->Length);
	
	app_ep_stack__release_tcp_buffer( tcp_buffer);	
}

uint32_t app_ep__get_u32( unsigned char * buff);

int app_ixdp__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	int sts = app_ep_stack__tcp_read( client, tcp_buffer, 12);
	
	//printf( "sts=%d isActive=%d  %s|%d\n", sts, client->isActive, __FILE__, __LINE__);
	
	if( sts <= 0)
		return sts;
	
	uint32_t u32 = app_ep__get_u32( tcp_buffer->buffer);
	
	if( u32 > sizeof(xidp_message_t))
		u32 = ntohl(u32);

	
	// printf( "message length=%d|%d sts=%d isActive=%d   %s|%d\n", u32, ntohl(u32), sts, client->isActive, __FILE__, __LINE__);
	// app_printf_buf( "", tcp_buffer->buffer, 12);
	
	if( u32 > 12)
	{
		sts = app_ep_stack__tcp_read( client, tcp_buffer, u32 - 12);
		//printf( "u32=%d sts=%d isActive=%d   %s|%d\n", u32, sts, client->isActive, __FILE__, __LINE__);
		
		if( sts <= 0)
			return sts;
	}
	
	return 1;
}


bool app_xidp__scan_cb( const as_val * p_val, void * udata)
{
	if (! p_val) 
	{
		return true;
	}

	as_record * p_rec = as_record_fromval( p_val);

	if (! p_rec) 
	{
		return true;
	}

	uint64_t base_id 	= as_record_get_int64( p_rec, "XIDP_BASE_ID", 0);
	uint64_t allo_id 	= as_record_get_int64( p_rec, "XIDP_FC", 0);
	uint32_t gen 		= (uint32_t)as_integer_get( as_record_get_integer( p_rec, "XIDP_GID"));
	uint32_t status 	= (uint32_t)as_integer_get( as_record_get_integer( p_rec, "STATUS"));
	as_string * as_str 	= as_record_get_string( p_rec, "XIDP_IMSI");
	
	//printf( "base_id = %-8lu  allo_id = %-8lu   gen = %-6u  status = %-6u\n", base_id, allo_id, gen, status);

	
	if( base_id >= 0)
	{
		if( status == 1)
		{
			app_rbnode__mark_as_allocate( __xidp->idpTree, base_id, allo_id, gen, NULL);
		}
		else
		{
			app_rbnode__set_id_and_gen( __xidp->idpTree, base_id, allo_id, gen);
		}
		
		if( as_str)
		{
			if( as_str->len > 0)
			{
				
			}
		}
		
		iRecordCount++;
	}
}

void app_xidp__load_as_context()
{
	//printf( "%s %d\n", __FUNCTION__, __LINE__);
	

	as_error err;

	as_scan scan;
	as_scan_init( &scan, __xidp->AerospikeNamespace, __xidp->AerospikeSet);

	if (aerospike_scan_foreach( __xidp->as, &err, NULL, &scan, app_xidp__scan_cb, NULL) != AEROSPIKE_OK) 
	{
		printf("aerospike_scan_foreach() returned %d - %s\n", err.code, err.message);
		as_scan_destroy( &scan);
		app_xidp__reset();
		return;
	}

	if( __xidp->appLogger)
	{
		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_DEBUG, "scan-completed with record-count=%d  %s|%s|%d", iRecordCount, __FILE__, __FUNCTION__, __LINE__);
	}
	
}


void app_xidp__as_put( uint64_t id, uint32_t genid, char * imsi)
{
	as_error 	err;
	as_key 		key;
	as_record 	rec;

	// memset( &err, 0, sizeof(as_error));
	// memset( &key, 0, sizeof(as_key));
	// memset( &rec, 0, sizeof(as_record));

	as_policy_write wpol;
	as_policy_write_init( &wpol);
	//wpol.exists = AS_POLICY_EXISTS_CREATE;
	//wpol.ttl = AS_RECORD_DEFAULT_TTL;
	
	if( imsi) {
		as_record_inita( &rec, 3);
	} else {
		as_record_inita( &rec, 2);
	}
	
	// char sid[20];
	// memset( sid, 0, sizeof(sid));
	// sprintf( sid, "%lu", id);
	
	// as_key_init_str( &key, __xidp->AerospikeNamespace, __xidp->AerospikeSet, sid);
	as_key_init_int64( &key, __xidp->AerospikeNamespace, __xidp->AerospikeSet, id);
	as_record_set_int64( &rec, "XIDP_BASE_ID", id);
	as_record_set_integer( &rec, "XIDP_GID", as_integer_new(genid));
	
	if( imsi)
	{
		as_record_set_str( &rec, "XIDP_IMSI", imsi);
	}
	

	if ( aerospike_key_put( __xidp->as, &err, &wpol, &key, &rec) != AEROSPIKE_OK) 
	{
		//err.file, 
		printf("put failed., error(%d) %s at [%d] %s %s conn=%u\n", 
				err.code, err.message, err.line, 
				__xidp->AerospikeNamespace, __xidp->AerospikeSet,
				aerospike_cluster_is_connected( __xidp->as)
			);
	}
	else
	{
		printf("put success\n");
	}
}


// void app_xidp__init_as_context()
// {
	// uint64_t i = 0;
	// uint64_t id = 0; 
	// uint32_t GenId = 0;
	// app_rbnode_t * node = NULL;
	

		
	// for( i = 0; i < __xidp->poolsize; i++)
	// {
		// sleep(1);
		// node = app_rbnode__find( __xidp->idpTree, i);
		// app_rbnode__get( node, &id, &GenId);
		
		// app_xidp__as_put( id, GenId, NULL);
	// }
	
	// printf( "%s %d\n", __FUNCTION__, __LINE__);
// }



void app_aero__as_putx( uint64_t base_id, uint64_t fv, uint32_t GenId, uint32_t status, char * imsi, uint16_t opc)
{
	// printf( "base_id=%lu   fv=%lu GenId=%u status=%u imsi=%s opc=%u\n", 
		// base_id,  fv, GenId, status, imsi, opc);
	
	
	uint8_t buffer[2048];
	memset( buffer, 0, sizeof(buffer));
	
	jet_message_t * msg_def = app_jet__find_message( __xidp->AERO_APP_MsgId);
	jet_message_buffer_t * msgBuff = app_jet_message__init( __xidp->AERO_APP_MsgId, buffer);

	if( msgBuff)
	{
		jet_element_t * eItem = msg_def->eHead;
		
		while( eItem)
		{
			switch( eItem->ID)
			{
				case 1:
					{
						app_jet__add_u64( eItem, msgBuff, base_id + 1);
					}
					break;
				case 2:
					{
						app_jet__add_u64( eItem, msgBuff, base_id);
					}
					break;
				case 3:
					{
						app_jet__add_u64( eItem, msgBuff, fv);
					}
					break;	
				case 4:
					{
						app_jet__add_u32( eItem, msgBuff, GenId);
					}
					break;
				case 5:
					{
						app_jet__add_u32( eItem, msgBuff, status);
					}
					break;
				case 6:
					{
						if( imsi)
						{
							if( strlen(imsi) > 0)
							{
								app_jet__add_char( eItem, msgBuff, imsi, strlen(imsi));
							}
						}
					}
					break;
				case 7:
					{
						app_jet__add_u32( eItem, msgBuff, (uint32_t)time(NULL));
					}
					break;	
				default:
					break;
			}
			
			eItem = eItem->Next;
		}
		
		app_jet_message__set_opc( msgBuff, 1);
		app_jet_message__set_len( msgBuff, msgBuff->pos);
		
		
		int sentBytes = app_ep_stack__send_message( __appAeroClient->client, msgBuff->buffer , msgBuff->pos);
		//printf( "sentBytes=%d pos=%d len=%d\n", sentBytes, msgBuff->pos, msgBuff->len);
		
		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "sentBytes=%d pos=%d len=%d  %s|%s|%d", 
			sentBytes, msgBuff->pos, msgBuff->len, __FILE__, __FUNCTION__, __LINE__);
		
		app_jet_message_buffer__release( msgBuff);
			
	}
}

void app_aero__init_records()
{
	uint64_t i 				= 0;
	uint64_t id 			= 0; 
	uint32_t GenId 			= 0;
	app_rbnode_t * node 	= NULL;
	uint64_t allocated_id 	= 0;
	uint64_t base_id 		= 0;

	uint64_t lcount = __xidp->poolsize * 5;
	
	for( i = 0; i < lcount; i++)
	{
		sleep(1);
		
		allocated_id 	= app_rbnode__get_next_idp( __xidp->idpTree, NULL);
		base_id 		= app_rbnode__get_baseid_idp( __xidp->idpTree, allocated_id);
		
		node = app_rbnode__find( __xidp->idpTree, base_id);
		app_rbnode__get( node, &id, &GenId);
		
		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "base_id=%lu allocated_id=%lu GenId=%u  %s|%s|%d", 
			base_id, allocated_id, GenId,
			__FILE__, __FUNCTION__, __LINE__);
		
		//exit(0);
		app_aero__as_putx( base_id, allocated_id, GenId, 1, NULL, 1);
		app_rbnode__free_idp( __xidp->idpTree, allocated_id);
	}
	
	printf( "%s %d\n", __FUNCTION__, __LINE__);	
}

// gcc -g3 main.c ../dplane/app_stack.c ../dplane/app_endpoint.c -o xidp -I../jansson-2.13/src/ -lpthread -ljansson
int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./idp.json\n", argv[0]);
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
	
	if(!__xidp)
	{
		__xidp = (__xidp_t*)malloc(sizeof(__xidp_t));
		memset( __xidp, 0, sizeof(__xidp_t));
	
		__xidp->r_head = NULL;
		__xidp->r_current = NULL;
		pthread_mutex_init( &__xidp->r_lock, NULL);

		__xidp->p_head = NULL;
		__xidp->p_current = NULL;
		pthread_mutex_init( &__xidp->p_lock, NULL);

	
		__xidp->as				= &as;
		__xidp->as_connected 	= 0;
		
		

		app_xidp__init_config( json_config);
		printf("configuration initalized\n");
		
		__xidp->appLogger = app_logger__create( __xidp->logsFolder, __xidp->logsPrefix, ".log", 10000000, 5);

		app_xidp__connect();
		printf( "aerospike connected=%d\n", aerospike_cluster_is_connected( __xidp->as));

		__xidp->idpTree = app_rbnode__create_idp( __xidp->poolsize, __xidp->beginNo);
		app_rbnode__set_maxid( __xidp->idpTree, __xidp->endNo);

		
		xidp_info_t * xidp_info_item = NULL;
		__xidp->xidps  = (xidp_info_t **)malloc( __xidp->poolsize * sizeof(xidp_info_t*));
		uint8_t * dPtr = (uint8_t*)malloc( __xidp->poolsize * sizeof(xidp_info_t));

		int i = 0;
		for( i = 0; i < __xidp->poolsize; i++)
		{
			xidp_info_item = (xidp_info_t *)dPtr;
			
			if(!__xidp->p_head)
			{
				__xidp->p_head = __xidp->p_current = xidp_info_item;
			}
			else
			{
				__xidp->p_current->Next = xidp_info_item;
				__xidp->p_current = xidp_info_item;
			}
			
			xidp_info_item->imsi[0] 	= 0;
			xidp_info_item->status 		= 0;

			__xidp->xidps[i] = xidp_info_item;
			dPtr += sizeof(xidp_info_t);
		}

		app_logger__log( __xidp->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Started XIDP-Server  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		
		
		// uint64_t allocated_id = app_rbnode__get_next_idp( __xidp->idpTree, NULL);
		// app_rbnode_t * node = NULL;
		
		// if( allocated_id != UINT64_MAX)
		// {
			// uint64_t base_id = app_rbnode__get_baseid_idp( __xidp->idpTree, allocated_id);
			
			// uint64_t id = 0; 
			// uint32_t GenId = 0;
	
			// node = app_rbnode__find( __xidp->idpTree, base_id);
			// app_rbnode__get( node, &id, &GenId);
			
			// printf( "base_id=%lu allocated_id=%lu  id=%lu  GenId=%u  %u\n", base_id, allocated_id, id, GenId, __LINE__);
		// }
		
		
		// allocated_id = app_rbnode__get_next_idp( __xidp->idpTree, NULL);
		// node = NULL;
		
		// if( allocated_id != UINT64_MAX)
		// {
			// uint64_t base_id = app_rbnode__get_baseid_idp( __xidp->idpTree, allocated_id);
			
			// uint64_t id = 0; 
			// uint32_t GenId = 0;
	
			// node = app_rbnode__find( __xidp->idpTree, base_id);
			// app_rbnode__get( node, &id, &GenId);
			
			// printf( "base_id=%lu allocated_id=%lu  id=%lu  GenId=%u  %u\n", base_id, allocated_id, id, GenId, __LINE__);
		// }
		
		
		// exit(0);


		// INSERT INTO test.amfuengaiid ( PK, XIDP_XID, XIDP_GID,XIDP_IMSI) VALUES ( 10005, 10005, 1, '');
		// INSERT INTO test.tmsiid ( PK, XIDP_XID, XIDP_GID,XIDP_IMSI) VALUES ( 1001, 1001, 1, '');
		
		
		app_xidp__load_as_context();
		
		//if( iRecordCount == 0)
		// {
			// app_xidp__init_as_context();
		// }

		app_ep_stack__tcp_stack_init( app_ixdp__tcp_qhandler);
		app_ep_stack__tcp_stack_set_logger( __xidp->appLogger);
		app_ep_stack__start_server( __xidp->IP, __xidp->Port, APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK, 0, app_ixdp__tcp_read, 5000, 1024);
	
		if( __appAeroClient)
		{
			__appAeroClient->client = app_ep_stack__tcp_start_client( __appAeroClient->IP, __appAeroClient->Port, 
				APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK, 0, app_ixdp__tcp_read, 2000, 2048);
		}

		// printf( "__appAeroClient=%p client=%p\n", __appAeroClient, __appAeroClient->client);

		//exit(0);
		// app_aero__init_records();


		printf("init completed, iRecordCount=%ld  available=%lu total=%lu \n", 
				iRecordCount,
				app_rbnode__get_available( __xidp->idpTree),
				app_rbnode__get_total( __xidp->idpTree)
			);
		
		// xidp_message_t mess;
		
		while(1) 
		{
			// memset( &mess, 0, sizeof(xidp_message_t));
			// app_xidp__handle_request( &mess, NULL);

			usleep(999999);
		}

	}
	
	return 0;
}














