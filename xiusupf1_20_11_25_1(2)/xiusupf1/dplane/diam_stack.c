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
#include "diam_stack.h"
#include "jansson.h"

jet_diam_stack_t * jet_diam_stack = NULL;


int jet_diam__get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

char * jet_diam__get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}


jet_diam_dict_message_t * jet_diam_stack__find_dict_message( uint32_t AppId, uint32_t CmdCode)
{
	jet_diam_dict_message_t * msg = jet_diam_stack->DDHead;
	
	while( msg)
	{
		if( msg->ApplicationId == AppId && msg->CommandCode == CmdCode)
		{
			return msg;
		}
		
		msg = msg->Next;
	}
	return NULL;
}


int jet_diam_stack__peer_ready( jet_diam_peer_client_t * client)
{
	if(client)
	{
		if( client->Connected == 1 && client->CEASuccess == 1)
		{
			return 1;
		}
	}	
	return 0;
}


int jet_diam_stack__peer_connect( jet_diam_peer_client_t * client)
{
	client->Connected = 0;
	client->CEASuccess = 0;

	int sockfd;

	struct sockaddr_in their_paddr;

	their_paddr.sin_family = PF_INET;
	their_paddr.sin_addr.s_addr = inet_addr( client->IP );
	their_paddr.sin_port = htons( client->Port);

	memset(&(their_paddr.sin_zero), '\0', 8);

	if ((sockfd = socket( PF_INET, SOCK_STREAM, 0)) == -1)
	{
		app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Socket creation failed for Peer[%s:%d]", client->IP, client->Port);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&their_paddr,sizeof(struct sockaddr)) == -1)
	{
		close( sockfd);
		app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Connect failed for Peer[%s:%d]", client->IP, client->Port);
		return -1;
	}

	app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Connected Successfully to Peer[%s:%d] fd[%d]", client->IP, client->Port, sockfd);
	
	client->fd = sockfd;
	client->Connected = 1;
	
	return 0;
}


void jet_diam_stack__diam_message_init_request( jet_diam_message_t * dMsg, uint32_t AppId, uint32_t CmdCode, uint8_t flags)
{
	memset( dMsg, 0, sizeof(jet_diam_message_t));
	
	dMsg->AppId 	= AppId;
	dMsg->CmdCode	= CmdCode;
	dMsg->Flags		= flags;
	
	pthread_mutex_init( &dMsg->AVPLock, NULL);
}

jet_diam_message_t * jet_diam_stack__init_request( uint32_t AppId, uint32_t CmdCode, uint8_t flags)
{
	jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
	if( dMsg) {
		jet_diam_stack__diam_message_init_request( dMsg, AppId, CmdCode, flags);
	}
	return dMsg;
}

jet_diam_message_t * jet_diam_stack__init_request2( uint32_t AppId, uint32_t CmdCode, uint8_t flags, uint32_t hbhId, uint32_t e2eId)
{
	jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
	if( dMsg) 
	{
		jet_diam_stack__diam_message_init_request( dMsg, AppId, CmdCode, flags);
		dMsg->HBHId = hbhId;
		dMsg->E2EId = e2eId;
	}
	return dMsg;
}


void jet_diam_stack__set_HBHId( jet_diam_message_t * dRequest, uint32_t hbhId)
{
	dRequest->HBHId = hbhId;
}

void jet_diam_stack__set_E2EId( jet_diam_message_t * dRequest, uint32_t e2eId)
{
	dRequest->E2EId = e2eId;
}


void jet_diam_stack__diam_message_init_answer( jet_diam_message_t * dRequest, jet_diam_message_t * dMsg, uint8_t flags)
{
	dMsg->AppId 	= dRequest->AppId;
	dMsg->CmdCode	= dRequest->CmdCode;
	dMsg->HBHId		= dRequest->HBHId;
	dMsg->E2EId		= dRequest->E2EId;
	dMsg->Flags		= flags;
	pthread_mutex_init( &dMsg->AVPLock, NULL);	
}

jet_diam_message_t * jet_diam_stack__diam_message_init_answer2( jet_diam_message_t * dRequest)
{
	jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
	jet_diam_stack__diam_message_init_answer( dRequest, dMsg, 0x40);
	return dMsg;
}




void jet_diam_stack__get_diam_message_attr( jet_diam_message_t * dmMsg, uint32_t * appId, uint32_t * CmdCode, uint8_t * isRequest)
{
	*appId 			= dmMsg->AppId;
	*CmdCode		= dmMsg->CmdCode;
	jet_diam_stack__getbit( isRequest, (char*)&dmMsg->Flags, 7);
}

uint32_t jet_diam_stack__get_E2EId( jet_diam_message_t * dmMsg)
{
	return dmMsg->E2EId;
}

uint32_t jet_diam_stack__get_HBHId( jet_diam_message_t * dmMsg)
{
	return dmMsg->HBHId;
}

void jet_diam_stack__diam_message_add_avp( jet_diam_message_t * dMsg, jet_diam_avp_t * avp)
{
	pthread_mutex_lock( &dMsg->AVPLock);
	
	if(!dMsg->avpHead) {
		dMsg->avpHead = dMsg->avpCurrent = avp;
	} else {
		dMsg->avpCurrent->Next = avp;
		dMsg->avpCurrent = avp;
	}
	
	pthread_mutex_unlock( &dMsg->AVPLock);
	dMsg->avpCount++;
}

void jet_diam_stack__diam_message_add_avp_to_avp( jet_diam_avp_t * pavp, jet_diam_avp_t * avp)
{
	pthread_mutex_lock( &pavp->AVPLock);
	
	if(!pavp->avpHead) 
	{
		pavp->avpHead = pavp->avpCurrent = avp;
	} 
	else 
	{
		pavp->avpCurrent->Next = avp;
		pavp->avpCurrent = avp;
	}
	
	pthread_mutex_unlock( &pavp->AVPLock);
	pavp->avpCount++;
}



void jet_diam_stack__diam_message_add_str_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, char * data, int datalen)
{
	jet_diam_avp_t * dAvp = (jet_diam_avp_t *)app_region__allocate_fd( jet_diam_stack->diam_avp_dpool);
	
	if( dAvp)
	{
		memset( dAvp, 0, sizeof(jet_diam_avp_t));
		
		dAvp->AvpCode = AVPCode;
		dAvp->PayLoadLength = datalen;
		dAvp->VendorId = VendorId;
		dAvp->DataType = JET_DIAM_STACK__OCTET_STRING;
		dAvp->Flags = Flags;
		dAvp->data = NULL;
		
		if( datalen > 0)
		{
			dAvp->data = (char *)app_region__allocate_fr( jet_diam_stack->dm_region, datalen);
		
			if( dAvp->data)
			{
				memcpy( dAvp->data, data, datalen);
			}
		}
		
		jet_diam_stack__diam_message_add_avp( dMsg, dAvp);
	}
}


void jet_diam_stack__diam_message_add_ipv4_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint32_t ipadd)
{
	char ipv4[7];
	ipv4[0] = 0x00;
	ipv4[1] = 0x01;
	*((uint32_t*)&ipv4[2]) = ipadd;
	
	jet_diam_stack__diam_message_add_str_avp( dMsg, AVPCode, Flags, VendorId, ipv4, 6);
}

void jet_diam_stack__diam_message_add_ipv6_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint8_t * ipadd)
{
	char ipv4[20];
	ipv4[0] = 0x00;
	ipv4[1] = 0x02;
	memcpy( &ipv4[2], ipadd, 16);
	
	jet_diam_stack__diam_message_add_str_avp( dMsg, AVPCode, Flags, VendorId, ipv4, 18);
}

void jet_diam_stack__diam_message_add_ipv4_avp2( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint32_t ipadd)
{
	char ipv4[7];
	ipv4[0] = 0x00;
	ipv4[1] = 0x01;
	*((uint32_t*)&ipv4[2]) = ipadd;
	
	jet_diam_stack__diam_message_add_str_avp_to_avp( pavp, AVPCode, Flags, VendorId, ipv4, 6);
}

void jet_diam_stack__diam_message_add_ipv6_avp2( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint8_t * ipadd)
{
	char ipv4[20];
	ipv4[0] = 0x00;
	ipv4[1] = 0x02;
	memcpy( &ipv4[2], ipadd, 16);
	
	jet_diam_stack__diam_message_add_str_avp_to_avp( pavp, AVPCode, Flags, VendorId, ipv4, 18);
}


void jet_diam_stack__diam_message_add_number_avp( jet_diam_message_t * dMsg, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint64_t data, uint32_t datalen, uint32_t dt)
{
	jet_diam_avp_t * dAvp = (jet_diam_avp_t *)app_region__allocate_fd( jet_diam_stack->diam_avp_dpool);

	if( dAvp)
	{
		memset( dAvp, 0, sizeof(jet_diam_avp_t));
		
		dAvp->AvpCode = AVPCode;
		dAvp->PayLoadLength = datalen;
		dAvp->VendorId = VendorId;
		dAvp->DataType = dt;
		dAvp->Flags = Flags;
		dAvp->data = (char *)app_region__allocate_fr( jet_diam_stack->dm_region, datalen);
	
		if( dAvp->data)
		{
			if( datalen == 4)
			{
				app_ep__encode__u32toc( dAvp->data, data);
			}
			else
			{
				app_ep__encode__u64toc( dAvp->data, data);
			}

		}
	
		jet_diam_stack__diam_message_add_avp( dMsg, dAvp);
	}	
}



void jet_diam_stack__diam_message_add_str_avp_to_avp( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, char * data, int datalen)
{
	jet_diam_avp_t * dAvp = (jet_diam_avp_t *)app_region__allocate_fd( jet_diam_stack->diam_avp_dpool);
	
	if( dAvp)
	{
		memset( dAvp, 0, sizeof(jet_diam_avp_t));
		
		dAvp->AvpCode = AVPCode;
		dAvp->PayLoadLength = datalen;
		dAvp->VendorId = VendorId;
		dAvp->DataType = JET_DIAM_STACK__OCTET_STRING;
		dAvp->Flags = Flags;
		dAvp->data = NULL;

		if( datalen > 0)
		{
			dAvp->data = (char *)app_region__allocate_fr( jet_diam_stack->dm_region, datalen);
		
			if( dAvp->data)
			{
				memcpy( dAvp->data, data, datalen);
			}
		}
		
		jet_diam_stack__diam_message_add_avp_to_avp( pavp, dAvp);
	}
}

void jet_diam_stack__diam_message_add_number_avp_to_avp( jet_diam_avp_t * pavp, uint32_t AVPCode, uint8_t Flags, uint32_t VendorId, uint64_t data, uint32_t datalen, uint32_t dt)
{
	jet_diam_avp_t * dAvp = (jet_diam_avp_t *)app_region__allocate_fd( jet_diam_stack->diam_avp_dpool);

	if( dAvp)
	{
		memset( dAvp, 0, sizeof(jet_diam_avp_t));
		
		dAvp->AvpCode = AVPCode;
		dAvp->PayLoadLength = datalen;
		dAvp->VendorId = VendorId;
		dAvp->DataType = dt;
		dAvp->Flags = Flags;

		dAvp->data = (char *)app_region__allocate_fr( jet_diam_stack->dm_region, datalen);
	
		if( dAvp->data)
		{
			if( datalen == 4)
			{
				app_ep__encode__u32toc( dAvp->data, data);
			}
			else
			{
				app_ep__encode__u64toc( dAvp->data, data);
			}
		}

	
		jet_diam_stack__diam_message_add_avp_to_avp( pavp, dAvp);
	}	
}


jet_diam_avp_t * jet_diam_stack__create_grouped_avp(  uint32_t AVPCode, uint8_t Flags, uint32_t VendorId)
{
	jet_diam_avp_t * dAvp = (jet_diam_avp_t *)app_region__allocate_fd( jet_diam_stack->diam_avp_dpool);
	
	if( dAvp)
	{
		memset( dAvp, 0, sizeof(jet_diam_avp_t));
		
		dAvp->AvpCode 		= AVPCode;
		dAvp->VendorId 		= VendorId;
		dAvp->DataType 		= JET_DIAM_STACK__GROUPED;
		dAvp->Flags 		= Flags;
		dAvp->avpHead		= NULL;
		dAvp->avpCurrent	= NULL;
		dAvp->avpCount 		= 0;
		pthread_mutex_init( &dAvp->AVPLock, NULL);
		
		return dAvp;
	}
	return NULL;
}


void jet_diam_stack__diam_message_add_grouped_avp( jet_diam_message_t * dMsg, jet_diam_avp_t * avp)
{
	jet_diam_stack__diam_message_add_avp( dMsg, avp);
}




void jet_diam_stack__calc_avp_len( jet_diam_avp_t * dmAvp)
{
	int iLength = 0;
	int iHeader = ( dmAvp->VendorId > 0) ? 12 : 8;
	int iDataLength = 0;
	int iPadded = 0;
	int iAvpLength;
	
	//printf( "PayLoadLength=%u\n", dmAvp->PayLoadLength);
	iDataLength = dmAvp->PayLoadLength;
	iAvpLength = iHeader + iDataLength;

	int rem = iDataLength % 4;

	if(rem > 0) {
		iPadded = 4 - rem;
	}

	dmAvp->HeaderLength = iHeader;
	dmAvp->AvpLength = iAvpLength;
	dmAvp->Padding = iPadded;
	dmAvp->PayLoadLength = iDataLength;	
}


void jet_diam_stack__calc_gavp_len( jet_diam_avp_t * dmAvp)
{
	int iLength = 0;
	int iHeader = ( dmAvp->VendorId > 0) ? 12 : 8;
	int iDataLength;
	int iPadded = 0;
	int iAvpLength;	
	
	jet_diam_avp_t * avp = dmAvp->avpHead;
	
	while( avp)
	{
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__calc_gavp_len( avp);
		}
		else
		{
			jet_diam_stack__calc_avp_len( avp);
		}
		iLength += (avp->AvpLength + avp->Padding);
		avp = avp->Next;
	}

	dmAvp->HeaderLength = iHeader;
	dmAvp->AvpLength = ( iHeader + iLength);
	dmAvp->Padding = 0;	
	dmAvp->PayLoadLength = iLength;	
}


void jet_diam_stack__diam_encode_gavp( jet_diam_avp_t * gavp, char * buffer);


void jet_diam_stack__diam_encode_avp( jet_diam_avp_t * avp, char * buffer)
{
	app_ep__encode__u32toc( buffer, avp->AvpCode);
	buffer[4] = avp->Flags;
	app_ep__encode__u24toc( &buffer[5], avp->AvpLength);
	
	if( avp->VendorId > 0) 
	{
		app_ep__encode__u32toc( &buffer[8], avp->VendorId);
	}
	
	if( avp->DataType == JET_DIAM_STACK__GROUPED)
	{
		jet_diam_stack__diam_encode_gavp( avp, buffer + avp->HeaderLength);
	}
	else
	{
		if( avp->PayLoadLength > 0 && avp->data)
		{
			memcpy( buffer + avp->HeaderLength, avp->data , avp->PayLoadLength);
		}
	}
}


void jet_diam_stack__diam_encode_gavp( jet_diam_avp_t * gavp, char * buffer)
{
	app_ep__encode__u32toc( buffer, gavp->AvpCode);
	buffer[4] = gavp->Flags;
	app_ep__encode__u24toc( &buffer[5], gavp->AvpLength);
	
	if( gavp->VendorId > 0) 
	{
		app_ep__encode__u32toc( &buffer[8], gavp->VendorId);
	}
	
	
	jet_diam_avp_t * avp = gavp->avpHead;
	
	int iCurrentIndex = gavp->HeaderLength;
	
	avp = gavp->avpHead;
	while(avp)
	{
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__diam_encode_gavp( avp, buffer + iCurrentIndex);
		}
		else
		{
			jet_diam_stack__diam_encode_avp( avp, buffer + iCurrentIndex);
		}

		iCurrentIndex += (avp->AvpLength + avp->Padding);
		avp = avp->Next;
	}
}


int jet_diam_stack__diam_encode( jet_diam_message_t * dMsg, char * buffer, int bLen)
{
	int iLength = 20;
	jet_diam_avp_t * avp = dMsg->avpHead;
	
	while( avp)
	{
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__calc_gavp_len( avp);
		}
		else
		{
			jet_diam_stack__calc_avp_len( avp);
		}
		
		iLength += (avp->AvpLength + avp->Padding);
		//printf( "AvpCode=%u AvpLength=%u Padding=%u\n", avp->AvpCode, avp->AvpLength, avp->Padding);
		
		avp = avp->Next;
	}
	
	if( iLength >= bLen)
	{
		printf("diam message encoding failed., buffer size insuffieient required=%d available=%d  %s|%s|%d\n", 
			iLength, bLen, __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	
	
	memset( buffer, 0, iLength);
	
	buffer[0] = 1;
	app_ep__encode__u24toc( &buffer[1], iLength);
	buffer[4] = dMsg->Flags;
	app_ep__encode__u24toc( &buffer[5], dMsg->CmdCode);
	app_ep__encode__u32toc( &buffer[8], dMsg->AppId);
	app_ep__encode__u32toc( &buffer[12], dMsg->HBHId);
	app_ep__encode__u32toc( &buffer[16], dMsg->E2EId);
	
	
	
	int iCurrentIndex = 20;


	avp = dMsg->avpHead;
	while(avp)
	{
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__diam_encode_gavp( avp, buffer + iCurrentIndex);
		}
		else
		{
			jet_diam_stack__diam_encode_avp( avp, buffer + iCurrentIndex);
		}

		iCurrentIndex += (avp->AvpLength + avp->Padding);
		avp = avp->Next;
	}

	
	return iLength;
}


int jet_diam_stack__peer_send_message( jet_diam_peer_client_t * client, jet_diam_message_t * dMsg)
{
	char buffer[2048];
	int dmLen = jet_diam_stack__diam_encode( dMsg, buffer, sizeof(buffer));

	int iSentBytes = 0;
	if( dmLen > 0)
	{
		iSentBytes = send( client->fd, buffer, dmLen, 0);	
	}
	
	//printf( "iSentBytes=%u  %s|%s|%d\n", iSentBytes, __FILE__, __FUNCTION__, __LINE__);
	return iSentBytes;
}


void  jet_diam_stack__free_diam_gavp( jet_diam_avp_t * gavp)
{
	jet_diam_avp_t * avp = gavp->avpHead;
	jet_diam_avp_t * avpNext = NULL;
	
	while(avp)
	{
		avpNext = avp->Next;

		if( avp->data)
		{
			app_region__free( (uint8_t*) avp->data);
		}
		
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__free_diam_gavp( avp);
			app_region__free( (uint8_t*) avp);
		}
		else
		{
			app_region__free( (uint8_t*) avp);
		}
		
		avp = avpNext;
	}
}


void jet_diam_stack__free_diam_messsage( jet_diam_message_t * dMsg)
{
	jet_diam_avp_t * avp = dMsg->avpHead;
	jet_diam_avp_t * avpNext = NULL;
	
	pthread_mutex_lock( &dMsg->AVPLock);
	
	while(avp)
	{
		avpNext = avp->Next;

		if( avp->data)
		{
			app_region__free( (uint8_t*) avp->data);
			avp->data = NULL;
		}
		
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__free_diam_gavp( avp);
			app_region__free( (uint8_t*) avp);
		}
		else
		{
			app_region__free( (uint8_t*) avp);
		}
		
		avp = avpNext;
	}
	pthread_mutex_unlock( &dMsg->AVPLock);
	
	pthread_mutex_destroy( &dMsg->AVPLock);
	app_region__free( (uint8_t*) dMsg);
}


void jet_diam_stack__add_origin_host( jet_diam_message_t * dMsg)
{
	jet_diam_stack__diam_message_add_str_avp( dMsg, 264, 64, 0, jet_diam_stack->HostName, strlen(jet_diam_stack->HostName));
}


void jet_diam_stack__add_origin_realm( jet_diam_message_t * dMsg)
{
	jet_diam_stack__diam_message_add_str_avp( dMsg, 296, 64, 0, jet_diam_stack->RealmName, strlen(jet_diam_stack->RealmName));
}

void jet_diam_stack__add_origin_state_id( jet_diam_message_t * dMsg)
{
	jet_diam_stack__diam_message_add_number_avp( dMsg, 278, 0x40, 0, jet_diam_stack->OriginStateId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
}

int jet_diam_stack__send_cer( jet_diam_peer_client_t * client)
{
	{
		jet_diam_dict_message_t * msg = jet_diam_stack__find_dict_message( 0, 257);
		

		jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
		
		if( dMsg)
		{
			
			
			jet_diam_stack__diam_message_init_request( dMsg, 0, 257, 0x80);
			jet_diam_dict_avp_t * ddAVP = msg->AVPHead;
			
			while( ddAVP)
			{
				switch( ddAVP->AVPCode)
				{
					case 257:
						{
							uint32_t hostip = app_ep__u32ip( client->host->IP);
							jet_diam_stack__diam_message_add_ipv4_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, hostip);
						}
						break;
					case 258:
						{
							int i = 0;
							for( i = 0; i < client->AuthAppCount; i++)
							{
								jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, client->AuthAppId[i], 4, JET_DIAM_STACK__UNSIG_NUMBER);
							}
						}
						break;
					case 260:
						{
							int i = 0;
							for( i = 0; i < client->VendAuthAppCount; i++)
							{
								jet_diam_avp_t * gAVP = jet_diam_stack__create_grouped_avp(  ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId);
								
								if( gAVP)
								{
									jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, 258, 0x40, 0, client->VendAuthAppId[i].AppId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
									jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, 266, 0x40, 0, client->VendAuthAppId[i].VendId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
									
									jet_diam_stack__diam_message_add_grouped_avp( dMsg, gAVP);
								}
							}
						}
						break;
					case 264:
						jet_diam_stack__diam_message_add_str_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->HostName, strlen(jet_diam_stack->HostName));
						break;
					case 266:
						jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, ddAVP->ufixedValue, 4, JET_DIAM_STACK__UNSIG_NUMBER);
						break;
					case 267:
						jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, ddAVP->ufixedValue, 4, JET_DIAM_STACK__UNSIG_NUMBER);
						break;
					case 269:
						jet_diam_stack__diam_message_add_str_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, ddAVP->cfixedValue, strlen(ddAVP->cfixedValue));
						break;
					case 278:
						jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->OriginStateId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
						break;	
					case 296:
						jet_diam_stack__diam_message_add_str_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->RealmName, strlen(jet_diam_stack->RealmName));
						break;				
					case 299:
						jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, ddAVP->ufixedValue, 4, JET_DIAM_STACK__UNSIG_NUMBER);
						break;
					default:
						break;
				}
				
				ddAVP = ddAVP->Next;
			}
			
			int iLen = jet_diam_stack__peer_send_message( client, dMsg);
			jet_diam_stack__free_diam_messsage( dMsg);
			return iLen;
		}
		
		return -1;
	}
	return 0;
}


int jet_diam_stack__send_watchdog( jet_diam_peer_client_t * client)
{
	{
		jet_diam_dict_message_t * msg = jet_diam_stack__find_dict_message( 0, 280);
		
		jet_diam_dict_avp_t * ddAVP = msg->AVPHead;
		jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
		
		if( dMsg)
		{
			jet_diam_stack__diam_message_init_request( dMsg, 0, 280, 0x80);
			jet_diam_dict_avp_t * ddAVP = msg->AVPHead;

			while( ddAVP)
			{
				switch( ddAVP->AVPCode)
				{
					case 264:
						jet_diam_stack__diam_message_add_str_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->HostName, strlen(jet_diam_stack->HostName));
						break;
					case 278:
						jet_diam_stack__diam_message_add_number_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->OriginStateId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
						break;
					case 296:
						jet_diam_stack__diam_message_add_str_avp( dMsg, ddAVP->AVPCode, ddAVP->Flags, ddAVP->VendorId, jet_diam_stack->RealmName, strlen(jet_diam_stack->RealmName));
						break;
					default:
						break;
				}
				
				ddAVP = ddAVP->Next;
			}
			
			int iLen = jet_diam_stack__peer_send_message( client, dMsg);
			jet_diam_stack__free_diam_messsage( dMsg);
			return iLen;
		}
		
		return -1;
	}
	return 0;
}

int jet_diam_stack__send_watchdog_answer( jet_diam_peer_client_t * client, jet_diam_message_t * diamRequest)
{
	jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);
	jet_diam_stack__diam_message_init_answer( diamRequest, dMsg, 0x00);
	
	jet_diam_stack__diam_message_add_number_avp( dMsg, 268, 0x40, 0, 2001, 4, JET_DIAM_STACK__UNSIG_NUMBER);
	jet_diam_stack__diam_message_add_str_avp( dMsg, 264, 0x40, 0, jet_diam_stack->HostName, strlen(jet_diam_stack->HostName));
	jet_diam_stack__diam_message_add_str_avp( dMsg, 296, 0x40, 0, jet_diam_stack->RealmName, strlen(jet_diam_stack->RealmName));
	jet_diam_stack__diam_message_add_number_avp( dMsg, 278, 0x40, 0, jet_diam_stack->OriginStateId, 4, JET_DIAM_STACK__UNSIG_NUMBER);
	
	int iLen = jet_diam_stack__peer_send_message( client, dMsg);
	jet_diam_stack__free_diam_messsage( dMsg);
	return iLen;
}

void jet_diam_stack__peer_client_close( jet_diam_peer_client_t * client)
{
	if(client->Connected == 1)
	{
		close( client->fd);
		shutdown( client->fd, 2);
		client->fd = -1;
		client->Connected = 0;
		client->CEASuccess = 0;
		
		app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Closing Connection for Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
	}
}




fp_diam_data_type diam_data_type_f;
void jet_diam_stack__set_data_type_handler( fp_diam_data_type diam_data_type_fc)
{
	diam_data_type_f = diam_data_type_fc;
}

void jet_diam_stack__getbit( uint8_t * bVal, char * _nwByte, int bitNumber)
{
	*bVal =  (_nwByte[0] & (1 << bitNumber)) ? 1 : 0;
}


uint8_t * jet_diam_stack__get_avp_data( jet_diam_avp_t * avp)
{
	return avp->data;
}


int jet_diam_stack__get_avp_data_len( jet_diam_avp_t * avp)
{
	return avp->PayLoadLength;
}

uint32_t jet_diam_stack__get_avp_u32( jet_diam_avp_t * avp)
{
	return app_ep__get_u32_ntoh( avp->data);
}

uint64_t jet_diam_stack__get_avp_u64( jet_diam_avp_t * avp)
{
	return app_ep__get_u64_ntoh( avp->data);
}

int32_t jet_diam_stack__get_avp_i32( jet_diam_avp_t * avp)
{
	return (int32_t)app_ep__get_u32_ntoh( avp->data);
}

int64_t jet_diam_stack__get_avp_i64( jet_diam_avp_t * avp)
{
	return (int64_t)app_ep__get_u64_ntoh( avp->data);
}


void jet_diam_stack__print_avp( jet_diam_avp_t * avp, int g)
{
	if( avp->DataType == JET_DIAM_STACK__GROUPED)
	{
		printf( "Grouped AvpCode=%u Len=%u\n", avp->AvpCode, avp->PayLoadLength);
	}
	else
	{
		printf( "%u  AvpCode=%u Len=%u ", g, avp->AvpCode, avp->PayLoadLength);
	
		if( avp->DataType == JET_DIAM_STACK__OCTET_STRING)
		{
			app_printf_char( "", avp->data, avp->PayLoadLength);
		}
		else
		{
			app_printf_buf( "", avp->data, avp->PayLoadLength);
			
			if( avp->PayLoadLength == 4)
			{
				uint32_t data = app_ep__get_u32_ntoh( avp->data);
				printf( " %u ", data);
			}
			else
			{
				uint64_t data = app_ep__get_u64_ntoh( avp->data);
				printf( " %lu ", data);
			}
		}
		
		printf("\n");
	}
}

void jet_diam_stack__print_gavp( jet_diam_avp_t * gavp)
{
	jet_diam_avp_t * avp = gavp->avpHead;
	
	while(avp)
	{
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__print_avp( avp, 1);
			jet_diam_stack__print_gavp( avp);
		}
		else
		{
			jet_diam_stack__print_avp( avp, 1);
		}
		avp = avp->Next;
	}	
}


void jet_diam_stack__print_msg( jet_diam_message_t * dMsg)
{
	printf( "MSG CmdCode=%u AppId=%u HBHId=%u E2EId=%u  dMsg->avpHead=%p\n", dMsg->CmdCode, dMsg->AppId, dMsg->HBHId, dMsg->E2EId, dMsg->avpHead);
	
	jet_diam_avp_t * avp = dMsg->avpHead;
	
	while(avp)
	{
		printf( "avp=%p\n", avp);
		
		if( avp->DataType == JET_DIAM_STACK__GROUPED)
		{
			jet_diam_stack__print_avp( avp, 1);
			jet_diam_stack__print_gavp( avp);
		}
		else
		{
			jet_diam_stack__print_avp( avp, 0);
		}		
		avp = avp->Next;
	}	
}


void jet_diam_stack__release2( jet_diam_buffer_t * diam_buffer, jet_diam_message_t * dMsg)
{
	if( diam_buffer->buffer)
	{
		app_region__free( (uint8_t*) diam_buffer->buffer);
	}
	diam_buffer->buffer = NULL;
	app_region__free( (uint8_t*) diam_buffer);
	jet_diam_stack__free_diam_messsage( dMsg);
}

void diam_app__peer_received_message( jet_diam_buffer_t * diam_buffer, jet_diam_message_t * dMsg);


jet_diam_avp_t * jet_diam_stack__get_diam_message_root_avp( jet_diam_message_t * dMsg)
{
	return dMsg->avpHead;
}

jet_diam_avp_t * jet_diam_stack__get_diam_avp_group_root_avp( jet_diam_avp_t * avp)
{
	return avp->avpHead;
}

jet_diam_avp_t * jet_diam_stack__get_diam_avp_next( jet_diam_avp_t * avp)
{
	return avp->Next;
}


jet_diam_peer_client_t * jet_diam_stack__get_client( jet_diam_buffer_t * diam_buffer)
{
	return (jet_diam_peer_client_t *)diam_buffer->ReceiverObj;
}

void jet_diam_stack__process_peer_received_message( jet_diam_buffer_t * diam_buffer, jet_diam_message_t * dMsg)
{
	//jet_diam_stack__print_msg( dMsg);
	jet_diam_peer_client_t * client = (jet_diam_peer_client_t *)diam_buffer->ReceiverObj;
	
	if(!client) 
	{
		app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Error:: Received NULL Client for Peer");
		return;
	}
	
	switch( dMsg->AppId)
	{
		case 0:
			{
				switch(dMsg->CmdCode)
				{
					case 257:
						{
							if( jet_diam_stack__is_request( dMsg) == 1)
							{
								app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Not Expected: Received CER for Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
							}
							else
							{
								//jet_diam_stack__print_msg( dMsg);
								
								jet_diam_avp_t * avp = dMsg->avpHead;
								
								uint32_t ResultCode = 0;
								
								while(avp)
								{
									if( avp->AvpCode == 268)
									{
										ResultCode = jet_diam_stack__get_avp_u32( avp);
										break;
									}
									avp = avp->Next;
								}
								
								if( ResultCode == 2001)
								{
									client->CEASuccess = 1;
									app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Success: Received CEA for Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
								}
								else
								{
									client->CEASuccess = 0;
									app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Error: Received CEA for Peer[%s:%d] SockFd[%d] with ResultCode=%u", client->IP, client->Port, client->fd, ResultCode);
								}
							}
							jet_diam_stack__release2( diam_buffer, dMsg);
						}
						break;
					case 280:
						{
							if( jet_diam_stack__is_request( dMsg) == 1)
							{
								app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Received WatchDogRequest for Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
								jet_diam_stack__send_watchdog_answer( client, dMsg);
							}
							else
							{
								client->WatchDogSent = 0;
								app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Received WatchDogAnswer for Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
							}
							jet_diam_stack__release2( diam_buffer, dMsg);
						}
						break;
					default:
						{
							 jet_diam_stack__release2( diam_buffer, dMsg);
						}
						break;
				}
			}
			break;
		default:
			{
				diam_app__peer_received_message( diam_buffer, dMsg);
			}
			break;
	}
}


uint8_t jet_diam_stack__is_request( jet_diam_message_t * dMsg)
{
	uint8_t IsReq = 0;
	jet_diam_stack__getbit( &IsReq, (char*)&dMsg->Flags, 7);
	return IsReq;
}

void jet_diam_stack__decode_diam_gavp_buffer( jet_diam_avp_t * gAVP, uint8_t * buffer, uint32_t length, uint32_t AppId, uint32_t CmdCode)
{
	int iCurrentByteIndex 	= 0;
	uint32_t DataType 		= 0;
	int iHeaderLength 		= 8;
	int iDataLength 		= 0;
	int iAvpTotLength 		= 0;
	int iAvpLength 			= 0;
	uint8_t Flags 			= 0;
	int iAvpCode 			= 0;
	uint8_t VendorSpecific 	= 0;
	uint8_t Mandatory 		= 0;
	int VendorId 			= 0;

	
	while( iCurrentByteIndex < length)
	{
		iHeaderLength = 8;

		iAvpCode 		= app_ep__get_u32( &buffer[ iCurrentByteIndex + 0]);
		Flags 			= buffer[ iCurrentByteIndex + 4 ];
		iAvpLength		= app_ep__get_u24( &buffer[ iCurrentByteIndex + 5]);
		
		jet_diam_stack__getbit( &Mandatory, (char*)&Flags, 6);
		jet_diam_stack__getbit( &VendorSpecific, (char*)&Flags, 7);
		
		//printf( "gAVP iAvpCode=%u Flags=%u iAvpLength=%u  Mandatory=%u  VendorSpecific=%u\n", iAvpCode, Flags, iAvpLength, Mandatory, VendorSpecific);
		

		if(VendorSpecific == 1)
		{
			iHeaderLength = 12;
		}
		
		iDataLength = iAvpLength - iHeaderLength;
		
		int rem = iAvpLength % 4;
		int iPadded = 0;

		if(rem > 0)
		{
			iPadded = 4 - rem;
		}

		iAvpTotLength = iAvpLength + iPadded;
		
		VendorId = 0;
		if(VendorSpecific == 1)
		{
			VendorId = app_ep__get_u32( &buffer[ iCurrentByteIndex + (iHeaderLength - 4)]);
		}
		
		DataType = diam_data_type_f( AppId, CmdCode, iAvpCode);
		
		switch( DataType)
		{
			case JET_DIAM_STACK__OCTET_STRING:
				{
					jet_diam_stack__diam_message_add_str_avp_to_avp( gAVP, iAvpCode, Flags, VendorId, &buffer[ iCurrentByteIndex + iHeaderLength], iDataLength);
				}
				break;
			case JET_DIAM_STACK__SIG_NUMBER:
				{
					if( iDataLength == 4) {
						jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, iAvpCode, Flags, VendorId, *(int32_t*)&buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__SIG_NUMBER);
					} else {
						jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, iAvpCode, Flags, VendorId, *(int64_t*)&buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__SIG_NUMBER);
					}
				}
				break;
			case JET_DIAM_STACK__UNSIG_NUMBER:
				{
					if( iDataLength == 4) {
						jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, iAvpCode, Flags, VendorId, *(uint32_t*)&buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__UNSIG_NUMBER);
					} else {
						jet_diam_stack__diam_message_add_number_avp_to_avp( gAVP, iAvpCode, Flags, VendorId, *(uint64_t*)&buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__UNSIG_NUMBER);
					}
				}
				break;
			case JET_DIAM_STACK__GROUPED:
				{
					jet_diam_avp_t * gAVPC = jet_diam_stack__create_grouped_avp( iAvpCode, Flags, VendorId);
					
					if( gAVPC)
					{
						jet_diam_stack__decode_diam_gavp_buffer( gAVPC, (uint8_t*)&buffer[ iCurrentByteIndex + iHeaderLength], iAvpTotLength - iHeaderLength, AppId, CmdCode);
						jet_diam_stack__diam_message_add_avp_to_avp( gAVP, gAVPC);
					}
				}
				break;
		}
			
		iCurrentByteIndex += iAvpTotLength;
	}
		
}

void jet_diam_stack__decode_diam_buffer( jet_diam_buffer_t * diam_buffer, int ix)
{
	if(!diam_data_type_f)
	{
		printf("data_type_handler not set\n");
		exit(0);
	}
	
	jet_diam_message_t * dMsg = (jet_diam_message_t *)app_region__allocate_fd( jet_diam_stack->diam_msg_dpool);

	pthread_mutex_init( &dMsg->AVPLock, NULL);
	dMsg->avpHead = NULL;
	dMsg->avpCurrent = NULL;
	dMsg->avpCount = 0;
	dMsg->avpCount2 = 0;
	dMsg->avpCount3 = 0;

	
	uint32_t Length 	= app_ep__get_u24( &diam_buffer->buffer[1]);
	
	dMsg->Flags = diam_buffer->buffer[4];
	
	dMsg->CmdCode		= app_ep__get_u24( &diam_buffer->buffer[5]);
	dMsg->AppId 		= app_ep__get_u32( &diam_buffer->buffer[8]);
	dMsg->HBHId			= app_ep__get_u32( &diam_buffer->buffer[12]);
	dMsg->E2EId			= app_ep__get_u32( &diam_buffer->buffer[16]);

	//printf( "CmdCode=%u AppId=%u HBHId=%u E2EId=%u Length=%u\n", dMsg->CmdCode, dMsg->AppId, dMsg->HBHId, dMsg->E2EId, Length);
	
	int iCurrentByteIndex 	= 20;
	uint32_t DataType 		= 0;
	int iHeaderLength 		= 8;
	int iDataLength 		= 0;
	int iAvpTotLength 		= 0;
	int iAvpLength 			= 0;
	uint8_t Flags 			= 0;
	int iAvpCode 			= 0;
	uint8_t VendorSpecific 	= 0;
	uint8_t Mandatory 		= 0;
	int VendorId 			= 0;

	
	while( iCurrentByteIndex < Length)
	{
		iHeaderLength = 8;

		iAvpCode 		= app_ep__get_u32( &diam_buffer->buffer[ iCurrentByteIndex + 0]);
		Flags 			= diam_buffer->buffer[ iCurrentByteIndex + 4 ];
		iAvpLength		= app_ep__get_u24( &diam_buffer->buffer[ iCurrentByteIndex + 5]);
		
		jet_diam_stack__getbit( &Mandatory, (char*)&Flags, 6);
		jet_diam_stack__getbit( &VendorSpecific, (char*)&Flags, 7);
		
		
		

		if(VendorSpecific == 1)
		{
			iHeaderLength = 12;
		}
		
		iDataLength = iAvpLength - iHeaderLength;
		
		int rem = iAvpLength % 4;
		int iPadded = 0;

		if(rem > 0)
		{
			iPadded = 4 - rem;
		}

		iAvpTotLength = iAvpLength + iPadded;
		
		VendorId = 0;
		if(VendorSpecific == 1)
		{
			VendorId = app_ep__get_u32( &diam_buffer->buffer[ iCurrentByteIndex + (iHeaderLength - 4)]);
		}
		
		DataType = diam_data_type_f( dMsg->AppId, dMsg->CmdCode, iAvpCode);
		
		// printf( "iAvpCode=%u Flags=%u iAvpLength=%u  Mandatory=%u  VendorSpecific=%u DataType=%u\n", 
			// iAvpCode, Flags, iAvpLength, Mandatory, VendorSpecific, DataType);

		
		switch( DataType)
		{
			case JET_DIAM_STACK__OCTET_STRING:
				{
					jet_diam_stack__diam_message_add_str_avp( dMsg, iAvpCode, Flags, VendorId, &diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength);
				}
				break;
			case JET_DIAM_STACK__SIG_NUMBER:
				{
					//printf( "iDataLength=%d  %d %d\n", iDataLength, *(int32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], ntohl(*(int32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength]));
					//app_printf_buf( " - ", (uint8_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength);
					
					if( iDataLength == 4) {
						jet_diam_stack__diam_message_add_number_avp( dMsg, iAvpCode, Flags, VendorId, *(int32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__SIG_NUMBER);
					} else {
						jet_diam_stack__diam_message_add_number_avp( dMsg, iAvpCode, Flags, VendorId, *(int64_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__SIG_NUMBER);
					}
				}
				break;
			case JET_DIAM_STACK__UNSIG_NUMBER:
				{
					//printf( "iDataLength=%d  %u %u\n", iDataLength, *(uint32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], ntohl(*(uint32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength]));
					//app_printf_buf( " - ", (uint8_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength);
					
					if( iDataLength == 4) {
						jet_diam_stack__diam_message_add_number_avp( dMsg, iAvpCode, Flags, VendorId, *(uint32_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__UNSIG_NUMBER);
					} else {
						jet_diam_stack__diam_message_add_number_avp( dMsg, iAvpCode, Flags, VendorId, *(uint64_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iDataLength, JET_DIAM_STACK__UNSIG_NUMBER);
					}
				}
				break;
			case JET_DIAM_STACK__GROUPED:
				{
					jet_diam_avp_t * gAVP = jet_diam_stack__create_grouped_avp( iAvpCode, Flags, VendorId);
					
					if( gAVP)
					{
						jet_diam_stack__decode_diam_gavp_buffer( gAVP, (uint8_t*)&diam_buffer->buffer[ iCurrentByteIndex + iHeaderLength], iAvpTotLength - iHeaderLength, dMsg->AppId, dMsg->CmdCode);
						jet_diam_stack__diam_message_add_grouped_avp( dMsg, gAVP);
					}
				}
				break;
		}
			
		iCurrentByteIndex += iAvpTotLength;
	}
	
	if( diam_buffer->Receiver == 1)
	{
		jet_diam_stack__process_peer_received_message( diam_buffer, dMsg);
	}	
}



void * jet_diam_stack__peer_client( void * args)
{
	jet_diam_peer_client_t * client = (jet_diam_peer_client_t *)args;

	int iSts 			= 0;
	int iIdleTime 		= 0;
	int iWaitTime 		= 3;
	int iRequestNo 		= 0;
	struct timeval tv;
	int selectFlag 		= 0;
	int iReceivedSize 	= 0;
	int iHeaderSize 	= 20;
	uint8_t * buffer	= NULL;
	int leftOverBytes 	= 0;
	int totalReceived 	= 0;
	uint32_t diamPayLoadLen 	= 0;

	
	while(1)
	{
		iSts = jet_diam_stack__peer_connect( client);
		
		if( iSts < 0)
		{
			int is = 0;
			
			for( is = 0; is < 30; is++) 
			{
				usleep( 999999);
			}	
		}
		
		if(client->Connected == 1)
		{
			iSts = jet_diam_stack__send_cer( client);

			if( iSts < 0 && errno == EPIPE)
			{
				app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Sending CER Failed For Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
				jet_diam_stack__peer_client_close( client);
			}
		}
		
		iIdleTime = 0;
		iWaitTime = 3;
		iRequestNo = 0;
		client->WatchDogSent = 0;

		while( client->Connected == 1)
		{
			fd_set rfds;
			selectFlag = 0;
			
			FD_ZERO( &rfds);
			FD_SET(  client->fd, &rfds);
			memset( (char *)&tv, 0, sizeof(tv));

			tv.tv_usec = iWaitTime;
			tv.tv_sec = iWaitTime;

			selectFlag = select( client->fd + 1, &rfds, NULL, NULL, &tv);

			if(selectFlag < 0 )
			{
				if( client->Connected == 0)
				{
					break;
				}
				continue;
			}
			
			if(selectFlag == 0)
			{
				if( client->Connected == 0)
				{
					break;
				}
				
				iIdleTime += iWaitTime;
				
				if( iIdleTime >= 30 )
				{
					if( client->CEASuccess == 0)
					{
						iSts = jet_diam_stack__send_cer( client);

						if( iSts < 0 && errno == EPIPE)
						{
							app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Sending CER Failed For Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
							jet_diam_stack__peer_client_close( client);
							continue;
						}
					}
					else
					{
						if( client->WatchDogSent >= 3)
						{
							app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "no response for watchdog, closing connection  Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
							jet_diam_stack__peer_client_close( client);
							continue;
						}
						
						iSts = jet_diam_stack__send_watchdog( client);

						if( iSts < 0 && errno == EPIPE)
						{
							app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "Sending watchdog Failed For Peer[%s:%d] SockFd[%d]", client->IP, client->Port, client->fd);
							jet_diam_stack__peer_client_close( client);
							continue;
						}
						client->WatchDogSent++;
					}
					
					iIdleTime = 0;
					
				}
			}
			
			
			if(selectFlag > 0)
			{
				if(FD_ISSET( client->fd, &rfds))
				{
					iReceivedSize 	= 0;
					iHeaderSize 	= 20;
					diamPayLoadLen 	= 0;
					
					buffer = app_region__allocate_fr( jet_diam_stack->dm_region, 2048);
					
					
					if( buffer)
					{
						totalReceived = 0;
						
						iReceivedSize = recv( client->fd, buffer, iHeaderSize, 0);
						totalReceived += iReceivedSize;


						if( iReceivedSize <= 0)
						{
							app_region__free( buffer);
							jet_diam_stack__peer_client_close( client);
							continue;
						}
						
						if( iReceivedSize < iHeaderSize)
						{
							leftOverBytes = iHeaderSize - iReceivedSize;
							iReceivedSize = recv( client->fd, &buffer[iReceivedSize], leftOverBytes, 0);
							
							
							if( iReceivedSize <= 0)
							{
								app_region__free( buffer);
								jet_diam_stack__peer_client_close( client);
								continue;
							}

							if( iReceivedSize < leftOverBytes)
							{
								app_region__free( buffer);
								jet_diam_stack__peer_client_close( client);
								continue;
							}
							
							totalReceived += iReceivedSize;
						}
						
						diamPayLoadLen = app_ep__get_u24( &buffer[1]);
						
						if( diamPayLoadLen > 0 && diamPayLoadLen < 2048)
						{
							while( diamPayLoadLen > totalReceived) 
							{
								iReceivedSize = recv( client->fd, &buffer[totalReceived], diamPayLoadLen - totalReceived, 0);
								
								if( iReceivedSize <= 0)
								{
									app_region__free( buffer);
									jet_diam_stack__peer_client_close( client);
									continue;
								}
								
								totalReceived += iReceivedSize;
							}
							
							iRequestNo++;
							
							jet_diam_buffer_t * diam_buff = (jet_diam_buffer_t *)app_region__allocate_fr( jet_diam_stack->dm_region, sizeof(jet_diam_buffer_t));
							
							if(!diam_buff)
							{
								app_region__free( buffer);
								jet_diam_stack__peer_client_close( client);
								continue;
							}
							else
							{
								client->WatchDogSent	= 0;
								diam_buff->buffer 		= buffer;
								diam_buff->Length		= totalReceived;
								diam_buff->RequestNo	= iRequestNo;
								diam_buff->Receiver		= 1;
								diam_buff->ReceiverObj	= (uint8_t*)client;
								
								app_queue__enquee( jet_diam_stack->appQueue, (uint8_t *)diam_buff, (fp_queue_call_back)jet_diam_stack__decode_diam_buffer);

								iIdleTime = 0;
							}
							
						}
						else
						{
							printf("Invalid Buffer Length=%u\n", diamPayLoadLen);
							exit(0);
						}

					}
					else
					{
						jet_diam_stack__peer_client_close( client);
						continue;
					}
				}
			}

			
		}
	}
	
	return NULL;
}


void jet_diam_stack__start_peer_client( jet_diam_peer_client_t * client)
{
	int iRet;
	pthread_t s_pthread_id;

	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);

	if( client->TransportType == 1)
	{
		iRet = pthread_create( &s_pthread_id, &attr, jet_diam_stack__peer_client, (void *)client);

		if(iRet)
		{
			perror("Error: ");
			printf("unable to create Thread for New Connection \n");
			exit(-1);
		}
		
	}
}


app_data_pool_t * jet_diam_stack__avp_pool( char * name, size_t sz, int count)
{
	return app_region__add_pool( jet_diam_stack->dm_region, name, sizeof(jet_diam_avp_t) + sz, count);
}


char * jet_diam_stack__set_hostname()
{
	return jet_diam_stack->HostName;
}


char * jet_diam_stack__set_hostrealm()
{
	return jet_diam_stack->RealmName;
}


void jet_diam_stack__set_host( char * hostName, char * realmName)
{
	strcpy( jet_diam_stack->HostName, hostName);
	strcpy( jet_diam_stack->RealmName, realmName);
}


char * jet_diam_stack__get_hostip( jet_diam_host_t * host)
{
	return host->IP;
}


int jet_diam_stack__get_hostport( jet_diam_host_t * host)
{
	return host->Port;
}


jet_diam_host_t * jet_diam_stack__add_host( unsigned char * IP, uint32_t Port, uint16_t TransportType)
{
	if( jet_diam_stack->host_count < 10)
	{
		jet_diam_host_t * host = &jet_diam_stack->host[ jet_diam_stack->host_count];
		
		strcpy( host->IP, IP);

		host->Port 			= Port;
		host->TransportType = TransportType;
		
		jet_diam_stack->host_count++;
		return host;
	}
	return NULL;
}


void jet_diam_stack__add_auth_app_id( jet_diam_host_t * host, uint32_t appId)
{
	if( host->AuthAppCount < 10)
	{
		host->AuthAppId[ host->AuthAppCount] = appId;
		host->AuthAppCount++;
	}
}

void jet_diam_stack__add_vend_auth_app_id( jet_diam_host_t * host, uint32_t appId, uint32_t vendId)
{
	if( host->VendAuthAppCount < 10)
	{
		host->VendAuthAppId[ host->VendAuthAppCount].VendId = vendId;
		host->VendAuthAppId[ host->VendAuthAppCount].AppId = appId;
		host->VendAuthAppCount++;
	}
}

void jet_diam_stack__add_acct_app_id( jet_diam_host_t * host, uint32_t appId)
{
	if( host->AcctAppCount < 10)
	{
		host->AcctAppId[ host->AcctAppCount] = appId;
		host->AcctAppCount++;
	}
}

void jet_diam_stack__add_vend_acct_app_id( jet_diam_host_t * host, uint32_t appId, uint32_t vendId)
{
	if( host->VendAcctAppCount < 10)
	{
		host->VendAcctAppId[ host->VendAcctAppCount].VendId = vendId;
		host->VendAcctAppId[ host->VendAcctAppCount].AppId = appId;
		host->VendAcctAppCount++;
	}
}

void jet_diam_stack__diam_dict_avp( jet_diam_dict_avp_t * diam_dict_avp, json_t * jDDAVP)
{
	diam_dict_avp->Next = NULL;
	diam_dict_avp->AVPCode = 0;
	diam_dict_avp->DataType = 0;
	diam_dict_avp->VendorId = 0;
	diam_dict_avp->Flags = 0;
	diam_dict_avp->cfixedValue = NULL;
	diam_dict_avp->ufixedValue = 0;
	diam_dict_avp->AVPHead = NULL;
	diam_dict_avp->AVPCurrent = NULL;
	diam_dict_avp->AVPCount = 0;
	
	diam_dict_avp->AVPCode 	= jet_diam__get_int( jDDAVP, "Code", 0);
	diam_dict_avp->DataType = jet_diam__get_int( jDDAVP, "Type", 0);
	diam_dict_avp->VendorId = jet_diam__get_int( jDDAVP, "VendorId", 0);
	diam_dict_avp->Flags 	= jet_diam__get_int( jDDAVP, "Flags", 0);	
	
	json_t * fV = json_object_get( jDDAVP, "FixedValue");
	
	if( fV)
	{
		if( diam_dict_avp->DataType == 1)
		{
			diam_dict_avp->cfixedValue = (char*)json_string_value( fV);
		}
		else if( diam_dict_avp->DataType > 1)
		{
			diam_dict_avp->ufixedValue = json_integer_value( fV);
		}
	}
	
	json_t * jDDAVPs = NULL;
	json_t * cjDDAVP = NULL;
	
	int avp_count = 0;
	int avp_i = 0;
	jet_diam_dict_avp_t * child_diam_dict_avp = NULL;
	
	jDDAVPs = json_object_get( jDDAVP, "AVPs");
						
	if( jDDAVPs)
	{
		if(json_is_array( jDDAVPs))
		{
			avp_count = json_array_size( jDDAVPs);
			
			for( avp_i = 0; avp_i < avp_count; avp_i++)
			{
				cjDDAVP = json_array_get( jDDAVPs, avp_i);
				
				child_diam_dict_avp = (jet_diam_dict_avp_t *)app_region__allocate_fr( jet_diam_stack->dm_region, sizeof( jet_diam_dict_avp_t));
			
				if( child_diam_dict_avp)
				{
					jet_diam_stack__diam_dict_avp( child_diam_dict_avp, cjDDAVP);
					
					if(!diam_dict_avp->AVPHead)
					{
						diam_dict_avp->AVPHead = diam_dict_avp->AVPCurrent = child_diam_dict_avp;
					}
					else
					{
						diam_dict_avp->AVPCurrent->Next = child_diam_dict_avp;
						diam_dict_avp->AVPCurrent = child_diam_dict_avp;
					}
					diam_dict_avp->AVPCount++;
				}
			}
		}
	}		
}

int jet_diam_stack__random( int min, int max)
{
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void jet_diam_stack__sigpipe( int signum)
{
}


void jet_diam_stack__log_region_info()
{
	app_region__print_stats( jet_diam_stack->plogger, jet_diam_stack->dm_region);
	app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "-----------------------------------------------------");
}

void jet_diam_stack__qd( uint8_t * Data, int tIndex){}

void jet_diam_stack__init( char * filepath)
{
	if(!jet_diam_stack)
	{
		signal( SIGPIPE, jet_diam_stack__sigpipe);
		
		jet_diam_stack = malloc(sizeof( jet_diam_stack_t));
		memset( jet_diam_stack, 0, sizeof(jet_diam_stack_t));
		
		jet_diam_stack->host_count = 0;
		jet_diam_stack->plogger = app_logger__create( "./diam_stack/", "DIAM_", ".log", 10 * (1024 * 1024), 5);
		
		//printf( "jet_diam_message_t=%ld %ld\n", sizeof(jet_diam_message_t) , sizeof(jet_diam_dict_message_t));
		
		jet_diam_stack->dm_region = app_region__create();
		
		jet_diam_stack__avp_pool( "b4", 	4, 		100000);
		jet_diam_stack__avp_pool( "b8", 	8, 		500000);
		jet_diam_stack__avp_pool( "b25", 	25, 	100000);
		jet_diam_stack__avp_pool( "bdm", 	sizeof(jet_diam_dict_message_t), 	500);
		jet_diam_stack__avp_pool( "b50", 	50, 	50000);
		jet_diam_stack->diam_msg_dpool = jet_diam_stack__avp_pool( "bdm2", 	sizeof(jet_diam_message_t), 	 20000);		
		jet_diam_stack->diam_avp_dpool = jet_diam_stack__avp_pool( "bdavp", sizeof(jet_diam_avp_t), 		100000);
		jet_diam_stack__avp_pool( "bda", 	sizeof(jet_diam_dict_avp_t), 	5000);
		jet_diam_stack__avp_pool( "b100",	100, 	50000);
		jet_diam_stack__avp_pool( "b2048",	2048, 	10000);
		
		app_logger__log( jet_diam_stack->plogger, NULL, APP_LOG__LEVEL_CRITICAL, "initialized diameter stack");
		
		srand(time(NULL)); 
		jet_diam_stack->OriginStateId = jet_diam_stack__random( 100, 10000);
		//printf( "OriginStateId=%u\n", jet_diam_stack->OriginStateId);


		jet_diam_stack->DDHead 		= NULL;
		jet_diam_stack->DDCurrent 	= NULL;
		jet_diam_stack->DDCount 	= 0;
		jet_diam_stack->appQueue	= app_queue__create( "dq", 10000, 20, jet_diam_stack__qd);
		
		json_error_t error;
		memset( &error, 0, sizeof( json_error_t));
		json_t * diam_dict = json_load_file( filepath, 0, &error);

		json_t * jDDMI = NULL;
		json_t * jDDAVPs = NULL;
		json_t * jDDAVP = NULL;
		
		int avp_count = 0;
		int avp_i = 0;
		
		jet_diam_dict_message_t * diam_dict_message = NULL;
		jet_diam_dict_avp_t * diam_dict_avp = NULL;
		
		if( diam_dict)
		{
			if( json_is_array(diam_dict))
			{
				int dd_count = json_array_size( diam_dict);
				int i = 0;
				
				for( i = 0; i < dd_count; i++)
				{
					jDDMI = json_array_get( diam_dict, i);
					diam_dict_message = (jet_diam_dict_message_t *)app_region__allocate_fr( jet_diam_stack->dm_region, sizeof( jet_diam_dict_message_t));
					
					if( jDDMI && diam_dict_message)
					{
						diam_dict_message->Next = NULL;
						diam_dict_message->ApplicationId = 0;
						diam_dict_message->CommandCode = 0;
						diam_dict_message->Flag = 0;
						diam_dict_message->AVPHead = NULL;
						diam_dict_message->AVPCurrent = NULL;
						diam_dict_message->AVPCount = 0;
						
						diam_dict_message->ApplicationId = jet_diam__get_int( jDDMI, "ApplicationId", 0);
						diam_dict_message->CommandCode = jet_diam__get_int( jDDMI, "CommandCode", 0);
						
						jDDAVPs = json_object_get( jDDMI, "AVPs");
						
						if( jDDAVPs)
						{
							if(json_is_array( jDDAVPs))
							{
								avp_count = json_array_size( jDDAVPs);
								
								for( avp_i = 0; avp_i < avp_count; avp_i++)
								{
									jDDAVP = json_array_get( jDDAVPs, avp_i);
									
									diam_dict_avp = (jet_diam_dict_avp_t *)app_region__allocate_fr( jet_diam_stack->dm_region, sizeof( jet_diam_dict_avp_t));
								
									if( diam_dict_avp)
									{
										jet_diam_stack__diam_dict_avp( diam_dict_avp, jDDAVP);
										
										if(!diam_dict_message->AVPHead)
										{
											diam_dict_message->AVPHead = diam_dict_message->AVPCurrent = diam_dict_avp;
										}
										else
										{
											diam_dict_message->AVPCurrent->Next = diam_dict_avp;
											diam_dict_message->AVPCurrent = diam_dict_avp;
										}
										
										diam_dict_message->AVPCount++;
									}	
								}
							}
						}
						
						if(!jet_diam_stack->DDHead)
						{
							jet_diam_stack->DDHead = jet_diam_stack->DDCurrent = diam_dict_message;
						}
						else
						{
							jet_diam_stack->DDCurrent->Next = diam_dict_message;
							jet_diam_stack->DDCurrent = diam_dict_message;
						}
						
						jet_diam_stack->DDCount++;
					}
				}
			}
		}
	}
}