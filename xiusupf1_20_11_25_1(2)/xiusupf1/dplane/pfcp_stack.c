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

#include <ctype.h>
#include <inttypes.h>

#include "app_stack.h"
#include "app_endpoint.h"
#include "pfcp_protocol_def.h"
#include "pfcp_types.h"
#include "pfcp_parser.h"
#include "pfcp_stack.h"
#include "app.h"
#include "ls.h"

// #ifdef ENABLE_PROMETHEUS
	// #include "metrics_link.h" 
// #endif

#if DPI_PACE2
	#include "dpi_wrapper.h"
#endif


#define LOG_SESS_PARAM_CAT_LOG							1
#define LOG_SESS_USAGE_CAT_LOG							2

void app__as__pfcpsavesession( pfcp_session_t * session); 
void app__as__remove( pfcp_session_t * session);

void successful_task_request(  const char *method,int data);
void successful_task_responce(  const char *method,int data);

int dpdk_session__ipv4_add( uint32_t ipv4, void * data, int deleteIfExists);
int dpdk_session__seid_add( uint64_t seid, void * data, int deleteIfExists);

//unsigned __int128 data = 0x44444444333333332222222211111111;

// #ifndef UINT128MAX
    // #error "__uint128_t not defined"
// #endif


#pragma pack(4)
typedef struct pfcp_msg_helper
{
	struct pfcp_node * node;
	app_ep_udp_message_t * msg;
	int tIndex;
	uint8_t * obj1;
	pfcp_session_t * session;
} pfcp_msg_helper_t;


#pragma pack(4)
typedef struct pfcp_stack
{
	int ipv;
	char IPv4[16];
	char IPv6[50];
	int iIPv4;
	uint8_t  iIPv6[16];
	
	struct pfcp_node * pfcpNodeHead;
	struct pfcp_node * pfcpNodeCurrent;
	uint32_t pfcpNodeTotal;
	pthread_mutex_t pfcpNodeLock;
	app_data_region_t * pfcp_mregion;					//tlv pool
	int HeartbeatSeconds;

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
		pfcp_urr_t * head;
		pfcp_urr_t * current;
		int count;
		pthread_mutex_t Lock;
	} urr;
	
	//mar not implemented
	//bar not applicable
	
	uint64_t ser_last;
	uint64_t ser_total;
	pthread_mutex_t ser_lock;

	uint64_t sea_last;
	uint64_t sea_total;
	pthread_mutex_t sea_lock;

	uint64_t smr_last;
	uint64_t smr_total;
	pthread_mutex_t smr_lock;
	
	uint64_t sma_last;
	uint64_t sma_total;
	pthread_mutex_t sma_lock;

	uint64_t sdr_last;
	uint64_t sdr_total;
	pthread_mutex_t sdr_lock;
	
	uint64_t sda_last;
	uint64_t sda_total;
	pthread_mutex_t sda_lock;
	
	uint64_t sur_last;
	uint64_t sur_total;
	pthread_mutex_t sur_lock;

	uint64_t sua_last;
	uint64_t sua_total;
	pthread_mutex_t sua_lock;
	
	uint64_t pkt_last;
	uint64_t pkt_total;
	pthread_mutex_t pkt_lock;
	
	uint64_t hb_request_sent_last;
	uint64_t hb_request_sent_total;
	pthread_mutex_t hb_request_sent_lock;

	uint64_t hb_response_received_last;
	uint64_t hb_response_received_total;
	pthread_mutex_t hb_response_received_lock;

	uint64_t hb_request_received_last;
	uint64_t hb_request_received_total;
	pthread_mutex_t hb_request_received_lock;

	uint64_t hb_response_sent_last;
	uint64_t hb_response_sent_total;
	pthread_mutex_t hb_response_sent_lock;


	uint64_t cong_dropped_last;
	uint64_t cong_dropped_total;
	pthread_mutex_t cong_dropped_lock;


	uint64_t sess_delete_last;
	uint64_t sess_delete_total;
	pthread_mutex_t sess_delete_lock;

	
	
	app_logger_categories_t * session_parameter_cat_log;
	app_logger_categories_t * session_usage_cat_log;
	
	uint32_t upf_ipv4;
	uint8_t  upf_ipv6[18];



	ls_uint_table_t * teid_table;
	ls_uint_table_t * ueipv4_table;
	ls_uint_table_t * ueipv6_table;
	
	app_rbtree_t * pfcp_sessions_tree;
	app_data_pool_t * pfcp_session_pool;
	app_data_pool_t * pfcp_session_ip_pool;
	pthread_mutex_t pfcp_session_lock;
	app_logger_t * appLogger;
	app_logger_t * pfcpLogger;
	app_logger_t * deviceLogger;
	app_ep_udp_server_t * udp_ipv4server;
	app_ep_udp_server_t * udp_ipv6server;	
	time_t startTime;
	int fsmThreads;
	app_fsmQueue_t * fsmQ;

	app_data_pool_t * flowPool;
	int initDPI;
	
	int MaxTPS;
	pthread_mutex_t maxtps_lock;

	app_data_pool_t 	* ippool_ipv4;
	app_data_pool_t 	* ippool_ipv6;
	app_data_pool_t 	* ippool_port;
	
	uint16_t stime;
	int pace2_enabled;
	
	int enable_upir;
	
	int session_count;
	int enable_quota_logs;
	
} pfcp_stack_t;

pfcp_stack_t * pfs = NULL;


#if DPI_PACE2
	static dpi_instance * dpi = NULL;
	int g_dpi_worker_count = 0;
#endif


uint32_t cp__get_ran_teid( uint32_t upf_teid)
{
	pfcp_session_t * pfcpSession = (pfcp_session_t *)ls__getobject_u32k( pfs->teid_table, upf_teid);
	if( pfcpSession) {
		return pfcpSession->ran_teid;
	}
	return 0;
}

int cp__teid_allowed_v4( uint32_t ran_ip, uint32_t teid, uint8_t ** session, uint8_t * mac)
{
	pfcp_session_t * pfcpSession = (pfcp_session_t *)ls__getobject_u32k( pfs->teid_table, teid);
	
	//printf("session=%p found with teid=%u  %s|%d\n", pfcpSession, teid, __FILE__, __LINE__);
	
	if( pfcpSession)
	{
		pfcpSession->ran_ipv4_from_packet = ran_ip;
		
		if( pfcpSession->ranmac[5] == 0)
		{
			pfcpSession->ranmac[0] = mac[0], pfcpSession->ranmac[1] = mac[1], pfcpSession->ranmac[2] = mac[2], pfcpSession->ranmac[3] = mac[3], pfcpSession->ranmac[4] = mac[4], pfcpSession->ranmac[5] = mac[5];
		}
		
		// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "adding raninfo for session=%p RAN-IP=%u.%u.%u.%u  teid=%u MAC=%02X:%02X:%02X:%02X:%02X:%02X   %s|%s|%d", 
			// pfcpSession, ran_ip & 0xFF, (ran_ip >> 8) & 0xFF, (ran_ip >> 16) & 0xFF, (ran_ip >> 24) & 0xFF, teid,
			// mac[0] & 0xFF, mac[1] & 0xFF, mac[2] & 0xFF, mac[3] & 0xFF, mac[4] & 0xFF, mac[5] & 0xFF,
		// __FILE__, __FUNCTION__, __LINE__);		
		
		*session = (uint8_t *) pfcpSession;
		return 1;
	}
	// else
	// {
		// printf("session not found with teid=%u  %s|%d\n", teid, __FILE__, __LINE__);
	// }
	
	return 0;
}

int cp__teid_allowed_v6( __uint128_t ran_ip, uint32_t teid, uint8_t ** session)
{
	return 0;
}

void pfcp_stack___setinitDPI( int initDPI)
{
	pfs->initDPI = initDPI;
}

ndpi_flow_struct_wt * upfndpi_allocate();

uint64_t pfcp_stack__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink);

pfcp_ip6_flow_t * pfcp__find_or_create_flow6( pfcp_session_t * pfcpSession, uint8_t * src, uint8_t * dst, uint16_t sport, uint16_t dport, uint8_t protocol)
{
	// if(!pfcpSession->ip_flows) 
	// {
		// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "pfcpSession->ip_flows is NULL   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		// return NULL;
	// }

	// pfcp_ip6_flow_t * item = pfcpSession->ip_flows->f6Head;
	// int bFFound = 0;

	// while( item)
	// {
		// if( item->dstip[0] == dst[0] && item->dstip[1] == dst[1] && item->dstip[2] == dst[2] && item->dstip[3] == dst[3] && item->dstip[4] == dst[4] && item->dstip[5] == dst[5] && item->dstip[6] == dst[6]
			 // && item->dstip[7] == dst[7] && item->dstip[8] == dst[8] && item->dstip[9] == dst[9] && item->dstip[10] == dst[10] && item->dstip[11] == dst[11] && item->dstip[12] == dst[12] && item->dstip[13] == dst[13]
			 // && item->dstip[14] == dst[14] && item->dstip[15] == dst[15])
		// {
			// bFFound = 1;
			// break;
		// }
		// item = item->FNext;
	// }

	// if(bFFound == 0)
	// {
		// item = (pfcp_ip6_flow_t *)app_region__allocate_fd( pfs->ippool_ipv6);
		
		// if(!item) 
		// {
			// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "allocation of pfcp_ip6_flow_t failed   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
			// return NULL;
		// }
		
		// item->dstip[0] = dst[0];
		// item->dstip[1] = dst[1];
		// item->dstip[2] = dst[2];
		// item->dstip[3] = dst[3];
		// item->dstip[4] = dst[4];
		// item->dstip[5] = dst[5];
		// item->dstip[6] = dst[6];
		// item->dstip[7] = dst[7];
		// item->dstip[8] = dst[8];
		// item->dstip[9] = dst[9];
		// item->dstip[10] = dst[10];
		// item->dstip[11] = dst[11];
		// item->dstip[12] = dst[12];
		// item->dstip[13] = dst[13];
		// item->dstip[14] = dst[14];
		// item->dstip[15] = dst[15];
			 
		// item->rgid			= 0;
		// item->lastused		= 0;
		// item->FNext			= NULL;
		// item->FPrev			= NULL;
		// item->portHead		= NULL;
		// item->portCurrent 	= NULL;
		
		// pthread_mutex_lock( &pfcpSession->ip_flows->f6Lock);
		
		// if(!pfcpSession->ip_flows->f6Head)
		// {
			// pfcpSession->ip_flows->f6Head = pfcpSession->ip_flows->f6Current = item;
		// } 
		// else
		// {
			// if(pfcpSession->ip_flows->f6Current)
			// {
				// item->FPrev = pfcpSession->ip_flows->f6Current;
			// }
			
			// pfcpSession->ip_flows->f6Current->FNext = item;
			// pfcpSession->ip_flows->f6Current = item;
		// }
		
		// pthread_mutex_unlock( &pfcpSession->ip_flows->f6Lock);
	// }

	
	// if( item)
	// {
		// pfcp_port_flow_t * pitem = item->portHead;
		// bFFound = 0;
		
		// while( pitem)
		// {
			// if( pitem->dstport == dport && pitem->srcport == sport && pitem->protocol == protocol)
			// {
				// bFFound = 1;
				// return item;
			// }
			// item = item->FNext;
		// }
		
		// if( bFFound == 0)
		// {
			// pitem = (pfcp_port_flow_t *)app_region__allocate_fd( pfs->ippool_port);
			
			// if(!pitem)
			// {
				// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "allocation of pfcp_port_flow_t failed   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
				// return NULL;
			// }
			
			// pitem->FNext	= NULL;
			// pitem->FPrev	= NULL;
			// pitem->session	= pfcpSession;
			// pitem->dstport	= dport;
			// pitem->srcport	= sport;
			// pitem->protocol = protocol;
			
			
			// pthread_mutex_lock( &item->portLock);
			
			// if(!item->portHead)
			// {
				// item->portHead = item->portCurrent = pitem;
			// }
			// else
			// {
				// if(item->portCurrent)
				// {
					// pitem->FPrev = pitem;
				// }
				
				// item->portCurrent->FNext = pitem;
				// item->portCurrent = pitem;
			// }
			
			// pthread_mutex_unlock( &item->portLock);
			
			// return item;
		// }
		
	// }
	
	
	return NULL;
}

pfcp_ip4_flow_t * pfcp__find_or_create_flow( pfcp_session_t * pfcpSession, uint32_t src, uint32_t dst, uint16_t sport, uint16_t dport, uint8_t protocol)
{
	return NULL;
	
	//printf("   pfcp__find_or_create_flow  %d\n", __LINE__);
	
	// if(!pfcpSession->ip_flows) 
	// {
		// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "pfcpSession->ip_flows is NULL   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		// return NULL;
	// }
	
	// //printf("   pfcp__find_or_create_flow  %d\n", __LINE__);

	// pfcp_ip4_flow_t * item = pfcpSession->ip_flows->f4Head;
	// int bFFound = 0;
	
	// while( item)
	// {
		// if( item->dstip == dst)
		// {
			// bFFound = 1;
			// break;
		// }
		// item = item->FNext;
	// }
	
	// //printf("   pfcp__find_or_create_flow  %d\n", __LINE__);
	
	// if(bFFound == 0)
	// {
		// item = (pfcp_ip4_flow_t *)app_region__allocate_fd( pfs->ippool_ipv4);
		
		// if(!item) 
		// {
			// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "allocation of pfcp_ip4_flow_t failed   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
			// return NULL;
		// }
		
		// item->dstip 		= dst;
		// item->rgid			= 0;
		// item->lastused		= 0;
		// item->FNext			= NULL;
		// item->FPrev			= NULL;
		// item->portHead		= NULL;
		// item->portCurrent 	= NULL;
		
		// pthread_mutex_lock( &pfcpSession->ip_flows->f4Lock);
		
		// if(!pfcpSession->ip_flows->f4Head)
		// {
			// pfcpSession->ip_flows->f4Head = pfcpSession->ip_flows->f4Current = item;
		// } 
		// else
		// {
			// if(pfcpSession->ip_flows->f4Current)
			// {
				// item->FPrev = pfcpSession->ip_flows->f4Current;
			// }
			
			// pfcpSession->ip_flows->f4Current->FNext = item;
			// pfcpSession->ip_flows->f4Current = item;
		// }
		
		// pthread_mutex_unlock( &pfcpSession->ip_flows->f4Lock);
	// }
	
	// if( item)
	// {
		// pfcp_port_flow_t * pitem = item->portHead;
		// bFFound = 0;
		
		// while( pitem)
		// {
			// if( pitem->dstport == dport && pitem->srcport == sport && pitem->protocol == protocol)
			// {
				// bFFound = 1;
				// return item;
			// }
			// item = item->FNext;
		// }
		
		// if( bFFound == 0)
		// {
			// pitem = (pfcp_port_flow_t *)app_region__allocate_fd( pfs->ippool_port);
			
			// if(!pitem)
			// {
				// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "allocation of pfcp_port_flow_t failed   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
				// return NULL;
			// }
			
			// pitem->FNext	= NULL;
			// pitem->FPrev	= NULL;
			// pitem->session	= pfcpSession;
			// pitem->dstport	= dport;
			// pitem->srcport	= sport;
			// pitem->protocol = protocol;
			// pitem->lastused = (uint64_t)time(NULL);
			
			
			// pthread_mutex_lock( &item->portLock);
			
			// if(!item->portHead)
			// {
				// item->portHead = item->portCurrent = pitem;
			// }
			// else
			// {
				// if(item->portCurrent)
				// {
					// pitem->FPrev = item->portCurrent;
				// }
				
				// item->portCurrent->FNext = pitem;
				// item->portCurrent = pitem;
			// }
			
			// pthread_mutex_unlock( &item->portLock);
			
			// return item;
		// }
		
	// }
	

	// ip4flow_t * item = pfcpSession->f4Head;
	
	// while(item)
	// {
		// //if(( item->src == src && item->dst == dst && item->sport == sport && item->dport == dport) || ( item->src == dst && item->dst == src && item->sport == dport && item->dport == sport))
		// //if(( item->dst == dst && item->sport == sport && item->dport == dport) || ( item->dst == src && item->sport == dport && item->dport == sport))
		// if(item->dst == dst && item->sport == sport && item->dport == dport && item->protocol == protocol)
		// {
			// return item;
		// }
		
		// item = item->Next;
	// }
	
	// if(!item)
	// {
		// item = (ip4flow_t *)app_region__allocate_fd( pfs->flowPool);
	
		// if(item)
		// {

			// //item->src 		= src;
			// item->dst 			= dst;
			// item->sport 		= sport;
			// item->dport 		= dport;
			// item->protocol 		= protocol;
			// //item->pcount = 0;
			
			// // #if DPI
				// // if( pfs->initDPI == 1 ) {
					// // item->ndpi_flow = upfndpi_allocate();
				// // }
			// // #endif
			
			// // item->Next = NULL;
			// // memset( item->sni, 0, sizeof(item->sni));
			
			// if(!pfcpSession->f4Head)
			// {
				// pfcpSession->f4Head = pfcpSession->f4Current = item;
			// }
			// else
			// {
				// pfcpSession->f4Current->Next = item;
				// pfcpSession->f4Current = item;
			// }			
		// }
	// }
}


int pfcp__flow4_is_allowed( ip4_flow_t * flow)
{
	flow->lastused = (uint64_t)time(NULL);
	
	#if DPI
	#endif
	return 1;
}


int pfcp__flow6_is_allowed( ip6_flow_t * flow)
{
	flow->lastused = (uint64_t)time(NULL);

	return 1;
}


void upfndpi_processpacket( int iThreadcount, ndpi_flow_struct_wt * flowItem, uint8_t * iph, uint16_t ipsize, int printline);
uint16_t upfndpi_protocol_detected( ndpi_flow_struct_wt * flowItem);


uint8_t * dpe_find_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol);
int dpe_add_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol, void * ip4flow);

uint8_t * dpe_find_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol);
int dpe_add_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol, void * ip4flow);

int dpe_del_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol);
int dpe_del_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol);

#define unlikely(x)     __builtin_expect(!!(x), 0)

int cp__flow_allowed_sess_v6( uint8_t * session, uint8_t* dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen, int direction, int coreid, int * rgid, int coreindex, uint8_t ** flowPtr)
{
	if( srcport == 0 && dstport == 0) return 0;
	
	if( session)
	{
		*rgid = 0;
		pfcp_session_t * pfcpSession = (pfcp_session_t *)session;
	
		//pfcp_ip6_flow_t * ipFlow = pfcp__find_or_create_flow6( pfcpSession, pfcpSession->ue_ipv6, dstip, srcport, dstport, protocol);
		
		
		ip6_flow_t * flow  = (ip6_flow_t*)dpe_find_ipv6flow( pfcpSession->ue_ipv6, dstip, srcport, dstport, protocol);
	
		if(!flow)
		{
			flow = (ip6_flow_t*)app_region__allocate_fd( pfs->ippool_ipv6);
			
			if(flow)
			{
				flow->session 	= pfcpSession;
				flow->lastused	= (uint64_t)time(NULL);
				flow->rgid		= 1;
				flow->version	= pfcpSession->version;
				dpe_add_ipv6flow( pfcpSession->ue_ipv6, dstip, srcport, dstport, protocol, flow);
				*flowPtr 		= (uint8_t*)flow;

				// #if DPI_PACE2
					// if( pfs->pace2_enabled == 1)
					// {
						// dpi_return_t ret_dpi = dpi_process_l2_frame( dpi, coreindex, dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC), (const void * const)pload, ploadlen, DPI_GTPU_ENABLED);
						
						// if (unlikely(ret_dpi != DPI_RETURN_SUCCESS)){
							// DEBUG_PRINTF("[ERROR] PACE2 process frame at ts %lu \n", ts);
						// }
					// }
				// #endif
			
				return 1;
			}
		}
		else
		{
			*rgid 			= flow->rgid;
			*flowPtr 		= (uint8_t*)flow;
			return 1;
		}
		
		return 0;
		
		// if( pfcp__flow6_is_allowed(flow))
		// {
			// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
			// {
				// // printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
			// }
			// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
			// {
				// // printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
			// }
		// }
	}
	return 0;
}




int cp__flow_allowed_sess_v4( uint8_t * session, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  int direction, int coreid, int * rgid, int coreindex, uint8_t ** flowPtr)
{
	//printf("session ptr=%p in line %d\n",session,__LINE__);
	if( session)
	{
		*rgid = 0;
		
		//printf("session ptr=%p in line %d\n",session,__LINE__);
		pfcp_session_t * pfcpSession = (pfcp_session_t *)session;
		ip4_flow_t * ipFlow = NULL;
		
		//if( pfs->initDPI == 1 ) 
		{
			//printf("session ptr=%p in line %d\n",session,__LINE__);			
			//ipFlow = pfcp__find_or_create_flow( pfcpSession, pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol);
			//printf("session ptr=%p in line %d\n",session,__LINE__);			
			
			ipFlow = (ip4_flow_t*)dpe_find_ipv4flow( pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol);
			
			if(!ipFlow)
			{
				ipFlow = (ip4_flow_t*)app_region__allocate_fd( pfs->ippool_ipv4);
			
				if(ipFlow)
				{
					ipFlow->session 	= pfcpSession;
					ipFlow->lastused	= (uint64_t)time(NULL);
					ipFlow->rgid		= 1;
					ipFlow->version		= pfcpSession->version;
					
					
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "adding session   ueip=%u|%u   %u.%u.%u.%u      %u  %u  %u  %u  %u      %s|%s|%d", 
						pfcpSession->ue_ipv4, htonl(pfcpSession->ue_ipv4), 
						pfcpSession->ue_ipv4 & 0xFF, (pfcpSession->ue_ipv4 >> 8) & 0xFF, 
						(pfcpSession->ue_ipv4 >> 16) & 0xFF, (pfcpSession->ue_ipv4 >> 24) & 0xFF,
						pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol,
						__FILE__, __FUNCTION__, __LINE__
					);
					
					
					dpe_add_ipv4flow( pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol, ipFlow);
					*flowPtr			= (uint8_t*)ipFlow;
				}
			}
			
			if( ipFlow)
			{
				*rgid = ipFlow->rgid;
				// #if DPI
					// if( upfndpi_protocol_detected( ipFlow->ndpi_flow) == 0)
					// {
						// upfndpi_processpacket( coreid, ipFlow->ndpi_flow, pload, ploadlen, 0);
					// }
				// #endif
			
				// #if DPI_PACE2
					// if( pfs->pace2_enabled == 1)
					// {
						// dpi_return_t ret_dpi = dpi_process_l2_frame( dpi, coreindex, dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC), (const void * const)pload, ploadlen, DPI_GTPU_ENABLED);
						
						// if (unlikely(ret_dpi != DPI_RETURN_SUCCESS)){
							// DEBUG_PRINTF("[ERROR] PACE2 process frame at ts %lu \n", ts);
						// }
					// }
				// #endif
			
				return 1;
			}
			else
			{
				return 0;
			}
		}
		
		return 0;
		
		// printf("session ptr=%p in line %d\n",session,__LINE__);
		
		
		// printf("flow-allowed dire=%u session with ueip=%u dstip=%u srcport=%u dstport=%u protocol=%u session=%p\n", 
			// direction, pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol, session);
		
		// if( ipFlow)
		// {
			// if( pfcp__flow4_is_allowed( ipFlow) == 1)
			// {
				// //printf( "last-used=%lu time=%lu  %u\n", ipFlow->lastused, (uint64_t)time(NULL), __LINE__);
				
				// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
				// }
				// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
				// {
					// printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
				// }
			// }
		// }
		
		
		// else
		// {
			// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
			// }
			// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
			// }
		// }
		
		//printf("session ptr=%p in line %d\n",session,__LINE__);
		return 0;
	}
	return 0;
}


int cp__record_usage( uint8_t * session, int direction, int rgid, int ploadlen, int enforce_qos)
{
	if( session)
	{
		pfcp_session_t * pfcpSession = (pfcp_session_t *)session;


		if( pfcpSession->stime != pfs->stime)
		{
			pfcpSession->con_0__ul_mbr = 0;
			pfcpSession->con_0__dl_mbr = 0;
			pfcpSession->stime = pfs->stime;
		}

		if( direction == 1)	//PROCESS_TYPE__GTP_Gi
		{
			if( pfcpSession->qer_0__ul_mbr > 0 && enforce_qos == 1)
			{
				if( pfcpSession->con_0__ul_mbr < pfcpSession->qer_0__ul_mbr)
				{
					pfcpSession->con_0__ul_mbr += ploadlen;
				}
				else
				{
					return 0;
				}
			}
			
			//TODO: remove below, if not commented
			//return 1;
			//printf("session ptr=%p in line %d\n",session,__LINE__);
			return (int)pfcp_stack__record_usage( (void *) session, rgid /*urr-id*/, ploadlen, 0);
		}
		else if( direction == 2) // PROCESS_TYPE__Gi_GTP
		{
			if( pfcpSession->qer_0__dl_mbr > 0 && enforce_qos == 1)
			{
				if( pfcpSession->con_0__dl_mbr < pfcpSession->qer_0__dl_mbr)
				{
					pfcpSession->con_0__dl_mbr += ploadlen;
				}
				else
				{
					return 0;
				}
			}

			//TODO: remove below,, if not commented
			//return 1;
			//printf("session ptr=%p in line %d\n",session,__LINE__);
			return (int)pfcp_stack__record_usage( (void *) session, rgid /*urr-id*/, 0, ploadlen);
		}
	}
	return 0;
}


void cp__usage( uint32_t seid, uint32_t rgid, uint32_t uplink, uint32_t downlink)
{
	pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, (uint64_t)seid);
	
	if( pfcpSession)
	{
		if( uplink > 0) {
			cp__record_usage( (uint8_t*)pfcpSession, 1, rgid, uplink, 0);
		} else if( downlink > 0) {
			cp__record_usage( (uint8_t*)pfcpSession, 2, rgid, downlink, 0);
		}
	}
}


void cp__enable_quota_logs( uint32_t enable_quota_logs)
{
	pfs->enable_quota_logs = enable_quota_logs;
}


void cp__quota( uint32_t seid)
{
	if( pfs->deviceLogger)
	{
		pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, (uint64_t)seid);
		
		if( pfcpSession)
		{
			pfcp_urr_t * sess_urr = pfcpSession->urr.head;
			
			while( sess_urr)
			{
					app_logger__log( pfs->deviceLogger, NULL, APP_LOG__LEVEL_DEBUG, 
						"up_f_seid=%ld  cp_f_seid=%ld rg-id=%d sts=%u  granted-times=%d  requested-times=%d  granted[%ld,%ld,%ld]  used[%ld,%ld,%ld]", 
						pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
						sess_urr->urr_id, sess_urr->quota_status, sess_urr->quota_granted_times, sess_urr->quota_requested_times, 
						sess_urr->granted_volume.uplink, sess_urr->granted_volume.downlink, sess_urr->granted_volume.total,
						sess_urr->used_volume.uplink, sess_urr->used_volume.downlink, sess_urr->used_volume.total
					);
				
				sess_urr = sess_urr->Next;
			}
		}
	}
}


int cp__flow_allowed_ueipv6( uint8_t* ueip, uint8_t* dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  uint8_t ** session, uint32_t * ran_teid, int direction, int coreid, uint8_t * qfi, int * rgid, int coreindex, uint8_t ** flowPtr)
{
	ip6_flow_t * flow = (ip6_flow_t*)dpe_find_ipv6flow( ueip, dstip, srcport, dstport, protocol);
	
	if(!flow) return 0;
	
	pfcp_session_t * pfcpSession = flow->session;
	
	if(!pfcpSession) return 0;
	
	*rgid = flow->rgid;
	*ran_teid = pfcpSession->ran_teid;
	*session = (uint8_t *) pfcpSession;	
	
	*qfi = pfcpSession->qer.head->qfi;
	*flowPtr = (uint8_t*)flow;

	if( pfcp__flow6_is_allowed( flow) == 1)
	{
		// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
		// {
			// //printf("session ptr=%p in line %d\n",session,__LINE__);
			// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
		// }
		// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
		// {
			// //printf("session ptr=%p in line %d\n",session,__LINE__);
			// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
		// }

		// #if DPI_PACE2
			// if( pfs->pace2_enabled == 1)
			// {
				// dpi_return_t ret_dpi = dpi_process_l2_frame( dpi, coreindex, dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC), (const void * const)pload, ploadlen, DPI_GTPU_ENABLED);
				
				// if (unlikely(ret_dpi != DPI_RETURN_SUCCESS)){
					// DEBUG_PRINTF("[ERROR] PACE2 process frame at ts %lu \n", ts);
				// }
			// }
		// #endif
				
		return 1;
	}
	return 0;
	
	
	// if(!pfs->ueipv6_table)
		// return 0;

	// pfcp_ip_flows_t * ip_flows = (pfcp_ip_flows_t *)ls__getobject_u128k( pfs->ueipv6_table, ueip);
	
	// if(!ip_flows)
	// {
		// return 0;
	// }

	// pfcp_ip6_flow_t * ipf6_flow = ip_flows->f6Head;
	// int iFound = 0;
	
	// while( ipf6_flow)
	// {
		// if( ipf6_flow->dstip[0] == dstip[0] && ipf6_flow->dstip[1] == dstip[1] && ipf6_flow->dstip[2] == dstip[2] && ipf6_flow->dstip[3] == dstip[3] && ipf6_flow->dstip[4] == dstip[4] && ipf6_flow->dstip[5] == dstip[5] && ipf6_flow->dstip[6] == dstip[6] && ipf6_flow->dstip[7] == dstip[7] && 
			// ipf6_flow->dstip[8] == dstip[8] && ipf6_flow->dstip[9] == dstip[9] && ipf6_flow->dstip[10] == dstip[10] && ipf6_flow->dstip[11] == dstip[11] && ipf6_flow->dstip[12] == dstip[12] && ipf6_flow->dstip[13] == dstip[13] && ipf6_flow->dstip[14] == dstip[14] && ipf6_flow->dstip[15] == dstip[15])
		// {
			// iFound = 1;
			// break;
		// }
		
		// ipf6_flow = ipf6_flow->FNext;
	// }
	
	// if( iFound == 0)
		// return 0;

	// pfcp_session_t * pfcpSession = NULL;
	
	// if( iFound == 1 && ipf6_flow)
	// {
		// pfcp_port_flow_t * pitem = ipf6_flow->portHead;
		// iFound = 0;
		
		// while( pitem)
		// {
			// if( pitem->dstport == srcport && pitem->srcport == dstport && pitem->protocol == protocol)
			// {
				// iFound = 1;
				// pfcpSession = pitem->session;
				// break;
			// }
			
			// pitem = pitem->FNext;
		// }
		
		// if(!pfcpSession)
			// return 0;
		
		// *ran_teid = pfcpSession->ran_teid;
		// *session = (uint8_t *) pfcpSession;

		// if( pfcpSession->qer.head )	// && pfcpSession->qer.count > 0
		// {
			// *qfi = pfcpSession->qer.head->qfi;
		// }
		
		
		// if( pfcp__flow6_is_allowed( ipf6_flow) == 1)
		// {
			// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
			// }
			// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
			// }
		// }
		// return 0;
	// }

	


	// return 0;

	
	// pfcp_session_t * pfcpSession = (pfcp_session_t *)ls__getobject_u128k( pfs->ueipv6_table, ueip);
	
	// if( pfcpSession)
	// {
		// *ran_teid = pfcpSession->ran_teid;
		// *session = (uint8_t *) pfcpSession;


		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "%p session context found ueip=%u|%u  %u.%u.%u.%u  ran-ip=%u.%u.%u.%u  ran-mac:%02X:%02X:%02X:%02X:%02X:%02X  ran_teid=%u   %s|%s|%d", 
			// // pfcpSession, hueip, htonl(hueip), 
			// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF, 
			// // pfcpSession->ran_ipv4 & 0xFF, (pfcpSession->ran_ipv4 >> 8) & 0xFF, (pfcpSession->ran_ipv4 >> 16) & 0xFF, (pfcpSession->ran_ipv4 >> 24) & 0xFF,
			// // pfcpSession->ranmac[0] & 0xFF, pfcpSession->ranmac[1] & 0xFF, 
			// // pfcpSession->ranmac[2] & 0xFF, pfcpSession->ranmac[3] & 0xFF, 
			// // pfcpSession->ranmac[4] & 0xFF, pfcpSession->ranmac[5] & 0xFF, *ran_teid,
		// // __FILE__, __FUNCTION__, __LINE__);


		
		
		// if( pfcpSession->qer.head )	// && pfcpSession->qer.count > 0
		// {
			// *qfi = pfcpSession->qer.head->qfi;
		// }
		
		// pfcp_ip4_flow_t * ipFlow = NULL;
		
		// // if( pfs->initDPI == 1 ) 
		// // {
			// // ipFlow = pfcp__find_or_create_flow( pfcpSession, pfcpSession->ue_ipv4, dstip, srcport, dstport);

			// // if( ipFlow)
			// // {
				// // #if DPI
					// // if( upfndpi_protocol_detected( ipFlow->ndpi_flow) == 0)
					// // {
						// // upfndpi_processpacket( coreid, ipFlow->ndpi_flow, pload, ploadlen, 0);
					// // }
				// // #endif
			// // }
		// // }
		
		// if( ipFlow)
		// {
			// if( pfcp__flow4_is_allowed( ipFlow) == 1)
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);		
				// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
				// }
				// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
				// }
				// else
				// {
					// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "NOT IMPLEMENTED  ueip=%u|%u   %u.%u.%u.%u    %s|%s|%d", 
						// // hueip, htonl(hueip), 
						// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
						// // __FILE__, __FUNCTION__, __LINE__
					// // );
					// //exit(0);
					// return 0;
				// }
			// }
		// }
		// else
		// {
			// return 1;
		// }
		
		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "enf of process  session context not found ueip=%u|%u   %u.%u.%u.%u    %s|%s|%d", 
			// // hueip, htonl(hueip), 
			// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
			// // __FILE__, __FUNCTION__, __LINE__
		// // );

		// return 0;
	// }
	// return 0;
}



int cp__flow_allowed_ueipv4( uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  uint8_t ** session, uint32_t * ran_teid, int direction, int coreid, uint8_t * qfi, int * rgid, int coreindex, uint8_t ** flowPtr)
{
	//uint32_t hueip = ((ueip << 8) & 0xFF00FF00 ) | ((ueip >> 8) & 0xFF00FF ); 
	//hueip = (hueip << 16) | (hueip >> 16);
	uint32_t hueip = htonl(ueip);
	
	//ipFlow = (ip4_flow_t*)dpe_find_ipv4flow( pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol);
	ip4_flow_t * flow = (ip4_flow_t*)dpe_find_ipv4flow( hueip, dstip, srcport, dstport, protocol);
	
	if(!flow) 
	{
		/*
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "session not found with  ueip=%u|%u   %u.%u.%u.%u      %u  %u  %u  %u  %u      %s|%s|%d", 
			hueip, htonl(hueip), 
			hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
			hueip, dstip, srcport, dstport, protocol,
			__FILE__, __FUNCTION__, __LINE__
		);
		*/
		
		return 0;
	}
	
	pfcp_session_t * pfcpSession = flow->session;
	
	if(!pfcpSession) return 0;
	
	*rgid = flow->rgid;
	*ran_teid = pfcpSession->ran_teid;
	*session = (uint8_t *) pfcpSession;	
	
	*qfi = pfcpSession->qer.head->qfi;
	*flowPtr = (uint8_t*)flow;

	if( pfcp__flow4_is_allowed( flow) == 1)
	{
		// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
		// {
			// //printf("session ptr=%p in line %d\n",session,__LINE__);
			// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
		// }
		// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
		// {
			// //printf("session ptr=%p in line %d\n",session,__LINE__);
			// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
		// }

		// #if DPI_PACE2
			// if( pfs->pace2_enabled == 1)
			// {
				// dpi_return_t ret_dpi = dpi_process_l2_frame( dpi, coreindex, dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC), (const void * const)pload, ploadlen, DPI_GTPU_ENABLED);
				
				// if (unlikely(ret_dpi != DPI_RETURN_SUCCESS)){
					// DEBUG_PRINTF("[ERROR] PACE2 process frame at ts %lu \n", ts);
				// }
			// }
		// #endif
				
		return 1;
	}
	return 0;
	
	
	
	// pfcp_ip_flows_t * ip_flows = NULL;
	// ip_flows = (pfcp_ip_flows_t *)ls__getobject_u32k( pfs->ueipv4_table, hueip);
	
	// if(!ip_flows)
	// {
		// return 0;
	// }
	
	
	// pfcp_ip4_flow_t * ipf4_flow = ip_flows->f4Head;
	// int iFound = 0;
	
	// while( ipf4_flow)
	// {
		// if( ipf4_flow->dstip == dstip)
		// {
			// iFound = 1;
			// break;
		// }
		
		// ipf4_flow = ipf4_flow->FNext;
	// }
	
	// if( iFound == 0)
		// return 0;
	

	// pfcp_session_t * pfcpSession = NULL;
	
	// if( iFound == 1 && ipf4_flow)
	// {
		// pfcp_port_flow_t * pitem = ipf4_flow->portHead;
		// iFound = 0;
		
		// while( pitem)
		// {
			// if( pitem->dstport == srcport && pitem->srcport == dstport && pitem->protocol == protocol)
			// {
				// iFound 			= 1;
				// pitem->lastused	= (uint64_t)time(NULL);
				// pfcpSession 	= pitem->session;
				// break;
			// }
			
			// pitem = pitem->FNext;
		// }
		
		// if(!pfcpSession && iFound == 0)
			// return 0;
		
		// *ran_teid = pfcpSession->ran_teid;
		// *session = (uint8_t *) pfcpSession;

		// if( pfcpSession->qer.head )	// && pfcpSession->qer.count > 0
		// {
			// *qfi = pfcpSession->qer.head->qfi;
		// }
		
		
		// if( ipf4_flow)
		// {
			// if( pfcp__flow4_is_allowed( ipf4_flow) == 1)
			// {
				// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
				// }
				// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
				// }
			// }
		// }
		// return 0;
	// }
	
	// pfcp_session_t * pfcpSession = (pfcp_session_t *)ls__getobject_u32k( pfs->ueipv4_table, hueip);
	// *qfi = 1;
	
	// // printf("finding flow-allowed dire=%u session with ueip=%u dstip=%u srcport=%u dstport=%u protocol=%u session=%p\n", 
		// // direction, hueip, dstip, srcport, dstport, protocol, pfcpSession);
	// //printf("session ptr=%p in line %d\n",session,__LINE__);
	// if( pfcpSession)
	// {
		// *ran_teid = pfcpSession->ran_teid;
		// *session = (uint8_t *) pfcpSession;


		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "%p session context found ueip=%u|%u  %u.%u.%u.%u  ran-ip=%u.%u.%u.%u  ran-mac:%02X:%02X:%02X:%02X:%02X:%02X  ran_teid=%u   %s|%s|%d", 
			// // pfcpSession, hueip, htonl(hueip), 
			// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF, 
			// // pfcpSession->ran_ipv4 & 0xFF, (pfcpSession->ran_ipv4 >> 8) & 0xFF, (pfcpSession->ran_ipv4 >> 16) & 0xFF, (pfcpSession->ran_ipv4 >> 24) & 0xFF,
			// // pfcpSession->ranmac[0] & 0xFF, pfcpSession->ranmac[1] & 0xFF, 
			// // pfcpSession->ranmac[2] & 0xFF, pfcpSession->ranmac[3] & 0xFF, 
			// // pfcpSession->ranmac[4] & 0xFF, pfcpSession->ranmac[5] & 0xFF, *ran_teid,
		// // __FILE__, __FUNCTION__, __LINE__);


		
		
		// if( pfcpSession->qer.head )	// && pfcpSession->qer.count > 0
		// {
			// *qfi = pfcpSession->qer.head->qfi;
		// }
		
		// pfcp_ip4_flow_t * ipFlow = NULL;
		
		// if( pfs->initDPI == 1 ) 
		// {
			// ipFlow = pfcp__find_or_create_flow( pfcpSession, pfcpSession->ue_ipv4, dstip, srcport, dstport, protocol);

			// if( ipFlow)
			// {
				// // #if DPI
					// // if( upfndpi_protocol_detected( ipFlow->ndpi_flow) == 0)
					// // {
						// // upfndpi_processpacket( coreid, ipFlow->ndpi_flow, pload, ploadlen, 0);
					// // }
				// // #endif
			// }
		// }
		
		// if( ipFlow)
		// {
			// if( pfcp__flow4_is_allowed( ipFlow) == 1)
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);		
				// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
				// }
				// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
				// {
					// //printf("session ptr=%p in line %d\n",session,__LINE__);
					// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
				// }
				// else
				// {
					// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "NOT IMPLEMENTED  ueip=%u|%u   %u.%u.%u.%u    %s|%s|%d", 
						// hueip, htonl(hueip), 
						// hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
						// __FILE__, __FUNCTION__, __LINE__
					// );
					// //exit(0);
					// return 0;
				// }
			// }
		// }
		// else
		// {
			// if( direction == 1)	//PROCESS_TYPE__GTP_Gi
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, ploadlen, 0);
			// }
			// else if( direction == 2) // PROCESS_TYPE__Gi_GTP
			// {
				// //printf("session ptr=%p in line %d\n",session,__LINE__);
				// return (int)pfcp_stack__record_usage( (void *) pfcpSession, 1 /*urr-id*/, 0, ploadlen);
			// }
		// }
		
		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "enf of process  session context not found ueip=%u|%u   %u.%u.%u.%u    %s|%s|%d", 
			// // hueip, htonl(hueip), 
			// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
			// // __FILE__, __FUNCTION__, __LINE__
		// // );

		// return 0;
	// }
	// // else
	// // {
		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "session context not found ueip=%u|%u   %u.%u.%u.%u    %s|%s|%d", 
			// // hueip, htonl(hueip),  
			// // hueip & 0xFF, (hueip >> 8) & 0xFF, (hueip >> 16) & 0xFF, (hueip >> 24) & 0xFF,
			// // __FILE__, __FUNCTION__, __LINE__
		// // );
	// // }
	
	return 0;
}

int cp__get_ranv4info( uint8_t * session, uint8_t * ranip_v, uint32_t * ranip4, __uint128_t * ranip6, uint8_t ** mac)
{
	if( session)
	{
		pfcp_session_t * pfcpSession = (pfcp_session_t *)session;
		*ranip_v 	= 4;
		
		 // memcpy( pfcpSession->ranmac, "\x00\x17\x5A\x8D\xA7\xC3", 6);
		// // // pfcpSession->ran_ipv4 = 0x17130A0A; 
		 // pfcpSession->ran_ipv4 = 0xdc130a0a; //10.10.19.220 -ensfo  
		
		*ranip4 	= pfcpSession->ran_ipv4;	//from pfcp session modification request
		*mac		= pfcpSession->ranmac;
		
		
		
		
		// app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "get raninfo for session=%p RAN-IP=%u.%u.%u.%u  MAC=%02X:%02X:%02X:%02X:%02X:%02X   %s|%s|%d", 
			// session, pfcpSession->ran_ipv4 & 0xFF, (pfcpSession->ran_ipv4 >> 8) & 0xFF, (pfcpSession->ran_ipv4 >> 16) & 0xFF, (pfcpSession->ran_ipv4 >> 24) & 0xFF,
			// pfcpSession->ranmac[0] & 0xFF, pfcpSession->ranmac[1] & 0xFF, 
			// pfcpSession->ranmac[2] & 0xFF, pfcpSession->ranmac[3] & 0xFF, 
			// pfcpSession->ranmac[4] & 0xFF, pfcpSession->ranmac[5] & 0xFF,
		// __FILE__, __FUNCTION__, __LINE__);		
		
		
		return 1;
	}
	return 0;
}

int cp__set_mac( uint8_t * session, uint8_t * mac)
{
	if( session) {
		pfcp_session_t * pfcpSession = (pfcp_session_t *)session;
		
		if( pfcpSession->ranmac[5] == 0) {
			pfcpSession->ranmac[0] = mac[0], pfcpSession->ranmac[1] = mac[1], pfcpSession->ranmac[2] = mac[2], pfcpSession->ranmac[3] = mac[3], pfcpSession->ranmac[4] = mac[4], pfcpSession->ranmac[5] = mac[5];
		}
	}
	return 0;
}


void dpdk_qos__find_user_pipe_and_q( uint64_t seid, uint16_t * pipe, uint16_t * nongbr_queue, uint16_t * gbr_queue);

int cp__get_tcinfo( uint8_t * sess, uint32_t * subport, uint16_t * pipe, uint32_t * traffic_class, uint32_t * queue, uint32_t rgid, int side)
{
	if( sess) 
	{
		pfcp_session_t * pfcpSession = (pfcp_session_t *)sess;
		
		if( rgid == 0 )
		{
			pfcp_qer_t * qer = pfcpSession->qer.head;
			uint16_t nongbr_queue = 0;
			uint16_t gbr_queue = 0;
			
			if( side == 1)
			{
				if( qer->ul_tc == 100)
					return -1;
				
				
				dpdk_qos__find_user_pipe_and_q( pfcpSession->up_f_seid, pipe, &nongbr_queue, &gbr_queue);
				*queue = gbr_queue;
			}
			else if( side == 2)
			{
				if( qer->dl_tc == 100)
					return -1;

				dpdk_qos__find_user_pipe_and_q( pfcpSession->up_f_seid, pipe, &nongbr_queue, &gbr_queue);
				*queue = gbr_queue;				
			}
			else
			{
				return -1;
			}
		}
		
		
		return 0;
	}
	return -1;
}


void pfcp_stack__set_upf_ip( uint32_t ipv4, uint8_t * ipv6)
{
	pfs->upf_ipv4 = ipv4;
	
	if (ipv6) {
		memcpy( pfs->upf_ipv6, ipv6, 16);
	}
}

pfcp_stack_t * pfcp_stack__get_instance()
{
	return pfs;
}

pfcp_session_t * pfcp_stack__find_session( uint64_t seid)
{
	if( seid > 0) {
		return (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, seid);
	}
	return NULL;
}

// uint8_t * pfcp_stack__find_dpi_session( uint64_t seid)
// {
	// pfcp_session_t * sess = pfcp_stack__find_session( seid);
	// if( sess != NULL)
	// {
		// return (uint8_t*) &sess->dpi;
	// }
	// return NULL;
// }

uint64_t pfcp_stack__find_upf_seid( void * ptr)
{
	if( ptr)
	{
		return ((pfcp_session_t*)ptr)->up_f_seid;
	}
	return 0;
}

uint64_t pfcp_stack__get_quota( void * sPtr, uint16_t rgid)
{
	pfcp_session_t * sess = ((pfcp_session_t*)sPtr);
	if( sess != NULL)
	{
		pfcp_urr_t * urrItem = sess->urr.head;
		
		while( urrItem)
		{
			if( urrItem->urr_id == rgid && urrItem->isremoved == 0)
			{
				//find far and if not allowed return 0;
				
				return (urrItem->granted_volume.total - urrItem->used_volume.total);
			}
			urrItem = urrItem->Next;
		}
	}
	return 0;
}


uint8_t pfcp_stack__get_qfi( void * sPtr)
{
	pfcp_session_t * sess = ((pfcp_session_t*)sPtr);
	
	if(sess)
	{
		pfcp_qer_t * pqer = sess->qer.head;
		
		while(pqer)
		{
			if( pqer->qfi_isset == 1)
				return pqer->qfi;
			
			pqer = pqer->Next;
		}
	}
	return 0;
}


void pfcp_stack__get_qerinfo( void * sPtr, uint8_t * found, uint8_t * qfi, uint8_t * gate_status, uint64_t * ul_mbr, uint64_t * dl_mbr)
{
	*found = 0;
	
	pfcp_session_t * sess = ((pfcp_session_t*)sPtr);
	
	if(sess)
	{
		pfcp_qer_t * pqer = sess->qer.head;
		
		while(pqer)
		{
			if( pqer->qfi_isset == 1)
			{
				*found 			= 1;
				*qfi 			= pqer->qfi;
				*ul_mbr 		= pqer->ul_mbr;
				*dl_mbr 		= pqer->dl_mbr;
				*gate_status 	= pqer->gate_status;
				break;
			}
			
			pqer = pqer->Next;
		}
	}
}



pfcp_urr_t * pfcp_stack___get_session_urr( pfcp_session_t * pfcpSession, uint32_t urr_id, int * added);
uint32_t pfcp_stack__get_next_sequence( struct pfcp_node * node);
void pfcp_stack__perf_inc_sur();
int pfcp_stack___send_request( pfcp_message_t * pfcp_msg, struct pfcp_node * node);

struct pfcp_node * pfcp_stack___find_node_by_ip( uint32_t ipv4);


void pfcp_stack___send_session_report_request__upir( pfcp_session_t * sess)
{
	if(!sess->node)
	{
		sess->node = pfcp_stack___find_node_by_ip( sess->nodeIPv4);
	}
	
	if(!sess->node)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"pfcp_node not found with ip=%u   %s|%s|%d", sess->nodeIPv4, __FILE__, __FUNCTION__, __LINE__);
		sess->idle_session_hb_count++;
		return;
	}
	
	uint32_t seqno = pfcp_stack__get_next_sequence( sess->node);

	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_SESSION_REPORT_REQUEST, 1, sess->cp_f_seid, seqno);
	pfcp_session_report_request_t * imsg = &msg.u.session_report_request;
		
	__up_pfcp_set__u8( &imsg->report_type, 8);
	
	pfcp_stack___send_request( &msg, sess->node);
	pfcp_stack__perf_inc_sur();
	sess->idle_session_hb_count++;
	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
		"sending pfcp report-request for cp_f_seid=%lu seqno=%u  upir %s|%s|%d", sess->cp_f_seid, seqno, __FILE__, __FUNCTION__, __LINE__);	
			
}

void pfcp_stack___send_session_report_request__fsm( uint8_t * data, int tIndex)
{
	pfcp_msg_helper_t * msg_helper = ((pfcp_msg_helper_t*)data);
	
	if( msg_helper)
	{
		pfcp_session_t * sess = (pfcp_session_t*)msg_helper->session;
		pfcp_urr_t * sess_urr = (pfcp_urr_t *)msg_helper->obj1;

		app_region__free( (uint8_t *)msg_helper);
	
		if( sess->released == 1)
			return;

		if(!sess->node)
		{
			sess->node = pfcp_stack___find_node_by_ip( sess->nodeIPv4);
			
			if(!sess->node)
			{
				// printf("unble to send urr as sess->node is NULL  cp-seid=%lu up-seid=%lu nodeIpv4=%u  %s|%s|%d\n", 
					// sess->cp_f_seid, sess->up_f_seid, sess->nodeIPv4, __FILE__, __FUNCTION__, __LINE__);
				
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
					"unble to send urr as sess->node is NULL  cp-seid=%lu up-seid=%lu nodeIpv4=%u  %s|%s|%d", 
					sess->cp_f_seid, sess->up_f_seid, sess->nodeIPv4, __FILE__, __FUNCTION__, __LINE__);
			}

			return;
		}

		uint32_t seqno = 0;
		uint32_t retry = 0;
		pthread_mutex_lock( &sess->UrrSeqLock);
		
		if((sess_urr->quota_status == 1 &&  sess_urr->quota_requested_time > 0 && (sess_urr->quota_requested_time + 10) > time(NULL)))
		{
			if( pfs->enable_quota_logs == 1)
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, "cp-seid=%lu up-seid=%lu  already requested for quota  quota_status=%u quota_requested_time=%lu time=%lu", 
					sess->cp_f_seid, sess->up_f_seid, sess_urr->quota_status, sess_urr->quota_requested_time, time(NULL));
			}
			
			pthread_mutex_unlock( &sess->UrrSeqLock);
			return;
		}
		
		if( sess_urr->quota_status == 0)
		{
			sess_urr->quota_status = 1;
			sess_urr->quota_requested_time = time( NULL);
			sess_urr->last_quota_requested_time = time( NULL);
			
			sess_urr->pfcp_seqno = pfcp_stack__get_next_sequence( sess->node);
			sess_urr->pfcp_report_request_seqno = sess->urr_rep_seq_no;
			
			sess->urr_rep_seq_no++;
		}
		else
		{
			retry = 1;
			sess_urr->quota_requested_time = time( NULL);
			sess_urr->last_quota_requested_time = time( NULL);
		}
		
		sess_urr->quota_requested_times++;
		seqno = sess_urr->pfcp_seqno;
		
		pthread_mutex_unlock( &sess->UrrSeqLock);
		
		

		
		char urseqn[4];	
		app_ep__encode__u32toc( urseqn, sess_urr->pfcp_report_request_seqno);
		
		pfcp_message_t msg;
		__up_pfcp_set__header( &msg, PFCP_SESSION_REPORT_REQUEST, 1, sess->cp_f_seid, seqno);
		pfcp_session_report_request_t * imsg = &msg.u.session_report_request;

		__up_pfcp_set__u8( &imsg->report_type, 2);
		
		imsg->usage_report.presence = 1;
		__up_pfcp_set__u32( &imsg->usage_report.urr_id, sess_urr->urr_id);
		__up_pfcp_set__octet( pfs->pfcp_mregion, &imsg->usage_report.ur_seqn, urseqn, 4);
			
		char volume_measurement[26];
		memset( volume_measurement, 0, sizeof(volume_measurement));
		volume_measurement[0] = 7;
		uint32_t volume_measurement_len = 25;
		
		// if( sess_urr->granted_volume.total > 0)
		// {
			// //printf("URR Function ptr =%p in line %d\n",sess,__LINE__);	
			
			// // if( sess_urr->granted_volume.uplink > sess_urr->used_volume.uplink) {
				// // app_ep__encode__u64toc( &volume_measurement[ 9], sess_urr->granted_volume.uplink 	- sess_urr->used_volume.uplink);
			// // } else {
				// // app_ep__encode__u64toc( &volume_measurement[ 9], sess_urr->used_volume.uplink);
			// // }
			
			// // if( sess_urr->granted_volume.downlink > sess_urr->used_volume.downlink) {
				// // app_ep__encode__u64toc( &volume_measurement[17], sess_urr->granted_volume.downlink 	- sess_urr->used_volume.downlink);
			// // } else {
				// // app_ep__encode__u64toc( &volume_measurement[17], sess_urr->used_volume.downlink);
			// // }
		
			// // if( sess_urr->granted_volume.uplink > 0 && sess_urr->granted_volume.downlink > 0)
			// // {
				// // // uint64_t uplink = sess_urr->used_volume.uplink - sess_urr->granted_volume.uplink;
				// // // uint64_t downlink = sess_urr->used_volume.downlink - sess_urr->granted_volume.downlink;
				
				// // // // sess_urr->granted_volume.uplink += uplink;
				// // // // sess_urr->granted_volume.downlink += downlink;
				
				// // // app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
				// // // app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
				// // // app_ep__encode__u64toc( &volume_measurement[17], downlink);
				
				
			// // }
			// // else
			// // {
				
			// // }
			
			// uint64_t uplink = sess_urr->used_volume.uplink - sess_urr->reported_volume.uplink;
			// uint64_t downlink = sess_urr->used_volume.downlink - sess_urr->reported_volume.downlink;
			
			// app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
			// app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
			// app_ep__encode__u64toc( &volume_measurement[17], downlink);
			
			// sess_urr->reported_volume.uplink = uplink;
			// sess_urr->reported_volume.downlink = downlink;
		// }
		// else
		// {
			// //printf("URR Function ptr =%p in line %d\n",sess,__LINE__);
			// uint64_t uplink = sess_urr->used_volume.uplink - sess_urr->granted_volume.uplink;
			// uint64_t downlink = sess_urr->used_volume.downlink - sess_urr->granted_volume.downlink;
			
			// // sess_urr->granted_volume.uplink += uplink;
			// // sess_urr->granted_volume.downlink += downlink;
			
			// //app_ep__encode__u64toc( &volume_measurement[ 1], sess_urr->used_volume.total);
			// app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
			// app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
			// app_ep__encode__u64toc( &volume_measurement[17], downlink);	
		// }
		
		if( retry == 0)
		{
			uint64_t uplink 	= sess_urr->used_volume.uplink - sess_urr->total_reported_volume.uplink;
			uint64_t downlink 	= sess_urr->used_volume.downlink - sess_urr->total_reported_volume.downlink;
			
			app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
			app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
			app_ep__encode__u64toc( &volume_measurement[17], downlink);
			
			sess_urr->reported_volume.uplink 			= uplink;
			sess_urr->reported_volume.downlink 			= downlink;
			
			sess_urr->total_reported_volume.uplink 		+= uplink;
			sess_urr->total_reported_volume.downlink 	+= downlink;			
		}
		else
		{
			uint64_t uplink 	= sess_urr->reported_volume.uplink;
			uint64_t downlink 	= sess_urr->reported_volume.downlink;

			app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
			app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
			app_ep__encode__u64toc( &volume_measurement[17], downlink);
		}
		
		__up_pfcp_set__octet( pfs->pfcp_mregion, &imsg->usage_report.volume_measurement, volume_measurement, volume_measurement_len);



		
		pfcp_stack___send_request( &msg, sess->node);
		pfcp_stack__perf_inc_sur();
		#ifdef ENABLE_PROMETHEUS
			successful_task_request( "POST",3);
		#endif
		//printf("URR Function ptr =%p in line %d\n",sess,__LINE__);
		
		if( pfs->enable_quota_logs == 1)
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"requesting quota for cp_f_seid=%lu with seq-no=%u urr-id=%d %s|%s|%d", sess->cp_f_seid, seqno,sess_urr->urr_id, __FILE__, __FUNCTION__, __LINE__);
		}
		// printf( "sent usage report request  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);	
	}
}



uint64_t pfcp_stack__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink)
{
	pfcp_session_t * sess = ((pfcp_session_t*)sPtr);
	
//	printf("session=%p %d\n",sess,__LINE__);
	
	if( sess != NULL)
	{
		// printf("session released=%d %d\n",sess->released,__LINE__);

		if( sess->released == 1)
			return 0;
		
		int added = 0;
		pfcp_urr_t * sess_urr = pfcp_stack___get_session_urr( sess, rgid, &added);

		if(!sess_urr)
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL,
				"up_f_seid=%lu  cp_f_seid=%lu  sess_urr  not found with RatingGroup=%d  %s|%d",
				sess->up_f_seid, sess->cp_f_seid, rgid, __FILE__,__LINE__);			
			return 0;
		}

		// printf("session ptr = %p added=%d in line %d\n",sess, added, __LINE__);

		if( added == 1)
		{
			if( pfs->enable_quota_logs == 1)
			{
				app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
					"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  adding RG now, and sending quota request", 
						sess->up_f_seid, sess->cp_f_seid, rgid);			
			}
			
			sess_urr->quota_status = 0;
			sess_urr->used_volume.uplink 	+= (uplink);
			sess_urr->used_volume.downlink 	+= (downlink);
			sess_urr->used_volume.total 	+= (uplink + downlink);	
		
			pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_msg_helper_t) + 1);
			
			if( msg_helper)
			{
				msg_helper->obj1 = (uint8_t*)sess_urr;
				msg_helper->session = sess;
				app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, sess->up_f_seid, pfcp_stack___send_session_report_request__fsm);
			}
			//allow first packet
			
			// printf("Packet is allowed %d\n",__LINE__);
			return 1;
		}
		else
		{
			// printf("quota_status=%u %d\n", sess_urr->quota_status, __LINE__);

			if( sess_urr->quota_status == 1 && sess_urr->granted_volume.total == 0)
			{
				//sess_urr->quota_status == 1  already sent request
				//printf("%s %d\n", __FUNCTION__, __LINE__);
				
				if( (sess_urr->quota_requested_time + 10) < time(NULL))
				{
					if( sess_urr->quota_requested_times < 10)
					{
						pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_msg_helper_t) + 1);
				
						if( msg_helper)
						{
							msg_helper->obj1 = (uint8_t*)sess_urr;
							msg_helper->session = sess;
							app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, sess->up_f_seid, pfcp_stack___send_session_report_request__fsm);
						}
					}
					else
					{
						if( pfs->enable_quota_logs == 1)
						{
							app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
								"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  granted_volume=0 quota_status=1   exceeded quota_requested_times=%d", 
									sess->up_f_seid, sess->cp_f_seid, rgid, sess_urr->quota_requested_times);
						}
						
						if((sess_urr->quota_requested_time + (10 * 60)) < time(NULL))
						{
							sess_urr->quota_requested_times = 0;
						}
						
						return 0;
					}
					
				}
				else
				{
					if( pfs->enable_quota_logs == 1)
					{
						app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
							"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  granted_volume=0 quota_status=1   already sent quota request", 
								sess->up_f_seid, sess->cp_f_seid, rgid);
					}
				}
				return 0;
			}
			else if( sess_urr->granted_volume.total > 0 && (sess_urr->used_volume.total + uplink + downlink) > 0)
			{
				// printf("%s %d\n", __FUNCTION__, __LINE__);
				
				long double total = sess_urr->granted_volume.total;
				long double used = (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink + uplink + downlink);
				double used_percent = ( (used / total) * 100);
				
				// printf("used_percent = %f Total =%Lf used =%Lf uplink =%ld downlink = %ld and line %d\n",used_percent,total,used,uplink,downlink,__LINE__);
				
				if( pfs->enable_quota_logs == 1)
				{
					app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG,
						"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  used_percent = %f Total=%Lf used=%Lf uplink=%ld downlink=%ld   %d",
						sess->up_f_seid, sess->cp_f_seid, rgid, used_percent, total, used, uplink, downlink,__LINE__);
				}
				
				//  && used_percent <= 100
				if( used_percent >= 75)
				{
					if( sess_urr->quota_status != 1 || sess_urr->quota_status == 1 && (sess_urr->quota_requested_time + 10) < time(NULL))
					{
						//sess_urr->quota_status = 0;
						
						if( used_percent < 100)
						{
							sess_urr->used_volume.uplink 	+= (uplink);
							sess_urr->used_volume.downlink 	+= (downlink);
							sess_urr->used_volume.total 	+= (uplink + downlink);	
						}

						if( sess_urr->quota_requested_times < 10)
						{
							pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_msg_helper_t) + 1);
							
							if( msg_helper)
							{
								msg_helper->obj1 = (uint8_t*)sess_urr;
								msg_helper->session = sess;
								app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, sess->up_f_seid, pfcp_stack___send_session_report_request__fsm);
							
								
								if( used_percent >= 100)
								{
									return 0;
								}
								
								return 1;
							}
						}
						else
						{
							if( pfs->enable_quota_logs == 1)
							{
								app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
									"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  granted_volume=0 quota_status=1   exceeded quota_requested_times=%d", 
									sess->up_f_seid, sess->cp_f_seid, rgid, sess_urr->quota_requested_times);
							}
							
							if((sess_urr->quota_requested_time + (10 * 60)) < time(NULL))
							{
								sess_urr->quota_requested_times = 0;
							}
							

							if( used_percent >= 100)
							{
								return 0;
							}
							
							return 1;
						}
					}
				}
				
				if( used_percent >= 100)
				{
					//printf("3 Packet is rejected %d\n",__LINE__);
					return 0;
				}

				//sess_urr->quota_status = 0;
				sess_urr->used_volume.uplink 	+= (uplink);
				sess_urr->used_volume.downlink 	+= (downlink);
				sess_urr->used_volume.total 	+= (uplink + downlink);	
					
				return 1;
			}
			else if( sess_urr->granted_volume.total == 0 && sess_urr->quota_status == 0)
			{
				//printf("granted_volume.total=%lu  %s %d\n", sess_urr->granted_volume.total, __FUNCTION__, __LINE__);
			
				if( pfs->enable_quota_logs == 1)
				{
					app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
						"up_f_seid=%lu  cp_f_seid=%lu  rgid=%d  granted_volume=0 quota_status=0[quota not requested, sending quota request now] hence rejected", 
							sess->up_f_seid, sess->cp_f_seid, rgid);
				}
				
				sess_urr->used_volume.uplink 	+= (uplink);
				sess_urr->used_volume.downlink 	+= (downlink);
				sess_urr->used_volume.total 	+= (uplink + downlink);	
				
				pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_msg_helper_t) + 1);
					
				if( msg_helper)
				{
					msg_helper->obj1 = (uint8_t*)sess_urr;
					msg_helper->session = sess;
					app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, sess->up_f_seid, pfcp_stack___send_session_report_request__fsm);
				}
				
				//printf("1 - packet is rejected %d\n",__LINE__);
				return 0;
			}
		}
	}
	
	//printf("2 - packet is rejected %d\n",__LINE__);
	return 0;
}

//uint32_t pfcp_stack___get_ueip( pfcp_session_t * pfcpSession);
uint16_t pfcp_stack___get_ranip( pfcp_session_t * pfcpSession, uint32_t * ranip, uint32_t * ranteid);

int pfcp_stack__session_ohi( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no)
{
	if( sPtr)
	{
		pfcp_session_t * sess = ((pfcp_session_t*)sPtr);
		
		
		
		if(0)	//disabled
		{
			uint32_t ueip = 0;
			
			
			uint32_t ran_ip = 0;
			uint32_t ran_teid = 0;
			uint32_t oh_ipv4 = 0;
			uint32_t oh_teid = 0;
	
			//ueip = pfcp_stack___get_ueip( sess);
			
			ueip = sess->ue_ipv4;
			pfcp_stack___get_ranip( sess, &ran_ip, &ran_teid);
	
			char ueip_str[INET_ADDRSTRLEN];
			memset( ueip_str, 0, INET_ADDRSTRLEN);

			char ranip_str[INET_ADDRSTRLEN];
			memset( ranip_str, 0, INET_ADDRSTRLEN);	
			
			char ohip_str[INET_ADDRSTRLEN];
			memset( ohip_str, 0, INET_ADDRSTRLEN);				
			
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP-OHI: sess=%p|%p UPF-SEID=%lu CP-SEID=%lu  ueip=%u|%s  ran->upf-ip=%u|%s  ran->upf-teid=%u  upf->ran-ip=%u|%s  upf->ran-teid=%u  %s|%s|%d", 
				sess, sPtr, sess->up_f_seid, sess->cp_f_seid, ueip, ueip_str, ran_ip, ranip_str, ran_teid, 
				oh_ipv4, ohip_str, oh_teid, __FILE__, __FUNCTION__, __LINE__);			
		}
		
		
		pfcp_pdr_t * pdr = sess->pdr.head;
		
		if(!pdr)
			return -1;
		
		*ipv 	= 4;
		*upfip 	= pfs->upf_ipv4;
		*upfip6 = pfs->upf_ipv6;
		
		//printf( "pfs->upf_ipv4=%u\n", pfs->upf_ipv4);
		
		
		/*
		while(pdr)
		{
			if( pdr->source_interface == 0)	// access
			{
				*ranip = pdr->ipv4;
				*ranip6 = pdr->ipv6;
				break;
			}
			
			pdr = pdr->Next;
		}
		*/
		
		pfcp_stack___get_ranip( sess, ranip, teid);
		
		/*
		pdr = sess->pdr.head;
		uint32_t far_id = 0;
		
		while(pdr)
		{
			if( pdr->source_interface == 1)	// core
			{
				far_id = pdr->far_id;
				break;
			}
			
			pdr = pdr->Next;
		}
		
		if( far_id == 0)
			return -2;
			
		pfcp_far_t * far = sess->far.head;	
		
		if(!far)
			return -3;
		
		while(far)
		{
			//printf( "far_id=%u far->far_id=%u  teid=%u  ohc=%u  ipv4=%u\n", far_id, far->far_id, far->teid, far->outer_header_creation, far->ipv4);
			if( far->far_id == far_id)
			{
				break;
			}
			
			far = far->Next;
		}
		
		if(!far)
			return -4;
		
		*teid = far->teid;
		*/
		
		
		*ranmac = "\x00\x00\x00\x00\x00\x00";
		*upfmac = "\x00\x00\x00\x00\x00\x00";
		
		*gtpHasSQN = 1;
		*gtp_seq_no = ( sess->gtp_seq_no++ & 0xFFFF);
		
		return 1;
	}
	// *teid = 1234;
	// *ipv = 4;
	// *ranip = 2130706433;
	// *upfip = 2130706434;
	// *ranip6 = NULL;
	// *upfip6 = NULL;
	// *ranmac = "\x00\x00\x00\x01\x02\x03";
	// *upfmac = "\x00\x00\x00\x33\x03\x03";
	// *gtpHasSQN = 1;
	// *gtp_seq_no = 12345;
	
	return 0;
}



void pfcp_stack__perf_inc_ser()
{
	pthread_mutex_lock( &pfs->ser_lock);
	pfs->ser_total++;
	pthread_mutex_unlock( &pfs->ser_lock);
}

// void pfcp_stack__perf_inc_ser()
// {
	// pthread_mutex_lock( &pfs->ser_lock);
	// successful_task_request( "POST",1);
	// pthread_mutex_unlock( &pfs->ser_lock);
// }
void pfcp_stack__perf_inc_sea()
{
	pthread_mutex_lock( &pfs->sea_lock);
	pfs->sea_total++;
	pthread_mutex_unlock( &pfs->sea_lock);
}

void pfcp_stack__perf_inc_smr()
{
	pthread_mutex_lock( &pfs->smr_lock);
	pfs->smr_total++;
	pthread_mutex_unlock( &pfs->smr_lock);
}

void pfcp_stack__perf_inc_sma()
{
	pthread_mutex_lock( &pfs->sma_lock);
	pfs->sma_total++;
	pthread_mutex_unlock( &pfs->sma_lock);
}

void pfcp_stack__perf_inc_sdr()
{
	pthread_mutex_lock( &pfs->sdr_lock);
	pfs->sdr_total++;
	pthread_mutex_unlock( &pfs->sdr_lock);
}

void pfcp_stack__perf_inc_sda()
{
	pthread_mutex_lock( &pfs->sda_lock);
	pfs->sda_total++;
	pthread_mutex_unlock( &pfs->sda_lock);
}

void pfcp_stack__perf_inc_sur()
{
	pthread_mutex_lock( &pfs->sur_lock);
	pfs->sur_total++;
	pthread_mutex_unlock( &pfs->sur_lock);
}

void pfcp_stack__perf_inc_sua()
{
	pthread_mutex_lock( &pfs->sua_lock);
	pfs->sua_total++;
	pthread_mutex_unlock( &pfs->sua_lock);
}

void pfcp_stack__perf_inc_pkt()
{
	pthread_mutex_lock( &pfs->pkt_lock);
	pfs->pkt_total++;
	pthread_mutex_unlock( &pfs->pkt_lock);
}



void dpdk__log_pref_counters();

uint64_t pfcp__time_counter = 0;
uint64_t pfcp__mesg_counter = 0;


void pfcp_stack__perflog( app_logger_t * logger, int log_buffers)
{
	pfcp__time_counter++;
	
	pthread_mutex_lock( &pfs->maxtps_lock);
	pfcp__mesg_counter = 0;
	pthread_mutex_unlock( &pfs->maxtps_lock);

	
	
	uint64_t c_ser = pfs->ser_total - pfs->ser_last;
	uint64_t c_sea = pfs->sea_total - pfs->sea_last;
	uint64_t c_smr = pfs->smr_total - pfs->smr_last;
	uint64_t c_sma = pfs->sma_total - pfs->sma_last;
	uint64_t c_sdr = pfs->sdr_total - pfs->sdr_last;
	uint64_t c_sda = pfs->sda_total - pfs->sda_last;
	uint64_t c_sur = pfs->sur_total - pfs->sur_last;
	uint64_t c_sua = pfs->sua_total - pfs->sua_last;
	uint64_t c_pkt = pfs->pkt_total - pfs->pkt_last;
	

	uint64_t pfcp_request_sent 		= pfs->hb_request_sent_total 		- pfs->hb_request_sent_last;
	uint64_t pfcp_response_received = pfs->hb_response_received_total 	- pfs->hb_response_received_last;
	uint64_t pfcp_request_received 	= pfs->hb_request_received_total 	- pfs->hb_request_received_last;
	uint64_t pfcp_response_sent 	= pfs->hb_response_sent_total 		- pfs->hb_response_sent_last;
	uint64_t pfcp_cong_dropped 		= pfs->cong_dropped_total 			- pfs->cong_dropped_last;
	uint64_t pfcp_sess_delete 		= pfs->sess_delete_total 			- pfs->sess_delete_last;
	
	
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Session Establishment Request %-8lu %-8lu   Response %-8lu %-8lu", c_ser, pfs->ser_total, c_sea, pfs->sea_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Session Modification Request  %-8lu %-8lu   Response %-8lu %-8lu", c_smr, pfs->smr_total, c_sma, pfs->sma_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Session Delete Request        %-8lu %-8lu   Response %-8lu %-8lu", c_sdr, pfs->sdr_total, c_sdr, pfs->sdr_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Session Report Request        %-8lu %-8lu   Response %-8lu %-8lu", c_sur, pfs->sur_total, c_sua, pfs->sua_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Heartbeat Request Sent        %-8lu %-8lu   RespRecv %-8lu %-8lu", pfcp_request_sent, 	pfs->hb_request_sent_total, 	pfcp_response_received, 	pfs->hb_response_received_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Heartbeat Request Received    %-8lu %-8lu   RespSent %-8lu %-8lu", pfcp_request_received, pfs->hb_request_received_total, pfcp_response_sent, 		pfs->hb_response_sent_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP-Congession               %-8lu %-8lu", pfcp_cong_dropped, pfs->cong_dropped_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP-Session-Self-Delete      %-8lu %-8lu", pfcp_sess_delete, pfs->sess_delete_total);
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Packets                       %-8lu %-8lu", c_pkt, pfs->pkt_total);

	
	pfs->ser_last = pfs->ser_total;
	pfs->sea_last = pfs->sea_total;
	pfs->smr_last = pfs->smr_total;
	pfs->sma_last = pfs->sma_total;
	pfs->sdr_last = pfs->sdr_total;
	pfs->sda_last = pfs->sda_total;
	pfs->sur_last = pfs->sur_total;
	pfs->sua_last = pfs->sua_total;	
	pfs->pkt_last = pfs->pkt_total;
	
	pfs->hb_request_sent_last 		= pfs->hb_request_sent_total;
	pfs->hb_response_received_last 	= pfs->hb_response_received_total;
	pfs->hb_request_received_last 	= pfs->hb_request_received_total;
	pfs->hb_response_sent_last 		= pfs->hb_response_sent_total;
	pfs->cong_dropped_last			= pfs->cong_dropped_total;
	pfs->sess_delete_last			= pfs->sess_delete_total;
	
	
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	
	uint64_t pfcpTotalCapacity 	= app_rbnode__get_total( pfs->pfcp_sessions_tree);
	uint64_t pfcpTotalAvailable  = app_rbnode__get_available( pfs->pfcp_sessions_tree);

	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP-SESSIONS                 CAPACITY %-8lu   ON-GOING %-8lu   AVAILABLE %-8lu]", 
		pfcpTotalCapacity, pfcpTotalCapacity - pfcpTotalAvailable, pfcpTotalAvailable);
	
	
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	
	struct pfcp_node * node = pfs->pfcpNodeHead;
	
	if( node)
	{
		app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "Control Planes:");
		
		char NodeIPv4[40];
		char lastMsg[40];
		struct tm * tm_info;
		
		while(node)
		{
			memset( NodeIPv4, 0, sizeof(NodeIPv4));
			memset( lastMsg, 0, sizeof(lastMsg));
			
			tm_info = localtime( &node->lastmsg.tv_sec);
			strftime( lastMsg, sizeof(lastMsg), "%d-%m-%Y %H:%M:%S", tm_info);
			
			// sprintf( lastMsg, "%02d-%02d-%d %02d:%02d:%02d", 
				// node->lastmsg.tm_mday, (node->lastmsg.tm_mon) + 1, (node->lastmsg.tm_year) + 1900, node->lastmsg.tm_hour, node->lastmsg.tm_min, node->lastmsg.tm_sec);
			

			app_ep__get_str_ipv4( node->IPv4, NodeIPv4);
			

			app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "%-3d  %-3d  %-20s  %-4d  Active %d   PendingHeartbeatResponse %d  LastSeen %s", 
				node->id, node->NodeIDAddressType, NodeIPv4, ntohs(node->port), node->isActive, node->pendingHeartbeatResponse, lastMsg);

			node = node->Next;
		}
	
		app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	}
	
	
	//TODO:
	//app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Session Count                       %8d]", dpdk_session__ipv4_count());
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "");
	
	#if DPDK_PLOG
		dpdk__log_pref_counters();
	#endif	
}

app_logger_t * pfcp_stack__get_pfcplogger()
{
	return pfs->pfcpLogger;
}

app_data_region_t * pfcp_stack__get_pfcpmregion()
{
	return pfs->pfcp_mregion;
}

void pfcp_stack__add_global_far( pfcp_far_t * sess_far)
{
	pthread_mutex_lock( &pfs->far.Lock);
	
	if(!pfs->far.head)
	{
		pfs->far.head = pfs->far.current = sess_far;
	}
	else
	{
		pfs->far.current->Next = sess_far;
		pfs->far.current = sess_far;
	}
	pfs->far.count++;
	
	pthread_mutex_unlock( &pfs->far.Lock);		
}

pfcp_far_t * pfcp_stack__get_global_far_root()
{
	return pfs->far.head;
}


char * nInterfaces[] = {"Access", "Core", "SGi-LAN/N6-LAN", "CP-function", "5G VN Internal","None"};

char * pfcp_stack__get_message_type( uint8_t MessageType)
{
	switch( MessageType)
	{
		case PFCP_HEARTBEAT_REQUEST:
			return "PFCP_HEARTBEAT_REQUEST";
		case PFCP_HEARTBEAT_RESPONSE:
			return "PFCP_HEARTBEAT_RESPONSE";
		case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
			return "PFCP_PFCP_PFD_MANAGEMENT_REQUEST";
		case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
			return "PFCP_PFCP_PFD_MANAGEMENT_RESPONSE";
		case PFCP_ASSOCIATION_SETUP_REQUEST:
			return "PFCP_ASSOCIATION_SETUP_REQUEST";
		case PFCP_ASSOCIATION_SETUP_RESPONSE:
			return "PFCP_ASSOCIATION_SETUP_RESPONSE";
		case PFCP_ASSOCIATION_UPDATE_REQUEST:
			return "PFCP_ASSOCIATION_UPDATE_REQUEST";
		case PFCP_ASSOCIATION_UPDATE_RESPONSE:
			return "PFCP_ASSOCIATION_UPDATE_RESPONSE";
		case PFCP_ASSOCIATION_RELEASE_REQUEST:
			return "PFCP_ASSOCIATION_RELEASE_REQUEST";
		case PFCP_ASSOCIATION_RELEASE_RESPONSE:
			return "PFCP_ASSOCIATION_RELEASE_RESPONSE";
		case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
			return "PFCP_VERSION_NOT_SUPPORTED_RESPONSE";
		case PFCP_NODE_REPORT_REQUEST:
			return "PFCP_NODE_REPORT_REQUEST";
		case PFCP_NODE_REPORT_RESPONSE:
			return "PFCP_NODE_REPORT_RESPONSE";
		case PFCP_SESSION_SET_DELETION_REQUEST:
			return "PFCP_SESSION_SET_DELETION_REQUEST";
		case PFCP_SESSION_SET_DELETION_RESPONSE:
			return "PFCP_SESSION_SET_DELETION_RESPONSE";
		case PFCP_SESSION_ESTABLISHMENT_REQUEST:
			return "PFCP_SESSION_ESTABLISHMENT_REQUEST";
		case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
			return "PFCP_SESSION_ESTABLISHMENT_RESPONSE";
		case PFCP_SESSION_MODIFICATION_REQUEST:
			return "PFCP_SESSION_MODIFICATION_REQUEST";
		case PFCP_SESSION_MODIFICATION_RESPONSE:
			return "PFCP_SESSION_MODIFICATION_RESPONSE";
		case PFCP_SESSION_DELETION_REQUEST:
			return "PFCP_SESSION_DELETION_REQUEST";
		case PFCP_SESSION_DELETION_RESPONSE:
			return "PFCP_SESSION_DELETION_RESPONSE";
		case PFCP_SESSION_REPORT_REQUEST:
			return "PFCP_SESSION_REPORT_REQUEST";
		case PFCP_SESSION_REPORT_RESPONSE:
			return "PFCP_SESSION_REPORT_RESPONSE";
		default:
			return "-1";
	}
	return "-1";
}

// --------------------------------------------------------------------------------------------------


// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Before Decode  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);
// app_region__print_stats( pfs->pfcp_mregion);
// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "----------------  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);

// __up_pfcp_decode__session_establishment_request( &pfcp_request, msg);

// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "After Decode  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);
// app_region__print_stats( pfs->pfcp_mregion);
// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "----------------  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);

// __up_pfcp_free__session_establishment_request( &pfcp_request);

// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "After Clear  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);
// app_region__print_stats( pfs->pfcp_mregion);		
// app_logger__log( pfs->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "----------------  %s|%s|%d",__FILE__, __FUNCTION__, __LINE__);

/*
pfcp_message_t pfcp_msg;
pfcp_stack___init_header( &pfcp_msg, PFCP_SESSION_ESTABLISHMENT_RESPONSE, buffer);

pfcp_session_establishment_response_t * imsg = &pfcp_msg.u.session_establishment_response;

pfcp_session_t * pfcpSession = NULL;
pfcpSession = (pfcp_session_t *)app_region__allocate_fd( pfs->pfcp_session_pool);
pfcpSession->seid = app_rbnode__get_next_idp( pfs->pfcp_sessions_tree, (uint8_t *)pfcpSession);
//pfcp_stack___set_header_seid( &pfcp_msg, 1, pfcpSession->seid);
//pfcpSession->cp_f_seid = pfcp_stack___get_seid( buffer);

pfcp_stack___set_nodeid( &imsg->node_id);
__up_pfcp_set__u8( &imsg->cause, 1);
// pfcp_stack___set_fseid( &imsg->up_f_seid, pfcpSession->seid);

// imsg->created_pdr[0].presence = 1;
// __up_pfcp_set__u16( &imsg->created_pdr[0].pdr_id, 1);

// imsg->created_pdr[1].presence = 1;
// __up_pfcp_set__u16( &imsg->created_pdr[1].pdr_id, 2);		

printf( "cp_seid=%ld up_seid=%ld\n", pfcpSession->cp_f_seid, pfcpSession->seid);
pfcp_stack___send_response( &pfcp_msg, msg);
*/


int pfcp_stack___create_pdr( int index, pfcp_tlv_create_pdr_t * create_pdr, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * se_response, pfcp_session_modification_response_t * sm_response)
{
	if( create_pdr->pdr_id.presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		app_data_region_t * pfcp_mregion = pfcp_stack__get_pfcpmregion();

/*		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionEstablishmentRequest: seid=[%lu|%lu] processing create_pdr pdr-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);
*/
		
		pfcp_pdr_t * sess_pdr = pfcpSession->pdr.head;
		int bFound = 0;

		while( sess_pdr)
		{
			if( sess_pdr->pdr_rule_id == create_pdr->pdr_id.u16)
			{
				bFound = 1;
				break;
			}
			sess_pdr = sess_pdr->Next;
		}
		
		if( bFound == 0)
		{
			sess_pdr = (pfcp_pdr_t *) app_region__allocate_fr( pfcp_mregion, sizeof(pfcp_pdr_t) + 1);
			
			if( sess_pdr)
			{
				memset( sess_pdr, 0, sizeof(pfcp_pdr_t) + 1);

				sess_pdr->session 		= pfcpSession;
				sess_pdr->pdr_rule_id 	= create_pdr->pdr_id.u16;
				
				if( create_pdr->precedence.presence == 1) 
				{
					sess_pdr->precedence = create_pdr->precedence.u32;
				}
				
				if( create_pdr->pdi.presence == 1) 
				{
					if( create_pdr->pdi.source_interface.presence == 1) 
					{
						sess_pdr->source_interface = create_pdr->pdi.source_interface.u8;
					}
					
					if( sess_pdr->source_interface > 5)
					{
						sess_pdr->source_interface = 5;
					}
					
				}


				
				sess_pdr->network_instance_len = 0;
				if( create_pdr->pdi.network_instance.presence == 1) 
				{
					sess_pdr->network_instance_len = create_pdr->pdi.network_instance.len;
					memcpy( sess_pdr->network_instance, create_pdr->pdi.network_instance.data, create_pdr->pdi.network_instance.len);
				}

				if( create_pdr->pdi.ue_ip_address.presence == 1)
				{
					if( ( create_pdr->pdi.ue_ip_address.data[0] & 0x01) && ( create_pdr->pdi.ue_ip_address.data[0] & 0x02))
					{
						//v4v6
						sess_pdr->ipv4 = app_ep__get_u32( &create_pdr->pdi.ue_ip_address.data[1]);
						memcpy( sess_pdr->ipv6, &create_pdr->pdi.ue_ip_address.data[5], 16);

						sess_pdr->ipv4_isset = 1;
						sess_pdr->ipv6_isset = 1;
					}
					else if( ( create_pdr->pdi.ue_ip_address.data[0] & 0x02))
					{
						//v4
						sess_pdr->ipv4 = app_ep__get_u32( &create_pdr->pdi.ue_ip_address.data[1]);
						sess_pdr->ipv4_isset = 1;
					}
					else if( ( create_pdr->pdi.ue_ip_address.data[0] & 0x01))
					{
						//v6
						memcpy( sess_pdr->ipv6, &create_pdr->pdi.ue_ip_address.data[1], 16);						
						sess_pdr->ipv6_isset = 1;
					}
					
					// printf( "ue-ipv4=%u ip=%u\n", sess_pdr->ipv4_isset, sess_pdr->ipv4);
					// printf( "ue-ipv6=%u \n", sess_pdr->ipv6_isset);
					// pfcp_stack__print_buffer_wn( "ipv6", sess_pdr->ipv6, 16);
				}

				if( create_pdr->pdi.local_f_teid.presence == 1)
				{
					sess_pdr->teid 			= app_ep__get_u32( &create_pdr->pdi.local_f_teid.data[1]);
					sess_pdr->teid_isset	= 1;
					
					//printf( "teid isset=%u number=%u\n", sess_pdr->teid_isset, sess_pdr->teid);
					
					if( ( create_pdr->pdi.local_f_teid.data[0] & 0x01) && ( create_pdr->pdi.local_f_teid.data[0] & 0x02))
					{
						//v4v6
						sess_pdr->ipv4 = app_ep__get_u32( &create_pdr->pdi.local_f_teid.data[5]);
						memcpy( sess_pdr->ipv6, &create_pdr->pdi.local_f_teid.data[9], 16);

						sess_pdr->ipv4_isset = 1;
						sess_pdr->ipv6_isset = 1;
					}
					else if( ( create_pdr->pdi.local_f_teid.data[0] & 0x01))
					{
						//v4
						sess_pdr->ipv4 = app_ep__get_u32( &create_pdr->pdi.local_f_teid.data[5]);
						sess_pdr->ipv4_isset = 1;
					}
					else if( ( create_pdr->pdi.local_f_teid.data[0] & 0x02))
					{
						//v6
						memcpy( sess_pdr->ipv6, &create_pdr->pdi.local_f_teid.data[5], 16);
						sess_pdr->ipv6_isset = 1;
					}
					
					// printf( "ran-ipv4=%u ip=%u\n", sess_pdr->ipv4_isset, sess_pdr->ipv4);
					// printf( "ran-ipv6=%u \n", sess_pdr->ipv6_isset);
					// pfcp_stack__print_buffer_wn( "ran-ipv6", sess_pdr->ipv6, 16);
				}

				if( create_pdr->pdi.qfi.presence == 1)
				{
					sess_pdr->qfi = create_pdr->pdi.qfi.u8;
					sess_pdr->qfi_isset = 1;
					//printf( "qfi=%u isset=%u \n", sess_pdr->qfi, sess_pdr->qfi_isset);
				}
				
				if( create_pdr->outer_header_removal.presence == 1)
				{
					sess_pdr->outer_header_removal = create_pdr->outer_header_removal.data[0];
					sess_pdr->outer_header_removal_isset = 1;
					//printf( "outer_header_removal=%u isset=%u \n", sess_pdr->outer_header_removal, sess_pdr->outer_header_removal_isset);
				}
				
				if( create_pdr->far_id.presence == 1)
				{
					sess_pdr->far_id = create_pdr->far_id.u32;
					//printf( "far_id=%u \n", sess_pdr->far_id);
				}

				if( create_pdr->qer_id.presence == 1)
				{
					sess_pdr->qer_id = create_pdr->qer_id.u32;
					//printf( "qer_id=%u \n", sess_pdr->qer_id);
				}

				// printf( "ran-ipv4=%u ip=%u\n", sess_pdr->ipv4_isset, sess_pdr->ipv4);
				// printf( "ran-ipv6=%u \n", sess_pdr->ipv6_isset);
				// pfcp_stack__print_buffer_wn( "ran-ipv6", sess_pdr->ipv6, 16);
				// printf( "qfi=%u isset=%u \n", sess_pdr->qfi, sess_pdr->qfi_isset);
				// printf( "outer_header_removal=%u isset=%u \n", sess_pdr->outer_header_removal, sess_pdr->outer_header_removal_isset);
				// printf( "far_id=%u \n", sess_pdr->far_id);
				// printf( "qer_id=%u \n", sess_pdr->qer_id);
				
				/*
				app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
					"SER: seid=[%lu|%lu]  pdr_rule_id=%u precedence=[%u|%u] pdi.presence=%u source_interface=[%u|%u|%s] network_instance=[%u|%u]  %s|%s|%d", 
						pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_pdr->pdr_rule_id, 
						create_pdr->precedence.presence, sess_pdr->precedence, 
						create_pdr->pdi.presence, 
							create_pdr->pdi.source_interface.presence, sess_pdr->source_interface, nInterfaces[sess_pdr->source_interface],
						create_pdr->pdi.network_instance.presence, 	sess_pdr->network_instance_len,
						__FILE__, __FUNCTION__, __LINE__);

				app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_CRITICAL,
					"SER: seid=[%lu|%lu]  pdr_rule_id=%u ip-v4=[%u|%08X] ip-v6=[%u|%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X] teid=[isset=%u|%u|%08X] qfi=[%u|%u] ohr=[%u|%u] far-id=%u qer-id=%u   %s|%s|%d", 
						pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
						sess_pdr->pdr_rule_id, sess_pdr->ipv4_isset, sess_pdr->ipv4, sess_pdr->ipv6_isset, 
						sess_pdr->ipv6[0] & 0xFF, sess_pdr->ipv6[1] & 0xFF, sess_pdr->ipv6[2] & 0xFF, sess_pdr->ipv6[3] & 0xFF,
						sess_pdr->ipv6[4] & 0xFF, sess_pdr->ipv6[5] & 0xFF, sess_pdr->ipv6[6] & 0xFF, sess_pdr->ipv6[7] & 0xFF,
						sess_pdr->ipv6[8] & 0xFF, sess_pdr->ipv6[9] & 0xFF, sess_pdr->ipv6[10] & 0xFF, sess_pdr->ipv6[11] & 0xFF,
						sess_pdr->ipv6[12] & 0xFF, sess_pdr->ipv6[13] & 0xFF, sess_pdr->ipv6[14] & 0xFF, sess_pdr->ipv6[15] & 0xFF,
						sess_pdr->teid_isset, sess_pdr->teid, sess_pdr->teid, sess_pdr->qfi, sess_pdr->qfi_isset, 
						sess_pdr->outer_header_removal, sess_pdr->outer_header_removal_isset, 
						sess_pdr->far_id, sess_pdr->qer_id, 
					__FILE__, __FUNCTION__, __LINE__);				
				*/

				pthread_mutex_lock( &pfcpSession->pdr.Lock);
				
				if(!pfcpSession->pdr.head)
				{
					pfcpSession->pdr.head = pfcpSession->pdr.current = sess_pdr;
				}
				else
				{
					pfcpSession->pdr.current->Next = sess_pdr;
					pfcpSession->pdr.current = sess_pdr;
				}
				pfcpSession->pdr.count++;
				
				pthread_mutex_unlock( &pfcpSession->pdr.Lock);
			
				if( se_response)
				{
					se_response->created_pdr[index].presence = 1;
					__up_pfcp_set__u16( &se_response->created_pdr[index].pdr_id, create_pdr->pdr_id.u16);
				}
				else if( sm_response)
				{
					sm_response->created_pdr[index].presence = 1;
					__up_pfcp_set__u16( &sm_response->created_pdr[index].pdr_id, create_pdr->pdr_id.u16);					
				}
				
				return 1;
			}
			else 
			{
				app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"seid=[%lu|%lu] create_pdr -- memory allocation failed for pdr-id=%u index=%d  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);				
				
				return -1;
			}
		}
		else
		{
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] create_pdr -- pdr already exists with pdr-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);
			
			return 2;
		}
		
	}
	
	return 0;
}

int pfcp_stack___update_pdr( int index, pfcp_tlv_update_pdr_t * update_pdr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response)
{
	if( update_pdr->pdr_id.presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionModificationRequest: seid=[%lu|%lu]  processing update_pdr pdr_id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid,  update_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);
		
		pfcp_pdr_t * sess_pdr = NULL;
		int bFound = 0;
		
		sess_pdr = pfcpSession->pdr.head;
			
		while( sess_pdr)
		{
			if( sess_pdr->pdr_rule_id == update_pdr->pdr_id.u16)
			{
				bFound = 1;
				break;
			}
			sess_pdr = sess_pdr->Next;
		}
		
		if( sess_pdr)
		{
			sess_pdr->isremoved 	= 0;
			sess_pdr->session 		= pfcpSession;
			//sess_pdr->pdr_rule_id 	= create_pdr->pdr_id.u16;
			
			//printf( "pdr_rule_id=%u\n", update_pdr->pdr_id.u16);
			
			if( update_pdr->precedence.presence == 1) 
			{
				sess_pdr->precedence = update_pdr->precedence.u32;
				//printf( "precedence=%d\n", update_pdr->precedence.u32);
			}
			
			if( update_pdr->pdi.presence == 1) 
			{
				if( update_pdr->pdi.source_interface.presence == 1) 
				{
					sess_pdr->source_interface = update_pdr->pdi.source_interface.u8;
					//printf( "source_interface=%u\n", sess_pdr->source_interface);
					
					if( sess_pdr->source_interface > 5)
					{
						sess_pdr->source_interface = 5;
					}
				}
			}
			
			sess_pdr->network_instance_len = 0;
			if( update_pdr->pdi.network_instance.presence == 1) 
			{
				sess_pdr->network_instance_len = update_pdr->pdi.network_instance.len;
				memcpy( sess_pdr->network_instance, update_pdr->pdi.network_instance.data, update_pdr->pdi.network_instance.len);
				//printf( "network_instance=%s\n", &sess_pdr->network_instance[1]);
			}
			
			if( update_pdr->pdi.ue_ip_address.presence == 1)
			{
				if( ( update_pdr->pdi.ue_ip_address.data[0] & 0x01) && ( update_pdr->pdi.ue_ip_address.data[0] & 0x02))
				{
					//v4v6
					sess_pdr->ipv4 = app_ep__get_u32( &update_pdr->pdi.ue_ip_address.data[1]);
					memcpy( sess_pdr->ipv6, &update_pdr->pdi.ue_ip_address.data[5], 16);

					sess_pdr->ipv4_isset = 1;
					sess_pdr->ipv6_isset = 1;
				}
				else if( ( update_pdr->pdi.ue_ip_address.data[0] & 0x02))
				{
					//v4
					sess_pdr->ipv4 = app_ep__get_u32( &update_pdr->pdi.ue_ip_address.data[1]);
					sess_pdr->ipv4_isset = 1;
				}
				else if( ( update_pdr->pdi.ue_ip_address.data[0] & 0x01))
				{
					//v6
					memcpy( sess_pdr->ipv6, &update_pdr->pdi.ue_ip_address.data[1], 16);						
					sess_pdr->ipv6_isset = 1;
				}
				
				//printf( "ue-ipv4=%u ip=%u\n", sess_pdr->ipv4_isset, sess_pdr->ipv4);
				//printf( "ue-ipv6=%u \n", sess_pdr->ipv6_isset);
				//pfcp_stack__print_buffer_wn( "ipv6", sess_pdr->ipv6, 16);
			}
			
			if( update_pdr->pdi.local_f_teid.presence == 1)
			{
				sess_pdr->teid 			= app_ep__get_u32( &update_pdr->pdi.local_f_teid.data[1]);
				sess_pdr->teid_isset	= 1;
				
				//printf( "teid isset=%u number=%u\n", sess_pdr->teid_isset, sess_pdr->teid);
				
				if( ( update_pdr->pdi.local_f_teid.data[0] & 0x01) && ( update_pdr->pdi.local_f_teid.data[0] & 0x02))
				{
					//v4v6
					sess_pdr->ipv4 = app_ep__get_u32( &update_pdr->pdi.local_f_teid.data[5]);
					memcpy( sess_pdr->ipv6, &update_pdr->pdi.local_f_teid.data[9], 16);

					sess_pdr->ipv4_isset = 1;
					sess_pdr->ipv6_isset = 1;
				}
				else if( ( update_pdr->pdi.local_f_teid.data[0] & 0x01))
				{
					//v4
					sess_pdr->ipv4 = app_ep__get_u32( &update_pdr->pdi.local_f_teid.data[5]);
					sess_pdr->ipv4_isset = 1;
				}
				else if( ( update_pdr->pdi.local_f_teid.data[0] & 0x02))
				{
					//v6
					memcpy( sess_pdr->ipv6, &update_pdr->pdi.local_f_teid.data[5], 16);
					sess_pdr->ipv6_isset = 1;
				}
				
			//	printf( "ran-ipv4=%u ip=%u\n", sess_pdr->ipv4_isset, sess_pdr->ipv4);
				//printf( "ran-ipv6=%u \n", sess_pdr->ipv6_isset);
				//pfcp_stack__print_buffer_wn( "ran-ipv6", sess_pdr->ipv6, 16);
			}
			
			if( update_pdr->pdi.qfi.presence == 1)
			{
				sess_pdr->qfi = update_pdr->pdi.qfi.u8;
				sess_pdr->qfi_isset = 1;
				//printf( "qfi=%u isset=%u \n", sess_pdr->qfi, sess_pdr->qfi_isset);
			}
			
			if( update_pdr->outer_header_removal.presence == 1)
			{
				sess_pdr->outer_header_removal = update_pdr->outer_header_removal.data[0];
				sess_pdr->outer_header_removal_isset = 1;
				//printf( "outer_header_removal=%u isset=%u \n", sess_pdr->outer_header_removal, sess_pdr->outer_header_removal_isset);
			}
			
			if( update_pdr->far_id.presence == 1)
			{
				sess_pdr->far_id = update_pdr->far_id.u32;
				//printf( "far_id=%u \n", sess_pdr->far_id);
			}

			if( update_pdr->qer_id.presence == 1)
			{
				sess_pdr->qer_id = update_pdr->qer_id.u32;
				//printf( "qer_id=%u \n", sess_pdr->qer_id);
			}
			

			app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
				"SMR: seid=[%lu|%lu]  pdr_rule_id=%u precedence=[%u|%u] pdi.presence=%u source_interface=[%u|%u|%s] network_instance=[%u|%u]  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_pdr->pdr_rule_id, 
					update_pdr->precedence.presence, sess_pdr->precedence, 
					update_pdr->pdi.presence, 
						update_pdr->pdi.source_interface.presence, sess_pdr->source_interface, nInterfaces[sess_pdr->source_interface],
					update_pdr->pdi.network_instance.presence, 	sess_pdr->network_instance_len,
					__FILE__, __FUNCTION__, __LINE__);

			app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG,
				"SMR: seid=[%lu|%lu]  pdr_rule_id=%u ip-v4=[%u|%08X] ip-v6=[%u|%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X] teid=[%u|%08X] qfi=[%u|%u] ohr=[%u|%u] far-id=%u qer-id=%u   %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_pdr->pdr_rule_id, sess_pdr->ipv4_isset, sess_pdr->ipv4, sess_pdr->ipv6_isset, 
					sess_pdr->ipv6[0] & 0xFF, sess_pdr->ipv6[1] & 0xFF, sess_pdr->ipv6[2] & 0xFF, sess_pdr->ipv6[3] & 0xFF,
					sess_pdr->ipv6[4] & 0xFF, sess_pdr->ipv6[5] & 0xFF, sess_pdr->ipv6[6] & 0xFF, sess_pdr->ipv6[7] & 0xFF,
					sess_pdr->ipv6[8] & 0xFF, sess_pdr->ipv6[9] & 0xFF, sess_pdr->ipv6[10] & 0xFF, sess_pdr->ipv6[11] & 0xFF,
					sess_pdr->ipv6[12] & 0xFF, sess_pdr->ipv6[13] & 0xFF, sess_pdr->ipv6[14] & 0xFF, sess_pdr->ipv6[15] & 0xFF,
					sess_pdr->teid_isset, sess_pdr->teid, sess_pdr->qfi, sess_pdr->qfi_isset, 
					sess_pdr->outer_header_removal, sess_pdr->outer_header_removal_isset, 
					sess_pdr->far_id, sess_pdr->qer_id, 
				__FILE__, __FUNCTION__, __LINE__);		
			
		
			if( sm_response)
			{
				sm_response->created_pdr[index].presence = 1;
				__up_pfcp_set__u16( &sm_response->created_pdr[index].pdr_id, update_pdr->pdr_id.u16);					
			}
			
			return 1;
		}
		else
		{
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				" seid=[%lu|%lu] pdr-not found with pdr_id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid,  update_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);			
			
			return 2;
		}
	}
	return 0;
}

int pfcp_stack___remove_pdr( int index, pfcp_tlv_remove_pdr_t * remove_pdr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response)
{
	if( remove_pdr->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionModificationRequest: seid=[%lu|%lu]  processing remove_pdr pdr-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, remove_pdr->pdr_id.u16, index, __FILE__, __FUNCTION__, __LINE__);

		pfcp_pdr_t * sess_pdr = pfcpSession->pdr.head;
		
		while( sess_pdr)
		{
			if( sess_pdr->pdr_rule_id == remove_pdr->pdr_id.u16)
			{
				app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG,
					"SMR: seid=[%lu|%lu]  pdr_id.u16=%u marked as removed  %s|%s|%d", pfcpSession->up_f_seid, pfcpSession->cp_f_seid, remove_pdr->pdr_id.u16, __FILE__, __FUNCTION__, __LINE__);
				
				sess_pdr->isremoved = 1;
				break;
			}
			sess_pdr = sess_pdr->Next;
		}
		
		return 1;
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------

int pfcp_stack___create_far( int index, pfcp_tlv_create_far_t * create_far, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response)
{
	if( create_far->far_id.presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		app_data_region_t * pfcp_mregion = pfcp_stack__get_pfcpmregion();

/*
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"seid=[%lu|%lu] processing create_far far-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);
*/
		
		pfcp_far_t * sess_far = NULL;
		int bFound = 0;

		uint32_t far_id 	=  create_far->far_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((create_far->far_id.u32 & 0x80000000) > 0) ? 1 : 0 ;
		
		
		if( isGlobal == 0)
		{
			sess_far = pfcpSession->far.head;
			
			while( sess_far)
			{
				if( sess_far->far_id == far_id)
				{
					bFound = 1;
					break;
				}
				sess_far = sess_far->Next;
			}
		}
		else if( isGlobal == 1)
		{
			sess_far = pfs->far.head;

			while( sess_far)
			{
				if( sess_far->far_id == far_id)
				{
					bFound = 1;
					break;
				}
				sess_far = sess_far->Next;
			}			
		}
		
		if( bFound == 0)
		{
			sess_far = (pfcp_far_t *) app_region__allocate_fr( pfcp_mregion, sizeof(pfcp_far_t) + 1);
			
			if( sess_far)
			{
				memset( sess_far, 0, sizeof(sizeof(pfcp_far_t) + 1));
				sess_far->far_id = far_id;
				
				
				if( create_far->apply_action.presence == 1)
				{
					sess_far->apply_action = create_far->apply_action.u8;
					sess_far->apply_action_isset = 1;
				}
				
				//printf( "forwarding_parameters.presence=%lu  \n", create_far->forwarding_parameters.presence);
				
				if( create_far->forwarding_parameters.presence == 1)
				{
					if( create_far->forwarding_parameters.destination_interface.presence == 1)
					{
						sess_far->destination_interface = create_far->forwarding_parameters.destination_interface.u8;
						sess_far->destination_interface_isset = 1;
						//printf( "destination_interface=%u isset=%u \n", sess_far->destination_interface, sess_far->destination_interface_isset);
					}
					
					if( create_far->forwarding_parameters.outer_header_creation.presence == 1)
					{
						sess_far->outer_header_creation = app_ep__get_u16( &create_far->forwarding_parameters.outer_header_creation.data[0]);
						sess_far->teid = app_ep__get_u32( &create_far->forwarding_parameters.outer_header_creation.data[2]);						
						
						sess_far->outer_header_creation_isset = ( sess_far->outer_header_creation > 0) ? 1 : 0;
						
						
						switch( sess_far->outer_header_creation)
						{
							case 256:
								{
									// GTP-U/UDP/IPv4
									sess_far->ipv4 = app_ep__get_u32( &create_far->forwarding_parameters.outer_header_creation.data[6]);
									sess_far->ipv4_isset = 1;
								}
								break;
							case 512:
								{
									// GTP-U/UDP/IPv6
									memcpy( sess_far->ipv6, &create_far->forwarding_parameters.outer_header_creation.data[6], 16);
									sess_far->ipv6_isset = 1;
								}
								break;
							case 1024:
								{
									// UDP/IPv4
								}
								break;
							case 2048:
								{
									// UDP/IPv6
								}
								break;
							default:
								break;
						}					
					}
					
				}

/*
				app_logger__log( pfcplogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
					"seid=[%lu|%lu] create_far far-id=%u index=%d apply_action=[%u|%u] fparam=[%u|%u|%u|%s] fparam-ohc[teid=%u|ipv4=%u]  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_far->far_id, index, 
					create_far->apply_action.presence, sess_far->apply_action, 
					create_far->forwarding_parameters.presence, create_far->forwarding_parameters.destination_interface.presence, 
					sess_far->destination_interface, nInterfaces[ sess_far->destination_interface],
					sess_far->teid, sess_far->ipv4,
					__FILE__, __FUNCTION__, __LINE__);				
*/				
				
				if( isGlobal == 0) 
				{
					pthread_mutex_lock( &pfcpSession->far.Lock);
					
					if(!pfcpSession->far.head)
					{
						pfcpSession->far.head = pfcpSession->far.current = sess_far;
					}
					else
					{
						pfcpSession->far.current->Next = sess_far;
						pfcpSession->far.current = sess_far;
					}
					pfcpSession->far.count++;
					sess_far->session = pfcpSession;
					
					pthread_mutex_unlock( &pfcpSession->far.Lock);				
				}
				else if( isGlobal == 1) 
				{
					pthread_mutex_lock( &pfs->far.Lock);
					
					if(!pfs->far.head)
					{
						pfs->far.head = pfs->far.current = sess_far;
					}
					else
					{
						pfs->far.current->Next = sess_far;
						pfs->far.current = sess_far;
					}
					pfs->far.count++;
					
					pthread_mutex_unlock( &pfs->far.Lock);
					
					int igc = 0;
					for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
					{
						if(!pfcpSession->global_far[igc])
						{
							pfcpSession->global_far[igc] = sess_far;
							break;
						}
					}
				}
				
				
			}
			else
			{
				app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"seid=[%lu|%lu] create_far -- memory allocation failed for far-id=%u index=%d  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
			}
			return 1;
		} 
		else 
		{
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] create_far -- far already exists with  far-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
			
			return -1;
		}
	}
	
	return 0;
}


void dpe_pkt__send_gtp_end_marker_ipv4( uint32_t ranip, uint32_t ran_teid);

int pfcp_stack___update_far( int index, pfcp_tlv_update_far_t * update_far, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( update_far->far_id.presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		//app_data_region_t * pfcp_mregion = pfcp_stack__get_pfcpmregion();

		/*
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionModificationRequest: seid=[%lu|%lu]  processing update_far far-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
			update_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);
		*/

		pfcp_far_t * sess_far = NULL;
		int bFound = 0;
		
		uint32_t far_id 	=  update_far->far_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((update_far->far_id.u32 & 0x80000000) > 0) ? 1 : 0 ;		
		
		
		if( isGlobal == 0)
		{
			sess_far = pfcpSession->far.head;
			
			while( sess_far)
			{
				if( sess_far->far_id == far_id)
				{
					bFound = 1;
					break;
				}
				sess_far = sess_far->Next;
			}
		}
		else if( isGlobal == 1)
		{
			sess_far = pfs->far.head;
			
			while( sess_far)
			{
				if( sess_far->far_id == far_id)
				{
					bFound = 1;
					break;
				}
				sess_far = sess_far->Next;
			}
		}
		
		if( bFound == 1 && sess_far)
		{
			sess_far->far_id = far_id;

			if( update_far->apply_action.presence == 1)
			{
				sess_far->apply_action = update_far->apply_action.u8;
				sess_far->apply_action_isset = 1;
			}

			//printf( "update far -- forwarding_parameters.presence=%lu  \n", update_far->update_forwarding_parameters.presence);

/*
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, "modification up_f_seid=%u cp_f_seid=%u processing for far_id=%u apply_action=%u destination_interface=%u %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_far->far_id, 
				sess_far->apply_action, sess_far->destination_interface,
				__FILE__, __FUNCTION__, __LINE__);
*/
			
			if( update_far->update_forwarding_parameters.presence == 1)
			{
				uint8_t hasAccessIPv4 = 0;
				uint8_t hasAccessIPv6 = 0;
				uint32_t oldRANIP = 0;
				uint32_t oldRANTEID = 0;
				
				
				if( sess_far->ipv4_isset == 1) 
				{
					hasAccessIPv4 = 1;
				}
				
				if( sess_far->ipv6_isset == 1) 
				{
					hasAccessIPv6 = 1;
				}				
				
				if( update_far->update_forwarding_parameters.destination_interface.presence == 1)
				{
					sess_far->destination_interface = update_far->update_forwarding_parameters.destination_interface.u8;
					sess_far->destination_interface_isset = 1;
					//printf( "update far -- destination_interface=%u isset=%u \n", sess_far->destination_interface, sess_far->destination_interface_isset);
				}
				
				if( update_far->update_forwarding_parameters.outer_header_creation.presence == 1)
				{
					oldRANTEID = sess_far->teid;

					sess_far->outer_header_creation = app_ep__get_u16( &update_far->update_forwarding_parameters.outer_header_creation.data[0]);
					sess_far->teid = app_ep__get_u32( &update_far->update_forwarding_parameters.outer_header_creation.data[2]);
					
					switch( sess_far->outer_header_creation)
					{
						case 256:
							{
								// GTP-U/UDP/IPv4
								oldRANIP = sess_far->ipv4;

								sess_far->ipv4 = app_ep__get_u32( &update_far->update_forwarding_parameters.outer_header_creation.data[6]);
								sess_far->ipv4_isset = 1;
							}
							break;
						case 512:
							{
								// GTP-U/UDP/IPv6
								memcpy( sess_far->ipv6, &update_far->update_forwarding_parameters.outer_header_creation.data[6], 16);
								sess_far->ipv6_isset = 1;
							}
							break;
						case 1024:
							{
								// UDP/IPv4
							}
							break;
						case 2048:
							{
								// UDP/IPv6
							}
							break;
						default:
							break;
					}
					
					if( sess_far->destination_interface == 0)
					{
						if( sess_far->destination_interface_isset == 1)
						{
							if( sess_far->ipv4_isset == 1 && hasAccessIPv4 == 1)
							{
								//send EndMarker
								//oldRANTEID, oldRANIP
								dpe_pkt__send_gtp_end_marker_ipv4( oldRANIP, oldRANTEID);
							}
							else if( sess_far->ipv6_isset == 1 && hasAccessIPv6 == 1)
							{
								//send EndMarker
								//oldRANTEID, oldRANIP								
							}								
						}
					}
						
					
					// printf( "teid=%u ipv4=%u ipv4_isset=%u ipv6_isset=%u   %s|%d\n", 
						// sess_far->teid, sess_far->ipv4, sess_far->ipv4_isset, sess_far->ipv6_isset, __FILE__, __LINE__);
					// pfcp_stack__print_buffer_wn( "outer header creation", sess_far->ipv6, 16);

/*					
				app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, "modification up_f_seid=%u cp_f_seid=%u processing for far_id=%u teid=%u ipv4=%u ipv4_isset=%u ipv6_isset=%u  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_far->far_id, 
					sess_far->teid, sess_far->ipv4, sess_far->ipv4_isset, sess_far->ipv6_isset,
					__FILE__, __FUNCTION__, __LINE__);					
*/					
					sess_far->outer_header_creation_isset = 1;
				}
			}

/*			
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"seid=[%lu|%lu] update_far far-id=%u index=%d apply_action=[%u|%u] fparam=[%u|%u|%u|%s] ohc=[%u|%u|teid(%u) ipv4(%u|%08X) ipv6(%u|%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)]  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_far->far_id, index, 
				update_far->apply_action.presence, sess_far->apply_action, 
				update_far->update_forwarding_parameters.presence, update_far->update_forwarding_parameters.destination_interface.presence, sess_far->destination_interface, nInterfaces[ sess_far->destination_interface],
				update_far->update_forwarding_parameters.outer_header_creation.presence, 
					sess_far->outer_header_creation, sess_far->teid, sess_far->ipv4_isset, sess_far->ipv4, sess_far->ipv6_isset, 
					sess_far->ipv6[0] & 0xFF, sess_far->ipv6[1] & 0xFF, sess_far->ipv6[2] & 0xFF, sess_far->ipv6[3] & 0xFF, sess_far->ipv6[4] & 0xFF, sess_far->ipv6[5] & 0xFF, sess_far->ipv6[6] & 0xFF, sess_far->ipv6[7] & 0xFF,
					sess_far->ipv6[8] & 0xFF, sess_far->ipv6[9] & 0xFF, sess_far->ipv6[10] & 0xFF, sess_far->ipv6[11] & 0xFF, sess_far->ipv6[12] & 0xFF, sess_far->ipv6[13] & 0xFF, sess_far->ipv6[14] & 0xFF, sess_far->ipv6[15] & 0xFF,
				__FILE__, __FUNCTION__, __LINE__);			
*/
			
			return 1;
		} 
		else 
		{
			// app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				// "update_far seid=[%lu|%lu]  far not found with far-id=%u index=%d  %s|%s|%d", 
				// pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
				// update_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
			
			return -1;
		}
	}
	
	return 0;
}


int pfcp_stack___remove_far( int index, pfcp_tlv_remove_far_t * remove_far, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * sm_response)
{
	if( remove_far->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"remove_far  seid=[%lu|%lu] processing far-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, remove_far->far_id.u32, index, __FILE__, __FUNCTION__, __LINE__);

		uint32_t far_id 	=  remove_far->far_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((remove_far->far_id.u32 & 0x80000000) > 0) ? 1 : 0 ;	
		
		if( isGlobal == 0)
		{	
			pfcp_far_t * sess_far = pfcpSession->far.head;
			
			while( sess_far)
			{
				if( sess_far->far_id == far_id)
				{
					sess_far->isremoved = 1;
					break;
				}
				sess_far = sess_far->Next;
			}
		}
		else
		{
			int igc = 0;
			for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
			{
				if( pfcpSession->global_far[igc])
				{
					if( pfcpSession->global_far[igc]->far_id == far_id)
					{
						pfcpSession->global_far[igc] = NULL;
						break;
					}
				}
			}
		}
		
		return 1;
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------

uint16_t dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes);
uint16_t dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes);

int pfcp_stack___create_qer( int index, pfcp_tlv_create_qer_t * create_qer, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response)
{
	if( create_qer->qer_id.presence == 1)
	{

/*
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
			" seid=[%lu|%lu]  processing create_qer qer-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);
*/

		pfcp_qer_t * sess_qer = NULL;
		int bFound = 0;
		
		uint32_t qer_id 	=  create_qer->qer_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((create_qer->qer_id.u32 & 0x80000000) > 0) ? 1 : 0 ;
		
		if( isGlobal == 0)
		{
			sess_qer = pfcpSession->qer.head;
			
			while( sess_qer)
			{
				if( sess_qer->qer_id == qer_id)
				{
					bFound = 1;
					break;
				}
				sess_qer = sess_qer->Next;
			}
		}
		else if( isGlobal == 1)
		{
			sess_qer = pfs->qer.head;
			
			while( sess_qer)
			{
				if( sess_qer->qer_id == qer_id)
				{
					bFound = 1;
					break;
				}
				sess_qer = sess_qer->Next;
			}
		}
		
		
		if( bFound == 0)
		{
			sess_qer = (pfcp_qer_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_qer_t) + 1);
			
			if( sess_qer)
			{
				memset( sess_qer, 0, sizeof(sizeof(pfcp_qer_t) + 1));
				sess_qer->qer_id = qer_id;
				sess_qer->ul_tc = 0;
				sess_qer->dl_tc = 0;
					
				if( create_qer->gate_status.presence == 1)
				{
					sess_qer->gate_status = create_qer->gate_status.u8;
					sess_qer->gate_status_isset = 1;
					//printf("gate_status=%u \n", sess_qer->gate_status);
				}
				
				if( create_qer->qos_flow_identifier.presence == 1)
				{
					sess_qer->qfi = create_qer->qos_flow_identifier.u8;;
					sess_qer->qfi_isset = 1;
					//printf("qfi=%u \n", sess_qer->qfi);
				}
				
				if( create_qer->maximum_bitrate.presence == 1)
				{
					sess_qer->ul_mbr = app_ep__get_u40( &create_qer->maximum_bitrate.data[0]);
					sess_qer->dl_mbr = app_ep__get_u40( &create_qer->maximum_bitrate.data[5]);
					sess_qer->mbr_isset = 1;
					
					sess_qer->ul_tc = dpdk_qos__find_uplink_traffic_class( sess_qer->ul_mbr);
					sess_qer->dl_tc = dpdk_qos__find_downlink_traffic_class( sess_qer->dl_mbr);
					
					//printf("ul=%lu  dl=%lu  isset=%u\n", sess_qer->ul_mbr, sess_qer->dl_mbr, sess_qer->mbr_isset);
				}

/*
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
					"create_qer  seid=[%lu|%lu] processing qer-id=%u index=%d gate_status[%u|%u] qfi=[%u|%u] mbr=[%u|%lu|%lu]  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_qer->qer_id, index, 
						create_qer->gate_status.presence, sess_qer->gate_status, create_qer->qos_flow_identifier.presence, sess_qer->qfi,
						create_qer->maximum_bitrate.presence, sess_qer->ul_mbr, sess_qer->dl_mbr,
					__FILE__, __FUNCTION__, __LINE__);
*/
				
				if( isGlobal == 0) 
				{
					pthread_mutex_lock( &pfcpSession->qer.Lock);
					
					if(!pfcpSession->qer.head)
					{
						pfcpSession->qer.head = pfcpSession->qer.current = sess_qer;
					}
					else
					{
						pfcpSession->qer.current->Next = sess_qer;
						pfcpSession->qer.current = sess_qer;
					}
					pfcpSession->qer.count++;
					sess_qer->session = pfcpSession;
					
					pthread_mutex_unlock( &pfcpSession->qer.Lock);
				}
				else if( isGlobal == 1) 
				{
					pthread_mutex_lock( &pfs->qer.Lock);
					
					if(!pfs->qer.head)
					{
						pfs->qer.head = pfs->qer.current = sess_qer;
					}
					else
					{
						pfs->qer.current->Next = sess_qer;
						pfs->qer.current = sess_qer;
					}
					pfs->qer.count++;
					
					pthread_mutex_unlock( &pfs->qer.Lock);
					
					int igc = 0;
					for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
					{
						if(!pfcpSession->global_qer[igc])
						{
							pfcpSession->global_qer[igc] = sess_qer;
							break;
						}
					}					
				}
				
			}
			else
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"seid=[%lu|%lu] create_qer memory allocation failed for qer-id=%u index=%d  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
					create_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);				
			}
			
			return 1;
		}
		else 
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] create_qer already found with qer-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
				create_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
			
			return -1;
		}
	}
	return 0;
}

int pfcp_stack___update_qer( int index, pfcp_tlv_update_qer_t * update_qer, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( update_qer->presence == 1)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"processing update_qer  seid=[%lu|%lu] qer-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
			update_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);

		pfcp_qer_t * sess_qer = NULL;
		int bFound = 0;
		
		uint32_t qer_id 	=  update_qer->qer_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((update_qer->qer_id.u32 & 0x80000000) > 0) ? 1 : 0 ;		
	
		if( isGlobal == 0)
		{
			sess_qer = pfcpSession->qer.head;
			
			while( sess_qer)
			{
				if( sess_qer->qer_id == qer_id)
				{
					bFound = 1;
					break;
				}
				sess_qer = sess_qer->Next;
			}
		}
		else if( isGlobal == 1)
		{
			sess_qer = pfs->qer.head;
			
			while( sess_qer)
			{
				if( sess_qer->qer_id == qer_id)
				{
					bFound = 1;
					break;
				}
				sess_qer = sess_qer->Next;
			}
		}
		
		if( bFound == 1 && sess_qer)
		{
			if( update_qer->gate_status.presence == 1)
			{
				sess_qer->gate_status = update_qer->gate_status.u8;
				sess_qer->gate_status_isset = 1;
				//printf("gate_status=%u \n", sess_qer->gate_status);
			}
			
			if( update_qer->qos_flow_identifier.presence == 1)
			{
				sess_qer->qfi = update_qer->qos_flow_identifier.u8;;
				sess_qer->qfi_isset = 1;
				//printf("qfi=%u \n", sess_qer->qfi);
			}
			
			if( update_qer->maximum_bitrate.presence == 1)
			{
				sess_qer->ul_mbr = app_ep__get_u40( &update_qer->maximum_bitrate.data[0]);
				sess_qer->dl_mbr = app_ep__get_u40( &update_qer->maximum_bitrate.data[5]);
				sess_qer->mbr_isset = 1;
				//printf("ul=%lu  dl=%lu  isset=%u\n", sess_qer->ul_mbr, sess_qer->dl_mbr, sess_qer->mbr_isset);
			}

			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"update_qer  seid=[%lu|%lu] processing qer-id=%u index=%d gate_status[%u|%u] qfi=[%u|%u] mbr=[%u|%lu|%lu]  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, sess_qer->qer_id, index, 
					update_qer->gate_status.presence, sess_qer->gate_status, update_qer->qos_flow_identifier.presence, sess_qer->qfi,
					update_qer->maximum_bitrate.presence, sess_qer->ul_mbr, sess_qer->dl_mbr,
				__FILE__, __FUNCTION__, __LINE__);			
		}
		else
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] qer not found with qer-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
				update_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
		}
	}
	return 0;
}

int pfcp_stack___remove_qer( int index, pfcp_tlv_remove_qer_t * remove_qer, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( remove_qer->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionModificationRequest: processing remove_qer pdr-id=%u index=%d  %s|%s|%d", 
			remove_qer->qer_id.u32, index, __FILE__, __FUNCTION__, __LINE__);

		uint32_t qer_id 	=  remove_qer->qer_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((remove_qer->qer_id.u32 & 0x80000000) > 0) ? 1 : 0 ;	
		
		if( isGlobal == 0)
		{	
			pfcp_qer_t * sess_qer = pfcpSession->qer.head;
			
			while( sess_qer)
			{
				if( sess_qer->qer_id == qer_id)
				{
					sess_qer->isremoved = 1;
					break;
				}
				sess_qer = sess_qer->Next;
			}
		}
		else
		{
			int igc = 0;
			for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
			{
				if( pfcpSession->global_qer[igc])
				{
					if( pfcpSession->global_qer[igc]->qer_id == qer_id)
					{
						pfcpSession->global_qer[igc] = NULL;
						break;
					}
				}
			}
		}
		
		return 1;
	}
	return 0;
}
// --------------------------------------------------------------------------------------------------

int pfcp_stack___create_mar( int index, pfcp_tlv_create_mar_t * create_mar, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response)
{
	return 0;
}

int pfcp_stack___update_mar( int index, pfcp_tlv_update_mar_t * update_mar, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	return 0;
}

int pfcp_stack___remove_mar( int index, pfcp_tlv_remove_mar_t * remove_mar, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	return 0;
}
// --------------------------------------------------------------------------------------------------
int pfcp_stack___create_urr( int index, pfcp_tlv_create_urr_t * create_urr, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response)
{
	if( create_urr->urr_id.presence == 1)
	{

/*
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionEstablishmentRequest: seid=[%lu|%lu]  processing create_urr urr-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
			create_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);
*/

		pfcp_urr_t * sess_urr = NULL;
		int bFound = 0;
		
		uint32_t urr_id 	=  create_urr->urr_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((create_urr->urr_id.u32 & 0x80000000) > 0) ? 1 : 0 ;
		
		if( isGlobal == 0)
		{
			pthread_mutex_lock( &pfcpSession->urr.Lock);
			
			sess_urr = pfcpSession->urr.head;
			
			while( sess_urr)
			{
				if( sess_urr->urr_id == urr_id)
				{
					bFound = 1;
					break;
				}
				sess_urr = sess_urr->Next;
			}
			
			pthread_mutex_unlock( &pfcpSession->urr.Lock);
		}
		else if( isGlobal == 1)
		{
			sess_urr = pfs->urr.head;
			
			while( sess_urr)
			{
				if( sess_urr->urr_id == urr_id)
				{
					bFound = 1;
					break;
				}
				sess_urr = sess_urr->Next;
			}
		}
		
		if( bFound == 0)
		{
			sess_urr = (pfcp_urr_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_urr_t) + 1);
			
			if( sess_urr)
			{
				memset( sess_urr, 0, sizeof(sizeof(pfcp_urr_t) + 1));
				sess_urr->urr_id = urr_id;

				if( create_urr->measurement_method.presence == 1)
				{
					sess_urr->measurement_method = create_urr->measurement_method.u8;
					sess_urr->measurement_method_isset = 1;
				}

				if( create_urr->reporting_triggers.presence == 1)
				{
					sess_urr->reporting_triggers = create_urr->reporting_triggers.u8;
					sess_urr->reporting_triggers_isset = 1;
				}
				
				sess_urr->used_volume.total = 0;
				sess_urr->used_volume.uplink = 0;
				sess_urr->used_volume.downlink = 0;

				sess_urr->reported_volume.total = 0;
				sess_urr->reported_volume.uplink = 0;
				sess_urr->reported_volume.downlink = 0;
				
				sess_urr->total_reported_volume.total = 0;
				sess_urr->total_reported_volume.uplink = 0;
				sess_urr->total_reported_volume.downlink = 0;

				
				uint32_t granted_volume_total_present = 0;
				uint64_t granted_volume_uplink = 0;
				uint64_t granted_volume_downlink = 0;
				
				uint32_t volume_threshold_total_present = 0;
				uint64_t volume_threshold_uplink = 0;
				uint64_t volume_threshold_downlink = 0;
				
				if( create_urr->volume_quota.presence == 1)
				{
					uint8_t bPos = 1;
					
					if( create_urr->volume_quota.data[0] & 0x01)
					{
						sess_urr->granted_volume.total = app_ep__get_u64( &create_urr->volume_quota.data[bPos]);
						granted_volume_total_present = 1;
						bPos += 8;
					}
					
					if( create_urr->volume_quota.data[0] & 0x02)
					{
						sess_urr->granted_volume.uplink = app_ep__get_u64( &create_urr->volume_quota.data[bPos]);
						granted_volume_uplink = app_ep__get_u64( &create_urr->volume_quota.data[bPos]);
						bPos += 8;
					}

					if( create_urr->volume_quota.data[0] & 0x03)
					{
						sess_urr->granted_volume.downlink = app_ep__get_u64( &create_urr->volume_quota.data[bPos]);
						granted_volume_downlink = app_ep__get_u64( &create_urr->volume_quota.data[bPos]);
						bPos += 8;
					}
					sess_urr->granted_volume_isset = 1;	

					if( granted_volume_total_present == 0)
					{
						sess_urr->granted_volume.total = (granted_volume_uplink + granted_volume_downlink);
					}

					
					if( pfs->enable_quota_logs == 1)
					{
						app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
							"create-quota  up_f_seid=%ld  cp_f_seid=%ld  RG=%d  gran-vol=%ld", pfcpSession->up_f_seid, pfcpSession->cp_f_seid, urr_id, sess_urr->granted_volume.total);
					}
				}

				if( create_urr->volume_threshold.presence == 1)
				{
					uint8_t bPos = 1;
					
					if( create_urr->volume_threshold.data[0] & 0x01)
					{
						sess_urr->trigger_volume.total = app_ep__get_u64( &create_urr->volume_threshold.data[bPos]);
						volume_threshold_total_present = 1;
						bPos += 8;
					}
					
					if( create_urr->volume_threshold.data[0] & 0x02)
					{
						sess_urr->trigger_volume.uplink = app_ep__get_u64( &create_urr->volume_threshold.data[bPos]);
						volume_threshold_uplink = app_ep__get_u64( &create_urr->volume_threshold.data[bPos]);
						bPos += 8;
					}

					if( create_urr->volume_threshold.data[0] & 0x03)
					{
						sess_urr->trigger_volume.downlink = app_ep__get_u64( &create_urr->volume_threshold.data[bPos]);
						volume_threshold_downlink = app_ep__get_u64( &create_urr->volume_threshold.data[bPos]);
						bPos += 8;
					}
					sess_urr->trigger_volume_isset = 1;
					
					if( volume_threshold_total_present == 0)
					{
						sess_urr->trigger_volume.total = (volume_threshold_uplink + volume_threshold_downlink);
					}
				}
				
				sess_urr->isallowed = 0;
				if( sess_urr->granted_volume.total > 0)
				{
					sess_urr->isallowed = 1;
				}
				sess_urr->quota_requested_times = 0;
				sess_urr->quota_granted_times = 1;
				sess_urr->quota_status = 0;
				sess_urr->quota_requested_time = 0;

/*
				app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
					"seid=[%lu|%lu] create_urr urr-id=%u index=%d measurement_method=[%u|%u] reporting_triggers=[%u|%u] volume_quota[%u|%u|%lu|%lu|%lu] trigger_volume[%u|%u|%lu|%lu|%lu]  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
					create_urr->urr_id.u32, index, create_urr->measurement_method.presence, create_urr->measurement_method.u8, 
					create_urr->reporting_triggers.presence, create_urr->reporting_triggers.u8,
					create_urr->volume_quota.presence, sess_urr->granted_volume_isset, sess_urr->granted_volume.total, sess_urr->granted_volume.uplink, sess_urr->granted_volume.downlink,
					create_urr->volume_threshold.presence, sess_urr->trigger_volume_isset, sess_urr->trigger_volume.total, sess_urr->trigger_volume.uplink, sess_urr->trigger_volume.downlink,
					__FILE__, __FUNCTION__, __LINE__);
*/
				

				
				if( isGlobal == 0) 
				{
					pthread_mutex_lock( &pfcpSession->urr.Lock);
					
					if(!pfcpSession->urr.head)
					{
						pfcpSession->urr.head = pfcpSession->urr.current = sess_urr;
					}
					else
					{
						pfcpSession->urr.current->Next = sess_urr;
						pfcpSession->urr.current = sess_urr;
					}
					pfcpSession->urr.count++;
					sess_urr->session = pfcpSession;
					
					pthread_mutex_unlock( &pfcpSession->urr.Lock);
				}
				else if( isGlobal == 1) 
				{
					pthread_mutex_lock( &pfs->urr.Lock);
					
					if(!pfs->urr.head)
					{
						pfs->urr.head = pfs->urr.current = sess_urr;
					}
					else
					{
						pfs->urr.current->Next = sess_urr;
						pfs->urr.current = sess_urr;
					}
					pfs->urr.count++;
					
					pthread_mutex_unlock( &pfs->urr.Lock);
					
					
					int igc = 0;
					for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
					{
						if(!pfcpSession->global_urr[igc])
						{
							pfcpSession->global_urr[igc] = sess_urr;
							break;
						}
					}
					
				}
				return 1;
			}
			else
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"seid=[%lu|%lu] memory allocation failed for urr-id=%u index=%d  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
					create_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);				
			}
		} 
		else
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
				"seid=[%lu|%lu] create_urr already exists for urr-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
				create_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
		}
	}
	return 0;
}

int pfcp_stack___update_urr( int index, pfcp_tlv_update_urr_t * update_urr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( update_urr->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
	
/*	
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"seid=[%lu|%lu]  processing update_urr urr-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, update_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);
*/

		pfcp_urr_t * sess_urr = NULL;
		int bFound = 0;
		
		uint32_t urr_id 	=  update_urr->urr_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((update_urr->urr_id.u32 & 0x80000000) > 0) ? 1 : 0 ;
		
		if( isGlobal == 0)
		{
			sess_urr = pfcpSession->urr.head;
			
			while( sess_urr)
			{
				if( sess_urr->urr_id == urr_id)
				{
					bFound = 1;
					break;
				}
				sess_urr = sess_urr->Next;
			}
		}
		else if( isGlobal == 1)
		{
			sess_urr = pfs->urr.head;
			
			while( sess_urr)
			{
				if( sess_urr->urr_id == urr_id)
				{
					bFound = 1;
					break;
				}
				sess_urr = sess_urr->Next;
			}
		}

		if( bFound == 1 && sess_urr)
		{
			if( update_urr->measurement_method.presence == 1)
			{
				sess_urr->measurement_method = update_urr->measurement_method.u8;
				sess_urr->measurement_method_isset = 1;
			}

			if( update_urr->reporting_triggers.presence == 1)
			{
				sess_urr->reporting_triggers = update_urr->reporting_triggers.u8;
				sess_urr->reporting_triggers_isset = 1;
			}
			
			pthread_mutex_lock( &pfcpSession->QLock);
			
			if( sess_urr->isremoved == 1)
			{
				sess_urr->granted_volume.total = 0;
				sess_urr->granted_volume.uplink = 0;
				sess_urr->granted_volume.downlink = 0;
				sess_urr->trigger_volume.total = 0;
				sess_urr->trigger_volume.uplink = 0;
				sess_urr->trigger_volume.downlink = 0;				
				sess_urr->isremoved = 0;
			}
			
			sess_urr->quota_status = 0;
			sess_urr->quota_requested_time = 0;
			sess_urr->quota_requested_times = 0;
			sess_urr->last_quota_requested_time = 0;
	
			uint32_t granted_volume_total_present = 0;
			uint64_t granted_volume_uplink = 0;
			uint64_t granted_volume_downlink = 0;
			
			uint32_t volume_threshold_total_present = 0;
			uint64_t volume_threshold_uplink = 0;
			uint64_t volume_threshold_downlink = 0;
				
			if( update_urr->volume_quota.presence == 1)
			{
				uint8_t bPos = 1;
				uint64_t granted_now = 0;
				
				if( update_urr->volume_quota.data[0] & 0x01)
				{
					sess_urr->isallowed = 1;
					sess_urr->quota_granted_times++;
					
					if( app_ep__get_u64( &update_urr->volume_quota.data[bPos]) == 0)
					{
						sess_urr->isallowed = 0;
					}
					
					granted_volume_total_present = 1;
					granted_now = app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					sess_urr->granted_volume.total += app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					bPos += 8;
				}
				
				if( update_urr->volume_quota.data[0] & 0x02)
				{
					sess_urr->granted_volume.uplink += app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					granted_volume_uplink = app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					bPos += 8;
				}

				if( update_urr->volume_quota.data[0] & 0x03)
				{
					sess_urr->granted_volume.downlink += app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					granted_volume_downlink = app_ep__get_u64( &update_urr->volume_quota.data[bPos]);
					bPos += 8;
				}
				
				if( granted_volume_total_present == 0)
				{
					granted_now = ( granted_volume_uplink + granted_volume_downlink);
					sess_urr->granted_volume.total += ( granted_volume_uplink + granted_volume_downlink);
				}
				
				sess_urr->granted_volume_isset = 1;

				if( pfs->enable_quota_logs == 1)
				{
					app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
						"update-quota  up_f_seid=%ld  cp_f_seid=%ld  RG=%d  gran-vol=%ld", 
							pfcpSession->up_f_seid, pfcpSession->cp_f_seid, urr_id, granted_now);				
				}
			}

			if( update_urr->volume_threshold.presence == 1)
			{
				uint8_t bPos = 1;
				
				if( update_urr->volume_threshold.data[0] & 0x01)
				{
					sess_urr->trigger_volume.total += app_ep__get_u64( &update_urr->volume_threshold.data[bPos]);
					volume_threshold_total_present = 1;
					bPos += 8;
				}
				
				if( update_urr->volume_threshold.data[0] & 0x02)
				{
					sess_urr->trigger_volume.uplink += app_ep__get_u64( &update_urr->volume_threshold.data[bPos]);
					volume_threshold_uplink = app_ep__get_u64( &update_urr->volume_threshold.data[bPos]);
					bPos += 8;
				}

				if( update_urr->volume_threshold.data[0] & 0x03)
				{
					sess_urr->trigger_volume.downlink += app_ep__get_u64( &update_urr->volume_threshold.data[bPos]);
					volume_threshold_downlink = app_ep__get_u64( &update_urr->volume_threshold.data[bPos]);
					bPos += 8;
				}
				sess_urr->trigger_volume_isset = 1;
				
				if( volume_threshold_total_present == 0)
				{
					sess_urr->trigger_volume.total += ( volume_threshold_uplink + volume_threshold_downlink);
				}
			}
			
			pthread_mutex_unlock( &pfcpSession->QLock);
			
			app_logger__log( pfs->pfcpLogger, pfs->session_parameter_cat_log, APP_LOG__LEVEL_DEBUG, 
				"seid=[%lu|%lu] update_urr urr-id=%u index=%d measurement_method=[%u|%u] reporting_triggers=[%u|%u] volume_quota[%u|%u|%lu|%lu|%lu] trigger_volume[%u|%u|%lu|%lu|%lu]  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, 
				update_urr->urr_id.u32, index, update_urr->measurement_method.presence, update_urr->measurement_method.u8, 
				update_urr->reporting_triggers.presence, update_urr->reporting_triggers.u8,
				update_urr->volume_quota.presence, sess_urr->granted_volume_isset, sess_urr->granted_volume.total, sess_urr->granted_volume.uplink, sess_urr->granted_volume.downlink,
				update_urr->volume_threshold.presence, sess_urr->trigger_volume_isset, sess_urr->trigger_volume.total, sess_urr->trigger_volume.uplink, sess_urr->trigger_volume.downlink,
				__FILE__, __FUNCTION__, __LINE__);			
			
		}
		else
		{
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] update_urr, urr-not found for urr-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, update_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);			
		}		
	}
	return 0;
}

int pfcp_stack___remove_urr( int index, pfcp_tlv_remove_urr_t * remove_urr, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( remove_urr->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"seid=[%lu|%lu]  processing remove_urr urr-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, remove_urr->urr_id.u32, index, __FILE__, __FUNCTION__, __LINE__);

		uint32_t urr_id 	=  remove_urr->urr_id.u32 & 0x7FFFFFFF;
		uint32_t isGlobal 	= ((remove_urr->urr_id.u32 & 0x80000000) > 0) ? 1 : 0 ;	
		
		if( isGlobal == 0)
		{	
			pfcp_urr_t * sess_urr = pfcpSession->urr.head;
			
			while( sess_urr)
			{
				if( sess_urr->urr_id == urr_id)
				{
					sess_urr->isremoved = 1;
					break;
				}
				sess_urr = sess_urr->Next;
			}
		}
		else
		{
			int igc = 0;
			for( igc = 0; igc < MAX_GLOBAL_IDS; igc++)
			{
				if( pfcpSession->global_urr[igc])
				{
					if( pfcpSession->global_urr[igc]->urr_id == urr_id)
					{
						pfcpSession->global_urr[igc] = NULL;
						break;
					}
				}
			}
		}
		
		return 1;
	}
	return 0;
}
// --------------------------------------------------------------------------------------------------
int pfcp_stack___create_bar( int index, pfcp_tlv_create_bar_t * create_bar, pfcp_session_t * pfcpSession, pfcp_session_establishment_response_t * response)
{
	if( create_bar->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"seid=[%lu|%lu] processing create_bar bar-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_bar->bar_id.u8, index, __FILE__, __FUNCTION__, __LINE__);

		pfcp_bar_t * sess_bar = NULL;
		int bFound = 0;
		
		sess_bar = pfcpSession->bar.head;
		
		while( sess_bar)
		{
			if( sess_bar->bar_id == create_bar->bar_id.u8)
			{
				bFound = 1;
				break;
			}
			sess_bar = sess_bar->Next;
		}

		if( bFound == 0)
		{
			sess_bar = (pfcp_bar_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_bar_t) + 1);
			
			if( sess_bar)
			{
				memset( sess_bar, 0, sizeof(sizeof(pfcp_bar_t) + 1));
				sess_bar->session = pfcpSession;
				sess_bar->bar_id = create_bar->bar_id.u8;


				pthread_mutex_lock( &pfcpSession->bar.Lock);
					
				if(!pfcpSession->bar.head)
				{
					pfcpSession->bar.head = pfcpSession->bar.current = sess_bar;
				}
				else
				{
					pfcpSession->bar.current->Next = sess_bar;
					pfcpSession->bar.current = sess_bar;
				}
				pfcpSession->bar.count++;
				sess_bar->session = pfcpSession;
				
				pthread_mutex_unlock( &pfcpSession->bar.Lock);
			}
			else
			{
				//memory allocation failed
				app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"seid=[%lu|%lu] create_bar -- memory allocation failed for bar-id=%u index=%d  %s|%s|%d", 
					pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_bar->bar_id.u8, index, __FILE__, __FUNCTION__, __LINE__);
			}
		}
		else
		{
			//alreay found
			app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_ERROR, 
				"seid=[%lu|%lu] create_bar -- bar not found with bar-id=%u index=%d  %s|%s|%d", 
				pfcpSession->up_f_seid, pfcpSession->cp_f_seid, create_bar->bar_id.u8, index, __FILE__, __FUNCTION__, __LINE__);			
		}
	}
	
	return 0;
}

int pfcp_stack___remove_bar( int index, pfcp_tlv_remove_bar_t * remove_bar, pfcp_session_t * pfcpSession, pfcp_session_modification_response_t * response)
{
	if( remove_bar->presence == 1)
	{
		app_logger_t * pfcplogger = pfcp_stack__get_pfcplogger();
		
		app_logger__log( pfcplogger, NULL, APP_LOG__LEVEL_DEBUG, 
			"PFCPSessionModificationRequest: seid=[%lu|%lu] processing remove_bar bar-id=%u index=%d  %s|%s|%d", 
			pfcpSession->up_f_seid, pfcpSession->cp_f_seid, remove_bar->bar_id.u8, index, __FILE__, __FUNCTION__, __LINE__);

		pfcp_bar_t * sess_bar = pfcpSession->bar.head;

		while( sess_bar)
		{
			if( sess_bar->bar_id == remove_bar->bar_id.u8)
			{
				sess_bar->isremoved = 1;
				break;
			}
			sess_bar = sess_bar->Next;
		}

		return 1;
	}
	return 0;
}
// --------------------------------------------------------------------------------------------------







void pfcp_stack___log_received_request( char * rName, app_ep_udp_message_t * msg)
{
	char ipa[100];
	app_ep__get_strip( msg, ipa);	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, "%s: Received from CP-IP:%s  %s|%s|%d", rName, ipa, __FILE__, __FUNCTION__, __LINE__);
}


uint32_t pfcp_stack___get_start_time_as_u32( uint32_t addseconds)
{
	time_t t2 = pfs->startTime + 2208988800UL + addseconds;
	
	uint32_t t1 = 0;
	u_char * timestamp = (u_char *)&t1;
	
	timestamp[3] = (t2 >> 24) & 0xFF;
	timestamp[2] = (t2 >> 16) & 0xFF;
	timestamp[1] = (t2 >> 8) & 0xFF;
	timestamp[0] = (t2) & 0xFF;		
	
	return t1;
}


struct pfcp_node * pfcp_stack___find_node_by_ip( uint32_t ipv4)
{
	struct pfcp_node * node = pfs->pfcpNodeHead;

	while(node)
	{
		//printf("IPv4=% ipv4=%u  %d\n", node->IPv4, ipv4, __LINE__);
		
		if( node->NodeIDAddressType == 0 && node->IPv4 == ipv4)
		{
			return node;
		}
		node = node->Next;
	}
		
	return NULL;
}

void pfcp_stack___association_heartbeat_timer( uint8_t * data, int tIndex);

/*
	find and adds based on connected ip address and not on Node ID
*/
struct pfcp_node * pfcp_stack___find_node( app_ep_udp_message_t * msg, int bAdd)
{
	int ipv = app_ep__get_ipv( msg);
	
	if( ipv == 4) 
	{
		struct sockaddr_in * sa4 = app_ep__get_ip4addr( msg);
		//printf( "fn  sin_port=%d  s_addr=%d\n", sa4->sin_port, (int)sa4->sin_addr.s_addr);

		struct pfcp_node * node = pfs->pfcpNodeHead;
		pthread_mutex_lock( &pfs->pfcpNodeLock);
		
		while(node)
		{
			if( node->NodeIDAddressType == 0 && node->IPv4 == (int)sa4->sin_addr.s_addr && node->port == sa4->sin_port)
			{
				if( bAdd == 1)
				{
					node->pendingHeartbeatResponse = 0;
					node->isActive = 1;
				}
				
				gettimeofday( &node->lastmsg, NULL);
				pthread_mutex_unlock( &pfs->pfcpNodeLock);
				return node;
			}
			node = node->Next;
		}
		pthread_mutex_unlock( &pfs->pfcpNodeLock);


		if( bAdd == 1)
		{
			// printf("adding node now   IP=%d  port=%d  T=%d  %s|%d\n", 
				// (int)sa4->sin_addr.s_addr, sa4->sin_port, pfs->pfcpNodeTotal, __FILE__, __LINE__);

			
			node = (struct pfcp_node*)malloc( sizeof(struct pfcp_node));
			memset( node, 0, sizeof(struct pfcp_node));
			
			pthread_mutex_lock( &pfs->pfcpNodeLock);

			node->NodeIDAddressType = 0;
			node->IPv4 = (int)sa4->sin_addr.s_addr;
			node->id = (pfs->pfcpNodeTotal+1);
			node->port = sa4->sin_port;
			memcpy( &node->u.clientaddr, sa4, sizeof(struct sockaddr_in));
			pthread_mutex_init( &node->SeqNoLock, NULL);
			node->lastSeqNo = 1;
			node->heartBeatTimer = NULL;
			node->pendingHeartbeatResponse = 0;
			node->isActive = 1;
			node->cp_seid_table = ls__create_uint_table( pfs->session_count * 1.5, 64, 0);
			gettimeofday( &node->lastmsg, NULL);

			if(!pfs->pfcpNodeHead)
			{
				pfs->pfcpNodeHead = pfs->pfcpNodeCurrent = node;
			}
			else
			{
				pfs->pfcpNodeCurrent->Next = node;
				pfs->pfcpNodeCurrent = node;
			}
			pfs->pfcpNodeTotal++;
			
			node->heartBeatTimer = app_timer__start( (uint8_t *) node, pfcp_stack___association_heartbeat_timer, pfs->HeartbeatSeconds);
			
			pthread_mutex_unlock( &pfs->pfcpNodeLock);
		}
		return node;
	}
	else 
	{
		struct sockaddr_in6 * sa6 = app_ep__get_ip6addr( msg);	
		//printf( "fn  IPv6 Not Handled  %s|%d\n", __FILE__, __LINE__);		
		exit(0);
	}
	return NULL;
}

uint32_t pfcp_stack__get_next_sequence( struct pfcp_node * node)
{
	uint32_t sqn = 0;
	pthread_mutex_lock( &node->SeqNoLock);
	sqn = ++node->lastSeqNo;
	pthread_mutex_unlock( &node->SeqNoLock);
	return sqn;
}


void pfcp_stack__print_buffer( uint8_t * buffer, int len)
{
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf("%02X", buffer[i] & 0xFF);
	}
	printf("\n");	
}


void pfcp_stack__print_buffer_wn( char * name, uint8_t * buffer, int len)
{
	if( name) 
		printf( "%s: ", name);
	
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf("%02X", buffer[i] & 0xFF);
	}
	printf("\n");	
}

void pfcp_stack___release_session( pfcp_session_t * pfcpSession)
{
	if( pfcpSession->released == 1)
		return;
	
	pfcpSession->node = NULL;
	pfcpSession->cp_f_seid = 0;
	pfcpSession->up_f_seid = 0;

	int i = 0;
	for( i = 0; i < 10; i++)
	{
		pfcpSession->global_far[i] = NULL;
		//pfcpSession->global_bar[i] = NULL;
		pfcpSession->global_qer[i] = NULL;
		pfcpSession->global_urr[i] = NULL;
		pfcpSession->global_mar[i] = NULL;
	}

	//-------------------------------------------------------------------------
	pfcp_pdr_t * pdr_item = pfcpSession->pdr.head;
	pfcp_pdr_t * pdr_item_next = NULL;
	
	while( pdr_item)
	{
		pdr_item_next = pdr_item->Next; 
		
		pdr_item->Next = NULL;
		app_region__free( (uint8_t*)pdr_item);
		pdr_item = NULL;
		
		pdr_item = pdr_item_next;
	}
	
	pfcpSession->pdr.head = NULL;
	pfcpSession->pdr.current = NULL;
	pfcpSession->pdr.count = 0;

	//-------------------------------------------------------------------------
	pfcp_far_t * far_item = pfcpSession->far.head;
	pfcp_far_t * far_item_next = NULL;
	
	while( far_item)
	{
		far_item_next = far_item->Next; 
		
		far_item->Next = NULL;
		app_region__free( (uint8_t*)far_item);
		far_item = NULL;
		
		far_item = far_item_next;
	}

	pfcpSession->far.head = NULL;
	pfcpSession->far.current = NULL;
	pfcpSession->far.count = 0;
	//-------------------------------------------------------------------------
	pfcp_qer_t * qer_item = pfcpSession->qer.head;
	pfcp_qer_t * qer_item_next = NULL;
	
	while( qer_item)
	{
		qer_item_next = qer_item->Next; 
		
		qer_item->Next = NULL;
		app_region__free( (uint8_t*)qer_item);
		qer_item = NULL;
		
		qer_item = qer_item_next;
	}

	pfcpSession->qer.head = NULL;
	pfcpSession->qer.current = NULL;
	pfcpSession->qer.count = 0;
	//-------------------------------------------------------------------------	
	pfcp_urr_t * urr_item = pfcpSession->urr.head;
	pfcp_urr_t * urr_item_next = NULL;
	
	while( urr_item)
	{
		urr_item_next = urr_item->Next; 
		
		urr_item->Next = NULL;
		app_region__free( (uint8_t*)urr_item);
		urr_item = NULL;
		
		urr_item = urr_item_next;
	}

	pfcpSession->urr.head = NULL;
	pfcpSession->urr.current = NULL;
	pfcpSession->urr.count = 0;
	//-------------------------------------------------------------------------
	
	//pfcpSession->ip_flows = NULL;
	pfcpSession->released = 1;
	pfcpSession->version++;
}


void pfcp_stack___reinitalize_session( pfcp_session_t * pfcpSession)
{
	pfcpSession->node = NULL;
	pfcpSession->cp_f_seid = 0;
	pfcpSession->up_f_seid = 0;	

	pfcpSession->pdr.head = NULL;
	pfcpSession->pdr.current = NULL;
	pfcpSession->pdr.count = 0;

	pfcpSession->far.head = NULL;
	pfcpSession->far.current = NULL;
	pfcpSession->far.count = 0;

	pfcpSession->qer.head = NULL;
	pfcpSession->qer.current = NULL;
	pfcpSession->qer.count = 0;

	pfcpSession->bar.head = NULL;
	pfcpSession->bar.current = NULL;
	pfcpSession->bar.count = 0;

	pfcpSession->urr.head = NULL;
	pfcpSession->urr.current = NULL;
	pfcpSession->urr.count = 0;

	pfcpSession->mar.head = NULL;
	pfcpSession->mar.current = NULL;
	pfcpSession->mar.count = 0;
	
	pfcpSession->urr_rep_seq_no = 0;
	pfcpSession->pdnType = 0;
	pfcpSession->pdnType_isset = 0;
	pfcpSession->released = 0;
	
	pfcpSession->gtp_seq_no = 0;
	pfcpSession->ran_ipv4				= 0;
	memset( pfcpSession->ran_ipv6, 0, sizeof(pfcpSession->ran_ipv6));
	pfcpSession->ran_teid				= 0;
	pfcpSession->ran_ipv4_from_packet	= 0;
	memset( pfcpSession->ranmac, 0, sizeof(pfcpSession->ranmac));;
	pfcpSession->ue_ipv4				= 0;
	pfcpSession->ue_ipv4_isset			= 0;	
	memset( pfcpSession->ue_ipv6, 0, sizeof(pfcpSession->ue_ipv6));	
	pfcpSession->ue_ipv6_isset			= 0;	
	pfcpSession->upf_teid				= 0;
	pfcpSession->upf_ip					= 0;	
	pfcpSession->version++;
	pfcpSession->stime					= 0;
	pfcpSession->pdnType				= 0;
	pfcpSession->pdnType_isset			= 0;
	pfcpSession->qer_0__gate_status		= 0;
	pfcpSession->qer_0__ul_mbr			= 0;
	pfcpSession->qer_0__dl_mbr			= 0;
	pfcpSession->qer_0__qfi				= 0;
	pfcpSession->con_0__ul_mbr			= 0;
	pfcpSession->con_0__dl_mbr			= 0;
	pfcpSession->restored_from_as		= 0;
	pfcpSession->created_time			= 0;
	pfcpSession->modified_time			= 0;
	pfcpSession->idle_session_hb_count	= 0;
}

void pfcp_stack___initalize_session( pfcp_session_t * pfcpSession)
{
	pthread_mutex_init( &pfcpSession->pdr.Lock, NULL);
	pthread_mutex_init( &pfcpSession->far.Lock, NULL);
	pthread_mutex_init( &pfcpSession->qer.Lock, NULL);
	pthread_mutex_init( &pfcpSession->QLock, NULL);
	pthread_mutex_init( &pfcpSession->UrrSeqLock, NULL);
	
	pfcp_stack___reinitalize_session( pfcpSession);
	pfcpSession->initalized 			= 1;
	pfcpSession->released 				= 0;
	pfcpSession->ran_ipv4				= 0;
	memset( pfcpSession->ran_ipv6, 0, sizeof(pfcpSession->ran_ipv6));
	pfcpSession->ran_teid				= 0;
	pfcpSession->ran_ipv4_from_packet	= 0;
	memset( pfcpSession->ranmac, 0, sizeof(pfcpSession->ranmac));;
	pfcpSession->ue_ipv4				= 0;
	pfcpSession->ue_ipv4_isset			= 0;	
	memset( pfcpSession->ue_ipv6, 0, sizeof(pfcpSession->ue_ipv6));	
	pfcpSession->ue_ipv6_isset			= 0;	
	pfcpSession->upf_teid				= 0;
	pfcpSession->upf_ip					= 0;
	pfcpSession->version 				= 0;
	pfcpSession->stime					= 0;
	pfcpSession->pdnType				= 0;
	pfcpSession->pdnType_isset			= 0;
	pfcpSession->qer_0__gate_status		= 0;
	pfcpSession->qer_0__ul_mbr			= 0;
	pfcpSession->qer_0__dl_mbr			= 0;
	pfcpSession->qer_0__qfi				= 0;
	pfcpSession->con_0__ul_mbr			= 0;
	pfcpSession->con_0__dl_mbr			= 0;
	pfcpSession->restored_from_as		= 0;
	pfcpSession->created_time			= 0;
	pfcpSession->modified_time			= 0;
	pfcpSession->idle_session_hb_count	= 0;
}




void pfcp_stack___set_nodeid( pfcp_tlv_octet_t * node_id)
{
	//printf("pfs->ipv=%d \n", pfs->ipv);

	if( pfs->ipv == 4)
	{
		char data[6];
		memset( data, 0, sizeof(data));
		memcpy( &data[1], (unsigned char *)&pfs->iIPv4, 4);
		__up_pfcp_set__octet( pfs->pfcp_mregion, node_id, data, 5);
	}
	else
	{
		char data[18];
		memset( data, 0, sizeof(data));
		data[0] = 1;
		memcpy( &data[1], (unsigned char *)&pfs->iIPv6, 16);
		__up_pfcp_set__octet( pfs->pfcp_mregion, node_id, data, 17);		
	}
}


void pfcp_stack___set_fseid( pfcp_tlv_octet_t * fseid, uint64_t seid)
{
	if( pfs->ipv == 4)
	{
		char data[20];
		memset( data, 0, sizeof(data));
		data[0] |= 1 << 1;
		app_ep__encode__u64toc( &data[1], seid);
		memcpy( &data[9], (unsigned char *)&pfs->iIPv4, 4);
		__up_pfcp_set__octet( pfs->pfcp_mregion, fseid, data, 13);
	}
	else
	{
		char data[30];
		memset( data, 0, sizeof(data));
		data[0] |= 1 << 0;
		app_ep__encode__u64toc( &data[1], seid);
		memcpy( &data[9], (unsigned char *)&pfs->iIPv6, 16);
		__up_pfcp_set__octet( pfs->pfcp_mregion, fseid, data, 25);		
	}
}


void pfcp_stack___send_response( pfcp_message_t * pfcp_msg, app_ep_udp_message_t * msg)
{
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	
	if(!bObj)
	{
		int i = 0;
	
		while(i < 4)
		{
			usleep(555555);
			
			bObj = app_ep__allocate_udp_message();
			
			if(bObj)
				break;
			
			i++;
		}
	}
	
	if( bObj)
	{
		__up_pfcp_encode__message( pfcp_msg, bObj);
		
		uint16_t len = app_ep__get_len( bObj);
		if( len >= 8)
		{
			app_ep__udp_sendbymsg( msg, bObj);
		}
		
		__up_pfcp_free__message( pfcp_msg);
		app_ep__free_udp_message( bObj);	
	}
	else
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, "Sending Message Failed, Allocation of Message Object Failed  %s|%s|%d", 
			 __FILE__, __FUNCTION__, __LINE__);		
	}
}


int pfcp_stack___send_request( pfcp_message_t * pfcp_msg, struct pfcp_node * node)
{
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__message( pfcp_msg, bObj);
	
	uint16_t len = app_ep__get_len( bObj);
	uint8_t * buffer = app_ep__get_buffer( bObj);
	
	if( len >= 8)
	{
		if( node->NodeIDAddressType == 0)
		{
			app_ep__udp_send2( pfs->udp_ipv4server, (struct sockaddr *) &node->u.clientaddr, sizeof(struct sockaddr_in), buffer, len);
		}
		else if( node->NodeIDAddressType == 1)
		{
			
		}
	}
	
	__up_pfcp_free__message( pfcp_msg);
	app_ep__free_udp_message( bObj);
	return 0;
}



uint64_t pfcp_stack___get_seid( uint8_t * buffer)
{
	PFCPFlags * Flags = (PFCPFlags *)&buffer[0];
	if( Flags->SEID == 1)
	{
		return app_ep__get_u64( &buffer[4]);
	}
	return 0;
}


void pfcp_stack___set_header_seid( pfcp_message_t * pfcp_msg, uint8_t has_seid, uint64_t seid)
{
	pfcp_msg->header.Flags.SEID = has_seid;
	if( has_seid == 1) {
		pfcp_msg->header.SEID = seid;
	} else {
		pfcp_msg->header.SEID = 0;
	}	
}


void pfcp_stack___init_header( pfcp_message_t * pfcp_msg, uint8_t msgType, uint8_t * buffer, uint8_t hasSEID, uint64_t SEID)
{
	PFCPFlags * Flags = (PFCPFlags *)&buffer[0];
	
	uint32_t seqNumber = 0;
	
	if( Flags->SEID == 1) {
		seqNumber = app_ep__get_u24( &buffer[12]);
	} else {
		seqNumber = app_ep__get_u24( &buffer[4]);
	}

	// printf( "SEID=%u seqNumber=%u  %s|%d\n", Flags->SEID, seqNumber, __FILE__, __LINE__);
	__up_pfcp_set__header( pfcp_msg, msgType, hasSEID, SEID, seqNumber);
}







void pfcp_stack___send__heartbeat_request( struct pfcp_node * node)
{
	uint32_t sqn = pfcp_stack__get_next_sequence( node);

	pfcp_message_t pfcp_msg;
	__up_pfcp_set__header( &pfcp_msg, PFCP_HEARTBEAT_REQUEST, 0, 0, sqn);
	
	uint32_t rtime = pfcp_stack___get_start_time_as_u32(0);
	__up_pfcp_set__u32( &pfcp_msg.u.heartbeat_request.recovery_time_stamp, rtime);
		
	node->pendingHeartbeatResponse++;
	
	
	pthread_mutex_lock( &pfs->hb_request_sent_lock);
	pfs->hb_request_sent_total++;
	pthread_mutex_unlock( &pfs->hb_request_sent_lock);
	
	
	pfcp_stack___send_request( &pfcp_msg, node);
}


void pfcp_stack___send__heartbeat_response( struct pfcp_node * node, app_ep_udp_message_t * msg)
{
	uint16_t len = app_ep__get_len( msg);
	uint8_t * buffer = app_ep__get_buffer( msg);
	uint32_t sequence = __up_pfcp_decode__header_get_sequence( buffer);
	
	
	pthread_mutex_lock( &pfs->hb_response_sent_lock);
	pfs->hb_response_sent_total++;
	pthread_mutex_unlock( &pfs->hb_response_sent_lock);
	
	
	pfcp_message_t pfcp_msg;
	__up_pfcp_set__header( &pfcp_msg, PFCP_HEARTBEAT_RESPONSE, 0, 0, sequence);
	
	pfcp_stack___send_response( &pfcp_msg, msg);
}


void pfcp_stack___send__association_setup_response( app_ep_udp_message_t * msg, uint8_t cause)
{
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_ASSOCIATION_SETUP_RESPONSE, buffer, 0, 0);
	
	pfcp_association_setup_response_t * req = &pfcp_msg.u.association_setup_response;

	uint32_t rtime = pfcp_stack___get_start_time_as_u32( 0);
	
	pfcp_stack___set_nodeid( &req->node_id);
	__up_pfcp_set__u8( &req->cause, cause);
	__up_pfcp_set__u32( &req->recovery_time_stamp, rtime);
	__up_pfcp_set__octet( pfs->pfcp_mregion, &req->up_function_features, "\x00\x01", 2);
	
	char up_info[30];
	memset( up_info, 0, sizeof(up_info));
	
	up_info[0] = 0x21;
	memcpy( &up_info[1], (unsigned char *)&pfs->iIPv4, 4);
	up_info[5] = 0x20;
	memcpy( &up_info[6], "internet", 8);	//TODO: pick from config

	__up_pfcp_set__octet( pfs->pfcp_mregion, &req->user_plane_ip_resource_information[0], up_info, 14);
	

	pfcp_stack___send_response( &pfcp_msg, msg);
}


void pfcp_stack___send__association_update_response( app_ep_udp_message_t * msg, uint8_t cause)
{
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_ASSOCIATION_UPDATE_RESPONSE, buffer, 0, 0);
	
	pfcp_association_update_response_t * response = &pfcp_msg.u.association_update_response;

	pfcp_stack___set_nodeid( &response->node_id);
	__up_pfcp_set__u8( &response->cause, cause);
	__up_pfcp_set__octet( pfs->pfcp_mregion, &response->up_function_features, "\x00\x01", 2);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__association_release_response( app_ep_udp_message_t * msg, uint8_t cause)
{
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_ASSOCIATION_RELEASE_RESPONSE, buffer, 0, 0);
	
	pfcp_association_release_response_t * response = &pfcp_msg.u.association_release_response;

	pfcp_stack___set_nodeid( &response->node_id);
	__up_pfcp_set__u8( &response->cause, cause);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__version_not_supported_response( app_ep_udp_message_t * msg)
{
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_VERSION_NOT_SUPPORTED_RESPONSE, buffer, 0, 0);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__node_report_response( app_ep_udp_message_t * msg, uint8_t cause)
{
	uint16_t len = app_ep__get_len( msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_NODE_REPORT_RESPONSE, buffer, 0, 0);
	
	pfcp_node_report_response_t * response = &pfcp_msg.u.node_report_response;

	__up_pfcp_set__u8( &response->cause, cause);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__session_establisment_response( app_ep_udp_message_t * msg, uint8_t cause, uint64_t seid)
{
	uint16_t len = app_ep__get_len( msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_SESSION_ESTABLISHMENT_RESPONSE, buffer, 1, seid);
	
	pfcp_session_establishment_response_t * response = &pfcp_msg.u.session_establishment_response;

	__up_pfcp_set__u8( &response->cause, cause);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__session_deletion_response( app_ep_udp_message_t * msg, uint8_t cause, uint64_t seid)
{
	uint16_t len = app_ep__get_len( msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_SESSION_DELETION_RESPONSE, buffer, 1, seid);
	
	pfcp_session_deletion_response_t * response = &pfcp_msg.u.session_deletion_response;

	__up_pfcp_set__u8( &response->cause, cause);

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

void pfcp_stack___send__session_report_request( app_ep_udp_message_t * msg, uint64_t seid)
{
	uint16_t len = app_ep__get_len( msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_msg;
	pfcp_stack___init_header( &pfcp_msg, PFCP_SESSION_REPORT_REQUEST, buffer, 1, seid);
	
	//pfcp_session_report_request_t * response = &pfcp_msg.u.session_report_request;

	pfcp_stack___send_response( &pfcp_msg, msg);	
}

// -----------------------------------------------------------------------------

void pfcp_stack___handle_heartbeat_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	// pfcp_stack___log_received_request( "PFCPHeartbeatRequest", msg);
	
	pthread_mutex_lock( &pfs->hb_request_received_lock);
	pfs->hb_request_received_total++;
	pthread_mutex_unlock( &pfs->hb_request_received_lock);
	
	pfcp_stack___send__heartbeat_response( node, msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_heartbeat_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	// pfcp_stack___log_received_request( "PFCPHearbeatResponse", msg);
	
	pthread_mutex_lock( &pfs->hb_response_received_lock);
	pfs->hb_response_received_total++;
	pthread_mutex_unlock( &pfs->hb_response_received_lock);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);

	if( node->pendingHeartbeatResponse > 0)
	{
		node->pendingHeartbeatResponse--;
	}

	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_pfd_management_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPPFDManagementRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_pfd_management_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPPFDManagementResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}

void pfcp_stack___get_strip( pfcp_node_t * node, char * data)
{
	memset( data, 0, 100);
	if( node->NodeIDAddressType == 0)
	{
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &node->u.clientaddr.sin_addr, str, INET_ADDRSTRLEN);
		sprintf( data, "%s:%d", str, ntohs( node->u.clientaddr.sin_port));		 
	}
	else
	{	
		char str[INET6_ADDRSTRLEN];
		inet_ntop( AF_INET6, &node->u.clientaddr6.sin6_addr, str, INET6_ADDRSTRLEN);
		sprintf( data, "%s:%d", str, ntohs( node->u.clientaddr6.sin6_port));
	}
}

void pfcp_stack___association_heartbeat_timer( uint8_t * data, int tIndex)
{
	struct pfcp_node * node = (struct pfcp_node *)data;
	
	if(!node)
		return;
	
	
	if(node->heartBeatTimer)
	{
		app_timer__clear( node->heartBeatTimer);
		node->heartBeatTimer = NULL;
	}
	
	//printf("node=%p HeartbeatSeconds=%u  %s %d\n", node, pfs->HeartbeatSeconds, __FUNCTION__, __LINE__);
	
	
	if( node->pendingHeartbeatResponse < 4)
	{
		pfcp_stack___send__heartbeat_request( node);
		
		if(!node->heartBeatTimer)
		{
			node->heartBeatTimer = app_timer__start( (uint8_t *) node, pfcp_stack___association_heartbeat_timer, pfs->HeartbeatSeconds);
		}
	}	
	else
	{
		node->isActive = 0;

		char ipa[100];
		pfcp_stack___get_strip( node, ipa);
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "CP-IP:%s Not Responding, Making Inactive  %s|%s|%d", ipa, __FILE__, __FUNCTION__, __LINE__);
	}
}


void pfcp_stack___handle_association_setup_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationSetupRequest", msg);


	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	int ipv = app_ep__get_ipv( msg);
	char ipa[100];
	app_ep__get_strip( msg, ipa);

	
	pfcp_message_t mObj;
	int sts = __up_pfcp_decode__association_setup_request( &mObj, msg);





	
	if( sts == 0)
	{
		pfcp_association_setup_request_t * request = &mObj.u.association_setup_request;
		
		if( request->node_id.presence == 1)
		{
			if( ipv == 4)
			{
				uint32_t ipV4 = htonl( app_ep__get_u32( &request->node_id.data[1]));
				//pfcp_stack__print_buffer_wn( "IPv4", request->node_id.data, 4);
				
				//printf("IPv4=%u  ipV4=%u len=%u  ipv=%d \n", node->IPv4, ipV4, request->node_id.len, ipv); 
				//pfcp_stack__print_buffer_wn( "node_id", mObj.u.association_setup_request.node_id.data, mObj.u.association_setup_request.node_id.len);
				
				if( node->IPv4 == ipV4)
				{
					pfcp_stack___send__association_setup_response( msg, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPAssociationSetupAccepted: CP IP:%s   NodeIP:%u  %s|%s|%d", ipa, ipV4, __FILE__, __FUNCTION__, __LINE__);
					
					// if(!node->heartBeatTimer)
					// {
						// node->heartBeatTimer = app_timer__start( (uint8_t *) node, pfcp_stack___association_heartbeat_timer, pfs->HeartbeatSeconds);
					// }
				}
				else
				{
					pfcp_stack___send__association_setup_response( msg, PFCP_CAUSE_VALUE_REQUEST_REJECTED);
					
					char ipv4add[INET_ADDRSTRLEN];
					memset( ipv4add, 0, INET_ADDRSTRLEN);
					app_ep__get_str_ipv4( ipV4, ipv4add);
					
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPAssociationSetupRejected: Node ID IP Not Matched with Connected IP:%s    NodeIP:%s|%u msg-len=%d  %s|%s|%d", ipa, ipv4add, ipV4, len, __FILE__, __FUNCTION__, __LINE__);
					//goto exit;
				}
			}
			else
			{
				printf("handle IPv6");
				
			}
		}
		else 
		{
			pfcp_stack___send__association_setup_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING);
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPAssociationSetupRejected: Node ID Missing CP Node failed for IP:%s  %s|%s|%d", ipa, __FILE__, __FUNCTION__, __LINE__);
		}
	}
	else
	{
		pfcp_stack___send__association_setup_response( msg, PFCP_CAUSE_VALUE_REQUEST_REJECTED);
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPAssociationSetupRejected: Decoding failed CP Node failed for IP:%s  %s|%s|%d", ipa, __FILE__, __FUNCTION__, __LINE__);
	}
	
	//exit:
	__up_pfcp_free__message( &mObj);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_association_setup_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationSetupResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_association_setup_response_t * recv_msg = &pfcp_msg.u.association_setup_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_association_update_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationUpdateRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPAssociationUpdateAccepted: %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_association_update_request_t * recv_msg = &pfcp_msg.u.association_update_request;
		pfcp_stack___send__association_update_response( msg, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_association_update_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationUpdateResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_association_update_response_t * recv_msg = &pfcp_msg.u.association_update_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_association_release_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationReleaseRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_association_release_request_t * recv_msg = &pfcp_msg.u.association_release_request;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_association_release_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPAssociationReleaseResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_association_release_response_t * recv_msg = &pfcp_msg.u.association_release_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_version_not_supported_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPVersionNotSupportedResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_version_not_supported_response_t * recv_msg = &pfcp_msg.u.version_not_supported_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_node_report_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPNodeReportRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_node_report_request_t * recv_msg = &pfcp_msg.u.node_report_request;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_node_report_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPNodeReportResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_node_report_response_t * recv_msg = &pfcp_msg.u.node_report_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_set_deletion_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSetDeletationRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_session_set_deletion_request_t * recv_msg = &pfcp_msg.u.session_set_deletion_request;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_set_deletion_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSetDeletationResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_session_set_deletion_response_t * recv_msg = &pfcp_msg.u.session_set_deletion_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


uint32_t pfcp_stack___get_ueip( pfcp_session_t * pfcpSession, uint8_t ** ipv6, uint8_t * ipv6isset, uint8_t * ipv4isset)
{
	pfcp_pdr_t * pdr_info = pfcpSession->pdr.head;
	
	while( pdr_info)
	{
		if( pdr_info->source_interface == 1 /*CORE*/)
		{
			*ipv4isset = pdr_info->ipv4_isset;
			*ipv6isset = pdr_info->ipv6_isset;
			*ipv6 = pdr_info->ipv6;
			
			return pdr_info->ipv4;
		}
		
		pdr_info = pdr_info->Next;
	}	
	return 0;
}


uint8_t * pfcp_stack___get_ueip6( pfcp_session_t * pfcpSession)
{
	pfcp_pdr_t * pdr_info = pfcpSession->pdr.head;
	
	while( pdr_info)
	{
		if( pdr_info->source_interface == 1 /*CORE*/)
		{
			return pdr_info->ipv6;
		}
		
		pdr_info = pdr_info->Next;
	}	
	return NULL;
}



uint16_t pfcp_stack___get_upfip_and_teid( pfcp_session_t * pfcpSession, uint32_t * upfip, uint32_t * upfteid)
{
	pfcp_pdr_t * pdr_info = pfcpSession->pdr.head;
	
	while( pdr_info)
	{
		if( pdr_info->source_interface == 0 )	// Access
		{
			*upfip = pdr_info->ipv4;
			*upfteid = pdr_info->teid;
			return 1;
		}
		
		pdr_info = pdr_info->Next;
	}
	return 0;
}

uint16_t pfcp_stack___get_ranip( pfcp_session_t * pfcpSession, uint32_t * ranip, uint32_t * ranteid)
{
	pfcp_far_t * sess_far = pfcpSession->far.head;
	
	while( sess_far)
	{
		//if( sess_far->destination_interface == 0)
		if( sess_far->outer_header_creation_isset == 1 )
		{
			*ranip		= sess_far->ipv4;
			*ranteid 	= sess_far->teid;
			return 1;
		}
		sess_far = sess_far->Next;
	}
	return 0;

	/*
	pfcp_pdr_t * pdr_info = pfcpSession->pdr.head;

	while( pdr_info)
	{
		if( pdr_info->source_interface == 0 )	// Access
		{
			*ranip = pdr_info->ipv4;
			*ranteid = pdr_info->teid;
			return 1;
		}
		
		pdr_info = pdr_info->Next;
	}
	*/

	// uint32_t far_id = 0;
	// while( pdr_info)
	// {
		// if( pdr_info->source_interface == 1 )	// Core
		// {
			// far_id = pdr_info->far_id;
			// break;
		// }
		
		// pdr_info = pdr_info->Next;
	// }
	
	// if( far_id > 0)
	// {
		// pfcp_far_t * far_info = pfcpSession->far.head;
		
		// while( far_info)
		// {
			// if( far_info->far_id == far_id)
			// {
				// *ranip = far_info->ipv4;
				// *ranteid = far_info->teid;
				// return 1;
			// }
			
			// far_info = far_info->Next;
		// }
	// }
	
	//return 0;
}

app_rbnode_t * app_rbnode__get_idp( app_rbtree_t * tree, uint64_t id, int recover);
void app_rbnode__set_idp( app_rbtree_t * tree, app_rbnode_t * node, uint8_t * obj);


void pfcp_stack___set_pfcp_session_idp( pfcp_session_t * session)
{
	app_rbnode_t * objNode = app_rbnode__get_idp( pfs->pfcp_sessions_tree, session->up_f_seid, 1);
	app_rbnode__set_idp( pfs->pfcp_sessions_tree, objNode, (uint8_t *)session);
	
	// ls__setobject_u32k_lm( pfs->ueipv4_table, 	session->ue_ipv4, (uint8_t *)session);
	ls__setobject_u32k_lm( pfs->teid_table, 	session->upf_teid, (uint8_t *)session);
	
	// if( session->ue_ipv6_isset == 1 && pfs->ueipv6_table)
	// {
		// ls__setobject_u128_lm( pfs->ueipv6_table, 	session->ue_ipv6, (uint8_t *)session);
	// }
	
	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike: restored session, with ueip=%u  %u.%u.%u.%u  and upf-teid=%u   %s|%s|%d", 
		session->ue_ipv4, 
		session->ue_ipv4 & 0xFF, (session->ue_ipv4 >> 8) & 0xFF, (session->ue_ipv4 >> 16) & 0xFF, (session->ue_ipv4 >> 24) & 0xFF,
	session->upf_teid, __FILE__, __FUNCTION__, __LINE__);

}


int pfcp_stack___pool_available_for_new_sesssion( char * pName)
{
	// if( app_region__available_fd( pfs->pfcp_session_pool) == 0)
	// {
		// strcpy(pName,"pfcp-sessions");
		// return 0;
	// }
	
	app_data_pool_t * pool_b_25 = app_region__find_pool( pfs->pfcp_mregion, 25);
	app_data_pool_t * pool_b_50 = app_region__find_pool( pfs->pfcp_mregion, 50);
	app_data_pool_t * pool_b100 = app_region__find_pool( pfs->pfcp_mregion, 100);
	app_data_pool_t * pool_b200 = app_region__find_pool( pfs->pfcp_mregion, 200);
	
	
	if( app_region__available_fd( pool_b_25) < 200)
	{
		strcpy(pName,"b0025");
		return 0;
	}
	
	if( app_region__available_fd( pool_b_50) < 200)
	{
		strcpy(pName,"b0050");
		return 0;
	}
	
	if( app_region__available_fd( pool_b100) < 100)
	{
		strcpy(pName,"b0100");
		return 0;
	}
	
	if( app_region__available_fd( pool_b200) < 100)
	{
		strcpy(pName,"b0200");
		return 0;
	}
	
	return 1;
}


pfcp_session_t * pfcp_stack___allocate_session()
{
	pfcp_session_t * pfcpSession 	= NULL;
	pfcpSession 					= (pfcp_session_t *)app_region__allocate_fd( pfs->pfcp_session_pool);
	
	if(!pfcpSession)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "pfcp session allocation failed  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}


	pfcpSession->restored_from_as = 0;
	
	if( pfcpSession->initalized == 0) {
		pfcp_stack___initalize_session( pfcpSession);
	} else {
		pfcp_stack___reinitalize_session( pfcpSession);
	}
	
	return pfcpSession;
}
	

void pfcp_stack___create_session( uint64_t cp_f_seid, uint32_t upf_teid, uint32_t ueIPv4, uint8_t * ueIPv6, uint32_t rgCount, uint32_t ipflowCount)
{
	int enablePrintf = 0;
	
	// printf( "cp_f_seid=%lu  upf_teid=%u  ueIPv4=%u ", cp_f_seid, upf_teid, ueIPv4);
	
	// if(ueIPv6)
	// {
		// app_printf_buf( NULL, ueIPv6, 16);
	// }
	
	// return;
	
	pfcp_session_t * pfcpSession = (pfcp_session_t *)app_region__allocate_fd( pfs->pfcp_session_pool);

	if( pfcpSession->initalized == 0) {
		pfcp_stack___initalize_session( pfcpSession);
	} else {
		pfcp_stack___reinitalize_session( pfcpSession);
	}

	pfcpSession->up_f_seid 			= app_rbnode__get_next_idp( pfs->pfcp_sessions_tree, (uint8_t *)pfcpSession);
	pfcpSession->cp_f_seid			= cp_f_seid;
	
	pfcpSession->pdnType 			= 1;
	pfcpSession->pdnType_isset 		= 1;

	pfcpSession->ue_ipv4			= ueIPv4;
	pfcpSession->ue_ipv4_isset		= 1;	

	if( ueIPv6)
	{
		memcpy( pfcpSession->ue_ipv6, ueIPv6, 16);
		pfcpSession->ue_ipv6_isset		= 1;	
	}
	
	pfcpSession->upf_teid			= upf_teid;

	if( pfcpSession->upf_teid > 0)
	{
		ls__setobject_u32k_lm( pfs->teid_table, 	pfcpSession->upf_teid, 	(uint8_t *)pfcpSession);
	}

	// pfcp_ip_flows_t * ip_flows = NULL;
	
	// if( ueIPv4 > 0)
	// {
		// pthread_mutex_lock( &pfs->pfcp_session_lock);
		
		// ip_flows = (pfcp_ip_flows_t *)ls__getobject_u32k( pfs->ueipv4_table, ueIPv4);
		
		// if(ip_flows)
		// {
			// pfcpSession->ip_flows = ip_flows;
			
			// pthread_mutex_lock( &ip_flows->session_count_lock);
			
			// int iy = 0;
			
			// while( iy < 5)
			// {
				// if(!ip_flows->session[iy])
				// {
					// ip_flows->session[iy] = pfcpSession;
					// ip_flows->session_count++;
					// break;
				// }
				// iy++;
			// }
			
			
			// pthread_mutex_unlock( &ip_flows->session_count_lock);
			
			// if( enablePrintf == 1)
				// printf( "added to IPv4-1 ueIPv4=%u  session_count=%d  %u\n", ueIPv4, ip_flows->session_count, __LINE__);
		// }
		// else
		// {
			// ip_flows = (pfcp_ip_flows_t *)app_region__allocate_fd( pfs->pfcp_session_ip_pool);
			
			// ip_flows->Next = NULL;
			// ip_flows->ueip4 = ueIPv4;
			// memset( ip_flows->ueip6, 0, 16);
			// ip_flows->session[0] = pfcpSession;
			// ip_flows->session_count = 1;
		
			// pfcpSession->ip_flows = ip_flows;
			
			// if( enablePrintf == 1)
				// printf( "allocated added to IPv4-2 ueIPv4=%u  session_count=%d   %u\n", ueIPv4, ip_flows->session_count, __LINE__);
		// }
		
		// ls__setobject_u32k_lm( pfs->ueipv4_table, ueIPv4, (uint8_t *)ip_flows);
		
		// pthread_mutex_unlock( &pfs->pfcp_session_lock);
		
		
	// }


	// if( ueIPv6 && pfcpSession->ue_ipv6_isset == 1&& pfs->ueipv6_table)
	// {
		// pthread_mutex_lock( &pfs->pfcp_session_lock);
		
		// if(ip_flows)
		// {
			// memcpy( ip_flows->ueip6, ueIPv6, 16);
			// ls__setobject_u128_lm( pfs->ueipv6_table, ueIPv6,  (uint8_t *)ip_flows);
			
			
			// if( enablePrintf == 1)
			// printf( "added to IPv6-1   %u\n", __LINE__);
		// }
		// else
		// {
			// ip_flows = (pfcp_ip_flows_t *)ls__getobject_u128k( pfs->ueipv6_table, ueIPv6);
			
			// if(ip_flows)
			// {
				// pfcpSession->ip_flows = ip_flows;
				
				// pthread_mutex_lock( &ip_flows->session_count_lock);
				// int iy = 0;
			
				// while( iy < 5)
				// {
					// if(!ip_flows->session[iy])
					// {
						// ip_flows->session[iy] = pfcpSession;
						// ip_flows->session_count++;
						// break;
					// }
					// iy++;
				// }
			
				// pthread_mutex_unlock( &ip_flows->session_count_lock);


				// if( enablePrintf == 1)
				// printf( "added to IPv6-2  %u\n", __LINE__);
			// }
			// else
			// {
				// ip_flows = (pfcp_ip_flows_t *)app_region__allocate_fd( pfs->pfcp_session_ip_pool);
				
				// ip_flows->Next = NULL;
				// ip_flows->ueip4 = 0;
				// // ip_flows->fHead = NULL;
				// // ip_flows->fCurrent = NULL;
				// ip_flows->session[0] = pfcpSession;
				// ip_flows->session_count = 1;

// //				pthread_mutex_init( &ip_flows->fLock, NULL);
// //				pthread_mutex_init( &ip_flows->session_count_lock, NULL);
			
				// pfcpSession->ip_flows = ip_flows;
				
				// memcpy( ip_flows->ueip6, ueIPv6, 16);
				// ls__setobject_u128_lm( pfs->ueipv6_table, ueIPv6, (uint8_t *)ip_flows);
				
				
				// if( enablePrintf == 1)
				// printf( "allocated added to IPv6-3  %u\n", __LINE__);
			// }
		// }
		
		// pthread_mutex_unlock( &pfs->pfcp_session_lock);		
	// }
	
	pfcpSession->modified_time 			= time( NULL);
	pfcpSession->idle_session_hb_count 	= 0;

	

	pfcp_urr_t * sess_urr = NULL;
	int i = 0;
	
	for( i = 0; i < rgCount; i++)
	{
		sess_urr = (pfcp_urr_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_urr_t) + 1);
		memset( sess_urr, 0, sizeof(sizeof(pfcp_urr_t) + 1));
		sess_urr->urr_id = (i + 1);

		sess_urr->measurement_method = 1;
		sess_urr->measurement_method_isset = 1;

		sess_urr->reporting_triggers = 1;
		sess_urr->reporting_triggers_isset = 1;

		sess_urr->used_volume.total = 0;
		sess_urr->used_volume.uplink = 0;
		sess_urr->used_volume.downlink = 0;

		sess_urr->reported_volume.total = 0;
		sess_urr->reported_volume.uplink = 0;
		sess_urr->reported_volume.downlink = 0;
		
		sess_urr->total_reported_volume.total = 0;
		sess_urr->total_reported_volume.uplink = 0;
		sess_urr->total_reported_volume.downlink = 0;
		
		sess_urr->granted_volume.total			= 500000;
		sess_urr->granted_volume.uplink			= 500000;
		sess_urr->granted_volume.downlink		= 500000;
			
		sess_urr->isallowed = 1;
		sess_urr->quota_granted_times = 1;
		sess_urr->quota_status = 0;
		sess_urr->quota_requested_time = 0;	
		sess_urr->quota_requested_times = 0;	
		sess_urr->last_quota_requested_time = 0;


		pthread_mutex_lock( &pfcpSession->urr.Lock);
						
		if(!pfcpSession->urr.head)
		{
			pfcpSession->urr.head = pfcpSession->urr.current = sess_urr;
		}
		else
		{
			pfcpSession->urr.current->Next = sess_urr;
			pfcpSession->urr.current = sess_urr;
		}
		pfcpSession->urr.count++;
		sess_urr->session = pfcpSession;
		
		pthread_mutex_unlock( &pfcpSession->urr.Lock);
	}
	

	i = 0;
	ip4_flow_t * ipFlow = NULL;
	
	for( i = 0; i < ipflowCount; i++)
	{
		ipFlow = (ip4_flow_t*)app_region__allocate_fd( pfs->ippool_ipv4);

		ipFlow->session 	= pfcpSession;
		ipFlow->lastused	= (uint64_t)time(NULL);
		ipFlow->rgid		= (1 + i);
		ipFlow->version		= pfcpSession->version;
		dpe_add_ipv4flow( pfcpSession->ue_ipv4, i + 100, i + 100, i + 100, 6, ipFlow);
	}

	// if( enablePrintf == 1)
	// printf("\n");


	app_logger__log( 
			pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
			"Created TDD Session with  SEID=%lu TEID=%u UEIPv4=%u rgCount=%u    %s|%s|%d", 
			cp_f_seid, upf_teid, ueIPv4, rgCount,
		__FILE__, __FUNCTION__, __LINE__);

	app_region__print_stats( pfs->pfcpLogger, pfs->pfcp_mregion);
	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
		"------------------------------------------------------------------------");

}


void pfcp_stack___send_session_establishment_response_from_sesssion( pfcp_session_t * pfcp_session, app_ep_udp_message_t * msg)
{
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_message_t pfcp_response;
	pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_ESTABLISHMENT_RESPONSE, buffer, 1, pfcp_session->cp_f_seid);	
	pfcp_session_establishment_response_t * response = &pfcp_response.u.session_establishment_response;	
	
	pfcp_stack___set_nodeid( &response->node_id);
	__up_pfcp_set__u8( &response->cause, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);	
	pfcp_stack___set_fseid( &response->up_f_seid, pfcp_session->up_f_seid);

	pfcp_stack___send_response( &pfcp_response, msg);
	pfcp_stack__perf_inc_sea();
}



void pfcp_stack___delete_session( pfcp_session_t * pfcpSession);
		
void pfcp_stack___handle_session_establishment_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	//pfcp_stack___log_received_request( "PFCPSessionEstablishmentRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	pfcp_message_t pfcp_request;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_request, msg);
	char pName[50];
	memset( pName, 0, sizeof(pName));
	
	if( sts != 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: Decoding Failed %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);		
		return;
	}

	pfcp_session_establishment_request_t * request 	 = &pfcp_request.u.session_establishment_request;

	if( request->node_id.presence == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: Node ID IE Missing  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, 0);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	
	if( request->cp_f_seid.presence == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: CP-F_SEID IE Missing  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, 0);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	
	uint64_t cp_f_seid =  app_ep__get_u64( &request->cp_f_seid.data[1]);
	//printf( "cp_f_seid=%lu  \n", cp_f_seid);

	if( cp_f_seid == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: CP-F_SEID IS Zero  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, 0);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	

	char cp_ip[INET_ADDRSTRLEN];
	memset( cp_ip, 0, INET_ADDRSTRLEN);
	app_ep__get_str_ipv4( node->IPv4, cp_ip);

	
	if( node->cp_seid_table)
	{
		pfcp_session_t * temp_pfcp_session 	= (pfcp_session_t *)ls__getobject_u64k( node->cp_seid_table, cp_f_seid);
	
		if(temp_pfcp_session)
		{
			if( temp_pfcp_session->created_time == 0)
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
					"PFCPSessionEstablishmentRequest: Session Establishment Request is in progress for %s:%lu  %s|%s|%d",  cp_ip, cp_f_seid, __FILE__, __FUNCTION__, __LINE__);
				
				__up_pfcp_free__message( &pfcp_request);
				app_ep__free_udp_message( msg);
				return;
			}
			else
			{
				if( (temp_pfcp_session->created_time + (5 * 60)) < time(NULL))
				{
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
						"PFCPSessionEstablishmentRequest: Session Establishment Request, Deleting existing session and creating new  %s:%lu  %s|%s|%d",  cp_ip, cp_f_seid, __FILE__, __FUNCTION__, __LINE__);
				
					pfcp_stack___delete_session( temp_pfcp_session);
				}
				else
				{
					//session established , send response
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
						"PFCPSessionEstablishmentRequest: Session Establishment Request, Session Alreay established, sending response %s:%lu  %s|%s|%d",  cp_ip, cp_f_seid, __FILE__, __FUNCTION__, __LINE__);
					
					pfcp_stack__perf_inc_sea();
					pfcp_stack___send_session_establishment_response_from_sesssion( temp_pfcp_session, msg);
					
					__up_pfcp_free__message( &pfcp_request);
					app_ep__free_udp_message( msg);
					return;
				}	
			}
		}
	}


	if( request->create_pdr[0].presence == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: create_pdr[0] IE Missing  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, cp_f_seid);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	
	if( pfcp_stack___pool_available_for_new_sesssion( pName) == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: Insufficient Pool=%s  %s|%s|%d", pName, __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_PFCP_ENTITY_IN_CONGESTION, cp_f_seid);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	

	if( request->create_pdr[1].presence == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: create_pdr[1] IE Missing  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, cp_f_seid);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;		
	}
	
	if( request->pdn_type.presence == 0)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: pdn_type IE Missing  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, cp_f_seid);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;
	}
	
	int integration_test = 0;

	if( integration_test == 1)
	{
		if( cp_f_seid == 4001)
		{
			pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING, cp_f_seid);
			__up_pfcp_free__message( &pfcp_request);
			app_ep__free_udp_message( msg);
			return;
		}

		if( cp_f_seid == 4002)
		{
			__up_pfcp_free__message( &pfcp_request);
			app_ep__free_udp_message( msg);
			return;
		}
	}


	pfcp_session_t * pfcpSession 	= NULL;
	pfcpSession 					= (pfcp_session_t *)app_region__allocate_fd( pfs->pfcp_session_pool);
	
	if(!pfcpSession)
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: Unable to allocate session  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		pfcp_stack___send__session_establisment_response( msg, PFCP_CAUSE_VALUE_SYSTEM_FAILURE, cp_f_seid);
		__up_pfcp_free__message( &pfcp_request);
		app_ep__free_udp_message( msg);
		return;			
	}
	
	if( pfcpSession->initalized == 0) {
		pfcp_stack___initalize_session( pfcpSession);
	} else {
		pfcp_stack___reinitalize_session( pfcpSession);
	}
	
	ls__setobject_u64k_lm( node->cp_seid_table , cp_f_seid, (uint8_t *)pfcpSession);
	
	
	pfcp_stack__perf_inc_ser();
	#ifdef ENABLE_PROMETHEUS
		successful_task_request( "POST",1); 
	#endif
	pfcp_message_t pfcp_response;
	pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_ESTABLISHMENT_RESPONSE, buffer, 1, cp_f_seid);	
	pfcp_session_establishment_response_t * response = &pfcp_response.u.session_establishment_response;	

	
	pfcpSession->up_f_seid 			= app_rbnode__get_next_idp( pfs->pfcp_sessions_tree, (uint8_t *)pfcpSession);
	pfcpSession->cp_f_seid			= cp_f_seid;
	pfcpSession->node				= node;
	pfcpSession->nodeIPv4			= node->IPv4;
	
	int i = 0;
	for( i = 0; i < 8; i++)
	{
		if( request->create_pdr[i].presence == 1)
		{
			pfcp_stack___create_pdr( i, &request->create_pdr[i], pfcpSession, response, NULL);
		} 
		else 
		{
			break;
		}
	}
	
	for( i = 0; i < 8; i++)
	{
		if( request->create_far[i].presence == 1)
		{
			pfcp_stack___create_far( i, &request->create_far[i], pfcpSession, response);
		}
		else
		{
			break;
		}
	}

	for( i = 0; i < 6; i++)
	{
		if( request->create_urr[i].presence == 1)
		{
			pfcp_stack___create_urr( i, &request->create_urr[i], pfcpSession, response);
		}
		else
		{
			break;
		}
	}
	
	for( i = 0; i < 4; i++)
	{
		if( request->create_qer[i].presence == 1)
		{
			pfcp_stack___create_qer( i, &request->create_qer[i], pfcpSession, response);
		}
		else
		{
			break;
		}
	}
	
	pfcp_stack___create_bar( i, &request->create_bar, pfcpSession, response);
	
	
	if( request->pdn_type.presence == 1)
	{
		pfcpSession->pdnType = request->pdn_type.u8;
		pfcpSession->pdnType_isset = 1;
	}
	
	
	pfcp_stack___set_nodeid( &response->node_id);
	__up_pfcp_set__u8( &response->cause, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);	
	pfcp_stack___set_fseid( &response->up_f_seid, pfcpSession->up_f_seid);
	
	
	
	uint32_t ueip = 0;
	uint32_t ran_ip = 0;
	uint32_t ran_teid = 0;
	uint8_t * ueipv6 = NULL;
	
	ueip = pfcp_stack___get_ueip( pfcpSession, &ueipv6, &pfcpSession->ue_ipv6_isset, &pfcpSession->ue_ipv4_isset);
	
	pfcpSession->ue_ipv4 = ueip;
	//pfcpSession->ue_ipv6_isset = 0;
	
	//uint8_t * ueipv6 = pfcp_stack___get_ueinfo( pfcpSession, &pfcpSession->ue_ipv4, &pfcpSession->upf_teid);
	//uint8_t * ueipv6 = pfcp_stack___get_ueip6( pfcpSession);

	
	if( ueipv6)
	{
		memcpy( pfcpSession->ue_ipv6, ueipv6, 16);
		pfcpSession->ue_ipv6_isset = 1;
	}
	
	
	pfcp_stack___get_ranip( pfcpSession, &ran_ip, &ran_teid);
	
	pfcp_stack___get_upfip_and_teid( pfcpSession, &pfcpSession->upf_ip, &pfcpSession->upf_teid);
	
	
	
	// pfcp_far_t * far_info = pfcpSession->far.head;
	// uint32_t oh_ipv4 = 0;
	// uint32_t oh_teid = 0;
	
	// while( far_info)
	// {
		// if( far_info->outer_header_creation_isset == 1)
		// {
			// oh_ipv4 = far_info->ipv4;
			// oh_teid = far_info->teid;
			// break;
		// }
		
		// far_info = far_info->Next;
	// }
	
	
	
	
	char ueip_str[INET_ADDRSTRLEN];
	memset( ueip_str, 0, INET_ADDRSTRLEN);
	
	char upfip_str[INET_ADDRSTRLEN];
	memset( upfip_str, 0, INET_ADDRSTRLEN);	

	char ranip_str[INET_ADDRSTRLEN];
	memset( ranip_str, 0, INET_ADDRSTRLEN);	
	
	// char ohip_str[INET_ADDRSTRLEN];
	// memset( ohip_str, 0, INET_ADDRSTRLEN);	


	
	app_ep__get_str_ipv4( htonl(ueip), ueip_str);
	app_ep__get_str_ipv4( htonl(ran_ip), ranip_str);
	//app_ep__get_str_ipv4( htonl(oh_ipv4), ohip_str);
	app_ep__get_str_ipv4( htonl(pfcpSession->upf_ip), upfip_str);
	
	
	if( pfcpSession->upf_teid > 0)
	{
		ls__setobject_u32k_lm( pfs->teid_table, 	pfcpSession->upf_teid, 	(uint8_t *)pfcpSession);
	}
	
	// pfcp_ip_flows_t * ip_flows = NULL;
	
	// if( ueip > 0)
	// {
		// //dpdk_session__ipv4_add( htonl(ueip), (void *) pfcpSession, 1);
		// //ls__setobject_u32k_lm( pfs->ueipv4_table, 	ueip, 					(uint8_t *)pfcpSession);
	
		// pthread_mutex_lock( &pfs->pfcp_session_lock);
		
		// ip_flows = (pfcp_ip_flows_t *)ls__getobject_u32k( pfs->ueipv4_table, ueip);
		
		// if(ip_flows)
		// {
			// pfcpSession->ip_flows = ip_flows;
			
			// pthread_mutex_lock( &ip_flows->session_count_lock);
			
			// int iy = 0;
			
			// while( iy < 5)
			// {
				// if(!ip_flows->session[iy])
				// {
					// ip_flows->session[iy] = pfcpSession;
					// ip_flows->session_count++;
					// break;
				// }
				// iy++;
			// }
			
			
			// pthread_mutex_unlock( &ip_flows->session_count_lock);
		// }
		// else
		// {
			// ip_flows = (pfcp_ip_flows_t *)app_region__allocate_fd( pfs->pfcp_session_ip_pool);
			
			// ip_flows->Next = NULL;
			// ip_flows->ueip4 = ueip;
			// memset( ip_flows->ueip6, 0, 16);
			// // ip_flows->fHead = NULL;
			// // ip_flows->fCurrent = NULL;
			// ip_flows->session[0] = pfcpSession;
			// ip_flows->session_count = 1;

// //			pthread_mutex_init( &ip_flows->fLock, NULL);
// //			pthread_mutex_init( &ip_flows->session_count_lock, NULL);
		
			// pfcpSession->ip_flows = ip_flows;
		// }
		
		// ls__setobject_u32k_lm( pfs->ueipv4_table, ueip, (uint8_t *)ip_flows);
		
		// pthread_mutex_unlock( &pfs->pfcp_session_lock);
		
		// //printf("saved session with upf_teid=%u ueip=%u  %d|%s\n", pfcpSession->upf_teid, ueip, __LINE__, __FILE__);
		
		// //dpdk_session__seid_add( uint64_t seid, void * data, int deleteIfExists);
		// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, "added session context with ueip=%u   %u.%u.%u.%u   and teid=%u   %s|%s|%d", ueip, 
				// // (ueip >> 24) & 0xFF, (ueip >> 16) & 0xFF, (ueip >> 8) & 0xFF, ueip & 0xFF, 
			// // pfcpSession->upf_teid, __FILE__, __FUNCTION__, __LINE__);
	// }
	
	// if( ueipv6 && pfcpSession->ue_ipv6_isset == 1&& pfs->ueipv6_table)
	// {
		// //ls__setobject_u128_lm( pfs->ueipv6_table, 	ueipv6, 				(uint8_t *)pfcpSession);
		
		// pthread_mutex_lock( &pfs->pfcp_session_lock);
		
		// if(ip_flows)
		// {
			// memcpy( ip_flows->ueip6, ueipv6, 16);
			// ls__setobject_u128_lm( pfs->ueipv6_table, ueipv6,  (uint8_t *)ip_flows);
		// }
		// else
		// {
			// ip_flows = (pfcp_ip_flows_t *)ls__getobject_u128k( pfs->ueipv6_table, ueipv6);
			
			// if(ip_flows)
			// {
				// pfcpSession->ip_flows = ip_flows;
				
				// pthread_mutex_lock( &ip_flows->session_count_lock);
				// int iy = 0;
			
				// while( iy < 5)
				// {
					// if(!ip_flows->session[iy])
					// {
						// ip_flows->session[iy] = pfcpSession;
						// ip_flows->session_count++;
						// break;
					// }
					// iy++;
				// }
			
				// pthread_mutex_unlock( &ip_flows->session_count_lock);
			// }
			// else
			// {
				// ip_flows = (pfcp_ip_flows_t *)app_region__allocate_fd( pfs->pfcp_session_ip_pool);
				
				// ip_flows->Next = NULL;
				// ip_flows->ueip4 = 0;
				// // ip_flows->fHead = NULL;
				// // ip_flows->fCurrent = NULL;
				// ip_flows->session[0] = pfcpSession;
				// ip_flows->session_count = 1;

// //				pthread_mutex_init( &ip_flows->fLock, NULL);
// //				pthread_mutex_init( &ip_flows->session_count_lock, NULL);
			
				// pfcpSession->ip_flows = ip_flows;
				
				// memcpy( ip_flows->ueip6, ueipv6, 16);
				// ls__setobject_u128_lm( pfs->ueipv6_table, ueipv6, (uint8_t *)ip_flows);
			// }
		// }
		
		// pthread_mutex_unlock( &pfs->pfcp_session_lock);
	// }
	
	

	
	//uint8_t qfi = pfcp_stack__get_qfi( (void *)pfcpSession);
	uint8_t 	qfifound = 0;
	uint8_t 	qfi = 0;
	uint8_t 	gate_status = 0;
	uint64_t 	ul_mbr = 0;
	uint64_t 	dl_mbr = 0;
	pfcp_stack__get_qerinfo( (void *)pfcpSession, &qfifound, &qfi, &gate_status, &ul_mbr, &dl_mbr);

	if( qfifound)
	{
		pfcpSession->qer_0__gate_status		= gate_status;
		pfcpSession->qer_0__ul_mbr			= ul_mbr;
		pfcpSession->qer_0__dl_mbr 			= dl_mbr;
		pfcpSession->qer_0__qfi				= qfi;
	}

	
	pfcpSession->idle_session_hb_count 	= 0;


	#if AEROSPIKE
		app__as__pfcpsavesession( pfcpSession);
	#endif

	// upf->ran-ip=%u|%s  upf->ran-teid=%u   oh_ipv4, ohip_str, oh_teid,   

	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionEstablishmentRequest: PFCP Created session=%p  with UPF-SEID=%lu CP-SEID=%s:%lu  ue-ip=%u|%s  ue-ip-v6[%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X]  ran_ip=%u|%s  ran_teid=%u  upf-access-ip=%u|%s  upf-teid=%u   qer-found=%u  qfi=%u  gate_status=%u   ul_mbr=%lu   dl_mbr=%lu    pdnType=%u    %s|%s|%d", 
			pfcpSession, pfcpSession->up_f_seid, cp_ip, pfcpSession->cp_f_seid, ueip, ueip_str, 
			pfcpSession->ue_ipv6[0] & 0xFF, pfcpSession->ue_ipv6[1] & 0xFF, pfcpSession->ue_ipv6[2] & 0xFF, pfcpSession->ue_ipv6[3] & 0xFF, pfcpSession->ue_ipv6[4] & 0xFF, pfcpSession->ue_ipv6[5] & 0xFF, pfcpSession->ue_ipv6[6] & 0xFF, pfcpSession->ue_ipv6[7] & 0xFF,
			pfcpSession->ue_ipv6[8] & 0xFF, pfcpSession->ue_ipv6[9] & 0xFF, pfcpSession->ue_ipv6[10] & 0xFF, pfcpSession->ue_ipv6[11] & 0xFF, pfcpSession->ue_ipv6[12] & 0xFF, pfcpSession->ue_ipv6[13] & 0xFF, pfcpSession->ue_ipv6[14] & 0xFF, pfcpSession->ue_ipv6[15] & 0xFF,
			ran_ip, ranip_str, ran_teid, pfcpSession->upf_ip, upfip_str, pfcpSession->upf_teid, qfifound, qfi, gate_status, ul_mbr, dl_mbr, pfcpSession->pdnType, 
		__FILE__, __FUNCTION__, __LINE__);

	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "CSV-SER, %ld, %s, %d, %s, %s, %d, %d, %d, %d, %lu, %lu, %u", 
			pfcpSession->up_f_seid, cp_ip, pfcpSession->cp_f_seid, ueip_str,
			upfip_str, pfcpSession->upf_teid, qfifound, qfi, gate_status, ul_mbr, dl_mbr, pfcpSession->pdnType
		);

	pfcp_stack___send_response( &pfcp_response, msg);
	pfcpSession->modified_time 			= time( NULL);
	pfcpSession->created_time 			= time( NULL);
	
	pfcp_stack__perf_inc_sea();
	#ifdef ENABLE_PROMETHEUS
		successful_task_responce( "POST",1);
	#endif
	//__up_pfcp_free__message( &pfcp_response);
	__up_pfcp_free__message( &pfcp_request);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___handle_session_establishment_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSessionEstablishmentResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);

	
	if( sts == 0 )
	{
		pfcp_session_establishment_response_t * recv_msg = &pfcp_msg.u.session_establishment_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}





void pfcp_stack___handle_session_modification_request__fsm( uint8_t * data, int tIndex)
{
	pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *)data;
	struct pfcp_node * node = msg_helper->node;
	app_ep_udp_message_t * msg = msg_helper->msg;
	
	app_region__free( (uint8_t *)msg_helper);

	
	//pfcp_stack___log_received_request( "PFCPSessionModificationRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	pfcp_session_t * pfcpSession = NULL;
	
	if( sts == 0)
	{
		if( pfcp_msg.SEID > 0)
		{

			pfcp_session_modification_request_t * recv_msg = &pfcp_msg.u.session_modification_request;
			pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, pfcp_msg.SEID);
		
			if( pfcpSession)
			{
				// printf( "SEID=%lu pfcpSession=%p \n", pfcp_msg.SEID, pfcpSession);
				pfcp_stack__perf_inc_smr();
				#ifdef ENABLE_PROMETHEUS
					successful_task_request( "POST",2);
				#endif
				pfcp_message_t pfcp_response;
				pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_MODIFICATION_RESPONSE, buffer, 1, pfcpSession->cp_f_seid);
				pfcp_session_modification_response_t * response = &pfcp_response.u.session_modification_response;	

				int i = 0;
				
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->create_pdr[i].presence == 1)
					{
						pfcp_stack___create_pdr( i, &recv_msg->create_pdr[i], pfcpSession, NULL, response);
					} 
					else 
					{
						break;
					}
				}
				
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->create_far[i].presence == 1)
					{
						pfcp_stack___create_far( i, &recv_msg->create_far[i], pfcpSession, NULL);
					}
					else
					{
						break;
					}
				}

				for( i = 0; i < 10; i++)
				{
					if( recv_msg->create_urr[i].presence == 1)
					{
						pfcp_stack___create_urr( i, &recv_msg->create_urr[i], pfcpSession, NULL);
					}
					else
					{
						break;
					}
				}
				
				for( i = 0; i < 4; i++)
				{
					if( recv_msg->create_qer[i].presence == 1)
					{
						pfcp_stack___create_qer( i, &recv_msg->create_qer[i], pfcpSession, NULL);
					}
					else
					{
						break;
					}
				}
				
				if( recv_msg->create_bar.presence == 1)
				{
					pfcp_stack___create_bar( i, &recv_msg->create_bar, pfcpSession, NULL);
				}
				
				//update
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->update_pdr[i].presence == 1)
					{
						pfcp_stack___update_pdr( i, &recv_msg->update_pdr[i], pfcpSession, response);
					} 
					else 
					{
						break;
					}
				}				
				
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->update_far[i].presence == 1)
					{
						pfcp_stack___update_far( i, &recv_msg->update_far[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}

				for( i = 0; i < 10; i++)
				{
					if( recv_msg->update_urr[i].presence == 1)
					{
						pfcp_stack___update_urr( i, &recv_msg->update_urr[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}

				for( i = 0; i < 4; i++)
				{
					if( recv_msg->update_qer[i].presence == 1)
					{
						pfcp_stack___update_qer( i, &recv_msg->update_qer[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}

				//remove
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->remove_pdr[i].presence == 1)
					{
						pfcp_stack___remove_pdr( i, &recv_msg->remove_pdr[i], pfcpSession, response);
					} 
					else 
					{
						break;
					}
				}				
				
				for( i = 0; i < 8; i++)
				{
					if( recv_msg->remove_far[i].presence == 1)
					{
						pfcp_stack___remove_far( i, &recv_msg->remove_far[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}

				for( i = 0; i < 10; i++)
				{
					if( recv_msg->remove_urr[i].presence == 1)
					{
						pfcp_stack___remove_urr( i, &recv_msg->remove_urr[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}

				for( i = 0; i < 4; i++)
				{
					if( recv_msg->remove_qer[i].presence == 1)
					{
						pfcp_stack___remove_qer( i, &recv_msg->remove_qer[i], pfcpSession, response);
					}
					else
					{
						break;
					}
				}
				
				
				uint32_t ran_ipv4 = pfcpSession->ran_ipv4;
				uint32_t ran_teid = pfcpSession->ran_teid; 
				uint32_t ran_ip_changed = 0;
				
				pfcp_stack___get_ranip( pfcpSession, &pfcpSession->ran_ipv4, &pfcpSession->ran_teid);
				pfcpSession->ran_ipv4 = htonl(pfcpSession->ran_ipv4);


				#if AEROSPIKE
					
					//if( pfcpSession->ran_ipv4 > 0)
					{
						//printf("updating ranip to aql =%u \n",pfcpSession->ran_ipv4);
						app__as__pfcpsavesession( pfcpSession);
					}
					
				#endif

				if( ran_ipv4 != pfcpSession->ran_ipv4)
				{
					ran_ip_changed = 1;
				}
				
				
				pfcpSession->modified_time = time( NULL);
				pfcpSession->idle_session_hb_count 	= 0;
				
				
				
	
				char ranip_str[INET_ADDRSTRLEN];
				memset( ranip_str, 0, INET_ADDRSTRLEN);	
				//app_ep__get_str_ipv4( htonl(pfcpSession->ran_ipv4), ranip_str);
				app_ep__get_str_ipv4( pfcpSession->ran_ipv4, ranip_str);
				
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionModificationRequest:  UPF-SEID=%lu   ran_ip=%u|%s   ran_teid=%u ran_ip_changed=%u   pdr-count=%d  far-count=%d  qer-count=%d  bar-count=%d  urr-count=%d  mar-count=%d    %s|%s|%d", 
					pfcp_msg.SEID, pfcpSession->ran_ipv4, ranip_str, pfcpSession->ran_teid, ran_ip_changed, 
					pfcpSession->pdr.count, pfcpSession->far.count, pfcpSession->qer.count,
					pfcpSession->bar.count, pfcpSession->urr.count, pfcpSession->mar.count,
					__FILE__, __FUNCTION__, __LINE__);
				
				__up_pfcp_set__u8( &response->cause, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);	
				pfcp_stack___send_response( &pfcp_response, msg);
				pfcp_stack__perf_inc_sma();
				#ifdef ENABLE_PROMETHEUS
					successful_task_responce("POST",2);
				#endif
			}
			else
			{
				
			}
		}
	}
	__up_pfcp_free__message( &pfcp_msg);
	//__up_pfcp_free__message( &pfcp_response);
	app_ep__free_udp_message( msg);
}

void pfcp_stack___handle_session_deletion_request__fsm( uint8_t * data, int tIndex);

void pfcp_stack___handle_session_modification_or_delete_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex, int type)
{
	pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_msg_helper_t) + 1);
	
	if( msg_helper)
	{
		msg_helper->node 		= node;
		msg_helper->msg			= msg;
		msg_helper->tIndex		= tIndex;
		
		uint8_t * buffer = app_ep__get_buffer( msg);
		uint64_t seid = pfcp_stack___get_seid( buffer);
		pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, seid);
		
		if( pfcpSession)
		{
			if( pfcpSession->released == 1)
			{	
				app_ep__free_udp_message( msg);
				return;	
			}
			
			if(!pfcpSession->node)
			{
				pfcpSession->node = node;
			}
				
			
			int sts = 0;
			
			if( type == 2) {
				sts = app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, seid, pfcp_stack___handle_session_modification_request__fsm);
			} else if( type == 3) {
				sts = app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)msg_helper, seid, pfcp_stack___handle_session_deletion_request__fsm);
			}
			
			if( sts < 0)
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, " fsm enquee failed for msg_helper at index=%d with sts=%d for seid=%lu  %s|%s|%d", index, sts, seid, __FILE__, __FUNCTION__, __LINE__);
				app_ep__free_udp_message( msg);
			}
		}
		else
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, " session not found with seid=%lu at index=%d   %s|%s|%d", seid, index, __FILE__, __FUNCTION__, __LINE__);
			app_ep__free_udp_message( msg);
		}
	}
	else
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, " memory allocation failed for msg_helper index=%d  %s|%s|%d", index, __FILE__, __FUNCTION__, __LINE__);
		app_ep__free_udp_message( msg);	
	}
}

void pfcp_stack___handle_session_modification_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSessionModificationResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}


void pfcp_stack___delete_session( pfcp_session_t * pfcpSession)
{
	app_exec_time_t as_del_time;
	app_exec_time_t seid_del_time;
	app_exec_time_t teid_del_time;
	app_exec_time_t ue_ipv4_del_time;
	app_exec_time_t ue_ipv6_del_time;
	
	
	#if AEROSPIKE
		app_start_time( &as_del_time);
		app__as__remove( pfcpSession);
		app_end_time( &as_del_time);
	#else
		app_start_time( &as_del_time);
		app_end_time( &as_del_time);
	#endif


	app_start_time( &seid_del_time);
	//usleep( 999999);
	app_rbnode__free_idp( pfs->pfcp_sessions_tree, pfcpSession->up_f_seid);
	app_end_time( &seid_del_time);
	
	app_start_time( &teid_del_time);
	ls__delobject_u32k( pfs->teid_table, pfcpSession->upf_teid);
	app_end_time( &teid_del_time);
	
	// pfcp_ip_flows_t * ip_flows = pfcpSession->ip_flows;
	// int release_ip_flows = 0;
	
	// if( pfcpSession->ue_ipv4_isset == 1 && ip_flows)
	// {
		// app_start_time( &ue_ipv4_del_time);
		
		// if( ip_flows->session_count == 1)
		// {
			// release_ip_flows = 1;
			// ls__delobject_u32k( pfs->ueipv4_table, 	pfcpSession->ue_ipv4);
		// }
		// else
		// {
			// pthread_mutex_lock( &ip_flows->session_count_lock);
			
			// int iy = 0;
			// while( iy < 5)
			// {
				// if(ip_flows->session[iy] == pfcpSession)
				// {
					// ip_flows->session[iy] = NULL;
					// ip_flows->session_count--;
					// break;
				// }
				// iy++;
			// }
			
			// pthread_mutex_unlock( &ip_flows->session_count_lock);
		// }
		
		// app_end_time( &ue_ipv4_del_time);
	// }
	// else
	// {
		// app_start_time( &ue_ipv4_del_time);
		// app_end_time( &ue_ipv4_del_time);
	// }
	
	// if( pfs->ueipv6_table && pfcpSession->ue_ipv6_isset == 1 && ip_flows)
	// {
		// app_start_time( &ue_ipv6_del_time);
		
		// //if( ip_flows->session_count == 1)
		// {
			// ls__delobject_u128k( pfs->ueipv6_table, pfcpSession->ue_ipv6);
		// }
		
		// app_end_time( &ue_ipv6_del_time);
	// }
	// else
	// {
		// app_start_time( &ue_ipv6_del_time);
		// app_end_time( &ue_ipv6_del_time);
	// }



	uint32_t ueip = pfcpSession->ue_ipv4;
	uint32_t ran_ip = 0;
	uint32_t ran_teid = 0;
	
	//ueip = pfcp_stack___get_ueip( pfcpSession);
	pfcp_stack___get_ranip( pfcpSession, &ran_ip, &ran_teid);
	
	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
		" deleting pfcp session  cp-seid=%u  upf-seid=%u  "
		" ueip=%u  ran_ip=%u  ran_teid=%u  as-del-time=%lu.%lu   seid-del-time=%lu.%lu  teid-del-time=%lu.%lu  ipv4-del-time=%lu.%lu  ipv6-del-time=%lu.%lu  %s|%s|%d", 
		pfcpSession->cp_f_seid, pfcpSession->up_f_seid, 
		ueip, ran_ip, ran_teid, 
		as_del_time.lapsed.tv_sec, as_del_time.lapsed.tv_usec,
		seid_del_time.lapsed.tv_sec, seid_del_time.lapsed.tv_usec,
		teid_del_time.lapsed.tv_sec, teid_del_time.lapsed.tv_usec,
		ue_ipv4_del_time.lapsed.tv_sec, ue_ipv4_del_time.lapsed.tv_usec,
		ue_ipv6_del_time.lapsed.tv_sec, ue_ipv6_del_time.lapsed.tv_usec,
		__FILE__, __FUNCTION__, __LINE__);

	// if( ip_flows && release_ip_flows == 1)
	// {
		// int iy = 0;
		// while( iy < 5)
		// {
			// ip_flows->session[iy] = 0;
			// iy++;
		// }
		
		// ip_flows->session_count = 0;

		// //pthread_mutex_destroy( &ip_flows->fLock);
		// //pthread_mutex_destroy( &ip_flows->session_count_lock);	
		
		// app_region__free( (uint8_t *)ip_flows);	
	// }
	
	pfcp_stack___release_session( pfcpSession);
	app_region__free( (uint8_t *)pfcpSession);	
}


void pfcp_stack___delete_session__fsm( uint8_t * data, int tIndex)
{
	pfcp_session_t * pfcpSession = (pfcp_session_t *)data;
	
	char sip[INET_ADDRSTRLEN];
	memset( sip, 0, INET_ADDRSTRLEN);
	
	app_ep__get_str_ipv4( pfcpSession->nodeIPv4, sip);

	
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "deleting idle session cp_node=%s  cp_f_seid=%lu  up_f_seid=%lu   %s|%s|%d", 
		sip, pfcpSession->cp_f_seid, pfcpSession->up_f_seid, __FILE__, __FUNCTION__, __LINE__);
	
	pfcp_stack___delete_session( pfcpSession);
	pthread_mutex_lock( &pfs->sess_delete_lock);
	pfs->sess_delete_total++;
	pthread_mutex_unlock( &pfs->sess_delete_lock);
}


//void app__as__remove( pfcp_session_t * session); 

//void pfcp_stack___handle_session_deletion_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
void pfcp_stack___handle_session_deletion_request__fsm( uint8_t * data, int tIndex)
{
	pfcp_msg_helper_t * msg_helper = (pfcp_msg_helper_t *)data;
	struct pfcp_node * node = msg_helper->node;
	app_ep_udp_message_t * msg = msg_helper->msg;
	
	app_region__free( (uint8_t *)msg_helper);
	
	//pfcp_stack___log_received_request( "PFCPSessionDeletionRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	pfcp_session_t * pfcpSession = NULL;

	if( sts == 0)
	{
		if( pfcp_msg.SEID > 0)
		{
			int integration_test = 0;
			
			if( pfcp_msg.SEID == 47 && integration_test == 1)
			{
				// pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, pfcp_msg.SEID);
				
				// if( pfcpSession)
				// {
					// pfcp_message_t pfcp_response;
					// pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_DELETION_RESPONSE, buffer, 1, pfcpSession->cp_f_seid);	
					// pfcp_session_deletion_response_t * response = &pfcp_response.u.session_deletion_response;


					// pfcp_stack___delete_session( pfcpSession);
					
					
					// __up_pfcp_set__u8( &response->cause, PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING);
					// pfcp_stack___send_response( &pfcp_response, msg);
				// }
				
				__up_pfcp_free__message( &pfcp_msg);
				app_ep__free_udp_message( msg);
				return;
			}
			
			
			
			
			//pfcp_session_modification_request_t * recv_msg = &pfcp_msg.u.session_modification_request;
			pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, pfcp_msg.SEID);
		
			if( pfcpSession)
			{
				pfcp_stack__perf_inc_sdr();
				#ifdef ENABLE_PROMETHEUS
					successful_task_request( "POST",4);
				#endif
				pfcp_message_t pfcp_response;
				pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_DELETION_RESPONSE, buffer, 1, pfcpSession->cp_f_seid);	
				pfcp_session_deletion_response_t * response = &pfcp_response.u.session_deletion_response;

				
				ls__delobject_u64k( node->cp_seid_table, pfcpSession->cp_f_seid);
				pfcp_stack___delete_session( pfcpSession);
				
				
				__up_pfcp_set__u8( &response->cause, PFCP_CAUSE_VALUE_REQUEST_ACCEPTED);
				pfcp_stack___send_response( &pfcp_response, msg);
				
				pfcp_stack__perf_inc_sda();
				#ifdef ENABLE_PROMETHEUS
					successful_task_responce("POST",4);
				#endif
			}
		}
	}
	
	//__up_pfcp_free__message( &pfcp_response);
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}

void pfcp_stack___handle_session_deletion_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSessionDeletionResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_session_deletion_response_t * recv_msg = &pfcp_msg.u.session_deletion_response;
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}

void pfcp_stack___handle_session_report_request__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_stack___log_received_request( "PFCPSessionReportRequest", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);


	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_session_report_request_t * recv_msg = &pfcp_msg.u.session_report_request;
		
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}

void pfcp_stack___handle_session_report_response__buffer( struct pfcp_node * node, app_ep_udp_message_t * msg, int tIndex)
{
	// pfcp_stack___log_received_request( "PFCPSessionReportResponse", msg);
	
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);

	// printf("Received session Report Response %d\n",__LINE__); 

	/*pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	
	if( sts == 0)
	{
		pfcp_session_report_response_t * recv_msg = &pfcp_msg.u.session_report_response;
		pfcp_stack__perf_inc_sua();
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);*/
	
	
	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__message( buffer[1], &pfcp_msg, msg);
	pfcp_session_t * pfcpSession = NULL;
	
	if( sts == 0)
	{
		if( pfcp_msg.SEID > 0)
		{
			pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, pfcp_msg.SEID);
			
			
			if( pfcpSession)
			{
				pfcpSession->modified_time = time( NULL);
				pfcpSession->idle_session_hb_count 	= 0;
			}
			
			pfcp_stack__perf_inc_sua();
			#ifdef ENABLE_PROMETHEUS
				successful_task_responce("POST",3);
			#endif
			// pfcp_session_report_response_t * recv_msg = &pfcp_msg.u.session_report_response;
			
			// if(recv_msg->cause.u8 == PFCP_CAUSE_VALUE_SESSION_CONTEXT_NOT_FOUND)
			// {
				
				
				// // if( pfcpSession)
				// // {
					// // #if AEROSPIKE
						// // app__as__remove( pfcpSession);
					// // #endif
					
					// // pfcp_message_t pfcp_response;
					// // pfcp_session_report_response_t * response = &pfcp_response.u.session_report_response;
					
					// // uint32_t ueip = 0;
					// // uint32_t ran_ip = 0;
					// // uint32_t ran_teid = 0;
					
					// // ueip = pfcpSession->ue_ipv4;
					// // pfcp_stack___get_ranip( pfcpSession, &ran_ip, &ran_teid);
		
					// // app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCPSessionReportResponce: deleting report pfcp session  cp-seid=%u  upf-seid=%u   ueip=%u  ran_ip=%u  ran_teid=%u    %s|%s|%d", pfcpSession->cp_f_seid, pfcpSession->up_f_seid, ueip, ran_ip, ran_teid, __FILE__, __FUNCTION__, __LINE__);
					
					// // app_rbnode__free_idp( pfs->pfcp_sessions_tree, pfcpSession->up_f_seid);
					// // pfcp_stack___release_session( pfcpSession);
					// // app_region__free( (uint8_t *)pfcpSession);
				
					// // pfcp_stack__perf_inc_sua();
				// // }
			// }
		}
	}
	
	__up_pfcp_free__message( &pfcp_msg);
	app_ep__free_udp_message( msg);
}

void pfcp_stack__buffer_handler( uint8_t * data, int tIndex)
{
	app_ep_udp_message_t * msg 	= (app_ep_udp_message_t *)data;
	uint16_t len 				= app_ep__get_len( msg);
	uint8_t * buffer 			= app_ep__get_buffer( msg);

	//printf("buffer=%p len=%d msgType=%u %s|%d\n", buffer, len, buffer[1], __FILE__, __LINE__);

	
	if( len >= 8)
	{
		uint8_t msgType = buffer[1];

		struct pfcp_node * node = NULL;
		
		if( PFCP_ASSOCIATION_SETUP_REQUEST == msgType) 
		{
			node = pfcp_stack___find_node( msg, 1);
			
			if(!node)
			{
				char ipa[100];
				app_ep__get_strip( msg, ipa);	
				char * mType = pfcp_stack__get_message_type( msgType);
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "%s: PFCP Node Allocation Failed for CP-IP:%s  %s|%s|%d", mType, ipa, __FILE__, __FUNCTION__, __LINE__);
				app_ep__free_udp_message( msg);
				return;
			}
		}
		else 
		{
			node = pfcp_stack___find_node( msg, 0);
		
			if(!node)
			{
				char ipa[100];
				app_ep__get_strip( msg, ipa);	
				char * mType = pfcp_stack__get_message_type( msgType);
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Received Message with Message Type: %s: and PFCP Node Not Found for CP-IP:%s  %s|%s|%d", mType, ipa, __FILE__, __FUNCTION__, __LINE__);
				app_ep__free_udp_message( msg);
				return;
			}
		}
		

		if( pfs->MaxTPS != 1)
		{
			if( pfcp__mesg_counter > pfs->MaxTPS)
			{
				pthread_mutex_lock( &pfs->cong_dropped_lock);
				pfs->cong_dropped_total++;
				pthread_mutex_unlock( &pfs->cong_dropped_lock);
			
				app_ep__free_udp_message( msg);
				return;
			}
			
			pthread_mutex_lock( &pfs->maxtps_lock);
			pfcp__mesg_counter++;
			pthread_mutex_unlock( &pfs->maxtps_lock);
		}

		
		switch( msgType)
		{
			case PFCP_HEARTBEAT_REQUEST:
				pfcp_stack___handle_heartbeat_request__buffer( node, msg, tIndex);
				break;
			case PFCP_HEARTBEAT_RESPONSE:
				//printf("HB_RES   buffer=%p len=%d msgType=%u %s|%d\n", buffer, len, buffer[1], __FILE__, __LINE__);
				pfcp_stack___handle_heartbeat_response__buffer( node, msg, tIndex);
				break;
			case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
				pfcp_stack___handle_pfd_management_request__buffer( node, msg, tIndex);
				break;
			case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
				pfcp_stack___handle_pfd_management_response__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_SETUP_REQUEST:
				//printf("ASS   buffer=%p len=%d msgType=%u %s|%d\n", buffer, len, buffer[1], __FILE__, __LINE__);
				pfcp_stack___handle_association_setup_request__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_SETUP_RESPONSE:
				pfcp_stack___handle_association_setup_response__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_UPDATE_REQUEST:
				pfcp_stack___handle_association_update_request__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_UPDATE_RESPONSE:
				pfcp_stack___handle_association_update_response__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_RELEASE_REQUEST:
				pfcp_stack___handle_association_release_request__buffer( node, msg, tIndex);
				break;
			case PFCP_ASSOCIATION_RELEASE_RESPONSE:
				pfcp_stack___handle_association_release_response__buffer( node, msg, tIndex);
				break;
			case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
				pfcp_stack___handle_version_not_supported_response__buffer( node, msg, tIndex);
				break;
			case PFCP_NODE_REPORT_REQUEST:
				pfcp_stack___handle_node_report_request__buffer( node, msg, tIndex);
				break;
			case PFCP_NODE_REPORT_RESPONSE:
				pfcp_stack___handle_node_report_response__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_SET_DELETION_REQUEST:
				pfcp_stack___handle_set_deletion_request__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_SET_DELETION_RESPONSE:
				pfcp_stack___handle_set_deletion_response__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_ESTABLISHMENT_REQUEST:
				pfcp_stack___handle_session_establishment_request__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
				pfcp_stack___handle_session_establishment_response__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_MODIFICATION_REQUEST:
				//pfcp_stack___handle_session_modification_request__buffer( node, msg, tIndex);
				//app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Received session_modification request %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
				pfcp_stack___handle_session_modification_or_delete_request__buffer( node, msg, tIndex, 2);
				break;
			case PFCP_SESSION_MODIFICATION_RESPONSE:
				pfcp_stack___handle_session_modification_response__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_DELETION_REQUEST:
				//pfcp_stack___handle_session_deletion_request__buffer( node, msg, tIndex);
				pfcp_stack___handle_session_modification_or_delete_request__buffer( node, msg, tIndex, 3);
				break;
			case PFCP_SESSION_DELETION_RESPONSE:
				pfcp_stack___handle_session_deletion_response__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_REPORT_REQUEST:
				pfcp_stack___handle_session_report_request__buffer( node, msg, tIndex);
				break;
			case PFCP_SESSION_REPORT_RESPONSE:
				pfcp_stack___handle_session_report_response__buffer( node, msg, tIndex);
				break;
			default:
				app_ep__free_udp_message( msg);
				break;
		}
	}
	else
	{
		app_ep__free_udp_message( msg);
	}
}

pfcp_urr_t * pfcp_stack___get_session_urr( pfcp_session_t * pfcpSession, uint32_t urr_id, int * added)
{
	pthread_mutex_lock( &pfcpSession->urr.Lock);
	int bFound = 0;
	*added = 0;
	
	pfcp_urr_t * sess_urr = pfcpSession->urr.head;
	
	while( sess_urr)
	{
		if( sess_urr->urr_id == urr_id)
		{
			bFound = 1;
			break;
		}
		sess_urr = sess_urr->Next;
	}	

	if( bFound == 1)
	{
		pthread_mutex_unlock( &pfcpSession->urr.Lock);
		return sess_urr;
	}
	
	sess_urr = (pfcp_urr_t *) app_region__allocate_fr( pfs->pfcp_mregion, sizeof(pfcp_urr_t) + 1);
	
	if( sess_urr)
	{
		memset( sess_urr, 0, sizeof(sizeof(pfcp_urr_t) + 1));
		sess_urr->urr_id = urr_id;		

		if(!pfcpSession->urr.head)
		{
			pfcpSession->urr.head = pfcpSession->urr.current = sess_urr;
		}
		else
		{
			pfcpSession->urr.current->Next = sess_urr;
			pfcpSession->urr.current = sess_urr;
		}
		pfcpSession->urr.count++;
		sess_urr->session = pfcpSession;		
	}
	
	*added = 1;
	pthread_mutex_unlock( &pfcpSession->urr.Lock);
	
	return sess_urr;
}


void pfcp_stack___record_usage( uint64_t up_f_seid, uint32_t rgid, uint64_t uplink, uint64_t downlink)
{
	pfcp_stack__perf_inc_pkt();

	pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, up_f_seid);
	
	if( pfcpSession)
	{
		int added = 0;
		pfcp_urr_t * sess_urr = pfcp_stack___get_session_urr( pfcpSession, rgid, &added);
		int send_request = 0;

		sess_urr->used_volume.uplink += uplink;
		sess_urr->used_volume.downlink += downlink;

		double used_percent = 0;

		long double total = sess_urr->granted_volume.total;
		long double used = (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink);
		used_percent = ( (used / total) * 100);

		if( sess_urr->granted_volume.total > 0 && (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink) > 0)
		{
			if( sess_urr->granted_volume.total > (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink))
			{
				used_percent = ( (used / total) * 100);
			}
		}

		if( added == 1)
		{
			send_request = 1;
			
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"sending usage report: added rg %u  newly sending request for quota cp_f_seid=%lu  %s|%s|%d", 
				rgid, pfcpSession->cp_f_seid, __FILE__, __FUNCTION__, __LINE__);			
		}
		else
		{
			
			if( sess_urr->granted_volume.total > 0 && (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink) > 0)
			{
				if( sess_urr->granted_volume.total > (sess_urr->used_volume.uplink + sess_urr->used_volume.downlink))
				{
					// printf( "used_percent=%f used=%Lf granted=%Lf used/granted=%Lf %s|%d\n", 
						// used_percent, used, total, 
						// (used / total) * 100,
						// __FILE__, __LINE__);
					
					if( used_percent >= 75)
					{
						send_request = 1;
						
						app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
							"sending usage report: threshold triggered used-percent=%f for cp_f_seid=%lu rgid=%u %s|%s|%d", 
							used_percent, pfcpSession->cp_f_seid, rgid, __FILE__, __FUNCTION__, __LINE__);
					}
				}
				else
				{
					send_request = 1;
					
					app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
						"sending usage report: used volume is greater than granted cp_f_seid=%lu rgid=%u  %s|%s|%d", 
						pfcpSession->cp_f_seid, rgid, __FILE__, __FUNCTION__, __LINE__);
				}
			}
			else
			{
				send_request = 1;

				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
						"sending usage report: granted and used are at 0  cp_f_seid=%lu rgid=%u  %s|%s|%d", 
						pfcpSession->cp_f_seid, rgid, __FILE__, __FUNCTION__, __LINE__);
			}
		}

		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"quota: rgid=%u cp_f_seid=%lu uplink=%lu downling=%lu total-granted=%lu total-used=%lu percent=%.2f send_req=%d  %s|%s|%d", 
					rgid, pfcpSession->cp_f_seid, uplink, downlink, sess_urr->granted_volume.total, 
					(sess_urr->used_volume.uplink + sess_urr->used_volume.downlink), used_percent, send_request,
				__FILE__, __FUNCTION__, __LINE__);
		
		
		if( send_request == 1)
		{
			uint32_t seqno = pfcp_stack__get_next_sequence( pfcpSession->node);
			
			char urseqn[4];
			app_ep__encode__u32toc( urseqn, pfcpSession->urr_rep_seq_no);
			
			pfcp_message_t msg;
			__up_pfcp_set__header( &msg, PFCP_SESSION_REPORT_REQUEST, 1, pfcpSession->cp_f_seid, seqno);
			pfcp_session_report_request_t * imsg = &msg.u.session_report_request;	

			__up_pfcp_set__u8( &imsg->report_type, 2);
			
			imsg->usage_report.presence = 1;
			__up_pfcp_set__u32( &imsg->usage_report.urr_id, rgid);
			__up_pfcp_set__octet( pfs->pfcp_mregion, &imsg->usage_report.ur_seqn, urseqn, 4);
		
			
			char volume_measurement[26];
			memset( volume_measurement, 0, sizeof(volume_measurement));
			volume_measurement[0] = 7;
			app_ep__encode__u64toc( &volume_measurement[ 1], uplink + downlink);
			app_ep__encode__u64toc( &volume_measurement[ 9], uplink);
			app_ep__encode__u64toc( &volume_measurement[17], downlink);	
			__up_pfcp_set__octet( pfs->pfcp_mregion, &imsg->usage_report.volume_measurement, volume_measurement, 25);
			/**/
		
			pthread_mutex_lock( &pfcpSession->UrrSeqLock);
			pfcpSession->urr_rep_seq_no++;
			pthread_mutex_unlock( &pfcpSession->UrrSeqLock);
			
			pfcp_stack___send_request( &msg, pfcpSession->node);

			//printf("requesting quota for cp_f_seid=%lu with seq-no=%u  %s|%d\n", pfcpSession->cp_f_seid, seqno, __FILE__, __LINE__);
			
			pfcp_stack__perf_inc_sur();
			
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_DEBUG, 
				"requesting quota for cp_f_seid=%lu with seq-no=%u  %s|%s|%d", pfcpSession->cp_f_seid, seqno, __FILE__, __FUNCTION__, __LINE__);
			
		}

	}
	else
	{
		app_logger__log( pfs->pfcpLogger, pfs->session_usage_cat_log, APP_LOG__LEVEL_DEBUG, "session not found with up_f_seid=%lu  %s|%s|%d", up_f_seid, __FILE__, __FUNCTION__, __LINE__);
	}
}




void pfcp_stack___setpool( app_rbtree_t * pf_tree, app_data_pool_t * pool, app_data_pool_t * ippool, app_data_pool_t * flowPool)
{
	pfs->pfcp_sessions_tree 	= pf_tree;
	pfs->pfcp_session_pool 		= pool;
	pfs->pfcp_session_ip_pool	= ippool;
	pfs->flowPool 				= flowPool;
}

void pfcp_stack___setippool( app_data_pool_t * ipv4pool, app_data_pool_t * ipv6pool, app_data_pool_t * portPool)
{
	pfs->ippool_ipv4 		= ipv4pool;
	pfs->ippool_ipv6		= ipv6pool;
	pfs->ippool_port 		= portPool;	
}


void pfcp_stack___delipflows()
{
	uint64_t TotalCount = 0;
	uint64_t Available 	= 0;
	
	app_region__get_counts( pfs->ippool_ipv4, &TotalCount, &Available);
	
	//printf("%lu %lu %lu %d %s\n", Available, TotalCount-Available, TotalCount, __LINE__, __FILE__);
	
	int t = 0;
	int alloc = 0;
	ip4_flow_t * flow4 = NULL;
	ip6_flow_t * flow6 = NULL;
	
	if( Available < TotalCount)
	{
		for( t = 0; t < TotalCount; t++)
		{
			alloc = app_region__is_item_allocated_at_index( pfs->ippool_ipv4, t);
		
		
			if( alloc > 0)
			{
				flow4 = (ip4_flow_t*)app_region__get_pointer_at_index( pfs->ippool_ipv4, t);
			
				if( flow4)
				{
					if( flow4->session)
					{
						if( flow4->session->version != flow4->version)
						{
							dpe_del_ipv4flow( flow4->ueip, flow4->dstip, flow4->srcport, flow4->dstport, flow4->protocol);
							app_region__free( (uint8_t *)flow4);
						}
						else
						{
							if(  (flow4->lastused + 120) < time(NULL))
							{
								dpe_del_ipv4flow( flow4->ueip, flow4->dstip, flow4->srcport, flow4->dstport, flow4->protocol);
								app_region__free( (uint8_t *)flow4);								
							}
						}
					}
					else
					{
						app_region__free( (uint8_t *)flow4);
					}
				}
			}
		}
	}

	TotalCount = 0;
	Available 	= 0;
	
	app_region__get_counts( pfs->ippool_ipv6, &TotalCount, &Available);
	
	if( Available < TotalCount)
	{
		for( t = 0; t < TotalCount; t++)
		{
			alloc = app_region__is_item_allocated_at_index( pfs->ippool_ipv6, t);
		
		
			if( alloc > 0)
			{
				flow6 = (ip6_flow_t*)app_region__get_pointer_at_index( pfs->ippool_ipv6, t);
			
				if( flow6)
				{
					if( flow6->session)
					{
						if( flow6->session->version != flow6->version)
						{
							dpe_del_ipv6flow( flow6->ueip, flow6->dstip, flow6->srcport, flow6->dstport, flow6->protocol);
							app_region__free( (uint8_t *)flow6);
						}
						else
						{
							if(  (flow6->lastused + 120) < time(NULL))
							{
								dpe_del_ipv6flow( flow6->ueip, flow6->dstip, flow6->srcport, flow6->dstport, flow6->protocol);
								app_region__free( (uint8_t *)flow6);								
							}
						}
					}
					else
					{
						app_region__free( (uint8_t *)flow6);
					}
				}
			}
		}
	}
	
}


void pfcp_stack__set_applogger( app_logger_t * al)
{
	pfs->appLogger = al;
}


void pfcp_stack__set_pfcplogger( app_logger_t * pl)
{
	pfs->pfcpLogger = pl;
	pfs->session_parameter_cat_log 	= app_logger__set_category( pfs->pfcpLogger, LOG_SESS_PARAM_CAT_LOG, 1, "PARAM", strlen("PARAM"));
	pfs->session_usage_cat_log 		= app_logger__set_category( pfs->pfcpLogger, LOG_SESS_USAGE_CAT_LOG, 1, "USAGE", strlen("USAGE"));
}

void pfcp_stack__set_devicelogger( app_logger_t * dl)
{
	pfs->deviceLogger = dl;
}


void pfcp_stack__set_pfcp_ipv4server( app_ep_udp_server_t * udp_ipv4server)
{
	pfs->udp_ipv4server = udp_ipv4server;
}

void pfcp_stack__set_pfcp_ipv6server( app_ep_udp_server_t * udp_ipv6server)
{
	pfs->udp_ipv6server = udp_ipv6server;
}

void pfcp_stack___set_heartbeat_seconds( int HeartbeatSeconds)
{
	if( HeartbeatSeconds < 5)
	{
		pfs->HeartbeatSeconds = 5;
	}
	else
	{
		pfs->HeartbeatSeconds = HeartbeatSeconds;
	}
}

void cp__tdd_set__session_urrs( pfcp_session_t * pfcp_session, uint32_t urrId, uint64_t volume)
{
	// printf( " cp_f_seid=%lu up_f_seid=%lu  urrId=%u volume=%lu  %s|%s|%d\n",  pfcp_session->cp_f_seid, pfcp_session->up_f_seid, urrId, volume, __FILE__, __FUNCTION__, __LINE__);
	pfcp_stack__record_usage( (void *)pfcp_session, urrId, 0, volume);
}


void cp__tdd_trigger__session_usage( pfcp_session_t * pfcp_session)
{
	
}


int cp__get_on_going_sessions( pfcp_session_t ** tdd_pfcp_sessions, int * tdd_pfcp_sessions_count)
{
	int iTotal 		= app_rbnode__get_total( 		pfs->pfcp_sessions_tree);
	int iAvailable 	= app_rbnode__get_available( 	pfs->pfcp_sessions_tree);
				
	//printf( "Session-Count=%u  iAvailable=%u  iTotal=%u  %s|%s|%d\n", iTotal - iAvailable, iAvailable, iTotal, __FILE__, __FUNCTION__, __LINE__);
	int ltdd_pfcp_sessions_count = 0;
	
	if( iAvailable < iTotal)
	{
		//printf( "iAvailable < iTotal=%d\n", iAvailable < iTotal);
		
		int i = 0; 
		
		for( i = 0 ; i < iTotal; i++)
		{
			pfcp_session_t * sess = (pfcp_session_t*)app_rbnode__get_obj_at_index( pfs->pfcp_sessions_tree, i);
			
			if( sess)
			{
				tdd_pfcp_sessions[ltdd_pfcp_sessions_count] = sess;
				ltdd_pfcp_sessions_count++;
				
				//printf( "session=%p Next=%p\n", tdd_pfcp_sessions[ltdd_pfcp_sessions_count]);

				if( ltdd_pfcp_sessions_count == 99)
				{
					break;
				}
			}
		}
	}
	
	*tdd_pfcp_sessions_count = ltdd_pfcp_sessions_count;


	return ltdd_pfcp_sessions_count;
}


void cp__set_flowIndex( uint8_t * flowptr, int flowindex)
{
	if(flowptr)
	{
		((ip4_flow_t*)flowptr)->flowIndex = flowindex;
	}
}

void cp__set_rgid( uint8_t * flowptr, int rgid)
{
	if(flowptr)
	{
		((ip4_flow_t*)flowptr)->rgid = rgid;
	}
}


void cp__create_session( uint32_t cp_f_seid, uint32_t ueip, uint32_t teid, uint32_t ranip, uint32_t ranteid)
{
	pfcp_session_t * pfcpSession = (pfcp_session_t *)app_region__allocate_fd( pfs->pfcp_session_pool);

	if(pfcpSession)
	{
		if( pfcpSession->initalized == 0) 
		{
			pfcp_stack___initalize_session( pfcpSession);
		} 
		else 
		{
			pfcp_stack___reinitalize_session( pfcpSession);
		}		
		
		pfcpSession->up_f_seid = app_rbnode__get_next_idp( pfs->pfcp_sessions_tree, (uint8_t *)pfcpSession);
		pfcpSession->cp_f_seid = cp_f_seid;
		pfcpSession->ue_ipv4   = ueip;
		pfcpSession->upf_teid  = teid;
		pfcpSession->ran_ipv4  = ranip;
		pfcpSession->ran_teid  = ranteid;

		//ls__setobject_u32k_lm( pfs->ueipv4_table, 	ueip, 					(uint8_t *)pfcpSession);
		ls__setobject_u32k_lm( pfs->teid_table, 	pfcpSession->upf_teid, 	(uint8_t *)pfcpSession);
		
		
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "added session context with ueip=%u  %u.%u.%u.%u   and teid=%u   %s|%s|%d", 
			ueip, 
			ueip & 0xFF, (ueip >> 8) & 0xFF, (ueip >> 16) & 0xFF, (ueip >> 24) & 0xFF,
			pfcpSession->upf_teid, __FILE__, __FUNCTION__, __LINE__);
	}
}


void pfcp_stack___set_pfcp_maxtps( int MaxTPS)
{
	if(pfs)
	{
		pfs->MaxTPS = MaxTPS;
	}
}


uint64_t pfcp_monitor_ticks = 0;
uint64_t pfcp_monitor_run_del_all = 0;
uint64_t pfcp_monitor_run_del_by_ip = 0;
uint64_t pfcp_monitor_run_del_by_up_seid = 0;
char cp_ip[48];
char up_seid[48];

void * pfcp_session__monitor( void * args)
{
	int i = 0;

	uint64_t pfcp_sess__TotalCount 		= 0;
	uint64_t pfcp_sess__Available      	= 0;
	pfcp_session_t * pfcpSession	= NULL;

	
	uint32_t checkEveryMin = 2;
	uint32_t sessionIdleMin = 5;
	int noOfPending_HB = 4;

	uint32_t nodeIP = 0;
	uint64_t up_f_seid = 0;
	
	pfcp_session_t * pfcp_sessions_for_del[50];
	int pfcp_sessions_for_del_counter = 0;
	uint64_t current_time;
	
	uint32_t batchsize = 50;
	uint32_t batchcounter = 0;
	
	
	while(1)
	{
		if( pfcp_monitor_ticks > (checkEveryMin * 60))
		{
			while(1)
			{
				current_time = time( NULL);
				batchcounter = 0;
				
				pfcp_sessions_for_del_counter = 0;
				pfcp_sess__TotalCount = 0;
				pfcp_sess__Available = 0;
				
				app_region__get_counts( pfs->pfcp_session_pool, &pfcp_sess__TotalCount, &pfcp_sess__Available);
				
				//printf( "pfcp_sess__available=%lu pfcp_sess__totalcount=%lu  %d\n", pfcp_sess__Available, pfcp_sess__TotalCount, __LINE__);

				
				if( pfcp_sess__Available < pfcp_sess__TotalCount)
				{
					for( i = 0; i < pfcp_sess__TotalCount; i++)
					{
						if( app_region__is_item_allocated_at_index( pfs->pfcp_session_pool, i) == 1)
						{
							pfcpSession = (pfcp_session_t*)app_region__get_pointer_at_index( pfs->pfcp_session_pool, i);
							
							if( pfcpSession)
							{
								// printf( "MONT pfcpSession=%p  cp_f_seid=%lu  i=%d  s_hb_count=%u  m_time=%lu  current_time=%ld  %ld  %d\n", 
									// pfcpSession, pfcpSession->cp_f_seid, i, pfcpSession->idle_session_hb_count, pfcpSession->modified_time, 
										// current_time, current_time - pfcpSession->modified_time, __LINE__);
								
								if( batchcounter >= batchsize)
								{
									batchcounter = 0;
									usleep( 999999);
								}
								
								if( pfs->enable_upir == 1)
								{
									if( pfcpSession->idle_session_hb_count < noOfPending_HB)
									{
										if( (pfcpSession->modified_time + (sessionIdleMin * 60)) < current_time)
										{
											pfcp_stack___send_session_report_request__upir( pfcpSession);
											//printf( "sending pfcp upir for session=%p  %d\n", pfcpSession, __LINE__);
											batchcounter++;
										}
									}
									else
									{
										//printf( "session elgible for delete=%p  %d\n", pfcpSession, __LINE__);
										
										pfcp_sessions_for_del[pfcp_sessions_for_del_counter] = pfcpSession;
										pfcp_sessions_for_del_counter++;
									}
								}
								
							}
						}
						
						if( pfcp_sessions_for_del_counter >= 49)
							break;
					}
				}
				
				for( i = 0; i < pfcp_sessions_for_del_counter; i++)
				{
					app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)pfcp_sessions_for_del[i], 
						pfcp_sessions_for_del[i]->cp_f_seid, pfcp_stack___delete_session__fsm);
				}
				
				if( pfcp_sessions_for_del_counter > 0)
				{
					// char ctimestamp[30];
					// memset( ctimestamp, 0, sizeof(ctimestamp));
					// app_makeTimeStamp2( ctimestamp);
		
					// printf( "count=%d pause .. %s\n", pfcp_sessions_for_del_counter, ctimestamp);
					usleep( 999999);
				}
				
				if( pfcp_sessions_for_del_counter == 0 || pfcp_sessions_for_del_counter < 49)
					break;
			}
			
			
			pfcp_monitor_ticks = 0;
		}
		
		
		if( pfcp_monitor_run_del_all == 1)
		{
			while(1)
			{
				//
				pfcp_sessions_for_del_counter = 0;
				pfcp_sess__TotalCount = 0;
				pfcp_sess__Available = 0;
				app_region__get_counts( pfs->pfcp_session_pool, &pfcp_sess__TotalCount, &pfcp_sess__Available);
				
				if( pfcp_sess__Available < pfcp_sess__TotalCount)
				{
					for( i = 0; i < pfcp_sess__TotalCount; i++)
					{
						if( app_region__is_item_allocated_at_index( pfs->pfcp_session_pool, i) == 1)
						{
							pfcpSession = (pfcp_session_t*)app_region__get_pointer_at_index( pfs->pfcp_session_pool, i);
							
							if( pfcpSession)
							{
								pfcp_sessions_for_del[pfcp_sessions_for_del_counter] = pfcpSession;
								pfcp_sessions_for_del_counter++;
							}
						}
						
						if( pfcp_sessions_for_del_counter >= 49)
							break;
					}
				}
				
				for( i = 0; i < pfcp_sessions_for_del_counter; i++)
				{
					app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)pfcp_sessions_for_del[i], 
						pfcp_sessions_for_del[i]->cp_f_seid, pfcp_stack___delete_session__fsm);
				}
				
				if( pfcp_sessions_for_del_counter > 0)
				{
					usleep( 999999);
				}
				
				if( pfcp_sessions_for_del_counter == 0 || pfcp_sessions_for_del_counter < 49)
					break;
				//
			}
			
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd - Deleted ALL Sessions   %s|%s|%d", 
				__FILE__, __FUNCTION__, __LINE__);			
			
			pfcp_monitor_run_del_all = 0;
		}

		if( pfcp_monitor_run_del_by_ip == 1 && strlen(cp_ip) > 0)
		{
			nodeIP = app_ep__u32ip( cp_ip);
			
			if( nodeIP > 0)
			{
				while(1)
				{
					//
					pfcp_sessions_for_del_counter = 0;
					pfcp_sess__TotalCount = 0;
					pfcp_sess__Available = 0;
					app_region__get_counts( pfs->pfcp_session_pool, &pfcp_sess__TotalCount, &pfcp_sess__Available);
					
					if( pfcp_sess__Available < pfcp_sess__TotalCount)
					{
						for( i = 0; i < pfcp_sess__TotalCount; i++)
						{
							if( app_region__is_item_allocated_at_index( pfs->pfcp_session_pool, i) == 1)
							{
								pfcpSession = (pfcp_session_t*)app_region__get_pointer_at_index( pfs->pfcp_session_pool, i);
								
								if( pfcpSession)
								{
									if( pfcpSession->nodeIPv4 == nodeIP)
									{
										pfcp_sessions_for_del[pfcp_sessions_for_del_counter] = pfcpSession;
										pfcp_sessions_for_del_counter++;
									}
								}
							}
							
							if( pfcp_sessions_for_del_counter >= 49)
								break;
						}
					}
					
					for( i = 0; i < pfcp_sessions_for_del_counter; i++)
					{
						app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)pfcp_sessions_for_del[i], 
							pfcp_sessions_for_del[i]->cp_f_seid, pfcp_stack___delete_session__fsm);
					}
					
					if( pfcp_sessions_for_del_counter > 0)
					{
						usleep( 999999);
					}
					
					if( pfcp_sessions_for_del_counter == 0 || pfcp_sessions_for_del_counter < 49)
						break;
					//
				}
			}

			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd - Deleted Sessions By cp_ip = %lu | %s   %s|%s|%d", 
				nodeIP, cp_ip, __FILE__, __FUNCTION__, __LINE__);
					
			pfcp_monitor_run_del_by_ip = 0;
		}

		if( pfcp_monitor_run_del_by_up_seid == 1 && strlen(up_seid) > 0)
		{
			up_f_seid = atol(up_seid);
			
			if( up_f_seid > 0)
			{
				pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, up_f_seid);
				
				if( pfcpSession)
				{
					app_fsmqueue__idenquee( pfs->fsmQ, (uint8_t *)pfcpSession, pfcpSession->cp_f_seid, pfcp_stack___delete_session__fsm);
				}

				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd - Deleted Session By up_f_seid=%lu  Session-Found=%s   %s|%s|%d", 
					up_f_seid, pfcpSession ? "yes" : "no", __FILE__, __FUNCTION__, __LINE__);
			}
			
			pfcp_monitor_run_del_by_up_seid = 0;
		}
		
		
		pfcp_monitor_ticks++;
		usleep(999999);
	}
	return NULL;
}


uint32_t slt_curent_ueip = 0;
uint32_t slt_total_ueip = 0;
pfcp_session_t ** slt_pfcpSession_List	= NULL;
uint32_t slt_init = 0;

uint32_t cp__get_next_ueip( uint32_t * upf_ip, uint32_t * upf_teid, uint32_t * ue_ipv4)
{
	uint64_t pfcp_sess__TotalCount 			= 0;
	uint64_t pfcp_sess__Available      		= 0;
	uint64_t pfcp_sess__created      		= 0;
	pfcp_session_t * pfcpSession			= NULL;
	int i = 0;
	int j = 0;

	if(!slt_pfcpSession_List && slt_init == 0)
	{
		slt_init = 1;
		app_region__get_counts( pfs->pfcp_session_pool, &pfcp_sess__TotalCount, &pfcp_sess__Available);
		pfcp_sess__created = pfcp_sess__TotalCount - pfcp_sess__Available;
		
		if( pfcp_sess__created > 0)
		{
			slt_pfcpSession_List = (pfcp_session_t**)malloc(sizeof(pfcp_session_t*) * pfcp_sess__created);
			
			if( pfcp_sess__Available < pfcp_sess__TotalCount)
			{
				for( i = 0; i < pfcp_sess__TotalCount; i++)
				{
					if( app_region__is_item_allocated_at_index( pfs->pfcp_session_pool, i) == 1)
					{
						pfcpSession = (pfcp_session_t*)app_region__get_pointer_at_index( pfs->pfcp_session_pool, i);
						
						slt_pfcpSession_List[j] = pfcpSession;
						j++;
						slt_total_ueip++;
					}
				}
			}
		}
	}
	
	if( slt_total_ueip > 0)
	{
		if( slt_curent_ueip >= slt_total_ueip)
		{
			slt_curent_ueip = 0;
		}
		
		pfcpSession = slt_pfcpSession_List[slt_curent_ueip];
		slt_curent_ueip++;
		

		*ue_ipv4 	= pfcpSession->ue_ipv4;
		*upf_teid	= pfcpSession->upf_teid;
		*upf_ip		= pfcpSession->upf_ip;
		 	
		return 1;
	}
	
	return 0;
}




void pfcp_stack___delete_all_cmd()
{
	pfcp_monitor_run_del_all = 1;
}


void pfcp_stack___delete_all_by_cp_ip_cmd( char * cpip)
{
	strcpy( cp_ip, cpip);
	pfcp_monitor_run_del_by_ip = 1;
}


void pfcp_stack___delete_all_by_up_seid_cmd( char * upseid)
{
	strcpy( up_seid, upseid);
	pfcp_monitor_run_del_by_up_seid = 1;
}


void pfcp_stack___log_sess_info_by_up_seid_cmd( char * upseid)
{
	if( strlen(upseid) > 0)
	{
		uint64_t up_f_seid = atol(upseid);
		
		if( up_f_seid > 0)
		{
			pfcp_session_t * pfcpSession = (pfcp_session_t *)app_rbnode__find_idp( pfs->pfcp_sessions_tree, up_f_seid);
			
			if( pfcpSession)
			{
				uint32_t ueip = 0;
				uint32_t ran_ip = 0;
				uint32_t ran_teid = 0;
				uint8_t * ueipv6 = NULL;
				uint8_t  ue_ipv4_isset;	
				uint8_t  ue_ipv6_isset;	
	
				ueip = pfcp_stack___get_ueip( pfcpSession, &ueipv6, &ue_ipv6_isset, &ue_ipv4_isset);

				uint8_t 	qfifound = 0;
				uint8_t 	qfi = 0;
				uint8_t 	gate_status = 0;
				uint64_t 	ul_mbr = 0;
				uint64_t 	dl_mbr = 0;
				pfcp_stack__get_qerinfo( (void *)pfcpSession, &qfifound, &qfi, &gate_status, &ul_mbr, &dl_mbr);
	
	
				char ueip_str[INET_ADDRSTRLEN];
				memset( ueip_str, 0, INET_ADDRSTRLEN);
				
				char upfip_str[INET_ADDRSTRLEN];
				memset( upfip_str, 0, INET_ADDRSTRLEN);	

				char ranip_str[INET_ADDRSTRLEN];
				memset( ranip_str, 0, INET_ADDRSTRLEN);	
				
				// char ohip_str[INET_ADDRSTRLEN];
				// memset( ohip_str, 0, INET_ADDRSTRLEN);	

				char cp_ip[INET_ADDRSTRLEN];
				memset( cp_ip, 0, INET_ADDRSTRLEN);
				
				app_ep__get_str_ipv4( htonl(ueip), ueip_str);
				app_ep__get_str_ipv4( htonl(ran_ip), ranip_str);
				//app_ep__get_str_ipv4( htonl(oh_ipv4), ohip_str);
				app_ep__get_str_ipv4( htonl(pfcpSession->upf_ip), upfip_str);
				app_ep__get_str_ipv4( (pfcpSession->nodeIPv4), cp_ip);
				
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Session=%p  with UPF-SEID=%lu CP-SEID=%s:%lu  ue-ip=%u|%s  ue-ip-v6[%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X]  ran_ip=%u|%s  ran_teid=%u  upf-access-ip=%u|%s  upf-teid=%u   qer-found=%u  qfi=%u  gate_status=%u   ul_mbr=%lu   dl_mbr=%lu    pdnType=%u    %s|%s|%d", 
						pfcpSession, pfcpSession->up_f_seid, cp_ip, pfcpSession->cp_f_seid, ueip, ueip_str, 
						pfcpSession->ue_ipv6[0] & 0xFF, pfcpSession->ue_ipv6[1] & 0xFF, pfcpSession->ue_ipv6[2] & 0xFF, pfcpSession->ue_ipv6[3] & 0xFF, pfcpSession->ue_ipv6[4] & 0xFF, pfcpSession->ue_ipv6[5] & 0xFF, pfcpSession->ue_ipv6[6] & 0xFF, pfcpSession->ue_ipv6[7] & 0xFF,
						pfcpSession->ue_ipv6[8] & 0xFF, pfcpSession->ue_ipv6[9] & 0xFF, pfcpSession->ue_ipv6[10] & 0xFF, pfcpSession->ue_ipv6[11] & 0xFF, pfcpSession->ue_ipv6[12] & 0xFF, pfcpSession->ue_ipv6[13] & 0xFF, pfcpSession->ue_ipv6[14] & 0xFF, pfcpSession->ue_ipv6[15] & 0xFF,
						ran_ip, ranip_str, ran_teid, pfcpSession->upf_ip, upfip_str, pfcpSession->upf_teid, qfifound, qfi, gate_status, ul_mbr, dl_mbr, pfcpSession->pdnType, 
					__FILE__, __FUNCTION__, __LINE__);
			}
			else
			{
				app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd: Session NOT Found with UP_SEID=%lu   %s|%s|%d", up_f_seid, __FILE__, __FUNCTION__, __LINE__);
			}
		}
		else
		{
			app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd: Cannot LOG Session , Invalid UP_SEID   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		}
	}
	else
	{
		app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd: Cannot LOG Session , UP_SEID IS EMPTY   %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
	}
}


void pfcp_stack___exec_cmd( uint32_t cmdid, uint32_t operation, uint32_t interface, char * command)
{
	app_logger__log( pfs->pfcpLogger, NULL, APP_LOG__LEVEL_CRITICAL, "PFCP Cmd - Received  CmdId=%u  Operation=%u  Command=%s   %s|%s|%d", 
		cmdid, operation, command, __FILE__, __FUNCTION__, __LINE__);
	
	if( cmdid == 3)
	{
		switch( operation)
		{
			case 1:  //1 del-all
				pfcp_stack___delete_all_cmd();
				break;
			case 2:  //2 del-by-ip
				pfcp_stack___delete_all_by_cp_ip_cmd( command);
				break;
			case 3:  //3 del-by-up-seid
				pfcp_stack___delete_all_by_up_seid_cmd( command);
				break;
			case 4:  //4 log-by-up-seid
				pfcp_stack___log_sess_info_by_up_seid_cmd( command);
				break;				
			default:
				break;
		}
	}
}


void * pfcp_session__stime( void * args)
{
	while( 1)
	{
		usleep( 999999);
		
		if( 65535 == pfs->stime)
		{
			pfs->stime = 1;
		}
		else
		{
			pfs->stime++;
		}
	}
}



int dpe__get_dpicorecount();




void pfcp_stack___exit()
{
	#if DPI_PACE2
		if( dpi)
		{
			dpi_destroy_instance( &dpi, g_dpi_worker_count);
		}
	#endif
}


int cp__packet_dectection( uint8_t coreindex, const void * pload, int ploadlen, int gtp)
{
	#if DPI_PACE2
	
	if( pfs->pace2_enabled == 1)
	{
		dpi_result_t dpi_result = {0};
		dpi_return_t ret_dpi = dpi_process_l2_frame( dpi, coreindex, dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC), (const void * const)pload, ploadlen, gtp, &dpi_result);

		if (unlikely(ret_dpi == DPI_RETURN_SUCCESS)){
			// DEBUG_PRINTF("[ERROR] PACE2 process frame at ts %lu \n", ts);
			
			if( dpi_result.classification_finished_or_offloaded == 1)
			{
				return dpi_result.application;
			}
			
		}
	}
	
	return 0;
	#endif
}

void pfcp_stack___init_pace2()
{
	#if DPI_PACE2
		int dpi_worker_count = dpe__get_dpicorecount();
		g_dpi_worker_count = dpi_worker_count; 
		
		if( pfs->pace2_enabled == 1)
		{
			dpi_optargs_t dpi_opts = {0};
			memset( &dpi_opts, 0, sizeof(dpi_optargs_t));
			
			dpi_opts.dpi.io_resolution 										= 1000000;
			dpi_opts.dpi.deca_max_entries 									= 0;
			dpi_opts.dpi.unclassified_after_packet_count 					= 25;
			dpi_opts.dpi.flow_offload 										= 15;
			dpi_opts.dpi.fpc_enabled 										= 1;

			dpi_opts.flow_table.max_entries									= 500000;
			dpi_opts.flow_table.timeout_in_s								= 30;
			dpi_opts.flow_table.skip_dns_tracking							= 1;

			dpi_opts.subscriber_table.max_entries							= 10000;
			dpi_opts.subscriber_table.timeout_in_s							= 120;
			dpi_opts.subscriber_table.enabled								= 1;

			dpi_opts.fdep_plugin.ip_dst 									= "127.0.0.1";
			dpi_opts.fdep_plugin.port										= 4739;
			dpi_opts.fdep_plugin.enabled 									= 0;
			dpi_opts.fdep_plugin.active_timeout_delta_time 					= 10;       
			dpi_opts.fdep_plugin.active_timeout_delta_packets 				= 0;
			dpi_opts.fdep_plugin.template_refresh_interval 					= 30;
			dpi_opts.fdep_plugin.observation_domain_id 						= 0;
			dpi_opts.fdep_plugin.application_id_mapping_refresh_interval	= 7200;
			dpi_opts.fdep_plugin.mss_size 									= 1400;
			dpi_opts.fdep_plugin.mss_timeout								= 500;
			dpi_opts.fdep_plugin.disable_name_resolution_flows 				= 0;
			
			//required changes in config.c and need to update dpdk version
			// dpi_read_configfile( getenv("DPI_CFG"), &dpi_opts );
			
			dpi_return_t ret_dpi = dpi_init_instance( &dpi, dpi_worker_count, &dpi_opts);
			dpi_print_dpi_config( stdout, dpi);
		}		
	#endif
}

void pfcp_stack___set_upir( int upir)
{
	if( pfs) {
		 pfs->enable_upir = upir;
	}
} 	

void pfcp_stack___init( int ipv, char * IPv4, char * IPv6, uint32_t iIPv4, uint8_t * iIPv6, app_data_region_t * pfcp_mregion, int fsmThreads, int iHeartbeatSeconds, int session_count, int ueipv6support, int enableOrDisable)
{
	//__up_pfcp__print_element_dict();
	
	// printf( "sz=%ld %ld\n", sizeof(pfcp_ip_flow_t), sizeof(pfcp_session_t));
	// exit(0);
	
	pfs =(pfcp_stack_t *)malloc(sizeof(pfcp_stack_t));
	memset( pfs, 0, sizeof(pfcp_stack_t));

	pfs->ipv = ipv;
	pfs->enable_quota_logs = 0;
	
	if( ipv == 4)
	{
		strcpy( pfs->IPv4, IPv4);
		pfs->iIPv4 = iIPv4;
	}
	else if( ipv == 6)
	{
		strcpy( pfs->IPv6, IPv6);
		memcpy( pfs->iIPv6, iIPv6, 16);
	}
	
	pfs->pfcp_mregion = pfcp_mregion;
	__up_pfcp__set_data_region( pfs->pfcp_mregion);

	pthread_mutex_init( &pfs->pfcp_session_lock, NULL);


	pfs->far.head = NULL;
	pfs->far.current = NULL;
	pfs->far.count = 0;
	pthread_mutex_init( &pfs->far.Lock, NULL);

	pfs->qer.head = NULL;
	pfs->qer.current = NULL;
	pfs->qer.count = 0;
	pthread_mutex_init( &pfs->qer.Lock, NULL);

	pfs->urr.head = NULL;
	pfs->urr.current = NULL;
	pfs->urr.count = 0;
	pthread_mutex_init( &pfs->urr.Lock, NULL);
	
	
	pfs->startTime = time( NULL);
	pfs->pfcpNodeHead = NULL;
	pfs->pfcpNodeCurrent = NULL;
	pfs->pfcpNodeTotal = 0;
	pthread_mutex_init( &pfs->pfcpNodeLock, NULL);	
	pfs->HeartbeatSeconds = iHeartbeatSeconds;
	pfs->fsmThreads = fsmThreads;	
	
	pfs->ser_last = 0;
	pfs->ser_total = 0;
	pthread_mutex_init( &pfs->ser_lock, NULL);

	pfs->sea_last = 0;
	pfs->sea_total = 0;
	pthread_mutex_init( &pfs->sea_lock, NULL);

	
	pfs->smr_last = 0;
	pfs->smr_total = 0;
	pthread_mutex_init( &pfs->smr_lock, NULL);

	pfs->sma_last = 0;
	pfs->sma_total = 0;
	pthread_mutex_init( &pfs->sma_lock, NULL);

	
	pfs->sdr_last = 0;
	pfs->sdr_total = 0;
	pthread_mutex_init( &pfs->sdr_lock, NULL);

	pfs->sda_last = 0;
	pfs->sda_total = 0;
	pthread_mutex_init( &pfs->sda_lock, NULL);

	
	pfs->sur_last = 0;
	pfs->sur_total = 0;
	pthread_mutex_init( &pfs->sur_lock, NULL);

	pfs->sua_last = 0;
	pfs->sua_total = 0;
	pthread_mutex_init( &pfs->sua_lock, NULL);
	

	pfs->MaxTPS = 0;
	pthread_mutex_init( &pfs->maxtps_lock, NULL);
	
	
	pfs->hb_request_sent_last = 0;
	pfs->hb_request_sent_total = 0;
	pthread_mutex_init( &pfs->hb_request_sent_lock, NULL);

	pfs->hb_response_received_last = 0;
	pfs->hb_response_received_total = 0;
	pthread_mutex_init( &pfs->hb_response_received_lock, NULL);

	pfs->hb_request_received_last = 0;
	pfs->hb_request_received_total = 0;
	pthread_mutex_init( &pfs->hb_request_received_lock, NULL);

	pfs->hb_response_sent_last = 0;
	pfs->hb_response_sent_total = 0;
	pthread_mutex_init( &pfs->hb_response_sent_lock, NULL);
	
	pfs->cong_dropped_last = 0;
	pfs->cong_dropped_total = 0;
	pthread_mutex_init( &pfs->cong_dropped_lock, NULL);
	
	pfs->sess_delete_last = 0;
	pfs->sess_delete_total = 0;
	pthread_mutex_init( &pfs->sess_delete_lock, NULL);

	
	int table_count = 2;
	if( ueipv6support == 1)
	{
		table_count++;
	}
	
	pfs->enable_upir = 0;
	
	ls__init( session_count * (table_count * 1.5));  /*  based on table count */
	
	pfs->teid_table 	= ls__create_uint_table( session_count * 2, 32, 0);
	// pfs->ueipv4_table 	= ls__create_uint_table( session_count * 2, 32, 0);	
	// pfs->ueipv6_table	= NULL;

	// if( ueipv6support == 1)
	// {
		// pfs->ueipv6_table 	= ls__create_u128_table( session_count * 2, 128, 0);
	// }
	
	
	pfs->session_count = session_count;
	pfs->fsmQ = app_fsmqueue__create( "s-fsm", 20000, fsmThreads, NULL);
	
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	pthread_create( &s_pthread_id, &attr, pfcp_session__monitor, (void *)NULL);
	
	pfs->stime = 0;
	
	pthread_create( &s_pthread_id, &attr, pfcp_session__stime, (void *)NULL);
	
	
	
	
	pfs->pace2_enabled = (enableOrDisable == 1) ? 1 : 0;
			


	// app_region__create();
	// app_region__add_pool( dp_instance->pfcp_session_region, "PFCP-S", sizeof(pfcp_session_t), dp_instance->pfcp_MaxSessions);
	
}




/*
	pfcp_stack__perflog( app_logger_t * logger, int log_buffers)

	pfcp_stack__buffer_handler
	pfcp_stack___handle_session_establishment_request__buffer( node, msg, tIndex);
	pfcp_stack___handle_session_modification_or_delete_request__buffer( node, msg, tIndex, 2);
	pfcp_stack___handle_session_modification_or_delete_request__buffer( node, msg, tIndex, 3);
	
	pfcp_stack___handle_session_modification_request__fsm
	pfcp_stack___handle_session_deletion_request__fsm

	pfcp_stack__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink)
	quota_requested_time
*/










