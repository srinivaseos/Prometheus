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



int dpdk_core__process_pkt( struct rte_mbuf * pkt, dpdk_pkt_info_t * info)
{
	if( pkt->pkt_len == 0)
	{
		if( info->iff->LogPPResult == 1) {
			dpdk__log( 5, "core-drop: packet length is 0  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__ );
		}
		
		info->result = PKT_FWD__DROP;
		return PKT_FWD__DROP;
	}

	int sts = dpdk_parse_pkt__parse( pkt, info);
	
	if( sts == 0)
	{
		if( info->result == PKT_FWD__ARP)
		{
			dpdk__log( 5, "core-arp: %s|%s|%d", __FILE__, __FUNCTION__, __LINE__ );
			return 0;
		}		
		
		void * sPtr 			= NULL; 
		uint64_t seid 			= 0;
		dpdk_flow_t * flow 		= NULL;

		switch( info->ue_ipv)
		{
			case 4:
				{
					dpdk_session__ipv4_find( info->ue_dst_ip, &sPtr);

					if( info->iff->LogIPHeader == 1) 
					{
						dpdk__log( 5, "core-pkt-header: ipv4 ue-ip:%u.%u.%u.%u v4ip=%u pfcp-session=%p dst-ip:%u.%u.%u.%u  ue-port:%u  dst-port:%u  proto:%u  %s|%s|%d", 
							info->ue_dst_ip & 0xFF, info->ue_dst_ip >> 8 & 0xFF, info->ue_dst_ip >> 16 & 0xFF, info->ue_dst_ip >> 24 & 0xFF, info->ue_dst_ip, sPtr,
							info->ue_src_ip & 0xFF, info->ue_src_ip >> 8 & 0xFF, info->ue_src_ip >> 16 & 0xFF, info->ue_src_ip >> 24 & 0xFF, 
							htons(info->ue_dst_port), htons(info->ue_src_port), info->ue_proto, __FILE__, __FUNCTION__, __LINE__ 
						);
					}
					
					if(!sPtr)
					{
						if( info->iff->LogPPResult == 1 || info->iff->LogSession == 1) 
						{
							dpdk__log( 5, "core-drop: ipv4 session not found with ue-ip:%u.%u.%u.%u  %s|%s|%d",
								info->ue_dst_ip & 0xFF, info->ue_dst_ip >> 8 & 0xFF, info->ue_dst_ip >> 16 & 0xFF, info->ue_dst_ip >> 24 & 0xFF, __FILE__, __FUNCTION__, __LINE__ 
							);
						}

						info->result = PKT_FWD__DROP;
						return 0;
					}
				}
				break;
			case 6:
				{
					if( info->ue_dst_ip6)
					{
						dpdk_session__ipv6_find( info->ue_dst_ip6, &sPtr);
						
						if( info->iff->LogIPHeader == 1) 
						{
							dpdk__log( 5, "core-pkt-header: ipv6 ue-ip:%u:%u:%u:%u  dst-ip:%u:%u:%u:%u  src-port:%u  dst-port:%u  proto:%u  %s|%s|%d", 
								info->ue_dst_ip6[12] & 0xFF, info->ue_dst_ip6[13] & 0xFF, info->ue_dst_ip6[14] & 0xFF, info->ue_dst_ip6[15] & 0xFF,
								info->ue_src_ip6[12] & 0xFF, info->ue_src_ip6[13] & 0xFF, info->ue_src_ip6[14] & 0xFF, info->ue_src_ip6[15] & 0xFF, 
								htons(info->ue_dst_port), htons(info->ue_src_port), info->ue_proto, __FILE__, __FUNCTION__, __LINE__ 
							);
						}
							
						if(!sPtr)
						{
							if( info->iff->LogPPResult == 1) 
							{
								dpdk__log( 5, "core-drop: ipv6 session not found with ue-ip:%u:%u:%u:%u  %s|%s|%d",
									info->ue_dst_ip6[12] & 0xFF, info->ue_dst_ip6[13] & 0xFF, info->ue_dst_ip6[14] & 0xFF, info->ue_dst_ip6[15] & 0xFF, __FILE__, __FUNCTION__, __LINE__ 
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
							dpdk__log( 5, "core-drop: ue-ipv6 is NULL  %s|%s|%d", __FILE__, __FUNCTION__, __LINE__ );
						}

						info->result = PKT_FWD__DROP;
						return 0;
					}					
				}
				break;
			default:
				{
					if( info->iff->LogPPResult == 1) {
						dpdk__log( 5, "core-drop: packet parsing failed, ip-version=%u|%u not determined  %s|%s|%d", info->ipv, info->ue_ipv, __FILE__, __FUNCTION__, __LINE__ );
					}
		
					info->result = PKT_FWD__DROP;
					return 0;
				}
				return 0;
		}			
		
		seid = dpdk_session__get_seid( sPtr);
		uint64_t quota = dpdk_session__get_quota( sPtr, 1);		// 1 is default rating group
		
		if( info->iff->LogSession == 1) 
		{
			switch( info->ue_ipv)
			{
				case 4:
					{
						dpdk__log( 5, "core-session: ipv4 ue-ip:%u.%u.%u.%u  seid:%lu  def-quota:%lu",
							info->ue_dst_ip & 0xFF, info->ue_dst_ip >> 8 & 0xFF, info->ue_dst_ip >> 16 & 0xFF, info->ue_dst_ip >> 24 & 0xFF, seid, quota
						);
					}
					break;
				case 6:
					{
						dpdk__log( 5, "core-session: ipv6 ue-ip:%u.%u.%u.%u  seid:%lu  def-quota:%lu",
							info->ue_dst_ip6[12] & 0xFF, info->ue_dst_ip6[13] & 0xFF, info->ue_dst_ip6[14] & 0xFF, info->ue_dst_ip6[15] & 0xFF, seid, quota
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
				dpdk__log( 5, "core-drop: quota not found for seid=%lu", seid);
			}			
			
			info->result = PKT_FWD__DROP;
			return 0;
		}
		
		if( dpdk_ipflow__is_dns_packet2( info->ue_proto, info->ue_src_port) == 1 )
		{
			if( info->iff->LogPPResult == 1) {
				dpdk__log( 5, "core-allow: identified as dns packet for seid=%lu  proto:%u  dst-port:%u", seid, info->ue_proto, info->ue_src_port);
			}
			
			dpdk_session__record_usage( NULL, sPtr, NULL, pkt->pkt_len, 0);
			info->result = PKT_FWD__ALLOW;
			return 0;
		}

		dpdk_session_t * session = dpdk_session__get_dpi_session( seid);
		flow = dpdk_session__find_flow( seid, info->ue_ipv, info->ue_dst_ip, info->ue_src_ip, info->ue_dst_ip6, info->ue_src_ip6, info->ue_dst_port, info->ue_src_port, info->ue_proto);
		
		if(!flow)
		{
			// in some cases ue acts as socket server
			flow = dpdk_session__create_flow( info->ue_ipv, info->ue_dst_ip, info->ue_src_ip, info->ue_dst_ip6, info->ue_src_ip6, info->ue_dst_port, info->ue_src_port, info->ue_proto);
			dpdk_session__addflow( seid, session, 0, flow);
			
			if( info->iff->LogFlow == 1) 
			{
				dpdk__log( 5, "core-flow: created flow for seid=%lu  proto:%u  src-port:%u  dst-port:%u pkt-len=%u", 
					seid, info->ue_proto, info->ue_src_port, info->ue_dst_port, pkt->pkt_len);
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
				dpdk_session__record_usage( flow, sPtr, session, 0, pkt->pkt_len);
				
				if( info->iff->nic->CreateOuterHeader == 1)
				{
					uint32_t	teid		= 0;
					uint8_t		ipv			= 0;
					uint32_t	ranip		= 0;
					uint32_t	upfip		= 0;
					uint8_t	  * ranip6		= NULL;
					uint8_t	  * upfip6		= NULL;
					char	  * ranmac		= NULL;
					char 	  * upfmac		= NULL;
					int 		gtpHasSQN	= 0;
					uint16_t 	gtp_seq_no	= 0;
					int ohi_sts = 0;
					
					
					if( (ohi_sts = dpdk_session__get_ohi( sPtr, &teid, &ipv, &ranip, &upfip, &ranip6, &upfip6, &ranmac, &upfmac, &gtpHasSQN, &gtp_seq_no)) == 1)
					{
						ranmac = info->iff->target_if->nic->DstMAC;
						upfmac = info->iff->target_if->nic->SrcMAC;
						
						//void dpdk_pkt__print_d( char * data, int len);
						//dpdk_pkt__print_d( ranmac, 6);
						//dpdk_pkt__print_d( upfmac, 6);
						
						dpdk_pkt__create_outer_header_256( pkt, upfmac, ranmac, ipv, upfip, ranip, upfip6, ranip6, teid, gtpHasSQN, gtp_seq_no);
					}
					else
					{
						if( info->iff->LogPPResult == 1) 
						{
							dpdk__log( 5, "core-drop: outer header creation failed for seid=%lu with sts=%d sPtr=%p", seid, ohi_sts, sPtr);
							info->result = PKT_FWD__DROP;
							return 0;
						}	
					}
					
					pcap_dumper_t * dumper = dpdk_pkt__open_pcap_dump( "./pcap_ohi.pcap");
					
					if( dumper)
					{
						dpdk_pkt__write_pcap( dumper, pkt);
						dpdk_pkt__close_pcap( dumper);
					}
					
				}

				if( info->iff->LogPPResult == 1) {
					dpdk__log( 5, "core-allowed: pkt-len=%u available-quota=%lu rgid=%u seid=%lu", pkt->pkt_len, quota, flow->rgid, seid);
				}
				
				info->result = PKT_FWD__ALLOW;
				return 0;
			}
			else
			{
				info->result = PKT_FWD__DROP;
				return 0;				
			}
		}
		
		info->result = PKT_FWD__DROP;
		return 0;
	}
	else
	{
		if( info->iff->LogPPResult == 1) {
			dpdk__log( 5, "DROP: packet parsing status returned with sts=%d", sts);
		}		
	
		info->result = PKT_FWD__DROP;
		return 0;	
	}

	
	return 0;
}
















