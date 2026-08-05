#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <resolv.h>
#include <ifaddrs.h>

#ifndef LIB_DPDK_ENG_H
#define LIB_DPDK_ENG_H

// #include <rte_memory.h>
// #include <rte_launch.h>
// #include <rte_eal.h>
// #include <rte_per_lcore.h>
// #include <rte_lcore.h>
// #include <rte_debug.h>
// #include <rte_ethdev.h>
// #include <rte_ether.h>
// #include <rte_gtp.h>
// #include <rte_distributor.h>

// #include <rte_table.h>
// #include <rte_hash.h>
// #include <rte_fbk_hash.h>
// #include <rte_jhash.h>
// #include <rte_hash_crc.h>
// #include <rte_arp.h>

// #include <pcap/pcap.h>
// #include <pcap/bpf.h>

//uint16_t dpe__process_received( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, int recv_worker_index);
//uint16_t dpe__process_recv_and_send( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, int recv_worker_index);
//uint16_t dpe__process_forward( dpdk_interface_t * interface, struct rte_mbuf ** pkts, uint16_t recv_pkts, struct rte_mbuf ** processed_pkts, struct rte_mbuf ** dropped_mbufs, uint16_t * dropped_cnt_out, struct rte_mbuf ** arp_mbufs, uint16_t * arp_cnt_out, int recv_worker_index);


void dpdk__initexit();
void dpe__init( char * configfile);
void dpe__init_engine( int argc, char **argv);
void dpe__test();

#endif


