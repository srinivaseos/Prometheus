#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <resolv.h>
#include <ifaddrs.h>

#ifndef LIB_DPDK_H
#define LIB_DPDK_H

#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_gtp.h>
#include <rte_distributor.h>

#include <rte_table.h>
#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>
#include <rte_arp.h>

#include <pcap/pcap.h>
#include <pcap/bpf.h>

#define PKT_IF__ACCESS						0
#define PKT_IF__CORE						1

#define PKT_TYPE__USER_DATA					1
#define PKT_TYPE__ARP						2
#define PKT_TYPE__UNKNOWN_USER_DATA			3

#define PKT_FWD__ALLOW						1
#define PKT_FWD__SCH						2
#define PKT_FWD__BUFFER						3
#define PKT_FWD__ARP						4
#define PKT_FWD__DROP						5

#if USENDPI
	#include "upfndpi.h"
#endif

typedef struct dpdkif dpdkif_t;

#pragma pack(4)
typedef struct dpdk_md
{
	uint8_t * p1;
	uint8_t * p2;
	uint8_t  * pfcp_session;
	dpdkif_t * src_iff;
} dpdk_md_t; 

#pragma pack(4)
typedef struct dpdkwt
{
	struct dpdkwt * Next;
	dpdkif_t * iff;
	int id;

	uint64_t rx_packets;
	//uint64_t tx_packets;

	uint64_t last_rx_packets;
	//uint64_t last_tx_packets;

	uint64_t fwd_packets;
	uint64_t dropped_packets;
	uint64_t buffered_packets;
	uint64_t arp_packets;
	uint64_t sch_packets;

} dpdkwt_t;

typedef struct dpdkif dpdkif_t;

#pragma pack(4)
typedef struct dpdkif_nic
{
	struct dpdkif_nic * Next;
	
	struct rte_mempool * mempool;
	
	uint32_t type;				// 0 - Access | 1 - Core
	char ifid[100];
	uint16_t port_id;
	uint16_t started;
	uint16_t mtu;
	uint16_t min_mtu;
	uint16_t max_mtu;
	
	uint8_t SrcMAC[7];
	uint8_t ValidateSrcMAC;
	uint8_t IsGTPEncapsulated;
	uint8_t CreateOuterHeader;

	uint8_t DstMAC[6];
	
	uint32_t ipv4;
	uint8_t ipv6[17];
	uint32_t WorkerThreadCount;
	uint8_t DedicatedCoreForRxQ;
	uint8_t DedicatedCoreForTxQ;
	
	uint16_t tx_q_count;
	uint16_t rx_q_count;
	uint16_t iRxBurstSize;
	uint16_t iTxBurstSize;
	int id;
	int iMemPoolSize;
	int iCacheSize;
	
	dpdkif_t * head;
	dpdkif_t * current;

} dpdkif_nic_t;

#pragma pack(4)
typedef struct dpdkif
{
	struct dpdkif * Next;
	
	dpdkif_nic_t * nic;
	uint32_t id;
	uint32_t type;				// 0 - Access | 1 - Core
	uint32_t pfcp_id;			// source identifier Access|Core
	uint16_t queueid;
	uint16_t tqueueid;
	uint16_t mtu;
	int workers;
	int test_mode;

	//struct rte_mempool * mempool;
	int iMemPoolSize;
	int iCacheSize;
	int iRxBurstSize;
	int iTxBurstSize;
	uint16_t nb_rx_desc;
	uint16_t nb_tx_desc;
	uint16_t enable_promiscuous_mode;

	// uint8_t SrcMAC[7];
	// uint8_t ValidateSrcMAC;
	// uint8_t IsGTPEncapsulated;
	// uint8_t CreateOuterHeader;

	// uint8_t DstMAC[6];
	
	// uint32_t ipv4;
	// uint8_t ipv6[17];
	
	//struct rte_ring * rx_ring;
	struct rte_ring * tx_ring;
	//int rx_ring_size;
	int tx_ring_size;
	int TxCore;
	
	uint64_t rx_packets;
	uint64_t rx_packets_rec;
	uint64_t rx_packets_enq;
	uint64_t tx_packets;
	uint64_t last_tx_packets;
	
	//uint32_t BurstSize;
	//uint32_t WorkerThreadCount;
	uint32_t LogIPHeader;
	uint32_t LogSession;
	uint32_t LogFlow;
	uint32_t LogPPResult;
	
	dpdkwt_t * wk_head;
	dpdkwt_t * wk_current;
	
	struct dpdkif * target_if;
} dpdkif_t;


#pragma pack(4)
typedef struct dpdk_pkt_info
{
	dpdkif_t * iff;
	uint16_t eth_type;
	uint8_t ipv;
	uint8_t proto;
	uint8_t result;
	uint8_t is_gtp_encapsulated;
	uint8_t  is_seqn_present;
	uint16_t seqn_no;
	
	uint8_t  ue_ipv;
	uint8_t  ue_proto;
	uint32_t ue_src_ip;
	uint32_t ue_dst_ip;

	uint8_t  * ue_src_ip6;
	uint8_t  * ue_dst_ip6;

	uint16_t ue_src_port;
	uint16_t ue_dst_port;
	
	uint32_t teid;
	
	uint32_t access_ip;
	uint32_t      n_ip;

	uint8_t  * access_ip6;
	uint8_t  *      n_ip6;

	uint32_t arp_targetip4;

	uint8_t * iph;
	uint32_t iplen;

} dpdk_pkt_info_t;

typedef struct dpdk_flow_key dpdk_flow_key_t;
typedef struct dpdk_flow dpdk_flow_t;
typedef struct dpdk_session dpdk_session_t;

#pragma pack(4)
typedef struct dpdk_flow_key
{
	union {
		struct {
			uint32_t src_ip;
			uint32_t dst_ip;
		} f;
		struct {
			uint8_t src_ip[16];
			uint8_t dst_ip[16];
		} s;
	} ip;
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t protocol;
	uint8_t ipv;
} dpdk_flow_key_t;

#pragma pack(4)
typedef struct dpdk_flow
{
	struct dpdk_flow * Next;
	dpdk_flow_key_t key;
	
	char sni[150];
	uint16_t master_protocol;
	uint16_t app_protocol;
	uint16_t category;
	
	#if USENDPI
		ndpi_flow_struct_wt * ndpi_flow;
	#endif
	
	uint16_t rgid;
	uint16_t rg_identified;
	uint32_t allowed;
	//uint64_t used;
} dpdk_flow_t;

typedef uint64_t (*fp_dpdk_session__get_seid)( void * sPtr);
typedef uint64_t (*fp_dpdk_session__get_quota)( void * sPtr, uint16_t rgid);
typedef uint64_t (*fp_dpdk_session__record_usage)( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink);
typedef uint8_t * (*fp_dpdk_session__get_dpi_session)( uint64_t seid);
typedef uint8_t * (*fp_dpdk_session__get_pfcp_session)( uint64_t seid);
typedef int (*fp_dpdk_session__get_ohi)( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no);


void dpdk_add_rgid( uint32_t rgid, uint16_t m_pid, uint16_t app_pid);
uint32_t dpdk_get_rgid( uint16_t m_pid, uint16_t app_pid);

void dpdk__print_buffer( unsigned char * buff, uint32_t len);
void dpdk__initexit();
void dpdk__init( int argc, char **argv);

typedef void (*fp_logger)(int logtype, int logcat, int logLevel, char * mLogMessage);
void dpdk__set_logger_m( fp_logger flog, fp_logger plog);
void dpdk__log_pref_counters();

int dpdk_pkt__encode_arp_response( struct rte_mbuf * mbuf_req, struct rte_mbuf * mbuf_rsp, char * srcmac, uint32_t srcip);
int dpdk_pkt__encode_arp_request( struct rte_mbuf * mbuf_req, char * srcmac, uint32_t srcip, uint32_t tip);
void dpdk_pkt__print( struct rte_mbuf * m);

int dpdk_pkt__remove_outer_header_256( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac);
int dpdk_pkt__create_outer_header_256( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, int an_ipv, uint32_t upfipv4, uint32_t anipv4, uint8_t * upfipv6, uint8_t * anipv6, uint32_t teid, int gtpHasSQN, uint16_t gtp_seq_no);
int dpdk_pkt__encode_gtp_echo_response_ipv4( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t msgid, uint8_t teid, uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi);
int dpdk_pkt__encode_gtp_echo_response_ipv6( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint8_t * src_ip, uint8_t * dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t msgid, uint8_t teid, uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi);

void dpdk_pkt__test_case_write_arp_request( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_write_arp_response( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_remove_outer_header_1( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_remove_outer_header_2( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_create_outer_header_ip4( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_create_outer_header_ip6( pcap_dumper_t * dumper, struct rte_mempool * mempool);
void dpdk_pkt__test_case_create_gtp_packet_ipv4( pcap_dumper_t * dumper, struct rte_mempool * mempool, uint8_t msgtype);
void dpdk_pkt__test_case_create_gtp_packet_ipv6( pcap_dumper_t * dumper, struct rte_mempool * mempool, uint8_t msgtype);

pcap_dumper_t * dpdk_pkt__open_pcap_dump( char * filename);
void dpdk_pkt__write_pcap( pcap_dumper_t * dumper, struct rte_mbuf * pkt);
void dpdk_pkt__close_pcap( pcap_dumper_t * dumper);

int dpdk_parse_pkt__parse( struct rte_mbuf * mbuf, dpdk_pkt_info_t * info);

int dpdk_access__process_pkt( struct rte_mbuf * pkt, dpdk_pkt_info_t * info);
int dpdk_core__process_pkt( struct rte_mbuf * pkt, dpdk_pkt_info_t * info);

void dpdk_parse_pkt__test_case_1( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_2( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_3( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_4( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_5( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_5( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_6( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_7( struct rte_mempool * mempool, dpdkif_t * dif);
void dpdk_parse_pkt__test_case_8( struct rte_mempool * mempool, dpdkif_t * dif);

void dpdk_pkt__print_d( char * data, int len);

int  dpdk_session__init( int count, int ipv, int ipv4_flow_table_count, int flows_per_flow_table, int total_flows);
void dpdk_session__teid_del(  uint32_t teid);
void dpdk_session__ipv4_del(  uint32_t ipv4);
int  dpdk_session__teid_add(  uint32_t teid, void *  data, int deleteIfExists);
int  dpdk_session__ipv4_add(  uint32_t ipv4, void *  data, int deleteIfExists);
int  dpdk_session__teid_find( uint32_t teid, void ** data);
int  dpdk_session__ipv4_count();
int  dpdk_session__ipv4_find( uint32_t ipv4, void ** data);
int  dpdk_session__ipv6_find( uint8_t * ipv6, void ** data);
int  dpdk_session__seid_find( uint64_t seid, void ** data);
void dpdk_session__seid_del( uint64_t seid);
int  dpdk_session__seid_add( uint64_t seid, void * data, int deleteIfExists);

int dpdk_session__addflow( uint64_t seid, dpdk_session_t * session, int deleteIfExists, dpdk_flow_t * flow);
dpdk_flow_t * dpdk_session__create_flow( uint8_t ipv, uint32_t src_ip, uint32_t dst_ip, uint8_t * src_ip6, uint8_t * dst_ip6, uint16_t src_port, uint16_t dst_port, uint8_t protocol);
dpdk_flow_t * dpdk_session__find_flow( uint64_t seid, uint8_t ipv, uint32_t src_ip, uint32_t dst_ip, uint8_t * src_ip6, uint8_t * dst_ip6, uint16_t src_port, uint16_t dst_port, uint8_t protocol);


int dpdk_nic__configure_and_start( dpdkif_nic_t * nic);
int dpdk_nic__stop( dpdkif_nic_t * nic);


void dpdk_dpi__l7inspection( dpdk_flow_t * flow, uint8_t * iph, uint32_t len);

uint16_t dpdk_ipflow__is_dns_packet2( uint8_t proto, uint16_t port);
uint16_t dpdk_ipflow__is_dns_packet( dpdk_flow_key_t * dpdk_flow_key);
uint16_t dpdk_ipflow__is_packet_allowed_without_quota( dpdk_flow_key_t * dpdk_flow_key);

void dpdk_session__set_fp_seid( fp_dpdk_session__get_seid pfp_get_seid);
void dpdk_session__set_fp_quota( fp_dpdk_session__get_quota pfp_get_quota);
void dpdk_session__set_fp_usage( fp_dpdk_session__record_usage pfp_record_usage);
void dpdk_session__set_fp_dpi_session( fp_dpdk_session__get_dpi_session pfp_get_dpi_session);
void dpdk_session__set_fp_pfcp_session( fp_dpdk_session__get_pfcp_session pfp_get_pfcp_session);
void dpdk_session__set_fp_ohi( fp_dpdk_session__get_ohi pfp_get_ohi);
int  dpdk_session__get_ohi( uint8_t * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no);

typedef struct dpdk_session dpdk_session_t;
dpdk_session_t * dpdk_session__get_dpi_session( uint64_t seid);
uint8_t * dpdk_session__get_pfcp_session( uint64_t seid);
uint64_t dpdk_session__get_quota( void * sPtr, uint16_t rgid);
uint64_t dpdk_session__get_seid( void * sPtr);
void dpdk_session__record_usage( dpdk_flow_t * flow, void * sPtr, dpdk_session_t * session, uint64_t uplink, uint64_t downlink);

int dpdk__inject_packet( int type, unsigned char * data, int len);
int dpdk_parse_pkt__parse_and_print( struct rte_mbuf * mbuf, dpdkif_t * dif);
void dpdk__log( int logLevel, char * mLogMessage, ...);

int dpdk_nic__print_device_info( char * device_name);
void dpdk_nic__link_status( dpdkif_t * ifc);
int dpdk_nic__stop_device( dpdkif_t * ifc);
int dpdk_nic__start_device( dpdkif_t * ifc);
int dpdk_nic__configure_device( dpdkif_t * ifc);

uint32_t dpdk__enabled_packet_inspection();

#endif











