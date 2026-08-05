#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_arp.h>
#include <rte_memcpy.h>
#include <rte_mempool.h>
#include <rte_malloc.h>

#include "jansson.h"
#include "dpdk.h"

#if USENDPI
	#include "upfndpi.h"
#endif


typedef struct dpdk_rgmap
{
	struct dpdk_rgmap * Next;
	
	uint32_t rg_id;
	uint16_t ndpi_master_protocol_id;
	uint16_t ndpi_app_protocol_id;
	
} dpdk_rgmap_t;


typedef struct dpdkstack
{
	json_t * config;
	
	struct rte_ring * access_wk_ring;
	struct rte_ring * access_sh_ring;

	struct rte_ring * core_wk_ring;
	struct rte_ring * core_sh_ring;

	int access_wk_ring_size;
	int access_sh_ring_size;

	int core_wk_ring_size;
	int core_sh_ring_size;
	

	dpdkif_t * accessIfHead;
	dpdkif_t * accessIfCurrent;
	int accessIfCount;
	
	dpdkif_t * coreIfHead;
	dpdkif_t * coreIfCurrent;
	int coreIfCount;

	dpdkif_nic_t * coreNicHead;
	dpdkif_nic_t * coreNicCurrent;
	int coreNicCount;

	dpdkif_nic_t * accessNicHead;
	dpdkif_nic_t * accessNicCurrent;
	int accessNicCount;
	
	uint64_t wk_processed_pkts;
	uint64_t sh_processed_pkts;

	uint64_t wk_dropped_pkts;
	uint64_t sh_dropped_pkts;
	
	int PrintStatsOnConsole;
	int DedicatedPerfThread;
	
	int SessionCount;
	int Supported_IPV;
	int FlowTableCount;
	int FlowsPerFlowTable;
	int TotalFlows;
	
	int BurstSize;
	int iPacketTest_Enable;
	int TestStage;
	int TestSleep;
	int Scheduler;
	int whileRun;
	
	int enable_packet_inspection;
	
	dpdk_rgmap_t * rgHead;
	dpdk_rgmap_t * rgCurrent;
} dpdkstack_t;

dpdkstack_t * dstack = NULL;

typedef void (*fp_logger)(int logtype, int logcat, int logLevel, char * mLogMessage);

uint32_t dpdk__enabled_packet_inspection()
{
	return dstack->enable_packet_inspection;
}

uint32_t dpdk_get_rgid( uint16_t m_pid, uint16_t app_pid)
{
	
	dpdk_rgmap_t * rgmap =  dstack->rgHead;
	
	while( rgmap)
	{
		if( rgmap->ndpi_master_protocol_id == m_pid)
			return rgmap->rg_id;
		
		rgmap = rgmap->Next;
	}
	
	
	return 1;
}

void dpdk_add_rgid( uint32_t rgid, uint16_t m_pid, uint16_t app_pid)
{
	
	dpdk_rgmap_t * rgmap = (dpdk_rgmap_t *)rte_malloc( NULL, sizeof(dpdk_rgmap_t), 0);
	
	if( rgmap)
	{
		rgmap->Next = NULL;
		rgmap->rg_id = rgid;
		rgmap->ndpi_master_protocol_id = m_pid;
		rgmap->ndpi_app_protocol_id = app_pid;

		if(!dstack->rgHead)
		{
			dstack->rgHead = dstack->rgCurrent = rgmap;
		}
		else
		{
			dstack->rgCurrent->Next = rgmap;
			dstack->rgCurrent = rgmap;
		}
	}
	
}

uint8_t * rte_malloc_fun( int sz)
{
	return (uint8_t *)rte_malloc( NULL, sz, 0);
}



fp_logger lflog = NULL;
fp_logger pflog = NULL;

void dpdk__set_logger_m( fp_logger flog, fp_logger plog)
{
	lflog = flog;
	pflog = plog;
}

void dpdk__log( int logLevel, char * mLogMessage, ...)
{
	char m_buffer[1000];
	
	va_list args;
	va_start( args, mLogMessage);
	vsnprintf( m_buffer, 1000, mLogMessage, args);
	va_end( args);

	if( lflog > 0)
	{
		//printf( "2 - %s\n", m_buffer);
		lflog( 0, 0, logLevel, m_buffer);
	}
	else
	{
		printf( "3 - %s\n", m_buffer);
	}
}

void dpdk__plog( char * mLogMessage, ...)
{
	if( pflog > 0)
	{
		char m_buffer[1000];
		
		va_list args;
		va_start( args, mLogMessage);
		vsnprintf( m_buffer, 1000, mLogMessage, args);
		va_end( args);	
		
		pflog( 0, 0, 1, m_buffer);
	}
}



int dpdk_config__get_int( json_t * json_config, char * key, int defval, int minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		int v = json_integer_value( jObj);
		if( v < minval)
			return minval;
		else
			return v;
	}
	return defval;
}


void dpdk__print_buffer( unsigned char * buff, uint32_t len)
{
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf( "%02X ", buff[i] & 0xFF );
	}
	
	printf("\n");
}



void dpdk__initexit()
{
	printf("initiate exit\n");
	dstack->whileRun = 0;
	
	sleep(1);
	
	/*
	dpdkif_t * dif 	= dstack->coreIfHead;
	while( dif)
	{
		if( dif->mempool)
		{
			rte_mempool_free( dif->mempool);
			printf("freeing rte_mempool_free completed\n");
		}
		dif = dif->Next;
	}
	
	dif = dstack->accessIfHead;
	while( dif)
	{
		if( dif->nic->mempool)
		{
			rte_mempool_free( dif->nic->mempool);
			printf("freeing rte_mempool_free completed\n");
		}
		dif = dif->Next;
	}
	*/
	
	dpdkif_nic_t * nic = dstack->coreNicHead;
	while( nic)
	{
		if( nic->started == 1)
		{
			dpdk_nic__stop( nic);
			printf("stopped dpdk_nic__stop     nic=%s  %s|%s|%d\n", nic->ifid, __FILE__, __FUNCTION__, __LINE__);
		}
		
		if( nic->mempool)
		{
			rte_mempool_free( nic->mempool);
			printf("freed rte_mempool_free completed for nic=%s  %s|%s|%d\n", nic->ifid, __FILE__, __FUNCTION__, __LINE__);
		}
		nic = nic->Next;
	}
	
	nic = dstack->accessNicHead;
	while( nic)
	{
		if( nic->started == 1)
		{
			dpdk_nic__stop( nic);
			printf("stopped dpdk_nic__stop     nic=%s    %s|%s|%d\n", nic->ifid, __FILE__, __FUNCTION__, __LINE__);
		}
		
		if( nic->mempool)
		{
			rte_mempool_free( nic->mempool);
			printf("freed rte_mempool_free completed for nic=%s  %s|%s|%d\n", nic->ifid, __FILE__, __FUNCTION__, __LINE__);
		}
		nic = nic->Next;
	}

	
	int sts = rte_eal_cleanup();
	
	printf("initiate exit - completed with sts=%d\n", sts);
}



int dpdk__iflcore_tx_func( dpdkif_t * ifc)
{
	int recv_pkts = 0;
	int sent = 0;
	struct rte_mbuf * mbufs[ifc->iTxBurstSize];
	int pkts_send = 0;
	
	recv_pkts = rte_ring_dequeue_burst( ifc->tx_ring, (void **)mbufs, ifc->iTxBurstSize, NULL);

	if( recv_pkts > 0)
	{
		if( ifc->target_if->test_mode > 0)
		{
			rte_pktmbuf_free_bulk( mbufs, recv_pkts);
			ifc->tx_packets += recv_pkts;
		}
		else 
		{
			pkts_send = 0;
			sent = 0;
			
			if( ifc->nic->started == 1)
			{
				//printf("transmitting packets to %s\n", ifc->nic->ifid);
				do 
				{
					sent = rte_eth_tx_burst( ifc->nic->port_id, ifc->tqueueid, &mbufs[pkts_send], recv_pkts - pkts_send);
					pkts_send += sent;
					ifc->tx_packets += sent;
				} while ( pkts_send < recv_pkts);
			}
			else
			{
				rte_pktmbuf_free_bulk( mbufs, recv_pkts);
				ifc->tx_packets += recv_pkts;
			}
		}
	}	
		
	return 0;
}

int dpdk__iflcore_tx( void * args)
{
	dpdkif_t * ifc = (dpdkif_t *)args;
	
	while(!ifc->nic->mempool)
	{
		usleep(111111);
	}
	
	while( dstack->whileRun)
	{
		dpdk__iflcore_tx_func( ifc);
	}
	
	return 0;
}


int dpdk__inject_packet( int type, unsigned char * data, int len)
{
	struct rte_mbuf * pkt = NULL;
	if( type == 0) {
		pkt = rte_pktmbuf_alloc( dstack->accessIfHead->nic->mempool);
	} else {
		pkt = rte_pktmbuf_alloc( dstack->coreIfHead->nic->mempool);
	}
	
	if( pkt)
	{
		char * c_mptr4 = rte_pktmbuf_mtod( pkt, char *);
		memcpy( c_mptr4, data, len);
	
		pkt->pkt_len = len;
		pkt->data_len = len;
		
		if( type == 0) {
			return rte_ring_enqueue( dstack->access_wk_ring, pkt); 	
		} else {
			return rte_ring_enqueue( dstack->core_wk_ring  , pkt); 	
		}
	}
	
	return -1;
}

int dpdk__iflcore( void * args)
{
	dpdkif_t * ifc = (dpdkif_t *)args;

	printf( "lcore_id=%d test_mode=%d type=%d test_mode=%d MemPoolSize=%d CacheSize=%d iRxBurstSize=%d iTxBurstSize=%d   %s|%s|%d\n", 
		rte_lcore_id(), ifc->test_mode, ifc->type, ifc->test_mode, 
		ifc->iMemPoolSize, ifc->iCacheSize, ifc->iRxBurstSize, ifc->iTxBurstSize, __FILE__, __FUNCTION__, __LINE__);


	/*
	char ring_name[30];
	memset( ring_name, 0, sizeof(ring_name));
	
	sprintf( ring_name, "mempool-%d-%d-%d", ifc->type, ifc->id, ifc->queueid);
	ifc->mempool = rte_pktmbuf_pool_create( ring_name, ifc->iMemPoolSize, ifc->iCacheSize, sizeof(dpdk_md_t), RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if(!ifc->mempool)
	{
		printf("mempool creation failed for id=%d type=%d iMemPoolSize=%d iCacheSize=%d\n", ifc->id, ifc->type, ifc->iMemPoolSize, ifc->iCacheSize);
		//dpdk__initexit();
		exit(0);
	}
	*/

	struct rte_ring * wk_ring = dstack->access_wk_ring;
	int wk_ring_size = dstack->access_wk_ring_size;
	
	if( ifc->type == 1) 
	{
		wk_ring = dstack->core_wk_ring;
		wk_ring_size = dstack->core_wk_ring_size;
	}

	struct rte_mbuf * mbufs[ifc->iRxBurstSize + ifc->iTxBurstSize];
	int received_cnt = 0;
	int pkts_send = 0;

	printf("ifLcore: mbufs-count=%d  %s|%s|%d\n", ifc->iRxBurstSize + ifc->iTxBurstSize, __FILE__, __FUNCTION__, __LINE__);
	int enquee_received_packets = 0;
	
	if(!wk_ring)
	{
		printf( "ifLcore: wk_ring is null  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}
	
	if(wk_ring_size == 0)
	{
		printf( "ifLcore: wk_ring_size is 0  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}
	
	if(!ifc->nic)
	{
		printf( "ifLcore: nic is null  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);		
	}
	
	while(ifc->nic->started == 0)
	{
		printf( "ifLcore: nic is null  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);		
	}
	
	/*
	while(1)
	{
		printf( "ifLcore: waitng  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		usleep( 999999);
	}
	*/
	
	
	while( dstack->whileRun)
	{
		received_cnt = 0;

		if( ifc->test_mode > 0)
		{
			if( rte_pktmbuf_alloc_bulk( ifc->nic->mempool, mbufs, ifc->iRxBurstSize) == 0)
			{
				received_cnt = ifc->iRxBurstSize;
				ifc->rx_packets_rec++;					// received times
			}
		}
		else
		{
			if( ifc->nic->started == 1)
			{
				received_cnt = rte_eth_rx_burst( ifc->nic->port_id, ifc->queueid, mbufs, ifc->iRxBurstSize);
				ifc->rx_packets_rec++;						// received times
			}
			else
			{
				usleep(111111);
			}
		}

		
		enquee_received_packets = 0;
		
		if( received_cnt > 0)
		{
			if( ifc->target_if->test_mode == 0 && dstack->TestStage == 0)
			{
				enquee_received_packets = 1;
			}
			else
			{
				if( dstack->TestStage == 1)
				{
					ifc->rx_packets += received_cnt;
					rte_pktmbuf_free_bulk( mbufs, received_cnt);
				}
				else if( dstack->TestStage >= 2)
				{
					enquee_received_packets = 1;
				}
			}
			
			if( enquee_received_packets == 1)
			{
				ifc->rx_packets += received_cnt;		// no of packets received
				pkts_send = 0;
				
				do {
					pkts_send += rte_ring_enqueue_burst( wk_ring, (void **)&mbufs[pkts_send], received_cnt - pkts_send, NULL);
					ifc->rx_packets_enq++;				// enqueed times
				} while( pkts_send < received_cnt);
				
				received_cnt = 0;
			}
		}

		
		if( ifc->TxCore == 0)
		{
			dpdk__iflcore_tx_func( ifc);
		}
		/**/
		
		if( dstack->TestSleep > 0)
		{
			usleep( dstack->TestSleep * 999999);
		}
	}

	return 0;
}


int dpdk__core_sched( void * args)
{
	return 0;
}

int dpdk__access_sched( void * args)
{
	return 0;
}


int dpdk__send_arp_response( dpdkif_t * dif, struct rte_mbuf * pkt)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( dif->nic->mempool);
	
	if( resp_m_buf)
	{
		/*
		printf( "sending arp response mac=%02X:%02X:%02X:%02X:%02X:%02X|%u\n", dif->nic->SrcMAC[0] & 0xFF, 
			dif->nic->SrcMAC[1] & 0xFF, dif->nic->SrcMAC[2] & 0xFF,dif->nic->SrcMAC[3] & 0xFF,
			dif->nic->SrcMAC[4] & 0xFF, dif->nic->SrcMAC[5] & 0xFF,
		dif->nic->ipv4);
		*/
		
		dpdk_pkt__encode_arp_response( pkt, resp_m_buf, dif->nic->SrcMAC, dif->nic->ipv4);
		//rte_ring_enqueue( dif->target_if->tx_ring, resp_m_buf);
		rte_ring_enqueue( dif->tx_ring, resp_m_buf);
	}
	
	return 0;
}



int dpdk__pkt_worker( void * args)
{
	dpdkwt_t * wkt = (dpdkwt_t *) args;
	dpdkif_t * dif = wkt->iff;
	
	struct rte_ring * wk_ring = dstack->access_wk_ring;
	//int wk_ring_size = dstack->access_wk_ring_size;

	if( dif->type == 1)
	{
		wk_ring 		= dstack->core_wk_ring;
		//wk_ring_size 	= dstack->core_wk_ring_size;
	}
	
	if(!wk_ring)
	{
		printf("wk_ring is null  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}

	
	int recv_pkts = 0;
	int pkts_send = 0;
	int i = 0;
	int sts = 0;
	
	int send = 0;
	int pkt_drop = 0;
	int pkt_fwd = 0;
	int pkt_buf = 0;
	int pkt_sch = 0;
	int pkt_arp = 0;
	
	struct rte_mbuf *     pkts[dif->iRxBurstSize];
	
	struct rte_mbuf * pkts_fwd[dif->iRxBurstSize];
	struct rte_mbuf * pkts_buf[dif->iRxBurstSize];
	struct rte_mbuf * pkts_sch[dif->iRxBurstSize];
	struct rte_mbuf * pkts_arp[dif->iRxBurstSize];
	struct rte_mbuf * pkts_drp[dif->iRxBurstSize];
	
	dpdk_pkt_info_t info;

	while( dif->nic->started == 0)
	{
		printf("worker thread waiting for nic status to be active  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		usleep(999999);
	}
	

	
	while( dstack->whileRun)
	{
		pkts_send = 0;
		recv_pkts = rte_ring_dequeue_burst( wk_ring, (void **)pkts, dif->iRxBurstSize, NULL);
		
		if( recv_pkts > 0)
		{
			wkt->rx_packets += recv_pkts;
			
			if( dstack->TestStage == 2 && dif->test_mode > 0)
			{
				rte_pktmbuf_free_bulk( pkts, recv_pkts);
				continue;
			}
			
			pkt_drop = 0;
			pkt_fwd = 0;
			pkt_buf = 0;
			pkt_sch = 0;
			pkt_arp = 0;

			for( i = 0; i < recv_pkts; i++)
			{
				info.iff = dif;
				
				if( dif->type == 0)
				{
					sts = dpdk_access__process_pkt( pkts[i], &info);
				}
				else
				{
					sts = dpdk_core__process_pkt( pkts[i], &info);
				}
				
				//info.result = PKT_FWD__ALLOW;
				
				switch( info.result)
				{
					case PKT_FWD__ALLOW:
						{
							pkts_fwd[pkt_fwd] = pkts[i];
							pkt_fwd++;
						}
						break;
					case PKT_FWD__SCH:
						{
							if( dstack->Scheduler == 1)
							{
								pkts_sch[pkt_sch] = pkts[i];
								pkt_sch++;
							}
							else
							{
								pkts_fwd[pkt_fwd] = pkts[i];
								pkt_fwd++;
							}
						}
						break;
					case PKT_FWD__BUFFER:
						{
							pkts_buf[pkt_buf] = pkts[i];
							pkt_buf++;
						}
						break;
					case PKT_FWD__ARP:
						{
							//printf( "ipv4=%u targetip=%u|%u\n", dif->nic->ipv4, info.arp_targetip4, htonl(info.arp_targetip4));
							//exit(0);
							
							if( dif->nic->ipv4 == info.arp_targetip4)
							{
								//printf( "found gtp packet  %u|%u\n", dif->nic->ipv4, info.arp_targetip4);
								pkts_arp[pkt_arp] = pkts[i];
								pkt_arp++;
							}
							else
							{
								pkts_drp[pkt_drop] = pkts[i];
								pkt_drop++;
							}
						}
						break;
					case PKT_FWD__DROP:							// fall through	
					default:
						{
							pkts_drp[pkt_drop] = pkts[i];
							pkt_drop++;
						}
						break;
				}
			}
			
			
			if( pkt_fwd > 0)
			{
				pkts_send = 0;
				while( pkts_send < pkt_fwd)
				{
					send = rte_ring_enqueue_burst( dif->target_if->tx_ring, (void **)&pkts_fwd[pkts_send], pkt_fwd - pkts_send, NULL);
					pkts_send += send;
					wkt->fwd_packets += send;
				}
			}

			
			pkts_send = 0;
			if( pkt_sch > 0)
			{
				while( pkts_send < pkt_sch)
				{
					send = rte_ring_enqueue_burst( dif->target_if->tx_ring, (void **)&pkts_sch[pkts_send], pkt_sch - pkts_send, NULL);
					pkts_send += send;
					wkt->sch_packets += send;
				}
			}
			
			
			pkts_send = 0;
			if( pkt_buf > 0)
			{
				while( pkts_send < pkt_buf)
				{
					send = rte_ring_enqueue_burst( dif->target_if->tx_ring, (void **)&pkts_buf[pkts_send], pkt_buf - pkts_send, NULL);
					pkts_send += send;
					wkt->buffered_packets += send;
				}
			}
			
			pkts_send = 0;
			if( pkt_arp > 0)
			{
				while( pkts_send < pkt_arp)
				{
					dpdk__send_arp_response( dif, pkts_arp[pkts_send]);
					rte_pktmbuf_free( pkts_arp[pkts_send]);
					pkts_send++;
					wkt->arp_packets++;
				}
			}
			
			if( pkt_drop > 0)
			{
				rte_pktmbuf_free_bulk( pkts_drp, pkt_drop);
				wkt->dropped_packets += pkt_drop;				
			}
		}
	}
	return 0;
}

int dpdk__core_worker__dep( void * args)
{
	dpdkif_t * dif = dstack->coreIfHead;
	
	struct rte_ring * wk_ring = dstack->core_wk_ring;
	int wk_ring_size = dstack->core_wk_ring_size;
	
	int recv_pkts = 0;
	int pkts_send = 0;
	
	struct rte_mbuf * pkts[wk_ring_size];
	
	while( dstack->whileRun)
	{
		//usleep( 999999);
		
		pkts_send = 0;
		//printf("wx_ring=%p rte_ring_count=%d wk_ring_size=%d  %s|%d\n", wk_ring, rte_ring_count( wk_ring), wk_ring_size, __FILE__, __LINE__);
		recv_pkts = rte_ring_dequeue_burst( wk_ring, (void **)pkts, wk_ring_size, NULL);
		
		if( recv_pkts > 0)
		{
			// dstack->wk_processed_pkts += recv_pkts;
			// rte_pktmbuf_free_bulk( pkts, recv_pkts);
			
			
			if( dstack->Scheduler == 1)
			{
				
			}
			else
			{
				pkts_send = 0;
				do {
					pkts_send += rte_ring_enqueue_burst( dif->target_if->tx_ring, (void **)&pkts[pkts_send], recv_pkts - pkts_send, NULL);
				} while( pkts_send < recv_pkts);
				dstack->wk_processed_pkts += pkts_send;
				
				//printf("pkts_send=%d tx_ring=%p rte_ring_count=%d  %s|%d\n", pkts_send, dif->target_if->tx_ring, rte_ring_count( dif->target_if->tx_ring), __FILE__, __LINE__);
			}
			
		}
	}
	return 0;
}


void dpdk__log_pref_counters()
{
	dpdkif_t * dif = NULL;
	dpdkwt_t * wrk = NULL;
	
	uint64_t wrk_totalpackets = 0;
	uint64_t wrk_cTotalpackets = 0;
	uint64_t wrk_cPackets = 0;

	uint64_t tx_totalpackets = 0;
	uint64_t tx_cTotalpackets = 0;
	uint64_t tx_iTotalpackets = 0;

	uint32_t totalsecs = 0;
	
	if(!dstack)
		return;
	
	dif = dstack->coreIfHead;
	wrk_cTotalpackets = 0;
	tx_cTotalpackets = 0;
	tx_iTotalpackets = 0;

	
	while( dif)
	{
		if( dif->nic->mempool)
		{
			tx_cTotalpackets = (dif->tx_packets - dif->last_tx_packets);

			dpdk__plog( "type=%-3u qid=%-3u rx_pkts=%-14lu rec=%-14lu enq=%-14lu tx_pkts=%-14lu | %-14lu  mempool[total=%-6d inuse=%-6d]", 
				dif->type, dif->queueid, dif->rx_packets, dif->rx_packets_rec, dif->rx_packets_enq, 
				dif->tx_packets, tx_cTotalpackets, rte_mempool_avail_count( dif->nic->mempool), rte_mempool_in_use_count( dif->nic->mempool));
			
			dif->last_tx_packets = dif->tx_packets;
			tx_iTotalpackets += tx_cTotalpackets;

			wrk = dif->wk_head;
			
			while( wrk)
			{
				wrk_cPackets = (wrk->rx_packets - wrk->last_rx_packets);
				
				dpdk__plog("id=%d  rx_packets=%-10lu|%-10lu  fwd_packets=%-10lu  dropped_packets=%-10lu  buffered_packets=%-10lu  arp_packets=%-10lu  sch_packets=%-10lu ", 
					wrk->id, wrk->rx_packets, wrk_cPackets, wrk->fwd_packets, wrk->dropped_packets, wrk->buffered_packets, wrk->arp_packets, wrk->sch_packets);
				
				wrk_cTotalpackets += wrk_cPackets;
				wrk->last_rx_packets = wrk->rx_packets;
				wrk = wrk->Next;
			}
		}
		dif = dif->Next;
	}

	
	//if( dstack->PrintStatsOnConsole == 1)
	//	printf("\n");
	
	dif = dstack->accessIfHead;
	while( dif)
	{
		if( dif->nic->mempool && dstack->PrintStatsOnConsole == 1)
		{
			tx_cTotalpackets = (dif->tx_packets - dif->last_tx_packets);
			
			dpdk__plog( "type=%-3u qid=%-3u rx_pkts=%-14lu rec=%-14lu enq=%-14lu tx_pkts=%-14lu | %-14lu  mempool[total=%-6d inuse=%-6d] ", 
				dif->type, dif->queueid, dif->rx_packets, dif->rx_packets_rec, dif->rx_packets_enq, 
				dif->tx_packets, tx_cTotalpackets, rte_mempool_avail_count( dif->nic->mempool), rte_mempool_in_use_count( dif->nic->mempool));
			
			dif->last_tx_packets = dif->tx_packets;
			tx_iTotalpackets += tx_cTotalpackets;
			
			wrk = dif->wk_head;
			
			while( wrk)
			{
				wrk_cPackets = (wrk->rx_packets - wrk->last_rx_packets);
				
				dpdk__plog("id=%d  rx_packets=%-10lu|%-10lu  fwd_packets=%-10lu  dropped_packets=%-10lu  buffered_packets=%-10lu  arp_packets=%-10lu  sch_packets=%-10lu ", 
					wrk->id, wrk->rx_packets, wrk_cPackets, wrk->fwd_packets, wrk->dropped_packets, wrk->buffered_packets, wrk->arp_packets, wrk->sch_packets);
				
				wrk_cTotalpackets += wrk_cPackets;
				wrk->last_rx_packets = wrk->rx_packets;
				wrk = wrk->Next;
			}
		}
		dif = dif->Next;
	}
	
	wrk_totalpackets += wrk_cTotalpackets;
	tx_totalpackets	 += tx_iTotalpackets;
	totalsecs++;
	
	dpdk__plog("Sec=%-6u ", totalsecs);
	dpdk__plog("Workers Total Packets=%-14lu | LifeTime Packets=%-14lu  | LifeTime Average=%-14lu", 
		wrk_cTotalpackets, wrk_totalpackets, wrk_totalpackets / totalsecs );
	dpdk__plog("Transmi Total Packets=%-14lu | LifeTime Packets=%-14lu  | LifeTime Average=%-14lu", 
		tx_iTotalpackets, tx_totalpackets, tx_totalpackets / totalsecs );
	
	//printf( "processed wk=%-10lu sh=%-10lu dropped=%-lu\n", dstack->wk_processed_pkts, dstack->sh_processed_pkts, dstack->wk_dropped_pkts);
	// printf( "dpdk-lib: mempool-available count: %d in-use-count: %d  %s|%d\n", 
		// rte_mempool_avail_count( dstack->mempool), rte_mempool_in_use_count(dstack->mempool), __FILE__, __LINE__);

	dpdk__plog( "---------------------------------------------------------------------------");
}



void * dpdk__run_pref_thread()
{
	dpdkif_t * dif = NULL;
	dpdkwt_t * wrk = NULL;
	
	uint64_t wrk_totalpackets = 0;
	uint64_t wrk_cTotalpackets = 0;
	uint64_t wrk_cPackets = 0;

	uint64_t tx_totalpackets = 0;
	uint64_t tx_cTotalpackets = 0;
	uint64_t tx_iTotalpackets = 0;

	
	uint32_t totalsecs = 0;
	
	while( dstack->whileRun)
	{
		dif = dstack->coreIfHead;
		wrk_cTotalpackets = 0;
		tx_cTotalpackets = 0;
		tx_iTotalpackets = 0;
		
		while( dif)
		{
			if( dif->nic->mempool)
			{
				tx_cTotalpackets = (dif->tx_packets - dif->last_tx_packets);

				if( dstack->PrintStatsOnConsole == 1)
				{
					printf( "type=%-3u qid=%-3u rx_pkts=%-14lu rec=%-14lu enq=%-14lu tx_pkts=%-14lu | %-14lu  mempool[total=%-6d inuse=%-6d]\n", 
						dif->type, dif->queueid, dif->rx_packets, dif->rx_packets_rec, dif->rx_packets_enq, 
						dif->tx_packets, tx_cTotalpackets, rte_mempool_avail_count( dif->nic->mempool), rte_mempool_in_use_count( dif->nic->mempool));
				}
				
				dif->last_tx_packets = dif->tx_packets;
				tx_iTotalpackets += tx_cTotalpackets;

				wrk = dif->wk_head;
				
				while( wrk)
				{
					wrk_cPackets = (wrk->rx_packets - wrk->last_rx_packets);
					
					if( dstack->PrintStatsOnConsole == 1)
					{
						printf("id=%d  rx_packets=%-10lu|%-10lu  fwd_packets=%-10lu  dropped_packets=%-10lu  buffered_packets=%-10lu  arp_packets=%-10lu  sch_packets=%-10lu \n", 
							wrk->id, wrk->rx_packets, wrk_cPackets, wrk->fwd_packets, wrk->dropped_packets, wrk->buffered_packets, wrk->arp_packets, wrk->sch_packets);
					}
					
					wrk_cTotalpackets += wrk_cPackets;
					wrk->last_rx_packets = wrk->rx_packets;
					wrk = wrk->Next;
				}
			}
			dif = dif->Next;
		}

		
		if( dstack->PrintStatsOnConsole == 1)
			printf("\n");
		
		dif = dstack->accessIfHead;
		while( dif)
		{
			if( dif->nic->mempool && dstack->PrintStatsOnConsole == 1)
			{
				tx_cTotalpackets = (dif->tx_packets - dif->last_tx_packets);
				
				if( dstack->PrintStatsOnConsole == 1)
				{
					printf( "type=%-3u qid=%-3u rx_pkts=%-14lu rec=%-14lu enq=%-14lu tx_pkts=%-14lu | %-14lu  mempool[total=%-6d inuse=%-6d] \n", 
						dif->type, dif->queueid, dif->rx_packets, dif->rx_packets_rec, dif->rx_packets_enq, 
						dif->tx_packets, tx_cTotalpackets, rte_mempool_avail_count( dif->nic->mempool), rte_mempool_in_use_count( dif->nic->mempool));
				}
				
				dif->last_tx_packets = dif->tx_packets;
				tx_iTotalpackets += tx_cTotalpackets;
				
				wrk = dif->wk_head;
				
				while( wrk)
				{
					wrk_cPackets = (wrk->rx_packets - wrk->last_rx_packets);
					
					if( dstack->PrintStatsOnConsole == 1)
					{
						printf("id=%d  rx_packets=%-10lu|%-10lu  fwd_packets=%-10lu  dropped_packets=%-10lu  buffered_packets=%-10lu  arp_packets=%-10lu  sch_packets=%-10lu \n", 
							wrk->id, wrk->rx_packets, wrk_cPackets, wrk->fwd_packets, wrk->dropped_packets, wrk->buffered_packets, wrk->arp_packets, wrk->sch_packets);
					}
					
					wrk_cTotalpackets += wrk_cPackets;
					wrk->last_rx_packets = wrk->rx_packets;
					wrk = wrk->Next;
				}
			}
			dif = dif->Next;
		}
		
		wrk_totalpackets += wrk_cTotalpackets;
	    tx_totalpackets	 += tx_iTotalpackets;
		totalsecs++;
		
		if( dstack->PrintStatsOnConsole == 1)
		{
			printf("Sec=%-6u \n", totalsecs);
			printf("Workers Total Packets=%-14lu | LifeTime Packets=%-14lu  | LifeTime Average=%-14lu\n", 
				wrk_cTotalpackets, wrk_totalpackets, wrk_totalpackets / totalsecs );
			printf("Transmi Total Packets=%-14lu | LifeTime Packets=%-14lu  | LifeTime Average=%-14lu\n", 
				tx_iTotalpackets, tx_totalpackets, tx_totalpackets / totalsecs );
			
			//printf( "processed wk=%-10lu sh=%-10lu dropped=%-lu\n", dstack->wk_processed_pkts, dstack->sh_processed_pkts, dstack->wk_dropped_pkts);
			// printf( "dpdk-lib: mempool-available count: %d in-use-count: %d  %s|%d\n", 
				// rte_mempool_avail_count( dstack->mempool), rte_mempool_in_use_count(dstack->mempool), __FILE__, __LINE__);

			printf( "---------------------------------------------------------------------------\n");
		}
		
		sleep(1);
	}
}


void dpdk__start_pref_thread()
{
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	int iRet = pthread_create( &s_pthread_id, &attr, dpdk__run_pref_thread, (void *)NULL);
}

// ./dapp -l 3-9 
// ./dapp -l 0-3
void dpdk__init(int argc, char **argv)
{
	if(!dstack)
	{
		dstack = (dpdkstack_t *)malloc(sizeof(dpdkstack_t));
		memset( dstack, 0, sizeof(dpdkstack_t));
	
		json_error_t error;
		memset( &error, 0, sizeof(json_error_t));
		dstack->config = json_load_file( "./dpdk.machine.config", 0, &error);
		
		if(!dstack->config)
		{
			printf("error loading confile file[%s] line[%d] column[%d] position[%d] [%s][%s]\n", "./dpdk.machine.config", error.line, error.column, error.position, error.source, error.text);
			exit(0);
		}

		//dstack->mempool_noOfElements = dpdk_config__get_int( dstack->config, "MemPool_NoOfElements", 16384, 16384);;


		#if USENDPI
			upfndpi_set_malloc_fun( rte_malloc_fun);
			upfndpi_Initalize( 10, 100);
		#endif


		int ret = rte_eal_init( argc, argv);
		if (ret < 0)
		{
			rte_panic("Cannot init EAL\n");
			exit(0);
		}
		
		// int access_rx_ring_count = 128;
		int access_wk_ring_count = 128;
		int access_sh_ring_count = 128;
		
		// int core_tx_ring_count = 128;
		int core_wk_ring_count = 128;
		int core_sh_ring_count = 128;
		
		dstack->Scheduler 			= dpdk_config__get_int( dstack->config, "Scheduler", 0, 0);
		dstack->iPacketTest_Enable	= dpdk_config__get_int( dstack->config, "PacketTest_Enable", 0, 0);
		dstack->TestStage			= dpdk_config__get_int( dstack->config, "TestStage", 0, 0);
		dstack->TestSleep			= dpdk_config__get_int( dstack->config, "TestSleep", 0, 0);

		
		json_t * ring_config = json_object_get( dstack->config, "Rings");
		
		if( ring_config)
		{
			json_t * access_config = json_object_get( ring_config, "Access");
			if( access_config)
			{
				//access_rx_ring_count = dpdk_config__get_int( access_config, "RxRingSize", access_rx_ring_count, access_rx_ring_count);
				access_wk_ring_count = dpdk_config__get_int( access_config, "WxRingSize", access_wk_ring_count, access_wk_ring_count);
				access_sh_ring_count = dpdk_config__get_int( access_config, "ShRingSize", access_sh_ring_count, access_sh_ring_count);
			}
			
			json_t * core_config = json_object_get( ring_config, "Core");
			if( core_config)
			{
				//core_tx_ring_count = dpdk_config__get_int( core_config, "RxRingSize", core_tx_ring_count, core_tx_ring_count);
				core_wk_ring_count = dpdk_config__get_int( core_config, "WxRingSize", core_wk_ring_count, core_wk_ring_count);
				core_sh_ring_count = dpdk_config__get_int( core_config, "ShRingSize", core_sh_ring_count, core_sh_ring_count);
			}			
		}
		
		dstack->access_wk_ring = rte_ring_create( "a_wk_r", access_wk_ring_count, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
		dstack->access_sh_ring = rte_ring_create( "a_sh_r", access_sh_ring_count, SOCKET_ID_ANY, RING_F_MP_RTS_ENQ | RING_F_MC_RTS_DEQ);

		dstack->core_wk_ring = rte_ring_create( "c_rx_r", core_wk_ring_count, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);
		dstack->core_sh_ring = rte_ring_create( "c_sx_r", core_sh_ring_count, SOCKET_ID_ANY, RING_F_MP_RTS_ENQ | RING_F_MC_RTS_DEQ);
		
		dstack->access_wk_ring_size = access_wk_ring_count;
		dstack->access_sh_ring_size = access_sh_ring_count;
		dstack->core_wk_ring_size = core_wk_ring_count;
		dstack->core_sh_ring_size = core_sh_ring_count;
		
		if(!dstack->access_wk_ring)
		{
			printf("ring creation failed for access_wk_ring with count=%d\n", access_wk_ring_count);
			exit(0);
		}
		
		if(!dstack->access_sh_ring)
		{
			printf("ring creation failed for access_sh_ring with count=%d\n", access_sh_ring_count);
			exit(0);
		}
		
		if(!dstack->core_wk_ring)
		{
			printf("ring creation failed for core_wk_ring with count=%d\n", core_wk_ring_count);
			exit(0);
		}

		if(!dstack->core_sh_ring)
		{
			printf("ring creation failed for core_sh_ring with count=%d\n", core_sh_ring_count);
			exit(0);
		}		

		json_t * jWorkers 					= json_object_get( dstack->config, "Workers");
		int iAccessWTCount = 0;
		int iCoreWTCount = 0;
		
		dstack->PrintStatsOnConsole			= dpdk_config__get_int( dstack->config, "PrintStatsOnConsole", 0, 0);
		dstack->DedicatedPerfThread			= dpdk_config__get_int( dstack->config, "DedicatedPerfThread", 0, 0);
		printf( "available-core-count=%u socket-count=%d\n", rte_lcore_count(), rte_socket_count());

		dstack->SessionCount				= dpdk_config__get_int( dstack->config, "SessionCount", 1000, 1000);
		dstack->Supported_IPV				= dpdk_config__get_int( dstack->config, "Supported_IPV", 1, 1);
		dstack->FlowTableCount				= dpdk_config__get_int( dstack->config, "FlowTableCount", 3, 5);
		dstack->FlowsPerFlowTable			= dpdk_config__get_int( dstack->config, "FlowsPerFlowTable", 1000, 1000);
		dstack->TotalFlows					= dpdk_config__get_int( dstack->config, "TotalFlows", 10000, 10000);
		dstack->enable_packet_inspection 	= dpdk_config__get_int( dstack->config, "EnablePacketInspection", 0, 0);
		
		printf( "DPI:: EnablePacketInspection=%d\n", dstack->enable_packet_inspection);
		printf( "PFCP:: SessionCount=%u   ipv=%u   FlowTableCount=%u   FlowsPerFlowTable=%u   TotalFlows=%u\n", 
			dstack->SessionCount, dstack->Supported_IPV, dstack->FlowTableCount, dstack->FlowsPerFlowTable, dstack->TotalFlows);


		dstack->accessIfHead = NULL;
		dstack->accessIfCurrent = NULL;
		dstack->accessIfCount = 0;

		dstack->coreIfHead = NULL;
		dstack->coreIfCurrent = NULL;
		dstack->coreIfCount = 0;
		
		dstack->wk_processed_pkts = 0;
		dstack->sh_processed_pkts = 0;

		dstack->wk_dropped_pkts = 0;
		dstack->sh_dropped_pkts = 0;
		dstack->enable_packet_inspection = 0;

		json_t * jInterfaces = json_object_get( dstack->config, "Interfaces");
		
		if( jInterfaces)
		{
			if( json_is_array( jInterfaces))
			{
				int count = json_array_size( jInterfaces);
				int iif = 0;
				json_t * itemConfig = NULL;
				
				for( iif = 0; iif < count; iif++)
				{
					itemConfig = json_array_get( jInterfaces, iif);
					
					if( itemConfig)
					{
						int type 			= dpdk_config__get_int( itemConfig, "Type", 0, 0);
						
						if( type >= 0 && type < 2)
						{
							char sBusID[100];
							memset( sBusID, 0, sizeof(sBusID));

							int id 			 		= dpdk_config__get_int( itemConfig, "ID", -1, 0);
							//int rxRingSize 	 		= dpdk_config__get_int( itemConfig, "RxRingSize", -1, 0);
							int txRingSize 	 		= dpdk_config__get_int( itemConfig, "TxRingSize", -1, 0);
							int test_mode	 		= dpdk_config__get_int( itemConfig, "TestMode", -1, 0);
							int iMemPoolSize 		= dpdk_config__get_int( itemConfig, "MemPoolSize", 8192, 4096);
							int iCacheSize 			= dpdk_config__get_int( itemConfig, "CacheSize", 4096, 128);
							int IsGTPEncapsulated	= dpdk_config__get_int( itemConfig, "GTPE", 0, 0);

							json_t * jBusID 		= json_object_get( itemConfig, "BusID");
							char * cBusId			= (char *)json_string_value( jBusID);
							json_t * qQueues 		= json_object_get( itemConfig, "Queues");
							json_t * tQueues 		= json_object_get( itemConfig, "TQueues");

							
							json_t * jSrcMac = json_object_get( itemConfig, "SrcMAC");
							json_t * jDstMac = json_object_get( itemConfig, "DstMAC");
							
							printf("id=%d txRingSize=%d cBusId=%p qQueues=%p \n", id, txRingSize, cBusId, qQueues);
							//rxRingSize > 0 && 
							
							if( id >= 0 && txRingSize > 0 && cBusId && qQueues && tQueues && jDstMac && jSrcMac)
							{
								if( json_is_array(qQueues) && json_is_array(tQueues))
								{
									int qCount = json_array_size( qQueues);
									int tCount = json_array_size( tQueues);
									
									if( qCount != tCount)
									{
										printf("Queues and TQueues count should match\n");
										dpdk__initexit();
										exit(0);
									}
									
									char * cSrcMac = (char *)json_string_value( jSrcMac);
									char * cDstMac = (char *)json_string_value( jDstMac);
									
									uint32_t u8SrcMac[7];
									uint32_t u8DstMac[7];
									sscanf( cSrcMac, "%x:%x:%x:%x:%x:%x", &u8SrcMac[0], &u8SrcMac[1], &u8SrcMac[2], &u8SrcMac[3], &u8SrcMac[4], &u8SrcMac[5]);
									sscanf( cDstMac, "%x:%x:%x:%x:%x:%x", &u8DstMac[0], &u8DstMac[1], &u8DstMac[2], &u8DstMac[3], &u8DstMac[4], &u8DstMac[5]);
									
									//printf("SRCMAC %02x:%02x:%02x:%02x:%02x:%02x\n", u8SrcMac[0], u8SrcMac[1], u8SrcMac[2], u8SrcMac[3], u8SrcMac[4], u8SrcMac[5]);
									//printf("DSTMAC %02x:%02x:%02x:%02x:%02x:%02x\n", u8DstMac[0], u8DstMac[1], u8DstMac[2], u8DstMac[3], u8DstMac[4], u8DstMac[5]);
									
									int qi = 0;
									
									dpdkif_nic_t * nic = (dpdkif_nic_t *) malloc( sizeof( dpdkif_nic_t));
									memset( nic, 0, sizeof(dpdkif_nic_t));
									
									nic->SrcMAC[0] = u8SrcMac[0];
									nic->SrcMAC[1] = u8SrcMac[1];
									nic->SrcMAC[2] = u8SrcMac[2];
									nic->SrcMAC[3] = u8SrcMac[3];
									nic->SrcMAC[4] = u8SrcMac[4];
									nic->SrcMAC[5] = u8SrcMac[5];

									nic->DstMAC[0] = u8DstMac[0];
									nic->DstMAC[1] = u8DstMac[1];
									nic->DstMAC[2] = u8DstMac[2];
									nic->DstMAC[3] = u8DstMac[3];
									nic->DstMAC[4] = u8DstMac[4];
									nic->DstMAC[5] = u8DstMac[5];									
									
									nic->ValidateSrcMAC 	= dpdk_config__get_int( itemConfig, "ValidateSrcMAC", 0, 0);
									//dif->BurstSize		= dpdk_config__get_int( itemConfig, "BurstSize", 128, 128);
									nic->WorkerThreadCount	= dpdk_config__get_int( itemConfig, "WTC", 1, 1);
									nic->CreateOuterHeader	= dpdk_config__get_int( itemConfig, "OHT", 0, 0);
									nic->IsGTPEncapsulated = IsGTPEncapsulated;
									nic->started = 0;
									nic->iMemPoolSize = iMemPoolSize;
									nic->iCacheSize = iCacheSize;
									nic->id = iif;
									
									json_t * jIPv4 			= json_object_get( itemConfig, "IPv4");
									char * cIPv4			= (char *)json_string_value( jIPv4);
									
									if( cIPv4)
									{
										inet_pton( AF_INET, cIPv4, &nic->ipv4);
									}
									// nic->ipv4				= dpdk_config__get_int( itemConfig, "IPv4", 0, 0);
									// nic->ipv6				= dpdk_config__get_int( itemConfig, "IPv6", 0, 0);
									nic->DedicatedCoreForRxQ	= dpdk_config__get_int( itemConfig, "DedicatedCoreForRxQ", 0, 0);
									nic->DedicatedCoreForTxQ	= dpdk_config__get_int( itemConfig, "DedicatedCoreForTxQ", 0, 0);
									nic->type 					= type;
									nic->tx_q_count				= 0;
									nic->rx_q_count				= 0;
									nic->iRxBurstSize 			= dpdk_config__get_int( itemConfig, "RxBurstSize", 128, 128);
									nic->iTxBurstSize 			= dpdk_config__get_int( itemConfig, "TxBurstSize", 128, 128);

									for( qi = 0; qi < qCount; qi++)
									{
										json_t * qConfig = json_array_get( qQueues, qi);
										json_t * tqConfig = json_array_get( tQueues, qi);
										
										int qid = json_integer_value( qConfig);
										int tqid = json_integer_value( tqConfig);
										
										if( qid >= 0)
										{
											//printf( "type=%d id=%d cBusId=%s rxRingSize=%d qid=%d\n", type, id, cBusId, rxRingSize, qid);
										
											dpdkif_t * dif = (dpdkif_t *)malloc( sizeof( dpdkif_t));
											memset( dif, 0, sizeof(dpdkif_t));
											
											dif->nic = nic;
											dif->id = id;
											dif->type = type;
											dif->queueid = qid;
											dif->tqueueid = tqid;
											
											nic->tx_q_count++;
											nic->rx_q_count++;


											
											//printf( "BurstSize=%d \n", dif->BurstSize);
	
											if( type == 0)
											{
												iAccessWTCount += dif->nic->WorkerThreadCount;
											}
											else
											{
												iCoreWTCount += dif->nic->WorkerThreadCount;
											}

											strcpy( dif->nic->ifid, cBusId);
											char ring_name[30];
											
											// memset(  ring_name, 0, sizeof(ring_name));
											// sprintf( ring_name, "ifr-rx-%d-%d-%d", type, id, qid);
											//dif->rx_ring = rte_ring_create( ring_name, rxRingSize, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_MC_RTS_DEQ);
											//dif->rx_ring_size = rxRingSize;
											
											memset(  ring_name, 0, sizeof(ring_name));
											sprintf( ring_name, "ifr-tx-%d-%d-%d", type, id, qid);
											dif->tx_ring = rte_ring_create( ring_name, txRingSize, SOCKET_ID_ANY, RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);
											dif->tx_ring_size = txRingSize;
											dif->iMemPoolSize = iMemPoolSize;
											dif->iCacheSize = iCacheSize;
											
											if( test_mode > 0) {
												dif->iRxBurstSize 	= dpdk_config__get_int( itemConfig, "RxBurstSize",   1,   1);
											} else {
												dif->iRxBurstSize 	= dpdk_config__get_int( itemConfig, "RxBurstSize", 128, 128);
											}
											
											dif->iTxBurstSize 	= dpdk_config__get_int( itemConfig, "TxBurstSize", 128, 128);
											dif->TxCore 		= dpdk_config__get_int( itemConfig, "TxCore", 0, 0);
											dif->LogIPHeader 	= dpdk_config__get_int( itemConfig, "LogIPHeader", 0, 0);
											dif->LogSession 	= dpdk_config__get_int( itemConfig, "LogSession", 0, 0);
											dif->LogFlow 		= dpdk_config__get_int( itemConfig, "LogFlow", 0, 0);
											dif->LogPPResult 	= dpdk_config__get_int( itemConfig, "LogPPResult", 0, 0);
											
											dif->rx_packets = 0;
											dif->rx_packets_rec = 0;
											dif->rx_packets_enq = 0;
											dif->tx_packets = 0;
											//dif->wrk_processed_packets = 0;

											dif->test_mode = test_mode;
											
											// if(!dif->rx_ring)
											// {
												// printf("Interfaces Ring Creation Failed\n");
												// dpdk__initexit();
												// exit(0);
											// }
											
											int jwtc = 0;
											for( jwtc = 0; jwtc < dif->nic->WorkerThreadCount; jwtc++)
											{
												dpdkwt_t * wtc = (dpdkwt_t *)malloc( sizeof(dpdkwt_t));
												memset( wtc, 0, sizeof( dpdkwt_t));
												
												wtc->iff = dif;
												wtc->id = jwtc;
												
												if(!dif->wk_head)
												{
													dif->wk_head = dif->wk_current = wtc;
												}
												else
												{
													dif->wk_current->Next = wtc;
													dif->wk_current = wtc;
												}
											}
											
											if( type == 0)
											{
												if(!dstack->accessIfHead)
												{
													dstack->accessIfHead = dstack->accessIfCurrent = dif;
												}
												else
												{
													dstack->accessIfCurrent->Next = dif;
													dstack->accessIfCurrent = dif;
												}
												dstack->accessIfCount++;
												
												if( dif->TxCore == 1)
												{
													dstack->accessIfCount++;
												}
																								
											}
											else if( type == 1)
											{
												if(!dstack->coreIfHead)
												{
													dstack->coreIfHead = dstack->coreIfCurrent = dif;
												}
												else
												{
													dstack->coreIfCurrent->Next = dif;
													dstack->coreIfCurrent = dif;
												}
												dstack->coreIfCount++;
												
												if( dif->TxCore == 1)
												{
													dstack->coreIfCount++;
												}
												
												
											}
										}
										
										//
										
										if( nic->type == 1)
										{
											if(!dstack->coreNicHead)
											{
												dstack->coreNicHead = dstack->coreNicCurrent = nic;
											}
											else
											{
												dstack->coreNicCurrent->Next = nic;
												dstack->coreNicCurrent = nic;
											}
											
											dstack->coreNicCount++;
										}
										else if( nic->type == 0)
										{
											if(!dstack->accessNicHead)
											{
												dstack->accessNicHead = dstack->accessNicCurrent = nic;
											}
											else
											{
												dstack->accessNicCurrent->Next = nic;
												dstack->accessNicCurrent = nic;
											}
											dstack->accessNicCount++;
										}								

									}
								}
								
									
							}
							else
							{
								printf("Mandatory Information Not Found for Type=%d Id=%d\n", type, id);
								exit(0);
							}
						}
						else
						{
							printf("Type=%d is out of Range expected 0 or 1\n", type);
							exit(0);
						}
					}
				}
			}
		}
		else
		{
			printf("Interfaces Not Configured\n");
			dpdk__initexit();
			exit(0);
		}
		
		if( dstack->coreIfCount == 0)
		{
			printf("CoreIf Not Configured\n");
			dpdk__initexit();
			exit(0);
		}
		
		if( dstack->accessIfCount == 0)
		{
			printf("AccessIf Not Configured\n");
			dpdk__initexit();
			exit(0);			
		}
		
		int totalCoresRequired = (dstack->coreIfCount + dstack->accessIfCount + iAccessWTCount + iCoreWTCount);
		if( rte_lcore_count() < totalCoresRequired)
		{
			printf( "exting:  Insufficient Cores, Required=%d Available=%d ----\n", totalCoresRequired, rte_lcore_count());
			dpdk__initexit();
			exit(0);
		}
		else
		{
			printf( "totalCoresRequired=%d available-cores=%d  %s|%s|%d\n", totalCoresRequired, rte_lcore_count(), __FILE__, __FUNCTION__, __LINE__);
		}
		
		json_t * ratGroupConfig = json_object_get( dstack->config, "DPI_MAP");
		
		if( ratGroupConfig)
		{
			if( json_is_array(ratGroupConfig))
			{
				int rgsize = json_array_size( ratGroupConfig);
				int rgi = 0;
				
				for( rgi = 0; rgi < rgsize; rgi++)
				{
					json_t * itemObj = json_array_get( ratGroupConfig, rgi);
					
					if( itemObj)
					{
						json_t * dpiObj = json_object_get( itemObj, "DPI");
						json_t * rgidObj = json_object_get( itemObj, "RG");
					
						if( dpiObj && rgidObj)
						{
							uint16_t dpi_id = dpdk_config__get_int( itemObj, "DPI", 0, 0);
							uint16_t rg_id = dpdk_config__get_int( itemObj, "RG", 0, 0);
						
							if( dpi_id > 0 && rg_id > 0)
							{
								dpdk_add_rgid( rg_id, dpi_id, 0);
								printf( "dpi-id=%u rg-id=%u\n", dpi_id, rg_id);
							}
						}
					}
				}
			}
		}
		
		
		
		dpdk_session__init(dstack->SessionCount, dstack->Supported_IPV, dstack->FlowTableCount, dstack->FlowsPerFlowTable, dstack->TotalFlows);
		
		
		dpdkif_t * dif 	= dstack->coreIfHead;
		dpdkif_t * tdif = NULL;
		
		while( dif)
		{
			tdif = dstack->accessIfHead;
			
			while( tdif)
			{
				if( dif->tqueueid == tdif->queueid)
				{
					dif->target_if = tdif;
					break;
				}
				tdif = tdif->Next;
			}
			dif = dif->Next;
		}


		dif  = dstack->accessIfHead;
		tdif = NULL;
		
		while( dif)
		{
			tdif = dstack->coreIfHead;
			
			while( tdif)
			{
				if( dif->tqueueid == tdif->queueid)
				{
					dif->target_if = tdif;
					break;
				}
				tdif = tdif->Next;
			}
			dif = dif->Next;
		}
		

		printf("Configured Scheduler=%d  %s|%s|%d\n", dstack->Scheduler, __FILE__, __FUNCTION__, __LINE__);
		
		dstack->whileRun = 1;
		
		
		printf("launching lcores, next lcore-id=%d\n",  rte_get_next_lcore(0,1,0));
		int workerCount = rte_get_next_lcore(0,1,0);
		int launchedCores = 0;
		
		if( dstack->Scheduler == 1)
		{
			rte_eal_remote_launch ( dpdk__core_sched, NULL, workerCount++);
			launchedCores++;
			
			rte_eal_remote_launch ( dpdk__access_sched, NULL, workerCount++);
			launchedCores++;
		}
		
		int i = 0;

		dpdkwt_t * wkt = NULL;
		
		int lwtc = 0;
		dif = dstack->coreIfHead;
		lwtc = 0;
		
		while( dif)
		{
			wkt = dif->wk_head;
			
			while( wkt)
			{
				rte_eal_remote_launch ( dpdk__pkt_worker, wkt, workerCount++);
				launchedCores++;
				wkt = wkt->Next;
				lwtc++;
			}
			dif = dif->Next;
		}
		
		
		printf("launched core-worker-threads=%d  %s|%s|%d\n", lwtc, __FILE__, __FUNCTION__, __LINE__);

		dif = dstack->accessIfHead;
		lwtc = 0;

		while( dif)
		{
			wkt = dif->wk_head;
			
			while( wkt)
			{
				rte_eal_remote_launch ( dpdk__pkt_worker, wkt, workerCount++);
				launchedCores++;
				wkt = wkt->Next;
				lwtc++;
			}
			dif = dif->Next;
		}
		
		printf("launched access-worker-threads=%d  %s|%s|%d\n", lwtc, __FILE__, __FUNCTION__, __LINE__);


		dpdkif_nic_t * coreNic = dstack->coreNicHead;
		int nic_sts = 0;
		
		while( coreNic)
		{
			nic_sts = dpdk_nic__configure_and_start( coreNic);
			
			if( nic_sts != 0) {
				printf("configuring core NIC=%s failed\n", coreNic->ifid);
			} else {
				printf("configuring core NIC=%s is succesfull port-id=%u\n\n", coreNic->ifid, coreNic->port_id);
			}
			coreNic = coreNic->Next;
		}
		
		
		dpdkif_nic_t * accessNic = dstack->accessNicHead;
		while( accessNic)
		{
			nic_sts = dpdk_nic__configure_and_start( accessNic);
			
			if( nic_sts != 0) {
				printf("configuring access NIC=%s failed\n", accessNic->ifid);
			} else {
				printf("configuring access NIC=%s is succesfull port-id=%u\n\n", accessNic->ifid, accessNic->port_id);
			}
			accessNic = accessNic->Next;
		}


		
		
		dif = dstack->coreIfHead;
		while( dif)
		{
			printf( "launching core-rx-lcore:   dif=%p target_if=%p\n", dif, dif->target_if);
			rte_eal_remote_launch ( dpdk__iflcore, dif, workerCount++);
			launchedCores++;
			
			if( dif->TxCore == 1)
			{
				printf( "launching core-tx-lcore:   dif=%p target_if=%p\n", dif, dif->target_if);
				rte_eal_remote_launch ( dpdk__iflcore_tx, dif, workerCount++);
			}
			
			dif = dif->Next;
		}

		dif = dstack->accessIfHead;
		while( dif)
		{
			printf( "launching access-rx-lcore:   dif=%p target_if=%p\n", dif, dif->target_if);
			rte_eal_remote_launch ( dpdk__iflcore, dif, workerCount++);
			launchedCores++;

			if( dif->TxCore == 1)
			{
				printf( "launching access-tx-lcore:   dif=%p target_if=%p\n", dif, dif->target_if);
				rte_eal_remote_launch ( dpdk__iflcore_tx, dif, workerCount++);
			}
			
			dif = dif->Next;
		}
		
		printf("launching lcores - completed[%d]\n", launchedCores);
		
		if( dstack->DedicatedPerfThread == 1)
		{
			dpdk__start_pref_thread();
		}
		
		
		/*
		dif = dstack->coreIfHead;
		while( dif)
		{
			while(!dif->mempool)
			{
				usleep(111111);
			}
			dif = dif->Next;
		}

		dif = dstack->accessIfHead;
		while( dif)
		{
			while(!dif->mempool)
			{
				usleep(111111);
			}
			dif = dif->Next;
		}
		*/
		
		
		if( dstack->iPacketTest_Enable == 1)
		{
			pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./pcap_encoding_test.pcap");
			if(dumper )
			{
				dpdk_pkt__test_case_write_arp_request( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_write_arp_response( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_remove_outer_header_1( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_remove_outer_header_2( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_create_outer_header_ip4( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_create_outer_header_ip6( dumper, dstack->accessIfHead->nic->mempool);
				dpdk_pkt__test_case_create_gtp_packet_ipv4( dumper, dstack->accessIfHead->nic->mempool, 2);
				dpdk_pkt__test_case_create_gtp_packet_ipv6( dumper, dstack->accessIfHead->nic->mempool, 254);
				dpdk_pkt__close_pcap( dumper);
			}
			else
			{
				printf("pcap dump creation failed\n");
			}
		}
		
		// uint16_t port_id = 0;
		// int sts = rte_eth_dev_get_port_by_name( "0000:0b:00.0", &port_id);
		
		//printf( "---------------------sts=%d port_id=%u -----------------------\n", sts, port_id);

		

		// dpdk_parse_pkt__test_case_1( dstack->accessIfHead->mempool, dstack->accessIfHead);
		// dpdk_parse_pkt__test_case_2( dstack->accessIfHead->mempool, dstack->accessIfHead);
		// dpdk_parse_pkt__test_case_3( dstack->accessIfHead->mempool, dstack->coreIfHead);
		// dpdk_parse_pkt__test_case_4( dstack->accessIfHead->mempool, dstack->coreIfHead);
		// dpdk_parse_pkt__test_case_5( dstack->accessIfHead->mempool, dstack->coreIfHead);
		// dpdk_parse_pkt__test_case_6( dstack->accessIfHead->mempool, dstack->accessIfHead);
		// dpdk_parse_pkt__test_case_7( dstack->accessIfHead->mempool, dstack->coreIfHead);
		// dpdk_parse_pkt__test_case_8( dstack->accessIfHead->mempool, dstack->coreIfHead);
		
		// printf("dpdk-lib initialized-22\n");
		
		// printf("-------------------------------------------------------\n");
		// dpdk_nic__print_device_info( "0000:1b:00.0");
		// printf("-------------------------------------------------------\n");
		// dpdk_nic__print_device_info( "0000:13:00.0");
		// printf("-------------------------------------------------------\n");
		
		printf("dpdk-lib initialized-22\n");
	}
}


//  ./dapp -l 3-7













