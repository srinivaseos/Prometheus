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


int dpdk_parse_pkt__parse_gtp( dpdk_pkt_info_t * info, struct rte_gtp_hdr * gtp, unsigned char ** payload)
{
	uint8_t gtp_hdr_info = gtp->gtp_hdr_info;
	uint8_t ver = gtp_hdr_info >> 5;
	uint8_t pt = (gtp_hdr_info & 0x10) >> 4;
	uint8_t gtp_ext = (gtp_hdr_info & 0x04);

	//info->result = PKT_FWD__DROP;
	if( pt != 1)
	{
		info->result = PKT_FWD__DROP;
		return -2;
	}

	if( ver != 1)
	{
		info->result = PKT_FWD__DROP;
		return -3;
	}
	
	unsigned char * buff = ((unsigned char *)gtp) + 4;
	info->teid = (uint32_t) (( buff[0] << 24) & 0xFF000000) + ((buff[1] << 16) & 0x00FF0000) + ((buff[2] << 8) & 0x0000FF00) + (buff[3] & 0x000000FF);
	info->is_seqn_present = ((gtp_hdr_info & 0x2) >> 1);
	
	if( info->is_seqn_present == 1) 
	{
		unsigned char * sqnptr = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr));
		info->seqn_no = (sqnptr[0] << 8) + sqnptr[1];
	}
	
	if( info->is_seqn_present == 1 || gtp_ext)
	{
		if(!gtp_ext) 
		{
			*payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4);
		}
		else
		{
			uint8_t extlen = *(uint8_t*)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4);

			//TODO: loop and check for no extension and then assign below pointer
			*payload = (unsigned char *)((unsigned char *)gtp + sizeof(struct rte_gtp_hdr) + 4 + (extlen * 4));
		}
	}
	
	return 0;
}

int dpdk_parse_pkt__parse_ueinfo( dpdk_pkt_info_t * info, unsigned char * payload)
{
	if(!payload)
	{
		info->result = PKT_FWD__DROP;
		return -1;
	}
	
	struct rte_ipv4_hdr * pl_ip_hdr  	= NULL;
	struct rte_ipv6_hdr * pl_ip_hdr6 	= NULL;
	struct rte_udp_hdr * udp 			= NULL;
	struct rte_tcp_hdr * tcp 			= NULL;
	
	unsigned char * proto_packet = NULL;
	
	if( ((payload[0] & 0xF0) >> 4) == 4)
	{
		pl_ip_hdr = (struct rte_ipv4_hdr *)payload;
		info->ue_ipv 		= 4; 
		info->ue_proto 		= pl_ip_hdr->next_proto_id;
		
		info->ue_src_ip 	= pl_ip_hdr->src_addr;
		info->ue_dst_ip 	= pl_ip_hdr->dst_addr;
		
		proto_packet = payload + sizeof(struct rte_ipv4_hdr);
	}
	else
	{
		pl_ip_hdr6 = (struct rte_ipv6_hdr *)payload;
		info->ue_ipv 		= 6;
		info->ue_proto 		= pl_ip_hdr6->proto;

		info->ue_src_ip6 	= pl_ip_hdr6->src_addr;
		info->ue_dst_ip6 	= pl_ip_hdr6->dst_addr;
		
		proto_packet = payload + sizeof(struct rte_ipv6_hdr);
	}
	
	if( info->ue_proto == IPPROTO_UDP)
	{
		udp = (struct rte_udp_hdr *)proto_packet;
		
		info->ue_src_port = udp->src_port;
		info->ue_dst_port = udp->dst_port;
	}
	else if( info->ue_proto == IPPROTO_TCP)
	{
		tcp = (struct rte_tcp_hdr *)proto_packet;
		
		info->ue_src_port = tcp->src_port;
		info->ue_dst_port = tcp->dst_port;		
	}
	else if( info->ue_proto == 50)
	{
		info->ue_src_port = *(uint16_t*)&proto_packet[0];
		info->ue_dst_port = *(uint16_t*)&proto_packet[2];
	}
	else
	{
		info->ue_src_port = *(uint16_t*)&proto_packet[0];
		info->ue_dst_port = *(uint16_t*)&proto_packet[2];
	}
	
	return 0;
}

int dpdk_parse_pkt__parse( struct rte_mbuf * mbuf, dpdk_pkt_info_t * info)
{
	info->result = 0;
	info->eth_type = 0;
	info->ipv = 0;
	info->proto = 0;
	info->result = 0;
	info->is_gtp_encapsulated = 0;
	info->is_seqn_present = 0;
	info->seqn_no = 0;
	info->ue_ipv = 0;
	info->ue_proto = 0;
	info->ue_src_ip = 0;
	info->ue_dst_ip = 0;
	info->ue_src_ip6 = NULL;
	info->ue_dst_ip6 = NULL;
	info->ue_src_port = 0;
	info->ue_dst_port = 0;
	info->access_ip = 0;
	info->n_ip = 0;
	info->access_ip6 = NULL;
	info->n_ip6 = NULL;
	info->teid = 0;
	info->iph = NULL;
	info->iplen = 0;

	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);
	info->eth_type = rte_cpu_to_be_16( eth->ether_type);									// be_16
	//printf( "info->eth_type=%d ether_type=%d\n", info->eth_type, eth->ether_type);
	
	switch( info->eth_type)
	{
		case RTE_ETHER_TYPE_IPV4:
			{
				struct rte_ipv4_hdr * ip_hdr = (struct rte_ipv4_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));
				info->ipv = 4;
				info->proto = ip_hdr->next_proto_id;
				
				unsigned char * payload = NULL;
				int usedbytes = 14;
				
				if( info->iff->nic->IsGTPEncapsulated == 1)
				{
					info->is_gtp_encapsulated = info->iff->nic->IsGTPEncapsulated;
					
					if( info->proto != IPPROTO_UDP)
					{
						info->result = PKT_FWD__DROP;
						return -1;
					}
					
					info->n_ip 				= ip_hdr->src_addr;
					info->access_ip 		= ip_hdr->dst_addr;
					
					struct rte_udp_hdr * udp = (struct rte_udp_hdr *)((unsigned char *)ip_hdr + 20);
					struct rte_gtp_hdr * gtp = (struct rte_gtp_hdr *)((unsigned char *)udp + sizeof(struct rte_udp_hdr));
					
					int gtsp_sts = 0;
					if( (gtsp_sts = dpdk_parse_pkt__parse_gtp( info, gtp, &payload)) != 0)
					{
						info->result = PKT_FWD__DROP;
						return gtsp_sts;
					}
					
					if(!payload)
					{
						info->result = PKT_FWD__DROP;
						return -4;
					}
					
					usedbytes += 20;
					usedbytes += sizeof(struct rte_udp_hdr);
					usedbytes += 8;
					
					if( info->is_seqn_present == 1)
						usedbytes += 4;
				}
				else
				{
					payload = (unsigned char *)ip_hdr;
				}
				
				info->iph = payload;
				info->iplen = (mbuf->pkt_len-usedbytes);
				dpdk_parse_pkt__parse_ueinfo( info, payload);
				return 0;
			}	
			break;
		case RTE_ETHER_TYPE_IPV6:
			{
				struct rte_ipv6_hdr * ip_hdr6 = (struct rte_ipv6_hdr *)((unsigned char *)eth + sizeof(struct rte_ether_hdr));
				info->ipv = 6;
				info->proto = ip_hdr6->proto;
				
				unsigned char * payload = NULL;
				int usedbytes = 14;
				
				if( info->iff->nic->IsGTPEncapsulated == 1)
				{
					info->is_gtp_encapsulated = info->iff->nic->IsGTPEncapsulated;
					
					if( info->proto != IPPROTO_UDP)
					{
						info->result = PKT_FWD__DROP;
						return -1;
					}
					
					info->n_ip6 			= ip_hdr6->src_addr;
					info->access_ip6 		= ip_hdr6->dst_addr;
					
					struct rte_udp_hdr * udp = (struct rte_udp_hdr *)((unsigned char *)ip_hdr6 + 40);
					struct rte_gtp_hdr * gtp = (struct rte_gtp_hdr *)((unsigned char *)udp + sizeof(struct rte_udp_hdr));
					
					int gtsp_sts = 0;
					if( (gtsp_sts = dpdk_parse_pkt__parse_gtp( info, gtp, &payload)) != 0)
					{
						info->result = PKT_FWD__DROP;
						return gtsp_sts;
					}
					
					if(!payload)
					{
						info->result = PKT_FWD__DROP;
						return -4;
					}
					
					usedbytes += 40;
					usedbytes += sizeof(struct rte_udp_hdr);
					usedbytes += 8;
					
					if( info->is_seqn_present == 1)
						usedbytes += 4;					
				}
				else
				{
					payload = (unsigned char *)ip_hdr6;
				}
				
				info->iph = payload;
				info->iplen = (mbuf->pkt_len - usedbytes);
				dpdk_parse_pkt__parse_ueinfo( info, payload);
				return 0;
			}
			break;
		case RTE_ETHER_TYPE_ARP:
			{
				info->result = PKT_FWD__ARP;
				info->arp_targetip4 = *(uint32_t*)((unsigned char *)eth + sizeof(struct rte_ether_hdr) + 24);
				// char * d = ((unsigned char *)eth + sizeof(struct rte_ether_hdr) + 24);
				
				// printf( "targetIp=%u | %u %u.%u.%u.%u %u.%u.%u.%u\n", 
					// targetIp, ntohl(targetIp),
					// d[0] & 0xFF,d[1] & 0xFF,d[2] & 0xFF,d[3] & 0xFF,
					// d[3] & 0xFF,d[2] & 0xFF,d[1] & 0xFF,d[0]  & 0xFF
					// );
				// exit(0);
			}
			break;
		default:
			info->ipv = info->eth_type;
			break;
	}
	
	return 0;
}



int dpdk_parse_pkt__parse_and_print( struct rte_mbuf * mbuf, dpdkif_t * dif)
{
	dpdk_pkt_info_t linfo;
	dpdk_pkt_info_t * info = &linfo;
	info->iff = dif;
	
	dpdk_parse_pkt__parse( mbuf, info);
	
	printf( "result=%d\n", info->result);
	printf( "eth_type=%u | %u\n", info->eth_type, rte_cpu_to_be_16(info->eth_type));
	printf( "proto=%u\n", info->proto);
	printf( "is_gtp_encapsulated=%u\n", info->is_gtp_encapsulated);
	printf( "ipv=%u\n", info->ipv);
	
	if( info->ipv == 4) 
	{
		printf( "access_ip=%u n_ip=%u\n", info->access_ip, info->n_ip);
	} 
	else if( info->ipv == 6) 
	{
		if( info->access_ip6)
			dpdk_pkt__print_d( (char *) info->access_ip6, 16);
		
		if( info->n_ip6)
			dpdk_pkt__print_d( (char *) info->n_ip6, 16);
	}
	
	printf( "is_seqn_present=%u seqn_no=%u teid=%u\n", info->is_seqn_present, info->seqn_no, info->teid);	

	printf( "ue_ipv=%u ue_proto=%u\n", info->ue_ipv, info->ue_proto);

	if( info->ue_ipv == 4) 
	{
		printf( "ue_src_ip=%u ue_dst_ip=%u\n", info->ue_src_ip, info->ue_dst_ip);
	} 
	else if( info->ue_ipv == 6) 
	{
		if( info->ue_src_ip6)
			dpdk_pkt__print_d( (char *) info->ue_src_ip6, 16);
		
		if( info->ue_dst_ip6)
			dpdk_pkt__print_d( (char *) info->ue_dst_ip6, 16);
	}
	
	printf( "ue_src_port=%u ue_dst_port=%u\n", info->ue_src_port, info->ue_dst_port);
}





void dpdk_parse_pkt__test_case_1( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		Validated
			Encapsulated
			IP-V4
			
		
		result=0
		eth_type=2048 | 8
		proto=17
		is_gtp_encapsulated=1
		ipv=4
		access_ip=1363247572 n_ip=390169044
		is_seqn_present=1 seqn_no=0 teid=134348800
		ue_ipv=4 ue_proto=17
		ue_src_ip=342862016 ue_dst_ip=4285507776
		ue_src_port=35072 ue_dst_port=35072
	*/
	
	unsigned char * data = 	"\xff\xff\xff\xff\xff\xff\x00\x25\xd3\xce\x5d\xf9\x08\x00\x45\x00" \
							"\x00\x76\x00\x00\x00\x00\x3f\x11\x50\x0c\xd4\x81\x41\x17\xd4\x81" \
							"\x41\x51\x08\x68\x08\x68\x00\x62\x00\x00\x32\xff\x00\x52\x08\x02" \
							"\x00\x00\x00\x00\xff\x00\x45\x00\x00\x4e\x5c\xe5\x00\x00\x80\x11" \
							"\x7d\x55\xc0\xa8\x6f\x14\xc0\xa8\x6f\xff\x00\x89\x00\x89\x00\x3a" \
							"\x2b\x50\xf5\x33\x01\x10\x00\x01\x00\x00\x00\x00\x00\x00\x20\x45" \
							"\x4b\x45\x42\x45\x4f\x45\x4f\x45\x42\x45\x4d\x45\x42\x46\x41\x46" \
							"\x45\x45\x50\x46\x41\x43\x41\x43\x41\x43\x41\x43\x41\x43\x41\x00" \
							"\x00\x20\x00\x01";
	int datalen = 244;
	
	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);
}


void dpdk_parse_pkt__test_case_2( struct rte_mempool * mempool, dpdkif_t * dif)
{
	// ARP PACKET
	// Validated - PKT_FWD__ARP
	
	// result=4
	
	unsigned char * data = 	"\xff\xff\xff\xff\xff\xff\x00\x1a\x6b\x6c\x0c\xcc\x08\x06\x00\x01" \
							"\x08\x00\x06\x04\x00\x01\x00\x1a\x6b\x6c\x0c\xcc\x0a\x0a\x0a\x02" \
							"\x00\x00\x00\x00\x00\x00\x0a\x0a\x0a\x01\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	int datalen = 60;
	
	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);
}


void dpdk_parse_pkt__test_case_3( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		Validated
			User Packet
				Validated IP, Protocol and Ports
		
		result=0
		eth_type=2048 | 8
		proto=17
		is_gtp_encapsulated=0
		ipv=4
		access_ip=0 n_ip=0
		is_seqn_present=0 seqn_no=0 teid=0
		ue_ipv=4 ue_proto=17
		ue_src_ip=16777343 ue_dst_ip=16777343
		ue_src_port=42968 ue_dst_port=25890	
	*/

	unsigned char * data = 	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x45\x00" \
							"\x00\x3a\xd7\x54\x40\x00\x40\x11\x65\x5c\x7f\x00\x00\x01\x7f\x00" \
							"\x00\x01\xd8\xa7\x22\x65\x00\x26\xfe\x39\x20\x05\x00\x1a\x00\x00" \
							"\x01\x00\x00\x3c\x00\x05\x00\x7f\x00\x00\x01\x00\x60\x00\x04\x00" \
							"\x00\x00\x19\x00\x59\x00\x01\x00";
	int datalen = 72;

	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);	
}

void dpdk_parse_pkt__test_case_4( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		//IP SEC-PACKET
			
			
		result=0
		eth_type=2048 | 8
		proto=50
		is_gtp_encapsulated=0
		ipv=4
		access_ip=0 n_ip=0
		is_seqn_present=0 seqn_no=0 teid=0
		ue_ipv=4 ue_proto=50
		ue_src_ip=3364661440 ue_dst_ip=3448547520
		ue_src_port=30984 ue_dst_port=23349	
	*/
	
	unsigned char * data = 	"\x00\x0c\x29\xc5\x7d\xdb\x00\x0c\x29\x4f\xee\xa2\x08\x00\x45\x00" \
							"\x00\x70\xe4\xd4\x00\x00\x40\x32\xfa\xa0\xc0\xa8\x8c\xc8\xc0\xa8" \
							"\x8c\xcd\x08\x79\x35\x5b\x00\x00\x00\x01\x28\x23\x92\x40\x59\xce" \
							"\xbb\x4e\x65\xfb\x26\x91\xd8\xba\xff\xf9\x4e\xdc\x75\xa6\xe6\x0e" \
							"\xe6\x80\x7b\x5b\x81\x8e\xfd\x47\x07\x35\xa2\x3a\x0e\xfe\x5f\x84" \
							"\xf1\x2e\x83\x0c\x77\x72\xb0\x03\xec\x8f\x1e\x3f\xf6\xf0\x94\x80" \
							"\x67\xda\xe4\xfc\x94\x53\x11\x86\x11\xa0\x8a\x11\x6e\xfd\x27\x95" \
							"\x68\xee\x35\x6d\x32\x67\x8b\x1e\xf0\x0c\x97\xd7\x8a\xff";
	int datalen = 126;

	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);
}


void dpdk_parse_pkt__test_case_5( struct rte_mempool * mempool, dpdkif_t * dif)
{
	unsigned char * data = 	"\x00\x0c\x29\xc5\x7d\xdb\x00\x0c\x29\x4f\xee\xa2\x08\x00\x45\x00" \
							"\x00\x60\x00\x00\x40\x00\x40\x11\x9f\xa6\xc0\xa8\x8c\xc8\xc0\xa8" \
							"\x8c\xcd\x01\xf4\x01\xf4\x00\x4c\xdb\x2a\xc6\xd1\x45\x92\x85\x15" \
							"\x0c\x7e\x6e\x96\x1f\x01\xbf\x17\x9b\x35\x05\x10\x02\x01\x00\x00" \
							"\x00\x00\x00\x00\x00\x44\xeb\x53\xa6\x19\xf9\x00\xa6\x00\xd0\x6b" \
							"\xde\x3b\x07\x87\x82\xaa\xce\x00\x67\x64\x7d\x66\x19\x05\x8e\x10" \
							"\xbb\x74\x1d\x34\x0d\xee\x52\xd9\xa5\xe8\x88\xb1\xcc\xf6";
	int datalen = 110;
	
	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);	
}

void dpdk_parse_pkt__test_case_6( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		//Validated
				
		result=0
		eth_type=2048 | 8
		proto=17
		is_gtp_encapsulated=1
		ipv=4
		access_ip=117440639 n_ip=33554559
		is_seqn_present=0 seqn_no=0 teid=1
		ue_ipv=4 ue_proto=1
		ue_src_ip=33565962 ue_dst_ip=16788746
		ue_src_port=0 ue_dst_port=0
	*/
	
	unsigned char * data = 	"\x03\x04\x00\x06\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00" \
							"\x45\x00\x00\xe4\x6f\x16\x40\x00\x40\x11\xcc\xe9\x7f\x00\x00\x02" \
							"\x7f\x00\x00\x07\x08\x68\x08\x68\x00\xd0\xfe\xea\x34\xff\x00\x24" \
							"\x00\x00\x00\x01\x00\x00\x00\x85\x01\x10\x01\x00\x45\x00\x00\x1c" \
							"\xc6\x23\x00\x00\xff\x01\xe1\x60\x0a\x2d\x00\x02\x0a\x2d\x00\x01" \
							"\x08\x00\x1b\x1f\x73\x48\x69\x98\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00";
	int datalen = 242;
	
	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);	
}


void dpdk_parse_pkt__test_case_7( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		Validated 
		
		result=0
		eth_type=34525 | 56710
		proto=6
		is_gtp_encapsulated=0
		ipv=6
		is_seqn_present=0 seqn_no=0 teid=0
		ue_ipv=6 ue_proto=6
		00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01
		00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01
		ue_src_port=24862 ue_dst_port=26835	
	*/

	unsigned char * data = 	"\x03\x04\x00\x06\x00\x00\x00\x00\x00\x00\x00\x00\x86\xdd" \
							"\x60\x02\xde\x4b\x00\xf4\x06\x40\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00" \
							"\x00\x00\x00\x00\x00\x00\x00\x01\x1e\x61\xd3\x68\x96\x8d\x03\x7a" \
							"\xac\xcb\xbb\xdd\x80\x18\x00\x40\x00\xfc\x00\x00\x01\x01\x08\x0a" \
							"\xf4\xd4\x7d\xad\xf4\xd4\x7d\xac\x48\x54\x54\x50\x2f\x31\x2e\x31" \
							"\x20\x32\x30\x31\x20\x43\x72\x65\x61\x74\x65\x64\x0d\x0a\x43\x6f" \
							"\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x3a\x20\x4b\x65\x65\x70\x2d\x41" \
							"\x6c\x69\x76\x65\x0d\x0a\x43\x6f\x6e\x74\x65\x6e\x74\x2d\x4c\x65" \
							"\x6e\x67\x74\x68\x3a\x20\x35\x30\x32\x0d\x0a\x43\x6f\x6e\x74\x65" \
							"\x6e\x74\x2d\x54\x79\x70\x65\x3a\x20\x61\x70\x70\x6c\x69\x63\x61" \
							"\x74\x69\x6f\x6e\x2f\x6a\x73\x6f\x6e\x0d\x0a\x4c\x6f\x63\x61\x74" \
							"\x69\x6f\x6e\x3a\x20\x2f\x6e\x6e\x72\x66\x2d\x6e\x66\x6d\x2f\x76" \
							"\x31\x2f\x6e\x66\x2d\x69\x6e\x73\x74\x61\x6e\x63\x65\x73\x2f\x39" \
							"\x33\x38\x64\x64\x37\x32\x34\x2d\x65\x34\x38\x66\x2d\x34\x31\x65" \
							"\x61\x2d\x39\x66\x33\x36\x2d\x37\x39\x66\x36\x66\x32\x34\x38\x61" \
							"\x33\x35\x39\x0d\x0a\x44\x61\x74\x65\x3a\x20\x53\x61\x74\x2c\x20" \
							"\x32\x32\x20\x41\x75\x67\x20\x32\x30\x32\x30\x20\x31\x35\x3a\x35" \
							"\x33\x3a\x30\x39\x20\x47\x4d\x54\x0d\x0a\x0d\x0a";
	int datalen = 298;
	
	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);	
}	
	
	
void dpdk_parse_pkt__test_case_8( struct rte_mempool * mempool, dpdkif_t * dif)
{
	/*
		Validated

		result=0
		eth_type=2048 | 8
		proto=132
		is_gtp_encapsulated=0
		ipv=4
		access_ip=0 n_ip=0
		is_seqn_present=0 seqn_no=0 teid=0
		ue_ipv=4 ue_proto=132
		ue_src_ip=83886207 ue_dst_ip=16777343
		ue_src_port=3222 ue_dst_port=26801	
	*/

	unsigned char * data =  "\x03\x04\x00\x06\x00\x00\x00\x00\x00\x00\x63\x62\x08\x00" \
							"\x45\x02\x00\x54\xb3\xfc\x40\x00\x40\x84\x88\x21\x7f\x00\x00\x05" \
							"\x7f\x00\x00\x01\x96\x0c\xb1\x68\xea\x2c\x58\x1b\x00\x00\x00\x00" \
							"\x03\x00\x00\x10\x80\x73\x03\x62\x00\x01\xa0\x00\x00\x00\x00\x00" \
							"\x00\x03\x00\x24\x05\x35\xea\x4b\x00\x01\x00\x03\x00\x00\x00\x3c" \
							"\x00\x29\x00\x10\x00\x00\x02\x00\x72\x00\x04\x00\x25\x00\x01\x00" \
							"\x0f\x40\x01\x40";
	int datalen = 98;

	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, data, datalen);
	
	tpdu_buf4->pkt_len  = datalen;
	tpdu_buf4->data_len = datalen;
	
	dpdk_pkt__print( tpdu_buf4);
	dpdk_parse_pkt__parse_and_print( tpdu_buf4, dif);		
}	
	
	
	
	
	
	
	
	
	
	
	
	