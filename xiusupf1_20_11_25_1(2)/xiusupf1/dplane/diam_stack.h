#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef S_DIAM_STACK_ADV
#define S_DIAM_STACK_ADV

#include "app_stack.h"
#include "app_endpoint.h"


#define JET_DIAM_STACK__OCTET_STRING 			1
#define JET_DIAM_STACK__SIG_NUMBER 				2
#define JET_DIAM_STACK__UNSIG_NUMBER 			3
#define JET_DIAM_STACK__GROUPED 				4

















#pragma pack(4)
typedef struct jet_diam_dict_avp
{
	struct jet_diam_dict_avp * Next;
	
	uint32_t 	AVPCode;
	uint32_t 	DataType;
	uint32_t 	VendorId;
	uint8_t 	Flags;
	
	char * cfixedValue;
	uint64_t ufixedValue;
	
	struct jet_diam_dict_avp * AVPHead;
	struct jet_diam_dict_avp * AVPCurrent;
	uint32_t AVPCount;
} jet_diam_dict_avp_t;


#pragma pack(4)
typedef struct jet_diam_dict_message
{
	struct jet_diam_dict_message * Next;
	
	uint32_t ApplicationId;
	uint32_t CommandCode;
	uint8_t Flag;
	
	
	jet_diam_dict_avp_t * AVPHead;
	jet_diam_dict_avp_t * AVPCurrent;
	uint32_t AVPCount;
} jet_diam_dict_message_t;


#pragma pack(4)
typedef struct jet_diam_vendor_app
{
	uint32_t VendId;
	uint32_t AppId;
} jet_diam_vendor_app_t;


#pragma pack(4)
typedef struct jet_diam_avp
{
	struct jet_diam_avp * Next;

	int AvpCode;
	int AvpLength;
	int Padding;
	int PayLoadLength;
	int HeaderLength;
	int VendorId;
	int DataType;
	uint8_t Flags;
	uint8_t * data;


	pthread_mutex_t AVPLock;
	struct jet_diam_avp * avpHead;
	struct jet_diam_avp * avpCurrent;
	uint32_t avpCount;

} jet_diam_avp_t;


#pragma pack(4)
typedef struct jet_diam_message
{
	uint32_t AppId;
	uint32_t CmdCode;
	uint32_t HBHId;
	uint32_t E2EId;
	uint8_t Flags;
	
	pthread_mutex_t AVPLock;
	jet_diam_avp_t * avpHead;
	jet_diam_avp_t * avpCurrent;
	uint32_t avpCount;
	uint32_t avpCount2;
	uint32_t avpCount3;
	uint32_t Receiver;
	uint8_t * ReceiverObj;

} jet_diam_message_t;


#pragma pack(4)
typedef struct jet_diam_buffer
{
	uint8_t * buffer;
	uint32_t Length;
	uint32_t RequestNo;
	uint32_t Receiver;
	uint8_t * ReceiverObj;
	
} jet_diam_buffer_t;


#pragma pack(4)
typedef struct jet_diam_host 
{
	unsigned char IP[30];
	uint16_t Port;
	uint16_t TransportType;	// 1 TCP, 2 SCTP
	//uint16_t Mode;			// 1 - Server, 2 - Client 
	
	uint32_t AuthAppId[10];
	uint32_t AuthAppCount;
	
	jet_diam_vendor_app_t VendAuthAppId[10];
	uint32_t VendAuthAppCount;
	
	uint32_t AcctAppId[10];
	uint32_t AcctAppCount;
	
	jet_diam_vendor_app_t VendAcctAppId[10];
	uint32_t VendAcctAppCount;
	
} jet_diam_host_t;


#pragma pack(4)
typedef struct jet_diam_peer_client
{
	struct jet_diam_peer_client * Next;

	uint32_t Id;
	char PLMN[8];
	char MCC[4];
	char MNC[4];
	uint32_t MNC_Len; 
	
	char PeerHostName[100];
	char PeerRealmName[100];

	uint32_t AuthAppId[10];
	uint32_t AuthAppCount;
	
	jet_diam_vendor_app_t VendAuthAppId[10];
	uint32_t VendAuthAppCount;
	
	uint32_t AcctAppId[10];
	uint32_t AcctAppCount;
	
	jet_diam_vendor_app_t VendAcctAppId[10];
	uint32_t VendAcctAppCount;
	
	jet_diam_host_t * host;

	unsigned char IP[30];
	uint16_t Port;
	uint16_t TransportType;	// 1 TCP, 2 SCTP
	
	uint16_t WatchDogSent;
	uint16_t Connected;
	uint16_t CEASuccess;
	uint32_t fd;
} jet_diam_peer_client_t;


#pragma pack(4)
typedef struct jet_diam_stack
{
	char HostName[100];
	char RealmName[100];
	
	jet_diam_host_t host[10];
	int host_count;
	
	app_logger_t * plogger;
	app_data_region_t * dm_region;
	
	
	jet_diam_dict_message_t * DDHead;
	jet_diam_dict_message_t * DDCurrent;
	uint32_t DDCount;
	
	app_data_pool_t * diam_msg_dpool;
	app_data_pool_t * diam_avp_dpool;
	uint32_t OriginStateId;
	app_iQueue * appQueue;
	
} jet_diam_stack_t;



void 	jet_diam_stack__init( char * filepath);
void 	jet_diam_stack__set_host( char * hostName, char * realmName);
char * 	jet_diam_stack__set_hostname();
char * 	jet_diam_stack__set_hostrealm();
int 	jet_diam_stack__get_hostport( jet_diam_host_t * host);
char * 	jet_diam_stack__get_hostip( jet_diam_host_t * host);
jet_diam_host_t * jet_diam_stack__add_host( unsigned char * IP, uint32_t Port, uint16_t TransportType);

void jet_diam_stack__add_auth_app_id( jet_diam_host_t * host, uint32_t appId);
void jet_diam_stack__add_vend_auth_app_id( jet_diam_host_t * host, uint32_t appId, uint32_t vendId);
void jet_diam_stack__add_acct_app_id( jet_diam_host_t * host, uint32_t appId);
void jet_diam_stack__add_vend_acct_app_id( jet_diam_host_t * host, uint32_t appId, uint32_t vendId);



void jet_diam_stack__start_peer_client( jet_diam_peer_client_t * client);

typedef uint32_t (*fp_diam_data_type)( uint32_t AppId, uint32_t CmdCode, uint32_t AVPCode);
void jet_diam_stack__set_data_type_handler( fp_diam_data_type diam_data_type_fc);

uint8_t jet_diam_stack__is_request( jet_diam_message_t * dMsg);


uint8_t * jet_diam_stack__get_avp_data( jet_diam_avp_t * avp);
int jet_diam_stack__get_avp_data_len( jet_diam_avp_t * avp);
uint32_t jet_diam_stack__get_avp_u32( jet_diam_avp_t * avp);
uint64_t jet_diam_stack__get_avp_u64( jet_diam_avp_t * avp);
int32_t jet_diam_stack__get_avp_i32( jet_diam_avp_t * avp);
int64_t jet_diam_stack__get_avp_i64( jet_diam_avp_t * avp);


jet_diam_message_t * jet_diam_stack__init_request( uint32_t AppId, uint32_t CmdCode, uint8_t flags);
jet_diam_message_t * jet_diam_stack__init_request2( uint32_t AppId, uint32_t CmdCode, uint8_t flags, uint32_t hbhId, uint32_t e2eId);
jet_diam_message_t * jet_diam_stack__diam_message_init_answer2( jet_diam_message_t * dRequest);
jet_diam_peer_client_t * jet_diam_stack__get_client( jet_diam_buffer_t * diam_buffer);
void jet_diam_stack__set_E2EId( jet_diam_message_t * dRequest, uint32_t e2eId);
void jet_diam_stack__set_HBHId( jet_diam_message_t * dRequest, uint32_t hbhId);
uint32_t jet_diam_stack__get_E2EId( jet_diam_message_t * dmMsg);
uint32_t jet_diam_stack__get_HBHId( jet_diam_message_t * dmMsg);

int jet_diam_stack__peer_send_message( jet_diam_peer_client_t * client, jet_diam_message_t * dMsg);
void jet_diam_stack__free_diam_messsage( jet_diam_message_t * dMsg);

void jet_diam_stack__add_origin_host( jet_diam_message_t * dMsg);
void jet_diam_stack__add_origin_realm( jet_diam_message_t * dMsg);

jet_diam_dict_message_t * jet_diam_stack__find_dict_message( uint32_t AppId, uint32_t CmdCode);
void jet_diam_stack__diam_message_add_str_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, char * data, int datalen);
void jet_diam_stack__diam_message_add_str_avp_to_avp( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, char * data, int datalen);
void jet_diam_stack__diam_message_add_number_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint64_t data, uint32_t datalen, uint32_t dt);
void jet_diam_stack__diam_message_add_number_avp_to_avp( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint64_t data, uint32_t datalen, uint32_t dt);
void jet_diam_stack__diam_message_add_grouped_avp( jet_diam_message_t * dMsg, jet_diam_avp_t * avp);
jet_diam_avp_t * jet_diam_stack__create_grouped_avp(  uint32_t AVPCode, uint8_t Flags, uint32_t VendorId);
void jet_diam_stack__diam_message_add_ipv4_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint32_t ipadd);
void jet_diam_stack__diam_message_add_ipv4_avp2( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint32_t ipadd);
void jet_diam_stack__diam_message_add_avp_to_avp( jet_diam_avp_t * pavp, jet_diam_avp_t * avp);
void jet_diam_stack__add_origin_state_id( jet_diam_message_t * dMsg);
void jet_diam_stack__release2( jet_diam_buffer_t * diam_buffer, jet_diam_message_t * dMsg);

void jet_diam_stack__diam_message_add_ipv6_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint8_t * ipadd);
void jet_diam_stack__diam_message_add_ipv6_avp2( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint8_t * ipadd);

void jet_diam_stack__get_diam_message_attr( jet_diam_message_t * dmMsg, uint32_t * appId, uint32_t * CmdCode, uint8_t * isRequest);
uint32_t jet_diam_stack__get_E2EId( jet_diam_message_t * dmMsg);
uint32_t jet_diam_stack__get_HBHId( jet_diam_message_t * dmMsg);

void jet_diam_stack__getbit( uint8_t * bVal, char * _nwByte, int bitNumber);
void jet_diam_stack__log_region_info();
int  jet_diam_stack__peer_ready( jet_diam_peer_client_t * client);

jet_diam_avp_t * jet_diam_stack__get_diam_message_root_avp( jet_diam_message_t * dMsg);
jet_diam_avp_t * jet_diam_stack__get_diam_avp_next( jet_diam_avp_t * avp);
jet_diam_avp_t * jet_diam_stack__get_diam_avp_group_root_avp( jet_diam_avp_t * avp);

#endif