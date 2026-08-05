#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int cp__teid_allowed_v4( uint32_t ran_ip, uint32_t teid, uint8_t ** session, uint8_t * mac)
{
	return 0;
}

int cp__teid_allowed_v6( __uint128_t ran_ip, uint32_t teid, uint8_t ** session)
{
	return 0;
}

int cp__flow_allowed_sess_v4( uint8_t * session, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen, int direction)
{
	return 0;
}

int cp__flow_allowed_ueipv4( uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  uint8_t ** session, uint32_t * ran_teid, int direction)
{
	return 0;
}

int cp__get_ranv4info( uint8_t * session, uint8_t * ranip_v, uint32_t * ranip4, __uint128_t * ranip6, uint8_t ** mac)
{
	return 0;
}




