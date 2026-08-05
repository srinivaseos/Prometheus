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

void dpdk_pkt__print( struct rte_mbuf * m)
{
	char * data = rte_pktmbuf_mtod( m, char *);
	int len = m->pkt_len;
	int i = 0;
	
	for( i = 0 ; i < len; i++)
	{
		printf("%02X ", data[i] & 0xFF);
	}
	
	printf("\n");
}

void dpdk_pkt__print_d( char * data, int len)
{
	int i = 0;
	
	for( i = 0 ; i < len; i++)
	{
		printf("%02X ", data[i] & 0xFF);
	}
	
	printf("\n");
}




int dpdk_pkt__encode_arp_response( struct rte_mbuf * mbuf_req, struct rte_mbuf * mbuf_rsp, char * srcmac, uint32_t srcip)
{
	struct rte_ether_hdr * 	request_eth = rte_pktmbuf_mtod( mbuf_req, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	request_arp = (struct rte_arp_hdr *)&request_eth[1];

	struct rte_ether_hdr * 	response_eth = rte_pktmbuf_mtod( mbuf_rsp, struct rte_ether_hdr *);
	struct rte_arp_hdr * 	response_arp = (struct rte_arp_hdr *)&response_eth[1];


	//eth
    //rte_ether_addr_copy( &request_eth->dst_addr, &response_eth->src_addr);
	memcpy( &response_eth->src_addr, srcmac, 6);
	rte_ether_addr_copy( &request_eth->src_addr, &response_eth->dst_addr);
	response_eth->ether_type = htons(RTE_ETHER_TYPE_ARP);

	
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
	
	
	
	/*
	printf( "sending arp response: target mac=%02X:%02X:%02X:%02X:%02X:%02X|%u\n", tarmac[0] & 0xFF, 
		tarmac[1] & 0xFF, tarmac[2] & 0xFF, tarmac[3] & 0xFF, tarmac[4] & 0xFF, tarmac[5] & 0xFF, 
		response_arp->arp_data.arp_tip);
		
	printf( "sending arp response: sender mac=%02X:%02X:%02X:%02X:%02X:%02X|%u\n", srcmac[0] & 0xFF, 
		srcmac[1] & 0xFF, srcmac[2] & 0xFF, srcmac[3] & 0xFF, srcmac[4] & 0xFF, srcmac[5] & 0xFF, srcip);
	*/	


    mbuf_rsp->pkt_len  = 60;
    mbuf_rsp->data_len = 60;
	
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
	
	return 0;
}

int dpdk_pkt__encode_gtp_echo_response_ipv4( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t msgid, uint8_t teid, uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi)
{
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
	
	if( ext == 1)
		ip_hdr->total_length 	= htons(20 + 8 + 8 + 8);		//ipv4=20 + udp=8 + gtp=8
	else
		ip_hdr->total_length 	= htons(20 + 8 + 8);		//ipv4=20 + udp=8 + gtp=8
	
	ip_hdr->packet_id 		= 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live 	= 64;
	ip_hdr->next_proto_id 	= 17;
	ip_hdr->hdr_checksum 	= 0;
	
	ip_hdr->src_addr 		= dst_ip;
	ip_hdr->dst_addr 		= src_ip;
	
	ip_hdr->hdr_checksum 	= 0;
	ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	udp->src_port				= rte_cpu_to_be_16(dst_port);
	udp->dst_port				= rte_cpu_to_be_16(src_port);	
	
	if( ext == 1)
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8 + 8);		
	else
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8);
	
	udp->dgram_cksum			= 0;
	udp->dgram_cksum			= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	
	
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= msgid;
	gtp->plen 					= rte_cpu_to_be_16(8);
	gtp->teid 					= teid; 
	
	if( ext == 0)
	{
		mbuf->pkt_len  = (14 + 20 + 8 + 8);
		mbuf->data_len = (14 + 20 + 8 + 8);
	}
	else
	{
		gtp->gtp_hdr_info = 0x34;
		gtp->plen 				= rte_cpu_to_be_16(16);
		mbuf->pkt_len  = (14 + 20 + 8 + 8 + 8);
		mbuf->data_len = (14 + 20 + 8 + 8 + 8);		
	}
	
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
	
	return 0;
}




int dpdk_pkt__encode_gtp_echo_response_ipv6( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, uint8_t * src_ip, uint8_t * dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t msgid, uint8_t teid, uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi)
{
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);

	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);
	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	eth->ether_type = rte_cpu_to_be_16( RTE_ETHER_TYPE_IPV6);

	struct rte_ipv6_hdr * ip_hdr 	= NULL;
	struct rte_udp_hdr  * udp  		= NULL;
	struct rte_gtp_hdr  * gtp  		= NULL;
	
	ip_hdr = (struct rte_ipv6_hdr *)(((unsigned char *)eth) + 14);
	
	ip_hdr->proto 	= 17;
	ip_hdr->hop_limits = 64;
	
	if( ext == 1)
		ip_hdr->payload_len = rte_cpu_to_be_16( 24);
	else
		ip_hdr->payload_len = rte_cpu_to_be_16( 16);
	
	ip_hdr->vtc_flow = htonl(6 << 28);

	memcpy( &ip_hdr->src_addr , src_ip, 16);
	memcpy( &ip_hdr->dst_addr , dst_ip, 16);
	
	udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv6_hdr));
	gtp = (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));		

	udp->src_port				= rte_cpu_to_be_16(dst_port);
	udp->dst_port				= rte_cpu_to_be_16(src_port);	
	
	if( ext == 1)
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8 + 8);
	else	
		udp->dgram_len				= rte_cpu_to_be_16(8 + 8);		
	
	udp->dgram_cksum			= 0;
	udp->dgram_cksum			= rte_ipv6_udptcp_cksum( ip_hdr, udp);
	
	
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= msgid;
	gtp->plen 					= rte_cpu_to_be_16(8);
	gtp->teid 					= teid; 

	//uint8_t ext, uint8_t type, uint8_t pdu_type, uint8_t qfi
	if( ext == 1)
	{
		gtp->gtp_hdr_info 			= 0x34;
		gtp->plen 					= rte_cpu_to_be_16(16);
		mbuf->pkt_len  = (14 + 40 + 8 + 8 + 8);
		mbuf->data_len = (14 + 40 + 8 + 8 + 8);
	}
	else
	{
		mbuf->pkt_len  = (14 + 40 + 8 + 8);
		mbuf->data_len = (14 + 40 + 8 + 8);		
	}
	
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
	
	return 0;
}

int dpdk_pkt__remove_outer_header_256( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac)
{
	int adjlen = 0;
	int ipv = 4;
	int gtp_seqn_present = 0;
	struct rte_gtp_hdr * gtp = NULL;
	
	struct rte_ether_hdr * eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);
	char * bPtr = rte_pktmbuf_mtod( mbuf, char *);
	
	if( eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6)) 
	{
		ipv = 6;
	}
	
	if( ipv == 4) 
	{
		adjlen = (20 + 8 + 8);	//IP + UDP + GTP
		gtp = ((struct rte_gtp_hdr *)&bPtr[28 + 14]);
	} 
	else if( ipv == 6) 
	{
		adjlen = (40 + 8 + 8);
		gtp = (struct rte_gtp_hdr *)&bPtr[48 + 14];
	} 
	else 
	{
		return -1;
	}

	// dpdk_pkt__print_d( (char *)eth, mbuf->pkt_len);
	// dpdk_pkt__print_d( (char *)gtp, mbuf->pkt_len);
	
	uint8_t * gtp_bPtr = (uint8_t *)gtp;
	
	if( gtp_bPtr[0] & 0x02)
	{
		gtp_seqn_present = 1;
		adjlen += 4;
	}
	
	rte_pktmbuf_adj( mbuf, adjlen);
	
	
	eth = rte_pktmbuf_mtod( mbuf, struct rte_ether_hdr *);
	

	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);

	if( ipv == 4) {
		eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
	} else {
		eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6);
	}

	return 0;
}


int dpdk_pkt__create_outer_header_256( struct rte_mbuf * mbuf, char * src_mac, char * dst_mac, int an_ipv, uint32_t upfipv4, uint32_t anipv4, uint8_t * upfipv6, uint8_t * anipv6, uint32_t teid, int gtpHasSQN, uint16_t gtp_seq_no)
{
	unsigned char * packet = rte_pktmbuf_mtod( mbuf, unsigned char *);
	int orig_pkt_len = mbuf->pkt_len;


	//int gtpIPVersion = 4;
	int prependLen = 0;
	//int gtpHasSQN = 1;
	int gtpHeaderLen = (gtpHasSQN == 1) ? 12 : 8;
	
	if( an_ipv == 4) {
		prependLen = 20 + 8 + gtpHeaderLen;
	} 
	else {
		prependLen = 40 + 8 + gtpHeaderLen;
	}


	char * p = rte_pktmbuf_prepend( mbuf, prependLen);
	
	//printf( "p=%p prependLen=%d\n", p, prependLen);
	
	if(!p)
	{
		printf("prependLen failed\n");
		exit(0);
	}
			
			
	struct rte_ether_hdr * eth = (struct rte_ether_hdr *)p;
	struct rte_udp_hdr * udp  	= NULL;
	struct rte_ipv4_hdr * ip_hdr = NULL;
	struct rte_ipv6_hdr * ip6_hdr = NULL;
	
	rte_memcpy( eth->src_addr.addr_bytes, src_mac, 6);
	rte_memcpy( eth->dst_addr.addr_bytes, dst_mac, 6);

	
	//ip
	if( an_ipv == 4) 
	{
		eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
	
		//ip_hdr = (struct rte_ipv4_hdr *)(((unsigned char *)eth) + 14);
		ip_hdr = (struct rte_ipv4_hdr *)&p[14];
		
		ip_hdr->version_ihl 	= 0x45;
		ip_hdr->type_of_service = 0x00;
		ip_hdr->total_length 	= rte_cpu_to_be_16((orig_pkt_len-14) + prependLen);
		ip_hdr->packet_id 		= 0;
		ip_hdr->fragment_offset = 0;
		ip_hdr->time_to_live 	= 64;
		ip_hdr->next_proto_id 	= 17;
		ip_hdr->hdr_checksum 	= 0;
		
		ip_hdr->src_addr 		= rte_cpu_to_be_32( upfipv4);
		ip_hdr->dst_addr 		= rte_cpu_to_be_32( anipv4);
		
		ip_hdr->hdr_checksum 	= 0;
		ip_hdr->hdr_checksum 	= rte_ipv4_cksum( ip_hdr);
		

		udp = (struct rte_udp_hdr *) ((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));
	} 
	else 
	{
		eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6);
		
		ip6_hdr = (struct rte_ipv6_hdr *)&p[14];
		ip6_hdr->proto 	= 17;
		ip6_hdr->hop_limits = 64;
		ip6_hdr->payload_len = rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen + 8);
		ip6_hdr->vtc_flow = htonl(6 << 28);
		
		memcpy( &ip6_hdr->src_addr , upfipv6, 16);
		memcpy( &ip6_hdr->dst_addr ,  anipv6, 16);
		
		udp = (struct rte_udp_hdr *) ((unsigned char *)ip6_hdr + sizeof(struct rte_ipv6_hdr));
		
		printf("udp=%p ip_hdr=%p sz=%ld\n", udp, ip6_hdr, sizeof(struct rte_ipv6_hdr));
	}


	//udp
	udp->src_port				= 26632;	//rte_cpu_to_be_32( 2152);
	udp->dst_port				= 26632;	//rte_cpu_to_be_32( 2152);	
	udp->dgram_cksum			= 0;
	
	if( an_ipv == 4) 
	{
		udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + (prependLen-20));	// remove udp-len from prependLen
		udp->dgram_cksum		= rte_ipv4_udptcp_cksum( ip_hdr, udp);
	} 
	else if( an_ipv == 6) 
	{
		udp->dgram_len 			= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen + 8);	// remove udp-len from prependLen
		udp->dgram_cksum		= rte_ipv6_udptcp_cksum( ip6_hdr, udp);
	}


	struct rte_gtp_hdr * gtp  	= (struct rte_gtp_hdr *) ((unsigned char *)udp + sizeof(struct rte_udp_hdr));
	gtp->gtp_hdr_info 			= 0x30;
	gtp->msg_type 				= 0xFF;
	if( an_ipv == 4) {
		//gtp->plen 					= rte_cpu_to_be_16( (orig_pkt_len-14) + (prependLen-(8+gtpHeaderLen)));
		gtp->plen 					= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen);
	} else {
		gtp->plen 					= rte_cpu_to_be_16( (orig_pkt_len-14) + gtpHeaderLen);
	}
	gtp->teid 					= rte_be_to_cpu_32( teid); 
			
	if( gtpHeaderLen == 12)
	{
		gtp->gtp_hdr_info 			= 0x32;
		*(uint16_t *)(((unsigned char *)gtp) + 8) = rte_be_to_cpu_16( gtp_seq_no);
	}
	
	return 0;
}


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
	uint8_t temp_data[2000];

	struct pcap_pkthdr header;
	gettimeofday( &header.ts, NULL);
	header.len = rte_pktmbuf_pkt_len( pkt);
	header.caplen = RTE_MIN( header.len, 2000);
	
	pcap_dump( (u_char *)dumper, &header, rte_pktmbuf_read( pkt, 0, header.caplen, temp_data));
}


void dpdk_pkt__close_pcap( pcap_dumper_t * dumper)
{
	if( dumper)
	{
		pcap_dump_close( dumper);
	}
}

void dpdk_pkt__test_case_write_arp_request( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( mempool);
	dpdk_pkt__encode_arp_request( m_buf, (char *)"\x00\x1a\x6b\x6c\x0c\xcc", 168430082, 168430081);
	dpdk_pkt__write_pcap( dumper, m_buf);
	rte_pktmbuf_free( m_buf);
}


void dpdk_pkt__test_case_write_arp_response( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	struct rte_mbuf * r_buf = rte_pktmbuf_alloc( mempool);
	dpdk_pkt__encode_arp_request( r_buf, (char *)"\x00\x1a\x6b\x6c\x0c\xcc", 168430082, 168430081);
	
	struct rte_mbuf * m_buf = rte_pktmbuf_alloc( mempool);
	dpdk_pkt__encode_arp_response( m_buf, r_buf, (char *)"\x00\x1d\x09\xf0\x92\xab", 168430081);
	
	dpdk_pkt__write_pcap( dumper, r_buf);
	dpdk_pkt__write_pcap( dumper, m_buf);
	
	rte_pktmbuf_free( r_buf);
	rte_pktmbuf_free( m_buf);
}

void dpdk_pkt__test_case_remove_outer_header_1( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	char * gtpTPduPacket = 	NULL;
	int gtplen = 0;

	gtpTPduPacket = 	"\x00\x0c\x29\xda\xd1\xde\x00\x0c\x29\xe3\xc6\x4d\x08\x00\x45\x00" \
							"\x00\x7c\x00\x00\x40\x00\x40\x11\x67\xbb\xc0\xa8\x28\xb3\xc0\xa8" \
							"\x28\xb2\x08\x68\x08\x68\x00\x68\xbf\x64\x32\xff\x00\x58\x00\x00" \
							"\x00\x01\x28\xdb\x00\x00\x45\x00\x00\x54\x00\x00\x40\x00\x40\x01" \
							"\x5e\xa5\xca\x0b\x28\x9e\xc0\xa8\x28\xb2\x08\x00\xbe\xe7\x00\x00" \
							"\x28\x7b\x04\x11\x20\x4b\xf4\x3d\x0d\x00\x08\x09\x0a\x0b\x0c\x0d" \
							"\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d" \
							"\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d" \
							"\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37";
	gtplen = 138;
	struct rte_mbuf * tpdu_buf = rte_pktmbuf_alloc( mempool);
	char * c_mptr = rte_pktmbuf_mtod( tpdu_buf, char *);
	memcpy( c_mptr, gtpTPduPacket, gtplen);
	
	tpdu_buf->pkt_len  = gtplen;
	tpdu_buf->data_len = gtplen;		
	
	dpdk_pkt__write_pcap( dumper, tpdu_buf);	//before
	dpdk_pkt__remove_outer_header_256( tpdu_buf, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02");
	dpdk_pkt__write_pcap( dumper, tpdu_buf);	//after
}

void dpdk_pkt__test_case_remove_outer_header_2( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	char * gtpTPduPacket = 	NULL;
	int gtplen = 0;

	gtpTPduPacket = 	"\x00\x25\xd3\xce\x5d\xf9\x00\x16\xb6\x8c\xcf\x7a\x08\x00\x45\x00" \
							"\x00\x7c\x00\x00\x00\x00\x3f\x11\x50\x06\xd4\x81\x41\x51\xd4\x81" \
							"\x41\x17\x08\x68\x08\x68\x00\x68\x00\x00\x32\xff\x00\x58\x37\x2f" \
							"\x00\x00\x00\x02\xff\x00\x45\x20\x00\x54\x62\x41\x00\x00\x30\x11" \
							"\x18\xce\x41\x37\x9e\x76\xc0\xa8\x6f\x14\x0d\xd8\xd3\x4c\x00\x40" \
							"\x37\xf0\x60\x00\x00\x00\x00\x00\x3b\x15\x20\x01\x00\x00\x41\x37" \
							"\x9e\x76\x30\xa3\x31\x49\x9f\x1a\x2f\x27\x20\x01\x00\x00\x41\x37" \
							"\x9e\x76\x2c\x5c\x2c\xb3\x52\xf0\x93\x46\x01\x04\x50\x52\xe8\x46" \
							"\x03\x08\x00\x00\xc0\xa8\x01\x02\xce\xb6";
	gtplen = 138;
	
	struct rte_mbuf * tpdu_buf2 = rte_pktmbuf_alloc( mempool);
	char * c_mptr2 = rte_pktmbuf_mtod( tpdu_buf2, char *);
	memcpy( c_mptr2, gtpTPduPacket, gtplen);
	
	tpdu_buf2->pkt_len  = gtplen;
	tpdu_buf2->data_len = gtplen;
	
	dpdk_pkt__write_pcap( dumper, tpdu_buf2);


	dpdk_pkt__remove_outer_header_256( tpdu_buf2, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02");
	dpdk_pkt__write_pcap( dumper, tpdu_buf2);
}


void dpdk_pkt__test_case_create_outer_header_ip4( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	char * gtpTPduPacket = 	NULL;
	int gtplen = 0;

	gtpTPduPacket = "\x00\x1d\x09\xf0\x92\xab\x00\x1a\x6b\x6c\x0c\xcc\x08\x00\x45\x00" \
	"\x00\x54\x00\x00\x40\x00\x40\x01\x12\x93\x0a\x0a\x0a\x02\x0a\x0a" \
	"\x0a\x01\x08\x00\x9d\x7b\x20\x93\x00\x01\x88\xf7\x9e\x50\x1d\xa5" \
	"\x0a\x00\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15" \
	"\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25" \
	"\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35" \
	"\x36\x37";
	gtplen = 98;

	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, gtpTPduPacket, gtplen);
	
	tpdu_buf4->pkt_len  = gtplen;
	tpdu_buf4->data_len = gtplen;
	
	dpdk_pkt__write_pcap( dumper, tpdu_buf4);
	
	dpdk_pkt__create_outer_header_256( tpdu_buf4, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02", 4, 168430082, 168430081, NULL, NULL, 123, 1, 100);
	// dpdk_pkt__create_outer_header_256( tpdu_buf4, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02", 6, 168430082, 168430081, 
		// "\x9e\x76\x2c\x5c\x2c\xb3\x52\xf0\x93\x46\x01\x04\x50\x52\xe8\x46", 
		// "\x37\xf0\x60\x00\x00\x00\x00\x00\x3b\x15\x20\x01\x00\x00\x41\x37", 2123, 1, 2002);

	dpdk_pkt__write_pcap( dumper, tpdu_buf4);
}


void dpdk_pkt__test_case_create_outer_header_ip6( pcap_dumper_t * dumper, struct rte_mempool * mempool)
{
	char * gtpTPduPacket = 	NULL;
	int gtplen = 0;

	gtpTPduPacket = "\x00\x1d\x09\xf0\x92\xab\x00\x1a\x6b\x6c\x0c\xcc\x08\x00\x45\x00" \
	"\x00\x54\x00\x00\x40\x00\x40\x01\x12\x93\x0a\x0a\x0a\x02\x0a\x0a" \
	"\x0a\x01\x08\x00\x9d\x7b\x20\x93\x00\x01\x88\xf7\x9e\x50\x1d\xa5" \
	"\x0a\x00\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15" \
	"\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25" \
	"\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35" \
	"\x36\x37";
	gtplen = 98;

	struct rte_mbuf * tpdu_buf4 = rte_pktmbuf_alloc( mempool);
	char * c_mptr4 = rte_pktmbuf_mtod( tpdu_buf4, char *);
	memcpy( c_mptr4, gtpTPduPacket, gtplen);
	
	tpdu_buf4->pkt_len  = gtplen;
	tpdu_buf4->data_len = gtplen;
	
	dpdk_pkt__write_pcap( dumper, tpdu_buf4);
	
	dpdk_pkt__create_outer_header_256( tpdu_buf4, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02", 6, 168430082, 168430081, 
		"\x9e\x76\x2c\x5c\x2c\xb3\x52\xf0\x93\x46\x01\x04\x50\x52\xe8\x46", 
		"\x37\xf0\x60\x00\x00\x00\x00\x00\x3b\x15\x20\x01\x00\x00\x41\x37", 2123, 1, 2002);

	dpdk_pkt__write_pcap( dumper, tpdu_buf4);
}

void dpdk_pkt__test_case_create_gtp_packet_ipv4( pcap_dumper_t * dumper, struct rte_mempool * mempool, uint8_t msgtype)
{
	struct rte_mbuf * tpdu_buf5 = rte_pktmbuf_alloc( mempool);
	
	dpdk_pkt__encode_gtp_echo_response_ipv4( tpdu_buf5, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02", 168430081, 168430082, 2152, 2152, msgtype, 76, 1, 1, 0, 9);
	dpdk_pkt__write_pcap( dumper, tpdu_buf5);	
}


void dpdk_pkt__test_case_create_gtp_packet_ipv6( pcap_dumper_t * dumper, struct rte_mempool * mempool, uint8_t msgtype)
{
	struct rte_mbuf * tpdu_buf5 = rte_pktmbuf_alloc( mempool);
	
	dpdk_pkt__encode_gtp_echo_response_ipv6( tpdu_buf5, "\x00\x00\x00\x01\x01\x01", "\x02\x02\x02\x02\x02\x02", "\x9e\x76\x2c\x5c\x2c\xb3\x52\xf0\x93\x46\x01\x04\x50\x52\xe8\x46", 
		"\x37\xf0\x60\x00\x00\x00\x00\x00\x3b\x15\x20\x01\x00\x00\x41\x37", 2152, 2152, msgtype, 76, 1, 1, 0, 9);

	dpdk_pkt__write_pcap( dumper, tpdu_buf5);	
}






