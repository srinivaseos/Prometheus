#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>

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
#include "dpdk.h"
#include <rte_memory.h>

int  dpdk_session__seid_add(  uint64_t seid, void * data, int deleteIfExists);
int  dpdk_session__seid_find( uint64_t seid, void ** data);
void dpdk_session__seid_del( uint64_t seid);



typedef struct dpdk_session
{
	dpdk_flow_t * fHead;
	dpdk_flow_t * fCurrent;
	int fCount;
	pthread_mutex_t fLock;
} dpdk_session_t;


typedef struct dpdk_session_mgr
{
	int count;
	struct rte_hash * teid_table;
	struct rte_hash * ipv4_table;
	struct rte_hash * ipv6_table;
	struct rte_hash * ipv6_flow;
	struct rte_hash * seid_table;
	struct rte_hash ** ip_flow;
	int ip_flow_count;
	struct rte_mempool * mempool;
} dpdk_session_mgr_t;

dpdk_session_mgr_t * smgr = NULL;



static struct rte_hash_parameters teid_ht_params = {
	.entries = 10000,
	.key_len = 4,
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};

static struct rte_hash_parameters ipv4_4t_params = {
	.entries = 2000,
	.key_len = 9,
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};

static struct rte_hash_parameters ipv4_5t_params = {
	.entries = 2000,
	.key_len = 13,
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};




fp_dpdk_session__get_seid 			fp_get_seid = NULL;
fp_dpdk_session__get_quota 			fp_get_quota = NULL;
fp_dpdk_session__record_usage		fp_record_usage = NULL;
fp_dpdk_session__get_dpi_session	fp_get_dpi_session = NULL;
fp_dpdk_session__get_pfcp_session	fp_get_pfcp_session = NULL;
fp_dpdk_session__get_ohi			fp_get_ohi = NULL;

void dpdk_session__set_fp_ohi( fp_dpdk_session__get_ohi pfp_get_ohi)
{
	fp_get_ohi = pfp_get_ohi;
}

void dpdk_session__set_fp_dpi_session( fp_dpdk_session__get_dpi_session pfp_get_dpi_session)
{
	fp_get_dpi_session = pfp_get_dpi_session;
}

void dpdk_session__set_fp_pfcp_session( fp_dpdk_session__get_pfcp_session pfp_get_pfcp_session)
{
	fp_get_pfcp_session = pfp_get_pfcp_session;
}

void dpdk_session__set_fp_seid( fp_dpdk_session__get_seid pfp_get_seid)
{
	fp_get_seid = pfp_get_seid;
}

void dpdk_session__set_fp_quota( fp_dpdk_session__get_quota pfp_get_quota)
{
	fp_get_quota = pfp_get_quota;
}

void dpdk_session__set_fp_usage( fp_dpdk_session__record_usage pfp_record_usage)
{
	fp_record_usage = pfp_record_usage;
}

int dpdk_session__get_ohi( uint8_t * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no)
{
	if( fp_get_ohi > 0)
	{
		return fp_get_ohi( sPtr, teid, ipv, ranip, upfip, ranip6, upfip6, ranmac, upfmac, gtpHasSQN, gtp_seq_no);
	}
	return 0;
}

uint64_t dpdk_session__get_seid( void * sPtr)
{
	if( fp_get_seid > 0)
	{
		return fp_get_seid( sPtr);
	}
	return 0;
}

uint64_t dpdk_session__get_quota( void * sPtr, uint16_t rgid)
{
	if( fp_get_quota > 0)
	{
		return fp_get_quota( sPtr, rgid);
	}
	return 0;
}


dpdk_session_t * dpdk_session__get_dpi_session( uint64_t seid)
{
	if( fp_get_dpi_session > 0)
	{
		return (dpdk_session_t *)fp_get_dpi_session( seid);
	}
	return NULL;
}

uint8_t * dpdk_session__get_pfcp_session( uint64_t seid)
{
	if( fp_get_pfcp_session > 0)
	{
		return fp_get_pfcp_session( seid);
	}
	return NULL;
}


void dpdk_session__record_usage( dpdk_flow_t * flow, void * sPtr, dpdk_session_t * session, uint64_t uplink, uint64_t downlink)
{
	if( fp_record_usage > 0)
	{
		if( flow)
		{
			fp_record_usage( sPtr, flow->rgid, uplink, downlink);
		}
		else
		{
			fp_record_usage( sPtr, 0, uplink, downlink);
		}
	}
}


void dpdk_session__teid_del( uint32_t teid)
{
	int32_t ksts = rte_hash_del_key( smgr->teid_table, &teid);
	if( ksts >= 0) {
		rte_hash_free_key_with_position( smgr->teid_table, ksts);
	}	
}

int dpdk_session__teid_add( uint32_t teid, void * data, int deleteIfExists)
{
	if( deleteIfExists == 1)
	{
		void * exis_data = NULL;
		int ksts = rte_hash_lookup_data( smgr->teid_table, &teid, &exis_data);
		
		if( ksts >= 0) {
			dpdk_session__teid_del( teid);
		}
	}
	
	int sts = rte_hash_add_key_data( smgr->teid_table, &teid, data);

	if( sts == EINVAL)
	{
		printf("if the parameters are invalid\n");
		return -1;
	}

	if( sts == ENOSPC)
	{
		printf("no space in the hash for this key.\n");
		return -2;
	}
	
	return 0;
}

void dpdk_session__ipv4_del( uint32_t ipv4)
{
	int32_t ksts = rte_hash_del_key( smgr->ipv4_table, &ipv4);
	if( ksts >= 0) {
		rte_hash_free_key_with_position( smgr->ipv4_table, ksts);
	}	
}

int dpdk_session__ipv4_count()
{
	if(!smgr) return 0;
	if(!smgr->ipv4_table) return 0;
	
	return rte_hash_count( smgr->ipv4_table);
}

int dpdk_session__ipv4_add( uint32_t ipv4, void * data, int deleteIfExists)
{
	if( deleteIfExists == 1)
	{
		void * exis_data = NULL;
		int ksts = rte_hash_lookup_data( smgr->ipv4_table, &ipv4, &exis_data);

		if( ksts >= 0) {
			dpdk_session__ipv4_del( ipv4);
		}
	}
	
	int sts = rte_hash_add_key_data( smgr->ipv4_table, &ipv4, data);
	

	if( sts == EINVAL)
	{
		printf("if the parameters are invalid\n");
		return -1;
	}

	if( sts == ENOSPC)
	{
		printf("no space in the hash for this key.\n");
		return -2;
	}
	
	return 0;
}

int dpdk_session__teid_find( uint32_t teid, void ** data)
{
	return rte_hash_lookup_data( smgr->teid_table, &teid, data);
}

int dpdk_session__ipv4_find( uint32_t ipv4, void ** data)
{
	return rte_hash_lookup_data( smgr->ipv4_table, &ipv4, data);
}

int dpdk_session__ipv6_find( uint8_t * ipv6, void ** data)
{
	if( smgr->ipv6_table) 
		return rte_hash_lookup_data( smgr->ipv6_table, &ipv6, data);
		
	return -1;
}

int dpdk_session__seid_add( uint64_t seid, void * data, int deleteIfExists)
{
	if( deleteIfExists == 1)
	{
		void * exis_data = NULL;
		int ksts = rte_hash_lookup_data( smgr->seid_table, &seid, &exis_data);
		
		if( ksts >= 0) {
			dpdk_session__seid_del( seid);
		}
	}
	
	int sts = rte_hash_add_key_data( smgr->seid_table, &seid, data);
	//printf( "sts=%d EINVAL=%d ENOSPC=%d\n", sts , EINVAL, ENOSPC);
	

	if( sts == EINVAL)
	{
		printf("if the parameters are invalid\n");
		return -1;
	}

	if( sts == ENOSPC)
	{
		printf("no space in the hash for this key.\n");
		return -2;
	}
	
	return 0;
}

void dpdk_session__seid_del( uint64_t seid)
{
	int32_t ksts = rte_hash_del_key( smgr->seid_table, &seid);
	if( ksts >= 0) {
		rte_hash_free_key_with_position( smgr->seid_table, ksts);
	}	
}


int dpdk_session__seid_find( uint64_t seid, void ** data)
{
	return rte_hash_lookup_data( smgr->seid_table, &seid, data);
}


dpdk_flow_t * dpdk_session__find_flow( uint64_t seid, uint8_t ipv, uint32_t src_ip, uint32_t dst_ip, uint8_t * src_ip6, uint8_t * dst_ip6, uint16_t src_port, uint16_t dst_port, uint8_t protocol)
{
	dpdk_flow_key_t flowkey;
	memset( &flowkey, 0, sizeof(dpdk_flow_key_t));
	flowkey.ipv = ipv;
	
	if( ipv == 4)
	{
		flowkey.ip.f.src_ip = src_ip;
		flowkey.ip.f.dst_ip = dst_ip;
	}
	else
	{
		memcpy( flowkey.ip.s.src_ip, src_ip6, 16);
		memcpy( flowkey.ip.s.dst_ip, dst_ip6, 16);
	}
	
	flowkey.src_port = src_port;
	flowkey.dst_port = dst_port;
	flowkey.protocol = protocol;

	
	dpdk_flow_t * exis_data = NULL;
	int ksts = rte_hash_lookup_data( smgr->ip_flow[ seid % smgr->ip_flow_count ], &flowkey, (void **)&exis_data);
	return exis_data;
}


dpdk_flow_t * dpdk_session__create_flow( uint8_t ipv, uint32_t src_ip, uint32_t dst_ip, uint8_t * src_ip6, uint8_t * dst_ip6, uint16_t src_port, uint16_t dst_port, uint8_t protocol)
{
	struct rte_mbuf * m = rte_pktmbuf_alloc( smgr->mempool);	

	if( m)
	{
		printf("rte_mempool_avail_count=%u\n", rte_mempool_avail_count(smgr->mempool));
		
		dpdk_flow_t * flow = (dpdk_flow_t *)(uint8_t *)rte_pktmbuf_mtod( m, uint8_t *);
		memset( flow, 0, sizeof( dpdk_flow_t));
		flow->key.ipv = ipv;
		
		if( ipv == 4)
		{
			flow->key.ip.f.src_ip = src_ip;
			flow->key.ip.f.dst_ip = dst_ip;
		}
		else
		{
			memcpy( flow->key.ip.s.src_ip, src_ip6, 16);
			memcpy( flow->key.ip.s.dst_ip, dst_ip6, 16);		
		}
		
		flow->key.src_port = src_port;
		flow->key.dst_port = dst_port;
		flow->key.protocol = protocol;
		flow->rg_identified = 0;
		flow->rgid = 0;
	
		return flow;
	}
	return NULL;
}

int dpdk_session__addflow( uint64_t seid, dpdk_session_t * session, int deleteIfExists, dpdk_flow_t * flow)
{
	// 1800-2666-868
	// JN563903938IN
	
	if( deleteIfExists == 1)
	{
		void * exis_data = NULL;
		int ksts = rte_hash_lookup_data( smgr->ip_flow[ seid % smgr->ip_flow_count ], &flow->key, &exis_data);
		
		if( ksts >= 0) {
			int32_t ksts2 = rte_hash_del_key( smgr->ip_flow[ seid % smgr->ip_flow_count ], &flow->key);
			if( ksts2 >= 0) {
				rte_hash_free_key_with_position( smgr->ip_flow[ seid % smgr->ip_flow_count ], ksts2);
			}
		}
	}
	
	int sts = rte_hash_add_key_data( smgr->ip_flow[ seid % smgr->ip_flow_count ], &flow->key, flow);
	
	// pthread_mutex_lock( &session->fLock);
	if(!session->fHead)
	{
		session->fHead = session->fCurrent = flow;
	}
	else
	{
		session->fCurrent->Next = flow;
		session->fCurrent = flow;
	}
	// pthread_mutex_unlock( &session->fLock);

	if( sts == EINVAL)
	{
		printf("if the parameters are invalid\n");
		return -1;
	}

	if( sts == ENOSPC)
	{
		printf("no space in the hash for this key.\n");
		return -2;
	}
	
	return 0;
}

/*
	int ipv  1 - IPv4, 2 - IPv6, 3 - IPv4v6
*/
int dpdk_session__init( int count, int ipv, int ip_flow_table_count, int flows_per_flow_table, int total_flows)
{
	if(!smgr)
	{
		smgr = (dpdk_session_mgr_t *)malloc(sizeof(dpdk_session_mgr_t));
		memset( smgr, 0, sizeof(dpdk_session_mgr_t));
		
		if(!smgr)
		{
			printf("session_mgr creation failed\n");
			exit(0);
			return -1;
		}

		printf("dpdk session, creating with count=%d\n", count);

		
		smgr->count 				= count;

		teid_ht_params.name 		= "teid_table";
		teid_ht_params.entries 		= count;
		teid_ht_params.extra_flag 	= RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY;
		smgr->teid_table 			= rte_hash_create( &teid_ht_params);

		if(!smgr->teid_table)
		{
			printf("teid_table creattion failed for count=%d\n", count);
			exit(0);
			return -1;
		}

		if( ipv == 1 || ipv == 3)
		{
			teid_ht_params.name 		= "ipv4_table";
			teid_ht_params.entries 		= count;
			teid_ht_params.key_len 		= 4;
			smgr->ipv4_table 			= rte_hash_create( &teid_ht_params);		

			if(!smgr->ipv4_table)
			{
				printf("ipv4_table creattion failed for count=%d\n", count);
				exit(0);
				return -1;
			}
		}
		
		if( ipv == 2 || ipv == 3)
		{
			teid_ht_params.name 		= "ipv6_table";
			teid_ht_params.entries 		= count;
			teid_ht_params.key_len 		= 16;
			smgr->ipv6_table 			= rte_hash_create( &teid_ht_params);

			if(!smgr->ipv6_table)
			{
				printf("ipv6_table creattion failed for count=%d\n", count);
				exit(0);
				return -1;
			}
		}
		
		/*
		teid_ht_params.name 		= "seid_table";
		teid_ht_params.entries 		= count;
		teid_ht_params.key_len 		= 8;
		smgr->seid_table 			= rte_hash_create( &teid_ht_params);

		if(!smgr->seid_table)
		{
			printf("seid_table creattion failed for count=%d\n", count);
			return -1;
		}
		*/
		
		ipv4_5t_params.name 		= "ipv4_5t_params";
		ipv4_5t_params.entries 		= flows_per_flow_table;
		ipv4_5t_params.key_len 		= sizeof(dpdk_flow_key_t);
		
		smgr->ip_flow_count 		= ip_flow_table_count;
		smgr->ip_flow 				= (struct rte_hash **) malloc( sizeof(void *) * ip_flow_table_count);

		if(!smgr->ip_flow)
		{
			printf("session ip_flow, creation failed\n");
			exit(0);
		}

		char flow_name[50];
		
		int i = 0;
		for( i = 0 ; i < ip_flow_table_count; i++)
		{
			memset( flow_name, 0, sizeof(flow_name));
			sprintf( flow_name, "hash-5t-%d", i);
			ipv4_5t_params.name = flow_name;
			
			smgr->ip_flow[i] = rte_hash_create( &ipv4_5t_params);
			
			if(!smgr->ip_flow[i])
			{
				printf("session ip_flow[%d], creation failed withname=%s\n", i, ipv4_5t_params.name);
				exit(0);
			}
		}
		
		smgr->mempool = rte_pktmbuf_pool_create( "flow_table", total_flows, 128, 16, sizeof(dpdk_flow_t), rte_socket_id());
		printf("created mempool for flows with count =%u\n",total_flows);
		if(!smgr->mempool)
		{
			printf("session manager, memory creation failed\n");
			exit(0);
		}
	
		return 1;
	}
	return 0;
}
















