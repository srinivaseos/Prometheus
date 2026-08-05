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



int dpdk_qos__get_int( json_t * json_config, char * key, int defval, int minval)
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


int dpdk_qos__get_int2( json_t * json_config, char * key, int defval, int minval)
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

uint64_t dpdk_qos__get_u64( json_t * json_config, char * key, uint64_t defval, uint64_t minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		uint64_t v = json_integer_value( jObj);
		if( v < minval)
			return minval;
		else
			return v;
	}
	return defval;
}


double dpdk_qos__get_double( json_t * json_config, char * key, double defval, double minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		double v = json_real_value( jObj);
			return v;
	}
	return defval;
}


typedef struct dpdk_qos__config_qci
{
	struct dpdk_qos__config_qci * Next;

	uint32_t qci_id;					// qci-id
	uint32_t qci_subport;				
	uint32_t qci_resouce_type;			// 1 GBR, 2 Non-GBR
	double   qci_priority;

} dpdk_qos__config_qci_t;


typedef struct dpdk_qos__config_sub_port
{
	struct dpdk_qos__config_sub_port * Next;

	uint32_t sub_port_id;
	uint32_t sub_port_resouce_type;			// 1 GBR, 2 Non-GBR

} dpdk_qos__config_sub_port_t;


typedef struct dpdk_qos__config 
{
	uint32_t sub_ports;
	
	dpdk_qos__config_sub_port_t * sub_port_head;
	dpdk_qos__config_sub_port_t * sub_port_current;
	
	dpdk_qos__config_qci_t * qci_head;
	dpdk_qos__config_qci_t * qci_current;
	
	uint64_t uplink__tc_rate[13];
	uint64_t dwlink__tc_rate[13];
	
	uint32_t qsize_gbr;
	uint32_t qsize_nongbr;
	uint32_t pipes;
	uint32_t print_log;
	
} dpdk_qos__config_t;


dpdk_qos__config_t * __qos_config = NULL;


void  dpdk_qos__add_subport_ifNotExists( dpdk_qos__config_qci_t * qci_obj_item)
{
	int bFound = 0;
	
	dpdk_qos__config_sub_port_t * sp_item = __qos_config->sub_port_head;
	
	while( sp_item)
	{
		if( sp_item->sub_port_id == qci_obj_item->qci_subport)
		{
			bFound = 1;
			break;
		}
		
		sp_item = sp_item->Next;
	}
	
	
	if( bFound == 0)
	{
		sp_item = (dpdk_qos__config_sub_port_t*) malloc(sizeof(dpdk_qos__config_sub_port_t));
		memset( sp_item, 0, sizeof(dpdk_qos__config_sub_port_t));

		sp_item->sub_port_id 			= qci_obj_item->qci_subport;
		sp_item->sub_port_resouce_type 	= qci_obj_item->qci_resouce_type;

		if(!__qos_config->sub_port_head)
		{
			__qos_config->sub_port_head = __qos_config->sub_port_current = sp_item;
		}
		else
		{
			__qos_config->sub_port_current->Next = sp_item;
			__qos_config->sub_port_current = sp_item;
		}
		__qos_config->sub_ports++;
	}
}


int dpdk_qos__init( char * configfile)
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
		__qos_config = (dpdk_qos__config_t*)malloc(sizeof(dpdk_qos__config_t));
		
		if( __qos_config)
		{
			memset( __qos_config, 0, sizeof(dpdk_qos__config_t)); 

			__qos_config->sub_ports 		= 0;
			__qos_config->qci_head    		= NULL;
			__qos_config->qci_current 		= NULL;

			__qos_config->qsize_gbr			= 64;
			__qos_config->qsize_nongbr		= 64;
			__qos_config->pipes				= dpdk_qos__get_int( config, "pipes", 32, 32);
			__qos_config->print_log			= dpdk_qos__get_int( config, "print-log", 0, 0);

			json_t * jqsize = json_object_get( config, "qsize");
			
			if( jqsize)
			{
				__qos_config->qsize_gbr			= dpdk_qos__get_int( jqsize, "gbr", 64, 64);
				__qos_config->qsize_nongbr		= dpdk_qos__get_int( jqsize, "non-gbr", 64, 64);
			}
			
			

			json_t * qci = json_object_get( config, "qci");
			
			if( qci)
			{
				if( json_is_array(qci))
				{
					json_t * qci_item = NULL;
					
					int qci_i = 0;
					int qci_length = json_array_size( qci);
					
					uint32_t 	qci_id = 0;
					double 		qci_priority = 0;
					uint32_t 	qci_sub_port = 0;
					char *      qci_resource_type = NULL;
					int      	qci_resource_type_len = 0;
					
					uint8_t * dPtr = (uint8_t*)malloc(qci_length * sizeof(dpdk_qos__config_qci_t));
					dpdk_qos__config_qci_t * qci_obj_item = NULL;
					
					
					for( qci_i = 0; qci_i < qci_length; qci_i++)
					{
						qci_item = json_array_get( qci, qci_i);
						
						if( qci_item)
						{
							qci_id 					= dpdk_qos__get_int( qci_item, "qci-id", 0, 0);
							qci_sub_port 			= dpdk_qos__get_int( qci_item, "sub-port", 0, 0);
							qci_priority 			= dpdk_qos__get_double( qci_item, "priority", 0, 0);
							qci_resource_type		= NULL;
							qci_resource_type_len	= 0;

							
							if( json_object_get( qci_item, "resource-type"))
							{
								qci_resource_type		= (char*)json_string_value( json_object_get( qci_item, "resource-type"));
								qci_resource_type_len	= json_string_length( json_object_get( qci_item, "resource-type"));
							}
							
						
							if( qci_id > 0 && qci_sub_port > 0)
							{
								qci_obj_item = (dpdk_qos__config_qci_t*)dPtr;
								
								qci_obj_item->qci_id 				= qci_id;
								qci_obj_item->qci_subport			= qci_sub_port;
								qci_obj_item->qci_priority  		= qci_priority;
								qci_obj_item->qci_resouce_type 		= 2;
								qci_obj_item->Next 					= NULL;
								
								if( qci_resource_type_len >= 3)
								{
									if( memcmp(qci_resource_type, "gbr", 3) == 0)
									{
										qci_obj_item->qci_resouce_type = 1;
									}
									else
									{
										qci_obj_item->qci_resouce_type = 2;
									}
								}
								
								if(!__qos_config->qci_head)
								{
									__qos_config->qci_head = __qos_config->qci_current = qci_obj_item;
								}
								else
								{
									__qos_config->qci_current->Next = qci_obj_item;
									__qos_config->qci_current = qci_obj_item;
								}
								
								dPtr += sizeof(dpdk_qos__config_qci_t);
							}
						}
					}
					
					
					qci_obj_item = __qos_config->qci_head;
					
					while( qci_obj_item)
					{
						dpdk_qos__add_subport_ifNotExists( qci_obj_item);
						
						qci_obj_item = qci_obj_item->Next;
					}
				}
			}
			
			if( __qos_config->sub_ports == 0)
			{
				printf( "sub-ports should be greater than 0\n");
				exit(0);
			}

			if( __qos_config->sub_ports > 8)
			{
				printf( "allowed maximum sub-ports upto 8 only\n");
				exit(0);
			}
			
			
			json_t * tcmb = json_object_get( config, "tc-mb");
			
			if( tcmb)
			{
				if( json_is_array(tcmb))
				{
					json_t * tcmb_item = NULL;
					
					int tcmb_i = 0;
					int tcmb_length = json_array_size( tcmb);
					
					uint64_t uplink = 0;
					uint64_t downlink = 0;

					if( tcmb_length == 0)
					{
						printf( "tc-mb is empty\n");
						exit(0);
					}
					
					if( tcmb_length > 13)
					{
						printf( "tc-mb should be less than 14\n");
						exit(0);
					}
					
					for( tcmb_i = 0; tcmb_i < tcmb_length; tcmb_i++)
					{
						tcmb_item = json_array_get( tcmb, tcmb_i);
						
						if( tcmb_item)
						{
							uplink		= dpdk_qos__get_u64( tcmb_item, "uplink", 0, 0);
							downlink	= dpdk_qos__get_u64( tcmb_item, "downlink", 0, 0);
							
							if( uplink == 0 || downlink == 0)
							{
								printf( "index=%u invalid value for uplink=%lu or downlink=%lu\n", tcmb_i, uplink, downlink);
								exit(0);
							}
							
							__qos_config->uplink__tc_rate[tcmb_i] = uplink;
							__qos_config->dwlink__tc_rate[tcmb_i] = downlink;
						}
					}
					
					return 1;
				}
				else
				{
					printf( "tc-mb should be array\n");
					exit(0);				
				}
			}
			else
			{
				printf( "tc-mb not found\n");
				exit(0);				
			}
		}
	}
	
	return 0;
}


uint64_t dpdk_qos__convert__mb_to_bytes( uint32_t mb)
{
	uint64_t kbits = mb * 1000;
	return (kbits * 1000) / 8;
}


void dpdk_qos__find_user_pipe_and_q( uint64_t seid, uint16_t * pipe, uint16_t * nongbr_queue, uint16_t * gbr_queue)
{
	if(!__qos_config) return;
	if( __qos_config->sub_ports == 0) return;
	
	*pipe 			= seid % __qos_config->pipes;
	*nongbr_queue	= seid % __qos_config->qsize_nongbr;
	*gbr_queue		= seid % __qos_config->qsize_gbr;
}


uint16_t dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes)
{
	if(!__qos_config) return 100;
	if( __qos_config->sub_ports == 0) return 100;
	int j = 0;
	
	uint64_t last_tc_rate = 0;
	uint64_t tc_rate = 0;

	
	for( j = 0; j < 13; j++)
	{
		tc_rate = dpdk_qos__convert__mb_to_bytes( __qos_config->uplink__tc_rate[j]);
		
		if( user_speed_bytes >= last_tc_rate && user_speed_bytes <= tc_rate)
			return j;

		last_tc_rate = tc_rate;
	}
	
	return 12;
}


uint16_t dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes)
{
	if(!__qos_config) return 100;
	if( __qos_config->sub_ports == 0) return 100;
	int j = 0;

	uint64_t last_tc_rate = 0;
	uint64_t tc_rate = 0;


	for( j = 0; j < 13; j++)
	{
		tc_rate = dpdk_qos__convert__mb_to_bytes( __qos_config->dwlink__tc_rate[j]);
		
		if( user_speed_bytes >= last_tc_rate && user_speed_bytes <= tc_rate)
			return j;

		last_tc_rate = tc_rate;
	}

	return 12;
}


// side  = 1 uplink, 2 downlink
struct rte_sched_port * dpdk_qos__create_port( char * name, int socket, uint64_t link_rate, int side)
{
	if(!__qos_config) return NULL;
	if( __qos_config->sub_ports == 0) return NULL;
	
	int i = 0;
	int j = 0;

	// ; Bytes per second
	// tb rate = 1250000000           ; Bytes per second
	// tb size = 1000000              ; Bytes


	struct rte_sched_pipe_params pipe_profiles;

	pipe_profiles.tb_rate 			= 125000000;
	pipe_profiles.tb_size 			= 1000000;
	// 	 10000 = ( 10K-BITS per milli second, means 100*10000 = 10,00,000KBITS Per second, means 10 mb per second) 
	//.tc_rate = { 100, 2000, 3000, 4000, 50000, 60000, 70000, 80000, 90000, 100000, 110000, 120000, 130000},
	//pipe_profiles.tc_rate = { 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175, 305175},
					//milli-seconds (1000 milli-seconds to 1 second)

	pipe_profiles.tc_period 		= 10;
	pipe_profiles.tc_ov_weight 		= 1;
	pipe_profiles.wrr_weights[0] 	= 1; 
	pipe_profiles.wrr_weights[1] 	= 1; 
	pipe_profiles.wrr_weights[2] 	= 1; 
	pipe_profiles.wrr_weights[3] 	= 1; 


	uint64_t max_tc_rate = pipe_profiles.tb_rate;
	
	if( __qos_config->print_log == 1) {
		printf( "pipes=%u\n", __qos_config->pipes);
	}
	
	if( __qos_config->print_log == 1) {
		printf("\n");
	}
	
	for( j = 0; j < 13; j++) 
	{
		if( side == 1) 
		{
			pipe_profiles.tc_rate[j] = dpdk_qos__convert__mb_to_bytes( __qos_config->uplink__tc_rate[j]);

			if( __qos_config->print_log == 1) {
				printf( "pipe_profiles  m-bit=%-4lu  tc_rate[%2u] = %10lu   in bytes\n", __qos_config->uplink__tc_rate[j], j, pipe_profiles.tc_rate[j]);
			}
		} 
		else 
		{
			pipe_profiles.tc_rate[j] = dpdk_qos__convert__mb_to_bytes( __qos_config->dwlink__tc_rate[j]);
			
			if( __qos_config->print_log == 1) {
				printf( "pipe_profiles  m-bit=%-4lu  tc_rate[%2u] = %10lu   in bytes\n", __qos_config->dwlink__tc_rate[j], j, pipe_profiles.tc_rate[j]);
			}
		}
		
		if( pipe_profiles.tc_rate[j] > pipe_profiles.tb_rate)
		{
			printf( "\ntb_rate exceeds tc_rate, tb_rate[%u]=%lu  %lu., updating tb_rate to %lu\n\n", j, pipe_profiles.tc_rate[j], pipe_profiles.tb_rate, pipe_profiles.tc_rate[j]);
			pipe_profiles.tb_rate = pipe_profiles.tc_rate[j];
			max_tc_rate = pipe_profiles.tc_rate[j];
		}
	}

	if( __qos_config->print_log == 1) {
		printf("\n");
	}
	

	struct rte_sched_subport_params subport_params[__qos_config->sub_ports];
	memset( &subport_params, 0, sizeof(struct rte_sched_subport_params) * __qos_config->sub_ports);
	
	dpdk_qos__config_sub_port_t * sp_item = __qos_config->sub_port_head;
	i = 0;
	
	while( sp_item)
	{
		subport_params[i].n_pipes_per_subport_enabled 	= __qos_config->pipes;
		subport_params[i].pipe_profiles 				= &pipe_profiles;
		subport_params[i].n_pipe_profiles 				= 1;
		subport_params[i].n_max_pipe_profiles 			= MAX_SCHED_PIPE_PROFILES;

		for( j = 0; j < 13; j++) 
		{
			if( sp_item->sub_port_resouce_type == 1)
			{
				subport_params[i].qsize[j] = __qos_config->qsize_gbr;
			}
			else
			{
				subport_params[i].qsize[j] = __qos_config->qsize_nongbr;
			}
			
			if( __qos_config->print_log == 1) {
				printf( "sub-port[%d] qsize[%2d] = %-4d\n", i, j, subport_params[i].qsize[j]);
			}
		}

		i++;
		sp_item = sp_item->Next;
	}
	
	
	if( __qos_config->print_log == 1) {
		printf("\n");
	}
	
	
	
	struct rte_sched_subport_profile_params subport_profile;
	memset( &subport_profile, 0, sizeof(struct rte_sched_subport_profile_params));
	
	subport_profile.tb_rate = 1250000000;				// ; Bytes per second
	
	if( subport_profile.tb_rate < max_tc_rate)
	{
		subport_profile.tb_rate = max_tc_rate;
	}		
	
	subport_profile.tb_size = 1000000;					// ; Bytes per second
	
	for( j = 0; j < 13; j++)
		subport_profile.tc_rate[j] = 1250000000;		// ; Bytes per second
	
	subport_profile.tc_period = 10;

	
	struct rte_sched_port_params port_params;
	memset( &port_params, 0, sizeof(struct rte_sched_port_params));
	
	port_params.name 					= name;
	port_params.socket 					= socket;
	port_params.rate 					= (uint64_t) link_rate * 1000 * 1000 / 8;
	port_params.mtu 					= 6 + 6 + 4 + 4 + 2 + 1500;
	port_params.frame_overhead 			= RTE_SCHED_FRAME_OVERHEAD_DEFAULT;
	port_params.n_subports_per_port 	= __qos_config->sub_ports;
	port_params.n_subport_profiles 		= 1;
	port_params.subport_profiles 		= &subport_profile;
	port_params.n_max_subport_profiles 	= MAX_SCHED_SUBPORT_PROFILES;
	port_params.n_pipes_per_subport 	= __qos_config->pipes;
	
	struct rte_sched_port * port = rte_sched_port_config( &port_params);
	
	
	if(!port)
	{
		printf("port creation failed with name=%s  sub_ports=%u   pipes=%u  rate=%lu\n", name, __qos_config->sub_ports, __qos_config->pipes, port_params.rate);
		return NULL;
	}
	


	int err								= 0;
	int subport 						= 0;
	uint32_t pipe						= 0;	
	uint32_t n_pipes_per_subport 		= 0;


	sp_item = __qos_config->sub_port_head;
	i = 0;
	
	while( sp_item)
	{
		subport = i;
		err = rte_sched_subport_config( port, subport, &subport_params[i], 0);
		
		if( err < 0)
		{
			printf( "sub-port creation failed at index=%u   sub-port-id=%u   LINE=%d\n", i, sp_item->sub_port_id, __LINE__);
			exit(0);
		}
		
		i++;
		sp_item = sp_item->Next;
	}
	
	
	
	sp_item = __qos_config->sub_port_head;
	i = 0;
	
	while( sp_item)
	{
		subport = i;
		
		n_pipes_per_subport = subport_params[subport].n_pipes_per_subport_enabled;
		
		for( pipe = 0; pipe < n_pipes_per_subport; pipe++) 
		{
			err = rte_sched_pipe_config( port, subport, pipe, 0);
			
			if( err < 0)
			{
				printf( " rte_sched_pipe_config-error:  port=%p  subport=%d  n_pipes_per_subport=%d  pipe=%u  err=%d  LINE=%d\n", port, subport, n_pipes_per_subport, pipe, err, __LINE__);
				exit(0);
			}
		}

		i++;
		sp_item = sp_item->Next;		
	}
	
	printf( "rte-sched port configured succesfully\n");
	return port;
}


#if 0

int main( int argc, char* argv[])
{
	printf("\n");

	int ret = rte_eal_init( argc, argv);
	if (ret < 0)
	{
		rte_panic("Cannot init EAL\n");
		exit(0);
	}


	
	


	int sts = dpdk_qos__init( "./dpdk_qos.json");


	
	if( sts == 1)
	{
		struct rte_sched_port * port = dpdk_qos__create_port( "port-1", 0, 10000, 1);
		
		uint16_t last_lcore = rte_get_next_lcore( 0, 1, 0) + 1;
		
		
		
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

#endif
