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
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "jansson.h"
#include "dpdk.h"


typedef struct pfcp_session
{
	struct pfcp_session * Next;
	
	uint64_t cp_seid;
	uint64_t up_seid;
	
	uint32_t teid;
	uint32_t ue_v4ip;
	
	struct
	{
		void * fHead;
		void * fCurrent;
		int fCount;
		pthread_mutex_t fLock;
	} dpi;

} pfcp_session_t;

typedef struct testapp
{
	pfcp_session_t * sHead;
	pfcp_session_t * sCurrent;
} testapp_t;

testapp_t * dApp = NULL;

void app__signalhandler( int signum)
{
	switch( signum)
	{	
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			break;
		default:
			break;
	}
	
	dpdk__initexit();

	int secs = 2;
	printf("\nCaught signal %d, coming out...in %d seconds.\n", signum, secs);
	sleep(secs);
	exit(0);
}


uint8_t * __dapp__get_dpi_session( uint64_t seid)
{
	pfcp_session_t * sess = dApp->sHead;
	
	while( sess)
	{
		if( sess->up_seid == seid)
			return (uint8_t *)&sess->dpi;
		
		sess = sess->Next;
	}
	
	return NULL;
}

uint8_t * __dapp__get_pfcp_session( uint64_t seid)
{
	pfcp_session_t * sess = dApp->sHead;
	
	while( sess)
	{
		if( sess->up_seid == seid)
			return (uint8_t *)sess;
		
		sess = sess->Next;
	}
	
	return NULL;
}


uint64_t __dapp__get_seid( void * sPtr)
{
	pfcp_session_t * sess = (pfcp_session_t *)sPtr;
	
	if( sess)
		return sess->up_seid;
	else
		return 0;
}

uint64_t __dapp__get_quota( void * sPtr, uint16_t rgid)
{
	pfcp_session_t * sess = (pfcp_session_t *)sPtr;
	
	if( sess)
		return 10000;
	else
		return 0;
}

uint64_t __dapp__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink)
{
	return 0;
}

int __dapp__session_ohi( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no)
{
	*teid = 1234;
	*ipv = 4;
	*ranip = 2130706433;
	*upfip = 2130706434;
	*ranip6 = NULL;
	*upfip6 = NULL;
	*ranmac = "\x00\x00\x00\x01\x02\x03";
	*upfmac = "\x00\x00\x00\x33\x03\x03";
	*gtpHasSQN = 1;
	*gtp_seq_no = 12345;
	
	return 1;
}

int main( int argc, char* argv[])
{
	printf("started app\n");
	
	signal( SIGINT, app__signalhandler);		// Ctrl + C
	signal( SIGQUIT, app__signalhandler);		// Ctrl + \ 			//
	signal( SIGTERM, app__signalhandler);		// shell command kill generates SIGTERM by default
	
	dpdk__init( argc, argv);
	
	
	dApp = (testapp_t *)malloc(sizeof(testapp_t));
	memset( dApp, 0, sizeof(testapp_t));
	
	dpdk_session__set_fp_seid( 			__dapp__get_seid);
	dpdk_session__set_fp_quota( 		__dapp__get_quota);
	dpdk_session__set_fp_usage( 		__dapp__record_usage);
	dpdk_session__set_fp_dpi_session( 	__dapp__get_dpi_session);
	dpdk_session__set_fp_pfcp_session( 	__dapp__get_pfcp_session);
	dpdk_session__set_fp_ohi( 			__dapp__session_ohi);
	
	/*
	pfcp_session_t * sess = NULL;
	
	int i = 1;
	for( i = 1; i < 100; i++)
	{
		sess = (pfcp_session_t *) malloc( sizeof(pfcp_session_t));
		memset( sess, 0, sizeof(pfcp_session_t));
		
		sess->teid 			= 100  + i;
		sess->cp_seid 		= 1000 + i;
		sess->up_seid 		= 2000 + i;
		sess->ue_v4ip		= 630565055 + i;
		
		sess->dpi.fHead		= NULL;
		sess->dpi.fCurrent	= NULL;
		sess->dpi.fCount	= 0;
		
		pthread_mutex_init( &sess->dpi.fLock, NULL);

		if(!dApp->sHead) 
		{
			dApp->sHead = dApp->sCurrent = sess;
		} 
		else 
		{
			dApp->sCurrent->Next = sess;
			dApp->sCurrent = sess;
		}
		
		dpdk_session__ipv4_add( sess->ue_v4ip, 	sess, 0);
		dpdk_session__teid_add( sess->teid, 	sess, 0);
	}
	sess = NULL;
	
	sleep(10);
	*/
	
	/*
	void * sPtr = NULL;
	dpdk_session__ipv4_find( 124, &sPtr);
	
	if( sPtr)
	{
		sess = (pfcp_session_t *)sPtr;
		uint8_t * pfcpsess = dpdk_session__get_pfcp_session( sess->up_seid);	
		
		printf("session found with ipv4=%d  ue_v4ip=%u teid=%u cp_seid=%lu up_seid=%lu %p-%p \n", 
			124, sess->ue_v4ip, sess->teid, sess->cp_seid, sess->up_seid, pfcpsess, sess);
			
	}
	else
	{
		printf("session NOT found with ipv4=%d\n", 124);
	}
	*/
	
	/*
	printf("injecting packet\n");
	char * data = 	NULL;
	int datalen = 	0;
	*/
	
	/*
	data 		= 	"\x00\x0c\x29\xc8\x4e\x72\x00\x0c\x29\x1f\x56\xef\x08\x00\x45\x00" \
					"\x00\x31\xd0\x41\x40\x00\x40\x11\xbe\xa1\xc0\xa8\x95\x62\xc0\xa8" \
					"\x95\x25\x22\x65\x22\x65\x00\x1d\xea\xaa\x21\x37\x00\x11\x00\x00" \
					"\x00\x00\x00\x00\x00\x0a\x00\x00\x02\x00\x00\x13\x00\x01\x01";
	datalen = 63;
	dpdk__inject_packet( 1, data, datalen);
	*/
	
	/*
	data		=	"\x00\x00\x00\x01\x02\x03\x00\x00\x00\x33\x03\x03\x08\x00\x45\x00" \
					"\x00\x59\x00\x00\x00\x00\x40\x11\x7c\x91\x7f\x00\x00\x02\x7f\x00" \
					"\x00\x01\x08\x68\x08\x68\x00\x45\x9c\x43\x32\xff\x00\x3d\x00\x00" \
					"\x04\xd2\x30\x39\x08\x00\x45\x00\x00\x31\xd0\x41\x40\x00\x40\x11" \
					"\xbe\xa1\xc0\xa8\x95\x25\xc0\xa8\x95\x62\x22\x65\x22\x65\x00\x1d" \
					"\xea\xaa\x21\x37\x00\x11\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00" \
					"\x02\x00\x00\x13\x00\x01\x01";
	datalen = 103;
	dpdk__inject_packet( 0, data, datalen);
	*/
	
	while(1)
	{
		sleep(1);
	}
}







