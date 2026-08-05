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

uint16_t dpdk_ipflow__is_dns_packet2( uint8_t proto, uint16_t port)
{
	return 0;
}

uint16_t dpdk_ipflow__is_dns_packet( dpdk_flow_key_t * dpdk_flow_key)
{
	return 0;
}


uint16_t dpdk_ipflow__is_packet_allowed_without_quota( dpdk_flow_key_t * dpdk_flow_key)
{
	if( dpdk_ipflow__is_dns_packet( dpdk_flow_key) == 1)
	{
		return 1;
	}
}










