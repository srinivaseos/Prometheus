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

#define MAX_SCHED_PIPES						4096
#define MAX_SCHED_SUBPORT_PROFILES			8
#define MAX_SCHED_SUBPORTS					8
#define MAX_SCHED_PIPE_PROFILES				1



static struct rte_sched_pipe_params pipe_profiles[MAX_SCHED_PIPE_PROFILES] = {
	{
		.tb_rate = 1250000,
		.tb_size = 1000000,
		
		// 	 10000 = ( 10K-BITS per milli second, means 100*10000 = 10,00,000KBITS Per second, means 10 mb per second) 
		//.tc_rate = { 100, 2000, 3000, 4000, 50000, 60000, 70000, 80000, 90000, 100000, 110000, 120000, 130000},
		.tc_rate = { 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175},
		.tc_period = 10,		//milli-seconds (1000 milli-seconds to 1 second)
		.wrr_weights = {1, 1, 1, 1},
		.tc_ov_weight = 1
	},
};



struct rte_sched_subport_params subport_params[MAX_SCHED_SUBPORTS] = {
	{
		.n_pipes_per_subport_enabled 	= 4096,
		.qsize 							= {256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
		.pipe_profiles 					= pipe_profiles,
		.n_pipe_profiles 				= sizeof(pipe_profiles) / sizeof(struct rte_sched_pipe_params),
		.n_max_pipe_profiles 			= MAX_SCHED_PIPE_PROFILES
	},
};



static struct rte_sched_subport_profile_params subport_profile[MAX_SCHED_SUBPORT_PROFILES] = {
	{
		.tb_rate = 1250000000,
		.tb_size = 1000000,
		.tc_rate = {1250000000, 1250000000, 1250000000, 1250000000,
			1250000000, 1250000000, 1250000000, 1250000000, 1250000000,
			1250000000, 1250000000, 1250000000, 1250000000},
		.tc_period = 10,
	},
};


struct rte_sched_port_params port_params = {
	.name 					= "port_scheduler_0",
	.socket 				= 0, /* computed */
	.rate 					= 0, /* computed */
	.mtu 					= 6 + 6 + 4 + 4 + 2 + 1500,
	.frame_overhead 		= RTE_SCHED_FRAME_OVERHEAD_DEFAULT,
	.n_subports_per_port 	= 1,
	.n_subport_profiles 	= 1,
	.subport_profiles 		= subport_profile,
	.n_max_subport_profiles = MAX_SCHED_SUBPORT_PROFILES,
	.n_pipes_per_subport 	= MAX_SCHED_PIPES,
};


struct rte_ring 	* qring = NULL;
struct rte_mempool 	* mpool = NULL;


int main( int argc, char* argv[])
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
	
	
	mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	qring = rte_ring_create( "qring", 16 * 1024, 0, RING_F_SP_ENQ | RING_F_SC_DEQ);

	printf( "created  mpool=%p  qring=%p\n", mpool, qring);

	
	static char port_name[32];
	memset( port_name, 0, sizeof(port_name));
	snprintf( port_name, sizeof(port_name), "port_%d", 0);

	port_params.socket = 0;
	port_params.rate = (uint64_t) 10000 * 1000 * 1000 / 8;
	port_params.name = port_name;
	
	struct rte_sched_port * port = rte_sched_port_config( &port_params);
	
	printf( "created  port=%p  rate=%lu  n_subports_per_port=%d\n", port, port_params.rate, port_params.n_subports_per_port);
	
	// ----------------------------
	// struct rte_sched_port_params port_params1;
	// memset( &port_params1, 0, sizeof(struct rte_sched_port_params));
	
	// port_params1.name 					= port_name;
	// port_params1.socket 				= 0;
	// port_params1.rate 					= (uint64_t) 10000 * 1000 * 1000 / 8;
	// port_params1.mtu 					= 6 + 6 + 4 + 4 + 2 + 1500;
	// port_params1.frame_overhead 		= RTE_SCHED_FRAME_OVERHEAD_DEFAULT;
	// port_params1.n_subports_per_port 	= 1;
	// port_params1.n_subport_profiles 	= 1;
	// port_params1.subport_profiles 		= subport_profile,
	// port_params1.n_max_subport_profiles = MAX_SCHED_SUBPORT_PROFILES;
	// port_params1.n_pipes_per_subport 	= 64;
	
	
	// struct rte_sched_port * port = rte_sched_port_config( &port_params);	
	printf( "port=%p ========== >>\n", port);
	sleep(1);
	// ----------------------------

	
	int subport 					= 0;
	uint32_t pipe					= 0;
	int err							= 0;
	uint32_t n_pipes_per_subport 	= 0;
	
	
	
	//pipe_profile.tc_rate[j]

	for(subport = 0; subport < port_params.n_subports_per_port; subport++) 
	{
		err = rte_sched_subport_config( port, subport, &subport_params[subport], 0);

		n_pipes_per_subport = subport_params[subport].n_pipes_per_subport_enabled;
		

		for( pipe = 0; pipe < n_pipes_per_subport; pipe++) 
		{
			err = rte_sched_pipe_config( port, 0, pipe, 0);
			
			if( err < 0)
			{
				printf( "   port=%p  subport=%d  n_pipes_per_subport=%d  pipe=%u  err=%d\n", port, subport, n_pipes_per_subport, pipe, err);
				exit(0);
			}
		}
		
		printf( "err=%d  subport=%d   n_pipes_per_subport=%d  n_pipe_profiles=%d pipe=%u\n", 
			err, subport, n_pipes_per_subport, subport_params[subport].n_pipe_profiles, pipe);
	}


	//sleep(5);
	
	
	//  https://en.wikipedia.org/wiki/QoS_Class_Identifier


	int i = 0;
	int sts = 0;
	int mbcount = 1000;
	struct rte_mbuf * mbufs[mbcount];
	int z = 0;
	
	struct rte_sched_queue_stats stats;
    uint16_t qlen = sizeof(struct rte_sched_queue_stats);	
	struct rte_sched_subport_params * subport_params2 = &subport_params[subport];
	
	int nb_sent = 0;
	int total_sent_succ = 0;
	int total_sent_fail = 0;
	int k0 = 0;
	int k1 = 0;
	
	//struct rte_sched_queue_stats stats;
	
	while(1)
	{
		mbcount = 50;
		
		qlen = sizeof(struct rte_sched_queue_stats);
		
		sts = rte_pktmbuf_alloc_bulk( mpool, mbufs, mbcount);
		
		if( sts < 0 )
		{
			printf( "pktmbuf_alloc failed\n");
		}
		
		printf( "mpool  available=%u  in-use=%u  qlen=%u\n", 
			rte_mempool_avail_count( mpool), 
			rte_mempool_in_use_count( mpool), qlen
		);
		
		
		for( i = 0; i < mbcount; i++)
		{
			mbufs[i]->pkt_len  	= 1500;
			mbufs[i]->data_len 	= 1500;

			//rte_sched_port_pkt_write( port, rx_mbufs[i], subport, pipe, traffic_class, queue, (enum rte_color) 1);
			rte_sched_port_pkt_write( port, mbufs[i], 0, k0, 10, 10, (enum rte_color) 3);
		}
		//k0++;

		nb_sent = rte_sched_port_enqueue( port, mbufs, mbcount);

		if( nb_sent > 0 )
		{
			total_sent_succ += nb_sent;
		}

		if( (mbcount - nb_sent) > 0)
		{
			total_sent_fail += (mbcount - nb_sent);
		}
			

		printf( "mpool  available=%u  in-use=%u  t-sent=%d  --failed=%d\n", 
			rte_mempool_avail_count( mpool), 	
			rte_mempool_in_use_count( mpool), 
			total_sent_succ, 	total_sent_fail
		);
		
		
		
		// if( (mbcount - nb_sent) == mbcount)
		// {
			// printf( "%u==%u\n",  (mbcount - nb_sent), mbcount);
			// exit(0);
		// }

		// int nb_sent = rte_sched_port_enqueue( port, mbufs, mbcount);
		// printf( "nb_sent=%u\n", nb_sent);
		
		// mbcount = 0;
		// rte_sched_port_dequeue( port, mbufs, mbcount);
		// printf( "sched_port_dequeue  mbcount=%u\n", mbcount);

		// if( mbcount > 0)
		// {
			// rte_pktmbuf_free_bulk( mbufs, mbcount);	
			// printf("freed----\n");
		// }
		
		// printf( "mpool  available=%u  in-use=%u\n", 
			// rte_mempool_avail_count( mpool), 	
			// rte_mempool_in_use_count( mpool)	
		// );
		
		// printf("\n");


		// uint32_t fp = rte_sched_port_get_memory_footprint( &port_params, &subport_params2);	
		// printf( "fp=%u\n", fp);

		// memset( &stats, 0, sizeof(struct rte_sched_queue_stats));
		// rte_sched_queue_read_stats( port, 10, &stats, &qlen);
		
		// printf( "n_pkts=%lu  n_pkts_dropped=%lu  n_pkts_cman_dropped=%lu  n_bytes=%lu  n_bytes_dropped=%lu    qlen=%u\n", 
			// stats.n_pkts, stats.n_pkts_dropped, 
			// stats.n_pkts_cman_dropped, stats.n_bytes, stats.n_bytes_dropped,
		// qlen);
		
		// //rte_sched_port_get_memory_footprint( port, )
		
		// sleep(2);
		printf("  cycle %d\n", z);
		z++;
		
		if( z == 1000)
			exit(0);
	}
	
	
	while(1) 
	{
		sleep(1);
	}
	return 0;
}
