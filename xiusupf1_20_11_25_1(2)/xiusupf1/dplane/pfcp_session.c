#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

#include "app_stack.h"
#include "app_endpoint.h"
#include "pfcp_protocol_def.h"
#include "pfcp_types.h"
#include "pfcp_parser.h"
#include "pfcp_stack.h"
#include "app.h"


pfcp_session_t * pfcp_stack__find_session( uint64_t seid);
uint8_t * 	pfcp_stack__find_dpi_session( uint64_t seid);
uint64_t 	pfcp_stack__find_upf_seid( void * ptr);
uint64_t 	pfcp_stack__get_quota( void * sPtr, uint16_t rgid);
uint64_t 	pfcp_stack__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink);
int 		pfcp_stack__session_ohi( uint8_t * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no);


// uint8_t * pfcp_session__get_dpi_session( uint64_t seid)
// {
	// return (uint8_t *)pfcp_stack__find_dpi_session( seid);
// }


uint8_t * pfcp_session__get_pfcp_session( uint64_t seid)
{
	return (uint8_t *)pfcp_stack__find_session( seid);
}


uint64_t pfcp_session__get_seid( void * sPtr)
{
	return pfcp_stack__find_upf_seid( sPtr);
}

uint64_t pfcp_session__get_quota( void * sPtr, uint16_t rgid)
{
	return pfcp_stack__get_quota( sPtr, rgid);
}

uint64_t pfcp_session__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink)
{
	return pfcp_stack__record_usage( sPtr, rgid, uplink, downlink);
}

int pfcp_session__session_ohi( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no)
{
	return pfcp_stack__session_ohi( sPtr, teid, ipv, ranip, upfip, ranip6, upfip6, ranmac, upfmac, gtpHasSQN, gtp_seq_no);
}





