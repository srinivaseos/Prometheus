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

#include "app_aero.h"
#include "aero_message.h"
#include "app_endpoint.h"
#include "jansson.h"
#include "aero_tcp.h"
#include "app_jet_message.h"

app_logger_t * appLogger;
uint32_t RecordCount = 0;

void app_aero_message__set_logger( app_logger_t * logger)
{
	appLogger = logger;
}

void app_aero_message__send_ack( jet_message_buffer_t * msg_buf)
{
	if( msg_buf->id == 0) return;
	
	uint8_t buffer[108];
	memset( buffer, 0, sizeof(buffer));
	
	//printf( "id=%u ------- \n", msg_buf->id);
	
	jet_message_buffer_t * msgBuff = app_jet_message__init( msg_buf->id, buffer);
	app_jet_message__set_opc( msgBuff, 5);

	app_jet_message__set_len( msgBuff, msgBuff->pos);
	

	int sentBytes = app_ep_stack__send_message( (app_ep_stack__tcp_client_t*)msg_buf->transObj, msgBuff->buffer , msgBuff->pos);
	app_jet_message_buffer__release( msg_buf);
	
	if( appLogger)
	{
		app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "sent-message-ack  with bytes=%d pos=%d len=%d  %s|%s|%d", 
			sentBytes, msgBuff->pos, msgBuff->len, __FILE__, __FUNCTION__, __LINE__);	
	}
}


bool app_aero_message__scan_cb( const as_val * p_val, void * udata)
{
	jet_message_buffer_t * msg_buf = (jet_message_buffer_t *)udata;
	jet_message_t * mItem = (jet_message_t *)msg_buf->user_data_1;

	if (! p_val) 
	{
		return true;
	}

	
	as_record * p_rec = as_record_fromval( p_val);

	if (! p_rec) 
	{
		return true;
	}
	
	jet_element_t * eItem = mItem->eHead;
	
	uint8_t buffer[2048];
	memset( buffer, 0, sizeof(buffer));
	
	
	
	jet_message_buffer_t * record_item = app_jet_message__init( mItem->ID, buffer);



	uint32_t u16 = 0;
	uint32_t u32 = 0;
	uint64_t u64 = 0;
	char * data = NULL;
	as_string * as_str;
	as_bytes * as_byt;
	
	while( eItem)
	{
		//printf( "ID=%u  Name=%s  Type=%u  Length=%u   %s|%s|%d\n", eItem->ID, eItem->Name, eItem->Type, eItem->Length, __FILE__, __FUNCTION__, __LINE__);

		switch( eItem->Type)
		{
			case JET_ELEMENT_TYPE__UINT8:
				app_jet__add_u8( eItem, record_item, (uint8_t)as_integer_get( as_record_get_integer( p_rec, eItem->Name)));
				break;
			case JET_ELEMENT_TYPE__UINT16:
				app_jet__add_u16( eItem, record_item, (uint16_t)as_integer_get( as_record_get_integer( p_rec, eItem->Name)));
				break;
			case JET_ELEMENT_TYPE__UINT32:
				app_jet__add_u32( eItem, record_item, (uint32_t)as_integer_get( as_record_get_integer( p_rec, eItem->Name)));
				break;
			case JET_ELEMENT_TYPE__UINT64:
				app_jet__add_u64( eItem, record_item, as_record_get_int64( p_rec, eItem->Name, 0));
				break;
			case JET_ELEMENT_TYPE__CHAR:
				{
					as_str 	= as_record_get_string( p_rec, eItem->Name);
					
					if( as_str) {
						app_jet__add_char( eItem, record_item, as_str->value, as_str->len);
					}
				}
				break;
			case JET_ELEMENT_TYPE__BYTES:
				{
					// as_str 	= as_record_get_string( p_rec, eItem->Name);
					
					// if( as_str) {
						// app_jet__add_char( eItem, record_item, as_str->value, as_str->len);
					// }
					as_byt = as_record_get_bytes( p_rec, eItem->Name);
					
					if( as_byt) {
						unsigned char bA[as_byt->size + 1];
						as_bytes_copy( as_byt, 0, bA, as_byt->size);
						
						app_jet__add_char( eItem, record_item, bA, as_byt->size);
					}
				}
				break;
			default:
				break;
		}
		
		eItem = eItem->Next;
	}
	
	//printf( "---------------------------------------------------------------\n");

	record_item->len = record_item->pos;

	app_jet_message__set_opc( record_item, 4);
	app_jet_message__set_len( record_item, record_item->pos);

	
	int sentBytes = app_ep_stack__send_message( (app_ep_stack__tcp_client_t*)msg_buf->transObj, record_item->buffer , record_item->pos);
	app_jet_message_buffer__release( record_item);

	//printf( "sent-bytes=%d  pos=%d  len=%d    %s|%s|%d\n", sentBytes, record_item->pos, record_item->len, __FILE__, __FUNCTION__, __LINE__);
	
	if( appLogger)
	{
		app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "sent-message-data with bytes=%d len=%d  %s|%s|%d", 
			sentBytes, record_item->pos, __FILE__, __FUNCTION__, __LINE__);	
	}

	RecordCount++;
	return true;
}


void app_aero_message__selectone( jet_message_buffer_t * msg_buf)
{
	jet_message_t * mDef = app_jet__find_message( msg_buf->id);
	
	if(!mDef)
	{
		// send ACk-5
		app_aero_message__send_ack( msg_buf);
		app_jet_message_buffer__release( msg_buf);
		return;
	}
	
	msg_buf->user_data_1 = (uint8_t *)mDef;
	
	aerospike * as = app_aero__as_connection();
	as_error err;
	
	
	//jet_element_t * app_jet_message__get_element_head( int mId);
	
	// what needs to be selected, populate here
	// char* select[] = {"bin1", "bin2", "bin3", NULL};
 
	// as_key key;
	// as_key_init( &key, mDef->Namespace, mDef->Table, "key");
 
	// as_record * rec = NULL;
	// if (aerospike_key_select(&as, &err, NULL, &key, select, &rec) != AEROSPIKE_OK) 
	// {
		// printf("error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
	// }
	// else 
	// {
		// as_record_destroy(rec);
	// }

}

void app_aero_message__selectall( jet_message_buffer_t * msg_buf)
{
	jet_message_t * mDef = app_jet__find_message( msg_buf->id);
	
	if(!mDef)
	{
		// send ACk-5
		app_aero_message__send_ack( msg_buf);
		app_jet_message_buffer__release( msg_buf);
		return;
	}
	
	RecordCount = 0;
	msg_buf->user_data_1 = (uint8_t *)mDef;
	
	//printf( "id=%u ID=%u  %s|%s|%d\n", msg_buf->id, mDef->ID, __FILE__, __FUNCTION__, __LINE__);

	
	
	aerospike * as = app_aero__as_connection();
	as_error err;

	as_scan scan;
	as_scan_init( &scan, mDef->Namespace, mDef->Table);
	

	if (aerospike_scan_foreach( as, &err, NULL, &scan, app_aero_message__scan_cb, msg_buf) != AEROSPIKE_OK) 
	{
		printf("aerospike_scan_foreach() returned %d - %s\n", err.code, err.message);
		as_scan_destroy( &scan);
		app_aero__reset();
		return;
	}

	if( appLogger)
	{
		app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "scan-completed with record-count=%d  %s|%s|%d", RecordCount, __FILE__, __FUNCTION__, __LINE__);	
	}
	
	app_aero_message__send_ack( msg_buf);
	app_jet_message_buffer__release( msg_buf);
	return;	
}


void app_aero_message__execute( jet_message_buffer_t * msg_buf)
{
	//printf( "opc=%d  %s|%s|%d\n", msg_buf->opc, __FILE__, __FUNCTION__, __LINE__);

	if( appLogger)
	{
		app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "received-message-data  id=%d  opc=%d  %s|%s|%d", 
			msg_buf->id, msg_buf->opc, __FILE__, __FUNCTION__, __LINE__);	
	}
	
	if( msg_buf->opc == 3)
	{
		app_aero_message__selectall( msg_buf);
		return;
	}
	else if( msg_buf->opc == 4)
	{
		app_aero_message__selectone( msg_buf);
		return;
	}
	else if( msg_buf->opc != 1 && msg_buf->opc != 2)
	{
		// 1 - insertOrUpdate, 2 - Remove
		app_jet_message_buffer__release( msg_buf);
		return;
	}
	
	
	jet_message_t * mDef = app_jet__find_message( msg_buf->id);

	//printf( "mDef=%p  id=%u  %s|%s|%d\n", mDef, msg_buf->id, __FILE__, __FUNCTION__, __LINE__);
	
	
	if( mDef)
	{
		//printf("Namespace=%s set=%s\n", mDef->Namespace, mDef->Table); 
		
		aerospike 	* as = app_aero__as_connection();
		as_error 	err;
		as_key 		key;

		
		uint32_t u32 	= 0;
		uint32_t u64 	= 0;
		char * cVal 	= NULL;
		uint32_t cLen 	= 0;

		int recCount = 0;
		msg_buf->pos = 12;

		jet_element_data_t edata;
		jet_element_t * element = NULL;


		// app_printf_buf( "", msg_buf->buffer, msg_buf->len);
		

		while( msg_buf->pos < msg_buf->len)
		{
			app_jet_message__decode_element_data( msg_buf, &edata);
			element = app_jet_message__find_element_by_id( edata.ID, mDef);
			
			//printf( "Id=%u element=%p  %s|%s|%d\n", edata.ID, element, __FILE__, __FUNCTION__, __LINE__);

			
			if(element)
			{
				recCount++;
			}
		}
		
		
		//printf( "eCount=%d  %s|%s|%d\n", recCount, __FILE__, __FUNCTION__, __LINE__);
		
		if( recCount == 0)
		{
			app_jet_message_buffer__release( msg_buf);
			return;
		}
		
		
		
		as_record rec;
		as_record_inita( &rec, recCount);
		
		
		as_bytes b[50];
		int bCounter = 0;
		
		msg_buf->pos = 12;
		
		while( msg_buf->pos < msg_buf->len)
		{
			app_jet_message__decode_element_data( msg_buf, &edata);
			element = app_jet_message__find_element_by_id( edata.ID, mDef);
			
			if( element)
			{
				//printf( "Name=%s  IsKey=%d  %s|%s|%d\n", element->Name, element->IsKey, __FILE__, __FUNCTION__, __LINE__);
				
				
				switch( element->Type)
				{
					case JET_ELEMENT_TYPE__UINT8:
					case JET_ELEMENT_TYPE__UINT16:
					case JET_ELEMENT_TYPE__UINT32:
						{
							u32 = edata.Value;
							as_record_set_integer( 	&rec, 		element->Name, 	as_integer_new(u32));
							// printf( "  as  added uint32=%u  %s|%s|%d\n", u32, __FILE__, __FUNCTION__, __LINE__);
						}
						break;
					case JET_ELEMENT_TYPE__UINT64:
						{
							u64 = edata.Value;

							if( element->IsKey == 1) {
								as_key_init_int64( &key, mDef->Namespace, mDef->Table, u64);
							} else {
								as_record_set_int64( 	&rec, 		element->Name, 	u64);
							}
							
							// printf( "  as  added uint64=%u  %s|%s|%d\n", u64, __FILE__, __FUNCTION__, __LINE__);
						}
						break;
					case JET_ELEMENT_TYPE__CHAR:
						{
							cVal = edata.CData;

							if( element->IsKey == 1) {
								as_key_init_str( 		&key, 		mDef->Namespace, 	mDef->Table, cVal);
								//printf("NS=%s T=%s VAL=%s\n", mDef->Namespace, mDef->Table, cVal);
							} else {
								as_record_set_str( 		&rec, 		element->Name, 		cVal);
							}
							
							// printf( "  as  added char*=%s len=%ld  %s|%s|%d\n", cVal, strlen(cVal), __FILE__, __FUNCTION__, __LINE__);
						}
						break;
					case JET_ELEMENT_TYPE__BYTES:
						{
							if( bCounter < 50)
							{
								as_bytes_init( &b[bCounter], element->Length);
								as_bytes_set( &b[bCounter], 0, edata.CData, element->Length);
								
								as_record_set_bytes( &rec, element->Name, &b[bCounter]);
								
								bCounter++;
							}
							else
							{
								printf("MAX byte bin exausted\n");
								exit(0);
							}
							// app_printf_buf( element->Name, edata.CData, element->Length);
							// printf( "save bytes -> Name=%s  Len=%d  %s|%s|%d\n", element->Name, element->Length, __FILE__, __FUNCTION__, __LINE__);
							
							/*
								reading
								as_bytes * b;
								uint8_t * res;
								b = as_record_get_bytes(rec, bin_name);
								res = malloc((b->size)*sizeof(uint8_t));
								res_size = as_bytes_copy(b, 0, res, b->size);
							*/
						}
						break;
					default:
						break;
				}
			}
			// printf("ID=%d Type=%u Length=%u Val=%lu CVal=%s\n", edata.ID, edata.Type, edata.Length, edata.Value, edata.CData);
		}

		//printf("opc=%d ", msg_buf->opc);
		//return;

		//printf("executing \n");
		

		if( msg_buf->opc == 2)
		{
			//printf("  aerospike_key_remove \n");

			if( appLogger)
			{
				app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "deleted-message-data  id=%d  opc=%d  %s|%s|%d", 
					msg_buf->id, msg_buf->opc, __FILE__, __FUNCTION__, __LINE__);	
			}
	
			if( aerospike_key_remove( as, &err, NULL, &key) != AEROSPIKE_OK) 
			{
				printf("remove failed., error(%d) %s at [%s:%d]", err.code, err.message, err.file, err.line);
			}
		}
		else
		{
			//printf("  aerospike_key_put \n");


			if( appLogger)
			{
				app_logger__log( appLogger, NULL, APP_LOG__LEVEL_DEBUG, "saved-data  id=%d  opc=%d  %s|%s|%d", 
					msg_buf->id, msg_buf->opc, __FILE__, __FUNCTION__, __LINE__);	
			}
			
			if ( aerospike_key_put( as, &err, NULL, &key, &rec) != AEROSPIKE_OK) 
			{
				printf("put failed., error(%d) %s at [%d]", err.code, err.message, err.line);
			}
		}
		
		//printf("operation=%d completed, with error(%d) %s at [%s:%d]\n", msg_buf->opc, err.code, err.message, err.file, err.line);
		app_jet_message_buffer__release( msg_buf);
		
		//exit(0);
	}
}







bool scan_cb( const as_val * p_val, void * udata)
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
	
	jet_message_t * mItem = (jet_message_t *)udata;
	jet_element_t * eItem = mItem->eHead;
	
	uint8_t buffer[2048];
	memset( buffer, 0, 2048);
	
	jet_message_buffer_t * msg_buf = app_jet_message__init( mItem->ID, buffer);

	
	uint32_t u32 = 0;
	uint64_t u64 = 0;
	char * data = NULL;
	
	
	while( eItem)
	{
		switch( eItem->Type)
		{
			case 1:
				u32 	= as_integer_get( as_record_get_integer( p_rec, eItem->Name));
				app_jet_message__add_U32( msg_buf, u32);
				break;
			case 2:
				u64 	= as_record_get_int64( p_rec, eItem->Name, 0);
				app_jet_message__add_U64( msg_buf, u64);
				break;
			case 3:
				data 	= as_record_get_str( p_rec, eItem->Name);
				
				if( data)
				{
					app_jet_message__add_char( msg_buf, data, strlen(data), eItem->Length);
				}
				break;
			default:
				break;
		}
		
		eItem = eItem->Next;
	}
	
	
	int sentBytes = app_ep_stack__send_message( NULL, buffer , msg_buf->pos);
	app_jet_message_buffer__release( msg_buf);
	
	RecordCount++;
	return true;
}



void app_aero_message__restore_set( jet_message_t * mItem)
{
	aerospike * as = app_aero__as_connection();
	as_error err;

	as_scan scan;
	as_scan_init( &scan, mItem->Namespace, mItem->Table);
	
	if (aerospike_scan_foreach( as, &err, NULL, &scan, scan_cb, mItem) != AEROSPIKE_OK) 
	{
		printf("aerospike_scan_foreach() returned %d - %s\n", err.code, err.message);
		as_scan_destroy( &scan);
		app_aero__reset();
		return;
	}
	
	//printf("Restored Aerospike Sessions count=%d %s|%s|%d\n", RecordCount, __FUNCTION__, __FILE__, __LINE__);
}




void app_aero_message__restore()
{
	jet_message_t * mItem = app_jet_message__getHead();
	
	while( mItem)
	{
		app_aero_message__restore_set( mItem);
		mItem = app_jet_message__getNext( mItem);
	}	
}















