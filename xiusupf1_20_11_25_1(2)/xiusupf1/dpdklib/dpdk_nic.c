#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#include "jansson.h"
#include "dpdk.h"

void dpdk_nic__print_mac( unsigned int portid, struct rte_ether_addr * addr)
{
    printf("Port=%u MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n", 
		portid, addr->addr_bytes[0], addr->addr_bytes[1], addr->addr_bytes[2], 
		addr->addr_bytes[3], addr->addr_bytes[4], addr->addr_bytes[5]);
}


int dpdk_nic__print_device_info( char * device_name)
{
	uint16_t port_id = 0;
	int ret = 0;
	ret = rte_eth_dev_get_port_by_name( device_name, &port_id);
	
	if (0 != ret) {
		printf( "rte_eth_dev_get_port_by_name(link = '%s') failed\n", device_name);
		return -1;
	} 
	else
	{
		printf( "dev_get_port_by_name ret=%d name=[%s] port_id=%u %s|%s|%d\n", 
			ret, device_name, port_id, __FILE__, __FUNCTION__, __LINE__);
	}
	
	ret = rte_eth_dev_is_valid_port( port_id);
	
	if (1 != ret)
	{
		printf( "rte_eth_dev_is_valid_port( %d) failed\n", port_id);
		return -1;
	}
	
	struct rte_eth_dev_info dev_info;
	memset( &dev_info, 0, sizeof( struct rte_eth_dev_info));
	
	ret = rte_eth_dev_info_get( port_id, &dev_info);

	if( ret == 0)
	{
		printf( "driver_name=%s\n", dev_info.driver_name);
		printf( "if_index=%d  min_mtu=%u  max_mtu=%u  \n", dev_info.if_index, dev_info.min_mtu, dev_info.max_mtu);
		printf( "min_rx_bufsize=%u max_rx_pktlen=%u\n", dev_info.min_rx_bufsize, dev_info.max_rx_pktlen);
		printf( "max_rx_queues=%u max_tx_queues=%u\n", dev_info.max_rx_queues, dev_info.max_tx_queues);
		printf( "nb_rx_queues=%u nb_tx_queues=%u\n", dev_info.nb_rx_queues, dev_info.nb_tx_queues);
		printf( "max_rx_mempools=%u speed_capa=%u\n", 0, dev_info.speed_capa);
		printf( "rx_desc_lim nb_max=%u nb_min=%u\n", dev_info.rx_desc_lim.nb_max, dev_info.rx_desc_lim.nb_min);
		printf( "tx_desc_lim nb_max=%u nb_min=%u\n", dev_info.tx_desc_lim.nb_max, dev_info.tx_desc_lim.nb_min);
	}

	struct rte_ether_addr addr;
	memset( &addr, 0, sizeof(struct rte_ether_addr));
	ret = rte_eth_macaddr_get( port_id, &addr);

	if (0 != ret)
	{
		printf("rte_eth_macaddr_get( port_id, &addr); failed.\n");
		
		//set mac address
		//ret = rte_eth_dev_default_mac_addr_set(link_id, (struct rte_ether_addr *)info_ptr->mac_addr);
	}
	else
	{
		//rte_memcpy( dev->MAC, addr.addr_bytes, 6);
		//sprintf( dev->SMAC, "%02X:%02X:%02X:%02X:%02X:%02X", addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2], addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);
		//dev->SMAC[17] = 0;
		dpdk_nic__print_mac( port_id, &addr);
	}

	return 0;
}

#define DEFAULT_MTU_SIZE        	1500
#define DEFAULT_MIN_MTU_SIZE   	 	68
#define DEFAULT_MAX_MTU_SIZE    	65535


int dpdk_nic__configure_and_start( dpdkif_nic_t * nic)
{
	// dpdk_nic__print_device_info( nic->ifid);
	// return 0;

	char ring_name[30];
	memset( ring_name, 0, sizeof(ring_name));
	
	sprintf( ring_name, "mempool-%d-%d", nic->type, nic->id);
	nic->mempool = rte_pktmbuf_pool_create( ring_name, nic->iMemPoolSize, nic->iCacheSize, sizeof(dpdk_md_t), RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	
	if(!nic->mempool)
	{
		dpdk__log( 5, "rte_pktmbuf_pool_create(%s|%s|size=%d|cache=%d), mempool creation failed  %s|%s|%d", nic->ifid, ring_name, nic->iMemPoolSize, nic->iCacheSize, __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	else
	{
		dpdk__log( 5, "rte_pktmbuf_pool_create(%s|%s|size=%d|cache=%d), mempool created succesfully  %s|%s|%d", nic->ifid, ring_name, nic->iMemPoolSize, nic->iCacheSize, __FILE__, __FUNCTION__, __LINE__);
	}

	
	int ret = 0;
	ret = rte_eth_dev_get_port_by_name( nic->ifid, &nic->port_id);

	if (0 != ret)
    {
		dpdk__log( 5, "rte_eth_dev_get_port_by_name(%s) failed   %s|%s|%d", nic->ifid, __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	else
	{
		dpdk__log( 5, "rte_eth_dev_get_port_by_name ret-value=%d name=[%s] port_id=%u succesful   %s|%s|%d", ret, nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}


	ret = rte_eth_dev_is_valid_port( nic->port_id);
	
	if (1 != ret)
	{
		dpdk__log( 5, "rte_eth_dev_is_valid_port(%s|%d) failed  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
		return -2;
	}
	else
	{
		dpdk__log( 5, "rte_eth_dev_is_valid_port(%s|%d) is valid port   %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}



	struct rte_eth_dev_info dev_info;
	struct rte_eth_conf port_conf;
	struct rte_eth_txconf tx_conf;
	struct rte_ether_addr addr;
	
	memset( &dev_info, 	0, sizeof(struct rte_eth_dev_info));
	memset( &port_conf, 0, sizeof(struct rte_eth_conf));
	memset( &tx_conf, 	0, sizeof(struct rte_eth_txconf));
	memset( &addr, 		0, sizeof(struct rte_ether_addr));
	
	
	ret = rte_eth_macaddr_get( nic->port_id, &addr);

	if (0 == ret)
	{
		dpdk__log( 5, "%s", nic->ifid);
		dpdk_nic__print_mac( nic->port_id, &addr);
	}	
	
	
	

	
	
	uint16_t mtu = 0;
	ret = rte_eth_dev_get_mtu( nic->port_id, &mtu);
	
	if (0 != ret)
	{
		nic->mtu = DEFAULT_MTU_SIZE;
		dpdk__log( 5, "rte_eth_dev_get_mtu(%s|%d) failed   %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}
	else
	{
		dpdk__log( 5, "rte_eth_dev_get_mtu(%s|%d,mtu=%d) succesful   %s|%s|%d", nic->ifid, nic->port_id, mtu, __FILE__, __FUNCTION__, __LINE__);
	
		if( mtu > DEFAULT_MTU_SIZE)
		{
			ret = rte_eth_dev_set_mtu( nic->port_id, (uint16_t)DEFAULT_MTU_SIZE);
			
			if( ret != 0)
			{
				dpdk__log( 5, "updating mtu failed for if=%s port=%d  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
				exit(0);
			}
			else
			{
				dpdk__log( 5, "rte_eth_dev_get_mtu(%s|%d) failed   %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}
	
	nic->mtu = mtu;
	
	
	ret = rte_eth_dev_info_get( nic->port_id, &dev_info);
	
	if (0 != ret)
	{
		nic->min_mtu = DEFAULT_MIN_MTU_SIZE;
		nic->max_mtu = DEFAULT_MAX_MTU_SIZE;
		dpdk__log( 5, "rte_eth_dev_info_get(%s|%d) failed   %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}
	else
	{
		nic->min_mtu = dev_info.min_mtu;
		nic->max_mtu = dev_info.max_mtu;
		dpdk__log( 5, "rte_eth_dev_info_get(%s|%d) succesful   %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}
	

	
	memset( &port_conf, 0, sizeof(struct rte_eth_conf));
	port_conf.rxmode.mq_mode 		= ETH_MQ_RX_RSS;
	//port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
	port_conf.rxmode.mq_mode 		= ETH_MQ_TX_NONE;
	port_conf.rx_adv_conf.rss_conf.rss_hf 		= (ETH_RSS_L3_DST_ONLY | ETH_RSS_GTPU);

	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
	{
		port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
	}

	unsigned int rss_hf = port_conf.rx_adv_conf.rss_conf.rss_hf;
	port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;

	// if (port_conf.rx_adv_conf.rss_conf.rss_hf != rss_hf)
	// {
		// pfm_log_rte_err(PFM_LOG_WARNING,
			// "LinkId=%d modified RSS hash function based on "
			// "hardware support, "
			// "requested:%#"PRIx64" configured:%#"PRIx64"",
			// link_id,
			// rss_hf,
			// port_conf.rx_adv_conf.rss_conf.rss_hf);
	// }
	
	ret = rte_eth_dev_configure( nic->port_id, nic->rx_q_count, nic->tx_q_count, &port_conf);
	if (ret != 0)
	{
		dpdk__log( 5, "rte_eth_dev_configure(%s(%d), rx=%u, tx=%u ) failed   %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
		return -3;
	} else {
		dpdk__log( 5, "rte_eth_dev_configure(%s(%d), rx=%u, tx=%u ) success  %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
	}

	uint16_t rx_sz = nic->iRxBurstSize;
	uint16_t tx_sz = nic->iTxBurstSize;
	ret = rte_eth_dev_adjust_nb_rx_tx_desc( nic->port_id, &rx_sz, &tx_sz);

	if (ret != 0)
	{
		dpdk__log( 5, "rte_eth_dev_adjust_nb_rx_tx_desc(%s(%d), rx=%u, tx=%u ) failed     %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
		return -4;
	}
	else
	{
		dpdk__log( 5, "rte_eth_dev_adjust_nb_rx_tx_desc(%s(%d), rx=%u, tx=%u ) succesful  %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
	}
	
	int i = 0;
	
	for( i = 0; i < nic->rx_q_count; i++ ) 
	{
		ret = rte_eth_rx_queue_setup( nic->port_id, i, rx_sz, rte_eth_dev_socket_id( nic->port_id), NULL, nic->mempool);
	
		if (0 != ret)
		{
			dpdk__log( 5, "rte_eth_rx_queue_setup failed ifid=%s port_id=%u rx_q_count=%u tx_q_count=%u  %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
			return -5;
		}
		else
		{
			dpdk__log( 5, "rte_eth_rx_queue_setup   ifid=%s port_id=%u  qid=%d   %s|%s|%d", nic->ifid, nic->port_id, i, __FILE__, __FUNCTION__, __LINE__);
		}
	}

	tx_conf = dev_info.default_txconf;
	tx_conf.offloads = port_conf.txmode.offloads;	
	
	i = 0;
	for( i = 0; i < nic->tx_q_count; i++ ) 
	{
		ret = rte_eth_tx_queue_setup( nic->port_id, i, tx_sz, rte_eth_dev_socket_id( nic->port_id), &tx_conf);
		
		if (0 != ret)
		{
			dpdk__log( 5, "rte_eth_tx_queue_setup failed ifid=%s port_id=%u rx_q_count=%u tx_q_count=%u  %s|%s|%d", nic->ifid, nic->port_id, nic->rx_q_count, nic->tx_q_count, __FILE__, __FUNCTION__, __LINE__);
			return -6;
		}
		else
		{
			dpdk__log( 5, "rte_eth_tx_queue_setup   ifid=%s port_id=%u  qid=%d  %s|%s|%d", nic->ifid, nic->port_id, i, __FILE__, __FUNCTION__, __LINE__);
		}		
	}

	ret = rte_eth_dev_start( nic->port_id);
	if (0 != ret)
	{
		dpdk__log( 5, "rte_eth_dev_start failed for  ifid=%s  port_id=%u  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
		nic->started = 0;
		return -7;
	}
	else
	{
		dpdk__log( 5, "rte_eth_dev_start succesfully for  ifid=%s  port_id=%u  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
		nic->started = 1;
	}

	ret = rte_eth_promiscuous_enable( nic->port_id);
	if (0 != ret)
	{
		dpdk__log( 5, "rte_eth_promiscuous_enable failed for  ifid=%s  port_id=%u  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
		return -8;
	}
	else
	{
		dpdk__log( 5, "rte_eth_promiscuous_enable succesful for  ifid=%s  port_id=%u  %s|%s|%d", nic->ifid, nic->port_id, __FILE__, __FUNCTION__, __LINE__);
	}
	
	return 0;
}



int dpdk_nic__stop( dpdkif_nic_t * nic)
{
	if( nic->started == 1)
	{
		int ret = rte_eth_dev_stop( nic->port_id);
		
	}
	
	return 0;
}

int dpdk_nic__configure_device( dpdkif_t * ifc)
{
	// int ret = 0;
	// ret = rte_eth_dev_get_port_by_name( ifc->nic->ifid, &ifc->nic->port_id);
	
	// if (0 != ret)
    // {
		// dpdk__log( 5, "rte_eth_dev_get_port_by_name(link = '%s') failed   %s|%s|%d", ifc->ifid, __FILE__, __FUNCTION__, __LINE__);
		// return -1;
	// }
	// else
	// {
		// dpdk__log( 5, "dev_get_port_by_name ret-value=%d name=[%s] port_id=%u   %s|%s|%d", ret, ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	// }
	
	
	
	// ret = rte_eth_dev_is_valid_port( ifc->port_id);
	
	// if (1 != ret)
	// {
		// dpdk__log( 5, "rte_eth_dev_is_valid_port(link = '%d') failed  %s|%s|%d", ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
		// return -2;
	// }
	// else
	// {
		// dpdk__log( 5, "rte_eth_dev_is_valid_port(link = '%d') is valid port   %s|%s|%d", ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	// }
	
	// ret = rte_eth_dev_get_mtu( ifc->port_id, &ifc->mtu);
	
	// if (0 != ret)
	// {
		// dpdk__log( 5, "mtu retrieving failed for port=%u   %s|%s|%d", ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
		// ifc->mtu = RTE_ETHER_MAX_LEN;
	// }

	// dpdk__log( 5, "ifid=%s mtu=%u for port=%u   %s|%s|%d", ifc->ifid, ifc->mtu, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	
	// //TODO: set max mtu?
	
	// struct rte_eth_dev_info dev_info;
	// memset( &dev_info, 0, sizeof(struct rte_eth_dev_info));
	
	// ret = rte_eth_dev_info_get( ifc->port_id, &dev_info);
	// if (0 != ret)
	// {
		// printf("rte_eth_dev_info_get( port_id, &dev_info); failed.\n");
	// }
	// else
	// {
		// printf( "min_mtu=%d max_mtu=%d %s|%s|%d\n", dev_info.min_mtu, dev_info.max_mtu, __FILE__, __FUNCTION__, __LINE__);
	// }


	// // struct rte_ether_addr addr;
	// // memset( &addr, 0, sizeof(struct rte_ether_addr));
	// // ret = rte_eth_macaddr_get( dev->port, &addr);

	// // if (0 != ret)
	// // {
		// // printf("rte_eth_macaddr_get( port_id, &addr); failed.\n");
		
		// // //set mac address
		// // //ret = rte_eth_dev_default_mac_addr_set(link_id, (struct rte_ether_addr *)info_ptr->mac_addr);
	// // }
	// // else
	// // {
		// // rte_memcpy( dev->MAC, addr.addr_bytes, 6);
		// // sprintf( dev->SMAC, "%02X:%02X:%02X:%02X:%02X:%02X", addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2], addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);
		// // dev->SMAC[17] = 0;
		// // __dp_print_mac( dev->port, &addr);
	// // }


	// struct rte_eth_conf port_conf;
	// memset( &port_conf, 0, sizeof(struct rte_eth_conf));
	
	// port_conf.rxmode.mq_mode 				= ETH_MQ_RX_RSS;
	// //port_conf.rxmode.max_rx_pkt_len 		= RTE_ETHER_MAX_LEN;
	// port_conf.rxmode.mq_mode 				= ETH_MQ_TX_NONE;
	// port_conf.rx_adv_conf.rss_conf.rss_hf 	= (ETH_RSS_L3_DST_ONLY | ETH_RSS_GTPU);
	
	// if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
	// {
		// port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		// port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
	// }
	
	// unsigned int rss_hf = port_conf.rx_adv_conf.rss_conf.rss_hf;
	// port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;

	// printf("rss_hf=%u %lu %s|%s|%d\n", rss_hf, dev_info.flow_type_rss_offloads, __FILE__, __FUNCTION__, __LINE__);

	// if (port_conf.rx_adv_conf.rss_conf.rss_hf != rss_hf)
	// {
		// printf( "port=%d modified RSS hash function based on hardware support, requested:%u configured:%lu\n",
			// ifc->port_id, rss_hf, port_conf.rx_adv_conf.rss_conf.rss_hf);
	// }
	// else
	// {
		// printf("port=%s rss configured %s|%s|%d\n", ifc->ifid, __FILE__, __FUNCTION__, __LINE__);
	// }
	


	// ret = rte_eth_dev_configure( ifc->port_id, ifc->iRxBurstSize, ifc->iTxBurstSize, &port_conf);

	// if (ret != 0)
	// {
		// printf( "rte_eth_dev_configure(link='%s(%d)',1,1) failed\n", ifc->ifid, ifc->port_id);
		// rte_exit( EXIT_FAILURE, "rte_eth_dev_configure failed\n");
	// }
	// else
	// {
		// printf("rte_eth_dev_configure succesful %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
	// }
	

	// ifc->nb_rx_desc = ifc->iRxBurstSize;
	// ifc->nb_tx_desc = ifc->iTxBurstSize;

	// ret = rte_eth_dev_adjust_nb_rx_tx_desc( ifc->port_id, &ifc->nb_rx_desc, &ifc->nb_tx_desc);

	// if (ret != 0)
	// {
		// printf( "rte_eth_dev_adjust_nb_rx_tx_desc('LinkId='%s(%d)',rx_sz=%d, tx_sz=%d) failed\n", ifc->ifid, ifc->port_id, ifc->nb_rx_desc, ifc->nb_tx_desc);
		// rte_exit( EXIT_FAILURE, "rte_eth_dev_adjust_nb_rx_tx_desc failed\n");
	// }
	// else
	// {
		// printf("rte_eth_dev_adjust_nb_rx_tx_desc configured rx_sz=%u tx_sz=%u  %s|%s|%d\n", ifc->nb_rx_desc, ifc->nb_tx_desc, __FILE__, __FUNCTION__, __LINE__);
	// }
	


	// ret = rte_eth_rx_queue_setup( ifc->port_id, ifc->queueid, ifc->nb_rx_desc, rte_eth_dev_socket_id( ifc->port_id), NULL, ifc->mempool);

	// if (0 != ret)
	// {
		// printf( "rte_eth_rx_queue_setup(link='%s(%d)',0,rx_sz=%d,sockId=%d,NULL,pool=%p) failed\n", ifc->ifid, ifc->port_id, ifc->nb_rx_desc, rte_eth_dev_socket_id( ifc->port_id), ifc->mempool);
		// rte_exit( EXIT_FAILURE, "rte_eth_rx_queue_setup failed\n");
	// }
	// else
	// {
		// printf("rx queue setup completed for portname=%s port=%u %s|%s|%d\n", ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	// }	


	// struct rte_eth_txconf tx_conf;
	// memset( &tx_conf, 0, sizeof(struct rte_eth_txconf));
	
	// tx_conf = dev_info.default_txconf;
	// tx_conf.offloads = port_conf.txmode.offloads;
	// ret = rte_eth_tx_queue_setup( ifc->port_id, ifc->queueid, ifc->nb_tx_desc, rte_eth_dev_socket_id( ifc->port_id), &tx_conf);

	// if (ret < 0)
	// {
		// printf( "rte_eth_tx_queue_setup(link='%s(%d)',0,tx_sz=%d,sockId=%d,tx_conf=%p\n", ifc->ifid, ifc->port_id, ifc->nb_tx_desc, rte_eth_dev_socket_id( ifc->port_id), &tx_conf);
		// rte_exit( EXIT_FAILURE, "rte_eth_tx_queue_setup failed\n");
	// }
	// else
	// {
		// printf("tx queue setup completed for portname=%s port=%u %s|%s|%d\n", ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	// }

	return 1;
}


int dpdk_nic__start_device( dpdkif_t * ifc)
{
	// int ret = rte_eth_dev_start( ifc->port_id);
	
	// if (0 != ret)
	// {
		// printf( "rte_eth_dev_start(link='%s(%d)') failed\n", ifc->ifid, ifc->port_id);
		// rte_exit( EXIT_FAILURE, "rte_eth_dev_start failed\n");
	// }
	// else
	// {
		// printf("dev_start started portname=%s port=%u  %s|%s|%d\n", ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
	// }
	
	// if( ifc->enable_promiscuous_mode == 1)
	// {
		// ret = rte_eth_promiscuous_enable( ifc->port_id);
		
		// if (0 != ret)
		// { 
			// printf( "rte_eth_promiscuous_enable('link=%s(%d)') failed %s|%s|%d\n", ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
		// }
		// else
		// {
			// printf("enabling promiscuous mode succesful portname=%s port=%u %s|%s|%d\n", ifc->ifid, ifc->port_id, __FILE__, __FUNCTION__, __LINE__);
		// }
	// }
	
	// ifc->started = 1;
}

int dpdk_nic__stop_device( dpdkif_t * ifc)
{
	int ret = rte_eth_dev_stop( ifc->nic->port_id);
	
	if (0 != ret)
	{
		printf( "rte_eth_dev_stop(link='%s(%d)') failed\n", ifc->nic->ifid, ifc->nic->port_id);
	}
	else
	{
		printf( "rte_eth_dev_stop(link='%s(%d)') success\n", ifc->nic->ifid, ifc->nic->port_id);
	}
	
	ifc->nic->started = 2;
}



void dpdk_nic__link_status( dpdkif_t * ifc)
{
	struct rte_eth_link link;
	int ret;
	char link_status_text[ RTE_ETH_LINK_MAX_STR_LEN];
	
	ret = rte_eth_link_get_nowait( ifc->nic->port_id, &link);
	
	if (ret < 0) 
	{
		printf("Failed link get on port %d: %s\n", ifc->nic->port_id, rte_strerror(-ret));
	}
	
	//printf("Port %d %s\n\n", ifc->port_id, link_status_text);
}




















