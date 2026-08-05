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
#include <rte_icmp.h>

#include <pcap/pcap.h>
#include <pcap/bpf.h>

#include <jansson.h>
#include "dhcp.h"
#include "app_stack.h"
#include "app_endpoint.h"



int 						dpdk_qos__init( char * configfile);
struct rte_sched_port * 	dpdk_qos__create_port( char * name, int socket, uint64_t link_rate, int side);
uint16_t 					dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes);
uint16_t 					dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes);
void 						dpdk_qos__find_user_pipe_and_q( uint64_t seid, uint16_t * pipe, uint16_t * nongbr_queue, uint16_t * gbr_queue);




#define		PROCESS_TYPE__NONE					0
#define		PROCESS_TYPE__GTP_Gi				1
#define		PROCESS_TYPE__Gi_GTP				2
#define		PROCESS_TYPE__Gi_Gi_INGRESS			3
#define		PROCESS_TYPE__Gi_Gi_EGRESS			4




typedef struct __dict_item
{
	struct __dict_item * Next;
	
	char key[128];
	char value[256];
} __dict_item_t;


typedef struct __dict
{
	__dict_item_t items[25];
	int count;
} __dict_t;


char * ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


char * rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}


char * trim(char *s)
{
    return rtrim(ltrim(s)); 
}


int __dpdk_env__read_cmd_output( char * output, char * command)
{
	FILE *fp;
	char line[256];
	memset( line, 0, sizeof(line));
	
	/* Open the command for reading. */
	fp = popen( command, "r");
	if (fp == NULL) 
	{
		printf("Failed to run command\n" );
		return 0;
	}
	
	int pos = 0;
	int slen = 0;
	
	/* Read the output a line at a time - output it. */
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		//printf("--%s--%ld\n", line, strlen(line));
		
		slen = strlen(line);
		memcpy( &output[pos], line, slen);
		pos += slen;

		// memcpy( &output[pos], "\n", 1);
		// pos++;
	}
	

	/* close */
	pclose(fp);	

	return 1;
}


int __dpdk_env__get_driver_info( __dict_t * dict, char * busid)
{
	char cmd[128];
	memset( cmd, 0, sizeof(cmd));
	
	if( memcmp( busid, "0000.", 5) == 0) {
		sprintf( cmd, "lspci -vmmks %s", &busid[5]);
	} else {
		sprintf( cmd, "lspci -vmmks %s", busid);
	}

	
	char output[1024];
	memset( output, 0, sizeof(output));
	__dpdk_env__read_cmd_output( output, cmd);
	
	
	//__dict_t dict;
	dict->count = 0;
	
	//printf( "-%s-\n", output);
	int i = 0;
	int begin_pos = 0;
	int end_pos = 0;
	char temp[256];
	
	char * save_ptr = NULL;
	char * key_ptr = NULL;
	
	while(i < 1024)
	{
		if( output[i] == '\n')
		{
			end_pos = i;
			
			if( end_pos-begin_pos > 0)
			{
				key_ptr = NULL;
				save_ptr = NULL;
				
				memset( temp, 0, sizeof(temp));
				memcpy( temp, &output[begin_pos], end_pos-begin_pos);
				//printf( "-%s-\n", temp);
				
				key_ptr = strtok_r( temp, ":", &save_ptr);
				
				if( key_ptr) 
				{
					//printf( "key=%s\nval=%s\n", key_ptr, save_ptr);
				
					if( strlen( key_ptr) > 0) 
					{
						memset( &dict->items[dict->count], 0, sizeof(__dict_item_t));
						memcpy( dict->items[dict->count].key, key_ptr, strlen(key_ptr));
						
						if( save_ptr) {
							if( strlen(save_ptr) > 0) {
								memcpy( dict->items[dict->count].value, save_ptr, strlen(save_ptr));
							}
						}
						dict->count++;
					}
				}
			
			}
			
			begin_pos 	= (i + 1);
		}
		i++;
	}

	// i = 0;
	// while( i < dict.count)
	// {
		// printf( "%d %s: -%s-\n", i, trim(dict.items[i].key), trim(dict.items[i].value));
		// i++;
	// }
	
	return 1;
}



int __dpdk_env__is_dpdk_driver_findkey( __dict_t * dict, char * findkey)
{
	int i = 0;
	char * key = NULL;
	
	while( i < dict->count)
	{
		key = trim( dict->items[i].key);
		
		if( strlen( key) == strlen( findkey) && strlen( key) > 0)
		{
			if( memcmp( findkey, key, strlen( key)) == 0)
			{
				return i;
			}
		}	
		i++;
	}
	
	return -1;
}


int __dpdk_env__is_dpdk_driver_attached( char * busid)
{
	__dict_t dict;
	__dpdk_env__get_driver_info( &dict, busid);

	int index = __dpdk_env__is_dpdk_driver_findkey( &dict, "Driver");

	if( memcmp( "vfio-pci", trim(dict.items[index].value), 8) == 0)
	{
		return 1;
	}

	return 0;
}



int __dpdk_env__is_numa()
{
	struct stat sb;
	char * folder = "/sys/devices/system/node";

	if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) 
	{
		return 1;
	} 	
	
	return 0;
}

int __dpdk_env__get_numa_node_exits( int i)
{
	struct stat sb;
	char folder[1024];
	memset( folder, 0, sizeof(folder));
	sprintf( folder, "/sys/devices/system/node/node%d/", i);

	if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) 
	{
		return 1;
	} 	
	
	return 0;
}

int __dpdk_env__find_full_numa_path_exits( int i)
{
	struct stat sb;
	char folder[1024];
	memset( folder, 0, sizeof(folder));
	sprintf( folder, "/sys/devices/system/node/node%d/hugepages/hugepages-2048kB/", i);

	if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) 
	{
		return 1;
	}
	else
	{
		printf( "numa node path not found=%s\n", folder);
	}
	
	return 0;
}



void __dpdk_env__set_hugepages( int i, int numa)
{
	char folder[1024];
	memset( folder, 0, sizeof(folder));
	
	if( numa == 1) {
		sprintf( folder, "echo 2048 > /sys/devices/system/node/node%d/hugepages/hugepages-2048kB/nr_hugepages", i);
	} else {
		sprintf( folder, "echo 2048 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages");
	}
	
	int sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));
}


void __dpdk_env__create_huge_pages()
{
	struct stat sb;
	char * folder = "/dev/hugepages/";

	if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) 
	{
		printf("%s - YES exists\n", folder);
	} 
	else 
	{
		printf("%s - NOT exists., creating \n", folder);
		
		if( mkdir( folder, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		{
			printf("creating %s failed., run with sudo  \n", folder);
		}
		else
		{
			printf("creating %s success \n", folder);
		}
	}
}


int __dpdk_env__is_higetlbfs_mounted() 
{
	int mounted = 0;
	struct mntent * ent = NULL;
	FILE * aFile = NULL;

	aFile = setmntent("/proc/mounts", "r");
	if (aFile == NULL) 
	{
		perror("setmntent., opening failed - /proc/mounts ");
		return mounted;
	}

	while (NULL != (ent = getmntent(aFile))) 
	{
		//printf("%s %s\n", ent->mnt_fsname, ent->mnt_dir);
		if( strcmp( ent->mnt_fsname, "hugetlbfs") == 0 && strcmp( ent->mnt_dir, "/dev/hugepages") == 0)
		{
			mounted = 1;
			break;
		}
	}

	endmntent(aFile);
	return mounted;
}


void __dpdk_env__mount()
{
	char * command = "mount -t hugetlbfs nodev /dev/hugepages/";
	int sts = system( command);
	printf( "[%s] - command status = %d errno=%d|%s\n", command, sts, errno, strerror(errno));
}

void __dpdk_env__configure_hugepages_for_nodes()
{
	if( __dpdk_env__is_numa() == 1)
	{
		int i = 0;
		int exists = 0;
		int fexists = 0;
		
		while(1)
		{
			exists = __dpdk_env__get_numa_node_exits(i);
			
			if( exists == 1)
			{
				fexists = __dpdk_env__find_full_numa_path_exits( i);
				
				if( fexists)
				{
					__dpdk_env__set_hugepages( i, 1);
				}
				
				printf("configured numa node index-%d  fexists=%d\n", i, fexists);
				
				if( fexists == 0) {
					break;
				}
				
				i++;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		__dpdk_env__set_hugepages( 0, 0);
	}
	
	//printf( "total numa nodes attempted=%d\n", i);
}

void __dpdk_env__start__vfio_pci()
{
	char folder[1024];
	int sts = 0 ;
	
	
	memset( folder, 0, sizeof(folder));
	strcpy( folder, "sudo modprobe vfio-pci");
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));

	memset( folder, 0, sizeof(folder));
	strcpy( folder, "modprobe vfio enable_unsafe_noiommu_mode=1");
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));

	memset( folder, 0, sizeof(folder));
	strcpy( folder, "echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode");
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));	
	
	int i = 0;
	int maxtime = 5;
	
	while( i < maxtime)
	{
		sleep(1);
		printf( "wating for vfio-pci initalize %d of %d\n", i + 1, maxtime);
		i++;
	}	
}


void __dpdk_env__configure__pci( char * dpdk_abs_path, char * ifname, char * bus_info)
{
	//char * 	dpdk_abs_path 	= "/home/dpdk/dpdk/dpdk-stable-21.11.2/usertools/";
	int sts = 0 ;
	char folder[1024];

	memset( folder, 0, sizeof(folder));
	sprintf( folder, "sudo ifconfig %s down", ifname);
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));	
	
	memset( folder, 0, sizeof(folder));
	sprintf( folder, "%s%s %s", dpdk_abs_path, "dpdk-devbind.py --bind=vfio-pci", bus_info);
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));
}

void __dpdk_env__print__pci_status( char * dpdk_abs_path)
{
	char folder[1024];
	int sts = 0 ;
	
	memset( folder, 0, sizeof(folder));	
	sprintf( folder, "%s%s", dpdk_abs_path, "dpdk-devbind.py -s");
	sts = system( folder);
	printf( "[%s] - command status = %d errno=%d|%s\n", folder, sts, errno, strerror(errno));	
}






int cp__teid_allowed_v4( uint32_t ran_ip, uint32_t teid, uint8_t ** session, uint8_t * mac);
int cp__teid_allowed_v6( __uint128_t ran_ip, uint32_t teid, uint8_t ** session);
int cp__flow_allowed_sess_v4( uint8_t * session, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen, int direction, int coreid);
int cp__flow_allowed_ueipv4( uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  uint8_t ** session, uint32_t * ran_teid, int direction, int coreid, uint8_t * qfi);
int cp__get_ranv4info( uint8_t * session, uint8_t * ranip_v, uint32_t * ranip4, __uint128_t * ranip6, uint8_t ** mac);
void cp__create_session( uint32_t cp_f_seid, uint32_t ueip, uint32_t teid, uint32_t ranip, uint32_t ranteid);

typedef void (*TG__UpdateReceivedPacket)( uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint32_t pktlen);
TG__UpdateReceivedPacket dpe__recvPacketHandler = NULL;

void dpe__SetRecvPacketHandler( TG__UpdateReceivedPacket pH) 
{
	dpe__recvPacketHandler = pH;
}

void dpe__print_buffer( char * key, char * buf, int len, int space, int line)
{
	printf("%s:", key);
	
	int i = 0;
	for( i = 0; i < len; i++)
	{
		if( space == 1)
		{
			printf("%02X ", buf[i] & 0xFF);
		}
		else
		{
			printf("%02X", buf[i] & 0xFF);
		}
	}
	printf("\n");
}

pthread_mutex_t pcap_lock;


pcap_dumper_t * dpdk_pkt__open_pcap_dump( char * filename)
{
	pcap_t * pcap = pcap_open_dead_with_tstamp_precision( DLT_EN10MB, 2000, PCAP_TSTAMP_PRECISION_NANO);
	
	if (!pcap)
	{
		printf("pcap creation failed\n");
		return NULL;
	}
	pcap_dumper_t * dumper = pcap_dump_open( pcap, filename);
	
	if (!dumper)
	{
		printf("pcap_dump_fopen failed: %s\n", pcap_geterr(pcap));
		return NULL;
	}
	
	return dumper;
}



void dpdk_pkt__write_pcap( pcap_dumper_t * dumper, struct rte_mbuf * pkt)
{
	pthread_mutex_lock( &pcap_lock);
	
	uint8_t temp_data[2000];

	struct pcap_pkthdr header;
	gettimeofday( &header.ts, NULL);
	header.len = rte_pktmbuf_pkt_len( pkt);
	header.caplen = RTE_MIN( header.len, 2000);
	
	
	pcap_dump( (u_char *)dumper, &header, rte_pktmbuf_read( pkt, 0, header.caplen, temp_data));

	pthread_mutex_unlock( &pcap_lock);
}


void dpdk_pkt__close_pcap( pcap_dumper_t * dumper)
{
	if( dumper)
	{
		pcap_dump_close( dumper);
	}
}


typedef struct arp_entry
{
	uint32_t ip;
	uint8_t Mac[7];
} arp_entry_t;


typedef struct dpdk_nat_ip dpdk_nat_ip_t;
typedef struct dpe_ue_nat_ip dpe_ue_nat_ip_t;
typedef struct dpdk_interface dpdk_interface_t;

// user flow entry
typedef struct dpe_nat_entry 
{
	struct dpe_nat_entry * Prev;
	struct dpe_nat_entry * Next;
	dpdk_nat_ip_t * nat_ip;
	
	uint32_t UeIP;
	uint32_t UeDstIP;
	uint16_t UeDstPort;
	uint16_t UeSrcPort;
	uint16_t Protocol;
	uint16_t NatSrcPort;
} dpe_nat_entry_t;


// user 
typedef struct dpe_ue_nat_ip
{
	struct dpe_ue_nat_ip * Next;
	
	uint32_t ip;
	dpdk_nat_ip_t * nat_ip;
	
	dpe_nat_entry_t * entryHead;
	dpe_nat_entry_t * entryCurrent;
	pthread_mutex_t   entryLock;
	int 			  entryCount;
	int				  deleteInProgress;

} dpe_ue_nat_ip_t;


// associated with interafce
typedef struct dpdk_nat_ip
{
	struct dpdk_nat_ip * Next;
	dpdk_interface_t * interface;

	uint32_t ip;
	uint8_t MAC[6];
	
	app_rbtree_t * natTree;
	dpe_nat_entry_t ** Index; 
	dpe_nat_entry_t * nat_entry_head;
	dpe_nat_entry_t * nat_entry_current;
	pthread_mutex_t nat_entry_lock;
	app_data_pool_t * dPool;
	
	uint32_t nat_entry_count;
	uint32_t NATPort;
	
} dpdk_nat_ip_t;



typedef struct dpdk_interface
{
	struct dpdk_interface * Next;
	
	int 		Index;
	char 		Port[20];
	uint16_t  	PortId;
	uint8_t		MacAddr[8];
	uint8_t		RecvQueue;
	uint8_t		SendQueue;
	uint8_t		RecvWorkerCount;
	uint8_t		RecvWorkerCountPerQ;
	uint8_t		SendWorkerCount;
	uint8_t		PacketGeneratorCount;
	uint32_t 	MemPoolSize;
	uint32_t 	MemPoolCacheSize;
	uint32_t 	RecvChunkSize;
	
	struct dpdk_interface * TxInterface;
	char 		TxPort[20];
	int 		TxPortHasConfigured;
	
	int 		started;
	struct 		rte_mempool ** mempool;
	struct 		rte_ring 	** recv_ring;
	struct 		rte_ring 	** send_ring;
	struct 		rte_ring 	*  sched_ring;
	
	uint64_t	total_received;
	uint64_t	total_dequeed;
	uint64_t	total_sent;
	uint64_t	total_dropped;
	uint64_t	total_enqueed;
	
	uint64_t	last_received;
	uint64_t	last_dequeed;
	uint64_t	last_sent;
	uint64_t	last_dropped;
	uint64_t	last_enqueed;
	
	uint64_t	arp_request_sent;
	uint64_t	arp_response_received;
	uint64_t	arp_request_received;
	uint64_t	arp_response_sent;

	uint64_t	icmp_request_sent;
	uint64_t	icmp_response_received;
	uint64_t	icmp_request_received;
	uint64_t	icmp_response_sent;
	
	uint64_t	eth_last_sent;
	uint64_t	eth_last_receved;
	uint64_t	eth_last_ierrors;
	uint64_t	eth_last_oerrors;
	uint64_t	eth_last_rx_nombuf;

	uint16_t	MatchSrcMac;
	uint8_t		SrcMac[8];
	uint32_t	IPv4;
	char		SIPv4[20];
	
	uint16_t	IPv6Set;
	uint16_t	IPv6SendRS;
	uint8_t		IPv6[18];
	char		SIPv6[30];
	uint16_t	NullMCLRCount;
	uint16_t	NullNebSol;
	uint8_t	 *  NSNonce;
	uint16_t	RSCounter;
	
	uint8_t		ProcessType;	//1: GTP-Gi, 2: Gi-GTP, 3:Gi-Gi
	uint32_t	DestIP;
	uint8_t		DstMac[8];
	uint32_t	TargetIP;
	
	uint32_t	QosEnabled;
	uint32_t	QosInitalized;
	uint32_t	QosConfigured;
	uint64_t    QosRate[13];
	uint32_t	QosRateCount;
	uint64_t    QosQSize;
	uint32_t	QosPipePerSubport;
	
	uint32_t	DHCPInform;
	uint32_t	DHCPInformAckReceived;
	uint32_t	DHCPInformTransactionId;
	
	uint32_t	DHCPDiscover;
	uint32_t	DHCPDiscoverAckReceived;
	uint32_t	DHCPDiscoverTransactionId;
	
	unsigned int 	lcore_id;
	struct rte_sched_port * sched_port;
	
	uint32_t	EnableNAPT;
	char 		ifName[20];
	

	dpdk_nat_ip_t * natIPHead;
	dpdk_nat_ip_t * natIPCurrent;
	app_rbtree_t * natRoot;
	int natIPCount;

} dpdk_interface_t;


typedef struct dpdk_interface_port
{
	dpdk_interface_t * interface;
	uint16_t queue_id;
	uint16_t port_type;
	int recv_worker_index;
} dpdk_interface_port_t;


typedef struct dpdk_dhcp_client
{
	char 	 MAC[6];
	uint32_t IPv4;
	uint8_t  IPv6[16];
} dpdk_dhcp_client_t;





typedef struct dpdk_engine 
{
	int Type;							// 1 - SendAndRecv, 2 - RecvAndSend, 3 - Forward
	
	uint32_t last_lcore;
	dpdk_interface_t * Head;
	dpdk_interface_t * Current;
	
	uint16_t packet_type_arp;
	uint16_t packet_type_ipv4;
	uint16_t packet_type_ipv6;
	
	uint32_t match_ueip_pcap_exit;
	
	int runing;

	int SkipAPICalls;
	int EnableLogs;
	int iLogHandle;
	int iLoggingThreshold;
	pthread_mutex_t LogLock;
	char LogPath[200];
	
	int EnablePerfLogs;
	int EnablePerfLogsTimeSpan;
	pthread_mutex_t PerfLogLock;
	char PerfLogPath[200];
	int iPerfLogHandle;	
	
	arp_entry_t static_arp_table[50];
	int static_arp_table_row_count;
	
	pthread_mutex_t arp_table_lock;
	arp_entry_t arp_table[100];
	int arp_table_row_count;
	
	int recv_worker_index;


	pcap_dumper_t * pdumper;
	int enable_pcap_on__gtp_gi;
	int enable_pcap_on__gi_gtp;
	int enable_pcap_on__gi_gi;


	char cli_ip[100];
	int cli_port;
	int cli_enabled;

	dpdk_dhcp_client_t dhcp_clients[10];
	int dhcp_clients_count;
	
	int access_mode;
	char dpdkToolsPath[250];

	app_data_region_t * natPool;
	
	
	
	
	uint32_t MaxSessions;
	dpe_ue_nat_ip_t * userNATHead;
	dpe_ue_nat_ip_t * userNATCurrent;
	pthread_mutex_t userNATLock;
	
	app_rbtree_t * userNAT;
	app_rbtree_t * userIPPortMAP;
	
} dpdk_engine_t;

dpdk_engine_t * __dpe = NULL;

int dpe__get_recv_worker()
{
	return __dpe->recv_worker_index;
}

void dpe__set_access_mode( int am)
{
	__dpe->access_mode = am;
}

void dpe__makeTimeStamp2( char * datestring)
{
	time_t rawtime;
	struct tm * timeinfo;
	struct timeval tval;
	struct timezone tzone;
	time( &rawtime);
	timeinfo = localtime( &rawtime);
	gettimeofday( &tval, &tzone);
	sprintf( datestring, "%02d_%02d_%d_%02d_%02d_%02d_%06lu", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tval.tv_usec);
	datestring[27] = 0;
}

int dpe__start_pcap_dumper()
{
	if(!__dpe->pdumper)
	{
		char fileName[200];
		memset( fileName, 0, sizeof(fileName));
		
		char datetime[30];
		memset( datetime, 0, sizeof(datetime));
		dpe__makeTimeStamp2( datetime);
		
		sprintf( fileName, "upf_packets_dump_%s.pcap", datetime);
		
		__dpe->pdumper = dpdk_pkt__open_pcap_dump( fileName);
		
		return 1;
	}
	return 0;
}


int dpe__stop_pcap_dumper()
{
	if(__dpe->pdumper)
	{
		dpdk_pkt__close_pcap( __dpe->pdumper);
		__dpe->pdumper = NULL;
		return 1;
	}
	return 0;
}



dpdk_interface_t * dpe__getinterface( int index);
uint32_t dpe__engine_get_linkspeed( dpdk_interface_t * interaface);


#define LOG_LEVEL_LOWER_LIMIT   1
#define LOG_LEVEL_CRITICAL      1
#define LOG_LEVEL_ERROR         2
#define LOG_LEVEL_WARNING       3
#define LOG_LEVEL_INFO          4
#define LOG_LEVEL_NORMAL        4
#define LOG_LEVEL_DEBUG         5
#define LOG_LEVEL_UPPER_LIMIT   5


#define LOGGER_ERR_LEVEL          1
#define LOGGER_ERR_MODE           2
#define LOGGER_ERR_FILE           3
#define LOGGER_ERR_NEW_CONSOLE    4
#define LOGGER_ERR_SEMAPHORE      5
#define LOGGER_ERR_BUFFER         6

#define LOGGER_WAIT_TIME        5000        
#define LOGGER_BUFFER_SIZE    	3000  
#define DATE_STRING_LEN         27
#define PATH_MAX_SIZE           260
#define MODULE_NAME_MAX_SIZE    50
#define LOG_MESSAGE_MAX_SIZE    LOGGER_BUFFER_SIZE - 100


void dpe__sleep()
{
	// rte_delay_ms();
	// rte_delay_us();
	usleep( 333);			// 333 - optimal, 1000- more delay and risky
	//usleep( 1000);
}

void dpe__makeIntFileName(char *mFileNameBuffer, char * mPath, char * mFilePrefix)
{
	time_t the_time;
	struct tm *tm_ptr;
	time( &the_time);
	tm_ptr = localtime( &the_time);
	sprintf( mFileNameBuffer, "%s/%s_%02d_%02d_%02d_%02d_%02d_%02d.log", mPath, mFilePrefix, tm_ptr->tm_mday, (tm_ptr->tm_mon + 1), (tm_ptr->tm_year + 1900), tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
}


void dpe__makeTimeStamp( char * datestring)
{
	time_t rawtime;
	struct tm * timeinfo;
	struct timeval tval;
	struct timezone tzone;
	time( &rawtime);
	timeinfo = localtime( &rawtime);
	gettimeofday( &tval, &tzone);
	sprintf( datestring, "%02d-%02d-%d:%02d:%02d:%02d:%06lu", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tval.tv_usec);
	datestring[27] = 0;
}


int dpe__log( unsigned long mLoggingLevel, const char *fileName, int lineNum, char* mLogMessage, ...)
{
	if( ( mLoggingLevel < LOG_LEVEL_LOWER_LIMIT) || ( mLoggingLevel > LOG_LEVEL_UPPER_LIMIT) || ( mLoggingLevel > __dpe->iLoggingThreshold))
    {
		//printf("dpe__log return 1\n");
		return 0;
    }
    
    if( !mLogMessage || (LOG_MESSAGE_MAX_SIZE < (int)strlen(mLogMessage)) )	
	{	
		//printf("dpe__log return 2\n");
		return 0;
	}
	
    pthread_mutex_lock ( &__dpe->LogLock);

    int debugTagLen;
    char* debugTag;
    unsigned long  ulThreadID = pthread_self();
    

    char m_buffer[LOGGER_BUFFER_SIZE];
    char f_buffer[LOGGER_BUFFER_SIZE + 200];
    char dateString[27];
    
    
    memset( &dateString, '\0', sizeof( dateString));
    dpe__makeTimeStamp( &dateString[0]);
    
	memset( m_buffer, '\0', sizeof(m_buffer));
	memset( f_buffer, '\0', sizeof(f_buffer));
		
	if( fileName != NULL)
	{
		if( strrchr(fileName,'/') )
		{
			fileName = strrchr(fileName,'/');
			fileName++;
		}
	}

    va_list args;// = 0;
    va_start( args, mLogMessage);
    vsnprintf( m_buffer, LOG_MESSAGE_MAX_SIZE, mLogMessage, args);
	va_end( args);
	 

	sprintf( f_buffer, "%s|%s|%s|%d\n", dateString, m_buffer, fileName, lineNum);	
	//printf("%s", f_buffer);
		

	unsigned long m_FileSize;
	
	int bRotate = 0;
	struct stat fileStat;
	fstat( __dpe->iLogHandle, &fileStat);
	m_FileSize = fileStat.st_size;
    
    if(m_FileSize >= 1048576)
		bRotate = 1;        
	
	if( bRotate == 1)
	{
		close( __dpe->iLogHandle);

		char mFileNameBuffer[500];
		memset( &mFileNameBuffer, 0, 500);
		dpe__makeIntFileName( &mFileNameBuffer[0], __dpe->LogPath, "dpe");
		
		__dpe->iLogHandle = creat( mFileNameBuffer, O_CREAT | S_IRWXU | S_IRWXG | S_IRWXO );
		chmod( mFileNameBuffer, 0600); 
	}
	
	int bytesWritten = write( __dpe->iLogHandle, f_buffer, strlen( f_buffer));
	
	if( bytesWritten < 0);
	
	pthread_mutex_unlock ( &__dpe->LogLock);

	return 1;	
	 
}

int dpe__perf_log( char * mLogMessage, ...)
{
    if( !mLogMessage || (LOG_MESSAGE_MAX_SIZE < (int)strlen(mLogMessage)) )	
		return 0;

    pthread_mutex_lock ( &__dpe->PerfLogLock);

    int debugTagLen;
    char* debugTag;
    unsigned long  ulThreadID = pthread_self();
    

    char m_buffer[LOGGER_BUFFER_SIZE];
	memset( m_buffer, '\0', sizeof(m_buffer));
		

    va_list args;// = 0;
    va_start( args, mLogMessage);
    vsnprintf( m_buffer, LOG_MESSAGE_MAX_SIZE, mLogMessage, args);
	va_end( args);
	 



	unsigned long m_FileSize;
	
	int bRotate = 0;
	struct stat fileStat;
	fstat( __dpe->iPerfLogHandle, &fileStat);
	m_FileSize = fileStat.st_size;
    
    if(m_FileSize >= 1048576)
		bRotate = 1;        
	
	if( bRotate == 1)
	{
		close( __dpe->iPerfLogHandle);

		char mFileNameBuffer[500];
		memset( &mFileNameBuffer, 0, 500);
		dpe__makeIntFileName( &mFileNameBuffer[0], __dpe->PerfLogPath, "dpe_perf");

		__dpe->iPerfLogHandle = creat( mFileNameBuffer, O_CREAT | S_IRWXU | S_IRWXG | S_IRWXO );
		chmod( mFileNameBuffer, 0600); 
	}
	
	int bytesWritten = write( __dpe->iPerfLogHandle, m_buffer, strlen( m_buffer));
	if( bytesWritten < 0);
	
	pthread_mutex_unlock ( &__dpe->PerfLogLock);

	return 1;	
}



void dpe__initlogger()
{
	int retd;
	int iUmask = S_IWGRP | S_IWOTH;
	int iFileCreationMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int iFlags = O_WRONLY | O_CREAT | O_APPEND;
	int iOldUmask = umask(iUmask);


	retd = open( __dpe->LogPath, O_RDONLY);
	if (retd == -1)
	{
		int iDirCreationMode = iFileCreationMode | S_IXUSR | S_IXGRP | S_IXOTH;
		int p = mkdir( __dpe->LogPath, iDirCreationMode);
  
		if(p == -1)
		{
			printf("Error in Creating Log Directory: %s\n", __dpe->LogPath);
			exit(0);
		}
	}
	
	retd = open( __dpe->PerfLogPath, O_RDONLY);
	if (retd == -1)
	{
		int iDirCreationMode = iFileCreationMode | S_IXUSR | S_IXGRP | S_IXOTH;
		int p = mkdir( __dpe->PerfLogPath, iDirCreationMode);
  
		if(p == -1)
		{
			printf("Error in Creating Log Directory: %s\n", __dpe->PerfLogPath);
			exit(0);
		}
	}
	


	char mFileNameBuffer[500];
	
	memset( &mFileNameBuffer, 0, sizeof(mFileNameBuffer));
	dpe__makeIntFileName( &mFileNameBuffer[0], __dpe->LogPath, "dpe");
	__dpe->iLogHandle = open( mFileNameBuffer, iFlags, iFileCreationMode);


	memset( &mFileNameBuffer, 0, sizeof(mFileNameBuffer));
	dpe__makeIntFileName( &mFileNameBuffer[0], __dpe->PerfLogPath, "dpe_perf");
	__dpe->iPerfLogHandle = open( mFileNameBuffer, iFlags, iFileCreationMode);
	
	umask(iOldUmask);
	
	printf("initLogger Completed\n");
}


void dpdk__initexit()
{
	__dpe->runing = 0;
	
	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "dpdk exiting");
	
	sleep(2);
	
	dpdk_interface_t * interaface = __dpe->Head;
	while( interaface)
	{
		if( interaface->started == 1)
		{
			rte_eth_dev_stop( interaface->PortId);
		}
		interaface = interaface->Next;
	}
	
	interaface = __dpe->Head;
	while( interaface)
	{
		int j = 0;
		for( j = 0; j < interaface->RecvQueue; j++)
		{
			rte_mempool_free( interaface->mempool[j]);
		}
		interaface = interaface->Next;
	}
	
	rte_eal_cleanup();
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

int dpdk_config__get_int2( json_t * json_config, char * key, int defval, int minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		int v = json_integer_value( jObj);
		return v;
	}
	return defval;
}

void * dpe_ascii_to_hex(char *in, int in_len, void *out, int out_len)
{
    int i = 0, j = 0, k = 0, hex;
    uint8_t *out_p = out;

    while(i < in_len && j < out_len) {
        if (!isspace(in[i])) {
            hex = isdigit(in[i]) ? in[i] - '0' : 
                islower(in[i]) ? in[i] - 'a' + 10 : in[i] - 'A' + 10;
            if ((k & 0x1) == 0) {
                out_p[j] = (hex << 4);
            } else {
                out_p[j] |= hex;
                j++;
            }
            k++;
        }
        i++;
    }

    return out;
}


dpdk_nat_ip_t * dpe__nat_ip__allocate_nat_ip( dpdk_interface_t * interface);
dpe_nat_entry_t * dpe__nat_ip__allocate_natentry( dpdk_nat_ip_t * nat_ip);

void dpe__user_nat___remove_entry_obj( dpe_ue_nat_ip_t * userNAT, dpe_nat_entry_t * nat_entry);
void dpe__user_nat___remove_entry( uint32_t user_ip, dpe_nat_entry_t * nat_entry);

void dpe__user_nat___add_entry( uint32_t user_ip, dpe_nat_entry_t * nat_entry);
void dpe__user_nat___add_entry_obj( dpe_ue_nat_ip_t * userNAT, dpe_nat_entry_t * nat_entry);


void dpe__delete_user_nat( uint32_t ueip)
{
	if( __dpe->userNAT)
	{
		dpe_ue_nat_ip_t * userNAT = (dpe_ue_nat_ip_t*)app_rbnode__find_rbitem( __dpe->userNAT, (uint8_t*)&ueip, 4);
		
		if( userNAT)
		{
			userNAT->deleteInProgress = 1;

			dpe_nat_entry_t * nat_entry = userNAT->entryHead;
			dpe_nat_entry_t * nat_entry_next = NULL;
			
			while( nat_entry)
			{
				nat_entry_next = nat_entry->Next;
				
				nat_entry->Prev = NULL;
				nat_entry->Next = NULL;
				
				if( nat_entry->nat_ip)
				{
					pthread_mutex_lock( &nat_entry->nat_ip->nat_entry_lock);
					
					if(!nat_entry->nat_ip->nat_entry_head)
					{
						nat_entry->nat_ip->nat_entry_head = nat_entry->nat_ip->nat_entry_current = nat_entry;
					}
					else
					{
						nat_entry->nat_ip->nat_entry_current->Next = nat_entry;
						nat_entry->nat_ip->nat_entry_current = nat_entry;
					}
					
					pthread_mutex_unlock( &nat_entry->nat_ip->nat_entry_lock);
				}
				
				nat_entry = nat_entry_next;
			}



			pthread_mutex_lock( &__dpe->userNATLock);
			
			userNAT->Next = NULL;
			
			if(!__dpe->userNATHead)
			{
				__dpe->userNATHead = __dpe->userNATCurrent = userNAT;
			}
			else
			{
				__dpe->userNATCurrent->Next = userNAT;
				__dpe->userNATCurrent = userNAT;
			}
			
			pthread_mutex_unlock( &__dpe->userNATLock);

			
			app_rbnode__setNull_rbitem( __dpe->userNAT, (uint8_t *)&ueip, 4);
		}
	}
}




dpe_nat_entry_t * dpe__find_or_create_natentry( dpdk_interface_t * interface, uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol)
{
	if( __dpe->userNAT)
	{
		uint8_t key[6];
		*(uint32_t*)&key[0] = ueip;
		*(uint16_t*)&key[4] = srcport;
		
		dpe_nat_entry_t * ritem = (dpe_nat_entry_t*)app_rbnode__find_rbitem( __dpe->userIPPortMAP, key, 6);
		
		if( ritem) 
		{
			return ritem;
		}
		
		
		dpe_ue_nat_ip_t * userNAT = (dpe_ue_nat_ip_t*)app_rbnode__find_rbitem( __dpe->userNAT, (uint8_t*)&ueip, 4);
		
		if(!userNAT)
		{
			pthread_mutex_lock( &__dpe->userNATLock);
			
			userNAT = __dpe->userNATHead;
			
			if(userNAT)
			{
				__dpe->userNATHead = userNAT->Next;
				userNAT->Next = NULL;
				userNAT->deleteInProgress = 0;
				
				app_rbnode__add_rbitem( __dpe->userNAT, (uint8_t*)&ueip, 4, (uint8_t*)userNAT);
			}
			
			pthread_mutex_unlock( &__dpe->userNATLock);
		}
		
		if(userNAT)
		{
			if( userNAT->deleteInProgress == 1)
			{
				return NULL;
			}
			
			dpdk_nat_ip_t * nat_ip = dpe__nat_ip__allocate_nat_ip( interface);
			
			if( nat_ip)
			{
				userNAT->nat_ip = nat_ip;
				dpe_nat_entry_t * item = dpe__nat_ip__allocate_natentry( nat_ip);
				
				if(item)
				{
					item->UeIP 			= ueip;
					item->UeDstIP 		= dstip;
					item->UeDstPort 	= dstport;
					item->UeSrcPort		= srcport;
					item->Protocol 		= protocol;					
					
					dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%p  AddNAT  -- allocated-NatSrcPort=%u  UeSrcPort=%u Proto=%u  ueip=%u  dstip=%u", item, item->NatSrcPort, item->UeSrcPort, item->Protocol, ueip, dstip);
					dpe__user_nat___add_entry_obj( userNAT, item);
					app_rbnode__add_rbitem( __dpe->userIPPortMAP, key, 6, (uint8_t*)item);
					
					return item;
				}
			}
		}
	}
	return NULL;
	
	// dpe_nat_entry_t * item = interface->nat_entry_head;
	
	// while( item)
	// {
		// if( item->UeIP == ueip && item->UeDstIP == dstip && item->UeDstPort == dstport && item->UeSrcPort == srcport && item->Protocol == protocol)
		// {
			// return item;
		// }
		// item = item->Next;
	// }

	
	// pthread_mutex_lock( &interface->nat_entry_lock);

	// interface->NATPort++;
	
	// if( interface->NATPort > 60000)
		// interface->NATPort = 10000;

	// item = (dpe_nat_entry_t *)malloc(sizeof(dpe_nat_entry_t));
	// memset( item, 0, sizeof(dpe_nat_entry_t));

	// item->UeIP 			= ueip;
	// item->UeDstIP 		= dstip;
	// item->UeDstPort 	= dstport;
	// item->UeSrcPort		= srcport;
	// item->NatSrcPort	= interface->NATPort;
	// item->Protocol 		= protocol;
	
	// if(!interface->nat_entry_head) 
	// {
		// interface->nat_entry_head = interface->nat_entry_current = item;
	// } 
	// else 
	// {
		// interface->nat_entry_current->Next = item;
		// interface->nat_entry_current = item;
	// }
	
	// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Added NAT Entry  UeIP=%u    UeDstIP=%u   UeDstPort=%u    UeSrcPort=%u    NatSrcPort=%u   Protocol=%u ", 
		// item->UeIP,  
		// item->UeDstIP, 
		// item->UeDstPort,
		// item->UeSrcPort,
		// item->NatSrcPort,
		// item->Protocol
	// );
	
	// pthread_mutex_unlock( &interface->nat_entry_lock);
	// return item;
}

dpe_nat_entry_t * dpe__find_natentry_by_natsrcport( dpdk_interface_t * interface, uint32_t pkt_dstip, uint16_t pkt_dstport, uint32_t pkt_srcip, uint16_t pkt_srcport, uint8_t protocol)
{
	dpdk_nat_ip_t * nat_ip = interface->natIPHead;
	dpe_nat_entry_t * item = NULL;
	
	while( nat_ip)
	{
		if( nat_ip->ip == pkt_dstip)
		{
			item = (dpe_nat_entry_t *)app_rbnode__find_rbitem( nat_ip->natTree, (uint8_t*)&pkt_dstport, 2);
			dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "FindNAT  -- %p  pkt_dstport=%u", item, pkt_dstport);
			
			if( item)
			{
				dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%p  FindNAT  - Found  pkt_dstip=%u == %u   pkt_dstport=%u == %u  pkt_srcip=%u == %u   pkt_srcport=%u == %u  UeSrcPort=%u  Protocol=%u == %u", 
					item, pkt_dstip, interface->IPv4, 
					pkt_dstport, item->NatSrcPort, 
					pkt_srcip, item->UeDstIP,
					pkt_srcport, item->UeDstPort,
					item->UeSrcPort, item->Protocol, protocol);
			}

			return item;
		}
		nat_ip = nat_ip->Next;
	}
	return NULL;

	// dpe_nat_entry_t * item = interface->nat_entry_head;
	
	// while( item)
	// {
		// // dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "FindNAT pkt_dstip=%u == %u   pkt_dstport=%u == %u  pkt_srcip=%u == %u   pkt_srcport=%u == %u  Protocol=%u == %u", 
			// // pkt_dstip, interface->IPv4, 
			// // pkt_dstport, item->NatSrcPort, 
			// // pkt_srcip, item->UeDstIP,
			// // pkt_srcport, item->UeDstPort,
			// // item->Protocol, protocol);

		// if( pkt_dstip == interface->IPv4 && pkt_dstport == item->NatSrcPort && pkt_srcip == item->UeDstIP && pkt_srcport == item->UeDstPort && item->Protocol == protocol)
		// {
			// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "FindNAT  - Found pkt_dstip=%u == %u   pkt_dstport=%u == %u  pkt_srcip=%u == %u   pkt_srcport=%u == %u  Protocol=%u == %u", 
				// pkt_dstip, interface->IPv4, 
				// pkt_dstport, item->NatSrcPort, 
				// pkt_srcip, item->UeDstIP,
				// pkt_srcport, item->UeDstPort,
				// item->Protocol, protocol);

			// return item;
		// }
		
		// item = item->Next;
	// }

		// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "FindNAT  -- Not Found  pkt_dstip=%u    pkt_dstport=%u   pkt_srcip=%u    pkt_srcport=%u   Protocol=%u ", 
			// pkt_dstip, 
			// pkt_dstport, 
			// pkt_srcip,
			// pkt_srcport,
			// protocol);
	
	return NULL;
}


typedef struct app_upf_cmd
{
	uint32_t size;
	uint32_t cmd;				// 1 pcap, 			2 log
	uint32_t operation;			// 1 start/enable   2 stop/disable
	uint32_t interface;			// 1024 or any number > 0
} app_upf_cmd_t;


void dpe__udp_client_message( uint8_t * Data, int tIndex)
{
	printf("received message from cli\n");
	app_ep_udp_message_t * udpPtr = (app_ep_udp_message_t*)Data;
	
	uint16_t len = app_ep__get_len( udpPtr);
	uint8_t * buff = app_ep__get_buffer( udpPtr);
	
	
	app_upf_cmd_t * cmd = (app_upf_cmd_t *)buff;
	
	if( cmd->size == len)
	{
		printf("valid command\n");
		
		/**/
		if( cmd->cmd == 1)
		{
			if( cmd->operation == 1)
			{
				if( cmd->interface == 1024)
				{
					__dpe->enable_pcap_on__gtp_gi	= 1;
					__dpe->enable_pcap_on__gi_gtp	= 1;
					__dpe->enable_pcap_on__gi_gi	= 1;					
				}
				else if( cmd->interface == PROCESS_TYPE__GTP_Gi)
				{
					__dpe->enable_pcap_on__gtp_gi	= 1;
				}
				else if( cmd->interface == PROCESS_TYPE__Gi_GTP)
				{
					__dpe->enable_pcap_on__gi_gtp	= 1;
				}
				else if( cmd->interface == PROCESS_TYPE__Gi_Gi_INGRESS)
				{
					__dpe->enable_pcap_on__gi_gi	= 1;
				}
				
				dpe__start_pcap_dumper();
			}
			else if( cmd->operation == 2)
			{
				__dpe->enable_pcap_on__gtp_gi	= 0;
				__dpe->enable_pcap_on__gi_gtp	= 0;
				__dpe->enable_pcap_on__gi_gi	= 0;
				
				dpe__stop_pcap_dumper();
			}
		}
		
		/**/
	}
	else
	{
		printf("invalid command\n");
	}
	
	app_ep__free_udp_message( udpPtr);
}



void dpe__user_nat___print( uint32_t user_ip);



void dpe__init( char * configfile) 
{
	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	
	json_t * config = json_load_file( configfile, 0, &error);
	
	if(!config)
	{
		printf( "config error: text:%s\nsource:%s\nline:%d\ncolumn:%d\nposition:%d\n", error.text, error.source, error.line, error.column, error.position);
		exit(0);
	}
	else
	{
		__dpe = (dpdk_engine_t *)malloc(sizeof(dpdk_engine_t));
		memset( __dpe, 0, sizeof(dpdk_engine_t));
		
		pthread_mutex_init( &pcap_lock, NULL);
		
		__dpe->pdumper = NULL;
		__dpe->enable_pcap_on__gtp_gi	= 0;
		__dpe->enable_pcap_on__gi_gtp	= 0;
		__dpe->enable_pcap_on__gi_gi	= 0;
		__dpe->access_mode 				= 0;

	
		__dpe->arp_table_row_count = 0;
		__dpe->recv_worker_index = 0;
		pthread_mutex_init( &__dpe->arp_table_lock, NULL);

		
		
		__dpe->cli_port = 0;
		__dpe->cli_enabled = 0;		
		
		
		json_t * jcli = json_object_get( config, "CLI");
		
		if( jcli)
		{
			__dpe->cli_port				= dpdk_config__get_int2( jcli, "Port", 0, 1);
			__dpe->cli_enabled 			= dpdk_config__get_int2( jcli, "Enabled", 0, 1);
			
			json_t * cli_ip = json_object_get( jcli, "IP");

			if( cli_ip)
			{
				char * logTempPath  = (char *)json_string_value( cli_ip);
				int  logTempLen = json_string_length( cli_ip);
				
				memcpy( __dpe->cli_ip, logTempPath, logTempLen);
			}
		}
		
		if(__dpe->cli_enabled == 1)
		{
			app_ep__init_udp( 1024, 1024);
			app_ep_udp_server_t * udpserver = app_ep__create_udpv4_server( __dpe->cli_ip, __dpe->cli_port, 1024, 5, dpe__udp_client_message);
			app_ep__udp_server_start( udpserver);
		}
			

		__dpe->Type = dpdk_config__get_int( config, "Type", 0, 0);
		
		
		pthread_mutex_init( &__dpe->LogLock, NULL);
		pthread_mutex_init( &__dpe->PerfLogLock, NULL);
		__dpe->EnableLogs 				= dpdk_config__get_int2( config, "EnableLogs", 0, 1);
		__dpe->EnablePerfLogs 			= dpdk_config__get_int2( config, "EnablePerfLogs", 0, 1);
		__dpe->EnablePerfLogsTimeSpan 	= dpdk_config__get_int2( config, "EnablePerfLogsTimeSpan", 1, 1);
		__dpe->SkipAPICalls				= dpdk_config__get_int2( config, "SkipAPICalls", 0, 1);
		
		printf("EnableLogs=%u\n", 				__dpe->EnableLogs);
		printf("EnablePerfLogs=%u\n", 			__dpe->EnablePerfLogs);
		printf("EnablePerfLogsTimeSpan=%u\n", 	__dpe->EnablePerfLogsTimeSpan);
		printf("SkipAPICalls=%u\n", 			__dpe->SkipAPICalls);


		json_t * jLogPath = json_object_get( config, "LogPath");

		if( jLogPath)
		{
			char * logTempPath  = (char *)json_string_value( jLogPath);
			int  logTempLen = json_string_length( jLogPath);
			
			memcpy( __dpe->LogPath, logTempPath, logTempLen);
		}
		
		if( __dpe->LogPath[0] == 0x00)
		{
			memcpy( __dpe->LogPath, "logs/", 5);
		}
		
		jLogPath = json_object_get( config, "PerfLogPath");

		if( jLogPath)
		{
			char * logTempPath  = (char *)json_string_value( jLogPath);
			int  logTempLen = json_string_length( jLogPath);
			
			memcpy( __dpe->PerfLogPath, logTempPath, logTempLen);
		}
		
		if( __dpe->PerfLogPath[0] == 0x00)
		{
			memcpy( __dpe->PerfLogPath, "logs/", 5);
		}
		
		
		json_t * jToolsPath = json_object_get( config, "UserToolsPath");
		
		if( jToolsPath)
		{
			char * logTempPath  = (char *)json_string_value( jToolsPath);
			int  logTempLen = json_string_length( jToolsPath);
			
			memcpy( __dpe->dpdkToolsPath, logTempPath, logTempLen);			
			printf( "UserToolsPath=%s\n", __dpe->dpdkToolsPath);
		}
		
		
		size_t app_rbtree_sof = app_rbnode__getsizeof_app_rbtree();

		//__dpe->natIPHead	= NULL;
		//__dpe->natIPCurrent	= NULL;
		__dpe->natPool 		= app_region__create();
		app_region__add_pool( __dpe->natPool, "natIP", sizeof(dpdk_nat_ip_t), 2048);
		app_region__add_pool( __dpe->natPool, "natTree", app_rbtree_sof, 2048);
		
		
		//__dpe->natRoot	= app_rbnode__create_rbtree( 1024, 4);


		dpdk_nat_ip_t * natIPItem = NULL;
		int port_count = ((256*256)-10001);
		
		
		__dpe->iLoggingThreshold = LOG_LEVEL_DEBUG;
		dpe__initlogger();

		
		if( __dpe->Type == 0 || __dpe->Type > 4)
		{
			printf("Invalid Type=%u  %d\n", __dpe->Type, __LINE__);
			exit(0);
		} 
		
		json_t * interfaces = json_object_get( config, "Interfaces");
		
		if(!interfaces)
		{
			printf("Please configure Interfaces\n");
			dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Please configure Interfaces");
			
			exit(0);
		} 
		else 
		{
			if(!json_is_array(interfaces))
			{
				printf("Invalid Interfaces config (should be an array)\n");
				dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Invalid Interfaces config (should be an array)");
				exit(0);				
			} 
			else
			{
				int count = json_array_size(interfaces);
				
				if( count == 0)
				{
					printf("Invalid Interfaces config (found empty array)\n");
					dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Invalid Interfaces config (found empty array)");
					exit(0);
				}
				else
				{
					int i = 0;
					for( i = 0; i < count; i++)
					{
						json_t * interface = json_array_get( interfaces, i);
						
						if( interface)
						{
							json_t * jport = json_object_get( interface, "BusId");
							
							if(!jport)
							{
								printf("BusId Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "BusId Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							json_t * jIfName = json_object_get( interface, "IfName");
							
							
							json_t * jTxport = json_object_get( interface, "TxBusId");
							

							
							
							json_t * jtype = json_object_get( interface, "Type");
							
							if(!jtype)
							{
								printf("Type Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Type Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							json_t * jRQ = json_object_get( interface, "RQ");
							
							if(!jRQ)
							{
								printf("RQ Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RQ Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							json_t * jSQ = json_object_get( interface, "SQ");
							
							if(!jSQ)
							{
								printf("SQ Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "SQ Tag is missing at interface Index=%d", i);
								exit(0);
							}


							json_t * jRWC = json_object_get( interface, "RWC");
							
							if(!jRWC)
							{
								printf("RWC Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RWC Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							// json_t * jSWC = json_object_get( interface, "SWC");
							
							// if(!jSWC)
							// {
								// printf("SWC Tag is missing at interface Index=%d\n", i);
								// exit(0);
							// }							


							json_t * jMemPoolSize = json_object_get( interface, "MemPoolSize");
							
							if(!jMemPoolSize)
							{
								printf("MemPoolSize Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MemPoolSize Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							json_t * jRecvChunkSize = json_object_get( interface, "RecvChunkSize");
							
							if(!jRecvChunkSize)
							{
								printf("RecvChunkSize Tag is missing at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RecvChunkSize Tag is missing at interface Index=%d", i);
								exit(0);
							}
							
							
							// json_t * jRingSize = json_object_get( interface, "RingSize");
							
							// if(!jRingSize)
							// {
								// printf("RingSize Tag is missing at interface Index=%d\n", i);
								// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RingSize Tag is missing at interface Index=%d", i);
								// exit(0);
							// }
							
							
							json_t * jMatchSrcMac = json_object_get( interface, "MatchSrcMac");
							
							
							json_t * jIPv4 			= json_object_get( interface, "IPv4");
							json_t * jIPv6 			= json_object_get( interface, "IPv6");
							json_t * jDestIP 		= json_object_get( interface, "DestIP");
							json_t * jDestMAC 		= json_object_get( interface, "DestMAC");
							
							
							
							dpdk_interface_t * interfaceObject = (dpdk_interface_t*)malloc( sizeof(dpdk_interface_t) );
							memset( interfaceObject, 0, sizeof(dpdk_interface_t));
							interfaceObject->TxPortHasConfigured = 0;
							interfaceObject->eth_last_sent = 0;
							interfaceObject->eth_last_receved = 0;
							interfaceObject->eth_last_ierrors = 0;
							interfaceObject->eth_last_oerrors = 0;
							interfaceObject->eth_last_rx_nombuf = 0;
							interfaceObject->arp_request_sent = 0;
							interfaceObject->arp_response_received = 0;
							interfaceObject->arp_request_received = 0;
							interfaceObject->arp_response_sent = 0;
							interfaceObject->icmp_request_sent = 0;
							interfaceObject->icmp_response_received = 0;
							interfaceObject->icmp_request_received = 0;
							interfaceObject->icmp_response_sent = 0;
							interfaceObject->QosEnabled			= 0;
							
							// interfaceObject->nat_entry_head 		= NULL;
							// interfaceObject->nat_entry_current 		= NULL;
							// pthread_mutex_init( &interfaceObject->nat_entry_lock, NULL);
							// interfaceObject->nat_entry_count 		= 0;
							// interfaceObject->NATPort				= 10000;
							
							

	
	
							const char * cport = json_string_value( jport);
							int  cportlen = json_string_length( jport);
							
							if( cportlen == 0)
							{
								printf("Port Tag length is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Port Tag length is 0 at interface Index=%d", i);
								exit(0);
							}

							memcpy( interfaceObject->Port, cport, cportlen);
							
							if( jTxport)
							{
								cport = json_string_value( jTxport);
								cportlen = json_string_length( jTxport);
							
								memcpy( interfaceObject->TxPort, cport, cportlen);
								
								interfaceObject->TxPortHasConfigured = 1;
							}
							
							if( jIfName)
							{
								char * logTempPath  = (char *)json_string_value( jIfName);
								int  logTempLen = json_string_length( jIfName);
								memcpy( interfaceObject->ifName, logTempPath, logTempLen);
								
								printf( "ifName=%s\n", interfaceObject->ifName);
							}
							
							
							
							interfaceObject->DHCPInform 	= dpdk_config__get_int( interface, "DHCPInform", 0, 0);
							interfaceObject->DHCPDiscover 	= dpdk_config__get_int( interface, "DHCPDiscover", 0, 0);
							interfaceObject->EnableNAPT 	= dpdk_config__get_int( interface, "EnableNAPT", 0, 0);
							
							interfaceObject->RecvQueue = dpdk_config__get_int( interface, "RQ", 0, 0);
							
							
							if( interfaceObject->RecvQueue == 0)
							{
								printf("RQ value is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RQ value is 0 at interface Index=%d", i);
								exit(0);
							}
							
							interfaceObject->SendQueue = dpdk_config__get_int( interface, "SQ", 0, 0);
							
							if( interfaceObject->SendQueue == 0)
							{
								printf("SQ value is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "SQ value is 0 at interface Index=%d", i);
								exit(0);
							}
							

							interfaceObject->RecvWorkerCount = dpdk_config__get_int( interface, "RWC", 0, 0);
							
							if( interfaceObject->RecvWorkerCount == 0)
							{
								printf("RWC value is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RWC value is 0 at interface Index=%d", i);
								exit(0);
							}
							
							
							interfaceObject->RecvWorkerCountPerQ = dpdk_config__get_int( interface, "RWCRWCPerQ", 1, 1);
							printf("RWCPerQ value is %d at interface Index=%d\n", interfaceObject->RecvWorkerCountPerQ, i);


							
							interfaceObject->SendWorkerCount = 0;
							// interfaceObject->SendWorkerCount = dpdk_config__get_int( interface, "SWC", 0, 0);
							
							// if( interfaceObject->SendWorkerCount == 0)
							// {
								// printf("SWC value is 0 at interface Index=%d\n", i);
								// exit(0);
							// }							
							interfaceObject->PacketGeneratorCount = dpdk_config__get_int( interface, "PGC", 0, 0);
							

							interfaceObject->MemPoolSize = dpdk_config__get_int( interface, "MemPoolSize", 0, 0);
							
							if( interfaceObject->MemPoolSize == 0)
							{
								printf("MemPoolSize value is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MemPoolSize value is 0 at interface Index=%d", i);
								exit(0);
							}
							
							if( interfaceObject->MemPoolSize < 4096)
							{
								printf("MemPoolSize value shoule be >= 4096 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MemPoolSize value is 0 at interface Index=%d", i);
								exit(0);
							}
							
							interfaceObject->MemPoolCacheSize = dpdk_config__get_int( interface, "MemPoolCacheSize", 0, 0);
							
							if( interfaceObject->MemPoolCacheSize == 0)
							{
								interfaceObject->MemPoolCacheSize = 512;
							}
							
							
							interfaceObject->RecvChunkSize = dpdk_config__get_int( interface, "RecvChunkSize", 0, 0);
							
							if( interfaceObject->RecvChunkSize == 0)
							{
								printf("RecvChunkSize value is 0 at interface Index=%d\n", i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RecvChunkSize value is 0 at interface Index=%d", i);
								exit(0);
							}
							
							if( interfaceObject->RecvWorkerCount < interfaceObject->RecvQueue)
							{
								printf("RecvWorkerCount[%d] should be >= RecvQueue[%d] at interface Index=%d\n", 
									interfaceObject->RecvWorkerCount, interfaceObject->RecvQueue, i);
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RecvWorkerCount[%d] should be >= RecvQueue[%d] at interface Index=%d", 
									interfaceObject->RecvWorkerCount, interfaceObject->RecvQueue, i);	
								exit(0);
							}
							
							// if( interfaceObject->SendWorkerCount < interfaceObject->SendQueue)
							// {
								// printf("SendWorkerCount[%d] should be >= SendQueue[%d] at interface Index=%d\n", 
									// interfaceObject->SendWorkerCount, interfaceObject->SendQueue, i);
								// exit(0);
							// }


							const char * macaddr = NULL;
							if( jMatchSrcMac)
							{
								macaddr = json_string_value( jMatchSrcMac);
								int  macaddrlen = json_string_length( jMatchSrcMac);
							
								if( macaddrlen == 12)
								{
									interfaceObject->MatchSrcMac = 1;
									dpe_ascii_to_hex( (char*)macaddr, macaddrlen, (void *)&interfaceObject->SrcMac, 6);
								}
								else if( macaddrlen > 0 && macaddrlen != 12)
								{
									printf("Invalid MatchSrcMac length=%d at interface Index=%d\n", macaddrlen, i);
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Invalid MatchSrcMac length=%d at interface Index=%d", macaddrlen, i);
									exit(0);
								}
							}
							
							const char * ipv4 = NULL;
							if( jIPv4)
							{
								struct sockaddr_in saddr;
								ipv4 = json_string_value( jIPv4);
								inet_aton( (char*)ipv4, &saddr.sin_addr);
								
								memcpy( interfaceObject->SIPv4, ipv4, strlen(ipv4));
								interfaceObject->IPv4 = saddr.sin_addr.s_addr;
							}
							
							
							interfaceObject->RSCounter 		= 5;
							interfaceObject->NullMCLRCount 	= 0;
							interfaceObject->NullNebSol 	= 0;
							interfaceObject->NSNonce 		= NULL;
							interfaceObject->IPv6SendRS 	= dpdk_config__get_int( interface, "SendRS", 0, 0);
							interfaceObject->IPv6Set 		= 0;
							const char * ipv6 = NULL;
							
							if( jIPv6)
							{
								ipv6 = json_string_value( jIPv6);
								
								if ( inet_pton( AF_INET6, ipv6, &interfaceObject->IPv6) == 1) // success!
								{
									interfaceObject->IPv6Set = 1;
									dpe__print_buffer( "IPv6 = ", (char *)&interfaceObject->IPv6, sizeof(struct in6_addr), 0, 0);
								}
							}
							
							
							if(jDestIP)
							{
								struct sockaddr_in saddr;
								ipv4 = json_string_value( jDestIP);
								inet_aton( (char*)ipv4, &saddr.sin_addr);
							
								interfaceObject->DestIP = saddr.sin_addr.s_addr;
								printf("DestIP=%s IntIP=%d\n", ipv4, interfaceObject->DestIP);
							}
							
							json_t * jTargetIP 	= json_object_get( interface, "TargetIP");
							interfaceObject->TargetIP = 0;
							
							if(jTargetIP)
							{
								struct sockaddr_in saddr;
								char * tIPv4 = (char *)json_string_value( jTargetIP);
								inet_aton( (char*)tIPv4, &saddr.sin_addr);
							
								interfaceObject->TargetIP = saddr.sin_addr.s_addr;
								printf("TargetIP=%s IntIP=%d\n", ipv4, interfaceObject->TargetIP);
							}
							
							
							if(jDestMAC)
							{
								macaddr = json_string_value( jDestMAC);
								int macaddrlen = json_string_length( jDestMAC);
								
								if( macaddrlen == 12)
								{
									dpe_ascii_to_hex( (char*)macaddr, macaddrlen, (void *)&interfaceObject->DstMac, 6);
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Bus=%s will send packets to DestMAC=%s", interfaceObject->Port, macaddr);
								}
								else
								{
									printf("error ******** Invalid DestMAC length=%d at interface Index=%d\n", macaddrlen, i);
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Invalid DestMAC length=%d at interface Index=%d", macaddrlen, i);
									exit(0);
								}
							}
							
							interfaceObject->ProcessType = PROCESS_TYPE__NONE;
							
							if( jtype)
							{
								const char * iType = json_string_value( jtype);
								
								// //1: GTP-Gi, 2: Gi-GTP, 3:Gi-Gi
								if( strcmp( iType, "GTP-Gi") == 0)
								{
									interfaceObject->ProcessType = PROCESS_TYPE__GTP_Gi;
								}
								else if( strcmp( iType, "Gi-GTP") == 0)
								{
									interfaceObject->ProcessType = PROCESS_TYPE__Gi_GTP;
								}
								else if( strcmp( iType, "Gi-Gi-INGRESS") == 0)
								{
									interfaceObject->ProcessType = PROCESS_TYPE__Gi_Gi_INGRESS;
								}
								else if( strcmp( iType, "Gi-Gi-EGRESS") == 0)
								{
									interfaceObject->ProcessType = PROCESS_TYPE__Gi_Gi_EGRESS;
								}
							}
							
							interfaceObject->QosEnabled = dpdk_config__get_int( interface, "EnableQoS", 0, 0);
							printf( "QosEnabled=%u  %u\n", interfaceObject->QosEnabled, __LINE__);
							
							// interfaceObject->QosInitalized = 0;
							// interfaceObject->QosConfigured = 0;
							
							// json_t * jBandwidth 		= json_object_get( interface, "Bandwidth");
							
							// if( jBandwidth)
							// {
								// json_t * jSpeeds 			= json_object_get( jBandwidth, "Speeds");
								// json_t * jQSize 			= json_object_get( jBandwidth, "QSize");
								// //json_t * jPipePerSubport 	= json_object_get( jBandwidth, "PipePerSubport");
								
								
								// if( jSpeeds && jQSize)
								// {
									// interfaceObject->QosQSize = dpdk_config__get_int( jBandwidth, "QSize", 0, 0);
								
									// if( interfaceObject->QosQSize == 0)
									// {
										// printf("Bandwidth->QSize should be greater than 0 - configured %lu\n", interfaceObject->QosQSize);
										// exit(0);
									// }
									
									// interfaceObject->QosPipePerSubport = dpdk_config__get_int( jBandwidth, "PipePerSubport", 32, 32);
									// printf( "QosPipePerSubport=%u\n", interfaceObject->QosPipePerSubport);

									
									// if( json_is_array( jSpeeds))
									// {
										// int bCount = json_array_size( jSpeeds);
										// int bi = 0;
										// json_t * jBandwidthItem = NULL;
										// uint64_t u64Bandwidth = 0;
										
										// if( bCount == 0)
										// {
											// printf("Bandwidth->Speeds Not Configured\n");
											// exit(0);
										// }
										
										// for( bi = 0; bi < bCount; bi++)
										// {
											// jBandwidthItem = json_array_get( jSpeeds, bi);
											
											// if( jBandwidthItem)
											// {
												// u64Bandwidth = json_integer_value(jBandwidthItem);
												
												// if( u64Bandwidth == 0)
												// {
													// printf("Bandwidth->Speeds Item should be greater than 0 at %d, configured value is %lu\n", bi, u64Bandwidth);
													// exit(0);
												// }
												
												// interfaceObject->QosRate[bi] = u64Bandwidth;
												// interfaceObject->QosRateCount++;
											// }
										// }
										
										// if( bCount > 0 && bCount < 13)
										// {
											// for( bi = bCount; bi < 13; bi++)
											// {
												// interfaceObject->QosRate[bi] = interfaceObject->QosRate[bCount];
											// }
										// }
										
										// interfaceObject->QosInitalized = 1;
										// interfaceObject->QosConfigured = 0;
									// }
									// else
									// {
										// printf("Bandwidth->Speeds should be Array\n");
										// exit(0);
									// }
								// }
							// }
							
							interfaceObject->natIPHead 			= NULL;
							interfaceObject->natIPCurrent 		= NULL;
							interfaceObject->natRoot 			= app_rbnode__create_rbtree( 1024, 4);;
							interfaceObject->natIPCount 		= 0;

							
							
							
							
							if( interfaceObject->EnableNAPT == 1)
							{	
								natIPItem = (dpdk_nat_ip_t*)app_region__allocate_fr( __dpe->natPool, sizeof(dpdk_nat_ip_t));
								
								if( natIPItem)
								{
									natIPItem->Next 		= NULL;
									natIPItem->ip 			= interfaceObject->IPv4;
									natIPItem->interface	= interfaceObject;
									//memcpy( natIPItem->MAC, MAC, 6);
									
									natIPItem->natTree 				= app_rbnode__create_rbtree( port_count, 4);
									natIPItem->Index 				= (dpe_nat_entry_t **)malloc( port_count * sizeof(dpe_nat_entry_t*)); 
									natIPItem->nat_entry_head 		= NULL;
									natIPItem->nat_entry_current 	= NULL;
									pthread_mutex_init( &natIPItem->nat_entry_lock, NULL);

									if(!interfaceObject->natIPHead)
									{
										interfaceObject->natIPHead = interfaceObject->natIPCurrent = natIPItem;
									}
									else
									{
										interfaceObject->natIPCurrent->Next = natIPItem;
										interfaceObject->natIPCurrent = natIPItem;
									}
									
									//natIPTree = (app_rbtree_t*)app_region__allocate_fr( __dpe->natPool, app_rbtree_sof);
									app_rbnode__add_rbitem( interfaceObject->natRoot, (uint8_t*)&natIPItem->ip, 4, (uint8_t *)natIPItem);
									
									interfaceObject->natIPCount++;
									printf( "NAT -> adding ip=%s|%u for interface=%s ip-count=%u\n", ipv4, natIPItem->ip, interfaceObject->Port, interfaceObject->natIPCount);
								}
											
								
								
								json_t * jsonNAT = json_object_get( interface, "NAT");
								
								if( jsonNAT)
								{
									int count = json_array_size( jsonNAT);
									
									int i = 0;
									for( i = 0; i < count; i++)
									{
										json_t * ipNAT = json_array_get( jsonNAT, i);

										if( ipNAT)
										{
											json_t * jIPv4 		= json_object_get( ipNAT, "IPv4");
											json_t * jMAC 		= json_object_get( ipNAT, "MAC");

											
											const char * ipv4 = NULL;
											struct sockaddr_in saddr;
											ipv4 = json_string_value( jIPv4);
											inet_aton( (char*)ipv4, &saddr.sin_addr);
											
											uint32_t IPv4 = saddr.sin_addr.s_addr;
										
											const char * macaddr = NULL;
											int macaddrlen = 0;
											
											if( jMAC )
											{
												macaddr = json_string_value( jMAC);
												macaddrlen = json_string_length( jMAC);
											}
											
											uint8_t MAC[7] = {0,0,0,0,0,0,0};
											
											if( macaddrlen == 12 && IPv4 > 0)
											{
												dpe_ascii_to_hex( (char*)macaddr, macaddrlen, (void *)&MAC, 6);
											}
											else if( macaddrlen > 0 && macaddrlen != 12)
											{
												printf("Invalid MAC address length for NAT-MAC for IP=%s\n", ipv4);
												exit(0);
											}
											
											
											natIPItem = (dpdk_nat_ip_t*)app_region__allocate_fr( __dpe->natPool, sizeof(dpdk_nat_ip_t));
											
											
											if( natIPItem)
											{
												natIPItem->Next 		= NULL;
												natIPItem->ip 			= IPv4;
												natIPItem->interface	= interfaceObject;
												memcpy( natIPItem->MAC, MAC, 6);
												
												natIPItem->natTree 				= app_rbnode__create_rbtree( port_count, 4);
												natIPItem->Index 				= (dpe_nat_entry_t **)malloc( port_count * sizeof(dpe_nat_entry_t*)); 
												natIPItem->nat_entry_head 		= NULL;
												natIPItem->nat_entry_current 	= NULL;
												pthread_mutex_init( &natIPItem->nat_entry_lock, NULL);
												
												
												if(!interfaceObject->natIPHead)
												{
													interfaceObject->natIPHead = interfaceObject->natIPCurrent = natIPItem;
												}
												else
												{
													interfaceObject->natIPCurrent->Next = natIPItem;
													interfaceObject->natIPCurrent = natIPItem;
												}
												
												//natIPTree = (app_rbtree_t*)app_region__allocate_fr( __dpe->natPool, app_rbtree_sof);
												app_rbnode__add_rbitem( interfaceObject->natRoot, (uint8_t*)&natIPItem->ip, 4, (uint8_t *)natIPItem);
												
												interfaceObject->natIPCount++;
												printf( "NAT -> adding additional ip=%s|%u for interface=%s ip-count=%u\n", ipv4, natIPItem->ip, interfaceObject->Port, interfaceObject->natIPCount);
											}
											else
											{
												printf( "app_region malloc failed for NAT-ITEM %s\n", ipv4);
											}
						
						
											
										}
									}
								}
							}


							
							
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "---------%d Interafce----------", i);
							interfaceObject->Index = i;
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "BusId: %s", interfaceObject->Port);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RQ: %d", interfaceObject->RecvQueue);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "SQ: %d", interfaceObject->SendQueue);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RWC: %d", interfaceObject->RecvWorkerCount);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "SWC: %d", interfaceObject->SendWorkerCount);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "PGC: %d", interfaceObject->PacketGeneratorCount);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MemPoolSize: %d", interfaceObject->MemPoolSize);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "RecvChunkSize: %d", interfaceObject->RecvChunkSize);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MatchSrcMac:   %d  %s  %02X:%02X:%02X:%02X:%02X:%02X", interfaceObject->MatchSrcMac, macaddr,
										  interfaceObject->SrcMac[0] & 0xFF, interfaceObject->SrcMac[1] & 0xFF, interfaceObject->SrcMac[2] & 0xFF
										, interfaceObject->SrcMac[3] & 0xFF, interfaceObject->SrcMac[4] & 0xFF, interfaceObject->SrcMac[5] & 0xFF);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "IPv4: %s  IPv4=%u", ipv4, interfaceObject->IPv4);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Port: %s  TxPort: %s  Configured: %d  ProcessType: %d", interfaceObject->Port, interfaceObject->TxPort, interfaceObject->TxPortHasConfigured, interfaceObject->ProcessType);
							
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "DHCPDiscover: %u", interfaceObject->DHCPDiscover);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "DHCPInform: %u", interfaceObject->DHCPInform);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "EnableNAPT: %u", interfaceObject->EnableNAPT);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "ifName: %s", interfaceObject->ifName);
							
							
							printf( "%s natIPCount=%u EnableNAPT=%u\n", interfaceObject->Port, interfaceObject->natIPCount,  interfaceObject->EnableNAPT);
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%s natIPCount=%u EnableNAPT=%u", interfaceObject->Port, interfaceObject->natIPCount,  interfaceObject->EnableNAPT);
							
							
							// if( interfaceObject->QosInitalized == 1)
							// {
								// printf( "Speeds %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu   QSize=%lu  QosRateCount=%u\n", 
									// interfaceObject->QosRate[0], interfaceObject->QosRate[1], interfaceObject->QosRate[2],
									// interfaceObject->QosRate[3], interfaceObject->QosRate[4], interfaceObject->QosRate[5],
									// interfaceObject->QosRate[6], interfaceObject->QosRate[7], interfaceObject->QosRate[8],
									// interfaceObject->QosRate[9], interfaceObject->QosRate[10], interfaceObject->QosRate[11], interfaceObject->QosRate[12],
									// interfaceObject->QosQSize, interfaceObject->QosRateCount);
							// }




							interfaceObject->started = 0;
							interfaceObject->total_received = 0;
							interfaceObject->total_sent = 0;
							interfaceObject->last_received = 0;
							interfaceObject->last_sent = 0;
							
	
							if(!__dpe->Head)
							{
								__dpe->Head = __dpe->Current = interfaceObject;
							}
							else
							{
								__dpe->Current->Next = interfaceObject;
								__dpe->Current = interfaceObject;
							}
						}
						else
						{
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__,"Error at interface Index=%d", i);
							exit(0);
						}
					}
					
					
					
					dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "-----------dpdk config init completed-----------");
				}
				
			}
		}
		
		__dpe->static_arp_table_row_count = 0;
		json_t * ipMacMap = json_object_get( config, "IPMAC");
		
		if(ipMacMap)
		{
			int count = json_array_size( ipMacMap);
		
			int i = 0;
			for( i = 0; i < count; i++)
			{
				json_t * ipMAC = json_array_get( ipMacMap, i);
				
				if(ipMAC)
				{
					json_t * jIPv4 		= json_object_get( ipMAC, "IPv4");
					json_t * jMAC 	= json_object_get( ipMAC, "MAC");
					
					// && jMAC
					
					if( jIPv4 )
					{
						const char * ipv4 = NULL;
						struct sockaddr_in saddr;
						ipv4 = json_string_value( jIPv4);
						inet_aton( (char*)ipv4, &saddr.sin_addr);
						
						uint32_t IPv4 = saddr.sin_addr.s_addr;
					
						const char * macaddr = NULL;
						int macaddrlen = 0;
						
						if( jMAC )
						{
							macaddr = json_string_value( jMAC);
							macaddrlen = json_string_length( jMAC);
						}
						
						uint8_t MAC[7] = {0,0,0,0,0,0,0};
						
						if( macaddrlen == 12 && IPv4 > 0)
						{
							dpe_ascii_to_hex( (char*)macaddr, macaddrlen, (void *)&MAC, 6);
						} 
						else if( macaddrlen > 0 && macaddrlen != 12 && IPv4 > 0)
						{
							printf("Invalid MAC Address for IPMAC-MAC %s\n", ipv4);
							exit(0);
						}
						
						
						printf("Static IPMAP - IPv4=%s|%u  %02X:%02X:%02X:%02X:%02X:%02X\n", 
							ipv4, IPv4, MAC[0] & 0xFF, MAC[1] & 0xFF, MAC[2] & 0xFF, MAC[3] & 0xFF, MAC[4] & 0xFF, MAC[5] & 0xFF);
					
						dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Static IPMAP - IPv4=%s|%u  %02X:%02X:%02X:%02X:%02X:%02X", 
							ipv4, IPv4, MAC[0] & 0xFF, MAC[1] & 0xFF, MAC[2] & 0xFF, MAC[3] & 0xFF, MAC[4] & 0xFF, MAC[5] & 0xFF);
					
						__dpe->static_arp_table[__dpe->static_arp_table_row_count].ip = htonl(IPv4);
						memcpy( __dpe->static_arp_table[__dpe->static_arp_table_row_count].Mac, MAC, 6);
						
						printf("ip=%u  %s|%s|%d\n", __dpe->static_arp_table[__dpe->static_arp_table_row_count].ip, __FILE__, __FUNCTION__, __LINE__);
						
						__dpe->static_arp_table_row_count++;
					}
				}
			}
					
		}
		
		
		
		
		json_t * jsonDhcpClient = json_object_get( config, "DHCPServer");
		
		if(jsonDhcpClient)
		{
			int count = json_array_size( jsonDhcpClient);
		
			int i = 0;
			for( i = 0; i < count; i++)
			{
				json_t * ipDHCP = json_array_get( jsonDhcpClient, i);
				
				if( ipDHCP)
				{
					json_t * jIPv4 		= json_object_get( ipDHCP, "IPv4");
					json_t * jMAC 		= json_object_get( ipDHCP, "MAC");
				
				
					if( jIPv4 )
					{
						const char * ipv4 = NULL;
						struct sockaddr_in saddr;
						ipv4 = json_string_value( jIPv4);
						inet_aton( (char*)ipv4, &saddr.sin_addr);
						
						uint32_t IPv4 = saddr.sin_addr.s_addr;
					
						const char * macaddr = NULL;
						int macaddrlen = 0;
						
						if( jMAC )
						{
							macaddr = json_string_value( jMAC);
							macaddrlen = json_string_length( jMAC);
						}
						
						uint8_t MAC[7] = {0,0,0,0,0,0,0};
						
						if( macaddrlen == 12 && IPv4 > 0)
						{
							dpe_ascii_to_hex( (char*)macaddr, macaddrlen, (void *)&MAC, 6);
						}
						else if( macaddrlen > 0 && macaddrlen != 12 && IPv4 > 0)
						{
							printf("Invalid MAC Address length for DHCPServer-MAC %s\n", ipv4);
							exit(0);
						}
						//printf( "macaddrlen=%d, IPv4=%d\n", macaddrlen, IPv4);
						
						__dpe->dhcp_clients[__dpe->dhcp_clients_count].IPv4 = htonl( IPv4);
						memcpy( __dpe->dhcp_clients[__dpe->dhcp_clients_count].MAC, MAC, 6);
						
						__dpe->dhcp_clients_count++;
					}
				
				}
			}
		}
		
		
		
		
		
				
		
		
		
		
		dpdk_interface_t * interface = __dpe->Head;
		dpe_nat_entry_t * nat_entry = NULL;
		
		int i = 0;
		int iNATEntryCount = 0;

		while( interface)
		{
			if( interface->EnableNAPT == 1)
			{
				natIPItem = interface->natIPHead;
				
				while( natIPItem)
				{
					natIPItem->dPool = app_region__add_pool( __dpe->natPool, "nipc", sizeof(dpe_nat_entry_t), port_count);
					
					printf( "  --> natIPItem=%p dPool=%p  %s|%d\n", natIPItem, natIPItem->dPool, __FILE__, __LINE__);
					
					for( i = 0; i < port_count; i++)
					{
						nat_entry = (dpe_nat_entry_t*)app_region__allocate_fd( natIPItem->dPool);
						//nat_entry = (dpe_nat_entry_t*)app_region__allocate_fr( __dpe->natPool, sizeof(dpe_nat_entry_t));
						
						if( nat_entry)
						{
							natIPItem->Index[i] 	= nat_entry;
							
							
							nat_entry->NatSrcPort 	= (10001 + i);
							nat_entry->nat_ip		= natIPItem;

							
							app_rbnode__add_rbitem( natIPItem->natTree, (uint8_t*)&nat_entry->NatSrcPort, 2, (uint8_t*)nat_entry);
							iNATEntryCount++;
							
							if(!natIPItem->nat_entry_head)
							{
								natIPItem->nat_entry_head = natIPItem->nat_entry_current = nat_entry;
							}
							else
							{
								natIPItem->nat_entry_current->Next = nat_entry;
								natIPItem->nat_entry_current = nat_entry;
							}
							
							natIPItem->nat_entry_count++;
						}
					}
					natIPItem = natIPItem->Next;
				}
			}
			interface = interface->Next;
		}


		json_t * jsonMaxSessions = json_object_get( config, "MaxSessions");
		uint32_t iUserSessionCount = 0;
		
		if( jsonMaxSessions && iNATEntryCount > 0)
		{
			__dpe->MaxSessions = dpdk_config__get_int( config, "MaxSessions", 0, 0);
			
			if( __dpe->MaxSessions > 0)
			{
				__dpe->userNATHead = NULL;
				__dpe->userNATCurrent = NULL;
				pthread_mutex_init( &__dpe->userNATLock, NULL);
				__dpe->userNAT 			= app_rbnode__create_rbtree( __dpe->MaxSessions, 4);
				__dpe->userIPPortMAP	= app_rbnode__create_rbtree( __dpe->MaxSessions * 100, 6);

				
				app_data_pool_t * dPool = app_region__add_pool( __dpe->natPool, "user-ip", sizeof(dpe_ue_nat_ip_t), __dpe->MaxSessions);
				
				printf( "created user-ip-pool with count=%u  dPool=%p  userNAT=%p\n", __dpe->MaxSessions, dPool, __dpe->userNAT);
				
				
				
				dpe_ue_nat_ip_t * user_ip_nat = NULL;
				
				
				int i = 0;
				for( i = 0; i < __dpe->MaxSessions; i++)
				{
					user_ip_nat = (dpe_ue_nat_ip_t*)app_region__allocate_fd( dPool);
				
					if( user_ip_nat)
					{
						user_ip_nat->ip = 0;
						
						user_ip_nat->entryHead = NULL;
						user_ip_nat->entryCurrent = NULL;
						pthread_mutex_init( &user_ip_nat->entryLock, NULL);
						user_ip_nat->entryCount = 0;
						
						
						if(!__dpe->userNATHead)
						{
							__dpe->userNATHead = __dpe->userNATCurrent = user_ip_nat;
						}
						else
						{
							__dpe->userNATCurrent->Next = user_ip_nat;
							__dpe->userNATCurrent = user_ip_nat;
						}
						
						iUserSessionCount++;
					}
				}
				
			}
		}
		
		
		
		printf("------ dpdk config init completed  Total NAT-Entry-Count=%u   User-Session-Count=%u    MaxSessions=%u\n", iNATEntryCount, iUserSessionCount, __dpe->MaxSessions);

		if( iNATEntryCount > 0)
		{
			printf("\n\n\n\n");
			printf("userNAT Test\n");
	
			interface = __dpe->Head;
			dpe_nat_entry_t * nat_entry = NULL;
			

			while( interface)
			{
				if( interface->EnableNAPT == 1)
				{
					break;
				}
				
				interface = interface->Next;
			}
			
	
			//  Testing Block
			if( interface && 1 == 2)
			{
				
				dpdk_nat_ip_t * nat_ip = interface->natIPHead;
				
				if( nat_ip)
				{
					printf( "interface ip=%u (%u.%u.%u.%u)   \n", nat_ip->ip, nat_ip->ip & 0xFF, nat_ip->ip >> 8 & 0xFF, nat_ip->ip >> 16 & 0xFF, nat_ip->ip >> 24 & 0xFF);
					
					uint32_t user_ip = 0x0100007F;
					printf( "user_ip=%u (%u.%u.%u.%u)   \n", user_ip, user_ip & 0xFF, user_ip >> 8 & 0xFF, user_ip >> 16 & 0xFF, user_ip >> 24 & 0xFF);
					printf("\n\n");
					
					
					dpe__user_nat___add_entry( user_ip, NULL);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					
					dpe_nat_entry_t * nat_entry_1 = dpe__nat_ip__allocate_natentry( nat_ip);
					printf( "nat_entry_1=%p\n", nat_entry_1);

					dpe_nat_entry_t * nat_entry_2 = dpe__nat_ip__allocate_natentry( nat_ip);
					printf( "nat_entry_2=%p\n", nat_entry_2);

					dpe_nat_entry_t * nat_entry_3 = dpe__nat_ip__allocate_natentry( nat_ip);
					printf( "nat_entry_3=%p\n", nat_entry_3);
					
					dpe_nat_entry_t * nat_entry_4 = dpe__nat_ip__allocate_natentry( nat_ip);
					printf( "nat_entry_4=%p\n", nat_entry_4);
					
					dpe_nat_entry_t * nat_entry_5 = dpe__nat_ip__allocate_natentry( nat_ip);
					printf( "nat_entry_5=%p\n", nat_entry_5);
					
					printf("\n\n");

					
					dpe__user_nat___add_entry( user_ip, nat_entry_1);
					dpe__user_nat___add_entry( user_ip, nat_entry_2);
					dpe__user_nat___add_entry( user_ip, nat_entry_3);
					dpe__user_nat___add_entry( user_ip, nat_entry_4);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					
					sleep(1);
					dpe__delete_user_nat( user_ip);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					
					
					/*
					//TC-1 remove 0 and add new
					
					dpe__user_nat___remove_entry( user_ip, nat_entry_1);
					dpe__user_nat___print( user_ip);
					printf("\n\n");

					dpe__user_nat___add_entry( user_ip, nat_entry_5);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					*/

					/*
					//TC-2 remove last and add new
					
					dpe__user_nat___remove_entry( user_ip, nat_entry_4);
					dpe__user_nat___print( user_ip);
					printf("\n\n");

					dpe__user_nat___add_entry( user_ip, nat_entry_5);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					*/
					
					/*
					dpe__user_nat___remove_entry( user_ip, nat_entry_4);
					dpe__user_nat___remove_entry( user_ip, nat_entry_3);
					dpe__user_nat___remove_entry( user_ip, nat_entry_2);
					dpe__user_nat___remove_entry( user_ip, nat_entry_1);
					dpe__user_nat___print( user_ip);
					printf("\n\n");					


					dpe__user_nat___add_entry( user_ip, nat_entry_5);
					dpe__user_nat___print( user_ip);
					printf("\n\n");
					*/
					
				}
			}
			else
			{
				printf("NAT Enabled Interface Not Found\n");
			}
			
			printf("userNAT End\n");
		}
	}
}

dpdk_nat_ip_t * dpe__nat_ip__allocate_nat_ip( dpdk_interface_t * interface)
{
	dpdk_nat_ip_t * nat_ip = interface->natIPHead;
	
	while( nat_ip)
	{
		if( nat_ip->nat_entry_head)
		{
			return nat_ip;
		}
		nat_ip = nat_ip->Next;
	}
	
	return NULL;
}

dpe_nat_entry_t * dpe__nat_ip__allocate_natentry( dpdk_nat_ip_t * nat_ip)
{
	pthread_mutex_lock( &nat_ip->nat_entry_lock);
	
	dpe_nat_entry_t * nat_entry = NULL;
	
	if( nat_ip->nat_entry_head)
	{
		nat_entry = nat_ip->nat_entry_head;
		
		if( nat_entry)
		{
			nat_ip->nat_entry_head = nat_entry->Next;
			nat_entry->Next = NULL;
			
			pthread_mutex_unlock( &nat_ip->nat_entry_lock);
			return nat_entry;
		}
	}
	
	pthread_mutex_unlock( &nat_ip->nat_entry_lock);
	return NULL;
}


void dpe__user_nat___print( uint32_t user_ip)
{
	if( __dpe->userNAT)
	{
		dpe_ue_nat_ip_t * userNAT = (dpe_ue_nat_ip_t*)app_rbnode__find_rbitem( __dpe->userNAT, (uint8_t*)&user_ip, 4);
	
		if(userNAT)
		{
			printf( "entry Head=%p  Current=%p  Count=%u\n", userNAT->entryHead, userNAT->entryCurrent, userNAT->entryCount);
		
			dpe_nat_entry_t * nat_entry = userNAT->entryHead;
			
			while( nat_entry)
			{
				printf( "NatSrcPort=%u  %p  Next=%p  Prev=%p\n", nat_entry->NatSrcPort, nat_entry, nat_entry->Next, nat_entry->Prev);
				nat_entry = nat_entry->Next;
			}
		}
		else
		{
			printf("user_nat session not found with  user_ip=%u\n", user_ip);
		}
	}
}

void dpe__user_nat___add_entry_obj( dpe_ue_nat_ip_t * userNAT, dpe_nat_entry_t * nat_entry)
{
	if(userNAT && nat_entry)
	{
		nat_entry->Prev = NULL;
		nat_entry->Next = NULL;
		
		pthread_mutex_lock( &userNAT->entryLock);
		
		if(!userNAT->entryHead)
		{
			userNAT->entryHead = userNAT->entryCurrent = nat_entry;
		}
		else
		{
			nat_entry->Prev = userNAT->entryCurrent;
			
			userNAT->entryCurrent->Next = nat_entry;
			userNAT->entryCurrent = nat_entry;
		}
		userNAT->entryCount++;
	
		pthread_mutex_unlock( &userNAT->entryLock);
	}
}


void dpe__user_nat___add_entry( uint32_t user_ip, dpe_nat_entry_t * nat_entry)
{
	if( __dpe->userNAT)
	{
		dpe_ue_nat_ip_t * userNAT = (dpe_ue_nat_ip_t*)app_rbnode__find_rbitem( __dpe->userNAT, (uint8_t*)&user_ip, 4);
	
		if(!userNAT)
		{
			pthread_mutex_lock( &__dpe->userNATLock);
			
			
			userNAT = __dpe->userNATHead;
			
			if( userNAT)
			{
				__dpe->userNATHead = userNAT->Next;
				userNAT->Next = NULL;
				
				app_rbnode__add_rbitem( __dpe->userNAT, (uint8_t*)&user_ip, 4, (uint8_t*)userNAT);
			}
			
			
			pthread_mutex_unlock( &__dpe->userNATLock);
		}		
		
		
		dpe__user_nat___add_entry_obj( userNAT, nat_entry);
	}
}


void dpe__user_nat___remove_entry_obj( dpe_ue_nat_ip_t * userNAT, dpe_nat_entry_t * nat_entry)
{
	pthread_mutex_lock( &userNAT->entryLock);
	
	if (!userNAT->entryHead || !nat_entry) 
	{
        pthread_mutex_unlock( &userNAT->entryLock);
		return;
    }

    // If nat_entry is the head node
    if (nat_entry == userNAT->entryHead) 
	{
        userNAT->entryHead = nat_entry->Next;
    
		if( userNAT->entryCurrent == nat_entry)
		{
			userNAT->entryCurrent = nat_entry->Next;
		}
	}
	else if( userNAT->entryCurrent == nat_entry)
	{
		userNAT->entryCurrent = nat_entry->Prev;
	}

    // Update next node's prev pointer if nat_entry->next is not NULL
    if (nat_entry->Next) 
	{
        nat_entry->Next->Prev = nat_entry->Prev;
    }

    // Update prev node's next pointer if nat_entry->prev is not NULL
    if (nat_entry->Prev) 
	{
        nat_entry->Prev->Next = nat_entry->Next;
    }
	
	
	userNAT->entryCount--;
	pthread_mutex_unlock( &userNAT->entryLock);
	
	nat_entry->Prev = NULL;
	nat_entry->Next = NULL;
}


void dpe__user_nat___remove_entry( uint32_t user_ip, dpe_nat_entry_t * nat_entry)
{
	if( __dpe->userNAT)
	{
		dpe_ue_nat_ip_t * userNAT = (dpe_ue_nat_ip_t*)app_rbnode__find_rbitem( __dpe->userNAT, (uint8_t*)&user_ip, 4);
	
		if(userNAT)
		{
			dpe__user_nat___remove_entry_obj( userNAT, nat_entry);
		}
	}	
}







int (*ptrPacketGeneratorFunction)( uint8_t * iobj, uint8_t *) = NULL;

int dpe__get_required_core_count()
{
	int rccount = 0;	
	
	dpdk_interface_t * interface = __dpe->Head;
	
	while( interface)
	{
		rccount += interface->RecvQueue + interface->SendQueue + (interface->RecvWorkerCount * interface->RecvWorkerCountPerQ) + interface->SendWorkerCount;

		if( interface->QosEnabled == 1)
		{
			rccount += 1;
		}

		if( ptrPacketGeneratorFunction > 0) 
		{
			rccount += interface->PacketGeneratorCount;
		}
		
		interface = interface->Next;
	}
	
	return rccount;
}




void dpe__print_mac( unsigned int portid, struct rte_ether_addr * addr)
{
    dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "MAC address: port=%u %02X:%02X:%02X:%02X:%02X:%02X", 
		portid, addr->addr_bytes[0] & 0xFF, addr->addr_bytes[1] & 0xFF, addr->addr_bytes[2] & 0xFF, 
		addr->addr_bytes[3] & 0xFF, addr->addr_bytes[4] & 0xFF, addr->addr_bytes[5] & 0xFF);
}


uint32_t dpe__get_u32ip( char * ipv4)
{
	struct sockaddr_in saddr;
	inet_aton( (char*)ipv4, &saddr.sin_addr);
	return saddr.sin_addr.s_addr;
}								

int dpe_pkt__encode_gtp_tpdu( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint32_t ran_ip, uint32_t upf_ip, uint32_t ueip, uint32_t dstip, uint16_t src_port, uint16_t dst_port, uint32_t teid, int protocol, int payloadlen)
{
	rte_prefetch0( rte_pktmbuf_mtod( mbuf, void *));
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);

	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);
	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV4);

	struct rte_ipv4_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr  * udp  		= NULL;
	struct rte_gtp_hdr  * gtp  		= NULL;
	
	ip_hdr = (struct rte_ipv4_hdr *)(((unsigned char *)eth) + 14);
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	
	ip_hdr->total_length 	= htons(20 + 8 + 8 + 20 + payloadlen);		//ipv4=20 + udp=8 + gtp=8 + ipv4=20 + payloadlen
	mbuf->pkt_len  = (14 + 20 + 8 + 8 + 20 + payloadlen);
	mbuf->data_len = (14 + 20 + 8 + 8 + 20 + payloadlen);

	ip_hdr->packet_id 		= 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	
	ip_hdr->src_addr 		= ran_ip;
	ip_hdr->dst_addr 		= upf_ip;
	
	ip_hdr->hdr_checksum 	= 0;
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);


	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	
	udp->src_port				= rte_cpu_to_be_16( 2152);
	udp->dst_port				= rte_cpu_to_be_16( 2152);
	udp->dgram_len				= rte_cpu_to_be_16( 8 + 8 + 20 + payloadlen);
	udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 0xFF;
	gtp->plen 					= rte_cpu_to_be_16( 8 + 20 + payloadlen);
	gtp->teid 					= teid; 


	struct rte_ipv4_hdr * usr_ip_hdr = NULL;
	usr_ip_hdr					= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr));
	usr_ip_hdr->version_ihl 	= 0x45;
	usr_ip_hdr->type_of_service = 0x00;
	usr_ip_hdr->total_length 	= rte_cpu_to_be_16( 20 + payloadlen);
	usr_ip_hdr->packet_id 		= 20 + payloadlen;
	usr_ip_hdr->fragment_offset = 0;
	usr_ip_hdr->time_to_live 	= 64;
	usr_ip_hdr->next_proto_id 	= protocol;			//17 UDP,  6 TCP
	usr_ip_hdr->hdr_checksum 	= 0;
	
	usr_ip_hdr->src_addr 		= rte_cpu_to_be_32( ueip);
	usr_ip_hdr->dst_addr 		= rte_cpu_to_be_32( dstip);
	
	usr_ip_hdr->hdr_checksum 	= rte_ipv4_cksum( usr_ip_hdr);	
	

	if( protocol == 1 )
	{
		char * pkt_icmp = ((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		pkt_icmp[0] = 0x00;
		pkt_icmp[1] = 0x00;
		pkt_icmp[2] = 0x23;
		pkt_icmp[3] = 0x1F;
		pkt_icmp[4] = 0x73;
		pkt_icmp[5] = 0x48;
		pkt_icmp[6] = 0x69;
		pkt_icmp[7] = 0x98;
	}
	else if( protocol == 6 )
	{
		struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		
		tcp->src_port				= rte_cpu_to_be_16( src_port);
		tcp->dst_port				= rte_cpu_to_be_16( dst_port);
		tcp->sent_seq				= usr_ip_hdr->packet_id;
		tcp->data_off				= 0x50;
		tcp->tcp_flags				= 0x18;
		tcp->rx_win					= 0x4410;
		tcp->cksum					= rte_ipv4_udptcp_cksum( usr_ip_hdr, tcp);
	}
	else if( protocol == 17)
	{
		struct rte_udp_hdr 		* usr_udp  		= NULL;

		usr_udp = (struct rte_udp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		usr_udp->src_port			= rte_cpu_to_be_16( src_port);
		usr_udp->dst_port			= rte_cpu_to_be_16( dst_port);
		usr_udp->dgram_cksum		= 0;
		usr_udp->dgram_len 			= rte_cpu_to_be_16( payloadlen - 8 );
		usr_udp->dgram_cksum		= rte_ipv4_udptcp_cksum( usr_ip_hdr, usr_udp);
	}
	
	return 1;
}


int dpe_pkt__encode_gtp_end_marker( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint32_t teid)
{
	rte_prefetch0( rte_pktmbuf_mtod( mbuf, void *));
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);

	// mbuf->pkt_len  = 72;
    // mbuf->data_len = 72;
	
	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);
	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV4);

	struct rte_ipv4_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr  * udp  		= NULL;
	struct rte_gtp_hdr  * gtp  		= NULL;
	
	ip_hdr = (struct rte_ipv4_hdr *)(((unsigned char *)eth) + 14);
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	
	ip_hdr->total_length 	= htons(20 + 8 + 8);		//ipv4=20 + udp=8 + gtp=8
	mbuf->pkt_len  = (14 + 20 + 8 + 8);
	mbuf->data_len = (14 + 20 + 8 + 8);
	
	ip_hdr->packet_id 		= 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	
	ip_hdr->src_addr 		= dst_ip;
	ip_hdr->dst_addr 		= src_ip;
	
	ip_hdr->hdr_checksum 	= 0;
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	udp->src_port				= rte_cpu_to_be_16(dst_port);
	udp->dst_port				= rte_cpu_to_be_16(src_port);	
	udp->dgram_len				= rte_cpu_to_be_16(8 + 8);
	udp->dgram_cksum			= 0;
	udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 254;
	gtp->plen 					= rte_cpu_to_be_16(8);
	gtp->teid 					= teid; 
	
	return 0;
}


int dpe_pkt__encode_gtp_echo_response_ipv4( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t msgid, uint32_t teid, uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi)
{
	rte_prefetch0( rte_pktmbuf_mtod( mbuf, void *));
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);

	// mbuf->pkt_len  = 72;
    // mbuf->data_len = 72;
	
	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);
	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV4);

	struct rte_ipv4_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr  * udp  		= NULL;
	struct rte_gtp_hdr  * gtp  		= NULL;
	
	ip_hdr = (struct rte_ipv4_hdr *)(((unsigned char *)eth) + 14);
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	
	if( ext == 1)
	{
		ip_hdr->total_length 	= htons(20 + 8 + 8 + 8 + 20 + 8);		//ipv4=20 + udp=8 + gtp=8
		mbuf->pkt_len  = (14 + 20 + 8 + 8 + 8 + 20 + 8);
		mbuf->data_len = (14 + 20 + 8 + 8 + 8 + 20 + 8);
	}
	else
	{
		ip_hdr->total_length 	= htons(20 + 8 + 8);		//ipv4=20 + udp=8 + gtp=8
		mbuf->pkt_len  = (14 + 20 + 8 + 8);
		mbuf->data_len = (14 + 20 + 8 + 8);
	}
	
	ip_hdr->packet_id 		= 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	
	ip_hdr->src_addr 		= dst_ip;
	ip_hdr->dst_addr 		= src_ip;
	
	ip_hdr->hdr_checksum 	= 0;
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	udp->src_port				= rte_cpu_to_be_16(dst_port);
	udp->dst_port				= rte_cpu_to_be_16(src_port);	
	
	if( ext == 1)
	{
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8 + 8 + 20 + 8);		
	}
	else
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8);
	
	udp->dgram_cksum			= 0;
	udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= msgid;
	
	if( ext == 1) {
		gtp->plen 					= rte_cpu_to_be_16(8 + 20);
	} else {
		gtp->plen 					= rte_cpu_to_be_16(8);
	}
	
	gtp->teid 					= teid; 
	//printf("gtp->teid=%u\n", gtp->teid);
	
	if( ext == 1)
	{
		gtp->msg_type 			= 0xFF;
		gtp->gtp_hdr_info 		= 0x34;
		gtp->plen 				= rte_cpu_to_be_16(16 + 20 + 8);
				
	
		unsigned char * cptr = (unsigned char *)((unsigned char *)gtp + 8);
		
		cptr[0] = 0;	//sequence_number
		cptr[1] = 0;	//sequence_number
		cptr[2] = 0;	//n_pdu_number

		// type
		// #define OGS_GTP_EXTENSION_HEADER_TYPE_UDP_PORT 0x40
		// #define OGS_GTP_EXTENSION_HEADER_TYPE_PDU_SESSION_CONTAINER 0x85
		// #define OGS_GTP_EXTENSION_HEADER_TYPE_NO_MORE_EXTENSION_HEADERS 0x0
		cptr[3] = 0x85;		//GTP_EXTENSION_HEADER_TYPE
		cptr[4] = 0x01;		;//len

		// #define OGS_GTP_EXTENSION_HEADER_PDU_TYPE_DL_PDU_SESSION_INFORMATION 0
		// #define OGS_GTP_EXTENSION_HEADER_PDU_TYPE_UL_PDU_SESSION_INFORMATION 1
		// ED2(uint8_t pdu_type:4;,uint8_t spare1:4;);
		
		if( pdu_type == 1)
		{
			cptr[5] = 0x10;		// OGS_GTP_EXTENSION_HEADER_PDU_TYPE_UL_PDU_SESSION_INFORMATION
		}
		else
		{
			cptr[5] = 0x00;		// OGS_GTP_EXTENSION_HEADER_PDU_TYPE_DL_PDU_SESSION_INFORMATION
		}
		
		// ED3(uint8_t paging_policy_presence:1;,uint8_t reflective_qos_indicator:1;,uint8_t qos_flow_identifier:6;);
		cptr[6] = qfi;
		cptr[7] = 0;

		
		struct rte_ipv4_hdr * user_ip = (struct rte_ipv4_hdr *)((unsigned char *)cptr + 8);
		
		user_ip->version_ihl 	= 0x45;
		user_ip->type_of_service = 0x00;
		user_ip->total_length 	= htons(20 + 8);
		
		user_ip->packet_id 		= 0;
		user_ip->fragment_offset = 0;
		user_ip->time_to_live 	= 64;
		user_ip->next_proto_id 	= 1;
		
		user_ip->src_addr 		= dst_ip + 20;
		user_ip->dst_addr 		= src_ip + 20;
		
		user_ip->hdr_checksum 	= 0;
		user_ip->hdr_checksum 	= rte_ipv4_cksum( user_ip);
		
		// unsigned char * icmp_packet = (unsigned char *)((unsigned char *)user_ip + 20);
		// icmp_packet[0] = 0x08;
		
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtpecho_07122023.pcap");
		// dpdk_pkt__write_pcap( dumper, mbuf);
		// dpdk_pkt__close_pcap( dumper);
		// exit(0);	
		//unsigned char * cptr = (unsigned char *)((unsigned char *)gtp + (8));
	}
	
	return 0;
}




int dpdk_pkt__encode_arp_request( struct rte_mbuf * mbuf_req, char * srcmac, uint32_t srcip, uint32_t tip)
{
	struct rte_ether_hdr * 	request_eth = rte_pktmbuf_mtod( mbuf_req, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	request_arp = (struct rte_arp_hdr *)&request_eth[1];


	memset( &request_eth->dst_addr, 0xFF, 6);
    memcpy( &request_eth->src_addr, srcmac, 6);
	request_eth->ether_type = htons(RTE_ETHER_TYPE_ARP);


	//arp
	memset( request_arp, 0, sizeof(struct rte_arp_hdr));

    request_arp->arp_hardware = htons(1);									//	ETH_HW_TYPE   1  /* Ethernet hardware type */
    request_arp->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    request_arp->arp_hlen     = 6;
    request_arp->arp_plen     = 4;
    request_arp->arp_opcode   = htons(1);									//  ARP_REQUEST
	

	memset( &request_arp->arp_data.arp_tha, 0xFF, 6);
	request_arp->arp_data.arp_tip = tip;
	
	memcpy( &request_arp->arp_data.arp_sha, srcmac, 6);
	request_arp->arp_data.arp_sip = srcip;
	
	
    mbuf_req->pkt_len  = 60;
    mbuf_req->data_len = 60;


	// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./static_arp_request_12042024.pcap");
	// dpdk_pkt__write_pcap( dumper2, mbuf_req);
	// dpdk_pkt__close_pcap( dumper2);
	// exit(0);
			
	
	return 0;
}

uint8_t * dpe__get_macaddr( uint32_t ip)
{
	int i = 0;
	
	while( i < __dpe->static_arp_table_row_count)
	{
		//printf( "compare ip=%u (%u||%u)\n", __dpe->static_arp_table[i].ip, ip, htonl(ip));
		if( __dpe->static_arp_table[i].ip == ip)
		{
			return __dpe->static_arp_table[i].Mac;
		}
		i++;
	}
	
	i = 0;
	while( i < __dpe->arp_table_row_count)
	{
		if( __dpe->arp_table[i].ip == ip)
		{
			return __dpe->arp_table[i].Mac;
			// memcpy( __dpe->arp_table[i].Mac, request_arp->arp_data.arp_sha.addr_bytes, 6);
			// bFound = 1;
			// //printf("Updated MAC=%02X to IP=%u\n", __dpe->arp_table[i].Mac[5] & 0xFF, __dpe->arp_table[i].ip);
			// break;
		}
		i++;
	}
	return NULL;
}



int dpe__send_gtp_packet( uint8_t * obj, uint32_t gtpseqno, uint32_t upfteid, uint32_t upfip, uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint16_t protocol, uint32_t tcpseqno, char * buffer, int blen, uint8_t * umbufs, uint8_t * mac)
{
	if( protocol == 1)
		gtpseqno = 0;
	
	dpdk_interface_port_t * interface_port = NULL;
	dpdk_interface_t * interface = NULL;
	
	if( obj)
	{
		interface_port 	= (dpdk_interface_port_t *)obj;
		interface 		= interface_port->interface;
	}
	else
	{
		interface 		= dpe__getinterface(0);
	}

	uint16_t queue_id = 0;
	
	if( interface_port) {
		queue_id = interface_port->queue_id;
	}
	
	//uint8_t * mac = dpe__get_macaddr( upfip);
	if(!mac) return -1;
	if(mac[5] == 0) return -2;


	// struct rte_mbuf * m_buf2 = (struct rte_mbuf *)umbufs;
	// rte_pktmbuf_free( m_buf2);
	// return 1;
		
	
	// int enable_chksum = 0;
	// int enable_chksum_usr = 0;
	
	// printf("%02X:%02X:%02X:%02X:%02X:%02X\n", 
		// mac[0] & 0xFF, mac[1] & 0xFF, mac[2] & 0xFF, mac[3] & 0xFF, mac[4] & 0xFF, mac[5] & 0xFF);

	//printf("queue_id=%u %s|%s|%d\n", queue_id, __FILE__, __FUNCTION__, __LINE__);

	
	struct rte_mbuf * m_buf = NULL;
	
	if( umbufs) {
		m_buf = (struct rte_mbuf *)umbufs;
	} else {
		m_buf = rte_pktmbuf_alloc( interface->mempool[queue_id]);
	}
	
	//struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[queue_id]);
	
	
	
	//struct rte_mbuf * m_buf = (struct rte_mbuf *)umbufs;

	


	
	if( m_buf)
	{
		// comments started here 22-07-2023
		
		/*
			ETH									14
			IPv4								20
			UDP									 8
			GTP									12			-- 54
			
			UserPacket
			IP									20
			TCP									20			-- 40
			DATA								Variable	
		*/
		
		int prependLen 		= 0;
		int gtpHeaderLen 	= (gtpseqno > 0) ? 12 : 8;
		
		//IPv4
		//prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 20 + blen;
		
		if( protocol == 1)
		{
			//ICMP request
			
			if(gtpseqno > 0)
				gtpHeaderLen		+= 4;
			else
				gtpHeaderLen		+= 8;
			
			prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + blen;
		}
		else if( protocol == 6)
		{
			prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 20 + blen;
		}
		else if( protocol == 17)
		{
			prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 8 + blen;
		}

		//printf("prependLen=%u protocol=%d\n", prependLen, protocol);

		
		m_buf->pkt_len  	= prependLen;
		m_buf->data_len 	= prependLen;

/*
#define RTE_STATIC_BSWAP16(v) \
        ((((uint16_t)(v) & UINT16_C(0x00ff)) << 8) | \
         (((uint16_t)(v) & UINT16_C(0xff00)) >> 8))

#define RTE_STATIC_BSWAP32(v) \
        ((((uint32_t)(v) & UINT32_C(0x000000ff)) << 24) | \
         (((uint32_t)(v) & UINT32_C(0x0000ff00)) <<  8) | \
         (((uint32_t)(v) & UINT32_C(0x00ff0000)) >>  8) | \
         (((uint32_t)(v) & UINT32_C(0xff000000)) >> 24))
*/		


		unsigned char * packet = rte_pktmbuf_mtod( m_buf, unsigned char *);
		
		struct rte_ether_hdr 	* eth 			= (struct rte_ether_hdr *)packet;
		struct rte_udp_hdr 		* udp  			= NULL;
		struct rte_ipv4_hdr 	* ip_hdr 		= NULL;
		struct rte_ipv4_hdr 	* usr_ip_hdr 	= NULL;
		struct rte_udp_hdr 		* usr_udp  		= NULL;
		
		eth->src_addr.addr_bytes[0] = interface->MacAddr[0];
		eth->src_addr.addr_bytes[1] = interface->MacAddr[1];
		eth->src_addr.addr_bytes[2] = interface->MacAddr[2];
		eth->src_addr.addr_bytes[3] = interface->MacAddr[3];
		eth->src_addr.addr_bytes[4] = interface->MacAddr[4];
		eth->src_addr.addr_bytes[5] = interface->MacAddr[5];
		
		eth->dst_addr.addr_bytes[0] = mac[0];
		eth->dst_addr.addr_bytes[1] = mac[1];
		eth->dst_addr.addr_bytes[2] = mac[2];
		eth->dst_addr.addr_bytes[3] = mac[3];
		eth->dst_addr.addr_bytes[4] = mac[4];
		eth->dst_addr.addr_bytes[5] = mac[5];
		
		eth->ether_type = 8;




		ip_hdr = (struct rte_ipv4_hdr *)&packet[14];
		
		ip_hdr->version_ihl 	= 0x45;
		ip_hdr->type_of_service = 0x00;
		ip_hdr->total_length 	= rte_cpu_to_be_16(prependLen - 14);
		
		ip_hdr->packet_id 		= tcpseqno + (prependLen - 14);
		ip_hdr->fragment_offset = 0;
		ip_hdr->time_to_live 	= 64;
		ip_hdr->next_proto_id 	= 17;
		//ip_hdr->hdr_checksum 	= 0;
		
		ip_hdr->src_addr 		= interface->IPv4;
		ip_hdr->dst_addr 		= rte_cpu_to_be_32( upfip);
		ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);


	// rte_pktmbuf_free( m_buf);
	// return 1;
	
		
		udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

		udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
		udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
		udp->dgram_cksum			= 0;

		udp->dgram_len 			= rte_cpu_to_be_16( prependLen - (14 + 20));
		udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
		
		struct rte_gtp_hdr * gtp  	= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
		gtp->gtp_hdr_info 			= 0x30;
		gtp->msg_type 				= 0xFF;
		gtp->plen 					= rte_cpu_to_be_16( prependLen - (14 + 20 + 8));
		gtp->teid 					= rte_be_to_cpu_32( upfteid); 
	
		if( gtpseqno > 0)
		{
			gtp->gtp_hdr_info 							= 0x32;
			*(uint16_t *)(((unsigned char *)gtp) + 8) 	= rte_be_to_cpu_16( gtpseqno);
			usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr) + 4);
		}
		else
		{
			usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr));
		}
		
		
		if( protocol == 1)
		{
			char * dx = ((char *)&gtp->gtp_hdr_info);
			dx[0] |= 1 << 2;
			
			if( gtpseqno > 0)
			{
				char * dPtr = ((char *)usr_ip_hdr) - 1;
				dPtr[0] = 0x85;
				dPtr[1] = 0x01;
				dPtr[2] = 0x10;
				dPtr[3] = 0x01;
				dPtr[4] = 0x00;
				usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[5];
			}
			else
			{
				char * dPtr = ((char *)usr_ip_hdr);
				dPtr[0] = 0x00;
				dPtr[1] = 0x00;
				dPtr[2] = 0x00;
				dPtr[3] = 0x85;
				dPtr[4] = 0x01;
				dPtr[5] = 0x10;
				dPtr[6] = 0x01;
				dPtr[7] = 0x00;
				usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[8];
			}
		}
		
		
		
		usr_ip_hdr->version_ihl 	= 0x45;
		usr_ip_hdr->type_of_service = 0x00;
		usr_ip_hdr->total_length 	= rte_cpu_to_be_16( prependLen - (14 + 20 + 8 + gtpHeaderLen));
		usr_ip_hdr->packet_id 		= tcpseqno + ( prependLen - (14 + 20 + 8 + gtpHeaderLen));
		usr_ip_hdr->fragment_offset = 0;
		usr_ip_hdr->time_to_live 	= 64;
		usr_ip_hdr->next_proto_id 	= protocol;			//17 UDP,  6 TCP
		
		usr_ip_hdr->src_addr 		= rte_cpu_to_be_32( ueip);
		usr_ip_hdr->dst_addr 		= rte_cpu_to_be_32( dstip);
		
		usr_ip_hdr->hdr_checksum 	= 0;
		usr_ip_hdr->hdr_checksum 	= rte_ipv4_cksum( usr_ip_hdr);

		if( protocol == 6 )
		{
			struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
			
			tcp->src_port				= rte_cpu_to_be_16( srcport);
			tcp->dst_port				= rte_cpu_to_be_16( dstport);
			tcp->cksum					= 0;
			tcp->sent_seq				= tcpseqno;
			tcp->data_off				= 0x50;
			tcp->tcp_flags				= 0x18;
			tcp->rx_win					= 0x4410;
			//tcp->cksum					= rte_ipv4_udptcp_cksum( usr_ip_hdr, tcp);
		}
		else if( protocol == 17)
		{
			usr_udp = (struct rte_udp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
			usr_udp->src_port			= rte_cpu_to_be_16( srcport);
			usr_udp->dst_port			= rte_cpu_to_be_16( dstport);
			usr_udp->dgram_cksum		= 0;

			usr_udp->dgram_len 			= rte_cpu_to_be_16( blen + 8);
			usr_udp->dgram_cksum		= rte_ipv4_udptcp_cksum( usr_ip_hdr, usr_udp);
		}
		else
		{
			char * dptr = ((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
			memcpy( dptr, buffer, blen);
		}
		
		// // // // comments ended here 22-07-2023
		
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtppacket_24012024_01.pcap"); 
		// dpdk_pkt__write_pcap( dumper, m_buf);
		// dpdk_pkt__close_pcap( dumper);
		// exit(0); 
	

		if( rte_ring_enqueue( interface->send_ring[0], m_buf) != 0)
		{
			rte_pktmbuf_free( m_buf);
			return -4;
		}

		//rte_pktmbuf_free( m_buf);
		return 1;
	}
	
	return 0;
}

#ifndef SWAP_BYTES_IN_WORD
#define SWAP_BYTES_IN_WORD(w) (((w) & 0xff) << 8) | (((w) & 0xff00) >> 8)
#endif /* SWAP_BYTES_IN_WORD */

#ifndef FOLD_U32T
#define FOLD_U32T(u)          ((uint32_t)(((u) >> 16) + ((u) & 0x0000ffffUL)))
#endif


static uint16_t ip_checksum_fold( uint64_t sum)
{
	while (sum & ~0xffffffffULL)
		sum = (sum >> 32) + (sum & 0xffffffffULL);
	while (sum & 0xffff0000ULL)
		sum = (sum >> 16) + (sum & 0xffffULL);

	return ~sum;
}

static uint64_t ip_checksum_partial( const void * p, size_t len, uint64_t sum)
{
	/* Main loop: 32 bits at a time.
	 * We take advantage of intel's ability to do unaligned memory
	 * accesses with minimal additional cost. Other architectures
	 * probably want to be more careful here.
	 */
	const uint32_t *p32 = (const uint32_t *)(p);
	for (; len >= sizeof(*p32); len -= sizeof(*p32))
		sum += *p32++;

	/* Handle un-32bit-aligned trailing bytes */
	const uint16_t *p16 = (const uint16_t *)(p32);
	if (len >= 2) {
		sum += *p16++;
		len -= sizeof(*p16);
	}
	if (len > 0) {
		const uint8_t *p8 = (const uint8_t *)(p16);
		sum += ntohs(*p8 << 8);	/* RFC says pad last byte */
	}

	return sum;
}

static uint64_t tcp_udp_v6_header_checksum_partial( struct in6_addr * src_ip, struct in6_addr * dst_ip, uint8_t protocol, uint32_t len)
{
	/* The IPv6 pseudo-header is defined in RFC 2460, Section 8.1. */
	struct ipv6_pseudo_header_t {
		/* We use a union here to avoid aliasing issues with gcc -O2 */
		union {
			struct header {
				struct in6_addr src_ip;
				struct in6_addr dst_ip;
				uint32_t length;
				uint8_t mbz[3];
				uint8_t next_header;
			} __packed fields;
			uint32_t words[10];
		};
	};
	struct ipv6_pseudo_header_t pseudo_header;
	//assert(sizeof(pseudo_header) == 40);

	/* Fill in the pseudo-header. */
	pseudo_header.fields.src_ip = *src_ip;
	pseudo_header.fields.dst_ip = *dst_ip;
	pseudo_header.fields.length = htonl(len);
	memset(pseudo_header.fields.mbz, 0, sizeof(pseudo_header.fields.mbz));
	pseudo_header.fields.next_header = protocol;
	return ip_checksum_partial( &pseudo_header, sizeof(pseudo_header), 0);
}




uint16_t tcp_udp_v6_checksum( struct in6_addr * src_ip, struct in6_addr * dst_ip, uint8_t protocol, void * payload, uint32_t len)
{
	uint64_t sum = tcp_udp_v6_header_checksum_partial( src_ip, dst_ip, protocol, len);
	sum = ip_checksum_partial( payload, len, sum);
	return ip_checksum_fold( sum);
}

struct pbuf {
	struct pbuf *next;
	void * payload;
	uint16_t tot_len;
	uint16_t len;
	uint8_t type_internal;
	uint8_t flags;
	uint8_t if_idx;
};

uint16_t lwip_standard_chksum( void * dataptr, int len)
{
	uint32_t acc;
	uint16_t src;
	uint8_t * octetptr;

	acc = 0;
	/* dataptr may be at odd or even addresses */
	octetptr = (uint8_t *)dataptr;
	while (len > 1) 
	{
		src = (*octetptr) << 8;
		octetptr++;
	
		/* declare second octet as least significant */
		src |= (*octetptr);
		octetptr++;
		acc += src;
		len -= 2;
	}
	
	if (len > 0) 
	{
		/* accumulate remaining octet */
		src = (*octetptr) << 8;
		acc += src;
	}
	
	/* add deferred carry bits */
	acc = (acc >> 16) + (acc & 0x0000ffffUL);
	if ((acc & 0xffff0000UL) != 0) 
	{
		acc = (acc >> 16) + (acc & 0x0000ffffUL);
	}

	return htons((uint16_t)acc);
}

uint16_t inet_cksum_pseudo_base( struct pbuf * p, uint8_t proto, uint16_t proto_len, uint32_t acc)
{
	struct pbuf * q;
	int swapped = 0;

	for (q = p; q != NULL; q = q->next) 
	{
		acc += lwip_standard_chksum( q->payload, q->len);
		acc = FOLD_U32T( acc);

		if (q->len % 2 != 0) 
		{
			swapped = !swapped;
			acc = SWAP_BYTES_IN_WORD(acc);
		}
	}
	
	if (swapped) 
	{
		acc = SWAP_BYTES_IN_WORD(acc);
	}	
	
	acc += (uint32_t)htons((uint16_t)proto);
	acc += (uint32_t)htons(proto_len);	

	acc = FOLD_U32T(acc);
	acc = FOLD_U32T(acc);

	//dpe__print_buffer( "acc", (char *)&acc, 4, 0, 0);	
	//uint16_t u1 = (uint16_t)~(acc & 0xffffUL);
	//dpe__print_buffer( "acc", (char *)&u1, 2, 0, 0);	
	return (uint16_t)~(acc & 0xffffUL);	
}


uint16_t ip6_chksum_pseudo( struct pbuf * p, uint8_t proto, uint16_t proto_len, uint8_t * src, uint8_t *dest)
{
	uint32_t 	acc = 0;
	uint32_t 	addr;
	uint8_t 	addr_part;

	for (addr_part = 0; addr_part < 4; addr_part++) 
	{
		addr = src[addr_part];
		acc = (uint32_t)(acc + (addr & 0xffffUL));
		acc = (uint32_t)(acc + ((addr >> 16) & 0xffffUL));
		addr = dest[addr_part];
		acc = (uint32_t)(acc + (addr & 0xffffUL));
		acc = (uint32_t)(acc + ((addr >> 16) & 0xffffUL));
	}

	/* fold down to 16 bits */
	acc = FOLD_U32T(acc);
	acc = FOLD_U32T(acc);

	return inet_cksum_pseudo_base( p, proto, proto_len, acc);
}

uint16_t sof2_checksum( uint16_t * addr, int len)
{
    int count = len;
    register uint32_t sum = 0;
    uint16_t answer = 0;

    while (count > 1) {
        //printf("%x ", htons(*addr));
        sum += *(addr++);
        count -= 2;
    }

    /* Add left-over byte, if any. */
    if (count > 0) {
        sum += *(uint8_t *)addr;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    /* Checksum is one's compliment of sum. */
    answer = ~sum;
 
    return (answer);
}




uint16_t sof_checksum( struct in6_addr * src, struct in6_addr * dst, const void * data, size_t len) 
{
    uint32_t checksum = 0;
    union 
	{
        uint32_t dword;
        uint16_t word[2];
        uint8_t byte[4];
    } temp;

    // IPv6 Pseudo header source address, destination address, length, zeros, next header

    checksum += src->s6_addr16[0];
    checksum += src->s6_addr16[1];
    checksum += src->s6_addr16[2];
    checksum += src->s6_addr16[3];
    checksum += src->s6_addr16[4];
    checksum += src->s6_addr16[5];
    checksum += src->s6_addr16[6];
    checksum += src->s6_addr16[7];

    checksum += dst->s6_addr16[0];
    checksum += dst->s6_addr16[1];
    checksum += dst->s6_addr16[2];
    checksum += dst->s6_addr16[3];
    checksum += dst->s6_addr16[4];
    checksum += dst->s6_addr16[5];
    checksum += dst->s6_addr16[6];
    checksum += dst->s6_addr16[7];

    temp.dword = htonl(len);
    checksum += temp.word[0];
	checksum += temp.word[1];
	
    temp.byte[0] = 0;
    temp.byte[1] = 0;
    temp.byte[2] = 0;
    temp.byte[3] = 58; // ICMPv6

    checksum += temp.word[0];
    checksum += temp.word[1];
	

    while (len > 1) 
	{
        checksum += *((const uint16_t *)data);
        data = (const uint16_t *)data + 1;
        len -= 2;
    }

    if (len > 0)
        checksum += *((const uint8_t *)data);

    printf("Checksum %x\n", checksum);

    while (checksum >> 16 != 0)
        checksum = (checksum & 0xffff) + (checksum >> 16);

    checksum = ~checksum;

    return (uint16_t)checksum;
	
	// checksum += checksum >> 16;
	// return (uint16_t)~checksum;
}

uint16_t y2checksum(uint16_t *addr, int len)
{
    int count = len;
    register uint32_t sum = 0;
    uint16_t answer = 0;

    /* Sum up 2-byte values until none or only one byte left. */
    //printf("The are bytes from the packets for calculating the checksum:\n");
    //printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    while (count > 1) 
	{
        //printf("%x ", htons(*addr));
        sum += *(addr++);
        count -= 2;
    }
    //printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    //printf("\n");

    /* Add left-over byte, if any. */
    if (count > 0) {
        sum += *(uint8_t *)addr;
    }

    /* 
     * Fold 32-bit sum into 16 bits; we lose information by doing this,
     * increasing the chances of a collision.
     * sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
    */
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    //printf("sum: %x\n", sum);

    /* Checksum is one's compliment of sum. */
    answer = ~sum;
    //printf("1's complement of sum is answer(Final Checksum): %x\n", answer);

    return (answer);
}


uint16_t y2icmp6_checksum( uint8_t * srcip, uint8_t * dstip, uint8_t protocol, uint8_t * msg, uint16_t icmppktlen)
{
	/*
	The IPv6 pseudo-header includes the 
		Source Address								16 bytes
		Destination Address							16 bytes
		Upper Layer Packet Length 				 	 4 bytes
		Zeros										 3 bytes
		Next Header									 1 byte
	
		40 bytes
		
	2.3.  Message Checksum Calculation

	   The checksum is the 16-bit one's complement of the one's complement
	   sum of the entire ICMPv6 message, starting with the ICMPv6 message
	   type field, and prepended with a "pseudo-header" of IPv6 header
	   fields, as specified in [IPv6, Section 8.1].  The Next Header value
	   used in the pseudo-header is 58.  (The inclusion of a pseudo-header
	   in the ICMPv6 checksum is a change from IPv4; see [IPv6] for the
	   rationale for this change.)

	   For computing the checksum, the checksum field is first set to zero.
   
	https://www.rfc-editor.org/rfc/rfc2460#page-27
	8.1 Upper-Layer Checksums
	*/
	
	char 	buf[200] = {0};
	char 	* ptr = NULL;	
    int 	chksumlen = 0;
    int 	i = 0;	

    ptr = &buf[0]; 

    memcpy( ptr, srcip, 16);
    ptr += 16;
    chksumlen += 16;
	
    memcpy( ptr, dstip, 16);
    ptr += 16;
    chksumlen += 16;

    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = (icmppktlen) / 256;
    ptr++;
    *ptr = (icmppktlen) % 256;
    ptr++;
    chksumlen += 4;

    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 3;
	
	*ptr = protocol;
	ptr++;
	chksumlen++;
	// pseudo-header completed
	
	memcpy( ptr, msg, icmppktlen);

	ptr += icmppktlen;
	chksumlen += icmppktlen;
	
	//dpe__print_buffer( "1", buf, chksumlen, 1, 8);
	//return y2checksum( (uint16_t *)ptr, icmppktlen);
	return sof2_checksum( (uint16_t *)buf, chksumlen);
}




// https://blog.apnic.net/2019/10/18/how-to-ipv6-neighbor-discovery/
void dpe__send_ipv6rs_request( dpdk_interface_t * interface)
{
	if( interface->NullMCLRCount < 5 || interface->NullNebSol < 2)
		return;
	
	interface->RSCounter++;
	
	if( interface->RSCounter < 5)
		return;
	
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	rte_prefetch0( rte_pktmbuf_mtod( m_buf, void *));

	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( m_buf, struct rte_ether_hdr *);
	
	rte_memcpy( eth->dst_addr.addr_bytes, "\x33\x33\x00\x00\x00\x02", 6);
	//rte_memcpy( eth->dst_addr.addr_bytes, "\x1C\x98\xEC\x17\x21\x8C", 6);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);

	struct rte_ipv6_hdr * ip6_hdr 	= NULL;
	
	ip6_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	

	ip6_hdr->vtc_flow 		= htonl(6 << 28);
	ip6_hdr->vtc_flow 		|= htonl( 104 + interface->Index + 1 );
	//ip6_hdr->vtc_flow 		|= htonl(0x7B0E); 
    ip6_hdr->proto 			= 58;
	ip6_hdr->hop_limits		= 255;
	ip6_hdr->payload_len	= htons(16);

	rte_memcpy( ip6_hdr->src_addr, &interface->IPv6, 16);
	rte_memcpy( ip6_hdr->dst_addr, "\xFF\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02", 16); 
	//rte_memcpy( ip6_hdr->dst_addr, "\xFE\x80\x00\x00\x00\x00\x00\x00\x1E\x98\xEC\xFF\xFE\x17\x21\x8C", 16); 
	
	uint8_t * icmpPtr = (uint8_t *)(((unsigned char *)eth) + 14 + sizeof(struct rte_ipv6_hdr));
	
	icmpPtr[0] = 133;	//type
	icmpPtr[1] = 0;		//code
	icmpPtr[2] = 0;		//checksum
	icmpPtr[3] = 0;
	icmpPtr[4] = 0;		//reserved
	icmpPtr[5] = 0;
	icmpPtr[6] = 0;
	icmpPtr[7] = 0;

	//Option - SLLA Source Link Layer Addreess
	icmpPtr[8] = 1;
	icmpPtr[9] = 1;
	rte_memcpy( &icmpPtr[10], interface->MacAddr, 6);
	
	uint16_t cksum = y2icmp6_checksum( ip6_hdr->src_addr, ip6_hdr->dst_addr, 58, icmpPtr, 16);
	*(uint16_t*)(&icmpPtr[2]) = cksum;

	
	m_buf->pkt_len  = 70;
    m_buf->data_len = 70;
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmpv6_RSFL_15052024.pcap");
	// dpdk_pkt__write_pcap( dumper, m_buf);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);

	if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
	{
		interface->RSCounter = 0;
		
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed Router Solicitation RSCounter=%u  for IP=%u.%u.%u.%u from BusId=%s", 
			interface->RSCounter, interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending Router Solicitation Failed for IP=%u.%u.%u.%u from BusId=%s", 
			interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
}



// Multicast Listner Report V2
void dpe__send_ipv6mlrm_request( dpdk_interface_t * interface)
{
	if( interface->NullMCLRCount >= 6)
		return;
	
	uint8_t NullTo16 = 0;
	
	if( interface->NullMCLRCount < 2)
	{
		NullTo16 = 1;
	}
	NullTo16 = 0;
	
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	rte_prefetch0( rte_pktmbuf_mtod( m_buf, void *));

	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( m_buf, struct rte_ether_hdr *);
	
	rte_memcpy( eth->dst_addr.addr_bytes, "\x33\x33\x00\x00\x00\x16", 6);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);

	struct rte_ipv6_hdr * ip6_hdr 	= NULL;
	
	ip6_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	
	ip6_hdr->vtc_flow 		= htonl(6 << 28);
    ip6_hdr->proto 			= 0;
	ip6_hdr->hop_limits		= 1;
	ip6_hdr->payload_len	= htons(36);

	if( NullTo16 == 1)
	{
		rte_memcpy( ip6_hdr->src_addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
	}
	else
	{
		rte_memcpy( ip6_hdr->src_addr, &interface->IPv6, 16);
	}
	
	rte_memcpy( ip6_hdr->dst_addr, "\xFF\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x16", 16); 

	uint8_t * hphPtr = (uint8_t *)(((unsigned char *)eth) + 14 + sizeof(struct rte_ipv6_hdr));
	hphPtr[0] = 0x3A;
	hphPtr[1] = 0x00;
	hphPtr[2] = 0x05;
	hphPtr[3] = 0x02;
	hphPtr[4] = 0x00;
	hphPtr[5] = 0x00;
	hphPtr[6] = 0x01;
	hphPtr[7] = 0x00;

	uint8_t * icmpPtr = (hphPtr + 8);
	
	icmpPtr[0] = 0x8F;
	icmpPtr[1] = 0;
	icmpPtr[2] = 0;	//checksum
	icmpPtr[3] = 0;	
	icmpPtr[4] = 0; //reserved
	icmpPtr[5] = 0;	//reserved
	icmpPtr[6] = 0;	
	icmpPtr[7] = 1;
	
	icmpPtr[8] 	= 4;
	icmpPtr[9] 	= 0;
	icmpPtr[10] = 0;
	icmpPtr[11] = 0;
	
	rte_memcpy( &icmpPtr[12], "\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff", 13);
	rte_memcpy( &icmpPtr[25], &interface->IPv6[13], 3);

	
	m_buf->pkt_len  = 90;
    m_buf->data_len = 90;	
	
	uint16_t cksum = y2icmp6_checksum( ip6_hdr->src_addr, ip6_hdr->dst_addr, 58, icmpPtr, 28);
	*(uint16_t*)(&icmpPtr[2]) = cksum;
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmpv6_MCLRv2_1505_1.pcap");
	// dpdk_pkt__write_pcap( dumper, m_buf);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);	

	if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
	{
		interface->NullMCLRCount++;

		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed Multi Listner Report V2 NullTo16=%u NullMCLRCount=%u  for IP=%u.%u.%u.%u from BusId=%s", 
			NullTo16, interface->NullMCLRCount, interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending Multi Listner Report V2 Failed for IP=%u.%u.%u.%u from BusId=%s", 
			interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
}


// Neighbor Solicitation (NS), this is specifically unspecified to self-ip
void dpe__send_ipv6ns_request( dpdk_interface_t * interface)
{
	if( interface->NullMCLRCount < 2)
	{
		return;
	}
	
	if( interface->NullNebSol >= 2)
	{
		return;
	}
	
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	rte_prefetch0( rte_pktmbuf_mtod( m_buf, void *));

	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( m_buf, struct rte_ether_hdr *);
	
	rte_memcpy( eth->dst_addr.addr_bytes, "\x33\x33\xFF", 3);
	rte_memcpy( &eth->dst_addr.addr_bytes[3], &interface->IPv6[13], 3);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);

	struct rte_ipv6_hdr * ip6_hdr 	= NULL;
	
	ip6_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	
	ip6_hdr->vtc_flow 		= htonl(6 << 28);
    ip6_hdr->proto 			= 58;
	ip6_hdr->hop_limits		= 255;
	ip6_hdr->payload_len	= htons(24+8);

	rte_memcpy( ip6_hdr->src_addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
	rte_memcpy( ip6_hdr->dst_addr, &interface->IPv6, 16);

	uint8_t * icmpPtr = (uint8_t *)(((unsigned char *)eth) + 14 + sizeof(struct rte_ipv6_hdr));
	icmpPtr[0] = 0x87;
	icmpPtr[1] = 0;
	icmpPtr[2] = 0;	//checksum
	icmpPtr[3] = 0;	
	icmpPtr[4] = 0; //reserved
	icmpPtr[5] = 0;	//reserved
	icmpPtr[6] = 0;	
	icmpPtr[7] = 0;
	

	interface->NSNonce = (uint8_t *)m_buf;
	
	
	rte_memcpy( &icmpPtr[8], &interface->IPv6, 16);

	icmpPtr[24] = 14;
	icmpPtr[25] = 1;
	
	rte_memcpy( &icmpPtr[26], &interface->NSNonce, 6);


	
	m_buf->pkt_len  = 78+8;
    m_buf->data_len = 78+8;	
	
	uint16_t cksum = y2icmp6_checksum( ip6_hdr->src_addr, ip6_hdr->dst_addr, 58, icmpPtr, 32);
	*(uint16_t*)(&icmpPtr[2]) = cksum;
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmpv6_NS_13052024_2.pcap");
	// dpdk_pkt__write_pcap( dumper, m_buf);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);

	if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
	{
		interface->NullNebSol++;
		
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed Self=%u Neighbor Solicitation for IP=%u.%u.%u.%u from BusId=%s", 
			interface->NullNebSol, interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending Neighbor Solicitation Failed for IP=%u.%u.%u.%u from BusId=%s", 
			interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
	}
}

void dpe__encode_icmpv6_echo__resp( struct rte_mbuf * mbuf_request, struct rte_mbuf * mbuf_response, char * srcmac, uint8_t * srcip, dpdk_interface_t * interface)
{
	rte_prefetch0( rte_pktmbuf_mtod( mbuf_request, void *));
	rte_prefetch0( rte_pktmbuf_mtod( mbuf_response, void *));
	
	struct rte_ether_hdr * srceth = rte_pktmbuf_mtod( mbuf_request, struct rte_ether_hdr *);
	
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf_response, struct rte_ether_hdr *);

	rte_memcpy( eth->dst_addr.addr_bytes, srceth->src_addr.addr_bytes, 6);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);

	struct rte_ipv6_hdr * req_ip6_hdr =  (struct rte_ipv6_hdr *)(((unsigned char *)srceth) + 14);
	uint8_t * req_icmpPtr = (uint8_t *)(((unsigned char *)srceth) + 14 + sizeof(struct rte_ipv6_hdr));
	
	struct rte_ipv6_hdr * ip6_hdr 	= NULL;
	ip6_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	
	ip6_hdr->vtc_flow 		= htonl(6 << 28);
    ip6_hdr->proto 			= 58;
	ip6_hdr->hop_limits		= 255;
	ip6_hdr->payload_len	= req_ip6_hdr->payload_len;

	rte_memcpy( ip6_hdr->src_addr, &interface->IPv6, 16);
	rte_memcpy( ip6_hdr->dst_addr, req_ip6_hdr->src_addr, 16);

	uint8_t * icmpPtr = (uint8_t *)(((unsigned char *)eth) + 14 + sizeof(struct rte_ipv6_hdr));
	icmpPtr[0] = 0x81;
	icmpPtr[1] = 0;
	icmpPtr[2] = 0;
	icmpPtr[3] = 0;
	
	rte_memcpy( &icmpPtr[4], &req_icmpPtr[4], htons( ip6_hdr->payload_len)-4);
	

	mbuf_response->pkt_len  = (14 + 40 + htons(ip6_hdr->payload_len));
    mbuf_response->data_len = mbuf_response->pkt_len;	
	
	uint16_t cksum = y2icmp6_checksum( ip6_hdr->src_addr, ip6_hdr->dst_addr, 58, icmpPtr, htons( ip6_hdr->payload_len));
	*(uint16_t*)(&icmpPtr[2]) = cksum;
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmpv6_echo_response_13052024_2.pcap");
	// dpdk_pkt__write_pcap( dumper, mbuf_response);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);	

}

void dpe__encode_neighbor_advertisement_resp( struct rte_mbuf * mbuf_request, struct rte_mbuf * mbuf_response, char * srcmac, uint8_t * srcip, dpdk_interface_t * interface)
{
	rte_prefetch0( rte_pktmbuf_mtod( mbuf_request, void *));
	rte_prefetch0( rte_pktmbuf_mtod( mbuf_response, void *));
	
	struct rte_ether_hdr * srceth = rte_pktmbuf_mtod( mbuf_request, struct rte_ether_hdr *);
	
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf_response, struct rte_ether_hdr *);

	rte_memcpy( eth->dst_addr.addr_bytes, srceth->src_addr.addr_bytes, 6);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);
	
	struct rte_ipv6_hdr * ip6_hdr 	= NULL;
	
	ip6_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	
	ip6_hdr->vtc_flow 		= htonl(6 << 28);
    ip6_hdr->proto 			= 58;
	ip6_hdr->hop_limits		= 255;
	ip6_hdr->payload_len	= htons(32);

	rte_memcpy( ip6_hdr->src_addr, &interface->IPv6, 16);
	rte_memcpy( ip6_hdr->dst_addr, &interface->IPv6, 13);
	rte_memcpy( &ip6_hdr->dst_addr[13], &eth->dst_addr.addr_bytes[3], 3);
	
	uint8_t * icmpPtr = (uint8_t *)(((unsigned char *)eth) + 14 + sizeof(struct rte_ipv6_hdr));
	icmpPtr[0] = 0x88;
	icmpPtr[1] = 0;
	icmpPtr[2] = 0;
	icmpPtr[3] = 0;
	
	//Flags
	icmpPtr[4] = 0x60;
	icmpPtr[5] = 0;
	icmpPtr[6] = 0;
	icmpPtr[7] = 0;
	
	rte_memcpy( &icmpPtr[8], &interface->IPv6, 16); 
	
	icmpPtr[24] = 2;
	icmpPtr[25] = 1;
	
	rte_memcpy( &icmpPtr[26], interface->MacAddr, 6);
	
	mbuf_response->pkt_len  = 86;
    mbuf_response->data_len = 86;	
	
	uint16_t cksum = y2icmp6_checksum( ip6_hdr->src_addr, ip6_hdr->dst_addr, 58, icmpPtr, 32);
	*(uint16_t*)(&icmpPtr[2]) = cksum;
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmpv6_NA_13052024_2.pcap");
	// dpdk_pkt__write_pcap( dumper, mbuf_response);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);
}

struct rte_mbuf * dpe__create__neighbor_advertisement_resp( struct rte_mbuf * mbuf, dpdk_interface_t * interface)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( interface->mempool[0]);

	if( resp_m_buf)
	{
		dpe__encode_neighbor_advertisement_resp( mbuf, resp_m_buf, interface->MacAddr, interface->IPv6, interface);		
		return resp_m_buf;
	}
	
	return NULL;
}

struct rte_mbuf * dpe__create__icmpv6_echo__resp( struct rte_mbuf * mbuf, dpdk_interface_t * interface)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( interface->mempool[0]);

	if( resp_m_buf)
	{
		dpe__encode_icmpv6_echo__resp( mbuf, resp_m_buf, interface->MacAddr, interface->IPv6, interface);		
		return resp_m_buf;
	}
	
	return NULL;
}


void dpe__send_dhcp_request( dpdk_interface_t * interface)
{
	if(!interface) 
	{
		return;
	}

	if(interface->DHCPDiscover == 0 && interface->DHCPInform == 0 ) 
	{
		return;
	}

	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	rte_prefetch0( rte_pktmbuf_mtod( m_buf, void *));
	
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( m_buf, struct rte_ether_hdr *);
	
	rte_memcpy( eth->dst_addr.addr_bytes, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rte_memcpy( eth->src_addr.addr_bytes, interface->MacAddr, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV4);
	
	struct rte_ipv4_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr  * udp  		= NULL;
	struct rte_gtp_hdr  * gtp  		= NULL;
	
	ip_hdr = (struct rte_ipv4_hdr *)(((unsigned char *)eth) + 14);
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	

	int payloadlen = 300;
	
	ip_hdr->total_length 	= htons(20 + 8 + payloadlen);		//ipv4=20 + udp=8 + payloadlen
	m_buf->pkt_len  = (14 + 20 + 8 + payloadlen);
	m_buf->data_len = (14 + 20 + 8 + payloadlen);

	ip_hdr->packet_id 		= 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	
	ip_hdr->src_addr 		= interface->IPv4;
	ip_hdr->dst_addr 		= 0xFFFFFFFF;
	
	ip_hdr->hdr_checksum 	= 0;
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);

	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	udp->src_port				= rte_cpu_to_be_16( 68);
	udp->dst_port				= rte_cpu_to_be_16( 67);
	udp->dgram_len				= rte_cpu_to_be_16( 8 + payloadlen);
	udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);

	
	
	
	if( interface->DHCPDiscover == 1) 
	{
		
	} 
	else if( interface->DHCPInform == 1) 
	{
		int mlex = 0;
		__dhcp__encode_inform_packet( (unsigned char *)gtp, &mlex, interface->MacAddr, interface->IPv4, "upf-in", 6, interface->DHCPInformTransactionId);
	
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./dhcp__inform_req_07052024.pcap");
		// dpdk_pkt__write_pcap( dumper, m_buf);
		// dpdk_pkt__close_pcap( dumper);
		// exit(0);

		if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
		{
			dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed DHCP Inform for IP=%u.%u.%u.%u from BusId=%s", 
				interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
		}
		else
		{
			dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending DHCP Inform Failed for IP=%u.%u.%u.%u from BusId=%s", 
				interface->IPv4 & 0xFF, (interface->IPv4 >> 8) & 0xFF, (interface->IPv4 >> 16) & 0xFF, (interface->IPv4 >> 24) & 0xFF, interface->Port);
		}
	}
}

void dpe__send_arp2( dpdk_interface_t * interface, uint32_t tip)
{
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	dpdk_pkt__encode_arp_request( m_buf, interface->MacAddr, interface->IPv4, htonl(tip));

	interface->arp_request_sent++;

	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./arp_24012024.pcap");
	// dpdk_pkt__write_pcap( dumper, m_buf);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);

	if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed ARP Request for IP=%u.%u.%u.%u from BusId=%s", 
			tip & 0xFF, (tip >> 8) & 0xFF, (tip >> 16) & 0xFF, (tip >> 24) & 0xFF, interface->Port);
	}			
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending ARP Request Failed for IP=%u.%u.%u.%u from BusId=%s", 
			tip & 0xFF, (tip >> 8) & 0xFF, (tip >> 16) & 0xFF, (tip >> 24) & 0xFF, interface->Port);
	}
}

void dpe__send_arp( dpdk_interface_t * interface, uint32_t tip)
{
	if(dpe__get_macaddr(tip))			//having static or dynamic MAC
		return;
	
	if(!interface) {
		interface = dpe__getinterface(0);
	}
	
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	dpdk_pkt__encode_arp_request( m_buf, interface->MacAddr, interface->IPv4, htonl(tip));

	interface->arp_request_sent++;

	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./arp_24012024.pcap");
	// dpdk_pkt__write_pcap( dumper, m_buf);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);

	if( rte_ring_enqueue( interface->send_ring[0], m_buf) == 0)
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed ARP Request for IP=%u.%u.%u.%u from BusId=%s", 
			tip & 0xFF, (tip >> 8) & 0xFF, (tip >> 16) & 0xFF, (tip >> 24) & 0xFF, interface->Port);
	}			
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending ARP Request Failed for IP=%u.%u.%u.%u from BusId=%s", 
			tip & 0xFF, (tip >> 8) & 0xFF, (tip >> 16) & 0xFF, (tip >> 24) & 0xFF, interface->Port);
	}
}


int dpe__encode_icmp_response( struct rte_mbuf * mbuf_req, struct rte_mbuf * mbuf_rsp, char * srcmac, uint32_t srcip)
{
	struct rte_ether_hdr * 	request_icmp_eth_hdr 	= rte_pktmbuf_mtod( mbuf_req, struct rte_ether_hdr *);
	struct rte_ether_hdr * 	response_icmp_eth_hdr 	= rte_pktmbuf_mtod( mbuf_rsp, struct rte_ether_hdr *);

	// dpe__print_mac( 0, &request_icmp_eth_hdr->src_addr);
	// dpe__print_mac( 0, &request_icmp_eth_hdr->dst_addr);

	memcpy( &response_icmp_eth_hdr->src_addr, srcmac, 6);
	//rte_ether_addr_copy( &response_icmp_eth_hdr->dst_addr, &request_icmp_eth_hdr->src_addr);
	rte_ether_addr_copy( &request_icmp_eth_hdr->src_addr, &response_icmp_eth_hdr->dst_addr);


	response_icmp_eth_hdr->ether_type = request_icmp_eth_hdr->ether_type;

	

	unsigned char * p_resp = rte_pktmbuf_mtod( mbuf_rsp, unsigned char *);
	struct rte_ipv4_hdr * response_ip_hdr = (struct rte_ipv4_hdr *)&p_resp[14];

	unsigned char * p_req = rte_pktmbuf_mtod( mbuf_req, unsigned char *);
	struct rte_ipv4_hdr * request_ip_hdr = (struct rte_ipv4_hdr *)&p_req[14];


	response_ip_hdr->version_ihl 		= 0x45;
	response_ip_hdr->type_of_service 	= 0x00;
	response_ip_hdr->total_length 		= request_ip_hdr->total_length;

	response_ip_hdr->src_addr 			= request_ip_hdr->dst_addr;
	response_ip_hdr->dst_addr 			= request_ip_hdr->src_addr;
	response_ip_hdr->packet_id 			= request_ip_hdr->packet_id + 1;
	response_ip_hdr->fragment_offset 	= 0;
	response_ip_hdr->time_to_live 		= 64;
	response_ip_hdr->next_proto_id 		= 1;
	response_ip_hdr->hdr_checksum		= 0;
	
	response_ip_hdr->hdr_checksum 		= rte_ipv4_cksum( response_ip_hdr);


	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMP Request FROM  %02X:%02X:%02X:%02X:%02X:%02X | %u.%u.%u.%u    %02X:%02X:%02X:%02X:%02X:%02X |%u.%u.%u.%u  req size=%d", 
		request_icmp_eth_hdr->src_addr.addr_bytes[0] & 0xFF, request_icmp_eth_hdr->src_addr.addr_bytes[1] & 0xFF, 
		request_icmp_eth_hdr->src_addr.addr_bytes[2] & 0xFF, request_icmp_eth_hdr->src_addr.addr_bytes[3] & 0xFF, 
		request_icmp_eth_hdr->src_addr.addr_bytes[4] & 0xFF, request_icmp_eth_hdr->src_addr.addr_bytes[5] & 0xFF,
		request_ip_hdr->src_addr & 0xFF, (request_ip_hdr->src_addr >> 8) & 0xFF, (request_ip_hdr->src_addr >> 16) & 0xFF, (request_ip_hdr->src_addr >> 24) & 0xFF,
		request_icmp_eth_hdr->dst_addr.addr_bytes[0] & 0xFF, request_icmp_eth_hdr->dst_addr.addr_bytes[1] & 0xFF, 
		request_icmp_eth_hdr->dst_addr.addr_bytes[2] & 0xFF, request_icmp_eth_hdr->dst_addr.addr_bytes[3] & 0xFF, 
		request_icmp_eth_hdr->dst_addr.addr_bytes[4] & 0xFF, request_icmp_eth_hdr->dst_addr.addr_bytes[5] & 0xFF,
		request_ip_hdr->dst_addr & 0xFF, (request_ip_hdr->dst_addr >> 8) & 0xFF, (request_ip_hdr->dst_addr >> 16) & 0xFF, (request_ip_hdr->dst_addr >> 24) & 0xFF,
		mbuf_req->pkt_len - 34
	);

	
		

	memcpy( &p_resp[34], &p_req[34], mbuf_req->pkt_len - 34);
	
	struct rte_icmp_hdr * response_icmp	= (struct rte_icmp_hdr *)&p_resp[34];
	response_icmp->icmp_type			= RTE_IP_ICMP_ECHO_REPLY;
	
	uint32_t cksum;
	cksum = ~response_icmp->icmp_cksum & 0xffff;
	cksum += ~htons(RTE_IP_ICMP_ECHO_REQUEST << 8) & 0xffff;
	cksum += htons(RTE_IP_ICMP_ECHO_REPLY << 8);
	cksum = (cksum & 0xffff) + (cksum >> 16);
	cksum = (cksum & 0xffff) + (cksum >> 16);
	response_icmp->icmp_cksum = ~cksum;


	
	mbuf_rsp->pkt_len  = mbuf_req->pkt_len;
    mbuf_rsp->data_len = mbuf_req->data_len;
	
	
	return 0;
}


int dpe__encode_dhcp_response( struct rte_mbuf * response, struct rte_mbuf * request, dpdk_interface_t * interface, int msg_type, struct dhcp_packet * dpacket, int len, uint32_t IPv4)
{
	int dhcplen = 0;
	
	char * res_buffer 	= rte_pktmbuf_mtod( response, char *);
	char * req_buffer 	= rte_pktmbuf_mtod( request, char *);

	memcpy( res_buffer, 		"\xFF\xFF\xFF\xFF\xFF\xFF", 6);	
	memcpy( &res_buffer[6], 	interface->MacAddr, 6);	
	memcpy( &res_buffer[12],	"\x08\x00", 2);	
		
	
	struct dhcp_packet * dresp_packet = (struct dhcp_packet *) &res_buffer[14 + 20 + 8];
	
	if( msg_type == 1)
	{
		__dhcp__encode_offer_packet( (unsigned char *)dresp_packet, &dhcplen, &req_buffer[6], interface->IPv4, IPv4, "", 0, dpacket->xid, 2);	
	} 
	else if( msg_type == 3)
	{
		__dhcp__encode_offer_packet( (unsigned char *)dresp_packet, &dhcplen, &req_buffer[6], interface->IPv4, IPv4, "", 0, dpacket->xid, 5);
	}
	
	struct rte_ipv4_hdr * response_ip_hdr = (struct rte_ipv4_hdr *)&res_buffer[14];
	
	response_ip_hdr->version_ihl 		= 0x45;
	response_ip_hdr->type_of_service 	= 0x00;
	response_ip_hdr->total_length 		= htons(20 + 8 + dhcplen);

	response_ip_hdr->src_addr 			= interface->IPv4;
	response_ip_hdr->dst_addr 			= 0xFFFFFFFF;
	response_ip_hdr->packet_id 			= 1;
	response_ip_hdr->fragment_offset 	= 0;
	response_ip_hdr->time_to_live 		= 64;
	response_ip_hdr->next_proto_id 		= 17;
	response_ip_hdr->hdr_checksum		= 0;
	
	response_ip_hdr->hdr_checksum 		= rte_ipv4_cksum( response_ip_hdr);
	
	
	struct rte_udp_hdr  * udp  			= (struct rte_udp_hdr *)&res_buffer[14 + 20];
	
	udp->src_port						= rte_cpu_to_be_16( 67);
	udp->dst_port						= rte_cpu_to_be_16( 68);
	udp->dgram_len						= rte_cpu_to_be_16( 8 + dhcplen);
	udp->dgram_cksum					= 0;
	udp->dgram_cksum					= rte_ipv4_udptcp_cksum( response_ip_hdr, udp);
	
	
	response->pkt_len  = ( 14 + 20 + 8 + dhcplen);
    response->data_len = ( 14 + 20 + 8 + dhcplen);
	
	
	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./dhcp_offer_response_04062024.pcap");
	// dpdk_pkt__write_pcap( dumper, response);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);
	
	return 0;
}

int dpe__encode_arp_response( struct rte_mbuf * mbuf_req, struct rte_mbuf * mbuf_rsp, char * srcmac, uint32_t srcip)
{
	struct rte_ether_hdr * 	request_eth = rte_pktmbuf_mtod( mbuf_req, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	request_arp = (struct rte_arp_hdr *)&request_eth[1];

	struct rte_ether_hdr * 	response_eth = rte_pktmbuf_mtod( mbuf_rsp, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	response_arp = (struct rte_arp_hdr *)&response_eth[1];


	//eth
    //rte_ether_addr_copy( &request_eth->dst_addr, &response_eth->src_addr);
	memcpy( &response_eth->src_addr, srcmac, 6);
	rte_ether_addr_copy( &request_eth->src_addr, &response_eth->dst_addr);
	response_eth->ether_type = htons( RTE_ETHER_TYPE_ARP);

	
	//arp
	memset( response_arp, 0, sizeof(struct rte_arp_hdr));

    response_arp->arp_hardware = htons(1);									//	ETH_HW_TYPE   1  /* Ethernet hardware type */
    response_arp->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    response_arp->arp_hlen     = 6;
    response_arp->arp_plen     = 4;
    response_arp->arp_opcode   = htons(2);									//  ARP_REPLY

	char tarmac[7];
	memcpy( tarmac, &request_arp->arp_data.arp_sha, 6);
	
	memcpy( &response_arp->arp_data.arp_tha, &request_arp->arp_data.arp_sha, 6);
	response_arp->arp_data.arp_tip = request_arp->arp_data.arp_sip;
	
	memcpy( &response_arp->arp_data.arp_sha, srcmac, 6);
	response_arp->arp_data.arp_sip = srcip;
	
    mbuf_rsp->pkt_len  = 60;
    mbuf_rsp->data_len = 60;

	// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./arp_response_24012024.pcap");
	// dpdk_pkt__write_pcap( dumper, mbuf_req);
	// dpdk_pkt__write_pcap( dumper, mbuf_rsp);
	// dpdk_pkt__close_pcap( dumper);
	// exit(0);
	
	return 0;
}

struct rte_mbuf * dpe__create_arp( struct rte_mbuf * mbuf, dpdk_interface_t * interface, dpdk_nat_ip_t * natIP)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( interface->mempool[0]);

	if( resp_m_buf)
	{
		if( natIP)
		{
			dpe__encode_arp_response( mbuf, resp_m_buf, natIP->MAC, natIP->ip);
		}
		else
		{
			dpe__encode_arp_response( mbuf, resp_m_buf, interface->MacAddr, interface->IPv4);
		}
		
		return resp_m_buf;
	}
	
	return NULL;
}


struct rte_mbuf * dpe__create_gtp_echo_response( struct rte_mbuf * mbuf, dpdk_interface_t * interface)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( interface->mempool[0]);
	
	// printf( "gtp request info  pkt_len=%u data_len=%u\n", 
		// mbuf->pkt_len, 
		// mbuf->data_len	
	// );
	
	if( resp_m_buf)
	{
		struct rte_ether_hdr * 	request_icmp_eth_hdr 	= rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);
		struct rte_ether_hdr * 	response_icmp_eth_hdr 	= rte_pktmbuf_mtod( resp_m_buf, struct rte_ether_hdr *);

		// dpe__print_mac( 0, &request_icmp_eth_hdr->src_addr);
		// dpe__print_mac( 0, &request_icmp_eth_hdr->dst_addr);

		memcpy( &response_icmp_eth_hdr->src_addr, interface->MacAddr, 6);
		//rte_ether_addr_copy( &response_icmp_eth_hdr->dst_addr, &request_icmp_eth_hdr->src_addr);
		rte_ether_addr_copy( &request_icmp_eth_hdr->src_addr, &response_icmp_eth_hdr->dst_addr);


		response_icmp_eth_hdr->ether_type = request_icmp_eth_hdr->ether_type;

		

		unsigned char * p_resp = rte_pktmbuf_mtod( resp_m_buf, unsigned char *);
		struct rte_ipv4_hdr * response_ip_hdr = (struct rte_ipv4_hdr *)&p_resp[14];

		unsigned char * p_req = rte_pktmbuf_mtod( mbuf, unsigned char *);
		struct rte_ipv4_hdr * request_ip_hdr = (struct rte_ipv4_hdr *)&p_req[14];


		response_ip_hdr->version_ihl 		= 0x45;
		response_ip_hdr->type_of_service 	= 0x00;
		response_ip_hdr->total_length 		= request_ip_hdr->total_length;

		response_ip_hdr->src_addr 			= request_ip_hdr->dst_addr;
		response_ip_hdr->dst_addr 			= request_ip_hdr->src_addr;
		response_ip_hdr->packet_id 			= request_ip_hdr->packet_id + 1;
		response_ip_hdr->fragment_offset 	= 0;
		response_ip_hdr->time_to_live 		= 64;
		response_ip_hdr->next_proto_id 		= 17;
		response_ip_hdr->hdr_checksum		= 0;
		
		response_ip_hdr->hdr_checksum 		= rte_ipv4_cksum( response_ip_hdr);
		
		
		char * res_buffer 	= rte_pktmbuf_mtod( resp_m_buf, char *);
		
		struct rte_udp_hdr  * udp  			= (struct rte_udp_hdr *)&res_buffer[14 + 20];
		
		udp->src_port						= rte_cpu_to_be_16( 2152);
		udp->dst_port						= rte_cpu_to_be_16( 2152);
		udp->dgram_len						= rte_cpu_to_be_16( 8 + 12);

		char * gtp_request 					= rte_pktmbuf_mtod( mbuf, char *);
		
		memcpy( &res_buffer[42], &gtp_request[42], 12);
		
		res_buffer[43] = 0x02;


		udp->dgram_cksum					= 0;
		udp->dgram_cksum					= rte_ipv4_udptcp_cksum( response_ip_hdr, udp);
		
		//printf( "cksum=%u %d\n", udp->dgram_cksum, __LINE__);


		
		resp_m_buf->pkt_len  = 54;
		resp_m_buf->data_len = 54;


		// resp_m_buf->pkt_len  = 60;
		// resp_m_buf->data_len = 60;
		
		
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtp_echo_response_28102024__01.pcap");
		// dpdk_pkt__write_pcap( dumper, resp_m_buf);
		// dpdk_pkt__close_pcap( dumper);
		
		
		return resp_m_buf;
	}
	
	return NULL;
}

struct rte_mbuf * dpe__create_icmp( struct rte_mbuf * mbuf, dpdk_interface_t * interface)
{
	struct rte_mbuf * resp_m_buf = rte_pktmbuf_alloc( interface->mempool[0]);

	if( resp_m_buf)
	{
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmp_response_12042024_req.pcap");
		// dpdk_pkt__write_pcap( dumper, mbuf);
		// dpdk_pkt__close_pcap( dumper);
		// exit(0);

		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmp_response_12042024.pcap");
		// dpdk_pkt__write_pcap( dumper, mbuf);

		//printf( "received icmp request on ip=%u\n", interface->IPv4);
		dpe__encode_icmp_response( mbuf, resp_m_buf, interface->MacAddr, interface->IPv4);		
		
		
		// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./icmp_response_12042024.pcap");
		// dpdk_pkt__write_pcap( dumper, mbuf);
		// dpdk_pkt__write_pcap( dumper, resp_m_buf);
		// dpdk_pkt__close_pcap( dumper);
		// exit(0);
	
		
		return resp_m_buf;
	}
	
	return NULL;
}


int dpe__swap_packet_info( struct rte_mbuf * mbuf, dpdk_interface_t * interface)
{
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);

	int bDSTMacIsInterfaceMac = 1;
	int i = 0;
	while( i < 6)
	{
		if( eth->dst_addr.addr_bytes[i] != interface->MacAddr[i])
		{
			bDSTMacIsInterfaceMac = 0;
			break;
		}
		i++;
	}

	// if(eth->src_addr.addr_bytes[0] == 0x55 && eth->src_addr.addr_bytes[5] == 0x55)
	// {
		// bDSTMacIsInterfaceMac = 0;
	// }
	
	// printf("1.bDSTMacIsInterfaceMac=%d  %02X:%02X:%02X:%02X:%02X:%02X \n", bDSTMacIsInterfaceMac, 
		// eth->dst_addr.addr_bytes[0] & 0xFF, eth->dst_addr.addr_bytes[1] & 0xFF, eth->dst_addr.addr_bytes[2] & 0xFF,
		// eth->dst_addr.addr_bytes[3] & 0xFF, eth->dst_addr.addr_bytes[4] & 0xFF, eth->dst_addr.addr_bytes[5] & 0xFF);

	
	if( bDSTMacIsInterfaceMac == 1)
	{
		//This is Not required, if packet passed this port macaddress( interface->MacAddr), that is enough
		/*
		if( interface->MatchSrcMac == 1)
		{
			i = 0;
			while( i < 6)
			{
				if( eth->src_addr.addr_bytes[i] != interface->SrcMac[i])
				{
					bDSTMacIsInterfaceMac = 0;
					break;
				}
				i++;
			}
			
			// printf("2.bDSTMacIsInterfaceMac=%d  %02X:%02X:%02X:%02X:%02X:%02X %02X:%02X:%02X:%02X:%02X:%02X\n", bDSTMacIsInterfaceMac, 
				// eth->src_addr.addr_bytes[0] & 0xFF, eth->src_addr.addr_bytes[1] & 0xFF, eth->src_addr.addr_bytes[2] & 0xFF,
				// eth->src_addr.addr_bytes[3] & 0xFF, eth->src_addr.addr_bytes[4] & 0xFF, eth->src_addr.addr_bytes[5] & 0xFF,
				// interface->SrcMac[0] & 0xFF, interface->SrcMac[1] & 0xFF, interface->SrcMac[2] & 0xFF,
				// interface->SrcMac[3] & 0xFF, interface->SrcMac[4] & 0xFF, interface->SrcMac[5] & 0xFF
				// );
				
		}
		*/

		if( bDSTMacIsInterfaceMac == 1)
		{
			uint32_t ipv4_src_addr = 0;
			uint16_t ipv4_src_port = 0;
	
			if( eth->ether_type == 8)
			{
				unsigned char * p = rte_pktmbuf_mtod( mbuf, unsigned char *);
				
				struct rte_ipv4_hdr * ip_hdr = (struct rte_ipv4_hdr *)&p[14];
				
				ipv4_src_addr			= ip_hdr->src_addr;
				ip_hdr->src_addr 		= ip_hdr->dst_addr;
				ip_hdr->dst_addr 		= ipv4_src_addr;
				
				ip_hdr->hdr_checksum 	= 0;
				ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
				
				//swap port
				if( ip_hdr->next_proto_id == 17)
				{
					//UDP
					struct rte_udp_hdr * udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
					
					ipv4_src_port				= udp->src_port;
					udp->src_port				= udp->dst_port;
					udp->dst_port				= ipv4_src_port;
					udp->dgram_cksum			= 0;
					
					udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);
				}
				else if(ip_hdr->next_proto_id == 6)
				{
					//TCP
					struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
					
					ipv4_src_port				= tcp->src_port;
					tcp->src_port				= tcp->dst_port;
					tcp->dst_port				= ipv4_src_port;
					tcp->cksum					= 0;
					
					tcp->cksum					= rte_ipv4_udptcp_cksum( ip_hdr, tcp);
				}
				else if( ip_hdr->next_proto_id == 2)
				{
					return 0;
				}
			}
			
			i = 0;
			while( i < 6)
			{
				eth->dst_addr.addr_bytes[i] = eth->src_addr.addr_bytes[i];
				eth->src_addr.addr_bytes[i] = interface->MacAddr[i];
				i++;
			}
			
			return 1;
		}
	}
	
	return 0;
}

int dpe__isarpformyip( struct rte_mbuf * mbuf_req, uint32_t myip, dpdk_interface_t * interface, dpdk_nat_ip_t * natIP)
{
	struct rte_ether_hdr * 	request_eth = rte_pktmbuf_mtod( mbuf_req, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	request_arp = (struct rte_arp_hdr *)&request_eth[1];
	
	//printf( "op_code=%d|%d  tip=%u myip=%u\n", request_arp->arp_opcode, htons(1), request_arp->arp_data.arp_tip, myip);

	if( natIP)
	{
		if( request_arp->arp_opcode == 512 && request_arp->arp_data.arp_tip == natIP->ip  /*(Reply to myRequest)*/)
		{
			interface->arp_response_received++;
			
			if( request_arp->arp_data.arp_sip == interface->DestIP)
			{
				memcpy( interface->DstMac, request_arp->arp_data.arp_sha.addr_bytes, 6);
			}
			else
			{
				
				
				int i = 0;
				int bFound = 0;
				while( i < __dpe->arp_table_row_count)
				{
					if( __dpe->arp_table[i].ip == request_arp->arp_data.arp_sip)
					{
						memcpy( __dpe->arp_table[i].Mac, request_arp->arp_data.arp_sha.addr_bytes, 6);
						bFound = 1;
						
						dpe__log( LOG_LEVEL_CRITICAL, __FILE__, __LINE__, "ARP Response Received, Updating MAC=%02X:%02X:%02X:%02X:%02X:%02X for IP=%u.%u.%u.%u|%u", 
							__dpe->arp_table[i].Mac[0] & 0xFF, __dpe->arp_table[i].Mac[1] & 0xFF, __dpe->arp_table[i].Mac[2] & 0xFF, 
							__dpe->arp_table[i].Mac[3] & 0xFF, __dpe->arp_table[i].Mac[4] & 0xFF, __dpe->arp_table[i].Mac[5] & 0xFF, 
							(__dpe->arp_table[i].ip) & 0xFF, (__dpe->arp_table[i].ip >> 8) & 0xFF,
							(__dpe->arp_table[i].ip >> 16) & 0xFF,  (__dpe->arp_table[i].ip >> 24) & 0xFF, 
							__dpe->arp_table[i].ip);
						
						//printf("Updated MAC=%02X to IP=%u\n", __dpe->arp_table[i].Mac[5] & 0xFF, __dpe->arp_table[i].ip);
						break;
					}
					i++;
				}
				
				if( bFound == 0 && __dpe->arp_table_row_count < 100)
				{
					__dpe->arp_table[__dpe->arp_table_row_count].ip = request_arp->arp_data.arp_sip;
					memcpy( __dpe->arp_table[__dpe->arp_table_row_count].Mac, request_arp->arp_data.arp_sha.addr_bytes, 6);
					
					// printf("Added MAC=%02X to IP=%u\n", 
						// __dpe->arp_table[__dpe->arp_table_row_count].Mac[5] & 0xFF, __dpe->arp_table[__dpe->arp_table_row_count].ip);

						dpe__log( LOG_LEVEL_CRITICAL, __FILE__, __LINE__, "ARP Response Received, Adding MAC=%02X:%02X:%02X:%02X:%02X:%02X for IP=%u.%u.%u.%u|%u", 
							__dpe->arp_table[i].Mac[0] & 0xFF, __dpe->arp_table[i].Mac[1] & 0xFF, __dpe->arp_table[i].Mac[2] & 0xFF, 
							__dpe->arp_table[i].Mac[3] & 0xFF, __dpe->arp_table[i].Mac[4] & 0xFF, __dpe->arp_table[i].Mac[5] & 0xFF, 
							(__dpe->arp_table[i].ip) & 0xFF, (__dpe->arp_table[i].ip >> 8) & 0xFF,
							(__dpe->arp_table[i].ip >> 16) & 0xFF,  (__dpe->arp_table[i].ip >> 24) & 0xFF, 						
							__dpe->arp_table[i].ip);
						
					__dpe->arp_table_row_count++;
				}
				
				
			}
		}
		
		if( request_arp->arp_opcode == 256 && request_arp->arp_data.arp_tip == natIP->ip)
		{
			//Request
			return 1;
		}
	}


	//->SrcMac
	if( request_arp->arp_opcode == 512 && request_arp->arp_data.arp_tip == myip /*(Reply to myRequest)*/)
	{
		// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./arp_response1.pcap");
		// dpdk_pkt__write_pcap( dumper2, mbuf_req);
		// dpdk_pkt__close_pcap( dumper2);
		// exit(0);		
		
		// printf( "ARP Response %02X:%02X:%02X:%02X:%02X:%02X\n", 
			// request_arp->arp_data.arp_sha.addr_bytes[0] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[1] & 0xFF,
			// request_arp->arp_data.arp_sha.addr_bytes[2] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[3] & 0xFF,
			// request_arp->arp_data.arp_sha.addr_bytes[4] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[5] & 0xFF
		// );

		interface->arp_response_received++;

		if( request_arp->arp_data.arp_sip == interface->DestIP)
		{
			dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "ARP Response Received, MAC-Addreess %02X:%02X:%02X:%02X:%02X:%02X for IP=%u.%u.%u.%u|%u.%u.%u.%u", 
					request_arp->arp_data.arp_sha.addr_bytes[0] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[1] & 0xFF,
					request_arp->arp_data.arp_sha.addr_bytes[2] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[3] & 0xFF,
					request_arp->arp_data.arp_sha.addr_bytes[4] & 0xFF, request_arp->arp_data.arp_sha.addr_bytes[5] & 0xFF,
					interface->DestIP & 0xFF, (interface->DestIP >> 8) & 0xFF, (interface->DestIP >> 16) & 0xFF, (interface->DestIP >> 24) & 0xFF,
					request_arp->arp_data.arp_sip & 0xFF, (request_arp->arp_data.arp_sip >> 8) & 0xFF, 
					(request_arp->arp_data.arp_sip >> 16) & 0xFF, (request_arp->arp_data.arp_sip >> 24) & 0xFF
				);

			// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./arp_response1.pcap");
			// dpdk_pkt__write_pcap( dumper2, mbuf_req);
			// dpdk_pkt__close_pcap( dumper2);
			// exit(0);

			//Reply (Reply to myRequest)
			memcpy( interface->DstMac, request_arp->arp_data.arp_sha.addr_bytes, 6);		
		}
		else
		{
			// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./static_arp_response_12042024.pcap");
			// dpdk_pkt__write_pcap( dumper2, mbuf_req);
			// dpdk_pkt__close_pcap( dumper2);
			// exit(0);
			
			
			int i = 0;
			int bFound = 0;
			while( i < __dpe->arp_table_row_count)
			{
				if( __dpe->arp_table[i].ip == request_arp->arp_data.arp_sip)
				{
					memcpy( __dpe->arp_table[i].Mac, request_arp->arp_data.arp_sha.addr_bytes, 6);
					bFound = 1;
					
					dpe__log( LOG_LEVEL_CRITICAL, __FILE__, __LINE__, "ARP Response Received, Updating MAC=%02X:%02X:%02X:%02X:%02X:%02X for IP=%u.%u.%u.%u|%u", 
						__dpe->arp_table[i].Mac[0] & 0xFF, __dpe->arp_table[i].Mac[1] & 0xFF, __dpe->arp_table[i].Mac[2] & 0xFF, 
						__dpe->arp_table[i].Mac[3] & 0xFF, __dpe->arp_table[i].Mac[4] & 0xFF, __dpe->arp_table[i].Mac[5] & 0xFF, 
						(__dpe->arp_table[i].ip) & 0xFF, (__dpe->arp_table[i].ip >> 8) & 0xFF,
						(__dpe->arp_table[i].ip >> 16) & 0xFF,  (__dpe->arp_table[i].ip >> 24) & 0xFF, 
						__dpe->arp_table[i].ip);
					
					//printf("Updated MAC=%02X to IP=%u\n", __dpe->arp_table[i].Mac[5] & 0xFF, __dpe->arp_table[i].ip);
					break;
				}
				i++;
			}
			
			if( bFound == 0 && __dpe->arp_table_row_count < 100)
			{
				__dpe->arp_table[__dpe->arp_table_row_count].ip = request_arp->arp_data.arp_sip;
				memcpy( __dpe->arp_table[__dpe->arp_table_row_count].Mac, request_arp->arp_data.arp_sha.addr_bytes, 6);
				
				// printf("Added MAC=%02X to IP=%u\n", 
					// __dpe->arp_table[__dpe->arp_table_row_count].Mac[5] & 0xFF, __dpe->arp_table[__dpe->arp_table_row_count].ip);

					dpe__log( LOG_LEVEL_CRITICAL, __FILE__, __LINE__, "ARP Response Received, Adding MAC=%02X:%02X:%02X:%02X:%02X:%02X for IP=%u.%u.%u.%u|%u", 
						__dpe->arp_table[i].Mac[0] & 0xFF, __dpe->arp_table[i].Mac[1] & 0xFF, __dpe->arp_table[i].Mac[2] & 0xFF, 
						__dpe->arp_table[i].Mac[3] & 0xFF, __dpe->arp_table[i].Mac[4] & 0xFF, __dpe->arp_table[i].Mac[5] & 0xFF, 
						(__dpe->arp_table[i].ip) & 0xFF, (__dpe->arp_table[i].ip >> 8) & 0xFF,
						(__dpe->arp_table[i].ip >> 16) & 0xFF,  (__dpe->arp_table[i].ip >> 24) & 0xFF, 						
						__dpe->arp_table[i].ip);
					
				__dpe->arp_table_row_count++;
			}
		}
		
		return 0;
	}


	if( request_arp->arp_opcode == 256 && request_arp->arp_data.arp_tip == myip)
	{
		//Request
		return 1;
	}

	return 0;	
}

uint16_t dpe__process_received( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, struct rte_mbuf ** arp_mbufs, uint16_t * arp_cnt_out, int recv_worker_index)
{
	/*
	uint32_t subport;
	uint32_t pipe;
	uint32_t traffic_class;
	uint32_t queue;
	uint32_t color;
	
	rte_sched_port_pkt_write( conf->sched_port, rx_mbufs[i], subport, pipe, traffic_class, queue, (enum rte_color) color);	
	
	rte_delay_us(lcore_idle_hint);
	usleep(lcore_idle_hint);
	
	*/
	
	
	struct rte_ether_hdr * eth = NULL;
	uint16_t dropped_cnt = 0;
	
	int i = 0;
	int j = 0;
	while( i < recv_pkts)
	{
		rte_prefetch0( rte_pktmbuf_mtod( pkts[i], void *));
		eth = rte_pktmbuf_mtod( pkts[i], struct rte_ether_hdr *);
		
		switch( eth->ether_type)
		{
			case 8:		// 	RTE_ETHER_TYPE_IPV4:
				break;
			case 1544:	//	RTE_ETHER_TYPE_ARP:
				{
					if( dpe__isarpformyip( pkts[i], interface->IPv4, interface, NULL) == 1)
					{
						arp_mbufs[*arp_cnt_out] = dpe__create_arp( pkts[i], interface, NULL);
						if( arp_mbufs[*arp_cnt_out])
						{
							*arp_cnt_out++;						
						}
					}
				}
				break;
			case 56710:	//	RTE_ETHER_TYPE_IPV6:
				break;
			default:
				break;
		}
		
		if( dpe__recvPacketHandler > 0)
		{
			dpe__recvPacketHandler( 0, 0, 0, 0, pkts[i]->pkt_len);
		}
		
		dropped_mbufs[dropped_cnt] = pkts[i];
		dropped_cnt++;
		i++;
	}
	
	//printf("------------------------------------------\n");
	*dropped_cnt_out = dropped_cnt;
	return j;
}

uint16_t dpe__process_recv_and_send( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, struct rte_mbuf ** arp_mbufs, uint16_t * arp_cnt_out, int recv_worker_index)
{
	struct rte_ether_hdr * eth = NULL;
	uint16_t dropped_cnt = 0;
	
	int i = 0;
	int j = 0;
	
	while( i < recv_pkts)
	{
		rte_prefetch0(rte_pktmbuf_mtod( pkts[i], void *));
		eth = rte_pktmbuf_mtod( pkts[i], struct rte_ether_hdr *);
	
		// printf("%d %d   %d %d   %d %d   %d %d\n", 
			// eth->ether_type, htons(eth->ether_type), 
			// RTE_ETHER_TYPE_ARP, htons(RTE_ETHER_TYPE_ARP), 
			// RTE_ETHER_TYPE_IPV4, htons(RTE_ETHER_TYPE_IPV4),  
			// RTE_ETHER_TYPE_IPV6, htons(RTE_ETHER_TYPE_IPV6));
			

		switch( eth->ether_type)
		{
			case 8:		// RTE_ETHER_TYPE_IPV4:
				{
					if( dpe__swap_packet_info( pkts[i], interface) == 1)
					{
						processed_pkts[j] = pkts[i];
						j++;
					} 
					else 
					{
						dropped_mbufs[dropped_cnt] = pkts[i];
						dropped_cnt++;
					}
					
				}
				break;
			case 1544:	//RTE_ETHER_TYPE_ARP:
				{
					if( dpe__isarpformyip( pkts[i], interface->IPv4, interface, NULL) == 1)
					{
						arp_mbufs[*arp_cnt_out] = dpe__create_arp( pkts[i], interface, NULL);
						
						if( arp_mbufs[*arp_cnt_out])
						{
							*arp_cnt_out++;
						}
					}
					
					dropped_mbufs[dropped_cnt] = pkts[i];
					dropped_cnt++;					
				}
				break;
			case 56710:	//RTE_ETHER_TYPE_IPV6:
				{
					dropped_mbufs[dropped_cnt] = pkts[i];
					dropped_cnt++;					
				}
				break;				
			default:
				{
					dropped_mbufs[dropped_cnt] = pkts[i];
					dropped_cnt++;					
				}
				break;
		}
		
		i++;
	}
	
	*dropped_cnt_out = dropped_cnt;
	return j;
}


int cp__set_mac( uint8_t * session, uint8_t * mac);

int cp__get_tcinfo( uint8_t * sess, uint32_t * subport, uint16_t * pipe, uint32_t * traffic_class, uint32_t * queue, uint32_t rgid, int side);

void dpe__sched_port_pkt_write( dpdk_interface_t * interface, uint8_t * sess, struct rte_mbuf * pkt, uint32_t rgid, int side)
{
	uint32_t subport = 0; 
	uint16_t pipe = 0; 
	uint32_t traffic_class = 0; 
	uint32_t queue = 0;
	
	if( cp__get_tcinfo( sess, &subport, &pipe, &traffic_class, &queue, rgid, side) >= 0)
	{
		rte_sched_port_pkt_write( interface->sched_port, pkt, subport, pipe, traffic_class, queue, (enum rte_color)3);
	}
}


uint16_t dpe__process_forward( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, struct rte_mbuf ** arp_mbufs, uint16_t * arp_cnt_out, struct rte_mbuf ** gtp_mbufs, uint16_t * gtp_cnt_out, int recv_worker_index)
{
	struct rte_ether_hdr * eth = NULL;
	uint16_t dropped_cnt = 0;
	uint16_t arp_cnt = 0;
	uint16_t gtp_cnt = 0;

	int i = 0;
	int j = 0;
	
	struct rte_ipv4_hdr * root_ip_hdr = NULL;
	struct rte_udp_hdr * udp = NULL;
	struct rte_gtp_hdr * gtp = NULL;
	unsigned char * gtpbuff = NULL;
	unsigned char * sqnptr = NULL;
	unsigned char * payload = NULL;

	struct rte_ipv4_hdr * ue_ip_hdr  		= NULL;
	struct rte_ipv6_hdr * ue_ip_hdr6 		= NULL;
	struct rte_udp_hdr * ue_udp 			= NULL;
	struct rte_tcp_hdr * ue_tcp 			= NULL;
	
	unsigned char * ue_proto_packet 		= NULL;
	
	
	uint32_t ran_ip = 0;
	__uint128_t ranip6 = 0;
	uint8_t gtp_hdr_info = 0;
	uint8_t ver = 0;
	uint8_t pt = 0;
	uint8_t gtp_ext = 0;
	uint32_t teid = 0;
	uint8_t is_seqn_present = 0;
	uint16_t seqn_no = 0;
	uint8_t extlen = 0;
	int usedbytes = 0;
	
	uint8_t ue_ipv = 0; 
	uint8_t ue_proto = 0;
	uint32_t ue_src_ip = 0;
	uint32_t ue_dst_ip = 0;
	uint8_t  * ue_src_ip6 = NULL;
	uint8_t  * ue_dst_ip6 = NULL;
	uint16_t ue_src_port;
	uint16_t ue_dst_port;
	
	uint8_t ran_ipv = 4;
	int prependLen = 0;
	int gtpHasSQN = 0;
	int gtpHeaderLen = (gtpHasSQN == 1) ? 12 : 8;
	struct rte_ipv4_hdr * ip_hdr = NULL;
	struct rte_ipv6_hdr * ip6_hdr = NULL;
	int orig_pkt_len = 0;
	char * p = NULL;
	uint8_t * ranmac;
	uint8_t ranmac_temp[7];
	uint8_t * sess = NULL;
	
	uint8_t 	p_ran_ip_v 		= 0;
	uint32_t 	p_ran_ip_v4 	= 0;
	__uint128_t p_ran_ip_v6 	= 0;
	uint32_t 	p_ran_teid 		= 0;

	uint8_t gtp_is_seqn_present = 0;
	uint8_t qfi = 1;
	uint8_t * icmp_buf = NULL;
	
	unsigned char * dhcp_packet = NULL;
	struct dhcp_packet * dpacket = NULL;
	int dhcp_packet_len = 0;
	
	
	
	uint16_t isMultiMACInterface = (interface->natIPCount > 1 && interface->EnableNAPT == 1) ? 1 : 0;
	dpdk_nat_ip_t * natIPHead = NULL;
	uint16_t dropPkt = 0;

	int break_outer_switch = 0;
	
	while( i < recv_pkts)
	{
		ranmac = NULL;

		rte_prefetch0(rte_pktmbuf_mtod( pkts[i], void *));
		eth = rte_pktmbuf_mtod( pkts[i], struct rte_ether_hdr *);

		// int bDSTMacIsInterfaceMac = 1;
		// int iw = 0;
		// while( iw < 6)
		// {
			// if( eth->dst_addr.addr_bytes[iw] != interface->MacAddr[iw])
			// {
				// bDSTMacIsInterfaceMac = 0;
				// break;
			// }
			// iw++;
		// }
		
		// if( bDSTMacIsInterfaceMac == 0)
		// {
			// //printf("dropping here %d\n", __LINE__);
			// dropped_mbufs[dropped_cnt] = pkts[i];
			// dropped_cnt++;
			// i++;
			// continue;
		// }
		
		/*if( eth->dst_addr.addr_bytes[0] != interface->MacAddr[0] && eth->dst_addr.addr_bytes[1] != interface->MacAddr[1] && eth->dst_addr.addr_bytes[2] != interface->MacAddr[2] && eth->dst_addr.addr_bytes[3] != interface->MacAddr[3] && eth->dst_addr.addr_bytes[4] != interface->MacAddr[4] && eth->dst_addr.addr_bytes[5] != interface->MacAddr[5])
		{
			//printf("dropping here %d\n", __LINE__);
			dropped_mbufs[dropped_cnt] = pkts[i];
			dropped_cnt++;
			i++;
			continue;
		}*/
		
		// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Packet from MAC %02X:%02X:%02X:%02X:%02X:%02X  %02X:%02X:%02X:%02X:%02X:%02X  %d", 
			// eth->src_addr.addr_bytes[0], eth->src_addr.addr_bytes[1], eth->src_addr.addr_bytes[2],
			// eth->src_addr.addr_bytes[3], eth->src_addr.addr_bytes[4], eth->src_addr.addr_bytes[5],
			
			// eth->dst_addr.addr_bytes[0], eth->dst_addr.addr_bytes[1], eth->dst_addr.addr_bytes[2],
			// eth->dst_addr.addr_bytes[3], eth->dst_addr.addr_bytes[4], eth->dst_addr.addr_bytes[5],
			
			// eth->ether_type
		// );
		
		
		
		//dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "rt=%d", eth->ether_type);
		
		// if( eth->ether_type != 9728 && eth->ether_type != 51969 && eth->ether_type != 8704 && eth->ether_type != 19456)
		// {
			// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "rt=%d exiting", eth->ether_type);
			
			// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./p_9729.pcap");
			// dpdk_pkt__write_pcap( dumper2, pkts[j]);
			// dpdk_pkt__close_pcap( dumper2);
			// exit(0);	
		// }
		
		
		// if( __dpe->pdumper)
		// {
			// if( interface->ProcessType == PROCESS_TYPE__GTP_Gi && __dpe->enable_pcap_on__gtp_gi == 1)
			// {
				// dpdk_pkt__write_pcap( __dpe->pdumper, pkts[i]);
			// }
			// else if( interface->ProcessType == PROCESS_TYPE__Gi_GTP && __dpe->enable_pcap_on__gi_gtp == 1)
			// {
				// dpdk_pkt__write_pcap( __dpe->pdumper, pkts[i]);
			// }
			// else if( interface->ProcessType == PROCESS_TYPE__Gi_Gi_INGRESS && __dpe->enable_pcap_on__gi_gi == 1)
			// {
				// dpdk_pkt__write_pcap( __dpe->pdumper, pkts[i]);
			// }
			// else if( interface->ProcessType == PROCESS_TYPE__Gi_Gi_EGRESS && __dpe->enable_pcap_on__gi_gi == 1)
			// {
				// dpdk_pkt__write_pcap( __dpe->pdumper, pkts[i]);
			// }
		// }
		
		
		// if( interface->ProcessType == 2)
		// {
			// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Packet On ProcessType=%d - ether_type=%d on MAC %02X:%02X:%02X:%02X:%02X:%02X  -- FROM %02X:%02X:%02X:%02X:%02X:%02X   TO %02X:%02X:%02X:%02X:%02X:%02X", 
				// interface->ProcessType, eth->ether_type, 
				// interface->MacAddr[0] & 0xFF, interface->MacAddr[1] & 0xFF, interface->MacAddr[2] & 0xFF, 
				// interface->MacAddr[3] & 0xFF, interface->MacAddr[4] & 0xFF, interface->MacAddr[5] & 0xFF,
				// eth->src_addr.addr_bytes[0] & 0xFF, eth->src_addr.addr_bytes[1] & 0xFF, eth->src_addr.addr_bytes[2] & 0xFF,
				// eth->src_addr.addr_bytes[3] & 0xFF, eth->src_addr.addr_bytes[4] & 0xFF, eth->src_addr.addr_bytes[5] & 0xFF,
				// eth->dst_addr.addr_bytes[0] & 0xFF, eth->dst_addr.addr_bytes[1] & 0xFF, eth->dst_addr.addr_bytes[2] & 0xFF,
				// eth->dst_addr.addr_bytes[3] & 0xFF, eth->dst_addr.addr_bytes[4] & 0xFF, eth->dst_addr.addr_bytes[5] & 0xFF
			// );
		// }
		
		
		if( isMultiMACInterface == 1)
		{
			dropPkt = 1;
			
			if( eth->ether_type == 56710)
			{
				dropPkt = 0;
			}
			
			if(eth->dst_addr.addr_bytes[0] == 0xFF && eth->dst_addr.addr_bytes[1] == 0xFF && eth->dst_addr.addr_bytes[2] == 0xFF && eth->dst_addr.addr_bytes[3] == 0xFF && eth->dst_addr.addr_bytes[4] == 0xFF && eth->dst_addr.addr_bytes[5] == 0xFF)
			{
				dropPkt = 0;
			}
			else
			{
				natIPHead = interface->natIPHead;
				
				while( natIPHead)
				{
					if(eth->dst_addr.addr_bytes[0] == natIPHead->MAC[0] && eth->dst_addr.addr_bytes[1] == natIPHead->MAC[1] && eth->dst_addr.addr_bytes[2] == natIPHead->MAC[2] && eth->dst_addr.addr_bytes[3] == natIPHead->MAC[3] && eth->dst_addr.addr_bytes[4] == natIPHead->MAC[4] && eth->dst_addr.addr_bytes[5] == natIPHead->MAC[5])
					{
						dropPkt = 0;
						break;
					}					
					natIPHead = natIPHead->Next;
				}
			}
			
			if( dropPkt == 1)
			{
				dropped_mbufs[dropped_cnt] = pkts[i];
				dropped_cnt++;
				i++;
				continue;
			}
		}
		else
		{
			if( 
					( eth->dst_addr.addr_bytes[0] != interface->MacAddr[0] && eth->dst_addr.addr_bytes[1] != interface->MacAddr[1] && eth->dst_addr.addr_bytes[2] != interface->MacAddr[2] && eth->dst_addr.addr_bytes[3] != interface->MacAddr[3] && eth->dst_addr.addr_bytes[4] != interface->MacAddr[4] && eth->dst_addr.addr_bytes[5] != interface->MacAddr[5]) 
						&& 
					(eth->dst_addr.addr_bytes[0] != 0xFF && eth->dst_addr.addr_bytes[1] != 0xFF && eth->dst_addr.addr_bytes[2] != 0xFF && eth->dst_addr.addr_bytes[3] != 0xFF && eth->dst_addr.addr_bytes[4] != 0xFF && eth->dst_addr.addr_bytes[5] != 0xFF)
						&&
					eth->ether_type != 56710
				)
			{
				//printf("dropping here %d\n", __LINE__);
				dropped_mbufs[dropped_cnt] = pkts[i];
				dropped_cnt++;
				i++;
				continue;   
			}
		}
		
		
		// if( eth->ether_type == 56710)
		// {
			// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Packet - ether_type = %d   FROM %02X:%02X:%02X:%02X:%02X:%02X   TO %02X:%02X:%02X:%02X:%02X:%02X", 
					// eth->ether_type, 
					// eth->src_addr.addr_bytes[0] & 0xFF, eth->src_addr.addr_bytes[1] & 0xFF, eth->src_addr.addr_bytes[2] & 0xFF,
					// eth->src_addr.addr_bytes[3] & 0xFF, eth->src_addr.addr_bytes[4] & 0xFF, eth->src_addr.addr_bytes[5] & 0xFF,
					// eth->dst_addr.addr_bytes[0] & 0xFF, eth->dst_addr.addr_bytes[1] & 0xFF, eth->dst_addr.addr_bytes[2] & 0xFF,
					// eth->dst_addr.addr_bytes[3] & 0xFF, eth->dst_addr.addr_bytes[4] & 0xFF, eth->dst_addr.addr_bytes[5] & 0xFF
				// );
			
			// //if( (eth->src_addr.addr_bytes[5] & 0xFF) == 1)
			// // {
				// // pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./icmpv6_received_msg_13052024_1.pcap");
				// // dpdk_pkt__write_pcap( dumper2, pkts[j]);
				// // dpdk_pkt__close_pcap( dumper2);
				// // exit(0);			
			// // }
				
		// }
		
		
		
		// else
		// {
			// printf("passed %d\n", __LINE__);
		// }
		
		// dpe__print_buffer( "  DST-MAC", eth->dst_addr.addr_bytes, 6, 0, 0);
		// dpe__print_buffer( "   MY-MAC", interface->MacAddr, 6, 0, 1);
		// printf("ether_type=%u  PT=%u %u  IPv4=%u\n", eth->ether_type, interface->ProcessType, PROCESS_TYPE__GTP_Gi, interface->IPv4);
		
		
		
		// if( interface->ProcessType == 2)
		// {
			// printf( "Received Packet On ProcessType=%d - ether_type=%d on MAC %02X:%02X:%02X:%02X:%02X:%02X  -- FROM %02X:%02X:%02X:%02X:%02X:%02X   TO %02X:%02X:%02X:%02X:%02X:%02X\n", 
				// interface->ProcessType, eth->ether_type, 
				// interface->MacAddr[0] & 0xFF, interface->MacAddr[1] & 0xFF, interface->MacAddr[2] & 0xFF, 
				// interface->MacAddr[3] & 0xFF, interface->MacAddr[4] & 0xFF, interface->MacAddr[5] & 0xFF,
				// eth->src_addr.addr_bytes[0] & 0xFF, eth->src_addr.addr_bytes[1] & 0xFF, eth->src_addr.addr_bytes[2] & 0xFF,
				// eth->src_addr.addr_bytes[3] & 0xFF, eth->src_addr.addr_bytes[4] & 0xFF, eth->src_addr.addr_bytes[5] & 0xFF,
				// eth->dst_addr.addr_bytes[0] & 0xFF, eth->dst_addr.addr_bytes[1] & 0xFF, eth->dst_addr.addr_bytes[2] & 0xFF,
				// eth->dst_addr.addr_bytes[3] & 0xFF, eth->dst_addr.addr_bytes[4] & 0xFF, eth->dst_addr.addr_bytes[5] & 0xFF
			// );
		// }
		
		
		
		switch( eth->ether_type)
		{
			case 8:		// RTE_ETHER_TYPE_IPV4:
				{
					if( interface->ProcessType == PROCESS_TYPE__GTP_Gi)
					{
						root_ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));


						if( root_ip_hdr->next_proto_id == IPPROTO_ICMP && root_ip_hdr->dst_addr == interface->IPv4)
						{
							icmp_buf = ((uint8_t*)root_ip_hdr + 20);
							
							if(icmp_buf)
							{
								if( icmp_buf[0] == 8)
								{
									arp_mbufs[arp_cnt] = dpe__create_icmp( pkts[i], interface);
									if( arp_mbufs[arp_cnt])
									{
										arp_cnt++;
									}
								}
							}
							
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;				// break switch case, not outer loop
						}
						
						if( root_ip_hdr->next_proto_id != IPPROTO_UDP /*|| root_ip_hdr->dst_addr != interface->IPv4*/)
						{
							//printf("dropping here %d\n", __LINE__);
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;				// break switch case, not outer loop
						}


						ran_ip = root_ip_hdr->src_addr;

						udp = (struct rte_udp_hdr *)((unsigned char *)root_ip_hdr + 20);
						gtp = (struct rte_gtp_hdr *)((unsigned char *)udp + sizeof(struct rte_udp_hdr));







						/**/
						if( root_ip_hdr->next_proto_id == IPPROTO_UDP )
						{
							ue_src_port = udp->src_port;
							ue_dst_port = udp->dst_port;
							
							if( ue_src_port == 17152 && ue_dst_port == 17408)
							{
								dpacket =  (struct dhcp_packet *)((unsigned char *)udp + sizeof(struct rte_udp_hdr));
								dhcp_packet_len = (pkts[i]->pkt_len - ( 14 + 20 + 8));								
								
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received DHCP Message for ClientIP=%u interfaceIP=%u pktlen=%u op=%d", *(uint32_t*)&dpacket->ciaddr, interface->IPv4, dhcp_packet_len, dpacket->op);
							

								interface->DHCPDiscoverAckReceived = 1;
								interface->DHCPInformAckReceived = 1;
							
								dropped_mbufs[dropped_cnt] = pkts[i];
								dropped_cnt++;
								break;				// break switch case, not outer loop
							}
						}
						/**/


						
						
						gtp_hdr_info 	= gtp->gtp_hdr_info;
						ver 			= gtp_hdr_info >> 5;
						pt 				= (gtp_hdr_info & 0x10) >> 4;
						gtp_ext 		= (gtp_hdr_info & 0x04);						
						
						
						if( pt != 1 || ver != 1)
						{
							//printf("dropping here  pt=%u  ver=%u  msg_type=%u   %d\n", pt, ver, gtp->msg_type, __LINE__);
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;
						}
						
						break_outer_switch = 0;
						
						switch( gtp->msg_type)
						{
							case 1:		// ECHO REQUEST
								{
									//gtp_mbufs[gtp_cnt] = dpe__create_icmp( pkts[i], interface);
									gtp_mbufs[gtp_cnt] = dpe__create_gtp_echo_response( pkts[i], interface);
									if( gtp_mbufs[gtp_cnt])
									{
										gtp_cnt++;
									}
									
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break_outer_switch = 1;
								}
								break;
							case 2:		// ECHO RESPONSE
								{
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break_outer_switch = 1;
								}
								break;
							case 26:	// Error Indication
								{
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break_outer_switch = 1;
								}
								break;
							case 31:	// Supported Extension Headers Notification
								{
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break_outer_switch = 1;
								}
								break;
							case 254:	// END MARKER
								{
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break_outer_switch = 1;
								}
								break;
							case 255:
								break;
							default:
								break;
						}
						
						if( break_outer_switch == 1)
						{
							break;
						}
						

						/*
						printf( "Received Packet On ProcessType=%d - ether_type=%d on MAC %02X:%02X:%02X:%02X:%02X:%02X  -- FROM %02X:%02X:%02X:%02X:%02X:%02X   TO %02X:%02X:%02X:%02X:%02X:%02X  procol-type=%u  sentto=%u  upf-gtp-ip=%u \n", 
							interface->ProcessType, eth->ether_type, 
							interface->MacAddr[0] & 0xFF, interface->MacAddr[1] & 0xFF, interface->MacAddr[2] & 0xFF, 
							interface->MacAddr[3] & 0xFF, interface->MacAddr[4] & 0xFF, interface->MacAddr[5] & 0xFF,
							eth->src_addr.addr_bytes[0] & 0xFF, eth->src_addr.addr_bytes[1] & 0xFF, eth->src_addr.addr_bytes[2] & 0xFF,
							eth->src_addr.addr_bytes[3] & 0xFF, eth->src_addr.addr_bytes[4] & 0xFF, eth->src_addr.addr_bytes[5] & 0xFF,
							eth->dst_addr.addr_bytes[0] & 0xFF, eth->dst_addr.addr_bytes[1] & 0xFF, eth->dst_addr.addr_bytes[2] & 0xFF,
							eth->dst_addr.addr_bytes[3] & 0xFF, eth->dst_addr.addr_bytes[4] & 0xFF, eth->dst_addr.addr_bytes[5] & 0xFF,
							root_ip_hdr->next_proto_id, root_ip_hdr->dst_addr, interface->IPv4
						);
						*/


						
						 // pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtp_packet_06062024_1.pcap");
						 // dpdk_pkt__write_pcap( dumper, pkts[i]);
						 // dpdk_pkt__close_pcap( dumper);						
						 // exit(0);
						
						
						gtpbuff = ((unsigned char *)gtp) + 4;
						teid = (uint32_t) (( gtpbuff[0] << 24) & 0xFF000000) + ((gtpbuff[1] << 16) & 0x00FF0000) + ((gtpbuff[2] << 8) & 0x0000FF00) + (gtpbuff[3] & 0x000000FF);
						gtp_is_seqn_present = ((gtp_hdr_info & 0x2) >> 1);
						
						
						
						//printf("SkipAPICalls=%u  %s|%d\n", __dpe->SkipAPICalls, __FILE__,__LINE__);
						//printf("teid=%u  %s|%d\n", teid, __FILE__,__LINE__);
						
						if( __dpe->SkipAPICalls == 0) 
						{
							if( cp__teid_allowed_v4( ran_ip, teid, &sess, eth->src_addr.addr_bytes) == 0)
							{
								if( __dpe->EnableLogs == 1)
								{
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "GTP Packet received from RAN[%u.%u.%u.%u] with TEID=%u and dropped", 
										ran_ip & 0xFF, (ran_ip >> 8) & 0xFF, (ran_ip >> 16) & 0xFF, (ran_ip >> 24) & 0xFF, teid);
								}
								
								//printf("dropping here  teid=%d   %d\n", teid, __LINE__);
								dropped_mbufs[dropped_cnt] = pkts[i];
								dropped_cnt++;
								break;
							}
							else if( __dpe->EnableLogs == 1)
							{
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "GTP Packet received from RAN[%u.%u.%u.%u] with TEID=%u and allowed", 
									ran_ip & 0xFF, (ran_ip >> 8) & 0xFF, (ran_ip >> 16) & 0xFF, (ran_ip >> 24) & 0xFF, teid);
							}
						}
						
						// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtp_packet_06062024_01.pcap");
						// dpdk_pkt__write_pcap( dumper, pkts[i]);
						// dpdk_pkt__close_pcap( dumper);
						// exit(0); 
 
						
						is_seqn_present = ((gtp_hdr_info & 0x2) >> 1);
						if( is_seqn_present == 1) 
						{
							sqnptr = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr));
							seqn_no = (sqnptr[0] << 8) + sqnptr[1];
						}
						
						
						/*
							14 ethernet
							20 ipv4
							8 udp
							8 gtp
							//(14 + 20 + 8 + 8);
						*/
						usedbytes = 50;
						//printf("is_seqn_present=%d, gtp_ext =%d\n ",is_seqn_present,gtp_ext);
						if( is_seqn_present == 1 || gtp_ext)
						{
							usedbytes = 58;
							payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4 + 4 + (extlen * 4));
							
							// if( is_seqn_present == 1 && gtp_ext)
							// {
								// usedbytes = 58;
							// }
							// else 
							// {
								// usedbytes = 54;	//sqn
							// }
							
							// if(!gtp_ext) 
							// {
								// if( is_seqn_present == 1) {
									// payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4 + 4);
								// } else {
									// payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4);
								// }
								
								// printf("1.\n");		
							// }
							// else
							// {
								// uint8_t extlen = *(uint8_t*)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4);

								// //TODO: loop and check for no extension and then assign below pointer
								
								// if( is_seqn_present == 1) {
									// payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4 + 4 + (extlen * 4));
								// } else {
									// payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4 + (extlen * 4));
								// }
								
								// printf("2.\n");
							// }
						} 
						else 
						{
							payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr));
						}
						
						
						if(!payload)
						{
							//printf("dropping here %d\n", __LINE__);
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;
						}
						
						
						if(((payload[0] & 0xF0) >> 4) == 4)
						{
							ue_ip_hdr 		= (struct rte_ipv4_hdr *)payload;

							ue_ipv 			= 4; 
							ue_proto 		= ue_ip_hdr->next_proto_id;
							ue_src_ip 		= ue_ip_hdr->src_addr;
							ue_dst_ip 		= ue_ip_hdr->dst_addr;
							
							//printf( "ue_src_ip=%u\n", ue_src_ip);
							// if( __dpe->match_ueip_pcap_exit == ue_src_ip)
							// {
								// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./gtp_packet_9999.pcap");
								// dpdk_pkt__write_pcap( dumper, pkts[i]);
								// dpdk_pkt__close_pcap( dumper);						
								// exit(0);
							// }
							
							ue_proto_packet = payload + sizeof(struct rte_ipv4_hdr);							
						}
						else
						{
							ue_ip_hdr6 			= (struct rte_ipv6_hdr *)payload;
							
							ue_ipv 				= 6;
							ue_proto 			= ue_ip_hdr6->proto;
							ue_src_ip6 			= ue_ip_hdr6->src_addr;
							ue_dst_ip6 			= ue_ip_hdr6->dst_addr;
							
							ue_proto_packet = payload + sizeof(struct rte_ipv6_hdr);
						}
						
						if( ue_proto == IPPROTO_UDP)
						{
							ue_udp = (struct rte_udp_hdr *)ue_proto_packet;
							
							ue_src_port = ue_udp->src_port;
							ue_dst_port = ue_udp->dst_port;
						}
						else if( ue_proto == IPPROTO_TCP)
						{
							ue_tcp = (struct rte_tcp_hdr *)ue_proto_packet;
							
							ue_src_port = ue_tcp->src_port;
							ue_dst_port = ue_tcp->dst_port;		
						}
						else if( ue_proto == 50)
						{
							ue_src_port = *(uint16_t*)&ue_proto_packet[0];
							ue_dst_port = *(uint16_t*)&ue_proto_packet[2];
						}
						else
						{
							ue_src_port = *(uint16_t*)&ue_proto_packet[0];
							ue_dst_port = *(uint16_t*)&ue_proto_packet[2];
						}
						
						
						
						rte_pktmbuf_adj( pkts[i], (usedbytes-14));
						
						eth = rte_pktmbuf_mtod( pkts[i], struct rte_ether_hdr *);
						eth->ether_type = 8;											//IPv4



						if( __dpe->SkipAPICalls == 0)
						{
							if( ue_ipv == 4)
							{
								p = rte_pktmbuf_mtod( pkts[i], char *);

								if( cp__flow_allowed_sess_v4( sess, ue_dst_ip, ue_src_port, ue_dst_port, ue_proto, p, pkts[i]->pkt_len, PROCESS_TYPE__GTP_Gi, recv_worker_index) == 0)
								{
									if( __dpe->EnableLogs == 1)
									{
										dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "GTP Packet received from RAN[%u.%u.%u.%u]  ue_ip=%u.%u.%u.%u  dst_ip=%u.%u.%u.%u  src_port=%u  dst_port=%u  protocol=%u  with TEID=%u and dropped", 
											ran_ip & 0xFF, (ran_ip >> 8) & 0xFF, (ran_ip >> 16) & 0xFF, (ran_ip >> 24) & 0xFF, 
											ue_src_ip & 0xFF, (ue_src_ip >> 8) & 0xFF, (ue_src_ip >> 16) & 0xFF, (ue_src_ip >> 24) & 0xFF,
											ue_dst_ip & 0xFF, (ue_dst_ip >> 8) & 0xFF, (ue_dst_ip >> 16) & 0xFF, (ue_dst_ip >> 24) & 0xFF,
											ue_src_port, ue_dst_port, ue_proto,
											teid);
									}
									
									dropped_mbufs[dropped_cnt] = pkts[i];
									dropped_cnt++;
									break;
								} 
								else if( __dpe->EnableLogs == 1)
								{
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "GTP Packet received from RAN[%u.%u.%u.%u]  ue_ip=%u.%u.%u.%u  dst_ip=%u.%u.%u.%u  src_port=%u  dst_port=%u  protocol=%u  with TEID=%u and allowed", 
										ran_ip & 0xFF, (ran_ip >> 8) & 0xFF, (ran_ip >> 16) & 0xFF, (ran_ip >> 24) & 0xFF, 
										ue_src_ip & 0xFF, (ue_src_ip >> 8) & 0xFF, (ue_src_ip >> 16) & 0xFF, (ue_src_ip >> 24) & 0xFF,
										ue_dst_ip & 0xFF, (ue_dst_ip >> 8) & 0xFF, (ue_dst_ip >> 16) & 0xFF, (ue_dst_ip >> 24) & 0xFF,
										ue_src_port, ue_dst_port, ue_proto,
										teid);
								}
							}
							else
							{
								
							}
						}
						
						if( interface->QosEnabled == 1 && interface->QosConfigured == 1)
						{
							dpe__sched_port_pkt_write( interface, sess, pkts[i], 0, 1);
						}
						
						// eth->src_addr.addr_bytes[0] = interface->MacAddr[0];
						// eth->src_addr.addr_bytes[1] = interface->MacAddr[1];
						// eth->src_addr.addr_bytes[2] = interface->MacAddr[2];
						// eth->src_addr.addr_bytes[3] = interface->MacAddr[3];
						// eth->src_addr.addr_bytes[4] = interface->MacAddr[4];
						// eth->src_addr.addr_bytes[5] = interface->MacAddr[5];
						
						eth->src_addr.addr_bytes[0] = interface->TxInterface->MacAddr[0];
						eth->src_addr.addr_bytes[1] = interface->TxInterface->MacAddr[1];
						eth->src_addr.addr_bytes[2] = interface->TxInterface->MacAddr[2];
						eth->src_addr.addr_bytes[3] = interface->TxInterface->MacAddr[3];
						eth->src_addr.addr_bytes[4] = interface->TxInterface->MacAddr[4];
						eth->src_addr.addr_bytes[5] = interface->TxInterface->MacAddr[5];
						
						eth->dst_addr.addr_bytes[0] = interface->TxInterface->DstMac[0];
						eth->dst_addr.addr_bytes[1] = interface->TxInterface->DstMac[1];
						eth->dst_addr.addr_bytes[2] = interface->TxInterface->DstMac[2];
						eth->dst_addr.addr_bytes[3] = interface->TxInterface->DstMac[3];
						eth->dst_addr.addr_bytes[4] = interface->TxInterface->DstMac[4];
						eth->dst_addr.addr_bytes[5] = interface->TxInterface->DstMac[5];
						
						// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gtp_packet_06062024_01.pcap");
						// dpdk_pkt__write_pcap( dumper2, pkts[i]);
						// dpdk_pkt__close_pcap( dumper2);
						// exit(0); 
						
						processed_pkts[j] = pkts[i];
						j++;
					}
					else if( interface->ProcessType == PROCESS_TYPE__Gi_GTP)
					{
						root_ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));
						
						if( root_ip_hdr->next_proto_id == IPPROTO_ICMP && root_ip_hdr->dst_addr == interface->IPv4)
						{
							icmp_buf = ((uint8_t*)root_ip_hdr + 20);
							
							if(icmp_buf)
							{
								if( icmp_buf[0] == 8)
								{
									arp_mbufs[arp_cnt] = dpe__create_icmp( pkts[i], interface);
									if( arp_mbufs[arp_cnt])
									{
										arp_cnt++;
									}
								}
							}


							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;				// break switch case, not outer loop
						}
						
						ue_ip_hdr 		= (struct rte_ipv4_hdr *)root_ip_hdr;

						ue_ipv 			= 4; 
						ue_proto 		= ue_ip_hdr->next_proto_id;
						ue_src_ip 		= ue_ip_hdr->src_addr;
						ue_dst_ip 		= ue_ip_hdr->dst_addr;
						
						//printf("ue_proto==%d  %lu\n",ue_proto, ((void*)root_ip_hdr) - ((void*)eth)); 
						
						if(ue_proto == 2)			// IGMP
						{
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;
						}
						
						ue_proto_packet = (char *)root_ip_hdr + sizeof(struct rte_ipv4_hdr);							
						
						
						if( ue_proto == IPPROTO_UDP)
						{
							ue_udp = (struct rte_udp_hdr *)ue_proto_packet;
							
							ue_src_port = ue_udp->src_port;
							ue_dst_port = ue_udp->dst_port;
						}
						else if( ue_proto == IPPROTO_TCP)
						{
							ue_tcp = (struct rte_tcp_hdr *)ue_proto_packet;
							
							ue_src_port = ue_tcp->src_port;
							ue_dst_port = ue_tcp->dst_port;		
						}
						else if( ue_proto == 50)	//  Encapsulating Security Payload
						{
							ue_src_port = *(uint16_t*)&ue_proto_packet[0];
							ue_dst_port = *(uint16_t*)&ue_proto_packet[2];
						}
						else
						{
							ue_src_port = *(uint16_t*)&ue_proto_packet[0];
							ue_dst_port = *(uint16_t*)&ue_proto_packet[2];
						}
						
						
						p = rte_pktmbuf_mtod( pkts[i], char *);
						
						
						
						// printf( "ue_src_port=%d ue_dst_port=%d ue_proto=%d \n", ue_src_port, ue_dst_port, ue_proto);
						// Received DHCP Discover / Request
						
						if( ue_proto == IPPROTO_UDP)
						{
							if( ue_src_port == 17408 && ue_dst_port == 17152)
							{
								dpacket =  (struct dhcp_packet *)(((unsigned char *)ue_udp)+8);
								dhcp_packet_len = (pkts[i]->pkt_len - ( 14 + 20 + 8));
								
								uint8_t dhcp_msg_type = __dhcp__get_message_type( dpacket, dhcp_packet_len);
								
								// printf( "op=%d htype=%d len=%d dhcp_clients_count=%d\n", dpacket->op, dpacket->htype, dpacket->hlen, __dpe->dhcp_clients_count);
								// __dhcp__print_packet( dpacket, dhcp_packet_len);
								int breakDHCPOuter = 0;
								
								
								if( dhcp_msg_type == 1 || dhcp_msg_type == 3)
								{
									if( dpacket->op == 1 && dpacket->htype == 1 && dpacket->hlen == 6 && __dpe->dhcp_clients_count > 0)
									{
										int dccount = 0;
										
										for( dccount = 0; dccount < __dpe->dhcp_clients_count; dccount++)
										{
											//printf("dccount=%d\n\n", dccount);
											
											//dpe__print_buffer( "", (char *)dpacket->chaddr, 6, 1, 0);
											//dpe__print_buffer( "", (char *)__dpe->dhcp_clients[dccount].MAC, 6, 1, 0);
											//printf( "dhcp-message_type=%d\n",  __dhcp__get_message_type( dpacket, dhcp_packet_len));
											
											
											
											if( memcmp( dpacket->chaddr, __dpe->dhcp_clients[dccount].MAC, 6) == 0)
											{
												// arp_mbufs[arp_cnt] = dpe__send_dhcp_response( dpacket, dhcp_packet_len, interface);
												// if( arp_mbufs[arp_cnt])
												// {
													// arp_cnt++;
												// }
												
												//printf("dhcp: found dhcp client MAC, assign IP=%d  dhcp_msg_type=%d\n", dccount, dhcp_msg_type);
												
												struct rte_mbuf * dhcp_resp_pck = rte_pktmbuf_alloc( interface->mempool[0]);
												
												if( dhcp_resp_pck)
												{
													dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "sending dhcp response for message-type=%d which received on ProcessType=%d BusId=%s", dhcp_msg_type, interface->ProcessType, interface->Port);
													
													
													dpe__encode_dhcp_response( dhcp_resp_pck, pkts[i], interface, dhcp_msg_type, dpacket, dhcp_packet_len, __dpe->dhcp_clients[dccount].IPv4);
													
													arp_mbufs[arp_cnt] = dhcp_resp_pck;
													arp_cnt++;
												}
												
												
												dropped_mbufs[dropped_cnt] = pkts[i];
												dropped_cnt++;
												
												breakDHCPOuter = 1;
												
												break;
											}
										}
										
										if( breakDHCPOuter == 1)
											break;
									}
								}
							}
						}
						
						
						// if( ue_src_port == 17152 && ue_dst_port == 17408 && ue_proto == IPPROTO_UDP)
						// {
							
							
							// dpacket =  (struct dhcp_packet *)((unsigned char *)ue_udp) + 8;
							// dhcp_packet_len = (pkts[i]->pkt_len - ( 14 + 20 + 8));
							
							// //if( dhcp_packet_len > 236)
							// {
								// //if(dpacket->op == 2)
								// {
									// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received DHCP Message for ClientIP=%u interfaceIP=%u pktlen=%u op=%d", *(uint32_t*)&dpacket->ciaddr, interface->IPv4, dhcp_packet_len, dpacket->op); 
								// }
							// }
							
							// //if(!dhcp_packet
							// //dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received DHCP Message");

							// interface->DHCPDiscoverAckReceived = 1;
							// interface->DHCPInformAckReceived = 1;
 
							// dropped_mbufs[dropped_cnt] = pkts[i];
							// dropped_cnt++;
							// break;
						// }
						
						if( __dpe->SkipAPICalls == 0)
						{	
							if( cp__flow_allowed_ueipv4( ue_dst_ip, ue_src_ip, ue_dst_port, ue_src_port, ue_proto, p, pkts[i]->pkt_len, &sess, &p_ran_teid, PROCESS_TYPE__Gi_GTP, recv_worker_index, &qfi) == 0)
							{
								if( __dpe->EnableLogs == 1)
								{
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Gi Packet received from ue_ip=%u.%u.%u.%u  dst_ip=%u.%u.%u.%u  src_port=%u  dst_port=%u  protocol=%u teid=%u  EnableNAPT=%u  ProcessType=%u  IPv4=%u|%u sess=%p  and dropped", 
										ue_dst_ip & 0xFF, (ue_dst_ip >> 8) & 0xFF, (ue_dst_ip >> 16) & 0xFF, (ue_dst_ip >> 24) & 0xFF,
										ue_src_ip & 0xFF, (ue_src_ip >> 8) & 0xFF, (ue_src_ip >> 16) & 0xFF, (ue_src_ip >> 24) & 0xFF,
										ue_src_port, ue_dst_port, ue_proto, p_ran_teid, interface->EnableNAPT, interface->ProcessType, interface->IPv4, ue_dst_ip, sess);
								}
								
								dropped_mbufs[dropped_cnt] = pkts[i];
								dropped_cnt++;
								break;
							}
							else if( __dpe->EnableLogs == 1)
							{
								dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Gi Packet received from ue_ip=%u.%u.%u.%u  dst_ip=%u.%u.%u.%u  src_port=%u  dst_port=%u  protocol=%u  p_ran_teid=%u sess=%p  and allowed", 
									ue_dst_ip & 0xFF, (ue_dst_ip >> 8) & 0xFF, (ue_dst_ip >> 16) & 0xFF, (ue_dst_ip >> 24) & 0xFF,
									ue_src_ip & 0xFF, (ue_src_ip >> 8) & 0xFF, (ue_src_ip >> 16) & 0xFF, (ue_src_ip >> 24) & 0xFF,
									ue_src_port, ue_dst_port, ue_proto, p_ran_teid, sess);
							}
						}
						
						//pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gi_gtp_packet_in_02012024_1.pcap");
						//dpdk_pkt__write_pcap( dumper2, pkts[i]);
						//dpdk_pkt__close_pcap( dumper2);
						//exit(0);
						
						gtpHeaderLen = 16;	// seq + extn
						
						int prependLen = 0;
						if( ran_ipv == 4)
						{
							prependLen = 20 + 8 + gtpHeaderLen;
						}
						else
						{
							prependLen = 40 + 8 + gtpHeaderLen;
						}
						
						// if( seqn_no > 0) 
						// {
							// prependLen += 4;
							// if( ue_proto == 1) 
							// {
								// prependLen += 4;
							// }
						// } 
						// else if( ue_proto == 1)
						// {
							// prependLen += 8;
						// }
						
						orig_pkt_len = pkts[i]->pkt_len;
						p = rte_pktmbuf_prepend( pkts[i], prependLen);
						
						if(!p)
						{
							//printf("level 2 dropping %d\n", __LINE__);
							
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
							break;
						}
						
						if( interface->QosEnabled == 1 && interface->QosConfigured == 1)
						{
							dpe__sched_port_pkt_write( interface, sess, pkts[i], 0, 2);
						}
						
						
						if( __dpe->SkipAPICalls == 0)
						{
							if( cp__get_ranv4info( sess, &p_ran_ip_v, &p_ran_ip_v4, &p_ran_ip_v6, &ranmac) == 0)
							{
								if( __dpe->EnableLogs == 1)
								{
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Forwarding to RAN Failed for ue=%u.%u.%u.%u", 
										ue_src_ip & 0xFF, (ue_src_ip >> 8) & 0xFF, (ue_src_ip >> 16) & 0xFF, (ue_src_ip >> 24) & 0xFF);
								}
								
								dropped_mbufs[dropped_cnt] = pkts[i];
								dropped_cnt++;
								break;
							}
						}
						
						if(!ranmac)
						{
							ranmac = ranmac_temp;
							// int ix = 0;
							// while( ix < __dpe->arp_table_row_count)
							// {
								// if( __dpe->arp_table[ix].ip == p_ran_ip_v4)
								// {
									// memcpy( ranmac, __dpe->arp_table[ix].Mac, 6);
									// cp__set_mac( sess, __dpe->arp_table[ix].Mac);
									// break;
								// }
								// ix++;
							// }
							
							uint8_t * mac = dpe__get_macaddr( p_ran_ip_v4);
							
							if(mac)
							{
								memcpy( ranmac, mac, 6);
								cp__set_mac( sess, mac);
							}
						}
	
						eth = (struct rte_ether_hdr *)p;
						
						eth->src_addr.addr_bytes[0] = interface->TxInterface->MacAddr[0];
						eth->src_addr.addr_bytes[1] = interface->TxInterface->MacAddr[1];
						eth->src_addr.addr_bytes[2] = interface->TxInterface->MacAddr[2];
						eth->src_addr.addr_bytes[3] = interface->TxInterface->MacAddr[3];
						eth->src_addr.addr_bytes[4] = interface->TxInterface->MacAddr[4];
						eth->src_addr.addr_bytes[5] = interface->TxInterface->MacAddr[5];
						
						// eth->dst_addr.addr_bytes[0] = interface->SrcMac[0];
						// eth->dst_addr.addr_bytes[1] = interface->SrcMac[1];
						// eth->dst_addr.addr_bytes[2] = interface->SrcMac[2];
						// eth->dst_addr.addr_bytes[3] = interface->SrcMac[3];
						// eth->dst_addr.addr_bytes[4] = interface->SrcMac[4];
						// eth->dst_addr.addr_bytes[5] = interface->SrcMac[5];
						
						

						eth->dst_addr.addr_bytes[0] = ranmac[0];
						eth->dst_addr.addr_bytes[1] = ranmac[1];
						eth->dst_addr.addr_bytes[2] = ranmac[2];
						eth->dst_addr.addr_bytes[3] = ranmac[3];
						eth->dst_addr.addr_bytes[4] = ranmac[4];
						eth->dst_addr.addr_bytes[5] = ranmac[5];					


						/* dpe__print_buffer( "ranmac", (char*) ranmac, 6, 1, 0);
						dpe__print_buffer( "TxInterface-MacAddr", (char*) interface->TxInterface->MacAddr, 6, 1, 0);
						
						printf( "%p  ran_ipv=%d  IPv4=%d.%d.%d.%d  p_ran_ip_v4=%d  p_ran_ip_v=%d\n", interface->TxInterface, ran_ipv, 
							interface->TxInterface->IPv4 & 0xFF, (interface->TxInterface->IPv4 >> 8) & 0xFF, 
							(interface->TxInterface->IPv4 >> 16) & 0xFF, (interface->TxInterface->IPv4 >> 24) & 0xFF,
							p_ran_ip_v4, p_ran_ip_v
						); */ 
	
						
						if( ran_ipv == 4) 
						{
							eth->ether_type = 8;
							
							ip_hdr = (struct rte_ipv4_hdr *)&p[14];
							
							ip_hdr->version_ihl 	= 0x45;
							ip_hdr->type_of_service = 0x00;
							ip_hdr->total_length 	= rte_cpu_to_be_16((orig_pkt_len-14) + prependLen);
							ip_hdr->packet_id 		= 0;
							ip_hdr->fragment_offset = 0;
							ip_hdr->time_to_live 	= 64;
							ip_hdr->next_proto_id 	= 17;
							ip_hdr->hdr_checksum 	= 0;
							
							//ip_hdr->src_addr 		= rte_cpu_to_be_32( 3232273716);
							//ip_hdr->dst_addr 		= rte_cpu_to_be_32( 3232273726);
							
							ip_hdr->src_addr 		= interface->TxInterface->IPv4;
							ip_hdr->dst_addr		= p_ran_ip_v4;
							
							//ip_hdr->hdr_checksum 	= 0;
							ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);		

							udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));							
						}
						else
						{
							ip6_hdr = (struct rte_ipv6_hdr *)&p[14];
							
							udp = (struct rte_udp_hdr *) ((unsigned char *)ip6_hdr + sizeof(struct rte_ipv6_hdr));
						}
						
						//udp
						udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
						udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
						udp->dgram_cksum			= 0;
						
						// if( ran_ipv == 4) 
						// {
							// udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + (prependLen-20));	// remove udp-len from prependLen
							// udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
						// } 
						// else if( ran_ipv == 6) 
						// {
							// udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen + 8);	// remove udp-len from prependLen
							// udp->dgram_cksum		= rte_ipv6_udptcp_cksum( ip6_hdr, udp);
						// }
						
						//printf( "dgram_cksum=%u %d\n", udp->dgram_cksum, __LINE__);
						
						gtp  						= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
						gtp->gtp_hdr_info 			= 0x30;
						gtp->msg_type 				= 0xFF;
						
						if( ran_ipv == 4) 
						{
							//gtp->plen 			= rte_cpu_to_be_16( (orig_pkt_len-14) + (prependLen-(8+gtpHeaderLen)));
							gtp->plen 				= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen);
						} 
						else 
						{
							gtp->plen 				= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen);
						}
						
						gtp->teid 					= rte_be_to_cpu_32( p_ran_teid);
								
						// if( seqn_no > 0)
						// {
							// gtp->gtp_hdr_info 		= 0x32;
							// *(uint16_t *)(((unsigned char *)gtp) + 8) = rte_be_to_cpu_16( seqn_no);
						// }
						
						*(uint16_t *)(((unsigned char *)gtp) + 8) = 0;	//seq no
						gtp->gtp_hdr_info 		= 0x36;
						uint8_t * bf = (uint8_t *) gtp;
						
						
						bf[11] = 0x85;	// GTP Extension
						bf[12] = 1;
						bf[13] = 0;
						bf[14] = qfi;
						bf[15] = 0;
						
						
						// if( ue_proto == 1)
						// {
							// char * dx = ((char *)&gtp->gtp_hdr_info);
							// dx[0] |= 1 << 2;

							// if( seqn_no > 0)
							// {
								// char * dPtr = ((char *)gtp) + 11;
								// dPtr[0] = 0x85;
								// dPtr[1] = 0x01;
								// dPtr[2] = 0x00;
								// dPtr[3] = 0x01;
								// dPtr[4] = 0x00;
							// }
							// else
							// {
								// char * dPtr = ((char *)gtp) + 8;
								// dPtr[0] = 0x00;
								// dPtr[1] = 0x00;
								// dPtr[2] = 0x00;
								// dPtr[3] = 0x85;
								// dPtr[4] = 0x01;
								// dPtr[5] = 0x00;
								// dPtr[6] = 0x01;
								// dPtr[7] = 0x00;
							// }
						// }
						

						// if( ue_proto == 1)
						// {
							// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gi_gtp_packet_05JUN2024_1.pcap");
							// dpdk_pkt__write_pcap( dumper2, pkts[i]);
							// dpdk_pkt__close_pcap( dumper2);
							// exit(0);
						// }


						if( ran_ipv == 4) 
						{
							udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + (prependLen-20));	// remove udp-len from prependLen
							udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
						} 
						else if( ran_ipv == 6) 
						{
							udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen + 8);	// remove udp-len from prependLen
							udp->dgram_cksum		= rte_ipv6_udptcp_cksum( ip6_hdr, udp);
						}
						
						//printf( "dgram_cksum=%u %d\n", udp->dgram_cksum, __LINE__);

						
						processed_pkts[j] = pkts[i];
						j++;
					}
					else
					{
						
					}
				}
				break;
			case 1544:	//RTE_ETHER_TYPE_ARP:
				{
					if( dpe__isarpformyip( pkts[i], interface->IPv4, interface, natIPHead) == 1)
					{
						arp_mbufs[arp_cnt] = dpe__create_arp( pkts[i], interface, natIPHead);
						
						if( arp_mbufs[arp_cnt])
						{
							arp_cnt++;
						}
					}
					
					dropped_mbufs[dropped_cnt] = pkts[i];
					dropped_cnt++;					
				}
				break;
			case 56710:	//RTE_ETHER_TYPE_IPV6:
				{
					
					if( eth->dst_addr.addr_bytes[0] == 0x33 && eth->dst_addr.addr_bytes[1] == 0x33 && eth->dst_addr.addr_bytes[2] == 0xFF && eth->dst_addr.addr_bytes[3] == interface->MacAddr[3] && eth->dst_addr.addr_bytes[4] == interface->MacAddr[4] && eth->dst_addr.addr_bytes[5] == interface->MacAddr[5])
					{
						struct rte_ipv6_hdr * root_ip6_hdr 	= NULL;
						root_ip6_hdr = (struct rte_ipv6_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));
						
						dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received IPv6 Packet, Protocol=%u SrcIP DstIP=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
							root_ip6_hdr->proto,
							root_ip6_hdr->dst_addr[0] & 0xFF, root_ip6_hdr->dst_addr[1] & 0xFF, root_ip6_hdr->dst_addr[2] & 0xFF, root_ip6_hdr->dst_addr[3] & 0xFF,
							root_ip6_hdr->dst_addr[4] & 0xFF, root_ip6_hdr->dst_addr[5] & 0xFF, root_ip6_hdr->dst_addr[6] & 0xFF, root_ip6_hdr->dst_addr[7] & 0xFF,
							root_ip6_hdr->dst_addr[8] & 0xFF, root_ip6_hdr->dst_addr[9] & 0xFF, root_ip6_hdr->dst_addr[10] & 0xFF, root_ip6_hdr->dst_addr[11] & 0xFF,
							root_ip6_hdr->dst_addr[12] & 0xFF, root_ip6_hdr->dst_addr[13] & 0xFF, root_ip6_hdr->dst_addr[14] & 0xFF, root_ip6_hdr->dst_addr[15] & 0xFF,
							root_ip6_hdr->src_addr[0] & 0xFF, root_ip6_hdr->src_addr[1] & 0xFF, root_ip6_hdr->src_addr[2] & 0xFF, root_ip6_hdr->src_addr[3] & 0xFF,
							root_ip6_hdr->src_addr[4] & 0xFF, root_ip6_hdr->src_addr[5] & 0xFF, root_ip6_hdr->src_addr[6] & 0xFF, root_ip6_hdr->src_addr[7] & 0xFF,
							root_ip6_hdr->src_addr[8] & 0xFF, root_ip6_hdr->src_addr[9] & 0xFF, root_ip6_hdr->src_addr[10] & 0xFF, root_ip6_hdr->src_addr[11] & 0xFF,
							root_ip6_hdr->src_addr[12] & 0xFF, root_ip6_hdr->src_addr[13] & 0xFF, root_ip6_hdr->src_addr[14] & 0xFF, root_ip6_hdr->src_addr[15] & 0xFF							
						);

						uint8_t * v6Payload = (uint8_t *)root_ip6_hdr + 40;
						
						if( root_ip6_hdr->proto == 58)
						{
							//ICMPv6
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Packet With Type=%02X|%u", v6Payload[0] & 0xFF, v6Payload[0] );
							
							switch( v6Payload[0] & 0xFF)
							{
								case 128:
									{
										dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Echo Request");
									}
									break;
								case 135: // Neighbor Solicitation
									{
										arp_mbufs[arp_cnt] = dpe__create__neighbor_advertisement_resp( pkts[i], interface);
										if( arp_mbufs[arp_cnt])
										{
											dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Neighbor Solicitation IPv6 packet And sending Success Neighbor Advitisement");
											arp_cnt++;
										}
										else
										{
											dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Neighbor Solicitation IPv6 packet And sending Failed Neighbor Advitisement");
										}
									}
									break;
								default:
									{
										dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Unknown ICMPv6 Packet %u, dropping it", v6Payload[0] & 0xFF );
										
										// pcap_dumper_t * dumper5 = dpdk_pkt__open_pcap_dump( "./icmpv6_rece_imcpv6_msg_13052024_1.pcap");
										// dpdk_pkt__write_pcap( dumper5, pkts[j]);
										// dpdk_pkt__close_pcap( dumper5);
										// exit(0);		
									}
									break;
							}
							
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
						}
						else
						{
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Unknown Packet With Type=%02X|%u, dropping it", v6Payload[0] & 0xFF, v6Payload[0] );
							
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;							
						}
					}
					else if( eth->dst_addr.addr_bytes[0] == interface->MacAddr[0] && eth->dst_addr.addr_bytes[1] == interface->MacAddr[1] && eth->dst_addr.addr_bytes[2] == interface->MacAddr[2] && eth->dst_addr.addr_bytes[3] == interface->MacAddr[3] && eth->dst_addr.addr_bytes[4] == interface->MacAddr[4] && eth->dst_addr.addr_bytes[5] == interface->MacAddr[5])
					{
						struct rte_ipv6_hdr * root_ip6_hdr 	= NULL;
						root_ip6_hdr = (struct rte_ipv6_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));
						
						dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received IPv6 Packet, Protocol=%u SrcIP DstIP=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
							root_ip6_hdr->proto,
							root_ip6_hdr->dst_addr[0] & 0xFF, root_ip6_hdr->dst_addr[1] & 0xFF, root_ip6_hdr->dst_addr[2] & 0xFF, root_ip6_hdr->dst_addr[3] & 0xFF,
							root_ip6_hdr->dst_addr[4] & 0xFF, root_ip6_hdr->dst_addr[5] & 0xFF, root_ip6_hdr->dst_addr[6] & 0xFF, root_ip6_hdr->dst_addr[7] & 0xFF,
							root_ip6_hdr->dst_addr[8] & 0xFF, root_ip6_hdr->dst_addr[9] & 0xFF, root_ip6_hdr->dst_addr[10] & 0xFF, root_ip6_hdr->dst_addr[11] & 0xFF,
							root_ip6_hdr->dst_addr[12] & 0xFF, root_ip6_hdr->dst_addr[13] & 0xFF, root_ip6_hdr->dst_addr[14] & 0xFF, root_ip6_hdr->dst_addr[15] & 0xFF,
							root_ip6_hdr->src_addr[0] & 0xFF, root_ip6_hdr->src_addr[1] & 0xFF, root_ip6_hdr->src_addr[2] & 0xFF, root_ip6_hdr->src_addr[3] & 0xFF,
							root_ip6_hdr->src_addr[4] & 0xFF, root_ip6_hdr->src_addr[5] & 0xFF, root_ip6_hdr->src_addr[6] & 0xFF, root_ip6_hdr->src_addr[7] & 0xFF,
							root_ip6_hdr->src_addr[8] & 0xFF, root_ip6_hdr->src_addr[9] & 0xFF, root_ip6_hdr->src_addr[10] & 0xFF, root_ip6_hdr->src_addr[11] & 0xFF,
							root_ip6_hdr->src_addr[12] & 0xFF, root_ip6_hdr->src_addr[13] & 0xFF, root_ip6_hdr->src_addr[14] & 0xFF, root_ip6_hdr->src_addr[15] & 0xFF							
						);
						
						if( root_ip6_hdr->proto == 58)
						{
							//ICMPv6
							uint8_t * v6Payload = (uint8_t *)root_ip6_hdr + 40;
							//dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Packet With Type=%02X|%u, dropping it", v6Payload[0] & 0xFF, v6Payload[0] );
							
							switch( v6Payload[0] & 0xFF)
							{
								case 128:
									{
										//dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Echo Request");
										
										arp_mbufs[arp_cnt] = dpe__create__icmpv6_echo__resp( pkts[i], interface);
										if( arp_mbufs[arp_cnt])
										{
											dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Echo Request And sending Echo Reply");
											arp_cnt++;
										}
										else
										{
											dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received ICMPv6 Echo Request And sending Failed Echo Reply");
										}
										
										// pcap_dumper_t * dumper5 = dpdk_pkt__open_pcap_dump( "./icmpv6_rece_echo_13052024_1.pcap");
										// dpdk_pkt__write_pcap( dumper5, pkts[j]);
										// dpdk_pkt__close_pcap( dumper5);
										// exit(0);
									}
									break;
								default:
									dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Unknown ICMPv6 Packet %u, dropping it", v6Payload[0] & 0xFF );
									break;
							}
							
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
						}
						else if( root_ip6_hdr->proto == 6)
						{
							//TCP
						}
						else if( root_ip6_hdr->proto == 17)
						{
							//UDP
						}						
						else
						{
							dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received IPv6 Packet, dropping it");
							dropped_mbufs[dropped_cnt] = pkts[i];
							dropped_cnt++;
						}
					}
					else
					{
						// pcap_dumper_t * dumper5 = dpdk_pkt__open_pcap_dump( "./icmpv6_packet_15052024_1.pcap");
						// dpdk_pkt__write_pcap( dumper5, pkts[j]);
						// dpdk_pkt__close_pcap( dumper5);
						// exit(0);
						
						// dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received IPv6 Packet, dropping it");
						dropped_mbufs[dropped_cnt] = pkts[i];
						dropped_cnt++;
					}
					
				}
				break;
			default:
				{
					dropped_mbufs[dropped_cnt] = pkts[i];
					dropped_cnt++;					
				}
				break;
		}
		
		i++;
	}
	
	
	*gtp_cnt_out		= gtp_cnt;
	*arp_cnt_out 		= arp_cnt;
	*dropped_cnt_out 	= dropped_cnt;
	return j;
}


int dpe__receive( void * args)
{
	// while(1){
		// sleep(1);
	// }

	dpdk_interface_port_t * interface_port = (dpdk_interface_port_t *) args;

	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "launching receive core at %s", interface_port->interface->Port);

	while( interface_port->interface->started == 0) 
	{
		usleep( 500000);
	}
	
	while( interface_port->interface->QosInitalized == 1 && interface_port->interface->QosConfigured == 0) 
	{
		usleep( 500000);
	}
	
	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "starting receive core at %s", interface_port->interface->Port);
	
	
	struct rte_mbuf * mbufs[interface_port->interface->RecvChunkSize];
	struct rte_ring * ring = interface_port->interface->recv_ring[interface_port->queue_id];
	
	uint16_t received_cnt = 0;
	uint16_t pkts_send = 0;
	
	
	struct rte_ipv4_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr 	* udp 		= NULL;
	struct rte_tcp_hdr 	* tcp		= NULL;		
	struct rte_icmp_hdr * icmp_h	= NULL;
	uint8_t * pkt 					= NULL;
	uint16_t * icmp_cksum 			= NULL;
	uint16_t * icmp_ident 			= NULL;
	uint16_t cksum1					= 0;
	
	dpe_nat_entry_t * nat_entry		= NULL;
	int cx = 0;
				
	while(__dpe->runing)
	{
		received_cnt = rte_eth_rx_burst( interface_port->interface->PortId, interface_port->queue_id, mbufs, interface_port->interface->RecvChunkSize);
	
		if( received_cnt == 0)
		{
			//rte_delay_us( 5000);
			dpe__sleep();
		} 
		else 
		{
			interface_port->interface->total_received += received_cnt;
			pkts_send = 0;



			if( __dpe->pdumper)
			{
				for( cx = 0; cx < received_cnt; cx++)
				{
					rte_prefetch0(rte_pktmbuf_mtod( mbufs[cx], void *));

					if( interface_port->interface->ProcessType == PROCESS_TYPE__GTP_Gi && __dpe->enable_pcap_on__gtp_gi == 1)
					{
						dpdk_pkt__write_pcap( __dpe->pdumper, mbufs[cx]);
					}
					else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_GTP && __dpe->enable_pcap_on__gi_gtp == 1)
					{
						dpdk_pkt__write_pcap( __dpe->pdumper, mbufs[cx]);
					}
					else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_Gi_INGRESS && __dpe->enable_pcap_on__gi_gi == 1)
					{
						dpdk_pkt__write_pcap( __dpe->pdumper, mbufs[cx]);
					}
					else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_Gi_EGRESS && __dpe->enable_pcap_on__gi_gi == 1)
					{
						dpdk_pkt__write_pcap( __dpe->pdumper, mbufs[cx]);
					}
				}
			}


			if( interface_port->interface->EnableNAPT == 1 && interface_port->interface->ProcessType == PROCESS_TYPE__Gi_GTP)
			{
				cx = 0;
				
				for( cx = 0; cx < received_cnt; cx++)
				{
					rte_prefetch0(rte_pktmbuf_mtod( mbufs[cx], void *));
					pkt 		= rte_pktmbuf_mtod( mbufs[cx], uint8_t *);
					ip_hdr 		= (struct rte_ipv4_hdr *)&pkt[14];


					
					//dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "version_ihl=%02X next_proto_id=%u dst_addr=%u IPv4=%u", ip_hdr->version_ihl, ip_hdr->next_proto_id, ip_hdr->dst_addr, interface_port->interface->IPv4);
					
					
					if( (ip_hdr->next_proto_id == IPPROTO_ICMP || ip_hdr->next_proto_id == IPPROTO_UDP ||  ip_hdr->next_proto_id == IPPROTO_TCP) && ip_hdr->dst_addr == interface_port->interface->IPv4 )
					{
						if( ip_hdr->version_ihl == 0x45)
						{
							if( ip_hdr->next_proto_id == IPPROTO_UDP)
							{
								udp			= (struct rte_udp_hdr *)&pkt[ 14 + 20 ];
								
								nat_entry 	= dpe__find_natentry_by_natsrcport( interface_port->interface, ip_hdr->dst_addr, udp->dst_port, ip_hdr->src_addr, udp->src_port, ip_hdr->next_proto_id);
								
								
								
								if( nat_entry)
								{
									ip_hdr->dst_addr 		= nat_entry->UeIP;
									udp->dst_port			= nat_entry->UeSrcPort;
									
									ip_hdr->hdr_checksum 	= 0;
									ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
									
									udp->dgram_cksum		= 0;
									udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
								}
							}
							else if(ip_hdr->next_proto_id == IPPROTO_TCP)
							{
								tcp	= (struct rte_tcp_hdr *)&pkt[ 14 + 20 ];

								nat_entry 	= dpe__find_natentry_by_natsrcport( interface_port->interface, ip_hdr->dst_addr, tcp->dst_port, ip_hdr->src_addr, tcp->src_port, ip_hdr->next_proto_id);
							
								if( nat_entry)
								{
									ip_hdr->dst_addr 		= nat_entry->UeIP;
									tcp->dst_port			= nat_entry->UeSrcPort;
									
									ip_hdr->hdr_checksum 	= 0;
									ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
								
									tcp->cksum				= 0;
									tcp->cksum				= rte_ipv4_udptcp_cksum( ip_hdr, tcp);
								}
							}
							else if(ip_hdr->next_proto_id == IPPROTO_ICMP)
							{
								icmp_h	= (struct rte_icmp_hdr *)&pkt[ 14 + 20 ];
								
								//rintf( "icmp_ident=%u icmp_seq_nb=%u  %s|%d\n", icmp_h->icmp_ident, icmp_h->icmp_seq_nb, __FUNCTION__, __LINE__);
								nat_entry 	= dpe__find_natentry_by_natsrcport( interface_port->interface, ip_hdr->dst_addr, icmp_h->icmp_ident, ip_hdr->src_addr, icmp_h->icmp_seq_nb, ip_hdr->next_proto_id);
								
								if( nat_entry)
								{
									//icmp_cksum 	= (uint16_t *)&pkt[ 14 + 20 + 2 ];

									ip_hdr->dst_addr 		= nat_entry->UeIP;
									icmp_h->icmp_ident		= nat_entry->UeSrcPort;

									ip_hdr->hdr_checksum 	= 0;
									ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
								
									icmp_h->icmp_cksum		= 0;
									icmp_h->icmp_cksum 		= y2checksum( (uint16_t*)(uint16_t *)&pkt[ 14 + 20], mbufs[cx]->pkt_len - (14 + 20));
									//*(uint16_t*)(&pkt[ 14 + 20 + 2 ]) = cksum1;
										
								}
							}
						}
					}
				}
			}

			do {
				pkts_send += rte_ring_enqueue_burst( ring, (void **)&mbufs[pkts_send], received_cnt - pkts_send, NULL);
				//ifc->rx_packets_enq++;				// enqueed times
			} while( pkts_send < received_cnt);


			received_cnt = 0;
		}
	}
	
	return 0;
}



int dpe__recv_worker( void * args)
{
	// while(1){
		// sleep(1);
	// }	
	
	dpdk_interface_port_t * interface_port = (dpdk_interface_port_t *) args;
	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "starting rec-worker core at %s", interface_port->interface->Port);

	
	struct rte_ring * ring = interface_port->interface->recv_ring[interface_port->queue_id];
	struct rte_mbuf * pkts[interface_port->interface->RecvChunkSize];
	
	struct rte_mbuf * processed_pkts[interface_port->interface->RecvChunkSize];
	uint16_t processed_pkts_count = 0;
	
	struct rte_mbuf * drop_pkts[interface_port->interface->RecvChunkSize];
	uint16_t drop_pkts_count = 0;
	
	struct rte_mbuf * arp_pkts[interface_port->interface->RecvChunkSize];
	uint16_t arp_pkts_count = 0;

	struct rte_mbuf * gtp_pkts[interface_port->interface->RecvChunkSize];
	uint16_t gtp_pkts_count = 0;
	
	

	while( interface_port->interface->started == 0) 
	{
		usleep( 100000);
	}

	
	dpdk_interface_t * txInterface = interface_port->interface;
	struct rte_ring * send_ring = NULL;
	
	int recv_pkts = 0;
	int pkts_send = 0;
	int rr_index = 0;
	int iterator = 0;
	int arp_pkts_send = 0;
	int gtp_pkts_send = 0;
	
	if( __dpe->Type == 3)
	{
		if(!interface_port->interface->TxInterface)
		{
			printf("target interface not configured\n");
			exit(0);
		}
		txInterface = interface_port->interface->TxInterface;
	}

	while(__dpe->runing)
	{
		recv_pkts = rte_ring_dequeue_burst( ring, (void **)pkts, interface_port->interface->RecvChunkSize, NULL);
		
		if( recv_pkts == 0)
		{
			//rte_delay_us( 5000);
			dpe__sleep();
		}
		else
		{
			interface_port->interface->total_dequeed += recv_pkts;
			
			if( __dpe->Type == 1)
			{
				//SendAndRecv
				processed_pkts_count = dpe__process_received( interface_port->interface, pkts, recv_pkts, processed_pkts, drop_pkts, &drop_pkts_count, arp_pkts, &arp_pkts_count, interface_port->recv_worker_index);
			}
			else if( __dpe->Type == 2)
			{
				//RecvAndSend
				processed_pkts_count = dpe__process_recv_and_send( interface_port->interface, pkts, recv_pkts, processed_pkts, drop_pkts, &drop_pkts_count, arp_pkts, &arp_pkts_count, interface_port->recv_worker_index);
			}
			else if( __dpe->Type == 3 || __dpe->Type == 4)
			{
				//Forward
				processed_pkts_count = dpe__process_forward( interface_port->interface, pkts, recv_pkts, processed_pkts, drop_pkts, &drop_pkts_count, arp_pkts, &arp_pkts_count, gtp_pkts, &gtp_pkts_count, interface_port->recv_worker_index);
			
				
				// if( recv_pkts != (processed_pkts_count + drop_pkts_count + arp_pkts_count))
				// {
					// printf( "recv_pkts=%d Not Equals Prossed Packets  processed_pkts_count=%u drop_pkts_count=%u arp_pkts_count=%u  enqueed=%lu dropped=%lu ---\n", 
						// recv_pkts, processed_pkts_count, drop_pkts_count, arp_pkts_count, txInterface->total_enqueed, txInterface->total_dropped);
				// }
				// else
				// {
					// printf( "recv_pkts=%d Equals Prossed Packets  processed_pkts_count=%u drop_pkts_count=%u arp_pkts_count=%u enqueed=%lu dropped=%lu\n", 
						// recv_pkts, processed_pkts_count, drop_pkts_count, arp_pkts_count, txInterface->total_enqueed, txInterface->total_dropped);					
				// }
				// sleep(1); 
			}
			
			
			if( txInterface->SendQueue == 1)
			{
				rr_index = 0;
			}
			else
			{
				if( iterator >= txInterface->SendQueue)
					iterator = 0;
				
				rr_index = iterator % txInterface->SendQueue;
				iterator++;
			}
			
			send_ring = txInterface->send_ring[rr_index];
			
			if( drop_pkts_count > 0)
			{
				//txInterface->total_dropped += drop_pkts_count;
				interface_port->interface->total_dropped += drop_pkts_count;
				rte_pktmbuf_free_bulk( drop_pkts, drop_pkts_count);
			}

			if( arp_pkts_count > 0)
			{
				arp_pkts_send = 0;
				do {
					arp_pkts_send += rte_ring_enqueue_burst( interface_port->interface->send_ring[0], (void **)&arp_pkts[arp_pkts_send], arp_pkts_count - arp_pkts_send, NULL);
				} while( arp_pkts_send < arp_pkts_count);
				
				//txInterface->arp_request_received += arp_pkts_count;
				//txInterface->total_enqueed += arp_pkts_count;
				interface_port->interface->arp_request_received += arp_pkts_count;
				interface_port->interface->arp_response_sent += arp_pkts_count;
				interface_port->interface->total_enqueed += arp_pkts_count;
			}
			
			if( gtp_pkts_count > 0)
			{
				gtp_pkts_send = 0;
				do {
					gtp_pkts_send += rte_ring_enqueue_burst( interface_port->interface->send_ring[0], (void **)&gtp_pkts[gtp_pkts_send], gtp_pkts_count - gtp_pkts_send, NULL);
				} while( gtp_pkts_send < gtp_pkts_count);
				
				//txInterface->arp_request_received += arp_pkts_count;
				//txInterface->total_enqueed += arp_pkts_count;
				// interface_port->interface->arp_request_received += arp_pkts_count;
				// interface_port->interface->arp_response_sent += arp_pkts_count;
				interface_port->interface->total_enqueed += gtp_pkts_count;
			}
			
			
			if( processed_pkts_count > 0)
			{
				pkts_send = 0;
				
				if( interface_port->interface->QosInitalized == 1 && interface_port->interface->QosConfigured == 1)
				{
					do {
						pkts_send += rte_ring_enqueue_burst( interface_port->interface->sched_ring, (void **)&processed_pkts[pkts_send], processed_pkts_count - pkts_send, NULL);
					} while( pkts_send < processed_pkts_count);
				}
				else
				{
					do {
						pkts_send += rte_ring_enqueue_burst( send_ring, (void **)&processed_pkts[pkts_send], processed_pkts_count - pkts_send, NULL);
					} while( pkts_send < processed_pkts_count);
				}
				
				//txInterface->total_enqueed += processed_pkts_count;
				interface_port->interface->total_enqueed += processed_pkts_count;
			}			
			
			
		}
		
	}
	
	return 0;
}

/*
int dpe__send_worker( void * args)
{
	dpdk_interface_port_t * interface_port = (dpdk_interface_port_t *) args;
	
	while(__dpe->runing) 
	{
		usleep( 555555);
	}
	
	return 0;
}
*/



void dpe__setPacketGeneratorFunction( int(*ptrPacketGenerator)( uint8_t * iobj, uint8_t *))
{
	ptrPacketGeneratorFunction = ptrPacketGenerator;
}


long dpe__scnt = 0;

void * dpe__generator_worker_cnt( void * v)
{
	while(1)
	{
		printf("dpe__scnt=%ld\n", dpe__scnt); 
		dpe__scnt = 0;
		usleep(999999);
	}
	return NULL;
}

void dpe__create_t__generator_worker()
{
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	int iRet = pthread_create( &s_pthread_id, &attr, dpe__generator_worker_cnt, (void *)NULL);	
}

int dpe__generator_worker( void * args)
{
	dpdk_interface_port_t * interface_port = (dpdk_interface_port_t *) args;
	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "starting packet generator core at %s", interface_port->interface->Port);

	dpdk_interface_t * interface = NULL;
	uint16_t queue_id = 0;
	
	if( interface_port) {
		queue_id = interface_port->queue_id;
	}
	
	if( interface_port->interface) {
		interface = interface_port->interface;
	} else {
		interface = dpe__getinterface(0);
	}
	
	while( interface->started == 0)
	{
		 rte_delay_ms( 100);
	}
	

	int mbufs_count = 64;
	struct rte_mbuf * mbufs[mbufs_count];
	int cmbuf = -1;
	int sts = 0;
	
	
	if( ptrPacketGeneratorFunction > 0) 
	{
		printf("startig ptrPacketGeneratorFunction..\n");
		struct rte_mempool * mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
		
		while(__dpe->runing) 
		{
			// if( cmbuf == -1 || cmbuf == mbufs_count)
			// {
				// sts = rte_pktmbuf_alloc_bulk( interface->mempool[queue_id], mbufs, mbufs_count);
				
				// while( sts != 0)
				// {
					// printf("rte_pktmbuf_alloc_bulk failed, retrying\n");
					// rte_delay_ms(100);
					// sts = rte_pktmbuf_alloc_bulk( interface->mempool[queue_id], mbufs, mbufs_count);
				// }
				
				// cmbuf = 0;
			// }
			
			//sts = ptrPacketGeneratorFunction( (uint8_t *)interface_port, (uint8_t*)mbufs[cmbuf]);
			
			//rte_pktmbuf_free( mbufs[cmbuf]);
			
			//struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interface->mempool[queue_id]);
			struct rte_mbuf * m_buf = rte_pktmbuf_alloc( mpool);
			
			if( m_buf)
			{
				sts = ptrPacketGeneratorFunction( (uint8_t *)interface_port, (uint8_t*)m_buf);

/*
	int protocol = 6;
	int blen = 100;
	int gtpseqno = 10;
	int tcpseqno = 20;

	
	int prependLen 		= 0;
	int gtpHeaderLen 	= (gtpseqno > 0) ? 12 : 8;
	
	if( protocol == 1)
	{
		if(gtpseqno > 0)
			gtpHeaderLen		+= 4;
		else
			gtpHeaderLen		+= 8;
		
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + blen;
	}
	else if( protocol == 6)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 20 + blen;
	}
	else if( protocol == 17)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 8 + blen;
	}

	m_buf->pkt_len  	= prependLen;
	m_buf->data_len 	= prependLen;

	unsigned char * packet = rte_pktmbuf_mtod( m_buf, unsigned char *);
	
	struct rte_ether_hdr 	* eth 			= (struct rte_ether_hdr *)packet;
	struct rte_udp_hdr 		* udp  			= NULL;
	struct rte_ipv4_hdr 	* ip_hdr 		= NULL;
	struct rte_ipv4_hdr 	* usr_ip_hdr 	= NULL;
	struct rte_udp_hdr 		* usr_udp  		= NULL;
	
	eth->src_addr.addr_bytes[0] = 0x01;
	eth->src_addr.addr_bytes[1] = 0x02;
	eth->src_addr.addr_bytes[2] = 0x03;
	eth->src_addr.addr_bytes[3] = 0x04;
	eth->src_addr.addr_bytes[4] = 0x05;
	eth->src_addr.addr_bytes[5] = 0x06;
	
	eth->dst_addr.addr_bytes[0] = 0x07;
	eth->dst_addr.addr_bytes[1] = 0x08;
	eth->dst_addr.addr_bytes[2] = 0x09;
	eth->dst_addr.addr_bytes[3] = 0x10;
	eth->dst_addr.addr_bytes[4] = 0x11;
	eth->dst_addr.addr_bytes[5] = 0x12;
	
	eth->ether_type = 8;


	ip_hdr = (struct rte_ipv4_hdr *)&packet[14];
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	ip_hdr->total_length 	= rte_cpu_to_be_16(prependLen - 14);
	
	ip_hdr->packet_id 		= tcpseqno + (prependLen - 14);
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	//ip_hdr->hdr_checksum 	= 0;
	
	ip_hdr->src_addr 		= 10929;
	ip_hdr->dst_addr 		= rte_cpu_to_be_32( 123456);
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);

	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

	udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
	udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
	udp->dgram_cksum			= 0;

	udp->dgram_len 			= rte_cpu_to_be_16( prependLen - (14 + 20));
	udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	struct rte_gtp_hdr * gtp  	= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 0xFF;
	gtp->plen 					= rte_cpu_to_be_16( prependLen - (14 + 20 + 8));
	gtp->teid 					= rte_be_to_cpu_32( 786783); 

	if( gtpseqno > 0)
	{
		gtp->gtp_hdr_info 							= 0x32;
		*(uint16_t *)(((unsigned char *)gtp) + 8) 	= rte_be_to_cpu_16( gtpseqno);
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr) + 4);
	}
	else
	{
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr));
	}

	if( protocol == 1)
	{
		char * dx = ((char *)&gtp->gtp_hdr_info);
		dx[0] |= 1 << 2;
		
		if( gtpseqno > 0)
		{
			char * dPtr = ((char *)usr_ip_hdr) - 1;
			dPtr[0] = 0x85;
			dPtr[1] = 0x01;
			dPtr[2] = 0x10;
			dPtr[3] = 0x01;
			dPtr[4] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[5];
		}
		else
		{
			char * dPtr = ((char *)usr_ip_hdr);
			dPtr[0] = 0x00;
			dPtr[1] = 0x00;
			dPtr[2] = 0x00;
			dPtr[3] = 0x85;
			dPtr[4] = 0x01;
			dPtr[5] = 0x10;
			dPtr[6] = 0x01;
			dPtr[7] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[8];
		}
	}
	
	
	
	usr_ip_hdr->version_ihl 	= 0x45;
	usr_ip_hdr->type_of_service = 0x00;
	usr_ip_hdr->total_length 	= rte_cpu_to_be_16( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->packet_id 		= tcpseqno + ( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->fragment_offset = 0;
	usr_ip_hdr->time_to_live 	= 64;
	usr_ip_hdr->next_proto_id 	= protocol;			//17 UDP,  6 TCP
	usr_ip_hdr->hdr_checksum 	= 0;
	
	usr_ip_hdr->src_addr 		= rte_cpu_to_be_32( 32323);
	usr_ip_hdr->dst_addr 		= rte_cpu_to_be_32( 5454545);
	
	//usr_ip_hdr->hdr_checksum 	= 0;
	//usr_ip_hdr->hdr_checksum 	= rte_ipv4_cksum( usr_ip_hdr);

	if( protocol == 6 )
	{
		struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		
		tcp->src_port				= rte_cpu_to_be_16( 6665);
		tcp->dst_port				= rte_cpu_to_be_16( 6656);
		tcp->cksum					= 0;
		tcp->sent_seq				= tcpseqno;
		tcp->data_off				= 0x50;
		tcp->tcp_flags				= 0x18;
		tcp->rx_win					= 0x4410;
		//tcp->cksum					= rte_ipv4_udptcp_cksum( usr_ip_hdr, tcp);
	}
	else if( protocol == 17)
	{
		usr_udp = (struct rte_udp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		usr_udp->src_port			= rte_cpu_to_be_16( 43434);
		usr_udp->dst_port			= rte_cpu_to_be_16( 5454);
		usr_udp->dgram_cksum		= 0;

		//usr_udp->dgram_len 			= rte_cpu_to_be_16( blen + 8);
		//usr_udp->dgram_cksum		= rte_ipv4_udptcp_cksum( usr_ip_hdr, usr_udp);
	}
	else
	{
		char * dptr = ((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		//memcpy( dptr, buffer, blen);
	}				
*/
				
//				rte_pktmbuf_free( m_buf);
				dpe__scnt++;
			}
			sts = 1;

			// if( sts == 1)
			// {
				// cmbuf++;
			// }
		}	
	} 
	else
	{
		printf("packet generator unset  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}
		
	
	return 0;
}


unsigned short calc_icmp__calculate_checksum( void * b, int len) 
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    // Sum all 16-bit words
    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    // Add leftover byte, if any
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    // Fold 32-bit sum to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}


int dpe__send( void * args)
{
	// while(1){
		// sleep(1);
	// }
	
	dpdk_interface_port_t * interface_port = (dpdk_interface_port_t *) args;
	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "starting send core at %s", interface_port->interface->Port);


	struct rte_ring * ring = interface_port->interface->send_ring[interface_port->queue_id];
	
	int recv_pkts = 0;
	int pkts_send = 0;
	uint16_t sent = 0;
	
	struct rte_mbuf * pkts[interface_port->interface->RecvChunkSize];
	
	int cx = 0;
	
	while(__dpe->runing) 
	{
		pkts_send = 0;
		recv_pkts = rte_ring_dequeue_burst( ring, (void **)pkts, interface_port->interface->RecvChunkSize, NULL);
		
		if( recv_pkts == 0)
		{
			//rte_delay_us( 5000);
			dpe__sleep();
		}
		else
		{
			do 
			{
				struct rte_ipv4_hdr * ip_hdr 	= NULL;
				struct rte_udp_hdr 	* udp 		= NULL;
				struct rte_tcp_hdr 	* tcp		= NULL;				
				struct rte_icmp_hdr * icmp_h	= NULL;				
				uint8_t * pkt 					= NULL;
				dpe_nat_entry_t * nat_entry		= NULL;
				uint16_t cksum1 				= 0;

				if( interface_port->interface->EnableNAPT == 1 && interface_port->interface->ProcessType == PROCESS_TYPE__Gi_GTP)
				{
					for( cx = 0; cx < recv_pkts; cx++)
					{
						pkt 		= rte_pktmbuf_mtod( pkts[cx], uint8_t *);
						ip_hdr 		= (struct rte_ipv4_hdr *)&pkt[14];
						
						
						
						if( (ip_hdr->next_proto_id == IPPROTO_ICMP || ip_hdr->next_proto_id == IPPROTO_UDP || ip_hdr->next_proto_id == IPPROTO_TCP) && ip_hdr->src_addr != interface_port->interface->IPv4 )
						{
							if( ip_hdr->version_ihl == 0x45)
							{
								if( ip_hdr->next_proto_id == IPPROTO_UDP)
								{
									udp	= (struct rte_udp_hdr *)&pkt[ 14 + 20 ];
									nat_entry = dpe__find_or_create_natentry( interface_port->interface, ip_hdr->src_addr, ip_hdr->dst_addr, udp->src_port, udp->dst_port, ip_hdr->next_proto_id);
								
									if( nat_entry)
									{
										ip_hdr->src_addr 	= interface_port->interface->IPv4;
										udp->src_port		= nat_entry->NatSrcPort;
										
										ip_hdr->hdr_checksum 	= 0;
										ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
										
										udp->dgram_cksum		= 0;
										udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
									}
								}
								else if(ip_hdr->next_proto_id == IPPROTO_TCP)
								{
									tcp	= (struct rte_tcp_hdr *)&pkt[ 14 + 20 ];
									nat_entry = dpe__find_or_create_natentry( interface_port->interface, ip_hdr->src_addr, ip_hdr->dst_addr, tcp->src_port, tcp->dst_port, ip_hdr->next_proto_id);

									if( nat_entry)
									{
										ip_hdr->src_addr 	= interface_port->interface->IPv4;
										tcp->src_port		= nat_entry->NatSrcPort;

										ip_hdr->hdr_checksum 	= 0;
										ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
										
										tcp->cksum			= 0;
										tcp->cksum			= rte_ipv4_udptcp_cksum( ip_hdr, tcp);
									}
								}
								else if(ip_hdr->next_proto_id == IPPROTO_ICMP)
								{
									icmp_h = (struct rte_icmp_hdr *)&pkt[ 14 + 20 ];
									
									uint16_t * icmp_cksum 	= (uint16_t *)&pkt[ 14 + 20 + 2 ];
									// uint16_t * icmp_ident 	= (uint16_t *)&pkt[ 14 + 20 + 4 ];
									// uint16_t * icmp_seq_nb 	= (uint16_t *)&pkt[ 14 + 20 + 6 ];
									
									nat_entry = dpe__find_or_create_natentry( interface_port->interface, ip_hdr->src_addr, ip_hdr->dst_addr, icmp_h->icmp_ident, icmp_h->icmp_seq_nb, ip_hdr->next_proto_id);
									
									if( nat_entry)
									{
										ip_hdr->src_addr 		= interface_port->interface->IPv4;
										icmp_h->icmp_ident		= nat_entry->NatSrcPort;

										ip_hdr->hdr_checksum 	= 0;
										ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
										
										*icmp_cksum			= 0;
										cksum1 = y2checksum( (uint16_t*)(uint16_t *)&pkt[ 14 + 20], pkts[cx]->pkt_len - (14 + 20));
										*(uint16_t*)(&pkt[ 14 + 20 + 2 ]) = cksum1;

										//printf( "pkt_len=%u icmplen=%u cksum1=%u\n", pkts[cx]->pkt_len, pkts[cx]->pkt_len - (14 + 20), cksum1);

									}
								}
							}
						}
					}
				}
				
				
				if( __dpe->pdumper)
				{
					for( cx = 0; cx < recv_pkts; cx++)
					{
						if( interface_port->interface->ProcessType == PROCESS_TYPE__GTP_Gi && __dpe->enable_pcap_on__gtp_gi == 1)
						{
							dpdk_pkt__write_pcap( __dpe->pdumper, pkts[cx]);
						}
						else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_GTP && __dpe->enable_pcap_on__gi_gtp == 1)
						{
							dpdk_pkt__write_pcap( __dpe->pdumper, pkts[cx]);
						}
						else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_Gi_INGRESS && __dpe->enable_pcap_on__gi_gi == 1)
						{
							dpdk_pkt__write_pcap( __dpe->pdumper, pkts[cx]);
						}
						else if( interface_port->interface->ProcessType == PROCESS_TYPE__Gi_Gi_EGRESS && __dpe->enable_pcap_on__gi_gi == 1)
						{
							dpdk_pkt__write_pcap( __dpe->pdumper, pkts[cx]);
						}
					}	
				}
				
				
				
				
				
				sent = rte_eth_tx_burst( interface_port->interface->PortId , interface_port->queue_id, &pkts[pkts_send], recv_pkts - pkts_send);
				// rte_pktmbuf_free_bulk( &pkts[pkts_send], recv_pkts - pkts_send);
				// sent = recv_pkts;
				pkts_send += sent;
			} while ( pkts_send < recv_pkts);
			
			interface_port->interface->total_sent += pkts_send;
			
			if( pkts_send > 0)
			{
				dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "pkts_send=%u ProcessType=%u  %d - %d - %d", 
						pkts_send, interface_port->interface->ProcessType, 
						__dpe->enable_pcap_on__gtp_gi, __dpe->enable_pcap_on__gi_gtp, __dpe->enable_pcap_on__gi_gi
					);
			}
			
			/*
			if( recv_pkts > 0)
			{
				unsigned char * packet = rte_pktmbuf_mtod( pkts[0], unsigned char *);
				
				if( packet[15] != 1)
				{
					pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gi_sent_packet_24012024.pcap");
					int z = 0;
					
					//recv_pkts
					for( z =0; z < 1 ; z++)
					{
						dpdk_pkt__write_pcap( dumper2, pkts[z]);
					}
					dpdk_pkt__close_pcap( dumper2);
					exit(0);
				}
			}
			*/
		}
	}
	
	return 0;
}


int dpe__set_rx_chksum_offload( uint8_t port_id, struct rte_eth_dev_info * dev_info, struct rte_eth_rxconf * rxq_conf)
{
    /* checking rx ip/udp/tcp offloads */
    if (!(dev_info->rx_offload_capa & RTE_ETH_RX_OFFLOAD_CHECKSUM)) {
        //RTE_LOG(INFO, APP, "Rx checksum offloads not supported by port %" PRIu8 ",\n", port_id);
        return -1;
    }
    rxq_conf->offloads |= RTE_ETH_RX_OFFLOAD_CHECKSUM;
    return 0;
}



int dpe__sched_port( void * interafaceArgs);

void dpe__configure_interface( dpdk_interface_t * interaface)
{
	int ret = 0;
	ret = rte_eth_dev_get_port_by_name( interaface->Port, &interaface->PortId);

	if (0 != ret)
	{
		printf("Error: rte_eth_dev_get_port_by_name failed for BusId=%s\n", interaface->Port);
		exit(0);
	}


	ret = rte_eth_dev_is_valid_port( interaface->PortId);
	
	if (1 != ret)
	{
		printf("Error: rte_eth_dev_is_valid_port failed for BusId=%s\n", interaface->Port);
		exit(0);
	}

	struct rte_eth_dev_info dev_info;
	struct rte_eth_conf port_conf;
	struct rte_eth_txconf tx_conf;
	struct rte_ether_addr addr;
	
	memset( &dev_info, 	0, sizeof(struct rte_eth_dev_info));
	memset( &port_conf, 0, sizeof(struct rte_eth_conf));
	memset( &tx_conf, 	0, sizeof(struct rte_eth_txconf));
	memset( &addr, 		0, sizeof(struct rte_ether_addr));
	
	ret = rte_eth_macaddr_get( interaface->PortId, &addr);

	if (0 == ret)
	{
		memcpy( interaface->MacAddr, (char *)&addr.addr_bytes, 6);
		
		printf("BusId:%s  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X   IPv4: %u.%u.%u.%u\n", 
				interaface->Port, 
				interaface->MacAddr[0] & 0xFF, interaface->MacAddr[1] & 0xFF, interaface->MacAddr[2] & 0xFF, 
				interaface->MacAddr[3] & 0xFF, interaface->MacAddr[4] & 0xFF, interaface->MacAddr[5] & 0xFF,
				(interaface->IPv4) & 0xFF, (interaface->IPv4 >> 8) & 0xFF, (interaface->IPv4 >> 16) & 0xFF, (interaface->IPv4 >> 24) & 0xFF 
			);
		
		
		dpe__log( 0, __FILE__, __LINE__, "BusId:%s  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", interaface->Port, interaface->MacAddr[0] & 0xFF, interaface->MacAddr[1] & 0xFF, interaface->MacAddr[2] & 0xFF, interaface->MacAddr[3] & 0xFF, interaface->MacAddr[4] & 0xFF, interaface->MacAddr[5] & 0xFF);
	}

	uint16_t mtu = 0;
	ret = rte_eth_dev_get_mtu( interaface->PortId, &mtu);
	
	if (0 != ret) 
	{
		mtu = 1500;
	} 
	else 
	{
		if( mtu > 1500) 
		{
			ret = rte_eth_dev_set_mtu( interaface->PortId, (uint16_t)1500);
			
			if( ret != 0) 
			{
				printf( "mtu size set failed for BusId=%s\n", interaface->Port);
				exit(0);
			}
		}
	}
	

	ret = rte_eth_dev_info_get( interaface->PortId, &dev_info);

	if (0 != ret) 
	{
		printf( "rte_eth_dev_info_get(%s) failed\n", interaface->Port);
		exit(0);
	}
	
	printf("BusId:%s if_index=%d  max_rx_queues=%d  max_tx_queues=%d\n", 
		interaface->Port, dev_info.if_index, dev_info.max_rx_queues, dev_info.max_tx_queues);

	
	
	if( interaface->RecvQueue > dev_info.max_rx_queues)
	{
		printf( "Error: BusId:%s, configured RQ=%d is > Supported RQ=%d \n", interaface->Port, interaface->RecvQueue, dev_info.max_rx_queues);
		exit(0);
	}

	if( interaface->SendQueue > dev_info.max_tx_queues)
	{
		printf( "Error: BusId:%s, configured SQ=%d is > Supported SQ=%d \n", interaface->Port, interaface->SendQueue, dev_info.max_tx_queues);
		exit(0);
	}


	interaface->mempool = NULL;
	interaface->mempool = (struct rte_mempool **)malloc(sizeof(struct rte_mempool *) * interaface->RecvQueue);
	if(!interaface->mempool)
	{
		printf("Error: memory allocation failed for port: BusId=%s\n", interaface->Port);
	}
	
	
	
	int mi = 0;
	char mempool_name[30];
	
	for( mi = 0; mi < interaface->RecvQueue; mi++)
	{
		memset( mempool_name, 0, sizeof(mempool_name));
		sprintf( mempool_name, "mempool--%d-%d", interaface->Index, mi);
		
		//printf("MemPoolSize=%u\n", interaface->MemPoolSize);
		
		interaface->mempool[mi] = rte_pktmbuf_pool_create( mempool_name, interaface->MemPoolSize, interaface->MemPoolCacheSize, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

		
		if(!interaface->mempool[mi])
		{
			printf("Error: memory allocation failed for port: BusId=%s Index=%u  mi=%u\n", interaface->Port, interaface->Index, mi);
			exit(0);
		}
	}
	

	memset( &port_conf, 0, sizeof(struct rte_eth_conf));
	port_conf.rxmode.mq_mode 				= RTE_ETH_MQ_RX_RSS;
	//port_conf.rxmode.max_rx_pkt_len 		= RTE_ETHER_MAX_LEN;
	//port_conf.rxmode.jumbo_frame 			= 1;
	port_conf.txmode.mq_mode 				= RTE_ETH_MQ_TX_NONE;

	//port_conf.rx_adv_conf.rss_conf.rss_hf 	= (RTE_ETH_RSS_L3_DST_ONLY | RTE_ETH_RSS_GTPU);
	//port_conf.rx_adv_conf.rss_conf.rss_hf	= RTE_ETH_RSS_IP | RTE_ETH_RSS_IPV4 | RTE_ETH_RSS_IPV6;
	//port_conf.rx_adv_conf.rss_conf.rss_key 	= NULL,
	//port_conf.rx_adv_conf.rss_conf.rss_hf	= RTE_ETH_RSS_IP | RTE_ETH_RSS_IPV4 | RTE_ETH_RSS_IPV6;
	//port_conf.rx_adv_conf.rss_conf.rss_hf	= RTE_ETH_RSS_PROTO_MASK;
	port_conf.rx_adv_conf.rss_conf.rss_hf	= RTE_ETH_RSS_IP;
	
	printf("RTE_ETH_RSS_PROTO_MASK=%ld %ld  %ld %ld %ld\n", 
		RTE_ETH_RSS_PROTO_MASK, RTE_ETH_RSS_IP | RTE_ETH_RSS_IPV4 | RTE_ETH_RSS_IPV6,
		RTE_ETH_RSS_IP, RTE_ETH_RSS_IPV4, RTE_ETH_RSS_IPV6);

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
	{
		port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
		//port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_IPV4_CKSUM;
		//port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_IPV6_CKSUM;
	}

	//unsigned int rss_hf = port_conf.rx_adv_conf.rss_conf.rss_hf;
	//port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads | ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
	port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;
	//port_conf.rx_adv_conf.rss_conf.rss_hf &= (dev_info.flow_type_rss_offloads | ETH_RSS_IP);


	ret = rte_eth_dev_configure( interaface->PortId, interaface->RecvQueue, interaface->SendQueue, &port_conf);
	if (ret != 0)
	{
		printf( "rte_eth_dev_configure(%s(%d), rx=%u, tx=%u ) failed   %s|%s|%d\n", 
			interaface->Port, interaface->PortId, interaface->RecvQueue, interaface->SendQueue, __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}

	uint16_t rx_sz = 1024;
	uint16_t tx_sz = 1024;
	ret = rte_eth_dev_adjust_nb_rx_tx_desc( interaface->PortId, &rx_sz, &tx_sz);

	if (ret != 0)
	{
		printf( "rte_eth_dev_adjust_nb_rx_tx_desc(%s(%d), rx=%u, tx=%u ) failed     %s|%s|%d\n", interaface->Port, interaface->PortId, rx_sz, tx_sz, __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}


	int i = 0;
	for( i = 0; i < interaface->RecvQueue; i++ ) 
	{
		ret = rte_eth_rx_queue_setup( interaface->PortId, i, rx_sz, rte_eth_dev_socket_id( interaface->PortId), NULL, interaface->mempool[i]);
	
		if (0 != ret)
		{
			printf( "rte_eth_rx_queue_setup failed ifid=%s port_id=%u rx_q_count=%u  %s|%s|%d\n", interaface->Port, interaface->PortId, interaface->RecvQueue, __FILE__, __FUNCTION__, __LINE__);
			exit(0);
		}
	}


	tx_conf 			= dev_info.default_txconf;
	tx_conf.offloads 	= port_conf.txmode.offloads;

	i = 0;
	for( i = 0; i < interaface->SendQueue; i++ ) 
	{
		ret = rte_eth_tx_queue_setup( interaface->PortId, i, tx_sz, rte_eth_dev_socket_id( interaface->PortId), &tx_conf);
		
		if (0 != ret)
		{
			printf( "rte_eth_tx_queue_setup failed ifid=%s port_id=%u tx_q_count=%u  %s|%s|%d\n", interaface->Port, interaface->PortId, interaface->SendQueue, __FILE__, __FUNCTION__, __LINE__);
			exit(0);
		}
	}
	
	
	interaface->recv_ring = (struct rte_ring **)malloc(sizeof(struct rte_ring *) * interaface->RecvQueue);
	
	char recv_ring_name[30];
	i = 0;
	
	for( i = 0; i < interaface->RecvQueue; i++ )
	{
		memset( recv_ring_name, 0, sizeof(recv_ring_name));
		sprintf( recv_ring_name, "recv-ring-%d-%d", interaface->Index, i);
		
		interaface->recv_ring[i] = rte_ring_create( recv_ring_name, rte_align32pow2(interaface->MemPoolSize), rte_socket_id(), RING_F_MP_HTS_ENQ | RING_F_MC_HTS_DEQ);
	
		if(!interaface->recv_ring[i])
		{
			printf( "rte_ring_create failed for recv BusId=%s at %d   %s|%s|%d\n", interaface->Port, i, __FILE__, __FUNCTION__, __LINE__);
			exit(0);
		}
	}


	//if( interaface->QosInitalized == 1)
	if( interaface->QosEnabled == 1)
	{
		memset( recv_ring_name, 0, sizeof(recv_ring_name));
		sprintf( recv_ring_name, "sched-ring-%d-%d", interaface->Index, i);
		
		interaface->sched_ring = rte_ring_create( recv_ring_name, rte_align32pow2(interaface->MemPoolSize), rte_socket_id(), RING_F_MP_HTS_ENQ | RING_F_MC_HTS_DEQ);
	
		if(!interaface->sched_ring)
		{
			printf( "rte_ring_create failed for sched BusId=%s at %d   %s|%s|%d\n", interaface->Port, i, __FILE__, __FUNCTION__, __LINE__);
			exit(0);			
		}
	}
	
	
	interaface->send_ring = (struct rte_ring **)malloc(sizeof(struct rte_ring *) * interaface->SendQueue);
	
	char send_ring_name[30];
	i = 0;
	
	for( i = 0; i < interaface->SendQueue; i++ )
	{
		memset( send_ring_name, 0, sizeof(send_ring_name));
		sprintf( send_ring_name, "send-ring-%d-%d", interaface->Index, i);
		
		interaface->send_ring[i] = rte_ring_create( send_ring_name, rte_align32pow2(interaface->MemPoolSize), rte_socket_id(), RING_F_MP_HTS_ENQ  | RING_F_MC_HTS_DEQ);
	
		if(!interaface->send_ring[i])
		{
			printf( "rte_ring_create failed for send BusId=%s at %d   %s|%s|%d\n", interaface->Port, i, __FILE__, __FUNCTION__, __LINE__);
			exit(0);
		}
	}
	
	
	
	i = 0;
	for( i = 0; i < interaface->RecvQueue; i++ ) 
	{
		dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
		
		interaface_port->interface = interaface;
		interaface_port->queue_id = i;
		interaface_port->port_type = 1;
		
		rte_eal_remote_launch( dpe__receive, 		interaface_port, __dpe->last_lcore++);
		printf("launched core for dpe__receive for Port=%d index=%d   %s|%s|%d\n", interaface->PortId, i, __FILE__, __FUNCTION__, __LINE__);
	}
	
	usleep( 111111);
	
	i = 0;
	int j = 0;
	
	for( i = 0; i < (interaface->RecvWorkerCount * interaface->RecvWorkerCountPerQ); i++ )
	{
		j = 0;
		for( j = 0; j < interaface->RecvWorkerCountPerQ; j++ )
		{
			dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
			
			interaface_port->interface 			= interaface;
			interaface_port->queue_id 			= i % interaface->RecvQueue;
			interaface_port->port_type 			= 1;
			interaface_port->recv_worker_index 	= __dpe->recv_worker_index;
			
			__dpe->recv_worker_index++;
			
			rte_eal_remote_launch( dpe__recv_worker,  	interaface_port, __dpe->last_lcore++);
			
			printf("launched core for dpe__recv_worker for Port=%d index=%d   %s|%s|%d\n", interaface->PortId, i, __FILE__, __FUNCTION__, __LINE__);
		}
	}
	
	usleep( 111111);
	
	if( interaface->QosEnabled == 1)
	{
		rte_eal_remote_launch( dpe__sched_port,  	interaface, __dpe->last_lcore++);
	}
	
	usleep( 111111);

	i = 0;
	for( i = 0; i < interaface->SendQueue; i++ ) 
	{
		dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
		
		interaface_port->interface = interaface;
		interaface_port->queue_id = i;
		interaface_port->port_type = 2;
		
		rte_eal_remote_launch( dpe__send, 			interaface_port, __dpe->last_lcore++);
	
		printf("launched core for dpe__send for Port=%d index=%d   %s|%s|%d\n", interaface->PortId, i, __FILE__, __FUNCTION__, __LINE__);
	}
	
	// i = 0;
	// for( i = 0; i < interaface->SendWorkerCount; i++ )
	// {
		// dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
		
		// interaface_port->interface = interaface;
		// interaface_port->queue_id = i % interaface->SendQueue;
		// interaface_port->port_type = 2;
		
		// rte_eal_remote_launch( dpe__send_worker,  	interaface_port, __dpe->last_lcore++);
	// }

	if( ptrPacketGeneratorFunction > 0)
	{
		i = 0;
		for( i = 0; i < interaface->PacketGeneratorCount; i++ )
		{
			dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
			
			interaface_port->interface = interaface;
			interaface_port->queue_id = i % interaface->RecvQueue;
			interaface_port->port_type = 2;
			
			rte_eal_remote_launch( dpe__generator_worker,  	interaface_port, __dpe->last_lcore++);
		}
	}

}

void dpe__engine_start( dpdk_interface_t * interaface)
{
	int ret = 0;

	struct rte_ether_addr addr = {.addr_bytes = "\x33\x33\x00\x00\x00\x01"};
	ret = rte_eth_dev_mac_addr_add( interaface->PortId, &addr, 0);
	
	struct rte_ether_addr addr2 = {.addr_bytes = "\x33\x33\x00\x00\x00\x02"};
	ret = rte_eth_dev_mac_addr_add( interaface->PortId, &addr2, 0);
	
	struct rte_ether_addr addr3 = {.addr_bytes = "\x33\x33\xFF\x00\x00\x01"};
	ret = rte_eth_dev_mac_addr_add( interaface->PortId, &addr3, 0);
	
	//printf("ret=%d\n", ret);

	
	// ret = rte_eth_promiscuous_enable( interaface->PortId);
	// if (0 != ret)
	// {
		// printf( "rte_eth_promiscuous_enable failed for  ifid=%s  port_id=%u  %s|%s|%d\n", interaface->Port, interaface->PortId, __FILE__, __FUNCTION__, __LINE__);
		// exit(0);
	// }	
	
	
	printf( "multicast_get port_id=%u status=%d  %s|%s|%d\n", interaface->PortId, rte_eth_allmulticast_get( interaface->PortId), __FILE__, __FUNCTION__, __LINE__);
	rte_eth_allmulticast_enable( interaface->PortId);
	printf( "multicast_get port_id=%u status=%d  %s|%s|%d\n", interaface->PortId, rte_eth_allmulticast_get( interaface->PortId), __FILE__, __FUNCTION__, __LINE__);


	ret = rte_eth_dev_start( interaface->PortId);
	if (0 != ret)
	{
		printf( "rte_eth_dev_start failed for  ifid=%s  port_id=%u  %s|%s|%d\n", interaface->Port, interaface->PortId, __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}


	interaface->started = 1;
}


void dpe__add_ranip_to_accessinterface( uint32_t ip)
{
	printf("ip=%u\n", ip);

	int i = 0;
	int bFound = 0;
	int bHasMac = 0;
	
	while( i < __dpe->arp_table_row_count)
	{
		if( __dpe->arp_table[i].ip == ip)
		{
			bFound = 1;
			if( __dpe->arp_table[i].Mac[5] != 0x0)
			{
				bHasMac = 1;
			}
			break;
		}
		i++;
	}
	
	if( bFound == 0)
	{
		pthread_mutex_lock( &__dpe->arp_table_lock );

		i = 0;
		while( i < __dpe->arp_table_row_count)
		{
			if( __dpe->arp_table[i].ip == ip)
			{
				bFound = 1;
				if( __dpe->arp_table[i].Mac[5] != 0x0)
				{
					bHasMac = 1;
				}
				break;
			}
			i++;
		}
	
		if( bFound == 0 && __dpe->arp_table_row_count < 100)
		{
			__dpe->arp_table[__dpe->arp_table_row_count].ip = ip;
			__dpe->arp_table_row_count++;
		}
	
		pthread_mutex_unlock( &__dpe->arp_table_lock );
	}
	
	if( bHasMac == 0)
	{
		dpdk_interface_t * interaface = __dpe->Head;
		
		while( interaface)
		{
			if( interaface->ProcessType == PROCESS_TYPE__GTP_Gi)
			{
				break;
			}
			interaface = interaface->Next;
		}
	
		if( interaface)
		{
			struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interaface->mempool[0]);
			dpdk_pkt__encode_arp_request( m_buf, interaface->MacAddr, interaface->IPv4, ip);
		}
	}
}


uint32_t dpe__engine_get_linkspeed( dpdk_interface_t * interaface)
{
	struct rte_eth_link link;
	memset( &link, 0, sizeof(struct rte_eth_link));
	int link_get_err = -EINVAL;
	link_get_err = rte_eth_link_get_nowait( interaface->PortId, &link);

	printf("Port: link_get_err=%u link_status=%u\n", link_get_err, link.link_status);

	if (link_get_err >= 0 && link.link_status) 
	{
		//const char *dp = (link.link_duplex == RTE_ETH_LINK_FULL_DUPLEX) ? "full-duplex" : "half-duplex";
		//printf("Port: %s(%u) Link Up - speed %d - %s\n", interaface->Port, interaface->PortId, link.link_speed, dp);
		return link.link_speed;
	}
	
	return 0;
}

void dpe__engine_print_linkspeed( dpdk_interface_t * interaface)
{
	struct rte_eth_link link;
	memset( &link, 0, sizeof(struct rte_eth_link));
	int link_get_err = -EINVAL;
	link_get_err = rte_eth_link_get_nowait( interaface->PortId, &link);

	printf("Port: link_get_err=%u link_status=%u\n", link_get_err, link.link_status);

	if (link_get_err >= 0 && link.link_status) 
	{
		const char *dp = (link.link_duplex == RTE_ETH_LINK_FULL_DUPLEX) ? "full-duplex" : "half-duplex";
		printf("Port: %s(%u) Link Up - speed %d - %s\n", interaface->Port, interaface->PortId, link.link_speed, dp);
	}	
}



void * dpe__run_pref_thread( void * args)
{
	dpdk_interface_t * interaface = __dpe->Head;

	uint64_t sent_c 	= 0;
	uint64_t received_c = 0;
	uint64_t dequeed_c 	= 0;
	uint64_t dropped_c 	= 0;
	uint64_t enqueed_c 	= 0;
	
	uint64_t eth_last_sent 		= 0;
	uint64_t eth_last_receved 	= 0;
	uint64_t eth_last_ierrors 	= 0;
	uint64_t eth_last_oerrors 	= 0;
	uint64_t eth_last_rx_nombuf = 0;
	
	int j = 0;
	
	char dateString[27];

	int x = 0;
	struct rte_eth_stats stats;
	
	char * portType = "GTP-Gi";
	
	while( __dpe->runing)
	{
		interaface = __dpe->Head;

		sent_c = 0;
		received_c = 0;

		memset( &dateString, '\0', sizeof( dateString));
		dpe__makeTimeStamp( &dateString[0]);
		
		dpe__perf_log("%s\n", dateString);
		
		while( interaface)
		{
			received_c = interaface->total_received - interaface->last_received;
			sent_c = interaface->total_sent - interaface->last_sent;
			dequeed_c = interaface->total_dequeed - interaface->last_dequeed;
			dropped_c = interaface->total_dropped - interaface->last_dropped;
			enqueed_c = interaface->total_enqueed - interaface->last_enqueed;
			
			/*
			dpe__perf_log( "Port: %s PType:%u  recv:%8lu %10lu  sent:%8lu %10lu   dequeed:%8lu %10lu   dropped:%8lu %10lu   enqueed:%8lu %10lu\n", 
				interaface->Port, interaface->ProcessType, received_c, interaface->total_received, sent_c, interaface->total_sent, 
				dequeed_c, interaface->total_dequeed,
				dropped_c, interaface->total_dropped,
				enqueed_c, interaface->total_enqueed);
			*/
			
			if( interaface->ProcessType == 1)
				portType = "GTP-Gi RAN-UPF";
			else if( interaface->ProcessType == 2)
				portType = "Gi-GTP ISP-UPF";
			else if( interaface->ProcessType == 3)
				portType = "Gi-Gi";
			
			dpe__perf_log( "IP:%s  Port: %s Type:%s:%u\n", interaface->SIPv4, interaface->Port, portType, interaface->ProcessType);
			dpe__perf_log( "    Packets Received 				: %8lu %10lu\n", received_c, interaface->total_received);
			dpe__perf_log( "    Worker Core Dequeued Pkts 		: %8lu %10lu\n", dequeed_c, interaface->total_dequeed);		// dequeued by worker, which enqueed by Receive core
			dpe__perf_log( "    Worker Core Enqueed  Pkts 		: %8lu %10lu\n", enqueed_c, interaface->total_enqueed);		// elgible for next stage, worker core will enguee
			dpe__perf_log( "    Worker Core Dropped  Pkts 		: %8lu %10lu\n", dropped_c, interaface->total_dropped); 	// not-elgible for next stage, worker core will drop
			dpe__perf_log( "    Packets Transmitted 			: %8lu %10lu\n", sent_c, interaface->total_sent);

			dpe__perf_log( "    ARP Request Sent 				: %8lu   ARP Response Received  	: %8lu\n", interaface->arp_request_sent, interaface->arp_response_received);
			dpe__perf_log( "    ARP Request Received 			: %8lu   ARP Response Sent  		: %8lu\n", interaface->arp_request_received, interaface->arp_response_sent);
			// dpe__perf_log( "    ICMP Request Sent 		: %8lu  ICMP Response Received  : %8lu\n", interaface->icmp_request_sent, interaface->icmp_response_received);
			// dpe__perf_log( "    ICMP Request Received 	: %8lu  ICMP Response Sent  	: %8lu\n", interaface->icmp_request_received, interaface->icmp_response_sent);
			//dpe__perf_log("\n");

			dpe__perf_log("\n");
			
			//if( x == 10)
			{
				memset( &stats, 0, sizeof(struct rte_eth_stats));
				
				if( rte_eth_stats_get( interaface->PortId, &stats) == 0)
				{
					int zx = 0;
					//for( zx = 0; zx <  interaface->RecvQueue; zx++)
					{
						eth_last_sent 		= stats.opackets 	- interaface->eth_last_sent;
						eth_last_receved 	= stats.ipackets 	- interaface->eth_last_receved;
						eth_last_ierrors 	= stats.ierrors 	- interaface->eth_last_ierrors;
						eth_last_oerrors 	= stats.oerrors 	- interaface->eth_last_oerrors;
						eth_last_rx_nombuf 	= stats.rx_nombuf 	- interaface->eth_last_rx_nombuf;
						
						dpe__perf_log( "        Stats  Port: %s  ipackets: %lu - %lu     opackets: %lu - %lu     ierrors: %lu - %lu     oerrors: %lu - %lu     rx_nombuf: %lu - %lu\n", 
							interaface->Port, 
								eth_last_receved, stats.ipackets, 
								eth_last_sent, stats.opackets,
								eth_last_ierrors, stats.ierrors,
								eth_last_oerrors, stats.oerrors,
								eth_last_rx_nombuf, stats.rx_nombuf
							);
					
						interaface->eth_last_sent 		= stats.opackets;
						interaface->eth_last_receved 	= stats.ipackets;
						interaface->eth_last_ierrors 	= stats.ierrors;
						interaface->eth_last_oerrors 	= stats.oerrors;
						interaface->eth_last_rx_nombuf 	= stats.rx_nombuf;
						
					}
					
				}
			}
			
			interaface->last_received 	= interaface->total_received;
			interaface->last_sent 		= interaface->total_sent;
			interaface->last_dequeed	= interaface->total_dequeed;
			interaface->last_dropped	= interaface->total_dropped;
			interaface->last_enqueed	= interaface->total_enqueed;
			
			interaface = interaface->Next;
			
			dpe__perf_log("\n");
		}
		
		if( x == 10)
		{
			x= 0;
		}
		
		dpe__perf_log("--------------------------------------------------------------------------------------------------------------------------------------------------\n");

		
		for( j = 0; j < __dpe->EnablePerfLogsTimeSpan; j++)
		{
			if(!__dpe->runing)
				break;
			
			usleep( 999999);
		}
		
		
		x++;
	}

	return NULL;
}

dpdk_interface_t * dpe__getinterface( int index)
{
	return __dpe->Head;
}

static int
lcore_hello(__attribute__((unused)) void *arg)
{
	unsigned lcore_id;
	lcore_id = rte_lcore_id();
	printf("hello from core %u\n", lcore_id);
	return 0;
}


void dpe__init_engine_test( int argc, char **argv)
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}

	
}

void dpe__init_engine2( int argc, char **argv)
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
	struct rte_mempool * mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if( mpool)
	{
		struct rte_mbuf * m_buf = NULL;
		while(1)
		{
			m_buf = rte_pktmbuf_alloc( mpool);
				
			if( m_buf)
			{
				rte_pktmbuf_free( m_buf);
				dpe__scnt++;
			}
		}
	}
}


void dpe__init_engine3( int argc, char **argv)
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
	struct rte_mempool * mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if( mpool)
	{
		struct rte_mbuf * m_buf = NULL;
		while(1)
		{
			m_buf = rte_pktmbuf_alloc( mpool);
				
			if( m_buf)
			{
				
				
	int protocol = 6;
	int blen = 100;
	int gtpseqno = 10;
	int tcpseqno = 20;

	
	int prependLen 		= 0;
	int gtpHeaderLen 	= (gtpseqno > 0) ? 12 : 8;
	
	if( protocol == 1)
	{
		if(gtpseqno > 0)
			gtpHeaderLen		+= 4;
		else
			gtpHeaderLen		+= 8;
		
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + blen;
	}
	else if( protocol == 6)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 20 + blen;
	}
	else if( protocol == 17)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 8 + blen;
	}

	m_buf->pkt_len  	= prependLen;
	m_buf->data_len 	= prependLen;

	unsigned char * packet = rte_pktmbuf_mtod( m_buf, unsigned char *);
	
	struct rte_ether_hdr 	* eth 			= (struct rte_ether_hdr *)packet;
	struct rte_udp_hdr 		* udp  			= NULL;
	struct rte_ipv4_hdr 	* ip_hdr 		= NULL;
	struct rte_ipv4_hdr 	* usr_ip_hdr 	= NULL;
	struct rte_udp_hdr 		* usr_udp  		= NULL;
	
	eth->src_addr.addr_bytes[0] = 0x01;
	eth->src_addr.addr_bytes[1] = 0x02;
	eth->src_addr.addr_bytes[2] = 0x03;
	eth->src_addr.addr_bytes[3] = 0x04;
	eth->src_addr.addr_bytes[4] = 0x05;
	eth->src_addr.addr_bytes[5] = 0x06;
	
	eth->dst_addr.addr_bytes[0] = 0x07;
	eth->dst_addr.addr_bytes[1] = 0x08;
	eth->dst_addr.addr_bytes[2] = 0x09;
	eth->dst_addr.addr_bytes[3] = 0x10;
	eth->dst_addr.addr_bytes[4] = 0x11;
	eth->dst_addr.addr_bytes[5] = 0x12;
	
	eth->ether_type = 8;


	ip_hdr = (struct rte_ipv4_hdr *)&packet[14];
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	ip_hdr->total_length 	= rte_cpu_to_be_16(prependLen - 14);
	
	ip_hdr->packet_id 		= tcpseqno + (prependLen - 14);
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	ip_hdr->hdr_checksum 	= 0;
	
	ip_hdr->src_addr 		= 10929;
	ip_hdr->dst_addr 		= rte_cpu_to_be_32( 123456);
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);

	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

	udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
	udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
	udp->dgram_cksum			= 0;

	udp->dgram_len 			= rte_cpu_to_be_16( prependLen - (14 + 20));
	udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	struct rte_gtp_hdr * gtp  	= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 0xFF;
	gtp->plen 					= rte_cpu_to_be_16( prependLen - (14 + 20 + 8));
	gtp->teid 					= rte_be_to_cpu_32( 786783); 

	if( gtpseqno > 0)
	{
		gtp->gtp_hdr_info 							= 0x32;
		*(uint16_t *)(((unsigned char *)gtp) + 8) 	= rte_be_to_cpu_16( gtpseqno);
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr) + 4);
	}
	else
	{
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr));
	}

	if( protocol == 1)
	{
		char * dx = ((char *)&gtp->gtp_hdr_info);
		dx[0] |= 1 << 2;
		
		if( gtpseqno > 0)
		{
			char * dPtr = ((char *)usr_ip_hdr) - 1;
			dPtr[0] = 0x85;
			dPtr[1] = 0x01;
			dPtr[2] = 0x10;
			dPtr[3] = 0x01;
			dPtr[4] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[5];
		}
		else
		{
			char * dPtr = ((char *)usr_ip_hdr);
			dPtr[0] = 0x00;
			dPtr[1] = 0x00;
			dPtr[2] = 0x00;
			dPtr[3] = 0x85;
			dPtr[4] = 0x01;
			dPtr[5] = 0x10;
			dPtr[6] = 0x01;
			dPtr[7] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[8];
		}
	}
	
	
	
	usr_ip_hdr->version_ihl 	= 0x45;
	usr_ip_hdr->type_of_service = 0x00;
	usr_ip_hdr->total_length 	= rte_cpu_to_be_16( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->packet_id 		= tcpseqno + ( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->fragment_offset = 0;
	usr_ip_hdr->time_to_live 	= 64;
	usr_ip_hdr->next_proto_id 	= protocol;			//17 UDP,  6 TCP
	usr_ip_hdr->hdr_checksum 	= 0;
	
	usr_ip_hdr->src_addr 		= rte_cpu_to_be_32( 32323);
	usr_ip_hdr->dst_addr 		= rte_cpu_to_be_32( 5454545);
	
	//usr_ip_hdr->hdr_checksum 	= 0;
	//usr_ip_hdr->hdr_checksum 	= rte_ipv4_cksum( usr_ip_hdr);

	if( protocol == 6 )
	{
		struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		
		tcp->src_port				= rte_cpu_to_be_16( 6665);
		tcp->dst_port				= rte_cpu_to_be_16( 6656);
		tcp->cksum					= 0;
		tcp->sent_seq				= tcpseqno;
		tcp->data_off				= 0x50;
		tcp->tcp_flags				= 0x18;
		tcp->rx_win					= 0x4410;
		//tcp->cksum					= rte_ipv4_udptcp_cksum( usr_ip_hdr, tcp);
	}
	else if( protocol == 17)
	{
		usr_udp = (struct rte_udp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		usr_udp->src_port			= rte_cpu_to_be_16( 43434);
		usr_udp->dst_port			= rte_cpu_to_be_16( 5454);
		usr_udp->dgram_cksum		= 0;

		//usr_udp->dgram_len 			= rte_cpu_to_be_16( blen + 8);
		//usr_udp->dgram_cksum		= rte_ipv4_udptcp_cksum( usr_ip_hdr, usr_udp);
	}
	else
	{
		char * dptr = ((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		//memcpy( dptr, buffer, blen);
	}				
				
				
				rte_pktmbuf_free( m_buf);
				dpe__scnt++;
			}
		}
	}
}


int dpe__init_engine4_worker( void * arg)
{
	struct rte_mempool * mpool = rte_pktmbuf_pool_create( "mpool1", 131072, 512, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if( mpool)
	{
		printf("created lcore %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		
		struct rte_mbuf * m_buf = NULL;
		while(1)
		{
			m_buf = rte_pktmbuf_alloc( mpool);
				
			if( m_buf)
			{
				
				
	int protocol = 6;
	int blen = 100;
	int gtpseqno = 10;
	int tcpseqno = 20;

	
	int prependLen 		= 0;
	int gtpHeaderLen 	= (gtpseqno > 0) ? 12 : 8;
	
	if( protocol == 1)
	{
		if(gtpseqno > 0)
			gtpHeaderLen		+= 4;
		else
			gtpHeaderLen		+= 8;
		
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + blen;
	}
	else if( protocol == 6)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 20 + blen;
	}
	else if( protocol == 17)
	{
		prependLen 			= 14 + 20 + 8 + gtpHeaderLen + 20 + 8 + blen;
	}

	m_buf->pkt_len  	= prependLen;
	m_buf->data_len 	= prependLen;

	unsigned char * packet = rte_pktmbuf_mtod( m_buf, unsigned char *);
	
	struct rte_ether_hdr 	* eth 			= (struct rte_ether_hdr *)packet;
	struct rte_udp_hdr 		* udp  			= NULL;
	struct rte_ipv4_hdr 	* ip_hdr 		= NULL;
	struct rte_ipv4_hdr 	* usr_ip_hdr 	= NULL;
	struct rte_udp_hdr 		* usr_udp  		= NULL;
	
	eth->src_addr.addr_bytes[0] = 0x01;
	eth->src_addr.addr_bytes[1] = 0x02;
	eth->src_addr.addr_bytes[2] = 0x03;
	eth->src_addr.addr_bytes[3] = 0x04;
	eth->src_addr.addr_bytes[4] = 0x05;
	eth->src_addr.addr_bytes[5] = 0x06;
	
	eth->dst_addr.addr_bytes[0] = 0x07;
	eth->dst_addr.addr_bytes[1] = 0x08;
	eth->dst_addr.addr_bytes[2] = 0x09;
	eth->dst_addr.addr_bytes[3] = 0x10;
	eth->dst_addr.addr_bytes[4] = 0x11;
	eth->dst_addr.addr_bytes[5] = 0x12;
	
	eth->ether_type = 8;


	ip_hdr = (struct rte_ipv4_hdr *)&packet[14];
	
	ip_hdr->version_ihl 	= 0x45;
	ip_hdr->type_of_service = 0x00;
	ip_hdr->total_length 	= rte_cpu_to_be_16(prependLen - 14);
	
	ip_hdr->packet_id 		= tcpseqno + (prependLen - 14);
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	ip_hdr->hdr_checksum 	= 0;
	
	ip_hdr->src_addr 		= 10929;
	ip_hdr->dst_addr 		= rte_cpu_to_be_32( 123456);
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);

	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

	udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
	udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
	udp->dgram_cksum			= 0;

	udp->dgram_len 			= rte_cpu_to_be_16( prependLen - (14 + 20));
	udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	struct rte_gtp_hdr * gtp  	= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 0xFF;
	gtp->plen 					= rte_cpu_to_be_16( prependLen - (14 + 20 + 8));
	gtp->teid 					= rte_be_to_cpu_32( 786783); 

	if( gtpseqno > 0)
	{
		gtp->gtp_hdr_info 							= 0x32;
		*(uint16_t *)(((unsigned char *)gtp) + 8) 	= rte_be_to_cpu_16( gtpseqno);
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr) + 4);
	}
	else
	{
		usr_ip_hdr									= (struct rte_ipv4_hdr *)(((unsigned char *)gtp) + sizeof(struct rte_gtp_hdr));
	}

	if( protocol == 1)
	{
		char * dx = ((char *)&gtp->gtp_hdr_info);
		dx[0] |= 1 << 2;
		
		if( gtpseqno > 0)
		{
			char * dPtr = ((char *)usr_ip_hdr) - 1;
			dPtr[0] = 0x85;
			dPtr[1] = 0x01;
			dPtr[2] = 0x10;
			dPtr[3] = 0x01;
			dPtr[4] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[5];
		}
		else
		{
			char * dPtr = ((char *)usr_ip_hdr);
			dPtr[0] = 0x00;
			dPtr[1] = 0x00;
			dPtr[2] = 0x00;
			dPtr[3] = 0x85;
			dPtr[4] = 0x01;
			dPtr[5] = 0x10;
			dPtr[6] = 0x01;
			dPtr[7] = 0x00;
			usr_ip_hdr = (struct rte_ipv4_hdr *)&dPtr[8];
		}
	}
	
	
	
	usr_ip_hdr->version_ihl 	= 0x45;
	usr_ip_hdr->type_of_service = 0x00;
	usr_ip_hdr->total_length 	= rte_cpu_to_be_16( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->packet_id 		= tcpseqno + ( prependLen - (14 + 20 + 8 + gtpHeaderLen));
	usr_ip_hdr->fragment_offset = 0;
	usr_ip_hdr->time_to_live 	= 64;
	usr_ip_hdr->next_proto_id 	= protocol;			//17 UDP,  6 TCP
	usr_ip_hdr->hdr_checksum 	= 0;
	
	usr_ip_hdr->src_addr 		= rte_cpu_to_be_32( 32323);
	usr_ip_hdr->dst_addr 		= rte_cpu_to_be_32( 5454545);
	
	//usr_ip_hdr->hdr_checksum 	= 0;
	usr_ip_hdr->hdr_checksum 	= rte_ipv4_cksum( usr_ip_hdr);

	if( protocol == 6 )
	{
		struct rte_tcp_hdr * tcp = (struct rte_tcp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		
		tcp->src_port				= rte_cpu_to_be_16( 6665);
		tcp->dst_port				= rte_cpu_to_be_16( 6656);
		tcp->cksum					= 0;
		tcp->sent_seq				= tcpseqno;
		tcp->data_off				= 0x50;
		tcp->tcp_flags				= 0x18;
		tcp->rx_win					= 0x4410;
		//tcp->cksum					= rte_ipv4_udptcp_cksum( usr_ip_hdr, tcp);
	}
	else if( protocol == 17)
	{
		usr_udp = (struct rte_udp_hdr *)((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		usr_udp->src_port			= rte_cpu_to_be_16( 43434);
		usr_udp->dst_port			= rte_cpu_to_be_16( 5454);
		usr_udp->dgram_cksum		= 0;

		//usr_udp->dgram_len 			= rte_cpu_to_be_16( blen + 8);
		//usr_udp->dgram_cksum		= rte_ipv4_udptcp_cksum( usr_ip_hdr, usr_udp);
	}
	else
	{
		char * dptr = ((unsigned char *)usr_ip_hdr + sizeof(struct rte_ipv4_hdr));
		//memcpy( dptr, buffer, blen);
	}				
				
				
				rte_pktmbuf_free( m_buf);
				dpe__scnt++;
			}
		}
	}
	
	return 0;	
}

void dpe__init_engine4( int argc, char **argv)
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
	rte_eal_remote_launch( dpe__init_engine4_worker,  NULL, last_lcore++);
}


dpdk_interface_port_t * dpe__find_pkgen_interface( uint32_t atIndex)
{
	dpdk_interface_t * interaface = __dpe->Head;
	
	while( interaface)
	{
		if( interaface->PacketGeneratorCount > 0)
		{
			dpdk_interface_port_t * interaface_port = (dpdk_interface_port_t*)malloc(sizeof(dpdk_interface_port_t));
			
			interaface_port->interface = interaface;
			interaface_port->queue_id = atIndex; // should be at send q.//  i % interaface->RecvQueue;
			interaface_port->port_type = 2;			
		
			return interaface_port;
		}
		interaface = interaface->Next;
	}
	return NULL;
}	


int dpe__sched_port( void * interafaceArgs)
{
	dpdk_interface_t * interface = (dpdk_interface_t *)interafaceArgs;
	
	int side = 1;
	
	if( interface->ProcessType == PROCESS_TYPE__Gi_GTP || interface->ProcessType == PROCESS_TYPE__Gi_Gi_EGRESS)
	{
		side = 2;
	}
	

	char rte_sched_port_name[100];
	memset( rte_sched_port_name, 0, sizeof(rte_sched_port_name));
	sprintf( rte_sched_port_name, "sched-port-%d-%d", interface->Index, side);
	

	uint32_t linkspeed = dpe__engine_get_linkspeed( interface);
	interface->sched_port = dpdk_qos__create_port( rte_sched_port_name, rte_socket_id(), linkspeed, side);

	if(!interface->sched_port)
	{
		printf( "creating sched_port interface failed for rte-sched-port=%s\n", rte_sched_port_name);
		exit(0);
	}

	interface->QosInitalized = 1;
	interface->QosConfigured = 1;



	uint32_t nb_pkt;
	uint32_t nb_pkt2;
	uint32_t nb_sent;
	uint32_t pkts_send;
	struct rte_mbuf * mbufs[ 64];
	int temp = 0; 

	int rr_index = 0;
	int iterator = 0;

	dpdk_interface_t * txInterface = interface->TxInterface;
	struct rte_ring * send_ring = NULL;
	
	if(!txInterface)
	{
		printf("TxInterface is null in sched, interface index=%u\n", interface->Index);
		exit(0);
	}
	
	
	while(__dpe->runing)
	{
		nb_pkt = 0;
		nb_pkt2 = 0;
		
		nb_pkt = rte_ring_sc_dequeue_burst( interface->sched_ring, (void **)mbufs, 64, NULL);
		
		if (likely(nb_pkt > 0))
		{
			nb_sent = rte_sched_port_enqueue( interface->sched_port, mbufs, nb_pkt);
			
			temp = (nb_pkt - nb_sent);
			
			if( temp > 0)
			{
				rte_pktmbuf_free_bulk( &mbufs[nb_sent], temp);
			}
		}
		
		nb_pkt2 = rte_sched_port_dequeue( interface->sched_port, mbufs, 64);
		
		if (likely(nb_pkt2 > 0))
		{
			if( txInterface->SendQueue == 1)
			{
				rr_index = 0;
			}
			else
			{
				if( iterator >= txInterface->SendQueue)
					iterator = 0;
				
				rr_index = iterator % txInterface->SendQueue;
				iterator++;
			}
					
			send_ring = txInterface->send_ring[rr_index];
			pkts_send = 0;

			do {
				pkts_send += rte_ring_enqueue_burst( send_ring, (void **)&mbufs[pkts_send], nb_pkt2 - pkts_send, NULL);
			} while( pkts_send < nb_pkt2);	
		}
		
		
		if( nb_pkt == 0 && nb_pkt2 == 0) {
			dpe__sleep();
		}
	}

	return 0;
}

// 
// VOID -- NOT USING THIS
//
int dpe__sched_init( void * interafaceArgs)
{
	dpdk_interface_t * interaface = (dpdk_interface_t *)interafaceArgs;


	printf( "lcore_id=%u lcore_to_socket_id=%u  %s|%s|%d\n", 
			interaface->lcore_id,  
			rte_lcore_to_socket_id( interaface->lcore_id),
			__FILE__, __FUNCTION__, __LINE__
		);


	uint32_t linkspeed = dpe__engine_get_linkspeed( interaface);
	
	
	int i = 0;
	int j = 0;
	int max_subport_profile = 1;
	
	struct rte_sched_subport_profile_params subport_profile[max_subport_profile];		// MAX_SCHED_SUBPORT_PROFILES
	memset( &subport_profile, 0, sizeof( struct rte_sched_subport_profile_params) * max_subport_profile);
	
	for( i = 0; i < max_subport_profile; i++)
	{
		subport_profile[i].tb_rate = 1250000000;
		subport_profile[i].tb_size = 1000000;
		subport_profile[i].tc_period = 10;
	
		for( j = 0; j < 13; j++)
		{
			subport_profile[i].tc_rate[j] = interaface->QosRate[i];
		}
	}
	
	
	struct rte_sched_port_params port_params;
	memset( &port_params, 0, sizeof( struct rte_sched_port_params));
	
	char port_name[150];
	memset( port_name, 0, sizeof(port_name));
	sprintf( port_name, "port-%d", interaface->Index);
	
	port_params.name					= port_name;
	port_params.socket 					= 0;
	port_params.rate 					= 0;
	port_params.mtu 					= 6 + 6 + 4 + 4 + 2 + 1500;
	port_params.frame_overhead 			= RTE_SCHED_FRAME_OVERHEAD_DEFAULT;
	

	port_params.n_subport_profiles 		= max_subport_profile;
	port_params.subport_profiles 		= subport_profile;
	port_params.n_max_subport_profiles 	= 13;	// MAX_SCHED_SUBPORT_PROFILES;

	port_params.n_subports_per_port 	= interaface->QosRateCount;
	port_params.n_pipes_per_subport 	= 4096; // MAX_SCHED_PIPES;


	uint32_t socket = rte_lcore_to_socket_id( interaface->lcore_id);
	
	port_params.socket = socket;
	port_params.rate = (uint64_t) linkspeed * 1000 * 1000 / 8;	
	
	printf( "n_subports_per_port=%u | %u | %u\n", 
			port_params.n_subports_per_port, 
			port_params.n_subports_per_port > 1u << 16,
			!rte_is_power_of_2(port_params.n_subports_per_port)
		);
	
	
	
	interaface->sched_port = rte_sched_port_config( &port_params);
	
	if(!interaface->sched_port)
	{
		printf("sched_port create failed\n");
		exit(0);
	}
	
	
	printf( "created scheduler port=%p for interface Index=%u\n", interaface->sched_port, interaface->Index);
	
	
	//exit(0);
	
	struct rte_sched_subport_params subport_params;
	struct rte_sched_pipe_params pipe_profile;
	
	
	
	
	uint32_t subport = 0;
	int err;
	
	for( subport = 0; subport < port_params.n_subports_per_port; subport++) 
	{
		memset( &subport_params, 0, sizeof(struct rte_sched_subport_params));
		memset( &pipe_profile, 0, sizeof(struct rte_sched_pipe_params));

		pipe_profile.tb_rate = interaface->QosRate[subport];
		pipe_profile.tb_size = 1000000;
		pipe_profile.tc_period = 40;
		pipe_profile.tc_ov_weight = 1;
		pipe_profile.wrr_weights[0] = 1;
		pipe_profile.wrr_weights[1] = 1;
		pipe_profile.wrr_weights[2] = 1;
		pipe_profile.wrr_weights[3] = 1;


		subport_params.n_pipes_per_subport_enabled 	= interaface->QosPipePerSubport;
		subport_params.pipe_profiles 				= &pipe_profile;
		subport_params.n_pipe_profiles 				= 1;
		subport_params.n_max_pipe_profiles 			= 8;	//256
		subport_params.cman_params = NULL;
		
		
		for( j = 0; j < 13; j++)
		{
			pipe_profile.tc_rate[j] = interaface->QosRate[subport];
			subport_params.qsize[j] = interaface->QosQSize;
			
			//printf( "subport=%d  qos-rate=%lu QSize=%lu\n", subport, interaface->QosRate[subport], interaface->QosQSize);
		}
		
		err = rte_sched_subport_config( interaface->sched_port, subport, &subport_params, 0);
		if (err) 
		{
			printf( "Unable to config sched subport %u, err=%d\n", subport, err);
			exit(0);
		}


		uint32_t pipe = 0;
		
		for (pipe = 0; pipe < subport_params.n_pipes_per_subport_enabled; pipe++) 
		{
			err = rte_sched_pipe_config( interaface->sched_port, subport, pipe, 0);			
			if (err) 
			{
				printf( "Unable to config sched pipe %u for profile %d, err=%d\n", pipe, 0, err);
			}
			
			//printf( "configured subport=%u pipe=%u\n", subport, pipe);
		}
		
		printf( "IIndex=%u configured subport=%u pipe-count=%u\n", interaface->Index, subport, pipe);
	}
	
	
	uint32_t nb_pkt;
	uint32_t nb_pkt2;
	uint32_t nb_sent;
	uint32_t pkts_send;
	struct rte_mbuf * mbufs[ 64];

	int rr_index = 0;
	int iterator = 0;

	dpdk_interface_t * txInterface = interaface->TxInterface;
	struct rte_ring * send_ring = NULL;
	
	if(!txInterface)
	{
		printf("TxInterface is null in sched, interface index=%u\n", interaface->Index);
		exit(0);
	}
	
	
	while(__dpe->runing)
	{
		nb_pkt = 0;
		nb_pkt2 = 0;
		
		nb_pkt = rte_ring_sc_dequeue_burst( interaface->sched_ring, (void **)mbufs, 64, NULL);
		
		if (likely(nb_pkt > 0))
		{
			nb_sent = rte_sched_port_enqueue( interaface->sched_port, mbufs, nb_pkt);
		}
		
		nb_pkt2 = rte_sched_port_dequeue( interaface->sched_port, mbufs, 64);
		
		if (likely(nb_pkt2 > 0))
		{
			if( txInterface->SendQueue == 1)
			{
				rr_index = 0;
			}
			else
			{
				if( iterator >= txInterface->SendQueue)
					iterator = 0;
				
				rr_index = iterator % txInterface->SendQueue;
				iterator++;
			}
					
			send_ring = txInterface->send_ring[rr_index];
			pkts_send = 0;

			do {
				pkts_send += rte_ring_enqueue_burst( send_ring, (void **)&mbufs[pkts_send], nb_pkt2 - pkts_send, NULL);
			} while( pkts_send < nb_pkt2);	
		}
		
		
		if( nb_pkt == 0 && nb_pkt2 == 0) {
			dpe__sleep();
		}
	}

	
}


void * dpe__run_dhcp_thread( void * args)
{
	int j = 0;
	dpdk_interface_t * interaface = NULL;

	
	while(1)
	{
		interaface = __dpe->Head;
		
		while( interaface)
		{
			if( interaface->DHCPInform == 1 && interaface->DHCPInformAckReceived == 0)
			{
				interaface->DHCPInformTransactionId = time(NULL);
				
				dpe__send_dhcp_request( interaface);
			}
			else if( interaface->DHCPDiscover == 1 && interaface->DHCPDiscoverAckReceived == 0)
			{
				interaface->DHCPDiscoverTransactionId = time(NULL);
				
				dpe__send_dhcp_request( interaface);				
			}
			
			if( interaface->DHCPInformAckReceived > 1)
			{
				interaface->DHCPInformAckReceived++;
			}

			if( interaface->DHCPDiscoverAckReceived > 1)
			{
				interaface->DHCPDiscoverAckReceived++;
			}

			
			if( interaface->DHCPInformAckReceived > 20000) 
			{
				interaface->DHCPInformAckReceived = 0;
			}
			
			if( interaface->DHCPDiscoverAckReceived > 20000) 
			{
				interaface->DHCPDiscoverAckReceived = 0;
			}
			
			interaface = interaface->Next;
		}
	
		
		for( j = 0; j < 10; j ++) {
			usleep( 999999);
		}
		
	}
}

void * dpe__run_ipv6rs_thread( void * args)
{
	int j = 0;
	dpdk_interface_t * interaface = NULL;

	
	while(1)
	{
		interaface = __dpe->Head;
		
		while( interaface)
		{
			if( interaface->IPv6SendRS == 1 && interaface->IPv6Set == 1)
			{
				dpe__send_ipv6rs_request( interaface);

				dpe__send_ipv6ns_request(interaface);
				dpe__send_ipv6mlrm_request( interaface);
			
				//interaface->RSCounter == 0 && 
				if( interaface->TargetIP > 0 && interaface->NullMCLRCount >3)
				{
					//printf("sending ARP \n");
					dpe__send_arp2( interaface, interaface->TargetIP);
				}
			}
			
			
			interaface = interaface->Next;
		}
		
		for( j = 0; j < 1; j ++) 
		{
			usleep( 999999);
		}

	}
}

void dpe__setup_env()
{
	if( __dpdk_env__is_numa() == 1) {
		printf("Non Numa System\n");
	} else {
		printf("Non Numa System\n");
	}

	__dpdk_env__create_huge_pages();
	__dpdk_env__mount();
	__dpdk_env__configure_hugepages_for_nodes();
	__dpdk_env__start__vfio_pci();
	
}


void dpe__init_engine( int argc, char **argv)
{
	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("UPF:  Cannot init EAL\n");
		exit(0);
	}
	
	
	
	int totalCoresRequired = dpe__get_required_core_count();
	
	if( rte_lcore_count() < totalCoresRequired)
	{
		printf("Error: Required Cores:%d, Available cores:%d\n", totalCoresRequired, rte_lcore_count());
		exit(0);
	}
	
	

	
	//dpe__setup_env(); 
	
	
	dpdk_interface_t * interaface = __dpe->Head;
	
	if( strlen( __dpe->dpdkToolsPath) > 0)
	{
		while( interaface)
		{
			if( strlen( interaface->ifName) > 0 && strlen( interaface->TxPort) > 0)
			{
				if( __dpdk_env__is_dpdk_driver_attached( interaface->TxPort) == 0)
				{
					__dpdk_env__configure__pci( __dpe->dpdkToolsPath, interaface->ifName, interaface->TxPort);
				}
			}
			interaface = interaface->Next;
		}
	}
	
	__dpe->packet_type_arp 		= rte_be_to_cpu_16(RTE_ETHER_TYPE_ARP);
	__dpe->packet_type_ipv4		= rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV4);
	__dpe->packet_type_ipv6		= rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV6);

	__dpe->runing = 1;
	__dpe->last_lcore = rte_get_next_lcore( 0, 1, 0);


	interaface = __dpe->Head;
	while( interaface)
	{
		if( interaface->QosEnabled == 1)	
		{
			dpdk_qos__init( "./dpdk_qos.json");
			break;
		}
		interaface = interaface->Next;
	}
	
	
	
	int requiredDhcpThread = 0;
	interaface = __dpe->Head;
	//app_rbtree_t * natIPTree = NULL;
	dpdk_nat_ip_t * natIP = NULL;
	
	while( interaface)
	{
		dpe__configure_interface( interaface);
		
		interaface->DHCPInformAckReceived = 0;
		interaface->DHCPDiscoverAckReceived = 0;
	
		if( requiredDhcpThread == 0 && (interaface->DHCPInform == 1 || interaface->DHCPDiscover == 1))
		{
			requiredDhcpThread = 1;
		}
		
		
		if( interaface->EnableNAPT == 1)
		{
			dpdk_nat_ip_t * natIPItem = interaface->natIPHead;
			
			if( natIPItem)
			{
				if( natIPItem->ip == interaface->IPv4)
				{
					memcpy(natIPItem->MAC, interaface->MacAddr, 6);
					
					printf( "NAP-IP-ADDR: %02X:%02X:%02X:%02X:%02X:%02X\n",
						natIPItem->MAC[0] & 0xFF, natIPItem->MAC[1] & 0xFF, natIPItem->MAC[2] & 0xFF,
						natIPItem->MAC[3] & 0xFF, natIPItem->MAC[4] & 0xFF, natIPItem->MAC[5] & 0xFF
					);
				
				}
			}
		}
		
		interaface = interaface->Next;
	}
	
	printf("Type=%d  %s|%s|%d\n", __dpe->Type, __FILE__, __FUNCTION__, __LINE__);

	
	if( __dpe->Type == 3)
	{
		interaface = __dpe->Head;
		dpdk_interface_t * txInteraface = NULL;
		
		while( interaface)
		{
			if( interaface->TxPortHasConfigured == 1)
			{
				txInteraface = __dpe->Head;
				
				while( txInteraface)
				{
					if( strcmp( interaface->TxPort, txInteraface->Port) == 0)
					{
						interaface->TxInterface = txInteraface;
						printf("interaface=%s|%p -> Tx %s|%p|  RxMAC=%02X:%02X:%02X:%02X:%02X:%02X RxIPv4=%u.%u.%u.%u  TxMAC=%02X:%02X:%02X:%02X:%02X:%02X TxIPv4=%u.%u.%u.%u\n", 
							interaface->Port, interaface, txInteraface->Port, txInteraface, 
							
							interaface->MacAddr[0] & 0xFF, interaface->MacAddr[1] & 0xFF, interaface->MacAddr[2] & 0xFF,
							interaface->MacAddr[3] & 0xFF, interaface->MacAddr[4] & 0xFF, interaface->MacAddr[5] & 0xFF,
							interaface->IPv4 & 0xFF, (interaface->IPv4 >> 8) & 0xFF, (interaface->IPv4 >> 16) & 0xFF, (interaface->IPv4 >> 24) & 0xFF,
							
							txInteraface->MacAddr[0] & 0xFF, txInteraface->MacAddr[1] & 0xFF, txInteraface->MacAddr[2] & 0xFF,
							txInteraface->MacAddr[3] & 0xFF, txInteraface->MacAddr[4] & 0xFF, txInteraface->MacAddr[5] & 0xFF,
							txInteraface->IPv4 & 0xFF, (txInteraface->IPv4 >> 8) & 0xFF, (txInteraface->IPv4 >> 16) & 0xFF, (txInteraface->IPv4 >> 24) & 0xFF
						);
						
						break;
					}
					txInteraface = txInteraface->Next;
				}
			}
			interaface = interaface->Next;
		}
	}
	
	
	
	interaface = __dpe->Head;
	while( interaface)
	{
		dpe__engine_start( interaface);
		interaface = interaface->Next;
	}

	




	interaface = __dpe->Head;
	while( interaface)
	{
		dpe__engine_print_linkspeed( interaface);
		interaface = interaface->Next;
	}

	
	interaface = __dpe->Head;
	while( interaface)
	{
		if( interaface->DestIP > 0)
		{
			struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interaface->mempool[0]);
			dpdk_pkt__encode_arp_request( m_buf, interaface->MacAddr, interaface->IPv4, interaface->DestIP);

			// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./arp_packet1.pcap");
			// dpdk_pkt__write_pcap( dumper2, m_buf);
			// dpdk_pkt__close_pcap( dumper2);
			interaface->arp_request_sent++;
			
			if( rte_ring_enqueue( interaface->send_ring[0], m_buf) == 0)
			{
				dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed ARP Request for IP=%u.%u.%u.%u from BusId=%s", 
					interaface->DestIP & 0xFF, (interaface->DestIP >> 8) & 0xFF, (interaface->DestIP >> 16) & 0xFF, 
					(interaface->DestIP >> 24) & 0xFF, interaface->Port);
			}
			else
			{
				dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending ARP Request Failed for IP=%u.%u.%u.%u from BusId=%s", 
					interaface->DestIP & 0xFF, (interaface->DestIP >> 8) & 0xFF, (interaface->DestIP >> 16) & 0xFF, 
					(interaface->DestIP >> 24) & 0xFF, interaface->Port);
			}
		}
		
		interaface = interaface->Next;
	}
	
	int bLaunchIPv6SendRS = 0;
	
	interaface = __dpe->Head;
	while( interaface)
	{
		if( interaface->IPv6SendRS > 0 && interaface->IPv6Set == 1)
		{
			bLaunchIPv6SendRS = 1;
			break;
		}
		interaface = interaface->Next;
	}
	
	
	int i = 0;
	for( i = 0; i < __dpe->static_arp_table_row_count; i++)
	{
		if( __dpe->static_arp_table[i].Mac[0] == 0 && __dpe->static_arp_table[i].Mac[1] == 0 && __dpe->static_arp_table[i].Mac[2] == 0 && __dpe->static_arp_table[i].Mac[3] == 0 && __dpe->static_arp_table[i].Mac[4] == 0 && __dpe->static_arp_table[i].Mac[5] == 0 && __dpe->static_arp_table[i].ip != 0)
		{
			interaface = __dpe->Head;
			
			while( interaface)
			{
				struct rte_mbuf * m_buf = rte_pktmbuf_alloc( interaface->mempool[0]);
				dpdk_pkt__encode_arp_request( m_buf, interaface->MacAddr, interaface->IPv4, htonl( __dpe->static_arp_table[i].ip) );
			
				interaface->arp_request_sent++;
				
				if( rte_ring_enqueue( interaface->send_ring[0], m_buf) == 0)
				{
					dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Enqueed ARP Request for Static IP=%u.%u.%u.%u from BusId=%s", 
						(__dpe->static_arp_table[i].ip >> 24) & 0xFF, (__dpe->static_arp_table[i].ip >> 16) & 0xFF,
						(__dpe->static_arp_table[i].ip >> 8) & 0xFF,  __dpe->static_arp_table[i].ip & 0xFF, 
						interaface->Port);
				}
				else
				{
					dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Sending ARP Request Failed for Static IP=%u.%u.%u.%u from BusId=%s", 
						(__dpe->static_arp_table[i].ip >> 24) & 0xFF, (__dpe->static_arp_table[i].ip >> 16) & 0xFF,
						(__dpe->static_arp_table[i].ip >> 8) & 0xFF,  __dpe->static_arp_table[i].ip & 0xFF, 
						interaface->Port);
				}
			
				interaface = interaface->Next;
			}
		}			
	}

	dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Logger Started, EnableLogs=%d", __dpe->EnableLogs);

	if( bLaunchIPv6SendRS == 1)
	{
		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		int iRet = pthread_create( &s_pthread_id, &attr, dpe__run_ipv6rs_thread, (void *)NULL);		
	}

	if( requiredDhcpThread == 1)
	{
		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		int iRet = pthread_create( &s_pthread_id, &attr, dpe__run_dhcp_thread, (void *)NULL);		
	}

	if( __dpe->EnablePerfLogs > 0)
	{
		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		int iRet = pthread_create( &s_pthread_id, &attr, dpe__run_pref_thread, (void *)NULL);
	}
	else
	{
		dpe__log( LOG_LEVEL_DEBUG, __FILE__, __LINE__, "EnablePerfLogs=%d Disabled", __dpe->EnablePerfLogs);
	}
}


void dpe__test__encode_icmppacket_to_ran()
{
	dpdk_interface_t * interface 	= dpe__getinterface(0);
	interface->ProcessType 			= PROCESS_TYPE__Gi_GTP;	

	struct rte_mbuf * pkts[1];
	pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);

	struct rte_mbuf * processed_pkts[1];
	struct rte_mbuf * dropped_mbufs[1];
	struct rte_mbuf * arp_mbufs[1];

	uint16_t recv_pkts				= 1;
	uint16_t dropped_cnt_out 		= 0;
	uint16_t arp_cnt_out			= 0;

	struct rte_mbuf * gtp_mbufs[1];
	uint16_t gtp_cnt_out			= 0;
	
	
	// Packek Info
		// MAC  - 14
		// IPv4 - 20 
		// ICMP - 8

	char * gtpTPduPacket = 	NULL;
	int gtplen = 14 + 20 + 8;
	
	gtpTPduPacket = "\x00\x0C\x29\xF5\xF3\x6C\x00\x0C\x29\x1D\x55\x39\x08\x00"	
					"\x45\x00\x00\x1C\x16\xCF\x00\x00\x40\x01\x4F\xB6\x0A\x2D\x00\x01\x0A\x2D\x00\x02"
					"\x00\x00\x23\x1F\x73\x48\x69\x98";

	char * c_mptr4 = rte_pktmbuf_mtod( pkts[0], char *);
	memcpy( c_mptr4, gtpTPduPacket, gtplen);
	memcpy( &c_mptr4[0], interface->MacAddr, 6);
	
	pkts[0]->pkt_len  = gtplen;
	pkts[0]->data_len = gtplen;
	
	pcap_dumper_t * dumper0 = dpdk_pkt__open_pcap_dump( "./gi_icmp_11_DEC_2023.pcap");
	dpdk_pkt__write_pcap( dumper0, pkts[0]);
	dpdk_pkt__close_pcap( dumper0);
	
	uint16_t cnt = dpe__process_forward( interface, pkts, recv_pkts, processed_pkts, dropped_mbufs, &dropped_cnt_out, arp_mbufs, &arp_cnt_out, gtp_mbufs, &gtp_cnt_out, 0);
	printf("cnt=%d  dropped_cnt_out=%d  arp_cnt_out=%d  %s|%d\n", cnt, dropped_cnt_out, arp_cnt_out, __FUNCTION__, __LINE__);
	
	if( cnt == 1)
	{
		pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gtp_icmp_11_DEC_2023.pcap");
		dpdk_pkt__write_pcap( dumper2, processed_pkts[0]);
		dpdk_pkt__close_pcap( dumper2);
	}
	
}

void dpe__test__create_single_session()
{
	uint32_t ran_ip = 0;
	uint32_t teid = 0;
	uint8_t * session = NULL;
	uint8_t mac[7];
	uint32_t p_ran_teid = 0;
	uint8_t qfi = 1;

	printf("%s|%d\n", __FUNCTION__, __LINE__);
	cp__create_session( 1001, 0x0A2D0001, 10001, 0x7F7F7F7F, 2001);
	
	
	teid = 10001;
	cp__teid_allowed_v4( ran_ip, teid, &session, mac);
	printf("teid=%u session=%p  %s|%d\n", teid, session, __FUNCTION__, __LINE__);
	
	
	char dataPkt[100];
	int pkt_len = 100;
	
	
	int res1 = cp__flow_allowed_sess_v4( session, 0x0A2D0001 + 100000, 5500, 6600, 6, dataPkt, pkt_len, PROCESS_TYPE__GTP_Gi, 0);
	printf("cp__flow_allowed_sess_v4 res1=%u session=%p  %s|%d\n", res1, session, __FUNCTION__, __LINE__);

	
	session = NULL;
	int res2 = cp__flow_allowed_ueipv4( htonl(0x0A2D0001), 0x0A2D0001 + 100000, 6600, 5500, 6, dataPkt, pkt_len, &session, &p_ran_teid, PROCESS_TYPE__Gi_GTP, 0, &qfi);
	printf("cp__flow_allowed_ueipv4 res2=%u session=%p  %s|%d\n", res2, session, __FUNCTION__, __LINE__);	
	
	
	uint8_t  ranip_v = 0;
	uint32_t ranip4 = 0;
	__uint128_t ranip6;
	uint8_t * rmac = NULL;
	
	int res3 = cp__get_ranv4info( session, &ranip_v, &ranip4, &ranip6, &rmac);
	printf( "cp__get_ranv4info session=%p ranip4=%u|%u rmac=%p   %s|%d\n", session, ranip4, 0x7F7F7F7F, rmac, __FUNCTION__, __LINE__);
	
	
}

void dpe__test__create_single_packet( int protocol)
{
	char pName_RAN_TO_UPF[200];
	char pName_UPF_TO_ISP[200];	
	char pName_ISP_TO_UPF[200];
	char pName_UPF_TO_RAN[200];	
	
	memset( pName_RAN_TO_UPF, 0, sizeof(pName_RAN_TO_UPF));
	memset( pName_UPF_TO_ISP, 0, sizeof(pName_UPF_TO_ISP));
	memset( pName_ISP_TO_UPF, 0, sizeof(pName_ISP_TO_UPF));
	memset( pName_UPF_TO_RAN, 0, sizeof(pName_UPF_TO_RAN));	
	
	sprintf( pName_RAN_TO_UPF, "./packet__%d_RAN_TO_UPF.pcap",  protocol);
	sprintf( pName_UPF_TO_ISP, "./packet__%d_UPF_TO_ISP.pcap",  protocol);
	sprintf( pName_ISP_TO_UPF, "./packet__%d_ISP_TO_UPF.pcap",  protocol);
	sprintf( pName_UPF_TO_RAN, "./packet__%d_UPF_TO_RAN.pcap",  protocol);
	
	
	dpdk_interface_t * interface 	= dpe__getinterface(0);
	interface->ProcessType 			= PROCESS_TYPE__GTP_Gi;

	struct rte_mbuf * pkts[1];
	pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);

	struct rte_mbuf * processed_pkts[1];
	struct rte_mbuf * dropped_mbufs[1];
	struct rte_mbuf * arp_mbufs[0];
	uint16_t recv_pkts				= 1;
	uint16_t dropped_cnt_out 		= 0;
	uint16_t arp_cnt_out			= 0;

	struct rte_mbuf * gtp_mbufs[1];
	uint16_t gtp_cnt_out			= 0;
	
	char * src_mac = "\x00\x00\x23\x1f\x73\x48";
	char * dst_mac = "\x00\x00\x00\x00\x73\x48";
	

	dpe_pkt__encode_gtp_tpdu( pkts[0], src_mac, interface->MacAddr, 0x7F7F7F7F, 0x6F6F6F6F, 0x0A2D0001, 0x0A2D0011, 8801, 9901, htonl(10001), protocol, 100);
	
	pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( pName_RAN_TO_UPF);
	dpdk_pkt__write_pcap( dumper2, pkts[0]);
	dpdk_pkt__close_pcap( dumper2);

	
	uint16_t cnt = dpe__process_forward( interface, pkts, recv_pkts, processed_pkts, dropped_mbufs, &dropped_cnt_out, arp_mbufs, &arp_cnt_out, gtp_mbufs, &gtp_cnt_out, 0);
	printf("cnt=%d  dropped_cnt_out=%d  arp_cnt_out=%d  %s|%d\n", cnt, dropped_cnt_out, arp_cnt_out, __FUNCTION__, __LINE__);
	
	if( cnt == 1)
	{
		dumper2 = dpdk_pkt__open_pcap_dump( pName_UPF_TO_ISP);
		dpdk_pkt__write_pcap( dumper2, processed_pkts[0]);
		dpdk_pkt__close_pcap( dumper2);
	
		interface->ProcessType = PROCESS_TYPE__Gi_GTP;
		
		char * c_mptr4 = rte_pktmbuf_mtod( processed_pkts[0], char *);
		memcpy( &c_mptr4[0], interface->MacAddr, 6);
		
		struct rte_ether_hdr * eth 			= rte_pktmbuf_mtod( processed_pkts[0], struct rte_ether_hdr *);
		struct rte_ipv4_hdr  * ue_ip_hdr 	= (struct rte_ipv4_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));

		uint32_t ue_src_ip = ue_ip_hdr->src_addr;
		uint32_t ue_dst_ip = ue_ip_hdr->dst_addr;
		
		ue_ip_hdr->dst_addr = ue_src_ip;
		ue_ip_hdr->src_addr = ue_dst_ip;
		
		dumper2 = dpdk_pkt__open_pcap_dump( pName_ISP_TO_UPF);
		dpdk_pkt__write_pcap( dumper2, processed_pkts[0]);
		dpdk_pkt__close_pcap( dumper2);

		
		pkts[0] = processed_pkts[0];
		processed_pkts[0] = NULL;
		cnt = dpe__process_forward( interface, pkts, recv_pkts, processed_pkts, dropped_mbufs, &dropped_cnt_out, arp_mbufs, &arp_cnt_out, gtp_mbufs, &gtp_cnt_out, 0);
		printf("cnt=%d  dropped_cnt_out=%d  arp_cnt_out=%d  %s|%d\n", cnt, dropped_cnt_out, arp_cnt_out, __FUNCTION__, __LINE__);
		
		if( cnt == 1)
		{
			dumper2 = dpdk_pkt__open_pcap_dump( pName_UPF_TO_RAN);
			dpdk_pkt__write_pcap( dumper2, processed_pkts[0]);
			dpdk_pkt__close_pcap( dumper2);			
		}
	}
}

void dpe__test__create_sessions( int count)
{
	printf("%s started|%d\n", __FUNCTION__, __LINE__);

	int i = 0;
	for( i = 0; i < count; i++)
	{
		cp__create_session( 1001 + i, 0x0A2D0001 + i, 10001 + i, 0x7F7F7F7F, 2001 + i);
	}
	
	printf("%s completed|%d\n", __FUNCTION__, __LINE__);
}

unsigned long __cnt_pkts = 0;
unsigned long __cnt_pkts_last = 0;

void * dpe__test__create_counting_thread( void * args)
{
	while(1)
	{
		printf("counter: processed packets | %12lu  | %12lu\n", __cnt_pkts, __cnt_pkts - __cnt_pkts_last);
		__cnt_pkts_last = __cnt_pkts;
		sleep(1);
	}
	return NULL;
}

void dpe__test__create_counter_thread()
{
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	int iRet = pthread_create( &s_pthread_id, &attr, dpe__test__create_counting_thread, (void *)NULL);
}

void dpe__test__run_gtp_gi_load()
{
	dpdk_interface_t * interface 	= dpe__getinterface(0);
	interface->ProcessType 			= PROCESS_TYPE__GTP_Gi;

	struct rte_mbuf * pkts[1];
	pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);

	struct rte_mbuf * processed_pkts[1];
	struct rte_mbuf * dropped_mbufs[1];
	struct rte_mbuf * arp_mbufs[0];
	uint16_t recv_pkts				= 1;
	uint16_t dropped_cnt_out 		= 0;
	uint16_t arp_cnt_out			= 0;

	struct rte_mbuf * gtp_mbufs[1];
	uint16_t gtp_cnt_out			= 0;
	
	char * src_mac = "\x00\x00\x23\x1f\x73\x48";
	char * dst_mac = "\x00\x00\x00\x00\x73\x48";
	uint16_t cnt = 0;
	
	//2241
	// only packet costruction 12,096,092  -- 12 million packets per second
	// packet forwarding from GTP_Gi 
	// 12096092
	// 11005210 -- after mac validation  
	// (11006409 even if commented mac validation)
	// 1,01,62,405  -- disabled 2 apis
	//   86,55,362	-- disabled 2nd api			
	//   80,19,130    -- enabled 2 apis
	
	while(1) 
	{
		rte_pktmbuf_reset( pkts[0]);
		rte_prefetch0( rte_pktmbuf_mtod( pkts[0], void *));
		dpe_pkt__encode_gtp_tpdu( pkts[0], src_mac, interface->MacAddr, 0x7F7F7F7F, 0x6F6F6F6F, 0x0A2D0001, 0x0A2D0011, 8801, 9901, htonl(10001), 6, 100);
		dpe__process_forward( interface, pkts, recv_pkts, processed_pkts, dropped_mbufs, &dropped_cnt_out, arp_mbufs, &arp_cnt_out, gtp_mbufs, &gtp_cnt_out, 0);
		
		__cnt_pkts++;
	}	
}


/*
	RAN will identify subscriber session by TEID
*/
void dpe_pkt__send_gtp_end_marker_ipv4( uint32_t ranip, uint32_t ran_teid)
{
	if(!__dpe) return;
	
	//find access interaface
	dpdk_interface_t * interface = __dpe->Head;
	int bFound = 0 ;
	
	while( interface)
	{
		if( interface->ProcessType == PROCESS_TYPE__GTP_Gi)
		{
			bFound = 1;
			break;
		}
		interface = interface->Next;
	}
	
	if( bFound == 1)
	{
		int i = 0;
		int bProcessed = 0;
		
		for( i = 0; i < 5; i++)
		{
			if( bProcessed == 0)
			{
				struct rte_mbuf * pkts[1];
				pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);		
			
				if( pkts[0])
				{
					// packet allocation failed
					bProcessed = 1;

					char * srcMac = "\x00\x00\x23\x1f\x73\x48";
					char * dstMac = "\x00\x00\x00\x00\x73\x48";
					
					uint8_t * mac = dpe__get_macaddr( ranip);
					
					if(!mac)
					{
						mac = dstMac;
					}
					
					dpe_pkt__encode_gtp_end_marker( pkts[0], interface->MacAddr, mac, htonl(ranip), interface->IPv4, 2152, 2152, htonl(ran_teid));
					rte_ring_enqueue_burst( interface->send_ring[0], (void **)&pkts[0], 1, NULL);
				}
				else
				{
					//log packet allocation failed
					usleep( 1000);
				}
			}	
		}
	}
	else
	{
		//log Access interface not found
	}
}

uint64_t app_ep__get_u40( unsigned char * buff);

void dpe__test()
{
	printf("dpe__test()\n");

	
	//DHCP Inform
	//unsigned char * dhcp_data_1 = "\x01\x01\x06\x00\x99Y\xf9(\x00\x00\x00\x00\xc0\xa8\x01o\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00(\xcf\xda\xdc\x8dv\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00c\x82Sc5\x01\x08=\x07\x01(\xcf\xda\xdc\x8dv\x0c\x0cNicksMacBook<\x08MSFT 5.07\x0d\x01\x0f\x03\x06,./\x1f!y\xf9+\xfc\xff\x00\x00\x00\x00\x00\x00\x00\x00";
	// unsigned char dhcp_data_1[300] = { 0x01,0x01,0x06,0x00,0x99,0x59,0xf9,0x28,0x00,0x00,0x00,0x00,0xc0,0xa8,0x01,0x6f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0xcf,0xda,0xdc,0x8d,0x76,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x82,0x53,0x63,0x35,0x01,0x08,0x3d,0x07,0x01,0x28,0xcf,0xda,0xdc,0x8d,0x76,0x0c,0x0c,0x4e,0x69,0x63,0x6b,0x73,0x4d,0x61,0x63,0x42,0x6f,0x6f,0x6b,0x3c,0x08,0x4d,0x53,0x46,0x54,0x20,0x35,0x2e,0x30,0x37,0x0d,0x01,0x0f,0x03,0x06,0x2c,0x2e,0x2f,0x1f,0x21,0x79,0xf9,0x2b,0xfc,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	// int dhcp_len_1				= 300;
	// __dhcp__decode_packet( dhcp_data_1, dhcp_len_1);


	// unsigned char dhcp_data_2[548] = { 0x02,0x01,0x06,0x00,0x99,0x59,0xf9,0x28,0x00,0x00,0x00,0x00,0xc0,0xa8,0x01,0x6f,0x00,0x00,0x00,0x00,0xc0,0xa8,0x01,0x01,0x00,0x00,0x00,0x00,0x28,0xcf,0xda,0xdc,0x8d,0x76,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x82,0x53,0x63,0x35,0x01,0x05,0x36,0x04,0xc0,0xa8,0x01,0x01,0x01,0x04,0xff,0xff,0xff,0x00,0x03,0x04,0xc0,0xa8,0x01,0x01,0x06,0x08,0x4b,0x4b,0x4c,0x4c,0x4b,0x4b,0x4b,0x4b,0x0f,0x14,0x68,0x73,0x64,0x31,0x2e,0x70,0x61,0x2e,0x63,0x6f,0x6d,0x63,0x61,0x73,0x74,0x2e,0x6e,0x65,0x74,0x2e,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	// int dhcp_len_2				= 548;
	// __dhcp__decode_packet( dhcp_data_2, dhcp_len_2);
	

	// unsigned char dhcp_data_3[500];
	// int dhcp_len_3			= 0;
	// __dhcp__encode_inform_packet( dhcp_data_3, &dhcp_len_3, "\x01\x02\x03\x04\x05\x06", 11, "upf-gtp", 7, 12345);


	// struct rte_ether_hdr hdr;
	// memset( &hdr, 0, sizeof(struct rte_ether_hdr));
	// memcpy( hdr.dst_addr.addr_bytes, "\x01\x02\x03\x04\x05\x06", 6);
	// memcpy( hdr.src_addr.addr_bytes, "\x01\x01\x01\x01\x01\x01", 6);
	
	// unsigned long udst_addr = *(unsigned long*)&hdr.dst_addr.addr_bytes;
	// char * ld = (char *)&udst_addr;
	
	// printf( "udst_addr = %lu  %lu\n", udst_addr, app_ep__get_u40("\x01\x02\x03\x04\x05\x06"));
	// dpe__print_buffer( "dst_addr...", hdr.dst_addr.addr_bytes, 6, 1, 1);
	// dpe__print_buffer( "ld.........", ld, 8, 1, 1);
	
	// unsigned long lld_1 = (udst_addr << 16);
	// char * ld_1 = (char *)&lld_1;
	// dpe__print_buffer( "udst_addr << ", ld_1, 8, 1, 1);

	

	// unsigned long usrc_addr = *(unsigned long*)&hdr.src_addr.addr_bytes;
	// char * sld = (char *)&usrc_addr;
	
	// printf( "usrc_addr = %lu  %lu\n", usrc_addr, app_ep__get_u40("\x01\x01\x01\x01\x01\x01"));
	// dpe__print_buffer( "usrc_addr...", hdr.src_addr.addr_bytes, 6, 1, 1);
	// dpe__print_buffer( "sld.........", sld, 8, 1, 1);
	
	// unsigned long slld_1 = (usrc_addr << 16);
	// char * sld_1 = (char *)&slld_1;
	// dpe__print_buffer( "usrc_addr << ", sld_1, 8, 1, 1);
	// exit(0);

	/*
	//Set 1
	dpe__test__create_single_session();
	dpe__test__create_single_packet( 1);
	dpe__test__create_single_packet( 6);
	dpe__test__create_single_packet( 17);
	dpe__test__create_single_packet( 50);
	*/
	
	//Set 2
	//dpe__test__create_sessions( 290000);
	
	//Set 3
	/*
	//TO Test Enconding Speed
	dpe__test__create_single_session();
	dpe__test__create_counter_thread();
	dpe__test__run_gtp_gi_load();
	
	while(1)
	{
		__cnt_pkts++;
		sleep(1);
	}
	*/
	
	//dpe__test__encode_icmppacket_to_ran();

	//exit(0);

	// dpdk_interface_t * interface 	= dpe__getinterface(0);
	// interface->ProcessType 			= PROCESS_TYPE__Gi_GTP;
	
	
	// struct rte_mbuf * pkts[1];
	// pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);
	
	// if( pkts[0])
	// {
		// char * srcMac = "\x00\x00\x23\x1f\x73\x48";
		// char * dstMac = "\x00\x00\x00\x00\x73\x48";
		
		// dpe_pkt__encode_gtp_echo_response_ipv4( pkts[0], srcMac, dstMac, htonl(3232273726), htonl(3232279726), 2152, 2152, 1, htonl(100), 1, 0, 0, 1);
	// }
	

	/*
	dpdk_interface_t * interface 	= dpe__getinterface(0);
	interface->ProcessType 			= PROCESS_TYPE__Gi_GTP;
	
	
	struct rte_mbuf * pkts[1];
	pkts[0] = rte_pktmbuf_alloc( interface->mempool[0]);
	
	if( pkts[0])
	{
		char * srcMac = "\x00\x00\x23\x1f\x73\x48";
		char * dstMac = "\x00\x00\x00\x00\x73\x48";
		
		dpe_pkt__encode_gtp_end_marker( pkts[0], srcMac, dstMac, htonl(3232273726), htonl(3232279726), 2152, 2152, htonl(100));

		pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./gtp_endmarker.pcap");
		dpdk_pkt__write_pcap( dumper2, pkts[0]);
		dpdk_pkt__close_pcap( dumper2);
		exit(0);
	}	
	*/
}

void dpe__test__gtp_gi_ipv4_ipv4()
{
}

void dpe__test__gi_gtp_ipv4_ipv4()
{
}


void dpe__test__gi_gtp_ipv4_ipv4_icmp()
{
	
	// dpdk_interface_t * interface 	= dpe__getinterface(0);
	// interface->ProcessType 			= PROCESS_TYPE__Gi_GTP;
	// //__dpe->SkipAPICalls 			= 1;

	// interface->TxInterface 			= interface;

	// struct rte_mbuf * processed_pkts[1];
	// struct rte_mbuf * dropped_mbufs[1];
	// struct rte_mbuf * arp_mbufs[0];
	// struct rte_mbuf * pkts[1];
	
	// pkts[0]							= rte_pktmbuf_alloc( interface->mempool[0]);
	// uint16_t recv_pkts				= 1;
	// uint16_t dropped_cnt_out 		= 0;
	// uint16_t arp_cnt_out			= 0;
	
	// char * gtpTPduPacket = 	NULL;
	// int gtplen = 0;

	// gtpTPduPacket = "\x00\x1d\x09\xf0\x92\xab\x00\x1a\x6b\x6c\x0c\xcc\x08\x00" \
			// "\x45\x00\x00\x1c\x16\xcf\x00\x00\x40\x01\x4f\xb6\x0a\x2d\x00\x01" \
			// "\x0a\x2d\x00\x02" \
			// "\x00\x00\x23\x1f\x73\x48\x69\x98";
	// gtplen = 14 + 20 + 8;
	
	// char * c_mptr4 = rte_pktmbuf_mtod( pkts[0], char *);
	// memcpy( c_mptr4, gtpTPduPacket, gtplen);
	// memcpy( &c_mptr4[0], interface->MacAddr, 6);
	
	// pkts[0]->pkt_len  = gtplen;
	// pkts[0]->data_len = gtplen;
	


	// uint16_t cnt = dpe__process_forward( interface, pkts, recv_pkts, processed_pkts, dropped_mbufs, &dropped_cnt_out, arp_mbufs, &arp_cnt_out, 0);
	
	// printf("cnt=%d  dropped_cnt_out=%d  arp_cnt_out=%d\n", cnt, dropped_cnt_out, arp_cnt_out);
	
	// if( cnt == 1)
	// {
		// pcap_dumper_t * dumper2 = dpdk_pkt__open_pcap_dump( "./icmp_2.pcap");
		// dpdk_pkt__write_pcap( dumper2, processed_pkts[0]);
		// dpdk_pkt__close_pcap( dumper2);
		// exit(0);
	// }

}

void dpe__test__gtp_gi_ipv6_ipv4()
{
}

void dpe__test__gi_gtp_ipv6_ipv4()
{
}

void dpe__test__gtp_gi_ipv4_ipv6()
{
}

void dpe__test__gi_gtp_ipv4_ipv6()
{
}

void dpe__test__gtp_gi_ipv6_ipv6()
{
}

void dpe__test__gi_gtp_ipv6_ipv6()
{
}





/*
	#cc -O3 -include rte_config.h -march=native -I/usr/local/include -I/usr/include/libnl3 -DALLOW_EXPERIMENTAL_API eng.c eng_support.c -o eng  -L/usr/local/lib/x86_64-linux-gnu -Wl,--whole-archive -l:librte_common_cpt.a -l:librte_common_dpaax.a -l:librte_common_iavf.a -l:librte_common_idpf.a -l:librte_common_octeontx.a -l:librte_bus_auxiliary.a -l:librte_bus_dpaa.a -l:librte_bus_fslmc.a -l:librte_bus_ifpga.a -l:librte_bus_pci.a -l:librte_bus_vdev.a -l:librte_bus_vmbus.a -l:librte_common_cnxk.a -l:librte_common_mlx5.a -l:librte_common_qat.a -l:librte_common_sfc_efx.a -l:librte_mempool_bucket.a -l:librte_mempool_cnxk.a -l:librte_mempool_dpaa.a -l:librte_mempool_dpaa2.a -l:librte_mempool_octeontx.a -l:librte_mempool_ring.a -l:librte_mempool_stack.a -l:librte_dma_cnxk.a -l:librte_dma_dpaa.a -l:librte_dma_dpaa2.a -l:librte_dma_hisilicon.a -l:librte_dma_idxd.a -l:librte_dma_ioat.a -l:librte_dma_skeleton.a -l:librte_net_af_packet.a -l:librte_net_ark.a -l:librte_net_atlantic.a -l:librte_net_avp.a -l:librte_net_axgbe.a -l:librte_net_bnx2x.a -l:librte_net_bnxt.a -l:librte_net_bond.a -l:librte_net_cnxk.a -l:librte_net_cxgbe.a -l:librte_net_dpaa.a -l:librte_net_dpaa2.a -l:librte_net_e1000.a -l:librte_net_ena.a -l:librte_net_enetc.a -l:librte_net_enetfec.a -l:librte_net_enic.a -l:librte_net_failsafe.a -l:librte_net_fm10k.a -l:librte_net_gve.a -l:librte_net_hinic.a -l:librte_net_hns3.a -l:librte_net_i40e.a -l:librte_net_iavf.a -l:librte_net_ice.a -l:librte_net_idpf.a -l:librte_net_igc.a -l:librte_net_ionic.a -l:librte_net_ipn3ke.a -l:librte_net_ixgbe.a -l:librte_net_liquidio.a -l:librte_net_memif.a -l:librte_net_mlx4.a -l:librte_net_mlx5.a -l:librte_net_netvsc.a -l:librte_net_nfp.a -l:librte_net_ngbe.a -l:librte_net_null.a -l:librte_net_octeontx.a -l:librte_net_octeon_ep.a -l:librte_net_pcap.a -l:librte_net_pfe.a -l:librte_net_qede.a -l:librte_net_ring.a -l:librte_net_sfc.a -l:librte_net_softnic.a -l:librte_net_tap.a -l:librte_net_thunderx.a -l:librte_net_txgbe.a -l:librte_net_vdev_netvsc.a -l:librte_net_vhost.a -l:librte_net_virtio.a -l:librte_net_vmxnet3.a -l:librte_raw_cnxk_bphy.a -l:librte_raw_cnxk_gpio.a -l:librte_raw_dpaa2_cmdif.a -l:librte_raw_ifpga.a -l:librte_raw_ntb.a -l:librte_raw_skeleton.a -l:librte_crypto_bcmfs.a -l:librte_crypto_caam_jr.a -l:librte_crypto_ccp.a -l:librte_crypto_cnxk.a -l:librte_crypto_dpaa_sec.a -l:librte_crypto_dpaa2_sec.a -l:librte_crypto_mlx5.a -l:librte_crypto_nitrox.a -l:librte_crypto_null.a -l:librte_crypto_octeontx.a -l:librte_crypto_openssl.a -l:librte_crypto_scheduler.a -l:librte_crypto_virtio.a -l:librte_compress_isal.a -l:librte_compress_mlx5.a -l:librte_compress_octeontx.a -l:librte_compress_zlib.a -l:librte_regex_mlx5.a -l:librte_regex_cn9k.a -l:librte_vdpa_ifc.a -l:librte_vdpa_mlx5.a -l:librte_vdpa_sfc.a -l:librte_event_cnxk.a -l:librte_event_dlb2.a -l:librte_event_dpaa.a -l:librte_event_dpaa2.a -l:librte_event_dsw.a -l:librte_event_opdl.a -l:librte_event_skeleton.a -l:librte_event_sw.a -l:librte_event_octeontx.a -l:librte_baseband_acc.a -l:librte_baseband_fpga_5gnr_fec.a -l:librte_baseband_fpga_lte_fec.a -l:librte_baseband_la12xx.a -l:librte_baseband_null.a -l:librte_baseband_turbo_sw.a -l:librte_node.a -l:librte_graph.a -l:librte_pipeline.a -l:librte_table.a -l:librte_pdump.a -l:librte_port.a -l:librte_fib.a -l:librte_ipsec.a -l:librte_vhost.a -l:librte_stack.a -l:librte_security.a -l:librte_sched.a -l:librte_reorder.a -l:librte_rib.a -l:librte_dmadev.a -l:librte_regexdev.a -l:librte_rawdev.a -l:librte_power.a -l:librte_pcapng.a -l:librte_member.a -l:librte_lpm.a -l:librte_latencystats.a -l:librte_jobstats.a -l:librte_ip_frag.a -l:librte_gso.a -l:librte_gro.a -l:librte_gpudev.a -l:librte_eventdev.a -l:librte_efd.a -l:librte_distributor.a -l:librte_cryptodev.a -l:librte_compressdev.a -l:librte_cfgfile.a -l:librte_bpf.a -l:librte_bitratestats.a -l:librte_bbdev.a -l:librte_acl.a -l:librte_timer.a -l:librte_hash.a -l:librte_metrics.a -l:librte_cmdline.a -l:librte_pci.a -l:librte_ethdev.a -l:librte_meter.a -l:librte_net.a -l:librte_mbuf.a -l:librte_mempool.a -l:librte_rcu.a -l:librte_ring.a -l:librte_eal.a -l:librte_telemetry.a -l:librte_kvargs.a -Wl,--no-whole-archive -Wl,--export-dynamic -lrt -latomic -lmlx5 -lpthread -lcrypto -ldl -pthread -lmlx4 -lpthread -libverbs -lbnxt_re-rdmav25 -lcxgb4-rdmav25 -lefa -lhns-rdmav25 -li40iw-rdmav25 -lmlx4 -lmlx5 -lmthca-rdmav25 -locrdma-rdmav25 -lqedr-rdmav25 -lvmw_pvrdma-rdmav25 -lhfi1verbs-rdmav25 -lipathverbs-rdmav25 -lrxe-rdmav25 -lsiw-rdmav25 -libverbs -lpthread -lnl-route-3 -lnl-3 -lpcap -lisal -lelf -lz -ljansson -Wl,--as-needed -lrte_node -lrte_graph -lrte_pipeline -lrte_table -lrte_pdump -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_dmadev -lrte_regexdev -lrte_rawdev -lrte_power -lrte_pcapng -lrte_member -lrte_lpm -lrte_latencystats -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_gpudev -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bpf -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -pthread -lm -ldl -lnuma -lfdt -lpcap -lbsd -L. -lTEDP
	#cc -O3 -include rte_config.h -march=native -I/usr/local/include -I/usr/include/libnl3 -DALLOW_EXPERIMENTAL_API -shared -fPIC -o libeng.so eng.c  -L/usr/local/lib/x86_64-linux-gnu -Wl,--whole-archive -l:librte_common_cpt.a -l:librte_common_dpaax.a -l:librte_common_iavf.a -l:librte_common_idpf.a -l:librte_common_octeontx.a -l:librte_bus_auxiliary.a -l:librte_bus_dpaa.a -l:librte_bus_fslmc.a -l:librte_bus_ifpga.a -l:librte_bus_pci.a -l:librte_bus_vdev.a -l:librte_bus_vmbus.a -l:librte_common_cnxk.a -l:librte_common_mlx5.a -l:librte_common_qat.a -l:librte_common_sfc_efx.a -l:librte_mempool_bucket.a -l:librte_mempool_cnxk.a -l:librte_mempool_dpaa.a -l:librte_mempool_dpaa2.a -l:librte_mempool_octeontx.a -l:librte_mempool_ring.a -l:librte_mempool_stack.a -l:librte_dma_cnxk.a -l:librte_dma_dpaa.a -l:librte_dma_dpaa2.a -l:librte_dma_hisilicon.a -l:librte_dma_idxd.a -l:librte_dma_ioat.a -l:librte_dma_skeleton.a -l:librte_net_af_packet.a -l:librte_net_ark.a -l:librte_net_atlantic.a -l:librte_net_avp.a -l:librte_net_axgbe.a -l:librte_net_bnx2x.a -l:librte_net_bnxt.a -l:librte_net_bond.a -l:librte_net_cnxk.a -l:librte_net_cxgbe.a -l:librte_net_dpaa.a -l:librte_net_dpaa2.a -l:librte_net_e1000.a -l:librte_net_ena.a -l:librte_net_enetc.a -l:librte_net_enetfec.a -l:librte_net_enic.a -l:librte_net_failsafe.a -l:librte_net_fm10k.a -l:librte_net_gve.a -l:librte_net_hinic.a -l:librte_net_hns3.a -l:librte_net_i40e.a -l:librte_net_iavf.a -l:librte_net_ice.a -l:librte_net_idpf.a -l:librte_net_igc.a -l:librte_net_ionic.a -l:librte_net_ipn3ke.a -l:librte_net_ixgbe.a -l:librte_net_liquidio.a -l:librte_net_memif.a -l:librte_net_mlx4.a -l:librte_net_mlx5.a -l:librte_net_netvsc.a -l:librte_net_nfp.a -l:librte_net_ngbe.a -l:librte_net_null.a -l:librte_net_octeontx.a -l:librte_net_octeon_ep.a -l:librte_net_pcap.a -l:librte_net_pfe.a -l:librte_net_qede.a -l:librte_net_ring.a -l:librte_net_sfc.a -l:librte_net_softnic.a -l:librte_net_tap.a -l:librte_net_thunderx.a -l:librte_net_txgbe.a -l:librte_net_vdev_netvsc.a -l:librte_net_vhost.a -l:librte_net_virtio.a -l:librte_net_vmxnet3.a -l:librte_raw_cnxk_bphy.a -l:librte_raw_cnxk_gpio.a -l:librte_raw_dpaa2_cmdif.a -l:librte_raw_ifpga.a -l:librte_raw_ntb.a -l:librte_raw_skeleton.a -l:librte_crypto_bcmfs.a -l:librte_crypto_caam_jr.a -l:librte_crypto_ccp.a -l:librte_crypto_cnxk.a -l:librte_crypto_dpaa_sec.a -l:librte_crypto_dpaa2_sec.a -l:librte_crypto_mlx5.a -l:librte_crypto_nitrox.a -l:librte_crypto_null.a -l:librte_crypto_octeontx.a -l:librte_crypto_openssl.a -l:librte_crypto_scheduler.a -l:librte_crypto_virtio.a -l:librte_compress_isal.a -l:librte_compress_mlx5.a -l:librte_compress_octeontx.a -l:librte_compress_zlib.a -l:librte_regex_mlx5.a -l:librte_regex_cn9k.a -l:librte_vdpa_ifc.a -l:librte_vdpa_mlx5.a -l:librte_vdpa_sfc.a -l:librte_event_cnxk.a -l:librte_event_dlb2.a -l:librte_event_dpaa.a -l:librte_event_dpaa2.a -l:librte_event_dsw.a -l:librte_event_opdl.a -l:librte_event_skeleton.a -l:librte_event_sw.a -l:librte_event_octeontx.a -l:librte_baseband_acc.a -l:librte_baseband_fpga_5gnr_fec.a -l:librte_baseband_fpga_lte_fec.a -l:librte_baseband_la12xx.a -l:librte_baseband_null.a -l:librte_baseband_turbo_sw.a -l:librte_node.a -l:librte_graph.a -l:librte_pipeline.a -l:librte_table.a -l:librte_pdump.a -l:librte_port.a -l:librte_fib.a -l:librte_ipsec.a -l:librte_vhost.a -l:librte_stack.a -l:librte_security.a -l:librte_sched.a -l:librte_reorder.a -l:librte_rib.a -l:librte_dmadev.a -l:librte_regexdev.a -l:librte_rawdev.a -l:librte_power.a -l:librte_pcapng.a -l:librte_member.a -l:librte_lpm.a -l:librte_latencystats.a -l:librte_jobstats.a -l:librte_ip_frag.a -l:librte_gso.a -l:librte_gro.a -l:librte_gpudev.a -l:librte_eventdev.a -l:librte_efd.a -l:librte_distributor.a -l:librte_cryptodev.a -l:librte_compressdev.a -l:librte_cfgfile.a -l:librte_bpf.a -l:librte_bitratestats.a -l:librte_bbdev.a -l:librte_acl.a -l:librte_timer.a -l:librte_hash.a -l:librte_metrics.a -l:librte_cmdline.a -l:librte_pci.a -l:librte_ethdev.a -l:librte_meter.a -l:librte_net.a -l:librte_mbuf.a -l:librte_mempool.a -l:librte_rcu.a -l:librte_ring.a -l:librte_eal.a -l:librte_telemetry.a -l:librte_kvargs.a -Wl,--no-whole-archive -Wl,--export-dynamic -lrt -latomic -lmlx5 -lpthread -lcrypto -ldl -pthread -lmlx4 -lpthread -libverbs -lbnxt_re-rdmav25 -lcxgb4-rdmav25 -lefa -lhns-rdmav25 -li40iw-rdmav25 -lmlx4 -lmlx5 -lmthca-rdmav25 -locrdma-rdmav25 -lqedr-rdmav25 -lvmw_pvrdma-rdmav25 -lhfi1verbs-rdmav25 -lipathverbs-rdmav25 -lrxe-rdmav25 -lsiw-rdmav25 -libverbs -lpthread -lnl-route-3 -lnl-3 -lpcap -lisal -lelf -lz -ljansson -Wl,--as-needed -lrte_node -lrte_graph -lrte_pipeline -lrte_table -lrte_pdump -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_dmadev -lrte_regexdev -lrte_rawdev -lrte_power -lrte_pcapng -lrte_member -lrte_lpm -lrte_latencystats -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_gpudev -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bpf -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -pthread -lm -ldl -lnuma -lfdt -lpcap -lbsd
*/

/*
int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf( "syntax %s <config.json>\n", argv[0]);
	}
	
	//dpe__init( argv[1]);
	dpe__init_engine2( argc, argv);
	
	while(1)
	{
		sleep(1);
	}
	
	return 0;
}
*/












