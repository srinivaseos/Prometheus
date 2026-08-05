#include <unistd.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <limits.h>
#include <getopt.h>


#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_memcpy.h>
#include <rte_byteorder.h>
#include <rte_branch_prediction.h>

#include <rte_sched.h>
#include <rte_cfgfile.h>


#define RTE_LOGTYPE_APP 									RTE_LOGTYPE_USER1


#define APP_INTERACTIVE_DEFAULT 							0

#define APP_RX_DESC_DEFAULT 								1024
#define APP_TX_DESC_DEFAULT 								1024

#define APP_RING_SIZE 										(8*1024)
#define NB_MBUF   											(1*1024*1024)

#define MAX_PKT_RX_BURST 									64
#define PKT_ENQUEUE 										64
#define PKT_DEQUEUE 										32
#define MAX_PKT_TX_BURST 									64

#define RX_PTHRESH 											8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 											8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 											4 /**< Default values of RX write-back threshold reg. */

#define TX_PTHRESH 											36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 											0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 											0  /**< Default values of TX write-back threshold reg. */

#define BURST_TX_DRAIN_US 									100


#ifndef APP_MAX_LCORE
#if (RTE_MAX_LCORE > 64)
#define APP_MAX_LCORE 										64
#else
#define APP_MAX_LCORE 										RTE_MAX_LCORE
#endif
#endif


#define MAX_DATA_STREAMS 									(APP_MAX_LCORE/2)
#define MAX_SCHED_SUBPORTS									8
#define MAX_SCHED_PIPES										4096
#define MAX_SCHED_PIPE_PROFILES								256
#define MAX_SCHED_SUBPORT_PROFILES							8

#ifndef APP_COLLECT_STAT
#define APP_COLLECT_STAT									1
#endif

#if APP_COLLECT_STAT
#define APP_STATS_ADD(stat,val) (stat) += 					(val)
#else
#define APP_STATS_ADD(stat,val) do {(void) (val);} while (0)
#endif

#define APP_QAVG_NTIMES 									10
#define APP_QAVG_PERIOD 									100

#define APP_NAME 											"qos_sched"
#define MAX_OPT_VALUES 										8
#define SYS_CPU_DIR 										"/sys/devices/system/cpu/cpu%u/topology/"

#define CFG_ALLOC_SECTION_BATCH 							8
#define CFG_ALLOC_ENTRY_BATCH 								16

#define SUBPORT_OFFSET										7
#define PIPE_OFFSET											9
#define QUEUE_OFFSET										20
#define COLOR_OFFSET										19
#define MAX_NAME_LEN 										32


#define APP_MODE_NONE 										0
#define APP_RX_MODE   										1
#define APP_WT_MODE   										2
#define APP_TX_MODE   										4



static uint32_t app_main_core 								= 1;
static uint32_t app_numa_mask;
static uint64_t app_used_core_mask 							= 0;
static uint64_t app_used_port_mask 							= 0;
static uint64_t app_used_rx_port_mask 						= 0;
static uint64_t app_used_tx_port_mask 						= 0;

static uint32_t app_inited_port_mask 						= 0;

uint32_t active_queues[RTE_SCHED_QUEUES_PER_PIPE];
uint32_t n_active_queues;

uint8_t interactive 										= APP_INTERACTIVE_DEFAULT;
uint32_t qavg_period 										= APP_QAVG_PERIOD;
uint32_t qavg_ntimes 										= APP_QAVG_NTIMES;


static const char usage[] =
	"                                                                               \n"
	"    %s <APP PARAMS>                                                            \n"
	"                                                                               \n"
	"Application mandatory parameters:                                              \n"
	"    --pfc \"RX PORT, TX PORT, RX LCORE, WT LCORE\" : Packet flow configuration \n"
	"           multiple pfc can be configured in command line                      \n"
	"                                                                               \n"
	"Application optional parameters:                                               \n"
	"    -i      : run in interactive mode (default value is %u)                    \n"
	"    --mnc I : main core index (default value is %u)                            \n"
	"    --rsz \"A, B, C\" :   Ring sizes                                           \n"
	"           A = Size (in number of buffer descriptors) of each of the NIC RX    \n"
	"               rings read by the I/O RX lcores (default value is %u)           \n"
	"           B = Size (in number of elements) of each of the SW rings used by the\n"
	"               I/O RX lcores to send packets to worker lcores (default value is\n"
	"               %u)                                                             \n"
	"           C = Size (in number of buffer descriptors) of each of the NIC TX    \n"
	"               rings written by worker lcores (default value is %u)            \n"
	"    --bsz \"A, B, C, D\": Burst sizes                                          \n"
	"           A = I/O RX lcore read burst size from NIC RX (default value is %u)  \n"
	"           B = I/O RX lcore write burst size to output SW rings,               \n"
	"               Worker lcore read burst size from input SW rings,               \n"
	"               QoS enqueue size (default value is %u)                          \n"
	"           C = QoS dequeue size (default value is %u)                          \n"
	"           D = Worker lcore write burst size to NIC TX (default value is %u)   \n"
	"    --msz M : Mempool size (in number of mbufs) for each pfc (default %u)      \n"
	"    --rth \"A, B, C\" :   RX queue threshold parameters                        \n"
	"           A = RX prefetch threshold (default value is %u)                     \n"
	"           B = RX host threshold (default value is %u)                         \n"
	"           C = RX write-back threshold (default value is %u)                   \n"
	"    --tth \"A, B, C\" :   TX queue threshold parameters                        \n"
	"           A = TX prefetch threshold (default value is %u)                     \n"
	"           B = TX host threshold (default value is %u)                         \n"
	"           C = TX write-back threshold (default value is %u)                   \n"
	"    --cfg FILE : profile configuration to load                                 \n"
;






struct thread_stat
{
	uint64_t nb_rx;
	uint64_t nb_drop;
};


struct thread_conf
{
	uint32_t counter;
	uint32_t n_mbufs;
	struct rte_mbuf **m_table;

	uint16_t rx_port;
	uint16_t tx_port;
	uint16_t rx_queue;
	uint16_t tx_queue;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
	struct rte_sched_port *sched_port;
	struct rte_mempool *mbuf_pool;

#if APP_COLLECT_STAT
	struct thread_stat stat;
#endif
} __rte_cache_aligned;


struct flow_conf
{
	uint32_t rx_core;
	uint32_t wt_core;
	uint32_t tx_core;
	uint16_t rx_port;
	uint16_t tx_port;
	uint16_t rx_queue;
	uint16_t tx_queue;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
	struct rte_sched_port *sched_port;
	struct rte_mempool *mbuf_pool;

	struct thread_conf rx_thread;
	struct thread_conf wt_thread;
	struct thread_conf tx_thread;
};


struct ring_conf
{
	uint32_t rx_size;
	uint32_t ring_size;
	uint32_t tx_size;
};


struct burst_conf
{
	uint16_t rx_burst;
	uint16_t ring_burst;
	uint16_t qos_dequeue;
	uint16_t tx_burst;
};


struct ring_thresh
{
	uint8_t pthresh; /**< Ring prefetch threshold. */
	uint8_t hthresh; /**< Ring host threshold. */
	uint8_t wthresh; /**< Ring writeback threshold. */
};

int app_pipe_to_profile[MAX_SCHED_SUBPORTS][MAX_SCHED_PIPES];



extern uint8_t interactive;
extern uint32_t qavg_period;
extern uint32_t qavg_ntimes;
extern uint32_t nb_pfc;
extern const char *cfg_profile;
extern int mp_size;
extern struct flow_conf qos_conf[];
extern int app_pipe_to_profile[MAX_SCHED_SUBPORTS][MAX_SCHED_PIPES];

extern struct ring_conf ring_conf;
extern struct burst_conf burst_conf;
extern struct ring_thresh rx_thresh;
extern struct ring_thresh tx_thresh;

extern uint32_t active_queues[RTE_SCHED_QUEUES_PER_PIPE];
extern uint32_t n_active_queues;

extern struct rte_sched_port_params port_params;
#ifdef RTE_SCHED_CMAN
extern struct rte_sched_cman_params cman_params;
#endif
extern struct rte_sched_subport_params subport_params[MAX_SCHED_SUBPORTS];

int app_parse_args(int argc, char **argv);
int app_init(void);

void prompt(void);
void app_rx_thread(struct thread_conf **qconf);
void app_tx_thread(struct thread_conf **qconf);
void app_worker_thread(struct thread_conf **qconf);
void app_mixed_thread(struct thread_conf **qconf);

void app_stat(void);
int subport_stat(uint16_t port_id, uint32_t subport_id);
int pipe_stat(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id);
int qavg_q(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id,  uint8_t tc, uint8_t q);
int qavg_tcpipe(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id, uint8_t tc);
int qavg_pipe(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id);
int qavg_tcsubport(uint16_t port_id, uint32_t subport_id, uint8_t tc);
int qavg_subport(uint16_t port_id, uint32_t subport_id);

void app_mixed_thread(struct thread_conf **confs);
void app_worker_thread(struct thread_conf **confs);
void app_tx_thread(struct thread_conf **confs);
static void app_send_packets(struct thread_conf *qconf, struct rte_mbuf **mbufs, uint32_t nb_pkt);
static void app_send_burst(struct thread_conf *qconf);
void app_rx_thread(struct thread_conf **confs);
static int get_pkt_sched(struct rte_mbuf *m, uint32_t *subport, uint32_t *pipe, uint32_t *traffic_class, uint32_t *queue, uint32_t *color);
int pipe_stat(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id);
int subport_stat(uint16_t port_id, uint32_t subport_id);
int qavg_subport(uint16_t port_id, uint32_t subport_id);
int qavg_tcsubport(uint16_t port_id, uint32_t subport_id, uint8_t tc);
int qavg_pipe(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id);
int qavg_tcpipe( uint16_t port_id, uint32_t subport_id, uint32_t pipe_id, uint8_t tc);
int qavg_q( uint16_t port_id, uint32_t subport_id, uint32_t pipe_id, uint8_t tc, uint8_t q);
int cfg_load_subport( struct rte_cfgfile * cfg, struct rte_sched_subport_params * subport_params);
void set_subport_cman_params( struct rte_sched_subport_params * subport_p, struct rte_sched_cman_params cman_p);
int cfg_load_subport_profile( struct rte_cfgfile * cfg, struct rte_sched_subport_profile_params * subport_profile);
int cfg_load_pipe( struct rte_cfgfile * cfg, struct rte_sched_pipe_params * pipe_params);
int cfg_load_port( struct rte_cfgfile * cfg, struct rte_sched_port_params * port_params);
int app_parse_args( int argc, char ** argv);
static int app_parse_burst_conf( const char * conf_str);
static int app_parse_flow_conf( const char * conf_str);
static int app_parse_tth_conf( const char * conf_str);
static int app_parse_rth_conf( const char * conf_str);
static int app_parse_ring_conf( const char * conf_str);
static int app_parse_opt_vals(const char *conf_str, char separator, uint32_t n_vals, uint32_t *opt_vals);
static uint32_t app_cpu_core_count(void);
static uint64_t app_eal_core_mask(void);
static void app_usage(const char * prgname);
int app_init(void);
static int app_load_cfg_profile(const char *profile);
static struct rte_sched_port * app_init_sched_port(uint32_t portid, uint32_t socketid);

//static uint64_t app_used_port_mask = 0;


struct ring_conf ring_conf = {
	.rx_size   = APP_RX_DESC_DEFAULT,
	.ring_size = APP_RING_SIZE,
	.tx_size   = APP_TX_DESC_DEFAULT,
};


struct burst_conf burst_conf = {
	.rx_burst    = MAX_PKT_RX_BURST,
	.ring_burst  = PKT_ENQUEUE,
	.qos_dequeue = PKT_DEQUEUE,
	.tx_burst    = MAX_PKT_TX_BURST,
};


struct ring_thresh rx_thresh = {
	.pthresh = RX_PTHRESH,
	.hthresh = RX_HTHRESH,
	.wthresh = RX_WTHRESH,
};

struct ring_thresh tx_thresh = {
	.pthresh = TX_PTHRESH,
	.hthresh = TX_HTHRESH,
	.wthresh = TX_WTHRESH,
};


uint32_t nb_pfc;
const char *cfg_profile = NULL;
int mp_size = NB_MBUF;
struct flow_conf qos_conf[MAX_DATA_STREAMS];


static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
	},
	.txmode = {
		.mq_mode = RTE_ETH_MQ_TX_NONE,
	},
};



static struct rte_sched_pipe_params pipe_profiles[MAX_SCHED_PIPE_PROFILES] = {
	{ /* Profile #0 */
		.tb_rate = 305175,
		.tb_size = 1000000,

		.tc_rate = {305175, 305175, 305175, 305175, 305175, 305175,
			305175, 305175, 305175, 305175, 305175, 305175, 305175},
		.tc_period = 40,
#ifdef RTE_SCHED_SUBPORT_TC_OV
		.tc_ov_weight = 1,
#endif

		.wrr_weights = {1, 1, 1, 1},
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

#ifdef RTE_SCHED_CMAN
struct rte_sched_cman_params cman_params = {
	.cman_mode = RTE_SCHED_CMAN_RED,
	.red_params = {
		/* Traffic Class 0 Colors Green / Yellow / Red */
		[0][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[0][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[0][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 1 - Colors Green / Yellow / Red */
		[1][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[1][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[1][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 2 - Colors Green / Yellow / Red */
		[2][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[2][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[2][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 3 - Colors Green / Yellow / Red */
		[3][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[3][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[3][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 4 - Colors Green / Yellow / Red */
		[4][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[4][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[4][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 5 - Colors Green / Yellow / Red */
		[5][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[5][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[5][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 6 - Colors Green / Yellow / Red */
		[6][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[6][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[6][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 7 - Colors Green / Yellow / Red */
		[7][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[7][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[7][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 8 - Colors Green / Yellow / Red */
		[8][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[8][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[8][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 9 - Colors Green / Yellow / Red */
		[9][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[9][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[9][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 10 - Colors Green / Yellow / Red */
		[10][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[10][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[10][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 11 - Colors Green / Yellow / Red */
		[11][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[11][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[11][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},

		/* Traffic Class 12 - Colors Green / Yellow / Red */
		[12][0] = {.min_th = 48, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[12][1] = {.min_th = 40, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
		[12][2] = {.min_th = 32, .max_th = 64, .maxp_inv = 10, .wq_log2 = 9},
	},
};
#endif /* RTE_SCHED_CMAN */


struct rte_sched_subport_params subport_params[MAX_SCHED_SUBPORTS] = {
	{
		.n_pipes_per_subport_enabled = 4096,
		.qsize = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
		.pipe_profiles = pipe_profiles,
		.n_pipe_profiles = sizeof(pipe_profiles) /
			sizeof(struct rte_sched_pipe_params),
		.n_max_pipe_profiles = MAX_SCHED_PIPE_PROFILES,
#ifdef RTE_SCHED_CMAN
		.cman_params = &cman_params,
#endif /* RTE_SCHED_CMAN */
	},
};


struct rte_sched_port_params port_params = {
	.name = "port_scheduler_0",
	.socket = 0, /* computed */
	.rate = 0, /* computed */
	.mtu = 6 + 6 + 4 + 4 + 2 + 1500,
	.frame_overhead = RTE_SCHED_FRAME_OVERHEAD_DEFAULT,
	.n_subports_per_port = 1,
	.n_subport_profiles = 1,
	.subport_profiles = subport_profile,
	.n_max_subport_profiles = MAX_SCHED_SUBPORT_PROFILES,
	.n_pipes_per_subport = MAX_SCHED_PIPES,
};






static struct rte_sched_port * app_init_sched_port(uint32_t portid, uint32_t socketid)
{
	static char port_name[32]; /* static as referenced from global port_params*/
	struct rte_eth_link link;
	struct rte_sched_port *port = NULL;
	uint32_t pipe, subport;
	int err;


	err = rte_eth_link_get(portid, &link);
	
	if (err < 0)
		rte_exit(EXIT_FAILURE, "rte_eth_link_get: err=%d, port=%u: %s\n", err, portid, rte_strerror(-err));

	port_params.socket = socketid;
	port_params.rate = (uint64_t) link.link_speed * 1000 * 1000 / 8;
	snprintf(port_name, sizeof(port_name), "port_%d", portid);
	port_params.name = port_name;

	port = rte_sched_port_config(&port_params);
	if (port == NULL)
	{
		rte_exit(EXIT_FAILURE, "Unable to config sched port\n");
	}

	printf( "--------------------------------------------------------------\n");
	printf( "Name = %s\n", 								port_params.name);
	printf( "Socket = %d\n", 							port_params.socket);
	printf( "Rate = %lu\n", 							port_params.rate);
	printf( "MTU = %u\n", 								port_params.mtu);
	printf( "FOH = %u\n", 								port_params.frame_overhead);
	printf( "n_subports_per_port = %u\n", 				port_params.n_subports_per_port);
	printf( "n_subport_profiles = %u\n", 				port_params.n_subport_profiles);
	printf( "n_max_subport_profiles = %u\n", 			port_params.n_max_subport_profiles);
	printf( "n_pipes_per_subport = %u\n", 				port_params.n_pipes_per_subport);
	printf( "subport_profiles[0].tb_rate = %lu\n",  	port_params.subport_profiles[0].tb_rate);
	printf( "subport_profiles[0].tb_size = %lu\n",  	port_params.subport_profiles[0].tb_size);
	printf( "subport_profiles[0].tc_period = %lu\n",  	port_params.subport_profiles[0].tc_period);
	printf( "subport_profiles[0].tc_rate = %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", 
			port_params.subport_profiles[0].tc_rate[0], port_params.subport_profiles[0].tc_rate[1], 	port_params.subport_profiles[0].tc_rate[2],  
			port_params.subport_profiles[0].tc_rate[3], port_params.subport_profiles[0].tc_rate[4], 	port_params.subport_profiles[0].tc_rate[5], 
			port_params.subport_profiles[0].tc_rate[6], port_params.subport_profiles[0].tc_rate[7], 	port_params.subport_profiles[0].tc_rate[8], 
			port_params.subport_profiles[0].tc_rate[9], port_params.subport_profiles[0].tc_rate[10], 	port_params.subport_profiles[0].tc_rate[11],
			port_params.subport_profiles[0].tc_rate[12]
		);
	
	
	printf( "--------------------------------------------------------------\n");
	

	for (subport = 0; subport < port_params.n_subports_per_port; subport ++) 
	{
		err = rte_sched_subport_config( port, subport, &subport_params[subport], 0);
		
		
		printf("n_pipes_per_subport_enabled = %u \n", subport_params[subport].n_pipes_per_subport_enabled);
		printf("qsize = %u %u %u %u %u %u %u %u %u %u %u %u %u\n", 
			subport_params[subport].qsize[0], subport_params[subport].qsize[1], subport_params[subport].qsize[2],  subport_params[subport].qsize[3],
			subport_params[subport].qsize[4], subport_params[subport].qsize[5], subport_params[subport].qsize[6],  subport_params[subport].qsize[7],
			subport_params[subport].qsize[8], subport_params[subport].qsize[9], subport_params[subport].qsize[10], subport_params[subport].qsize[11],
			subport_params[subport].qsize[12]
		);
		
		printf("n_pipe_profiles = %u \n", subport_params[subport].n_pipe_profiles);
		printf("n_max_pipe_profiles = %u \n", subport_params[subport].n_max_pipe_profiles);
		printf("cman_params = %p \n", subport_params[subport].cman_params);

		printf("subport_params[0].pipe_profiles[0].tb_rate = %lu \n", subport_params[subport].pipe_profiles[0].tb_rate);	
		printf("subport_params[0].pipe_profiles[0].tb_size = %lu \n", subport_params[subport].pipe_profiles[0].tb_size);	
		printf("subport_params[0].pipe_profiles[0].tc_period = %lu \n", subport_params[subport].pipe_profiles[0].tc_period);	
		printf("subport_params[0].pipe_profiles[0].tc_rate = %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", 
			subport_params[subport].pipe_profiles[0].tc_rate[0], subport_params[subport].pipe_profiles[0].tc_rate[1], subport_params[subport].pipe_profiles[0].tc_rate[2],  
			subport_params[subport].pipe_profiles[0].tc_rate[3], subport_params[subport].pipe_profiles[0].tc_rate[4], subport_params[subport].pipe_profiles[0].tc_rate[5], 
			subport_params[subport].pipe_profiles[0].tc_rate[6], subport_params[subport].pipe_profiles[0].tc_rate[7], subport_params[subport].pipe_profiles[0].tc_rate[8], 
			subport_params[subport].pipe_profiles[0].tc_rate[9], subport_params[subport].pipe_profiles[0].tc_rate[10],subport_params[subport].pipe_profiles[0].tc_rate[11],
			subport_params[subport].pipe_profiles[0].tc_rate[12]
		);
		
		printf( "--------------------------------------------------------------\n");
	
		
		if (err) 
		{
			rte_exit(EXIT_FAILURE, "Unable to config sched subport %u, err=%d\n", subport, err);
		}

		uint32_t n_pipes_per_subport = subport_params[subport].n_pipes_per_subport_enabled;

		for (pipe = 0; pipe < n_pipes_per_subport; pipe++) 
		{
			//	printf( "subport=%u, pipe=%u, app_pipe_to_profile=%d\n", subport, pipe, app_pipe_to_profile[subport][pipe]);
			
			if (app_pipe_to_profile[subport][pipe] != -1) 
			{
				err = rte_sched_pipe_config( port, subport, pipe, app_pipe_to_profile[subport][pipe]);
				
				printf( "subport=%u, pipe=%u, app_pipe_to_profile=%d\n", subport, pipe, app_pipe_to_profile[subport][pipe]);
				
				if (err) 
				{
					rte_exit(EXIT_FAILURE, "Unable to config sched pipe %u for profile %d, err=%d\n", pipe, app_pipe_to_profile[subport][pipe], err);
				}
			}
		}
	}

	return port;
}

static int app_load_cfg_profile(const char *profile)
{
	if (profile == NULL)
		return 0;
	struct rte_cfgfile *file = rte_cfgfile_load(profile, 0);
	if (file == NULL)
		rte_exit(EXIT_FAILURE, "Cannot load configuration profile %s\n", profile);

	cfg_load_port(file, &port_params);
	cfg_load_subport(file, subport_params);
	cfg_load_subport_profile(file, subport_profile);
	cfg_load_pipe(file, pipe_profiles);

	rte_cfgfile_close(file);

	return 0;
}



static int app_init_port(uint16_t portid, struct rte_mempool *mp)
{
	int ret;
	struct rte_eth_link link;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_rxconf rx_conf;
	struct rte_eth_txconf tx_conf;
	uint16_t rx_size;
	uint16_t tx_size;
	struct rte_eth_conf local_port_conf = port_conf;
	char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

	/* check if port already initialized (multistream configuration) */
	if (app_inited_port_mask & (1u << portid))
		return 0;

	rx_conf.rx_thresh.pthresh = rx_thresh.pthresh;
	rx_conf.rx_thresh.hthresh = rx_thresh.hthresh;
	rx_conf.rx_thresh.wthresh = rx_thresh.wthresh;
	rx_conf.rx_free_thresh = 32;
	rx_conf.rx_drop_en = 0;
	rx_conf.rx_deferred_start = 0;

	tx_conf.tx_thresh.pthresh = tx_thresh.pthresh;
	tx_conf.tx_thresh.hthresh = tx_thresh.hthresh;
	tx_conf.tx_thresh.wthresh = tx_thresh.wthresh;
	tx_conf.tx_free_thresh = 0;
	tx_conf.tx_rs_thresh = 0;
	tx_conf.tx_deferred_start = 0;

	/* init port */
	RTE_LOG(INFO, APP, "Initializing port %"PRIu16"... ", portid);
	fflush(stdout);

	ret = rte_eth_dev_info_get(portid, &dev_info);
	if (ret != 0)
		rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n", portid, strerror(-ret));

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		local_port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);

	if (ret < 0)
		rte_exit( EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, portid);

	rx_size = ring_conf.rx_size;
	tx_size = ring_conf.tx_size;
	ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &rx_size, &tx_size);
	if (ret < 0)
		rte_exit(EXIT_FAILURE,
			 "rte_eth_dev_adjust_nb_rx_tx_desc: err=%d,port=%u\n",
			 ret, portid);
	ring_conf.rx_size = rx_size;
	ring_conf.tx_size = tx_size;

	/* init one RX queue */
	fflush(stdout);
	rx_conf.offloads = local_port_conf.rxmode.offloads;
	ret = rte_eth_rx_queue_setup(portid, 0, (uint16_t)ring_conf.rx_size,
		rte_eth_dev_socket_id(portid), &rx_conf, mp);
	if (ret < 0)
		rte_exit(EXIT_FAILURE,
			 "rte_eth_tx_queue_setup: err=%d, port=%u\n",
			 ret, portid);

	/* init one TX queue */
	fflush(stdout);
	tx_conf.offloads = local_port_conf.txmode.offloads;
	ret = rte_eth_tx_queue_setup(portid, 0,
		(uint16_t)ring_conf.tx_size, rte_eth_dev_socket_id(portid), &tx_conf);
	if (ret < 0)
		rte_exit(EXIT_FAILURE,
			 "rte_eth_tx_queue_setup: err=%d, port=%u queue=%d\n",
			 ret, portid, 0);

	/* Start device */
	ret = rte_eth_dev_start(portid);
	if (ret < 0)
		rte_exit(EXIT_FAILURE,
			 "rte_pmd_port_start: err=%d, port=%u\n",
			 ret, portid);

	printf("done: ");

	/* get link status */
	ret = rte_eth_link_get(portid, &link);
	if (ret < 0)
		rte_exit(EXIT_FAILURE,
			 "rte_eth_link_get: err=%d, port=%u: %s\n",
			 ret, portid, rte_strerror(-ret));

	rte_eth_link_to_str(link_status_text, sizeof(link_status_text), &link);
	printf("%s\n", link_status_text);

	ret = rte_eth_promiscuous_enable(portid);
	if (ret != 0)
		rte_exit(EXIT_FAILURE,
			"rte_eth_promiscuous_enable: err=%s, port=%u\n",
			rte_strerror(-ret), portid);

	/* mark port as initialized */
	app_inited_port_mask |= 1u << portid;

	return 0;
}


int app_init(void)
{
	uint32_t i;
	char ring_name[MAX_NAME_LEN];
	char pool_name[MAX_NAME_LEN];

	if (rte_eth_dev_count_avail() == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet port - bye\n");

	/* load configuration profile */
	if (app_load_cfg_profile(cfg_profile) != 0)
		rte_exit(EXIT_FAILURE, "Invalid configuration profile\n");

	printf( "line %d completed\n", __LINE__);


	/* Initialize each active flow */
	for(i = 0; i < nb_pfc; i++) 
	{
		uint32_t socket = rte_lcore_to_socket_id(qos_conf[i].rx_core);
		struct rte_ring *ring;
		
		printf( "line %d completed\n", __LINE__);
		
		snprintf(ring_name, MAX_NAME_LEN, "ring-%u-%u", i, qos_conf[i].rx_core);
		ring = rte_ring_lookup(ring_name);
		if (ring == NULL)
			qos_conf[i].rx_ring = rte_ring_create(ring_name, ring_conf.ring_size,
			 	socket, RING_F_SP_ENQ | RING_F_SC_DEQ);
		else
			qos_conf[i].rx_ring = ring;

		printf( "line %d completed\n", __LINE__);
		
		snprintf(ring_name, MAX_NAME_LEN, "ring-%u-%u", i, qos_conf[i].tx_core);
		ring = rte_ring_lookup(ring_name);
		if (ring == NULL)
			qos_conf[i].tx_ring = rte_ring_create(ring_name, ring_conf.ring_size,
				socket, RING_F_SP_ENQ | RING_F_SC_DEQ);
		else
			qos_conf[i].tx_ring = ring;


		printf( "line %d completed port=%u  mp_size=%u rx_burst=%u\n", __LINE__, qos_conf[i].rx_port, mp_size, burst_conf.rx_burst * 4);

		/* create the mbuf pools for each RX Port */
		snprintf(pool_name, MAX_NAME_LEN, "mbuf_pool%u", i);
		
		
		
		qos_conf[i].mbuf_pool = rte_pktmbuf_pool_create( pool_name, mp_size, burst_conf.rx_burst * 4, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_eth_dev_socket_id(qos_conf[i].rx_port));
		
		if (qos_conf[i].mbuf_pool == NULL)
			rte_exit(EXIT_FAILURE, "Cannot init mbuf pool for socket %u\n", i);

		printf( "line %d completed\n", __LINE__);
		
		app_init_port(qos_conf[i].rx_port, qos_conf[i].mbuf_pool);
		app_init_port(qos_conf[i].tx_port, qos_conf[i].mbuf_pool);

		qos_conf[i].sched_port = app_init_sched_port(qos_conf[i].tx_port, socket);
	}

	printf( "line %d completed\n", __LINE__);
	
	RTE_LOG(INFO, APP, "time stamp clock running at %" PRIu64 " Hz\n",  rte_get_timer_hz());

	RTE_LOG(INFO, APP, "Ring sizes: NIC RX = %u, Mempool = %d SW queue = %u,"
			 "NIC TX = %u\n", ring_conf.rx_size, mp_size, ring_conf.ring_size,
			 ring_conf.tx_size);

	RTE_LOG(INFO, APP, "Burst sizes: RX read = %hu, RX write = %hu,\n"
						  "             Worker read/QoS enqueue = %hu,\n"
						  "             QoS dequeue = %hu, Worker write = %hu\n",
		burst_conf.rx_burst, burst_conf.ring_burst, burst_conf.ring_burst,
		burst_conf.qos_dequeue, burst_conf.tx_burst);

	RTE_LOG(INFO, APP, "NIC thresholds RX (p = %hhu, h = %hhu, w = %hhu),"
				 "TX (p = %hhu, h = %hhu, w = %hhu)\n",
		rx_thresh.pthresh, rx_thresh.hthresh, rx_thresh.wthresh,
		tx_thresh.pthresh, tx_thresh.hthresh, tx_thresh.wthresh);

	return 0;
}




/* display usage */
static void app_usage(const char * prgname)
{
	printf
	(
		usage, prgname, APP_INTERACTIVE_DEFAULT, app_main_core,
		APP_RX_DESC_DEFAULT, APP_RING_SIZE, APP_TX_DESC_DEFAULT, MAX_PKT_RX_BURST, PKT_ENQUEUE, PKT_DEQUEUE,
		MAX_PKT_TX_BURST, NB_MBUF, RX_PTHRESH, RX_HTHRESH, RX_WTHRESH, TX_PTHRESH, TX_HTHRESH, TX_WTHRESH
	);
}


/* returns core mask used by DPDK */
static uint64_t app_eal_core_mask(void)
{
	uint64_t cm = 0;
	uint32_t i;

	for (i = 0; i < APP_MAX_LCORE; i++) 
	{
		if (rte_lcore_has_role(i, ROLE_RTE))
			cm |= (1ULL << i);
	}

	cm |= (1ULL << rte_get_main_lcore());

	return cm;
}


/* returns total number of cores presented in a system */
static uint32_t app_cpu_core_count(void)
{
	int i, len;
	char path[PATH_MAX];
	uint32_t ncores = 0;

	for (i = 0; i < APP_MAX_LCORE; i++) {
		len = snprintf(path, sizeof(path), SYS_CPU_DIR, i);
		if (len <= 0 || (unsigned)len >= sizeof(path))
			continue;

		if (access(path, F_OK) == 0)
			ncores++;
	}

	return ncores;
}


/* returns:
	 number of values parsed
	-1 in case of error
*/
static int app_parse_opt_vals(const char *conf_str, char separator, uint32_t n_vals, uint32_t *opt_vals)
{
	char *string;
	int i, n_tokens;
	char *tokens[MAX_OPT_VALUES];

	if (conf_str == NULL || opt_vals == NULL || n_vals == 0 || n_vals > MAX_OPT_VALUES)
		return -1;

	/* duplicate configuration string before splitting it to tokens */
	string = strdup(conf_str);
	if (string == NULL)
		return -1;

	n_tokens = rte_strsplit(string, strnlen(string, 32), tokens, n_vals, separator);

	if (n_tokens > MAX_OPT_VALUES)
		return -1;

	for (i = 0; i < n_tokens; i++)
		opt_vals[i] = (uint32_t)atol(tokens[i]);

	free(string);

	return n_tokens;
}



static int app_parse_ring_conf( const char * conf_str)
{
	int ret;
	uint32_t vals[3];

	ret = app_parse_opt_vals(conf_str, ',', 3, vals);
	if (ret != 3)
		return ret;

	ring_conf.rx_size = vals[0];
	ring_conf.ring_size = vals[1];
	ring_conf.tx_size = vals[2];

	return 0;
}


static int app_parse_rth_conf( const char * conf_str)
{
	int ret;
	uint32_t vals[3];

	ret = app_parse_opt_vals(conf_str, ',', 3, vals);
	if (ret != 3)
		return ret;

	rx_thresh.pthresh = (uint8_t)vals[0];
	rx_thresh.hthresh = (uint8_t)vals[1];
	rx_thresh.wthresh = (uint8_t)vals[2];

	return 0;
}


static int app_parse_tth_conf( const char * conf_str)
{
	int ret;
	uint32_t vals[3];

	ret = app_parse_opt_vals(conf_str, ',', 3, vals);
	if (ret != 3)
		return ret;

	tx_thresh.pthresh = (uint8_t)vals[0];
	tx_thresh.hthresh = (uint8_t)vals[1];
	tx_thresh.wthresh = (uint8_t)vals[2];

	return 0;
}


static int app_parse_flow_conf( const char * conf_str)
{
	int ret;
	uint32_t vals[5];
	struct flow_conf *pconf;
	uint64_t mask;

	memset(vals, 0, sizeof(vals));
	ret = app_parse_opt_vals(conf_str, ',', 6, vals);
	if (ret < 4 || ret > 5)
		return ret;

	pconf = &qos_conf[nb_pfc];

	pconf->rx_port = vals[0];
	pconf->tx_port = vals[1];
	pconf->rx_core = (uint8_t)vals[2];
	pconf->wt_core = (uint8_t)vals[3];
	
	printf("%s rx_port=%d tx_port=%d\n", conf_str, pconf->rx_port, pconf->tx_port);

	
	if (ret == 5)
		pconf->tx_core = (uint8_t)vals[4];
	else
		pconf->tx_core = pconf->wt_core;

	if (pconf->rx_core == pconf->wt_core) 
	{
		RTE_LOG(ERR, APP, "pfc %u: rx thread and worker thread cannot share same core\n", nb_pfc);
		return -1;
	}

	if (pconf->rx_port >= RTE_MAX_ETHPORTS) 
	{
		RTE_LOG(ERR, APP, "pfc %u: invalid rx port %"PRIu16" index\n", nb_pfc, pconf->rx_port);
		return -1;
	}
	
	if (pconf->tx_port >= RTE_MAX_ETHPORTS) {
		RTE_LOG(ERR, APP, "pfc %u: invalid tx port %"PRIu16" index\n", nb_pfc, pconf->tx_port);
		return -1;
	}

	mask = 1lu << pconf->rx_port;
	
	if (app_used_rx_port_mask & mask) 
	{
		RTE_LOG(ERR, APP, "pfc %u: rx port %"PRIu16" is used already\n", nb_pfc, pconf->rx_port);
		return -1;
	}
	
	app_used_rx_port_mask |= mask;
	app_used_port_mask |= mask;

	mask = 1lu << pconf->tx_port;
	if (app_used_tx_port_mask & mask) 
	{
		RTE_LOG(ERR, APP, "pfc %u: port %"PRIu16" is used already\n", nb_pfc, pconf->tx_port);
		return -1;
	}
	
	app_used_tx_port_mask |= mask;
	app_used_port_mask |= mask;

	mask = 1lu << pconf->rx_core;
	app_used_core_mask |= mask;

	mask = 1lu << pconf->wt_core;
	app_used_core_mask |= mask;

	mask = 1lu << pconf->tx_core;
	app_used_core_mask |= mask;

	nb_pfc++;

	return 0;
}



static int app_parse_burst_conf( const char * conf_str)
{
	int ret;
	uint32_t vals[4];

	ret = app_parse_opt_vals(conf_str, ',', 4, vals);
	
	if (ret != 4)
		return ret;

	burst_conf.rx_burst    = (uint16_t)vals[0];
	burst_conf.ring_burst  = (uint16_t)vals[1];
	burst_conf.qos_dequeue = (uint16_t)vals[2];
	burst_conf.tx_burst    = (uint16_t)vals[3];

	return 0;
}



enum {
	#define OPT_PFC "pfc" 
		OPT_PFC_NUM = 256,
	#define OPT_MNC "mnc"
		OPT_MNC_NUM,
	#define OPT_RSZ "rsz"
		OPT_RSZ_NUM,
	#define OPT_BSZ "bsz"
		OPT_BSZ_NUM,
	#define OPT_MSZ "msz"
		OPT_MSZ_NUM,
	#define OPT_RTH "rth"
		OPT_RTH_NUM,
	#define OPT_TTH "tth"
		OPT_TTH_NUM,
	#define OPT_CFG "cfg"
		OPT_CFG_NUM,
};





/*
 * Parses the argument given in the command line of the application,
 * calculates mask for used cores and initializes EAL with calculated core mask
 */
int app_parse_args( int argc, char ** argv)
{
	int opt, ret;
	int option_index;
	char *prgname = argv[0];
	uint32_t i, nb_lcores;

	static struct option lgopts[] = 
	{
		{OPT_PFC, 1, NULL, OPT_PFC_NUM},
		{OPT_MNC, 1, NULL, OPT_MNC_NUM},
		{OPT_RSZ, 1, NULL, OPT_RSZ_NUM},
		{OPT_BSZ, 1, NULL, OPT_BSZ_NUM},
		{OPT_MSZ, 1, NULL, OPT_MSZ_NUM},
		{OPT_RTH, 1, NULL, OPT_RTH_NUM},
		{OPT_TTH, 1, NULL, OPT_TTH_NUM},
		{OPT_CFG, 1, NULL, OPT_CFG_NUM},
		{NULL,    0, 0,    0          }
	};

	/* initialize EAL first */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		return -1;

	
	printf( "rte_eal_init completed\n");

	
	argc -= ret;
	argv += ret;

	
	/* set en_US locale to print big numbers with ',' */
	setlocale( LC_NUMERIC, "en_US.utf-8");

	while ((opt = getopt_long(argc, argv, "i",
		lgopts, &option_index)) != EOF) {

			switch (opt) {
			case 'i':
				printf("Interactive-mode selected\n");
				interactive = 1;
				break;
			/* long options */

			case OPT_PFC_NUM:
				ret = app_parse_flow_conf(optarg);
				if (ret) {
					RTE_LOG(ERR, APP, "Invalid pipe configuration %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_MNC_NUM:
				app_main_core = (uint32_t)atoi(optarg);
				break;

			case OPT_RSZ_NUM:
				ret = app_parse_ring_conf(optarg);
				if (ret) {
					RTE_LOG(ERR, APP, "Invalid ring configuration %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_BSZ_NUM:
				ret = app_parse_burst_conf(optarg);
				if (ret) {
					RTE_LOG(ERR, APP, "Invalid burst configuration %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_MSZ_NUM:
				mp_size = atoi(optarg);
				if (mp_size <= 0) {
					RTE_LOG(ERR, APP, "Invalid mempool size %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_RTH_NUM:
				ret = app_parse_rth_conf(optarg);
				if (ret) {
					RTE_LOG(ERR, APP, "Invalid RX threshold configuration %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_TTH_NUM:
				ret = app_parse_tth_conf(optarg);
				if (ret) {
					RTE_LOG(ERR, APP, "Invalid TX threshold configuration %s\n",
							optarg);
					return -1;
				}
				break;

			case OPT_CFG_NUM:
				cfg_profile = optarg;
				break;

			default:
				app_usage(prgname);
				return -1;
			}
	}

	/* check main core index validity */
	for (i = 0; i <= app_main_core; i++) 
	{
		if (app_used_core_mask & RTE_BIT64(app_main_core)) 
		{
			RTE_LOG(ERR, APP, "Main core index is not configured properly\n");
			app_usage(prgname);
			return -1;
		}
	}

	app_used_core_mask |= RTE_BIT64(app_main_core);

	/*
	if ((app_used_core_mask != app_eal_core_mask()) || (app_main_core != rte_get_main_lcore())) 
	{
		RTE_LOG(ERR, APP, "eal core mask not configured properly, must be %" PRIX64
				" instead of %" PRIX64 "\n" , app_used_core_mask, app_eal_core_mask());
		return -1;
	}
	*/
	
	if (nb_pfc == 0) 
	{
		RTE_LOG(ERR, APP, "Packet flow not configured!\n");
		app_usage(prgname);
		return -1;
	}

	printf("line %d completed\n", __LINE__);


	/* sanity check for cores assignment */
	nb_lcores = app_cpu_core_count();

	for(i = 0; i < nb_pfc; i++) 
	{
		if (qos_conf[i].rx_core >= nb_lcores) 
		{
			RTE_LOG(ERR, APP, "pfc %u: invalid RX lcore index %u\n", i + 1,
					qos_conf[i].rx_core);
			return -1;
		}
		
		if (qos_conf[i].wt_core >= nb_lcores) 
		{
			RTE_LOG(ERR, APP, "pfc %u: invalid WT lcore index %u\n", i + 1,
					qos_conf[i].wt_core);
			return -1;
		}
		
		uint32_t rx_sock = rte_lcore_to_socket_id(qos_conf[i].rx_core);
		uint32_t wt_sock = rte_lcore_to_socket_id(qos_conf[i].wt_core);
		
		if (rx_sock != wt_sock) 
		{
			RTE_LOG(ERR, APP, "pfc %u: RX and WT must be on the same socket\n", i + 1);
			return -1;
		}
		
		app_numa_mask |= 1 << rte_lcore_to_socket_id(qos_conf[i].rx_core);
	}

	printf("line %d completed\n", __LINE__);
	
	return 0;
}








int cfg_load_port( struct rte_cfgfile * cfg, struct rte_sched_port_params * port_params)
{
	const char *entry;

	if (!cfg || !port_params)
		return -1;

	entry = rte_cfgfile_get_entry(cfg, "port", "frame overhead");
	if (entry)
		port_params->frame_overhead = (uint32_t)atoi(entry);

	entry = rte_cfgfile_get_entry(cfg, "port", "number of subports per port");
	if (entry)
		port_params->n_subports_per_port = (uint32_t)atoi(entry);

	return 0;
}


int cfg_load_pipe( struct rte_cfgfile * cfg, struct rte_sched_pipe_params * pipe_params)
{
	int i, j;
	char *next;
	const char *entry;
	int profiles;

	if (!cfg || !pipe_params)
		return -1;

	profiles = rte_cfgfile_num_sections(cfg, "pipe profile", sizeof("pipe profile") - 1);
	subport_params[0].n_pipe_profiles = profiles;

	for (j = 0; j < profiles; j++) 
	{
		char pipe_name[32];
		snprintf(pipe_name, sizeof(pipe_name), "pipe profile %d", j);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tb rate");
		if (entry)
			pipe_params[j].tb_rate = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tb size");
		if (entry)
			pipe_params[j].tb_size = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc period");
		if (entry)
			pipe_params[j].tc_period = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 0 rate");
		if (entry)
			pipe_params[j].tc_rate[0] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 1 rate");
		if (entry)
			pipe_params[j].tc_rate[1] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 2 rate");
		if (entry)
			pipe_params[j].tc_rate[2] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 3 rate");
		if (entry)
			pipe_params[j].tc_rate[3] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 4 rate");
		if (entry)
			pipe_params[j].tc_rate[4] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 5 rate");
		if (entry)
			pipe_params[j].tc_rate[5] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 6 rate");
		if (entry)
			pipe_params[j].tc_rate[6] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 7 rate");
		if (entry)
			pipe_params[j].tc_rate[7] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 8 rate");
		if (entry)
			pipe_params[j].tc_rate[8] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 9 rate");
		if (entry)
			pipe_params[j].tc_rate[9] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 10 rate");
		if (entry)
			pipe_params[j].tc_rate[10] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 11 rate");
		if (entry)
			pipe_params[j].tc_rate[11] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 12 rate");
		if (entry)
			pipe_params[j].tc_rate[12] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 12 oversubscription weight");
		if (entry)
			pipe_params[j].tc_ov_weight = (uint8_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, pipe_name, "tc 12 wrr weights");
		if (entry) {
			for (i = 0; i < RTE_SCHED_BE_QUEUES_PER_PIPE; i++) {
				pipe_params[j].wrr_weights[i] =
					(uint8_t)strtol(entry, &next, 10);
				if (next == NULL)
					break;
				entry = next;
			}
		}
	}
	return 0;
}


int cfg_load_subport_profile( struct rte_cfgfile * cfg, struct rte_sched_subport_profile_params * subport_profile)
{
	int i;
	const char *entry;
	int profiles;

	if (!cfg || !subport_profile)
		return -1;

	profiles = rte_cfgfile_num_sections( cfg, "subport profile", sizeof("subport profile") - 1);
	subport_params[0].n_pipe_profiles = profiles;

	for (i = 0; i < profiles; i++) 
	{
		char sec_name[32];
		snprintf(sec_name, sizeof(sec_name), "subport profile %d", i);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tb rate");
		if (entry)
			subport_profile[i].tb_rate = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tb size");
		if (entry)
			subport_profile[i].tb_size = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc period");
		if (entry)
			subport_profile[i].tc_period = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 0 rate");
		if (entry)
			subport_profile[i].tc_rate[0] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 1 rate");
		if (entry)
			subport_profile[i].tc_rate[1] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 2 rate");
		if (entry)
			subport_profile[i].tc_rate[2] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 3 rate");
		if (entry)
			subport_profile[i].tc_rate[3] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 4 rate");
		if (entry)
			subport_profile[i].tc_rate[4] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 5 rate");
		if (entry)
			subport_profile[i].tc_rate[5] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 6 rate");
		if (entry)
			subport_profile[i].tc_rate[6] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 7 rate");
		if (entry)
			subport_profile[i].tc_rate[7] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 8 rate");
		if (entry)
			subport_profile[i].tc_rate[8] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 9 rate");
		if (entry)
			subport_profile[i].tc_rate[9] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 10 rate");
		if (entry)
			subport_profile[i].tc_rate[10] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 11 rate");
		if (entry)
			subport_profile[i].tc_rate[11] = (uint64_t)atoi(entry);

		entry = rte_cfgfile_get_entry(cfg, sec_name, "tc 12 rate");
		if (entry)
			subport_profile[i].tc_rate[12] = (uint64_t)atoi(entry);
	}

	return 0;
}


#ifdef RTE_SCHED_CMAN
void set_subport_cman_params( struct rte_sched_subport_params * subport_p, struct rte_sched_cman_params cman_p)
{
	int j, k;
	subport_p->cman_params->cman_mode = cman_p.cman_mode;

	for (j = 0; j < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; j++) 
	{
		if (subport_p->cman_params->cman_mode == RTE_SCHED_CMAN_RED) 
		{
			for (k = 0; k < RTE_COLORS; k++) 
			{
				subport_p->cman_params->red_params[j][k].min_th 		= cman_p.red_params[j][k].min_th;
				subport_p->cman_params->red_params[j][k].max_th 		= cman_p.red_params[j][k].max_th;
				subport_p->cman_params->red_params[j][k].maxp_inv 		= cman_p.red_params[j][k].maxp_inv;
				subport_p->cman_params->red_params[j][k].wq_log2 		= cman_p.red_params[j][k].wq_log2;
			}
		} 
		else 
		{
			subport_p->cman_params->pie_params[j].qdelay_ref 			= cman_p.pie_params[j].qdelay_ref;
			subport_p->cman_params->pie_params[j].dp_update_interval 	= cman_p.pie_params[j].dp_update_interval;
			subport_p->cman_params->pie_params[j].max_burst 			= cman_p.pie_params[j].max_burst;
			subport_p->cman_params->pie_params[j].tailq_th 				= cman_p.pie_params[j].tailq_th;
		}
	}
}
#endif


int cfg_load_subport( struct rte_cfgfile * cfg, struct rte_sched_subport_params * subport_params)
{
	const char *entry;
	int i, j, k;

	if (!cfg || !subport_params)
		return -1;

	memset(app_pipe_to_profile, -1, sizeof(app_pipe_to_profile));
	memset(active_queues, 0, sizeof(active_queues));
	n_active_queues = 0;

#ifdef RTE_SCHED_CMAN
	struct rte_sched_cman_params cman_params = {
		.cman_mode = RTE_SCHED_CMAN_RED,
		.red_params = { },
	};

	if (rte_cfgfile_has_section(cfg, "red")) 
	{
		cman_params.cman_mode = RTE_SCHED_CMAN_RED;

		for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) 
		{
			char str[32];

			/* Parse RED min thresholds */
			snprintf(str, sizeof(str), "tc %d red min", i);
			entry = rte_cfgfile_get_entry(cfg, "red", str);
			if (entry) 
			{
				char *next;
				/* for each packet colour (green, yellow, red) */
				for (j = 0; j < RTE_COLORS; j++) {
					cman_params.red_params[i][j].min_th
						= (uint16_t)strtol(entry, &next, 10);
					if (next == NULL)
						break;
					entry = next;
				}
			}

			/* Parse RED max thresholds */
			snprintf(str, sizeof(str), "tc %d red max", i);
			entry = rte_cfgfile_get_entry(cfg, "red", str);
			if (entry) {
				char *next;
				/* for each packet colour (green, yellow, red) */
				for (j = 0; j < RTE_COLORS; j++) {
					cman_params.red_params[i][j].max_th
						= (uint16_t)strtol(entry, &next, 10);
					if (next == NULL)
						break;
					entry = next;
				}
			}

			/* Parse RED inverse mark probabilities */
			snprintf(str, sizeof(str), "tc %d red inv prob", i);
			entry = rte_cfgfile_get_entry(cfg, "red", str);
			if (entry) {
				char *next;
				/* for each packet colour (green, yellow, red) */
				for (j = 0; j < RTE_COLORS; j++) {
					cman_params.red_params[i][j].maxp_inv
						= (uint8_t)strtol(entry, &next, 10);

					if (next == NULL)
						break;
					entry = next;
				}
			}

			/* Parse RED EWMA filter weights */
			snprintf(str, sizeof(str), "tc %d red weight", i);
			entry = rte_cfgfile_get_entry(cfg, "red", str);
			if (entry) {
				char *next;
				/* for each packet colour (green, yellow, red) */
				for (j = 0; j < RTE_COLORS; j++) {
					cman_params.red_params[i][j].wq_log2
						= (uint8_t)strtol(entry, &next, 10);
					if (next == NULL)
						break;
					entry = next;
				}
			}
		}
	}

	if (rte_cfgfile_has_section(cfg, "pie")) {
		cman_params.cman_mode = RTE_SCHED_CMAN_PIE;

		for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
			char str[32];

			/* Parse Queue Delay Ref value */
			snprintf(str, sizeof(str), "tc %d qdelay ref", i);
			entry = rte_cfgfile_get_entry(cfg, "pie", str);
			if (entry)
				cman_params.pie_params[i].qdelay_ref =
					(uint16_t) atoi(entry);

			/* Parse Max Burst value */
			snprintf(str, sizeof(str), "tc %d max burst", i);
			entry = rte_cfgfile_get_entry(cfg, "pie", str);
			if (entry)
				cman_params.pie_params[i].max_burst =
					(uint16_t) atoi(entry);

			/* Parse Update Interval Value */
			snprintf(str, sizeof(str), "tc %d update interval", i);
			entry = rte_cfgfile_get_entry(cfg, "pie", str);
			if (entry)
				cman_params.pie_params[i].dp_update_interval =
					(uint16_t) atoi(entry);

			/* Parse Tailq Threshold Value */
			snprintf(str, sizeof(str), "tc %d tailq th", i);
			entry = rte_cfgfile_get_entry(cfg, "pie", str);
			if (entry)
				cman_params.pie_params[i].tailq_th =
					(uint16_t) atoi(entry);

		}
	}
#endif /* RTE_SCHED_CMAN */

	for (i = 0; i < MAX_SCHED_SUBPORTS; i++) {
		char sec_name[CFG_NAME_LEN];
		snprintf(sec_name, sizeof(sec_name), "subport %d", i);

		if (rte_cfgfile_has_section(cfg, sec_name)) {
			entry = rte_cfgfile_get_entry(cfg, sec_name,
				"number of pipes per subport");
			if (entry)
				subport_params[i].n_pipes_per_subport_enabled =
					(uint32_t)atoi(entry);

			entry = rte_cfgfile_get_entry(cfg, sec_name, "queue sizes");
			if (entry) {
				char *next;

				for (j = 0; j < RTE_SCHED_TRAFFIC_CLASS_BE; j++) {
					subport_params[i].qsize[j] =
						(uint16_t)strtol(entry, &next, 10);
					if (subport_params[i].qsize[j] != 0) {
						active_queues[n_active_queues] = j;
						n_active_queues++;
					}
					if (next == NULL)
						break;
					entry = next;
				}

				subport_params[i].qsize[RTE_SCHED_TRAFFIC_CLASS_BE] =
					(uint16_t)strtol(entry, &next, 10);

				for (j = 0; j < RTE_SCHED_BE_QUEUES_PER_PIPE; j++) {
					active_queues[n_active_queues] =
						RTE_SCHED_TRAFFIC_CLASS_BE + j;
					n_active_queues++;
				}
			}

			int n_entries = rte_cfgfile_section_num_entries(cfg, sec_name);
			struct rte_cfgfile_entry entries[n_entries];

			rte_cfgfile_section_entries(cfg, sec_name, entries, n_entries);

			for (j = 0; j < n_entries; j++) {
				if (strncmp("pipe", entries[j].name, sizeof("pipe") - 1) == 0) {
					int profile;
					char *tokens[2] = {NULL, NULL};
					int n_tokens;
					int begin, end;

					profile = atoi(entries[j].value);
					n_tokens = rte_strsplit(&entries[j].name[sizeof("pipe")],
							strnlen(entries[j].name, CFG_NAME_LEN), tokens, 2, '-');

					begin =  atoi(tokens[0]);
					if (n_tokens == 2)
						end = atoi(tokens[1]);
					else
						end = begin;

					if (end >= MAX_SCHED_PIPES || begin > end)
						return -1;

					for (k = begin; k <= end; k++) {
						char profile_name[CFG_NAME_LEN];

						snprintf(profile_name, sizeof(profile_name),
								"pipe profile %d", profile);
						if (rte_cfgfile_has_section(cfg, profile_name))
							app_pipe_to_profile[i][k] = profile;
						else
							rte_exit(EXIT_FAILURE, "Wrong pipe profile %s\n",
									entries[j].value);

					}
				}
			}
#ifdef RTE_SCHED_CMAN
			set_subport_cman_params(subport_params+i, cman_params);
#endif
		}
	}

	return 0;
}





int qavg_q( uint16_t port_id, uint32_t subport_id, uint32_t pipe_id, uint8_t tc, uint8_t q)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint32_t count, i, queue_id = 0;
	uint32_t average;

	for (i = 0; i < nb_pfc; i++) 
	{
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc || subport_id >= port_params.n_subports_per_port ||
		pipe_id >= subport_params[subport_id].n_pipes_per_subport_enabled  ||
		tc >= RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE ||
		q >= RTE_SCHED_BE_QUEUES_PER_PIPE ||
		(tc < RTE_SCHED_TRAFFIC_CLASS_BE && q > 0))
		return -1;

	port = qos_conf[i].sched_port;
	for (i = 0; i < subport_id; i++)
		queue_id += subport_params[i].n_pipes_per_subport_enabled * RTE_SCHED_QUEUES_PER_PIPE;
	
	if (tc < RTE_SCHED_TRAFFIC_CLASS_BE)
		queue_id += pipe_id * RTE_SCHED_QUEUES_PER_PIPE + tc;
	else
		queue_id += pipe_id * RTE_SCHED_QUEUES_PER_PIPE + tc + q;

	average = 0;
	for (count = 0; count < qavg_ntimes; count++) 
	{
		rte_sched_queue_read_stats(port, queue_id, &stats, &qlen);
		average += qlen;
		usleep(qavg_period);
	}

	average /= qavg_ntimes;

	printf("\nAverage queue size: %" PRIu32 " bytes.\n\n", average);

	return 0;
}


int qavg_tcpipe( uint16_t port_id, uint32_t subport_id, uint32_t pipe_id, uint8_t tc)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint32_t count, i, queue_id = 0;
	uint32_t average, part_average;

	for (i = 0; i < nb_pfc; i++) 
	{
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc || subport_id >= port_params.n_subports_per_port ||
		pipe_id >= subport_params[subport_id].n_pipes_per_subport_enabled ||
		tc >= RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE)
		return -1;

	port = qos_conf[i].sched_port;

	for (i = 0; i < subport_id; i++)
		queue_id +=
			subport_params[i].n_pipes_per_subport_enabled *
			RTE_SCHED_QUEUES_PER_PIPE;

	queue_id += pipe_id * RTE_SCHED_QUEUES_PER_PIPE + tc;

	average = 0;

	for (count = 0; count < qavg_ntimes; count++) {
		part_average = 0;

		if (tc < RTE_SCHED_TRAFFIC_CLASS_BE) {
			rte_sched_queue_read_stats(port, queue_id,
				&stats, &qlen);
			part_average += qlen;
		} else {
			for (i = 0; i < RTE_SCHED_BE_QUEUES_PER_PIPE; i++) {
				rte_sched_queue_read_stats(port, queue_id + i,
					&stats, &qlen);
				part_average += qlen;
			}
			average += part_average / RTE_SCHED_BE_QUEUES_PER_PIPE;
		}
		usleep(qavg_period);
	}

	average /= qavg_ntimes;

	printf("\nAverage queue size: %" PRIu32 " bytes.\n\n", average);

	return 0;
}


int qavg_pipe(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint32_t count, i, queue_id = 0;
	uint32_t average, part_average;

	for (i = 0; i < nb_pfc; i++) {
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc ||
		subport_id >= port_params.n_subports_per_port ||
		pipe_id >= subport_params[subport_id].n_pipes_per_subport_enabled)
		return -1;

	port = qos_conf[i].sched_port;

	for (i = 0; i < subport_id; i++)
		queue_id += subport_params[i].n_pipes_per_subport_enabled *
				RTE_SCHED_QUEUES_PER_PIPE;

	queue_id += pipe_id * RTE_SCHED_QUEUES_PER_PIPE;

	average = 0;

	for (count = 0; count < qavg_ntimes; count++) {
		part_average = 0;
		for (i = 0; i < RTE_SCHED_QUEUES_PER_PIPE; i++) {
			rte_sched_queue_read_stats(port, queue_id + i,
				&stats, &qlen);
			part_average += qlen;
		}
		average += part_average / RTE_SCHED_QUEUES_PER_PIPE;
		usleep(qavg_period);
	}

	average /= qavg_ntimes;

	printf("\nAverage queue size: %" PRIu32 " bytes.\n\n", average);

	return 0;
}

int qavg_tcsubport(uint16_t port_id, uint32_t subport_id, uint8_t tc)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint32_t queue_id, count, i, j, subport_queue_id = 0;
	uint32_t average, part_average;

	for (i = 0; i < nb_pfc; i++) {
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc ||
		subport_id >= port_params.n_subports_per_port ||
		tc >= RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE)
		return -1;

	port = qos_conf[i].sched_port;

	for (i = 0; i < subport_id; i++)
		subport_queue_id +=
			subport_params[i].n_pipes_per_subport_enabled *
			RTE_SCHED_QUEUES_PER_PIPE;

	average = 0;

	for (count = 0; count < qavg_ntimes; count++) {
		uint32_t n_pipes_per_subport =
			subport_params[subport_id].n_pipes_per_subport_enabled;

		part_average = 0;
		for (i = 0; i < n_pipes_per_subport; i++) {
			if (tc < RTE_SCHED_TRAFFIC_CLASS_BE) {
				queue_id = subport_queue_id +
					i * RTE_SCHED_QUEUES_PER_PIPE + tc;
				rte_sched_queue_read_stats(port, queue_id,
					&stats, &qlen);
				part_average += qlen;
			} else {
				for (j = 0; j < RTE_SCHED_BE_QUEUES_PER_PIPE; j++) {
					queue_id = subport_queue_id +
							i * RTE_SCHED_QUEUES_PER_PIPE +
							tc + j;
					rte_sched_queue_read_stats(port, queue_id,
						&stats, &qlen);
					part_average += qlen;
				}
			}
		}

		if (tc < RTE_SCHED_TRAFFIC_CLASS_BE)
			average += part_average /
				(subport_params[subport_id].n_pipes_per_subport_enabled);
		else
			average += part_average /
				(subport_params[subport_id].n_pipes_per_subport_enabled) *
				RTE_SCHED_BE_QUEUES_PER_PIPE;

		usleep(qavg_period);
	}

	average /= qavg_ntimes;

	printf("\nAverage queue size: %" PRIu32 " bytes.\n\n", average);

	return 0;
}


int qavg_subport(uint16_t port_id, uint32_t subport_id)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint32_t queue_id, count, i, j, subport_queue_id = 0;
	uint32_t average, part_average;

	for (i = 0; i < nb_pfc; i++) {
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc ||
		subport_id >= port_params.n_subports_per_port)
		return -1;

	port = qos_conf[i].sched_port;

	for (i = 0; i < subport_id; i++)
		subport_queue_id += subport_params[i].n_pipes_per_subport_enabled *
			RTE_SCHED_QUEUES_PER_PIPE;

	average = 0;

	for (count = 0; count < qavg_ntimes; count++) {
		uint32_t n_pipes_per_subport =
			subport_params[subport_id].n_pipes_per_subport_enabled;

		part_average = 0;
		for (i = 0; i < n_pipes_per_subport; i++) {
			queue_id = subport_queue_id + i * RTE_SCHED_QUEUES_PER_PIPE;

			for (j = 0; j < RTE_SCHED_QUEUES_PER_PIPE; j++) {
				rte_sched_queue_read_stats(port, queue_id + j,
					&stats, &qlen);
				part_average += qlen;
			}
		}

		average += part_average /
			(subport_params[subport_id].n_pipes_per_subport_enabled *
			RTE_SCHED_QUEUES_PER_PIPE);
		usleep(qavg_period);
	}

	average /= qavg_ntimes;

	printf("\nAverage queue size: %" PRIu32 " bytes.\n\n", average);

	return 0;
}


int subport_stat(uint16_t port_id, uint32_t subport_id)
{
	struct rte_sched_subport_stats stats;
	struct rte_sched_port *port;
	uint32_t tc_ov[RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE];
	uint8_t i;

	for (i = 0; i < nb_pfc; i++) {
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc || subport_id >= port_params.n_subports_per_port)
		return -1;

	port = qos_conf[i].sched_port;
	memset(tc_ov, 0, sizeof(tc_ov));

	rte_sched_subport_read_stats(port, subport_id, &stats, tc_ov);

	printf("\n");
	printf("+----+-------------+-------------+-------------+-------------+-------------+\n");
	printf("| TC |   Pkts OK   |Pkts Dropped |  Bytes OK   |Bytes Dropped|  OV Status  |\n");
	printf("+----+-------------+-------------+-------------+-------------+-------------+\n");

	for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
		printf("|  %d | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu32 " |\n",
			i, stats.n_pkts_tc[i], stats.n_pkts_tc_dropped[i],
		stats.n_bytes_tc[i], stats.n_bytes_tc_dropped[i], tc_ov[i]);
		printf("+----+-------------+-------------+-------------+-------------+-------------+\n");
	}
	printf("\n");

	return 0;
}


int pipe_stat(uint16_t port_id, uint32_t subport_id, uint32_t pipe_id)
{
	struct rte_sched_queue_stats stats;
	struct rte_sched_port *port;
	uint16_t qlen;
	uint8_t i, j;
	uint32_t queue_id = 0;

	for (i = 0; i < nb_pfc; i++) {
		if (qos_conf[i].tx_port == port_id)
			break;
	}

	if (i == nb_pfc ||
		subport_id >= port_params.n_subports_per_port ||
		pipe_id >= subport_params[subport_id].n_pipes_per_subport_enabled)
		return -1;

	port = qos_conf[i].sched_port;
	for (i = 0; i < subport_id; i++)
		queue_id += subport_params[i].n_pipes_per_subport_enabled *
			RTE_SCHED_QUEUES_PER_PIPE;

	queue_id += pipe_id * RTE_SCHED_QUEUES_PER_PIPE;

	printf("\n");
	printf("+----+-------+-------------+-------------+-------------+-------------+-------------+\n");
	printf("| TC | Queue |   Pkts OK   |Pkts Dropped |  Bytes OK   |Bytes Dropped|    Length   |\n");
	printf("+----+-------+-------------+-------------+-------------+-------------+-------------+\n");

	for (i = 0; i < RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE; i++) {
		if (i < RTE_SCHED_TRAFFIC_CLASS_BE) {
			rte_sched_queue_read_stats(port, queue_id + i, &stats, &qlen);
			printf("|  %d |   %d   | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11i |\n",
				i, 0, stats.n_pkts, stats.n_pkts_dropped, stats.n_bytes,
				stats.n_bytes_dropped, qlen);
			printf("+----+-------+-------------+-------------+-------------+-------------+-------------+\n");
		} else {
			for (j = 0; j < RTE_SCHED_BE_QUEUES_PER_PIPE; j++) {
				rte_sched_queue_read_stats(port, queue_id + i + j,
					&stats, &qlen);
				printf("|  %d |   %d   | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11" PRIu64 " | %11i |\n",
					i, j, stats.n_pkts, stats.n_pkts_dropped, stats.n_bytes,
					stats.n_bytes_dropped, qlen);
				printf("+----+-------+-------------+-------------+-------------+-------------+-------------+\n");
			}
		}
	}
	printf("\n");

	return 0;
}



static inline int get_pkt_sched(struct rte_mbuf *m, uint32_t *subport, uint32_t *pipe, uint32_t *traffic_class, uint32_t *queue, uint32_t *color)
{
	uint16_t *pdata = rte_pktmbuf_mtod(m, uint16_t *);
	uint16_t pipe_queue;

	/* Outer VLAN ID*/
	*subport = (rte_be_to_cpu_16(pdata[SUBPORT_OFFSET]) & 0x0FFF) &
		(port_params.n_subports_per_port - 1);

	/* Inner VLAN ID */
	*pipe = (rte_be_to_cpu_16(pdata[PIPE_OFFSET]) & 0x0FFF) &
		(subport_params[*subport].n_pipes_per_subport_enabled - 1);

	pipe_queue = active_queues[(pdata[QUEUE_OFFSET] >> 8) % n_active_queues];

	/* Traffic class (Destination IP) */
	*traffic_class = pipe_queue > RTE_SCHED_TRAFFIC_CLASS_BE ?
			RTE_SCHED_TRAFFIC_CLASS_BE : pipe_queue;

	/* Traffic class queue (Destination IP) */
	*queue = pipe_queue - *traffic_class;

	/* Color (Destination IP) */
	*color = pdata[COLOR_OFFSET] & 0x03;

	return 0;
}


void app_rx_thread(struct thread_conf **confs)
{
	uint32_t i, nb_rx;
	struct rte_mbuf *rx_mbufs[burst_conf.rx_burst] __rte_cache_aligned;
	struct thread_conf *conf;
	int conf_idx = 0;

	uint32_t subport;
	uint32_t pipe;
	uint32_t traffic_class;
	uint32_t queue;
	uint32_t color;
	
	
	printf( "X----------ddddddddddddddddddddddddddddeeeeeeeeeS\n");
	
	while ((conf = confs[conf_idx])) 
	{
		printf( "mbuf_pool-.%p\n", qos_conf->mbuf_pool);
		sleep(1);
		nb_rx = rte_eth_rx_burst(conf->rx_port, conf->rx_queue, rx_mbufs, burst_conf.rx_burst);

		if (likely(nb_rx != 0)) {
			APP_STATS_ADD(conf->stat.nb_rx, nb_rx);

			for(i = 0; i < nb_rx; i++) {
				get_pkt_sched(rx_mbufs[i],
						&subport, &pipe, &traffic_class, &queue, &color);
				rte_sched_port_pkt_write(conf->sched_port,
						rx_mbufs[i],
						subport, pipe,
						traffic_class, queue,
						(enum rte_color) color);
			}

			if (unlikely(rte_ring_sp_enqueue_bulk(conf->rx_ring,
					(void **)rx_mbufs, nb_rx, NULL) == 0)) {
				for(i = 0; i < nb_rx; i++) {
					rte_pktmbuf_free(rx_mbufs[i]);

					APP_STATS_ADD(conf->stat.nb_drop, 1);
				}
			}
		}
		conf_idx++;
		if (confs[conf_idx] == NULL)
			conf_idx = 0;
	}
}



/* Send the packet to an output interface
 * For performance reason function returns number of packets dropped, not sent,
 * so 0 means that all packets were sent successfully
 */

static inline void app_send_burst(struct thread_conf *qconf)
{
	struct rte_mbuf **mbufs;
	uint32_t n, ret;

	mbufs = (struct rte_mbuf **)qconf->m_table;
	n = qconf->n_mbufs;

	do {
		ret = rte_eth_tx_burst(qconf->tx_port, qconf->tx_queue, mbufs, (uint16_t)n);
		/* we cannot drop the packets, so re-send */
		/* update number of packets to be sent */
		n -= ret;
		mbufs = (struct rte_mbuf **)&mbufs[ret];
	} while (n);
}


/* Send the packet to an output interface */
static void app_send_packets(struct thread_conf *qconf, struct rte_mbuf **mbufs, uint32_t nb_pkt)
{
	uint32_t i, len;

	len = qconf->n_mbufs;
	for(i = 0; i < nb_pkt; i++) {
		qconf->m_table[len] = mbufs[i];
		len++;
		/* enough pkts to be sent */
		if (unlikely(len == burst_conf.tx_burst)) {
			qconf->n_mbufs = len;
			app_send_burst(qconf);
			len = 0;
		}
	}

	qconf->n_mbufs = len;
}


void app_tx_thread(struct thread_conf **confs)
{
	struct rte_mbuf *mbufs[burst_conf.qos_dequeue];
	struct thread_conf *conf;
	int conf_idx = 0;
	int retval;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;

	while ((conf = confs[conf_idx])) {
		retval = rte_ring_sc_dequeue_bulk(conf->tx_ring, (void **)mbufs,
					burst_conf.qos_dequeue, NULL);
		if (likely(retval != 0)) {
			app_send_packets(conf, mbufs, burst_conf.qos_dequeue);

			conf->counter = 0; /* reset empty read loop counter */
		}

		conf->counter++;

		/* drain ring and TX queues */
		if (unlikely(conf->counter > drain_tsc)) {
			/* now check is there any packets left to be transmitted */
			if (conf->n_mbufs != 0) {
				app_send_burst(conf);

				conf->n_mbufs = 0;
			}
			conf->counter = 0;
		}

		conf_idx++;
		if (confs[conf_idx] == NULL)
			conf_idx = 0;
	}
}


void app_worker_thread(struct thread_conf **confs)
{
	struct rte_mbuf *mbufs[burst_conf.ring_burst];
	struct thread_conf *conf;
	int conf_idx = 0;

	while ((conf = confs[conf_idx])) {
		uint32_t nb_pkt;

		/* Read packet from the ring */
		nb_pkt = rte_ring_sc_dequeue_burst(conf->rx_ring, (void **)mbufs,
					burst_conf.ring_burst, NULL);
		if (likely(nb_pkt)) {
			int nb_sent = rte_sched_port_enqueue(conf->sched_port, mbufs,
					nb_pkt);

			APP_STATS_ADD(conf->stat.nb_drop, nb_pkt - nb_sent);
			APP_STATS_ADD(conf->stat.nb_rx, nb_pkt);
		}

		nb_pkt = rte_sched_port_dequeue(conf->sched_port, mbufs,
					burst_conf.qos_dequeue);
		if (likely(nb_pkt > 0))
			while (rte_ring_sp_enqueue_bulk(conf->tx_ring,
					(void **)mbufs, nb_pkt, NULL) == 0)
				; /* empty body */

		conf_idx++;
		if (confs[conf_idx] == NULL)
			conf_idx = 0;
	}
}


void app_mixed_thread(struct thread_conf **confs)
{
	struct rte_mbuf *mbufs[burst_conf.ring_burst];
	struct thread_conf *conf;
	int conf_idx = 0;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;

	while ((conf = confs[conf_idx])) {
		uint32_t nb_pkt;

		/* Read packet from the ring */
		nb_pkt = rte_ring_sc_dequeue_burst(conf->rx_ring, (void **)mbufs,
					burst_conf.ring_burst, NULL);
		if (likely(nb_pkt)) {
			int nb_sent = rte_sched_port_enqueue(conf->sched_port, mbufs,
					nb_pkt);

			APP_STATS_ADD(conf->stat.nb_drop, nb_pkt - nb_sent);
			APP_STATS_ADD(conf->stat.nb_rx, nb_pkt);
		}


		nb_pkt = rte_sched_port_dequeue(conf->sched_port, mbufs,
					burst_conf.qos_dequeue);
		if (likely(nb_pkt > 0)) {
			app_send_packets(conf, mbufs, nb_pkt);

			conf->counter = 0; /* reset empty read loop counter */
		}

		conf->counter++;

		/* drain ring and TX queues */
		if (unlikely(conf->counter > drain_tsc)) {

			/* now check is there any packets left to be transmitted */
			if (conf->n_mbufs != 0) {
				app_send_burst(conf);

				conf->n_mbufs = 0;
			}
			conf->counter = 0;
		}

		conf_idx++;
		if (confs[conf_idx] == NULL)
			conf_idx = 0;
	}
}


static int app_main_loop(__rte_unused void *dummy)
{
	uint32_t lcore_id;
	uint32_t i, mode;
	uint32_t rx_idx = 0;
	uint32_t wt_idx = 0;
	uint32_t tx_idx = 0;
	
	struct thread_conf *rx_confs[MAX_DATA_STREAMS];
	struct thread_conf *wt_confs[MAX_DATA_STREAMS];
	struct thread_conf *tx_confs[MAX_DATA_STREAMS];

	memset(rx_confs, 0, sizeof(rx_confs));
	memset(wt_confs, 0, sizeof(wt_confs));
	memset(tx_confs, 0, sizeof(tx_confs));


	mode = APP_MODE_NONE;
	lcore_id = rte_lcore_id();

	for (i = 0; i < nb_pfc; i++) 
	{
		struct flow_conf *flow = &qos_conf[i];

		if (flow->rx_core == lcore_id) 
		{
			flow->rx_thread.rx_port = flow->rx_port;
			flow->rx_thread.rx_ring =  flow->rx_ring;
			flow->rx_thread.rx_queue = flow->rx_queue;
			flow->rx_thread.sched_port = flow->sched_port;

			rx_confs[rx_idx++] = &flow->rx_thread;

			mode |= APP_RX_MODE;
		}
		
		if (flow->tx_core == lcore_id) 
		{
			flow->tx_thread.tx_port = flow->tx_port;
			flow->tx_thread.tx_ring =  flow->tx_ring;
			flow->tx_thread.tx_queue = flow->tx_queue;

			tx_confs[tx_idx++] = &flow->tx_thread;

			mode |= APP_TX_MODE;
		}
		
		if (flow->wt_core == lcore_id) 
		{
			flow->wt_thread.rx_ring 	=  flow->rx_ring;
			flow->wt_thread.tx_ring 	=  flow->tx_ring;
			flow->wt_thread.tx_port 	=  flow->tx_port;
			flow->wt_thread.sched_port 	=  flow->sched_port;
			flow->wt_thread.sched_port 	=  flow->sched_port;
			flow->wt_thread.mbuf_pool 	=  flow->mbuf_pool;
			
			printf( "mbuf_pool=%p\n", flow->mbuf_pool);
			
			wt_confs[wt_idx++] = &flow->wt_thread;

			mode |= APP_WT_MODE;
			//wt_confs->mbuf_pool = 
		}
	}

	if (mode == APP_MODE_NONE) 
	{
		RTE_LOG(INFO, APP, "lcore %u has nothing to do\n", lcore_id);
		return -1;
	}

	if (mode == (APP_RX_MODE | APP_WT_MODE)) 
	{
		RTE_LOG(INFO, APP, "lcore %u was configured for both RX and WT !!!\n", lcore_id);
		return -1;
	}

	RTE_LOG(INFO, APP, "entering main loop on lcore %u\n", lcore_id);

	
	//printf("APP_RX_MODE=%u %d  ---------------------------------SSSSSSSSSSSSSSSSSSSSS\n", mode, __LINE__);

	/* initialize mbuf memory */
	if (mode == APP_RX_MODE) 
	{
		printf("mode line %d\n", __LINE__);
		
		for (i = 0; i < rx_idx; i++) 
		{
			RTE_LOG(INFO, APP, "flow%u lcoreid%u reading port%u\n", i, lcore_id, rx_confs[i]->rx_port);
		}

		app_rx_thread(rx_confs);
	}
	else if (mode == (APP_TX_MODE | APP_WT_MODE)) 
	{
		printf("mode line %d\n", __LINE__);
		
		for (i = 0; i < wt_idx; i++) 
		{
			wt_confs[i]->m_table = rte_malloc("table_wt", sizeof(struct rte_mbuf *) * burst_conf.tx_burst, RTE_CACHE_LINE_SIZE);

			if (wt_confs[i]->m_table == NULL)
				rte_panic("flow %u unable to allocate memory buffer\n", i);

			RTE_LOG( INFO, APP, "flow %u lcoreid %u sched+write port %u\n", i, lcore_id, wt_confs[i]->tx_port);
		}
		
		
		
		app_mixed_thread(wt_confs);
	}
	else if (mode == APP_TX_MODE) 
	{
		printf("mode line %d\n", __LINE__);
		
		for (i = 0; i < tx_idx; i++) 
		{
			tx_confs[i]->m_table = rte_malloc("table_tx", sizeof(struct rte_mbuf *) * burst_conf.tx_burst, RTE_CACHE_LINE_SIZE);

			if (tx_confs[i]->m_table == NULL)
				rte_panic("flow %u unable to allocate memory buffer\n", i);

			RTE_LOG(INFO, APP, "flow%u lcoreid%u write port%u\n", i, lcore_id, tx_confs[i]->tx_port);
		}

		app_tx_thread(tx_confs);
	}
	else if (mode == APP_WT_MODE)
	{
		for (i = 0; i < wt_idx; i++) 
		{
			RTE_LOG(INFO, APP, "flow %u lcoreid %u scheduling \n", i, lcore_id);
		}

		app_worker_thread(wt_confs);
	}

	return 0;
}

void app_stat(void)
{
	uint32_t i;
	struct rte_eth_stats stats;
	static struct rte_eth_stats rx_stats[MAX_DATA_STREAMS];
	static struct rte_eth_stats tx_stats[MAX_DATA_STREAMS];

	/* print statistics */
	for(i = 0; i < nb_pfc; i++) 
	{
		struct flow_conf *flow = &qos_conf[i];

		rte_eth_stats_get(flow->rx_port, &stats);
		
		printf("\nRX port %"PRIu16": rx: %"PRIu64 " err: %"PRIu64 " no_mbuf: %"PRIu64 "\n",
				flow->rx_port, stats.ipackets - rx_stats[i].ipackets, stats.ierrors - rx_stats[i].ierrors,
				stats.rx_nombuf - rx_stats[i].rx_nombuf);

		memcpy(&rx_stats[i], &stats, sizeof(stats));

		rte_eth_stats_get(flow->tx_port, &stats);
		
		printf("TX port %"PRIu16": tx: %" PRIu64 " err: %" PRIu64 "\n", flow->tx_port,
				stats.opackets - tx_stats[i].opackets,
				stats.oerrors - tx_stats[i].oerrors);

		memcpy(&tx_stats[i], &stats, sizeof(stats));

#if APP_COLLECT_STAT
		printf("-------+------------+------------+\n");
		printf("       |  received  |   dropped  |\n");
		printf("-------+------------+------------+\n");
		printf("  RX   | %10" PRIu64 " | %10" PRIu64 " |\n", flow->rx_thread.stat.nb_rx, flow->rx_thread.stat.nb_drop);
		printf("QOS+TX | %10" PRIu64 " | %10" PRIu64 " |   pps: %"PRIu64 " \n", flow->wt_thread.stat.nb_rx,
			flow->wt_thread.stat.nb_drop, flow->wt_thread.stat.nb_rx - flow->wt_thread.stat.nb_drop);
		printf("-------+------------+------------+\n");

		memset(&flow->rx_thread.stat, 0, sizeof(struct thread_stat));
		memset(&flow->wt_thread.stat, 0, sizeof(struct thread_stat));
#endif
	}
}

// ./qosc -l 1,5,7 -n 4 -- --pfc "3,2,5,7" --cfg ./profile.cfg
// ./qosc -l 1,5,7 -n 4 -- --pfc "0,0,0,7" --cfg ./profile.cfg
// ./qosc -l 1,2,6,7 -n 4 --file-prefix 102  -- --pfc "0,0,0,7" --cfg ./profile.cfg 
// ./build/qos_sched -l 1

int main(int argc, char **argv)
{
	int ret;

	ret = app_parse_args(argc, argv);
	if (ret < 0)
		return -1;

	ret = app_init();
	if (ret < 0)
		return -1;

	printf("%d completed\n", __LINE__);
	

	/* launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch( app_main_loop, NULL, SKIP_MAIN);
	printf("%d completed\n", __LINE__);
	

	if (interactive) 
	{
		sleep(1);
		//prompt();
		printf("Not Implemented\n");
	}
	else 
	{
		printf("2\n");
		/* print statistics every second */
		while(1) 
		{
			printf("2.1\n");
			sleep(1);
			app_stat();
		}
	}

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}































