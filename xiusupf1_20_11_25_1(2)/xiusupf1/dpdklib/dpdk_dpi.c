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

#if USENDPI
	#include "upfndpi.h"
#endif


/*
	URR-ID = RG
	
	RG 1 -  DEFAULT
	RG 2 - WHATSAPP
	RG 3 - FB-VOIP
*/



void dpdk_dpi__l7inspection( dpdk_flow_t * flow, uint8_t * iph, uint32_t len)
{
	if( flow->rgid == 0)
	{
		flow->rgid = 1; // default rating group
	}
	#if USENDPI
		
		if( flow->rg_identified == 0)
		{
			
			if(!flow->ndpi_flow) 
			{
				flow->ndpi_flow = upfndpi_allocate();
				printf("flow->ndpi_flow=%p\n",flow->ndpi_flow);
			}
			
			upfndpi_processpacket( rte_lcore_index( rte_lcore_id()), flow->ndpi_flow, iph, len, 0);
			
			if( upfndpi_protocol_detected( flow->ndpi_flow) == 1)
			{
				upfndpi_protocol_info( flow->ndpi_flow, &flow->master_protocol, &flow->app_protocol, &flow->category);
				upfndpi_protocol_getsni( flow->ndpi_flow, flow->sni, 150);				
			
				upfndpi_release( flow->ndpi_flow);
				flow->ndpi_flow = NULL;
				
				flow->rgid = dpdk_get_rgid( flow->master_protocol, flow->app_protocol);
				flow->rg_identified = 1;
				printf("rgid=%u app_protocol=%u master_protocol=%u\n",flow->rgid,flow->app_protocol,flow->master_protocol);
			}
		
		}
	
	
	#endif
	
}















