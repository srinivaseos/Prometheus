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
#include <ctype.h>

#include "dhcp.h"

void __dhcp__print_buffer( char * name, unsigned char * buffer, int len)
{
	printf("%s: ", name);
	
	int i = 0;
	for( i = 0; i < len; i++) {
		printf("%02X ", buffer[i] & 0xFF);
	}
	
	
	
	printf("\n");
}

void __dhcp__print_ipv4( char * name, struct in_addr * ciaddr)
{
	uint32_t uIP = *(uint32_t*)ciaddr;
	
	printf("%-30s: - %u   %u.%u.%u.%u   %02X %02X %02X %02X \n", 
			name, uIP, 
			uIP & 0xFF, (uIP >> 8) & 0xFF, (uIP >> 16) & 0xFF,(uIP >> 24) & 0xFF, 
			uIP & 0xFF, (uIP >> 8) & 0xFF,(uIP >> 16) & 0xFF, (uIP >> 24) & 0xFF
	);
}



int __dhcp__print_packet( struct dhcp_packet * dpacket, int len)
{
	printf( "opcode/type           = %u\n", dpacket->op );
	printf( "Hardware addr type    = %u\n", dpacket->htype );
	printf( "Hardware addr length  = %u\n", dpacket->hlen );
	printf( "Hops                  = %u\n", dpacket->hops );
	printf( "Transaction ID        = %u\n", dpacket->xid );	
	printf( "Seconds               = %u\n", dpacket->secs );
	printf( "Flags                 = %u\n", dpacket->flags);
	__dhcp__print_ipv4( "Client IP address", &dpacket->ciaddr);
	__dhcp__print_ipv4( "Your (client) IP address", &dpacket->yiaddr);
	__dhcp__print_ipv4( "Next Server IP address", &dpacket->siaddr);
	__dhcp__print_ipv4( "Relay agent IP address", &dpacket->giaddr);
	printf( "Client MAC address    = %02X:%02X:%02X:%02X:%02X:%02X\n", dpacket->chaddr[0], 
		dpacket->chaddr[1], dpacket->chaddr[2], 
		dpacket->chaddr[3], dpacket->chaddr[4], dpacket->chaddr[5]);


	printf( "Magic cookie    = %02X %02X %02X %02X \n", 
			dpacket->options[0], dpacket->options[1], dpacket->options[2], dpacket->options[3]
		);
		
	
	
	uint8_t	option = 0;

	int optpos = 4;
	int pos = (236 + optpos);
	uint8_t ilen = 0;
	
	while( dpacket->options[optpos] != 255 && pos < len)
	{
		option 	= dpacket->options[optpos];
		ilen 	= dpacket->options[optpos + 1];
		
		//printf("    opt=%u  len=%u  optpos=%u pos=%u len=%u\n", option, ilen, optpos, pos, len);
		
		__dhcp__print_buffer( "option", &dpacket->options[optpos + 2], ilen);
		
		optpos 	+= (ilen + 2);
		pos 	+= (ilen + 2);
	}

	printf("    opt=%u\n", dpacket->options[optpos]);
		
	return 1;
}

uint8_t __dhcp__get_message_type( struct dhcp_packet * dpacket, int len)
{
	uint8_t	option = 0;

	int optpos = 4;
	int pos = (236 + optpos);
	uint8_t ilen = 0;

	while( dpacket->options[optpos] != 255 && pos < len)
	{
		option 	= dpacket->options[optpos];
		ilen 	= dpacket->options[optpos + 1];
		
		if( option == 53)
		{
			return dpacket->options[optpos + 2];
		}
	
		optpos 	+= (ilen + 2);
		pos 	+= (ilen + 2);		
	}
	
	return 0;
}


int __dhcp__decode_packet( unsigned char * packet, int len)
{
	printf("stru-len=%ld data-len=%d -  %s|%s|%d\n", sizeof(struct dhcp_packet), len, __FILE__, __FUNCTION__, __LINE__);
	
	if( len > 236)
	{
		struct dhcp_packet * dpacket = (struct dhcp_packet *)packet;
		
		__dhcp__print_packet( dpacket, len);
		
	}
	else
	{
		return -1;
	}
	
	return 0;
}


int __dhcp__add_option( unsigned char * option, uint8_t itype, uint8_t length, int prefixlen, unsigned char * prefix, unsigned char * val)
{
	option[0] = itype;
	option[1] = ( prefixlen + length);
	
	if( prefixlen > 0)
	{
		memcpy( &option[2], prefix, prefixlen);
		memcpy( &option[2 + prefixlen], val, length);
		return (2 + prefixlen+ length);
	}
	else
	{
		memcpy( &option[2], val, length);
		return (2 + length);
	}
}	

int __dhcp__encode_inform_packet( unsigned char * packet, int * len, char * mac, int ipv4, char * hostName, int hostNameLength, uint32_t xid)
{
	struct dhcp_packet * dpacket = (struct dhcp_packet *)packet;
	
	dpacket->op						= BOOTREQUEST;
	dpacket->htype					= HTYPE_ETHER;
	dpacket->hlen 					= 6;
	dpacket->hops 					= 0;
	dpacket->xid 					= xid;
	dpacket->secs					= 0;
	dpacket->flags					= 0;
	*(uint32_t*)&dpacket->ciaddr	= ipv4;
	*(uint32_t*)&dpacket->yiaddr	= 0;
	*(uint32_t*)&dpacket->siaddr	= 0;
	*(uint32_t*)&dpacket->giaddr	= 0;	
	memcpy( dpacket->chaddr, mac, 6);
	
	dpacket->options[0] 			= 0x63;
	dpacket->options[1] 			= 0x82;
	dpacket->options[2] 			= 0x53;
	dpacket->options[3] 			= 0x63;
	
	int optpos = 4;
	unsigned char inform[1] 		= {0x08};
	unsigned char hwType[1] 		= {0x01};
	unsigned char prl[13]	 		= {01,15,03,06,44,46,47,31,33,121,249,43,14};
	
	optpos += __dhcp__add_option( &dpacket->options[optpos], 53, 1, 0, NULL, 	inform);
	optpos += __dhcp__add_option( &dpacket->options[optpos], 61, 6, 1, hwType, 	mac);
	optpos += __dhcp__add_option( &dpacket->options[optpos], 12, hostNameLength, 0, NULL, 	hostName);
	optpos += __dhcp__add_option( &dpacket->options[optpos], 60, 8, 0, NULL, 	"MSFT 5.0");
	optpos += __dhcp__add_option( &dpacket->options[optpos], 55, 13, 0, NULL, 	prl);	
	dpacket->options[optpos] = 0xFF;
	
	
	//printf("ecoded packet -------------------------------\n");
	//__dhcp__decode_packet( packet, 500);
	
	return (236 + 4 + optpos + 20);
}


int __dhcp__encode_offer_packet( unsigned char * packet, int * len, char * clientmac, uint32_t dchp_server_ipv4, uint32_t allocated_ipv4, char * hostName, int hostNameLength, uint32_t xid, int rt)
{
	struct dhcp_packet * dpacket = (struct dhcp_packet *)packet;
	
	dpacket->op						= BOOTREPLY;
	dpacket->htype					= HTYPE_ETHER;
	dpacket->hlen 					= 6;
	dpacket->hops 					= 0;
	dpacket->xid 					= xid;
	dpacket->secs					= 0;
	dpacket->flags					= 0;
	*(uint32_t*)&dpacket->ciaddr	= 0;
	*(uint32_t*)&dpacket->yiaddr	= htonl(allocated_ipv4);
	*(uint32_t*)&dpacket->siaddr	= dchp_server_ipv4;
	*(uint32_t*)&dpacket->giaddr	= 0;	
	memcpy( dpacket->chaddr, clientmac, 6);
	
	dpacket->options[0] 			= 0x63;
	dpacket->options[1] 			= 0x82;
	dpacket->options[2] 			= 0x53;
	dpacket->options[3] 			= 0x63;	
	
	
	unsigned char offer[1] 		= {0x02};
	unsigned char ack[1] 		= {0x05};
	
	int optpos = 4;
	
	if( rt == 2)
	{
		optpos += __dhcp__add_option( &dpacket->options[optpos], 53, 1, 0, NULL, 	offer);
	}
	else if( rt == 5)
	{
		optpos += __dhcp__add_option( &dpacket->options[optpos], 53, 1, 0, NULL, 	ack);
	}

	optpos += __dhcp__add_option( &dpacket->options[optpos], 54, 4, 0, NULL, (char*)&dchp_server_ipv4);
	//optpos += __dhcp__add_option( &dpacket->options[optpos], 51, 4, 0, NULL, (char*)"\x00\x01\x51\x80");
	optpos += __dhcp__add_option( &dpacket->options[optpos], 51, 4, 0, NULL, (char*)"\x00\x00\x01\x2C");
	optpos += __dhcp__add_option( &dpacket->options[optpos], 1, 4, 0, NULL, (char*)"\xFF\xFF\xFF\x00");
	optpos += __dhcp__add_option( &dpacket->options[optpos], 3, 4, 0, NULL, (char*)&dchp_server_ipv4);
	optpos += __dhcp__add_option( &dpacket->options[optpos], 6, 8, 0, NULL, (char*)"\x04\x04\x04\x04\x08\x08\x08\x08");
	optpos += __dhcp__add_option( &dpacket->options[optpos], 15, 8, 0, NULL, (char*)"xius.com");
	
	dpacket->options[optpos] = 0xFF;
	
	*len = (236 + 4 + optpos + 20);
	return (236 + 4 + optpos + 20);
}