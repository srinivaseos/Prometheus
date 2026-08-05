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
#include "app_jet_message.h"
#include "app_endpoint.h"
#include "aero_tcp.h"

int app_jet__json_get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

char * app_jet__json_get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}




void app_jet__json_set_str( json_t * json_config, char * key, char * vItem, int maxlen)
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


#pragma pack(4)
typedef struct app_jetm
{
	jet_message_t * mHead;
	jet_message_t * mCurrent;
	int mCount;
	
	jet_message_buffer_t * mbHead;
	jet_message_buffer_t * mbCurrent;
	pthread_mutex_t mbLock;
	int mbAvailable;
	int mbTotal;	
} app_jetm_t;

app_jetm_t * __appJetM = NULL;




jet_message_t * app_jet__add_message( uint32_t id, char * tName, char * ns)
{
	jet_message_t * msg = (jet_message_t*)malloc(sizeof(jet_message_t));
	memset( msg, 0, sizeof(jet_message_t));
	
	msg->ID = id;
	strcpy( msg->Table, tName);

	if( ns)
	{
		strcpy( msg->Namespace, ns);
	}

	
	if(!__appJetM->mHead)
	{
		__appJetM->mHead = __appJetM->mCurrent = msg;
	}
	else
	{
		__appJetM->mCurrent->Next = msg;
		__appJetM->mCurrent = msg;
	}
	
	__appJetM->mCount++;
	return msg;
}

jet_message_t * app_jet__find_message( uint32_t id)
{
	jet_message_t * mItem = __appJetM->mHead;
	
	while( mItem)
	{
		if( mItem->ID == id)
		{
			return mItem;
		}
		
		mItem = mItem->Next;
	}
	
	//app_logger__log( __appAero->appLogger, NULL, APP_LOG__LEVEL_DEBUG, "message-defination not found with id=%d   %s|%s|%d", id, __FILE__, __FUNCTION__, __LINE__);
	return NULL;
}


void app_jet__init( json_t * jMessages, int validateKeyInRecord)
{
	if(!__appJetM)
	{
		__appJetM = (app_jetm_t *)malloc(sizeof(app_jetm_t));
		memset( __appJetM, 0, sizeof(app_jetm_t));
	}
	
	
	if(json_is_array(jMessages))
	{
		int j = 0;
		int aipc = json_array_size(jMessages);
		
		json_t 	* mItem = NULL;
		uint32_t mID = 0;
		char 	* mTableName = NULL;
		char 	* mNamespace = NULL;
		char 	* mKey = NULL;
		jet_message_t * msg = NULL;
		
		json_t 	* elements = NULL;
		json_t 	* eItem = NULL;
		int ei = 0;
		int ec = 0;
		char * eName = NULL;
		char * eType = NULL;
		int eLen = 0;
		int ID = 0;
		
		jet_element_t * mEle = NULL;
		
		printf("\n");
		
		for( j = 0; j < aipc; j++)
		{
			mItem = json_array_get( jMessages, j);
			
			if( mItem)
			{
				mID 		= app_jet__json_get_int( mItem, "ID", 0);
				mNamespace 	= app_jet__json_get_str( mItem, "Namespace");
				mTableName 	= app_jet__json_get_str( mItem, "Name");
				mKey 		= app_jet__json_get_str( mItem, "Key");
			
				if(mID == 0)
				{
					printf("Messages-ID is Not Found\n");
					exit(0);
				}

				if(!mTableName)
				{
					printf("Messages-TableName is NULL\n");
					exit(0);
				}
			
				if(!mKey && validateKeyInRecord == 1)
				{
					printf("Messages-Key is NULL\n");
					exit(0);
				}

				printf( "Messages  ID=%d Namespace=%s Name=%s Key=%s\n", mID, mNamespace, mTableName, mKey);
				
				msg = app_jet__add_message( mID, mTableName, mNamespace);
				
				if( msg)
				{
					elements = json_object_get( mItem, "Elements");
					
					if( elements)
					{
						ec = json_array_size( elements);
						
						for( ei = 0; ei < ec; ei++)
						{
							eItem = json_array_get( elements, ei);
							
							if( eItem)
							{
								eName 	= app_jet__json_get_str( eItem, "Name");
								eType 	= app_jet__json_get_str( eItem, "Type");
								eLen 	= app_jet__json_get_int( eItem, "Length", 0);
								ID 		= app_jet__json_get_int( eItem, "ID", 0);
								
								
								if( eName && eType)
								{
									mEle = (jet_element_t*)malloc(sizeof(jet_element_t));
									memset( mEle, 0, sizeof( jet_element_t));
									
									if( mKey)
									{
										if( strcmp( eName, mKey) == 0)
										{
											mEle->IsKey = 1;
										}
										else
										{
											mEle->IsKey = 0;
										}
									}
									
									if( strcmp( eType, "UInt8") == 0)
									{
										mEle->Type = 1;
										mEle->Length = 1;
									}
									else if( strcmp( eType, "UInt16") == 0)
									{
										mEle->Type = 2;
										mEle->Length = 2;
									}
									else if( strcmp( eType, "UInt32") == 0)
									{
										mEle->Type = 3;
										mEle->Length = 4;
									}
									else if( strcmp( eType, "UInt64") == 0)
									{
										mEle->Type = 4;
										mEle->Length = 8;
									}
									else if( strcmp( eType, "CHAR") == 0)
									{
										mEle->Type = 5;
										mEle->Length = eLen;
									}
									else if( strcmp( eType, "BYTES") == 0)
									{
										mEle->Type = 6;
										mEle->Length = eLen;
									}
									else
									{
										printf( "unknown data type=%s\n", eType);
										exit(0);
									}
									
									mEle->ID = ID;
									
									if(!msg->eHead)
									{
										msg->eHead = msg->eCurrent = mEle;
									}
									else
									{
										msg->eCurrent->Next = mEle;
										msg->eCurrent = mEle;
									}
									msg->eCount++;
									
									
									app_jet__json_set_str( eItem, "Name", mEle->Name, 20);
									printf( "     Name=%-20s|%-20s Type=%-20s|%-4d Len=%-10d\n", eName, mEle->Name, eType, mEle->Type, mEle->Length);
								}
								
							}
						}
					}
				}
				
				
			}
		}
		
		printf("\n");

		__appJetM->mbHead = NULL;
		__appJetM->mbCurrent = NULL;
		pthread_mutex_init( &__appJetM->mbLock, NULL);
		
		__appJetM->mbAvailable = 0;
		__appJetM->mbTotal = 0;
		
		
		uint8_t * buffer = (uint8_t *)malloc( sizeof(jet_message_buffer_t) * 2000);
		
		if( buffer)
		{
			int j = 0;
			jet_message_buffer_t * mbuff = NULL;
			
			for( j = 0; j < 2000; j++)
			{
				mbuff = (jet_message_buffer_t *)buffer;
				
				if(!__appJetM->mbHead)
				{
					__appJetM->mbHead = __appJetM->mbCurrent = mbuff;
				}
				else
				{
					__appJetM->mbCurrent->Next = mbuff;
					__appJetM->mbCurrent = mbuff;
				}
				
				__appJetM->mbAvailable++;
				__appJetM->mbTotal++;
		
				buffer += sizeof(jet_message_buffer_t);
			}
		}
	
	}
	else
	{
		printf("Messages should be an array\n");
		exit(0);
	}
}


void app_jet_message__logstats(  app_logger_t * plogger)
{
	if( __appJetM)
	{
		app_logger__log( plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Jet Message   Available=%lu  Total=%lu", __appJetM->mbAvailable, __appJetM->mbTotal);
	}	
}


jet_element_t * app_jet_message__get_element_head( int mId)
{
	jet_message_t * mDef = app_jet__find_message( mId);
	
	if( mDef)
	{
		return mDef->eHead;
	}
	
	return NULL;
}

jet_element_t * app_jet_message__get_element_next( jet_element_t * element)
{
	return element->Next;
}

jet_element_t * app_jet_message__find_element_by_id( uint32_t id, jet_message_t * jmsg)
{
	if( jmsg)
	{
		jet_element_t * element = jmsg->eHead;
		
		while( element)
		{
			if( element->ID == id)
			{
				return element;
			}
			element = element->Next;
		}
	}
	return NULL;
}


uint32_t app_jet_message__calc_len( int mId)
{
	jet_message_t * mDef = app_jet__find_message( mId);
	
	if( mDef)
	{
		uint32_t len = 12;
		
		jet_element_t * eItem = mDef->eHead;
	
		while( eItem)
		{
			switch( eItem->Type)
			{
				case JET_ELEMENT_TYPE__UINT8:			// UInt8
					len += 2;							// 2 ID
					len += 2;							// 2 Type
					len += 2;							// 2 Length
					len += 1;							// 1 VALUE
					break;
				case JET_ELEMENT_TYPE__UINT16:			// UInt16
					len += 2;							// 2 ID
					len += 2;							// 2 Type
					len += 2;							// 2 Length
					len += 2;							// 1 VALUE
					break;
				case JET_ELEMENT_TYPE__UINT32:			// UInt32
					len += 2;							// 2 ID
					len += 2;							// 2 Type
					len += 2;							// 2 Length
					len += 4;							// 4 VALUE
					break;
				case JET_ELEMENT_TYPE__UINT64:			// UInt64
					len += 2;							// 2 ID
					len += 2;							// 2 Type
					len += 2;							// 2 Length
					len += 8;							// 8 VALUE
					break;
				case JET_ELEMENT_TYPE__CHAR:			// CHAR
					len += 2;							// 2 ID
					len += 2;							// 2 Type
					len += 2;							// 2 Length
					len += eItem->Length;				// VALUE
					break;
				default:
					break;
			}
			
			eItem = eItem->Next;
		}
		
		return len;
	}
	
	return 0;
}


jet_message_buffer_t * app_jet_message_buffer__allocate()
{
	jet_message_buffer_t * buffer = NULL;
	pthread_mutex_lock( &__appJetM->mbLock);
	
	buffer = __appJetM->mbHead;
	
	if(buffer)
	{
		__appJetM->mbHead = buffer->Next;
		buffer->Next = NULL;
		__appJetM->mbAvailable--;
	}
	
	pthread_mutex_unlock( &__appJetM->mbLock);
	return buffer;
}

void app_jet_message_buffer__release( jet_message_buffer_t * buffer)
{
	pthread_mutex_lock( &__appJetM->mbLock);

	buffer->buffer 		= NULL;

	buffer->len 			= 0;
	buffer->id 				= 0;
	buffer->opc 			= 0;

	
	if(!__appJetM->mbHead)
	{
		__appJetM->mbHead = __appJetM->mbCurrent = buffer;
	}
	else
	{
		__appJetM->mbCurrent->Next = buffer;
		__appJetM->mbCurrent = buffer;
	}
	__appJetM->mbAvailable++;
	
	pthread_mutex_unlock( &__appJetM->mbLock);
}

jet_message_buffer_t * app_jet_message__init( int mId, uint8_t * buffer)
{
	uint32_t mLen = app_jet_message__calc_len( mId);
	
	if( mLen == 0) return NULL;
	
	
	jet_message_buffer_t * msg = app_jet_message_buffer__allocate();

	msg->id 	= mId;
	msg->opc 	= 1;
	msg->len 	= mLen;
	msg->buffer = buffer;
	msg->pos = 0;
	
	
	app_ep__encode__u32toc( &msg->buffer[msg->pos], msg->len);
	msg->pos = 4;
	
	app_ep__encode__u32toc( &msg->buffer[msg->pos], mId);
	msg->pos = 8;
	
	app_ep__encode__u32toc( &msg->buffer[msg->pos], 1);
	msg->pos = 12;
	
	//length, message, operation
	
	return msg;
}


void app_jet_message__set_opc( jet_message_buffer_t * msg, uint32_t opc)
{
	msg->opc = opc;
	app_ep__encode__u32toc( &msg->buffer[8], opc);
}


void app_jet_message__set_len( jet_message_buffer_t * msg, uint32_t len)
{
	msg->len = len;
	app_ep__encode__u32toc( &msg->buffer[0], len);
}


void app_jet_message__add_U8( jet_message_buffer_t * msg, uint8_t u8)
{
	msg->buffer[msg->pos] = u8;
	msg->pos += 1;
}

void app_jet_message__add_U16( jet_message_buffer_t * msg, uint16_t u16)
{
	app_ep__encode__u16toc( &msg->buffer[msg->pos], u16);
	msg->pos += 2;
}

void app_jet_message__add_U32( jet_message_buffer_t * msg, uint32_t u32)
{
	app_ep__encode__u32toc( &msg->buffer[msg->pos], u32);
	msg->pos += 4;
}

void app_jet_message__add_U64( jet_message_buffer_t * msg, uint64_t u64)
{
	app_ep__encode__u64toc( &msg->buffer[msg->pos], u64);
	msg->pos += 8;
}

void app_jet_message__add_char( jet_message_buffer_t * msg, char * str, int istrlen, int itemlen)
{
	memcpy( &msg->buffer[msg->pos], str, istrlen);
	msg->pos += itemlen;
}

uint32_t app_jet_message__get_u32( jet_message_buffer_t * msg)
{
	uint32_t u32 = app_ep__get_u32( &msg->buffer[msg->pos]);
	msg->pos += 4;
	return u32;
}

uint64_t app_jet_message__get_u64( jet_message_buffer_t * msg)
{
	uint64_t u64 = app_ep__get_u64( &msg->buffer[msg->pos]);
	msg->pos += 8;
	return u64;
}


jet_message_buffer_t * app_jet_message__decode( app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	jet_message_buffer_t * mbuff = app_jet_message_buffer__allocate();
	
	mbuff->transObj		= (uint8_t*)tcp_buffer->client;
	mbuff->pos			= 0;
	mbuff->buffer 		= tcp_buffer->buffer;

	mbuff->len 			= app_jet_message__get_u32( mbuff);
	mbuff->id 			= app_jet_message__get_u32( mbuff);
	mbuff->opc 			= app_jet_message__get_u32( mbuff);
	
	return mbuff;
}




jet_message_buffer_t * app_jet_message__decode_buffer( unsigned char * buffer)
{
	jet_message_buffer_t * mbuff = app_jet_message_buffer__allocate();
	
	mbuff->pos			= 0;
	mbuff->buffer 		= buffer;

	mbuff->len 			= app_jet_message__get_u32( mbuff);
	mbuff->id 			= app_jet_message__get_u32( mbuff);
	mbuff->opc 			= app_jet_message__get_u32( mbuff);
	
	return mbuff;
}

jet_message_t * app_jet_message__getHead()
{
	return __appJetM->mHead;
}


jet_message_t * app_jet_message__getNext( jet_message_t * jM)
{
	return jM->Next;
}

void app_jet__add_u8(jet_element_t * element, jet_message_buffer_t * msgBuff, uint8_t u8)
{
	app_jet_message__add_U16( msgBuff, element->ID);
	app_jet_message__add_U16( msgBuff, JET_ELEMENT_TYPE__UINT8);
	app_jet_message__add_U16( msgBuff, 1);
	app_jet_message__add_U8( msgBuff, u8);
}

void app_jet__add_u16(jet_element_t * element, jet_message_buffer_t * msgBuff, uint16_t u16)
{
	app_jet_message__add_U16( msgBuff, element->ID);
	app_jet_message__add_U16( msgBuff, JET_ELEMENT_TYPE__UINT16);
	app_jet_message__add_U16( msgBuff, 2);
	app_jet_message__add_U16( msgBuff, u16);
}

void app_jet__add_u32(jet_element_t * element, jet_message_buffer_t * msgBuff, uint32_t u32)
{
	app_jet_message__add_U16( msgBuff, element->ID);
	app_jet_message__add_U16( msgBuff, JET_ELEMENT_TYPE__UINT32);
	app_jet_message__add_U16( msgBuff, 4);
	app_jet_message__add_U32( msgBuff, u32);
}

void app_jet__add_u64(jet_element_t * element, jet_message_buffer_t * msgBuff, uint64_t u64)
{
	app_jet_message__add_U16( msgBuff, element->ID);
	app_jet_message__add_U16( msgBuff, JET_ELEMENT_TYPE__UINT64);
	app_jet_message__add_U16( msgBuff, 8);
	app_jet_message__add_U64( msgBuff, u64);
}

void app_jet__add_char(jet_element_t * element, jet_message_buffer_t * msgBuff, unsigned char * data, int dlen)
{
	if( dlen == 0) return;
	
	// if( dlen > element->Length)
	// {
		// printf( "encoding failed\n");
		// return;
	// }
	
	app_jet_message__add_U16( msgBuff, element->ID);
	app_jet_message__add_U16( msgBuff, JET_ELEMENT_TYPE__CHAR);
	//app_jet_message__add_U16( msgBuff, element->Length);
	//app_jet_message__add_char( msgBuff, data, dlen, element->Length);
	
	//printf( "ID=%d Len=%d dle=%d   %s|%d\n", element->ID, element->Length, dlen, __FILE__, __LINE__);
	
	if( dlen > element->Length)
	{
		app_jet_message__add_U16( msgBuff, element->Length);
		app_jet_message__add_char( msgBuff, data, element->Length, element->Length);
	}
	else
	{
		app_jet_message__add_U16( msgBuff, dlen);
		app_jet_message__add_char( msgBuff, data, dlen, dlen);
	}
}


uint8_t app_jet_message__decode_u8( jet_message_buffer_t * msgBuff)
{
	uint8_t u8 = msgBuff->buffer[msgBuff->pos];
	msgBuff->pos += 1;
	return u8;
}

uint16_t app_jet_message__decode_u16( jet_message_buffer_t * msgBuff)
{
	uint16_t u16 = app_ep__get_u16( &msgBuff->buffer[msgBuff->pos]);
	msgBuff->pos += 2;
	return u16;
}

uint32_t app_jet_message__decode_u32( jet_message_buffer_t * msgBuff)
{
	uint32_t u32 = app_ep__get_u32( &msgBuff->buffer[msgBuff->pos]);
	msgBuff->pos += 4;
	return u32;
}

uint64_t app_jet_message__decode_u64( jet_message_buffer_t * msgBuff)
{
	uint64_t u64 = app_ep__get_u64( &msgBuff->buffer[msgBuff->pos]);
	msgBuff->pos += 8;
	return u64;
}

char * app_jet_message__decode_char( jet_message_buffer_t * msgBuff, int ilen)
{
	char * data = (char *)&msgBuff->buffer[msgBuff->pos];
	msgBuff->pos += ilen;
	return data;
}


void app_jet_message__decode_element_data( jet_message_buffer_t * msgBuff, jet_element_data_t * edata)
{
	edata->ID 			= app_jet_message__decode_u16( msgBuff);
	edata->Type			= app_jet_message__decode_u16( msgBuff);
	edata->Length		= app_jet_message__decode_u16( msgBuff);
	edata->Value 		= 0;
	edata->CData 		= NULL;
	
	switch( edata->Type)
	{
		case JET_ELEMENT_TYPE__UINT8:
			edata->Value = app_jet_message__decode_u8( msgBuff);
			break;
		case JET_ELEMENT_TYPE__UINT16:
			edata->Value = app_jet_message__decode_u16( msgBuff);
			break;
		case JET_ELEMENT_TYPE__UINT32:
			edata->Value = app_jet_message__decode_u32( msgBuff);
			break;
		case JET_ELEMENT_TYPE__UINT64:
			edata->Value = app_jet_message__decode_u64( msgBuff);
			break;
		case JET_ELEMENT_TYPE__CHAR:
		case JET_ELEMENT_TYPE__BYTES:
			edata->CData = app_jet_message__decode_char( msgBuff, edata->Length);
			break;
		default:
			msgBuff->pos += edata->Length;
			break;
	}
}