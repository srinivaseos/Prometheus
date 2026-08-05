#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef S_PFCP_STACK_DEF
#define S_PFCP_STACK_DEF

typedef struct pfcp_node pfcp_node_t;
 

#include "app_stack.h"
#include "app_endpoint.h"
#include "pfcp_protocol_def.h"
#include "pfcp_types.h"
#include "pfcp_parser.h"
#include "app.h"

#define PFC

int  dpdk_session__ipv4_count();


#pragma pack(1)
typedef struct pfcp_apply_action
{
	uint8_t DuplicateForRedundantTransmission : 1;
	uint8_t IPMulticastDeny : 1;
	uint8_t IPMulticastAccept : 1;
	uint8_t Duplicate : 1;
	uint8_t NotifyTheCPFunction : 1;
	uint8_t Buffer : 1;
	uint8_t Forward : 1;
	uint8_t Drop : 1;
} pfcp_apply_action_t;


#pragma pack(1)
typedef struct pfcp_gate_status
{
	uint8_t pad:4;
	uint8_t ul_gate:2;
	uint8_t dl_gate:2;	
} pfcp_gate_status_t;


#pragma pack(4)
typedef struct pfcp_pdr
{
	struct pfcp_pdr * Next;
	struct pfcp_session * session;
	
	uint16_t isremoved;

	uint16_t pdr_rule_id;
	uint32_t precedence;
	uint8_t source_interface;						// PFCP_INTERFACE_ACCESS | RAN * PFCP_INTERFACE_CORE | ISP
	unsigned char network_instance[30];
	uint16_t network_instance_len;
	uint32_t far_id;
	uint32_t qer_id;

	//UE IP when its core
	//RAN IP when its access
	uint32_t ipv4;
	uint8_t ipv4_isset;
	uint8_t ipv6[16];
	uint8_t ipv6_isset;
	
	uint32_t teid;
	uint32_t teid_isset;
	uint8_t qfi;
	uint8_t qfi_isset;
	uint8_t outer_header_removal;
	uint8_t outer_header_removal_isset;
} pfcp_pdr_t;


#pragma pack(4)
typedef struct pfcp_far
{
	struct pfcp_far * Next;
	struct pfcp_session * session;
	
	uint16_t isremoved;
	
	uint32_t far_id;

	uint8_t apply_action;
	uint8_t apply_action_isset;
	
	uint8_t destination_interface;
	uint8_t destination_interface_isset;

	uint8_t outer_header_creation_isset;
	uint16_t outer_header_creation;
	
	uint32_t teid;
	uint32_t ipv4;
	uint8_t ipv4_isset;
	uint8_t ipv6[16];
	uint8_t ipv6_isset;	
	
} pfcp_far_t;


#pragma pack(4)
typedef struct pfcp_qer
{
	struct pfcp_qer * Next;
	struct pfcp_session * session;
	
	uint16_t isremoved;
	
	uint32_t qer_id;
	uint8_t gate_status;
	uint8_t gate_status_isset;
	uint64_t ul_mbr;
	uint64_t dl_mbr;
	uint8_t mbr_isset;
	uint8_t qfi;
	uint8_t qfi_isset;
	
	uint16_t ul_tc;
	uint16_t dl_tc;
	
} pfcp_qer_t;


#pragma pack(4)
typedef struct pfcp_urr
{
	struct pfcp_urr * Next;
	struct pfcp_session * session;

	uint16_t isremoved;
	
	uint32_t urr_id;
	uint8_t  measurement_method;
	uint8_t  measurement_method_isset;
	uint8_t  reporting_triggers;
	uint8_t  reporting_triggers_isset;
	uint8_t  isallowed;					//rg-allowed?	1
	uint16_t quota_granted_times;		//quota granted times
	uint16_t quota_requested_times;		//quota requested times

	uint8_t  quota_status;				//0-requested for quota, 1-waiting for quota response, 2-received quota response
	uint64_t quota_requested_time;
	uint64_t last_quota_requested_time;

	struct 
	{
		uint64_t total;
		uint64_t uplink;
		uint64_t downlink;
	} used_volume;
	
	struct 
	{
		uint64_t total;
		uint64_t uplink;
		uint64_t downlink;
	} reported_volume;
	
	struct 
	{
		uint64_t total;
		uint64_t uplink;
		uint64_t downlink;
	} total_reported_volume;

	struct 
	{
		uint64_t total;
		uint64_t uplink;
		uint64_t downlink;
	} granted_volume;
	
	struct 
	{
		uint64_t total;
		uint64_t uplink;
		uint64_t downlink;
	} trigger_volume;

	uint32_t trigger_volume_isset;
	uint32_t granted_volume_isset;
	
	uint32_t pfcp_seqno;
	uint32_t pfcp_report_request_seqno;
	
} pfcp_urr_t;


#pragma pack(4)
typedef struct pfcp_bar
{
	struct pfcp_bar * Next;
	struct pfcp_session * session;
	
	uint16_t isremoved;
	
	uint8_t bar_id;
	
} pfcp_bar_t;


#pragma pack(4)
typedef struct pfcp_mar
{
	struct pfcp_mar * Next;
	struct pfcp_session * session;

	uint16_t isremoved;
	
	uint32_t mar_id;
	
} pfcp_mar_t;



typedef struct ndpi_flow_struct_w ndpi_flow_struct_wt;


#pragma pack(4)
typedef struct ip4flow
{
	struct ip4flow * Next;
	
	//uint32_t src;
	uint32_t dst;
	uint16_t sport;
	uint16_t dport;
	uint16_t protocol;
	// uint16_t pcount;
	// char sni[150];
	// ndpi_flow_struct_wt * ndpi_flow;
} ip4flow_t;

typedef struct pfcp_session pfcp_session_t;

#pragma pack(4) 
typedef struct ip4_flow
{
	pfcp_session_t * session;
	int flowIndex;
	uint16_t rgid;

	uint32_t ueip;
	uint32_t dstip;
	uint16_t dstport;
	uint16_t srcport;
	uint16_t protocol;
	
	uint16_t version;
	uint64_t lastused;

} ip4_flow_t;

#pragma pack(4) 
typedef struct ip6_flow
{
	pfcp_session_t * session;
	int flowIndex;
	uint16_t rgid;

	uint8_t ueip[16];
	uint8_t dstip[16];
	uint16_t dstport;
	uint16_t srcport;
	uint16_t protocol;
	
	uint16_t version;
	uint64_t lastused;

} ip6_flow_t;




typedef struct pfcp_ip4_flow pfcp_ip4_flow_t;
typedef struct pfcp_ip6_flow pfcp_ip6_flow_t;

#pragma pack(4) 
typedef struct pfcp_port_flow
{
	//struct pfcp_port_flow * Next;
	struct pfcp_port_flow * FNext;
	struct pfcp_port_flow * FPrev;
	
	pfcp_session_t * session;

	uint16_t dstport;
	uint16_t srcport;
	uint16_t protocol;
	uint64_t lastused;
	
} pfcp_port_flow_t;

#pragma pack(4) 
typedef struct pfcp_ip4_flow
{
	//struct pfcp_ip4_flow * Next;
	struct pfcp_ip4_flow * FNext;
	struct pfcp_ip4_flow * FPrev;
	
	pfcp_port_flow_t * portHead;
	pfcp_port_flow_t * portCurrent;
	pthread_mutex_t    portLock;
	
	uint32_t dstip;
	uint16_t rgid;
	uint64_t lastused;

} pfcp_ip4_flow_t;


#pragma pack(4) 
typedef struct pfcp_ip6_flow
{
	//struct pfcp_ip6_flow * Next;
	struct pfcp_ip6_flow * FNext;
	struct pfcp_ip6_flow * FPrev;

	pfcp_port_flow_t * portHead;
	pfcp_port_flow_t * portCurrent;
	pthread_mutex_t    portLock;

	uint8_t  dstip[16];
	uint16_t rgid;
	uint64_t lastused;
	
} pfcp_ip6_flow_t;


#pragma pack(4) 
typedef struct pfcp_ip_flows
{
	struct pfcp_ip_flows * Next;

	uint32_t ueip4;
	uint8_t ueip6[16];

	pfcp_ip4_flow_t * f4Head;
	pfcp_ip4_flow_t * f4Current;
	pthread_mutex_t f4Lock;

	pfcp_ip6_flow_t * f6Head;
	pfcp_ip6_flow_t * f6Current;
	pthread_mutex_t f6Lock;
	
	pfcp_session_t * session[5];
	uint16_t session_count;
	pthread_mutex_t session_count_lock;
	
} pfcp_ip_flows_t;

#define MAX_GLOBAL_IDS						10

typedef struct pfcp_stack pfcp_stack_t;
// typedef struct pfcp_ip4_flow pfcp_ip4_flow_t;

#pragma pack(4)
typedef struct pfcp_session
{
	//struct pfcp_session * Left, * Right, * Parent; 

	uint16_t initalized;
	uint16_t released;
	pfcp_node_t * node;
	uint32_t nodeIPv4;

	uint64_t cp_f_seid;
	uint64_t up_f_seid;
	
	struct {
		pfcp_pdr_t * head;
		pfcp_pdr_t * current;
		int count;
		pthread_mutex_t Lock;
	} pdr;

	struct {
		pfcp_far_t * head;
		pfcp_far_t * current;
		int count;
		pthread_mutex_t Lock;
	} far;
	
	struct {
		pfcp_qer_t * head;
		pfcp_qer_t * current;
		int count;
		pthread_mutex_t Lock;
	} qer;

	struct {
		pfcp_bar_t * head;
		pfcp_bar_t * current;
		int count;
		pthread_mutex_t Lock;
	} bar;

	struct {
		pfcp_urr_t * head;
		pfcp_urr_t * current;
		int count;
		pthread_mutex_t Lock;
	} urr;
	
	struct {
		pfcp_mar_t * head;
		pfcp_mar_t * current;
		int count;
		pthread_mutex_t Lock;
	} mar;

	
	pfcp_far_t * global_far[MAX_GLOBAL_IDS];
	pfcp_qer_t * global_qer[MAX_GLOBAL_IDS];
	pfcp_urr_t * global_urr[MAX_GLOBAL_IDS];
	pfcp_mar_t * global_mar[MAX_GLOBAL_IDS];
	
	uint32_t urr_rep_seq_no;
	pthread_mutex_t UrrSeqLock;		//lock for URR Seq No
	pthread_mutex_t QLock;			//lock for voulme usage 
	
	uint16_t gtp_seq_no;
	
	// struct
	// {
		// void * fHead;
		// void * fCurrent;
		// int fCount;
		// pthread_mutex_t fLock;
	// } dpi;
	
	uint32_t ran_ipv4;
	uint8_t  ran_ipv6[18];
	uint32_t ran_teid;
	
	uint32_t ran_ipv4_from_packet;
	uint8_t ranmac[7];
	
	uint32_t ue_ipv4;
	uint8_t  ue_ipv4_isset;	
	
	uint8_t  ue_ipv6[18];	
	uint8_t  ue_ipv6_isset;	

	uint32_t upf_teid;
	uint32_t upf_ip;
	
	
	uint8_t pdnType;
	uint8_t pdnType_isset;




	uint8_t 	qer_0__gate_status;
	uint64_t 	qer_0__ul_mbr;
	uint64_t 	qer_0__dl_mbr;
	uint8_t 	qer_0__qfi;

	uint64_t 	con_0__ul_mbr;
	uint64_t 	con_0__dl_mbr;
	
	// ip4flow_t * f4Head;
	// ip4flow_t * f4Current;
	
	//pfcp_ip_flows_t * ip_flows;
	
	uint16_t restored_from_as;
	uint64_t modified_time;
	uint64_t created_time;
	uint16_t idle_session_hb_count;
	
	uint16_t version;
	uint16_t stime;
	
} pfcp_session_t;



typedef struct ls_uint_table  ls_uint_table_t;


#pragma pack(4)
typedef struct pfcp_node
{
	struct pfcp_node * Next;
	int isActive;
	int id;
	uint8_t NodeIDAddressType;
	int IPv4;
	uint8_t IPv6[16];
	uint8_t FQDN[95];
	unsigned short port;
	
	union {
		struct sockaddr_in clientaddr;
		struct sockaddr_in6 clientaddr6;
	} u;
	uint32_t uType;
	
	int pendingHeartbeatResponse;
	struct timeval lastmsgsent;
	struct timeval lastmsg;
	app_timer_t * heartBeatTimer;
	uint32_t lastSeqNo;
	pthread_mutex_t SeqNoLock;
	
	ls_uint_table_t * cp_seid_table;
	
} pfcp_node_t;


void pfcp_stack__perflog( app_logger_t * logger, int log_buffers);

void pfcp_stack__buffer_handler( uint8_t * Data, int tIndex);
struct pfcp_node * pfcp_stack___find_node( app_ep_udp_message_t * msg, int bAdd);
void pfcp_stack___init( int ipv, char * IPv4, char * IPv6, uint32_t iIPv4, uint8_t * iIPv6, app_data_region_t * pfcp_mregion, int fsmThreads, int iHeartbeatSeconds, int session_count, int ueipv6support, int enableOrDisable);
void pfcp_stack___exit();
void pfcp_stack___init_pace2();
void pfcp_stack___set_pfcp_maxtps( int MaxTPS);
void pfcp_stack___setpool( app_rbtree_t * pf_tree, app_data_pool_t * pool, app_data_pool_t * ippool, app_data_pool_t * flowPool);
void pfcp_stack___setippool( app_data_pool_t * ipv4pool, app_data_pool_t * ipv6pool, app_data_pool_t * portPool);
void pfcp_stack___set_heartbeat_seconds( int HeartbeatSeconds);

void pfcp_stack__set_applogger( app_logger_t * al);
void pfcp_stack__set_pfcplogger( app_logger_t * pl);
void pfcp_stack__set_devicelogger( app_logger_t * dl);

void pfcp_stack__set_pfcp_ipv4server( app_ep_udp_server_t * udp_ipv4server);
void pfcp_stack__set_pfcp_ipv6server( app_ep_udp_server_t * udp_ipv6server);

void pfcp_stack___send__heartbeat_request( struct pfcp_node * node);
void pfcp_stack___send__heartbeat_response( struct pfcp_node * node, app_ep_udp_message_t * msg);
void pfcp_stack___send__association_setup_response( app_ep_udp_message_t * msg, uint8_t cause);
void pfcp_stack___send__association_update_response( app_ep_udp_message_t * msg, uint8_t cause);
void pfcp_stack___send__association_release_response( app_ep_udp_message_t * msg, uint8_t cause);
void pfcp_stack___send__version_not_supported_response( app_ep_udp_message_t * msg);
void pfcp_stack___send__node_report_response( app_ep_udp_message_t * msg, uint8_t cause);
void pfcp_stack___send__session_deletion_response( app_ep_udp_message_t * msg, uint8_t cause, uint64_t seid);

app_logger_t * pfcp_stack__get_pfcplogger();
app_data_region_t * pfcp_stack__get_pfcpmregion();
void pfcp_stack__print_buffer( uint8_t * buffer, int len);
void pfcp_stack__print_buffer_wn( char * name, uint8_t * buffer, int len);
void pfcp_stack__add_global_far( pfcp_far_t * sess_far);
pfcp_far_t * pfcp_stack__get_global_far_root();

int pfcp_stack___create_pdr( int index, pfcp_tlv_create_pdr_t * create_pdr, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * se_response, pfcp_session_modification_response_t * sm_response);
int pfcp_stack___update_pdr( int index, pfcp_tlv_update_pdr_t * update_pdr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response);
int pfcp_stack___remove_pdr( int index, pfcp_tlv_remove_pdr_t * remove_pdr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response);

int pfcp_stack___create_far( int index, pfcp_tlv_create_far_t * create_far, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response);
int pfcp_stack___update_far( int index, pfcp_tlv_update_far_t * update_far, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response);
int pfcp_stack___remove_far( int index, pfcp_tlv_remove_far_t * remove_far, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response);

void pfcp_stack___record_usage( uint64_t up_f_seid, uint32_t rgid, uint64_t uplink, uint64_t downlink);

uint64_t pfcp_stack___get_seid( uint8_t * buffer);
pfcp_stack_t * pfcp_stack__get_instance();
pfcp_session_t * pfcp_stack__find_session( uint64_t seid);

void pfcp_stack__set_upf_ip( uint32_t ipv4, uint8_t * ipv6);
pfcp_session_t * pfcp_stack___allocate_session();
void pfcp_stack___set_pfcp_session_idp( pfcp_session_t * session);

uint8_t pfcp_stack__get_qfi( void * sPtr);


uint32_t pfcp_stack___get_ueip( pfcp_session_t * pfcpSession, uint8_t ** ipv6, uint8_t * ipv6isset, uint8_t * ipv4isset);
void pfcp_stack___set_upir( int upir);



#endif











