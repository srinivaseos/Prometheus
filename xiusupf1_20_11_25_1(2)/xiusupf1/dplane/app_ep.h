#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef S_APP_ENDPOINT_DEF
#define S_APP_ENDPOINT_DEF

typedef struct app_ep_udp_message
{
	uint8_t * buffer;
	int len;
	int pos;
} app_ep_udp_message_t;


#endif