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

#ifndef S_APP_JET_M
#define S_APP_JET_M

#include "jansson.h"
#include "aero_tcp.h"

#define JET_ELEMENT_TYPE__UINT8			1
#define JET_ELEMENT_TYPE__UINT16		2
#define JET_ELEMENT_TYPE__UINT32		3
#define JET_ELEMENT_TYPE__UINT64		4
#define JET_ELEMENT_TYPE__CHAR			5
#define JET_ELEMENT_TYPE__BYTES			6


#pragma pack(4)
typedef struct jet_message_buffer
{
	struct jet_message_buffer * Next;

	uint32_t len;
	uint32_t id;
	uint32_t opc;
	uint8_t * buffer;

	uint8_t * user_data_1;
	uint8_t * transObj;

	int pos;
} jet_message_buffer_t;


#pragma pack(4)
typedef struct jet_element
{
	struct jet_element * Next;
	
	uint32_t ID;
	uint32_t Type;
	uint32_t IsKey;
	char Name[20];
	int Length;
	
} jet_element_t;

#pragma pack(4)
typedef struct jet_element_data
{
	uint16_t ID;
	uint16_t Type;	
	uint16_t Length;
	uint64_t Value;
	unsigned char * CData;
} jet_element_data_t;

// #pragma pack(4)
// typedef struct jet_element_pval
// {
	// //uint16_t isset;
	// uint64_t uval;
	// char cval[256];
// } jet_element_pval_t;

// #pragma pack(4)
// typedef struct jet_element_val
// {
	// //uint16_t isset;
	// uint64_t uval;
	// unsigned char * cval;
// } jet_element_val_t;

#pragma pack(4)
typedef struct jet_message
{
	struct jet_message * Next;
	
	uint32_t ID;
	char Namespace[20];
	char Table[20];
	
	jet_element_t * eHead;
	jet_element_t * eCurrent;
	int eCount;
	
} jet_message_t;


void app_jet__init( json_t * jMessages, int validateKeyInRecord);
uint32_t app_jet_message__calc_len( int mId);
jet_message_t * app_jet__find_message( uint32_t id);
jet_message_buffer_t * app_jet_message_buffer__allocate();
void app_jet_message_buffer__release( jet_message_buffer_t *);;
jet_message_buffer_t * app_jet_message__init( int mId, uint8_t * buffer);
void app_jet_message__add_U16( jet_message_buffer_t * msg, uint16_t u16);
void app_jet_message__add_U32( jet_message_buffer_t * msg, uint32_t u32);
void app_jet_message__add_U64( jet_message_buffer_t * msg, uint64_t u64);
void app_jet_message__add_char( jet_message_buffer_t * msg, char * str, int istrlen, int itemlen);
uint32_t app_jet_message__get_u32( jet_message_buffer_t * msg);
uint64_t app_jet_message__get_u64( jet_message_buffer_t * msg);
jet_message_buffer_t * app_jet_message__decode( app_ep_stack__tcp_buffer_t * tcp_buffer);
jet_message_buffer_t * app_jet_message__decode_buffer( unsigned char * buffer);

jet_message_t * app_jet_message__getHead();
jet_message_t * app_jet_message__getNext( jet_message_t * jM);

void app_jet__add_u8(jet_element_t * element, jet_message_buffer_t * msgBuff, uint8_t u8);
void app_jet__add_u16(  jet_element_t * element, jet_message_buffer_t * msgBuff, uint16_t u16);
void app_jet__add_u32(  jet_element_t * element, jet_message_buffer_t * msgBuff, uint32_t u32);
void app_jet__add_u64(  jet_element_t * element, jet_message_buffer_t * msgBuff, uint64_t u64);
void app_jet__add_char( jet_element_t * element, jet_message_buffer_t * msgBuff, unsigned char * data, int dlen);

void app_jet_message__logstats(  app_logger_t * plogger);
jet_element_t * app_jet_message__get_element_head( int mId);
jet_element_t * app_jet_message__get_element_next( jet_element_t * element);
jet_element_t * app_jet_message__find_element_by_id( uint32_t id, jet_message_t * jmsg);

void app_jet_message__set_len( jet_message_buffer_t * msg, uint32_t len);
void app_jet_message__set_opc( jet_message_buffer_t * msg, uint32_t opc);

void app_jet_message__decode_element_data( jet_message_buffer_t * msgBuff, jet_element_data_t * edata);

#endif