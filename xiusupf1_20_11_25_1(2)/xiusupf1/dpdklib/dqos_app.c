#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
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
#include <ctype.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <numa.h>
#include <mntent.h>

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
#include <rte_sched.h>

#include <pcap/pcap.h>
#include <pcap/bpf.h>

#include <jansson.h>


int 						dpdk_qos__init( char * configfile);
struct rte_sched_port * 	dpdk_qos__create_port( char * name, int socket, uint64_t link_rate, int side);
uint16_t 					dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes);
uint16_t 					dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes);
void 						dpdk_qos__find_user_pipe_and_q( uint64_t seid, uint16_t * pipe, uint16_t * nongbr_queue, uint16_t * gbr_queue);


typedef struct __test_ue 
{
	struct __test_ue * Next;
	
	uint32_t  ue_id;
	// uint32_t  uplink_speed_bytes;
	// uint32_t  dwlink_speed_bytes;
	
	uint32_t tc_class;
	
	
	uint64_t  uplink_port_received_bytes;
	uint64_t  uplink_port_transmitted_bytes;
	
	uint64_t  uplink_port_received_packets;
	uint64_t  uplink_port_transmitted_packets;

	uint64_t  uplink_port_last_received_bytes;
	uint64_t  uplink_port_last_transmitted_bytes;
	
	uint64_t  uplink_port_last_received_packets;
	uint64_t  uplink_port_last_transmitted_packets;
	
} __test_ue_t;



struct rte_ring 		* qring 							= NULL;
struct rte_mempool 		* mpool 							= NULL;
struct rte_sched_port 	* port 								= NULL;
__test_ue_t 			* ue_head 							= NULL;
__test_ue_t 			* ue_current 						= NULL;

uint64_t				total_sched_port_enquee_failed 		= 0;
uint64_t				current_sched_port_enqueed_failed 	= 0;


__test_ue_t * dqos_app__find_by_id( uint32_t id)
{
	__test_ue_t * ue_item = ue_head;
	
	while( ue_item)
	{
		if( ue_item->ue_id == id)
			return ue_item;
		
		ue_item = ue_item->Next;
	}
	
	return NULL;
}

int dqos_app__sched_core( void * args)
{
	struct rte_mbuf * mbufs[64];
	
	int retval = 0;
	int sched_port_enqueed = 0; 
	int temp = 0; 
	int sched_port_dequeed = 0; 
	int i = 0;
	uint32_t ue_id = 0;
	__test_ue_t * ue = NULL;
	
	while(1)
	{
		retval = rte_ring_sc_dequeue_bulk( qring, (void **)mbufs, 64, NULL);		
		
		if (retval > 0) 
		{
			sched_port_enqueed = rte_sched_port_enqueue( port, mbufs, retval);
			
			temp = (retval - sched_port_enqueed);
			
			if( temp > 0)
			{
				total_sched_port_enquee_failed += temp;
				current_sched_port_enqueed_failed = temp;
				
				rte_pktmbuf_free_bulk( &mbufs[sched_port_enqueed], temp);
			}
			
		}

		sched_port_dequeed = rte_sched_port_dequeue( port, mbufs, 64);
	
		if( sched_port_dequeed > 0)
		{
			for( i = 0; i < sched_port_dequeed; i++)
			{
				ue_id = *(uint32_t*)rte_pktmbuf_mtod( mbufs[i], uint32_t *);
				
				ue = dqos_app__find_by_id( ue_id);
				
				if( ue)
				{
					ue->uplink_port_transmitted_bytes += mbufs[i]->pkt_len;
					ue->uplink_port_transmitted_packets++;
				}
			}
			
			rte_pktmbuf_free_bulk( mbufs, sched_port_dequeed);
		}
	
		usleep( 333);
	}
	return 1;
}


int dqos_app__perf( void * args)
{
	__test_ue_t * ue_item = ue_head;

	uint64_t uplink_port_last_received_bytes = 0;
	uint64_t uplink_port_last_transmitted_bytes = 0;
	uint64_t uplink_port_last_received_packets = 0;
	uint64_t uplink_port_last_transmitted_packets = 0;

				// (((uplink_port_last_received_bytes*8) / 1000) / 1000),
				// (((uplink_port_last_transmitted_bytes*8) / 1000) / 1000),

	while(1)
	{
		ue_item = ue_head;
		
		while( ue_item)
		{
			uplink_port_last_received_bytes 		= ue_item->uplink_port_received_bytes		- ue_item->uplink_port_last_received_bytes;
			uplink_port_last_transmitted_bytes		= ue_item->uplink_port_transmitted_bytes	- ue_item->uplink_port_last_transmitted_bytes;
			uplink_port_last_received_packets		= ue_item->uplink_port_received_packets		- ue_item->uplink_port_last_received_packets;
			uplink_port_last_transmitted_packets	= ue_item->uplink_port_transmitted_packets	- ue_item->uplink_port_last_transmitted_packets;
			
			printf( "id=%-4u  tc_class=%-2u   bytes|m-bit  recv=%-10lu|%4lu  tran=%-10lu|%4lu    packets recv=%-10lu  tran=%-10lu\n",
				ue_item->ue_id,
				ue_item->tc_class,
				uplink_port_last_received_bytes, ((uplink_port_last_received_bytes * 8)/1000)/1000,
				uplink_port_last_transmitted_bytes, ((uplink_port_last_transmitted_bytes * 8)/1000)/1000,
				uplink_port_last_received_packets,
				uplink_port_last_transmitted_packets
			);
			
			ue_item->uplink_port_last_received_bytes		= ue_item->uplink_port_received_bytes;
			ue_item->uplink_port_last_transmitted_bytes		= ue_item->uplink_port_transmitted_bytes;
	
			ue_item->uplink_port_last_received_packets		= ue_item->uplink_port_received_packets;
			ue_item->uplink_port_last_transmitted_packets	= ue_item->uplink_port_transmitted_packets;
	
			ue_item = ue_item->Next;
		}
		
		printf( "rte_sched enqueed_failed = %lu\n", current_sched_port_enqueed_failed);
		printf( "mpool  available=%u  in-use=%u\n", rte_mempool_avail_count( mpool), rte_mempool_in_use_count( mpool) );
		
		
		printf( "------------------------------------------------------------------------\n");
		sleep(1);
	}
	return 1;
}

int dqos_app__random_number(int min_num, int max_num)
{
	int result = 0, low_num = 0, hi_num = 0;

	if (min_num < max_num)
	{
		low_num = min_num;
		hi_num = max_num + 1;
	} else {
		low_num = max_num + 1;
		hi_num = min_num;
	}

	result = (rand() % (hi_num - low_num)) + low_num;
	return result;
}


int main( int argc, char* argv[])
{
	srand(time(NULL));

	int ret = rte_eal_init( argc, argv);

	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	int sts = dpdk_qos__init( "./dpdk_qos.json");


	
	if( sts == 1)
	{
		mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
		qring = rte_ring_create( "qring", 16 * 1024, 0, RING_F_SP_ENQ | RING_F_SC_DEQ);
		
		if(!mpool)
		{
			printf( "rte_pktmbuf_pool_create failed LINE=%u\n", __LINE__);
			exit(0);
		}
	
		if(!qring)
		{
			printf( "rte_ring_create failed LINE=%u\n", __LINE__);
			exit(0);
		}
	
		
		port = dpdk_qos__create_port( "port-1", 0, 10000, rte_socket_id());
		
		if( port)
		{
			uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
			rte_eal_remote_launch( dqos_app__sched_core, NULL, last_lcore++);
			rte_eal_remote_launch( dqos_app__perf, NULL, last_lcore++);
		}
		else
		{
			printf("rte-sched port creation failed\n");
			exit(0);
		}
		

		uint8_t * dPtr = malloc( sizeof(__test_ue_t) * 13);
		__test_ue_t * ue_item = NULL;
		
		int i = 0;
		for( i = 0; i < 13; i++)
		{
			ue_item = (__test_ue_t *)dPtr;
			memset( ue_item, 0, sizeof(__test_ue_t));
			
			ue_item->ue_id 		= (i + 1);
			ue_item->tc_class 		= i;
			
			if(!ue_head)
			{
				ue_head = ue_current = ue_item;
			}
			else
			{
				ue_current->Next = ue_item;
				ue_current = ue_item;
			}
			
			dPtr += sizeof(__test_ue_t);
			
			
			printf( "created and provisioned ue-id: %-10u   with traffic-class: %-10u\n", ue_item->ue_id, ue_item->tc_class);
		}
		printf("\n");
		
		
		
		// uint16_t x = dpdk_qos__find_uplink_traffic_class( 16250000);
		// printf( "x=%u\n", x);
		
		// uint16_t pipe 			= 0;
		// uint16_t nongbr_queue 	= 0;
		// uint16_t gbr_queue 		= 0;
		
		// dpdk_qos__find_user_pipe_and_q( 63, &pipe, &nongbr_queue, &gbr_queue);
		
		// printf( "pipe=%u  nongbr_queue=%u  gbr_queue=%u\n", pipe, nongbr_queue, gbr_queue);
		// exit(0);
		
		// sleep(5);
		
		int sts = 0;
		int mbcount = 64;
		struct rte_mbuf * mbufs[mbcount];
		ue_item = NULL;
		uint32_t * ue_ptr;
		
		int maxit = 30;
		int maxit_counter = 0;
		
		
		while(1)
		{
			sts = rte_pktmbuf_alloc_bulk( mpool, mbufs, mbcount);
			
			if( sts < 0 )
			{
				continue;
			}

			for( i = 0; i < mbcount; i++)
			{
				rte_prefetch0( rte_pktmbuf_mtod( mbufs[i], void *));
				
				mbufs[i]->pkt_len  	= dqos_app__random_number( 500, 1500);
				mbufs[i]->data_len 	= mbufs[i]->pkt_len;
			
				if(!ue_item)
					ue_item = ue_head;
				
				if(ue_item)
				{
					ue_ptr = rte_pktmbuf_mtod( mbufs[i], uint32_t *);
					*ue_ptr = ue_item->ue_id;
					
					ue_item->uplink_port_received_packets++;
					ue_item->uplink_port_received_bytes += mbufs[i]->pkt_len;
					
					//rte_sched_port_pkt_write( port, rx_mbufs[i], subport, pipe,              traffic_class,     queue,   (enum rte_color) 1);
					rte_sched_port_pkt_write(   port, mbufs[i],    0,       ue_item->tc_class, ue_item->tc_class, 0,       (enum rte_color) 3);
					
					//printf( "ue-id=%u   pkt_len=%-4u  received_packets=%lu\n", ue_item->ue_id, mbufs[i]->pkt_len, ue_item->uplink_port_received_packets);
					// sleep(1);
				}
				
				ue_item = ue_item->Next;
			}
			
			if( mbcount > 0)
			{
				rte_ring_sp_enqueue_bulk( qring, (void**)mbufs, mbcount, NULL);
			}

			usleep(333);
			
			maxit_counter++;
			
			if( rte_mempool_avail_count( mpool) < 1000)
				break;
		}
		
		printf("EXIT DDDDDDDDDDDDDDDDD\n");
		
		while(1) {
			sleep(1);
		}
		
	}
	else
	{
		printf( "qos init failed, with sts=%d\n", sts);
	}
	

	while(1) {
		sleep(1);
	}

	
	
	return 0;
}