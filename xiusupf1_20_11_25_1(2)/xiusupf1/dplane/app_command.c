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

#include "app_endpoint.h"
#include "pfcp_stack.h"
#include "app_command.h"

typedef struct app_cmd_stack
{
	app_ep_udp_server_t * udp_ipv4server;
	app_ep_udp_server_t * udp_ipv6server;
} app_cmd_stack_t;

app_cmd_stack_t * cmd_stack = NULL;


void app_cmd__buffer_handler( uint8_t * data, int tIndex)
{
	app_ep_udp_message_t * msg 	= (app_ep_udp_message_t *)data;
	uint16_t len 				= app_ep__get_len( msg);
	uint8_t * buffer 			= app_ep__get_buffer( msg);

	//pfcp_stack__print_buffer_wn( "cmd-buffer", buffer, len);
	
	uint32_t cmd = htonl(app_ep__get_u32( buffer));
	//printf( "cmd=%u|%d  %s|%d\n", cmd, htonl(cmd),__FILE__, __LINE__);
	
	switch( cmd)
	{
		case 1:
			{
				app_cmd_usage_t cmd;
				memcpy( &cmd, buffer, sizeof(app_cmd_usage_t));
				
				//printf( "up_f_seid=%lu rgid=%u total=%u uplink=%u downlink=%u\n", cmd.cp_seid, cmd.rgid, cmd.total, cmd.uplink, cmd.downlink);
				pfcp_stack___record_usage( cmd.cp_seid, cmd.rgid, cmd.uplink, cmd.downlink);
			}
			break;
		default:
			break;
	}
	
	app_ep__free_udp_message( msg);	
}


void app_cmd__init( int ipv, char * ip, int port, int wThreads)
{
	cmd_stack = (app_cmd_stack_t *)malloc( sizeof(app_cmd_stack_t));
	memset( cmd_stack, 0, sizeof(app_cmd_stack_t));
	
	if( ipv == 4) {
		cmd_stack->udp_ipv4server = app_ep__create_udpv4_server( ip, port, 1500, wThreads, app_cmd__buffer_handler);
		app_ep__udp_server_start( cmd_stack->udp_ipv4server);
	} else if( ipv == 6) {
		cmd_stack->udp_ipv6server = app_ep__create_udpv4_server( ip, port, 1500, wThreads, app_cmd__buffer_handler);
		app_ep__udp_server_start( cmd_stack->udp_ipv6server);
	}
	
	printf("udp_ipv4server=%p ipv=%d ip=%s port=%d  %s|%d\n", cmd_stack->udp_ipv4server, ipv, ip, port, __FILE__, __LINE__ );
}


void app_cmd__send_usage( int fd, char * ip, int port, uint64_t up_f_seid, uint32_t rgid, uint32_t uplink, uint32_t downlink)
{
	// printf( "fd=%d, ip=%s, port=%d, up_f_seid=%lu, total=%u, uplink=%u, downlink=%u\n", 
		// fd, ip, port, up_f_seid, total, uplink, downlink);
		
	app_cmd_usage_t c_usage;
	memset( &c_usage, 0, sizeof(app_cmd_usage_t));
	
	c_usage.cmd = 1;
	c_usage.cp_seid = up_f_seid;
	c_usage.rgid = rgid;
	c_usage.uplink = uplink;
	c_usage.downlink = downlink;
	
	app_ep__udp_sendto_v4( fd, ip, port, (char *)&c_usage, sizeof(app_cmd_usage_t));
}












