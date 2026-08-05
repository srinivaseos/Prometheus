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



int dpdk_access__process_pkt( struct rte_mbuf * pkt, dpdk_pkt_info_t * info)
{
	if( pkt->pkt_len == 0)
	{
		if( info->iff->LogPPResult == 1) {
			dpdk__log( 5, "access-drop: packet length is 0  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		}
		
		info->result = PKT_FWD__DROP;
		return 0;
	}
	
	int sts = dpdk_parse_pkt__parse( pkt, info);
	
	if( sts == 0)
	{
		if( info->result == PKT_FWD__ARP)
		{
			dpdk__log( 5, "access-arp: %s|%s|%d", __FILE__, __FUNCTION__, __LINE__ );
			return 0;
		}

		void * sPtr 			= NULL; 
		uint64_t seid 			= 0;
		dpdk_flow_t * flow 		= NULL;

		switch( info->ue_ipv)
		{
			case 4:
				{
					dpdk_session__ipv4_find( info->ue_src_ip, &sPtr);
					
					if( info->iff->LogIPHeader == 1) 
					{
						dpdk__log( 5, "access-pkt-header: ipv4 ue-ip:%u.%u.%u.%u v4ip=%u pfcp-session=%p dst-ip:%u.%u.%u.%u  src-port:%u  dst-port:%u  proto:%u  %s|%s|%d", 
							info->ue_src_ip & 0xFF, info->ue_src_ip >> 8 & 0xFF, info->ue_src_ip >> 16 & 0xFF, info->ue_src_ip >> 24 & 0xFF, info->ue_src_ip, sPtr,
							info->ue_dst_ip & 0xFF, info->ue_dst_ip >> 8 & 0xFF, info->ue_dst_ip >> 16 & 0xFF, info->ue_dst_ip >> 24 & 0xFF,
							htons(info->ue_src_port), htons(info->ue_dst_port), info->ue_proto, __FILE__, __FUNCTION__, __LINE__
						);
					}
					
					if(!sPtr)
					{
						if( info->iff->LogPPResult == 1 || info->iff->LogSession == 1) 
						{
							dpdk__log( 5, "access-drop: ipv4 session not found with ue-ip:%u.%u.%u.%u  %s|%s|%d",
								info->ue_src_ip & 0xFF, info->ue_src_ip >> 8 & 0xFF, info->ue_src_ip >> 16 & 0xFF, info->ue_src_ip >> 24 & 0xFF, __FILE__, __FUNCTION__, __LINE__
							);
						}
						
						info->result = PKT_FWD__DROP;
						return 0;
					}
				}
				break;
			case 6:
				{
					if( info->ue_src_ip6)
					{
						dpdk_session__ipv6_find( info->ue_src_ip6, &sPtr);

						if( info->iff->LogIPHeader == 1) 
						{
							dpdk__log( 5, "access-pkt-header: ipv6 ue-ip:%u:%u:%u:%u  dst-ip:%u:%u:%u:%u  src-port:%u  dst-port:%u  proto:%u  %s|%s|%d", 
								info->ue_src_ip6[12] & 0xFF, info->ue_src_ip6[13] & 0xFF, info->ue_src_ip6[14] & 0xFF, info->ue_src_ip6[15] & 0xFF, 
								info->ue_dst_ip6[12] & 0xFF, info->ue_dst_ip6[13] & 0xFF, info->ue_dst_ip6[14] & 0xFF, info->ue_dst_ip6[15] & 0xFF,
								htons(info->ue_src_port), htons(info->ue_dst_port), info->ue_proto, __FILE__, __FUNCTION__, __LINE__
							);
						}
					
						if(!sPtr)
						{
							if( info->iff->LogPPResult == 1) 
							{
								dpdk__log( 5, "access-drop: ipv6 session not found with ue-ip:%u:%u:%u:%u  %s|%s|%d",
									info->ue_src_ip6[12] & 0xFF, info->ue_src_ip6[13] & 0xFF, info->ue_src_ip6[14] & 0xFF, info->ue_src_ip6[15] & 0xFF, __FILE__, __FUNCTION__, __LINE__
								);
							}
							
							info->result = PKT_FWD__DROP;
							return 0;
						}
					}
					else
					{
						if( info->iff->LogPPResult == 1) 
						{
							dpdk__log( 5, "access-drop: ue-ipv6 is NULL  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
						}

						info->result = PKT_FWD__DROP;
						return 0;
					}
				}
				break;
			default:
				{
					if( info->iff->LogPPResult == 1) {
						dpdk__log( 5, "access-drop: packet parsing failed, unknown ip-version=%u|%u  %s|%s|%d", info->ipv, info->ue_ipv, __FILE__, __FUNCTION__, __LINE__);
					}
		
					info->result = PKT_FWD__DROP;
					return 0;
				}
				break;
		}
		
		
		seid = dpdk_session__get_seid( sPtr);
		uint64_t quota = dpdk_session__get_quota( sPtr, 1);	// 1 is default rating group
		
		if( info->iff->LogSession == 1) 
		{
			switch( info->ue_ipv)
			{
				case 4:
					{
						dpdk__log( 5, "access-session: ipv4 ue-ip=%u.%u.%u.%u  seid=%lu  def-quota=%lu  %s|%s|%d",
							info->ue_src_ip & 0xFF, info->ue_src_ip >> 8 & 0xFF, info->ue_src_ip >> 16 & 0xFF, info->ue_src_ip >> 24 & 0xFF, seid, quota, __FILE__, __FUNCTION__, __LINE__
						);
					}
					break;
				case 6:
					{
						dpdk__log( 5, "access-session: ipv6 ue-ip=%u.%u.%u.%u  seid=%lu  def-quota=%lu  %s|%s|%d",
							info->ue_src_ip6[12] & 0xFF, info->ue_src_ip6[13] & 0xFF, info->ue_src_ip6[14] & 0xFF, info->ue_src_ip6[15] & 0xFF, seid, quota, __FILE__, __FUNCTION__, __LINE__
						);
					}
					break;				
				default:
					break;
			}
		}
		
		if( quota == 0)
		{
			if( info->iff->LogPPResult == 1) {
				dpdk__log( 5, "access-drop: quota not found for seid=%lu  %s|%s|%d", seid, __FILE__, __FUNCTION__, __LINE__);
			}
					
			info->result = PKT_FWD__DROP;
			return 0;
		}
		
		if( dpdk_ipflow__is_dns_packet2( info->ue_proto, info->ue_dst_port) == 1 )
		{
			if( info->iff->LogPPResult == 1) {
				dpdk__log( 5, "access-allow: identified as dns packet for seid=%lu  proto:%u  dst-port:%u  %s|%s|%d", seid, info->ue_proto, info->ue_dst_port, __FILE__, __FUNCTION__, __LINE__);
			}
			
			dpdk_session__record_usage( NULL, sPtr, NULL, pkt->pkt_len, 0);
			info->result = PKT_FWD__ALLOW;
			return 0;
		}

		dpdk_session_t * session = dpdk_session__get_dpi_session( seid);
		flow = dpdk_session__find_flow( seid, info->ue_ipv, info->ue_src_ip, info->ue_dst_ip, info->ue_src_ip6, info->ue_dst_ip6, info->ue_src_port, info->ue_dst_port, info->ue_proto);

		if(!flow)
		{
			flow = dpdk_session__create_flow( info->ue_ipv, info->ue_src_ip, info->ue_dst_ip, info->ue_src_ip6, info->ue_dst_ip6, info->ue_src_port, info->ue_dst_port, info->ue_proto);
			dpdk_session__addflow( seid, session, 0, flow);

			if( info->iff->LogFlow == 1) 
			{
				dpdk__log( 5, "access-flow: created flow for seid=%lu  proto:%u  src-port:%u  dst-port:%u pkt-len=%u  %s|%s|%d", 
					seid, info->ue_proto, info->ue_src_port, info->ue_dst_port, pkt->pkt_len, __FILE__, __FUNCTION__, __LINE__ );
			}
		}
		
		if(flow)
		{
			if( flow->rg_identified == 0)
			{
				if( dpdk__enabled_packet_inspection())
				{
					dpdk_dpi__l7inspection( flow, info->iph, info->iplen);
				}
			}
			
			uint64_t rg_quota = dpdk_session__get_quota( sPtr, flow->rgid);
			
			if( rg_quota >= pkt->pkt_len || quota >= pkt->pkt_len)
			{
				dpdk_session__record_usage( flow, sPtr, session, pkt->pkt_len, 0);
				
				if( dpdk_pkt__remove_outer_header_256( pkt, info->iff->target_if->nic->SrcMAC, info->iff->target_if->nic->DstMAC) != 0)
				{
					if( info->iff->LogPPResult == 1) {
						dpdk__log( 5, "access-drop: outer header removal failed for seid=%lu  %s|%s|%d", seid, __FILE__, __FUNCTION__, __LINE__ );
					}					
					
					info->result = PKT_FWD__DROP;
					return 0;
				}

				if( info->iff->LogPPResult == 1) {
					dpdk__log( 5, "access-allowed: pkt-len=%u available-quota=%lu rgid=%u seid=%lu  %s|%s|%d", pkt->pkt_len, quota, flow->rgid, seid, __FILE__, __FUNCTION__, __LINE__ );
				}
				
				// pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./pcap_ohr.pcap");
				
				// if( dumper)
				// {
					// dpdk_pkt__write_pcap( dumper, pkt);
					// dpdk_pkt__close_pcap( dumper);
				// }
				
				info->result = PKT_FWD__ALLOW;
				return 0;
			}
			else
			{				
				if( info->iff->LogPPResult == 1) {
					dpdk__log( 5, "access-drop: insufficent quota, pkt-len=%u available quota=%lu seid=%lu  %s|%s|%d", pkt->pkt_len, quota, seid, __FILE__, __FUNCTION__, __LINE__ );
				}
				
				info->result = PKT_FWD__DROP;
				return 0;
			}			
		}

		if( info->iff->LogPPResult == 1) {
			dpdk__log( 5, "access-drop: flow creation failed seid=%lu  %s|%s|%d", seid, __FILE__, __FUNCTION__, __LINE__ );
		}
				
		info->result = PKT_FWD__DROP;
		return 0;
	}
	else
	{
		if( info->iff->LogPPResult == 1) {
			dpdk__log( 5, "access-drop: packet parsing status returned with sts=%d  %s|%s|%d", sts, __FILE__, __FUNCTION__, __LINE__ );
		}
	
		info->result = PKT_FWD__DROP;
		return 0;
	}
	return 0;
}




